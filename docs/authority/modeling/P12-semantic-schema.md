---
authority_id: AXL-V1-P12
stage: P12
scope: axlicense
kind: schema
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c813c93b8cf63f128cf85
migration_class: location-only
semantic_change: none
---

# 12 — P12 Semantic Schema — AxLicense V1 v0.1

> 🧬 **Authority status: Accepted / P12 FR-019 targeted reconciliation 2026-09-12.** 本页冻结 AxLicense V1 的 semantic schema：稳定 identity、动态可注册 Product/Entitlement catalog、字段含义、Grant/Constraint 表达、credential snapshot、revision 分层、版本/兼容和 validation rules。FR-019 明确 SaaS enrollment/ownership 仍在产品域之外；NearHub V1 legacy migration schema 已 supersede 为 historical compatibility context。Catalog registration、commercial grant mutation 与 signed credential issuance 使用彼此独立的 revision 语义。它不选择数据库、ORM、REST/IPC、CBOR/JSON/Protobuf、具体签名算法或平台 secure-store realization。

## 1. Stage contract

- **Role:** P12 Semantic Schema
- **Authority:** P02/P03 reconciled requirement/capability baseline + P10 reconciled Product Object Model + P11 reconciled Interaction / Behavior (2026-09-12)。
- **Objective:** 将已接受对象和行为编码成一个长期可兼容、可离线验证、不会混淆 durable/transient/derived state 的 canonical schema，并允许产品在不同发布时间安全注册新 capability、显式更新 customer Grant、签发 successor credential，而无需把 entitlement 列表写死在 AxLicense 核心代码中。
- **Non-goals:** 不冻结 storage schema、wire encoding、endpoint、database transaction、process/module architecture、crypto primitive、TEE/TPM/Keystore 实现。
- **Quality gate:** stable identity、field meaning、defaults、validation、versioning、compatibility、optionality 明确；任何 unsigned/transient/derived 字段不得成为 entitlement authority。
- **Handoff:** P13 只能基于本页 schema 定义 mutation vocabulary / payload / atomicity / ordering / idempotency，不得重新发明字段含义。

## 2. Canonical truth boundary

### Canonical durable state

V1 canonical authorization world 由以下对象组成：
- `Licensee`
- `ProductDefinition`
- `EntitlementDefinition`
- catalog-wide `CatalogRevision` state（registry ordering/synchronization metadata，不是 license authority）
- `LicenseGrant`
- `Device`
- `DeviceBinding`
- `SignedLicenseCredential`
- `ProvisioningAuthorization`
- `LicenseLifecycleEvent`

`LegacyMigrationAuthorization` / `LegacyMigrationApplication` 等 FR-013/FR-018 legacy-specific schema 对 NearHub V1 不再属于 Current Authority；若已有实现/数据需要保留，只作为 historical compatibility / audit data，不得参与新的 entitlement evaluation。

### Explicitly non-canonical

以下可以序列化或持久化，但**不是授权 source of truth**：
- ActivationSession / RehostSession / IdentityProvisioningSession / FactoryPreActivationSession 状态；FR-018 `LegacyMigrationSession` 仅为 historical compatibility state；
- OfflineActivationRequest 与 transport response wrapper；FR-018 `OfflineLegacyMigrationRequest` 仅为 historical compatibility artifact；
- HTTP retry / job progress / UI state；
- `LicenseStatus` / `EntitlementSnapshot` / `CurrentBinding` 等 derived projection；
- cache、database index、local file path；
- License Key / activation code 等用于证明“允许尝试 activation”的 secret/token；
- SKU display name、Standard/Pro/Ultra 标签。

这些 artifact 的具体 operation payload 在 P13 定义，wire encoding 在后续 architecture/platform contract 定义。

## 3. Normative scalar types

### StableOpaqueId

系统生成 entity identity。语义规则：
- opaque；调用方不得解析其中业务含义；
- globally unique within AxLicense authority domain；
- created 后永不复用，即使对象 retired；
- comparison 为 exact identity equality。

具体 UUID/ULID/其他编码由后续实现决定。

### ProductId

人工治理的稳定 machine identifier。V1 规则：
- lowercase ASCII；
- 推荐单一短 namespace，例如 `nearhub`、`axiom`、`arc`；
- 一旦发布不可 rename/reuse；display name 变化不改变 `product_id`。

### EntitlementId

全局稳定 machine identifier。V1 规则：
- lowercase ASCII dot-separated；
- 必须以其 `product_id + "."` 为 prefix，例如 `nearhub.byod`、`axiom.runtime`、`arc.fastink`；
- 发布后不可改变语义或复用；若语义发生不兼容变化，创建新 entitlement ID。

### Instant

绝对时间点，语义上为 UTC instant。序列化必须无歧义；具体 wire representation 后置。

### CorrelationId / ActorRef

- `CorrelationId`：一次逻辑 interaction 的稳定 retry identity，opaque 且不可因为 transport retry 重建；P13 冻结其 operation 使用规则。
- `ActorRef`：指向 external authentication principal 的 opaque reference；用于 internal admin/support/factory/system audit，不把 named-user licensing 引入 V1。

## 4. Common enums / value semantics

### RightKind

V1 closed set：
- `runtime`
- `maintenance_update`
- `cloud_service`

`RightKind` 属于 **EntitlementDefinition 的稳定语义**。同一个 `entitlement_id` 不得在不同 LicenseGrant 中改变 RightKind。

### ValidityWindow

```plain text
ValidityWindow {
  kind: perpetual | bounded
  not_before?: Instant
  not_after?: Instant
}
```

规则：
- `perpetual`：`not_after` 必须 absent；`not_before` 可 absent；
- `bounded`：`not_after` 必须 present；若 `not_before` present，则 `not_before < not_after`；
- 时间区间语义统一为 `[not_before, not_after)`；
- absent `not_before` 表示签发/授予后立即可用，不代表未知时间；
- wall-clock trust、secure monotonic time 与高可信 offline trial enforcement 留给 architecture/security；schema 只定义时间语义。

### CatalogState

`active | deprecated | retired`

### LicenseGrantState

`active | suspended | revoked | retired`

`issued` 是 lifecycle event，不再作为第二个 ambiguous durable state。创建成功且可授予权利的 grant 进入 `active`。

### DeviceState

`registered | active | replaced | retired | compromised`

其中 `registered` 表示 AxLicense 已承认该 physical Device 与 current identity，可以长期没有任何 LicenseGrant/Active DeviceBinding/credential；`active` 是 device-registry lifecycle state，不等价于“已授权/已激活”。License truth 只能从 Grant/Binding/Credential 判断。

### DeviceBindingState

`active | released | replaced | revoked`

`pending` 不进入 canonical schema；未 commit 的 pending 只属于 interaction/session state。

## 5. Product / entitlement catalog schema

### CatalogRevision

`CatalogRevision = UInt64`，在同一 AxLicense catalog authority domain 内从 1 单调递增。任何 committed canonical catalog mutation（新增 Product/Entitlement definition、CatalogState 变化、canonical catalog metadata 变化）推进 revision。它只用于 registry ordering、cache/sync invalidation、release validation 与审计定位；**不得作为某个 customer/device 是否拥有 entitlement 的依据。**

### ProductDefinition

```plain text
ProductDefinition {
  product_id: ProductId
  state: CatalogState
  display_name?: String
}
```

规则：
- `display_name` 仅 presentation metadata，不参与授权判断或签名 credential entitlement identity；
- ProductDefinition 可以在产品生命周期中后续注册；同一 `product_id` + 相同 immutable semantics 的重复注册必须可视为 idempotent；冲突重定义必须拒绝；
- retired ProductId 永不复用。

