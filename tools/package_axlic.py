#!/usr/bin/env python3
"""EV-R1-04: package axlic.exe as a self-contained, verifiable zip.

`package` assembles `axlic-<version>-win-x64.zip`: the executable, the app-local
VC++ runtime DLLs it needs (the ones it imports and, transitively, the runtime
DLLs those import), the third-party notices with full license texts,
SHA256SUMS.txt and build-manifest.json, with the zip's own `.sha256` alongside.
Every import must be a Windows component or one of the bundled runtime DLLs;
anything else fails the package, because the binary would start on the build
machine and fail on a clean target. The executable must embed exactly its
environment's trust floor before it is packaged at all.

`verify` re-checks a package after a download and before publishing: the zip
hash, every payload hash against SHA256SUMS.txt and the manifest, the exact
member set, and the embedded trust floor of the packaged executable.

Usage:
  python tools/package_axlic.py package --exe <axlic.exe> --build-dir build \
      --deps-dir build/_deps --environment prod --version 0.1.0 --revision <sha> \
      --out-dir dist [--crt-dir <dir>] [--toolchain-report <json>]
  python tools/package_axlic.py verify --zip dist/axlic-0.1.0-win-x64.zip --environment prod
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import build_trust_floor  # noqa: E402
import check_embedded_trust_floor  # noqa: E402

# Observed with dumpbin /DEPENDENTS on the built executable; each of these is a
# Windows component. The UCRT API sets (api-ms-win-crt-*.dll) resolve to the
# inbox ucrtbase.dll on every supported Windows version. Anything else must be
# one of CRT_DLLS, or the package fails: an import that is neither would be
# satisfied on the build machine and missing on a clean target.
OS_DLLS = frozenset(
    {"advapi32.dll", "bcrypt.dll", "crypt32.dll", "kernel32.dll", "ncrypt.dll", "ole32.dll", "shell32.dll"}
)
OS_DLL_PATTERN = re.compile(r"api-ms-win-crt-[a-z0-9-]+\.dll")
CRT_DLLS = frozenset({"msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"})
CRT_DIR_GLOB = "Microsoft.VC14*.CRT"
DEPENDENCIES_HEADER = "Image has the following dependencies:"

LICENSE_SOURCES = {
    "licenses/QCBOR-LICENSE": ("qcbor-src", "LICENSE"),
    "licenses/nlohmann-json-LICENSE.MIT": ("nlohmann_json-src", "LICENSE.MIT"),
}
NOTICES = "THIRD_PARTY_NOTICES.md"
SUMS_NAME = "SHA256SUMS.txt"
MANIFEST_NAME = "build-manifest.json"
# Fixed zip metadata: the package depends on its inputs only, never on the clock.
ZIP_TIMESTAMP = (1980, 1, 1, 0, 0, 0)
VERSION_PATTERN = re.compile(r"[0-9A-Za-z][0-9A-Za-z.+-]*")


class PackageError(RuntimeError):
    """The package cannot be built, or is not intact and self-contained."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_dependencies(dumpbin_output: str) -> list[str]:
    """DLL names of the first dependency list of `dumpbin /DEPENDENTS`, lowercased, in order."""
    names: list[str] = []
    inside = False
    for line in dumpbin_output.splitlines():
        stripped = line.strip()
        if not inside:
            if stripped == DEPENDENCIES_HEADER:
                inside = True
            continue
        # A delay-load list follows under its own heading; only the plain
        # import list says what must sit next to the executable.
        if stripped == "Summary" or stripped.startswith("Image has the following"):
            break
        if stripped:
            names.append(stripped.lower())
    if not inside:
        raise PackageError("dumpbin printed no dependency list; is the file a PE image?")
    return names


def classify_imports(names: list[str]) -> tuple[list[str], list[str], list[str]]:
    """Split imports into (Windows components, app-local runtime, unresolved)."""
    unique = sorted({name.lower() for name in names})
    os_dlls = [name for name in unique if name in OS_DLLS or OS_DLL_PATTERN.fullmatch(name)]
    crt_dlls = [name for name in unique if name in CRT_DLLS]
    unresolved = [name for name in unique if name not in os_dlls and name not in crt_dlls]
    return os_dlls, crt_dlls, unresolved


