#!/usr/bin/env python3
"""EV-A2-05 deliverable scan: no external repository references.

Greps deliverable source surfaces for external repository hosts/URLs, the
check registered as part of EV-A2-05's exact execution (P20 v3 section 17)
and enforced by the A2 forbidden-changes list. The frozen dependency pins
(nlohmann/QCBOR/Catch2 FetchContent URLs) are whitelisted for
client/CMakeLists.txt only. Fails closed: any non-whitelisted hit exits
non-zero.

Usage:
  python tools/check_external_references.py \
      [--report evidence/a2-no-external-references.json] \
      [--implementation-revision SHA]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

SCAN_ROOTS = [
    "client/src",
    "client/include",
    "client/app",
    "client/tests",
    "client/CMakeLists.txt",
    "tools",
    "reference/tools",
]

# External repository indicators, matched case-insensitively.
PATTERNS = [
    r"github\.com/",
    r"raw\.githubusercontent\.com",
    r"codeload\.github\.com",
    r"gitlab\.com/",
    r"bitbucket\.org/",
    r"git\+https?://",
    r"git@[A-Za-z0-9.-]+:",
]

# The frozen dependency pins legitimately download from codeload.github.com;
# allowed only inside the CMake manifest that declares them.
WHITELIST_PATTERNS = {
    pathlib.PurePosixPath("client/CMakeLists.txt"): r"codeload\.github\.com",
}

# This scanner necessarily contains the patterns it looks for.
EXCLUDED_FILES = {pathlib.PurePosixPath("tools/check_external_references.py")}


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", type=pathlib.Path)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    args = parser.parse_args(argv)

    repository = pathlib.Path(__file__).resolve().parent.parent
    findings: list[dict[str, object]] = []
    allowed: list[dict[str, object]] = []
    scanned = 0
    for root in SCAN_ROOTS:
        base = repository / root
        if base.is_file():
            paths = [base]
        elif base.is_dir():
            paths = sorted(base.rglob("*"))
        else:
            print(f"check_external_references: scan root missing: {root}", file=sys.stderr)
            return 1
        for path in paths:
            if not path.is_file():
                continue
            relative = pathlib.PurePosixPath(path.relative_to(repository).as_posix())
            if relative in EXCLUDED_FILES:
                continue
            scanned += 1
            text = path.read_text(encoding="utf-8", errors="replace")
            whitelist_pattern = WHITELIST_PATTERNS.get(relative)
            for lineno, line in enumerate(text.split("\n"), 1):
                for pattern in PATTERNS:
                    for match in re.finditer(pattern, line, re.IGNORECASE):
                        entry = {"file": str(relative), "line": lineno, "match": match.group(0)}
                        # A whitelisted file is forgiven only on lines that
                        # actually carry the frozen pin URL, so unrelated
                        # references in it still fail.
                        if whitelist_pattern is not None and re.search(whitelist_pattern, line, re.IGNORECASE):
                            allowed.append(entry)
                        else:
                            findings.append(entry)

    report = {
        "artifact_id": "EV-A2-05-no-external-references",
        "authority_revision": "P20-v3-A2",
        "command_or_runner": "python tools/check_external_references.py",
        "implementation_revision": args.implementation_revision,
        "scanned_files": scanned,
        "findings": findings,
        "whitelisted": allowed,
        "result": "FAIL" if findings else "PASS",
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    if findings:
        print(f"check_external_references: {len(findings)} external reference(s) found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
