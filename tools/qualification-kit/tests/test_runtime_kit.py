#!/usr/bin/env python3
"""Behavior tests for the runtime-only physical TPM qualification kit."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import zipfile


SOURCE_REVISION = "a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25"
PACKAGING_REVISION = "b" * 40
REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
KIT_TOOLS = REPOSITORY_ROOT / "tools" / "qualification-kit"
POWERSHELL = Path(os.environ["SystemRoot"]) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"


def run(command: list[str], *, env: dict[str, str] | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(command, check=False, capture_output=True, text=True, encoding="utf-8", env=env)
    if check and completed.returncode != 0:
        raise AssertionError(
            f"command returned {completed.returncode}: {command!r}\nstdout={completed.stdout!r}\nstderr={completed.stderr!r}"
        )
    return completed


class RuntimeKitContractTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="axlic-runtime-kit-")
        self.root = Path(self.temporary.name)
        self.inputs = self.root / "inputs"
        self.inputs.mkdir()
        self.axlic = self.inputs / "axlic.exe"
        self.axlic.write_bytes(b"fixture-axlic-binary")
        self.python_runtime = self.inputs / "python-3.13.15-embed-amd64"
        self.python_runtime.mkdir()
        (self.python_runtime / "python.exe").write_bytes(b"fixture-python-binary")
        (self.python_runtime / "python313.zip").write_bytes(b"fixture-python-stdlib")
        self.vc_runtime = self.inputs / "Microsoft.VC143.CRT"
        self.vc_runtime.mkdir()
        (self.vc_runtime / "vcruntime140.dll").write_bytes(b"fixture-vcruntime")
        (self.vc_runtime / "msvcp140.dll").write_bytes(b"fixture-msvcp")
        self.dependencies = self.inputs / "runtime-dependencies.txt"
        self.dependencies.write_text("KERNEL32.dll\nVCRUNTIME140.dll\n", encoding="utf-8")
        self.output = self.root / "output"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def assemble(self) -> tuple[dict[str, object], Path]:
        completed = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(KIT_TOOLS / "New-RuntimeKit.ps1"),
                "-AxlicExe",
                str(self.axlic),
                "-PythonRuntimeDirectory",
                str(self.python_runtime),
                "-PythonPackageSha256",
                "c" * 64,
                "-OraclePath",
                str(REPOSITORY_ROOT / "reference" / "tools" / "a1_physical_tpm.py"),
                "-VcRuntimeDirectory",
                str(self.vc_runtime),
                "-RuntimeDependencyReport",
                str(self.dependencies),
                "-OutputDirectory",
                str(self.output),
                "-SourceRevision",
                SOURCE_REVISION,
                "-PackagingRevision",
                PACKAGING_REVISION,
                "-BuildRun",
                "1234",
                "-BuildAttempt",
                "2",
                "-BuildJob",
                "5678",
                "-CMakeVersion",
                "4.4.2",
                "-MsvcVersion",
                "19.44.35219.0",
                "-ToolsetVersion",
                "14.44.35207",
                "-WindowsSdk",
                "10.0.26100.0",
            ]
        )
        result = json.loads(completed.stdout.strip().splitlines()[-1])
        return result, Path(str(result["zip_path"]))

    def extract(self, archive: Path, directory_name: str) -> Path:
        extracted = self.root / directory_name
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(extracted)
        return extracted / "AXL-V1-A1-Physical-TPM-Runtime-Kit"

    def test_assembled_zip_is_exact_result_bound_and_self_verifying(self) -> None:
        result, archive = self.assemble()
        expected_name = "AXL-V1-A1-Physical-TPM-Runtime-Kit-a1a37d07.zip"
        self.assertEqual(archive.name, expected_name)
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), result["kit_sha256"])

        extracted = self.root / "extracted"
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(extracted)
        kit = extracted / "AXL-V1-A1-Physical-TPM-Runtime-Kit"
        required = {
            "README_CN.md",
            "START-INITIAL.cmd",
            "START-POST-REBOOT.cmd",
            "VERIFY-KIT.ps1",
            "00-check-machine.ps1",
            "01-run-initial.ps1",
            "02-run-post-reboot.ps1",
            "03-package-evidence.ps1",
            "bin/axlic.exe",
            "runtime/python-3.13.15-embed-amd64/python.exe",
            "runtime/Microsoft.VC143.CRT/vcruntime140.dll",
            "oracle/a1_physical_tpm.py",
            "manifest/build-manifest.json",
            "manifest/SHA256SUMS.txt",
            "manifest/toolchain-report.json",
            "manifest/runtime-dependencies.txt",
        }
        actual = {path.relative_to(kit).as_posix() for path in kit.rglob("*") if path.is_file()}
        self.assertTrue(required <= actual)

        build_manifest = json.loads((kit / "manifest" / "build-manifest.json").read_text(encoding="utf-8-sig"))
        self.assertEqual(build_manifest["source_revision"], SOURCE_REVISION)
        self.assertEqual(build_manifest["packaging_revision"], PACKAGING_REVISION)
        self.assertEqual(build_manifest["build"], {"run": "1234", "attempt": "2", "job": "5678"})
        self.assertEqual(build_manifest["axlic_exe_sha256"], hashlib.sha256(self.axlic.read_bytes()).hexdigest())
        self.assertEqual(build_manifest["python"]["version"], "3.13.15")
        self.assertTrue(build_manifest["python"]["bundled"])
        self.assertEqual(build_manifest["vc_runtime"]["strategy"], "app-local Microsoft VC143 CRT")

        verified = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(kit / "VERIFY-KIT.ps1"),
            ]
        )
        verification = json.loads(verified.stdout.strip().splitlines()[-1])
        self.assertEqual(verification["status"], "PASS")
        self.assertEqual(verification["source_revision"], SOURCE_REVISION)

        (kit / "bin" / "axlic.exe").write_bytes(b"tampered")
        rejected = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(kit / "VERIFY-KIT.ps1"),
            ],
            check=False,
        )
        self.assertNotEqual(rejected.returncode, 0)
        self.assertIn("HASH_MISMATCH", rejected.stderr + rejected.stdout)

    def test_machine_check_runs_without_development_tools_and_reports_nonphysical_uncertainty(self) -> None:
        _, archive = self.assemble()
        kit = self.extract(archive, "machine-check")
        report = kit / "evidence" / "test-environment.json"
        restricted = os.environ.copy()
        restricted["PATH"] = os.pathsep.join(
            [
                str(Path(os.environ["SystemRoot"]) / "System32"),
                str(Path(os.environ["SystemRoot"])),
            ]
        )
        completed = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(kit / "00-check-machine.ps1"),
                "-ClassificationOnly",
                "-ReportPath",
                str(report),
            ],
            env=restricted,
        )
        classification = json.loads(completed.stdout.strip().splitlines()[-1])
        environment = json.loads(report.read_text(encoding="utf-8-sig"))
        self.assertIn(classification["qualification_status"], {"CANDIDATE_PHYSICAL_MACHINE", "DRY_RUN_ONLY"})
        self.assertEqual(environment["development_tools_required"], [])
        self.assertEqual(environment["physical_tpm_attestation"]["automatic_detection"], "inconclusive")
        self.assertTrue(environment["physical_tpm_attestation"]["reviewer_physical_machine_confirmation_required"])

    def test_machine_check_default_report_path_resolves_in_fresh_windows_powershell_5_1_process(self) -> None:
        _, archive = self.assemble()
        kit = self.extract(archive, "default-report")
        completed = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(kit / "00-check-machine.ps1"),
                "-ClassificationOnly",
            ],
            check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr + completed.stdout)
        report = kit / "evidence" / "environment-report.json"
        self.assertTrue(report.is_file())
        environment = json.loads(report.read_text(encoding="utf-8-sig"))
        self.assertIn(environment["qualification_status"], {"CANDIDATE_PHYSICAL_MACHINE", "DRY_RUN_ONLY"})

    def test_initial_wrapper_does_not_read_undefined_last_exit_code_after_powershell_scripts(self) -> None:
        _, archive = self.assemble()
        kit = self.extract(archive, "strict-wrapper")
        completed = run(
            [
                str(POWERSHELL),
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(kit / "01-run-initial.ps1"),
            ],
            check=False,
        )
        combined = completed.stdout + completed.stderr
        self.assertNotEqual(completed.returncode, 0)
        self.assertNotIn("LASTEXITCODE", combined)
        self.assertIn("NOT_VALID_FOR_T-A1-04", combined)

    def test_all_powershell_entrypoints_parse(self) -> None:
        scripts = [
            KIT_TOOLS / "New-RuntimeKit.ps1",
            *(KIT_TOOLS / "payload").glob("*.ps1"),
        ]
        self.assertGreaterEqual(len(scripts), 6)
        for script in scripts:
            command = (
                "$tokens=$null; $errors=$null; "
                f"[void][System.Management.Automation.Language.Parser]::ParseFile('{script}',[ref]$tokens,[ref]$errors); "
                "if ($errors.Count -ne 0) { $errors | ForEach-Object { Write-Error $_ }; exit 1 }"
            )
            with self.subTest(script=script.name):
                run([str(POWERSHELL), "-NoProfile", "-NonInteractive", "-Command", command])


if __name__ == "__main__":
    unittest.main()
