---
authority_id: AXL-GOV-90
stage: governance
scope: axlicense
kind: governance-log
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c8159b134c1a6e3d94b39
migration_class: location-only
semantic_change: none
---

# 90 — AxLicense Governance & Decision Log

## Governance posture

AxLicense 使用 evidence-gated lifecycle。Current Authority 与 implementation reality 分开维护；代码存在不等于 requirement/architecture 已被接受。

## Lifecycle

1. **P00 Problem Discovery** — problem / constraints / success / non-goals / unknowns
2. **P01 Product Research** — alternatives / challenge evidence
3. **P02 Product Requirement** — JTBD / FR / NFR / acceptance criteria
4. **P03 Capability Traceability** — upstream value → downstream proof path
5. **Modeling / Design** — P10 Product Object Model → P11 Interaction / Behavior → P12 Semantic Schema → P13 Operation / Mutation；只有前序 semantic authority 可信后才进入 P14 Architecture
6. **Verification design** — 在实现前定义高价值 failure modes 与 proof
7. **Implementation / integration / release** — 受已接受 authority 控制

## Decision log

### D-001 — Build a lightweight AxLicense platform

**State:** Accepted direction / not architecture freeze

决定优先自研满足 Auditoryworks 核心需求的轻量 AxLicense，而不是采购完整 Cryptlex/FlexNet 类商业授权平台。

**Rationale:** 多产品共用、embedded/offline/OEM 场景明确；所需能力集中在 signed license、device identity、activation、entitlement、rehost、factory provisioning；商业平台大量 metering/floating/billing/reseller 能力不属于当前价值核心。

### D-002 — One shared licensing system across products

**State:** Accepted

NearHub、Axiom/Arc 及未来产品共享统一 license semantics。任何产品不得建立独立 incompatible activation format。

### D-003 — Runtime validation must be locally possible

**State:** Accepted

永久设备授权的运行路径不得把 AxLicense Server availability 变成单点依赖。Online/offline/factory 是 issuance/activation channel，不是不同的 runtime license model。

## Decision status after P00–P03 closure

- **O-001 ACCEPTED:** V1 license unit = per physical device。
- **O-003 ACCEPTED:** perpetual local runtime 可永久离线；cloud rights separate。
- **O-005 ACCEPTED:** factory 使用 online scoped provisioning station；V1 不做 delegated offline signing。
- **O-006 ACCEPTED:** V1 rehost authority = internal admin/support。
- **O-007 ACCEPTED:** runtime、maintenance/update、cloud-service rights 分离。
- **O-002 DEFERRED TO DESIGN/SECURITY:** Device Identity assurance realization。
- **O-004 NON-BLOCKING / DEFERRED:** trial trusted-clock semantics。
- **O-008 DEFERRED TO DESIGN/SECURITY:** signing algorithm / key custody；只允许标准密码原语。

### D-004 — Discovery closure and successor routing

**State:** Accepted routing / 2026-09-10

P00–P03 已满足 Discovery exit conditions。中央 Aegis 按 Earliest Untrusted Layer 顺序检查：Problem → Requirement 已可信；**Product Object Model 尚未建立 Current Authority，因此 earliest untrusted layer = P10 Product Object Model。**

**Route:** `P10 → P11 → P12 → P13 → P14`，其中后续阶段只有在前序 authority 足够可信时才继续；不得从 P03 直接跳到 P14。

**Next Primary Owner:** `aegis-modeling`，从 **P10 Product Object Model** 开始。当前 routing 不自动执行 P10 substantive work。

**Process profile:** **Full**。理由：AxLicense 涉及签名授权、设备绑定、签名密钥生命周期、长期 license-format compatibility、跨 Linux/Windows/Android 的安全语义；属于 security-critical + long-lived compatibility boundary。Full 仍受 blocking-evidence unique-detection 与 Anti-Proof-Recursion 原则约束，不为证明而证明。

## Authority rule

Accepted product policy 已写回 P00/P02/P03。后续 modeling/architecture 必须 trace 回这些 Current Authority，不得自行改变 per-device、perpetual-offline、rehost authority、factory connectivity 或 right-separation 语义。

## P10 modeling closure — 2026-09-10

**Status: ACCEPTED.** `aegis-modeling` 已完成 P10 Product Object Model。

### D-005 — Separate Grant, Binding, and Signed Credential

**State:** Accepted / P10 authority

`LicenseGrant`（授权业务 authority）、`DeviceBinding`（某次 physical-device assignment epoch）与 `SignedLicenseCredential`（immutable offline-verifiable credential）必须是不同对象。Rehost 保持 `license_grant_id`，关闭旧 binding、建立新 binding，并签发 successor credential；不得原地修改旧 signed credential。

### D-006 — Device is a physical-device identity, not an OS install

**State:** Accepted / P10 authority

`Device` 表示 physical device；OS reinstall、factory image 或 filesystem instance 不天然等于新设备。DeviceIdentity realization 可由后续 platform/security authority 决定，但镜像克隆不得复制有效 physical-device authority。

### D-007 — Entitlement identity is independent of commercial SKU

**State:** Accepted / P10 authority

`product_id` / `entitlement_id` 是稳定机器身份；Standard/Pro/Ultra 等 SKU/display labels 不进入客户端 canonical authorization identity。一个 LicenseGrant 可包含多个 product namespace 的 additive entitlement，以支持 NearHub bundle 携带 Axiom/Arc rights。

### D-008 — Offline credential observability is explicit

**State:** Accepted / P10 authority

Signed credential 是 immutable authority snapshot。Server-side suspension/revocation 不会改变已经离线保存的 artifact；永久离线设备只能在后续可观测 lifecycle point 获知远端状态变化。

### P10 handoff

Earliest untrusted layer 现为 **P11 Interaction / Behavior**。下一步应由 `aegis-modeling` 定义 online/offline activation、rehost、factory provisioning、recovery 等 session 的 start/transient/commit/cancel/retry 语义；不得提前进入 P14 architecture。

## P11 interaction / behavior closure — 2026-09-10

**Status: ACCEPTED.** `aegis-modeling` 已完成 P11 Interaction / Behavior。

### D-009 — Canonical commit and device observation are separate facts

**State:** Accepted / P11 authority

授权在 canonical durable state 成功 commit 时成立；设备是否已经收到、验证并安装结果是后续 observation。HTTP/file/transport response 丢失不能被解释为“没有 commit”，retry 必须能恢复已经提交的结果。

### D-010 — Cancel stops only uncommitted work; retry is idempotent

**State:** Accepted / P11 authority

Commit 前 cancel 不产生 durable authorization mutation；commit 后不能通过 session cancel 回滚历史，必须用显式 successor lifecycle action。相同逻辑请求 retry 必须收敛到同一 durable outcome，不得重复创建 Active binding、重复占用 license unit 或重复消耗 factory capacity。

### D-011 — Online and offline activation share one authorization semantic

**State:** Accepted / P11 authority

Online activation 与 offline request/response 只是不同 transport/issuance channel。两者 canonical commit 都建立同一 `DeviceBinding + SignedLicenseCredential` 语义；offline request 本身不是 license，同一 credential 重复 import 必须幂等。

### D-012 — Rehost is one old→new authority transition

**State:** Accepted / P11 authority

Rehost 保持 `LicenseGrant` identity，在一个 canonical transition 中使旧 binding 不再 Active、新 binding 成为唯一 Active，并签发 successor credential。旧设备若永久离线，可能继续使用旧 credential 直到下一 observable lifecycle point；V1 不承诺远程瞬时失效。

### D-013 — Factory provisioning commits per physical device

**State:** Accepted / P11 authority

Factory batch 只是 coordination；canonical commit 以单台 physical Device 为单位。批次中断时已 commit 设备保持有效，未 commit 设备可恢复继续；retry 不得再次消耗 provisioning capacity。Golden image 不得预置可被克隆的有效 Device identity/credential。

### D-014 — Recovery follows identity continuity, not file presence

**State:** Accepted / P11 authority

OS/app reinstall、credential loss 或 corruption 只有在仍能证明同一 DeviceIdentity/current binding 时才属于 same-device recovery，不消耗新的 license unit或建立新 binding。若 identity continuity 无法证明，则必须转入 support-controlled rehost/reprovision，而不能仅凭复制旧 license 文件自我恢复。

### D-015 — Administrative lifecycle does not bypass offline observability

**State:** Accepted / P11 authority

Grant issuance 不等于 device activation；deactivate/suspend/revoke 是明确 durable lifecycle transitions。它们不会原地修改已签发 credential bytes，也不能对永久离线设备声称即时生效。所有 committed lifecycle changes 必须有 append-only audit event。

### P11 handoff

Earliest untrusted layer 现为 **P12 Semantic Schema**。下一步仍由 `aegis-modeling` 把 P10 object identity 与 P11 behavior outcomes 编码为 canonical schema：稳定 IDs、字段语义、validity/right representation、binding/credential references、versioning/compatibility 与 validation。不得在 P12 提前决定 REST/database/module/process architecture。

## P12 semantic schema closure — 2026-09-10

**Status: ACCEPTED.** `aegis-modeling` 已完成 P12 Semantic Schema。

### D-016 — Entitlement presence is the grant; RightKind is definition-owned

**State:** Accepted / P12 authority

`EntitlementGrant` 以 entry presence 表达授权，不建立第二个 `enabled` Boolean。`runtime | maintenance_update | cloud_service` 属于 `EntitlementDefinition` 的稳定语义；同一 `entitlement_id` 不得在不同 LicenseGrant 中改变 RightKind。

### D-017 — Canonical lifecycle state excludes transient/derived duplicates

**State:** Accepted / P12 authority

`DeviceBinding.pending` 不进入 canonical state；pending 只属于 interaction/session。`ProvisioningAuthorization` 只持久化 active/revoked，expired/exhausted 由 validity + committed usage 推导。Issued 是 lifecycle fact，不额外制造与 active 重叠的 LicenseGrant 状态。

### D-018 — Credential authority is fully protected and self-sufficient for offline evaluation

**State:** Accepted / P12 authority

`SignedLicenseCredential` 的 protected payload 必须包含 grant/binding/device identity references、authority revision、credential generation、issued time 与 entitlement snapshot；所有影响 entitlement/device-binding/validity 的字段必须被签名完整性覆盖。Unsigned metadata 永不成为授权依据。

### D-019 — Separate authority revision from credential generation

**State:** Accepted / P12 authority

`LicenseGrant.authority_revision` 表示授权语义变化并单调递增；`credential_generation` 在同一 grant/binding 内表示 immutable credential successor issuance，可用于 entitlement refresh、key/format migration 等而不创建新 binding。

### D-020 — Schema compatibility is fail-closed on authorization-critical ambiguity

**State:** Accepted / P12 authority

Credential schema 使用 major/minor compatibility 语义：major 表示可能不兼容；minor 只能做不会改变旧 verifier 授权结果的 additive evolution。Missing/malformed required field、unknown authorization-critical enum/semantics、unknown key/algorithm 均 fail closed；营销 rename 不改变 ProductId/EntitlementId。

### D-021 — Device identity representation is opaque and scheme-versioned

**State:** Accepted / P12 authority

`DeviceIdentity` 以 `scheme_id + opaque identity_value + identity_epoch` 表达。P12 不冻结 fingerprint/TEE/TPM/Keystore realization；identity scheme 可以演进而保留同一 physical `device_id`，但同一 current identity 不得同时映射到多个 active/non-retired Device。

### P12 handoff

Earliest untrusted layer 现为 **P13 Operation / Mutation Model**。下一步仍由 `aegis-modeling` 定义显式 domain mutations、payload、precondition、atomicity、ordering、idempotency/dedup、replay 与 error behavior；必须保持 P10/P11/P12 已冻结的 Grant/Binding/Credential 分离、one-active-binding、credential immutability、offline observability 和 semantic versioning。不得提前进入 P14 architecture。

## Requirement reconciliation routing — 2026-09-12

### D-022 — Legacy migration and image-clone identity policy reopen P02

**State:** Accepted routing / BLOCKED_AUTHORITY at P13

During P13 preparation, new product requirements were confirmed:

- already-sold devices must be able to adopt AxLicense through software update without recall;
- a golden/factory image may be cloned, but no current DeviceIdentity, device private key, DeviceBinding, or signed credential may be inherited merely because the image was cloned;
- DeviceIdentity establishment and License consumption are separate lifecycle facts;
- factory workflows must still support the existing FR-007 outcome in which selected shipped products can leave production already licensed, but identity provisioning itself must not imply that every manufactured/inventory device has consumed a LicenseGrant;
- legacy devices may establish a new DeviceIdentity locally, but binding that identity to historical commercial rights requires explicit legacy-migration authority/evidence; identity bootstrap alone must never self-authorize a license.

**Authority conflict found:** Existing P02 FR-007 and P11 Factory Provisioning model factory provisioning as a flow whose successful commit includes authorization provisioning. The new requirement introduces an additional identity-only provisioning state and a new legacy-migration lifecycle that are not represented in the accepted P02/P03 baseline. P13 therefore may not silently redefine these semantics.

**Earliest Untrusted Layer:** **P02 Product Requirement**.

**Required reconciliation path:**

`P02 targeted reconciliation → P03 traceability reconciliation → P10 object-model reconciliation → P11 behavior reconciliation → P12 schema reconciliation → P13 Operation / Mutation Model`

**Required P02 additions/clarifications for the next owner:**

1. Legacy Device Migration as an explicit V1 requirement, including online/offline migration paths and fail-closed authorization rules.
2. Golden Image Identity-Free invariant: device private key/current identity/license credential must be created or installed only after clone/materialization on the target physical device.
3. Separate `Identity Provisioning` from `License Activation/Factory Pre-Activation` while preserving the existing requirement that designated products may ship already licensed.
4. Define reset/reinstall/RMA expectations: ordinary update/reinstall should preserve physical-device identity where possible; identity loss requires controlled recovery; physical replacement requires rehost.
5. Migration safety acceptance: possessing a legacy image or merely generating a new keypair must never be sufficient to mint/claim historical entitlements.

**Next Primary Owner:** `aegis-discovery` at **P02 targeted requirement reconciliation**.

P13 remains blocked until this upstream reconciliation is completed and downstream semantic authorities are revalidated.

## Targeted reconciliation closure — 2026-09-12

### D-023 — Identity establishment and license consumption are separate authority facts

**State:** Accepted / reconciled P02–P13 authority

Golden Image 是 identity-free software template。DeviceIdentity establishment 可以独立完成，并允许 Device 长期处于“已注册但无 Active Binding/Credential”的 canonical 状态。Factory `identity_provision` 不得隐式升级为 `factory_pre_activation`；只有显式授权 mutation 才能消费 LicenseGrant/capacity 并签发 credential。

### D-024 — Legacy migration is an explicit authority-gated lifecycle

**State:** Accepted / reconciled P02–P13 authority

已售历史设备可以通过软件升级建立 DeviceIdentity，但 identity bootstrap、旧 IMG、旧 license 文件、MAC 或新生成 keypair 均不能自我授予历史 entitlement。`LegacyMigrationAuthorization` 是独立 canonical authority；online/offline migration 都必须通过同一 `MigrateLegacyDevice` canonical mutation 消费 historical claim/capacity，并保持 retry idempotent。

### D-025 — Recovery preserves physical-device identity; replacement requires Rehost

**State:** Accepted / reconciled P10–P13 authority

identity material 丢失时，只有受信 continuity evidence 足以证明仍为同一 physical Device，才允许在同一 `device_id` 下增加 `identity_epoch` 并恢复同一 binding。无法证明 continuity 或 physical board/device 已替换时，必须走 Rehost/new Device path。

### D-026 — Factory lifecycle is two-step and independently scoped

**State:** Accepted / reconciled P11–P13 authority

Factory canonical flow 分为：

`Golden Image materialize → ProvisionDeviceIdentity → optional FactoryPreActivateDevice`。

两步拥有独立 action scope / capacity / commit point。Identity provisioning 成功而 pre-activation 失败时，Device 保持 identity-only inventory state，不回滚 identity。

### D-027 — P13 Operation / Mutation Model closed

**State:** ACCEPTED / CLOSED 2026-09-12

P13 已冻结：

- stable `CorrelationId` / OperationIdentity idempotency contract；
- canonical operations：IssueLicenseGrant、ProvisionDeviceIdentity、ActivateDevice、FactoryPreActivateDevice、CompleteOfflineActivation、MigrateLegacyDevice、RecoverDevice、RehostDevice、ReviseLicenseGrant、RefreshCredential、DeactivateDeviceBinding、Suspend/Resume/RevokeLicenseGrant；
- transient/local operations：CreateOfflineActivationRequest、CreateOfflineLegacyMigrationRequest、InstallSignedCredential；
- binding + required credential logical atomicity；
- grant authority revision、identity epoch、credential generation 三个 ordering domain；
- replay/concurrency/error contract；
- one-active-binding、credential immutability、legacy-migration authority、factory action-scope isolation、recovery/rehost boundary。

**Semantic Foundation Complete:** `P10 → P11 → P12 → P13` 已重新建立一致 Current Authority。

**Successor route:** earliest untrusted layer = **P14 System Architecture**；next owner = `aegis-architecture`。P14 不得重新定义上述 canonical semantics。

## Windows-first Device Identity Assurance policy — 2026-09-12

### D-028 — Windows is the V1 production reference for Device Identity

**State:** Accepted / P02-P03 policy authority

V1 Device Identity provider implementation prioritizes Windows 10/11. Windows must implement provider discovery, TPM-backed identity path, software-persistent fallback, diagnostics, and recovery integration. Linux ARM64/RK、Android 及其他平台在 V1 只保留 platform-neutral provider/capability contract 与 scheme extensibility；不要求实现 TPM/OP-TEE/RPMB/Android Keystore provider。

### D-029 — Provider capability, not hardware label, decides usability

**State:** Accepted / P02-P03 policy authority

`TPM present`、`TEE present` 或产品 SKU 不足以证明 provider 可用于 AxLicense。首次 identity establishment 必须基于实际 provider qualification：至少能够区分 present / usable / temporarily unavailable / unsupported，并验证 identity create/load/use capability。Capability probe 本身不产生 DeviceIdentity 或 license mutation。

### D-030 — Windows V1 assurance ladder and fallback

**State:** Accepted / P02-P03 policy authority

Windows V1 assurance classes：`hardware_bound`、`software_persistent`、`unsupported`。首次 identity establishment 默认选择：usable TPM-backed provider → `hardware_bound`；否则若满足 clone-boundary 与 persistence minimum → `software_persistent`；否则 `unsupported` 并 fail closed。普通 Windows activation / legacy migration 默认最低允许 `software_persistent`；future product/batch policy 可以提高 minimum assurance 到 `hardware_bound`。

### D-031 — Existing identity never silently downgrades

**State:** Accepted / security policy

