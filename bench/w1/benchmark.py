#!/usr/bin/env python3
"""A0 W1 direct-process latency baseline."""

from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import time


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    index = max(0, math.ceil(fraction * len(ordered)) - 1)
    return ordered[index]


def invoke(executable: Path, environment: dict[str, str]) -> float:
    start = time.perf_counter_ns()
    completed = subprocess.run(
        [str(executable), "status"],
        check=False,
        capture_output=True,
        env=environment,
        shell=False,
    )
    elapsed_ms = (time.perf_counter_ns() - start) / 1_000_000.0
    if completed.returncode != 0:
        raise RuntimeError("benchmark invocation failed")
    result = json.loads(completed.stdout)
    if result.get("code") != "OK":
        raise RuntimeError("benchmark status was not OK")
    return elapsed_ms


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--warmup", type=int, default=20)
    parser.add_argument("--samples", type=int, default=200)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()
    if args.warmup < 20 or args.samples < 200:
        raise SystemExit("W1 requires at least 20 warmups and 200 measured invocations")

    environment = os.environ.copy()
    environment["AXLIC_A0_FIXTURE_STATE"] = str(args.state)
    for _ in range(args.warmup):
        invoke(args.exe, environment)
    values = [invoke(args.exe, environment) for _ in range(args.samples)]
    report = {
        "artifact_id": "EV-07-A0-W1-baseline",
        "build_configuration": "Release",
        "command": "axlic status",
        "credential_verify_segment": {"instrumented": False, "reason": "A0 baseline uses whole-process timing"},
        "implementation_revision": args.implementation_revision,
        "machine": os.environ.get("COMPUTERNAME", "unknown"),
        "measured_samples": args.samples,
        "p50_ms": round(percentile(values, 0.50), 3),
        "p95_ms": round(percentile(values, 0.95), 3),
        "p99_ms": round(percentile(values, 0.99), 3),
        "mean_ms": round(statistics.fmean(values), 3),
        "result": "BASELINE_PRODUCED",
        "warmup_samples": args.warmup,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