### EntitlementGrantSemantics

V1 closed set：
- `presence`
- `bounded_u64`

### EntitlementConstraintKind

V1 closed set：
- `max_u64`

### EntitlementDefinition

```plain text
EntitlementDefinition {
  entitlement_id: EntitlementId
  product_id: ProductId
  right_kind: RightKind
  grant_semantics: EntitlementGrantSemantics
  constraint_kind?: EntitlementConstraintKind
  state: CatalogState
  display_name?: String
}
```

规则：
- `product_id` 必须引用存在的 ProductDefinition；
- `entitlement_id` 必须满足 product prefix rule；
- `product_id`、`right_kind`、`grant_semantics`、`constraint_kind` 在首次成功注册后均属于 immutable semantics；不兼容语义必须创建新的 `entitlement_id`；
- `presence` 要求 `constraint_kind` absent；entry presence 本身即授权；
- `bounded_u64` 要求 `constraint_kind = max_u64`；具体单位/能力含义由稳定 entitlement identity 的定义固定，例如 `nearcast.concurrent_sources` 的值表示该 capability 的最大允许数量；
- 同一 `entitlement_id` + 相同 immutable semantics 的重复注册为 idempotent；同 ID 冲突注册必须 reject；
- `deprecated` entitlement 可继续被既有 grant/credential 引用；`retired` 禁止把该 entitlement 新增到新的/既有 customer Grant，但**不会自动修改已有 LicenseGrant**。已有 Grant 为保持已授予权利，仍可在 key rotation、format migration、recovery 或 ordinary refresh 时把该 retired entitlement 带入 successor credential；只有显式 Grant reconciliation 才能移除客户权利；
- Catalog registration 只改变 catalog truth，不自动修改任何 LicenseGrant。

## 6. Licensee schema

```plain text
Licensee {
  licensee_id: StableOpaqueId
  state: active | inactive | retired
  display_name?: String
}
```

`Licensee` 只承载授权所需最小组织/商业主体 identity。CRM、payment、invoice、named users 不进入 V1 canonical schema。

## 7. LicenseGrant aggregate schema

```plain text
LicenseGrant {
  license_grant_id: StableOpaqueId
  licensee_id: StableOpaqueId
  state: LicenseGrantState
  binding_policy: BindingPolicy
  authority_revision: UInt64
  issued_at: Instant
  entitlements: List<EntitlementGrant>
}

BindingPolicy {
  mode: single_physical_device
}

EntitlementGrant {
  entitlement_id: EntitlementId
  validity: ValidityWindow
  constraint?: EntitlementConstraint
}

EntitlementConstraint {
  kind: max_u64
  value: UInt64
}
```

### LicenseGrant rules

1. `authority_revision` 从 1 开始，任何会改变授权评估语义的 durable mutation 后严格单调递增；具体 mutation 集由 P13 冻结。
2. V1 `binding_policy.mode` 唯一允许值为 `single_physical_device`；未来新增 mode 属于显式 schema evolution，旧客户端不得自行猜测未知 binding policy。
3. `entitlements` 中同一个 `entitlement_id` 最多出现一次。**Entry presence means granted**；V1 不存在第二个 `enabled` Boolean。
4. EntitlementGrant 必须引用可识别的 EntitlementDefinition；`right_kind` 与 `grant_semantics` 从 definition 获得，不允许 grant 覆盖。
5. 对 `grant_semantics = presence`，`constraint` 必须 absent；对 `bounded_u64`，`constraint` 必须 present、`kind` 必须匹配 definition 且 `value >= 1`。需要“无权利”时删除该 entitlement entry，不得以 `value = 0` 创建第二种 disabled truth。
6. Constraint 只能收窄已授予 capability 的 extent；V1 不允许 arbitrary JSON、script、expression、formula 或嵌套 policy object。
7. `runtime` / `maintenance_update` / `cloud_service` validity 独立评估，不存在隐式继承。例如 maintenance 过期不得删除仍 perpetual 的 runtime entitlement。
8. add/remove entitlement、改变 validity、改变 bounded constraint value 等会改变授权评估结果的 durable mutation 必须推进 `LicenseGrant.authority_revision`；仅 catalog 新增 definition 不推进任何现有 Grant 的 revision。
9. `suspended/revoked/retired` 是 server canonical authority；它不会原地改变已经离线的 SignedLicenseCredential bytes，其本地可观测性遵守 P11。

## 8. Device and DeviceIdentity schema

```plain text
Device {
  device_id: StableOpaqueId
  state: DeviceState
  registered_at: Instant
  identities: List<DeviceIdentity>
}

DeviceIdentity {
  identity_epoch: UInt32
  scheme_id: String
  identity_value: OpaqueValue
  established_at: Instant
  retired_at?: Instant
}
```

### DeviceIdentity rules

1. `device_id` 标识 physical device，不等于 OS install ID、filesystem UUID、MAC address 或某个 license file。
2. `scheme_id` 是稳定、版本化的 identity scheme identifier，例如未来可分别指向 fingerprint profile 或 hardware-backed key profile；**P12 不冻结具体 scheme**。
3. `identity_value` 是 scheme 规范化后的 opaque comparison value；client/product 不得解析其硬件属性。
4. `identity_epoch` 在同一 Device 内从 1 单调递增；identity provider migration 可新增 epoch，不要求创建新 `device_id`。
5. 同一 Device 同时最多一个 current identity（`retired_at` absent）。
6. 同一 identity scheme/value 不得同时映射到两个非 retired physical Device；冲突必须 fail closed 或进入 support/security remediation。
7. Golden/factory image 必须是 identity-free template：不得预置 clone 后仍被视为 current 的 DeviceIdentity、device-private identity material、DeviceBinding 或 SignedLicenseCredential。
8. DeviceIdentity establishment 本身不创建 LicenseGrant/DeviceBinding/SignedLicenseCredential，也不授予 entitlement。
9. identity material 丢失时，只有在受信 continuity evidence 证明仍是同一 physical Device 的情况下，才允许在同一 `device_id` 下建立更高 `identity_epoch`；否则必须创建/使用另一 Device 并走 rehost/重新授权。
10. raw hardware inventory、attestation blob、TEE object handle 等是 evidence/platform realization，不是此 canonical schema 的业务字段。

## 9. DeviceBinding schema

```plain text
DeviceBinding {
  binding_id: StableOpaqueId
  license_grant_id: StableOpaqueId
  device_id: StableOpaqueId
  state: DeviceBindingState
  bound_at: Instant
  ended_at?: Instant
  replaced_by_binding_id?: StableOpaqueId
}
```

规则：
- `active` 时 `ended_at` 必须 absent；非 active 时 `ended_at` 必须 present；
- `replaced_by_binding_id` 仅在 state=`replaced` 时允许，并必须引用同一 LicenseGrant 的 successor binding；
- 同一 LicenseGrant canonical state 同时最多一个 active DeviceBinding；
- rehost 创建新 `binding_id`，不改变 `license_grant_id`；
- OS/app reinstall 且 identity continuity 成立时不得仅因为重装而创建新 binding。

## 10. SignedLicenseCredential semantic schema

Signed credential 分成 **protected payload** 与 **signature envelope**。具体 CBOR/JWS/Protobuf 等 wire format 后置，但任何实现必须保证所有会影响 entitlement evaluation 的字段都受完整性保护。

### CredentialPayload

