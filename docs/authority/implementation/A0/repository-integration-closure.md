---
authority_id: AXL-V1-A0-INTEGRATION
stage: integration
scope: axlicense/A0
kind: integration-closure
version: "1"
status: historical
source_type: notion
source_url: https://app.notion.com/p/3db4c57a590c8166b8b3c247a4fd1423
migration_class: location-only
semantic_change: none
---

# A0 — Repository Integration Closure — AXL-V1-A0

> ✅
>
> **Status: CLOSED / INTEGRATED — 2026-09-14.** The exact Gate-closed AXL-V1-A0 result is now part of canonical `main`. Integration preserves D-095 evidence applicability by using a merge commit that retains the reviewed result as a parent.

## 1. Integration identity

- Repository: `github/Mostorm-Labs/axlic`
- Source branch: `codex/axl-v1-a0`
- Gate-closed source revision: `13d16ebb34913dafbddc83222cf29187e92c8053`
- Gate: P34 rereview `PASS_WITH_FINDINGS / CLOSED` — D-095
- Pull request: [PR #1](https://github.com/Mostorm-Labs/axlic/pull/1)
- Pre-integration canonical `main`: `6190288cf7d83bcbe588a4e19fda8d6354335e6f`
- Integrated canonical `main`: `c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`
- Merge method: merge commit
- Merge tree: `93b3d969d6593b68b2e415e9e5fd205686690c6a`

## 2. Occurrence proof

The canonical merge commit has exactly two parents:

1. previous `main` — `6190288cf7d83bcbe588a4e19fda8d6354335e6f`
2. exact Gate-reviewed result — `13d16ebb34913dafbddc83222cf29187e92c8053`

The merge tree `93b3d969…` is the same tree as the Gate-closed A0 source revision. Therefore repository integration changes ancestry only; it does not introduce post-Gate source bytes.

## 3. Gate/evidence binding preserved

The integration occurrence consumes, but does not replace, the D-095 Gate evidence:

- hosted run `34824491876`, attempt 1
- job `103913345075`
- evidence artifact `10340300160` (`AXL-V1-A0-evidence`)
- artifact digest `sha256:e1c696436a1377a5ce1d3671abe5aab6c4a5470400516fc7d7d5a41cee87cfe7`

No new Gate claim is created by the merge. Gate acceptance remains bound to `13d16ebb…`; integration occurrence is independently bound to `main@c83e38de…`.

## 4. Closure criteria

- Gate verdict is PASS/PASS_WITH_FINDINGS: **PASS**.
- Source revision is exact and reviewer-resolvable: **PASS**.
- Source branch is 0 commits behind pre-integration `main`: **PASS**.
- Merge preserves exact reviewed revision as ancestry: **PASS**.
- Canonical `main` now contains the reviewed A0 tree: **PASS**.
- No post-Gate substantive mutation introduced during integration: **PASS**.

## 5. Disposition

**AXL-V1-A0 Repository Integration Closure is CLOSED.** A0 is now the canonical implementation baseline on `main@c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`.

This closure does **not** directly authorize A1 coding. Return to central `aegis` for fresh successor routing using the new canonical baseline; the expected candidate is AXL-V1-A1 C++ Windows Identity + Protected Local State ingress.