Provider selection 只发生在首次 identity establishment 或显式 controlled provider migration/recovery。已经建立 `hardware_bound` identity 后，TPM/provider 暂时不可用必须返回 provider-unavailable/recovery path；不得自动生成 software identity、不得创建第二 Device、不得因 runtime probe result 改写 current identity。Provider/scheme migration 只有在同一 physical Device continuity 可证明时才允许增加 `identity_epoch`；否则走 Rehost/重新授权。

### D-032 — Existing P10-P13 semantic authority remains valid

**State:** Accepted routing

本轮 policy reconciliation 没有改变 `Device` / `DeviceIdentity(scheme_id + identity_epoch)` / `LicenseGrant` / `DeviceBinding` / `SignedLicenseCredential` 的 canonical semantics，也没有新增 durable mutation vocabulary。因此 P10–P13 不重新打开。Earliest untrusted layer 保持 **P14 System Architecture**；P14 负责 provider abstraction/policy ownership，P17 再冻结 Windows provider/platform contract 与具体 probe/storage integration。

## P14 System Architecture closure — 2026-09-12

**Status: ACCEPTED.** `aegis-architecture` 已完成 Windows-first AxLicense V1 P14 System Architecture。

### D-028 — Windows Agent is the single device-local AxLicense authority

**State:** Accepted / P14 authority

Windows V1 使用独立 `AxLicense Agent` system service。NearHub/Axiom/其他产品通过 thin SDK + local IPC 消费 status/entitlement/control；产品进程不得直接操作 TPM/software identity key、license file 或建立第二套 entitlement truth。

### D-029 — V1 server is a modular monolith with one canonical ACID boundary

**State:** Accepted / P14 authority

AxLicense Server V1 保持 Operation Core、License/Device domain、Policy、Credential Issuance、Audit 在一个 logical deployable authority 中，并使用单一 logical ACID canonical persistence boundary。当前不拆微服务，避免破坏 P13 Binding/Credential/Audit/Idempotency logical atomicity。

### D-030 — Device Identity is policy + provider abstraction + provider realization

**State:** Accepted / P14 authority

Identity architecture 分为 policy/selection、provider-neutral capability boundary、platform realization。Windows V1 实现 TPM hardware-backed 与 software-persistent provider；Linux/RK/Android 只保留 provider seam。Existing identity provider-pinned；provider failure 不重新执行 fallback selection。

### D-031 — Credential-required mutations sign before canonical commit, deliver after commit

**State:** Accepted / P14 authority

禁止 `commit binding → later async sign credential` 作为成功模型。Operation Core 必须先建立/恢复 operation identity、验证 state、构造并签名 candidate credential，再在重新验证 expected revision 后用单一 ACID commit 写入 canonical mutation、credential record、capacity consumption、lifecycle event 与 operation result；commit 后才 Deliver/Observe。

### D-032 — Signing Authority and device private identity material are separate trust boundaries

**State:** Accepted / P14 authority

Server application 不持有 master signing private key；Signing Authority 不拥有 LicenseGrant/Binding policy。Device private identity material 保持在目标 Windows device/provider boundary 内，factory/server 只消费 claim/proof/reference。

### D-033 — Local perpetual runtime is server-independent

**State:** Accepted / P14 authority

正常 runtime path 为 `Product → SDK → Agent → Runtime License Core → local identity evidence + signed credential`，不访问 AxLicense Server。Server/Internet/KMS/DB outage 不得阻止已安装 perpetual local runtime；它们只影响新的 control-plane mutation/issuance。

### P14 handoff

P14 exit conditions 已满足。**Earliest untrusted layer = P15 Module Design**，下一 Primary Owner 仍为 `aegis-architecture`。P15 应把 Agent、Runtime Core、Device Identity、Provider Registry、Local State、Server Operation Core、Policy、Credential Issuance、Audit 等 subsystem 细化成稳定 module/interface/invariant；不得提前把 Windows TPM API 或具体 storage mechanism 混入 common semantics。

## P14 targeted deployment reconciliation — 2026-09-12

### CLI-only Windows V1

**State: Accepted / P14 authority reconciliation**

- Windows V1 device-side deployable 由 mandatory `AxLicense Agent` / local IPC 改为 **on-demand `axlic.exe` CLI**。
- V1 **不发布 public `axlic.dll`**；AxLicense Core 可以内部以 static library/module 组织，但不形成外部 ABI authority。
- 外部产品只依赖 `axlic.exe` 的 versioned command/result contract，不得直接访问 TPM/KSP private-key primitive、protected local state 或实现第二套 authoritative verifier。
- 取消常驻 Agent 后，所有会修改 local identity association / credential store / provider lifecycle 的 command 必须具备 machine-wide single-writer coordination 与 crash-safe commit；read-only query 可以并发。
- CLI 不得提供 arbitrary signing oracle 或 private-key export。
- DLL / Service Host 仅在未来有测量到的 process-start 性能问题、更强 service-account ACL 隔离、后台 refresh/push/heartbeat 或多产品长期共享状态需求时重新评估；任何 successor host 必须复用同一 AxLicense Core 与 P10–P14 semantics。

**Impact:** deployment simplification only。P10–P13 不重新打开；P14 保持 CLOSED；successor 仍为 **P15 Module Design**。

### D-034 — Diagnostics are minimum-disclosure and policy-gated

**State:** Accepted / P14 authority reconciliation / 2026-09-12

AxLicense V1 diagnostics adopt a three-layer exposure model: minimal stable product-facing CLI result, bounded structured local production log, and explicit sanitized Support Bundle. Diagnostics must be useful for support without exposing bypass-relevant implementation detail or reconstructable authorization/identity authority.

Rules:

- product-facing output is limited to stable status/error/category/retry/action/correlation information;
- local production logs are structured against registered event/field schemas; security-sensitive modules do not freely print unclassified payloads;
- Support Bundle export is explicit, sanitized before packaging, and production bundles should be encrypted to a support-only public key/recipient;
- private identity/signing material, bearer/activation/admin/factory secrets, authorization headers, secret-bearing migration/offline fields, and credential signing preimages are never logged or exported at any diagnostic level;
- raw canonical identifiers and machine/customer identifiers are sensitive-by-default and should be replaced with opaque diagnostic references unless an approved field policy requires otherwise;
- local diagnostics are non-authoritative and removable/rotating; server `LicenseLifecycleEvent` audit remains canonical lifecycle evidence and never depends on device logs.

**Successor constraint:** P15 must define a dedicated Diagnostics & Safe Logging module/policy boundary, stable error taxonomy, registered event schema, field classification/redaction policy, retention/rotation, support-bundle sanitization, and correlation contract. Exact Windows logging backend, ACL/path, crash integration, bundle crypto profile and support-key handling remain P17/P18 concerns.

**Impact:** P10–P13 remain closed; P14 remains CLOSED; next stage remains P15 Module Design.

## Device Assertion & Product Ownership Recovery requirement reconciliation — 2026-09-12

### D-035 — Device registration is separate from account/organization ownership

**State:** Accepted / P02-P03 targeted reconciliation

AxLicense DeviceIdentity registration establishes trusted physical-device identity only. It does not establish customer/account/organization ownership. A device may legally exist as registered but unclaimed/unassociated in the consuming product backend.

The preferred product-association commit point is successful completion of an authenticated enrollment/claim session after both device possession and human/organization authority are proven. For enterprise/kiosk products, durable ownership should normally be `Device → Organization/Tenant`; the individual user is the claim/admin actor. Direct Device→personal-account ownership is product-specific, not the default enterprise model.

### D-036 — Device Identity Assertion is purpose-bound evidence, not ownership authority

**State:** Accepted / P02-P03 targeted reconciliation

AxLicense V1 adds a cross-product `Purpose-Bound Device Identity Assertion` capability. A trusted backend may issue an audience/purpose/nonce/expiry-bound challenge and obtain proof that the currently participating device controls its established DeviceIdentity. AxLicense must not expose arbitrary private-key signing, and an assertion alone must not reveal an account, reset credentials, rebind ownership, or authorize transfer.

QR/account recovery therefore uses dual evidence:

1. device proof from AxLicense;
2. independent human/account/organization authentication/authorization from the product account system.

Ordinary forgotten-credential recovery restores access to an existing association; changing to a different account/organization is a distinct privileged ownership-transfer flow.

### Routing impact

P02/P03 are reconciled with FR-016, FR-017 and NFR-SEC-04. Because FR-016 creates a new public cross-product device-trust capability, **P15 is paused** until downstream targeted reconciliation completes:

`P10 impact review → P11 assertion/enrollment behavior → P12 challenge/assertion schema → P13 operation/command contract → P14 architecture boundary reconciliation`.

Earliest untrusted layer = **P10 targeted reconciliation**; next Primary Owner = `aegis-modeling`.

## Device Assertion downstream reconciliation + P15 closure — 2026-09-12

### D-037 — Product ownership objects stay outside AxLicense canonical model

**State:** Accepted / P10 targeted reconciliation

`Account`, `Organization/Tenant`, `ProductDeviceAssociation`, product management credential and ownership-transfer history remain consuming-product/backend objects. AxLicense owns `Device/DeviceIdentity` plus short-lived Device Assertion evidence only. A registered Device may remain externally unclaimed indefinitely.

### D-038 — Claim, access recovery, binding-credential recovery, and ownership transfer are distinct behaviors

**State:** Accepted / P11 targeted reconciliation

First claim requires both valid device proof and human/organization authorization before product association commit. Username/password/SSO recovery does not mutate an existing Device→Organization association. Product management-credential recovery reissues product access only. Ownership transfer is a separate high-authority `A → B` product-side transition and cannot be authorized by QR/device possession alone.

### D-039 — Device Assertion is challenge-bound transient evidence

**State:** Accepted / P12 targeted reconciliation

`DeviceAssertionChallenge` and `DeviceIdentityAssertion` are non-canonical security evidence. Challenge semantics bind issuer, audience, purpose, session, nonce, expiry and optional expected device. Assertion binds the current registered `device_id + identity_epoch + scheme_id` to that trusted challenge without exposing raw DeviceIdentity/private material. Unknown authorization-critical semantics fail closed.

### D-040 — Device Assertion operations are transient and never generic signing

**State:** Accepted / P13 targeted reconciliation

`CreateDeviceIdentityAssertion` and `VerifyDeviceIdentityAssertion` create/verify transient proof only; they do not mutate Device, Grant, Binding, Credential or product ownership state. Public arbitrary `sign(data)` / private-key export is forbidden. Established hardware identity unavailable returns provider-unavailable/recovery semantics and never silently falls back to software identity.

### D-041 — Challenge Authority and Assertion Verifier are separate from product ownership

**State:** Accepted / P14 targeted reconciliation

AxLicense Server owns logical Challenge Authority + Device Assertion Verifier capabilities. Challenge signing is purpose-separated from license-credential signing. Product backend owns QR/recovery session, IAM/SSO, Account/Organization/ProductDeviceAssociation and the final claim/recovery/transfer commit. QR carries only an opaque short-lived session handle; raw DeviceIdentity/proof/private material is never embedded in the QR.

### D-042 — P15 module architecture is established

**State:** Accepted / P15 authority

Windows V1 keeps a single public device integration binary: `axlic.exe`; no public DLL and no mandatory Agent/Service. Device-side modules are frozen around `CliFrontend`, `CommandApplication`, Runtime/Credential/Identity/Assertion, local-state/single-writer coordination, transport/offline exchange and Diagnostics/SupportBundle. Server modules preserve a single OperationCore/CanonicalUnitOfWork mutation boundary plus separate ChallengeAuthority/AssertionVerifier security capabilities. Diagnostics has one policy-gated structured logging boundary.

### P15 handoff

**P15 ACCEPTED / CLOSED — 2026-09-12.** Device Assertion reconciliation is complete through P10→P14 and has been incorporated into P15. Earliest untrusted layer = **P16 Runtime Data Flow**; next Primary Owner remains `aegis-architecture`. P16 must explicitly trace normal/failure/retry/crash/replay flows for license lifecycle, local-state corruption, server disaster recovery, AxLicense upgrade compatibility, and Device Assertion enrollment/recovery/transfer.

## P16 Runtime Data Flow review opened — 2026-09-12

**State:** DRAFT / PRODUCT-FLOW REVIEW REQUIRED. `aegis-architecture` has traversed the current P02–P15 Authority across local runtime, identity/registration, online/offline activation, factory provisioning/pre-activation, legacy migration, administrative lifecycle, same-device recovery, RMA/rehost, product enrollment/account recovery/management-credential recovery/ownership transfer, diagnostics/support bundle, device concurrency/crash/local corruption, AxLicense self-upgrade, server outage/transaction/signing failure, disaster recovery, signing-key rollout, Device Assertion replay/failure and product-backend association commit failure.

This P16 draft intentionally **does not create new product Authority** from gaps discovered during architecture traversal. The surfaced candidates C-01..C-14 are review inputs only. High-priority review items are: device decommission/secure disposal; RMA product-association/config migration; commercial Licensee transfer when product ownership changes; lost/stolen/compromised device; fully offline enrollment/account recovery; return-to-unclaimed/refurbishment; and activation-material recovery/reissue.

**Routing:** P17 is paused. If product review accepts the current flow set without new capability, P16 may close and route to P17 Platform Contract. If product review accepts any new capability, route that capability to its Earliest Untrusted Layer (normally P02/P03 for new product behavior), reconcile downstream semantic/architecture authorities as needed, then regenerate/finalize P16.

## Legacy Provisional Migration requirement reconciliation — 2026-09-12

### D-043 — Historical devices use provisional migration, not inferred grandfather entitlement

**State:** Accepted / P02–P03 targeted reconciliation

Historical deployed devices have no reliable pre-AxLicense hardware/account/backend record. Therefore AxLicense will not attempt to infer historical entitlement from missing records, old files, MAC/serial, legacy software presence or a newly created DeviceIdentity.

Accepted policy:

1. Factory/new devices use the normal strict identity/activation path and do not receive historical provisional rights merely because AxLicense has never seen them.
2. A device arriving through the designated legacy in-place software-upgrade entry path may be classified as `legacy_upgrade_candidate`; that classification is rollout evidence only and never entitlement authority.
3. If such a candidate has no valid AxLicense credential, the product may continue operating in **Legacy Provisional Mode** with a persistent visible `Unactivated/未激活` watermark.
4. The user may enter a contact email and submit a migration application. If offline, the application is durably queued and retried when connectivity returns; the queued request itself grants nothing.
5. The backend, not the device, owns case creation and email notification. Repeated uploads are deduplicated by stable request/device identity.
6. Human support/customer-success review remains the historical migration gate. High-volume request clusters may trigger direct customer-development outreach to understand fleet size, deployment scenario and management needs; low-volume/single-device requests may be approved through a normal email response flow.
7. Approved migration uses a device/request/product-scoped, replay-safe activation/migration response rather than a generic reusable license code, and completes through the ordinary `MigrateLegacyDevice`/offline migration authority path.
8. After successful migration, the Device follows ordinary AxLicense update/reinstall/recovery semantics and does not repeat the historical migration application.

**Risk posture:** Legacy upgrade provenance cannot prove historical physical-device sale. V1 explicitly accepts this limitation because provisional mode remains visibly unactivated and **full entitlement is never automatically grandfathered**. Human approval is the commercial control that turns a migration candidate into an authorized historical license.

### Routing consequence

FR-018 introduces new product-visible runtime and workflow semantics. P16 remains open. Earliest untrusted downstream layer is **P10 targeted impact review**; required route is `P10 → P11 → P12 → P13 → P14 → P15 → P16 targeted reconciliation` as needed. Next Primary Owner after this Discovery handoff: `aegis-modeling`. P17 remains paused.

## FR-018 P10 targeted reconciliation — 2026-09-12

### D-044 — Legacy provisional usage is workflow state, not license authority

**State:** Accepted / P10 targeted reconciliation

FR-018 does not introduce a second temporary-license model. P10 freezes the following object boundaries:

1. `LegacyMigrationApplication` is the only new durable workflow entity required by FR-018. It records a stable historical-migration application tied to Device/product/contact/workflow context but grants no entitlement.
2. Existing `LegacyMigrationAuthorization` remains the sole durable human-approved authority that may permit formal `MigrateLegacyDevice` completion.
3. `LegacyProvisionalMode` is derived product runtime state only. It must not be represented as `LicenseGrant`, `DeviceBinding`, `SignedLicenseCredential`, temporary entitlement, or auto-grandfather credential.
4. `legacy_upgrade_candidate` is non-authoritative provenance/routing evidence. Absence of AxLicense history is never legacy eligibility.
5. `LegacyMigrationOutboxEntry` is local durable delivery state used to preserve one application identity through offline/retry; persistence does not make it authorization truth.
6. Email, CRM/support case, domain/request-count grouping and customer-development heuristics are contact/business workflow information only and cannot establish Licensee/Organization ownership or migration authority.
7. A user-facing migration code/offline response is only a scoped redemption/transport representation of an approved migration context; it must not become a reusable generic license key.
8. After migration commits, the Device exits all special legacy workflow semantics and returns to ordinary AxLicense update/reinstall/recovery behavior.

**Routing consequence:** P10 FR-018 targeted reconciliation is **ACCEPTED**. Earliest untrusted layer advances to **P11 Interaction / Behavior**. Next Primary Owner remains `aegis-modeling`; P11 must define provisional entry/exit, application submission, offline outbox retry, human approval/rejection, scoped response redemption, cancel/retry and post-migration behavior. P17 remains paused.

## FR-018 P11 targeted reconciliation — 2026-09-12

### D-045 — Human-approved migration has separate application, approval, commit, and observation boundaries

**State:** Accepted / P11 targeted reconciliation

FR-018 behavior is frozen as a non-authoritative provisional/workflow layer terminating in existing AxLicense authorization semantics.

1. A designated legacy-upgrade candidate may enter `legacy_provisional` only after DeviceIdentity establishment and only while no valid AxLicense credential is locally available; Factory/new-install/unknown devices do not obtain provisional rights merely from missing records.
2. `LegacyMigrationApplication` submission, local offline outbox, backend case creation, email delivery and customer-development grouping are workflow facts only. None grants entitlement.
3. Offline application retry and duplicate online submission must converge on the same logical application/case; transport retry must never multiply migration authority or licenses.
4. Contact email can be corrected while an application is open, but email never becomes Device identity, Organization ownership, Licensee authority or historical-entitlement proof.
5. Human **approve** produces/references durable `LegacyMigrationAuthorization`; notification/email delivery is separate. Lost email must be recoverable by resending/retrieving the same approval rather than creating a second approval.
6. Human **reject** creates no migration authority and no license mutation. Rejection does not itself hard-lock the product; the device remains subject to provisional rollout policy until a later product policy says otherwise. Any grace/deadline/hard-stop policy would require a new P02 decision.
7. User-visible activation/migration code is only a device/request/product-scoped redemption representation. Wrong-device/product redemption fails before consumption. Same-device retry after canonical commit returns/recover the committed result instead of creating another grant/binding.
8. Online redemption completes through existing `MigrateLegacyDevice`; air-gapped completion uses the existing offline migration request/response path rather than inventing a universal offline code.
9. The visible `Unactivated / 未激活` watermark is removed only after the target device has verified and installed a valid AxLicense credential. Server approval or canonical commit without local observation is not sufficient for local Licensed state.
10. A provisional legacy candidate may also exit provisional through ordinary legitimate AxLicense activation. After either migration or ordinary activation, the Device enters normal AxLicense lifecycle and must never re-enter legacy provisional merely because local files/credential are later lost; recovery/rehost semantics apply instead.

