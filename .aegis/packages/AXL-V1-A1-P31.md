# AXL-V1-A1 P31 Task Package — C++ Windows Identity + Protected Local State

status: APPROVED_FOR_P32_HANDOFF
stage_owner: aegis-implementation
process_profile: Full
created_at: 2026-09-14

## Repository identity

- provider: github
- full_name: Mostorm-Labs/axlic
- canonical_branch_at_packaging: main
- task_anchor:
  - revision: c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4
  - relation: ancestor
  - meaning: D-097 canonical A0 integrated baseline
- execution_branch: codex/axl-v1-a1
- resume_cursor: null

## Trusted basis

- P15 Module Design: https://app.notion.com/p/3d94c57a590c81359649d808edd4869e?pvs=204
- P17 Platform Contract: https://app.notion.com/p/3d94c57a590c8149b8e7e0838a5e46b9?pvs=204
- P18 Engineering / Optimization: https://app.notion.com/p/3d94c57a590c8182a28aed9fa1a54a46?pvs=204
- P20 Verification Design: https://app.notion.com/p/3d94c57a590c81cb8b62c8391702d839?pvs=204
- P30 Implementation Plan: https://app.notion.com/p/3d94c57a590c813ba9e6fb1396334bd7?pvs=204
- D-098 successor routing: A1 may enter P31 directly from canonical A0 baseline.

This package does not change those authorities. Any semantic ambiguity that cannot be resolved from them is an AUTHORITY_CONFLICT, not executor discretion.

## Purpose

Build the second client-first vertical slice. `axlic.exe identity` must establish or load one machine-wide Windows DeviceIdentity without any AxLicense Server, preferring a usable TPM-backed CNG identity and falling back to a software-persistent CNG identity only when the TPM path is conclusively absent/permanently unusable and fallback policy allows it. After establishment, provider/scheme/key reference is pinned in crash-safe protected local state. A transient provider failure must never silently create a weaker identity.

A1 also replaces the A0 production fixture-state dependency with a real Windows machine-state boundary. A0 fixture loading may remain only as an explicitly test-only seam.

## Frozen implementation toolchain

A1 inherits A0 production toolchain and dependency pins unless this package explicitly adds a Windows system library:

- C++20.
- CMake CI line: 4.4.2.
- MSVC v143 / MSVC 14.44, Visual Studio 2022 17.14 line.
- Windows SDK target API version: 10.0.26100.0.
- x64.
- project-owned code: `/std:c++20 /permissive- /W4` with project warnings blocking.
- QCBOR v1.6.1 exact commit `930708bb86481e88879eb1d87fd4d664f1d69503` remains the A0 wire dependency.
- nlohmann/json v3.12.0 exact commit `55f93686c01528224f448c19128836e7df245f72` may be reused for local protected-state plaintext/carrier JSON; it is not authorization canonicalization.
- Catch2 v3.15.3 exact commit `8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb` remains the test framework.

A1 may link these Windows system libraries/APIs without adding a third-party runtime:

- `ncrypt.lib` / NCrypt persisted-key APIs.
- `bcrypt.lib` / BCrypt hash, RNG, public-key verification helpers.
- `crypt32.lib` / DPAPI `CryptProtectData` and `CryptUnprotectData`.
- `advapi32.lib` / security descriptor / ACL helpers.
- `shell32.lib` or equivalent known-folder API for `FOLDERID_ProgramData`.

No OpenSSL production runtime, custom private-key file, registry private-key store, service, agent, public DLL, server runtime or database is authorized by A1.

## Frozen identity schemes and public identity representation

Initial identity epoch is `1`.

Stable scheme identifiers introduced by A1:

- TPM hardware-backed: `axl-win-cng-tpm-p256-v1`
- software-persistent fallback: `axl-win-cng-software-p256-v1`