```plain text
CredentialPayload {
  schema_version: SchemaVersion
  credential_id: StableOpaqueId
  license_grant_id: StableOpaqueId
  authority_revision: UInt64
  binding_id: StableOpaqueId
  device_id: StableOpaqueId
  credential_generation: UInt64
  issued_at: Instant
  supersedes_credential_id?: StableOpaqueId
  entitlements: List<CredentialEntitlement>
}

CredentialEntitlement {
  entitlement_id: EntitlementId
  product_id: ProductId
  right_kind: RightKind
  grant_semantics: EntitlementGrantSemantics
  validity: ValidityWindow
  constraint?: EntitlementConstraint
}

SchemaVersion {
  major: UInt16
  minor: UInt16
}
```

### SignatureEnvelope semantics

```plain text
SignatureEnvelope {
  key_id: String
  algorithm_id: String
  signature: OpaqueBytes
  payload: CredentialPayload
}
```

规则：
1. `credential_id` 对一次 signed issuance 唯一且 immutable。
2. `authority_revision` 必须对应签发时 LicenseGrant 的授权语义 revision。
3. `binding_id/device_id` 必须对应签发时唯一 active binding。
4. `credential_generation` 在同一个 `(license_grant_id, binding_id)` 内从 1 严格单调递增；key/format migration、credential refresh 可产生新 generation，而不必创建新 binding。
5. `supersedes_credential_id` 用于显式 successor chain；若 present，必须指向同一 grant/binding 的旧 credential。
6. `entitlements` 是签发时的离线 authority snapshot。每个 entry 必须复制 `entitlement_id + product_id + right_kind + grant_semantics + validity + applicable constraint`，使 client 不依赖在线 catalog 才能评估。
7. snapshot 中 `product_id/right_kind/grant_semantics/constraint kind` 必须与签发时 EntitlementDefinition 一致；server 不得签发矛盾组合。
8. `presence` entry 必须无 constraint；`bounded_u64` entry 必须携带合法 `max_u64` value。duplicate `entitlement_id` 使整个 credential invalid。
9. 对同一 `(license_grant_id, binding_id, authority_revision)`，不同 `credential_generation` 必须表达 authorization-equivalent entitlement snapshot；generation 可因 key/format migration、重签或 delivery recovery 变化，但不得在不推进 `authority_revision` 的情况下改变客户权利。
10. `key_id` 与 `algorithm_id` 必须属于 signature-protected metadata 或以等价方式 cryptographically bound；不得允许攻击者替换算法/密钥选择而不破坏验证。
11. **任何 unsigned field 都不得影响 entitlement、device binding、validity、authority revision 或 product behavior。**
12. exact cryptographic algorithm 不在 P12 冻结；unknown/untrusted `key_id` 或 unsupported `algorithm_id` 必须 fail closed。
13. credential bytes immutable；server-side revoke/suspend/supersession 不修改旧 bytes。

## 11. Credential versioning and compatibility

### Schema version rules

- `major` 改变表示存在旧 verifier 无法安全解释的语义变化；不支持该 major 的 client 必须拒绝 credential。
- `minor` 只允许 backward-compatible additive evolution；旧 verifier 可以忽略它不理解的**非授权关键**新增字段。
- 删除/重定义 required 字段、改变 ID/validity/right semantics、改变 signature coverage 均要求 major bump。
- 新增会影响授权判断但旧客户端无法安全 fail closed 的字段，必须 major bump，不能伪装成 minor additive change。

### Unknown data rules

- missing required field → invalid；
- malformed known field → invalid；
- unknown enum value in authorization-critical field → 至少对受影响 entitlement fail closed；若当前 encoding/parser 无法安全隔离该 entry，则整份 credential fail closed；
- **unknown future entitlement ID 本身不是 credential parse error**。旧产品可以忽略自己不认识的 entitlement entry，并继续评估已知且结构可安全解析的 rights；不得从未知 ID 推导任何未明确请求的能力；
- 应用只能按 exact stable `entitlement_id` 请求能力，不能通过 prefix/模糊匹配把未知 future entitlement 解释成已知权利；
- 新增 entitlement ID 不要求修改 credential schema major。新增新的 `grant_semantics` / constraint kind 属于 authorization-critical schema evolution，只有在旧 verifier 能安全隔离并 deny 受影响 entry 时才允许 backward-compatible evolution，否则必须 major bump；
- unknown non-critical future field 可忽略，但不得覆盖 core field semantic。

### Long-lived deployment rule

AxLicense Server 升级不得假设所有设备同时升级。新版本签发器必须能够在迁移窗口内为仍受支持的 client profile 生成其可验证的 credential major/version；具体 compatibility matrix 在 P17/P20 冻结。

## 12. ProvisioningAuthorization schema

```plain text
ProvisioningAuthorization {
  provisioning_auth_id: StableOpaqueId
  licensee_id: StableOpaqueId
  subject: ActorRef
  state: active | revoked
  validity: ValidityWindow
  product_scope: Set<ProductId>
  entitlement_scope: Set<EntitlementId>
  capacity_limit: UInt64
  batch_ref?: String
  issued_at: Instant
}
```

规则：
- `capacity_limit > 0`；
- expired / exhausted 是由 validity 和已 committed provisioning outcomes 推导的 status，不创建第二套 durable enum；
- scope 必须同时限制 product/entitlement，不能因 product 被允许就自动允许该 product 未来新增的所有 entitlement；
- `subject` 只表示被授权的 external principal/station，不含 master signing key；
- V1 一份 ProvisioningAuthorization 绑定一个 Licensee；不同授权归属使用不同 authorization；
- `batch_ref` 仅审计/生产追踪，不影响 entitlement authority。

## 13. LicenseLifecycleEvent common schema

```plain text
LicenseLifecycleEvent {
  event_id: StableOpaqueId
  event_kind: String
  occurred_at: Instant
  actor: ActorRef
  correlation_id: CorrelationId
  license_grant_id?: StableOpaqueId
  device_id?: StableOpaqueId
  binding_id?: StableOpaqueId
  credential_id?: StableOpaqueId
  provisioning_auth_id?: StableOpaqueId
}
```

规则：
- append-only，event_id 永不复用；
- 本对象记录 committed lifecycle fact；transient retry/error diagnostics 可进入 operational log，但不是 canonical authorization source；
- `event_kind` 的闭集、event-specific payload 与 mutation mapping 由 P13 冻结；
- audit event 不能反向成为当前状态的第二份 authority；当前授权仍由 Grant/Binding/Credential canonical state决定。

## 14. Canonical validation invariants

以下 validation 是 P12 blocking semantic contract：
1. **No orphan refs:** 所有 required refs 必须存在且类型正确。
2. **One active binding:** 一个 LicenseGrant 同时最多一个 active DeviceBinding。
3. **Grant / binding consistency:** credential 的 grant/binding/device 必须形成同一 authoritative chain。
4. **Snapshot consistency:** credential entitlement snapshot 必须对应其 `authority_revision` 的 grant entitlement semantics。
5. **No duplicate entitlement:** grant 与 credential snapshot 内 entitlement ID 均唯一。
6. **Right-kind immutability:** 同一 EntitlementId 不得改变 RightKind。
7. **Validity correctness:** bounded window 必须有合法 `not_after`；时间区间按 `[start,end)` 解释。
8. **Identity uniqueness:** current DeviceIdentity 不得被两个 active/non-retired Device 同时声称。
9. **No clone authority:** cloneable image/file state 不得成为唯一 DeviceIdentity authority。
10. **Credential immutability:** 已签发 payload/signature 不原地改变；变化用 successor issuance。
11. **Monotonic revisions:** `authority_revision` 与 binding-scoped `credential_generation` 不倒退。
12. **Provisioning scope:** committed provisioning 不得超出 licensee/product/entitlement/validity/capacity scope。
13. **Unsigned data non-authoritative:** UI/cache/local filename/transport wrapper/unsigned metadata 永不授予 entitlement。
14. **Perpetual runtime independence:** 有效 perpetual runtime entitlement 的 credential local evaluation 不以 server availability 为 schema 前提。

