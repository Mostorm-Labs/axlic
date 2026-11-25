#!/usr/bin/env python3
"""Workflow contract tests for the AXL-V1-R1 publish workflows (EV-R1-06).

Text checks over the four R1 workflow files, split into jobs and steps by
indentation (no YAML dependency): every action is one of the three pinned
first-party actions or a local reusable workflow; nothing is interpolated into
a `run:` block (workflow inputs and contexts reach scripts through step `env:`
instead); no secret other than the automatic token is read; and the invariants
behind EV-R1-06 hold — the build job holds a read-only token, the token
reaches only the steps that call gh, the release identity is resolved before
any tool is installed or built, checkouts keep no credentials, and each entry
workflow binds the environment and grants the pipeline declares.
"""

from __future__ import annotations

import os
import pathlib
import re
import unittest

# The mutation check for these tests points this at a mutated copy.
WORKFLOWS = pathlib.Path(
    os.environ.get("AXLIC_WORKFLOWS_DIR")
    or pathlib.Path(__file__).resolve().parents[2] / ".github" / "workflows"
)
PIPELINE = "axlic-publish.yml"
ENTRY_WORKFLOWS = ("release.yml", "test-publish.yml", "r1-windows.yml")
R1_WORKFLOWS = (PIPELINE, *ENTRY_WORKFLOWS)
PINNED_ACTIONS = {
    "actions/checkout": "3d3c42e5aac5ba805825da76410c181273ba90b1",
    "actions/setup-python": "ece7cb06caefa5fff74198d8649806c4678c61a1",
    "actions/upload-artifact": "ea165f8d65b6e75b540449e92b4886f43607fa02",
}
LOCAL_WORKFLOW = re.compile(r"\./\.github/workflows/[a-z0-9-]+\.yml")
USES = re.compile(r"^\s*(?:-\s+)?uses:\s*(\S+)\s*$")
RUN = re.compile(r"^(\s*)(?:-\s+)?run:\s*(.*)$")
SECRET = re.compile(r"secrets\.([A-Za-z0-9_]+)")


def workflow(name: str) -> str:
    path = WORKFLOWS / name
    if not path.is_file():
        raise AssertionError(f"workflow file missing: {path}")
    return path.read_text(encoding="utf-8")


def block(text: str, key: str, indent: int) -> str:
    """The mapping from `<indent><key>:` up to the next non-blank line that is not deeper."""
    lines = text.splitlines()
    header = " " * indent + key + ":"
    start = next((index for index, line in enumerate(lines) if line == header), None)
    if start is None:
        raise AssertionError(f"{key!r} block not found")
    end = start + 1
    while end < len(lines) and (not lines[end].strip() or len(lines[end]) - len(lines[end].lstrip()) > indent):
        end += 1
    return "\n".join(lines[start:end])


def keys(text: str, indent: int) -> list[str]:
    return re.findall(r"(?m)^" + " " * indent + r"([A-Za-z_][A-Za-z0-9_-]*):", text)


def mapping_values(text: str) -> set[str]:
    """The `key: value` lines of a flat mapping, without comments."""
    return {
        line.strip()
        for line in text.splitlines()[1:]
        if line.strip() and not line.lstrip().startswith("#")
    }


def steps(job: str) -> dict[str, str]:
    """Step name -> step text, in the order the job runs them."""
    result: dict[str, str] = {}
    current: str | None = None
    buffer: list[str] = []
    for line in job.splitlines():
        if re.match(r"^      - ", line):
            if current is not None:
                result[current] = "\n".join(buffer)
            named = re.match(r"^      - name: (.+)$", line)
            current = named.group(1) if named else "<unnamed>"
            buffer = [line]
        elif current is not None:
            buffer.append(line)
    if current is not None:
        result[current] = "\n".join(buffer)
    return result


def pipeline_job(name: str) -> str:
    return block(block(workflow(PIPELINE), "jobs", 0), name, 2)


def entry_job(name: str) -> str:
    jobs = block(workflow(name), "jobs", 0)
    (job_name,) = keys(jobs, 2)
    return block(jobs, job_name, 2)


def granted(name: str) -> set[str]:
    """What an entry workflow grants, at workflow or calling-job level."""
    text = workflow(name)
    grants: set[str] = set()
    if re.search(r"(?m)^permissions:$", text):
        grants |= mapping_values(block(text, "permissions", 0))
    job = entry_job(name)
    if re.search(r"(?m)^    permissions:$", job):
        grants |= mapping_values(block(job, "permissions", 4))
    return grants


def run_blocks(text: str) -> list[str]:
    """The content of every `run:` step: block scalars and inline commands."""
    blocks: list[str] = []
    lines = text.splitlines()
    index = 0
    while index < len(lines):
        match = RUN.match(lines[index])
        index += 1
        if not match:
            continue
        indent, remainder = len(match.group(1)), match.group(2).strip()
        if remainder in ("|", "|-", "|+", ">", ">-", ">+"):
            content: list[str] = []
            while index < len(lines) and (not lines[index].strip() or len(lines[index]) - len(lines[index].lstrip()) > indent):
                content.append(lines[index])
                index += 1
            blocks.append("\n".join(content))
        else:
            blocks.append(remainder)
    return blocks


