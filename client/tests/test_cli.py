#!/usr/bin/env python3
"""Process-level A0 CLI contract oracle. Always uses direct process creation."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from typing import Any


def run(executable: Path, args: list[str], state_path: Path | None) -> tuple[int, dict[str, Any], str, str]:
    environment = os.environ.copy()
    if state_path is None:
        environment.pop("AXLIC_A0_FIXTURE_STATE", None)
    else:
        environment["AXLIC_A0_FIXTURE_STATE"] = str(state_path)
    completed = subprocess.run(
        [str(executable), *args],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        env=environment,
        shell=False,
    )
    lines = completed.stdout.splitlines()
    if len(lines) != 1 or not completed.stdout.endswith("\n"):
        raise AssertionError(f"stdout must be exactly one newline-terminated JSON document: {completed.stdout!r}")
    if completed.stderr:
        raise AssertionError(f"stderr must be empty: {completed.stderr!r}")
    return completed.returncode, json.loads(lines[0]), completed.stdout, completed.stderr


def require_envelope(result: dict[str, Any]) -> None:
    required = {"contract_version", "ok", "code", "category", "retryable", "correlation_ref"}
    if not required.issubset(result):
        raise AssertionError(f"missing CommandResult fields: {sorted(required - set(result))}")
    if result["contract_version"] != "1.0":
        raise AssertionError("wrong CLI contract version")


def write_state(path: Path, vector: dict[str, Any], corpus: dict[str, Any]) -> None:
    public_key = vector.get("trusted_public_key_xy_hex", corpus["public_key_xy_hex"])
    state = {
        "state": "present",
        "artifact_hex": vector["artifact_hex"],
        "trusted_keys": [{"key_id": corpus["key_id"], "public_key_xy_hex": public_key}],
    }
    path.write_text(json.dumps(state), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--valid-state", type=Path, required=True)
    parser.add_argument("--absent-state", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()
    corpus = json.loads(args.vectors.read_text(encoding="utf-8"))
    observations: list[dict[str, Any]] = []

    def observe(case_id: str, argv: list[str], state: Path | None, expected_code: str, expected_ok: bool) -> dict[str, Any]:
        returncode, result, stdout, stderr = run(args.exe, argv, state)
        require_envelope(result)
        if result["code"] != expected_code or result["ok"] is not expected_ok:
            raise AssertionError(f"{case_id}: unexpected result {result}")
        if expected_ok and returncode != 0:
            raise AssertionError(f"{case_id}: success returned {returncode}")
        if not expected_ok and returncode == 0:
            raise AssertionError(f"{case_id}: failure returned zero")
        observations.append({"id": case_id, "result": "PASS", "code": expected_code})
        return result

    valid = observe("status-valid", ["status"], args.valid_state, "OK", True)
    if valid.get("data") != {"credential_state": "valid", "authority_revision": 8, "credential_generation": 4}:
        raise AssertionError(f"unexpected valid status data: {valid.get('data')}")

    bounded = observe(
        "entitlement-bounded", ["entitlement", "nearhub.concurrent_sources"], args.valid_state, "OK", True
    )
    if bounded.get("data") != {
        "entitlement_id": "nearhub.concurrent_sources",
        "granted": True,
        "value_u64": 4,
    }:
        raise AssertionError(f"unexpected bounded entitlement data: {bounded.get('data')}")

    absent_entitlement = observe(
        "entitlement-absent", ["entitlement", "nearhub.not_granted"], args.valid_state, "OK", True
    )
    if absent_entitlement.get("data") != {"entitlement_id": "nearhub.not_granted", "granted": False}:
        raise AssertionError(f"unexpected absent entitlement data: {absent_entitlement.get('data')}")

    absent = observe("status-credential-absent", ["status"], args.absent_state, "CREDENTIAL_NOT_FOUND", False)
    if absent.get("data") != {"credential_state": "absent"}:
        raise AssertionError(f"unexpected absent state: {absent.get('data')}")

    observe("command-invalid", ["unknown"], args.valid_state, "COMMAND_INVALID", False)

    with tempfile.TemporaryDirectory(prefix="axlic-a0-cli-") as directory:
        root = Path(directory)
        invalid_vector = next(item for item in corpus["negatives"] if item["id"] == "payload-one-bit-tamper")
        invalid_state = root / "invalid.json"
        write_state(invalid_state, invalid_vector, corpus)
        invalid = observe("status-invalid", ["status"], invalid_state, "CREDENTIAL_INVALID", False)
        if invalid.get("data") != {"credential_state": "invalid"}:
            raise AssertionError(f"unexpected invalid state: {invalid.get('data')}")

        unsupported_vector = next(item for item in corpus["negatives"] if item["id"] == "wrong-algorithm-id")
        unsupported_state = root / "unsupported.json"
        write_state(unsupported_state, unsupported_vector, corpus)
        unsupported = observe("status-unsupported", ["status"], unsupported_state, "CREDENTIAL_UNSUPPORTED", False)
        if unsupported.get("data") != {"credential_state": "invalid"}:
            raise AssertionError(f"unexpected unsupported state: {unsupported.get('data')}")

    combined_output = json.dumps(observations)
    forbidden = [
        str(args.valid_state),
        "519b423d715f8b5d5499f8e3aef260765c1f1f5c1cc53f3f8c86c1f11b2e6a7d",
        corpus["positives"][0]["signature_hex"],
        corpus["positives"][0]["payload_hex"],
    ]
    if any(value.lower() in combined_output.lower() for value in forbidden):
        raise AssertionError("forbidden fixture material leaked into CLI observations")

    report = {
        "artifact_id": "EV-06-A0-cli-contract",
        "authority_revision": "P20-FR019-v0.1",
        "command_or_runner": "python client/tests/test_cli.py",
        "execution_environment": "Windows direct process creation",
        "failures": [],
        "fixture_or_corpus_id": corpus["corpus_id"],
        "implementation_revision": args.implementation_revision,
        "result": "PASS",
        "cases": observations,
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
