# AxLicense Authority Migration Ledger

## M0 bootstrap

Source root: `https://app.notion.com/p/3d74c57a590c817386c8e9a7a896882c`

Target repository: `Mostorm-Labs/axlic`

Repository baseline inspected for M0: `main@eb1ab6ecb6afc31a6bc178a671d4f433414ad8c5`

M0 does not change product semantics. It records the current Notion Authority inventory, repository reality, existing durable Aegis packages, known Gate/integration history, and the next untrusted stage.

## Current lifecycle baseline

The latest root Authority records P00-P20 and P30 as accepted/closed. AXL-V1-A0 and AXL-V1-A1 are Gate-closed and repository-integrated. The earliest untrusted downstream layer is **P31 AXL-V1-A2 Task Packaging**. A2 P32 coding is not authorized until its P31 package is frozen and accepted.

## Migration status vocabulary

- `pending` — Notion remains Current Authority.
- `migrating` — repository draft exists but cutover is not complete.
- `verified` — repository copy has been checked against source but has not yet become Current Authority.
- `cutover` — repository copy is Current Authority; Notion is historical/redirect only.
- `repository-native` — durable artifact already existed in Git before M0 and is preserved as repository evidence/authority according to its original lifecycle role.

## Authority inventory

| ID | Notion document | Target | Status |
|---|---|---|---|
| AXL-V1-P00 | 00 — P00 Problem Discovery — AxLicense v0.1 | `docs/authority/discovery/P00-problem-discovery.md` | cutover |
| AXL-V1-P01 | 01 — P01 Product Research — Licensing Pattern Challenge v0.1 | `docs/authority/discovery/P01-product-research.md` | cutover |
| AXL-V1-P02 | 02 — P02 Product Requirements — AxLicense V1 v0.1 | `docs/authority/discovery/P02-product-requirements.md` | cutover |
| AXL-V1-P03 | 03 — P03 Capability Traceability — AxLicense V1 v0.1 | `docs/authority/discovery/P03-capability-traceability.md` | cutover |
| AXL-V1-P10 | 10 — P10 Product Object Model — AxLicense V1 v0.1 | `docs/authority/modeling/P10-product-object-model.md` | cutover |
| AXL-V1-P11 | 11 — P11 Interaction & Behavior — AxLicense V1 v0.1 | `docs/authority/modeling/P11-interaction-behavior.md` | cutover |
| AXL-V1-P12 | 12 — P12 Semantic Schema — AxLicense V1 v0.1 | `docs/authority/modeling/P12-semantic-schema.md` | cutover |
| AXL-V1-P13 | 13 — P13 Operation & Mutation Model — AxLicense V1 v0.1 | `docs/authority/modeling/P13-operation-mutation-model.md` | cutover |
| AXL-V1-P14 | 14 — P14 System Architecture — AxLicense V1 v0.1 | `docs/authority/architecture/P14-system-architecture.md` | cutover |
| AXL-V1-P15 | 15 — P15 Module Design — AxLicense V1 v0.1 | `docs/authority/architecture/P15-module-design.md` | cutover |
| AXL-V1-P16 | 16 — P16 Runtime Data Flow — AxLicense V1 v0.1 | `docs/authority/architecture/P16-runtime-data-flow.md` | pending |
| AXL-V1-P16.1 | 16.1 — Runtime Flow Atlas — 总览、状态面与读图约定 | `docs/authority/architecture/runtime-flow-atlas/16.1-overview.md` | pending |
| AXL-V1-P16.2 | 16.2 — Runtime Flow Atlas — 本地运行、DeviceIdentity 与设备注册 | `docs/authority/architecture/runtime-flow-atlas/16.2-local-identity-registration.md` | pending |
| AXL-V1-P16.3 | 16.3 — Runtime Flow Atlas — LicenseGrant、在线激活与离线激活 | `docs/authority/architecture/runtime-flow-atlas/16.3-activation.md` | pending |
| AXL-V1-P16.4 | 16.4 — Runtime Flow Atlas — 工厂 Provisioning 与 Legacy Migration | `docs/authority/architecture/runtime-flow-atlas/16.4-factory-legacy.md` | pending |
| AXL-V1-P16.5 | 16.5 — Runtime Flow Atlas — Entitlement 变更、Credential 演进与 Admin Lifecycle | `docs/authority/architecture/runtime-flow-atlas/16.5-entitlement-credential-admin.md` | pending |
| AXL-V1-P16.6 | 16.6 — Runtime Flow Atlas — Recovery、Identity Continuity 与 RMA/Rehost | `docs/authority/architecture/runtime-flow-atlas/16.6-recovery-rehost.md` | pending |
| AXL-V1-P16.7 | 16.7 — Runtime Flow Atlas — Product Claim、二维码找回、Ownership 与 Support Verification | `docs/authority/architecture/runtime-flow-atlas/16.7-product-claim-ownership.md` | pending |
| AXL-V1-P16.8 | 16.8 — Runtime Flow Atlas — Diagnostics、本地并发、Crash、State Corruption 与 Self-Upgrade | `docs/authority/architecture/runtime-flow-atlas/16.8-diagnostics-concurrency.md` | pending |
| AXL-V1-P16.9 | 16.9 — Runtime Flow Atlas — Server Timeout、Signing/DB Failure、Outage、DR 与 Assertion Replay | `docs/authority/architecture/runtime-flow-atlas/16.9-server-failure-dr.md` | pending |
| AXL-V1-P17 | 17 — P17 Platform Contract — AxLicense V1 v0.1 | `docs/authority/architecture/P17-platform-contract.md` | pending |
| AXL-V1-P18 | 18 — P18 Engineering & Optimization — AxLicense V1 v0.1 | `docs/authority/architecture/P18-engineering-optimization.md` | pending |
| AXL-V1-P20 | 20 — P20 Verification Design — AxLicense V1 v0.1 | `docs/authority/verification/P20-verification-design.md` | pending |
| AXL-V1-P30 | 30 — P30 Implementation Plan — AxLicense V1 v0.1 | `docs/authority/implementation/P30-implementation-plan.md` | pending |
| AXL-V1-A0-P31 | 31 — P31 AXL-V1-A0 Task Package — C++ Local Signed-Credential Gate | `.aegis/packages/AXL-V1-A0-P31.md` | repository-native |
| AXL-V1-A0-P34-B1 | 34 — P34 AXL-V1-A0 Gate Review — BLOCKED | `docs/authority/implementation/A0/P34-gate-review-blocked.md` | pending |
| AXL-V1-A0-P35 | 35 — P35 AXL-V1-A0 Defect Classification — QCBOR Contract Divergence | `docs/authority/implementation/A0/P35-defect-classification.md` | pending |
| AXL-V1-A0-P36 | 36 — P36 AXL-V1-A0 Fix / Reverification — QCBOR Boundary Repair | `docs/authority/implementation/A0/P36-fix-reverification.md` | pending |
| AXL-V1-A0-P34 | 34R — P34 AXL-V1-A0 Gate Rereview — PASS_WITH_FINDINGS | `docs/authority/implementation/A0/P34-gate-rereview.md` | pending |
| AXL-V1-A0-INTEGRATION | A0 — Repository Integration Closure — AXL-V1-A0 | `docs/authority/implementation/A0/repository-integration-closure.md` | pending |
| AXL-V1-A1-P31 | 31 — P31 AXL-V1-A1 Task Package — C++ Windows Identity + Protected Local State | `.aegis/packages/AXL-V1-A1-P31.md` | repository-native |
| AXL-V1-A1-P33 | 33 — P33 AXL-V1-A1 Final Reconciliation — READY_FOR_CONTROL_REVIEW | `docs/authority/implementation/A1/P33-final-reconciliation.md` | pending |
| AXL-V1-A1-P34 | 34 — P34 AXL-V1-A1 Gate Review — PASS_WITH_FINDINGS | `docs/authority/implementation/A1/P34-gate-review.md` | pending |
| AXL-V1-A1-INTEGRATION | AXL-V1-A1 Repository Integration Closure | `docs/authority/implementation/A1/repository-integration-closure.md` | pending |
| AXL-GOV-90 | 90 — AxLicense Governance & Decision Log | `docs/authority/governance/decision-log.md` | pending |
| AXL-ROOT | AxLicense — Product & System Authority | `docs/authority/index.md` | pending |

