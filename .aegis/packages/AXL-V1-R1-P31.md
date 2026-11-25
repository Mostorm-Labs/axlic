# AXL-V1-R1 P31 Task Package — Publish Pipeline (committed public keys)

status: FROZEN_READY_FOR_P32
stage_owner: aegis-implementation
process_profile: Full
created_at: 2026-11-25
result_revision: null
gate_decision: null

## Repository identity

- provider: github
- full_name: Mostorm-Labs/axlic
- canonical_branch_at_packaging: main
- task_anchor:
  - revision: 81370f31aac0af1e9444a831f89bee781dff3ffd
  - relation: ancestor
  - meaning: D-108 integration-closure record on canonical main; the D-109 supersession (P30 v3 / P15 v3 / P20 v4) and this package are committed on the execution branch ahead of implementation
- execution_branch: codex/axl-v1-r1-publish
- resume_cursor: null

## Trusted basis

- P30 v3 (R1 slice and the revised A2 trust-keyring bullet): docs/authority/implementation/P30-implementation-plan.md
- P15 v3 (`TrustKeyringSource` realization): docs/authority/architecture/P15-module-design.md
- P20 v4 (§17 as revised; §18 EV-R1-01…06): docs/authority/verification/P20-verification-design.md
- D-109 authorization and positioning: docs/authority/governance/decision-log.md

This package changes none of those authorities. Any semantic ambiguity that cannot be resolved from them is an AUTHORITY_CONFLICT, not executor discretion.

## Purpose

Give the Gate-closed `axlic.exe` a repeatable, traceable manufacturing path: two thin entry workflows — `release.yml` (push of a `vX.Y.Z` tag, plus manual re-publish of an existing tag) and `test-publish.yml` (manual only) — call one reusable pipeline, `axlic-publish.yml`. Its `build` job proves the release identity, derives the environment trust floor offline from the committed public key, builds the complete tree with that floor, runs the full test suite and the existing A1/A2 evidence steps, proves that the executable embeds that environment's key only, and assembles a self-contained zip; its `publish` job re-verifies the package after download and publishes it to GitHub Releases. The verifier, the floor document format and the default local build (empty floor) are unchanged; the binary keeps zero network code.

## Frozen implementation toolchain

R1 inherits the A0/A1/A2 pins: runner `windows-2022`; CMake 4.4.2 (pip); VS 2022 17.14 / MSVC 19.44 (`v143`); Windows SDK 10.0.26100.0; x64; Python 3.13.15 with `cmake==4.4.2 cryptography==50.0.1`; packaging and publishing scripts use the Python standard library only.

- GitHub Actions, pinned by full commit SHA as in the existing workflows: `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1`, `actions/setup-python@ece7cb06caefa5fff74198d8649806c4678c61a1`, `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02`; no other third-party action; downloads and release operations use the runner's preinstalled `gh`. The workflows read no secret other than the automatic `GITHUB_TOKEN`; the only trust-key inputs are `client/keyring/license-test.pub.pem` (key_id `license-test`, SPKI SHA-256 `ca77…0aea`) and `client/keyring/license-prod.pub.pem` (key_id `license-prod`, SPKI SHA-256 `58bb…77a8`).

## Frozen R1 design

- **Pipeline shape:** `axlic-publish.yml` is `workflow_call`-only; inputs `environment` (`test` | `prod`), `ref`, `publish` (default true). `build` holds `contents: read`; `publish` (`contents: write` + `actions: read`) runs only when `publish` is true. Inputs reach scripts through step `env:`; nothing is interpolated into a `run:` block.
- **Trust floor:** `tools/build_trust_floor.py --environment <env> --out <path>` parses exactly one PEM `PUBLIC KEY` block, requires the pinned P-256 SubjectPublicKeyInfo fingerprint and writes the `axlic-trust-floor` document atomically. CMake embeds the document named by the cache variable `AXLIC_TRUST_FLOOR_FILE` (default: the checked-in empty `client/keyring/trust-floor.json`) and re-configures when that file changes. Each artifact embeds its own environment's key only.
- **Version, tag and release rules:** production publishes only an existing tag matching `^v(\d+\.\d+\.\d+)$` whose version equals the tagged commit's `project(axlicense VERSION …)` and whose commit is an ancestor of `origin/main`; a tag is never created or moved; manual re-publish replaces the same-named assets. Test: any ref (default `main`), version `<project version>-test.<yyyyMMdd>.<run_number>` (UTC), tag `v<version>`, published as a Pre-release.
- **Package layout:** `axlic-<version>-win-x64.zip` with one top-level directory holding `axlic.exe`, the app-local VC++ runtime DLLs it imports, `THIRD_PARTY_NOTICES.md`, `licenses/`, `SHA256SUMS.txt` and `build-manifest.json`; `.zip.sha256` alongside. Every import must be a Windows component or one of the bundled runtime DLLs. Release notes state the environment, the embedded key id and fingerprint, the source revision and — for production — that the build is a pre-A8 engineering release.
- `r1-windows.yml` runs the pipeline on the execution branch for both environments with publishing disabled; it is the hosted evidence for this package.

