#!/usr/bin/env python3
"""Unit tests for tools/build_trust_floor.py (offline trust-floor derivation)."""

from __future__ import annotations

import base64
import contextlib
import io
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import build_trust_floor  # noqa: E402

# Hand-decoded values of the two committed public keys: the DER
# SubjectPublicKeyInfo of an uncompressed P-256 point and its X||Y halves.
P256_SPKI_PREFIX = bytes.fromhex("3059301306072a8648ce3d020106082a8648ce3d030107034200")
TEST_XY = bytes.fromhex(
    "d7027588352422870b83a9a154ba0aaf105b8b5c191bf5a138562b369428"
    "8676e11271cf055e03e452a45448e825ec125bec1eea94fc06ae6e4d996ec6fac4d7"
)
TEST_DER = P256_SPKI_PREFIX + b"\x04" + TEST_XY
TEST_FINGERPRINT = "ca7795786820dd5d3d669fb3321b3e11bdafc1cce7d39c4fd59f52e7ef110aea"
PROD_XY = bytes.fromhex(
    "b3cae3d4a15f2e81719b9373b1b37a7050d829f563354868f5b98063bd98608"
    "ce32d037297fbf79e94ee9e2b78b6be25ca692d234382ac731f70ea065e397de0"
)
PROD_DER = P256_SPKI_PREFIX + b"\x04" + PROD_XY
PROD_FINGERPRINT = "58bb4787245691066105914d20ff3da5da2cb8677b13e943a40f6a73bcc277a8"

TEST_DOCUMENT = (
    "{\n"
    '  "format": "axlic-trust-floor",\n'
    '  "version": 1,\n'
    '  "keys": [\n'
    "    {\n"
    '      "key_id": "license-test",\n'
    '      "public_key_xy_hex": "' + TEST_XY.hex() + '"\n'
    "    }\n"
    "  ]\n"
    "}\n"
)


def pem(der: bytes, newline: str = "\n") -> str:
    body = base64.b64encode(der).decode("ascii")
    lines = [body[index:index + 64] for index in range(0, len(body), 64)]
    return newline.join(["-----BEGIN PUBLIC KEY-----", *lines, "-----END PUBLIC KEY-----"]) + newline


class ParseSpkiPemTests(unittest.TestCase):
    def test_parse_spki_pem_accepts_crlf_bom_and_trailing_blank_lines(self):
        text = "\ufeff" + pem(TEST_DER, "\r\n") + "\r\n\r\n  \r\n"
        self.assertEqual(build_trust_floor.parse_spki_pem(text), TEST_DER)

    def test_parse_spki_pem_rejects_two_blocks(self):
        with self.assertRaises(build_trust_floor.TrustFloorError):
            build_trust_floor.parse_spki_pem(pem(TEST_DER) + pem(PROD_DER))

    def test_parse_spki_pem_rejects_text_outside_the_block(self):
        private_block = "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n"
        with self.assertRaises(build_trust_floor.TrustFloorError):
            build_trust_floor.parse_spki_pem(private_block + pem(TEST_DER))
        with self.assertRaises(build_trust_floor.TrustFloorError):
            build_trust_floor.parse_spki_pem(pem(TEST_DER) + "trailing prose\n")

    def test_parse_spki_pem_rejects_non_p256_or_compressed_point(self):
        wrong_prefix = P256_SPKI_PREFIX[:-1] + b"\xff"
        cases = {
            "wrong-algorithm-prefix": wrong_prefix + b"\x04" + TEST_XY,
            "compressed-marker": P256_SPKI_PREFIX + b"\x03" + TEST_XY,
            "truncated-point": P256_SPKI_PREFIX + b"\x04" + TEST_XY[:40],
        }
        for name, der in cases.items():
            with self.subTest(case=name), self.assertRaises(build_trust_floor.TrustFloorError):
                build_trust_floor.parse_spki_pem(pem(der))