Both use a provider-owned persisted P-256 private key. `identity_value` is the lowercase hexadecimal encoding of the exact 64-byte affine public key `X || Y`, where X and Y are each 32-byte big-endian coordinates obtained from a validated P-256 public-key export. The combination `(scheme_id, identity_value, identity_epoch)` is the local DeviceIdentity claim consumed by A2; A1 does not create a server Device record.

The private key must never be serialized by AxLicense, placed in local JSON, command output, logs or support evidence.

## Windows provider realization

### Provider order

First establishment follows exactly this policy:

1. TPM provider: Windows Microsoft Platform Crypto Provider (`MS_PLATFORM_CRYPTO_PROVIDER`).
2. Software fallback: Windows Microsoft Software Key Storage Provider (`MS_KEY_STORAGE_PROVIDER`) only when TPM is classified `absent_or_permanently_unusable` and local policy `allow_software_fallback=true`.
3. Otherwise fail closed.

A transient/unavailable TPM classification returns `IDENTITY_PROVIDER_UNAVAILABLE` with `retryable=true`; it MUST NOT trigger software key creation.

After local identity state commits, provider selection is pinned. Later loads open only the recorded provider/key reference; they MUST NOT rerun strongest-provider selection or silently migrate TPM to software.

### Persisted key creation

Use NCrypt-equivalent realization:

- open the selected storage provider with `NCryptOpenStorageProvider`;
- create a machine persisted P-256 key with `NCryptCreatePersistedKey`, `NCRYPT_ECDSA_P256_ALGORITHM`, and `NCRYPT_MACHINE_KEY_FLAG`;
- key name: `Auditoryworks.AxLicense.Identity.v1.` followed by 16 bytes from the Windows system RNG rendered as 32 lowercase hex characters;
- explicitly set `NCRYPT_EXPORT_POLICY_PROPERTY` to zero / no plaintext private-export permission before finalization;
- finalize with `NCryptFinalizeKey`;
- export only `BCRYPT_ECCPUBLIC_BLOB`, validate P-256 public magic and 32-byte coordinates, and derive the 64-byte `X || Y` public identity value;
- an explicit `BCRYPT_ECCPRIVATE_BLOB` export attempt in qualification tests must fail for the configured persisted key;
- provider qualification must perform an internal P-256 sign/verify self-test over a fixed domain-separated test digest to prove the persisted key can actually perform the future proof primitive. This is internal qualification only; A1 MUST NOT expose a generic `sign(bytes)` CLI/ABI.

If establishment fails before machine-state commit, delete the candidate key best-effort. A crash-created orphan key without committed machine state is not an identity authority and MUST NOT be auto-adopted merely by name discovery.

### Provider outcome model

The Windows adapter must normalize platform outcomes to at least:

- `usable`
- `absent_or_permanently_unusable`
- `temporarily_unavailable`
- `key_not_found_or_corrupt`
- `access_denied`
- `unexpected_failure`

The Windows status-code mapping must be isolated in the adapter and table-tested. P32 may resolve ordinary documented Windows status-code details, but it may not redefine the downgrade rule: only the `absent_or_permanently_unusable` class may authorize first-establishment software fallback.

## IdentityManager / provider boundary

A1 must introduce a narrow interface equivalent to:

```text
IIdentityProvider
  probe()
  establish(candidate_key_name)
  load(pinned_key_name)
  public_identity()
  qualify_private_non_exportability()
```

The exact C++ signature may differ, but ownership must remain:

- IdentityManager decides strongest-provider policy and pinning.
- provider adapters own NCrypt handles and platform error normalization.
- providers never decide commercial entitlement.
- no generic arbitrary-byte signing surface is public to products or CLI.

An existing committed identity path must load and validate the pinned provider/key. Temporary provider failure -> `IDENTITY_PROVIDER_UNAVAILABLE`; missing/corrupt pinned key -> `IDENTITY_RECOVERY_REQUIRED`. Neither case creates a new ordinary identity.

## Frozen machine-state realization

### State root / paths

Resolve machine-wide root from Windows `FOLDERID_ProgramData`, never from per-user AppData or HKCU.

A1 production root:

`<ProgramData>\Auditoryworks\AxLicense`

Authoritative state file:

`<root>\machine-state.v1.json`

Temporary state candidate naming:

`<root>\.machine-state.v1.<pid>.<random>.tmp`

All temp files must be created in the same directory/volume as the authoritative state file.

### Directory ACL

When A1 creates/repairs its state root under an elevated mutation path, apply an explicit protected DACL semantically equivalent to:

- LocalSystem: full control, inherited to children.
- Builtin Administrators: full control, inherited to children.
- Builtin Users: read/execute only, inherited to children.
- no ordinary-user write grant.

The implementation may express the descriptor through SDDL / Windows ACL APIs. A1 does not claim protection against a machine administrator.

### Cross-process mutation lock

Frozen logical mutex name:

`Global\Auditoryworks.AxLicense.MachineMutation.v1`

Create/open it with an explicit protected security descriptor that allows LocalSystem and Builtin Administrators to acquire/mutate. Mutating paths must:

1. acquire the machine-wide lock;
2. re-read committed state after acquisition;
3. decide whether a mutation is still needed;
4. commit or return the already-committed identity;
5. release ownership.

Two concurrent first-establishment processes must converge to the same committed identity. The second writer must not create a second authoritative identity after it acquires the lock and re-reads state.

### Protected state envelope

Outer file is bounded UTF-8 JSON:

```json
{
  "format": "axlicense-machine-state",
  "envelope_version": 1,
  "protected_blob_hex": "..."
}
```

`protected_blob_hex` is lowercase hex of the DPAPI protected bytes. The complete outer file is capped at 1 MiB in A1; larger input is rejected before DPAPI/JSON-heavy work.

DPAPI realization:

- `CryptProtectData` / `CryptUnprotectData` with machine scope (`CRYPTPROTECT_LOCAL_MACHINE`) and UI forbidden.
- optional entropy is the exact UTF-8 byte string `AxLicense/MachineState/v1`.
- DPAPI plaintext is UTF-8 JSON; DPAPI confidentiality/integrity is defense-in-depth and never replaces SignedLicenseCredential verification.

A1 plaintext schema:

```json
{
  "schema_version": {"major": 1, "minor": 0},
  "state_generation": 1,
  "identity": {
    "scheme_id": "axl-win-cng-tpm-p256-v1",
    "identity_epoch": 1,
    "identity_value": "<128 lowercase hex chars>",
    "provider_kind": "tpm",
    "provider_name": "Microsoft Platform Crypto Provider",
    "key_name": "Auditoryworks.AxLicense.Identity.v1.<32 hex chars>"
  }
}
```

Software provider state uses `provider_kind=software`, provider name `Microsoft Software Key Storage Provider`, and the software scheme ID. No private key bytes, signature secret, activation secret, token, credential high-watermark or server Device ID exists in A1 state.

Unknown major or a future unsupported minor must not be rewritten by A1. Return stable local-state unsupported/corrupt semantics rather than silently down-converting a future state.

### Atomic replace

For every authoritative state write:

1. machine mutation lock is held;
2. committed state has been re-read;
3. create the complete candidate temp file in the state root;
4. flush candidate file data with `FlushFileBuffers` or equivalent;
5. close the candidate handle;
6. atomically replace the destination on the same volume using `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` or a demonstrably equivalent same-volume replace primitive with write-through semantics;
7. only after successful replace may the operation publish success;
8. stale/incomplete temp files are never selected as authority on startup.

A crash must leave either the old complete committed state or the new complete committed state, never a partial parsed state.

## A0 fixture seam transition

Current `main@c83e38de...` still compiles `A0FixtureAdapter` directly into production `axlic.exe`. A1 MUST remove that production dependency.

Required transition:

- production `axlic.exe` uses the A1 machine-state / identity boundary;
- A0 fixture adapter remains available only under `BUILD_TESTING` or an equivalent explicit test-only target;
- `AXLIC_A0_FIXTURE_STATE` must not affect the production executable;
- preserve A0 credential/wire/runtime tests and CLI semantic regressions through a test-only fixture surface or refactored command application;
- do not change P17/P20 credential semantics or golden vectors merely to accommodate the new state source.

