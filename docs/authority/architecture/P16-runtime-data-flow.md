---
authority_id: AXL-V1-P16
stage: P16
scope: axlicense
kind: flow
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c815dbed1fe9093e7dd08
migration_class: location-only
semantic_change: none
---

# 16 — P16 Runtime Data Flow — AxLicense V1 v0.1

> 🔀 **Authority status: Accepted / P16 FR-019 targeted Runtime Data Flow reconciliation CLOSED — 2026-09-13.** 本页 Current Authority 以 trusted P10–P15 FR-019（含 P14 D-078 / P15 D-079）为基础，冻结 ordinary first-run Device registration、Product Backend enrollment/Organization claim、managed-but-unlicensed commercial convergence、dynamic catalog rollout、Grant revision + successor credential、ordinary credential resolve/install、online/offline activation、factory、recovery/rehost 的 happy / reject / retry / cancel / response-loss / crash / recovery 时间顺序。FR-018 migration/provisional application flows 与旧 §39 candidate product-flow review 仅保留为 historical discovery / compatibility context，不再阻塞 FR-019 P16 closure。P16 不新增产品 capability；未被 P02/P03 接受的候选流程仍不得被实现层自行当作 V1 requirement。

## 1. Stage contract

- **Role:** P16 Runtime Data Flow.
- **Authority:** Accepted/reconciled P02/P03 FR-019 requirement/capability baseline；trusted P10–P13 semantics；P14 FR-019 System Architecture D-078；P15 FR-019 Module Design D-079。FR-018-specific migration/provisional material is historical/compatibility-only unless a future authority explicitly reopens it.
- **Objective:** 在稳定 module/state ownership 上逐条追踪 AxLicense V1 的运行时间序列，覆盖正常、失败、重试、取消、并发、崩溃、本地损坏、升级与灾备，并暴露尚未被当前产品 Authority 覆盖的流程缺口供产品评审。
- **Non-goals:** 不在本页新增商业 capability；不冻结 Windows API、文件路径、Named Mutex、JSON/HTTP wire、数据库产品、crypto algorithm 或部署 sizing；不把候选产品流程自动升级为 V1 requirement。
- **Required Analysis:** happy/error/retry/recovery/cancel/backpressure/persistence；每个 state transition 的 owner；server canonical commit 与 local/product observation 分离；failure 后的 authoritative state 判定。
- **Required Output:** state-plane model、flow inventory、temporal traces、failure/recovery matrix、candidate product-flow review list。
- **Quality / Evidence Gate:** 任意失败点都能回答“哪个 state 已 commit、谁拥有、retry 做什么、会不会重复授权/绑定/归属”；不能产生第二套 authority；assertion/QR 不得变成 ownership authority。
- **Handoff:** P16 FR-019 targeted reconciliation CLOSED 后，earliest untrusted downstream layer = **P17 Platform Contract**。P17 可在不改变 P10–P16 semantics/module/runtime ordering 的前提下冻结 Windows-first CLI/process contract、secure-storage/provider realization、transport/wire/platform capability boundary；若 P17 发现需要改变 accepted product/semantic behavior，则必须按 Earliest Untrusted Layer 回流。

## 2. Runtime state planes

P16 必须始终区分五个 state plane；其中 Product-local workflow plane 可以 durable，但明确不是 license/product-ownership authority：

| State plane | Owner | Examples | Authority meaning |
|---|---|---|---|
| Device local | `IdentityManager` / provider / `LocalStateRepository` / `CredentialManager` | provider ref, local identity association, installed credential, trust metadata | Local realization / observation；不是 server canonical truth |
| AxLicense canonical | `OperationCore` • `CanonicalUnitOfWork` • domain modules | Device/DeviceIdentity, LicenseGrant, DeviceBinding, SignedLicenseCredential record, lifecycle event, operation result | License/identity canonical authority |
| Product-local workflow | Product/Launcher | LegacyMigrationFlow, LogicalApplicationId, pending migration application, local outbox/retry metadata, contact draft | Durable workflow/support state only；丢失或存在都不能创建、撤销或证明 License authority |
| Product management | Launcher/NearHub product backend | ProductDeviceAssociation, Organization/Tenant, product management credential, enrollment/recovery session | 产品归属/IAM authority；不属于 AxLicense canonical model |
| Transient transport/session | 各 flow coordinator | CLI process, HTTP retry, offline request/response, QR handle, DeviceAssertionChallenge, DeviceIdentityAssertion | Evidence/transport only；存在本身不授权 |

### 2.1 Commit boundaries

必须区分三个 durable commit：

1. **AxLicense canonical commit** — server license/device state 成立；response 是否送达无关。
2. **Device local commit** — candidate credential/local association 完整验证后 crash-safe replace 成立。
3. **Product association commit** — consuming product backend 完成 Device→Organization/Account association transition。

它们不能被 UI/session 的“完成”状态替代。

此外 FR-018 存在一个 **non-authoritative durable persistence point**：Product/Launcher 的 `MigrationApplicationStore` 可以把 pending application/outbox 跨重启保存。它只保证客户申请可靠投递；不是第四个 authorization commit，也不能改变 Grant/Binding/Credential 或 ProductDeviceAssociation。Server 一旦接受 application，server-side `LegacyMigrationApplication` workflow truth 独立于 Launcher 本地 outbox 存活。

## 3. User-visible device states

以下是产品/UI 可以遇到的组合状态，不是新的 P12 canonical enum：

| View state | AxLicense identity | License | Product association | Typical entry |
|---|---|---|---|---|
| S0 Fresh | Absent | Absent | Absent | Golden Image materialized / fresh install |
| S1 Registered / unlicensed / unclaimed | Established + registered | None | None | Factory identity-only provisioning |
| S2 Registered / licensed / unclaimed | Established | Valid credential | None | Factory pre-activated inventory |
| S3 Registered / unlicensed / claimed | Established | None | Organization/Account present | Management enrollment before commercial activation |
| S4 Registered / licensed / claimed | Established | Valid credential | Organization/Account present | Normal deployed device |
| S5 Legacy bootstrapped / migration pending | Established | No AxLicense credential | Existing product relation may exist | Legacy rollout transition |
| S6 Provider unavailable | Canonical identity exists | Credential may exist | Unchanged | TPM/KSP temporarily unavailable |
| S7 Credential invalid/lost | Identity intact | Local credential unusable | Unchanged | Disk corruption/delete |
| S8 Identity continuity recovery | Old identity unavailable | Binding exists | Unchanged | Secure-store loss with continuity evidence |
| S9 Replacement device | New Device identity | Requires Rehost | Product association may require separate transfer/update | RMA / motherboard replacement |

## 4. Current-authority flow inventory

| ID | Flow | AxLicense canonical mutation | Product association mutation | Channel |
|---|---|---|---|---|
| RUN-01 | Local status / entitlement evaluation | No | No | Offline/local |
| ID-01 | Provider discovery / first local identity establishment | No until registration | No | Local |
| ID-02 | Device identity registration/provisioning | Device + DeviceIdentity | No | Online V1 |
| LIC-01 | LicenseGrant issuance | LicenseGrant | No | Admin/server |
| ACT-01 | Online activation | Binding + credential (+ Device if needed) | No | Online |
| ACT-02 | Offline activation | Same as ACT-01 at connected side | No | Air-gap |
| FAC-01 | Factory identity-only provisioning | Device + DeviceIdentity | No | Online factory |
| FAC-02 | Factory optional pre-activation | Grant/Binding/Credential as authorized | No | Online factory |
| MIG-01 | Legacy identity bootstrap | May register Device/Identity; no entitlement | No implicit change | Local + server registration |
| MIG-02 | Online legacy migration | Grant/Binding/Credential + claim consumption | No | Online |
| MIG-03 | Offline legacy migration | Same as MIG-02 at connected side | No | Air-gap |
| ADM-01 | Revise entitlement/validity | Grant revision + optional successor credential | No | Admin/server |
| ADM-02 | Credential refresh / key-format rotation | Successor credential | No | Online/control |
| ADM-03 | Deactivate/release binding | Binding closed | No | Admin/support |
| ADM-04 | Suspend / resume grant | Grant state + optional refresh | No | Admin |
| ADM-05 | Revoke grant | Grant/binding revoked | No | Admin/security |
| REC-01 | App/OS reinstall with identity continuity | Usually no binding change | No | Local + optional recovery |
| REC-02 | Credential corruption/loss | Optional successor credential | No | Online/offline recovery |
| REC-03 | Identity material loss, continuity proven | Identity epoch successor + credential recovery | No | Support-controlled |
| REC-04 | Identity provider temporarily unavailable | No | No | Local retry/recovery |
| RMA-01 | Physical replacement / Rehost | Old binding closed + new binding/credential | Separate product policy | Support/admin |
| OWN-01 | First product enrollment / claim | No | Create association | Online web/QR capable |
| OWN-02 | Forgot account / organization access recovery | No | No association change | QR + web |
| OWN-03 | Product management credential recovery | No | Reissue management credential only | Online |
| OWN-04 | Ownership / organization transfer | No implicit license transfer | A→B association transition | Privileged product flow |
| OWN-05 | Support device verification | No | Product-specific | Purpose-bound assertion |
| OPS-01 | Minimum-disclosure diagnostics | No | No | Local |
| OPS-02 | Support Bundle | No | No | Explicit local export |

## 5. RUN-01 — Local status / entitlement evaluation

### Temporal flow

1. Product spawns `axlic.exe` read-only command → `CliFrontend` validates contract version/input.
2. `CommandApplication` routes to `RuntimeLicense`; **no local mutation lock** is required.
3. `CredentialManager` loads installed candidate credential and trusted credential key metadata.
4. `IdentityManager` loads the already-established current identity context; it does **not** rerun first-establishment provider selection.
5. `RuntimeLicense` verifies credential signature/format/key, device/binding reference, entitlement validity, and current identity association.
6. Returns derived `LicenseStatus` / entitlement result through stable `CommandResult`.

### Failure/recovery

- No credential → unlicensed/not-activated result, not a crash.
- Malformed/tampered/unknown-key credential → fail closed for affected entitlement; existing bytes are not rewritten by a read-only query.
- Established provider temporarily unavailable → distinguish `IDENTITY_PROVIDER_UNAVAILABLE` from identity mismatch; do not silently establish software identity.
- Server/network unavailable → irrelevant to perpetual local runtime path.
- Product may cache derived result in memory, but cache never becomes authority.

## 6. ID-01 / ID-02 — First identity establishment and registration

### ID-01 Local establishment

1. Product/factory invokes an identity establishment-capable command.
2. `CommandApplication` acquires `LocalMutationCoordinator` because provider/local association may change.
3. `IdentityManager` checks for existing established association first.
4. If none: `IdentityProviderRegistry` probes candidates; `IdentityPolicy` selects strongest provider satisfying minimum assurance: usable TPM hardware-bound → software-persistent → unsupported.
5. Chosen provider creates provider-local private identity material and returns only opaque provider ref + public/normalized claim.
6. Candidate local association remains **pre-registration local evidence**, not canonical Device identity.

### ID-02 Server registration

1. Device/factory sends `ProvisionDeviceIdentity`/equivalent allowed registration request with stable correlation.
2. Server `OperationCore` validates caller/provisioning authority, identity uniqueness and assurance policy.
3. `CanonicalUnitOfWork` commits Device + current DeviceIdentity + lifecycle event + operation result atomically.
4. Response delivers server `device_id`/registration result.
5. Device updates protected local identity association under single-writer, crash-safe replace.

### Failure/retry

- Provider creation fails before canonical commit → no Device record; retry may reuse/repair provider-local candidate according to P17 realization, but must not create multiple canonical Devices.
- Server rejects uniqueness/policy → no canonical mutation; local pre-registration key is not authorization authority.
- Server commit succeeds but response is lost → retry same correlation recovers the same Device outcome; must not create another Device.
- Local update crashes after server commit → server Device remains canonical; next run reconciles/recovers registration association rather than re-registering as a new physical Device.

## 7. LIC-01 — LicenseGrant issuance

1. Admin/control plane authenticates actor and submits `IssueLicenseGrant` with stable correlation.
2. `CallerAuth` resolves authority; `OperationCore` validates Licensee, entitlement definitions, validity and binding policy.
3. `CanonicalUnitOfWork` commits new LicenseGrant revision 1 + lifecycle event + operation result.
4. No DeviceBinding or local credential is created merely because the grant exists.
5. Retry same operation/payload returns same grant; payload conflict returns idempotency conflict.

## 8. ACT-01 — Online activation

### Happy path

