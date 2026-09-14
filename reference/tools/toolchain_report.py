#!/usr/bin/env python3
"""Record reproducible A0 build/toolchain identity."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import platform
import re
import subprocess
import sys


def output(command: list[str]) -> str:
    return subprocess.run(command, check=True, capture_output=True, text=True, errors="replace", shell=False).stdout.strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--implementation-revision", required=True)
    args = parser.parse_args()
    cache = (args.build_dir / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace")
    metadata_files = list((args.build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if len(metadata_files) != 1:
        raise SystemExit("unique CMake compiler metadata missing")
    metadata = metadata_files[0].read_text(encoding="utf-8", errors="replace")
    match = re.search(r'^set\(CMAKE_CXX_COMPILER "(.+)"\)$', metadata, re.MULTILINE)
    if not match:
        raise SystemExit("CMAKE_CXX_COMPILER missing")
    compiler = Path(match.group(1).strip())
    compiler_process = subprocess.run(
        [str(compiler), "/Bv"], check=False, capture_output=True, text=True, errors="replace", shell=False
    )
    compiler_version = compiler_process.stdout + compiler_process.stderr
    if "19.44" not in compiler_version:
        raise SystemExit("frozen MSVC 19.44 compiler not active")
    if not Path(r"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0").is_dir():
        raise SystemExit("Windows SDK 10.0.26100.0 missing")

    cmake_match = re.search(r"^CMAKE_COMMAND:INTERNAL=(.+)$", cache, re.MULTILINE)
    if not cmake_match:
        raise SystemExit("CMAKE_COMMAND missing")
    cmake_command = cmake_match.group(1).strip()
    report = {
        "artifact_id": "AXL-V1-A0-build-toolchain",
        "cmake": output([cmake_command, "--version"]).splitlines()[0],
        "compiler": str(compiler),
        "compiler_family_version": "MSVC 19.44 / toolset 14.44",
        "dependencies": {
            "Catch2": "8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb",
            "nlohmann_json": "55f93686c01528224f448c19128836e7df245f72",
        },
        "implementation_revision": args.implementation_revision,
        "platform": platform.platform(),
        "python": sys.version.split()[0],
        "result": "PASS",
        "windows_sdk": "10.0.26100.0",
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
