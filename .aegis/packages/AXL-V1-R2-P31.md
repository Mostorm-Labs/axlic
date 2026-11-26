# AXL-V1-R2 P31 Task Package — Remove channel-side policyVersion

status: P32_COMPLETE_GATE_D113_PASS_WITH_FINDINGS
stage_owner: aegis-implementation
process_profile: Full
created_at: 2026-11-26
result_revision: 208502409bbf768098f0a34e42a5f49bd8e716fa
gate_decision: D-113 (PASS_WITH_FINDINGS, 2026-11-26)

## Repository identity

- provider: github
- full_name: Mostorm-Labs/axlic
- canonical_branch_at_packaging: main
- task_anchor:
  - revision: 0ab525cd795aff6ed2df1a93a319d35fe0c11078
  - relation: ancestor
  - meaning: D-111 integration-closure record on canonical main; the D-112 supersession (P30 v4 / P15 v4 / P20 v5) and this package are committed on the execution branch ahead of implementation
- execution_branch: codex/axl-v1-r2-policy-version
- resume_cursor: null

## Trusted basis

- P30 v4 (A2 slice as reworded; R2 slice): docs/authority/implementation/P30-implementation-plan.md
- P15 v4 (A2 realization addendum as reworded; invariant 4): docs/authority/architecture/P15-module-design.md
- P20 v5 (§17 EV-A2-01 and EV-A2-04 as revised; EV-A2-06): docs/authority/verification/P20-verification-design.md
- P17 §3.3 and §11 (unchanged constraints this package must satisfy): docs/authority/architecture/P17-platform-contract.md
- D-112 authorization and dispositions: docs/authority/governance/decision-log.md

This package changes none of those authorities. Any semantic ambiguity that cannot be resolved from them is an AUTHORITY_CONFLICT, not executor discretion.

## Purpose

Remove the channel-side policyVersion from the Gate-closed A2 library plane. `axlic install <artifact_path>` takes the delivered artifact bytes only; `held-version` reports the held `(authorityRevision, credentialGeneration)` pair; the license-state file no longer records `policy_version`. An executable upgrade must keep reading the state files earlier builds wrote (the retired key is ignored, never judged corrupt), so no installed credential is lost on upgrade. The credential format, the installation pipeline, entitlement evaluation and validity handling are unchanged; the binary keeps zero network code.

## Frozen R2 contract

- **Install input:** `InstallRequest` carries the artifact bytes only. Both CLI surfaces dispatch `install <artifact_path>` with exactly one argument; a line that still passes `--policy-version`, and an install with a missing or extra argument, is `COMMAND_INVALID` (exit 2) at dispatch, before any local state is read or written. `parse_u64_strict` loses `allow_zero`; its only caller, the scenario executable's `entitlement` timestamp, allows zero.
- **Responses:** `install` returns `data{result, credential_id, authority_revision, credential_generation}`; `held-version` returns `data{hasCredential:false}` or `data{hasCredential:true, version{authorityRevision, credentialGeneration}}`. The `CommandResult` envelope, `contract_version` `1.0` and the exit-code family are unchanged (D-112 disposition ①).
- **Local state:** the credential document has exactly the eight remaining keys; `valid_installed_credential` no longer consults `policy_version`; `decode` does not read the key, so a document that still carries it is valid state. `schema_version` and the envelope version stay unchanged (D-112 disposition ②).
- **Pipeline:** order and decisions are unchanged; the equal-version path still commits, now without a refresh (D-112 disposition ③).

## Task decomposition (RED → GREEN per task)

- T-R2-01 governance authorization: D-112, P30 v4, P15 v4, P20 v5, this package, `.aegis` manifests, root index.
- T-R2-02 RED oracles (writer case in `client/tests/test_license_state_repository.cpp`; contract in `client/tests/test_a2_cli.py`), T-R2-03 implementation to GREEN (headers, sources, `app/main.cpp`, scenario executable, unit tests, legacy-state case) and T-R2-04 README.

## Frozen tests / oracles

