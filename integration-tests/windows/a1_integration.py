#!/usr/bin/env python3
"""A1 real-process crash, serialization, and Software KSP evidence runner."""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import tempfile
import uuid


def run(driver: Path, arguments: list[str], expected: set[int] = {0}) -> dict[str, object]:
    completed = subprocess.run(
        [str(driver), *arguments], check=False, capture_output=True, text=True, encoding="utf-8", shell=False
    )
    if completed.returncode not in expected:
        raise AssertionError(
            f"driver {arguments} returned {completed.returncode}: stdout={completed.stdout!r} stderr={completed.stderr!r}"
        )
    if completed.stderr:
        raise AssertionError(f"driver emitted stderr: {completed.stderr!r}")
    if len(completed.stdout.splitlines()) == 1:
        result = json.loads(completed.stdout)
        result["exit_code"] = completed.returncode
        return result
    if completed.returncode != 0 and not completed.stdout:
        return {"exit_code": completed.returncode}
    if len(completed.stdout.splitlines()) != 1:
        raise AssertionError(f"driver output contract violated: {completed.stdout!r} {completed.stderr!r}")
    raise AssertionError("unreachable driver result")


def require_identity(result: dict[str, object], digit: str) -> None:
    if result.get("code") != "OK" or result.get("identity_value") != digit * 128:
        raise AssertionError(f"unexpected authoritative state: {result}")


def crash_matrix(driver: Path, root: Path) -> list[dict[str, str]]:
    observations: list[dict[str, str]] = []
    expected = {
        "before_temp_write": "1",
        "after_temp_write": "1",
        "after_temp_flush": "1",
        "before_replace": "1",
        "after_replace": "2",
    }
    for point, digit in expected.items():
        scenario = root / point
        run(driver, ["state-write", str(scenario), "1", "none"])
        interrupted = run(driver, ["state-write", str(scenario), "2", point], expected={91})
        if interrupted.get("exit_code") != 91:
            raise AssertionError(f"{point}: failpoint did not terminate writer")
        require_identity(run(driver, ["state-read", str(scenario)]), digit)
        observations.append({"id": point, "result": "PASS", "authoritative": "old" if digit == "1" else "new"})
    return observations


def state_negative_matrix(driver: Path, root: Path) -> list[dict[str, str]]:
    scenario = root / "stale-temp"
    run(driver, ["state-write", str(scenario), "1", "none"])
    (scenario / ".machine-state.v1.999.stale.tmp").write_text("partial", encoding="utf-8")
    require_identity(run(driver, ["state-read", str(scenario)]), "1")

    corrupt = root / "corrupt"
    corrupt.mkdir()
    (corrupt / "machine-state.v1.json").write_text("not-json", encoding="utf-8")
    if run(driver, ["state-read", str(corrupt)], expected={3}).get("code") != "LOCAL_STATE_CORRUPT":
        raise AssertionError("malformed outer state did not fail closed")

    unsupported = root / "unsupported"
    unsupported.mkdir()
    (unsupported / "machine-state.v1.json").write_text(
        '{"format":"axlicense-machine-state","envelope_version":2,"protected_blob_hex":"00"}', encoding="utf-8"
    )
    if run(driver, ["state-read", str(unsupported)], expected={3}).get("code") != "LOCAL_STATE_UNSUPPORTED":
        raise AssertionError("future envelope did not remain unsupported")

    oversized = root / "oversized"
    oversized.mkdir()
    (oversized / "machine-state.v1.json").write_bytes(b"x" * (1024 * 1024 + 1))
    if run(driver, ["state-read", str(oversized)], expected={3}).get("code") != "LOCAL_STATE_CORRUPT":
        raise AssertionError("oversized state did not fail before decode")

    return [
        {"id": "stale-temp-ignored", "result": "PASS"},
        {"id": "malformed-envelope", "result": "PASS"},
        {"id": "unsupported-envelope", "result": "PASS"},
        {"id": "oversized-envelope", "result": "PASS"},
    ]


def software_ksp_concurrency(driver: Path, root: Path) -> list[dict[str, str]]:
    state_root = root / "software-concurrency"
    lock_name = "Local\\AxLicense.A1.Integration." + uuid.uuid4().hex
    before_count = run(driver, ["count-software-keys"]).get("count")
    commands = [[str(driver), "identity-software", str(state_root), lock_name] for _ in range(2)]
    processes = [subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8") for command in commands]
    results: list[dict[str, object]] = []
    for process in processes:
        stdout, stderr = process.communicate(timeout=60)
        if process.returncode != 0 or stderr or len(stdout.splitlines()) != 1:
            raise AssertionError(f"software KSP process failed: rc={process.returncode} stdout={stdout!r} stderr={stderr!r}")
        results.append(json.loads(stdout))
    identities = {result.get("identity_value") for result in results}
    if len(identities) != 1 or any(result.get("code") != "OK" for result in results):
        raise AssertionError(f"concurrent establishment did not converge: {results}")
    if any("key_name" in result or "provider_name" in result for result in results):
        raise AssertionError("provider internals leaked from qualification output")
    after_count = run(driver, ["count-software-keys"]).get("count")
    if not isinstance(before_count, int) or after_count != before_count + 1:
        raise AssertionError(f"concurrent establishment created an unexpected number of keys: {before_count} -> {after_count}")
    run(driver, ["cleanup-key", str(state_root)])
    if run(driver, ["count-software-keys"]).get("count") != before_count:
        raise AssertionError("Software KSP qualification key cleanup failed")
    return [
        {"id": "software-ksp-persist-reopen-sign-verify-nonexport", "result": "PASS"},
        {"id": "cross-process-first-establishment-converges", "result": "PASS"},
    ]


def protected_acl(driver: Path, root: Path) -> list[dict[str, str]]:
    state_root = root / "protected-acl"
    run(driver, ["prepare-protected-root", str(state_root)])
    completed = subprocess.run(
        [
            "powershell",
            "-NoProfile",
            "-NonInteractive",
            "-Command",
            "& { param($p) (Get-Acl -LiteralPath $p).Sddl }",
            str(state_root),
        ],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        shell=False,
    )
    sddl = completed.stdout.strip()
    required = ("D:P", "(A;OICI;FA;;;SY)", "(A;OICI;FA;;;BA)", "(A;OICI;GRGX;;;BU)")
    if any(fragment not in sddl for fragment in required):
        raise AssertionError(f"protected root DACL does not match the frozen principal/access contract: {sddl}")
    return [{"id": "protected-programdata-equivalent-dacl", "result": "PASS"}]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--implementation-revision", default="WORKTREE")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--require-software-ksp", action="store_true")
    args = parser.parse_args()

    cases: list[dict[str, str]] = []
    with tempfile.TemporaryDirectory(prefix="axlic-a1-integration-") as directory:
        root = Path(directory)
        cases.extend(crash_matrix(args.driver, root))
        cases.extend(state_negative_matrix(args.driver, root))
        elevated = bool(ctypes.windll.shell32.IsUserAnAdmin())
        if elevated:
            cases.extend(protected_acl(args.driver, root))
            cases.extend(software_ksp_concurrency(args.driver, root))
        elif args.require_software_ksp:
            raise AssertionError("real Software KSP qualification requires an elevated Windows runner")
        else:
            cases.append({"id": "software-ksp-hosted-only", "result": "NOT_RUN_LOCAL_PRIVILEGE"})

    report = {
        "artifact_id": "EV-03-EV-04-A1-windows-integration",
        "implementation_revision": args.implementation_revision,
        "execution_environment": "Windows real-process",
        "result": "PASS",
        "cases": cases,
        "failures": [],
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
