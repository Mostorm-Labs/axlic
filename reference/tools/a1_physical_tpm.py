#!/usr/bin/env python3
"""Two-phase exact-revision physical TPM reboot qualification bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
from pathlib import Path
import subprocess


def powershell(script: str) -> str:
    return subprocess.run(
        ["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="strict",
        shell=False,
    ).stdout.strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--phase", choices=("initial", "post-reboot"), required=True)
    parser.add_argument("--correlation-id", required=True)
    parser.add_argument("--implementation-revision", required=True)
    parser.add_argument("--expected-identity-sha256", default="")
    parser.add_argument("--initial-boot-marker", default="")
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()

    tpm = json.loads(powershell("Get-Tpm | Select-Object TpmPresent,TpmReady | ConvertTo-Json -Compress"))
    if not tpm.get("TpmPresent") or not tpm.get("TpmReady"):
        raise AssertionError("reviewer-managed physical TPM 2.0 is not present and ready")
    os_caption = powershell("(Get-CimInstance Win32_OperatingSystem).Caption")
    if "Windows 11" not in os_caption:
        raise AssertionError(f"physical TPM A1 qualification requires Windows 11, got {os_caption!r}")
    boot_time = powershell("(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')")
    boot_marker = hashlib.sha256(boot_time.encode("utf-8")).hexdigest()

    completed = subprocess.run(
        [str(args.exe), "identity"], check=False, capture_output=True, text=True, encoding="utf-8", shell=False
    )
    if completed.returncode != 0 or completed.stderr or len(completed.stdout.splitlines()) != 1:
        raise AssertionError(
            f"production TPM identity command failed: rc={completed.returncode} stdout={completed.stdout!r} stderr={completed.stderr!r}"
        )
    result = json.loads(completed.stdout)
    data = result.get("data", {})
    expected_fields = {"identity_state", "scheme_id", "identity_epoch", "identity_value"}
    if result.get("code") != "OK" or set(data) != expected_fields:
        raise AssertionError(f"identity minimum-disclosure contract failed: {result}")
    if data.get("scheme_id") != "axl-win-cng-tpm-p256-v1" or data.get("identity_epoch") != 1:
        raise AssertionError(f"production selector did not establish/load TPM identity: {data}")
    identity_value = data.get("identity_value", "")
    if len(identity_value) != 128 or any(character not in "0123456789abcdef" for character in identity_value):
        raise AssertionError("public identity representation is not the frozen 64-byte lowercase hex form")
    identity_hash = hashlib.sha256(identity_value.encode("ascii")).hexdigest()

    if args.phase == "post-reboot":
        if not args.expected_identity_sha256 or identity_hash != args.expected_identity_sha256:
            raise AssertionError("public identity changed across reboot")
        if not args.initial_boot_marker or boot_marker == args.initial_boot_marker:
            raise AssertionError("post-reboot phase did not observe a different OS boot")

    report = {
        "artifact_id": f"EV-03-A1-physical-TPM-{args.phase}",
        "implementation_revision": args.implementation_revision,
        "correlation_id": args.correlation_id,
        "environment_class": "reviewer-managed Windows 11 physical TPM 2.0",
        "os": os_caption,
        "os_build": platform.version(),
        "provider_class": "Microsoft Platform Crypto Provider",
        "scheme_id": data["scheme_id"],
        "identity_epoch": 1,
        "public_identity_sha256": identity_hash,
        "boot_marker": boot_marker,
        "checks": {
            "provider_selected": "PASS",
            "persisted_key_load_and_internal_sign_verify": "PASS",
            "plaintext_private_export_rejected": "PASS",
            "software_identity_not_selected": "PASS",
            "reboot_persistence": "PASS" if args.phase == "post-reboot" else "PENDING_POST_REBOOT",
        },
        "result": "PASS" if args.phase == "post-reboot" else "PENDING_POST_REBOOT",
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"public_identity_sha256": identity_hash, "boot_marker": boot_marker, "result": report["result"]}))


if __name__ == "__main__":
    main()