## Task decomposition (RED → GREEN per task)

- T-R1-01 governance authorization: D-109, P30 v3, P15 v3, P20 v4, this package, `.aegis` manifests, root index.
- T-R1-02 committed public keys and `tools/build_trust_floor.py`; removal of `tools/fetch_trust_floor.py`. T-R1-03 CMake `AXLIC_TRUST_FLOOR_FILE` and the embedded-floor unit test.
- T-R1-04–07 `tools/check_embedded_trust_floor.py`, `tools/release_identity.py`, `tools/package_axlic.py`, `tools/publish_release.py`. T-R1-08 tooling unit tests under `ctest`; the four workflows and `tools/tests/test_workflow_contract.py`; README publishing section.

## Frozen tests / oracles

- EV-R1-01…06 as registered in P20 v4 §18 (all blocking).
- Corroborative: the dual-environment hosted run on the execution branch; the first test-channel publish after integration; the initial production release.

## EXECUTION_CLOSURE_CONTRACT

```yaml
task_id: AXL-V1-R1
required_changes:
  - docs/authority/governance/decision-log.md (D-109); docs/authority/implementation/P30-implementation-plan.md (v3)
  - docs/authority/architecture/P15-module-design.md (v3); docs/authority/verification/P20-verification-design.md (v4); docs/authority/index.md; .aegis/authorities.json, evidence.json, gates.json, state.json
  - client/keyring/license-test.pub.pem, client/keyring/license-prod.pub.pem
  - tools/build_trust_floor.py, tools/check_embedded_trust_floor.py, tools/release_identity.py, tools/package_axlic.py, tools/publish_release.py; tools/tests/test_*.py for every tool and for the workflow contract
  - client/CMakeLists.txt (AXLIC_TRUST_FLOOR_FILE, CMAKE_CONFIGURE_DEPENDS, tooling test registration)
  - client/tests/test_trust_keyring.cpp (embedded-floor case); comments in embedded_trust_floor.cpp.in and trust_keyring.hpp
  - .github/workflows/axlic-publish.yml, release.yml, test-publish.yml, r1-windows.yml
  - .github/workflows/a2-windows.yml (trust-floor export step removed only)
  - README.md, THIRD_PARTY_NOTICES.md, .gitignore (/dist/)
forbidden_changes:
  - client/src, client/include (comments excepted), client/app, cmake/, client/keyring/trust-floor.json; A0 verification crypto (wire/credential.cpp, qcbor_codec.cpp)
  - .github/workflows/a0-windows.yml, a1-windows.yml, a1-physical-tpm.yml
  - any network code or dependency in the binary (offline boundary stays); external repository references in any deliverable
  - complete key rotation/distribution mechanism (overlay write side); new third-party runtime dependencies or GitHub Actions; fixture keys as any environment's trust key
required_tests:
  - EV-R1-01 published-revision zero regression; EV-R1-02 trust-floor source and fingerprint pin
  - EV-R1-03 embedded floor matches the selected environment only; EV-R1-04 package self-containment and integrity
  - EV-R1-05 keyed-binary offline boundary and deliverable hygiene; EV-R1-06 release identity
  - tooling unit and workflow-contract tests green under ctest (all items above are blocking)
terminal_success:
  all_of:
    - every required change present, no forbidden scope introduced
    - EV-R1-01…05 and the unit/contract part of EV-R1-06 PASS locally on the result revision
    - one hosted dual-environment run (r1-windows.yml) green on the result revision
    - full ctest green on the default (empty-floor) build
terminal_blockers:
  explicit_classes: [AUTHORITY_CONFLICT, MISSING_REQUIRED_INPUT, BLOCKED_REPOSITORY_IDENTITY, BLOCKED_EXECUTION_DIVERGENCE, ENVIRONMENT_BLOCKER, FROZEN_VERIFICATION_FAILURE, NEW_HIGH_IMPACT_FAILURE_MODE]
return_policy:
  continue_until_terminal_state: true
```

## P32 return contract

Return to CONTROL_REVIEW only after terminal success or an explicit terminal blocker; code-complete/local-test-complete alone is insufficient. Do not emit or imply P34 PASS from P32. Required return fields:

- `task_id: AXL-V1-R1`, `status: READY_FOR_CONTROL_REVIEW | BLOCKED_*`, `repository: github/Mostorm-Labs/axlic`, `actual_starting_revision`, `result_revision`, changed-file set, per-task commit list, exact test/verification run refs (local ctest output; hosted run refs)