## 15. Derived evaluation semantics

`LicenseStatus` 与 `EntitlementSnapshot` 仍是 derived，不持久化成第二份 canonical truth。

### hasEntitlement(entitlement_id, now)

在当前 client 可观察信息范围内，只有以下条件全部满足才为 true：
1. 存在 supported credential；
2. signature/protected metadata 验证成功；
3. credential device binding 与当前 DeviceIdentity 匹配；
4. credential 中存在**唯一**目标 `entitlement_id`；
5. entitlement validity 在 `now` 有效；
6. client 已知的 authoritative state 未明确使 grant/right 无效；
7. 对该 right kind 的附加 policy（例如 cloud online condition）满足。

未找到 entitlement、duplicate、malformed、identity mismatch 或授权关键 unknown semantics 均不得推导为 true。

### Known remote authority

本地已知的 server revoke/suspend/successor observation 可以参与 derived LicenseStatus；但它是观测到的 authority projection，不改变已签发 credential bytes。完全离线且从未观察到远端变化的设备仍遵守 P11 offline observability boundary。

## 16. Schema evolution discipline

- machine ID 永不因 display/marketing rename 改变；
- incompatible semantic change 使用新 entitlement ID 或 schema major；
- additive optional metadata 不得改变旧 verifier 的 authorization result；
- 不允许通过 generic `metadata`/`extras` map 偷渡授权逻辑；
- future extension 必须先明确是 canonical、derived 还是 transient，再选择是否进入 schema；
- storage migration、API DTO、wire codec 可以不同，但都必须映射回同一 P12 semantic meaning。

## 17. P12 disposition

**Status: ACCEPTED / CLOSED.** P10 objects 与 P11 behavior 已能无歧义映射到 canonical fields、identity、version、validity 和 validation rules；当前未发现需要返回 P00–P11 的 authority conflict。

### Stable outputs for P13

P13 可以依赖：
- `LicenseGrant` / `Device` / `DeviceBinding` / `SignedLicenseCredential` 的 identity 与字段语义；
- `authority_revision` 与 `credential_generation` monotonicity；
- Entitlement presence semantics、RightKind immutability 与 ValidityWindow；
- one-active-binding、credential immutability、verify-before-replace、offline observability；
- ProvisioningAuthorization scope/capacity semantics；
- lifecycle event common envelope。

### Explicitly still unresolved / not P12 blockers

- exact activation/rehost/provision operation names and payloads；
- transaction/atomicity boundaries as executable mutation contract；
- idempotency key ownership/dedup retention；
- concrete wire encoding / canonical byte serialization；
- concrete signing algorithm / key custody / trust-store update；
- DB schema / REST/IPC/module/process topology；
- platform DeviceIdentity realization。

**Earliest untrusted layer after P12: P13 Operation / Mutation Model.**

## 2026-09-12 Reconciliation Amendment — Normative

> 🧬 本节是 P12 Current Authority 的增量修订。若本节与本页早期 `ProvisioningAuthorization` / factory provisioning / recovery 条款冲突，以本节为准；未冲突部分继续有效。

### A. Provisioning action semantics

```plain text
ProvisioningActionKind = identity_provision | factory_pre_activation

ProvisioningAuthorization {
  provisioning_auth_id: StableOpaqueId
  subject: ActorRef
  state: active | revoked
  validity: ValidityWindow
  action_grants: List<ProvisioningActionGrant>
}

ProvisioningActionGrant {
  action: ProvisioningActionKind
  product_scope: Set<ProductId>
  batch_scope?: Set<OpaqueBatchRef>
  licensee_id?: StableOpaqueId
  entitlement_scope?: Set<EntitlementId>
  credential_validity_scope?: ValidityWindow
  capacity_limit?: UInt64
}
```

Rules:
1. `action_grants` 至少一个，且同一 `(action, product/batch scope)` 不得产生互相矛盾的授权解释。
2. `identity_provision` 只授权建立/确认 `Device + DeviceIdentity`；其 action grant 中 `licensee_id` / `entitlement_scope` / `credential_validity_scope` 必须 absent。
3. `factory_pre_activation` 才允许创建/确认 LicenseGrant、Active DeviceBinding 与 SignedLicenseCredential；该 action 必须有可确定的 `licensee_id` 与 entitlement/right scope。
4. `capacity_limit` 是 **per action grant** 的上限；identity-provision capacity 与 pre-activation/license capacity 不共享隐式计数。实际 remaining capacity 由 committed lifecycle events 推导。
5. 一份 ProvisioningAuthorization 可以同时包含两种 action grant，也可以只包含其中一种；拥有 `identity_provision` 不推导拥有 `factory_pre_activation`。
6. ProvisioningAuthorization state/validity 只决定“是否允许尝试对应 operation”，不直接产生 entitlement。

### B. LegacyMigrationAuthorization schema

```plain text
LegacyMigrationSourceKind = trusted_device_record | licensee_claim_pool | support_approval

LegacyMigrationAuthorization {
  migration_auth_id: StableOpaqueId
  subject: ActorRef
  state: active | revoked
  validity: ValidityWindow
  source_kind: LegacyMigrationSourceKind
  source_ref: OpaqueValue
  licensee_id: StableOpaqueId
  product_scope: Set<ProductId>
  entitlement_scope: Set<EntitlementId>
  capacity_limit: UInt64
}
```

Rules:
1. `capacity_limit > 0`；remaining capacity 由 committed legacy-migration events 推导，不存第二份可漂移 counter truth。
2. `source_ref` 只引用可信 historical commercial/device evidence；raw CRM/order payload 不进入 canonical license schema。
3. legacy image、旧本地 license 文件、MAC、filesystem UUID、仅生成一个新 keypair 都不得单独成为 `source_ref` authority。
4. 一个 migration authorization 可以允许一批已售设备迁移，但每个 physical Device 的 migration commit 必须独立、可审计、可幂等恢复。
5. migration authority 只能在其 licensee/product/entitlement/capacity/validity scope 内创建或绑定授权结果。

### C. Identity-only canonical state

以下 canonical state 明确合法：

```plain text
Device {
  current DeviceIdentity = present
}

Active DeviceBinding = none
SignedLicenseCredential = none
```

该状态适用于：
- factory inventory / identity-only provisioned device；
- legacy device 已完成 identity bootstrap、尚未完成 migration；
- 已注册但尚未被销售/激活的设备。

**不得从 DeviceState=`registered|active` 推导任何 entitlement。**

### D. Identity recovery semantics

- same physical Device continuity 被受信 evidence 证明时：保持 `device_id`，创建更高 `identity_epoch`，退休旧 current identity；既有 Active DeviceBinding 不因 identity rotation 自动变化。
- identity rotation 后需要新的 credential 时，产生同一 grant/binding 的 successor credential；不消耗新的 license unit。
- continuity 无法证明或 physical board/device 被替换时：不得在旧 `device_id` 下添加新 identity；必须走 Rehost / new Device path。

