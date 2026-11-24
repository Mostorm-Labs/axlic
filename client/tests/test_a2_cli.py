#!/usr/bin/env python3
"""Process-level A2 library-plane contract oracle (host CLI surface).

Drives the scenario executable (same command implementations as the
production CLI, injected temporary state root) through the full registration
/ install / held-version lifecycle and asserts the JSON envelope, exit-code
family and output hygiene. Always uses direct process creation.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from typing import Any


def run(executable: Path, argv: list[str]) -> tuple[int, dict[str, Any]]:
    completed = subprocess.run(
        [str(executable), *argv],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        env=os.environ.copy(),
        shell=False,
    )
    lines = completed.stdout.splitlines()
    if len(lines) != 1 or not completed.stdout.endswith("\n"):
        raise AssertionError(f"stdout must be exactly one JSON document: {completed.stdout!r}")
    if completed.stderr:
        raise AssertionError(f"stderr must be empty: {completed.stderr!r}")
    return completed.returncode, json.loads(lines[0])


def require_envelope(result: dict[str, Any]) -> None:
    required = {"contract_version", "ok", "code", "category", "retryable", "correlation_ref"}
    if not required.issubset(result):
        raise AssertionError(f"missing CommandResult fields: {sorted(required - set(result))}")
    if result["contract_version"] != "1.0":
        raise AssertionError("wrong CLI contract version")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()
    corpus = json.loads(args.vectors.read_text(encoding="utf-8"))

    observations: list[dict[str, Any]] = []
    failures: list[str] = []

    def observe(case_id: str, argv_tail: list[str], expected_code: str, expected_ok: bool) -> dict[str, Any]:
        returncode, result = run(args.exe, argv_tail)
        require_envelope(result)
        if result["code"] != expected_code or result["ok"] is not expected_ok:
            raise AssertionError(f"{case_id}: unexpected result {result} (rc={returncode})")
        if expected_ok and returncode != 0:
            raise AssertionError(f"{case_id}: success returned {returncode}")
        if not expected_ok and returncode == 0:
            raise AssertionError(f"{case_id}: failure returned zero")
        # Keep the observable data in the scan set so the leak check below
        # covers the actual command output, not just locally built constants.
        observations.append(
            {"id": case_id, "result": "PASS", "code": expected_code, "exit_code": returncode,
             "data": result.get("data")}
        )
        return result

    with tempfile.TemporaryDirectory(prefix="axlic-a2-cli-") as directory:
        root = Path(directory)
        floor = root / "floor.json"
        floor.write_text(
            json.dumps(
                {
                    "format": "axlic-trust-floor",
                    "version": 1,
                    "keys": [
                        {"key_id": corpus["key_id"], "public_key_xy_hex": corpus["public_key_xy_hex"]}
                    ],
                }
            ),
            encoding="utf-8",
        )
        artifacts = root / "artifacts"
        artifacts.mkdir()
        artifact_paths = {
            name: artifacts / f"{name}.cbor"
            for name in (
                "v1",
                "v2_new",
                "clear",
                "stale_lower_revision",
                "stale_lower_generation",
                "mismatch_device",
                "invalid_signature",
            )
        }
        for name, path in artifact_paths.items():
            path.write_bytes(bytes.fromhex(corpus["artifacts"][name]["hex"]))
        device_id = corpus["device_id"]
        base = [str(root), "--floor", str(floor)]

        # Empty machine: no credential anywhere.
        observe("status-absent", [*base, "status"], "CREDENTIAL_NOT_FOUND", False)
        observe("held-version-empty", [*base, "held-version"], "OK", True)
        observe("device-register-no-identity", [*base, "device-register"], "IDENTITY_NOT_FOUND", False)

        # Establish identity, then the self-report is identity_value.
        if run(args.exe, [str(root), "seed-identity"])[0] != 0:
            raise AssertionError("seed-identity failed")
        report = observe("device-register-unregistered", [*base, "device-register"], "OK", True)
        if report["data"] != {"registered": False, "self_report_value": "c" * 128}:
            raise AssertionError(f"unexpected unregistered self-report: {report['data']}")

        # Install before registration is rejected.
        observe(
            "install-unregistered",
            [*base, "install", str(artifact_paths["v1"]), "--policy-version", "2"],
            "CREDENTIAL_DEVICE_MISMATCH",
            False,
        )

        # Adopt the platform reference.
        observe("adopt", [*base, "device-register", "adopt", device_id], "OK", True)
        report = observe("device-register-registered", [*base, "device-register"], "OK", True)
        if report["data"] != {"registered": True, "self_report_value": device_id, "device_id": device_id}:
            raise AssertionError(f"unexpected registered self-report: {report['data']}")
        observe("adopt-invalid-format", [*base, "device-register", "adopt", "not-a-reference"], "COMMAND_INVALID", False)

        # Verification/binding failures.
        observe(
            "install-mismatch-device",
            [*base, "install", str(artifact_paths["mismatch_device"]), "--policy-version", "1"],
            "CREDENTIAL_DEVICE_MISMATCH",
            False,
        )
        observe(
            "install-invalid-signature",
            [*base, "install", str(artifact_paths["invalid_signature"]), "--policy-version", "1"],
            "CREDENTIAL_INVALID",
            False,
        )

        # First install, then read the held surfaces.
        installed = observe(
            "install-v1", [*base, "install", str(artifact_paths["v1"]), "--policy-version", "2"], "OK", True
        )
        if installed["data"]["result"] != "OK" or installed["data"]["authority_revision"] != 3:
            raise AssertionError(f"unexpected install data: {installed['data']}")
        status = observe("status-valid", [*base, "status"], "OK", True)
        if status["data"] != {"credential_state": "valid", "authority_revision": 3, "credential_generation": 7}:
            raise AssertionError(f"unexpected valid status data: {status['data']}")
        held = observe("held-version-v1", [*base, "held-version"], "OK", True)
        if held["data"] != {
            "hasCredential": True,
            "version": {"authorityRevision": 3, "credentialGeneration": 7, "policyVersion": 2},
        }:
            raise AssertionError(f"unexpected held version: {held['data']}")
        granted = observe(
            "entitlement-granted", [*base, "entitlement", "nearhub.runtime", "100"], "OK", True
        )
        if granted["data"] != {"entitlement_id": "nearhub.runtime", "granted": True}:
            raise AssertionError(f"unexpected entitlement data: {granted['data']}")

        # Anti-rollback on both axes.
        observe(
            "install-stale-lower-revision",
            [*base, "install", str(artifact_paths["stale_lower_revision"]), "--policy-version", "1"],
            "CREDENTIAL_STALE",
            False,
        )
        observe(
            "install-stale-lower-generation",
            [*base, "install", str(artifact_paths["stale_lower_generation"]), "--policy-version", "1"],
            "CREDENTIAL_STALE",
            False,
        )

        # Same version is an idempotent reinstall refreshing policyVersion.
        idempotent = observe(
            "install-idempotent",
            [*base, "install", str(artifact_paths["v1"]), "--policy-version", "4"],
            "OK",
            True,
        )
        if idempotent["data"]["result"] != "OK_IDEMPOTENT":
            raise AssertionError(f"unexpected idempotent result: {idempotent['data']}")
        held = observe("held-version-idempotent", [*base, "held-version"], "OK", True)
        if held["data"]["version"]["policyVersion"] != 4:
            raise AssertionError(f"policyVersion not refreshed: {held['data']}")

        # Numeric and artifact input boundaries are rejected consistently on
        # both CLI surfaces; a 20-digit UINT64_MAX stays legal.
        observe(
            "install-policy-version-zero",
            [*base, "install", str(artifact_paths["v1"]), "--policy-version", "0"],
            "COMMAND_INVALID",
            False,
        )
        observe(
            "install-policy-version-overflow",
            [*base, "install", str(artifact_paths["v1"]), "--policy-version", "99999999999999999999"],
            "COMMAND_INVALID",
            False,
        )
        max_install = observe(
            "install-policy-version-max",
            [*base, "install", str(artifact_paths["v1"]), "--policy-version", "18446744073709551615"],
            "OK",
            True,
        )
        if max_install["data"]["result"] != "OK_IDEMPOTENT":
            raise AssertionError(f"UINT64_MAX should be an idempotent reinstall: {max_install['data']}")
        held = observe("held-version-max", [*base, "held-version"], "OK", True)
        if held["data"]["version"]["policyVersion"] != 18446744073709551615:
            raise AssertionError(f"UINT64_MAX policy version not persisted: {held['data']}")
        observe(
            "install-artifact-missing",
            [*base, "install", str(artifacts / "missing.cbor"), "--policy-version", "1"],
            "CREDENTIAL_INVALID",
            False,
        )
        empty_artifact = artifacts / "empty.cbor"
        empty_artifact.write_bytes(b"")
        observe(
            "install-artifact-empty",
            [*base, "install", str(empty_artifact), "--policy-version", "1"],
            "CREDENTIAL_INVALID",
            False,
        )

        # Newer revision, then the revocation credential clears entitlements.
        observe(
            "install-v2-new", [*base, "install", str(artifact_paths["v2_new"]), "--policy-version", "3"], "OK", True
        )
        held = observe("held-version-v2", [*base, "held-version"], "OK", True)
        if held["data"]["version"] != {
            "authorityRevision": 4,
            "credentialGeneration": 8,
            "policyVersion": 3,
        }:
            raise AssertionError(f"unexpected v2 held version: {held['data']}")
        observe("install-clear", [*base, "install", str(artifact_paths["clear"]), "--policy-version", "5"], "OK", True)
        status = observe("status-cleared", [*base, "status"], "OK", True)
        if status["data"]["credential_state"] != "valid" or status["data"]["authority_revision"] != 5:
            raise AssertionError(f"unexpected cleared status: {status['data']}")
        denied = observe(
            "entitlement-cleared", [*base, "entitlement", "nearhub.runtime", "100"], "OK", True
        )
        if denied["data"]["granted"] is not False:
            raise AssertionError(f"revocation credential still grants: {denied['data']}")

        # Terminal-rejection reset falls back to identity_value; credential kept.
        observe("reset", [*base, "device-register", "reset"], "OK", True)
        report = observe("device-register-after-reset", [*base, "device-register"], "OK", True)
        if report["data"] != {"registered": False, "self_report_value": "c" * 128}:
            raise AssertionError(f"unexpected post-reset self-report: {report['data']}")
        observe("held-version-after-reset", [*base, "held-version"], "OK", True)

        # Command surface hygiene.
        returncode, result = run(args.exe, [str(root), "bogus-command"])
        if returncode != 2 or result["code"] != "COMMAND_INVALID":
            raise AssertionError(f"scenario must reject unknown commands: {returncode} {result}")

        combined = json.dumps(observations)
        forbidden = [corpus["public_key_xy_hex"], str(root)]
        if any(value.lower() in combined.lower() for value in forbidden):
            raise AssertionError("forbidden material (trust key / state paths) leaked into observations")

    report = {
        "artifact_id": "EV-A2-04-cli-contract",
        "authority_revision": "P20-v3-A2",
        "command_or_runner": "python client/tests/test_a2_cli.py",
        "execution_environment": "Windows direct process creation",
        "failures": failures,
        "fixture_or_corpus_id": corpus["format"] + "-v" + str(corpus["version"]),
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