def crt_closure(direct: list[str], imports_of) -> list[str]:
    """The runtime DLLs a package needs: the imported ones plus, transitively, the runtime DLLs those import."""
    needed: set[str] = set()
    pending = list(direct)
    while pending:
        dll = pending.pop()
        if dll in needed:
            continue
        needed.add(dll)
        pending += [name for name in imports_of(dll) if name in CRT_DLLS]
    return sorted(needed)

def locate_dumpbin(build_dir: pathlib.Path) -> pathlib.Path:
    metadata_files = list((build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if len(metadata_files) != 1:
        raise PackageError("unique CMake compiler metadata missing")
    metadata = metadata_files[0].read_text(encoding="utf-8", errors="replace")
    match = re.search(r'^set\(CMAKE_CXX_COMPILER "(.+)"\)$', metadata, re.MULTILINE)
    if not match:
        raise PackageError("CMAKE_CXX_COMPILER missing")
    dumpbin = pathlib.Path(match.group(1).strip()).with_name("dumpbin.exe")
    if not dumpbin.is_file():
        raise PackageError(f"dumpbin not found next to the active compiler: {dumpbin}")
    return dumpbin


def locate_crt_dir(dumpbin: pathlib.Path, override: pathlib.Path | None = None) -> pathlib.Path:
    """The x64 Microsoft.VC14*.CRT directory of the compiler's own redist version."""
    if override is not None:
        return override
    # <VS>\VC\Tools\MSVC\<version>\bin\Hostx64\x64\dumpbin.exe
    parents = dumpbin.resolve().parents
    if len(parents) < 7 or parents[4].name != "MSVC" or parents[5].name != "Tools" or parents[6].name != "VC":
        raise PackageError(f"unexpected compiler layout around {dumpbin}")
    redist = parents[6] / "Redist" / "MSVC"
    # The redist directory of a toolset line is not always named like the
    # compiler directory (v143 14.44 ships Tools/MSVC/14.44.35207 beside
    # Redist/MSVC/14.44.35112), so fall back to the line's major.minor.
    tools_version = parents[3].name
    if (redist / tools_version).is_dir():
        versions = [tools_version]
    else:
        versions = sorted(path.name for path in redist.glob(f"{tools_version.rsplit('.', 1)[0]}.*") if path.is_dir())
    candidates = [
        candidate for version in versions for candidate in sorted((redist / version / "x64").glob(CRT_DIR_GLOB))
    ]
    if len(candidates) != 1:
        raise PackageError(
            f"expected exactly one {CRT_DIR_GLOB} directory for toolset {tools_version} under {redist}, found "
            f"{[candidate.name for candidate in candidates]}; pass --crt-dir"
        )
    return candidates[0]


def read_dependencies(exe: pathlib.Path, dumpbin: pathlib.Path) -> str:
    completed = subprocess.run(
        [str(dumpbin), "/NOLOGO", "/DEPENDENTS", str(exe)],
        check=True, capture_output=True, text=True, encoding="utf-8", errors="replace", shell=False,
    )
    return completed.stdout


def write_zip(zip_path: pathlib.Path, top: str, members: list[tuple[str, bytes]]) -> None:
    with zipfile.ZipFile(zip_path, "w") as archive:
        for name, content in members:
            info = zipfile.ZipInfo(f"{top}/{name}", date_time=ZIP_TIMESTAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, content)


def stage_package(
    *,
    exe: pathlib.Path,
    dumpbin_output: str,
    crt_dir: pathlib.Path,
    deps_dir: pathlib.Path,
    notices: pathlib.Path,
    environment: str,
    version: str,
    revision: str,
    out_dir: pathlib.Path,
    keyring_dir: pathlib.Path,
    imports_of,
    toolchain: dict | None = None,
) -> pathlib.Path:
    """Assemble the package and return the zip path; nothing is written on failure.

    imports_of(dll) returns the DLL names a runtime DLL imports itself."""
    if not VERSION_PATTERN.fullmatch(version):
        raise PackageError(f"invalid version label {version!r}")
    if not revision or any(character.isspace() for character in revision):
        raise PackageError(f"invalid source revision {revision!r}")
    imports = parse_dependencies(dumpbin_output)
    if "kernel32.dll" not in imports:
        raise PackageError("the dependency list does not name kernel32.dll; dumpbin output is not plausible")
    os_dlls, direct_crt, unresolved = classify_imports(imports)
    if unresolved:
        raise PackageError(
            "unresolved imports (neither Windows components nor the app-local VC++ runtime): "
            + ", ".join(unresolved)
        )
    crt_dlls = crt_closure(direct_crt, imports_of)
    try:
        check_embedded_trust_floor.check_exe(exe.read_bytes(), environment, keyring_dir)
        trust_floor = build_trust_floor.load_environment_keys(keyring_dir, environment)
    except check_embedded_trust_floor.EmbeddingError as error:
        raise PackageError(str(error)) from error
    except OSError as error:
        raise PackageError(f"cannot read {exe}: {error}") from error

    name = f"axlic-{version}-win-x64"

    def read(path: pathlib.Path, what: str) -> bytes:
        try:
            return path.read_bytes()
        except OSError as error:
            raise PackageError(f"cannot read the {what} {path}: {error}") from error

    payload: list[tuple[str, bytes]] = [("axlic.exe", exe.read_bytes())]
    payload += [(dll, read(crt_dir / dll, "imported VC++ runtime DLL")) for dll in crt_dlls]
    payload.append((NOTICES, read(notices, "third-party notices")))
    payload += [
        (target, read(deps_dir / source_dir / source_name, "license text"))
        for target, (source_dir, source_name) in LICENSE_SOURCES.items()
    ]
    files = [{"path": path, "sha256": sha256(content), "size": len(content)} for path, content in payload]
    pins = {key.key_id: key.spki_sha256 for key in build_trust_floor.ENVIRONMENTS[environment]}
    manifest = {
        "format": "axlic-build-manifest",
        "version": 1,
        "package": f"{name}.zip",
        "axlic_version": version,
        "environment": environment,
        "trust_floor": [{**entry, "spki_sha256": pins[entry["key_id"]]} for entry in trust_floor],
        "source_revision": revision,
        "vc_runtime": {
            "crt_directory": crt_dir.name,
            "redist_version": crt_dir.parents[1].name if len(crt_dir.parents) > 1 else "",
            "dlls": crt_dlls,
        },
        "operating_system_imports": os_dlls,
        "toolchain": toolchain,
        "files": files,
    }
    manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
    sums = "".join(f"{entry['sha256']}  {entry['path']}\n" for entry in files).encode("utf-8")

    out_dir.mkdir(parents=True, exist_ok=True)
    zip_path = out_dir / f"{name}.zip"
    write_zip(zip_path, name, [*payload, (SUMS_NAME, sums), (MANIFEST_NAME, manifest_bytes)])
    (out_dir / MANIFEST_NAME).write_bytes(manifest_bytes)
    (out_dir / f"{zip_path.name}.sha256").write_text(
        f"{sha256(zip_path.read_bytes())}  {zip_path.name}\n", encoding="utf-8", newline="\n"
    )
    return zip_path


def verify_zip(zip_path: pathlib.Path, environment: str, keyring_dir: pathlib.Path) -> dict:
    """Re-check a package; raises on any inconsistency."""
    digest = sha256(zip_path.read_bytes())
    recorded = zip_path.with_name(zip_path.name + ".sha256").read_text(encoding="utf-8").split()
    if not recorded or recorded[0].lower() != digest:
        raise PackageError(f"the package sha256 does not match {zip_path.name}.sha256")
    top = zip_path.stem
    try:
        with zipfile.ZipFile(zip_path) as archive:
            names = archive.namelist()
            if len(set(names)) != len(names):
                raise PackageError("duplicate package members")
            if f"{top}/{MANIFEST_NAME}" not in names:
                raise PackageError(f"{MANIFEST_NAME} is missing from the package")
            manifest = json.loads(archive.read(f"{top}/{MANIFEST_NAME}"))
            if top != f"axlic-{manifest['axlic_version']}-win-x64":
                raise PackageError(f"package name {top!r} does not match its manifest version")
            if manifest["environment"] != environment:
                raise PackageError(
                    f"the package was built for environment {manifest['environment']!r}, not {environment!r}"
                )
            dlls = sorted(manifest["vc_runtime"]["dlls"])
            foreign = sorted(set(dlls) - CRT_DLLS)
            if foreign:
                raise PackageError(f"the manifest declares files that are not a VC++ runtime DLL: {foreign}")
            payload_paths = ["axlic.exe", *dlls, NOTICES, *LICENSE_SOURCES]
            expected = [*(f"{top}/{path}" for path in payload_paths), f"{top}/{SUMS_NAME}", f"{top}/{MANIFEST_NAME}"]
            if names != expected:
                raise PackageError(f"unexpected package members: {names}")
            sums = dict(reversed(line.split("  ", 1)) for line in archive.read(f"{top}/{SUMS_NAME}").decode().splitlines())
            files = {entry["path"]: entry["sha256"] for entry in manifest["files"]}
            if sorted(sums) != sorted(payload_paths) or sorted(files) != sorted(payload_paths):
                raise PackageError("SHA256SUMS.txt and the manifest must list exactly the payload files")
            for path in payload_paths:
                actual = sha256(archive.read(f"{top}/{path}"))
                if actual != sums[path]:
                    raise PackageError(f"{path} does not match SHA256SUMS.txt")
                if actual != files[path]:
                    raise PackageError(f"{path} does not match the manifest")
            executable = archive.read(f"{top}/axlic.exe")
    except (json.JSONDecodeError, KeyError, UnicodeDecodeError, zipfile.BadZipFile) as error:
        raise PackageError(f"malformed package: {error}") from error
    try:
        check_embedded_trust_floor.check_exe(executable, environment, keyring_dir)
    except check_embedded_trust_floor.EmbeddingError as error:
        raise PackageError(str(error)) from error
    return {
        "artifact_id": "EV-R1-04-package-integrity",
        "authority_revision": "P20-v4-R1",
        "package": zip_path.name,
        "sha256": digest,
        "environment": environment,
        "result": "PASS",
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    package = commands.add_parser("package", help="assemble the release zip")
    package.add_argument("--exe", type=pathlib.Path, required=True, help="built axlic.exe")
    package.add_argument("--build-dir", type=pathlib.Path, required=True, help="CMake build directory")
    package.add_argument("--deps-dir", type=pathlib.Path, required=True, help="directory holding the fetched dependency sources")
    package.add_argument("--notices", type=pathlib.Path, default=pathlib.Path(NOTICES))
    package.add_argument("--environment", required=True, help="test or prod")
    package.add_argument("--version", required=True, help="package version label")
    package.add_argument("--revision", required=True, help="source revision")
    package.add_argument("--out-dir", type=pathlib.Path, required=True)
    package.add_argument("--crt-dir", type=pathlib.Path, help="override the located Microsoft.VC14*.CRT directory")
    package.add_argument("--toolchain-report", type=pathlib.Path, help="toolchain_report.py output to embed")
    verify = commands.add_parser("verify", help="re-check a package before publishing")
    verify.add_argument("--zip", type=pathlib.Path, required=True)
    verify.add_argument("--environment", required=True, help="test or prod")
    verify.add_argument("--report", type=pathlib.Path)
    args = parser.parse_args(argv)

    try:
        if args.command == "package":
            dumpbin = locate_dumpbin(args.build_dir)
            crt_dir = locate_crt_dir(dumpbin, args.crt_dir)
            toolchain = (
                json.loads(args.toolchain_report.read_text(encoding="utf-8")) if args.toolchain_report else None
            )
            zip_path = stage_package(
                exe=args.exe,
                dumpbin_output=read_dependencies(args.exe, dumpbin),
                crt_dir=crt_dir,
                imports_of=lambda dll: parse_dependencies(read_dependencies(crt_dir / dll, dumpbin)),
                deps_dir=args.deps_dir,
                notices=args.notices,
                environment=args.environment,
                version=args.version,
                revision=args.revision,
                out_dir=args.out_dir,
                keyring_dir=build_trust_floor.KEYRING_DIR,
                toolchain=toolchain,
            )
            report = {"package": zip_path.name, "sha256": sha256(zip_path.read_bytes()), "result": "PASS"}
        else:
            report = verify_zip(args.zip, args.environment, build_trust_floor.KEYRING_DIR)
    except (PackageError, OSError, subprocess.CalledProcessError) as error:
        print(f"package_axlic: {error}", file=sys.stderr)
        return 1
    if args.command == "verify" and args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
