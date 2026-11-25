#!/usr/bin/env python3
"""Unit tests for tools/check_embedded_trust_floor.py (EV-R1-03)."""

from __future__ import annotations

import contextlib
import io
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import check_embedded_trust_floor  # noqa: E402

KEYRING = pathlib.Path(__file__).resolve().parents[2] / "client" / "keyring"

TEST_XY = (
    "d7027588352422870b83a9a154ba0aaf105b8b5c191bf5a138562b369428"
    "8676e11271cf055e03e452a45448e825ec125bec1eea94fc06ae6e4d996ec6fac4d7"
)
PROD_XY = (
    "b3cae3d4a15f2e81719b9373b1b37a7050d829f563354868f5b98063bd98608"
    "ce32d037297fbf79e94ee9e2b78b6be25ca692d234382ac731f70ea065e397de0"
)
EMPTY_FLOOR = '{\n  "format": "axlic-trust-floor",\n  "version": 1,\n  "keys": []\n}\n'


def floor_document(key_id: str, xy: str) -> str:
    """The canonical single-entry document, written out by hand."""
    return (
        "{\n"
        '  "format": "axlic-trust-floor",\n'
        '  "version": 1,\n'
        '  "keys": [\n'
        "    {\n"
        f'      "key_id": "{key_id}",\n'
        f'      "public_key_xy_hex": "{xy}"\n'
        "    }\n"
        "  ]\n"
        "}\n"
    )


def fake_exe(*chunks: bytes) -> bytes:
    return b"MZ" + bytes(64) + b"".join(chunks) + bytes(64)


class CheckExeTests(unittest.TestCase):
    def test_exe_with_canonical_document_passes(self):
        for environment, key_id, xy in (("test", "license-test", TEST_XY), ("prod", "license-prod", PROD_XY)):
            with self.subTest(environment=environment):
                report = check_embedded_trust_floor.check_exe(
                    fake_exe(floor_document(key_id, xy).encode()), environment, KEYRING
                )
                self.assertEqual(report["result"], "PASS")
                self.assertEqual(report["artifact_id"], "EV-R1-03-embedded-trust-floor")
                self.assertEqual(report["environment"], environment)
                self.assertEqual(report["key_ids"], [key_id])

    def test_exe_with_empty_floor_fails(self):
        for environment in ("test", "prod"):
            with self.subTest(environment=environment):
                with self.assertRaises(check_embedded_trust_floor.EmbeddingError):
                    check_embedded_trust_floor.check_exe(fake_exe(EMPTY_FLOOR.encode()), environment, KEYRING)

    def test_exe_of_the_other_environment_fails(self):
        exe = fake_exe(floor_document("license-prod", PROD_XY).encode())
        with self.assertRaises(check_embedded_trust_floor.EmbeddingError):
            check_embedded_trust_floor.check_exe(exe, "test", KEYRING)

    def test_exe_with_other_environment_key_fails(self):
        exe = fake_exe(floor_document("license-test", TEST_XY).encode(), PROD_XY.encode())
        with self.assertRaises(check_embedded_trust_floor.EmbeddingError):
            check_embedded_trust_floor.check_exe(exe, "test", KEYRING)

    def test_exe_with_equivalent_but_non_canonical_document_fails(self):
        compact = json.dumps(json.loads(floor_document("license-test", TEST_XY)), separators=(",", ":"))
        with self.assertRaises(check_embedded_trust_floor.EmbeddingError):
            check_embedded_trust_floor.check_exe(fake_exe(compact.encode()), "test", KEYRING)

    def test_unknown_environment_fails(self):
        with self.assertRaises(check_embedded_trust_floor.EmbeddingError):
            check_embedded_trust_floor.check_exe(fake_exe(), "production", KEYRING)


class MainTests(unittest.TestCase):
    def test_main_passes_and_writes_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            exe = root / "axlic.exe"
            exe.write_bytes(fake_exe(floor_document("license-prod", PROD_XY).encode()))
            report = root / "report.json"
            with contextlib.redirect_stdout(io.StringIO()):
                code = check_embedded_trust_floor.main(
                    ["--environment", "prod", "--exe", str(exe), "--report", str(report),
                     "--implementation-revision", "0123abcd"]
                )
            self.assertEqual(code, 0)
            written = json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(written["result"], "PASS")
            self.assertEqual(written["environment"], "prod")
            self.assertEqual(written["implementation_revision"], "0123abcd")

    def test_main_returns_one_and_writes_no_report_on_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            exe = root / "axlic.exe"
            exe.write_bytes(fake_exe(EMPTY_FLOOR.encode()))
            report = root / "report.json"
            with contextlib.redirect_stderr(io.StringIO()):
                code = check_embedded_trust_floor.main(
                    ["--environment", "test", "--exe", str(exe), "--report", str(report)]
                )
            self.assertEqual(code, 1)
            self.assertFalse(report.exists())

    def test_main_returns_one_for_a_missing_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            with contextlib.redirect_stderr(io.StringIO()):
                code = check_embedded_trust_floor.main(
                    ["--environment", "test", "--exe", str(pathlib.Path(directory) / "missing.exe")]
                )
            self.assertEqual(code, 1)


if __name__ == "__main__":
    unittest.main()
