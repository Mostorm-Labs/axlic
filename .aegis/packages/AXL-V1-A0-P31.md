# AXL-V1-A0 P31 Task Package — C++ Local Signed-Credential Gate

status: APPROVED_FOR_P32_HANDOFF
stage_owner: aegis-implementation
process_profile: Full
created_at: 2026-09-13

## Repository identity

- provider: github
- full_name: Mostorm-Labs/axlic
- canonical_branch: main
- task_anchor:
  - revision: 5a2cb0fc105193523e518cd9c109ccae35bb2188
  - relation: ancestor
  - meaning: authority-neutral repository seed only
- execution_branch: codex/axl-v1-a0
- resume_cursor: null

## Trusted basis

- P12 Semantic Schema: https://app.notion.com/p/3d74c57a590c813c93b8cf63f128cf85?pvs=204
- P17 Platform Contract, including D-085 canonical-signing repair: https://app.notion.com/p/3d94c57a590c8149b8e7e0838a5e46b9?pvs=204
- P18 Engineering / Optimization: https://app.notion.com/p/3d94c57a590c8182a28aed9fa1a54a46?pvs=204
- P20 Verification Design / D-088: https://app.notion.com/p/3d94c57a590c81cb8b62c8391702d839?pvs=204
- P30 Implementation Plan / D-091: https://app.notion.com/p/3d94c57a590c813ba9e6fb1396334bd7?pvs=204

This package does not change those authorities. Any semantic ambiguity that cannot be resolved from them is an AUTHORITY_CONFLICT, not executor discretion.

## Purpose

Produce the first real Windows `axlic.exe` vertical slice. With no server, database, TPM identity, activation or machine mutation, the executable must consume an A0 fixture credential/state adapter, validate the P17/P20 signed-credential artifact fail-closed, evaluate local entitlements, and expose stable `status` and `entitlement` CLI result envelopes.

A0 proves the local signed-credential gate before any backend or DeviceIdentity work begins.

## Frozen implementation toolchain

### Windows production/test build

- Language: C++20.
- Build system: CMake; CI tool version = 4.4.2.
- `cmake_minimum_required`: 3.31 or newer policies compatible with CI 4.4.2.
- Compiler family: MSVC v143 / MSVC 14.44.
- Visual Studio Build Tools: VS 2022 17.14.39, build 17.14.37614.0.
- Windows SDK target API version: 10.0.26100.0.
- Architecture: x64.
- Required compiler posture: `/std:c++20 /permissive- /W4`; project-owned code warnings are blocking. Third-party warnings must not be globally promoted into project-owned warning debt.
- Release build must use normal optimized release semantics; no sanitizer result is a substitute for the required release test run.

### Production dependencies

Dependencies are fetched/pinned by immutable commit, not a floating tag:

- QCBOR v1.6.1 commit `930708bb86481e88879eb1d87fd4d664f1d69503`.
  - Allowed only behind `client/src/wire` wrapper.
  - Floats, tags, indefinite-length authorization data and unsupported generic CBOR types are forbidden by AxLicense semantics even if the library supports them.
- nlohmann/json v3.12.0 commit `55f93686c01528224f448c19128836e7df245f72`.
  - Allowed only for CLI/test JSON carrier/result serialization; never a cryptographic canonicalization source.
- Windows CNG/BCrypt system API.
  - `BCrypt` verification is the production A0 P-256/SHA-256 verification path.
  - No OpenSSL production runtime is introduced in A0.

### Test/reference dependencies

- Catch2 v3.15.3 commit `8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb`.
- Python 3.13.15 for non-production reference tooling and W1 process benchmark driver.
- Python `cryptography==50.0.1` for independent reference ECDSA verification only; it must not be linked or imported by `axlic.exe`.
- GitHub Actions must pin action implementations by immutable commit. Initial approved pins:
  - `actions/checkout` v7 commit `3d3c42e5aac5ba805825da76410c181273ba90b1`.
  - `actions/setup-python` v6 commit `ece7cb06caefa5fff74198d8649806c4678c61a1`.

## Authorized scope

Expected paths may be refined mechanically, but substantive changes are limited to these surfaces:

- `CMakeLists.txt`, `cmake/**`
- `client/CMakeLists.txt`
- `client/include/axlic/**`
- `client/src/wire/**`
- `client/src/core/**`
- `client/app/**`
- `client/tests/**`
- `reference/vectors/a0/**`
- `reference/tools/**`
- `bench/w1/**`
- `.github/workflows/a0-windows.yml`
- dependency/license metadata required by the above

A0 fixture-state/trust loading must be isolated behind an explicit adapter/seam so A1 can replace it without changing the CLI or credential-verification semantics. A fixture adapter must never be represented as production protected-state authority.

## Required behavior

### Signed credential validation

Implement the P17/P20 credential envelope and protected payload contract required by A0 fixtures:

1. bound artifact size before expensive work;
2. parse the exact envelope and payload bytes;
3. reject unsupported artifact kind, envelope/signing-input version, algorithm, key, schema major or authorization-critical enum;
4. reject forbidden/non-canonical CBOR forms;
5. enforce fixed 64-byte `r || s` P-256 signature representation and ECDSA range checks;
6. reject high-S signatures before/independently of successful cryptographic verification;
7. verify P-256/SHA-256 over the exact P17 credential signing input bytes using Windows CNG;
8. validate required credential semantics;
9. evaluate the signed entitlement snapshot locally without network access.

Canonicality checking may parse and independently re-encode for equality, but it MUST NOT substitute repaired/re-encoded bytes for the received bytes during signature verification or acceptance. Non-canonical input is rejected.

### A0 CLI contract

Public A0 capabilities:

- `axlic status`
- `axlic entitlement <entitlement_id>`

For A0 tests, fixture state/trust may be injected only through an explicitly test/A0 adapter; do not make fixture paths or fixture private keys part of the normal product contract.

`stdout` must contain exactly one UTF-8 JSON `CommandResult` object followed by a newline. Required top-level fields:

- `contract_version` = `1.0`
- `ok`
- `code`
- `category`
- `retryable`
- `correlation_ref`
- `data` when applicable

A0 stable code vocabulary:

- `OK`
- `CREDENTIAL_NOT_FOUND`
- `CREDENTIAL_INVALID`
- `CREDENTIAL_UNSUPPORTED`
- `COMMAND_INVALID`
- `INTERNAL_ERROR`

`entitlement` absence is a successful query with `ok=true`, `code=OK`, `data.granted=false`; it is not a transport/process error.

A0 `status.data` minimum contract:

- `credential_state`: `valid | absent | invalid`
- when valid: `authority_revision`, `credential_generation`

A0 `entitlement.data` minimum contract:

- `entitlement_id`
- `granted`
- optional `value_u64` only when the signed entitlement uses bounded-u64 semantics

Do not emit credential bytes, signatures, fixture private keys, full signed payloads, raw identity material or arbitrary exception text to stdout/stderr.

## Required A0 reference corpus

The executor must materialize a small non-production reference encoder/oracle under `reference/tools` and checked-in literal vectors under `reference/vectors/a0`. The reference encoder must not call production `client/src/wire` code.

Blocking A0 corpus subset:

Positive:

1. canonical perpetual credential with presence entitlement;
2. canonical bounded-validity credential with bounded-u64 entitlement;
3. same entitlement set supplied to the reference encoder in different input order produces identical canonical payload/signing-input bytes.

Negative:

1. one-bit tamper in an authorization-critical payload byte;
2. wrong public key;
3. wrong `algorithm_id`;
4. wrong `artifact_kind` / credential domain;
5. unknown schema major;
6. non-minimal integer/length encoding;
7. indefinite-length item;
8. CBOR tag;
9. duplicate map key;
10. wrong canonical map-key order;
11. forbidden null in a required/optional-absent position;
12. signature wrong length;
13. r/s out of range;
14. high-S equivalent signature;
15. oversized artifact and excess nesting/item-count boundary cases.

Golden data must include the semantic fixture, expected `payload_bytes`, exact P17 credential signing-input bytes, trusted fixture public key and fixed valid signature/artifact. The fixture private key may exist only in reference/test material and must be unmistakably test-only.

## Required tests and oracles

### T-A0-01 Build

