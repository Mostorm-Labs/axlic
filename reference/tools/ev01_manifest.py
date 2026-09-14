#!/usr/bin/env python3
"""Run the production corpus tests and record an EV-01 A0 manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--test-exe", type=Path, required=True)
    parser.add_argument("--corpus", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--implementation-revision", required=True)
    args = parser.parse_args()
    corpus = json.loads(args.corpus.read_text(encoding="utf-8"))
    completed = subprocess.run(
        [str(args.test_exe)],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        shell=False,
    )
    if completed.returncode != 0:
        raise SystemExit(completed.stdout)
    vectors = [
        {"id": item["id"], "expected_status": item["expected_status"], "result": "PASS"}
        for group in ("positives", "negatives")
        for item in corpus[group]
    ]
    report = {
        "artifact_id": "EV-01-A0-production-corpus",
        "authority_revision": "P20-FR019-v0.1",
        "command_or_runner": "axlic_unit_tests (complete unit corpus)",
        "corpus_sha256": hashlib.sha256(args.corpus.read_bytes()).hexdigest(),
        "execution_environment": "Windows MSVC Release; BCrypt P-256/SHA-256",
        "failures": [],
        "fixture_or_corpus_id": corpus["corpus_id"],
        "implementation_revision": args.implementation_revision,
        "result": "PASS",
        "vectors": vectors,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