## A1 CLI contract

Add public command family:

`axlic identity`

Successful result remains CommandResult contract `1.0` and has at least:

```json
{
  "identity_state": "ready",
  "scheme_id": "axl-win-cng-tpm-p256-v1",
  "identity_epoch": 1,
  "identity_value": "<public 128-char lowercase hex>"
}
```

Do not output provider key name, NCrypt handle/provider internals, DPAPI blob, private key, self-test signature or raw Windows error strings.

A1 stable semantic codes added to the existing code vocabulary:

- `LOCAL_WRITE_PRIVILEGE_REQUIRED`
- `LOCAL_STATE_BUSY`
- `LOCAL_STATE_CORRUPT`
- `LOCAL_STATE_UNSUPPORTED`
- `IDENTITY_PROVIDER_UNAVAILABLE`
- `IDENTITY_PROVIDER_UNSUPPORTED`
- `IDENTITY_RECOVERY_REQUIRED`

`IDENTITY_PROVIDER_UNAVAILABLE` and `LOCAL_STATE_BUSY` are retryable when safe. Existing A0 command/result codes remain compatible.

If machine state is absent, first identity establishment is a machine mutation and must fail before provider key creation when the process lacks machine-state write authority. If state already exists, A1 may return the committed public identity only after protected-state validation and pinned provider/key validation succeed.

## Authorized repository scope

Expected paths:

- `client/CMakeLists.txt`
- `client/include/axlic/core/**`
- `client/include/axlic/windows/**`
- `client/src/core/**`
- `client/src/windows/**`
- `client/app/**`
- `client/tests/**`
- `integration-tests/windows/**`
- `reference/tools/**` only when needed for A0 regression reuse; no new authorization semantics
- `bench/**` only for inherited A0 regression compatibility
- `.github/workflows/a1-windows.yml`
- dependency/license metadata if Windows system-link realization requires documentation

Mechanical top-level CMake/test wiring changes are allowed.

## Forbidden changes

- no `server/**`, Node.js, TypeScript or PostgreSQL implementation;
- no `RegisterDeviceIdentity` or canonical server Device record;
- no online activation, refresh, offline activation, rehost, factory or admin operation;
- no credential installation/high-watermark semantics; those remain A3+;
- no Product Backend / Organization / SKU / IAM logic;
- no public DLL or service/agent;
- no registry/file private-key serialization;
- no public generic `sign(bytes)` API/CLI;
- no user-selectable `--force-software` downgrade switch in production;
- no silent TPM-to-software downgrade after an identity has been pinned;
- no new credential/CBOR/crypto profile;
- no weakening or regeneration of A0 golden vectors to make A1 tests pass.

## Required tests / evidence

### T-A1-01 — Build + A0 regression

Oracle:

- frozen Windows x64 Release toolchain build;
- all existing A0 unit/canonical/crypto/runtime regressions remain green;
- production target no longer depends on A0 fixture adapter/environment variable.

Blocking reason: A1 is a successor slice and must not destroy the Gate-closed A0 signed-license foundation.

### T-A1-02 — Provider policy matrix

Run deterministic unit tests through fake/injected provider adapters for at least:

1. TPM usable -> TPM selected, software provider never established;
2. TPM `temporarily_unavailable` -> retryable failure, zero software establishment calls;
3. TPM `absent_or_permanently_unusable` + fallback allowed -> software selected;
4. TPM unavailable/permanent + fallback disallowed -> unsupported/policy failure, no software key;
5. committed TPM state + later transient TPM failure -> unavailable, no selector/fallback;
6. committed TPM state + missing key -> recovery required, no new identity;
7. committed software state remains software on later TPM availability; no automatic migration.

Blocking reason: uniquely detects silent downgrade/provider-pinning failures before relying on hardware availability.

