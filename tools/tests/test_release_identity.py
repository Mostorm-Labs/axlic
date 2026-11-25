#!/usr/bin/env python3
"""Unit tests for tools/release_identity.py (EV-R1-06 release identity guards)."""

from __future__ import annotations

import contextlib
import datetime
import functools
import io
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import release_identity  # noqa: E402

FIXED_NOW = datetime.datetime(2026, 11, 25, 23, 30, tzinfo=datetime.timezone.utc)
CMAKELISTS = "cmake_minimum_required(VERSION 3.31)\nproject(axlicense VERSION 0.1.0 LANGUAGES C CXX)\n"


def git(*args: str, cwd: pathlib.Path | None = None) -> str:
    completed = subprocess.run(
        ["git", *args], cwd=cwd, check=True, capture_output=True, text=True, encoding="utf-8", errors="replace", shell=False
    )
    return completed.stdout.strip()


class Repo:
    """A temporary git repository shaped like a checkout with a fetched main."""

    def __init__(self, path: pathlib.Path):
        self.path = path
        path.mkdir(parents=True)
        (path / "CMakeLists.txt").write_text(CMAKELISTS, encoding="utf-8", newline="\n")
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.name", "R1 Test")
        self.git("config", "user.email", "r1-test@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.hooksPath", str(path / "no-hooks"))
        self.git("add", "CMakeLists.txt")
        self.commit("initial")
        self.main = self.git("rev-parse", "HEAD")
        self.git("update-ref", "refs/remotes/origin/main", self.main)

    def git(self, *args: str) -> str:
        return git(*args, cwd=self.path)

    def commit(self, message: str) -> str:
        self.git("commit", "-q", "--allow-empty", "-m", message)
        return self.git("rev-parse", "HEAD")


class ResolveIdentityTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.repo = Repo(self.root / "repo")
        self.run_git = functools.partial(git, cwd=self.repo.path)

    def resolve(self, environment, ref, publish, run_number=None):
        return release_identity.resolve_identity(
            environment, ref, publish, run_number,
            source_dir=self.repo.path, run_git=self.run_git, now=FIXED_NOW,
        )

    def test_prod_tag_on_main_resolves(self):
        self.repo.git("tag", "v0.1.0")
        identity = self.resolve("prod", "v0.1.0", True)
        self.assertEqual(
            identity,
            release_identity.Identity(self.repo.main, "0.1.0", "v0.1.0", "0.1.0"),
        )

    def test_prod_annotated_tag_on_main_resolves(self):
        # Production tags are usually annotated: the identity must be the commit
        # the tag points at, not the tag object.
        self.repo.git("tag", "-a", "v0.1.0", "-m", "axlic 0.1.0")
        identity = self.resolve("prod", "v0.1.0", True)
        self.assertEqual(identity, release_identity.Identity(self.repo.main, "0.1.0", "v0.1.0", "0.1.0"))

    def test_missing_main_ref_fails_closed(self):
        self.repo.git("tag", "v0.1.0")
        self.repo.git("update-ref", "-d", "refs/remotes/origin/main")
        with self.assertRaisesRegex(release_identity.IdentityError, "not available"):
            self.resolve("prod", "v0.1.0", True)

    def test_prod_tag_version_must_equal_project_version(self):
        self.repo.git("tag", "v0.2.0")
        for publish in (True, False):
            with self.subTest(publish=publish), self.assertRaises(release_identity.IdentityError):
                self.resolve("prod", "v0.2.0", publish)

    def test_prod_requires_an_existing_well_formed_tag_when_publishing(self):
        self.repo.git("tag", "v0.1.0-rc1")
        for ref in ("main", "v0.1.0-rc1"):
            with self.subTest(ref=ref), self.assertRaises(release_identity.IdentityError):
                self.resolve("prod", ref, True)

    def test_prod_tag_commit_must_be_on_main(self):
        self.repo.git("switch", "-q", "-c", "feature")
        self.repo.commit("off main")
        self.repo.git("tag", "v0.1.0")
        for publish in (True, False):
            with self.subTest(publish=publish), self.assertRaises(release_identity.IdentityError):
                self.resolve("prod", "v0.1.0", publish)

    def test_prod_rehearsal_without_tag_uses_project_version(self):
        identity = self.resolve("prod", "main", False)
        self.assertEqual(identity, release_identity.Identity(self.repo.main, "0.1.0", "", "0.1.0"))

    def test_test_version_format(self):
        identity = self.resolve("test", "main", True, run_number=7)
        self.assertEqual(identity.version, "0.1.0-test.20261125.7")
        self.assertEqual(identity.tag, "v0.1.0-test.20261125.7")
        self.assertEqual(identity.revision, self.repo.main)

    def test_test_channel_requires_a_run_number(self):
        with self.assertRaises(release_identity.IdentityError):
            self.resolve("test", "main", True)

    def test_unknown_or_unsafe_ref_fails_before_build(self):
        for environment in ("test", "prod"):
            for ref in ("no-such-ref", "--upload-pack=evil", "", "v0.1.0^{}", "a..b", "v.lock"):
                with self.subTest(environment=environment, ref=ref):
                    with self.assertRaises(release_identity.IdentityError):
                        self.resolve(environment, ref, False, run_number=1)

    def test_unsafe_ref_never_reaches_git(self):
        calls: list[tuple[str, ...]] = []

        def spy(*args: str) -> str:
            calls.append(args)
            return self.run_git(*args)

        with self.assertRaises(release_identity.IdentityError):
            release_identity.resolve_identity(
                "test", "--upload-pack=evil", False, 1,
                source_dir=self.repo.path, run_git=spy, now=FIXED_NOW,
            )
        self.assertEqual(calls, [])

    def test_checked_out_commit_must_match_the_tag(self):
        self.repo.git("tag", "v0.1.0")
        self.repo.commit("after the tag")
        with self.assertRaises(release_identity.IdentityError):
            self.resolve("prod", "v0.1.0", False)

    def test_missing_project_version_fails(self):
        (self.repo.path / "CMakeLists.txt").write_text("cmake_minimum_required(VERSION 3.31)\n", encoding="utf-8", newline="\n")
        with self.assertRaises(release_identity.IdentityError):
            self.resolve("test", "main", True, run_number=1)

    def test_unknown_environment_fails(self):
        with self.assertRaises(release_identity.IdentityError):
            self.resolve("production", "main", True)


class MainTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.repo = Repo(self.root / "repo")

    def main(self, *arguments: str) -> int:
        argv = ["--source-dir", str(self.repo.path), *arguments]
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return release_identity.main(argv)

    def test_main_writes_step_outputs_and_job_environment(self):
        self.repo.git("tag", "v0.1.0")
        output = self.root / "output.txt"
        environment = self.root / "env.txt"
        code = self.main(
            "--environment", "prod", "--ref", "v0.1.0", "--publish", "true",
            "--github-output", str(output), "--github-env", str(environment),
        )
        self.assertEqual(code, 0)
        self.assertEqual(
            output.read_text(encoding="utf-8"),
            f"revision={self.repo.main}\nversion=0.1.0\ntag=v0.1.0\n",
        )
        self.assertEqual(
            environment.read_text(encoding="utf-8"),
            f"AXLIC_REVISION={self.repo.main}\nAXLIC_VERSION=0.1.0\nAXLIC_TAG=v0.1.0\n",
        )

    def test_main_fails_closed_and_writes_nothing(self):
        output = self.root / "output.txt"
        code = self.main(
            "--environment", "prod", "--ref", "main", "--publish", "true", "--github-output", str(output)
        )
        self.assertEqual(code, 1)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