### E. Legacy migration and offline artifacts

`OfflineLegacyMigrationRequest` 属于 transient transport artifact，不是 canonical authority。其重复复制/传输不改变 server state。只有 server-side `MigrateLegacyDevice` commit 才能消费 `LegacyMigrationAuthorization` capacity 并建立 Grant/Binding/Credential outcome。

### F. Schema compatibility impact

本次 reconciliation 不改变 `CredentialPayload` 的既有核心字段语义，也不要求 credential major-version bump：Factory Pre-Activation 与 Legacy Migration 最终仍签发同一种 `SignedLicenseCredential`。新增的 Provisioning / LegacyMigration server-side canonical objects 属于 server authority evolution；若未来需要把 migration/provisioning provenance 放入客户端授权判断，则必须重新评估 schema major/minor compatibility，不得偷偷用 unsigned metadata 改变 entitlement。

## P12 reconciliation closure

- **Status:** ACCEPTED / RECONCILED
- `DeviceIdentity`、`LicenseGrant`、`DeviceBinding`、`SignedLicenseCredential` 的既有分离保持不变。
- 新增 `LegacyMigrationAuthorization` canonical object。
- `ProvisioningAuthorization` 改为 action-scoped，明确 `identity_provision` 与 `factory_pre_activation` 是不同授权动作与 capacity。
- identity-only Device state、same-device identity epoch recovery、legacy migration schema 已冻结。
- **Handoff:** Earliest untrusted layer = **P13 Operation / Mutation Model**。P13 必须为上述 canonical state 定义显式 operation、payload、precondition、atomicity、ordering、idempotency/replay 与 error contract。

## P12 targeted reconciliation — Device Assertion Challenge / Assertion schema — 2026-09-12

### Stage contract

- **Role:** P12 Semantic Schema targeted reconciliation.
- **Authority:** FR-016/FR-017/NFR-SEC-04 + reconciled P10/P11 device-association boundary and behavior.
- **Objective:** Define stable semantics for trusted device challenges, purpose-bound device assertions, and verification results without exposing DeviceIdentity private material or importing product IAM state into AxLicense.
- **Non-goals:** No QR payload schema, Account/Organization schema, HTTP/CLI encoding, exact crypto algorithm, Windows KSP/TPM API, or database table.
- **Required analysis:** identity, scope binding, expiry/replay, proof envelope, verification projection, compatibility.
- **Required output:** semantic schemas and validation rules below.
- **Quality gate:** an assertion cannot be retargeted across audience/purpose/session/device; no field exposes private identity material; assertion is non-canonical evidence.
- **Handoff:** P13 defines create/verify operation semantics and idempotency/replay behavior.

### New normative scalar semantics

```plain text
AudienceId  := stable opaque/machine identifier for the intended consuming backend
PurposeId   := stable machine identifier for the allowed device-proof purpose
SessionRef  := opaque external workflow/session reference
Nonce       := cryptographically unpredictable opaque challenge value
```

Recommended V1 purposes include `device.enrollment`, `association.recovery`, `ownership.transfer`, and `support.verify`; purpose values are authorization-relevant and must not be silently substituted.

### DeviceAssertionChallengePayload

```plain text
DeviceAssertionChallengePayload {
  schema_version: SchemaVersion
  challenge_id: StableOpaqueId
  issuer_id: String
  audience: AudienceId
  purpose: PurposeId
  session_ref: SessionRef
  nonce: Nonce
  expected_device_id?: StableOpaqueId
  issued_at: Instant
  expires_at: Instant
}
```

Rules:
1. `expires_at > issued_at`; challenge lifetime is bounded by policy.
2. `challenge_id + nonce` are unique enough to prevent accidental/replay reuse.
3. `audience`, `purpose`, `session_ref`, `nonce`, time bounds and optional `expected_device_id` are all integrity-protected challenge semantics.
4. `expected_device_id` may be absent when the caller needs AxLicense verification to discover which registered Device answered; when present, verification must fail if the proof resolves to another Device.
5. Challenge contains no username, password, organization secret, license credential, private key, or raw DeviceIdentity value.

### Trusted challenge envelope

```plain text
DeviceAssertionChallengeEnvelope {
  key_id: String
  algorithm_id: String
  payload: DeviceAssertionChallengePayload
  signature: OpaqueBytes
}
```

The envelope proves the challenge was issued by an AxLicense-trusted Challenge Authority or equivalent configured trusted issuer. Challenge signing keys are logically purpose-separated from license-credential signing authority; exact algorithms/key custody are P17 concerns. Unknown issuer/key/algorithm or invalid signature fails closed.

### DeviceIdentityAssertion

```plain text
DeviceIdentityAssertion {
  schema_version: SchemaVersion
  assertion_id: StableOpaqueId
  challenge_id: StableOpaqueId
  challenge_digest: OpaqueBytes
  device_id: StableOpaqueId
  identity_epoch: UInt32
  scheme_id: String
  audience: AudienceId
  purpose: PurposeId
  session_ref: SessionRef
  issued_at: Instant
  expires_at: Instant
  proof: DeviceIdentityProofEnvelope
}

DeviceIdentityProofEnvelope {
  proof_scheme_id: String
  proof_bytes: OpaqueBytes
}
```

Rules:
1. Assertion is **transient evidence**, not a new canonical Device/DeviceIdentity/License object.
2. `proof_bytes` must cryptographically bind the current DeviceIdentity to the exact trusted challenge semantics (directly or through `challenge_digest`) and the assertion's subject fields.
3. `device_id + identity_epoch + scheme_id` select/identify the registered identity context used for verification；raw `identity_value` and provider-private handle are not exposed.
4. `audience`, `purpose`, `session_ref` and challenge binding must exactly match the trusted challenge；retargeting any of them invalidates verification.
5. Assertion expiry cannot exceed challenge/policy bounds.
6. Assertion does not contain Account/Organization ownership truth and cannot authorize password reset/ownership transfer by itself.
7. Provider-specific proof bytes are opaque to consuming products；products must not parse TPM/KSP-specific internals.

### DeviceAssertionVerificationResult — derived/read-only

```plain text
DeviceAssertionVerificationResult {
  valid: Boolean
  device_id?: StableOpaqueId
  identity_epoch?: UInt32
  audience?: AudienceId
  purpose?: PurposeId
  session_ref?: SessionRef
  assurance_class?: hardware_bound | software_persistent
  verified_at: Instant
  failure_code?: StableErrorCode
}
```

Rules:
- Valid result is derived from trusted challenge + registered Device/current identity + proof verification + replay/expiry policy.
- `assurance_class` is derived from AxLicense trusted identity/provider state；caller-supplied assertion text cannot elevate it.
- Invalid results reveal only minimum-disclosure stable failure classification.
- This result is not a license/ownership mutation and may be cached only within the bounded session/policy window.

### Replay and single-use semantics

- `challenge_id` is subject to one-time or bounded-use policy for enrollment/recovery/transfer purposes；successful consumption may be tracked as transient security state rather than canonical license authority.
- Used/expired/revoked challenge, modified challenge envelope, assertion for another session/audience/purpose, non-current/retired identity epoch, or invalid proof fails closed.
- Re-verification for transport retry may return the same deterministic verdict without creating any new canonical object.

### External product schema boundary

`ProductDeviceAssociation`, Account, Organization/Tenant, enrollment/recovery session state, QR handle and account credential-reset state remain **external product schemas**. AxLicense exposes only stable opaque `device_id`/verified assertion context required for the product backend to apply its own association policy.

