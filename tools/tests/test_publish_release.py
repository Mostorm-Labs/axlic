#!/usr/bin/env python3
"""Unit tests for tools/publish_release.py (EV-R1-06 release publishing)."""

from __future__ import annotations

import contextlib
import io
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import package_axlic  # noqa: E402
import publish_release  # noqa: E402
from keyring_fixtures import (  # noqa: E402
    KEYRING,
    PROD_FINGERPRINT,
    TEST_FINGERPRINT,
    TEST_XY,
    keyed_exe,
)

REPOSITORY = "example/axlic"
REVISION = "a" * 40
TEST_VERSION = "0.1.0-test.20261125.7"
PROD_VERSION = "0.1.0"
DUMPBIN_OUTPUT = "Dump of file axlic.exe\n\n  Image has the following dependencies:\n\n    KERNEL32.dll\n    MSVCP140.dll\n\n  Summary\n"
GENERATED_NOTES = "## What's Changed\n- nothing yet\n"


def build_dist(root: pathlib.Path, environment: str, version: str) -> pathlib.Path:
    """A real package from package_axlic over a fake build tree."""
    tree = root / f"tree-{environment}"
    tree.mkdir(parents=True)
    exe = tree / "axlic.exe"
    exe.write_bytes(keyed_exe("license-test", TEST_XY) if environment == "test" else keyed_exe("license-prod"))
    crt = tree / "crt"
    crt.mkdir()
    (crt / "msvcp140.dll").write_bytes(b"fake msvcp140")
    for source, name, text in (("qcbor-src", "LICENSE", b"qcbor\n"), ("nlohmann_json-src", "LICENSE.MIT", b"json\n")):
        (tree / "deps" / source).mkdir(parents=True)
        (tree / "deps" / source / name).write_bytes(text)
    notices = tree / "THIRD_PARTY_NOTICES.md"
    notices.write_bytes(b"notices\n")
    out = root / f"dist-{environment}"
    package_axlic.stage_package(
        exe=exe,
        dumpbin_output=DUMPBIN_OUTPUT,
        crt_dir=crt,
        deps_dir=tree / "deps",
        notices=notices,
        environment=environment,
        version=version,
        revision=REVISION,
        out_dir=out,
        keyring_dir=KEYRING,
    )
    return out


class FakeGh:
    """Records every gh invocation; knows which tags already have a release."""

    def __init__(self, releases: tuple[str, ...] = ()):
        self.releases = set(releases)
        self.calls: list[tuple[str, ...]] = []
        self.notes: list[str] = []

    def __call__(self, *args: str) -> str:
        self.calls.append(args)
        if args[:2] == ("release", "view"):
            if args[2] in self.releases:
                return json.dumps({"tagName": args[2]})
            raise subprocess.CalledProcessError(1, ["gh", *args])
        if args[0] == "api":
            return json.dumps({"name": "auto", "body": GENERATED_NOTES})
        if "--notes-file" in args:
            self.notes.append(pathlib.Path(args[args.index("--notes-file") + 1]).read_text(encoding="utf-8"))
        if args[:2] == ("release", "create"):
            self.releases.add(args[2])
            return f"https://example.invalid/releases/tag/{args[2]}"
        return ""

    def only(self, command: str, subcommand: str) -> tuple[str, ...]:
        matching = [call for call in self.calls if call[:2] == (command, subcommand)]
        assert len(matching) == 1, matching
        return matching[0]


class PublishReleaseTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.dist = {
            "test": build_dist(self.root, "test", TEST_VERSION),
            "prod": build_dist(self.root, "prod", PROD_VERSION),
        }
        self.labels = {"test": (f"v{TEST_VERSION}", TEST_VERSION), "prod": (f"v{PROD_VERSION}", PROD_VERSION)}

    def publish(self, gh, environment, tag=None, version=None, revision=REVISION, dist=None):
        default_tag, default_version = self.labels[environment]
        return publish_release.publish_release(
            gh, environment, default_tag if tag is None else tag, default_version if version is None else version,
            revision, self.dist[environment] if dist is None else dist, repository=REPOSITORY,
        )

    def test_new_test_release_is_prerelease_at_revision(self):
        gh = FakeGh()
        self.assertEqual(self.publish(gh, "test"), "created")
        create = gh.only("release", "create")
        self.assertEqual(create[2], f"v{TEST_VERSION}")
        self.assertIn("--prerelease", create)
        self.assertEqual(create[create.index("--target") + 1], REVISION)
        self.assertNotIn("--latest", create)
        self.assertNotIn("--verify-tag", create)
        assets = [pathlib.Path(argument).name for argument in create[3:create.index("--title")]]
        self.assertEqual(
            assets,
            [f"axlic-{TEST_VERSION}-win-x64.zip", f"axlic-{TEST_VERSION}-win-x64.zip.sha256", "build-manifest.json"],
        )
        self.assertEqual(create[create.index("--title") + 1], f"axlic {TEST_VERSION} (test build)")
        self.assertFalse(any(call[0] == "api" for call in gh.calls))
        notes = gh.notes[0]
        self.assertIn("Testing only", notes)
        self.assertIn("license-test", notes)
        self.assertIn(TEST_FINGERPRINT, notes)
        self.assertIn(REVISION, notes)

    def test_new_prod_release_verifies_tag_and_is_latest(self):
        gh = FakeGh()
        self.assertEqual(self.publish(gh, "prod"), "created")
        create = gh.only("release", "create")
        self.assertEqual(create[2], f"v{PROD_VERSION}")
        self.assertIn("--verify-tag", create)
        self.assertIn("--latest", create)
        self.assertNotIn("--target", create)
        self.assertNotIn("--prerelease", create)
        self.assertEqual(create[create.index("--title") + 1], f"axlic {PROD_VERSION}")
        api = gh.only("api", "-X")
        self.assertIn(f"repos/{REPOSITORY}/releases/generate-notes", api)
        self.assertIn(f"tag_name=v{PROD_VERSION}", api)
        notes = gh.notes[0]
        self.assertTrue(notes.startswith(GENERATED_NOTES))
        self.assertIn("Pre-A8 engineering release", notes)
        self.assertIn("license-prod", notes)
        self.assertIn(PROD_FINGERPRINT, notes)
        self.assertNotIn("license-test", notes)

    def test_existing_release_replaces_assets(self):
        for environment in ("test", "prod"):
            with self.subTest(environment=environment):
                gh = FakeGh(releases=(self.labels[environment][0],))
                self.assertEqual(self.publish(gh, environment), "updated")
                self.assertFalse(any(call[:2] == ("release", "create") for call in gh.calls))
                upload = gh.only("release", "upload")
                self.assertEqual(upload[2], self.labels[environment][0])
                self.assertIn("--clobber", upload)
                self.assertEqual(len([argument for argument in upload[3:] if not argument.startswith("--")]), 3)
                self.assertEqual(gh.only("release", "edit")[2], self.labels[environment][0])
                self.assertEqual(len(gh.notes), 1)

    def test_prod_never_creates_tag(self):
        for releases in ((), (f"v{PROD_VERSION}",)):
            with self.subTest(existing=bool(releases)):
                gh = FakeGh(releases=releases)
                self.publish(gh, "prod")
                for call in gh.calls:
                    self.assertNotIn("--target", call)

    def test_unexpected_dist_file_fails(self):
        (self.dist["prod"] / "stowaway.txt").write_text("x", encoding="utf-8")
        gh = FakeGh()
        with self.assertRaises(publish_release.PublishError):
            self.publish(gh, "prod")
        (self.dist["prod"] / f"axlic-{PROD_VERSION}-win-x64.zip.sha256").unlink()
        with self.assertRaises(publish_release.PublishError):
            self.publish(gh, "prod")
        self.assertEqual(gh.calls, [])

    def test_labels_must_match_the_package(self):
        gh = FakeGh()
        cases = (
            ("v0.2.0", PROD_VERSION),  # tag is not the version's
            ("v0.2.0", "0.2.0"),  # version is not the package's
        )
        for tag, version in cases:
            with self.subTest(tag=tag, version=version), self.assertRaises(publish_release.PublishError):
                self.publish(gh, "prod", tag=tag, version=version)
        with self.assertRaises(publish_release.PublishError):
            self.publish(gh, "prod", revision="b" * 40)
        with self.assertRaises(publish_release.PublishError):
            self.publish(gh, "test", dist=self.dist["prod"])
        self.assertEqual(gh.calls, [])

    def test_unknown_environment_fails(self):
        gh = FakeGh()
        with self.assertRaises(publish_release.PublishError):
            publish_release.publish_release(
                gh, "production", "v0.1.0", PROD_VERSION, REVISION, self.dist["prod"], repository=REPOSITORY
            )
        self.assertEqual(gh.calls, [])


class MainTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.dist = build_dist(self.root, "prod", PROD_VERSION)

    def main(self, *arguments: str, gh=None) -> int:
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return publish_release.main(
                ["--environment", "prod", "--tag", f"v{PROD_VERSION}", "--version", PROD_VERSION,
                 "--revision", REVISION, "--dist-dir", str(self.dist), "--repository", REPOSITORY, *arguments],
                gh_run=gh,
            )

    def test_main_publishes_with_the_injected_runner(self):
        gh = FakeGh()
        self.assertEqual(self.main(gh=gh), 0)
        self.assertIn(("release", "create"), [call[:2] for call in gh.calls])

    def test_main_fails_closed_without_touching_github(self):
        gh = FakeGh()
        (self.dist / "stowaway.txt").write_text("x", encoding="utf-8")
        self.assertEqual(self.main(gh=gh), 1)
        self.assertEqual(gh.calls, [])


if __name__ == "__main__":
    unittest.main()