### P11 handoff

**P11 FR-018 TARGETED RECONCILIATION ACCEPTED.** Earliest untrusted layer advances to **P12 Semantic Schema**. P12 must encode `LegacyMigrationApplication` identity/status/contact lineage, outbox/request identity semantics, approval/redemption binding and the derived/non-authoritative nature of provisional state without introducing a second license schema.

## FR-018 P12 targeted reconciliation — 2026-09-12

### D-046 — Legacy migration workflow schema is durable but non-authoritative; approval scope binds application/device/product

**State:** Accepted / P12 targeted reconciliation

FR-018 semantic schema is frozen with the following boundaries:

1. `LegacyMigrationApplication` is durable server workflow truth with stable identity, contact revision history, lineage and workflow status; it is not entitlement authority.
2. `LogicalApplicationId` is the stable retry/dedup identity carried from local offline outbox through backend intake. Reconnect, process restart, duplicate click and lost response reuse the same logical identity.
3. Contact email is sensitive workflow data only. Contact revisions are monotonic; stale retries cannot overwrite a newer contact revision. Email/domain/request-count grouping never establishes Licensee, Organization ownership or historical entitlement.
4. Existing `LegacyMigrationAuthorization` remains the sole human-approved historical migration authority. FR-018 support approval must explicitly bind the approved application and target Device; single-device approval has capacity 1 and product/entitlement scope.
5. Customer-facing migration code/handle is a scoped redemption representation, not a reusable generic license key or SignedLicenseCredential. Wrong-device/product/request redemption fails before authorization consumption.
6. Redemption does not create a second durable `consumed` flag. Consumption/replay status is derived from the migration authorization plus committed `MigrateLegacyDevice` outcome/lifecycle evidence.
7. `LegacyProvisionalState` remains derived product/runtime state. Absence of a credential or server record alone never creates provisional eligibility; a previously migrated/activated Device with missing credential routes to ordinary recovery.
8. FR-018 workflow/contact/provisional data MUST NOT enter `CredentialPayload`. Existing SignedLicenseCredential schema and compatibility semantics remain unchanged.

**Routing consequence:** P12 FR-018 targeted reconciliation is **ACCEPTED**. Earliest untrusted layer advances to **P13 Operation / Mutation Model**. Next Primary Owner remains `aegis-modeling`; P13 must define application create/update/withdraw, review approve/reject, migration-authorization issuance/revoke, scoped redemption and final `MigrateLegacyDevice` idempotency/atomicity/error contracts. P17 remains paused.

## FR-018 P13 targeted reconciliation — 2026-09-12

### D-047 — Human-assisted legacy migration separates workflow mutation, approval authority, redemption, and entitlement commit

**State:** Accepted / P13 targeted reconciliation CLOSED

FR-018 operation semantics are frozen as follows:

1. `CreateLegacyMigrationApplication`, contact update, review, reject and withdraw mutate durable workflow truth only; they cannot create entitlement.
2. `ApproveLegacyMigrationApplication` is the only FR-018 workflow decision that may create a `LegacyMigrationAuthorization`; approval and application `approved` state are one logical outcome, but still do not create LicenseGrant/Binding/Credential.
3. Single-device human approval is application/device/product scoped with capacity 1. User-facing migration code/artifact is only a scoped redemption representation of that authority and cannot widen it.
4. `ResolveLegacyMigrationRedemption` and offline request creation are non-authoritative validation/transport steps. Wrong Device/Product/Application fails before capacity consumption.
5. `MigrateLegacyDevice` is the only FR-018 operation that may commit standard entitlement authority. Its logical commit includes Grant/Binding/Credential, migration capacity consumption, `legacy_device_migrated` lifecycle event and application completion linkage.
6. Retry protection exists at both `OperationIdentity` and domain level: application intake dedupes by logical application + Device + Product; migration dedupes by migration authorization + application + Device + Product. A repeated code after successful migration recovers the same outcome rather than minting a second license.
7. Approval/rejection/withdraw/contact-update races are serializable. `decision_contact_revision` fences the exact contact snapshot reviewed by the human.
8. Revoking an unconsumed migration authorization removes permission to migrate; it cannot undo a migration that already committed. Completed license changes require ordinary license lifecycle operations.
9. FR-018 does not change `CredentialPayload` or create any generic reusable license-key semantic.

**Disposition:** P13 FR-018 targeted reconciliation is ACCEPTED/CLOSED. Semantic Foundation is complete again through P13. Because the next boundary crosses Modeling → Architecture, successor selection returns to central `aegis` before any P14 substantive work.

## FR-018 successor routing after P13 — 2026-09-12

### D-048 — Route FR-018 from Modeling to P14 targeted System Architecture reconciliation

**State:** Accepted routing / central `aegis`

P10–P13 are now trustworthy for FR-018. The new capability still introduces system-level ownership that the accepted pre-FR-018 architecture did not freeze: migration-application intake/dedup, device-local outbox sync boundary, human review/approval authority, notification/redemption delivery, security-authority storage, and the transition into canonical `MigrateLegacyDevice` execution.

**Earliest untrusted layer:** `P14 System Architecture`.

**Route:** `P14 targeted reconciliation → P15 targeted module reconciliation → P16 targeted runtime reconciliation`, with each stage executed only if its predecessor remains trusted. P17 stays paused until P16 is reconciled/closed.

**Next Primary Owner:** `aegis-architecture` at **P14 targeted System Architecture reconciliation**. This routing does not itself execute P14 substantive work.

## FR-018 P14 targeted System Architecture reconciliation — 2026-09-12

### D-049 — Legacy migration workflow stays inside AxLicense modular monolith; device outbox belongs to `axlic.exe`

**State:** Accepted / P14 targeted reconciliation CLOSED

1. V1 deployment remains `Product UI/Launcher → on-demand axlic.exe → AxLicense Server modular monolith`; FR-018 does not introduce a public DLL, mandatory device Agent/Service, second licensing service or distributed approval transaction.
2. Product UI owns email entry, provisional watermark/status presentation and opportunistic sync triggers only. It does not own durable migration authority or local license/identity state.
3. `axlic.exe` owns `LogicalApplicationId`, durable local migration outbox, retry/reconciliation, redemption submission bound to current Device context, and final credential verify/install. Device-side SMTP is explicitly rejected.
4. AxLicense Server gains a logical Migration Workflow Authority owning durable `LegacyMigrationApplication`, contact revision/lineage, intake dedupe, review queue and workflow status. It remains workflow truth, not entitlement truth.
5. `ApproveLegacyMigrationApplication` must enter Operation Core and atomically establish application=`approved` plus one application/device/product-scoped `LegacyMigrationAuthorization` within the same logical ACID transaction authority. Browser/Portal cannot directly write either workflow or canonical stores.
6. Notification Dispatcher + external email provider own delivery only; mail outage cannot undo approval or mint another authorization. Resend reuses the same approval context.
7. Optional CRM/Customer-Success integration may consume application/domain/request-count events for installed-base discovery and customer outreach, but its classifications/notes can never automatically authorize migration.
8. Redemption Resolver maps a user-visible code/handle to the exact migration authorization/application/device/product context, rejects wrong-device/product attempts before capacity consumption, and never owns an independent `consumed` truth.
9. Only Operation Core may execute final `MigrateLegacyDevice`; successful commit still produces the existing standard `LicenseGrant + DeviceBinding + SignedLicenseCredential` authority. Device watermark is removed only after `axlic.exe` verifies and installs that credential.
10. Current P13 remains single-application/single-device approval with capacity 1. Review UI may group likely fleet customers, but true bulk/fleet migration authorization is not introduced by architecture and would require upstream product/semantic reconciliation.

**Routing consequence:** P14 FR-018 targeted reconciliation is **ACCEPTED/CLOSED**. Earliest untrusted layer advances to **P15 targeted Module Design reconciliation**, still owned by `aegis-architecture`. P17 remains paused until P15/P16 targeted reconciliation completes.

## FR-018 P14 ownership correction — 2026-09-12

### D-050 — Product/Launcher owns migration application interaction, pending outbox and workflow sync; `axlic.exe` returns to license/security boundary

**State:** Accepted / supersedes conflicting ownership statements in D-049

The P14 ownership allocation for FR-018 is corrected as follows:

1. Product/Launcher owns the complete customer-facing migration application interaction: provisional watermark/status UX, email/contact entry, application-side local pending/outbox state, retry scheduling, connectivity recovery detection, direct workflow submission/reconciliation, and application status presentation.
2. Product/Launcher communicates directly with the AxLicense Server Migration Workflow API for `LegacyMigrationApplication` create/update/retry/status operations. It does **not** later spawn `axlic.exe` merely to synchronize a pending migration application.
3. `axlic.exe` does not own `LegacyMigrationApplication` outbox, customer contact data, reconnect polling, email workflow, support-case UX or CRM/customer-success grouping. It remains the device security/license boundary for DeviceIdentity, local entitlement/status, device-bound redemption, `MigrateLegacyDevice` execution/recovery and SignedLicenseCredential verify/install.
4. Launcher may obtain/use only safe opaque Device context needed to correlate the application. Possession of a Device ID/workflow handle does not authorize migration; final approval scope and device-bound redemption are revalidated through AxLicense authority.
5. AxLicense Server continues to own durable Migration Workflow truth, review/approval authority, notification/redemption orchestration and final handoff to Operation Core. Email/CRM remain non-authoritative adapters.
6. Offline behavior is now product-owned: Launcher persists the unsent application locally and submits it directly when network access returns. Loss of this product-side pending state cannot create/revoke entitlement; server-accepted applications remain durable server workflow truth.
7. P10–P13 semantics are unchanged. This is a P14 subsystem ownership correction only.

**Disposition:** P14 remains ACCEPTED/CLOSED after correction. Earliest untrusted layer remains **P15 targeted Module Design reconciliation**, which must place the migration application/outbox client in Product/Launcher rather than `axlic.exe`.

## FR-018 P15 targeted Module Design reconciliation — 2026-09-12

### D-051 — Product-side application workflow is a separate non-authoritative module family; `axlic.exe` remains narrow

**State:** Accepted / P15 targeted reconciliation CLOSED

1. Product/Launcher owns exactly the FR-018 customer-interaction module family: `LegacyMigrationFlow`, `MigrationApplicationStore`, and `MigrationWorkflowClient`. It owns provisional UX, email/contact input, `LogicalApplicationId`, durable pending/outbox, retry scheduling, direct workflow sync and application-status presentation.
2. Product/Launcher does not own license truth. Application `approved/completed` state cannot remove the watermark; only a current `axlic.exe` license-status result backed by a valid installed credential may transition the product to licensed UX.
3. Product obtains only a minimum-disclosure `LegacyMigrationApplicationContext`/opaque device reference from `axlic.exe`; raw DeviceIdentity, provider references, proofs and credential internals remain hidden. After the application is queued, `axlic.exe` is not called for application retry/sync.
4. AxLicense Server adds stable modules `MigrationWorkflow`, `MigrationReview`, `MigrationApproval`, `NotificationDispatcher`, and `RedemptionResolver` inside the existing modular monolith. Workflow/contact state remains non-authoritative for entitlement.
5. `MigrationApproval` can only create/revoke `LegacyMigrationAuthorization` through `OperationCore + CanonicalUnitOfWork`; application approval state and the scoped migration authorization are one logical approval outcome. Review/grouping/CRM signals cannot auto-approve licenses in V1.
6. Notification is post-commit. Email delivery failure/retry never rolls back approval and never creates a second authorization; notification state is workflow/support metadata only.
7. `RedemptionResolver` resolves the visible code/handle to exact application/device/product/migration-authorization scope, rejects wrong-scope input before capacity consumption, and owns no independent durable `consumed` truth.
8. `axlic.exe` adds no `MigrationApplicationOutbox` or workflow-sync module. Final migration reuses `CommandApplication + IdentityManager + ServerClient + CredentialManager + LocalMutationCoordinator` for current-device binding, canonical `MigrateLegacyDevice`, response recovery and verify-before-install.
9. Product-side outbox loss does not delete a server application; server dedupe by logical application identity remains authoritative after receipt. A completed migration with later credential loss routes to normal recovery, not back to FR-018 application/provisional privilege.

**Routing consequence:** P15 FR-018 targeted reconciliation is **ACCEPTED/CLOSED**. Earliest untrusted layer advances to **P16 targeted Runtime Data Flow reconciliation**, still owned by `aegis-architecture`. P16 must trace Product/Launcher-owned offline outbox and direct workflow sync, support approval/notification failures, final `axlic.exe` redemption, response loss, credential install recovery, and watermark removal from verified local license state. P17 remains paused until P16 reconciliation closes.

## FR-018 P16 targeted Runtime Data Flow reconciliation — 2026-09-12

### D-052 — Product-local workflow durability is not authorization; watermark exit requires verified local AxLicense state

**State:** Accepted / P16 FR-018 targeted reconciliation CLOSED

1. Product/Launcher owns the complete runtime lifecycle for `LegacyMigrationApplication` submission before approval: create/reuse `LogicalApplicationId`, durable pending/outbox persistence, offline retry scheduling, reconnect/startup retry, direct `MigrationWorkflow` sync and application-status presentation. It does not spawn `axlic.exe` merely to synchronize workflow state.
2. Product-local outbox durability is a **non-authoritative persistence point**, not a fourth authorization commit. Losing the outbox before server acceptance loses only the unsent customer request; losing it after server acceptance does not remove server workflow truth.
3. Server dedupes/reconciles duplicate submit/retry by logical application identity. Contact updates after server acceptance are revisioned; stale contact/update/reviewer races fail through the accepted P13 precondition semantics rather than silently overwriting newer contact state.
4. `MigrationReview`/`MigrationApproval` may reject or approve only current workflow state. Reject creates no migration authority. Approve must create the application/device/product-scoped `LegacyMigrationAuthorization` as the accepted logical approval outcome; workflow `approved` alone is never License authority.
5. `NotificationDispatcher` runs after approval commit. Email success/failure, resend and CRM/customer-success grouping remain non-authoritative delivery/support state; failure may only trigger notification retry, never re-approval or duplicate authorization.
6. Final redemption re-enters the `axlic.exe` boundary. `axlic.exe` loads the current established DeviceIdentity context itself; Launcher-stored application/device references are not identity authority.
7. `RedemptionResolver` validates exact migration authorization/application/device/product scope **before** capacity consumption. Wrong-device/product, invalid, expired or revoked redemption fails closed without consuming the original authorization.
8. `MigrateLegacyDevice` remains the sole final canonical migration mutation. Domain-level `MigrationCommitKey` plus operation idempotency ensures commit-before-response-loss, repeated code entry or a new transport session converges on the same Grant/Binding/Credential outcome.
9. If canonical migration commits but credential delivery/install fails, server authority remains committed; retry/recovery re-exposes the same credential. `CredentialManager` verifies before crash-safe local replacement, preserving old committed local state on corruption/failure.
10. Product removes the provisional watermark only after a fresh `axlic.exe status` observes a valid installed license credential bound to the current identity. Application received/approved, email delivered or even server migration commit by itself does not authorize watermark removal.
11. A normal paid activation may independently exit provisional mode once a valid credential is installed; any open migration application becomes workflow cleanup only and must not later create a second grant/binding.

**Disposition:** FR-018 is now reconciled through P16. **P16 overall remains DRAFT / PRODUCT-FLOW REVIEW REQUIRED** because the other §39 product-flow candidates remain unresolved. Earliest untrusted layer at project level therefore remains P16 Product-Flow Review; P17 stays paused.

## P16 Human Product-Flow Review — 2026-09-12

### D-053 — C-01 Device decommission / secure disposal is already covered by existing lifecycle primitives

**State:** Accepted Human Decision / P16 Product-Flow Review

C-01 disposition = **ALREADY COVERED**. AxLicense V1 does not add a new `DecommissionDevice` canonical operation or a new Device retirement authority state solely for appliance disposal. Decommission is an orchestration of existing, separately governed facts:

1. `ADM-03` closes/releases the active `DeviceBinding` when the License remains reusable/rehostable.
2. `ADM-05` revokes the `LicenseGrant` only when an explicit commercial/security policy requires the grant itself to end; device retirement does not imply grant revocation.
3. Product `Device → Organization/Tenant/Account` association removal remains owned by the consuming product backend and is not coupled to AxLicense binding closure.
4. Product/factory secure reset may remove local `SignedLicenseCredential`, product-management credentials and customer/configuration secrets. Ordinary decommission does not by itself require destruction of the stable physical-device `DeviceIdentity`.
5. AxLicense server-side `Device`, `DeviceIdentity`, historical `DeviceBinding`, lifecycle and operation evidence are not deleted merely because the appliance is decommissioned. Retention duration remains a separate retention/compliance policy.
6. Commercial Licensee transfer is intentionally left to candidate C-03. Refurbishment / return-to-unclaimed is intentionally left to C-12.

**Routing consequence:** C-01 creates no new V1 requirement and no upstream reconciliation. P16 remains `PRODUCT-FLOW REVIEW REQUIRED`; next Human Product Review item is **C-02 — RMA Replacement + Product Association / Room / Config**. P17 remains paused.

## P16 Human Product-Flow Review — C-02

### D-054 — RMA product association / room / configuration migration is product orchestration, not new AxLicense authority

**State:** Accepted Human Decision / P16 Product-Flow Review

C-02 disposition = **ALREADY COVERED**. Existing `RMA-01` / Rehost semantics already cover the AxLicense portion of physical replacement: replacement hardware establishes its own `Device` / `DeviceIdentity`; the same `LicenseGrant` is rehosted by closing the old binding, creating the new binding and issuing the successor credential. AxLicense does not own Organization, Room, Device name, product configuration, management credential or cloud-management association.

A consuming product such as NearHub may provide a one-click RMA workflow that orchestrates AxLicense Rehost together with product association migration, room rebinding and configuration restore. These remain independently owned commits and are not required to form a cross-system distributed atomic transaction. If product-side migration fails after AxLicense Rehost has canonically committed, the product workflow retries/reconciles its own state rather than rolling back the license transition.

**Routing consequence:** C-02 does not reopen P02–P15 and does not add a new AxLicense V1 capability. P16 Product-Flow Review remains open for the remaining candidates; P17 remains paused.

## P16 Human Product-Flow Review — C-03

### D-055 — Commercial Licensee transfer is a V1 non-goal

**State:** Accepted Human Decision / P16 Product-Flow Review

C-03 disposition = **REJECT / NON-GOAL (V1)**. Product ownership / Organization transfer remains independent from AxLicense commercial LicenseGrant ownership. V1 does not add a `TransferLicensee` mutation and does not rewrite an existing LicenseGrant's historical Licensee in place merely because the physical device or product association moves from Organization A to Organization B.