Command/oracle: configure and build x64 Release using the frozen MSVC/CMake toolchain.
Expected: `axlic.exe`, unit/integration tests and reference tools build without project-owned warnings.
Blocking reason: a reviewer cannot inspect or execute the vertical slice otherwise.

### T-A0-02 EV-01 core canonical/crypto corpus

Command/oracle: CTest/Catch2 executes the complete A0 positive/negative corpus against production wire/verifier code.
Expected: all positives accepted; all negatives fail closed in the expected error family.
Blocking reason: uniquely covers FM-01/FM-09 canonical/crypto acceptance failures.

### T-A0-03 Independent reference cross-check

Command/oracle: Python reference tooling regenerates/validates A0 literal canonical bytes and uses `cryptography==50.0.1` to verify fixed golden signatures and production test-signature output.
Expected: byte-exact canonical match; independent verifier accepts valid low-S signatures and rejects invalid ones.
Blocking reason: production C++ implementation cannot be its own oracle.

### T-A0-04 CLI contract

Command/oracle: process-level tests launch the Release `axlic.exe` against fixture adapter states for valid, absent, invalid and unsupported credential cases and query at least one granted and one absent entitlement.
Expected: exactly one parseable JSON result on stdout per invocation; required `1.0` fields/codes/data semantics hold; stderr is non-authoritative and contains no fixture secret/canary.
Blocking reason: uniquely covers the first public product integration contract.

### T-A0-05 Offline/no-server boundary

Command/oracle: A0 tests execute with network unavailable and with no server process/configuration present; dependency/import inspection confirms A0 has no server/HTTP implementation dependency.
Expected: valid local `status` and `entitlement` behavior is unchanged.
Blocking reason: independently demonstrates the V1 local-runtime requirement at the first usable slice.

### T-A0-06 W1 benchmark harness

Command/oracle: Release `axlic.exe status` benchmark on Windows using 20 warm-up + at least 200 measured invocations; record p50/p95/p99 plus credential verification timing where instrumented without secrets.
Expected: a machine-readable baseline report is produced.
Blocking reason: report presence/harness operability is required for A0 closure, but numeric P18 threshold PASS is NOT an A0 blocking criterion; EV-07 becomes blocking at release qualification.

## Hosted verification

Required:

- GitHub Actions Windows job on the exact result revision.
- Build x64 Release.
- Run T-A0-01 through T-A0-05.
- Run T-A0-06 and upload the report even if values exceed later release budgets.
- Upload one reviewer-accessible artifact named `AXL-V1-A0-evidence` containing:
  - EV-01 A0 vector/report manifest;
  - independent-reference report;
  - CLI process-contract report;
  - W1 baseline JSON;
  - build/toolchain identity summary.

Optional/corroborative:

- Debug build.
- ASan-capable secondary configuration where supported.
- random/fuzz CBOR seeds beyond the frozen deterministic negative corpus.
- duplicate local execution of the same oracle already proven by hosted CI.

## Evidence classification

Blocking for A0 package closure:

- EV-01 A0 subset materialized and all frozen vectors pass.
- EV-06 A0 CLI contract subset materialized and process tests pass.
- T-A0-05 local/offline/no-server boundary pass.
- exact hosted Windows build/test run on result revision.
- reviewer-accessible evidence artifact and exact refs.

Corroborative/non-blocking for A0 package closure:

- EV-07 numeric performance threshold; only the baseline artifact is required at A0.
- extended fuzzing.
- memory/cold-start budget thresholds not yet designated by P20 as A0 blocking.

## Forbidden changes

- no `server/**` implementation;
- no PostgreSQL, HTTP server or control-plane implementation;
- no `RegisterDeviceIdentity`, Grant mutation, activation, refresh, offline activation, rehost, factory or admin operation implementation;
- no TPM/Software KSP identity establishment or machine-wide protected-state mutation; those are A1;
- no public DLL/SDK ABI;
- no private-key export or generic `sign(bytes)` product API;
- no alternate crypto algorithm/profile;
- no second credential format or JSON authorization representation;
- no weakening of signature, canonicality, low-S or fail-closed checks for benchmark performance;
- no production use of fixture signer/private key/reference Python dependencies.

## EXECUTION_CLOSURE_CONTRACT