### Compatibility

Challenge/Assertion use independent schema versioning. Minor evolution may only add fields that cannot cause an old verifier to grant a proof for a broader audience/purpose/device than intended. Unknown authorization-critical semantics fail closed.

### P12 disposition

**P12 targeted reconciliation ACCEPTED.** Existing P12 license schema remains unchanged；Challenge/Assertion are explicitly non-canonical security evidence. Earliest untrusted layer advances to **P13 Device Assertion operation reconciliation**.

## Historical — P12 FR-018 Legacy Provisional Activation & Human-Assisted Migration — superseded by FR-019

> **Not Current Authority for NearHub V1.** 本节仅保留治理历史与可能的既有数据/实现兼容；`LegacyMigrationApplication` / `LegacyMigrationAuthorization` / outbox / provisional state / migration redemption 不得驱动新的 NearHub V1 P13/P14-P16 设计，也不得进入新的 credential entitlement evaluation。

### A. Historical schema boundary

FR-018 **did not change the canonical authorization schema** for `LicenseGrant` / `DeviceBinding` / `SignedLicenseCredential`。

It introduces one additional durable **workflow** entity plus local/transient artifacts:
- **Durable server workflow truth:** `LegacyMigrationApplication` — records a migration application/case, but grants no entitlement.
- **Existing durable authorization truth:** `LegacyMigrationAuthorization` — remains the sole human-approved authority that may permit historical migration.
- **Local durable non-authoritative transport state:** `LegacyMigrationOutboxEntry` — supports offline submission/retry.
- **Transient/non-canonical representation:** migration redemption code/handle/response wrapper.
- **Derived product runtime state:** `LegacyProvisionalState`.

Critical invariant:

> `LegacyMigrationApplication`, email/contact data, provisional state, outbox state, approval notification and redemption presentation MUST NOT enter `CredentialPayload` or become an alternate entitlement source.

### B. New scalar / value semantics

#### ContactEmail

`ContactEmail` is personally identifying workflow/contact data, not license identity.

Rules:
- must be syntactically acceptable for customer-contact delivery;
- must never be interpreted as `Licensee`, Account, Organization/Tenant, Device ownership or historical entitlement evidence;
- email domain/request volume may be used for **non-authoritative customer-development grouping only**;
- raw email must not be copied into SignedLicenseCredential or ordinary license lifecycle evidence.

#### LogicalApplicationId

A stable opaque identity for one logical migration application intent.

Rules:
- created once when the user first submits that logical application, including offline submission;
- transport retry, process restart, reconnect and lost HTTP response MUST reuse the same identity;
- deleting/recreating local transport files does not prove the server never received the application;
- server deduplication is keyed by this stable logical identity plus bound Device/product context, not by email text alone.

### C. LegacyMigrationApplication schema

```plain text
LegacyMigrationApplicationStatus =
  received |
  under_review |
  approved |
  rejected |
  withdrawn |
  completed

LegacyMigrationApplication {
  application_id: StableOpaqueId
  logical_application_id: LogicalApplicationId
  lineage_root_application_id: StableOpaqueId
  predecessor_application_id?: StableOpaqueId

  device_id: StableOpaqueId
  product_id: ProductId

  contact_history: List<LegacyMigrationContactRevision>
  status: LegacyMigrationApplicationStatus

  migration_auth_id?: StableOpaqueId
  decision_contact_revision?: UInt32
  completion_event_id?: StableOpaqueId

  created_at: Instant
  updated_at: Instant
}

LegacyMigrationContactRevision {
  revision: UInt32
  email: ContactEmail
  recorded_at: Instant
  actor_ref?: ActorRef
}
```

#### Application identity / lineage rules

1. `application_id` is the durable server workflow entity identity; `logical_application_id` is the retry/dedup identity carried from the device-side logical intent.
2. First application in a lineage has `lineage_root_application_id = application_id` and no predecessor.
3. A rejected/withdrawn application that is later retried with materially new evidence/contact context SHOULD create a successor application with:
   - same `lineage_root_application_id`;
   - `predecessor_application_id` pointing to the previous terminal application;
   - a new `application_id` and logical application identity.
4. A terminal rejected/withdrawn application is not silently rewritten into “approved” merely because the customer changes email; approval must follow an explicit reopened/successor workflow defined by P13.
5. Multiple transport retries of the same open application MUST NOT create a new lineage member.

#### Contact revision rules

1. `contact_history` is ordered by strictly increasing `revision`, starting at 1.
2. Highest revision is the current contact view for a non-terminal application.
3. A late retry carrying an older contact revision MUST NOT overwrite a newer server-held revision.
4. `decision_contact_revision`, when present, records which contact revision was current when the human decision was committed; later contact edits cannot erase or reinterpret that decision.
5. Contact history is workflow/audit data only; it never affects entitlement evaluation.

#### Status rules

- `received` — backend durably accepted/deduped the application; no entitlement.
- `under_review` — human/support review in progress; no entitlement.
- `approved` — workflow decision says an associated `LegacyMigrationAuthorization` has been issued; **the status itself is not authority**.
- `rejected` — review rejected the application; no authorization mutation.
- `withdrawn` — customer/support closed the contact request before approval; no authorization mutation.
- `completed` — a migration canonical commit associated with the application has occurred; `completion_event_id` may reference that outcome for workflow traceability, but Grant/Binding/Credential remain the authorization source of truth.

Validation:
- `migration_auth_id` MUST be present for `approved` and `completed`.
- `completion_event_id` MUST be present for `completed`.
- `migration_auth_id` absent for `received`, `under_review`, `rejected`, `withdrawn`.
- Application status never authorizes local runtime by itself.

### D. Local LegacyMigrationOutboxEntry schema

`LegacyMigrationOutboxEntry` is **device-local durable transport state**, not server canonical state and not license authority.

```plain text
LegacyMigrationOutboxEntry {
  logical_application_id: LogicalApplicationId
  device_id: StableOpaqueId
  product_id: ProductId

  contact_revision: UInt32
  contact_email: ContactEmail

  created_at: Instant
}
```

Rules:
1. The tuple `(logical_application_id, device_id, product_id)` identifies the logical submission for retry/dedup purposes.
2. Connectivity loss, process restart and retry preserve `logical_application_id`.
3. If the user edits the contact address while the application is still only local/pending, increment `contact_revision` and replace the pending contact snapshot for that same logical application.
4. Retry scheduling, backoff counters, last transport error, filesystem path and queue implementation are runtime/storage realization and **not P12 semantic fields**.
5. Successful backend acknowledgement may remove the local outbox entry without deleting the durable server application.
6. Loss/deletion of a local outbox entry MUST NOT be interpreted as proof that no server application exists.

### E. LegacyMigrationAuthorization FR-018 extension

Existing `LegacyMigrationAuthorization` remains the sole durable human-approved migration authority. FR-018 extends its schema with optional request/device scoping:

```plain text
LegacyMigrationAuthorization {
  migration_auth_id: StableOpaqueId
  subject: ActorRef
  state: active | revoked
  validity: ValidityWindow
  source_kind: LegacyMigrationSourceKind
  source_ref: OpaqueValue

  application_ref?: StableOpaqueId
  device_scope?: Set<StableOpaqueId>

  licensee_id: StableOpaqueId
  product_scope: Set<ProductId>
  entitlement_scope: Set<EntitlementId>
  capacity_limit: UInt64
}
```