### T-A1-03 — Real Software KSP qualification

On hosted Windows or another reviewer-accessible real Windows environment using Microsoft Software Key Storage Provider:

- create a machine persisted P-256 key through production adapter;
- export/validate public key and derive stable identity value;
- close/reopen provider/key and confirm identity is unchanged;
- internal sign/verify qualification succeeds;
- explicit private-key plaintext export attempt fails;
- cleanup the test key after evidence capture.

Blocking reason: fake adapters cannot prove real CNG persisted-key/non-exportability behavior.

### T-A1-04 — Physical TPM qualification

Required A1 blocking evidence from at least one reviewer-identifiable Windows 11 reference machine with a physical TPM 2.0 and Microsoft Platform Crypto Provider:

- production selector chooses TPM scheme on first establishment;
- persisted identity survives process restart;
- evidence includes one OS restart/reboot persistence check before terminal A1 closure;
- identity value remains stable after restart;
- internal sign/verify qualification succeeds;
- explicit plaintext private-key export attempt fails;
- no software identity is created in the successful TPM establishment scenario.

The exact machine serial/user identity must not be logged. Evidence records OS build, TPM presence/provider class, AxLicense result revision, scheme ID, a test-run correlation ID, PASS/FAIL checks, and a hash/fingerprint of the public identity when useful.

A Windows 10 reference-platform run remains required by final P20/A8 release qualification but is not an additional A1 blocking matrix item; A1 blocks on one real physical-TPM Windows 11 qualification because TPM behavior is the slice's unique high-impact residual gap.

If no physical TPM execution surface is accessible, P32 returns `BLOCKED_ENVIRONMENT`; it MUST NOT replace T-A1-04 with a mock and claim terminal success.

### T-A1-05 — Protected-state corruption / version tests

Using the production state codec/protection boundary where practical:

- valid DPAPI protected envelope round-trips;
- modified/truncated DPAPI blob -> `LOCAL_STATE_CORRUPT`;
- malformed outer JSON / odd or non-hex blob -> fail closed;
- >1 MiB outer state rejected before expensive decode;
- wrong envelope version / unsupported state schema version -> stable unsupported/corrupt result and no rewrite;
- state never contains private-key bytes.

Blocking reason: uniquely covers protected-state tamper/corruption acceptance.

### T-A1-06 — Cross-process first-establishment serialization

Run at least two real processes concurrently against one clean machine-state root using production lock/IdentityManager semantics. Expected:

- at most one authoritative identity is committed;
- both successful callers converge to the same scheme/epoch/identity value;
- second writer re-reads state after acquiring the mutex;
- no second authoritative provider key is adopted;
- lock busy/timeout path, if exposed, returns stable retryable semantics rather than bypassing serialization.

Blocking reason: in-process unit locking cannot prove the Windows cross-process single-writer contract.

### T-A1-07 — Crash/atomic-replace matrix

Provide test-only deterministic kill/fail points around the state writer at minimum:

- before temp write;
- after temp write;
- after temp flush;
- immediately before replace;
- immediately after replace but before success publication.

Each scenario must terminate the writer abruptly, restart a reader, and prove authoritative state is exactly the old complete state or new complete state. Partial JSON/DPAPI candidate or temp file must never become authority.

Blocking reason: ordinary unit tests cannot detect crash-window half-state failures.

### T-A1-08 — Identity CLI / minimum-disclosure contract

Process-level checks for `axlic identity`:

- stdout exactly one UTF-8 CommandResult JSON object;
- success exposes only public identity fields frozen above;
- transient provider, unsupported provider, recovery-required, local-state-corrupt and insufficient-write-authority cases map to stable semantic families;
- argv/stdout/stderr/test evidence do not contain private key material, DPAPI plaintext, provider key name or raw provider handle;
- `AXLIC_A0_FIXTURE_STATE` has no effect on production `axlic.exe`;
- existing `status` / `entitlement` command semantics remain compatible.

