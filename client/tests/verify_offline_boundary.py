#!/usr/bin/env python3
"""Verify that the A0 executable has no server/network runtime dependency."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args()

    compiler = args.compiler
    if compiler is None:
        if args.build_dir is None:
            parser.error("provide --compiler or --build-dir")
        metadata_files = list((args.build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
        if len(metadata_files) != 1:
            raise SystemExit("unique CMake compiler metadata missing")
        metadata = metadata_files[0].read_text(encoding="utf-8", errors="replace")
        match = re.search(r'^set\(CMAKE_CXX_COMPILER "(.+)"\)$', metadata, re.MULTILINE)
        if not match:
            raise SystemExit("CMAKE_CXX_COMPILER missing")
        compiler = Path(match.group(1).strip())
    dumpbin = compiler.with_name("dumpbin.exe")
    dependencies = subprocess.run(
        [str(dumpbin), "/DEPENDENTS", str(args.exe)],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        shell=False,
    ).stdout
    forbidden = ["winhttp.dll", "wininet.dll", "ws2_32.dll", "libcurl", "node.exe", "postgres"]
    discovered = [name for name in forbidden if name in dependencies.lower()]
    if discovered:
        raise AssertionError(f"network/server dependencies detected: {discovered}")

    environment = os.environ.copy()
    environment["AXLIC_A0_FIXTURE_STATE"] = str(args.state)
    environment["HTTP_PROXY"] = "http://127.0.0.1:9"
    environment["HTTPS_PROXY"] = "http://127.0.0.1:9"
    for name in ("AXLIC_SERVER", "DATABASE_URL", "PGHOST", "PGPORT"):
        environment.pop(name, None)
    completed = subprocess.run(
        [str(args.exe), "status"],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        env=environment,
        shell=False,
    )
    result = json.loads(completed.stdout)
    if completed.returncode != 0 or result.get("code") != "OK" or result.get("data", {}).get("credential_state") != "valid":
        raise AssertionError("local status failed without server configuration")

    report = {
        "artifact_id": "T-A0-05-offline-no-server",
        "authority_revision": "P20-FR019-v0.1",
        "command_or_runner": "python client/tests/verify_offline_boundary.py",
        "execution_environment": "Windows; unreachable proxy; no server/database configuration",
        "failures": [],
        "forbidden_imports_found": [],
        "implementation_revision": args.implementation_revision,
        "result": "PASS",
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
