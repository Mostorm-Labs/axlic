#!/usr/bin/env python3
"""EV-R1-06: resolve and guard the release identity before anything is built.

Decides, from the checked-out source tree, which revision, version and tag a
publish run stands for, and refuses the run before any build step when the
rules of the channel are not met:

- production publishes only an existing tag vX.Y.Z whose X.Y.Z equals the
  tagged commit's project(axlicense VERSION ...) and whose commit is on the
  main line; a tag is never created or moved.
- a non-tag ref is a rehearsal (publishing disabled) and takes the project
  version as its label.
- the test channel labels any ref <project version>-test.<yyyyMMdd>.<run
  number> (UTC) and publishes it as the Pre-release v<label>.

Usage:
  python tools/release_identity.py --environment prod --ref v0.1.0 --publish true \
      [--run-number N] [--github-output $GITHUB_OUTPUT] [--github-env $GITHUB_ENV]
"""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import datetime
import functools
import json
import pathlib
import re
import subprocess
import sys

PROD_TAG_PATTERN = r"v(\d+\.\d+\.\d+)"
PROJECT_VERSION_PATTERN = r"project\(\s*axlicense\s+VERSION\s+(\d+\.\d+\.\d+)"
# Ref names come from a workflow input and reach git as an argument: allow
# only plain names so they can never be read as an option or a revision range.
REF_PATTERN = re.compile(r"[A-Za-z0-9_][A-Za-z0-9._/+-]*")
DEFAULT_MAIN_REF = "refs/remotes/origin/main"


@dataclass(frozen=True)
class Identity:
    revision: str
    version: str
    tag: str
    project_version: str


class IdentityError(RuntimeError):
    """The run does not stand for a publishable release identity."""


def git(*args: str, cwd: pathlib.Path | None = None) -> str:
    completed = subprocess.run(
        ["git", *args], cwd=cwd, check=True, capture_output=True, text=True, encoding="utf-8", errors="replace", shell=False
    )
    return completed.stdout.strip()


def read_project_version(source_dir: pathlib.Path) -> str:
    path = source_dir / "CMakeLists.txt"
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        raise IdentityError(f"cannot read {path}: {error}") from error
    match = re.search(PROJECT_VERSION_PATTERN, text)
    if not match:
        raise IdentityError(f"project(axlicense VERSION X.Y.Z ...) not found in {path}")
    return match.group(1)


def _optional(run_git, *args: str) -> str | None:
    """run_git's stripped output, or None when the command failed."""
    try:
        return run_git(*args).strip()
    except (subprocess.CalledProcessError, OSError):
        return None


def _require_on_main(run_git, revision: str, main_ref: str) -> None:
    if _optional(run_git, "rev-parse", "--verify", "--quiet", f"{main_ref}^{{commit}}") is None:
        raise IdentityError(f"{main_ref} is not available; check out with full history")
    if _optional(run_git, "merge-base", "--is-ancestor", revision, main_ref) is None:
        raise IdentityError(f"commit {revision} is not on {main_ref}")


def resolve_identity(
    environment: str,
    ref: str,
    publish: bool,
    run_number: int | None,
    *,
    source_dir: pathlib.Path,
    run_git,
    now: datetime.datetime | None = None,
    main_ref: str = DEFAULT_MAIN_REF,
) -> Identity:
    if environment not in ("test", "prod"):
        raise IdentityError(f"unknown environment {environment!r}; expected one of ['prod', 'test']")
    if not ref or not REF_PATTERN.fullmatch(ref) or ".." in ref or ref.endswith(".lock"):
        raise IdentityError(f"invalid ref {ref!r}")
    project_version = read_project_version(source_dir)
    revision = _optional(run_git, "rev-parse", "--verify", "--quiet", "HEAD")
    if revision is None:
        raise IdentityError("not inside a git repository with a commit")
    if _optional(run_git, "rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}") != revision:
        raise IdentityError(f"ref {ref!r} does not resolve to the checked-out commit {revision}")
    tagged = _optional(run_git, "rev-parse", "--verify", "--quiet", f"refs/tags/{ref}^{{commit}}") == revision

    if environment == "prod":
        if tagged:
            match = re.fullmatch(PROD_TAG_PATTERN, ref)
            if not match:
                raise IdentityError(f"production tag {ref!r} must be vX.Y.Z")
            if match.group(1) != project_version:
                raise IdentityError(
                    f"tag {ref!r} does not match project version {project_version} in CMakeLists.txt"
                )
            _require_on_main(run_git, revision, main_ref)
            return Identity(revision, match.group(1), ref, project_version)
        if publish:
            raise IdentityError(
                f"production publishes only an existing vX.Y.Z tag; {ref!r} is not a tag "
                "(a non-tag ref is allowed only with publishing disabled)"
            )
        return Identity(revision, project_version, "", project_version)

    if run_number is None:
        raise IdentityError("the test channel requires --run-number")
    stamp = (now or datetime.datetime.now(datetime.timezone.utc)).astimezone(datetime.timezone.utc).strftime("%Y%m%d")
    version = f"{project_version}-test.{stamp}.{run_number}"
    return Identity(revision, version, f"v{version}", project_version)


def _boolean(text: str) -> bool:
    lowered = text.strip().lower()
    if lowered == "true":
        return True
    if lowered == "false":
        return False
    raise argparse.ArgumentTypeError(f"expected true or false, got {text!r}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--environment", required=True, help="test or prod")
    parser.add_argument("--ref", required=True, help="the ref that was checked out")
    parser.add_argument("--publish", type=_boolean, required=True, help="true when a release will be published")
    parser.add_argument("--run-number", type=int, help="workflow run number (test channel)")
    parser.add_argument("--source-dir", type=pathlib.Path, default=pathlib.Path("."), help="checked-out source root")
    parser.add_argument("--github-output", type=pathlib.Path, help="step output file ($GITHUB_OUTPUT)")
    parser.add_argument("--github-env", type=pathlib.Path, help="job environment file ($GITHUB_ENV)")
    args = parser.parse_args(argv)

    try:
        identity = resolve_identity(
            args.environment,
            args.ref,
            args.publish,
            args.run_number,
            source_dir=args.source_dir,
            run_git=functools.partial(git, cwd=args.source_dir),
        )
    except IdentityError as error:
        print(f"release_identity: {error}", file=sys.stderr)
        return 1
    if args.github_output:
        block = f"revision={identity.revision}\nversion={identity.version}\ntag={identity.tag}\n"
        args.github_output.write_text(block, encoding="utf-8", newline="\n")
    if args.github_env:
        block = (
            f"AXLIC_REVISION={identity.revision}\nAXLIC_VERSION={identity.version}\nAXLIC_TAG={identity.tag}\n"
        )
        args.github_env.write_text(block, encoding="utf-8", newline="\n")
    print(json.dumps(asdict(identity), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