1. Product starts activation; `axlic.exe` loads/establishes current Device identity context.
2. Device sends activation request with stable correlation and device proof/claim through `ServerClient`.
3. Server authenticates caller/activation authority and `OperationCore` validates grant state, binding policy, Device identity and one-active-binding invariant.
4. `LicenseDomain` + `DeviceRegistry` prepare candidate state; `CredentialIssuance` builds protected credential payload; signer signs before final canonical commit is declared successful.
5. `CanonicalUnitOfWork` commits binding + signed credential record + grant revision/capacity/event + operation result as one logical outcome.
6. Server returns committed credential.
7. Device acquires local mutation lock; `CredentialManager` verifies candidate completely against trusted key/current identity before replacing local credential.
8. Crash-safe local replace commits; only then can CLI report fully observed device success.

### Failure/retry/cancel

- Reject before server commit → no canonical mutation.
- Server signer failure before commit → no successful credential-required canonical mutation.
- Server commit succeeds, network response lost → canonical activation remains; same correlation retry returns committed outcome.
- Credential received but fails local verification → local old valid credential remains untouched; server canonical activation may already exist and must be recovered, not duplicated.
- Device crashes before local replace → server outcome remains; retry recovers same credential.
- Cancel before server commit → no mutation; cancel after commit cannot roll back and becomes recovery/delivery problem.
- Grant already bound to same Device → recover current result; do not create second binding.
- Grant bound to another Device → explicit Rehost required.

## 9. ACT-02 — Offline activation

1. Offline device acquires/loads Device identity and creates `OfflineActivationRequest` with stable correlation + anti-replay/device proof binding.
2. Exporting/copying request causes **no canonical mutation**.
3. Operator transfers request via USB/file/other transport to connected environment.
4. Connected side validates request and invokes canonical `CompleteOfflineActivation` through the same server operation semantics as online activation.
5. Server commits unique binding + credential + event/result once.
6. Signed response/credential can be copied back to offline device.
7. Device imports under local mutation lock; `CredentialManager` verify-before-replace then crash-safe install.

### Replay/failure

- Duplicate exact request → same committed outcome.
- Same correlation with modified canonical payload → idempotency/replay rejection.
- Response lost → re-export committed result, never create a second binding.
- Repeated import of same credential → idempotent success/no-op.
- Not importing a server-committed response does **not** mean server activation was cancelled.

## 10. FAC-01 / FAC-02 — Factory flows

### FAC-01 Identity-only provisioning

1. Golden Image is materialized on target physical device with no current identity/private device key/binding/credential.
2. Factory station obtains scoped `identity_provision` authority.
3. Per-device local identity establishment runs after clone boundary.
4. Server validates authorization scope, product/batch/capacity and identity uniqueness.
5. Per-device canonical commit creates/confirms Device + DeviceIdentity only and consumes only identity-provision capacity if applicable.
6. Local registration association is committed; device may remain S1 Registered/Unlicensed/Unclaimed indefinitely.

### FAC-02 Optional pre-activation

1. Only when batch/product policy requests ship-licensed and authorization has separate `factory_pre_activation` scope, station starts pre-activation.
2. Server validates existing Device/current identity and grant/entitlement/capacity scope.
3. Commit creates/confirms grant as allowed + unique binding + signed credential + lifecycle event/capacity consumption.
4. Device verifies and installs credential locally; product association may still remain unclaimed.

### Batch interruption

- Each physical Device commits independently; no whole-batch rollback.
- Identity success + pre-activation failure leaves a legitimate identity-only Device.
- Retry cannot duplicate Device registration, binding, credential, or capacity consumption.

## 11. MIG-01 / MIG-02 / MIG-03 — Legacy migration

### MIG-01 Identity bootstrap

1. Existing field product upgrades to AxLicense-capable software.
2. It may establish/register DeviceIdentity without obtaining new AxLicense entitlement.
3. Device can remain S5 while product follows an explicit rollout policy; this is **not** equivalent to having an AxLicense license.

### MIG-02 Online migration

1. Product/device presents trusted migration authority/historical claim evidence plus current Device proof.
2. Server validates claim has not been consumed by another Device, entitlement scope, Device uniqueness and binding policy.
3. Canonical commit creates/confirms grant + binding + credential + migration claim consumption + lifecycle event.
4. Device verifies/installs credential locally.

### MIG-03 Offline migration

- Device exports migration request; connected side executes the exact same `MigrateLegacyDevice` canonical mutation; signed result is imported locally.

### Failure/retry

- Old image/MAC/copied files/new keypair alone never authorize historical entitlement.
- Commit-before-delivery response loss recovers same grant/binding/credential.
- Same historical claim on different Device → conflict/fail closed.
- Migration cancel before commit does not consume claim/capacity; after commit requires explicit successor lifecycle action.

## 12. ADM-01 — Grant revision / entitlement change

1. Admin authenticates and submits `ReviseLicenseGrant` with `expected_authority_revision`.
2. `OperationCore` rejects stale revision/concurrent conflict rather than silently last-write-wins.
3. `LicenseDomain` validates replacement entitlement set and independent runtime/maintenance/cloud validity semantics.
4. Canonical commit increments `authority_revision`, records lifecycle event, and—if an active binding needs an updated offline snapshot—includes or coordinates a successor credential as one recoverable logical outcome.
5. Device does not observe the new rights until it obtains and installs a compatible successor credential or reaches another defined observable lifecycle point.

### Failure/retry

- Concurrent admin edit → explicit revision conflict; caller refreshes authority before retrying as a new logical intent.
- Response lost after commit → same correlation returns committed revision/result.
- Device offline → old installed credential continues to express its signed snapshot; server revision alone cannot rewrite offline bytes.

## 13. ADM-02 — Credential refresh / signing-key or format migration

1. Authorized refresh is started for an existing grant/binding.
2. Server validates current binding, requested/required credential generation and trusted signing profile.
3. `CredentialIssuance` constructs successor payload; signing succeeds before operation is committed as successful.
4. Canonical result records successor `credential_generation` and supersession linkage; grant authority revision changes only when authorization semantics change.
5. Device receives successor and `CredentialManager` verify-before-replace installs it atomically.

### Failure/retry

- Signer unavailable → do not declare successful refresh or mutate into an unrecoverable credential-required terminal state.
- New credential verification fails locally → keep old still-valid credential; surface error for recovery.
- Repeated refresh retry recovers the same logical outcome; it must not create unbounded credential churn.

## 14. ADM-03 / ADM-04 / ADM-05 — Administrative lifecycle

### ADM-03 Deactivate / release binding

1. Authorized admin/support targets current Active binding.
2. Server validates it is still authoritative.
3. Commit closes binding + lifecycle event; old credential bytes remain unchanged.
4. LicenseGrant may later be activated/rehosted according to policy; reuse creates successor binding semantics rather than resurrecting the closed binding identity.

### ADM-04 Suspend / resume grant

- Suspend commit changes server grant authority and blocks new authorized issuance according to policy; already-offline credential cannot be assumed instantly dead.
- Resume commit returns grant to active; if a bound device needs updated signed snapshot, successor credential refresh follows explicit operation semantics.

### ADM-05 Revoke grant

- Revoke is durable withdrawal; grant/binding enter revoked state as defined and no further valid credential is issued.
- Permanently offline old credential remains subject to the accepted offline-observability limitation.

### Cancel/retry

- Before commit → no durable change.
- After commit → cannot use session cancel to reverse history; use explicit successor operation where one exists.
- Same correlation retry is idempotent.

## 15. REC-01 — Application / OS reinstall with identity continuity

### Case A: app reinstall, local identity/provider material preserved

1. New app invokes `axlic.exe`.
2. `IdentityManager` loads the established provider-local reference/current identity.
3. If local credential also exists and verifies, normal RUN-01 resumes with no server mutation.
4. If credential was removed, transition to REC-02.

### Case B: OS reinstall/reset under a supported persistence model

1. Identity/provider continuity must be established first; presence of copied application files is not evidence.
2. If current DeviceIdentity can still be proven and server binding remains to the same `device_id`, recovery can restore existing authoritative credential or issue same-binding successor.
3. No new LicenseGrant and no new binding are created merely because the OS/app changed.

### Forbidden shortcut

A reinstall must never execute “no local files → create new Device → consume another license” if trusted continuity can still be established.

## 16. REC-02 — Local credential corruption / loss

1. Product calls runtime query; `CredentialManager` cannot load/verify credential.
2. Runtime fails closed for affected entitlement; corrupted fields are never partially trusted.
3. `IdentityManager` proves current Device identity remains intact.
4. Recovery request identifies current `device_id`/binding through trusted server association, not through untrusted damaged credential bytes.
5. Server returns current authoritative credential or signs same-binding successor under `RecoverDevice(mode=credential_only)` policy.
6. Device verify-before-replace installs recovered credential atomically.

### Offline variant

If an offline recovery channel is later/explicitly supported for this operation, the authority semantics remain identical: target Device proof + current binding authority → signed result → local verify/install. P16 does not invent a new offline recovery capability beyond current Authority.

## 17. REC-03 — Identity material loss with physical continuity proven

1. Existing provider/local key cannot be loaded; system distinguishes **lost/unusable identity material** from ordinary credential loss.
2. No automatic fallback identity is created and no old credential is trusted merely because it exists.
3. Support/recovery flow gathers accepted continuity evidence for the same physical Device.
4. Server validates continuity against current canonical Device/current binding and authorizes `RecoverDevice(mode=identity_continuity)`.
5. Device establishes successor local identity through policy-approved provider path.
6. Canonical commit retires/advances identity epoch for the same `device_id`, preserves the LicenseGrant/binding semantics, issues/recover successor credential as required, and appends lifecycle evidence.
7. Local association + credential are installed under single-writer/crash-safe rules.

### Failure boundary

If continuity cannot be proven, REC-03 stops; it **must not** silently become “accept a new key as same Device.” Route to RMA-01/rehost or explicit reprovisioning policy.

## 18. REC-04 — Established identity provider temporarily unavailable

1. `IdentityManager` finds existing association pinned to provider/scheme.
2. Loading/proving through that provider returns temporary unavailable/driver/TPM access failure.
3. Runtime/identity-sensitive command returns stable provider-unavailable/retry/recovery classification.
4. No Device, identity epoch, binding or provider selection changes occur.
5. Retry after provider recovers reuses the original identity.

### Security invariant

An established hardware-bound identity **never** triggers first-establishment fallback to software-persistent because of a transient TPM problem.

## 19. RMA-01 — Physical replacement / Rehost

### Happy path

1. Replacement physical Device establishes/registers its own independent DeviceIdentity; it is not absorbed into the old Device.
2. Internal admin/support starts `RehostDevice` against current grant/binding and target Device.
3. Server validates rehost authority, current old binding, target identity and one-active-binding invariant.
4. Single canonical transition closes old binding, creates new binding, signs successor credential for replacement Device, preserves `license_grant_id`, and appends event/result.
5. New Device retrieves/verifies/installs credential.

### Old device semantics

Old permanently offline device may continue using its old signed bytes until an observable lifecycle point; rehost cannot truthfully promise instantaneous remote kill.

### Product association interaction

AxLicense rehost does **not** automatically decide Launcher Organization ownership. If the product wants the replacement appliance to inherit the old room/organization association, that is a separate product-managed flow/policy and is intentionally flagged for product review below.

## 20. OWN-01 — First product enrollment / claim

### Precondition

AxLicense Device is already registered. Product-side state may be `unclaimed`; the device may be licensed or unlicensed independently.

### Temporal flow

1. Launcher starts product enrollment.
2. Product backend creates `EnrollmentSession` and asks AxLicense `ChallengeAuthority` for a purpose-bound challenge (`device.enrollment` / product-defined stable purpose).
3. Launcher receives trusted challenge and invokes `axlic.exe device-assert`.
4. `DeviceAssertion` validates challenge issuer/audience/purpose/expiry; `IdentityManager` loads current established identity; provider produces restricted proof.
5. Product backend submits assertion to `DeviceAssertionVerifier`; verifier checks challenge, registered current Device identity, proof, assurance and replay policy.
6. Session is marked **device-verified**, but no ownership exists yet.
7. Human authenticates to Account/Organization/SSO and product backend verifies organization authority.
8. Only after both device proof + human/org authority succeed does product backend commit `ProductDeviceAssociation` (enterprise default Device→Organization/Tenant; user is actor/admin).
9. AxLicense Device/License state is unchanged by association commit.

### Cancel/retry

