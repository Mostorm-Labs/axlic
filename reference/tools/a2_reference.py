#!/usr/bin/env python3
"""Independent, test-only A2 install-vector generator.

Reuses the A0 reference codec/signer (a0_reference.py) to emit device-bound
signed artifacts for the credential-installation decision matrix. Output is
deterministic: same source revision always yields the same install.json.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

import a0_reference as a0  # noqa: E402

DEVICE_ID = "axldev_" + "a2" * 16
OTHER_DEVICE_ID = "axldev_" + "b3" * 16


def semantic(
    *,
    credential_id: str,
    authority_revision: int,
    credential_generation: int,
    entitlements: list[dict[str, Any]],
    device_id: str = DEVICE_ID,
    supersedes: str | None = None,
) -> dict[str, Any]:
    value: dict[str, Any] = {
        "schema_version": [1, 0],
        "credential_id": credential_id,
        "license_grant_id": "grant-a2",
        "authority_revision": authority_revision,
        "binding_id": "binding-a2",
        "device_id": device_id,
        "credential_generation": credential_generation,
        "issued_at": 1767225600,
        "entitlements": entitlements,
    }
    if supersedes is not None:
        value["supersedes_credential_id"] = supersedes
    return value


def presence_entitlement() -> dict[str, Any]:
    return {
        "entitlement_id": "nearhub.runtime",
        "product_id": "nearhub",
        "right_kind": "runtime",
        "grant_semantics": "presence",
        "validity": {"kind": "perpetual"},
    }


def bounded_entitlement() -> dict[str, Any]:
    return {
        "entitlement_id": "nearhub.seats",
        "product_id": "nearhub",
        "right_kind": "runtime",
        "grant_semantics": "bounded_u64",
        "validity": {"kind": "bounded", "not_before": 10, "not_after": 2000000000},
        "constraint": {"kind": "max_u64", "value": 5},
    }


def artifact_bytes(value: dict[str, Any]) -> bytes:
    payload = a0.encode_payload(value)
    signature = a0.sign(a0.signing_input(payload))
    return a0.envelope(payload, signature)


def describe(value: dict[str, Any]) -> dict[str, Any]:
    return {
        "authority_revision": value["authority_revision"],
        "credential_generation": value["credential_generation"],
        "entitlements": len(value["entitlements"]),
    }


def generate_corpus() -> dict[str, Any]:
    v1 = semantic(
        credential_id="cred-a2-v1",
        authority_revision=3,
        credential_generation=7,
        entitlements=[presence_entitlement(), bounded_entitlement()],
    )
    v2_new = semantic(
        credential_id="cred-a2-v2",
        authority_revision=4,
        credential_generation=8,
        entitlements=[presence_entitlement()],
        supersedes="cred-a2-v1",
    )
    clear = semantic(
        credential_id="cred-a2-clear",
        authority_revision=5,
        credential_generation=9,
        entitlements=[],
        supersedes="cred-a2-v2",
    )
    same_rev_higher_gen = semantic(
        credential_id="cred-a2-reissue",
        authority_revision=3,
        credential_generation=8,
        entitlements=[presence_entitlement()],
    )
    stale_lower_revision = semantic(
        credential_id="cred-a2-stale-rev",
        authority_revision=2,
        credential_generation=99,
        entitlements=[presence_entitlement()],
    )
    stale_lower_generation = semantic(
        credential_id="cred-a2-stale-gen",
        authority_revision=3,
        credential_generation=6,
        entitlements=[presence_entitlement()],
    )
    mismatch_device = semantic(
        credential_id="cred-a2-other-device",
        authority_revision=9,
        credential_generation=9,
        entitlements=[presence_entitlement()],
        device_id=OTHER_DEVICE_ID,
    )

    v1_bytes = artifact_bytes(v1)
    invalid_signature = bytearray(v1_bytes)
    invalid_signature[-1] ^= 0xFF

    artifacts: dict[str, dict[str, Any]] = {
        "v1": {"hex": v1_bytes.hex(), **describe(v1)},
        "v2_new": {"hex": artifact_bytes(v2_new).hex(), **describe(v2_new)},
        "clear": {"hex": artifact_bytes(clear).hex(), **describe(clear)},
        "same_rev_higher_gen": {"hex": artifact_bytes(same_rev_higher_gen).hex(), **describe(same_rev_higher_gen)},
        "stale_lower_revision": {"hex": artifact_bytes(stale_lower_revision).hex(), **describe(stale_lower_revision)},
        "stale_lower_generation": {"hex": artifact_bytes(stale_lower_generation).hex(), **describe(stale_lower_generation)},
        "mismatch_device": {"hex": artifact_bytes(mismatch_device).hex(), **describe(mismatch_device)},
        "invalid_signature": {"hex": bytes(invalid_signature).hex(), **describe(v1)},
    }
    return {
        "format": "axlic-a2-install-vectors",
        "version": 1,
        "key_id": a0.KEY_ID,
        "public_key_xy_hex": a0.public_xy().hex(),
        "device_id": DEVICE_ID,
        "other_device_id": OTHER_DEVICE_ID,
        "artifacts": artifacts,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--output", type=Path, default=Path("reference/vectors/a2/install.json"))
    args = parser.parse_args()
    if not args.generate:
        parser.error("use --generate")
    corpus = generate_corpus()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(corpus, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {args.output} with {len(corpus['artifacts'])} artifacts")


if __name__ == "__main__":
    main()