Additional rules:
1. For FR-018 human-assisted single-device approval (`source_kind = support_approval`):
   - `application_ref` MUST reference the approved `LegacyMigrationApplication`;
   - `device_scope` MUST contain exactly the approved current `device_id`;
   - `capacity_limit` MUST be 1;
   - `product_scope` MUST include only the approved product scope for that application/use case.
2. Approval for Device A MUST NOT be redeemable by Device B.
3. Wrong-device/product/request redemption MUST fail **before** consuming migration capacity.
4. `application_ref` links workflow evidence to authorization but does not make the Application itself authority.
5. Existing trusted-device-record / claim-pool migration authorizations may omit `application_ref` or use broader device scope according to their accepted source semantics; FR-018 does not redefine those existing modes.

### F. Migration redemption semantic representation

A customer-facing “activation code” is a **scoped redemption representation**, not a reusable license key and not a SignedLicenseCredential.

Conceptual semantics:

```plain text
LegacyMigrationRedemptionArtifact {
  schema_version: SchemaVersion
  redemption_ref: StableOpaqueId

  migration_auth_id: StableOpaqueId
  application_id: StableOpaqueId
  device_id: StableOpaqueId
  product_id: ProductId

  issued_at: Instant
  expires_at?: Instant

  secret_or_handle: OpaqueValue
}
```

Rules:
1. The visible form may be a short human-entered code, opaque handle, URL token or equivalent; P12 does not freeze wire encoding.
2. If the visible code is only an opaque lookup key, the server-side authoritative binding MUST still resolve to the exact `migration_auth_id + application_id + device_id + product_id` tuple.
3. The artifact grants no entitlement until `MigrateLegacyDevice` successfully commits Grant/Binding/Credential authority.
4. A wrong-device/product/application attempt is rejected without consuming the underlying authorization.
5. Transport timeout or lost response does not create a second authorization or second migration.
6. After migration commit, repeating the same redemption must recover/return the same committed outcome rather than creating a second Grant/Binding.
7. The redemption artifact does **not** own an independent durable `consumed` Boolean. Consumption is derived from the underlying `LegacyMigrationAuthorization` capacity plus committed migration outcome/lifecycle event. This avoids a second counter/state that can drift.
8. Derived redemption status may be presented as `redeemable | consumed | expired | revoked`, but this is a projection, not a separate authority object.
9. For a still-air-gapped target, use the existing `OfflineLegacyMigrationRequest` / signed offline response semantics; a short online redemption code is not magically converted into an offline master license.

### G. LegacyProvisionalState — derived only

`LegacyProvisionalState` MUST remain a product/runtime projection, never a canonical license object.

Conceptual projection:

```plain text
LegacyProvisionalState {
  eligible: Boolean
  watermark_required: Boolean
  application_status?: LegacyMigrationApplicationStatus
  reason?: String
}
```

Eligibility may be true only when all of the following are known:
1. product rollout provenance indicates the designated legacy in-place upgrade path;
2. current DeviceIdentity has been established;
3. no valid current SignedLicenseCredential is locally observable;
4. there is no known completed migration or ordinary AxLicense activation for this Device/product that should instead enter recovery;
5. current rollout policy explicitly permits provisional use.

Rules:
- `watermark_required = true` whenever provisional use is active.
- Absence of a credential or absence of a server Device record **alone is never sufficient** to infer provisional eligibility.
- If the system knows the Device previously completed migration/activation but credential state is now missing/corrupt, route to ordinary recovery; do not regenerate provisional entitlement.
- If previous migration state is ambiguous after local-state loss, the system must not infer a fresh legacy right merely from missing files; it should require recovery/server/support resolution.
- `LegacyProvisionalState` MUST NOT appear inside `CredentialPayload`, `EntitlementGrant`, `DeviceBinding`, or any field used by local entitlement evaluation.

### H. Privacy / audit separation

1. Raw contact email belongs to migration workflow storage only.
2. Ordinary `LicenseLifecycleEvent` SHOULD reference opaque `application_id` / `migration_auth_id` when linkage is needed rather than copying raw email.
3. Diagnostics/support bundle treat contact email as sensitive-by-default; exact redaction/retention policy belongs to P17/P18.
4. CRM/customer-success grouping may consume application/contact metadata, but CRM classifications never become license authority.

### I. Compatibility impact

FR-018 does **not** require any change to `CredentialPayload` and therefore does not require a credential schema major/minor bump solely for this feature.

The following remain unchanged:
- Grant/Binding/Credential separation;
- one-active-binding invariant;
- entitlement snapshot semantics;
- device-bound offline verification;
- credential generation / authority revision ordering;
- existing online/offline activation semantics.

If a future design attempts to place provisional status, migration email, customer case status or human-review metadata into client authorization evaluation, that is a new authorization-schema change and MUST reopen P12 compatibility review.

### P12 FR-018 targeted disposition

**Historical status: previously accepted under FR-018; superseded for NearHub V1 by FR-019.**

Frozen outcomes:
- `LegacyMigrationApplication` has stable server workflow identity/status/contact lineage but remains non-authoritative.
- `LogicalApplicationId` provides retry/dedup continuity from offline outbox through backend case creation.
- contact updates are revisioned; stale retries cannot overwrite newer contact data.
- FR-018 support approval extends existing `LegacyMigrationAuthorization` with explicit application/device scope; single-device approval is capacity 1.
- customer-facing migration code/handle is only a request/device/product-scoped redemption representation; consumption is derived from canonical migration commit, not a second Boolean truth.
- `LegacyProvisionalState` is derived only and never enters SignedLicenseCredential.
- existing credential schema is unchanged.

**Historical routing only.** 该 FR-018 route 已被 FR-019 supersede；新的 P13 不得为 NearHub V1 继续设计 application/review/redemption/`MigrateLegacyDevice` operations，除非未来另一个 accepted consumer 独立重新提出该需求。

## P12 FR-019 Targeted Reconciliation — Dynamic Entitlement Registry, Revision Separation & Credential Refresh

### 19.1 Authority boundary

本节是 NearHub V1 / shared AxLicense platform 的 **Current Authority**，覆盖 P11 FR-019 + Catalog Evolution Addendum 的 schema consequences。它不新增 SaaS IAM、billing、SKU packaging 或 deployment scheduler schema。

### 19.2 Product-owned SaaS state remains external

以下不是 AxLicense canonical license fields：
- `Account` / `Organization` / `Tenant` / `Room`；
- `ProductDeviceAssociation`；
- `EnrollmentSession` / `SetupCode` / QR handle；
- purchase/order/subscription/license-pool UI state。

因此 `claimed + unlicensed`、`claimed + server licensed + device pending`、`unclaimed + locally licensed` 等均是**跨系统 derived/composite projection**，不得新增为 AxLicense `LicenseGrantState` / `DeviceState` / `CredentialState` enum。AxLicense 只持有它自己的 Device / Grant / Binding / Credential truth。

Product Backend 可以向 AxLicense operation 提交受信 commercial decision / grant-assignment intent；其外部 order/account/support evidence 只作为 operation authorization / audit provenance。P13 可以定义 opaque external reference，但它**不得进入 CredentialPayload 作为 entitlement authority**。

### 19.3 Registry registration semantics

动态 registry 的 canonical identity 仍是 `ProductDefinition` / `EntitlementDefinition`。注册一个新的 definition：
1. 必须使用稳定 `ProductId` / `EntitlementId`；
2. 成功后推进 `CatalogRevision`；
3. 不创建、修改或批量补齐任何 `LicenseGrant`；
4. 不签发任何 `SignedLicenseCredential`；
5. 同 ID + 相同 immutable semantics 可幂等返回 existing definition；同 ID + 冲突 immutable semantics 必须 fail closed。