class LoadKeysTests(unittest.TestCase):
    def test_committed_keys_match_the_pinned_points(self):
        keyring = build_trust_floor.KEYRING_DIR
        test_der = build_trust_floor.parse_spki_pem((keyring / "license-test.pub.pem").read_text(encoding="utf-8"))
        prod_der = build_trust_floor.parse_spki_pem((keyring / "license-prod.pub.pem").read_text(encoding="utf-8"))
        self.assertEqual(build_trust_floor.public_key_xy_hex(test_der), TEST_XY.hex())
        self.assertEqual(build_trust_floor.public_key_xy_hex(prod_der), PROD_XY.hex())

    def test_load_environment_keys_reads_both_committed_environments(self):
        self.assertEqual(
            build_trust_floor.load_environment_keys(build_trust_floor.KEYRING_DIR, "test"),
            [{"key_id": "license-test", "public_key_xy_hex": TEST_XY.hex()}],
        )
        self.assertEqual(
            build_trust_floor.load_environment_keys(build_trust_floor.KEYRING_DIR, "prod"),
            [{"key_id": "license-prod", "public_key_xy_hex": PROD_XY.hex()}],
        )

    def test_load_environment_keys_rejects_unknown_environment(self):
        with self.assertRaises(build_trust_floor.TrustFloorError):
            build_trust_floor.load_environment_keys(build_trust_floor.KEYRING_DIR, "production")

    def test_load_keys_rejects_a_tampered_key_file(self):
        original = (build_trust_floor.KEYRING_DIR / "license-prod.pub.pem").read_text(encoding="utf-8")
        lines = original.splitlines()
        first, last = next(index for index, line in enumerate(lines) if not line.startswith("-----")), -2
        # Tampering the first body character breaks the DER structure; tampering
        # inside the last body line keeps the structure and breaks the fingerprint.
        for name, index, position in (("structure", first, 0), ("fingerprint", last, 20)):
            with self.subTest(case=name), tempfile.TemporaryDirectory() as directory:
                keyring = pathlib.Path(directory)
                line = lines[index]
                replacement = ("A" if line[position] != "A" else "B") + line[position + 1:]
                lines[index] = line[:position] + replacement
                (keyring / "license-prod.pub.pem").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
                with self.assertRaises(build_trust_floor.TrustFloorError):
                    build_trust_floor.load_environment_keys(keyring, "prod")
                lines[index] = line

    def test_load_keys_rejects_a_missing_key_file(self):
        with tempfile.TemporaryDirectory() as directory, self.assertRaises(build_trust_floor.TrustFloorError):
            build_trust_floor.load_environment_keys(pathlib.Path(directory), "test")

    def test_load_keys_rejects_key_id_outside_identifier_charset(self):
        with tempfile.TemporaryDirectory() as directory:
            keyring = pathlib.Path(directory)
            (keyring / "key.pub.pem").write_text(pem(TEST_DER), encoding="utf-8", newline="\n")
            for key_id in ("", " license-test", "license test", "a" * 129, "license-test)axlic"):
                with self.subTest(key_id=key_id):
                    key = build_trust_floor.TrustKey(key_id, "key.pub.pem", TEST_FINGERPRINT)
                    with self.assertRaises(build_trust_floor.TrustFloorError):
                        build_trust_floor.load_keys(keyring, (key,))


class DocumentTests(unittest.TestCase):
    def test_render_document_is_canonical(self):
        entries = [
            {"key_id": "k2", "public_key_xy_hex": "02" * 64},
            {"key_id": "k1", "public_key_xy_hex": "01" * 64},
        ]
        expected = (
            "{\n"
            '  "format": "axlic-trust-floor",\n'
            '  "version": 1,\n'
            '  "keys": [\n'
            "    {\n"
            '      "key_id": "k1",\n'
            '      "public_key_xy_hex": "' + "01" * 64 + '"\n'
            "    },\n"
            "    {\n"
            '      "key_id": "k2",\n'
            '      "public_key_xy_hex": "' + "02" * 64 + '"\n'
            "    }\n"
            "  ]\n"
            "}\n"
        )
        self.assertEqual(build_trust_floor.render_document(entries), expected)

    def test_main_writes_document(self):
        with tempfile.TemporaryDirectory() as directory:
            out = pathlib.Path(directory) / "nested" / "trust-floor.json"
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                code = build_trust_floor.main(["--environment", "test", "--out", str(out)])
            self.assertEqual(code, 0)
            self.assertEqual(out.read_text(encoding="utf-8"), TEST_DOCUMENT)
            report = json.loads(stdout.getvalue())
            self.assertEqual(report["environment"], "test")
            self.assertEqual(report["keys"], [{"key_id": "license-test", "public_key_xy_hex": TEST_XY.hex()}])

    def test_main_never_writes_on_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            out = pathlib.Path(directory) / "trust-floor.json"
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                code = build_trust_floor.main(["--environment", "production", "--out", str(out)])
            self.assertEqual(code, 1)
            self.assertFalse(out.exists())
            self.assertEqual(list(pathlib.Path(directory).iterdir()), [])


if __name__ == "__main__":
    unittest.main()