- Before association commit → session expiry/cancel leaves Device registered and unclaimed.
- Device assertion retry/revalidation does not create ownership.
- Product association commit must itself be idempotent; duplicate completion must not create competing active associations.

## 21. OWN-02 — Forgot account / organization access recovery via QR

### Primary scenario

Existing `ProductDeviceAssociation` is valid; user forgot username/password, lost SSO access, or does not know which organization manages the kiosk.

### Temporal flow

1. Launcher user selects “recover account/device management access”.
2. Product backend creates an `AssociationRecoverySession` and an AxLicense challenge bound to that product audience + `association.recovery` purpose + session + nonce + expiry.
3. Launcher calls `device-assert`; backend verifies assertion and marks the recovery session device-verified.
4. Backend returns only an opaque, high-entropy, short-lived recovery handle/URL for QR display. QR contains no raw DeviceIdentity/assertion/account/password/license secret.
5. Human scans QR in a browser.
6. Product account system independently authenticates/authorizes the human—email/phone/SSO/org admin/support policy as product chooses.
7. Only after human authorization may backend reveal appropriately masked/authorized account/org information and enter password/SSO recovery.
8. Existing Device→Organization association remains unchanged during ordinary account recovery.

### Replay/failure

- Screenshot/stolen QR alone is insufficient; used/expired handle is rejected.
- Expired/replayed/retargeted device assertion is rejected.
- Browser response loss/retry does not create new product ownership or any AxLicense mutation.
- A logged-in but different organization must not use this flow to take ownership; that is OWN-04.

## 22. OWN-03 — Product management credential/session recovery

Use when server association is intact but Launcher lost its product-local management token/session.

1. Backend starts a credential-recovery session.
2. Device proves current DeviceIdentity through purpose-bound assertion.
3. Product backend additionally applies account/organization authorization policy where required.
4. Backend reissues only the product management credential/session.
5. `ProductDeviceAssociation` is unchanged.
6. AxLicense Device/License state is unchanged.

This flow is explicitly distinct from AxLicense REC-02/REC-03, which recover license/identity state.

## 23. OWN-04 — Ownership / organization transfer

1. Device is currently associated with Organization A; target is B.
2. Product backend starts a dedicated transfer session—not ordinary recovery.
3. Device presence may be proven by assertion purpose `ownership.transfer`.
4. Backend requires independent transfer authority: current A admin approval or explicit support/admin override, plus authorization to bind to target B.
5. Physical access/QR scan alone is insufficient.
6. Product backend commits one auditable association transition `A → B`.
7. AxLicense license state does not automatically transfer commercial ownership. If a LicenseGrant's commercial Licensee must also change, that is a **separate license-lifecycle policy/capability not currently implied by ProductDeviceAssociation transfer**.

### Cancel/retry

Before product commit, A remains owner. Retry must recover same transfer outcome; it must not produce A+B simultaneous authoritative ownership.

## 24. OWN-05 — Support device verification

Current Authority allows Device Assertion for support/high-risk workflows.

1. Support system creates challenge bound to explicit support purpose/audience/session.
2. Device produces assertion without exposing private key.
3. Backend verifier confirms registered Device/current identity/assurance.
4. Verification is evidence that support is interacting with that Device; it **does not by itself authorize rehost, account reset, ownership transfer or license mutation**.
5. Any privileged successor operation still requires its own support/admin authority.

## 25. OPS-01 — Minimum-disclosure diagnostics

1. Product/support invokes `diagnostics`/safe status.
2. Modules emit only registered event IDs and approved safe fields through `Diagnostics`.
3. Result contains stable code/category/retry/safe action/correlation and limited capability summaries.
4. It never outputs raw private identity material, authorization secrets, raw server response, stack traces or generic provider handles.
5. Reading diagnostics does not mutate canonical or product state.

## 26. OPS-02 — Support Bundle

1. User/support explicitly requests bundle generation.
2. `SupportBundle` collects approved bounded logs/snapshots only through safe diagnostic policies.
3. Sanitization occurs before package export.
4. Production package should be encrypted to support recipient according to later P17 platform/crypto contract.
5. Even encrypted bundles must never contain never-log secrets/private key material.
6. Bundle generation/export is non-authoritative and may fail/delete without affecting license or ownership truth.

## 27. X-01 — Multi-process concurrency on device

Windows V1 没有常驻 Agent，因此多个产品/进程可能同时 spawn `axlic.exe`。

### Read-only concurrency

- `status` / `entitlement` / safe diagnostics may run concurrently against a consistent committed local snapshot.
- Read-only commands must not create identity/provider material simply because state is missing.

### Mutating concurrency

1. Each mutating command classifies itself before touching local state.
2. `CommandApplication` acquires machine-wide `LocalMutationCoordinator` ownership.
3. After lock acquisition it **re-reads current committed local state**; it must not rely on stale pre-lock snapshot.
4. It performs server/provider work and builds a validated candidate local state.
5. Candidate is committed generation-aware/crash-safe; lock is then released.

### Conflict behavior

- Second mutating process waits/returns stable busy/retry classification according to P17 policy; it must not bypass serialization.
- Long network waits should not unnecessarily hold the local writer if the design can split prepare vs commit safely; exact lock scope is P17/P18, but revalidation is mandatory before local commit.
- A read started during mutation must see either old committed snapshot or new committed snapshot, never intentionally parse half-written state.

## 28. X-02 — Device process crash / power loss matrix

| Crash point | Authoritative result after restart | Required recovery |
|---|---|---|
| Before provider/local mutation | Old state | Retry normally |
| Provider key created, server registration not committed | No canonical Device yet; orphan/pre-registration local evidence may exist | Detect/reuse or safely retire candidate; do not mint duplicate identities blindly |
| Server Device commit done, local `device_id` association not saved | Server Device is canonical | Retry same correlation / prove same key and recover registration result |
| Server activation committed, credential not received | Binding + credential canonical | Retry same operation/delivery |
| Credential received, before local replace | Old local credential | Verify/reinstall committed credential |
| During local replace | Old or new committed generation only | Repository recovery chooses last complete valid generation |
| After local replace, before CLI response | New local state committed | Retry/read current state; do not repeat server mutation |

P17 must choose Windows primitives that can realize these outcomes; P16 freezes the temporal semantics.

## 29. X-03 — Local-state corruption / partial-loss matrix

| Observed local condition | Classification | Allowed action | Forbidden shortcut |
|---|---|---|---|
| Credential missing/corrupt; identity intact | REC-02 | Recover current/successor credential | Infer entitlement from damaged metadata |
| Local identity association metadata corrupt; provider key exists | Association reconstruction/recovery candidate | Use trusted provider evidence + server canonical mapping under recovery policy | Create new Device solely because metadata file is gone |
| Association says provider/key exists; provider key missing | REC-03 candidate | Continuity evidence → successor epoch; otherwise rehost | Silent software fallback |
| Provider key and association intact; `device_id` mapping missing locally | Registration observation loss | Prove current key / recover server Device mapping | Register as another physical Device without reconciliation |
| Credential valid but bound to different Device/current identity | Identity/binding mismatch | Fail closed + investigate/recover/rehost | Rewrite credential/device reference locally |
| Trust-key metadata corrupt/outdated | Verifier trust-state failure | Recover trusted key-set through signed/authorized update path | Accept unknown signer to keep product running |
| All AxLicense local state lost | Unknown until provider/continuity evidence checked | Attempt identity continuity discovery first, then controlled recovery/rehost | Assume this is a brand-new unlicensed Device immediately |

## 30. X-04 — AxLicense executable upgrade / rollback / schema migration

### Upgrade happy path

1. Installer replaces program binaries independently from protected device identity/credential state.
2. New `axlic.exe` reads local-state schema/version before mutation.
3. If old schema is supported directly, run normally.
4. If migration is required, acquire local mutation lock, retain recoverable old generation, transform into candidate new generation, validate, then atomically commit.
5. DeviceIdentity provider ref/private material is not regenerated merely because executable version changed.
6. Existing valid credential remains valid if its credential schema/key are still supported.

### Compatibility rules

- New client + old credential/local state: supported within declared backward-compatibility window or fail with explicit upgrade/migration requirement.
- Old client + new credential: server must not assume support; credential/schema/key rollout requires compatibility policy before issuance.
- Server upgrade cannot require all perpetual-offline clients to synchronously upgrade.

### Migration failure

- Before new local-state commit: old committed state remains recoverable.
- After commit: migration is considered locally observed; retry reads current version.
- Failed binary upgrade must not delete provider keys/identity/credential state as cleanup.

### Downgrade

Exact downgrade policy is P17/P20. P16 freezes: an older executable must never silently reinterpret unknown authorization-critical schema or destroy newer state; incompatible downgrade fails closed/preserves data.

## 31. X-05 — Server request timeout / duplicate delivery / backpressure

1. Caller assigns logical `correlation_id` once per operation intent.
2. `ServerClient` may retry transport with the same logical operation identity.
3. Server operation ledger distinguishes not-seen / in-progress / committed / deterministically rejected states.
4. Duplicate delivery with identical canonical payload converges to same result.
5. Same identity + changed canonical payload → explicit idempotency conflict.
6. Rate limiting/load shedding before commit returns retryable/non-retryable stable outcome without partial canonical mutation.
7. If caller times out while server is still executing, caller must not create a new correlation simply to “try again” unless intentionally starting a different operation after resolving current status.

## 32. X-06 — Server transaction / signing failure

### Credential-producing operations

1. Operation validates and constructs candidate canonical state.
2. Credential payload is derived deterministically from candidate authority.
3. Isolated signer is invoked.
4. Signer failure/timeout means operation cannot enter a successful canonical terminal state requiring that credential.
5. After signature is available, server revalidates any conflict-sensitive revision/state if necessary.
6. `CanonicalUnitOfWork` commits canonical mutation + signed credential record + event + operation result atomically.

### Database commit failure

- No success may be returned unless committed outcome is recoverable from canonical store/operation ledger.
- A signature generated for an uncommitted candidate does not independently authorize Device/Grant; orphan signature material is non-authoritative and may be discarded/garbage-collected.

### Response failure after commit

- Canonical outcome remains valid and recoverable by same correlation.

## 33. X-07 — AxLicense Server outage

### Existing perpetual runtime

- RUN-01 continues locally with installed valid credential/current identity context.

### New control-plane operations

- Activation, registration, migration, recovery, rehost, refresh and Device Assertion challenge/verification that require server become unavailable/retryable according to each contract.
- No local module may invent canonical server approval during outage.

### Offline activation/migration

- Offline target device can prepare/export request where local prerequisites permit; canonical completion still requires an available connected AxLicense Server.

### Product account recovery

- Current QR recovery design requires product backend + AxLicense challenge/verifier reachability at session creation/verification time. **Fully disconnected kiosk recovery is not currently guaranteed** and is listed as a product-review candidate below.

## 34. X-08 — Canonical database backup / disaster recovery

P16 does not choose backup technology, but freezes consistency obligations.

### Backup set

A recoverable authority backup must preserve a mutually consistent point for at least:

- Device / DeviceIdentity;
- LicenseGrant / DeviceBinding;
- Signed credential records/metadata required for result recovery;
- Provisioning / migration authorization and consumed claims/capacity evidence;
- lifecycle events required by authority/audit policy;
- operation/idempotency ledger needed to prevent duplicate replay after restore;
- trust/key metadata required to interpret existing credentials.

### Restore invariant

After disaster recovery, the restored authority must not silently resurrect an older state as if later committed activation/rehost/revoke/migration had never occurred when externally issued credentials or irreversible claim consumption prove otherwise.

### Consequence

A simple stale DB snapshot restore can create authority regression. P17/P20 must define/verify a strategy (backup cadence + durable operation/audit evidence + restore reconciliation/fencing) that prevents the restored system from issuing contradictory successor authority.

### Signing authority

DB restore and signing-key/KMS recovery are separate failure domains; restoring DB without required verification/signing key availability may permit local old credentials to continue but must not cause the server to issue untrusted replacements.

## 35. X-09 — Credential/signing-key rotation rollout

1. Server introduces new signing key/profile and publishes/ships corresponding trusted public verification metadata through an authenticated compatibility path.
2. Clients must know the verification key/profile **before** receiving credentials that require it, unless credential format contains a separately trusted compatible chain mechanism explicitly designed later.
3. Server may continue issuing old-compatible credentials during migration window according to policy.
4. `RefreshCredential` moves bound devices to successor credential where reachable/required.
5. Offline devices with old still-trusted credential continue under the declared compatibility/retirement policy; retiring a key cannot magically update permanently offline clients.
6. Compromised signing-key emergency handling is high-impact security policy and requires P17/P20 detail; P16 does not invent instant revocation for offline devices.