Blocking reason: A1 creates a new product-facing command and a new secret-bearing platform boundary.

## Hosted verification

Required on exact result revision:

1. GitHub Actions Windows Release build/test job, preferably `windows-2025` or the current repository-supported Windows image, for T-A1-01/02/03/05/06/07/08 where the hosted runner can execute them.
2. Existing A0 EV-01 / EV-06 regression obligations rerun or consumed through the same trusted test targets without changing golden vectors.
3. Reviewer-accessible artifact named `AXL-V1-A1-evidence` containing at minimum:
   - build/toolchain/result-revision identity;
   - provider-policy matrix report;
   - Software KSP qualification report;
   - protected-state corruption/version report;
   - cross-process serialization report;
   - crash-matrix report;
   - identity CLI/minimum-disclosure report;
   - A0 regression summary.
4. T-A1-04 physical TPM evidence must also be reviewer-accessible and exact-result-bound. It may be produced by a self-hosted runner or by a deterministic qualification bundle generated by the exact result revision and uploaded/attached through an approved evidence path. Local prose alone is insufficient.

Optional/corroborative for A1:

- Windows 10 real-machine qualification before A8;
- additional TPM vendors/firmware;
- sanitizer/static-analysis runs;
- stress repetitions beyond the frozen concurrency/crash matrix;
- performance measurements beyond the existing P18 budgets unless they expose a new high-impact failure.

## A1 performance/reference budgets

A1 does not introduce a new independent performance Gate beyond P18. Record these as corroborative measurements when available:

- uncontended local mutation lock p95 target <= 10 ms;
- local candidate flush + replace p95 target <= 100 ms;
- CNG key open/proof p95 target <= 750 ms;
- first identity establishment p95 target <= 3 s, p99 <= 5 s.

Exceeding a target during A1 does not automatically block unless the observed behavior creates a new high-impact product failure mode; final release performance classification remains P20/A8 governed.

## Evidence classification

Blocking for A1 Gate input:

- inherited A0 regression PASS;
- EV-03 A1 subset: T-A1-02 provider policy + T-A1-03 real Software KSP + T-A1-04 real physical TPM qualification;
- EV-04 A1 subset: T-A1-05 protected-state integrity + T-A1-06 cross-process serialization + T-A1-07 crash/atomic replace;
- EV-06 A1 subset: T-A1-08 identity CLI/minimum disclosure;
- exact hosted run/job/artifact refs bound to result revision;
- reviewer-accessible exact-result-bound TPM evidence.

Corroborative/non-blocking for A1:

- full Windows 10 + Windows 11 matrix beyond the one required TPM Windows 11 machine;
- extended stress/fuzz/static analysis;
- A1 performance target values themselves;
- duplicate local execution of the same hosted oracle.

## EXECUTION_CLOSURE_CONTRACT

