#!/usr/bin/env python3
"""Workflow contract tests for the AXL-V1-R1 publish workflows (EV-R1-06).

Pure text checks over the four R1 workflow files: every action is one of the
three pinned first-party actions or a local reusable workflow, nothing is
interpolated into a `run:` block (workflow inputs and contexts reach scripts
through step `env:` instead), no secret other than the automatic token is
read, and permissions are declared explicitly.
"""

from __future__ import annotations

import pathlib
import re
import unittest

WORKFLOWS = pathlib.Path(__file__).resolve().parents[2] / ".github" / "workflows"
R1_WORKFLOWS = ("axlic-publish.yml", "release.yml", "test-publish.yml", "r1-windows.yml")
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
                for block in run_blocks(workflow(name)):
                    self.assertNotIn("${{", block, f"{name}: a run: block interpolates a workflow expression")

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


if __name__ == "__main__":
    unittest.main()
