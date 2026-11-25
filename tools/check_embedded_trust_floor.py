#!/usr/bin/env python3
"""EV-R1-03: prove a built axlic.exe embeds exactly its environment's trust floor.

CMake compiles the trust-floor document into the binary as one raw string, so
the canonical document of the environment must occur in the executable byte
for byte, and no other environment's public key may occur at all. This closes
the silent failure in which an empty or stale floor makes every credential
`unsupported` without any build or runtime error.

Usage:
  python tools/check_embedded_trust_floor.py --environment prod --exe <axlic.exe> \
      [--report evidence/embedded-trust-floor.json] [--implementation-revision SHA]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import build_trust_floor  # noqa: E402


class EmbeddingError(RuntimeError):
    """The executable does not carry exactly the environment's trust floor."""


def check_exe(exe_bytes: bytes, environment: str, keyring_dir: pathlib.Path) -> dict:
    try:
        entries = build_trust_floor.load_environment_keys(keyring_dir, environment)
        document = build_trust_floor.render_document(entries).encode("utf-8")
        if document not in exe_bytes:
            raise EmbeddingError(
                f"the canonical {environment} trust-floor document is not embedded in the executable"
            )
        for other in sorted(build_trust_floor.ENVIRONMENTS):
            if other == environment:
                continue
            for entry in build_trust_floor.load_environment_keys(keyring_dir, other):
                if entry["public_key_xy_hex"].encode("ascii") in exe_bytes:
                    raise EmbeddingError(
                        f"public key {entry['key_id']!r} of environment {other!r} "
                        f"is embedded in a {environment} executable"
                    )
    except build_trust_floor.TrustFloorError as error:
        raise EmbeddingError(str(error)) from error
    return {
        "artifact_id": "EV-R1-03-embedded-trust-floor",
        "authority_revision": "P20-v4-R1",
        "environment": environment,
        "key_ids": [entry["key_id"] for entry in entries],
        "result": "PASS",
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--environment", required=True, help="test or prod")
    parser.add_argument("--exe", type=pathlib.Path, required=True, help="built axlic.exe")
    parser.add_argument("--report", type=pathlib.Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args(argv)

    try:
        report = check_exe(args.exe.read_bytes(), args.environment, build_trust_floor.KEYRING_DIR)
    except OSError as error:
        print(f"check_embedded_trust_floor: cannot read {args.exe}: {error}", file=sys.stderr)
        return 1
    except EmbeddingError as error:
        print(f"check_embedded_trust_floor: {error}", file=sys.stderr)
        return 1
    report["command_or_runner"] = "python tools/check_embedded_trust_floor.py"
    report["implementation_revision"] = args.implementation_revision
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