The Notion root records A1 P32 as `BLOCKED_ENVIRONMENT / VALID PARTIAL EXECUTION PRESERVED`, but no standalone P32 page was discovered in the M0 scoped search. Its lifecycle fact is therefore tracked through the root Authority until a dedicated source is identified or explicitly confirmed absent.

## Source URLs

Canonical Notion page URLs are recorded in `.aegis/authorities.json` and `.aegis/evidence.json`. The P16 Flow Atlas source URLs are retained here during migration:

- 16.1 `https://app.notion.com/p/3d94c57a590c8173a2e5cf1a1f42361f`
- 16.2 `https://app.notion.com/p/3d94c57a590c8119b738f37371cb6954`
- 16.3 `https://app.notion.com/p/3d94c57a590c81299efed11b69a0a34f`
- 16.4 `https://app.notion.com/p/3d94c57a590c81f081a5c6e6588ec940`
- 16.5 `https://app.notion.com/p/3d94c57a590c810f8bd7f506ede88807`
- 16.6 `https://app.notion.com/p/3d94c57a590c81ee98aad09283f51b54`
- 16.7 `https://app.notion.com/p/3d94c57a590c81ff861dfba2e23da138`
- 16.8 `https://app.notion.com/p/3d94c57a590c8196abe8ceeadc88f0fa`
- 16.9 `https://app.notion.com/p/3d94c57a590c8170ac0df40488f3580e`

