#!/usr/bin/env python3
"""Compile exact A1 CTest observations into reviewer-facing evidence inputs."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import xml.etree.ElementTree as ET


GROUPS = {
    "provider-policy": (
        "EV-03-A1-provider-policy",
        ("TPM", "fallback", "committed software", "CNG status", "missing committed"),
    ),
    "protected-state": (
        "EV-04-A1-protected-state",
        ("machine state", "DPAPI", "atomic replace", "state root", "mutation lock"),
    ),
    "a0-regression": (
        "A1-inherited-A0-regression",
        ("credential", "canonical", "production verifier", "a0_cli", "a0_offline", "entitlement"),
    ),
}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--junit", type=Path, required=True)
    parser.add_argument("--implementation-revision", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    root = ET.parse(args.junit).getroot()
    tests = []
    for case in root.iter("testcase"):
        tests.append(
            {
                "name": case.attrib.get("name", ""),
                "result": "FAIL" if case.find("failure") is not None or case.find("error") is not None else "PASS",
            }
        )

    args.output_dir.mkdir(parents=True, exist_ok=True)
    for filename, (artifact_id, needles) in GROUPS.items():
        selected = [case for case in tests if any(needle.lower() in case["name"].lower() for needle in needles)]
        if not selected:
            raise SystemExit(f"no JUnit cases matched {filename}")
        failures = [case["name"] for case in selected if case["result"] != "PASS"]
        report = {
            "artifact_id": artifact_id,
            "implementation_revision": args.implementation_revision,
            "source": str(args.junit),
            "result": "FAIL" if failures else "PASS",
            "cases": selected,
            "failures": failures,
        }
        (args.output_dir / f"{filename}.json").write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        if failures:
            raise SystemExit(f"{filename} contains failures: {failures}")


if __name__ == "__main__":
    main()