class WorkflowContractTests(unittest.TestCase):
    def test_every_r1_workflow_exists(self):
        for name in R1_WORKFLOWS:
            with self.subTest(workflow=name):
                self.assertTrue(workflow(name).strip())

    def test_only_the_pinned_first_party_actions_are_used(self):
        for name in R1_WORKFLOWS:
            with self.subTest(workflow=name):
                for line_number, line in enumerate(workflow(name).splitlines(), 1):
                    match = USES.match(line)
                    if not match:
                        continue
                    action = match.group(1)
                    if LOCAL_WORKFLOW.fullmatch(action):
                        continue
                    repository, _, revision = action.partition("@")
                    self.assertIn(repository, PINNED_ACTIONS, f"{name}:{line_number}: unexpected action {action}")
                    self.assertEqual(revision, PINNED_ACTIONS[repository], f"{name}:{line_number}: {repository} is not pinned")

    def test_run_blocks_never_interpolate_workflow_expressions(self):
        for name in R1_WORKFLOWS:
            with self.subTest(workflow=name):
                for step in run_blocks(workflow(name)):
                    self.assertNotIn("${{", step, f"{name}: a run: block interpolates a workflow expression")

    def test_no_secret_other_than_the_automatic_token_is_read(self):
        for name in R1_WORKFLOWS:
            with self.subTest(workflow=name):
                names = set(SECRET.findall(workflow(name)))
                self.assertFalse(names - {"GITHUB_TOKEN"}, f"{name}: reads secrets {sorted(names)}")

    def test_every_workflow_declares_its_permissions(self):
        for name in R1_WORKFLOWS:
            with self.subTest(workflow=name):
                self.assertRegex(workflow(name), r"(?m)^\s*permissions:\s*(\{|$)")

    def test_run_block_scan_sees_block_and_inline_commands(self):
        sample = "a:\n  run: |\n    echo ${{ inputs.x }}\nb:\n  run: echo ${{ github.sha }}\n"
        self.assertEqual(len(run_blocks(sample)), 2)
        self.assertTrue(all("${{" in block for block in run_blocks(sample)))


class PipelineInvariantTests(unittest.TestCase):
    def test_build_job_holds_a_read_only_token(self):
        build = mapping_values(block(pipeline_job("build"), "permissions", 4))
        publish = mapping_values(block(pipeline_job("publish"), "permissions", 4))
        self.assertEqual(build, {"contents: read"})
        self.assertEqual(publish, {"contents: write", "actions: read"})

    def test_the_automatic_token_reaches_only_the_steps_that_call_gh(self):
        build = pipeline_job("build")
        self.assertNotIn("GH_TOKEN", build)
        self.assertNotIn("github.token", build)
        with_token = set()
        for name, step in steps(pipeline_job("publish")).items():
            if "GH_TOKEN" in step:
                with_token.add(name)
                self.assertIn("GH_TOKEN: ${{ github.token }}", step, name)
                self.assertRegex(step, r"gh run download|tools/publish_release\.py", name)
        self.assertEqual(with_token, {"Download the package built by this run", "Publish or re-publish the release"})
        for name in ENTRY_WORKFLOWS:
            with self.subTest(workflow=name):
                self.assertNotIn("GH_TOKEN", workflow(name))
                self.assertNotIn("github.token", workflow(name))

    def test_identity_is_resolved_before_any_tool_is_installed_or_built(self):
        build = steps(pipeline_job("build"))
        names = list(build)
        identity = names.index("Resolve the release identity")
        self.assertRegex(build["Resolve the release identity"], r"tools/release_identity\.py")
        for later in (
            "Install frozen verification tools",
            "Verify frozen toolchain availability",
            "Derive the trust floor from the committed public key",
            "Configure x64 Release with the keyed trust floor",
            "Build x64 Release",
            "Run frozen unit and process suite",
        ):
            self.assertLess(identity, names.index(later), later)

    def test_checkouts_keep_no_credentials_and_the_build_fetches_history(self):
        for job in ("build", "publish"):
            checkouts = [step for step in steps(pipeline_job(job)).values() if "actions/checkout@" in step]
            self.assertTrue(checkouts, job)
            for step in checkouts:
                self.assertIn("persist-credentials: false", step, job)
        first = next(iter(steps(pipeline_job("build")).values()))
        self.assertIn("actions/checkout@", first)
        self.assertIn("fetch-depth: 0", first)

    def test_the_publish_job_needs_the_build_and_runs_only_when_publishing(self):
        publish = pipeline_job("publish")
        self.assertRegex(publish, r"(?m)^    needs: build$")
        self.assertRegex(publish, r"(?m)^    if: inputs\.publish == true$")


class EntryWorkflowInvariantTests(unittest.TestCase):
    def test_release_binds_the_production_key_and_keeps_a_rehearsal_a_rehearsal(self):
        lines = entry_job("release.yml").splitlines()
        self.assertIn("      environment: prod", lines)
        self.assertIn("      ref: ${{ inputs.tag || github.ref_name }}", lines)
        # A boolean switch: only an explicit true publishes, so a rehearsal can
        # never turn into a production release by a malformed input.
        self.assertIn("      publish: ${{ github.event_name != 'workflow_dispatch' || inputs.publish == true }}", lines)
        self.assertEqual(keys(block(workflow("release.yml"), "on", 0), 2), ["push", "workflow_dispatch"])

    def test_test_publish_binds_the_test_key_and_is_manual_only(self):
        self.assertIn("      environment: test", entry_job("test-publish.yml").splitlines())
        self.assertEqual(keys(block(workflow("test-publish.yml"), "on", 0), 2), ["workflow_dispatch"])

    def test_the_evidence_workflow_never_publishes(self):
        self.assertIn("      publish: false", entry_job("r1-windows.yml").splitlines())

    def test_every_caller_grants_what_the_pipeline_declares(self):
        declared: set[str] = set()
        for job in ("build", "publish"):
            declared |= mapping_values(block(pipeline_job(job), "permissions", 4))
        self.assertEqual(declared, {"contents: read", "contents: write", "actions: read"})
        for name in ENTRY_WORKFLOWS:
            with self.subTest(workflow=name):
                self.assertLessEqual({"contents: write", "actions: read"}, granted(name))


if __name__ == "__main__":
    unittest.main()