If commercial/support policy explicitly authorizes the new customer to receive rights, V1 uses existing lifecycle primitives: close/deactivate or revoke the old grant/binding as appropriate, issue a new LicenseGrant to the new Licensee, and activate that grant on the same physical Device. This is a new commercial authorization, not a transfer of the old grant identity. A future transferable-license product may define a successor-grant transfer model, approval authority, entitlement subset rules and audit semantics, but that is outside V1.

**Routing consequence:** C-03 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-04 — Lost / Stolen / Compromised Device**. P17 remains paused.

## P16 Human Product-Flow Review — C-04

### D-056 — Lost / stolen / compromised device handling is a V1 non-goal

**State:** Accepted Human Decision / P16 Product-Flow Review

C-04 disposition = **REJECT / NON-GOAL (V1)**. Current NearHub deployment is a large-screen / meeting-room appliance; physical theft is not a material V1 product risk sufficient to justify a dedicated AxLicense incident-response capability. V1 therefore does not add a `MarkDeviceCompromised` product flow, remote-kill guarantee, or new incident-response requirement. Existing admin/revoke/rehost primitives remain available for exceptional support cases.

Existing P10/P12 `DeviceState.compromised` capacity may remain as reserved semantic/model capacity; its presence does not make lost/stolen handling a Current Authority flow. The accepted perpetual-offline observability limitation also remains unchanged: V1 does not claim that a permanently offline credential can be remotely invalidated instantly.

**Routing consequence:** C-04 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-06 — Fully Offline Enrollment / Account Recovery**. P17 remains paused.

## P16 Human Product-Flow Review — C-06

### D-057 — Fully offline Product Enrollment / Account Recovery is a V1 non-goal

**State:** Accepted Human Decision / P16 Product-Flow Review

C-06 disposition = **REJECT / NON-GOAL (V1)**. AxLicense V1 continues to require Offline License Activation for air-gapped/perpetual local runtime, but Product Enrollment, Organization claim and account recovery remain backend-authorized management workflows and are not required to operate fully offline. A NearHub may remain registered/licensed while product-side unclaimed/unassociated and continue local operation until Product Backend connectivity is available.

V1 therefore adds no offline Device Assertion relay, phone-mediated QR enrollment exchange, air-gapped account-recovery protocol or static QR ownership token. Device Identity Assertion remains purpose-bound evidence of device possession and must not be weakened into durable human/organization ownership authority. If future field evidence shows repeated demand for a device-offline / phone-online installation path, that capability requires a separately designed disconnected enrollment protocol and fresh upstream product review.

**Routing consequence:** C-06 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-12 — Refurbishment / Return-to-Unclaimed**. P17 remains paused.

## P16 Human Product-Flow Review — C-12

### D-058 — Refurbishment / return-to-unclaimed is product orchestration, not new AxLicense authority

**State:** Accepted Human Decision / P16 Product-Flow Review

C-12 disposition = **ALREADY COVERED**. Demo return, refurbishment or stock recycling may move the consuming-product `ProductDeviceAssociation` back to an unclaimed/unassociated state while preserving the same physical AxLicense `Device` / DeviceIdentity when continuity remains valid. A local factory reset may clear customer/product configuration and local management credentials, but it does not by itself delete server-side product association truth, create a new AxLicense Device, revoke a LicenseGrant or transfer commercial Licensee.

A privileged consuming-product backend may perform the unclaim/refurbish transition and audit it. License deactivate/revoke/reissue remains a separate commercial/admin decision using existing AxLicense lifecycle primitives; C-03 commercial Licensee policy remains unchanged.

**Routing consequence:** C-12 adds no new AxLicense V1 capability and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-14 — Activation Material Recovery / Reissue**. P17 remains paused.

## P16 Human Product-Flow Review — C-14

### D-059 — Activation-material recovery/reissue is already covered by existing authority separation

**State:** Accepted Human Decision / P16 Product-Flow Review

C-14 disposition = **ALREADY COVERED**. V1 treats License Key / activation code / activation material as a non-canonical validation token or activation-authority reference, not as the purchased commercial right itself. The durable commercial authority remains the canonical `LicenseGrant`.

If an unbound customer loses activation material, authorized support/admin may re-deliver or regenerate activation material after verifying the commercial record / Licensee. This must not create a second LicenseGrant, change entitlement or Licensee identity, or bypass one-active-binding / DeviceIdentity validation. Once a LicenseGrant is already bound, loss/corruption of the installed signed credential is handled through the existing credential/device recovery lifecycle rather than activation-material recovery.

Customer self-service portal UX is not a V1 AxLicense core requirement; support/admin-assisted recovery is sufficient for V1.

**Routing consequence:** C-14 adds no new AxLicense V1 capability and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-05 — Organization deletion / merge / admin offboarding**. P17 remains paused.

## P16 Human Product-Flow Review — C-05

### D-060 — Organization lifecycle / administrator offboarding is already covered by product ownership boundaries

**State:** Accepted Human Decision / P16 Product-Flow Review

C-05 disposition = **ALREADY COVERED**. Organization/Tenant lifecycle, administrator identity, roles/sessions and `ProductDeviceAssociation` are consuming-product/backend authority rather than AxLicense canonical license authority. Administrator offboarding changes product IAM only. Organization merge or deletion must first resolve affected device associations through product-side transfer or return-to-unclaimed; destructive tenant deletion must not leave orphaned active associations.

AxLicense `Device` / DeviceIdentity are unchanged by these product-management transitions. `LicenseGrant` is not implicitly revoked, rebound or commercially transferred because a product Organization is merged/deleted or an administrator leaves. Existing OWN-04 association transfer plus C-12 return-to-unclaimed provide the required device disposition semantics without new AxLicense operations.

**Routing consequence:** C-05 adds no new AxLicense V1 capability and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-07 — Customer Self-Service Rehost / Device Replacement**. P17 remains paused.

## P16 Human Product-Flow Review — C-07

### D-061 — Customer self-service rehost is a V1 non-goal

**State:** Accepted Human Decision / P16 Product-Flow Review

C-07 disposition = **REJECT / NON-GOAL (V1)**. Existing `RehostDevice` semantics remain the authoritative replacement-device lifecycle, but V1 keeps rehost authority restricted to internal admin/support. Customer authentication or Organization membership is not by itself sufficient authority to move a commercial `LicenseGrant` between physical devices.

V1 therefore adds no customer-facing self-service rehost portal, customer-to-LicenseGrant rehost authorization mapping, or direct customer invocation of privileged rehost. A future self-service capability may reuse the existing `RehostDevice` mutation only behind a separately designed product/backend authorization layer with strong customer/Organization authorization, anti-abuse controls, audit and support/dispute recovery.

**Routing consequence:** C-07 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-08 — Trial / Evaluation / Time-limited Offline License**. P17 remains paused.

## P16 Human Product-Flow Review — C-08

### D-062 — High-assurance fully offline trial enforcement is deferred

**State:** Accepted Human Decision / P16 Product-Flow Review

C-08 disposition = **DEFER**. V1 may represent and issue ordinary trial/evaluation rights using existing bounded validity semantics, and those rights may be evaluated online or with best-effort local wall-clock behavior. V1 does not guarantee rollback-resistant fully offline trial enforcement when the user controls device time/storage.

Secure monotonic time, TPM/TEE-backed counters, trusted RTC/time sources, reinstall/clone-resistant trial continuity and equivalent high-assurance offline anti-rollback mechanisms remain deferred until a concrete product/business requirement justifies that security scope. This decision does not weaken or block perpetual offline runtime licensing and does not remove ordinary bounded entitlement support.

**Routing consequence:** C-08 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-09 — Subscription / Renewal Grace UX**. P17 remains paused.

## P16 Human Product-Flow Review — C-09

### D-063 — Subscription / renewal grace UX is already covered by right separation and product-owned commercial policy

**State:** Accepted Human Decision / P16 Product-Flow Review

C-09 disposition = **ALREADY COVERED**. AxLicense V1 already separates `runtime`, `maintenance_update` and `cloud_service` rights and supports bounded validity plus grant revision / successor credential issuance. Therefore expiry and an approved renewal can be represented without adding a new canonical subscription lifecycle, and perpetual runtime must not be implicitly removed because maintenance/cloud rights expire.

Renewal reminders, payment status, auto-renew, dunning, checkout and the business decision to grant a grace period remain consuming-product/commercial-backend responsibilities. When grace is approved, AxLicense only receives the effective authorized validity for the relevant right; it does not model `past_due`, `grace`, `delinquent` or similar billing states as license authority.

**Routing consequence:** C-09 adds no new AxLicense V1 capability and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-10 — Bulk Organization Enrollment / Fleet Claim**. P17 remains paused.

## P16 Human Product-Flow Review — C-10

### D-064 — Bulk Organization Enrollment / Fleet Claim is deferred pending a concrete enterprise deployment requirement

**State:** Accepted Human Decision / P16 Product-Flow Review

C-10 disposition = **DEFER**. V1 retains the existing per-device product enrollment / claim semantics: each physical Device proves possession through a purpose-bound Device Identity Assertion, and the consuming Product Backend independently authenticates and authorizes the human/Organization actor before committing `ProductDeviceAssociation`.

A future fleet-claim workflow may remove repetitive per-device human interaction, but it must not remove per-device identity verification. It must introduce an explicit Product Backend authority for batch membership and target Organization, with bounded scope such as validity, capacity, allowed Device set/batch source, partial-success behavior, retry/replay and audit. Serial-number lists, procurement CSVs or opaque batch IDs must not by themselves become ownership authority.

The fleet orchestration belongs to the consuming product / device-management backend, not AxLicense commercial `LicenseGrant` authority.

**Routing consequence:** C-10 creates no new V1 requirement and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-11 — Dealer / Reseller / Install-Partner Enrollment Authority**. P17 remains paused.

## P16 Human Product-Flow Review — C-11

### D-065 — Dealer / reseller / install-partner enrollment authority is a V1 non-goal

**State:** Accepted Human Decision / P16 Product-Flow Review

C-11 disposition = **REJECT / NON-GOAL (V1)**. V1 may let an installer or channel partner deploy, configure, register and activate a physical device within the permissions already defined for those workflows, while the product-side ownership association remains unclaimed until an authorized customer Organization actor completes claim. V1 does not introduce Dealer/Reseller/Installer principals, reseller hierarchy, delegated partner-to-customer Organization claim authority, partner-owned enrollment capacity or channel-management IAM.

Any future delegated partner enrollment must be designed as a separate consuming-product / management-backend authorization capability with explicit delegation scope, target Organization, device/batch scope, expiry, revocation and audit. Such product-side delegation must never silently grant commercial `LicenseGrant` authority: it cannot by itself transfer Licensee, rehost a grant, revoke a commercial license or otherwise cross the C-03 commercial-authority boundary.

**Routing consequence:** C-11 adds no new V1 capability and does not reopen P02–P15. P16 Product-Flow Review remains open; next item is **C-13 — AxLicense Server Migration Across Environments / Regions**. P17 remains paused.

## P16 Human Product-Flow Review — C-13

### D-066 — Planned AxLicense server/environment migration is already covered as logical-authority continuity

**State:** Accepted Human Decision / P16 Product-Flow Review

C-13 disposition = **ALREADY COVERED**. V1 treats planned region/cloud/account/database/KMS infrastructure replacement as relocation of the same logical AxLicense authority rather than creation of a new authority. Existing P14/P16 invariants already require preservation of canonical Device/DeviceIdentity, LicenseGrant/DeviceBinding, issued credential/result-recovery evidence, operation/idempotency state, lifecycle/audit evidence and trust/key metadata; database recovery and signing-authority recovery remain separate failure domains.

A migration must fence the old canonical writer before the destination becomes authoritative and must not resurrect stale state or create split-brain issuance. Concrete backup/replication, KMS/HSM migration, endpoint/service-discovery cutover, RPO/RTO and rollback runbooks remain P17/P18/P20 realization/evidence work. Active-active multi-region canonical writes, partition-autonomous issuance, customer-hosted sovereign AxLicense and independent regional signing authorities are not implied by V1 and would require a new explicit requirement.

**Routing consequence:** C-13 adds no new V1 product capability and does not reopen P02–P15. However, P16 is **not yet ready for final closure** because the Human Review inventory has now been found to omit at least one explicit end-to-end V1 product journey review: factory/new-device code-entry activation. FR-018 legacy provisional watermark/email flow is already Current Authority and was traversed in targeted P16 reconciliation, but must be included in the final primary-flow coverage checklist so it is visible alongside candidate-gap dispositions. P17 remains paused.

## P16 Primary Product Journey Audit — PF-01 / PF-02

### D-067 — PF-01 verified; PF-02 exposes unresolved NearHub pairing→license product policy

**State:** BLOCKED_UNRESOLVED_DECISION / P16 Primary Journey Audit

PF-01 Legacy Sold-Device Provisional Migration is confirmed covered by the accepted FR-018 `MIG-04 → MIG-05 → MIG-06` runtime reconciliation; no new upstream requirement is introduced.

PF-02 New NearHub first deployment is not yet fully frozen as a product journey. Existing authority provides DeviceIdentity establishment/registration, Product Organization claim, LicenseGrant issuance and online/offline activation as separate primitives, but it does not currently authorize the proposed default UX in which the Device displays a short pairing code, an authenticated Organization admin enters that code in management SaaS, the Product Backend commits Device→Organization association, and that same orchestration allocates/binds the commercial license to the Device.

A static DeviceId/serial must not become a claim secret, ownership proof or License. The recommended future-facing primitive is a short-lived one-time purpose-bound PairingCode issued only after current DeviceIdentity possession is proven; the code is a session locator and carries no entitlement by itself. Product association and AxLicense license activation remain separate durable commits, with idempotent convergence rather than a cross-system distributed transaction.

Calling this default journey "offline activation" is rejected as terminology: if the Device participates with the server during pairing/activation, it is online pairing/activation followed by offline-capable runtime. True air-gap activation remains ACT-02 and requires two-way request/response transport.

**Routing consequence:** P16 final closure is blocked. If the proposed PF-02 product behavior is accepted, the earliest untrusted layer is P02 Product Requirement, followed by targeted P03 traceability and downstream semantic/runtime reconciliation before P16 resumes. P17 remains paused.

## NearHub Licensing Product-Policy Reconciliation — 2026-09-12

### D-068 — Software distribution is not the license boundary; sold and new NearHub devices share one first-run lifecycle

**State:** Accepted Product Requirement / P02–P03 targeted reconciliation

NearHub V1 accepts uncontrolled/re-distributed software as a valid acquisition/promotional channel rather than attempting to protect entitlement through installer distribution. Installing or copying the software grants no LicenseGrant. On first AxLicense-aware execution, each physical device establishes its own protected local private identity material; DeviceIdentity establishment remains separate from customer Organization ownership and commercial license authority.

Already-sold field devices and newly shipped/factory devices therefore use the same default product journey: local identity establishment → SaaS enrollment/setup session → authenticated Organization claim → backend commercial license acquisition/assignment → ordinary AxLicense activation → signed credential verification/install. A device may be managed/claimed while still unlicensed. Setup/enrollment code is only a short-lived session handle; neither static DeviceId/serial nor setup code is ownership or license authority.

Historical/customer accommodation is moved to backend commercial policy. If an already-sold device should receive grandfathered/included entitlement, the Product Backend/support system decides this from trusted sales/customer/account evidence and then uses ordinary license issuance/assignment. NearHub no longer needs a distinct client-side legacy-provisional migration path merely because the software was previously sold or freely distributed.

For NearHub V1, dedicated `FR-013 Legacy Device Migration` and `FR-018 Legacy Provisional Activation & Human-Assisted Migration` are superseded as mandatory product journeys by FR-019. Legacy candidate classification, watermark-as-migration-control, migration email/outbox, human migration approval and migration redemption are no longer required unless another accepted consumer independently needs them. True air-gap licensing remains the standard ACT-02 offline request/response exception.

**Routing consequence:** earliest untrusted downstream layer = **P10 Product Object Model**. P10–P16 require targeted reconciliation to remove/defer legacy-specific authority where no longer justified and to model the unified SaaS-enrollment/backend-license journey. P16 closure and P17 remain paused until that reconciliation returns.

## FR-019 downstream reconciliation

### D-069 — P10 unified licensing object model reconciliation

**State:** ACCEPTED / P10 Current Authority / 2026-09-12

FR-019 changes NearHub's product entry path but does not require a new AxLicense ownership, IAM, billing or enrollment aggregate. Current P10 authority is reconciled as follows:

- `Device` / `DeviceIdentity`, `LicenseGrant` / `EntitlementGrant`, `DeviceBinding` and `SignedLicenseCredential` remain the canonical AxLicense product world.
- SaaS `Account` / `Organization` / `Room` / `ProductDeviceAssociation` remain external product-owned durable truth. `EnrollmentSession` / `SetupCode` remain product-owned transient state. DeviceIdentity proof is evidence, not ownership authority.
- For NearHub V1, legacy-specific `LegacyMigrationApplication`, `LegacyMigrationAuthorization`, `LegacyMigrationSession`, provisional migration state, email/outbox/review/redemption flow and remaining-migration-capacity projections are superseded as Current Authority. Historical customer accommodation becomes backend commercial policy that produces ordinary Grant/Entitlement authority.
- `ProvisioningAuthorization` and Factory Pre-Activation remain only as optional specialized factory capabilities; they are not the default shipment/onboarding path.
- Cross-product entitlement is first-class: one grant may carry stable entitlement identities across NearHub, NearCast, Launcher-hosted capabilities, Axiom/Arc and future product namespaces; SKU/package labels remain non-authoritative presentation.
- V1 entitlement authorization remains presence-based. P10 adds only a narrow `EntitlementConstraint` value concept for future bounded quantity/concurrency semantics; constraints may narrow an existing grant but never create one, and no generic JSON/expression policy engine is authorized.

**Preserved invariants:** per-physical-device identity/binding, Grant/Binding/Credential separation, offline-verifiable immutable credential, right-kind separation, rehost/recovery semantics, no silent identity downgrade, true air-gap activation, and device-observation-after-canonical-commit remain current.

**Routing:** P10 is now trusted for FR-019. Earliest untrusted downstream layer = **P11 Interaction / Behavior targeted reconciliation**. P17 remains paused; P11→P12→P13→P14→P15→P16 must reconcile before architecture/runtime closure can resume.

## FR-019 downstream reconciliation — P11

### D-070 — P11 unified enrollment → backend licensing behavior reconciliation

**State:** ACCEPTED / P11 Current Authority / 2026-09-12

FR-019 downstream behavior is reconciled around independent commits/observations rather than one monolithic setup transaction:

- first-run local DeviceIdentity establishment/registration does not grant entitlement;
- product-owned EnrollmentSession + DeviceIdentity proof + authenticated Organization authorization commits ProductDeviceAssociation only;
- `claimed + unlicensed` is a normal durable product condition and must not be rolled back when purchase/license assignment fails;
- purchase/trial/redeem/pool assignment/included or grandfathered entitlement are different commercial sources that must converge into explicit backend authority before ordinary AxLicense issuance/activation;
- ProductDeviceAssociation and AxLicense authorization are separate commits with idempotent convergence, not a cross-system distributed transaction;
- AxLicense canonical license commit and device-local credential observation are separate facts. `license committed + device observation pending` is a supported recovery state;
- local product features are enabled only after valid SignedLicenseCredential/entitlement is verified locally, never directly from SaaS claim/purchase flags;
- entitlement/constraint changes on an existing bound grant use authority revision + successor credential and do not automatically create a new DeviceBinding/rehost;
- SetupCode/QR is a transient session locator, never ownership or License authority;
- ownership transfer/unclaim does not silently move/revoke/rehost license authority;
- true air-gap Offline Activation and specialized Factory Pre-Activation remain explicit exception paths;
- FR-018 legacy provisional/email/review/redemption behavior is historical/superseded for NearHub V1 and must not drive new downstream schema/design.

**Routing consequence:** P10 and P11 are trusted under FR-019. Earliest untrusted downstream layer = **P12 Semantic Schema targeted reconciliation**. P12 must reconcile composite projections vs canonical authority, backend-driven activation authority representation, closed `EntitlementConstraint` semantics, successor credential semantics for entitlement changes, and legacy-schema supersession. P16 remains open and P17 remains paused until downstream reconciliation returns.

### D-071 — P11 Catalog Evolution / Dynamic Entitlement Registration Addendum

**State:** ACCEPTED / P11 Current Authority Addendum / 2026-09-12

AxLicense 必须允许受信 control plane 在产品生命周期中后续注册新的 `ProductDefinition` / `EntitlementDefinition`，使 NearCast、NearHub、Launcher、Axiom 等产品可以在不同发布时间新增 capability，而无需把 entitlement 列表写死在 AxLicense 核心代码中。

行为边界冻结为：

- registering an entitlement definition only changes catalog truth; it does not grant that entitlement to any existing `LicenseGrant` or device；
- commercial packaging / SKU mapping is separate from catalog registration；
- adding/removing/changing a granted entitlement or bounded constraint is an explicit `LicenseGrant` authority mutation and increments `authority_revision`；
- if the affected grant has an Active `DeviceBinding`, the changed authority must converge to a successor `SignedLicenseCredential` without creating a new binding merely because features changed；
- device credential refresh is delivery/observation, not a second activation or license consumption, and must be idempotent + verify-before-replace；
- future/unknown entitlement IDs must not invalidate unrelated known rights, while clients may never infer authorization from an entitlement they do not safely understand；
- dynamic catalog evolution must not convert perpetual runtime licensing into mandatory short-TTL online leasing. Offline devices continue using their current valid credential and observe additions/downgrades only at the next online or offline credential-refresh point；
- an already-published entitlement ID cannot be reused with incompatible authorization-critical meaning; incompatible semantics require a new ID.

**Routing:** P11 remains trusted. Earliest untrusted downstream layer remains **P12 Semantic Schema targeted reconciliation**, which must now freeze Entitlement Registry / catalog revision semantics, closed `EntitlementConstraint` representation, grant + credential snapshot compatibility, and refresh/unknown-entitlement rules before P13 resumes.

### D-072 — P12 Dynamic Entitlement Registry / Revision-Separation Schema Reconciliation

**State:** ACCEPTED / P12 Current Authority / 2026-09-12

P12 已完成 FR-019 + P11 Catalog Evolution Addendum 的 semantic-schema reconciliation：

- `ProductDefinition` / `EntitlementDefinition` 作为可后续注册的 stable catalog identity，不再要求 AxLicense core code 预埋全部未来 capability；
- catalog registration 只改变 registry truth，不自动修改任何 customer `LicenseGrant`；
- `CatalogRevision`、`LicenseGrant.authority_revision`、`credential_generation` 三个 revision domain 完全分离，禁止互相比较或隐式同步；
- `EntitlementDefinition` 冻结 `grant_semantics = presence | bounded_u64`，bounded 使用 closed `max_u64` constraint；禁止 arbitrary JSON/expression policy engine；
- `EntitlementGrant` presence 仍是 grant，bounded constraint 只能收窄 capability extent，`value=0` 不得成为第二个 disabled truth；
- `SignedLicenseCredential` snapshot 增加 `grant_semantics + applicable constraint`，继续保持 self-contained offline evaluation；`CatalogRevision` 不需要进入 CredentialPayload；
- 同一 Grant/Binding/authority_revision 的不同 credential generations 必须 authorization-equivalent；customer rights 改变必须先推进 authority_revision；
- unknown future entitlement ID 本身不使 credential invalid；旧 consumer 只评估明确理解的 exact IDs。未知 authorization-critical grant/constraint semantics 至少对受影响 right fail closed，若无法安全隔离则整份 credential fail closed；
- local refresh 明确防 rollback：低 authority_revision / generation 的 stale candidate 不得替换当前 credential；higher authority_revision 可以表达合法 upgrade/downgrade；verify-before-replace 保持；
- SaaS `ProductDeviceAssociation`、setup/enrollment、purchase/order state 继续留在 Product Backend；`claimed/unlicensed`、`server licensed/device pending` 是 cross-system derived projection，不新增 AxLicense canonical enum；
- FR-013/FR-018 `LegacyMigrationAuthorization` / `LegacyMigrationApplication` / provisional/outbox/redemption schema 对 NearHub V1 正式 superseded，仅可作为 historical compatibility/audit data。

**Routing:** P10/P11/P12 现均 trusted under FR-019。Earliest untrusted downstream layer = **P13 Operation / Mutation Model targeted reconciliation**。P13 必须定义 catalog register/deprecate/retire、Grant entitlement add/remove/change-constraint、successor credential issuance/refresh、revision precondition/conflict、idempotency/dedup/retry；不得重新定义 P12 schema semantics。P14-P16 后续仍需 targeted reconciliation，P17 继续 paused。

### D-073 — P13 FR-019 Dynamic Catalog / Grant Reconciliation / Credential Refresh Mutation Closure

**State:** ACCEPTED / P13 Current Authority / 2026-09-12

P13 已完成 FR-019 targeted reconciliation，并冻结以下 mutation semantics：

- dynamic catalog 由 `RegisterProductDefinition` / `RegisterEntitlementDefinition` / `ReviseCatalogEntry` 管理；real catalog mutation 推进 `CatalogRevision`，但 catalog registration 永不自动修改 customer Grant；
- stable ProductId/EntitlementId 不允许以相同 ID 重定义 authorization-critical semantics；same ID + same immutable semantics 的重复注册可 idempotent 收敛；
- `ReviseLicenseGrant` 是 V1 唯一 canonical entitlement-rights mutation primitive，使用完整 replacement set + `expected_authority_revision` CAS；上层 add/remove/set-limit 仅是 convenience command，不形成第二套 domain mutation truth；
- bound Grant 的 entitlement/validity/constraint authority 改变时，Grant revision 与对应 successor SignedLicenseCredential 必须形成一个 recoverable logical commit；不得存在 new Grant authority 已 commit 但 active binding 无对应可恢复 credential 的合法终态；
- retired entitlement 不得新增到此前不含它的 Grant；existing retired entitlement 可保持/收窄/移除，但不得扩张为等价新 assignment；
- credential 行为正式拆分为 `ReissueCredential` 与 `ResolveCurrentCredential`：前者才会在相同 authority revision 下推进 `credential_generation`，后者只是 non-mutating delivery/read；因此 product-facing `axlic refresh` 不会因轮询而制造 credential churn；
- `InstallSignedCredential` 必须 verify-before-replace，并按 authority_revision / credential_generation 执行 anti-rollback；higher authority revision 可以合法表示 upgrade 或 downgrade；
- Product SaaS enrollment、SetupCode、Organization claim、ProductDeviceAssociation、order/payment/SKU/subscription 仍不属于 AxLicense canonical operation plane；受信 commercial decision 最终收敛到普通 Issue/Revise/Activate operations；
- bulk entitlement rollout 只是 coordinator，每个 LicenseGrant 独立 OperationIdentity / expected revision / commit；partial success 不做跨 Grant rollback；
- FR-018 legacy application/review/redemption/`MigrateLegacyDevice` operations 对 NearHub V1 为 historical/superseded，不能再作为新 downstream architecture 的 Current Authority；
- authorization/catalog history 不提供 generic undo/redo；纠错必须通过显式 successor mutation。

**Semantic Foundation status:** P10 Product Object Model、P11 Interaction/Behavior、P12 Semantic Schema、P13 Operation/Mutation 在 FR-019 下均为 trusted Current Authority。

**Routing consequence:** Modeling family 已完成。跨越 Modeling → Architecture family 必须返回中央 `aegis` 做 successor routing。若中央路由确认 architecture 为 earliest untrusted layer，则下一 Primary stage 为 **P14 targeted System Architecture reconciliation**；当前 decision 不自动执行 P14 substantive work。

## Central Aegis successor routing — FR-019 Semantic Foundation → Architecture

### D-074 — Earliest Untrusted Layer = P14 System Architecture targeted reconciliation

**State:** ACCEPTED ROUTING / READY_TO_ROUTE / 2026-09-12

Central `aegis` re-evaluated the project after P10/P11/P12/P13 FR-019 reconciliation.

**Trusted upstream Current Authority:**

- P02 Product Requirements — FR-019 reconciled;
- P03 Capability Traceability — FR-019 reconciled;
- P10 Product Object Model — trusted;
- P11 Interaction / Behavior + Catalog Evolution Addendum — trusted;
- P12 Semantic Schema — trusted;
- P13 Operation / Mutation Model — trusted / closed.

**Earliest Untrusted Layer:** **P14 System Architecture**.

Reason: the existing P14 authority still assigns architecture ownership for FR-018 legacy provisional/migration workflow, includes `LegacyMigrationAuthorization` / `MigrateLegacyDevice` as active architecture inputs, and does not yet allocate ownership for the FR-019 dynamic Entitlement Registry, CatalogRevision, Grant reconciliation, `ResolveCurrentCredential` vs `ReissueCredential`, Product Backend commercial-decision boundary, or the new unified enrollment → ordinary activation model. These are architecture-ownership changes, not semantic-model defects, so routing does not reopen P02–P13.

**Required route:**

`P14 targeted System Architecture reconciliation → P15 targeted Module Design reconciliation → P16 targeted Runtime Data Flow reconciliation`

P17 remains paused until P14–P16 converge under FR-019. P20 Verification Design must later be reconciled against the resulting architecture/runtime-flow authority before implementation planning resumes.

**P14 reconciliation scope:**

- remove FR-018 legacy workflow components from NearHub V1 Current Architecture and isolate any historical compatibility path;
- allocate server ownership for dynamic Product/Entitlement Registry + CatalogRevision without making catalog state a license authority;
- allocate Product Backend / SaaS ownership for enrollment, Organization association and commercial eligibility/packaging, while keeping AxLicense canonical authority separate;
- allocate `ReviseLicenseGrant` / successor credential issuance ownership and transaction boundary;
- separate non-mutating `ResolveCurrentCredential` delivery from canonical `ReissueCredential` issuance;
- preserve modular-monolith canonical ACID boundary, external Signing Authority, CLI-only Windows V1 device boundary, provider-pinned identity, offline runtime independence, one-active-binding, and credential immutability;
- define cross-system failure boundaries for claimed-but-unlicensed and server-licensed/device-pending without introducing distributed transaction semantics.

**Not authorized by this routing:** substantive P14 design work, P15/P16 edits, P17 execution, verification redesign, or implementation work.

**Next Primary Owner:** `aegis-architecture` at **P14 targeted System Architecture reconciliation**.

## P14 architecture safety preflight — FR-019 first-run identity registration gap

### D-075 — P14 blocked by missing ordinary first-run Device registration mutation

**State:** BLOCKED_AUTHORITY / ROUTE TO P13 TARGETED REPAIR / 2026-09-12

`aegis-architecture` started the authorized P14 FR-019 targeted System Architecture reconciliation and found an upstream operation-contract omission before assigning new architecture ownership.

**Trusted authority that exposes the gap:**

- P11 FR-019 §13.3 requires ordinary first-run DeviceIdentity establishment to persist one local current identity and then complete server registration that creates/confirms `Device + current DeviceIdentity` without creating `LicenseGrant`, `DeviceBinding` or `SignedLicenseCredential`.
- P11 FR-019 enrollment subsequently depends on a purpose-bound DeviceIdentity assertion / device proof before `ProductDeviceAssociation` commit.

**Current P13 mismatch:**

- the only identity-only canonical mutation is `ProvisionDeviceIdentity`;
- its payload requires `provisioning_auth_id` and its authorization source is `ProvisioningAuthorization(identity_provision)`;
- P13 requirement trace positions `ProvisionDeviceIdentity` as the Factory identity-provisioning operation;
- no Current P13 operation currently represents the ordinary non-factory first-run server registration required by P11 FR-019.

**Why P14 cannot repair this:** choosing whether ordinary first-run uses a new `RegisterDeviceIdentity` operation, generalizes `ProvisionDeviceIdentity`, or requires a new authorization semantic changes P13 mutation vocabulary / authority / preconditions. P14 may assign ownership only after that semantic contract exists; it must not silently turn factory `ProvisioningAuthorization` into a universal device-registration authority.

**Earliest Untrusted Layer:** **P13 Operation / Mutation Model — targeted first-run identity-registration repair.** P02/P03/P10/P11/P12 remain trusted; no broader semantic reopening is required.

**Required P13 repair outcome:** freeze one explicit ordinary first-run identity-registration mutation contract that:

1. creates/confirms only `Device + current DeviceIdentity` and never commercial entitlement;
2. does not require factory `ProvisioningAuthorization` unless an upstream requirement explicitly chooses that policy;
3. is idempotent for the same established identity and fail-closed on cross-device identity conflict;
4. preserves the already-established local key/material across network/server retry;
5. emits the appropriate canonical lifecycle/audit result;
6. provides the registered Device/current identity context required by `VerifyDeviceIdentityAssertion` before Product Backend Organization claim;
7. keeps `ProvisionDeviceIdentity` as a factory specialization if that remains the cleanest operation vocabulary.

**Routing:** stop P14 substantive reconciliation now. Next Primary Owner = `aegis-modeling` at **P13 targeted reconciliation**. After that targeted repair is accepted, return to central `aegis` successor routing; expected route is back to **P14 FR-019 targeted System Architecture reconciliation**. No P14 Current Authority was modified by this blocked attempt.

### D-076 — P13 ordinary first-run Device registration mutation repair

**State:** ACCEPTED / P13 Current Authority Repair / 2026-09-12

P14 architecture safety preflight exposed one upstream mutation-contract omission: P11 FR-019 requires ordinary first-run local identity establishment followed by server-side `Device + current DeviceIdentity` registration before SaaS enrollment/license activation, while P13 previously exposed only factory-authorized `ProvisionDeviceIdentity` for identity-only canonical creation.

P13 is repaired with a distinct canonical `RegisterDeviceIdentity` operation for ordinary connected first-run:

- authorization source = accepted ordinary self-registration policy + valid proof of possession/control of the claimed local identity; it does **not** require `ProvisioningAuthorization`, LicenseGrant, purchase/order evidence or ProductDeviceAssociation;
- commit = create/confirm one stable `Device` + current `DeviceIdentity` + identity lifecycle event + recoverable idempotency result;
- forbidden mutation = no Licensee, LicenseGrant/EntitlementGrant, DeviceBinding, SignedLicenseCredential, Organization/ProductDeviceAssociation, provisioning capacity or factory-pre-activation capacity;
- same operation retry recovers the same result; a fresh correlation presenting the same valid identity resolves the same `device_id`; response loss must not create a second Device;
- same current identity mapped to another non-retired Device fails closed as `DEVICE_IDENTITY_CONFLICT`;
- ordinary registration and factory `ProvisionDeviceIdentity` share the same DeviceIdentity uniqueness fence but remain separate operation/authority classes;
- `product_id` is policy context only and is not part of Device uniqueness or product ownership authority;
- stable failures now include `DEVICE_IDENTITY_SCHEME_NOT_ALLOWED` and `DEVICE_IDENTITY_PROOF_INVALID` in addition to existing conflict/idempotency classes.

**Authority impact:** P10/P11/P12 remain unchanged and trusted. D-075's missing-operation blocker is resolved. P13 is again trusted under FR-019.

**Handoff:** return to central `aegis` for successor routing before resuming P14 substantive reconciliation. Expected candidate remains **P14 FR-019 targeted System Architecture reconciliation**, but D-076 does not itself execute or authorize P14.

## Central Aegis successor routing — D-075 closure → P14 reauthorization

### D-077 — D-075 blocker closed; P14 FR-019 targeted System Architecture reconciliation reauthorized

**State:** ACCEPTED ROUTING / READY_TO_ROUTE / 2026-09-12

Central `aegis` re-evaluated the project after D-076 repaired P13 with ordinary first-run `RegisterDeviceIdentity`.

**Blocker closure check:**

- D-075 blocked P14 only because P11 FR-019 required ordinary first-run server registration of `Device + current DeviceIdentity`, while P13 previously exposed only factory-authorized `ProvisionDeviceIdentity` for identity-only creation.
- D-076 now defines canonical `RegisterDeviceIdentity` for ordinary connected first-run, with accepted self-registration policy + proof of possession, no `ProvisioningAuthorization`, no LicenseGrant/DeviceBinding/Credential mutation, stable retry/replay, same-identity uniqueness, and explicit fail-closed errors.
- Therefore the missing-operation failure mode that caused D-075 no longer exists. D-075 is **resolved/superseded as an active blocker**; it remains historical routing evidence only.

**Trusted upstream Current Authority:** P02 Product Requirements, P03 Capability Traceability, P10 Product Object Model, P11 Interaction/Behavior including catalog-evolution addendum, P12 Semantic Schema, and P13 Operation/Mutation including D-076 repair.

**Earliest Untrusted Layer:** **P14 System Architecture**. Current P14 remains FR-018-oriented and stale against FR-019: it still assigns legacy migration workflow/authorization as active architecture and does not yet map ordinary `RegisterDeviceIdentity`, Product Backend enrollment/commercial-decision boundary, Dynamic Entitlement Registry/CatalogRevision, `ReviseLicenseGrant`, `ResolveCurrentCredential` vs `ReissueCredential`, or FR-019 legacy supersession.

**Authorized next work:** `aegis-architecture → P14 FR-019 targeted System Architecture reconciliation`.

**Required P14 scope:**

- map ordinary first-run identity establishment + `RegisterDeviceIdentity` to device/server ownership and trust boundaries;
- freeze Product Backend ownership of Organization/EnrollmentSession/SetupCode/ProductDeviceAssociation/commercial eligibility without importing those as AxLicense canonical state;
- map backend commercial decisions into explicit AxLicense Grant/Binding/Credential operations without distributed transaction semantics;
- assign Dynamic Entitlement Registry/CatalogRevision ownership and separate it from Grant authority;
- map `ResolveCurrentCredential` as non-mutating delivery/observation and `ReissueCredential` as true issuance mutation;
- preserve CLI-only Windows V1, modular-monolith + single logical ACID canonical boundary, external Signing Authority, provider-pinned identity/no silent downgrade, perpetual offline runtime, one-active-binding and credential immutability;
- move FR-018 legacy migration architecture to historical/compatibility-only boundaries for NearHub V1.