## 36. X-10 — Challenge / Device Assertion replay and failure

1. Product backend obtains challenge from `ChallengeAuthority` for exact audience/purpose/session/device context.
2. Device validates challenge trust/time/context before provider proof.
3. Assertion verifier checks current registered identity/epoch, proof, challenge digest, audience/purpose/session and replay/expiry state.
4. Successful verification returns transient derived evidence only.

### Failure cases

- Expired challenge/assertion → reject; no mutation.
- Used single-use recovery challenge → reject/recover product session according to policy, never rebind ownership.
- Assertion generated for another audience/purpose/session → reject.
- Old/retired identity epoch → reject or route to identity recovery; do not silently map to current epoch.
- Current provider unavailable → device-presence proof fails closed/retry; no fallback signing identity.
- Challenge Authority unavailable → new online proof session cannot start; existing license runtime unaffected.

## 37. X-11 — Product backend failure around association commit

### First claim / transfer

- Device verification and human/org authentication can complete transiently before durable association commit.
- If product backend crashes before association commit, ownership remains previous state (`unclaimed` or Organization A).
- If commit succeeds but web/device response is lost, retry must recover committed association rather than create a duplicate/competing association.
- Product session status is not authority; durable association store is.

### Cross-system atomicity

AxLicense assertion verification and ProductDeviceAssociation commit are **not one distributed transaction** and do not need to be: assertion is bounded evidence consumed by product policy. Product backend must verify evidence is still valid/not replayed when it commits its own association.

## 38. Flow-coupling rules

1. **Registration does not imply license activation.** S1 is legal.
2. **License activation does not imply product ownership claim.** S2 is legal.
3. **Product claim does not imply paid license.** S3 is legal if product policy permits management of an unlicensed device.
4. **Account recovery does not imply ownership transfer.** OWN-02 cannot become OWN-04 silently.
5. **AxLicense rehost does not imply product Device→Organization transfer.** They are separately governed facts.
6. **Product ownership transfer does not imply LicenseGrant commercial Licensee transfer.** Current V1 has no automatic coupling.
7. **Device Assertion proves current device possession only within bounded context.** It is never human/org authority.
8. **Offline license runtime does not imply offline management/account recovery.** These are independent availability properties.

## 39. Product-flow completeness review — candidate gaps, not yet Current Authority

The following scenarios emerged from the full runtime traversal. They are **not automatically added to V1 requirements by P16**. Product owner should explicitly ACCEPT / DEFER / NON-GOAL each item; accepted new capabilities route upstream before P16 closure.

| Candidate | Why it matters | Current coverage | P16 recommendation |
|---|---|---|---|
| C-01 Device decommission / secure disposal | Meeting-room appliance is retired, sold, recycled or wiped | `ADM-03` deactivate/release binding, `ADM-05` revoke when commercially/security required, separate ProductDeviceAssociation lifecycle, and product/local reset already provide the required primitives | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 adds no `DecommissionDevice` canonical operation. Decommission is orchestration of existing lifecycle actions, not a new AxLicense authority transition. Ordinary decommission may clear local credential/product/customer secrets but does not by itself destroy stable DeviceIdentity or delete AxLicense server history. Commercial Licensee transfer remains C-03; refurbishment/return-to-unclaimed remains C-12. |
| C-02 RMA replacement inherits product organization/room configuration | License rehost moves rights, but customer expects replacement unit to appear as same managed room/device | `RMA-01` already covers AxLicense license continuity; ProductDeviceAssociation / Room / configuration remain product-owned state | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 adds no new AxLicense RMA lifecycle beyond existing Rehost. Replacement hardware establishes its own Device/DeviceIdentity; AxLicense moves the LicenseGrant binding to that Device. Organization, Room, Device name, configuration and management credentials remain consuming-product authority. A product may provide one-click RMA orchestration across Rehost + product association/room/config migration, but these commits are not required to be a distributed atomic transaction; product-side partial failure retries/reconciles without rolling back an already committed AxLicense Rehost. |
| C-03 Commercial Licensee transfer when Product ownership A→B | Used device resale / customer tenant change may require license commercial owner transfer | OWN-04 transfers product association only; LicenseGrant Licensee stays unchanged | **HUMAN DECISION — REJECT / NON-GOAL (V1) / 2026-09-12.** Product ownership / Organization transfer does not transfer an existing `LicenseGrant` commercial Licensee. V1 adds no `TransferLicensee` operation and does not rewrite a historical grant in place. If business/support explicitly approves the new customer to receive rights, the old grant/binding is closed or revoked as appropriate and a new LicenseGrant is issued/activated for the new Licensee. A future transferable-license product may model successor-grant transfer separately, but that is outside V1. |
| C-04 Lost/stolen/compromised physical device | Customer wants management access removed and future control blocked | Grant revoke/device compromised state concepts exist, but product-side incident flow is not required for the current NearHub V1 product profile | **HUMAN DECISION — REJECT / NON-GOAL (V1) / 2026-09-12.** Current NearHub deployment is a large-screen / meeting-room appliance whose physical theft risk is not material enough to justify a dedicated AxLicense incident-response capability in V1. Existing revoke/rehost/admin primitives remain available for exceptional support cases, but V1 adds no `MarkDeviceCompromised` product flow, remote-kill promise, or new upstream requirement. Existing schema/model `DeviceState.compromised` remains reserved semantic capacity and does not by itself make the scenario a required V1 flow. |
| C-05 Organization deletion / merge / administrator offboarding | Enterprise tenant lifecycle can orphan managed devices | Organization/Tenant ownership, administrator identity and `ProductDeviceAssociation` are consuming-product authority; existing OWN-04 association transfer and C-12 return-to-unclaimed cover device disposition | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 adds no AxLicense Organization/Admin lifecycle operations. Administrator offboarding changes product IAM/roles/sessions only. Organization merge/deletion must be resolved in the product backend by transferring or unclaiming affected ProductDeviceAssociations before destructive tenant deletion. AxLicense `Device` / DeviceIdentity remain unchanged, and `LicenseGrant` is not implicitly revoked or commercially transferred by Organization lifecycle. This is a consuming-product data-integrity/IAM policy, not AxLicense canonical lifecycle. |
| C-06 Fully offline kiosk enrollment/account recovery | Kiosk may have no backend connectivity while phone has Internet, or entire site is air-gapped | Offline License Activation is already supported; Product Enrollment / Account Recovery currently assumes Product Backend challenge/verifier and human/organization authority reachability | **HUMAN DECISION — REJECT / NON-GOAL (V1) / 2026-09-12.** AxLicense V1 requires offline license activation, but does not require fully offline Product Enrollment, Organization claim or account recovery. A NearHub may remain licensed + unclaimed/unassociated and continue local operation until Product Backend connectivity is available. V1 adds no offline Device Assertion relay, QR enrollment exchange, air-gapped account recovery or static QR ownership token. If a future product repeatedly needs device-offline / phone-online enrollment, define a separate disconnected enrollment protocol rather than weakening Device Identity Assertion or human/organization authorization semantics. |
| C-07 Customer self-service rehost/device replacement | Could reduce support cost | Existing RehostDevice semantics are complete, but V1 authority is explicitly limited to internal admin/support rather than customer self-service | **HUMAN DECISION — REJECT / NON-GOAL (V1) / 2026-09-12.** V1 keeps rehost/device replacement under internal admin/support authority. No customer-facing portal or direct self-service rehost authorization is added. Customer self-service would require a new authorization layer linking authenticated customer/Organization authority to commercial LicenseGrant rehost permission, plus abuse controls, audit and dispute handling. Future self-service may reuse the existing RehostDevice mutation behind a product/backend mediated authorization flow, but it is not a V1 AxLicense capability. |
| C-08 Trial/evaluation/time-limited offline product | Sales/demo use cases may need trusted-clock semantics | Current schema already supports bounded entitlement / time-limited credential semantics, while rollback-resistant trusted-time enforcement for fully offline trial remains intentionally unresolved | **HUMAN DECISION — DEFER / 2026-09-12.** V1 may issue ordinary trial/evaluation rights using bounded validity and may evaluate them online or with best-effort local time, but V1 does not guarantee rollback-resistant fully offline trial enforcement against a user who controls the device clock/storage. Secure monotonic time, TPM/TEE-backed counters, trusted RTC/time sources and reinstall/clone-resistant trial continuity are deferred until a concrete business requirement justifies the additional security scope. This does not block bounded entitlement support or perpetual offline runtime licensing. |
| C-09 Subscription/renewal grace UX | Cloud/update rights and bounded rights may expire while runtime stays perpetual | Runtime / maintenance_update / cloud_service right separation, bounded validity and grant revision already support expiry and approved renewal without changing perpetual runtime semantics | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** AxLicense V1 owns right validity and applies an approved renewal by revising the existing LicenseGrant / issuing successor credential as needed. Renewal reminders, payment state, auto-renew, dunning, subscription lifecycle and the business decision to grant grace remain product/commercial-backend responsibilities. Grace does not require a new canonical subscription state: if commercially approved it is reflected as the effective validity window for the relevant maintenance/cloud right. V1 adds no billing/checkout/subscription state machine. |
| C-10 Bulk organization enrollment / fleet claim | Enterprise deployment may claim dozens/hundreds of devices without per-device human scan | Per-device OWN-01 enrollment, purpose-bound Device Identity Assertion and Organization authorization already define the secure primitive; what is not defined is a trusted batch-membership / fleet-claim authority that can replace per-device human interaction | **HUMAN DECISION — DEFER / 2026-09-12.** V1 keeps per-device product enrollment/claim. Bulk Organization Enrollment / Fleet Claim is deferred until enterprise deployment volume makes it a concrete requirement. Any future batch workflow must preserve independent per-device identity proof and define a scoped Product Backend authority for batch membership, target Organization, validity/capacity, partial success, retry/replay and audit. Serial lists, batch IDs or procurement metadata must not by themselves become ownership authority. The orchestration belongs to the consuming product / management backend rather than AxLicense LicenseGrant authority. |
| C-11 Dealer/reseller/install-partner enrollment authority | Channel partner may deploy before end customer admin is present | Current V1 can let an installer deploy, configure, register and activate a device while leaving product ownership unclaimed; reseller/channel hierarchy and delegated partner-to-customer Organization claim authority are not part of accepted V1 authority | **HUMAN DECISION — REJECT / NON-GOAL (V1) / 2026-09-12.** Partners may perform physical deployment and permitted activation/configuration, but V1 does not create Dealer/Reseller/Installer principals, partner delegation, channel hierarchy or authority to commit `ProductDeviceAssociation` on behalf of a customer Organization. The supported handoff is installer-deployed + licensed + unclaimed, followed by claim from an authorized customer Organization actor. Any future delegated partner enrollment must be introduced as a separate product-management/IAM capability and must not imply authority to transfer/rehost/revoke a commercial `LicenseGrant` or change its Licensee. |
| C-12 Factory/device wipe or “return to unclaimed” | Demo/returned stock/refurbishment may need product association cleared without changing physical Device identity | Existing identity-continuity rules preserve the same physical `Device`; `ProductDeviceAssociation` is product-owned and can transition back to unclaimed through a privileged product workflow; license lifecycle remains separate | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 adds no new AxLicense refurbishment/unclaim operation. Local factory reset may clear product/customer configuration and local management credentials, but it does not itself delete the server-side ProductDeviceAssociation, create a new AxLicense Device, revoke a LicenseGrant, or transfer commercial Licensee. A privileged consuming-product backend may transition the association to unclaimed for demo/return/refurbishment while preserving physical Device identity. License deactivate/revoke/reissue is a separate commercial/admin decision using existing primitives. |
| C-13 AxLicense server migration between environments/regions | Production infrastructure replacement can affect canonical authority/key custody | P14 single-logical-authority architecture plus P16 X-08 DR invariants already require preservation of canonical Device/Grant/Binding/Credential state, operation/idempotency evidence, trust metadata and separate signing-authority recovery | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 treats planned region/cloud/account/database/KMS infrastructure replacement as relocation of the same logical AxLicense authority, not creation of a new authority. Migration must preserve stable canonical identities and credential validity, fence the old writer before the new writer becomes authoritative, and avoid stale-state resurrection or split-brain issuance. Concrete backup/replication, KMS/HSM migration, service discovery, cutover, RPO/RTO and rollback runbooks belong to P17/P18/P20. Active-active multi-region writes, partition-autonomous issuance, customer-hosted sovereign AxLicense or independent regional signing authorities are not implied and would require a new explicit requirement. |
| C-14 Customer-visible license activation-code recovery/reissue | Customer can lose purchased activation material before binding | P12/P13 already separate activation material from canonical LicenseGrant authority; activation code/key is a validation token/authority reference, while LicenseGrant remains the purchased commercial right | **HUMAN DECISION — ALREADY COVERED / 2026-09-12.** V1 does not add a new canonical activation-material recovery mutation. If an unbound customer loses activation material, authorized support/admin may re-deliver or regenerate activation material after verifying the commercial record/Licensee; this must not create a second LicenseGrant, change entitlements/Licensee, or bypass one-active-binding/device validation. After a grant is already bound, loss/corruption of the installed credential uses the existing credential/device recovery lifecycle instead of activation-code recovery. Customer self-service portal UX is not a V1 AxLicense core requirement. |