```yaml
EXECUTION_CLOSURE_CONTRACT:
  implementation:
    required_changes:
      - introduce IdentityManager / IdentityPolicy and narrow IIdentityProvider boundary
      - implement Windows TPM CNG provider and software-persistent CNG fallback provider
      - implement frozen scheme IDs and 64-byte P-256 public identity representation
      - implement non-exportable persisted P-256 machine key creation/load and internal sign/verify qualification
      - implement protected ProgramData machine-state repository using DPAPI machine scope
      - implement protected machine-wide mutation mutex with re-read-after-lock semantics
      - implement same-volume temp/flush/atomic-replace state commit
      - add `axlic identity` CommandResult 1.0 behavior and stable A1 error families
      - remove A0 fixture adapter from production axlic dependency while preserving it as test-only regression seam
      - materialize A1 hosted evidence and real physical-TPM evidence inputs
    forbidden_changes:
      - any server/Node/TypeScript/PostgreSQL implementation
      - RegisterDeviceIdentity or any server Device mutation
      - activation/refresh/offline/rehost/factory/admin implementation
      - credential install/high-watermark implementation
      - public DLL/service/agent
      - private-key serialization or public generic sign(bytes)
      - production software-provider force/downgrade switch
      - credential/CBOR/crypto-profile changes

  tests:
    required:
      - id: T-A1-01
        command_or_oracle: frozen x64 Release build plus all inherited A0 regressions
        expected_result: build succeeds, A0 regressions PASS, production axlic has no fixture-state dependency
        blocking_reason: successor must preserve Gate-closed A0 foundation
      - id: T-A1-02
        command_or_oracle: deterministic provider-policy/pinning matrix
        expected_result: only permanent/absent TPM state permits initial software fallback; transient/pinned failures never downgrade
        blocking_reason: unique silent-downgrade/provider-pinning coverage
      - id: T-A1-03
        command_or_oracle: real Windows Software KSP qualification
        expected_result: persisted P-256 identity reopens stably, signs/verifies internally, private plaintext export fails
        blocking_reason: fake adapters cannot prove real CNG persistence/non-exportability
      - id: T-A1-04
        command_or_oracle: exact-result-bound physical TPM 2.0 Windows 11 qualification including reboot persistence
        expected_result: TPM scheme selected, identity stable, private export fails, no software identity created
        blocking_reason: only real TPM execution closes the hardware-provider residual failure mode
      - id: T-A1-05
        command_or_oracle: protected-state corruption/version/bounds corpus
        expected_result: valid round-trip; malformed/tampered/oversized/unsupported state fails closed without rewrite
        blocking_reason: unique local protected-state integrity coverage
      - id: T-A1-06
        command_or_oracle: concurrent real-process first-establishment scenario
        expected_result: one authoritative identity; callers converge after re-read-under-lock
        blocking_reason: unique cross-process single-writer coverage
      - id: T-A1-07
        command_or_oracle: deterministic kill-point atomic-replace matrix
        expected_result: after every abrupt termination only old or new complete state is authoritative
        blocking_reason: unique crash-window durability coverage
      - id: T-A1-08
        command_or_oracle: process-level identity CLI and canary/minimum-disclosure scan
        expected_result: CommandResult contract stable and no forbidden identity/provider/private-state disclosure
        blocking_reason: unique public contract and secret-boundary coverage

  hosted_verification:
    required:
      - exact-result GitHub Actions Windows Release run for hosted-capable required tests
      - reviewer-accessible AXL-V1-A1-evidence artifact
      - exact-result-bound reviewer-accessible physical TPM qualification evidence
    optional:
      - Windows 10 early qualification
      - extra TPM vendor matrix
      - sanitizer/static analysis
      - extended stress runs

  evidence:
    blocking:
      - EV-03 A1 provider-policy + Software KSP + physical TPM subset
      - EV-04 A1 protected-state + cross-process + crash subset
      - EV-06 A1 identity CLI/minimum-disclosure subset
      - inherited A0 regression evidence on result revision
      - exact hosted run/job/artifact refs and exact TPM evidence ref
    corroborative:
      - Windows 10/full release matrix before A8
      - extended stress/static analysis
      - A1 performance measurements

  terminal_success:
    all_of:
      - every required A1 implementation change is present and no forbidden scope is introduced
      - T-A1-01 through T-A1-08 PASS
      - exact hosted Windows verification on result revision is successful
      - AXL-V1-A1-evidence is reviewer-accessible
      - physical TPM evidence is reviewer-accessible and bound to the exact result revision
      - result_revision, materialized_ref, package_materialization_ref and evidence refs are repository/provider resolvable

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

- `task_id: AXL-V1-A1`
- `status: READY_FOR_CONTROL_REVIEW | BLOCKED_*`
- `repository: github/Mostorm-Labs/axlic`
- `actual_starting_revision`
- `result_revision`
- `materialized_ref`
- `package_materialization_ref`
- `evidence_input_refs`
- exact hosted run / attempt / job / artifact refs
- exact physical-TPM evidence ref and environment class, without machine/user secrets
- concise changed-file set and blocker class if blocked

Do not emit or imply P34 PASS from P32.
