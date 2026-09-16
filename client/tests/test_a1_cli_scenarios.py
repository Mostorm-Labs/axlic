#!/usr/bin/env python3
"""Process-level stable A1 identity error-family oracle."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()

    cases = {
        "provider-unavailable": ("IDENTITY_PROVIDER_UNAVAILABLE", "identity", True, 5),
        "provider-unsupported": ("IDENTITY_PROVIDER_UNSUPPORTED", "identity", False, 3),
        "recovery-required": ("IDENTITY_RECOVERY_REQUIRED", "identity", False, 3),
        "state-corrupt": ("LOCAL_STATE_CORRUPT", "local_state", False, 3),
        "privilege-required": ("LOCAL_WRITE_PRIVILEGE_REQUIRED", "local_state", False, 3),
        "state-busy": ("LOCAL_STATE_BUSY", "local_state", True, 5),
    }
    observations = []
    for scenario, expected in cases.items():
        completed = subprocess.run(
            [str(args.exe), scenario], check=False, capture_output=True, text=True, encoding="utf-8", shell=False
        )
        if completed.stderr or len(completed.stdout.splitlines()) != 1:
            raise AssertionError(f"{scenario}: process output contract failed")
        result = json.loads(completed.stdout)
        code, category, retryable, exit_code = expected
        if (result.get("code"), result.get("category"), result.get("retryable"), completed.returncode) != expected:
            raise AssertionError(f"{scenario}: got {result}, exit={completed.returncode}")
        if "data" in result:
            raise AssertionError(f"{scenario}: failure disclosed data")
        observations.append({"id": scenario, "result": "PASS", "code": code})

    report = {
        "artifact_id": "EV-06-A1-identity-error-families",
        "implementation_revision": args.implementation_revision,
        "result": "PASS",
        "cases": observations,
        "failures": [],
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
