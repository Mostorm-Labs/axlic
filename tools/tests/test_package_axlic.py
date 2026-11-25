#!/usr/bin/env python3
"""Unit tests for tools/package_axlic.py (EV-R1-04 package self-containment and integrity)."""

from __future__ import annotations

import contextlib
import hashlib
import io
import json
import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import package_axlic  # noqa: E402
from keyring_fixtures import KEYRING, PROD_XY, TEST_XY, keyed_exe  # noqa: E402

REVISION = "a" * 40
TOP = "axlic-0.1.0-win-x64"
NOTICES = "THIRD_PARTY_NOTICES.md"
LICENSE_PATHS = ("licenses/QCBOR-LICENSE", "licenses/nlohmann-json-LICENSE.MIT")

DUMPBIN_OUTPUT = """Dump of file axlic.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    ADVAPI32.dll
    bcrypt.dll
    CRYPT32.dll
    ncrypt.dll
    SHELL32.dll
    KERNEL32.dll
    ole32.dll
    MSVCP140.dll
    VCRUNTIME140.dll
    VCRUNTIME140_1.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    api-ms-win-crt-runtime-l1-1-0.dll

  Summary

        1000 .data
        3000 .pdata
"""

CRT_CONTENT = {
    "msvcp140.dll": b"fake msvcp140",
    "vcruntime140.dll": b"fake vcruntime140",
    "vcruntime140_1.dll": b"fake vcruntime140_1",
}
# What each runtime DLL imports itself, as listed by dumpbin /DEPENDENTS on
# the Microsoft.VC143.CRT files: the runtime DLLs depend on one another.
CRT_IMPORTS = {
    "msvcp140.dll": ["kernel32.dll", "vcruntime140.dll", "vcruntime140_1.dll"],
    "vcruntime140.dll": ["kernel32.dll"],
    "vcruntime140_1.dll": ["kernel32.dll", "vcruntime140.dll"],
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Fixture:
    """A fake build tree: executable, dumpbin output, redist, license sources."""

    def __init__(self, root: pathlib.Path, exe: bytes | None = None, dumpbin_output: str = DUMPBIN_OUTPUT):
        self.root = root
        root.mkdir(parents=True, exist_ok=True)
        self.exe = root / "axlic.exe"
        self.exe.write_bytes(keyed_exe() if exe is None else exe)
        self.dumpbin_output = dumpbin_output
        self.crt_dir = root / "Redist" / "14.44.35112" / "x64" / "Microsoft.VC143.CRT"
        self.crt_dir.mkdir(parents=True)
        for name, content in CRT_CONTENT.items():
            (self.crt_dir / name).write_bytes(content)
        (root / "deps" / "qcbor-src").mkdir(parents=True)
        (root / "deps" / "qcbor-src" / "LICENSE").write_bytes(b"QCBOR license text\n")
        (root / "deps" / "nlohmann_json-src").mkdir(parents=True)
        (root / "deps" / "nlohmann_json-src" / "LICENSE.MIT").write_bytes(b"nlohmann json license text\n")
        self.notices = root / "THIRD_PARTY_NOTICES.md"
        self.notices.write_bytes(b"third-party notices\n")
        self.out_dir = root / "out"

    def stage(self, **overrides):
        arguments = dict(
            exe=self.exe,
            dumpbin_output=self.dumpbin_output,
            crt_dir=self.crt_dir,
            deps_dir=self.root / "deps",
            notices=self.notices,
            environment="prod",
            version="0.1.0",
            revision=REVISION,
            out_dir=self.out_dir,
            keyring_dir=KEYRING,
            imports_of=CRT_IMPORTS.__getitem__,
        )
        arguments.update(overrides)
        return package_axlic.stage_package(**arguments)


EXPECTED_PAYLOAD = {
    "axlic.exe": keyed_exe(),
    "msvcp140.dll": CRT_CONTENT["msvcp140.dll"],
    "vcruntime140.dll": CRT_CONTENT["vcruntime140.dll"],
    "vcruntime140_1.dll": CRT_CONTENT["vcruntime140_1.dll"],
    "THIRD_PARTY_NOTICES.md": b"third-party notices\n",
    "licenses/QCBOR-LICENSE": b"QCBOR license text\n",
    "licenses/nlohmann-json-LICENSE.MIT": b"nlohmann json license text\n",
}
EXPECTED_MEMBERS = [
    *(f"{TOP}/{name}" for name in EXPECTED_PAYLOAD),
    f"{TOP}/SHA256SUMS.txt",
    f"{TOP}/build-manifest.json",
]


def rewrite_zip(source: pathlib.Path, target: pathlib.Path, replace: dict[str, bytes] | None = None,
                extra: dict[str, bytes] | None = None) -> None:
    """Copy a package, replacing or adding members, without touching its hashes."""
    with zipfile.ZipFile(source) as archive, zipfile.ZipFile(target, "w") as rewritten:
        for info in archive.infolist():
            content = archive.read(info.filename)
            rewritten.writestr(info, (replace or {}).get(info.filename, content))
        for name, content in (extra or {}).items():
            rewritten.writestr(name, content)
    target.with_name(target.name + ".sha256").write_text(
        f"{digest(target.read_bytes())}  {target.name}\n", encoding="utf-8", newline="\n"
    )


class PackageTestCase(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.fixture = Fixture(self.root)
        self.zip_path = self.root / "out" / f"{TOP}.zip"


class StagePackageTests(PackageTestCase):
    def test_package_layout_and_checksums(self):
        zip_path = self.fixture.stage()
        self.assertEqual(zip_path, self.zip_path)
        self.assertEqual(
            sorted(path.name for path in (self.root / "out").iterdir()),
            ["axlic-0.1.0-win-x64.zip", "axlic-0.1.0-win-x64.zip.sha256", "build-manifest.json"],
        )
        with zipfile.ZipFile(zip_path) as archive:
            self.assertEqual(archive.namelist(), EXPECTED_MEMBERS)
            sums = archive.read(f"{TOP}/SHA256SUMS.txt").decode()
            for name, content in EXPECTED_PAYLOAD.items():
                self.assertEqual(archive.read(f"{TOP}/{name}"), content, name)
            self.assertEqual(
                sums,
                "".join(f"{digest(content)}  {name}\n" for name, content in EXPECTED_PAYLOAD.items()),
            )
            manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
        zip_bytes = zip_path.read_bytes()
        self.assertEqual(
            zip_path.with_name(zip_path.name + ".sha256").read_text(encoding="utf-8"),
            f"{digest(zip_bytes)}  {zip_path.name}\n",
        )
        self.assertEqual(
            manifest,
            {
                "format": "axlic-build-manifest",
                "version": 1,
                "package": f"{TOP}.zip",
                "axlic_version": "0.1.0",
                "environment": "prod",
                "trust_floor": [{
                    "key_id": "license-prod",
                    "public_key_xy_hex": PROD_XY,
                    "spki_sha256": "58bb4787245691066105914d20ff3da5da2cb8677b13e943a40f6a73bcc277a8",
                }],
                "source_revision": REVISION,
                "vc_runtime": {
                    "crt_directory": "Microsoft.VC143.CRT",
                    "redist_version": "14.44.35112",
                    "dlls": ["msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"],
                },
                "operating_system_imports": [
                    "advapi32.dll", "api-ms-win-crt-runtime-l1-1-0.dll", "api-ms-win-crt-stdio-l1-1-0.dll",
                    "bcrypt.dll", "crypt32.dll", "kernel32.dll", "ncrypt.dll", "ole32.dll", "shell32.dll",
                ],
                "toolchain": None,
                "files": [
                    {"path": name, "sha256": digest(content), "size": len(content)}
                    for name, content in EXPECTED_PAYLOAD.items()
                ],
            },
        )
        self.assertEqual(
            json.loads((self.root / "out" / "build-manifest.json").read_text(encoding="utf-8")), manifest
        )

    def test_repackaging_the_same_inputs_is_byte_identical(self):
        first = self.fixture.stage()
        second = self.fixture.stage(out_dir=self.root / "out-again")
        self.assertEqual(first.read_bytes(), second.read_bytes())

    def test_zip_entry_order_and_timestamp_are_fixed(self):
        with zipfile.ZipFile(self.fixture.stage()) as archive:
            self.assertEqual(archive.namelist(), EXPECTED_MEMBERS)
            for info in archive.infolist():
                with self.subTest(member=info.filename):
                    self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                    self.assertEqual(info.compress_type, zipfile.ZIP_DEFLATED)
                    self.assertFalse(info.filename.startswith("/") or "\\" in info.filename)

    def test_unresolved_import_fails_closed(self):
        output = DUMPBIN_OUTPUT.replace("    ole32.dll\n", "    ole32.dll\n    SOMECONTROL.dll\n")
        with self.assertRaisesRegex(package_axlic.PackageError, "(?i)somecontrol\\.dll"):
            Fixture(self.root / "unresolved", dumpbin_output=output).stage()
        self.assertFalse((self.root / "unresolved" / "out").exists())

    def test_missing_crt_dll_fails_closed(self):
        (self.fixture.crt_dir / "vcruntime140_1.dll").unlink()
        with self.assertRaisesRegex(package_axlic.PackageError, "vcruntime140_1.dll"):
            self.fixture.stage()
        self.assertFalse(self.zip_path.exists())

    def test_runtime_dll_closure_is_bundled(self):
        # (CRT DLLs the executable imports) -> (CRT DLLs the package must carry):
        # the imported ones plus the runtime DLLs those import themselves.
        cases = {
            ("MSVCP140.dll",): ["msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"],
            ("VCRUNTIME140_1.dll",): ["vcruntime140.dll", "vcruntime140_1.dll"],
            ("VCRUNTIME140.dll",): ["vcruntime140.dll"],
            (): [],
        }
        for imported, bundled in cases.items():
            with self.subTest(imported=imported):
                output = DUMPBIN_OUTPUT
                for name in ("MSVCP140.dll", "VCRUNTIME140.dll", "VCRUNTIME140_1.dll"):
                    output = output.replace(f"    {name}\n", "")
                output = output.replace("    ole32.dll\n", "    ole32.dll\n" + "".join(f"    {name}\n" for name in imported))
                with zipfile.ZipFile(Fixture(self.root / ("-".join(imported) or "none"), dumpbin_output=output).stage()) as archive:
                    names = [pathlib.PurePosixPath(name).name for name in archive.namelist()]
                    manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
                self.assertEqual([name for name in names if name.endswith(".dll")], bundled)
                self.assertEqual(manifest["vc_runtime"]["dlls"], bundled)

    def test_dumpbin_output_without_a_dependency_list_fails_closed(self):
        for output in ("", "Dump of file axlic.exe\n\nFile Type: EXECUTABLE IMAGE\n\n  Summary\n"):
            with self.subTest(output=output), self.assertRaisesRegex(package_axlic.PackageError, "dependency list"):
                Fixture(self.root / str(len(output)), dumpbin_output=output).stage()
            self.assertFalse((self.root / str(len(output)) / "out" / f"{TOP}.zip").exists())

    def test_an_implausible_dependency_list_fails_closed(self):
        output = "  Image has the following dependencies:\n\n    SOMETHING.dll\n\n  Summary\n"
        with self.assertRaisesRegex(package_axlic.PackageError, "kernel32.dll"):
            Fixture(self.root / "implausible", dumpbin_output=output).stage()

    def test_version_label_cannot_escape_the_package_name(self):
        for version in ("0.1.0/../evil", "0.1.0 extra", "", "0.1.0\\evil"):
            with self.subTest(version=version), self.assertRaises(package_axlic.PackageError):
                self.fixture.stage(version=version)

    def test_a_missing_license_text_fails_closed(self):
        (self.root / "deps" / "qcbor-src" / "LICENSE").unlink()
        with self.assertRaisesRegex(package_axlic.PackageError, "LICENSE"):
            self.fixture.stage()

    def test_an_executable_of_the_other_environment_fails_closed(self):
        fixture = Fixture(self.root / "foreign", exe=keyed_exe("license-test", TEST_XY))
        with self.assertRaisesRegex(package_axlic.PackageError, "trust-floor"):
            fixture.stage()
        self.assertFalse((self.root / "foreign" / "out" / f"{TOP}.zip").exists())


class ImportTests(unittest.TestCase):
    def test_parse_dependencies_reads_only_the_dependency_list(self):
        self.assertEqual(
            package_axlic.parse_dependencies(DUMPBIN_OUTPUT),
            [
                "advapi32.dll", "bcrypt.dll", "crypt32.dll", "ncrypt.dll", "shell32.dll", "kernel32.dll",
                "ole32.dll", "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll",
                "api-ms-win-crt-stdio-l1-1-0.dll", "api-ms-win-crt-runtime-l1-1-0.dll",
            ],
        )

    def test_parse_dependencies_stops_at_the_first_list(self):
        two_lists = DUMPBIN_OUTPUT.replace(
            "  Summary\n", "    Image has the following delay load dependencies:\n\n    DELAYED.dll\n\n  Summary\n"
        )
        self.assertNotIn("delayed.dll", package_axlic.parse_dependencies(two_lists))
        self.assertEqual(len(package_axlic.parse_dependencies(two_lists)), 12)

    def test_parse_dependencies_requires_the_dependency_list(self):
        for output in ("", "Dump of file axlic.exe\n\nFile Type: EXECUTABLE IMAGE\n\n  Summary\n", "garbage\n"):
            with self.subTest(output=output), self.assertRaises(package_axlic.PackageError):
                package_axlic.parse_dependencies(output)

    def test_classify_imports_splits_os_crt_and_unresolved(self):
        self.assertEqual(
            package_axlic.classify_imports(
                ["MSVCP140.dll", "api-ms-win-core-synch-l1-2-0.dll", "CRYPT32.dll", "ws2_32.dll", "MSVCP140.dll"]
            ),
            (["crypt32.dll"], ["msvcp140.dll"], ["api-ms-win-core-synch-l1-2-0.dll", "ws2_32.dll"]),
        )

    def test_locate_dumpbin_reads_the_cmake_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            build = pathlib.Path(directory)
            compiler = build / "toolchain" / "cl.exe"
            compiler.parent.mkdir()
            compiler.write_bytes(b"")
            (build / "CMakeFiles" / "4.4.2").mkdir(parents=True)
            (build / "CMakeFiles" / "4.4.2" / "CMakeCXXCompiler.cmake").write_text(
                f'set(CMAKE_CXX_COMPILER "{compiler.as_posix()}")\n', encoding="utf-8"
            )
            with self.assertRaisesRegex(package_axlic.PackageError, "dumpbin"):
                package_axlic.locate_dumpbin(build)
            (compiler.parent / "dumpbin.exe").write_bytes(b"")
            self.assertEqual(package_axlic.locate_dumpbin(build), compiler.parent / "dumpbin.exe")
            (build / "CMakeFiles" / "other").mkdir()
            (build / "CMakeFiles" / "other" / "CMakeCXXCompiler.cmake").write_text("", encoding="utf-8")
            with self.assertRaises(package_axlic.PackageError):
                package_axlic.locate_dumpbin(build)

    def test_locate_crt_dir_follows_the_compiler_version(self):
        with tempfile.TemporaryDirectory() as directory:
            visual_studio = pathlib.Path(directory) / "VS" / "VC"
            dumpbin = visual_studio / "Tools" / "MSVC" / "14.44.35112" / "bin" / "Hostx64" / "x64" / "dumpbin.exe"
            dumpbin.parent.mkdir(parents=True)
            redist = visual_studio / "Redist" / "MSVC"
            old = redist / "14.43.00000" / "x64" / "Microsoft.VC143.CRT"
            matching = redist / "14.44.35112" / "x64" / "Microsoft.VC143.CRT"
            old.mkdir(parents=True)
            with self.assertRaisesRegex(package_axlic.PackageError, "Microsoft.VC14"):
                package_axlic.locate_crt_dir(dumpbin)
            matching.mkdir(parents=True)
            self.assertEqual(package_axlic.locate_crt_dir(dumpbin), matching)
            (matching.parent / "Microsoft.VC145.CRT").mkdir()
            with self.assertRaisesRegex(package_axlic.PackageError, "exactly one"):
                package_axlic.locate_crt_dir(dumpbin)
            self.assertEqual(package_axlic.locate_crt_dir(dumpbin, old), old)

    def test_locate_crt_dir_falls_back_to_the_toolset_line(self):
        # v143 14.44 in the wild: Tools/MSVC/14.44.35207 but Redist/MSVC/14.44.35112.
        with tempfile.TemporaryDirectory() as directory:
            visual_studio = pathlib.Path(directory) / "VS" / "VC"
            dumpbin = visual_studio / "Tools" / "MSVC" / "14.44.35207" / "bin" / "Hostx64" / "x64" / "dumpbin.exe"
            dumpbin.parent.mkdir(parents=True)
            redist = visual_studio / "Redist" / "MSVC"
            older_line = redist / "14.43.00000" / "x64" / "Microsoft.VC143.CRT"
            older_line.mkdir(parents=True)
            with self.assertRaisesRegex(package_axlic.PackageError, "exactly one"):
                package_axlic.locate_crt_dir(dumpbin)
            same_line = redist / "14.44.35112" / "x64" / "Microsoft.VC143.CRT"
            same_line.mkdir(parents=True)
            self.assertEqual(package_axlic.locate_crt_dir(dumpbin), same_line)
            (same_line.parent / "Microsoft.VC145.CRT").mkdir()
            with self.assertRaisesRegex(package_axlic.PackageError, "exactly one"):
                package_axlic.locate_crt_dir(dumpbin)
            (same_line.parent / "Microsoft.VC145.CRT").rmdir()
            (redist / "14.44.35208" / "x64" / "Microsoft.VC143.CRT").mkdir(parents=True)
            with self.assertRaisesRegex(package_axlic.PackageError, "exactly one"):
                package_axlic.locate_crt_dir(dumpbin)
            self.assertEqual(package_axlic.locate_crt_dir(dumpbin, older_line), older_line)


class VerifyZipTests(PackageTestCase):
    def test_verify_accepts_an_untampered_package(self):
        zip_path = self.fixture.stage()
        report = package_axlic.verify_zip(zip_path, "prod", KEYRING)
        self.assertEqual(report["result"], "PASS")
        self.assertEqual(report["package"], zip_path.name)
        self.assertEqual(report["sha256"], digest(zip_path.read_bytes()))
        self.assertEqual(report["environment"], "prod")

    def test_verify_rejects_a_package_of_the_other_environment(self):
        zip_path = self.fixture.stage()
        with self.assertRaises(package_axlic.PackageError):
            package_axlic.verify_zip(zip_path, "test", KEYRING)
        test_zip = Fixture(self.root / "test-env", exe=keyed_exe("license-test", TEST_XY)).stage(environment="test")
        self.assertEqual(package_axlic.verify_zip(test_zip, "test", KEYRING)["result"], "PASS")
        with self.assertRaises(package_axlic.PackageError):
            package_axlic.verify_zip(test_zip, "prod", KEYRING)

    def test_verify_detects_a_tampered_member(self):
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        rewrite_zip(zip_path, tampered, replace={f"{TOP}/THIRD_PARTY_NOTICES.md": b"silently replaced\n"})
        with self.assertRaisesRegex(package_axlic.PackageError, "THIRD_PARTY_NOTICES.md"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_detects_a_manifest_hash_mismatch(self):
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        with zipfile.ZipFile(zip_path) as archive:
            manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
        manifest["files"][0]["sha256"] = "0" * 64
        rewritten_sums = (
            f"{digest(EXPECTED_PAYLOAD['axlic.exe'])}  axlic.exe\n"
            + "".join(
                f"{digest(content)}  {name}\n" for name, content in EXPECTED_PAYLOAD.items() if name != "axlic.exe"
            )
        )
        rewrite_zip(
            zip_path, tampered,
            replace={
                f"{TOP}/SHA256SUMS.txt": rewritten_sums.encode(),
                f"{TOP}/build-manifest.json": json.dumps(manifest, indent=2).encode(),
            },
        )
        with self.assertRaisesRegex(package_axlic.PackageError, "axlic.exe"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_detects_an_unexpected_member(self):
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        rewrite_zip(zip_path, tampered, extra={f"{TOP}/extra.dll": b"stowaway"})
        with self.assertRaisesRegex(package_axlic.PackageError, "extra.dll"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_detects_a_checksum_file_entry_mismatch(self):
        # The member and the manifest agree; only the checksum file is wrong.
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        wrong = "".join(
            f"{'0' * 64 if name == 'axlic.exe' else digest(content)}  {name}\n"
            for name, content in EXPECTED_PAYLOAD.items()
        )
        rewrite_zip(zip_path, tampered, replace={f"{TOP}/SHA256SUMS.txt": wrong.encode()})
        with self.assertRaisesRegex(package_axlic.PackageError, "does not match SHA256SUMS.txt"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_rejects_checksum_and_manifest_that_agree_on_an_extra_file(self):
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        with zipfile.ZipFile(zip_path) as archive:
            manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
            sums = archive.read(f"{TOP}/SHA256SUMS.txt").decode()
        manifest["files"].append({"path": "extra.txt", "sha256": digest(b"stowaway"), "size": 9})
        sums += f"{digest(b'stowaway')}  extra.txt\n"
        rewrite_zip(
            zip_path, tampered,
            replace={
                f"{TOP}/SHA256SUMS.txt": sums.encode(),
                f"{TOP}/build-manifest.json": (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode(),
            },
        )
        with self.assertRaisesRegex(package_axlic.PackageError, "exactly the payload files"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_rejects_a_manifest_that_passes_a_dll_off_as_runtime(self):
        # Every DLL beside axlic.exe must be one of the known runtime DLLs;
        # listing it in the manifest, consistently everywhere, must not be enough.
        zip_path = self.fixture.stage()
        stowaway = b"not a runtime dll"
        with zipfile.ZipFile(zip_path) as archive:
            manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
            members = {name[len(TOP) + 1:]: archive.read(name) for name in archive.namelist()}
        manifest["vc_runtime"]["dlls"] = ["evil.dll"]
        manifest["files"] = [entry for entry in manifest["files"] if not entry["path"].endswith(".dll")]
        files = [*manifest["files"][:1], {"path": "evil.dll", "sha256": digest(stowaway), "size": len(stowaway)}, *manifest["files"][1:]]
        sums = "".join(f"{entry['sha256']}  {entry['path']}\n" for entry in files)
        manifest["files"] = files
        order = ["axlic.exe", "evil.dll", NOTICES, *LICENSE_PATHS, "SHA256SUMS.txt", "build-manifest.json"]
        content = {
            **members,
            "evil.dll": stowaway,
            "SHA256SUMS.txt": sums.encode(),
            "build-manifest.json": (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode(),
        }
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        with zipfile.ZipFile(tampered, "w") as rewritten:
            for name in order:
                info = zipfile.ZipInfo(f"{TOP}/{name}", date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                rewritten.writestr(info, content[name])
        tampered.with_name(tampered.name + ".sha256").write_text(
            f"{digest(tampered.read_bytes())}  {tampered.name}\n", encoding="utf-8", newline="\n"
        )
        with self.assertRaisesRegex(package_axlic.PackageError, "not a VC\\+\\+ runtime DLL.*evil\\.dll"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_detects_a_swapped_executable(self):
        zip_path = self.fixture.stage()
        tampered = self.root / "tampered" / zip_path.name
        tampered.parent.mkdir()
        foreign = keyed_exe("license-test", TEST_XY)
        sums = "".join(
            f"{digest(foreign if name == 'axlic.exe' else content)}  {name}\n"
            for name, content in EXPECTED_PAYLOAD.items()
        )
        with zipfile.ZipFile(zip_path) as archive:
            manifest = json.loads(archive.read(f"{TOP}/build-manifest.json"))
        manifest["files"][0]["sha256"] = digest(foreign)
        manifest["files"][0]["size"] = len(foreign)
        rewrite_zip(
            zip_path, tampered,
            replace={
                f"{TOP}/axlic.exe": foreign,
                f"{TOP}/SHA256SUMS.txt": sums.encode(),
                f"{TOP}/build-manifest.json": json.dumps(manifest, indent=2).encode(),
            },
        )
        with self.assertRaisesRegex(package_axlic.PackageError, "trust-floor"):
            package_axlic.verify_zip(tampered, "prod", KEYRING)

    def test_verify_detects_wrong_zip_hash(self):
        zip_path = self.fixture.stage()
        sums = zip_path.with_name(zip_path.name + ".sha256")
        sums.write_text(f"{'0' * 64}  {zip_path.name}\n", encoding="utf-8", newline="\n")
        with self.assertRaisesRegex(package_axlic.PackageError, "sha256"):
            package_axlic.verify_zip(zip_path, "prod", KEYRING)

    def test_verify_fails_when_the_checksum_file_is_missing(self):
        zip_path = self.fixture.stage()
        zip_path.with_name(zip_path.name + ".sha256").unlink()
        with self.assertRaises(OSError):
            package_axlic.verify_zip(zip_path, "prod", KEYRING)


class MainTests(PackageTestCase):
    def main(self, *arguments: str) -> int:
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return package_axlic.main(list(arguments))

    def test_main_verify_writes_a_report(self):
        zip_path = self.fixture.stage()
        report = self.root / "report.json"
        code = self.main("verify", "--zip", str(zip_path), "--environment", "prod", "--report", str(report))
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(report.read_text(encoding="utf-8"))["result"], "PASS")

    def test_main_verify_fails_closed(self):
        zip_path = self.fixture.stage()
        report = self.root / "report.json"
        code = self.main("verify", "--zip", str(zip_path), "--environment", "test", "--report", str(report))
        self.assertEqual(code, 1)
        self.assertFalse(report.exists())


if __name__ == "__main__":
    unittest.main()