```yaml
EXECUTION_CLOSURE_CONTRACT:
  implementation:
    required_changes:
      - establish C++20/CMake Windows client workspace for A0 only
      - implement narrow P17 credential CBOR/envelope parser and canonical encoder/checker
      - implement P20 P-256/SHA-256 fixed-width low-S verifier using Windows CNG
      - implement RuntimeLicense A0 credential/entitlement evaluation
      - implement axlic status and entitlement CommandResult 1.0 CLI
      - isolate A0 fixture-state/trust adapter from production seams
      - materialize independent A0 reference vectors/tooling
      - add Windows hosted CI and W1 baseline harness
    forbidden_changes:
      - any server/backend/PostgreSQL implementation
      - any DeviceIdentity/CNG persisted-key establishment
      - any online activation/refresh/offline/rehost/factory/admin implementation
      - public DLL ABI or alternate credential/crypto semantics
      - production dependency on reference fixture keys or Python tooling

  tests:
    required:
      - id: T-A0-01
        command_or_oracle: frozen-toolchain x64 Release configure/build
        expected_result: build succeeds without project-owned warnings
        blocking_reason: executable vertical slice must be buildable/reviewable
      - id: T-A0-02
        command_or_oracle: CTest/Catch2 EV-01 A0 corpus
        expected_result: all positives accept and all frozen negatives fail closed
        blocking_reason: unique canonical/crypto safety coverage
      - id: T-A0-03
        command_or_oracle: independent Python canonical/crypto cross-check
        expected_result: byte-exact vectors and independent signature verification agree
        blocking_reason: production implementation cannot be its own oracle
      - id: T-A0-04
        command_or_oracle: process-level axlic status/entitlement contract tests
        expected_result: CommandResult 1.0 and safe output semantics hold
        blocking_reason: unique public integration-contract coverage
      - id: T-A0-05
        command_or_oracle: offline/no-server execution and dependency-boundary check
        expected_result: local signed entitlement works with no server/network dependency
        blocking_reason: unique product offline-runtime proof
      - id: T-A0-06
        command_or_oracle: W1 20 warmup + >=200 measured Release invocations
        expected_result: machine-readable baseline produced
        blocking_reason: harness/report existence only; numeric threshold is corroborative at A0

  hosted_verification:
    required:
      - GitHub Actions Windows Release build/tests on exact result revision
      - reviewer-accessible AXL-V1-A0-evidence artifact
    optional:
      - Debug build
      - sanitizer configuration where supported
      - extended random fuzzing

  evidence:
    blocking:
      - EV-01 A0 canonical/crypto subset report
      - EV-06 A0 CLI contract subset report
      - T-A0-05 offline/no-server report
      - exact hosted run/job/artifact refs bound to result revision
    corroborative:
      - EV-07 A0 W1 numeric baseline
      - extended fuzz/ASan reports

  terminal_success:
    all_of:
      - every required change is present and no forbidden scope is introduced
      - T-A0-01 through T-A0-05 PASS
      - T-A0-06 produces a baseline artifact
      - required hosted verification on exact result revision is successful
      - AXL-V1-A0-evidence is reviewer-accessible
      - result revision and materialized_ref are exact and repository-resolvable

  terminal_blockers:
    explicit_classes:
      - AUTHORITY_CONFLICT
      - MISSING_REQUIRED_INPUT
      - BLOCKED_REPOSITORY_IDENTITY
      - ENVIRONMENT_BLOCKER
      - FROZEN_VERIFICATION_FAILURE
      - NEW_HIGH_IMPACT_FAILURE_MODE

  return_policy:
    continue_until_terminal_state: true
```

## P32 return contract

Return to CONTROL_REVIEW only after terminal success or an explicit terminal blocker. A local-only working tree is insufficient.

Required return fields:

- `task_id: AXL-V1-A0`
- `status: READY_FOR_CONTROL_REVIEW | BLOCKED_*`
- `repository: github/Mostorm-Labs/axlic`
- `actual_starting_revision`
- `result_revision`
- `materialized_ref`
- `package_materialization_ref`
- `evidence_input_refs`
- exact GitHub Actions run / attempt / job / artifact refs required by this package
- concise changed-file set and blocker class if blocked

Do not emit or imply P34 PASS from P32.
