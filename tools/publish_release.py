#!/usr/bin/env python3
"""EV-R1-06: publish or re-publish one verified package to GitHub Releases.

The dist directory must hold exactly the package zip, its .sha256 and the
build manifest produced by tools/package_axlic.py, and their labels must match
the release being published, so a package can never be published under the
wrong version, revision or environment.

- New release: `gh release create`. Production passes --verify-tag (the tag
  must already exist and is never created) and --latest; the test channel
  creates its tag at the built revision with --target and marks the release
  as a pre-release.
- Existing release (manual re-publish, or a re-run after a failed publish):
  `gh release upload --clobber` replaces the same-named assets and
  `gh release edit` refreshes the title and notes.

Release notes are rendered from the manifest: environment, embedded key ids
with their fingerprints, source revision and file hashes, plus a channel
notice; production notes start with GitHub's generated changelog.

Usage:
  python tools/publish_release.py --environment prod --tag v0.1.0 --version 0.1.0 \
      --revision <sha> --dist-dir dist
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import subprocess
import sys
import tempfile

MANIFEST_NAME = "build-manifest.json"
NOTICES = {
    "prod": (
        "> **Pre-A8 engineering release.** This build has not passed the AXL-V1-A8 release "
        "qualification (installer/upgrade contract, physical TPM qualification, release W1 benchmark)."
    ),
    "test": (
        "> **Testing only.** This build embeds the TEST verification key: credentials signed "
        "with the production key are rejected."
    ),
}
TITLES = {"prod": "axlic {version}", "test": "axlic {version} (test build)"}


class PublishError(RuntimeError):
    """The release cannot be published as requested."""


def gh(*args: str) -> str:
    completed = subprocess.run(
        ["gh", *args], check=True, capture_output=True, text=True, encoding="utf-8", errors="replace", shell=False
    )
    return completed.stdout


def read_manifest(dist_dir: pathlib.Path, environment: str, version: str, revision: str) -> dict:
    path = dist_dir / MANIFEST_NAME
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise PublishError(f"cannot read the build manifest {path}: {error}") from error
    if manifest.get("format") != "axlic-build-manifest":
        raise PublishError(f"{path} is not an axlic-build-manifest")
    expected = {"axlic_version": version, "source_revision": revision, "environment": environment}
    mismatched = [
        f"{key}: release says {value!r}, package says {manifest.get(key)!r}"
        for key, value in expected.items()
        if manifest.get(key) != value
    ]
    if mismatched:
        raise PublishError("the package does not match the release being published: " + "; ".join(mismatched))
    return manifest


def render_notes(manifest: dict, environment: str, generated_notes: str, package_sha256: str) -> str:
    lines: list[str] = []
    if generated_notes:
        lines += [generated_notes.strip(), "", "---", ""]
    lines += [
        NOTICES[environment],
        "",
        "## Package",
        "",
        f"- Package: `{manifest['package']}`",
        f"- Package SHA-256: `{package_sha256}`",
        f"- Environment: `{environment}`",
        "- Embedded verification keys:",
    ]
    lines += [
        f"  - `{key['key_id']}` (sha256 of the DER SubjectPublicKeyInfo: `{key['spki_sha256']}`)"
        for key in manifest["trust_floor"]
    ]
    runtime = manifest["vc_runtime"]
    lines += [
        f"- Source revision: `{manifest['source_revision']}`",
        f"- VC++ runtime, app-local ({runtime['crt_directory']}): {', '.join(runtime['dlls'])}",
        "",
        "## Files",
        "",
        "| File | Size (bytes) | SHA-256 |",
        "| --- | --- | --- |",
    ]
    lines += [f"| `{entry['path']}` | {entry['size']} | `{entry['sha256']}` |" for entry in manifest["files"]]
    return "\n".join(lines) + "\n"


def publish_release(
    gh_run,
    environment: str,
    tag: str,
    version: str,
    revision: str,
    dist_dir: pathlib.Path,
    *,
    repository: str | None = None,
) -> str:
    """Publish the package in dist_dir under tag; returns `created` or `updated`."""
    if environment not in TITLES:
        raise PublishError(f"unknown environment {environment!r}; expected one of ['prod', 'test']")
    if tag != f"v{version}":
        raise PublishError(f"tag {tag!r} does not match version {version!r}")
    repository = repository or os.environ.get("GITHUB_REPOSITORY")
    if not repository:
        raise PublishError("the repository is unknown; pass --repository or set GITHUB_REPOSITORY")

    package = f"axlic-{version}-win-x64.zip"
    expected_files = sorted([package, f"{package}.sha256", MANIFEST_NAME])
    try:
        present = sorted(entry.name for entry in dist_dir.iterdir())
    except OSError as error:
        raise PublishError(f"cannot read {dist_dir}: {error}") from error
    if present != expected_files:
        raise PublishError(f"{dist_dir} must contain exactly {expected_files}; found {present}")
    manifest = read_manifest(dist_dir, environment, version, revision)
    package_sha256 = (dist_dir / f"{package}.sha256").read_text(encoding="utf-8").split()[0]

    try:
        gh_run("release", "view", tag, "--json", "tagName")
        exists = True
    except subprocess.CalledProcessError:
        exists = False
    generated_notes = ""
    if environment == "prod":
        try:
            response = gh_run(
                "api", "-X", "POST", f"repos/{repository}/releases/generate-notes", "-f", f"tag_name={tag}"
            )
            generated_notes = json.loads(response).get("body", "")
        except subprocess.CalledProcessError as error:
            raise PublishError(f"cannot generate the release notes for {tag}: {error}") from error

    title = TITLES[environment].format(version=version)
    assets = [str(dist_dir / name) for name in expected_files]
    with tempfile.TemporaryDirectory(prefix="axlic-notes.") as directory:
        notes = pathlib.Path(directory) / "RELEASE-NOTES.md"
        notes.write_text(
            render_notes(manifest, environment, generated_notes, package_sha256), encoding="utf-8", newline="\n"
        )
        if exists:
            gh_run("release", "upload", tag, *assets, "--clobber")
            gh_run("release", "edit", tag, "--title", title, "--notes-file", str(notes))
            return "updated"
        command = ["release", "create", tag, *assets, "--title", title, "--notes-file", str(notes)]
        if environment == "prod":
            command += ["--verify-tag", "--latest"]
        else:
            command += ["--target", revision, "--prerelease"]
        gh_run(*command)
        return "created"


def main(argv: list[str], gh_run=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--environment", required=True, help="test or prod")
    parser.add_argument("--tag", required=True, help="release tag (must equal v<version>)")
    parser.add_argument("--version", required=True, help="package version label")
    parser.add_argument("--revision", required=True, help="source revision the package was built from")
    parser.add_argument("--dist-dir", type=pathlib.Path, required=True, help="directory written by package_axlic.py")
    parser.add_argument("--repository", help="owner/name (default: $GITHUB_REPOSITORY)")
    args = parser.parse_args(argv)

    try:
        outcome = publish_release(
            gh_run or gh,
            args.environment,
            args.tag,
            args.version,
            args.revision,
            args.dist_dir,
            repository=args.repository,
        )
    except PublishError as error:
        print(f"publish_release: {error}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as error:
        detail = (error.stderr or "").strip()
        print(f"publish_release: gh {' '.join(error.cmd[1:3])} failed (exit {error.returncode}): {detail}", file=sys.stderr)
        return 1
    print(json.dumps({"environment": args.environment, "tag": args.tag, "outcome": outcome}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