## 40. Recommended product-review order

To decide whether P16 is complete, review candidates in this order because they most affect V1 end-to-end appliance lifecycle:

1. **C-01 Decommission / secure disposal**
2. **C-02 RMA product-association/config migration**
3. **C-03 Commercial license ownership transfer policy**
4. **C-04 Lost/stolen/compromised device**
5. **C-06 Fully offline enrollment/recovery**
6. **C-12 Return-to-unclaimed / refurbishment**
7. **C-14 Activation-material recovery/reissue**

Then decide C-05/C-08/C-09/C-10/C-11 based on enterprise/go-to-market scope.

## 41. P16 current disposition

**Status: DRAFT / READY_FOR_PRODUCT_FLOW_REVIEW.**

- Current-authority flows have been traced through normal, failure, retry, cancel, response-loss and major crash/recovery boundaries.
- Cross-plane ownership is explicit: AxLicense canonical state, device local state, product association state and transient evidence are not conflated.
- No candidate in §39 is Current Authority merely because it appears here.
- **Earliest untrusted decision is now product-flow completeness**, not implementation detail.
- P17 must not start until either: (a) product owner confirms current flow set is sufficient and P16 is accepted, or (b) selected gaps are reconciled through their earliest upstream stage and P16 is regenerated.

### Runtime Flow Atlas child pages