## Repository reality at M0

The canonical baseline contains a Windows C++20 client, A0 signed-credential verification/runtime entitlement logic, A1 Windows DeviceIdentity and protected machine state, unit/integration/reference tooling, Windows CI, a physical TPM workflow, and the two durable P31 packages. A0 was integrated at `c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`. A1 was integrated at `eb1ab6ecb6afc31a6bc178a671d4f433414ad8c5` from exact Gate-reviewed result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`.

## M1 — P00 cutover

P00 was migrated as a location-only Authority move. The repository copy preserves the source page's problem statement, constraints, scenarios, seed scope, supporting concerns, success criteria, non-goals, policy disposition table, and P00 disposition. No product-semantic change is introduced by the migration.

Current Authority target after merge: `docs/authority/discovery/P00-problem-discovery.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c81f8b4a3ee31b093afd0`.

## M2 — P01 cutover

P01 was migrated as a location-only Authority move. The repository copy preserves the source page's four reviewed alternatives, five research findings, build-vs-buy challenge, P01 conclusion, validated licensing pattern, excluded commercial-platform scope, and research-status follow-up evidence topics. No product-semantic or research-conclusion change is introduced by the migration.

Current Authority target after merge: `docs/authority/discovery/P01-product-research.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c812a9442c5571a5e776f`.

## M3 — P02 cutover

P02 was migrated as a location-only Authority move. The repository copy preserves the accepted requirement baseline, FR-001 through FR-019, all NFRs and acceptance criteria, the Windows-first Device Identity Assurance Policy, requirement-freeze disposition, the 2026-09-12 targeted reconciliations, and the explicit internal supersession of NearHub V1 legacy-specific mandatory journeys by FR-019. No requirement, supersession decision, or product semantic is changed by the migration.

Current Authority target after merge: `docs/authority/discovery/P02-product-requirements.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c81c79a58e2c7098c8648`.

## M4 — P03 cutover

P03 was migrated as a location-only Authority move. The repository copy preserves the initial requirement-to-capability matrix, consumer mapping, accepted product-policy trace, downstream traceability policy, the FR-016/FR-017 assertion and enrollment-recovery addendum, the FR-018 legacy-provisional traceability addendum, the FR-019 unified first-run reconciliation, their intermediate downstream routing dispositions, and the final FR-019 supersession/routing result. No capability mapping, supersession decision, or traceability semantic is changed by the migration.

Current Authority target after merge: `docs/authority/discovery/P03-capability-traceability.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c81f48247e0af39e23326`.

## M5 — P10 cutover

P10 was migrated as a location-only Authority move. The repository copy preserves the durable product-world taxonomy, value/session/derived-state classification, aggregate and identity invariants, the Device Assertion/Product Device Association targeted reconciliation, the full historical FR-018 object model and supersession disposition, and the current FR-019 unified first-run/cross-product entitlement reconciliation. No object classification, authority boundary, supersession decision, entitlement constraint boundary, or downstream handoff semantic is changed by the migration.

Current Authority target after merge: `docs/authority/modeling/P10-product-object-model.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c8182b7e4d48fdaccfc7e`.

## M6 — P11 cutover

P11 was migrated as a location-only Authority move. The repository copy preserves the cross-flow interaction invariants, generic Start/Validate/Commit/Deliver/Complete model, Online/Offline Activation, Rehost/RMA, Factory Identity Provisioning / optional Pre-Activation, Recovery, runtime/refresh/admin behavior, failure classification, Claim/Recovery/Transfer targeted reconciliation, the full historical superseded FR-018 behavior model, the current FR-019 unified first-run/backend-licensing behavior reconciliation, and the Current Authority Catalog Evolution addendum for dynamic entitlement registration and credential refresh. No interaction commit boundary, cancel/retry rule, supersession decision, partial-success semantics, catalog-evolution behavior, or downstream P12 handoff is changed by the migration.

Current Authority target after merge: `docs/authority/modeling/P11-interaction-behavior.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c818ca6d7e7d5811831fb`.

## M7 — P12 cutover

P12 was migrated as a location-only Authority move. The repository copy preserves the complete semantic-schema authority, including canonical truth boundaries, identity/version/compatibility rules, normative reconciliation amendments, Device Assertion schema reconciliation, superseded FR-018 historical schema, and current FR-019 dynamic-entitlement/revision/credential-refresh authority. No schema meaning, supersession decision, validation invariant, or downstream routing semantic is changed by the migration.

Current Authority target after merge: `docs/authority/modeling/P12-semantic-schema.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d74c57a590c813c93b8cf63f128cf85`.

## M8 — P13 cutover

P13 was migrated as a location-only Authority move. The repository copy preserves the operation/mutation contract, atomicity/idempotency/replay/error semantics, Device Assertion operations, superseded FR-018 workflow/migration history, current FR-019 catalog/grant/credential reconciliation, and the ordinary first-run `RegisterDeviceIdentity` targeted repair. No operation contract, supersession decision, commit boundary, or lifecycle routing semantic is changed by the migration.

Current Authority target after merge: `docs/authority/modeling/P13-operation-mutation-model.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d94c57a590c81108345fac48ed6d2b8`.

## M9 — P14 cutover

P14 was migrated as a location-only Authority move. The repository copy preserves the base system architecture, modular-monolith/canonical-ACID/signing boundaries, CLI-only deployment reconciliation, local single-writer constraints, Diagnostics & Safe Logging authority, Device Assertion architecture, the superseded FR-018 migration architecture and ownership correction, and the final FR-019 unified enrollment/dynamic-entitlement/credential-observation architecture. No ownership assignment, trust/failure boundary, supersession decision, deployment invariant, or P15 handoff semantic is changed by the migration.

Current Authority target after merge: `docs/authority/architecture/P14-system-architecture.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d94c57a590c81909351e96d9c3469e0`.

## M10 — P15 cutover

P15 was migrated as a location-only Authority move. The repository copy preserves the base module map and stable ports, CLI-only public boundary, local single-writer/diagnostics/Device Assertion module invariants, the superseded FR-018 migration workflow module reconciliation, and the final FR-019 Current Authority with `DeviceRegistrationClient`, `CredentialSync`, Product Backend enrollment/commercial modules, Product/Entitlement Catalog Registry, Grant Authority, read-only `CredentialResolver`, mutation-only `CredentialIssuance`, and compatibility-only legacy migration modules. No module ownership, port contract, dependency direction, command-family classification, supersession decision, or P16 handoff semantic is changed by the migration.

Current Authority target after merge: `docs/authority/architecture/P15-module-design.md`.

Historical source retained as evidence: `https://app.notion.com/p/3d94c57a590c81359649d808edd4869e`.

## M11 entry condition

M11 starts with P16. Migration of a document means: fetch complete Notion source, reproduce it faithfully in Markdown, verify structure/decisions, update Authority/evidence manifests, recompute state, review the diff, then change the ledger status to `cutover`. P16 also has the 16.1–16.9 Runtime Flow Atlas inventory, which remains pending until each recorded source is migrated according to the migration ledger. Authority migration remains independent of the implementation lifecycle; the active development stage remains whatever `.aegis/state.json` derives from the authored manifests and repository evidence.
