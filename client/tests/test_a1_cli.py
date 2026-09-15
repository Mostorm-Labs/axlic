#!/usr/bin/env python3
"""Process-level A1 production CLI boundary checks."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess


def invoke(executable: Path, arguments: list[str], environment: dict[str, str]) -> tuple[int, dict[str, object], str]:
    completed = subprocess.run(
        [str(executable), *arguments],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        env=environment,
        shell=False,
    )
    if completed.stderr:
        raise AssertionError(f"stderr must be empty: {completed.stderr!r}")
    if not completed.stdout.endswith("\n") or len(completed.stdout.splitlines()) != 1:
        raise AssertionError(f"stdout must be exactly one JSON object: {completed.stdout!r}")
    return completed.returncode, json.loads(completed.stdout), completed.stdout


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--a0-state", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()

    environment = os.environ.copy()
    environment["AXLIC_A0_FIXTURE_STATE"] = str(args.a0_state)
    status_code, status, status_stdout = invoke(args.exe, ["status"], environment)
    if status_code == 0 or status.get("code") != "CREDENTIAL_NOT_FOUND":
        raise AssertionError(f"production status consumed the A0 fixture seam: {status}")

    invalid_code, invalid, invalid_stdout = invoke(args.exe, ["identity", "unexpected"], environment)
    if invalid_code == 0 or invalid.get("code") != "COMMAND_INVALID":
        raise AssertionError(f"identity argument contract drifted: {invalid}")

    combined = status_stdout + invalid_stdout
    forbidden = [str(args.a0_state), "Auditoryworks.AxLicense.Identity.v1.", "Microsoft Platform Crypto Provider"]
    if any(value in combined for value in forbidden):
        raise AssertionError("production CLI disclosed fixture/provider internals")

    report = {
        "artifact_id": "EV-06-A1-production-cli-boundary",
        "implementation_revision": args.implementation_revision,
        "result": "PASS",
        "cases": [
            {"id": "a0-fixture-env-ignored", "result": "PASS"},
            {"id": "identity-arguments-bounded", "result": "PASS"},
            {"id": "minimum-disclosure-failure-output", "result": "PASS"},
        ],
        "failures": [],
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
