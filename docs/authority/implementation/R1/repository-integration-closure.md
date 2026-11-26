---
authority_id: AXL-V1-R1-INTEGRATION
stage: integration
scope: axlicense/R1
kind: integration-closure
version: "1"
status: current
source_type: repository
migration_class: none
semantic_change: none
---

# AXL-V1-R1 Repository Integration Closure

> ✅
>
> **Status: CLOSED / INTEGRATED — 2026-11-26 (D-111).** The AXL-V1-R1 Publish Pipeline has been integrated into canonical `main` through PR #15. The merge commit and the Gate-reviewed branch head resolve to the same tree, so the integration introduced no content drift. The first test-channel publish ran on the merged `main` on the same day and was verified against the published assets.

## 1. Integration occurrence

- Repository: `github/Mostorm-Labs/axlic`
- Gate verdict: `P34 PASS_WITH_FINDINGS / D-110`
- Gate-evidenced result (D-110): `f9e8c2e96bfd9fff3a8c1fa89a8f3fa292546ecf`
- Merged branch head: `4bdb7fac5a7d1e379cb27d10dac40c00d49140ff` (the result revision plus the D-110 registration commit; the nine files in between are governance records only)
- Integration PR: [PR #15](https://github.com/Mostorm-Labs/axlic/pull/15) (merged 2026-11-26 01:41:32Z, by the user)
- Previous canonical main: `81370f31aac0af1e9444a831f89bee781dff3ffd`
- Merge commit / new canonical main: `9112497df2d73fbbecf65ba4dcf950d5285f6e7c`
- Machine registration: `INT-AXL-V1-R1` in `.aegis/integrations.json`, bound to D-110

## 2. Reviewed-result preservation

- Merge parent 1: `81370f31aac0af1e9444a831f89bee781dff3ffd`
- Merge parent 2: `4bdb7fac5a7d1e379cb27d10dac40c00d49140ff`
- Branch head tree: `1bf92dfab8773d8461be442fc120de23febe4cc7`
- Merge commit tree: `1bf92dfab8773d8461be442fc120de23febe4cc7` (equal)
- Tree equality vs the Gate-evidenced result: **not equal by registration only** — `f9e8c2e..4bdb7fa` is the D-110 commit, which touches nine files under `docs/authority/` and `.aegis/` and nothing else.
- Integration classification: **conforming exact-result integration**.

## 3. Preserved Gate evidence

D-110 remains the controlling Gate decision for `81370f3..f9e8c2e`:

- Local evidence at `f9e8c2e`: 106 tooling unit and contract tests; `ctest` 102/102 on the default, test-key and production-key builds; a v143 14.44 toolset build; mutation checks over the tools and the workflow files.
- Hosted evidence at `f9e8c2e`: run `45627517396` / jobs `139251571153`, `139251570782` on the frozen toolchain — every build step green in both environments, publish job skipped as designed; both packages re-verified locally.
- Three independent read-only reviews: the code review of `81370f3..dd64060`, the P34 review of `81370f3..60a5ede` and the incremental review of `055f630`.
- D-110 findings ①–⑩ (non-blocking; the successor's hardening backlog).

## 4. First test-channel publish

- Run `45738678112` (workflow_dispatch on `main`, head `9112497`): build job `139638035125` and publish job `139639190081` green. First real execution of the publish job (download, re-verification against the built revision, `gh release create`).
- Pre-release `v0.1.0-test.20261126.1` at `9112497`: `axlic-0.1.0-test.20261126.1-win-x64.zip` (sha256 `f878baf2e983577cd1cfb782e784ae32f85bf55bbf9acd58f8f291a3bd0f5498`), its `.sha256` and `build-manifest.json`.
- Verified after download from the merged `main` with `tools/package_axlic.py verify`: PASS as `test`, rejected as `prod`; the `.sha256` asset, the recomputed hash and the release notes agree; the manifest names the test key, the source revision, the runtime and the frozen toolchain.

## 5. Successor routing

Per the D-106/D-110 boundary, the self-built server direction is out of repository scope; A3–A8 retain device-side semantics pending the **host-bridge integration follow-up**, and the complete key rotation/distribution mechanism (overlay write side) remains deferred. There is no authorized P31 successor at closure time.

Routing result:

```yaml
successor_routing:
  predecessor: AXL-V1-R1
  predecessor_gate: PASS_WITH_FINDINGS (D-110)
  canonical_main: 9112497df2d73fbbecf65ba4dcf950d5285f6e7c
  next_task: host-bridge integration follow-up (scope undefined)
  next_stage: successor-definition (central aegis, user instruction)
  next_owner: aegis
  p31_authorized: false
```

No new P31 packaging is authorized from this closure. The D-110 non-blocking findings (publish-job paths of the production channel, manifest runtime version, environment-file handling, scanner coverage, and the remaining small items) and the D-108 backlog enter the successor's hardening backlog. The first production release — a `vX.Y.Z` tag on `main` whose version equals the project version — is the user's call and is the corroborative materialization of the production-only guards.