**Not authorized by this routing:** P15/P16 substantive edits, P17 execution, verification redesign, implementation planning or coding.

**Root-index note:** the AxLicense root page still contains FR-018/P16-era lifecycle text; this is stale navigation metadata, not competing Current Authority. Its active route is superseded by D-077 and should be read as P14 FR-019 until the index is synchronized.

## P14 FR-019 System Architecture closure

### D-078 — P14 FR-019 unified enrollment / dynamic entitlement / credential observation architecture accepted

**State:** ACCEPTED / P14 CURRENT AUTHORITY / 2026-09-12

`aegis-architecture` completed the D-077-authorized P14 targeted reconciliation against trusted FR-019 P10–P13 authority, including D-076 ordinary first-run `RegisterDeviceIdentity`.

**Frozen architecture ownership:**

- ordinary first-run device registration is device-initiated through `axlic.exe` into AxLicense Device Registry / Identity Validation; local identity material is persisted before network registration, provider remains pinned, and retry resolves the same canonical Device;
- Product Backend owns Organization/IAM, EnrollmentSession/SetupCode, ProductDeviceAssociation, SKU/package mapping, purchase/redeem/pool/included/grandfathered commercial eligibility and associated workflow retry/support state;
- Product Backend and AxLicense are separate transaction domains. Association commit and license commit are never modeled as a distributed transaction; `claimed + unlicensed` and `server licensed + local observation pending` are valid recoverable partial states;
- AxLicense Server remains a V1 modular monolith with one logical canonical ACID persistence boundary plus external Signing Authority;
- Product & Entitlement Catalog Registry owns ProductDefinition/EntitlementDefinition/CatalogRevision; authorized release/admin tooling can register new capability definitions without modifying customer Grants, while SKU/package policy remains Product Backend-owned;
- LicenseGrant/Binding Authority owns Grant entitlement reconciliation, `authority_revision` and one-active-binding semantics. Bound `ReviseLicenseGrant` keeps Grant mutation + successor credential as one recoverable logical commit;
- Credential Resolver owns non-mutating `ResolveCurrentCredential`; repeated device refresh does not call Signer, increment `credential_generation`, consume a seat or create a binding;
- `ReissueCredential` remains a true canonical mutation through Operation Core + Signing Authority for authorized key/format/recovery reissue with authorization-equivalent rights at the same authority revision;
- local perpetual runtime remains server-independent and self-contained from signed credential + local identity state; Dynamic Catalog is not a runtime dependency;
- previous FR-018 migration workflow architecture is historical / compatibility-only for NearHub V1. Existing compatibility data/code may remain isolated, but must not participate in current entitlement evaluation or default FR-019 first-run/commercial licensing paths.

**Quality gate result:** PASS for P14. No second canonical truth was introduced; no architecture-level distributed transaction was introduced between Product SaaS and AxLicense; catalog registration cannot implicitly grant rights; refresh cannot implicitly reissue; signer/DB failure boundaries preserve P13 canonical atomicity; CLI-only Windows V1, no mandatory Agent/Service, provider-pinning/no-silent-downgrade, one-active-binding, credential immutability and perpetual-offline runtime remain preserved.

**Earliest untrusted downstream layer:** **P15 targeted Module Design reconciliation**.

**Next Primary Owner:** `aegis-architecture` at P15.

**Downstream guard:** P15/P16 must materialize Product Backend enrollment/commercial integration, `axlic.exe` registration/assertion/credential modules, Server Device Registry/Catalog Registry/Grant Authority/Credential Resolver & Issuance, and isolated legacy compatibility without redefining P10–P14 authority. P17 remains paused until P15/P16 targeted reconciliation completes.

## P15 FR-019 Module Design closure

### D-079 — P15 FR-019 module boundaries accepted

**State:** ACCEPTED / P15 CURRENT AUTHORITY / 2026-09-12

`aegis-architecture` completed the P15 targeted reconciliation against P14 D-078 and trusted FR-019 P10–P13 authority.

**Frozen device-side module boundaries:**

- Windows V1 remains CLI-only through `axlic.exe`; no public DLL and no required resident Agent/Service;
- `IdentityManager` exclusively owns first-establishment provider selection/pinning and exposes read-only current identity context;
- new `DeviceRegistrationClient` owns ordinary first-run `RegisterDeviceIdentity` client orchestration and persists only the returned opaque Device association after a valid server result; it cannot activate/grant/license or regenerate identity after transport failure;
- new `CredentialSync` owns ordinary device-facing `ResolveCurrentCredential`; `CredentialManager` alone verifies/anti-rolls-back/atomically installs candidates; normal refresh cannot trigger `ReissueCredential` or signer access;
- `LocalMutationCoordinator` remains machine-wide single-writer for local mutations; Diagnostics/SupportBundle remain minimum-disclosure and policy-gated.

**Frozen Product Backend integration modules:**

- `EnrollmentSessionService`, `DeviceVerificationAdapter`, `ProductDeviceAssociationService`, `CommercialEntitlementPolicy`, read-only runtime `EntitlementCatalogClient`, `CommercialLicenseCoordinator`, and scoped `AxLicenseServiceClient`;
- SKU/package/purchase/pool/included/grandfathered policy may map business facts to stable entitlement IDs but cannot redefine `EntitlementDefinition` semantics;
- catalog publication is separated into an authorized Release/Admin `ProductReleaseCatalogPublisher`, not ordinary Product Backend runtime;
- Product association and AxLicense licensing remain separate transaction domains; coordinator retry/reconciliation must not introduce a distributed transaction.

**Frozen server module boundaries:**

- `OperationCore + CanonicalUnitOfWork` remain the only canonical mutation coordination/commit path;
- `DeviceRegistry` owns Device/DeviceIdentity registration/uniqueness planning;
- `ProductEntitlementCatalogRegistry` owns Product/Entitlement definition semantics and CatalogRevision but never customer Grant mutation;
- `GrantAuthority` owns LicenseGrant/Entitlement/Binding invariants but cannot mutate catalog definitions or Product SaaS state;
- `CredentialResolver` is read-only and has no signer dependency; `CredentialIssuance` is reachable only from authorized mutation paths and cannot choose customer rights;
- `ProvisioningAuthority` is factory-specific; ordinary self-registration consumes no ProvisioningAuthorization;
- Challenge/Assertion modules may read DeviceRegistry but cannot mutate ProductDeviceAssociation or license authority.

**Compatibility:** FR-018 migration-specific modules/commands are historical/compatibility-only and must be isolated from the FR-019 normal first-run/commercial path.

**Handoff:** P15 is closed. **Earliest untrusted downstream layer = P16 FR-019 Runtime Data Flow.** P16 must re-trace module-to-module happy/failure/retry/recovery flows including ordinary registration, enrollment/claim, commercial licensing, catalog rollout, Grant revision + successor credential, repeated refresh no-op, partial cross-system success, offline activation, factory, recovery/rehost and compatibility isolation. This decision does not execute P16.

## P16 FR-019 Runtime Data Flow closure

### D-080 — P16 FR-019 runtime temporal model accepted

**State:** ACCEPTED / P16 CURRENT AUTHORITY / 2026-09-13

`aegis-architecture` completed the P16 targeted reconciliation against P14 D-078, P15 D-079 and trusted FR-019 P10–P13 authority.

**Frozen runtime behavior:**

- fresh Windows first-run establishes/pins local identity before network registration; ordinary `RegisterDeviceIdentity` commits canonical Device/current identity without factory authority or license consumption; transport/response loss must retry the same identity and converge to the same `device_id`;
- Product Backend enrollment/SetupCode/Device Assertion/Organization authorization commits `ProductDeviceAssociation` independently of AxLicense license authority; `managed + unlicensed` is a valid recoverable state;
- trusted commercial decisions converge through ordinary `IssueLicenseGrant` / `ReviseLicenseGrant` / `ActivateDevice`; Product SaaS and AxLicense are not wrapped in a distributed transaction;
- dynamic catalog publication advances catalog truth/CatalogRevision only; selected customer rollout mutates Grants independently, with no cross-Grant rollback;
- an already-bound Grant rights change commits Grant revision + successor credential as one recoverable canonical outcome; device observation follows later through resolve/verify/install;
- ordinary `refresh` uses non-mutating `ResolveCurrentCredential` and never calls signer, increments generation or consumes a license merely because polling repeated; `ReissueCredential` is explicit mutation only;
- activation/revision response loss yields `server licensed + observation pending`, recovered by idempotent committed-result lookup or credential resolve;
- local credential installation is verify-before-replace, anti-rollback and crash-safe; provider pinning/no-silent-downgrade is preserved;
- true offline activation, factory identity-only provision + optional pre-activation, same-device recovery and rehost preserve the same canonical/local observation separation;
- Product Organization unclaim/transfer and AxLicense rehost/license lifecycle remain independent facts.

**Historical supersession:** FR-018 provisional/migration application/review/redemption runtime material and the old P16 candidate-flow review remain historical discovery/compatibility context only. This reconciliation does not silently promote those candidates into FR-019 requirements.

**Quality gate result:** PASS. Every accepted FR-019 failure point has an explicit authoritative state owner and retry/recovery path; no second canonical truth, implicit license consumption, distributed transaction, catalog→Grant shortcut, refresh→reissue shortcut or provider downgrade was introduced.

**Earliest untrusted downstream layer:** **P17 Platform Contract**. P17 is now unpaused and may freeze Windows-first CLI/process/wire compatibility, provider/platform capability realization, protected-local-state contract, platform error mapping and related implementation-facing boundaries without redefining P10–P16 authority.

**Next Primary Owner:** `aegis-architecture` at **P17 FR-019 targeted Platform Contract reconciliation**.

## P17 FR-019 Platform Contract closure

### D-081 — Windows-first CLI/CNG/local-state platform contract accepted

**State:** ACCEPTED / P17 CURRENT AUTHORITY / 2026-09-13

`aegis-architecture` completed P17 against trusted P10–P16 FR-019 authority.

**Frozen Windows V1 platform contract:**

- product-facing integration remains machine-installed `axlic.exe`; no public DLL and no mandatory resident Agent/Service;
- read-only local evaluation may run under ordinary read permission, while machine-wide identity/state mutation requires Administrator/System-equivalent write authority; V1 does not introduce a hidden service to bypass that boundary;
- structured request data must support stdin and product-facing structured result is emitted on stdout; product behavior follows stable semantic codes rather than human stderr text; CLI contract uses major/minor compatibility;
- TPM hardware-backed identity is realized through Windows CNG/KSP equivalent to Microsoft Platform Crypto Provider/NCrypt; software fallback uses Windows software KSP equivalent to Microsoft Software Key Storage Provider; private key material remains provider-owned/non-exported and application code never stores raw private key in registry/file;
- TPM→software fallback is permitted only during initial establishment when policy allows and hardware provider is absent/permanently unusable; transient provider failure does not justify permanent downgrade; established provider remains pinned;
- machine local state is machine-wide under a protected ProgramData-class root with ACL/OS-protection defense-in-depth; app-controlled registry is not the private-key store;
- local authority observation updates use machine-wide cross-process serialization, re-read-after-lock, full candidate validation and same-volume crash-safe atomic replace; committed credential/high-watermark move together;
- ordinary `refresh` remains `ResolveCurrentCredential` read/delivery and never invokes signer or increments credential generation; explicit `ReissueCredential` remains a separate authorized mutation path;
- Windows device transport uses mandatory TLS certificate/hostname validation and preserves logical correlation/idempotency across retry; Product Backend/Release/Admin/Factory identities/scopes remain separate;
- platform Win32/CNG/TPM/TLS/file failures are normalized to stable AxLicense semantic families; raw HRESULT/NTSTATUS/provider text is diagnostics-only and policy-gated;
- typed proof/challenge boundary prevents a public generic `sign(bytes)` capability and preserves purpose/audience/freshness separation.

**Deferred without blocking P17:** final crypto algorithm/key-size profile and test vectors, exact installer technology/literal paths/mutex names, exact REST paths/compact QR encoding, cloud/DB/KMS vendor, performance sizing and retry/cache tuning, and malicious-local-Administrator hardening beyond V1 trust scope.

**Quality gate:** PASS. No second authority, provider downgrade shortcut, raw-key persistence, refresh→reissue collapse, or loss of crash-safe/single-writer semantics was introduced.

**Handoff:** P17 CLOSED. **Earliest untrusted downstream layer = P18 Engineering / Optimization.** Next Primary Owner: `aegis-architecture` at P18. P18 may optimize workload/latency/I/O/cache/backoff/operational hardening only within P10–P17 authority.

## P18 FR-019 Engineering / Optimization closure

### D-082 — Windows-first engineering budgets, observability and optimization guardrails accepted

**State:** ACCEPTED / P18 CURRENT AUTHORITY / 2026-09-13

`aegis-architecture` completed P18 against trusted P10–P17 FR-019 authority.

**Frozen engineering posture:**

- current observed baseline is explicitly **UNMEASURED** until implementation benchmark exists; P18 freezes initial engineering budgets and measurement methods, not fabricated benchmark claims;
- fast-path priority is local/offline `status` / entitlement evaluation; normal product startup must not require network refresh before local authorization evaluation;
- Windows budgets cover `axlic.exe` cold/warm latency, credential verification, CNG/TPM operations, local writer contention/atomic replace, refresh, process memory and bounded local state;
- server budgets decompose resolver, canonical DB/CAS, signer and full authority mutation latency so external signer/network cost cannot be hidden inside one opaque number;
- cache may accelerate Catalog/Resolver reads but can never become a second authority, synthesize credentials, bypass canonical validation or turn refresh into reissue;
- retry/backoff preserves stable operation identity and uses jitter; security/policy/schema/TLS-trust failures are not blindly retried;
- bulk rollout remains per-Grant independent work with bounded concurrency/backpressure and no cross-Grant transaction;
- observability captures latency/result/conflict/queue/cache signals while prohibiting raw DeviceIdentity, credential bytes, private/provider key material, bearer secrets and high-cardinality customer/device identifiers from metric labels;
- every optimization keeps a simpler correctness reference path, but there is no reference mode that bypasses signature verification, anti-rollback, provider pinning, canonical CAS, machine single-writer or crash-safe replace.

**Initial engineering budgets:** local warm status/entitlement p95 ≤ 150 ms; cold p95 ≤ 300 ms; credential verify p95 ≤ 50 ms; ordinary proof/key-open p95 ≤ 750 ms; first identity establishment p95 ≤ 3 s; healthy-network no-change refresh device-visible p95 ≤ 2 s; local install after credential receipt p95 ≤ 250 ms; resolver app-side p95 ≤ 150 ms; full credential-producing authority mutation app-side target p95 ≤ 1.5 s excluding client Internet RTT. These are targets to verify, not observed facts.

**Evidence rule:** performance evidence becomes blocking only when it uniquely detects a high-impact release failure not already independently covered; correctness/security invariants remain blocking regardless of performance.

**Quality gate:** PASS. No optimization decision redefines P10–P17 authority or introduces resident service/DLL, weaker identity fallback, unsigned authority cache, distributed transaction or relaxed crash-safety.

**Architecture family status:** **P14 → P15 → P16 → P17 → P18 ACCEPTED / CLOSED** for FR-019 Windows-first V1.

**Successor routing:** return to central `aegis`. P18 does not authorize verification or implementation planning by itself.

## Architecture Foundation successor routing — 2026-09-13

### D-083 — P14–P18 Architecture Foundation complete; route directly to P20 Verification Design

**State:** ACCEPTED ROUTING / READY_TO_ROUTE / 2026-09-13

Central `aegis` reviewed the FR-019 authority chain after P18 D-082.

- **Current Authority:** P02/P03 and P10–P18 are trusted for the Windows-first V1 baseline. P14–P18 are CLOSED as one Architecture Foundation.
- **Repository state:** `Mostorm-Labs/axlic` is currently empty and contains no `.aegis` project-control manifests or implementation baseline that would justify routing to implementation stages.
- **Verification state:** no current FR-019 P20 Verification Design authority exists after the P10–P18 reconciliation.
- **Conflict review:** no material conflict or unresolved authority ambiguity was found that requires a separate P21 Authority Review or P22 Five-Axis Drift Review before verification design. Historical FR-018 material is already explicitly superseded/compatibility-only and does not create a competing Current Authority.
- **Supersession/readiness review:** P23 is not required merely to restate the already-established FR-019 current-authority relation; P24 is premature because there is no implementation/release candidate.

**Earliest Untrusted Layer:** **P20 Verification Design**.

**Next Primary Owner:** `aegis-verification`.

**Required route:** `P20 FR-019 Verification Design reconciliation → central aegis successor routing → P30 Implementation Planning` if P20 closes without reopening upstream authority.

**P20 emphasis:** define requirement/invariant/failure-mode coverage, independent oracles/references, fixtures/corpora, exact execution method, blocking vs corroborative evidence, and Gate criteria across signed credential integrity/offline evaluation, DeviceIdentity/provider pinning, first-run registration/idempotency, Grant/Binding atomicity, refresh-vs-reissue separation, local crash safety/single-writer/anti-rollback, catalog/Grant revision semantics, CLI/platform compatibility, diagnostics minimum-disclosure, and P18 engineering budgets.

**Evidence rule:** an evidence artifact may become blocking only when it uniquely detects a material high-impact failure mode not already adequately covered by another independent mechanism. Performance measurements are not blocking merely because P18 defines budgets; P20 must justify which budget violations are release-critical.

**Routing result:** **READY_TO_ROUTE TO P20**. This decision does not execute P20 substantive work.

## P20 preflight authority handoff — 2026-09-13

### D-084 — P20 blocked by missing canonical signing-byte contract; route to P17 targeted repair

**State:** ACCEPTED ROUTING / BLOCKED_AUTHORITY AT P20 / 2026-09-13

`aegis-verification` began P20 FR-019 Verification Design against D-083 and discovered a verification-blocking upstream contract gap before freezing evidence.

**Observed authority gap:**

- P12 explicitly leaves concrete credential wire encoding / canonical byte serialization to later architecture/platform authority.
- P17 freezes CLI, CNG provider, proof purpose/audience/freshness, local state, HTTPS and signing-authority ports, but does **not** freeze the canonical byte sequence that a `SignedLicenseCredential` signer/verifier signs/verifies.
- P17 likewise defines typed `ProofInput` semantics but does not freeze the canonical proof/challenge signing transcript bytes required for deterministic cross-implementation security vectors.
- P17 delegates final crypto algorithm/key-size/profile and security test vectors to P20.