- EV-A2-01 as revised in P20 v5 (blocking) — installation decision matrix; the same-version reinstall leaves the stored credential unchanged. Signed fixtures come from the unchanged `reference/tools/a2_reference.py` / `reference/vectors/a2/install.json`.
- EV-A2-04 as revised in P20 v5 (blocking) — process-level CLI contract over the scenario executable: version pair only, no flag, and four command-shape cases (retired flag with its value, lone retired flag, missing argument, extra argument), each run on a readable and on an unreadable state root.
- EV-A2-06 (blocking) — the legacy credential document reads as valid state and is written back without the key; the written document carries no `policy_version`.
- EV-A2-05 (blocking) — zero regression: full `ctest`, offline boundary, no external repository references.
- Corroborative: a cross-version run with a pre-change scenario executable (old state read by the new build; new state rejected by the old build without rewriting); the negative dispatch check on the production executable.

## EXECUTION_CLOSURE_CONTRACT

```yaml
task_id: AXL-V1-R2
required_changes:
  - docs/authority/governance/decision-log.md (D-112); docs/authority/implementation/P30-implementation-plan.md (v4)
  - docs/authority/architecture/P15-module-design.md (v4); docs/authority/verification/P20-verification-design.md (v5)
  - docs/authority/index.md; .aegis/authorities.json, evidence.json, gates.json, state.json
  - client/include/axlic/core/license_state.hpp, installation.hpp, cli_numbers.hpp
  - client/src/core/installation.cpp, license_command.cpp; client/src/windows/license_state_repository.cpp; client/app/main.cpp
  - client/tests/test_installation.cpp, test_license_state.cpp, test_license_state_repository.cpp, test_license_command.cpp, test_registration.cpp
  - client/tests/a2_cli_scenario.cpp, client/tests/test_a2_cli.py; README.md
forbidden_changes:
  - client/src/wire and the CBOR credential format; reference/; client/keyring
  - the installation pipeline order, the anti-rollback decision, the equal-version commit, entitlement evaluation, validity handling
  - the state-file schema_version, envelope version and DPAPI envelope; the machine-state identity file and the A1 identity plane
  - the CommandResult envelope, contract_version and the exit-code family; A0/A1 command behavior
  - .github/workflows, cmake/, client/CMakeLists.txt, tools/
  - any network code or dependency in the binary; external repository references in any deliverable
  - adopting any external adjustment or protocol document as authority, or citing it in the revised texts
required_tests:
  - EV-A2-01 installation matrix as revised (blocking)
  - EV-A2-04 CLI contract oracle as revised (blocking)
  - EV-A2-06 local-state forward compatibility (blocking)
  - EV-A2-05 zero regression: full ctest, offline boundary, external-reference scan (blocking)
hosted_verification:
  required:
    - one `A2 Windows evidence` run (a2-windows.yml, frozen toolchain) green on the result revision
terminal_success:
  all_of:
    - every required change present, no forbidden scope introduced
    - EV-A2-01, EV-A2-04 and EV-A2-06 PASS on the result revision, each oracle first observed RED
    - full ctest green on the default build, clean under blocking warnings, and the required hosted run green on the result revision
terminal_blockers:
  explicit_classes: [AUTHORITY_CONFLICT, MISSING_REQUIRED_INPUT, BLOCKED_REPOSITORY_IDENTITY, BLOCKED_EXECUTION_DIVERGENCE, ENVIRONMENT_BLOCKER, FROZEN_VERIFICATION_FAILURE, NEW_HIGH_IMPACT_FAILURE_MODE]
return_policy:
  continue_until_terminal_state: true
```

## P32 return contract

Return to CONTROL_REVIEW only after terminal success or an explicit terminal blocker; code-complete/local-test-complete alone is insufficient. Do not emit or imply P34 PASS from P32. Required return fields:

- `task_id: AXL-V1-R2`, `status: READY_FOR_CONTROL_REVIEW | BLOCKED_*`, `repository: github/Mostorm-Labs/axlic`, `actual_starting_revision`, `result_revision`, changed-file set, per-task commit list, exact test/verification run refs (local ctest output; hosted run refs)
