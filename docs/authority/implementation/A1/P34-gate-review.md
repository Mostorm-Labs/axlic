---
authority_id: AXL-V1-A1-P34
stage: P34
scope: axlicense/A1
kind: gate-review
version: "1"
status: historical
source_type: notion
source_url: https://app.notion.com/p/3dd4c57a590c81c0af91faaee79aef4a
migration_class: location-only
semantic_change: none
---

# 34 — P34 AXL-V1-A1 Gate Review — PASS_WITH_FINDINGS

> ✅
>
> **Verdict: PASS_WITH_FINDINGS — 2026-09-16.** P34 independently resolved the frozen P31 package, exact result, hosted evidence, physical-TPM reboot evidence, and repository reality. No `FROZEN_REQUIREMENT_FAILURE` and no `blocking_high_impact` late finding remains. A1 Gate is closed on exact result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`. The findings below are non-blocking tooling/auditability hardening only.

## 1. Review identity

- Repository: `github/Mostorm-Labs/axlic`
- Process profile: **Full**
- P31 package: `aa481ad20860715112fe9bc696eb7d6bdef1c2ea:.aegis/packages/AXL-V1-A1-P31.md`
- Task anchor: `c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`
- Exact Gate result: `codex/axl-v1-a1@a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`
- Current branch resolution at review: exact same result revision; no drift/divergence.
- Hosted run: `34922762083`, attempt `1`, job `104234284895`, conclusion `success`.
- Hosted artifact: `10378707796` / `AXL-V1-A1-evidence`, digest `sha256:ad4c57ba4496376ad5ff3e8779d0bd5dca310fcef685180751ac7a5609ae9df4`.
- Physical TPM evidence ZIP SHA-256: `87347e9feb6715ce9515840ecdddc85280f55baf126eb2c4e79ce1da22914cd7`.
- Physical TPM correlation: `99df2de2-9d87-42ab-909e-1a22d865e704`.

## 2. Frozen Requirement Audit

**PASS.** Repository compare from task anchor to exact result is ahead-only; changed paths stay within A1 identity/local-state/test/workflow/evidence scope. No `server/**`, Node/TypeScript/PostgreSQL, activation/refresh/offline/rehost/factory/admin implementation, public DLL/service/agent, credential-format change, public generic signing API, or production force-software switch was introduced.

Production reality matches the frozen contract:

- `IdentityManager` owns provider order/pinning and re-reads state under the machine-wide lock.
- only `absent_or_permanently_unusable` TPM outcomes may enter first-establishment software fallback; transient TPM failures return retryable unavailable and pinned identities never rerun strongest-provider selection.
- TPM/software providers use persisted P-256 CNG machine keys, no plaintext private export policy, public `X || Y` identity representation, and internal sign/verify qualification.
- machine state is `<ProgramData>\Auditoryworks\AxLicense\machine-state.v1.json`, DPAPI machine-scope protected with frozen entropy, bounded to 1 MiB, protected by the frozen global mutex/DACL, and committed through same-volume temp + flush + `MoveFileExW(...REPLACE_EXISTING | WRITE_THROUGH)`.
- production `axlic` no longer links the A0 fixture adapter; the fixture adapter is under `BUILD_TESTING` only.
- `axlic identity` exposes only frozen public identity fields and stable A1 semantic error families.

## 3. Frozen Evidence Audit

**PASS.** Independent review resolved the hosted artifact and inspected its evidence payload.

### T-A1-01 — Build + A0 regression

- x64 Release toolchain identity matches the frozen line: CMake `4.4.2`, MSVC 19.44/toolset 14.44, Windows SDK `10.0.26100.0`, pinned QCBOR/nlohmann/Catch2 revisions.
- `ctest.xml`: **37 tests, 0 failures, 0 skipped**.
- inherited A0 canonical/crypto/runtime/CLI/offline reports all PASS at exact result revision.
- production fixture-environment dependency removal is covered by production CLI evidence and CMake repository reality.

### T-A1-02 — Provider policy matrix

**PASS.** Exact-result provider-policy evidence covers usable TPM, transient failure/no downgrade, permanent-unusable + policy fallback, fallback denial, pinned TPM failure, missing pinned key/recovery, and pinned software identity after later TPM availability.

### T-A1-03 — Real Software KSP

**PASS.** Real Windows integration evidence records persisted/reopened software KSP P-256 identity, internal sign/verify, and plaintext private-export rejection.

### T-A1-04 — Physical TPM qualification

**PASS.** Reviewer-confirmed Windows 11 Pro build 22631 x64 physical-machine execution reports TPM present/ready/2.0, Microsoft Platform Crypto Provider, no obvious VM signal, scheme `axl-win-cng-tpm-p256-v1`, epoch `1`, and exact result binding.

- public identity SHA-256 before/after reboot: `67db3a4899e4b3d0d653c46fb3a87dc631f137f1bea0350039eed060b1d7ae27`
- initial boot marker: `af93dc5c7a0cccf0d83904ca731e4b4cc301f7f8549d67a093a4753c544fbbaa`
- post-reboot boot marker: `548a78b2dcdd4de819e935872abeea8d8b2ebf2df31014e7887057c0a496e51e`
- post-reboot checks: provider selected PASS; persisted key load + internal sign/verify PASS; plaintext private export rejected PASS; reboot persistence PASS; software identity not selected PASS.

### T-A1-05 — Protected state

**PASS.** DPAPI round-trip/tamper, malformed/odd/non-hex envelope, oversize bound, unsupported version, stale-temp exclusion, protected-state schema and ACL evidence are present; repository codec contains no private-key serialization field.

### T-A1-06 — Cross-process serialization

**PASS.** Real-process integration evidence records concurrent first establishment converging to one authoritative identity; the production manager re-reads committed state after acquiring the frozen global mutex.

### T-A1-07 — Crash / atomic replace

**PASS.** Required kill points `before_temp_write`, `after_temp_write`, `after_temp_flush`, `before_replace`, `after_replace` all PASS with old-or-new complete-state authority; stale temp is ignored.

### T-A1-08 — Identity CLI / minimum disclosure

**PASS.** Production CLI boundary + error-family reports are exact-result-bound; success output is public-only and failure outputs do not expose provider key names/private state. A0 `status` / `entitlement` regressions remain green through the test-only fixture seam.

## 4. Repository Reality Audit

**PASS.** Direct production-code inspection confirms the evidence corresponds to the exact implementation rather than only generated summaries:

- `client/src/core/identity.cpp` enforces provider selection, fallback, pinning, lock/re-read and recovery semantics.
- `client/src/windows/cng_identity_provider.cpp` sets persisted-key export policy to zero, finalizes P-256 keys, derives exact public identity bytes, and qualification performs sign/verify plus private-export rejection.
- `client/src/windows/machine_state.cpp` implements frozen DPAPI entropy/schema/bounds and stores only public/provider reference material.
- `client/src/windows/machine_state_repository.cpp` implements protected ProgramData ACL and atomic temp/flush/replace.
- `client/src/windows/mutation_lock.cpp` uses the frozen global mutex name with protected System/Administrators ACL.
- `client/CMakeLists.txt` keeps `A0FixtureAdapter` test-only and production `axlic` linked only to `axlic_core`.

## 5. New findings

### NON_BLOCKING_FINDING F-01 — Evidence ZIP manifest semantics

The returned physical-TPM Evidence ZIP contains `SHA256SUMS.txt` copied from the full Runtime Kit rather than a bundle-specific manifest for only files inside the Evidence ZIP.

- failure mode: reviewer may mistakenly run the copied manifest as if it were an evidence-bundle manifest and see missing-file errors.
- impact: auditability/diagnostic confusion only; it does not invalidate the evidence bytes.
- existing independent coverage: outer Evidence ZIP SHA-256 was independently recomputed as `87347e9f...`; revision/correlation/identity/boot/provider bindings were independently cross-checked from included JSONs.
- unique blocking detection value of a replacement manifest: none for the current high-impact A1 failure modes.
- classification: **NON_BLOCKING_HARDENING**. Fix in successor qualification tooling if desired.

### NON_BLOCKING_FINDING F-02 — Static environment notice wording

`environment-*.json` retains `NOT_VALID_FOR_T-A1-04` even when `qualification_status=CANDIDATE_PHYSICAL_MACHINE` and `reviewer_physical_machine_confirmation=true`.

- failure mode: human reviewer may misread the static notice as a final failure.
- impact: wording/auditability ambiguity only.
- existing independent coverage: initial session binding requires reviewer confirmation; post-reboot wrapper requires candidate classification; frozen oracle then binds same identity + changed boot marker and returns PASS.
- unique blocking detection value of changing the notice: none.
- classification: **NON_BLOCKING_HARDENING**.

## 6. Gate result

```yaml
gate_review:
  frozen_requirements:
    passed:
      - A1 authorized implementation scope
      - provider selection and pinning
      - persisted CNG identity / private non-exportability
      - protected machine state / single writer / atomic replace
      - A0 fixture production removal
      - identity CLI contract
    failed: []

  frozen_evidence:
    passed:
      - T-A1-01
      - T-A1-02
      - T-A1-03
      - T-A1-04
      - T-A1-05
      - T-A1-06
      - T-A1-07
      - T-A1-08
      - exact hosted run/job/artifact binding
      - exact physical TPM evidence binding
    failed: []

  new_findings:
    blocking_high_impact: []
    non_blocking:
      - F-01 evidence ZIP bundle-manifest hardening
      - F-02 environment notice wording hardening

  verdict: PASS_WITH_FINDINGS
```

## 7. Successor routing boundary

P34 is closed on exact result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`.

This review does **not** merge `codex/axl-v1-a1`, does not perform repository integration, and does not authorize A2 by itself. Return to central `aegis` for successor routing. The expected next decision is integration-first handling of this exact Gate-closed A1 result before issuing any A2 P31 package.
