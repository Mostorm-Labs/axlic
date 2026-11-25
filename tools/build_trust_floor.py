#!/usr/bin/env python3
"""Offline trust-floor derivation from a committed public key.

Reads the public-key file committed for one environment (test or prod) under
client/keyring/, checks it against the fingerprint pinned in this file and
writes the trust-floor document that the CMake build embeds into axlic.exe
(AXLIC_TRUST_FLOOR_FILE). No network access is involved: the private keys stay
in the signing backend and are never needed to build or publish. This is
operator tooling only: it is never compiled into axlic.exe.

Fail-closed: if any step fails (unknown environment, unreadable or malformed
key file, fingerprint mismatch), nothing is written.

Usage:
  python tools/build_trust_floor.py --environment prod --out <path>
"""

from __future__ import annotations

import argparse
import base64
from dataclasses import dataclass
import hashlib
import json
import os
import pathlib
import re
import sys
import tempfile

# Fixed DER SubjectPublicKeyInfo prefix for an uncompressed NIST P-256 point:
# SEQUENCE { SEQUENCE { OID ecPublicKey, OID prime256v1 }, BIT STRING 04||X||Y }
P256_SPKI_PREFIX = bytes.fromhex("3059301306072a8648ce3d020106082a8648ce3d030107034200")
UNCOMPRESSED_POINT_LENGTH = 65  # 0x04 || X(32) || Y(32)
SPKI_LENGTH = len(P256_SPKI_PREFIX) + UNCOMPRESSED_POINT_LENGTH
PEM_BEGIN = "-----BEGIN PUBLIC KEY-----"
PEM_END = "-----END PUBLIC KEY-----"

# Key ids become JSON string values embedded in a C++ raw string; keep them to
# a conservative identifier charset so they can never form the ")axlic"
# raw-string terminator (defense in depth for the CMake guard).
KEY_ID_PATTERN = r"[A-Za-z0-9][A-Za-z0-9._-]{0,127}"

ARTIFACT_FORMAT = "axlic-trust-floor"
ARTIFACT_VERSION = 1

KEYRING_DIR = pathlib.Path(__file__).resolve().parent.parent / "client" / "keyring"


@dataclass(frozen=True)
class TrustKey:
    key_id: str
    pem: str
    spki_sha256: str


# One dedicated verification key per environment (P20 §3.3 key-purpose
# separation). A rotation adds a retained previous key to the environment's
# tuple; changing a pin is a reviewed change to this file.
ENVIRONMENTS: dict[str, tuple[TrustKey, ...]] = {
    "test": (
        TrustKey(
            "license-test",
            "license-test.pub.pem",
            "ca7795786820dd5d3d669fb3321b3e11bdafc1cce7d39c4fd59f52e7ef110aea",
        ),
    ),
    "prod": (
        TrustKey(
            "license-prod",
            "license-prod.pub.pem",
            "58bb4787245691066105914d20ff3da5da2cb8677b13e943a40f6a73bcc277a8",
        ),
    ),
}


class TrustFloorError(RuntimeError):
    """Any failure that must leave the output untouched."""


def check_spki(der: bytes) -> None:
    if len(der) != SPKI_LENGTH or not der.startswith(P256_SPKI_PREFIX):
        raise TrustFloorError("public key is not an uncompressed P-256 SubjectPublicKeyInfo")
    if der[len(P256_SPKI_PREFIX)] != 0x04:
        raise TrustFloorError("compressed point, expected an uncompressed one")


def parse_spki_pem(text: str) -> bytes:
    """Return the DER of exactly one PEM `PUBLIC KEY` block, nothing else."""
    text = text.lstrip("\ufeff")
    if text.count(PEM_BEGIN) != 1 or text.count(PEM_END) != 1:
        raise TrustFloorError("expected exactly one PEM PUBLIC KEY block")
    begin = text.index(PEM_BEGIN)
    end = text.index(PEM_END)
    if end < begin or text[:begin].strip() or text[end + len(PEM_END) :].strip():
        raise TrustFloorError("unexpected content outside the PEM PUBLIC KEY block")
    try:
        der = base64.b64decode("".join(text[begin + len(PEM_BEGIN) : end].split()), validate=True)
    except (ValueError, TypeError) as error:
        raise TrustFloorError(f"invalid base64 in the PEM block: {error}") from error
    check_spki(der)
    return der


def public_key_xy_hex(der: bytes) -> str:
    check_spki(der)
    return der[len(P256_SPKI_PREFIX) + 1 :].hex()


def load_keys(keyring_dir: pathlib.Path, keys: tuple[TrustKey, ...]) -> list[dict[str, str]]:
    entries: list[dict[str, str]] = []
    for key in keys:
        if not re.fullmatch(KEY_ID_PATTERN, key.key_id):
            raise TrustFloorError(
                f"key id {key.key_id!r} must match [A-Za-z0-9][A-Za-z0-9._-] (max 128 chars)"
            )
        path = keyring_dir / key.pem
        try:
            raw = path.read_bytes()
        except OSError as error:
            raise TrustFloorError(f"cannot read {path}: {error}") from error
        try:
            der = parse_spki_pem(raw.decode("utf-8-sig"))
        except UnicodeDecodeError as error:
            raise TrustFloorError(f"{path} is not UTF-8 text: {error}") from error
        fingerprint = hashlib.sha256(der).hexdigest()
        if fingerprint != key.spki_sha256:
            raise TrustFloorError(
                f"{path.name} does not match the fingerprint pinned for {key.key_id!r}: "
                f"expected {key.spki_sha256}, found {fingerprint}"
            )
        entries.append({"key_id": key.key_id, "public_key_xy_hex": public_key_xy_hex(der)})
    return entries


def load_environment_keys(keyring_dir: pathlib.Path, environment: str) -> list[dict[str, str]]:
    try:
        keys = ENVIRONMENTS[environment]
    except KeyError:
        raise TrustFloorError(
            f"unknown environment {environment!r}; expected one of {sorted(ENVIRONMENTS)}"
        ) from None
    return load_keys(keyring_dir, keys)


def render_document(entries: list[dict[str, str]]) -> str:
    document = {
        "format": ARTIFACT_FORMAT,
        "version": ARTIFACT_VERSION,
        "keys": sorted(entries, key=lambda entry: entry["key_id"]),
    }
    return json.dumps(document, indent=2) + "\n"


def write_document_atomic(path: pathlib.Path, document: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    handle, temp_name = tempfile.mkstemp(dir=str(path.parent), prefix=".trust-floor.", suffix=".tmp")
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(document)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temp_name, path)
    except OSError as error:
        try:
            os.unlink(temp_name)
        except OSError:
            pass
        raise TrustFloorError(f"cannot write {path}: {error}") from error


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--environment", required=True, help="test or prod")
    parser.add_argument("--out", type=pathlib.Path, required=True, help="output document path")
    args = parser.parse_args(argv)

    try:
        entries = load_environment_keys(KEYRING_DIR, args.environment)
        write_document_atomic(args.out, render_document(entries))
    except TrustFloorError as error:
        print(f"build_trust_floor: {error}; nothing written", file=sys.stderr)
        return 1
    print(json.dumps({"environment": args.environment, "keys": entries, "out": str(args.out)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