`CatalogState` 变化也不自动改变既有 Grant：catalog retirement 是“不能继续用于新的商业授予”的 registry policy，不是远程撤销所有历史 customer rights。

### 19.4 Three revision domains are intentionally independent

| Revision | Scope | When it advances | What it must never mean |
|---|---|---|---|
| `CatalogRevision` | 整个 Product/Entitlement registry | committed catalog mutation | 客户已经获得该 capability |
| `LicenseGrant.authority_revision` | 单个 LicenseGrant | 任何改变该 Grant 授权评估结果的 durable mutation | 第几次签名文件/第几个 catalog 版本 |
| `credential_generation` | 单个 `(license_grant_id, binding_id)` 的 signed artifact succession | 每次 successor credential issuance | 商业授权一定发生变化 |

Normative rules：
- 三者不得相互赋值、比较大小或假设同步推进；
- 新 entitlement definition 注册：通常只推进 `CatalogRevision`；
- 将新 entitlement 加到 Customer Grant：推进该 Grant 的 `authority_revision`；
- 为已绑定设备签发 successor credential：推进 `credential_generation`；
- key rotation / wire-format migration / delivery recovery 可以只推进 `credential_generation` 而保持 `authority_revision` 不变；
- **CatalogRevision intentionally does not need to appear in CredentialPayload**。credential 必须依赖其自包含 entitlement snapshot，而不是依赖设备在线同步 catalog。

### 19.5 Grant semantics and bounded constraints

V1 Current Authority 支持两种 closed grant semantics：

```plain text
presence
bounded_u64
```

`presence` 适合：

```plain text
nearcast.protocol.miracast
nearcast.protocol.dlna
nearhub.signage.advanced
```

`bounded_u64 + max_u64` 适合：

```plain text
nearcast.concurrent_sources = 4
nearhub.signage.zones = 4
```

Rules：
- absence = not granted；禁止额外 `enabled=false`；
- bounded value 必须 `>= 1`；
- constraint 的单位/商业含义属于稳定 entitlement semantic，不允许 runtime 动态解释 arbitrary unit；
- V1 不支持 string/object/json/expression/script policy；未来需要新 semantics 时必须显式 schema evolution。

### 19.6 Signed credential snapshot is self-contained

`CredentialEntitlement` 必须携带足够信息让设备离线评估：

```plain text
entitlement_id
product_id
right_kind
grant_semantics
validity
constraint?  // only when required by grant_semantics
```

设备**不需要在线查询 EntitlementDefinition catalog** 才能决定当前 credential 中已知 entitlement 的授权结果。Server 在 issuance 时负责保证 snapshot 与当时 canonical definition / Grant 一致。

Catalog 后续新增、deprecated、retired 或 display-name 修改不会改变已经离线保存的 credential bytes。已有权利只通过显式 Grant mutation + successor credential 发生变化。

### 19.7 Unknown-future-entitlement compatibility

为了允许产品与 AxLicense 独立发布：
- 一个支持当前 credential schema major 的旧客户端，看到自己不认识的 future `entitlement_id` 时，不得因此把整份 credential 判 invalid；
- 未知 entitlement entry 不授予任何已知 capability；旧应用只按自己明确请求并理解的 exact entitlement ID 求值；
- 不允许 prefix、通配、SKU label 或 display name 推导 entitlement；
- 如果客户端遇到未知 authorization-critical `grant_semantics` / constraint kind，至少该 entitlement 必须 fail closed；只有当 parser/encoding 能安全隔离该 entry 时，其他已知 rights 才可继续求值，否则整份 credential fail closed；
- 新增新的 entitlement ID 本身不是 credential schema major change；新增新的 grant/constraint semantics 必须重新做 compatibility judgement。

例如旧 NearCast 可安全读取包含：

```plain text
nearcast.protocol.airplay
nearcast.cast.multiscreen
nearcast.protocol.miracast   // old version does not understand this ID
```

并继续使用前两个已知 entitlement，而不会凭空启用 Miracast。

### 19.8 Successor credential / refresh anti-rollback semantics

设备本地安装 candidate credential 时，在 signature/schema/device/binding/validity 验证之外，还必须遵守同一 Grant/Binding 的 revision ordering：
1. exact same `credential_id` 重复导入 → idempotent success/no-op；
2. candidate `authority_revision` 低于当前已安装 credential → stale/rollback candidate，拒绝替换；
3. candidate `authority_revision` 更高 → 可以代表 entitlement add/remove/validity/constraint/downgrade，验证通过后成为新的 authoritative local snapshot；
4. candidate 与当前 `authority_revision` 相同但 `credential_generation` 更低 → stale，拒绝；
5. 同 revision、同 generation、不同 credential identity/bytes → inconsistent，fail closed；
6. 同 `authority_revision`、更高 `credential_generation` 只允许 authorization-equivalent snapshot，用于重签、key/format migration、delivery recovery 等；
7. verify-before-replace：任何 candidate 失败都必须保留当前仍有效 credential。

这些规则不意味着永久离线设备知道 server 上是否已经有更高 revision；只有在 online refresh / offline import 等 observable lifecycle point 才能比较 candidate 并更新。

### 19.9 Dynamic feature release example

```plain text
T0:
CatalogRevision = 20
nearcast.protocol.airplay registered
Grant G1 authority_revision = 5
Credential generation = 3

T1: NearCast releases DLNA
register nearcast.protocol.dlna
CatalogRevision = 21
G1 remains authority_revision = 5
Device rights unchanged

T2: commercial policy adds DLNA to G1
G1 authority_revision = 6

T3: successor credential issued
credential_generation = 4
authority_revision = 6
snapshot now includes nearcast.protocol.dlna

T4: device refresh/import
verify → atomic replace → local DLNA entitlement becomes observable
```

这保证 software release、catalog registration、commercial availability、customer assignment 与 device observation 可以分别发生而不互相冒充 authority。

### 19.10 Legacy schema supersession

For NearHub V1 Current Authority：
- `LegacyMigrationAuthorization`
- `LegacyMigrationApplication`
- `LegacyMigrationOutboxEntry`
- `LegacyProvisionalState`
- migration redemption/code/contact/review lineage

均不再属于新的 canonical/runtime schema 设计输入。若已有数据需要兼容，必须隔离为 historical namespace / compatibility handling；不得被新 entitlement registry、Grant reconciliation、CredentialPayload 或 local entitlement evaluator 读取为授权依据。

### 19.11 P12 disposition

**P12 FR-019 TARGETED RECONCILIATION ACCEPTED.** 当前 schema authority 冻结为：
- dynamic Product/Entitlement Registry with stable immutable machine semantics；
- `CatalogRevision` / `authority_revision` / `credential_generation` 三域严格分离；
- cross-product entitlement + presence / bounded_u64 closed semantics；
- self-contained SignedLicenseCredential entitlement snapshot；
- future entitlement IDs safely ignorable by older consumers while unknown authorization semantics fail closed；
- explicit successor credential refresh / anti-rollback ordering；
- SaaS association/commercial workflow externalized；
- legacy migration schema superseded for NearHub V1。

**Earliest untrusted downstream layer:** **P13 Operation / Mutation Model targeted reconciliation**。

P13 必须据此定义至少：catalog register/deprecate/retire、Grant entitlement add/remove/change-constraint、successor credential issuance/refresh、idempotency/dedup、revision precondition/conflict 与 retry semantics；不得重新定义 P12 字段含义。
