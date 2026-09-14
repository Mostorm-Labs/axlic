#!/usr/bin/env python3
"""Independent, test-only AxLicense A0 vector generator and verifier."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Iterable

from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import (
    decode_dss_signature,
    encode_dss_signature,
)

ALGORITHM_ID = "axl-ecdsa-p256-sha256-v1"
KEY_ID = "a0-fixture-key-1"
DOMAIN = "axlicense.credential"
CURVE_ORDER = int("FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551", 16)
TEST_ONLY_PRIVATE_SCALAR = int(
    "519B423D715F8B5D5499F8E3AEF260765C1F1F5C1CC53F3F8C86C1F11B2E6A7D", 16
)


def _head(major: int, value: int) -> bytes:
    if value < 0:
        raise ValueError("negative CBOR length")
    prefix = major << 5
    if value < 24:
        return bytes([prefix | value])
    if value <= 0xFF:
        return bytes([prefix | 24, value])
    if value <= 0xFFFF:
        return bytes([prefix | 25]) + value.to_bytes(2, "big")
    if value <= 0xFFFFFFFF:
        return bytes([prefix | 26]) + value.to_bytes(4, "big")
    if value <= 0xFFFFFFFFFFFFFFFF:
        return bytes([prefix | 27]) + value.to_bytes(8, "big")
    raise ValueError("integer exceeds CBOR uint64")


def cbor_uint(value: int) -> bytes:
    return _head(0, value)


def cbor_int(value: int) -> bytes:
    return cbor_uint(value) if value >= 0 else _head(1, -1 - value)


def cbor_text(value: str) -> bytes:
    encoded = value.encode("utf-8")
    return _head(3, len(encoded)) + encoded


def cbor_bytes(value: bytes) -> bytes:
    return _head(2, len(value)) + value


def cbor_array(values: Iterable[bytes]) -> bytes:
    items = list(values)
    return _head(4, len(items)) + b"".join(items)


def cbor_map(pairs: Iterable[tuple[int, bytes]]) -> bytes:
    items = list(pairs)
    return _head(5, len(items)) + b"".join(cbor_uint(key) + value for key, value in items)


def encode_validity(value: dict[str, Any]) -> bytes:
    pairs = [(1, cbor_uint({"perpetual": 1, "bounded": 2}[value["kind"]]))]
    if "not_before" in value:
        pairs.append((2, cbor_int(value["not_before"])))
    if "not_after" in value:
        pairs.append((3, cbor_int(value["not_after"])))
    return cbor_map(pairs)


def encode_entitlement(value: dict[str, Any]) -> bytes:
    pairs = [
        (1, cbor_text(value["entitlement_id"])),
        (2, cbor_text(value["product_id"])),
        (3, cbor_uint({"runtime": 1, "maintenance_update": 2, "cloud_service": 3}[value["right_kind"]])),
        (4, cbor_uint({"presence": 1, "bounded_u64": 2}[value["grant_semantics"]])),
        (5, encode_validity(value["validity"])),
    ]
    if "constraint" in value:
        constraint = value["constraint"]
        pairs.append((6, cbor_map([(1, cbor_uint(1)), (2, cbor_uint(constraint["value"]))])))
    return cbor_map(pairs)


def encode_payload(value: dict[str, Any]) -> bytes:
    entitlements = sorted(value["entitlements"], key=lambda item: item["entitlement_id"].encode("utf-8"))
    pairs = [
        (1, cbor_array([cbor_uint(value["schema_version"][0]), cbor_uint(value["schema_version"][1])])),
        (2, cbor_text(value["credential_id"])),
        (3, cbor_text(value["license_grant_id"])),
        (4, cbor_uint(value["authority_revision"])),
        (5, cbor_text(value["binding_id"])),
        (6, cbor_text(value["device_id"])),
        (7, cbor_uint(value["credential_generation"])),
        (8, cbor_int(value["issued_at"])),
    ]
    if "supersedes_credential_id" in value:
        pairs.append((9, cbor_text(value["supersedes_credential_id"])))
    pairs.append((10, cbor_array(encode_entitlement(item) for item in entitlements)))
    return cbor_map(pairs)


def signing_input(payload: bytes, algorithm_id: str = ALGORITHM_ID, key_id: str = KEY_ID) -> bytes:
    return cbor_array(
        [cbor_text(DOMAIN), cbor_uint(1), cbor_text(algorithm_id), cbor_text(key_id), cbor_bytes(payload)]
    )


def _private_key(scalar: int = TEST_ONLY_PRIVATE_SCALAR) -> ec.EllipticCurvePrivateKey:
    return ec.derive_private_key(scalar, ec.SECP256R1())


def public_xy(scalar: int = TEST_ONLY_PRIVATE_SCALAR) -> bytes:
    numbers = _private_key(scalar).public_key().public_numbers()
    return numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")


def sign(message: bytes) -> bytes:
    der = _private_key().sign(message, ec.ECDSA(hashes.SHA256(), deterministic_signing=True))
    r, s = decode_dss_signature(der)
    s = min(s, CURVE_ORDER - s)
    return r.to_bytes(32, "big") + s.to_bytes(32, "big")


def envelope(
    payload: bytes,
    signature: bytes,
    *,
    artifact_kind: int = 1,
    algorithm_id: str = ALGORITHM_ID,
    key_id: str = KEY_ID,
    order: tuple[int, ...] = (1, 2, 3, 4, 5, 6),
) -> bytes:
    fields = {
        1: cbor_uint(artifact_kind),
        2: cbor_uint(1),
        3: cbor_text(algorithm_id),
        4: cbor_text(key_id),
        5: cbor_bytes(payload),
        6: cbor_bytes(signature),
    }
    return cbor_map((key, fields[key]) for key in order)


def fixture_perpetual() -> dict[str, Any]:
    return {
        "schema_version": [1, 0],
        "credential_id": "cred-a0-perpetual",
        "license_grant_id": "grant-a0",
        "authority_revision": 7,
        "binding_id": "binding-a0",
        "device_id": "device-a0",
        "credential_generation": 3,
        "issued_at": 1767225600,
        "entitlements": [
            {
                "entitlement_id": "nearhub.runtime",
                "product_id": "nearhub",
                "right_kind": "runtime",
                "grant_semantics": "presence",
                "validity": {"kind": "perpetual"},
            }
        ],
    }


def fixture_bounded(reverse: bool = False) -> dict[str, Any]:
    entries = [
        {
            "entitlement_id": "nearhub.runtime",
            "product_id": "nearhub",
            "right_kind": "runtime",
            "grant_semantics": "presence",
            "validity": {"kind": "perpetual", "not_before": 1767225600},
        },
        {
            "entitlement_id": "nearhub.concurrent_sources",
            "product_id": "nearhub",
            "right_kind": "runtime",
            "grant_semantics": "bounded_u64",
            "validity": {"kind": "bounded", "not_before": 1767225600, "not_after": 4102444800},
            "constraint": {"kind": "max_u64", "value": 4},
        },
    ]
    if reverse:
        entries.reverse()
    return {
        "schema_version": [1, 0],
        "credential_id": "cred-a0-bounded",
        "license_grant_id": "grant-a0",
        "authority_revision": 8,
        "binding_id": "binding-a0",
        "device_id": "device-a0",
        "credential_generation": 4,
        "issued_at": 1767225600,
        "supersedes_credential_id": "cred-a0-perpetual",
        "entitlements": entries,
    }


def _positive(vector_id: str, semantic: dict[str, Any]) -> dict[str, Any]:
    payload = encode_payload(semantic)
    message = signing_input(payload)
    signature = sign(message)
    return {
        "id": vector_id,
        "semantic": semantic,
        "payload_hex": payload.hex(),
        "signing_input_hex": message.hex(),
        "signature_hex": signature.hex(),
        "artifact_hex": envelope(payload, signature).hex(),
        "expected_status": "valid",
    }


def _signed_artifact(payload: bytes, *, algorithm_id: str = ALGORITHM_ID, key_id: str = KEY_ID) -> bytes:
    return envelope(payload, sign(signing_input(payload, algorithm_id, key_id)), algorithm_id=algorithm_id, key_id=key_id)


def generate_corpus() -> dict[str, Any]:
    perpetual = _positive("canonical-perpetual-presence", fixture_perpetual())
    bounded = _positive("canonical-bounded-u64", fixture_bounded())
    ordered = _positive("canonical-input-order-independent", fixture_bounded(reverse=True))

    base_payload = bytes.fromhex(perpetual["payload_hex"])
    base_signature = bytes.fromhex(perpetual["signature_hex"])
    base_artifact = bytes.fromhex(perpetual["artifact_hex"])

    tampered_semantic = fixture_perpetual()
    tampered_semantic["authority_revision"] = 6
    tampered_payload = encode_payload(tampered_semantic)

    schema2 = fixture_perpetual()
    schema2["schema_version"] = [2, 0]
    schema2_payload = encode_payload(schema2)

    null_payload = base_payload[:-1]  # replaced below with a full map containing field 9 = null
    semantic = fixture_perpetual()
    normal_pairs = [
        (1, cbor_array([cbor_uint(1), cbor_uint(0)])),
        (2, cbor_text(semantic["credential_id"])),
        (3, cbor_text(semantic["license_grant_id"])),
        (4, cbor_uint(semantic["authority_revision"])),
        (5, cbor_text(semantic["binding_id"])),
        (6, cbor_text(semantic["device_id"])),
        (7, cbor_uint(semantic["credential_generation"])),
        (8, cbor_int(semantic["issued_at"])),
        (9, b"\xf6"),
        (10, cbor_array([encode_entitlement(semantic["entitlements"][0])])),
    ]
    null_payload = cbor_map(normal_pairs)

    nonminimal_payload = base_payload.replace(
        cbor_uint(2) + cbor_text("cred-a0-perpetual"),
        cbor_uint(2) + bytes([0x78, len("cred-a0-perpetual")]) + b"cred-a0-perpetual",
        1,
    )
    wrong_payload_order = cbor_map([normal_pairs[1], normal_pairs[0], *normal_pairs[2:8], normal_pairs[9]])
    indefinite_payload = b"\xbf" + b"".join(cbor_uint(key) + value for key, value in normal_pairs[:8] + [normal_pairs[9]]) + b"\xff"
    tagged_payload = b"\xc0" + base_payload
    nested_payload = cbor_array([b"\x81" * 17 + b"\x00"])
    excess_items_payload = cbor_array(cbor_uint(0) for _ in range(513))

    r = int.from_bytes(base_signature[:32], "big")
    s = int.from_bytes(base_signature[32:], "big")
    high_s = r.to_bytes(32, "big") + (CURVE_ORDER - s).to_bytes(32, "big")
    alternate_key = public_xy(TEST_ONLY_PRIVATE_SCALAR + 1)

    negatives = [
        {"id": "payload-one-bit-tamper", "artifact_hex": envelope(tampered_payload, base_signature).hex(), "expected_status": "invalid"},
        {"id": "wrong-public-key", "artifact_hex": base_artifact.hex(), "trusted_public_key_xy_hex": alternate_key.hex(), "expected_status": "invalid"},
        {"id": "wrong-algorithm-id", "artifact_hex": envelope(base_payload, base_signature, algorithm_id="unsupported").hex(), "expected_status": "unsupported"},
        {"id": "wrong-artifact-kind", "artifact_hex": envelope(base_payload, base_signature, artifact_kind=2).hex(), "expected_status": "unsupported"},
        {"id": "unknown-schema-major", "artifact_hex": _signed_artifact(schema2_payload).hex(), "expected_status": "unsupported"},
        {"id": "non-minimal-length", "artifact_hex": _signed_artifact(nonminimal_payload).hex(), "expected_status": "invalid"},
        {"id": "indefinite-length-item", "artifact_hex": _signed_artifact(indefinite_payload).hex(), "expected_status": "invalid"},
        {"id": "cbor-tag", "artifact_hex": _signed_artifact(tagged_payload).hex(), "expected_status": "invalid"},
        {"id": "duplicate-map-key", "artifact_hex": (bytes([0xA7]) + cbor_uint(1) + cbor_uint(1) + cbor_uint(1) + cbor_uint(1) + base_artifact[3:]).hex(), "expected_status": "invalid"},
        {"id": "wrong-canonical-map-key-order", "artifact_hex": envelope(base_payload, base_signature, order=(2, 1, 3, 4, 5, 6)).hex(), "expected_status": "invalid"},
        {"id": "forbidden-null", "artifact_hex": _signed_artifact(null_payload).hex(), "expected_status": "invalid"},
        {"id": "signature-wrong-length", "artifact_hex": envelope(base_payload, base_signature[:-1]).hex(), "expected_status": "invalid"},
        {"id": "signature-rs-out-of-range", "artifact_hex": envelope(base_payload, bytes(64)).hex(), "expected_status": "invalid"},
        {"id": "signature-high-s", "artifact_hex": envelope(base_payload, high_s).hex(), "expected_status": "invalid"},
        {"id": "oversized-artifact", "artifact_hex": bytes(65537).hex(), "expected_status": "invalid"},
        {"id": "excess-nesting", "artifact_hex": _signed_artifact(nested_payload).hex(), "expected_status": "invalid"},
        {"id": "excess-item-count", "artifact_hex": _signed_artifact(excess_items_payload).hex(), "expected_status": "invalid"},
        {"id": "wrong-payload-map-order", "artifact_hex": _signed_artifact(wrong_payload_order).hex(), "expected_status": "invalid"},
    ]

    return {
        "corpus_id": "AXL-V1-A0-EV01-v1",
        "algorithm_id": ALGORITHM_ID,
        "key_id": KEY_ID,
        "public_key_xy_hex": public_xy().hex(),
        "positives": [perpetual, bounded, ordered],
        "negatives": negatives,
    }


def _canonical_json(value: Any) -> str:
    return json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def write_outputs(output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    corpus = generate_corpus()
    (output_dir / "corpus.json").write_text(_canonical_json(corpus), encoding="utf-8", newline="\n")
    (output_dir / "TEST_ONLY_key.json").write_text(
        _canonical_json(
            {
                "warning": "NON-PRODUCTION TEST FIXTURE PRIVATE KEY",
                "key_id": KEY_ID,
                "private_scalar_hex": f"{TEST_ONLY_PRIVATE_SCALAR:064x}",
                "public_key_xy_hex": public_xy().hex(),
            }
        ),
        encoding="utf-8",
        newline="\n",
    )
    valid = corpus["positives"][1]
    state = {
        "state": "present",
        "artifact_hex": valid["artifact_hex"],
        "trusted_keys": [{"key_id": KEY_ID, "public_key_xy_hex": corpus["public_key_xy_hex"]}],
    }
    (output_dir / "valid_state.json").write_text(_canonical_json(state), encoding="utf-8", newline="\n")
    (output_dir / "absent_state.json").write_text(_canonical_json({"state": "absent"}), encoding="utf-8", newline="\n")


def verify_checked_in(corpus_path: Path, revision: str, report_path: Path | None) -> None:
    expected = json.loads(corpus_path.read_text(encoding="utf-8"))
    actual = generate_corpus()
    if expected != actual:
        raise SystemExit("checked-in A0 corpus differs from independent regeneration")

    public = ec.EllipticCurvePublicNumbers(
        int.from_bytes(bytes.fromhex(actual["public_key_xy_hex"])[:32], "big"),
        int.from_bytes(bytes.fromhex(actual["public_key_xy_hex"])[32:], "big"),
        ec.SECP256R1(),
    ).public_key()
    results = []
    for vector in actual["positives"]:
        raw = bytes.fromhex(vector["signature_hex"])
        r = int.from_bytes(raw[:32], "big")
        s = int.from_bytes(raw[32:], "big")
        if len(raw) != 64 or not (0 < r < CURVE_ORDER) or not (0 < s <= CURVE_ORDER // 2):
            raise SystemExit(f"invalid fixed-width low-S fixture: {vector['id']}")
        public.verify(encode_dss_signature(r, s), bytes.fromhex(vector["signing_input_hex"]), ec.ECDSA(hashes.SHA256()))
        results.append({"id": vector["id"], "result": "PASS"})

    corpus_hash = hashlib.sha256(corpus_path.read_bytes()).hexdigest()
    report = {
        "artifact_id": "EV-01-A0-independent-reference",
        "authority_revision": "P20-FR019-v0.1",
        "command_or_runner": "python reference/tools/a0_reference.py --check",
        "corpus_sha256": corpus_hash,
        "execution_environment": "Python cryptography 50.0.1 independent verifier",
        "failures": [],
        "fixture_or_corpus_id": actual["corpus_id"],
        "implementation_revision": revision,
        "result": "PASS",
        "vectors": results,
    }
    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(_canonical_json(report), encoding="utf-8", newline="\n")
    print(_canonical_json(report), end="")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--output-dir", type=Path, default=Path("reference/vectors/a0"))
    parser.add_argument("--corpus", type=Path, default=Path("reference/vectors/a0/corpus.json"))
    parser.add_argument("--implementation-revision", default="WORKTREE")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.generate == args.check:
        parser.error("choose exactly one of --generate or --check")
    if args.generate:
        write_outputs(args.output_dir)
    else:
        verify_checked_in(args.corpus, args.implementation_revision, args.report)


if __name__ == "__main__":
    main()