**Why P20 cannot silently repair this:** A cryptographic golden vector is meaningless unless both the protected logical fields and the exact deterministic signing input are authority-defined. Choosing CBOR/JSON/protobuf layout, field ordering, length-prefixing, transcript framing or equivalent canonicalization inside P20 would make Verification Design invent a Platform Contract rather than verify one.

**Earliest Untrusted Layer:** **P17 Platform Contract**, narrowly scoped to canonical signing-input / wire canonicalization.

**Required targeted repair scope:**

1. define deterministic canonical signing bytes for `SignedLicenseCredential` protected payload and signature envelope boundaries;
2. define deterministic canonical transcript bytes for DeviceIdentity proof and signed challenge envelopes, preserving purpose/audience/session/freshness domain separation;
3. define stable algorithm/key identifiers and signature/public-key encoding slots **without** selecting the final cryptographic primitive that remains P20-owned;
4. define parser/canonicalizer ambiguity rules: one logical object must not admit multiple authorization-equivalent byte representations that verify differently; malformed/non-canonical authorization-critical representations fail closed;
5. preserve P12 semantic fields, P13 operation semantics, P16 ordering and P17 provider/key-custody boundaries; this repair must not add a second authority or change license behavior.

**Downstream impact:** P18 engineering budgets are not substantively contradicted, but because P18 depends on P17 it requires a targeted impact revalidation after the P17 repair. No P02–P16 authority is reopened.

**Required route:** `P17 targeted canonical-signing contract repair → P18 targeted impact revalidation → central aegis successor routing → P20 FR-019 Verification Design`.

**P20 status:** no Verification Authority was frozen and no P20 page/evidence contract was created from incomplete upstream authority.

## P17 D-084 targeted repair closure — 2026-09-13

### D-085 — Canonical signing-byte contract accepted; D-084 P17 blocker closed

**State:** ACCEPTED / P17 TARGETED REPAIR CLOSED / 2026-09-13

`aegis-architecture` repaired the single P17 authority gap discovered by P20 preflight without reopening P10–P16 semantics.

**Accepted contract:**

- V1 authorization-critical signing representation is **AxLicense Canonical Binary Encoding v1**, based on RFC 8949 Core Deterministic CBOR plus stricter AxLicense rules: definite-length only, no float/tag/null in current signed payloads, fixed unsigned-integer field labels, exact UTF-8 strings, byte strings for opaque material, explicit absent optional fields, and canonical rejection rather than parse/re-encode acceptance;
- `SignedLicenseCredential` and trusted Device Assertion Challenge use deterministic signed envelopes that integrity-bind `algorithm_id`, `key_id` and exact canonical `payload_bytes`;
- credential entitlement entries are canonicalized by ascending `entitlement_id`, with fixed wire enum codes and fixed canonical Instant representation;
- `challenge_digest` is over the deterministic ChallengeSigningInput bytes using the hash selected later by P20;
- DeviceIdentity proof uses a separate `axlicense.device-proof` domain-separated deterministic transcript binding assertion identity, challenge digest, device/current identity context, audience, purpose, session and time bounds;
- JSON/CLI/HTTP/QR/Base64/file encodings remain carrier encodings only and are not alternative signing representations;
- P17 still does **not** choose the final signature/hash algorithm profile, key sizes or golden crypto vectors. Those remain P20 verification authority.

**Quality result:** D-084's residual proof gap is closed: P20 can now define independent golden credential/challenge/proof vectors and parser/tamper/cross-implementation evidence without inventing serialization semantics.

**Downstream impact:** no P10–P16 contract changed. P18 engineering budgets remain substantively intact but depend on P17 and therefore require the already-routed **targeted impact revalidation** for encoding size / verification latency / memory assumptions before central `aegis` may reauthorize P20.

**Next owner/stage:** `aegis-architecture → P18 targeted impact revalidation`. This decision does not resume P20 yet.

## P18 D-085 targeted impact revalidation closure — 2026-09-13

### D-086 — Deterministic signing representation has no substantive P18 conflict; Architecture Foundation re-closed

**State:** ACCEPTED / P18 TARGETED IMPACT REVALIDATION CLOSED / 2026-09-13

`aegis-architecture` revalidated P18 against the D-085 deterministic canonical-signing repair.

**Impact verdict:** **NO SUBSTANTIVE P18 CONTRADICTION.** The deterministic-CBOR signing representation changes internal encode/decode/canonicality-check work but does not change workload classes, state ownership, process topology, retry/backoff, local persistence, canonical transaction ordering, cache authority boundaries, or existing resource ceilings.

**Engineering clarifications accepted:**

- the existing `credential verify inside process` p95 ≤ 50 ms budget now includes bounded canonical-CBOR parse/canonical-form validation plus envelope/domain/version, signature, schema and entitlement checks;
- challenge canonicalization/digest, device-proof transcript construction, provider/KSP proof, server canonical payload encoding and external signer latency must be measurable as distinct components;
- the existing ≤ 256 KiB single SignedLicenseCredential/offline-response guardrail applies to the complete delivered canonical signed artifact;
- malformed/non-canonical artifacts require bounded parser depth/item-count/size handling before expensive cryptographic work; no unbounded allocation or parse/re-encode acceptance path is permitted;
- canonical encode/decode/size measurements are diagnostic decomposition and do not automatically become blocking Gates.

**Unchanged authority:** all existing P18 latency, memory, local-state, lock/backpressure, retry, server, cache, observability and rollback/reference-path budgets remain Current Authority. Baselines remain **UNMEASURED** until implementation measurement.

**Architecture status:** P17 D-084 is closed under D-085; P18 targeted impact revalidation is closed under D-086. P14–P18 Architecture Foundation is again fully closed.

**Required next route:** return to central `aegis` successor routing. This decision does not automatically resume P20 substantive Verification Design.

## Post-D-086 successor routing — 2026-09-13

### D-087 — Architecture Foundation re-closed; P20 Verification Design reauthorized

**State:** ACCEPTED ROUTING / READY_TO_ROUTE / 2026-09-13

Central `aegis` recomputed lifecycle routing after D-085/D-086.

**Authority check:**

- D-084 identified one verification-blocking P17 gap: canonical signing-byte / deterministic-wire authority was missing.
- D-085 repaired that exact P17 gap without reopening P10–P16 semantics; credential/challenge/device-proof deterministic signing representation is now Current Authority while final crypto profile remains P20-owned.
- D-086 completed the required P18 targeted impact revalidation and found **NO SUBSTANTIVE CONTRADICTION**; existing engineering budgets/resource ceilings remain accepted with canonical encode/decode/canonicality-check cost added to measurement decomposition and bounded-parser guardrails.
- P02/P03 and P10–P18 therefore form a continuous trusted Current Authority chain again. Architecture Foundation P14–P18 is CLOSED.

**Earliest Untrusted Layer:** **P20 Verification Design.** No Current P20 Verification Authority was frozen during the blocked preflight, so verification remains the first missing layer.

**No intermediate governance stage is required:**

- P21 Authority Review is not independently required because no competing Current Authority or unresolved source-of-truth conflict is present.
- P22 Five-Axis Drift Review is not independently required because there is no implementation/repository reality yet that conflicts with Current Authority and no newly identified cross-axis drift remains after D-086.
- P23 Authority Supersession is not required merely to record D-085/D-086; Current Authority already identifies the repaired P17/P18 state and historical FR-018 material remains explicitly historical/compatibility-only.
- P24 Release Readiness is premature because no release candidate exists.

**Route:** `aegis-verification → P20 FR-019 Verification Design reconciliation`.

**P20 authority inputs:** P02/P03 FR-019 requirements/traceability; P10–P13 trusted semantics; P14–P16 trusted architecture/module/runtime flow; P17 D-081 + D-085 platform/signing-input contract; P18 D-082 + D-086 engineering budgets and measurement contract.

**P20 required emphasis:** signed credential canonical bytes/signature/offline evaluation; challenge/device-proof domain separation and replay resistance; TPM/software provider selection and no-silent-downgrade; first-run registration/idempotency/identity uniqueness; Grant/Binding/credential atomicity; `ResolveCurrentCredential` non-mutation vs explicit `ReissueCredential`; local single-writer/crash-safe/anti-rollback behavior; CatalogRevision vs authority_revision vs credential_generation isolation; CLI/platform compatibility; diagnostics minimum-disclosure; bounded canonical-CBOR parser behavior; and only release-critical P18 performance/resource evidence.

**Evidence rule:** blocking evidence is allowed only when it uniquely detects a material high-impact failure mode not already adequately covered by another independent mechanism. Do not turn every measurable P18 metric or every cryptographic vector into a separate blocking Gate.

**Handoff:** READY_TO_ROUTE to `aegis-verification` at P20. This routing decision does not execute P20 substantive work.

## P20 Verification Design closure — 2026-09-13

### D-088 — FR-019 Verification Design accepted; seven blocking evidence families frozen

**State: ACCEPTED / P20 CURRENT AUTHORITY / CLOSED / 2026-09-13**

`aegis-verification` completed P20 after D-087 reauthorization. No remaining upstream authority gap was found.

**Accepted crypto profile:**

- credential/challenge `algorithm_id = axl-ecdsa-p256-sha256-v1`;
- NIST P-256 / secp256r1 + SHA-256;
- fixed 64-byte `r || s` signature representation, 32-byte big-endian components;
- accepted artifact signatures require low-S canonical form;
- credential-signing, challenge-signing and per-device proof key purposes remain separated;
- P17 canonical CBOR/signing-input bytes remain unchanged; P20 only freezes the crypto profile and verification vectors/oracles.

**Blocking evidence families frozen by P20:**

1. `EV-01` Canonical / Crypto Conformance Corpus;
2. `EV-02` Domain Mutation / Lifecycle Conformance;
3. `EV-03` Windows Device Identity Provider Qualification;
4. `EV-04` Local Durability / Concurrency / Anti-Rollback;
5. `EV-05` Device Assertion / Control-Plane Scope Separation;
6. `EV-06` CLI / Diagnostics Public Contract;
7. `EV-07` W1 Startup-Critical Performance Gate.

Each blocking family records a distinct high-impact residual failure mode and unique detection value. Random fuzzing beyond deterministic negative corpora, duplicate local/hosted runs of the same oracle, extended cold-start/memory/server-capacity benchmarks, alternate vendor matrices and manual duplicate reviews remain corroborative unless a later genuinely new uncovered high-impact failure mode justifies promotion.

**P20 evidence posture:** P20 freezes proof contracts, not observed PASS claims. The repository has no materialized implementation yet; blocking evidence is produced only after the relevant implementation exists. P31 must later freeze these seven families into its `EXECUTION_CLOSURE_CONTRACT` without expanding the finish line.

**Handoff:** P20 is CLOSED. Return to central `aegis` successor routing; do not directly chain from `aegis-verification` into P30/P31.

## Successor routing after P20 closure — 2026-09-13

### D-089 — P20 Verification Authority closed; route directly to P30 Implementation Planning

**State: ACCEPTED ROUTING / READY_TO_ROUTE / 2026-09-13**

Central `aegis` recomputed the earliest untrusted layer after D-088.

**Trusted Current Authority:** P02/P03 + P10–P18 + P20 D-088. The seven blocking evidence families are frozen as Verification Authority; they are not yet materialized PASS evidence because implementation does not yet exist.

**Repository reality:** `Mostorm-Labs/axlic` remains an empty `main` repository (`size=0`) with no implementation baseline, `.aegis` project state, active Gate, PR, release candidate, or conflicting repository occurrence.

**Routing determination:**

- P21 Authority Review is not required: there is one continuous trusted Current Authority chain and no competing source of truth.
- P22 Five-Axis Drift Review is not required: there is no implementation reality capable of drifting from Current Authority.
- P23 Authority Supersession is not required: FR-018 historical/compatibility material is already explicitly non-current; D-088 does not create competing authority.
- P24 Release Readiness is not applicable before implementation/release-candidate existence.
- P31 cannot safely precede P30 because the V1 implementation is multi-module, security-critical, cross-boundary, and has seven frozen evidence families that must be allocated across dependency-ordered implementation slices.

**Earliest untrusted downstream layer:** **P30 Implementation Planning**.

**Next Primary Owner:** `aegis-implementation` at P30.

**P30 objective:** convert trusted P10–P20 authority into a dependency graph / vertical-slice implementation plan that establishes repository/tooling foundations, canonical/crypto primitives, domain core, Windows identity/local-state platform layer, CLI, server/control-plane integration, offline/factory/recovery flows, and staged materialization of EV-01..EV-07. P30 must allocate each blocking evidence family to the earliest implementation slice capable of materializing it, but must not yet authorize coding or freeze P31 task-package closure contracts.

**Handoff:** READY_TO_ROUTE to `aegis-implementation → P30 AxLicense V1 Implementation Planning`. This routing does not execute P30 substantive work.

## P30 Implementation Planning closure — 2026-09-13

### D-090 — AxLicense V1 implementation plan accepted; vertical-slice sequence frozen

**State: ACCEPTED / P30 CURRENT AUTHORITY / CLOSED / 2026-09-13**

`aegis-implementation` completed P30 against trusted P02/P03 + P10–P20 Current Authority and D-089 routing.

**Accepted implementation direction:**

- Rust stable monorepo / Cargo workspace for both Windows `axlic.exe` and AxLicense Server modular monolith;
- PostgreSQL as V1 canonical server persistence implementation;
- production signing only through `LicenseSigningPort` / `ChallengeSigningPort`; fixture signer remains test/dev-only and production-disabled;
- P17 deterministic CBOR remains the only cryptographic representation; JSON/HTTP/CLI are carrier boundaries only;
- Product-specific Account/Organization/SKU/IAM truth remains outside `Mostorm-Labs/axlic`.

**Accepted vertical implementation sequence:** `AXL-V1-A0 Local Signed-Credential Gate → A1 Device Identity + RegisterDeviceIdentity → A2 Online Grant Activation + Credential Install → A3 Dynamic Catalog/Grant Revision/Refresh + A4 Device Assertion/Control-Plane Scope + A5 Offline Activation → A6 Recovery/Rehost/Factory/Admin → A7 Windows Hardening + Release Qualification`.

A3/A4/A5 may execute in parallel only after A2 accepted baseline and only through separately frozen P31 packages with non-overlapping/controlled shared-surface changes.

**Evidence allocation:** EV-01/EV-06/EV-07 begin materialization in A0; EV-02/EV-03/EV-04 in A1/A2; EV-05 in A4; final release-materialized EV-01～EV-07 is reached by A7. Early evidence is not automatically final Gate PASS.

**Empty-repository control:** `Mostorm-Labs/axlic` currently has no commit. Before the first repository-backed P32 package, P31 must establish a minimal authority-neutral seed commit, capture its exact SHA, then materialize the A0 package with that revision as non-null ancestor `task_anchor`. The seed commit is repository-control metadata only and is not product implementation evidence.

**P30 disposition:** no unresolved Authority or Verification gap prevents packaging. Earliest untrusted layer = **P31 Task Packaging**. Recommended first package = **AXL-V1-A0 — Local Signed-Credential Gate**. Coding remains unauthorized until P31 freezes the A0 `EXECUTION_CLOSURE_CONTRACT`.

## P30 targeted implementation-plan reconciliation — 2026-09-13

### D-091 — Client-first C++ / Node.js implementation stack accepted

**State: ACCEPTED / P30 CURRENT IMPLEMENTATION PLAN AUTHORITY / 2026-09-13**

用户在 P31 尚未开始前调整实现策略，以长期可读性、团队理解和维护便利为优先：Windows `axlic.exe` 客户端改为 **C++20 + CMake**；AxLicense Server 改为 **Node.js + TypeScript**；PostgreSQL 继续作为 V1 canonical persistence implementation。

**Impact classification:** 该变化属于 P30 implementation-plan authority change，不改变 P10–P18 产品/语义/架构/平台契约，也不改变 P20 verification obligations。P17 deterministic CBOR + P20 EV-01 golden corpus成为 C++ client 与 TypeScript server 的跨语言兼容合同；不得各自发明第二套 authorization wire semantics。

**Client-first sequencing:**

- A0 = C++ Local Signed-Credential Gate；
- A1 = C++ Windows Identity + Protected Local State；
- A2 才引入 Node.js/TypeScript + PostgreSQL，并完成 `RegisterDeviceIdentity`；
- 后续 Online Activation / Refresh / Assertion / Offline / Recovery-Rehost-Factory / Windows Release Qualification依次推进至 A8。

**Governance result:** D-090 中 Rust/Cargo unified-production-language wording被本决议 supersede；其余 P30 principles（vertical slices、evidence allocation、empty-repository seed-anchor、P31-per-slice closure）继续有效。当前 earliest untrusted layer仍为 **P31 AXL-V1-A0 Task Packaging**；substantive coding仍未授权。

## P31 AXL-V1-A0 Task Package closure — 2026-09-13

### D-092 — AXL-V1-A0 package frozen and authorized for P32 execution

**State: ACCEPTED / P31 CLOSED / READY_FOR_P32 / 2026-09-13**

`aegis-implementation` 已完成首个 A0 package，并在 repository 中 materialize durable package：`Mostorm-Labs/axlic@6190288cf7d83bcbe588a4e19fda8d6354335e6f:.aegis/packages/AXL-V1-A0-P31.md`。

Repository binding：

- authority-neutral seed `task_anchor` = `5a2cb0fc105193523e518cd9c109ccae35bb2188`, relation=`ancestor`；
- package materialization ref = `6190288cf7d83bcbe588a4e19fda8d6354335e6f`；
- execution branch = `codex/axl-v1-a0`；
- `resume_cursor = null`。

A0 scope只包含 C++20/CMake Windows local signed-credential gate：deterministic-CBOR credential parsing/canonicality、P-256/SHA-256 fixed-width low-S verification via Windows CNG、offline RuntimeLicense entitlement evaluation、`status`/`entitlement` CLI、independent golden vectors/reference verifier、Windows hosted CI与W1 baseline harness。明确禁止 Node.js server/PostgreSQL、DeviceIdentity establishment、online/offline activation、refresh/rehost/factory/admin以及 public DLL。

P31 已冻结 `EXECUTION_CLOSURE_CONTRACT`：T-A0-01～T-A0-05 为 blocking PASS obligations；T-A0-06 必须 materialize baseline report，但数值门槛在 A0 仅 corroborative；hosted Windows CI与 reviewer-accessible `AXL-V1-A0-evidence` artifact 为 terminal success 条件。`continue_until_terminal_state: true`。

**Next stage:** P32 AXL-V1-A0 Implementation Control on CODE_EXECUTION surface. P32 may return only terminal success or an explicit frozen blocker and MUST NOT imply official P34 PASS.

## P34 / P35 AXL-V1-A0 Gate control — 2026-09-14

### D-093 — A0 evidence passes but frozen QCBOR implementation boundary is violated; Gate BLOCKED and routed to P36

**State: ACCEPTED GATE DECISION / P34 BLOCKED / P35 IMPLEMENTATION_DEFECT / READY_FOR_P36.**