- [16.1 — Runtime Flow Atlas — 总览、状态面与读图约定](https://app.notion.com/p/3d94c57a590c8173a2e5cf1a1f42361f)
- [16.2 — Runtime Flow Atlas — 本地运行、DeviceIdentity 与设备注册](https://app.notion.com/p/3d94c57a590c8119b738f37371cb6954)
- [16.3 — Runtime Flow Atlas — LicenseGrant、在线激活与离线激活](https://app.notion.com/p/3d94c57a590c81299efed11b69a0a34f)
- [16.4 — Runtime Flow Atlas — 工厂 Provisioning 与 Legacy Migration](https://app.notion.com/p/3d94c57a590c81f081a5c6e6588ec940)
- [16.5 — Runtime Flow Atlas — Entitlement 变更、Credential 演进与 Admin Lifecycle](https://app.notion.com/p/3d94c57a590c810f8bd7f506ede88807)
- [16.6 — Runtime Flow Atlas — Recovery、Identity Continuity 与 RMA/Rehost](https://app.notion.com/p/3d94c57a590c81ee98aad09283f51b54)
- [16.7 — Runtime Flow Atlas — Product Claim、二维码找回、Ownership 与 Support Verification](https://app.notion.com/p/3d94c57a590c81ff861dfba2e23da138)
- [16.8 — Runtime Flow Atlas — Diagnostics、本地并发、Crash、State Corruption 与 Self-Upgrade](https://app.notion.com/p/3d94c57a590c8196abe8ceeadc88f0fa)
- [16.9 — Runtime Flow Atlas — Server Timeout、Signing/DB Failure、Outage、DR 与 Assertion Replay](https://app.notion.com/p/3d94c57a590c8170ac0df40488f3580e)

## 40. Runtime Flow Atlas — 子文档索引

以下子文档把本页每个主要 runtime flow 转成 **Mermaid 流程图 + 中文逐步解释 + 设计意义 + 风险/控制**，并保留 P16 Flow ID 方便回溯：

- [16.1 — Runtime Flow Atlas — 总览、状态面与读图约定](https://app.notion.com/p/3d94c57a590c8173a2e5cf1a1f42361f) — 总览、四个 state plane、三个 durable commit、通用失败语义。
- [16.2 — Runtime Flow Atlas — 本地运行、DeviceIdentity 与设备注册](https://app.notion.com/p/3d94c57a590c8119b738f37371cb6954) — `RUN-01`、`ID-01`、`ID-02`：本地 entitlement、DeviceIdentity 建立、设备注册。
- [16.3 — Runtime Flow Atlas — LicenseGrant、在线激活与离线激活](https://app.notion.com/p/3d94c57a590c81299efed11b69a0a34f) — `LIC-01`、`ACT-01`、`ACT-02`：LicenseGrant、在线激活、离线激活。
- [16.4 — Runtime Flow Atlas — 工厂 Provisioning 与 Legacy Migration](https://app.notion.com/p/3d94c57a590c81f081a5c6e6588ec940) — `FAC-01/02`、`MIG-01/02/03`：工厂 Identity Provisioning、Pre-Activation、历史设备迁移。
- [16.5 — Runtime Flow Atlas — Entitlement 变更、Credential 演进与 Admin Lifecycle](https://app.notion.com/p/3d94c57a590c810f8bd7f506ede88807) — `ADM-01`～`ADM-05`：entitlement revision、credential refresh/key rotation、deactivate、suspend/resume、revoke。
- [16.6 — Runtime Flow Atlas — Recovery、Identity Continuity 与 RMA/Rehost](https://app.notion.com/p/3d94c57a590c81ee98aad09283f51b54) — `REC-01`～`REC-04`、`RMA-01`：重装、credential 丢失、identity material 丢失、provider unavailable、Rehost/RMA。
- [16.7 — Runtime Flow Atlas — Product Claim、二维码找回、Ownership 与 Support Verification](https://app.notion.com/p/3d94c57a590c81ff861dfba2e23da138) — `OWN-01`～`OWN-05`、`X-11`：首次 Claim、二维码账号找回、管理 credential 恢复、Organization transfer、Support verification、Product Backend commit failure。
- [16.8 — Runtime Flow Atlas — Diagnostics、本地并发、Crash、State Corruption 与 Self-Upgrade](https://app.notion.com/p/3d94c57a590c8196abe8ceeadc88f0fa) — `OPS-01/02`、`X-01`～`X-04`：Diagnostics/Support Bundle、多进程、断电、local-state corruption、self-upgrade/rollback。
- [16.9 — Runtime Flow Atlas — Server Timeout、Signing/DB Failure、Outage、DR 与 Assertion Replay](https://app.notion.com/p/3d94c57a590c8170ac0df40488f3580e) — `X-05`～`X-10`：server timeout/idempotency、signing/DB failure、server outage、backup/DR、key rotation rollout、Device Assertion replay。

### Coverage rule

本组 Atlas 只解释 **Current Authority 已覆盖的流程**。P16 §39 中的 candidate gaps（例如 device decommission、RMA 后 Product Association 迁移、商业 LicenseGrant ownership transfer、fully-disconnected account recovery 等）继续保持 **待产品评审**；在你明确接受前，不把它们画成已冻结 V1 行为。

## 41. FR-018 targeted Runtime Data Flow reconciliation — 2026-09-12

### 41.1 MIG-04 — Legacy provisional application submission / Launcher outbox

1. Designated legacy-upgrade Product/Launcher first obtains current minimum-disclosure AxLicense context/status from `axlic.exe`; no application is created merely by querying status.
2. If the device is eligible for the accepted legacy provisional policy and no valid AxLicense credential is installed, Launcher presents the `Unactivated/未激活` watermark and application UI.
3. When the user submits contact information, `LegacyMigrationFlow` creates or reuses one `LogicalApplicationId` and persists the request to `MigrationApplicationStore` **before** relying on network delivery.
4. If online, `MigrationWorkflowClient` sends the application **directly** to AxLicense Server `MigrationWorkflow`; it does not spawn `axlic.exe` for application sync.
5. If offline/unreachable, the request remains product-local pending/outbox. Product continues only under the accepted provisional-mode policy and keeps the watermark.
6. On network recovery/startup/manual retry, Launcher retries the same logical application. Server dedupes/reconciles by `LogicalApplicationId`; a server-accepted application becomes durable server workflow truth.
7. Once server acceptance is confirmed, the local outbox may mark the item delivered/reconciled; deleting that local queue item cannot delete the server application or any later authorization.

**Failure/recovery:**

- Launcher crashes before local persistence → no durable application exists; user may submit again.
- Local persistence succeeds but send never occurs → retry from the same outbox entry.
- Server accepts but response is lost → retry/query with the same logical identity returns/reconciles the existing application; never create a duplicate case.
- Product-local pending state is lost **before** server acceptance → no entitlement or approval exists; only the unsent customer request was lost.
- Product-local pending state is lost **after** server acceptance → server application remains durable; Launcher may reconcile by retained logical/application reference or user-visible workflow lookup permitted by the product contract.
- Repeated Submit clicks must converge to the same logical application while the current application lineage remains active.

### 41.2 Contact update and review race

1. If an application is still only local/pending, Launcher may update the contact draft while preserving the same `LogicalApplicationId`.
2. After server acceptance, Launcher calls the workflow update path directly with the expected/current contact revision.
3. Server appends/advances contact revision/history; stale retry cannot overwrite a newer contact revision.
4. `MigrationReview` consumes the current accepted application/contact revision.
5. Approve/Reject racing with contact update uses P13 revision/precondition semantics: a stale reviewer decision fails and must re-read rather than silently approving different contact data.

**Invariant:** email/contact is workflow information only. Updating it never proves Device ownership, Licensee identity, historical entitlement or Organization membership.

### 41.3 MIG-05 — Human review, approval/reject and notification

1. `MigrationReview` reads the durable `LegacyMigrationApplication` and may move it into `under_review` according to P13 workflow semantics.
2. Reviewer may inspect product/customer context and grouping signals. Fleet-like grouping is advisory only; V1 approval remains application/device scoped.
3. Reject commits only the workflow decision. It creates no `LegacyMigrationAuthorization`; the device remains provisional/unlicensed according to the accepted rollout policy.
4. Approve enters `MigrationApproval`, which must invoke the accepted P13 authority path so that `Application → approved` and the corresponding application/device/product-scoped `LegacyMigrationAuthorization` become one logical approval outcome.
5. Only **after** the approval commit may `NotificationDispatcher` send/re-send email or other notification containing an opaque/scoped redemption representation.
6. Notification delivery state is support state only. Email success/failure cannot create, revoke or duplicate migration authority.

**Failure/recovery:**

- Approval request fails before commit → no approval authority exists.
- Approval commits but portal response is lost → reviewer retry/reload recovers the same approved application/authorization; must not approve twice.
- Approval commits but email provider is down → authorization remains valid; retry notification only.
- Email is lost/deleted → resend or lookup the same existing approval/redemption context; do not create a new authorization.
- Reject after an already committed approval is not a valid way to revoke authority; use explicit `RevokeLegacyMigrationAuthorization` where allowed.

### 41.4 MIG-06 — Device-bound redemption and canonical migration

1. After approval, the user enters the received code/handle in Product/Launcher.
2. Launcher now invokes the `axlic.exe migrate`/equivalent migration-redemption capability. This is the point where execution re-enters the device security/license boundary.
3. `CommandApplication` acquires the local mutation discipline required for a credential-changing command and asks `IdentityManager` for the **current established** identity context; it never trusts the Launcher-stored device reference as identity authority.
4. `ServerClient` submits the redemption representation plus current trusted device context/correlation to AxLicense Server.
5. `RedemptionResolver` resolves the code/handle to the exact `LegacyMigrationAuthorization + application + device + product` context and validates scope **before** any migration capacity can be consumed.
6. Wrong-device, wrong-product, expired/revoked/invalid redemption fails closed and leaves the original authorization unconsumed.
7. Valid redemption enters `OperationCore → MigrateLegacyDevice`. `MigrationCommitKey(migration_auth_id, application_id, device_id, product_id)` plus normal operation idempotency ensures one logical migration outcome.
8. Canonical commit creates/confirms the standard `LicenseGrant + active DeviceBinding + SignedLicenseCredential`, consumes migration capacity, appends lifecycle evidence and marks the application completed as part of the accepted P13 outcome.
9. Server returns or re-exposes the committed credential/result. `CredentialManager` verifies signature/schema/device/binding/current-identity association **before** replacing local credential state.
10. Successful local crash-safe install creates the device-local observation of the server-authoritative license.
11. Launcher subsequently queries `axlic.exe status`; only a valid local licensed result may remove the provisional watermark.

**Failure/recovery:**

- Wrong device enters a valid code → `MIGRATION_DEVICE_SCOPE_MISMATCH`; no capacity consumption.
- Wrong product → fail before migration commit; no capacity consumption.
- Server commit succeeds but response is lost → retry with the same logical migration context finds the same committed outcome/credential; no second Grant/Binding/capacity consumption.
- Server commit succeeds, credential is delivered, but `axlic.exe` crashes before local install → canonical license already exists; next redemption/recovery retrieves the same committed result and resumes verify/install.
- Credential download is truncated/corrupt → verify fails; old local committed state is preserved and watermark remains.
- Credential verify succeeds but local replacement crashes → `LocalStateRepository`/`LocalMutationCoordinator` must leave old or new committed local generation, never intentional half-state; next status/recovery reconciles.
- Application is `approved` but migration authorization is revoked/expired before successful redemption → migration fails; approval/workflow status alone does not license the device.

### 41.5 Normal paid activation while provisional

A provisional legacy device that receives a normal valid AxLicense activation path does not need to wait for human-assisted migration. Successful normal activation installs a valid standard credential. Launcher then removes the watermark based on `axlic.exe status`. Any still-open migration application is workflow cleanup only and must not create a second Grant/Binding later.

### 41.6 End-state authority matrix

| Observed event/state | Licensed? | Watermark removal authority? | Retry meaning |
|---|---|---|---|
| Launcher has local pending application | No | No | Retry workflow submission |
| Server application received/under_review | No | No | Query/update same workflow |
| Application approved + MigrationAuthorization exists | No | No | Recover/resend same redemption context |
| Email delivered | No | No | Notification-only fact |
| `MigrateLegacyDevice` canonical commit succeeded | Server: Yes | Not yet by itself | Recover same committed credential/result |
| Credential verified + locally installed; `axlic.exe status = licensed` | Yes | **Yes** | Normal AxLicense runtime/recovery from then on |

### 41.7 FR-018 P16 disposition

**FR-018 targeted Runtime Data Flow reconciliation: ACCEPTED.** The runtime ordering preserves all P10–P15 invariants:

- Launcher owns application UX, product-local durable outbox and direct workflow sync;
- `axlic.exe` is not a background workflow synchronizer;
- server workflow/notification state never becomes license authority;
- human approval is distinct from migration commit;
- final redemption is device/product/application scoped before capacity consumption;
- commit-before-response-loss and local-install failure both converge on the same canonical credential;
- watermark removal is derived from verified local AxLicense state, not application/email status.

**P16 overall disposition remains DRAFT / PRODUCT-FLOW REVIEW REQUIRED.** FR-018 itself is reconciled through P16, but §39 candidate product-flow gaps remain a separate product review. Therefore **P17 remains paused at project level** until P16 overall is accepted; no new upstream requirement is introduced by this targeted reconciliation.

### Review-inventory correction — 2026-09-12

The §39 Human Product-Flow Review is a **candidate-gap review**, not a complete inventory of every V1 primary product journey. This distinction caused two important flows to be visually absent from the candidate-by-candidate review:

1. **Legacy provisional activation / watermark + email support flow (FR-018):** this is already accepted Current Authority and has already been traversed in §41 (`MIG-04` through `MIG-06`), so it was intentionally not reintroduced as a candidate gap. It must nevertheless appear in the final P16 primary-flow coverage checklist.
2. **Factory/new-device code-entry activation journey:** normal `ACT-01` / factory provisioning semantics contain the lower-level activation authority primitives, but the end-to-end product journey from activation material issuance/delivery → operator code entry → device identity binding → canonical activation → local credential install → licensed UI state has not been separately reviewed as a named primary flow. This is a P16 review-coverage gap and must be resolved before P16 final closure.

**Closure rule correction:** P16 may close only after both (a) all §39 candidate gaps have a Human disposition and (b) all V1 primary product journeys have an explicit end-to-end coverage check. P17 remains paused.

## 42. Primary Product Journey Audit — PF coverage correction / 2026-09-12

This section complements §39. §39 is a **candidate-gap review**; this section is the end-to-end **primary product journey audit** required before P16 may close.

### PF-01 — Legacy Sold-Device Provisional Migration

**Status: COVERED / VERIFIED against Current Authority.**

The designated sold-device upgrade journey is already fully traced by §41 `MIG-04 → MIG-05 → MIG-06`:

`legacy device without AxLicense credential → local DeviceIdentity establishment/registration → Unactivated watermark + contact/email application → durable Launcher outbox/retry → server human review/approval → application/device/product-scoped LegacyMigrationAuthorization → notification/redemption handle → Launcher invokes axlic.exe → current DeviceIdentity proof → MigrateLegacyDevice canonical commit → SignedLicenseCredential verify/install → only then watermark removal`.

Application submitted, review approved, notification/email delivered and code possession are not by themselves license authority. Only the committed migration outcome plus successful local credential verification/install causes the Launcher to observe `licensed`.

**Disposition:** no new requirement; PF-01 is already covered by FR-018 reconciliation.

### PF-02 — New NearHub first deployment: pairing / Organization claim / license activation

**Status: BLOCKED_UNRESOLVED_DECISION / P16 coverage finding.**

Existing authority contains the primitives (`ID-01/02`, `OWN-01`, `LIC-01`, `ACT-01/02`) but does not yet freeze one product-facing default journey that combines them. In particular, Current Authority does not define the proposed NearHub UX: `device establishes private identity locally → Launcher displays a short device pairing code → authenticated admin enters the code in management SaaS → ProductDeviceAssociation commits → the same orchestration causes the eligible LicenseGrant to bind to that Device → device retrieves/verifies/installs the credential`.

**Important semantic boundary:** a displayed static DeviceId/serial/unique identifier must not itself be possession proof, Organization ownership authority, or a License. If a human-friendly code is used, the recommended model is a short-lived, one-time, purpose-bound **PairingCode / EnrollmentSession handle** issued only after the Device has proven possession of its current DeviceIdentity to the trusted backend. The code is a locator/session handle; it does not contain or mint commercial rights.

**Recommended composition if accepted upstream:**

1. Golden-image materialization leaves the device identity-free.
2. `axlic.exe` establishes the DeviceIdentity private key locally after materialization and preserves it in the selected TPM/software-persistent provider boundary; private material never leaves the device.
3. Device identity is registered with AxLicense while remaining unlicensed/unclaimed (`S1`), either in factory identity-only provisioning or on first trusted online bootstrap.
4. Launcher starts a pairing session; the device proves possession of the current DeviceIdentity and receives a short-lived one-time PairingCode bound to Device + product + session + expiry.
5. Launcher displays PairingCode/QR. A stable DeviceId/serial may also be shown for support, but is not a secret or claim credential.
6. Organization admin authenticates in NearHub management SaaS and enters/scans PairingCode.
7. Product backend independently authorizes the admin for the target Organization, resolves the pairing session and commits `ProductDeviceAssociation`.
8. If commercial policy says this Device is entitled to a NearHub runtime license, the product/backend then invokes the existing AxLicense activation path using an already-valid commercial authority / LicenseGrant allocation. PairingCode itself must never create entitlement from nothing.
9. AxLicense performs the ordinary one-active-binding activation commit and issues the device-bound SignedLicenseCredential.
10. Device retrieves the committed result using its authenticated identity context, verifies signature/schema/device/binding/current identity and installs atomically.
11. Launcher reports setup complete only after it observes the required product association plus valid local `licensed` state.

**Partial-failure rule:** Product association and AxLicense license commit remain separate durable authorities; no distributed atomic transaction is introduced. Recommended ordering is association first, activation second. Crash/failure after association but before activation leaves a valid `claimed / unlicensed` state (`S3`) and must retry activation idempotently. Server activation commit with response/local-install loss is recovered through existing ACT-01 idempotency/credential recovery; code re-entry must not create a second binding or grant.

**Terminology correction:** this default connected journey is **not Offline Activation** merely because the final runtime credential can be used offline. It is `online pairing / online activation → offline-capable runtime`. True Offline Activation (`ACT-02`) is reserved for a target device that cannot reach AxLicense Server and therefore requires a two-way request/response artifact transfer.

**Routing finding:** making this the default NearHub V1 journey, and especially deciding that successful product pairing automatically consumes/allocates a commercial LicenseGrant, is a product requirement/behavior decision not yet frozen by P02/P03/P11. P16 must not silently promote it. If accepted, route to targeted upstream reconciliation before resuming P16.

### 42.1 Additional primary journeys that must be explicit before P16 closure

| ID | Primary journey | Current coverage | Audit status |
|---|---|---|---|
| PF-03 | Factory identity-only device leaves production unlicensed/unclaimed | FAC-01 + S1 | COVERED |
| PF-04 | Factory pre-activated device leaves production licensed/unclaimed, later joins Organization | FAC-02 + S2 + OWN-01 | COVERED; composition should be checked in final audit |
| PF-05 | True air-gapped new-device activation | ACT-02 | COVERED; requires two-way request/response transport |
| PF-06 | Already licensed but unclaimed device joins Organization | S2 + OWN-01 | COVERED |
| PF-07 | Claimed but unlicensed device receives license later | S3 + ACT-01/ACT-02 | COVERED primitives; explicit product orchestration should be checked |
| PF-08 | Legacy provisional device obtains a normal purchased activation instead of waiting for support migration | §41.5 + ACT-01 | COVERED |
| PF-09 | Same physical device reinstall / credential loss / provider outage / controlled identity recovery | REC-01..04 | COVERED |
| PF-10 | RMA physical replacement with license rehost and product association migration | RMA-01 + C-02 | COVERED |
| PF-11 | Return/refurbishment back to unclaimed state | C-12 | COVERED |
| PF-12 | Account/Organization access or local management-credential recovery | OWN-02/OWN-03 | COVERED |
| PF-13 | Device decommission / secure disposal | C-01 | COVERED |
| PF-14 | Lost activation material before binding / re-delivery or reissue | C-14 | COVERED |

### 42.2 Code/token unification rule

V1 should unify the **security meaning**, not force every flow into the same UX direction. Any human-entered or scanned code is an opaque representation of a narrowly scoped server/session authority and is never itself the License.

- **PairingCode** — device-originated, short-lived handle used by an authenticated product admin to locate/claim one currently participating Device; no commercial entitlement by itself.
- **LegacyMigrationRedemptionCode** — server/support-originated handle representing an already-approved migration authority and consumed on the device through the migration redemption path.
- **OfflineActivationRequest/Response** — two-way air-gap artifacts; not reducible to a one-way short code if the device truly cannot reach the server.

A common code-entry/QR component may parse/route these representations, but the scope, issuer, audience, expiry, replay policy and allowed mutation remain type/purpose-specific.

**Historical FR-018 review status — superseded for Current Authority by the FR-019 reconciliation below.** PF-01/PF-02 and the associated product-flow candidate review remain discovery context only; they are not silently accepted as FR-019 V1 requirements and no longer block the trusted FR-019 P16 path.

## P16 FR-019 targeted reconciliation — Unified First-Run / Commercial / Catalog / Credential Observation Runtime — 2026-09-13

### A. Scope and precedence

This section is the latest P16 Current Authority. It re-traces trusted P10–P15 FR-019 behavior through the D-079 module boundaries and **supersedes conflicting FR-018 runtime wording** for NearHub V1. Earlier runtime traces remain reusable only where they are consistent with this section. In particular:

- ordinary first-run `RegisterDeviceIdentity` is the normal connected identity-registration path; factory `ProvisionDeviceIdentity` is a separate factory-only path;
- Product Backend enrollment/Organization claim and `ProductDeviceAssociation` commit are independent from AxLicense license authority;
- `ResolveCurrentCredential` is read/delivery only; ordinary `refresh` does not create a credential generation;
- `ReissueCredential` is an explicit canonical mutation for authorized reissue reasons only;
- catalog registration changes registry truth only and never implicitly grants customer entitlement;
- FR-018 migration application/review/redemption flows and the old §39 candidate-flow review are historical discovery / compatibility context and do not block this FR-019 closure.

P16 defines temporal ordering, commit/observation boundaries, retry/recovery and failure containment. It does not redefine P13 payloads, P14 ownership or P15 module interfaces.

### B. Current runtime state planes and commit points

FR-019 runtime reasoning uses four durable/support worlds plus transient delivery state:

| Plane | Owner | Representative state | Authority meaning |
|---|---|---|---|
| Device protected local | `IdentityManager` / provider / `LocalStateRepository` / `CredentialManager` | provider ref, pinned identity association, optional `device_id` reference, installed credential, trust metadata | Local realization / observation. Local identity material may pre-exist server registration; installed signed credential is the local offline-verifiable authority snapshot. |
| AxLicense canonical | `OperationCore` • domain modules + `CanonicalUnitOfWork` | Catalog, Device/DeviceIdentity, LicenseGrant, DeviceBinding, SignedLicenseCredential record, lifecycle event, idempotency result | Canonical device/license authority. |
| Product SaaS | Product Backend modules | Organization/Tenant/IAM, EnrollmentSession, SetupCode, ProductDeviceAssociation, package/commercial workflow state | Product ownership/management/commercial truth. Not AxLicense license authority. |
| Transient / delivery | Flow coordinator | CLI process, HTTP request, retry timer, challenge/assertion, QR/file/USB transport, response delivery | Transport/evidence only. Loss must not be interpreted as canonical rollback. |

The following durable points are independent and must never be collapsed into one UI/session state:

1. **L0 — local identity establishment:** provider-private material and pinned local association are crash-safely established. This is local durable evidence, not yet canonical Device registration.
2. **A1 — AxLicense Device registration commit:** `Device + current DeviceIdentity + lifecycle event + operation result` become canonical.
3. **P1 — Product association commit:** Product Backend commits `ProductDeviceAssociation` after device verification and human/Organization authorization.
4. **A2 — AxLicense license authority commit:** Grant/Binding/Credential authority change is committed according to P13.
5. **L1 — local credential observation commit:** `CredentialManager` verifies and atomically installs the authoritative signed credential.

No distributed transaction is required across A1/P1/A2/L1. Retry/reconciliation moves the system forward from whichever commit point already succeeded.

### C. Derived composite states — not new canonical enums

The product may project these user-visible states from independent planes:

| Projection | Identity | Product association | AxLicense authority | Local observation |
|---|---|---|---|---|
| U0 Fresh | Absent | Absent | None | None |
| U1 Identified / unclaimed / unlicensed | Local established + server registered | Absent | No applicable Grant/Binding | No credential |
| U2 Managed / unlicensed | Registered | Committed | No applicable license authority | No credential |
| U3 Managed / server licensed / observation pending | Registered | Committed | Grant/Binding/Credential authority committed | Successor/current credential not yet installed |
| U4 Managed / licensed / locally observed | Registered | Committed | Committed | Valid current credential installed |

`unclaimed + licensed` can also exist after independent product-association changes or factory pre-activation. These projections must not be persisted as a new shared cross-system authority row.

### D. FR-019 Current Authority flow inventory

| ID | Flow | Canonical mutation | Primary modules |
|---|---|---|---|
| FR19-RUN-01 | Local status / entitlement evaluation | None | `RuntimeLicense` / `CredentialManager` / `IdentityManager` |
| FR19-ID-01 | First local identity establishment | None until registration | `IdentityManager` / provider / `LocalStateRepository` |
| FR19-ID-02 | Ordinary first-run Device registration | `RegisterDeviceIdentity` | `DeviceRegistrationClient` → `DeviceRegistry` / `OperationCore` |
| FR19-ENR-01 | Enrollment / Organization claim | No AxLicense license mutation; Product SaaS association commit | `EnrollmentSessionService` / `DeviceVerificationAdapter` / `ProductDeviceAssociationService` |
| FR19-COM-01 | Commercial decision → Grant / activation convergence | `IssueLicenseGrant`, `ReviseLicenseGrant`, `ActivateDevice` as applicable | `CommercialEntitlementPolicy` / `CommercialLicenseCoordinator` / AxLicense server |
| FR19-CAT-01 | Register/evolve Product or Entitlement definition | Catalog mutation + `CatalogRevision` | `ProductReleaseCatalogPublisher` → `ProductEntitlementCatalogRegistry` |
| FR19-CAT-02 | Commercial rollout of new entitlement to selected Grants | Independent `ReviseLicenseGrant` per Grant | Commercial coordinator → `GrantAuthority` |
| FR19-GRANT-01 | Already-bound Grant rights revision + successor credential | Grant revision + successor credential in one logical canonical outcome | `GrantAuthority` / `CredentialIssuance` / `CanonicalUnitOfWork` |
| FR19-REF-01 | Ordinary device refresh / reconnect | None | `CredentialSync` → `CredentialResolver` → `CredentialManager` |
| FR19-REISSUE-01 | Authorized key/format/recovery reissue | `ReissueCredential` | `OperationCore` / `CredentialIssuance` |
| FR19-ACT-01 | Online activation | Binding + credential canonical outcome | Commercial/device control path → `OperationCore` |
| FR19-ACT-02 | True offline activation | Same canonical activation outcome at connected side | `OfflineExchange` • Portal/control plane + server |
| FR19-FAC-01 | Factory identity-only provisioning | `ProvisionDeviceIdentity` | Factory tooling / `ProvisioningAuthority` / `DeviceRegistry` |
| FR19-FAC-02 | Factory optional pre-activation | Grant/Binding/Credential as scoped | Factory tooling / `ProvisioningAuthority` / license modules |
| FR19-REC-01 | Credential loss with identity continuity | Usually none; current credential redelivery | `CredentialSync` / `CredentialResolver` |
| FR19-REC-02 | Identity continuity recovery | `RecoverDevice` when authorized | Support/recovery path / `DeviceRegistry` / credential issuance |
| FR19-RH-01 | Physical replacement / Rehost | `RehostDevice` | Support/admin / Grant/Device/Credential modules |
| FR19-ASSERT-01 | Purpose-bound Device Assertion | No license mutation | `DeviceAssertion` / `ChallengeAuthority` / `DeviceAssertionVerifier` |

### E. FR19-ID-01/02 — fresh Windows first-run identity establishment and ordinary registration

**Happy path:**

1. Product invokes the AxLicense first-run/device-registration capability through `AxlicProcessAdapter → axlic.exe`.
2. `CommandApplication` enters a mutating local path and acquires `LocalMutationCoordinator` before changing identity/local association state.
3. `IdentityManager` first loads any existing pinned identity. If one exists, it is reused; provider discovery is not rerun.
4. If no identity exists, `IdentityProviderRegistry` probes providers and `IdentityPolicy` selects the strongest provider that satisfies policy. Windows V1 ordering remains usable hardware-backed TPM → software-persistent fallback → unsupported.
5. Selected provider creates private identity material inside the provider boundary and returns only provider reference/public normalized claim.
6. **Before network registration**, the provider reference + pinned local identity association are crash-safely persisted as L0. From this point, retry must reuse the same identity.
7. `DeviceRegistrationClient` obtains the read-only current identity context, requests restricted proof-of-possession, preserves a stable logical correlation and calls `RegisterDeviceIdentity` through `ServerClient`.
8. Server `CallerAuth/OperationCore` routes validation to `DeviceRegistry` + identity assurance policy. The `(scheme_id, identity_value)` uniqueness fence is checked in the canonical transaction domain.
9. `CanonicalUnitOfWork` commits `Device + current DeviceIdentity + lifecycle event + operation result` as A1. No Grant/Binding/Credential/Organization ownership is created.
10. Response returns opaque `device_id`; device local association stores it as a convenience/reference under local mutation discipline. The local `device_id` copy is not canonical authority.

**Failure / retry:**

- Provider creation fails before L0 → no established identity; a later attempt may select/create according to normal first-establishment policy.
- L0 succeeded but server/network fails → keep the same local identity; retry registration. **Never regenerate merely because registration failed.**
- Server A1 commits but response is lost → retry with the same OperationIdentity returns the committed result. Even if correlation metadata was lost, the same valid current identity must resolve to the same canonical Device rather than creating a second one.
- Server A1 commits but local persistence of returned `device_id` fails → next registration/lookup using the same identity resolves the same Device; missing local `device_id` reference is recoverable.
- Two local `axlic.exe` mutators race → `LocalMutationCoordinator` serializes local establishment/association. Server uniqueness remains the final canonical fence.
- An established hardware-backed provider later becomes temporarily unavailable → return provider-unavailable/recovery state; **do not silently create software identity**.

### F. FR19-ENR-01 — Product Backend enrollment / Organization claim

1. Product Backend `EnrollmentSessionService` creates an enrollment session and SetupCode/QR handle in Product SaaS state. No AxLicense authority changes.
2. `DeviceVerificationAdapter` requests an approved purpose-bound Device Assertion challenge through `AxLicenseServiceClient` / `ChallengeAuthority`.
3. Launcher invokes `axlic.exe device-assert`; `DeviceAssertion` validates trusted challenge purpose/audience/expiry before requesting a restricted provider proof from the pinned current identity.
4. `DeviceAssertionVerifier` validates challenge/assertion binding, replay status, registered Device/current identity and assurance. It returns minimum-disclosure verified-device context; it does not create ownership or license state.
5. Product Backend independently authenticates the human/account and checks Organization authorization.
6. `ProductDeviceAssociationService` commits `ProductDeviceAssociation` in the Product SaaS store as P1.
7. Device is now **managed but may still be unlicensed (U2)**. No automatic AxLicense activation follows merely from claim success.

**Failure / cancel / recovery:**

- SetupCode/session expires or user cancels before P1 → discard/expire Product workflow state; no AxLicense rollback is needed.
- Assertion verification fails/replays/expires → no association commit and no license mutation.
- P1 commits but HTTP/UI response is lost → Product Backend recovers from durable session/association state; it must not create duplicate association or ask AxLicense to undo anything.
- Product claim succeeds but later commercial licensing fails → remain valid U2 `managed + unlicensed`; retry commercial workflow independently.

### G. FR19-COM-01 — managed-but-unlicensed to commercial authority

1. Product Backend reads package/purchase/redeem/pool/included/grandfathered business facts and read-only catalog definitions.
2. `CommercialEntitlementPolicy` derives the desired stable entitlement IDs/constraints. SKU/display labels are not sent as authorization semantics.
3. `CommercialLicenseCoordinator` determines the canonical convergence plan using stable operation identities: create a Grant if none exists; revise an existing Grant when rights change; activate an unbound Grant to the registered Device when required.
4. Each AxLicense canonical mutation enters `TransportApi → CallerAuth → OperationCore`; domain modules validate/plan and only `CanonicalUnitOfWork` commits.
5. If `IssueLicenseGrant` commits but `ActivateDevice` later fails, the Grant remains a valid unbound Grant. Retry activation; do not compensate by deleting the Grant solely because a later transport/business step failed.
6. If activation commits, A2 now contains the authoritative active binding + signed credential outcome. Device observation is still independent; the device may remain U3 until refresh/install.
7. Product Backend records workflow/reconciliation status for operator UX, but such status is not license authority.

**Retry / cancel rules:**

- Product retry preserves the logical correlation for each AxLicense operation; transport retry cannot mint a second Grant/binding.
- Cancel before a canonical operation commit can stop that uncommitted work. After commit, cancel cannot erase history; an explicit successor lifecycle operation is required.
- Product association and AxLicense authority are not wrapped in a distributed transaction. U2 and U3 are expected recoverable states, not data-corruption states.

### H. FR19-CAT-01/02 — dynamic catalog registration and partial customer rollout

**Catalog registration:**

1. Release/Admin pipeline `ProductReleaseCatalogPublisher` submits a trusted Product/Entitlement definition mutation; ordinary Product Backend runtime is not allowed to publish catalog semantics.
2. Server `OperationCore` routes to `ProductEntitlementCatalogRegistry`; immutable machine semantics, current CatalogState and expected `CatalogRevision` are validated.
3. `CanonicalUnitOfWork` commits definition/catalog mutation + incremented `CatalogRevision` + lifecycle/audit/idempotency outcome.
4. Repeating the same ID with authorization-equivalent immutable semantics converges idempotently; conflicting semantics reject without mutation.
5. Product Backend `EntitlementCatalogClient` later observes the new definition for package authoring/validation. No customer Grant changes merely because catalog registration succeeded.

**Customer rollout:**

1. Commercial policy selects affected customer Grants independently of the catalog commit.
2. Rollout coordinator invokes one `ReviseLicenseGrant` per affected Grant with that Grant's `expected_authority_revision` and stable operation identity.
3. Each Grant succeeds or conflicts independently. Successful Grant commits remain committed if another Grant fails.
4. There is no cross-Grant ACID transaction and no global rollback of successful customers.
5. `CatalogRevision` is never compared to `authority_revision` and is never used by the device as customer-license authority.

### I. FR19-GRANT-01 — already-bound Grant revision and successor credential

1. `CommercialLicenseCoordinator` produces the desired full entitlement replacement set/constraints from trusted business policy + catalog semantics.
2. Server `GrantAuthority` validates current Grant state and `expected_authority_revision` and plans the new authority revision.
3. Because the Grant has an active binding, `CredentialIssuance` deterministically composes a successor credential candidate for the same `device_id/binding_id`, carrying the new authority revision and next credential generation.
4. `LicenseSigningPort`/Signing Authority signs the candidate. Signature production alone is not authority.
5. `OperationCore` revalidates conflict/CAS after signing. A post-sign conflict discards the candidate; it must never be delivered as authoritative.
6. `CanonicalUnitOfWork` atomically commits the Grant mutation + successor credential record/artifact + lifecycle event + operation result as A2.
7. Device may still hold the older valid credential. It enters U3 until it later observes the successor.
8. On `refresh`, `CredentialSync` resolves the current server credential. If newer, `CredentialManager` acquires local mutation discipline, re-reads current installed state, validates signature/schema/device/binding/authority revision/generation and performs verify-before-replace crash-safe install as L1.
9. Subsequent `RuntimeLicense` evaluation sees the newly installed entitlement snapshot.

**Failure / recovery:**

- Signer failure before canonical commit → no Grant revision change.
- Signed candidate created but CAS fails → candidate is discarded/non-deliverable; no authority change.
- A2 commit succeeds but response is lost → server authority remains committed; Product/device recover through idempotent operation result or `ResolveCurrentCredential`.
- Device candidate verification fails → keep old installed credential unchanged; return stable diagnostic/error classification.
- Power loss during local replace → local storage must expose either prior committed credential or new committed credential, not a half-written authority artifact.
- Stale lower authority revision or lower same-revision generation is rejected by anti-rollback. A higher authority revision may be a legitimate downgrade and must not be rejected merely because rights decreased.

### J. FR19-REF-01 — ordinary credential refresh is observation, not issuance

1. Product triggers `axlic.exe refresh` at an allowed lifecycle point such as app start, reconnect, software update or explicit user/admin refresh.
2. `IdentityManager` loads the already-pinned current identity; refresh does not perform provider reselection.
3. `CredentialSync` calls the server `CredentialResolver` through `ServerClient` using current device/identity context.
4. `CredentialResolver` reads canonical current binding/credential state and returns either current/no-change or the already-authoritative current credential. **It does not call Signing Authority.**
5. If the returned credential is authorization-equivalent to what is already installed at the same authority revision/generation, refresh is a no-op.
6. If a newer authoritative credential exists, `CredentialManager` validates and atomically installs it under local mutation coordination.
7. Repeating refresh does not increment `authority_revision`, does not increment `credential_generation`, does not create a binding and does not consume a license unit.

**Server/network failure:** an already-installed perpetual runtime credential remains locally usable according to its signed semantics; refresh failure is control-plane unavailability, not automatic license invalidation.

### K. FR19-REISSUE-01 — explicit credential reissue

`ReissueCredential` is reached only through an explicitly authorized key/format/controlled-recovery reason. It enters `OperationCore + CredentialIssuance + Signing Authority + CanonicalUnitOfWork`, keeps Grant authority semantics unchanged, increments `credential_generation`, and requires an authorization-equivalent entitlement snapshot for the same authority revision. Ordinary refresh must never fall through into this mutation automatically.

### L. FR19-ACT-01 — online activation and response-loss recovery

1. AxLicense validates activation authority, registered Device/current identity, active Grant and one-active-binding invariant.
2. Required credential candidate is signed and the server commits binding + credential + lifecycle event + operation result as one canonical outcome.
3. If the response reaches the caller, device may immediately install or later retrieve through `ResolveCurrentCredential`.
4. If the response is lost after commit, retry with the same OperationIdentity returns/reconstructs the committed outcome; it must not create another binding or consume another license unit.
5. If the product/backend learns that the canonical license is committed while the device has not installed it, display/project U3 `server licensed + observation pending` and invoke ordinary refresh/recovery delivery.

### M. FR19-ACT-02 — true offline activation

1. Device `OfflineExchange` creates an offline activation request bound to the current identity/proof/correlation. Request creation/export is transient/local and causes no server mutation.
2. File/USB/QR transport to a connected operator/admin side is untrusted transport.
3. Connected side submits the request to the normal canonical activation path. Server validation/commit semantics are equivalent to online activation.
4. Server produces/delivers the already-authoritative signed response/credential artifact after canonical commit.
5. Artifact is transferred back to the offline device through untrusted transport.
6. `OfflineExchange` hands candidate credential to `CredentialManager`; verify-before-replace + anti-rollback + atomic install establish L1.
7. Duplicate import of the same authoritative credential is idempotent/no-op. A stale credential cannot roll local authority backward.
8. Once a perpetual credential is installed, normal runtime evaluation remains server-independent.

### N. FR19-FAC-01/02 — factory identity provision and optional pre-activation

**Identity-only factory provisioning:** each physical device independently establishes local identity after image materialization and invokes `ProvisionDeviceIdentity` using scoped `ProvisioningAuthorization(identity_provision)`. Canonical commit creates/confirms only Device/current identity and consumes only the relevant scoped capacity when applicable. No license is implied.

**Optional pre-activation:** a separate authorized `FactoryPreActivateDevice` operation may create/confirm Grant + Active binding + credential according to factory scope. Batch tooling is coordination only; each physical device has an independent canonical commit. If device N fails, already committed devices 1..N-1 remain valid and device N can retry without rolling back the batch.

**Golden-image rule:** no current DeviceIdentity/private identity material/Binding/SignedCredential is inherited merely by cloning the image. Ordinary first-run self-registration must not consume factory ProvisioningAuthorization capacity.

### O. Recovery, provider failure and rehost

**Credential bytes lost/corrupted, identity intact:** use `CredentialSync → ResolveCurrentCredential` to redeliver the existing authoritative current credential whenever possible. Credential loss alone does not require a new Grant/binding/generation. An actual `ReissueCredential` is used only if policy explicitly requires reissue.

**Provider temporarily unavailable:** no canonical mutation. Return provider-unavailable/retry/support state; established provider stays pinned and there is no automatic TPM→software downgrade.

**Identity material lost but physical continuity proven:** support-controlled `RecoverDevice` may establish a successor identity epoch on the same Device and issue/recover the required credential according to P13. This is not ordinary fallback and must not be triggered merely because a local key file/reference is missing.

**Physical replacement:** establish/register the replacement Device identity, then `RehostDevice` performs one canonical old-binding→new-binding transition with successor credential. ProductDeviceAssociation/Organization transfer is a separate Product Backend concern; AxLicense rehost does not implicitly transfer product ownership, and product unclaim/transfer does not implicitly rehost or revoke the LicenseGrant.

### P. Cross-system failure / recovery matrix

| Failure point | Authoritative state after failure | Required recovery |
|---|---|---|
| L0 local identity established; registration network fails | Same local identity exists; no A1 yet | Retry `RegisterDeviceIdentity` with same identity/correlation semantics. Never regenerate because of transport failure. |
| A1 registration committed; response/local `device_id` persistence lost | Canonical Device exists | Retry/resolve same identity → same `device_id`; restore local reference. |
| P1 ProductDeviceAssociation committed; commercial licensing fails | Managed + unlicensed | Keep association; retry commercial workflow independently. |
| Grant issued; activation fails before binding commit | Valid unbound Grant | Retry activation. Do not delete Grant merely to simulate a distributed transaction. |
| A2 activation/revision committed; response lost | Server authoritative license/credential exists | Recover committed operation result or use `ResolveCurrentCredential`. |
| A2 committed; device install fails | Server licensed; local old/none credential remains | U3 observation-pending; retry resolve + verify-before-replace. |
| Catalog registration succeeds; some Grant rollout operations fail | Catalog remains registered; successful Grants remain updated | Retry only failed/conflicted Grants after fresh revision resolution. |
| Signer fails before commit | No new canonical authority | Retry operation according to idempotency policy. |
| Signer succeeds but final CAS/UOW fails | Signed candidate is non-authoritative/non-deliverable | Discard candidate; re-read canonical state and retry with valid expected revision. |
| Repeated ordinary refresh | No authority mutation | Return current/no-change; never reissue merely because polling repeated. |
| Local credential replace crashes | Old or new committed local snapshot, never intentional half-state | On next start re-read committed snapshot; resolve again if needed. |
| Established TPM/provider unavailable | Canonical identity unchanged | Retry/recover same provider; no silent software fallback. |
| Product Organization unclaim/transfer | Product association changes only | License remains until an explicit AxLicense lifecycle operation changes it. |
| Rehost canonical commit succeeds; old device is permanently offline | Server old binding closed/new binding active | New device obtains successor credential; old offline device may remain unable to observe server change until a future observable point, consistent with offline semantics. |

### Q. Concurrency, backpressure, cancellation and retry discipline

1. **Local writer serialization:** any operation that may establish identity, update local identity association or install credential uses `LocalMutationCoordinator`. After acquiring the writer, the command re-reads committed local state before replace; stale pre-lock snapshots are never trusted for mutation.
2. **Read isolation:** local status/entitlement reads may run without writer ownership but must observe an old or new committed snapshot, never half-written state.
3. **Server idempotency:** canonical mutations are deduplicated by P13 `OperationIdentity`; transport retries preserve logical correlation and cannot duplicate authority.
4. **Expected-revision conflicts:** Catalog and Grant CAS conflicts are explicit business/domain conflicts, not transport retries. Caller must re-read current state before deciding a new logical mutation.
5. **Batch rollout backpressure:** customer-wide entitlement rollout is a coordinator/job concern. Rate limit/retry per Grant; never expand the server transaction to thousands of Grants merely to obtain all-or-nothing behavior.
6. **Product↔AxLicense partial success:** `CommercialLicenseCoordinator` persists enough product workflow/reconciliation context to retry, but that context is not canonical license authority. No two-phase/distributed transaction is introduced.
7. **Cancel semantics:** cancel/expiry may stop uncommitted local/product/server work. Once a canonical mutation commits, cancellation is observation/UI only; reversal requires an explicit successor lifecycle operation.
8. **Delivery retries:** response/file/QR delivery can repeat safely because delivery is downstream of canonical authority and local verification/anti-rollback is idempotent.

### R. Preserved flows and explicit supersessions

The following previously traced areas remain Current Authority where consistent with FR-019: local entitlement evaluation, Device Assertion, diagnostics/support bundle, administrative suspend/resume/revoke observability, offline perpetual runtime, provider-pinning/no-silent-downgrade, crash-safe local mutation, same-device recovery and rehost.

Explicit supersessions:

- old `ID-02` wording that treated ordinary registration as provisioning-authorized is replaced by FR19-ID-02;
- old `ADM-02 Credential refresh / key-format rotation` is split into **FR19-REF-01 non-mutating resolve/delivery** and **FR19-REISSUE-01 explicit mutation**;
- old MIG-01/02/03 and FR-018 provisional/application/review/redemption runtime traces are historical compatibility only for the FR-019 NearHub default path;
- old §39 candidate gaps remain historical product-discovery notes. They are not silently ACCEPTED by this reconciliation and therefore cannot drive implementation unless a future requirement decision promotes them.

### S. P16 FR-019 consistency review

The targeted traversal finds **no new upstream semantic or product-authority gap**. All required FR-019 transitions can be assigned to trusted P15 modules while preserving:

- no second canonical truth;
- no distributed transaction between Product SaaS and AxLicense;
- ordinary identity registration without factory authority or license consumption;
- catalog-registration / customer-grant separation;
- one-active-binding and credential-required canonical atomicity;
- response-loss recovery from committed authority;
- local verify-before-replace and anti-rollback;
- ordinary refresh without signer/generation churn;
- provider pinning and no silent downgrade;
- perpetual offline runtime after valid credential installation.

### T. P16 disposition

**P16 FR-019 TARGETED RUNTIME DATA FLOW RECONCILIATION ACCEPTED / CLOSED.** P10–P16 now form a coherent FR-019 Current Authority chain from product requirement through semantic operations, system/module ownership and temporal runtime behavior.

**Earliest untrusted downstream layer:** **P17 Platform Contract**. P17 may now unpause and define the Windows-first realization contracts required by this runtime model — CLI process/wire/result compatibility, platform identity/provider capability boundary, protected-local-state realization contract, transport/platform error mapping and other platform-specific realization — without redefining the accepted P10–P16 behavior.
