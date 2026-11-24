#!/usr/bin/env python3
"""Packaging-time trust-floor export from a signing backend (OpenBao transit).

Fetches the P-256 public keys of the named transit keys from an OpenBao
instance and writes the embedded trust-floor artifact consumed by the CMake
build (client/keyring/trust-floor.json). This is operator tooling only: it is
never compiled into axlic.exe and the binary keeps zero network code.

Fail-closed: if any step fails (missing env, unreachable instance, malformed
response), the existing artifact is left untouched.

Usage:
  VAULT_ADDR=https://vault.example.com VAULT_TOKEN=... \
  python tools/fetch_trust_floor.py --key license-credential-v1 [--key ...] \
      [--out client/keyring/trust-floor.json]
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import pathlib
import re
import sys
import tempfile
import urllib.error
import urllib.request

# Fixed DER SubjectPublicKeyInfo prefix for an uncompressed NIST P-256 point:
# SEQUENCE { SEQUENCE { OID ecPublicKey, OID prime256v1 }, BIT STRING 04||X||Y }
P256_SPKI_PREFIX = bytes.fromhex("3059301306072a8648ce3d020106082a8648ce3d030107034200")
UNCOMPRESSED_POINT_LENGTH = 65  # 0x04 || X(32) || Y(32)

ARTIFACT_FORMAT = "axlic-trust-floor"
ARTIFACT_VERSION = 1


class ExportError(RuntimeError):
    """Any failure that must leave the existing artifact untouched."""


def fetch_public_key_hex(vault_addr: str, token: str, key_name: str, timeout: float) -> str:
    url = f"{vault_addr.rstrip('/')}/v1/transit/keys/{urllib.request.quote(key_name)}"
    request = urllib.request.Request(url, headers={"X-Vault-Token": token})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = json.load(response)
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError) as error:
        raise ExportError(f"cannot read transit key {key_name!r}: {error}") from error
    try:
        data = payload["data"]
        latest = int(data["latest_version"])
        der_b64 = data["keys"][str(latest)]["public_key"]
        key_type = data.get("type", "ecdsa-p256")
    except (KeyError, TypeError, ValueError) as error:
        raise ExportError(f"malformed transit response for {key_name!r}: {error}") from error
    if key_type != "ecdsa-p256":
        raise ExportError(f"transit key {key_name!r} is {key_type!r}, expected ecdsa-p256")
    try:
        der = base64.b64decode(der_b64, validate=True)
    except (ValueError, TypeError) as error:
        raise ExportError(f"invalid base64 public key for {key_name!r}: {error}") from error
    if len(der) != len(P256_SPKI_PREFIX) + UNCOMPRESSED_POINT_LENGTH or not der.startswith(
        P256_SPKI_PREFIX
    ):
        raise ExportError(f"unexpected public-key encoding for {key_name!r}")
    point = der[len(P256_SPKI_PREFIX) :]
    if point[0] != 0x04:
        raise ExportError(f"compressed point for {key_name!r}, expected uncompressed")
    return point[1:].hex()


def write_artifact_atomic(path: pathlib.Path, keys: list[dict[str, str]]) -> None:
    document = {
        "format": ARTIFACT_FORMAT,
        "version": ARTIFACT_VERSION,
        "keys": sorted(keys, key=lambda entry: entry["key_id"]),
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    handle, temp_name = tempfile.mkstemp(dir=str(path.parent), prefix=".trust-floor.", suffix=".tmp")
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(document, stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temp_name, path)
    except OSError as error:
        try:
            os.unlink(temp_name)
        except OSError:
            pass
        raise ExportError(f"cannot write artifact {path}: {error}") from error


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--key", action="append", required=True, dest="keys",
                        help="transit key name (repeatable)")
    parser.add_argument("--out", default="client/keyring/trust-floor.json",
                        help="output artifact path")
    parser.add_argument("--timeout", type=float, default=15.0,
                        help="per-request timeout in seconds")
    args = parser.parse_args(argv)

    vault_addr = os.environ.get("VAULT_ADDR", "").strip()
    vault_token = os.environ.get("VAULT_TOKEN", "").strip()
    if not vault_addr or not vault_token:
        print("fetch_trust_floor: VAULT_ADDR and VAULT_TOKEN must be set", file=sys.stderr)
        return 1
    if len(set(args.keys)) != len(args.keys):
        print("fetch_trust_floor: duplicate --key names", file=sys.stderr)
        return 1
    # Key ids become JSON string values embedded in a C++ raw string; keep
    # them to a conservative identifier charset so they can never form the
    # ")axlic" raw-string terminator (defense for tools/check + CMake guard).
    invalid_names = [name for name in args.keys if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,127}", name)]
    if invalid_names:
        print(f"fetch_trust_floor: key names must match [A-Za-z0-9][A-Za-z0-9._-] (max 128 chars): {invalid_names}", file=sys.stderr)
        return 1

    try:
        entries = [
            {"key_id": name, "public_key_xy_hex": fetch_public_key_hex(vault_addr, vault_token, name, args.timeout)}
            for name in args.keys
        ]
        write_artifact_atomic(pathlib.Path(args.out), entries)
    except ExportError as error:
        print(f"fetch_trust_floor: {error}; existing artifact left untouched", file=sys.stderr)
        return 1
    print(f"fetch_trust_floor: wrote {len(entries)} key(s) to {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