P34 independently resolved `Mostorm-Labs/axlic`, package `6190288cf7d83bcbe588a4e19fda8d6354335e6f`, result `09920aa719e93f076a04cfc62717104c547956d3`, execution branch, hosted run `34821657839` attempt 1, job `103904388076`, and evidence artifact `10338895862` digest `sha256:222212517e34efeebc3732587d8b7759643a3abdd8ed011b3308d2ba8d691704`.

Frozen evidence is credible and exact-result-bound: 11/11 CTest PASS, EV-01 PASS, independent reference PASS, EV-06 PASS, offline/no-server PASS, W1 baseline materialized at p95 5.601 ms / p99 5.833 ms. No proof-recursion or duplicate evidence is required.

The Gate nevertheless fails one frozen requirement: P31 selected QCBOR v1.6.1 commit `930708bb86481e88879eb1d87fd4d664f1d69503` as the production CBOR dependency behind `client/src/wire`, while result `09920aa…` does not fetch/link QCBOR and instead contains a project-owned CBOR reader/writer in `credential.cpp`. This is `FROZEN_REQUIREMENT_FAILURE`, classified at P35 as `IMPLEMENTATION_DEFECT`, not an Authority/Verification/package/evidence defect.

**Route:** preserve all valid A0 work; P36 performs targeted QCBOR production-boundary repair, updates dependency notices/reporting, reruns T-A0-01..06 and exact-result hosted evidence, then returns to P34 rereview. Do not reopen P20/P30/P31 and do not widen A0 scope.

Non-blocking findings: literal VS patch-label drift while MSVC 19.44/toolset 14.44 + SDK 10.0.26100 remain verified; mechanical README/git metadata additions; upload-artifact Node runtime deprecation warning.

## P36 AXL-V1-A0 repair closure — 2026-09-14

### D-094 — A0-P34-B1 QCBOR production-boundary defect repaired and reverified

**State: ACCEPTED P36 RESULT / READY_FOR_P34_REREVIEW**

P35-classified implementation defect `A0-P34-B1` has been repaired without reopening P20/P30/P31 or widening A0 scope. New exact result is `13d16ebb34913dafbddc83222cf29187e92c8053` on `codex/axl-v1-a0`.

- Frozen QCBOR v1.6.1 commit `930708bb86481e88879eb1d87fd4d664f1d69503` is now in the production CMake graph through `qcbor::qcbor`.
- Project-owned generic CBOR reader/writer was replaced by a narrow QCBOR-backed `client/src/wire/qcbor_codec.*` boundary while retaining AxLicense fail-closed canonicalization, bounds, UTF-8, P17 signing-input, low-S and Windows CNG rules.
- Exact hosted run `34824491876`, job `103913345075` succeeded on the new result revision; artifact `10340300160` / digest `sha256:e1c696436a1377a5ce1d3671abe5aab6c4a5470400516fc7d7d5a41cee87cfe7` is reviewer-resolvable and bound to the same SHA.
- Frozen regression set T-A0-01..06 is satisfied: 11/11 CTest, EV-01 PASS, independent reference PASS, EV-06 PASS, offline/no-server PASS, W1 baseline produced (p95 `5.574 ms`, p99 `5.875 ms`).
- P36 discovered no new blocking finding and does **not** issue P34 PASS.

**Route:** `P34 rereview` must independently bind `13d16ebb…` and the new hosted evidence, then determine official A0 Gate status.

## AXL-V1-A0 Gate rereview closure — 2026-09-14

### D-095 — AXL-V1-A0 P34 rereview PASS_WITH_FINDINGS

**State: ACCEPTED / GATE CLOSED / READY_FOR_SUCCESSOR_ROUTING.**

P34 independently rebound the repaired repository result `13d16ebb34913dafbddc83222cf29187e92c8053` to governing package `6190288cf7d83bcbe588a4e19fda8d6354335e6f:.aegis/packages/AXL-V1-A0-P31.md`, hosted run `34824491876`, job `103913345075`, and reviewer artifact `10340300160` (`sha256:e1c696436a1377a5ce1d3671abe5aab6c4a5470400516fc7d7d5a41cee87cfe7`). Package→result ancestry is valid; the execution branch currently resolves to the repaired result.

The historical `A0-P34-B1 / FR-A0-DEP-QCBOR` frozen-requirement failure is closed: QCBOR exact commit `930708bb86481e88879eb1d87fd4d664f1d69503` is now in the production dependency graph, linked as `qcbor::qcbor`, isolated behind `client/src/wire/qcbor_codec.*`, with AxLicense-specific canonicality/bounds/fail-closed policy preserved above the library boundary. T-A0-01 through T-A0-05 PASS and T-A0-06 produces the required baseline; exact hosted evidence is reviewer-accessible and bound to the repaired result.

P34 records only non-blocking findings: independent-reference negative-result reporting granularity, exact VS patch-label drift within the frozen compiler/toolset family, upload-artifact Node runtime deprecation, ancestry-only content-identical noop commit, and README/git metadata hygiene. None leaves an uncovered high-impact failure mode with unique blocking detection value under P20 / Anti-Proof-Recursion rules.

**Verdict:** `PASS_WITH_FINDINGS`. **AXL-V1-A0 Gate CLOSED.** D-093 remains historical blocked review evidence and D-094 remains the P36 repair record; D-095 is the current Gate disposition.

**Route:** return to central `aegis` for fresh successor routing from the new repository/Gate reality. Central routing must decide Repository Integration Closure vs successor A1 lifecycle ingress; P34 does not directly authorize A1 or integration.

### D-096 — A0 Gate-closed result must enter canonical main before A1 ingress

**State:** ACCEPTED ROUTING / AWAITING_REPOSITORY_INTEGRATION / 2026-09-14

Fresh successor routing after D-095 establishes the following repository reality:

- canonical `main` = `6190288cf7d83bcbe588a4e19fda8d6354335e6f`;
- Gate-closed A0 result = `13d16ebb34913dafbddc83222cf29187e92c8053` on `codex/axl-v1-a0`;
- `main` is the merge base / package materialization ancestor and does **not** yet contain the A0 implementation;
- no open PR currently exists from `codex/axl-v1-a0` to `main`;
- P34 rereview is `PASS_WITH_FINDINGS / CLOSED` (D-095), so the A0 result is eligible for repository integration;
- the repository contains `.aegis/packages/**` only; no authored Project State manifest set exists, so there is no manifest state to recompute or use as a substitute for repository occurrence evidence.

**Routing decision:** complete **AXL-V1-A0 Repository Integration Closure** before any A1 task package or execution authority is issued.

**Why:** P30 freezes `AXL-V1-A1 — C++ Windows Identity + Protected Local State` as a direct successor/dependent slice of A0. Packaging or executing A1 from current `main@6190288…` would omit the Gate-closed A0 wire/runtime/CLI foundation from the canonical baseline and create an avoidable split between implementation reality and the baseline used for A1 ancestry.

**Integration closure target:**

1. integrate exact Gate-closed source `13d16ebb34913dafbddc83222cf29187e92c8053` from `codex/axl-v1-a0` into canonical `main` through a reviewer-resolvable repository occurrence;
2. preserve D-095 Gate identity and its exact run/job/artifact evidence; merging does not create or replace Gate evidence;
3. record exact integrated `main` revision and occurrence evidence after integration;
4. verify that the resulting canonical baseline contains the A0 implementation and remains consistent with the D-095 Gate-closed tree/history;
5. only after Integration Closure, return to central `aegis` for fresh A1 successor routing.

**A1 is not yet authorized.** Do not create P31 A1 from `main@6190288…`, and do not treat the Gate-closed feature branch as canonical baseline before integration occurrence is proven.

**Next action:** AXL-V1-A0 Repository Integration Closure. After it is closed, central `aegis` must route A1 from the new canonical `main` and decide whether A1 can go directly to P31 or needs a targeted readiness/reconciliation check first.

### D-097 — AXL-V1-A0 Repository Integration Closure completed

**State:** ACCEPTED / INTEGRATED / CLOSED — 2026-09-14

AXL-V1-A0 exact Gate-reviewed result `13d16ebb34913dafbddc83222cf29187e92c8053` has entered canonical `main` through [PR #1](https://github.com/Mostorm-Labs/axlic/pull/1) using a merge commit.

**Repository occurrence:**

- pre-integration `main`: `6190288cf7d83bcbe588a4e19fda8d6354335e6f`
- Gate-reviewed source: `13d16ebb34913dafbddc83222cf29187e92c8053`
- integrated canonical revision: `c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`
- merge parents: `6190288…`, `13d16ebb…`
- integrated tree: `93b3d969d6593b68b2e415e9e5fd205686690c6a`, identical to the D-095 Gate-reviewed source tree

**Conformance:** D-095 Gate verdict was `PASS_WITH_FINDINGS / CLOSED` before integration; therefore this repository occurrence is conforming. The merge does not create new Gate acceptance evidence and does not rewrite historical D-093/D-094/D-095 records.

**Disposition:** A0 Repository Integration Closure is complete. Canonical implementation baseline is now `main@c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`. Return to central `aegis` for fresh successor routing of AXL-V1-A1. The integration itself does not authorize A1 coding or create a P31 A1 package.

### D-098 — AXL-V1-A1 may enter P31 directly from integrated A0 baseline

**State:** ACCEPTED ROUTING / READY_FOR_P31 / 2026-09-14

Central `aegis` recomputed successor routing from canonical `main@c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4` after D-097 Repository Integration Closure.

- A0 is both Gate-closed and repository-integrated; the canonical baseline now contains the reviewed signed-credential/runtime foundation.
- P30 already defines AXL-V1-A1 as **C++ Windows Identity + Protected Local State**, before any Node/TypeScript server work.
- P17 already freezes the Windows CNG provider order, persisted non-exportable key boundary, provider pinning, machine-wide ProgramData-class state, protected envelope, cross-process serialization, re-read-after-lock, crash-safe atomic replace, privilege/error semantics, and no-service/no-public-DLL boundary.
- P20 already freezes the relevant A1 verification ownership through EV-03 Windows Device Identity Qualification and EV-04 Local Durability / Anti-Rollback, with EV-06 identity CLI contract maturation.
- No P21/P22/P23 reconciliation is required: no conflicting Current Authority or implementation drift was found that changes A1 semantics.
- No separate targeted readiness stage is required before P31. The current A0 fixture adapter is intentionally A0-only implementation reality; P31 A1 must explicitly freeze its transition to test-only/capability-gated use while production `axlic.exe` gains real identity/protected-state seams. This is A1 implementation scope, not an upstream defect.

**Earliest Untrusted Layer:** **P31 AXL-V1-A1 Task Packaging**.

**Required A1 task anchor:** canonical `main@c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`.

**Next Primary Owner:** `aegis-implementation`.

**Next stage:** `P31 AXL-V1-A1 Task Packaging — C++ Windows Identity + Protected Local State`.

This routing authorizes package construction only. A1 substantive coding remains unauthorized until P31 freezes its own `EXECUTION_CLOSURE_CONTRACT` and materializes a repository-backed package.

### D-099 — AXL-V1-A1 P31 task package materialized; P32 authorized

**State:** ACCEPTED / P31 CURRENT AUTHORITY / CLOSED / 2026-09-14

Following D-098 routing, `aegis-implementation` materialized the AXL-V1-A1 C++ Windows Identity + Protected Local State task package against the canonical A0 baseline.

Frozen execution identity:

- repository = `github/Mostorm-Labs/axlic`
- task anchor = `c83e38ded5cdb0f385cbdd2fa3a633aa8c7c09c4`
- package materialization = `aa481ad20860715112fe9bc696eb7d6bdef1c2ea:.aegis/packages/AXL-V1-A1-P31.md`
- execution branch = `codex/axl-v1-a1`
- execution branch initial revision = `aa481ad20860715112fe9bc696eb7d6bdef1c2ea`

A1 freezes TPM-first / software-fallback CNG scheme IDs, provider pinning, non-exportable persisted P-256 identity, ProgramData + DPAPI machine state, protected cross-process mutex, same-volume crash-safe replace, the transition of `A0FixtureAdapter` to test-only use, and `axlic identity` public semantics.

Blocking evidence is fixed before coding: A0 regression; provider policy matrix; real Software KSP qualification; exact-result-bound real physical TPM 2.0 Windows 11 qualification with reboot persistence; protected-state corruption/version tests; cross-process first-establishment serialization; crash/atomic-replace matrix; and identity CLI/minimum-disclosure evidence. Absence of a physical TPM execution surface is an `ENVIRONMENT_BLOCKER`, not permission to replace the real-platform proof with a mock.

**Disposition:** P31 is CLOSED / READY_FOR_P32. This decision authorizes only `P32 AXL-V1-A1 CODE_EXECUTION`; it does not authorize A2 and does not imply P34 PASS.

## D-100 — AXL-V1-A1 P32 stopped correctly on physical-TPM environment blocker

**State: ACCEPTED P33 RESUME RECONCILIATION / 2026-09-15**

P32 materialized A1 at exact result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25` and returned `BLOCKED_ENVIRONMENT` because frozen T-A1-04 requires a reviewer-accessible Windows 11 + physical TPM 2.0 + Microsoft Platform Crypto Provider + reboot execution surface, while no repository runner currently satisfies that contract.

Remote verification confirms `codex/axl-v1-a1` points to `a1a37d07…`; hosted run `34922762083` has exact `head_sha=a1a37d07…` and conclusion `success`; job `104234284895` succeeded; artifact `10378707796` (`AXL-V1-A1-evidence`) is exact-result-bound with digest `sha256:ad4c57ba4496376ad5ff3e8779d0bd5dca310fcef685180751ac7a5609ae9df4`.

This is **not** a P31 package defect, P20 verification defect, or implementation defect. P33 accepts `a1a37d07…` as the resume cursor and preserves the already completed implementation plus T-A1-01/02/03/05/06/07/08 evidence inputs. The only next executable obligation is T-A1-04 physical TPM qualification using `.github/workflows/a1-physical-tpm.yml` on the exact result revision, including initial phase, real reboot, and post-reboot persistence phase.

A2 remains unauthorized. P34 entry remains unavailable until A1 reaches the frozen P31 terminal-success condition.

## A1 P33 physical TPM closure — 2026-09-16

### D-101 — T-A1-04 physical TPM execution accepted; A1 routes to P34

**State: ACCEPTED / READY_FOR_CONTROL_REVIEW.** P33 final reconciliation verified the returned physical-machine evidence ZIP against the frozen A1 result and accepted T-A1-04 as completed execution input.

- A1 result / execution branch: `codex/axl-v1-a1@a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25` (**EXACT_CURSOR**).
- Evidence ZIP SHA-256: `87347e9feb6715ce9515840ecdddc85280f55baf126eb2c4e79ce1da22914cd7`.
- Correlation ID: `99df2de2-9d87-42ab-909e-1a22d865e704`.
- Reviewer-confirmed environment: Windows 11 Pro build 22631 x64, administrator, physical TPM 2.0, Microsoft Platform Crypto Provider; no obvious VM signal.
- Scheme: `axl-win-cng-tpm-p256-v1`, epoch `1`; public identity SHA-256 remained stable across real reboot.
- Boot marker changed across reboot; post-reboot provider selection, persisted-key sign/verify, private-export rejection, reboot persistence, and software-identity-not-selected checks all PASS.
- T-A1-01 through T-A1-08 are now completed execution inputs for Gate review.
- P34 has **not** yet been evaluated; this decision must not be represented as Gate PASS.
- A2 remains unauthorized until official A1 P34 Gate routing.

**Non-blocking tooling finding:** the evidence ZIP copies the Runtime Kit `SHA256SUMS.txt`, whose entries reference the complete Runtime Kit rather than only files carried by the evidence ZIP. Therefore it is provenance metadata, not a self-verifying evidence-bundle manifest. The independently returned outer ZIP SHA-256, exact source/executable bindings, and frozen oracle outputs provide the applicable evidence binding; this finding does not introduce a new blocking requirement under Gate Closure Stability / Anti-Proof-Recursion.

**Next Primary Owner:** `aegis-gate-review` at **P34 AXL-V1-A1 Gate Review**.

### D-102 — AXL-V1-A1 P34 Gate Review closes PASS_WITH_FINDINGS

**State:** Accepted / Gate-closed / 2026-09-16

P34 independently audited the frozen P31 completion target against three review passes: Frozen Requirement Audit, Frozen Evidence Audit, and Repository Reality Audit. Exact result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25` remains the current `codex/axl-v1-a1` head with no execution divergence. T-A1-01 through T-A1-08 are independently reviewable and PASS; hosted run `34922762083` / job `104234284895` / artifact `10378707796` is exact-result-bound, and the reviewer-confirmed physical TPM reboot bundle SHA-256 `87347e9feb6715ce9515840ecdddc85280f55baf126eb2c4e79ce1da22914cd7` closes T-A1-04.

No frozen requirement failed and no newly discovered high-impact uncovered failure mode was found. Two late findings are non-blocking hardening only: the physical evidence ZIP carries the Runtime Kit `SHA256SUMS.txt` rather than an evidence-bundle-specific manifest, and candidate physical-machine reports retain a static `NOT_VALID_FOR_T-A1-04` notice even after reviewer confirmation. Independent outer ZIP hashing and exact revision/correlation/identity/boot/provider binding close the relevant proof gaps, so neither finding has unique blocking detection value.

**Official A1 Gate verdict:** `PASS_WITH_FINDINGS`.

**P34 record:** [34 — P34 AXL-V1-A1 Gate Review — PASS_WITH_FINDINGS](34%20%E2%80%94%20P34%20AXL-V1-A1%20Gate%20Review%20%E2%80%94%20PASS_WITH_FINDING%203dd4c57a590c81c0af91faaee79aef4a.md)

**Successor boundary:** return to central `aegis` for routing. P34 does not merge the branch or authorize A2. Integration-first handling of the exact Gate-closed A1 result is the expected next decision before any A2 P31 package.

## D-103 — AXL-V1-A1 Repository Integration Closure and A2 successor routing

**State: ACCEPTED / CLOSED — 2026-09-16.**

AXL-V1-A1 P34 Gate-closed result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25` was integrated into canonical `main` through [PR #2](https://github.com/Mostorm-Labs/axlic/pull/2). Merge commit `eb1ab6ecb6afc31a6bc178a671d4f433414ad8c5` has parents `aa481ad20860715112fe9bc696eb7d6bdef1c2ea` and exact Gate-reviewed result `a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`; both the merge commit and Gate-reviewed result resolve to tree `1317485e61181a058f760f9005089607565a4f7f`. Therefore repository integration is conforming and introduces no content drift.

**Successor routing:** P30 dependency authority places A2 after A1. With A1 now Gate-closed and repository-integrated, earliest untrusted downstream layer becomes **P31 AXL-V1-A2 Task Packaging**. `aegis-implementation` is the next Primary Owner. A2 P31 packaging is authorized against canonical `main@eb1ab6ecb6afc31a6bc178a671d4f433414ad8c5`; **P32 coding remains unauthorized** until the A2 `EXECUTION_CLOSURE_CONTRACT` is frozen/materialized.
