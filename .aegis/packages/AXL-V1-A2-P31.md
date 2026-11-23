# AXL-V1-A2 P31 Task Package — Platform Licensing Library Plane

status: APPROVED_FOR_P32_HANDOFF
stage_owner: aegis-implementation
process_profile: Full
created_at: 2026-11-23

## Repository identity

- provider: github
- full_name: Mostorm-Labs/axlic
- canonical_branch_at_packaging: main
- task_anchor:
  - revision: 28e806e
  - relation: ancestor
  - meaning: D-105 verifier-limit integrated baseline on canonical main; the D-106 authority supersession (branch commit `docs: revise P30 A2 direction …`) is packaged on the execution branch ahead of implementation
- execution_branch: codex/axl-v1-a2
- resume_cursor: null

## Trusted basis

- P15 Module Design v2 (A2 realization addendum): docs/authority/architecture/P15-module-design.md
- P16 Runtime Data Flow (FR19-ID-02 association semantics, 16.5 verify-before-replace/anti-rollback): docs/authority/architecture/P16-runtime-data-flow.md
- P17 Platform Contract (CLI family, credential payload face, anti-rollback wording, deterministic CBOR): docs/authority/architecture/P17-platform-contract.md
- P20 Verification Design v3 (§17 EV-A2-01…05): docs/authority/verification/P20-verification-design.md
- P30 Implementation Plan v2 (A2 slice = this package's scope): docs/authority/implementation/P30-implementation-plan.md
- D-106 authority supersession: docs/authority/governance/decision-log.md

This package does not change those authorities. Any semantic ambiguity that cannot be resolved from them is an AUTHORITY_CONFLICT, not executor discretion.

## Purpose

Build the third vertical slice: complete axlic as a **pure local library plane** for platform licensing. `axlic.exe` gains (1) a platform device-reference domain (`axldev_` + 32 lowercase hex), (2) a registration association plane (dual-state self-report / adopt / terminal-rejection reset), (3) a credential installation plane (A0-verify reuse → device-reference match → (authority_revision, credential_generation) lexicographic high-watermark anti-rollback → atomic commit; empty-entitlements revocation credential legal; sole version-rollback rejection class = stale), (4) channel-side policyVersion local persistence, (5) a trust keyring (embedded production floor compiled in at build time from a packaging-time public-key artifact, initially empty = fail-closed, merged with a state-root append overlay where the same key_id overrides the floor entry), and (6) a held-version query surface. The host launcher bridge consumes everything through the `axlic.exe` CLI contract.

`status` / `entitlement` stop hardcoding `RuntimeLicense::absent()` and read the installed credential. The binary keeps zero network code; the offline-boundary import scan stays in force.

## Frozen implementation toolchain

A2 inherits the A0/A1 pins unless explicitly extended here:

- C++20; CMake 4.4.2 CI line; MSVC v143 / VS 2022 17.14 line; Windows SDK 10.0.26100.0; x64.
- `/W4`-equivalent warnings blocking on project code.
- QCBOR exact commit `930708bb86481e88879eb1d87fd4d664f1d69503` (wire, unchanged).
- nlohmann/json exact commit `55f93686c01528224f448c19128836e7df245f72` (state/carrier JSON + embedded floor artifact parsing).
- Catch2 exact commit `8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb` (tests).
- Windows system libraries already authorized for A0/A1 only; no new third-party runtime, no network stack, no server runtime.

## Frozen A2 semantics

### Platform device reference domain

- Text form: `axldev_` + exactly 32 lowercase hex characters (39 total). Uppercase, wrong length or wrong prefix fails parsing. Canonical round-trip: `to_string(parse(x)) == x`.
- Internal form: 16 bytes; comparisons are byte-wise on the parsed form (no string-format ambiguity).

### Registration association

- Persisted as part of license-state (not machine-state): `{device_reference}`; absent = unregistered.
- Dual-state self-report: unregistered or reference-lost → `identity_value` (the A1 128-hex public identity); registered → the platform-assigned reference string.
- Adopt: validate parse → mutate under the machine-wide lock → persist; the installed credential is preserved.
- Reset (terminal rejection: self-report adoption permanently refused / local reference has no subject): drop the local reference, fall back to `identity_value` self-report; the installed credential is preserved until a new-subject credential replaces it.
- The reference is a convenience association, never canonical authority (P16 FR19-ID-02).

### Credential installation pipeline (order is frozen)

1. Acquire the machine-wide mutation lock (busy → retryable local-state failure); read license-state; prepare for mutation.
2. Reuse A0 `verify_credential` unchanged; invalid/unsupported classifications pass through as-is.
3. Parse payload `device_id` as a device reference; parse failure, no local registration, or inequality with the local registration → device-mismatch rejection.
4. High-watermark: `(candidate_rev, candidate_gen) < (stored_rev, stored_gen)` lexicographic → stale rejection (the sole version-rollback rejection class; on-wire errorCode mapping is host-owned). Equality → idempotent reinstall allowed, refreshing the stored policyVersion. Greater, or no stored credential → full install (an empty-entitlements revocation credential is a legal install object).
5. Atomic commit of the stored credential (payload fields + entitlements + policyVersion); release the lock.

Channel-transient values (deliveryRef, reason) never enter axlic state or output.

### Trust keyring

- Embedded floor: build-time artifact `client/keyring/trust-floor.json` (`{"format":"axlic-trust-floor","version":1,"keys":[{key_id, public_key_xy_hex}]}`, xy = 128 lowercase hex), embedded via a CMake-generated source file into `axlic_core`. Initial production floor is the empty key set (fail-closed by construction).
- Append overlay: state-root file `trust-keyring.v1.json` (plaintext JSON, same entry shape, `axlic-trust-keyring-append` format tag). Merge: floor ∪ overlay; the same key_id in the overlay **overrides** the floor entry. Missing overlay → floor only. Malformed overlay → ignored (trust-loss direction only, never trust gain).
- Packaging-time export tool `tools/fetch_trust_floor.py`: reads signing-backend transit public keys via `VAULT_ADDR`/`VAULT_TOKEN` + a key-name list, emits the floor artifact; any failure leaves the existing artifact untouched. Tooling only — never part of the binary.
- The complete rotation/distribution mechanism (overlay write side, key retirement) is explicitly out of scope.

### Local state layout

- `license-state.v1.json` at the protected state root: DPAPI `CRYPTPROTECT_LOCAL_MACHINE` envelope with entropy `"AxLicense/LicenseState/v1"`, envelope tag `axlicense-license-state`, temp-file + flush + `MoveFileExW` atomic replace, failure cleanup, `CommitPoint` observation hooks, same mutation-lock discipline as machine-state.
- Inner model: `{schema_version, state_generation, registration?, credential?}`; `credential` = payload identity fields + entitlements + policy_version.
- `machine-state.v1.json` (A1 identity file) is byte-for-byte untouched.

### CLI surface (host contract)

- Existing envelope/exit-code family unchanged (0 success / 2 COMMAND_INVALID / 3 credential-class / 4 internal / 5 retryable).
- `status`: installed credential → OK + `data{credential_state:"valid", authority_revision, credential_generation}`; none → `CREDENTIAL_NOT_FOUND` as today.
- `entitlement <id>`: same source; time-windowed grant decision.
- `device-register`: `data{registered, self_report_value, device_id?}` (identity must exist, else `IDENTITY_NOT_FOUND`, exit 3).
- `device-register adopt <axldev_ref>` / `device-register reset`.
- `install <path> --policy-version <n>`: artifact >64 KiB pre-rejected; keyring-loaded trust set; pipeline verdict JSON.
- `held-version`: `data{hasCredential:false}` or `{hasCredential:true, version:{authorityRevision, credentialGeneration, policyVersion}}` (camelCase triple for host passthrough).

## Task decomposition (RED → GREEN per task)

- T-A2-01 device reference domain: header/source/tests; matrix = valid canonical round-trip, uppercase/length/prefix negatives, byte equality.
- T-A2-02 trust keyring: header/source/embedded-floor template + empty artifact/tests; matrix = empty floor fail-closed, overlay override, malformed overlay ignored, entry validation.
- T-A2-03 license state model + Windows store: model header/store header+source/tests; matrix = roundtrip, atomic replace, corrupt classification, CommitPoint crash injection.
- T-A2-04 registration plane: resolve/adopt/reset + tests (fake + real store).
- T-A2-05 installation plane: pipeline + `RuntimeLicense::from_installed` + tests; full EV-A2-01 matrix.
- T-A2-06 CLI wiring: `license_command` rendering + `main.cpp` dispatch; README revision.
- T-A2-07 build/CI wiring: CMake sources/tests/configure_file, `axlic_a2_cli_scenario` target, `client/tests/test_a2_cli.py` contract oracle, `.github/workflows/a2-windows.yml`.

## Frozen tests / oracles

- EV-A2-01…05 as registered in P20 v3 §17 (all blocking).
- Corroborative: manual end-to-end walkthrough; export-tool fail-closed behavior without a signing backend.
- Independent oracle for signed fixtures: `reference/tools/a0_reference.py` (unchanged).

## EXECUTION_CLOSURE_CONTRACT

```yaml
task_id: AXL-V1-A2
required_changes:
  - client/include/axlic/core/device_reference.hpp + src + tests (T-A2-01)
  - client/include/axlic/core/trust_keyring.hpp + src + embedded floor template
  - client/keyring/trust-floor.json (initial empty)
  - tools/fetch_trust_floor.py
  - trust keyring tests (T-A2-02)
  - client/include/axlic/core/license_state.hpp
  - client/include/axlic/windows/license_state_repository.hpp + src
  - license state tests x2 (T-A2-03)
  - client/include/axlic/core/registration.hpp + src + tests (T-A2-04)
  - client/include/axlic/core/installation.hpp + src + tests (T-A2-05)
  - runtime_license from_installed (modification)
  - client/include/axlic/core/license_command.hpp + src (T-A2-06)
  - client/app/main.cpp rewiring (modification)
  - README.md revision (T-A2-06)
  - client/CMakeLists.txt wiring (T-A2-07)
  - client/tests/a2_cli_scenario.cpp + client/tests/test_a2_cli.py
  - .github/workflows/a2-windows.yml
forbidden_changes:
  - machine-state.v1.json format/semantics or A1 identity-plane code behavior
  - A0 verification crypto (wire/credential.cpp, qcbor_codec.cpp) — reuse only
  - any network code or dependency in the binary (offline boundary stays)
  - external repository references in any deliverable
  - complete key rotation/distribution mechanism (overlay write side)
  - new third-party runtime dependencies
required_tests:
  - EV-A2-01 installation matrix (blocking)
  - EV-A2-02 keyring merge matrix (blocking)
  - EV-A2-03 registration dual-state matrix (blocking)
  - EV-A2-04 CLI contract oracle (blocking)
  - EV-A2-05 zero-regression full ctest + offline boundary (blocking)
  - manual end-to-end walkthrough (corroborative)
terminal_success:
  all_of:
    - every required change present, no forbidden scope introduced
    - EV-A2-01…05 PASS on the result revision
    - full ctest green including all pre-existing suites
    - build clean under blocking warnings (/W4 /WX equivalent)
terminal_blockers:
  explicit_classes:
    - AUTHORITY_CONFLICT
    - MISSING_REQUIRED_INPUT
    - BLOCKED_REPOSITORY_IDENTITY
    - BLOCKED_EXECUTION_DIVERGENCE
    - ENVIRONMENT_BLOCKER
    - FROZEN_VERIFICATION_FAILURE
    - NEW_HIGH_IMPACT_FAILURE_MODE
return_policy:
  continue_until_terminal_state: true
```

## P32 return contract

Return to CONTROL_REVIEW only after terminal success or an explicit terminal blocker. Code-complete/local-test-complete alone is insufficient.

Required return fields:

- `task_id: AXL-V1-A2`
- `status: READY_FOR_CONTROL_REVIEW | BLOCKED_*`
- `repository: github/Mostorm-Labs/axlic`
- `actual_starting_revision`, `result_revision`
- changed-file set, per-task commit list
- exact test/verification run refs (local ctest output; hosted run refs when available)
- blocker class if blocked

Do not emit or imply P34 PASS from P32.
