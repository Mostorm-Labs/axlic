---
authority_id: AXL-V1-R2-INTEGRATION
stage: integration
scope: axlicense/R2
kind: integration-closure
version: "1"
status: current
source_type: repository
migration_class: none
semantic_change: none
---

# AXL-V1-R2 Repository Integration Closure

> ✅
>
> **Status: CLOSED / INTEGRATED — 2026-11-27 (D-114).** The AXL-V1-R2 removal of channel-side policyVersion has been integrated into canonical `main` through PR #16. The merge commit and the Gate-reviewed branch head resolve to the same tree, so the integration introduced no content drift. The first test-channel publish carrying the change ran on the merged `main` the same day; its assets were verified after download and its release notes state the downgrade caveat.

## 1. Integration occurrence

- Repository: `github/Mostorm-Labs/axlic`
- Gate verdict: `P34 PASS_WITH_FINDINGS / D-113`
- Gate-evidenced result (D-113): `208502409bbf768098f0a34e42a5f49bd8e716fa`
- Merged branch head: `2942b86affdc222285a0bdcf592ade67e72dd69a` (the result revision plus the D-113 registration commit; the seven files in between are governance records only)
- Integration PR: [PR #16](https://github.com/Mostorm-Labs/axlic/pull/16) (merged 2026-11-27 01:58:07Z, by the user)
- Previous canonical main: `0ab525cd795aff6ed2df1a93a319d35fe0c11078`
- Merge commit / new canonical main: `46f264f3e39ca25d18f3d4994fe3d17aa33daa3c`
- Machine registration: `INT-AXL-V1-R2` in `.aegis/integrations.json`, bound to D-113

## 2. Reviewed-result preservation

- Merge parent 1: `0ab525cd795aff6ed2df1a93a319d35fe0c11078`
- Merge parent 2: `2942b86affdc222285a0bdcf592ade67e72dd69a`
- Branch head tree: `6fdf9df2d60d72c24d4c1814228f9fc0582c813f`
- Merge commit tree: `6fdf9df2d60d72c24d4c1814228f9fc0582c813f` (equal)
- Tree equality vs the Gate-evidenced result: **not equal by registration only** — `2085024..2942b86` is the D-113 commit, which touches seven files under `docs/authority/` and `.aegis/` and nothing else; the product sources of `main` are byte-identical to the hosted-evidenced revision.
- Integration classification: **conforming exact-result integration**.

## 3. Preserved Gate evidence

D-113 remains the controlling Gate decision for `0ab525c..2085024`; D-114 extends it over the registration commit and the integration:

- Local evidence at `2085024`: `ctest` 104/104; 106 tooling unit and contract tests; the A2 CLI contract with 36 cases; eleven mutations each caught by the intended oracle; cross-version, downgrade-procedure and production-dispatch checks with the pre-change executables.
- Hosted evidence at `2085024`: run `45801506458` / job `139845620815` on the frozen toolchain — every step green; its evidence artifact was downloaded and inspected.
- One independent read-only review (`0ab525c..a5ec432`).
- D-113 findings ①–⑨ (non-blocking; the successor's hardening backlog).

## 4. First test-channel publish carrying the change

- Run `45926103154` (`Test publish Windows`, workflow_dispatch on `main`, head `46f264f3`): build job `140267654522` and publish job `140268852816` green. The keyed build ran `ctest` 104/104 and the A2 evidence steps on the merged `main`; its evidence artifact was downloaded and inspected.
- Pre-release `v0.1.0-test.20261127.2` at `46f264f3`: `axlic-0.1.0-test.20261127.2-win-x64.zip` (sha256 `84b9f7cafd5669d9aee121f08f40334dfaa9e8e321049b132e22919e9c0c7631`), its `.sha256` and `build-manifest.json`.
- Verified after download from the merged `main`: `tools/package_axlic.py verify` PASS as `test`, rejected as `prod`; the published `.sha256`, the recomputed hash and the release notes agree; `tools/check_embedded_trust_floor.py` passes as `test` and fails as `prod`; the manifest names the test key `license-test` (`ca779578…0aea`), source revision `46f264f3`, runtime `Microsoft.VC143.CRT` `14.44.35112` and the frozen toolchain. The published `axlic.exe` answers `COMMAND_INVALID` (exit 2) to the four rejected install shapes.
- Release notes: the notes the publish tool renders from the manifest were extended with the change summary and the downgrade notice, then read back and compared. This closes the obligation of D-112 disposition ② and D-113 finding ⑦ for the first release that carries the change.

## 5. Successor routing

Per the D-106/D-110/D-113 boundary, the self-built server direction is out of repository scope; A3–A8 retain device-side semantics pending the **host-bridge integration follow-up**, and the complete key rotation/distribution mechanism (overlay write side) remains deferred. There is no authorized P31 successor at closure time.

Routing result:

```yaml
successor_routing:
  predecessor: AXL-V1-R2
  predecessor_gate: PASS_WITH_FINDINGS (D-113)
  canonical_main: 46f264f3e39ca25d18f3d4994fe3d17aa33daa3c
  next_task: host-bridge integration follow-up (scope undefined)
  next_stage: successor-definition (central aegis, user instruction)
  next_owner: aegis
  p31_authorized: false
```

No new P31 packaging is authorized from this closure. The host launcher bridge must adopt the revised CLI surface — `axlic install <artifact_path>` without the flag, and the `(authorityRevision, credentialGeneration)` pair only — before or together with the axlic upgrade. The D-113 non-blocking findings and the D-110 and D-108 backlogs enter the successor's hardening backlog. The first production release — a `vX.Y.Z` tag on `main` whose version equals the project version — is the user's call and remains the corroborative materialization of the production-only guards.
