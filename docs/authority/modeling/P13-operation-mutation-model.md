---
authority_id: AXL-V1-P13
stage: P13
scope: axlicense
kind: operation
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c81108345fac48ed6d2b8
migration_class: location-only
semantic_change: none
---

# 13 — P13 Operation & Mutation Model — AxLicense V1 v0.1

> ⚙️ **Authority status: Accepted / P13 FR-019 targeted reconciliation + ordinary first-run identity-registration repair 2026-09-12.** 本页冻结 AxLicense V1 的 domain operation / mutation contract：ordinary first-run `RegisterDeviceIdentity`、dynamic Product/Entitlement catalog registration、customer Grant reconciliation、Device binding/activation、successor credential issuance、device credential resolution/install、rehost/recovery/factory lifecycle 的 payload、precondition、atomic commit、ordering、idempotency/replay 与 error behavior。FR-019 将 legacy migration mutations 降为 historical compatibility only，并把 device-facing refresh 与 credential reissuance 分离。它基于 reconciled P02/P03/P10/P11/P12，不定义 REST endpoint、数据库事务技术、消息队列、具体 wire encoding、密码算法或进程/module topology。

## 1. Stage contract

- **Role:** P13 Operation / Mutation Model
- **Authority:** P02 Product Requirements reconciled 2026-09-12；P03 Capability Traceability reconciled；P10 Product Object Model reconciled；P11 Interaction / Behavior reconciled；P12 Semantic Schema reconciled。
- **Objective:** 把 ordinary first-run Device registration、Catalog、Device/Identity、Grant/Binding/Credential、Factory Provisioning、Recovery/Rehost 的 state change 定义成显式 domain operations，使首次身份注册、动态 capability 注册、商业 entitlement 更新、credential refresh/reissue、retry、replay、并发和响应丢失都不会制造第二个 Device、第二份授权、错误 revision、无界 credential churn 或克隆设备身份。
- **Non-goals:** 不冻结 HTTP/REST/gRPC/IPC endpoint、数据库/ORM、transaction engine、KMS/HSM API、TPM/TEE realization、JSON/CBOR/Protobuf encoding、UI/workflow screen。
- **Required analysis:** operation vocabulary；authority/precondition；commit set；revision/ordering；idempotency key；replay；cancel；error contract；cross-operation invariants。
- **Required output:** canonical mutation operations + transient artifact operations + shared payload semantics + error taxonomy。
- **Quality gate:** 每个 durable mutation 必须有唯一 commit point；same logical retry 收敛到同一 outcome；CatalogRevision / authority_revision / credential_generation 不混淆；catalog registration 不自动 grant；device refresh 不自动 reissue；identity-only 与 license-consumption mutation 不混淆；one-active-binding、credential immutability、same-device recovery boundary 可由 operation contract直接判定。
- **Handoff:** P13 FR-019 targeted reconciliation accepted 后 Semantic Foundation Complete。跨越 Modeling → Architecture family 必须先返回中央 `aegis` 做 successor routing；若路由到 P14 targeted System Architecture reconciliation，P14 只能分配 ownership/模块/数据流，不得重新定义本页 mutation semantics。

## 2. Global mutation contract

### 2.1 Operation identity

所有可能触发 durable mutation 的请求必须携带稳定 `correlation_id: CorrelationId`。它代表**一次逻辑 operation**，transport retry、HTTP retry、USB 重复提交、process restart 不得重新生成。

Idempotency identity：

```plain text
OperationIdentity = (operation_kind, correlation_id)
```

规则：
1. 同一 OperationIdentity + canonical payload 等价 → 返回已存在的 committed/rejected deterministic outcome，不重复 mutation。
2. 同一 OperationIdentity + canonical payload 不等价 → `IDEMPOTENCY_CONFLICT`，不得选择其中一个继续执行。
3. transport metadata、retry count、request arrival time 不属于 canonical payload；会改变授权/identity outcome 的字段全部属于 canonical payload。
4. committed result 必须能够在响应丢失后被重新定位；delivery state 不成为第二份 authorization truth。

### 2.2 Commit / cancel

- **Before commit:** validation/cancel/failure 必须是 non-mutating。
- **Commit:** operation 的 required canonical objects、revision changes、capacity consumption 和 `LicenseLifecycleEvent` 作为一个语义原子 outcome 成立。
- **After commit:** cancel 不允许回滚历史；只能执行明确 successor operation。
- **Delivery after commit:** credential/response 未送达不改变 canonical commit；retry 恢复同一 committed outcome。

### 2.3 Audit atomicity

每个 durable mutation 的 lifecycle event 必须与其 canonical mutation 属于同一 logical commit。不得存在“授权已经改变但系统无法形成对应 committed lifecycle event”的合法终态。

### 2.4 Credential issuance atomicity

凡 operation 的合法终态要求 Active DeviceBinding + SignedLicenseCredential，则二者必须属于同一 logical commit outcome。不得出现 authoritative binding 已 Active、但对应 credential issuance 未知/不可恢复的合法终态。

### 2.5 Revision domains

- `CatalogRevision`：catalog canonical mutation 的全局 ordering/synchronization revision；只描述 registry 演进，不是 customer/device license authority。
- `LicenseGrant.authority_revision`：授权评估语义变化时递增；新 grant 从 `1` 开始。
- `DeviceIdentity.identity_epoch`：同一 physical Device 的 current identity successor；从 `1` 开始，只在 continuity 被证明的同一 Device 内递增。
- `credential_generation`：同一 `(license_grant_id, binding_id)` immutable credential successor sequence，从 `1` 开始。
- Provisioning capacity：由 committed operation events 推导，不维护第二个可独立漂移的 remaining counter truth；FR-018 LegacyMigration capacity 仅属 historical compatibility。

## 3. Base operation vocabulary — FR-019 Current Authority additions/overrides are in §34–42

<table>
<tr><th>Operation</th><th>Kind</th><th>Primary durable mutation</th><th>Authorization source</th></tr>
<tr><td><code>IssueLicenseGrant</code></td><td>Canonical</td><td>Create LicenseGrant</td><td>License admin</td></tr>
<tr><td><code>RegisterDeviceIdentity</code></td><td>Canonical</td><td>Ordinary first-run create/confirm Device + current DeviceIdentity only</td><td>Accepted self-registration policy + proof of possession; no commercial/provisioning authority</td></tr>
<tr><td><code>ProvisionDeviceIdentity</code></td><td>Canonical</td><td>Factory/provisioning create/confirm Device + current DeviceIdentity only</td><td>ProvisioningAuthorization <code>identity_provision</code></td></tr>
<tr><td><code>ActivateDevice</code></td><td>Canonical</td><td>Bind grant to device + issue credential</td><td>Activation authority</td></tr>
<tr><td><code>FactoryPreActivateDevice</code></td><td>Canonical</td><td>Issue/confirm grant + bind + credential</td><td>ProvisioningAuthorization <code>factory_pre_activation</code></td></tr>
<tr><td><code>CompleteOfflineActivation</code></td><td>Canonical</td><td>Same authorization outcome as ActivateDevice</td><td>Offline request + activation authority</td></tr>
<tr><td><code>MigrateLegacyDevice</code> — Historical / superseded</td><td>Canonical</td><td>Consume legacy claim + issue/confirm grant + bind + credential</td><td>LegacyMigrationAuthorization</td></tr>
<tr><td><code>RecoverDevice</code></td><td>Canonical</td><td>Credential recovery and/or same-device identity epoch successor</td><td>Current binding + continuity/support authority</td></tr>
<tr><td><code>RehostDevice</code></td><td>Canonical</td><td>Replace old binding with new binding + successor credential</td><td>Internal admin/support</td></tr>
<tr><td><code>ReviseLicenseGrant</code></td><td>Canonical</td><td>Change entitlement/validity/constraint authority; if bound, successor credential is part of the same recoverable logical commit</td><td>License admin</td></tr>
<tr><td><code>ReissueCredential</code></td><td>Canonical</td><td>Issue immutable successor credential on same grant/binding</td><td>Authorized refresh policy</td></tr>
<tr><td><code>DeactivateDeviceBinding</code></td><td>Canonical</td><td>Release current active binding</td><td>Admin/support policy</td></tr>
<tr><td><code>SuspendLicenseGrant</code></td><td>Canonical</td><td>Grant active → suspended</td><td>License admin</td></tr>
<tr><td><code>ResumeLicenseGrant</code></td><td>Canonical</td><td>Grant suspended → active + refresh if bound</td><td>License admin</td></tr>
<tr><td><code>RevokeLicenseGrant</code></td><td>Canonical</td><td>Grant → revoked; active binding → revoked if present</td><td>License admin/security</td></tr>
<tr><td><code>CreateOfflineActivationRequest</code></td><td>Transient/local</td><td>No server canonical mutation</td><td>Device/local activation intent</td></tr>
<tr><td><code>CreateOfflineLegacyMigrationRequest</code> — Historical / superseded</td><td>Transient/local</td><td>No server canonical mutation</td><td>Device/local migration intent</td></tr>
<tr><td><code>InstallSignedCredential</code></td><td>Local observation</td><td>No server canonical mutation</td><td>Valid signed credential</td></tr>
</table>

## 4. Shared payload semantics

```plain text
OperationHeader {
  correlation_id: CorrelationId
  actor: ActorRef
}

NewGrantSpec {
  licensee_id: StableOpaqueId
  entitlements: List<EntitlementGrant>
  binding_policy: single_physical_device
}

GrantTarget {
  kind: existing | issue_new
  existing_license_grant_id?: StableOpaqueId
  new_grant?: NewGrantSpec
}
```

`GrantTarget` 必须 exactly-one：`existing` 只允许 existing ID；`issue_new` 只允许 NewGrantSpec。某 operation 是否允许 `issue_new` 由其 authority scope 决定，不能由 caller 自行选择绕过 policy。

Device identity proof / attestation / activation code / historical order evidence 等可以作为 validation input/reference，但除 P12 已明确 canonical object 外，不因出现在 payload 中自动成为 authorization truth。

## 5. IssueLicenseGrant

### Payload

```plain text
IssueLicenseGrant {
  header
  grant: NewGrantSpec
}
```

### Preconditions

- actor 有 license issuance authority；
- Licensee active；
- entitlement definitions 可识别且 scope/validity 合法；
- duplicate entitlement 不允许。

### Commit

- 创建 `LicenseGrant`，`authority_revision = 1`，state=`active`；
- 不创建 DeviceBinding / credential；
- append `grant_issued` event。

### Idempotency / replay

同一 correlation + same spec 返回同一 grant。不同 spec → `IDEMPOTENCY_CONFLICT`。

## 6. ProvisionDeviceIdentity

### Payload

```plain text
ProvisionDeviceIdentity {
  header
  provisioning_auth_id: StableOpaqueId
  product_id: ProductId
  batch_ref?: OpaqueValue
  device_identity_claim: OpaqueValue
}
```

### Preconditions

- ProvisioningAuthorization active/in-validity；
- 存在匹配 `identity_provision` action grant；
- product/batch/capacity scope 允许；
- identity claim 合法且未映射到另一 non-retired Device。

### Commit

- 新设备：创建 `Device(state=registered)` + `DeviceIdentity(identity_epoch=1)`；
- 已由同一 identity 映射到同一 Device：允许 idempotent existing outcome；
- append `device_identity_provisioned` event；
- 消耗 identity-provision action capacity（若配置）。

### Forbidden mutation

**不得**创建 LicenseGrant、DeviceBinding 或 SignedLicenseCredential；不得消耗 factory-pre-activation/license capacity。

### Conflict

同一 identity scheme/value 已属于另一 non-retired Device → `DEVICE_IDENTITY_CONFLICT`，fail closed。

## 7. ActivateDevice — online

### Payload

```plain text
ActivateDevice {
  header
  license_grant_id: StableOpaqueId
  device_identity_claim: OpaqueValue
  activation_authority_ref: OpaqueValue
}
```

### Preconditions

- activation authority 有效并允许该 grant；
- grant state=`active`；
- identity claim 可创建/确认一个 physical Device；
- grant 无 Active binding，或已经 Active binding 到同一 Device。

### Commit

若 grant 尚未绑定：
1. 创建/确认 Device 与 current identity；
2. `authority_revision += 1`；
3. 创建唯一 Active DeviceBinding；
4. 签发 `credential_generation=1` credential；
5. append activation event。

若 grant 已绑定同一 Device：不得创建第二 binding；返回当前 authoritative outcome。设备只是检查/获取最新 credential 时使用 non-mutating `ResolveCurrentCredential`；只有确有 key/format/recovery reissue need 时才执行 `ReissueCredential`。

### Conflict

绑定到另一 Device → `BINDING_CONFLICT_REHOST_REQUIRED`。

## 8. FactoryPreActivateDevice

### Payload

```plain text
FactoryPreActivateDevice {
  header
  provisioning_auth_id: StableOpaqueId
  device_id: StableOpaqueId
  grant_target: GrantTarget
}
```

### Preconditions

- Device 已存在 current DeviceIdentity；
- authorization 有匹配 `factory_pre_activation` action grant；
- licensee/product/entitlement/validity/pre-activation capacity scope 全部满足；
- identity-provision scope 不能被当作 pre-activation authority。

### Commit

- `issue_new`：在同一 logical outcome 创建新 LicenseGrant revision=1 + Active binding + credential generation=1；
- `existing`：验证 grant 未绑定其他 Device，授权变化时 increment grant revision，建立 Active binding + credential；
- append `factory_pre_activated` event；
- 只消耗对应 pre-activation capacity。

### Recovery

若 identity provisioning 已成功但本 operation 失败，Device 保持 identity-only registered 状态，不回滚 DeviceIdentity。

## 9. Offline Activation operations

### CreateOfflineActivationRequest — transient

产生稳定 request identity；最低语义包含：
- correlation_id；
- DeviceIdentityClaim / proof；
- grant intent / activation authority reference；
- anti-replay nonce/challenge；
- device proof binding request content。

生成/复制/export request **不产生 canonical mutation**。

### CompleteOfflineActivation — canonical

联网侧导入 request 后执行与 `ActivateDevice` 等价的 grant/device/binding validation。

Commit outcome 与 online activation 相同：唯一 binding + signed credential + lifecycle event。

同一 request 重复提交：
- exact same request → 返回原 committed outcome；
- same correlation 但内容变更 → `IDEMPOTENCY_CONFLICT` / `REPLAY_REJECTED`；
- response 丢失 → 重新导出同一 committed credential outcome，不重新 bind。

## 10. Historical — MigrateLegacyDevice (FR-018 superseded for NearHub V1)

### Payload

```plain text
MigrateLegacyDevice {
  header
  migration_auth_id: StableOpaqueId
  device_identity_claim: OpaqueValue
  grant_target: GrantTarget
  historical_claim_ref?: OpaqueValue
}
```

### Preconditions

- LegacyMigrationAuthorization active/in-validity；
- source_ref / optional historical claim 与 authorization 匹配；
- licensee/product/entitlement/capacity scope 允许；
- device identity 可创建/确认 Device；
- 相同 historical claim 未被另一 physical Device 的 committed migration 消费；
- legacy image、MAC、旧 license file、新 keypair 本身不满足 migration authority。

### Commit

1. 创建/确认 Device + current identity；
2. issue/confirm LicenseGrant；
3. 建立唯一 Active DeviceBinding；
4. 签发 SignedLicenseCredential；
5. 消耗一个匹配 migration capacity/claim；
6. append `legacy_device_migrated` event。

### Idempotency

相同 migration logical request retry 返回同一 grant/binding/credential outcome，不重复消费 historical seat/capacity。

同一 claim 被不同 Device 抢占 → `MIGRATION_CLAIM_CONFLICT`。

### Offline channel

`CreateOfflineLegacyMigrationRequest` 只创建 transport artifact；联网侧仍执行同一个 `MigrateLegacyDevice` canonical mutation。Online/offline 不产生两套 migration authority。

## 11. RecoverDevice

```plain text
RecoverDevice {
  header
  device_id: StableOpaqueId
  mode: credential_only | identity_continuity
  new_identity_claim?: OpaqueValue
  continuity_evidence_ref?: OpaqueValue
}
```

### Mode A — credential_only

Precondition：current DeviceIdentity 可证明、current Active binding 仍 authoritative。

Commit：恢复现有 authoritative credential 或签发 same grant/binding successor credential；若新 issuance，则 `credential_generation += 1`，grant authority_revision 不变。

### Mode B — identity_continuity

Precondition：identity material 已丢失/需替换，但受信 continuity evidence 足以证明**仍是同一 physical Device**；`new_identity_claim` 必须存在且不与其他 Device 冲突。

Commit：
1. retire old current DeviceIdentity；
2. 同一 `device_id` 创建 `identity_epoch + 1`；
3. 保持现有 LicenseGrant / binding identity 不变；
4. 若有 Active binding，签发 same binding successor credential（generation +1）；
5. append recovery event。

### Failure boundary

continuity 无法证明 / physical board-device 已替换 → `CONTINUITY_NOT_PROVEN_REHOST_REQUIRED`，不得在旧 device_id 下吸收新 identity。

## 12. RehostDevice

```plain text
RehostDevice {
  header
  license_grant_id: StableOpaqueId
  expected_old_binding_id: StableOpaqueId
  target_device_identity_claim: OpaqueValue
  expected_authority_revision: UInt64
}
```

### Preconditions

- internal admin/support authority；
- expected old binding 正是唯一 current Active binding；
- grant state 允许 rehost；
- target identity 可创建/确认另一 physical Device；
- expected revision 与 current revision 一致。

### Commit — single transition

1. old binding → `replaced`，设置 ended_at；
2. 创建 target Device（若尚未注册）；
3. 创建 new Active binding，old `replaced_by_binding_id = new binding`；
4. `LicenseGrant.authority_revision += 1`；
5. 新 binding credential generation=1；
6. append rehost event。

不得存在 canonical 双 Active binding 中间终态。

### Retry

相同 operation 恢复同一个 new binding/credential；不得连续创建 replacement binding。

## 13. ReviseLicenseGrant

```plain text
ReviseLicenseGrant {
  header
  license_grant_id: StableOpaqueId
  expected_authority_revision: UInt64
  replacement_entitlements: List<EntitlementGrant>
}
```

此 operation 是**完整 entitlement set replacement domain mutation**，不是任意字段 patch。

### Commit

- validate definitions/rights/validity；
- 用 replacement set 更新 grant entitlements；
- authority_revision += 1；
- 若存在 Active binding，同一 logical commit 签发同 binding successor credential，credential_generation += 1，supersedes previous；
- append grant-revised event。

### No-op

replacement set 与 canonical current set 语义等价 → idempotent/no-op，可返回 current outcome，不应制造 credential churn。

## 14. Historical naming — RefreshCredential (Current Authority splits into ReissueCredential + ResolveCurrentCredential)

用途：key rotation、format migration、credential artifact refresh 等**不改变 Grant/Binding authorization semantics** 的 issuance。

### Preconditions

- grant/binding 仍 authoritative；
- refresh reason/policy 被允许。

### Commit

- grant authority_revision 不变；
- same binding credential_generation += 1；
- 新 credential supersedes prior credential；
- append credential-refreshed event。

Retry 不得无限 generation churn；同一 correlation 返回同一 generation。

## 15. Binding / Grant administrative mutations

### DeactivateDeviceBinding

Precondition：target binding 是 current Active binding。

Commit：binding → released；grant authority_revision += 1；append event。不会删除/修改旧 credential bytes；未来重新绑定必须通过新的受控 activation/rehost policy。

### SuspendLicenseGrant

Precondition：grant state=`active`。

Commit：grant → suspended；authority_revision += 1；append event。已离线 credential 不被魔法修改。

### ResumeLicenseGrant

Precondition：grant state=`suspended`。

Commit：grant → active；authority_revision += 1；若有 Active binding，签发同 binding successor credential；append event。

### RevokeLicenseGrant

Precondition：grant 尚未 revoked/retired。

Commit：grant → revoked；authority_revision += 1；若存在 Active binding，则 binding → revoked + ended_at；append event。不得签发新的正向 entitlement credential 来表达 revoke。永久离线旧 credential 的可观测限制继续遵守 P11。

## 16. InstallSignedCredential — local observation contract

该 operation 不改变 server canonical state。

1. verify signature / key / algorithm / schema；
2. verify `device_id` 与本地 current Device identity mapping；
3. verify grant/binding/right/validity semantics；
4. candidate 全部通过后才 replace local current credential；
5. 同一 credential 重复 install → success/no-op；
6. invalid candidate 不得覆盖仍有效的 current credential。

## 17. Ordering and concurrency

1. 对同一 LicenseGrant 的 authorization-changing mutation 必须表现为 serializable ordering；`authority_revision` 是 ordering fence，不是 wall-clock sequence。
2. 显式 admin/rehost/revise operation 携带 `expected_authority_revision`；不匹配 → `STALE_AUTHORITY_REVISION`，non-mutating。
3. Activation/offline/factory/migration 即使 caller 不知道 current revision，commit 时也必须在 authoritative grant snapshot 上重新验证 one-active-binding；并发竞争只有一个 outcome 可以成功建立 Active binding。
4. 同一 Device 的 identity-continuity recovery 以 current `identity_epoch` 为序；并发 successor 只能一个成为 current。
5. Provisioning / migration capacity 在 commit point 检查并消费；并发不得导致 committed usage 超出 scope capacity。
6. credential generation 对同一 binding 严格单调；同一 correlation retry 不得生成新 generation。

## 18. Replay rules

- Online retry：same logical operation → same outcome。
- Offline activation/migration request exact replay：若已 commit，返回同一 outcome；未 commit 则重新验证后最多 commit 一次。
- 修改 request 后复用 correlation → reject。
- 已消费 legacy claim 被另一 Device 使用 → reject。
- factory station replay 已 provisioned device → 返回同一 Device；不得再次消耗 capacity。
- old credential replay/install 到错误 Device → local verification reject。
- old/offline credential 本身 immutable；server supersession/revoke 不能 retroactively 改 bytes。

## 19. Error contract

<table>
<tr><th>Error</th><th>Meaning</th><th>Mutation</th></tr>
<tr><td><code>INVALID_ARGUMENT</code></td><td>Payload/schema/domain invariant invalid</td><td>None</td></tr>
<tr><td><code>AUTHORIZATION_DENIED</code></td><td>Actor/request authority insufficient</td><td>None</td></tr>
<tr><td><code>AUTHORITY_NOT_ACTIVE</code></td><td>Grant/provisioning/migration authority revoked/expired/inactive</td><td>None</td></tr>
<tr><td><code>SCOPE_VIOLATION</code></td><td>Product/licensee/entitlement/batch/action outside authority scope</td><td>None</td></tr>
<tr><td><code>CAPACITY_EXHAUSTED</code></td><td>Factory/migration committed capacity exhausted</td><td>None</td></tr>
<tr><td><code>DEVICE_IDENTITY_SCHEME_NOT_ALLOWED</code></td><td>Submitted identity scheme/assurance is not accepted by the applicable ordinary-registration policy</td><td>None</td></tr>
<tr><td><code>DEVICE_IDENTITY_PROOF_INVALID</code></td><td>Submitted registration proof does not validly demonstrate possession/control of the claimed current identity</td><td>None</td></tr>
<tr><td><code>DEVICE_IDENTITY_CONFLICT</code></td><td>Identity maps to another physical Device</td><td>None</td></tr>
<tr><td><code>BINDING_CONFLICT_REHOST_REQUIRED</code></td><td>Grant already bound to another Device</td><td>None</td></tr>
<tr><td><code>MIGRATION_CLAIM_CONFLICT</code></td><td>Historical claim already consumed by another outcome</td><td>None</td></tr>
<tr><td><code>CONTINUITY_NOT_PROVEN_REHOST_REQUIRED</code></td><td>Cannot prove same physical Device for recovery</td><td>None</td></tr>
<tr><td><code>STALE_AUTHORITY_REVISION</code></td><td>Concurrent authoritative grant mutation occurred</td><td>None</td></tr>
<tr><td><code>IDEMPOTENCY_CONFLICT</code></td><td>Same OperationIdentity reused with different canonical payload</td><td>None</td></tr>
<tr><td><code>REPLAY_REJECTED</code></td><td>Request/proof violates anti-replay contract</td><td>None</td></tr>
<tr><td><code>UNSUPPORTED_VERSION</code></td><td>Cannot safely interpret request/credential semantics</td><td>None</td></tr>
<tr><td><code>SIGNING_FAILED</code></td><td>Required credential could not be created</td><td>No legal partial authorization commit</td></tr>
<tr><td><code>COMMIT_FAILED</code></td><td>Canonical atomic outcome not committed</td><td>None; retry same OperationIdentity</td></tr>
</table>

Errors must be stable domain classes; transport-specific HTTP/gRPC/IPC mapping belongs to later architecture/platform contract。

## 20. Cross-operation invariants

1. **DeviceIdentity existence never implies entitlement.**
2. **Golden Image never carries current Device authority.**
3. **At most one Active DeviceBinding per LicenseGrant.**
4. **Credential bytes are immutable.** Successor = new credential ID/generation。
5. **Binding change and required credential issuance form one logical commit.**
6. **Legacy migration requires LegacyMigrationAuthorization; identity bootstrap is insufficient.**
7. **Identity recovery requires physical continuity; replacement requires Rehost.**
8. **Factory `identity_provision` authority cannot be escalated into `factory_pre_activation`.**
9. **Offline request is not a license.**
10. **Retry/replay never creates additional commercial rights/capacity consumption.**
11. **Committed mutation is auditable; audit event is not a second source of truth.**
12. **Runtime / maintenance / cloud rights remain independently evaluated.**

## 21. Requirement → Operation trace

- FR-001 Signed License → credential-producing operations + `InstallSignedCredential` verification contract。
- FR-002 Device Identity / clone isolation → ordinary first-run `RegisterDeviceIdentity` + factory-specialized `ProvisionDeviceIdentity` + identity portions of activation + `RecoverDevice`；historical migration operations remain superseded。
- FR-003 Online Activation → `ActivateDevice`。
- FR-004 Offline Activation → `CreateOfflineActivationRequest` + `CompleteOfflineActivation`。
- FR-005 Entitlement Evaluation → `IssueLicenseGrant` / `ReviseLicenseGrant` + immutable credential snapshot；runtime query itself is derived read, not mutation。
- FR-006 Rehost/RMA → `RehostDevice`。
- FR-007 Factory → `ProvisionDeviceIdentity` + optional `FactoryPreActivateDevice`。
- FR-008 Right Separation → `IssueLicenseGrant` / `ReviseLicenseGrant` preserve independent entitlement validity。
- FR-009 Admin lifecycle → issue / revise / deactivate / suspend / resume / revoke / rehost operations。
- FR-010 Key/Format Rotation → `ReissueCredential`；普通 device `axlic refresh` 语义由 non-mutating `ResolveCurrentCredential` 承担。
- FR-011 Product Integration → mutation semantics remain product-agnostic；products consume derived license state/entitlement contract。
- FR-012 Recovery → `RecoverDevice`，明确 same-device continuity 与 Rehost boundary。
- FR-013 Legacy Migration → **Historical / superseded for NearHub V1 by FR-019**；不得驱动新的 Current Authority operation design。

## 22. Historical base P13 exit review — superseded by later targeted reconciliations

**Status: ACCEPTED / CLOSED.**
- operation vocabulary 已冻结；
- payload meaning / preconditions / commit set 明确；
- identity provisioning 与 license consumption 已分离；
- factory pre-activation 保留；
- legacy migration 已拥有独立 authority 与 mutation；
- recovery vs rehost boundary 已冻结；
- atomicity / idempotency / replay / ordering / error contract 已建立；
- P10/P11/P12 reconciled invariants 未被 P13 重定义。

**Semantic Foundation Complete:** P10 → P11 → P12 → P13 均已建立 Current Authority。

**Next earliest untrusted layer:** **P14 System Architecture**。

**Next owner:** `aegis-architecture`。

P14 可决定 server/agent/core/portal/process/module/storage/KMS/secure-store ownership 与 runtime data flow，但不得重新定义本页 canonical operation semantics。

## P13 targeted reconciliation — Device Assertion operation semantics — 2026-09-12

### Stage contract

- **Role:** P13 Operation / Mutation Model targeted reconciliation.
- **Authority:** FR-016/FR-017/NFR-SEC-04 + reconciled P10/P11/P12 Device Assertion model/behavior/schema.
- **Objective:** Define how trusted challenges are consumed and how device assertions are created/verified without introducing a canonical ownership or license mutation.
- **Non-goals:** No CLI syntax, HTTP endpoint, QR format, account recovery endpoint, exact crypto, TPM/KSP API, or product association mutation.
- **Required analysis:** operation vocabulary, preconditions, replay/idempotency, failure contract, non-mutation guarantees.
- **Required output:** operations and invariants below.
- **Quality gate:** no operation may expose arbitrary private-key signing or mutate Account/Organization/ProductDeviceAssociation/license state; replay/retargeting fails closed.
- **Handoff:** P14 maps these operations onto `axlic.exe` and server/trust boundaries.

### Operation vocabulary extension

| Operation | Kind | Primary outcome | Canonical mutation |
|---|---|---|---|
| `CreateDeviceIdentityAssertion` | Device-local / transient security operation | Produce purpose-bound assertion for a trusted challenge using current established DeviceIdentity | None |
| `VerifyDeviceIdentityAssertion` | Server-side / transient security verification | Return minimum-disclosure verification result bound to challenge/device/current identity | None |

Neither operation joins the P13 durable mutation family. `OperationIdentity = (operation_kind, correlation_id)` remains required when the transport/workflow needs stable retry identity, but success creates no Device/Binding/Grant/Credential lifecycle mutation.

### CreateDeviceIdentityAssertion

Conceptual input:

```plain text
CreateDeviceIdentityAssertion {
  correlation_id: CorrelationId
  trusted_challenge: DeviceAssertionChallengeEnvelope
}
```

Preconditions:
1. challenge envelope issuer/key/algorithm is trusted and valid;
2. challenge is not expired/revoked/consumed beyond policy;
3. audience/purpose is allowed for the local AxLicense policy;
4. existing local identity association resolves to a current registered Device/current DeviceIdentity;
5. established provider can perform the required proof operation; if the established provider is temporarily unavailable, fail with provider-unavailable/recovery semantics and **do not** select a lower-assurance provider;
6. if challenge carries `expected_device_id`, it must match the established registered Device.

Execution/outcome:
- AxLicense constructs a canonical-to-the-operation proof input from the trusted challenge binding plus current `device_id / identity_epoch / scheme_id` context.
- Identity Provider performs only the AxLicense-defined proof primitive; caller does not supply arbitrary bytes to sign.
- AxLicense emits `DeviceIdentityAssertion` with a fresh assertion identity and bounded expiry.
- No Device, DeviceIdentity, LicenseGrant, DeviceBinding, SignedLicenseCredential or product association mutation occurs.

Retry/idempotency:
- transport retry with the same challenge/correlation may return the same or an equivalent fresh assertion according to policy, but must remain bound to the same challenge/device/purpose/audience/session;
- retry may never broaden scope or select another device/provider;
- if single-use challenge consumption is enforced before assertion creation, retry must recover the same deterministic operation result rather than become an arbitrary new proof opportunity.

### VerifyDeviceIdentityAssertion

Conceptual input:

```plain text
VerifyDeviceIdentityAssertion {
  correlation_id: CorrelationId
  trusted_challenge: DeviceAssertionChallengeEnvelope
  assertion: DeviceIdentityAssertion
}
```

Preconditions/validation:
1. challenge signature/trust and time window valid;
2. assertion schema supported and unexpired;
3. `challenge_id/digest + audience + purpose + session_ref` exactly match;
4. assertion resolves to the expected Device when specified;
5. referenced identity epoch/scheme is valid for the registered Device and acceptable under current verification policy;
6. provider proof verifies against the registered identity authority/reference;
7. replay/single-use policy permits verification for this workflow.

Result:
- return `DeviceAssertionVerificationResult` with minimum-disclosure status, verified `device_id`, bound audience/purpose/session and trusted assurance class when valid;
- invalid/expired/replayed/retargeted proof returns a stable failure code and no canonical mutation.

### Required stable error classes

At minimum:
- `ASSERTION_CHALLENGE_UNTRUSTED`
- `ASSERTION_CHALLENGE_EXPIRED`
- `ASSERTION_CHALLENGE_REPLAYED`
- `ASSERTION_SCOPE_MISMATCH`
- `ASSERTION_DEVICE_MISMATCH`
- `ASSERTION_IDENTITY_NOT_ESTABLISHED`
- `ASSERTION_IDENTITY_PROVIDER_UNAVAILABLE`
- `ASSERTION_IDENTITY_EPOCH_INVALID`
- `ASSERTION_PROOF_INVALID`
- `ASSERTION_SCHEMA_UNSUPPORTED`

These are semantic classes; exact CLI exit-code/JSON mapping belongs to P15/P17.

### Security invariants

1. **No arbitrary signing oracle.** There is no `Sign(data)` operation in the public AxLicense contract.
2. **No ownership mutation.** A valid assertion cannot by itself claim, recover, transfer, reset, rehost, activate, bind or revoke anything.
3. **No license coupling.** Assertion success does not depend on an Active LicenseGrant/DeviceBinding unless a consuming product independently chooses such policy; AxLicense Device registration is sufficient for the proof primitive.
4. **No downgrade.** Existing hardware-bound DeviceIdentity unavailable → provider-unavailable, not software fallback.
5. **Challenge trust is mandatory.** Raw caller-provided `purpose/audience/nonce` is not enough; `axlic.exe` must consume a challenge from a trusted authority/envelope.
6. **Audit distinction.** Assertion attempts may produce security/diagnostic telemetry under safe-logging policy, but are not `LicenseLifecycleEvent` canonical authorization history.

### P13 disposition

**P13 targeted reconciliation ACCEPTED / CLOSED.** Existing durable mutation model remains unchanged; Device Assertion adds two transient security operations only. Semantic Foundation is again complete for this capability. Handoff to central routing: next earliest untrusted layer is **P14 targeted System Architecture reconciliation**.

## Historical — FR-018 targeted Operation / Mutation reconciliation — Legacy Provisional + Human-Assisted Migration

### 24. Scope and operation classes

FR-018 adds a durable **workflow plane** in front of the existing legacy-migration authorization mutation. It does not introduce a second license plane.

Operations are classified as:

| Operation | Class | Durable effect | Entitlement authority? |
|---|---|---|---|
| `CreateLegacyMigrationApplication` | Workflow mutation | Create durable application/case truth | No |
| `UpdateLegacyMigrationApplicationContact` | Workflow mutation | Append contact revision | No |
| `BeginLegacyMigrationReview` | Workflow mutation | `received → under_review` | No |
| `WithdrawLegacyMigrationApplication` | Workflow mutation | Open application → `withdrawn` | No |
| `RejectLegacyMigrationApplication` | Workflow mutation | Open application → `rejected` | No |
| `ApproveLegacyMigrationApplication` | Security-authority mutation | Application → `approved` • issue scoped `LegacyMigrationAuthorization` | Migration permission only; still no runtime entitlement |
| `RevokeLegacyMigrationAuthorization` | Security-authority mutation | Active migration authorization → revoked | Removes unconsumed migration permission; does not revoke a completed license |
| `ResolveLegacyMigrationRedemption` | Transient validation | Resolve/validate scoped code/artifact | No |
| `CreateOfflineLegacyMigrationRequest` — Historical / superseded | Transient/local | Create portable request bound to approved context/device proof | No |
| `MigrateLegacyDevice` — Historical / superseded | Canonical authorization mutation | Grant/Binding/Credential + migration consumption + lifecycle event + application completion | Yes, at canonical commit |
| `InstallSignedCredential` | Local observation | Install verified committed credential | Observes authority; does not create server authority |

### 25. Shared FR-018 operation identities

The existing `OperationIdentity = (operation_kind, correlation_id)` contract remains mandatory for every durable workflow/security mutation.

FR-018 also requires domain-level stable identities:

```plain text
ApplicationDedupeKey = (logical_application_id, device_id, product_id)
MigrationRedemptionScope = (migration_auth_id, application_id, device_id, product_id)
MigrationCommitKey = (migration_auth_id, application_id, device_id, product_id)
```

Rules:
1. Transport retry MUST preserve `correlation_id` and, for application delivery, `logical_application_id`.
2. Server application intake MUST dedupe by `ApplicationDedupeKey` even if the device lost a response or local outbox record.
3. `MigrateLegacyDevice` MUST additionally dedupe by `MigrationCommitKey`, not only by correlation. Re-entering the same valid code/artifact with a fresh UI/session/correlation after a prior successful migration MUST recover the same committed migration outcome rather than create a second grant/binding.
4. Same operation identity with materially different canonical payload remains `IDEMPOTENCY_CONFLICT`.
5. A logical application identity reused for another Device or Product is an identity conflict, not a new application.

### 26. CreateLegacyMigrationApplication

Conceptual payload:

```plain text
CreateLegacyMigrationApplication {
  header
  logical_application_id: LogicalApplicationId
  device_id: StableOpaqueId
  product_id: ProductId
  contact_revision: UInt32
  contact_email: ContactEmail
  predecessor_application_id?: StableOpaqueId
}
```

Preconditions:
- target Device exists with a current DeviceIdentity;
- product is recognized;
- request is eligible to participate in the designated legacy-upgrade workflow; this eligibility still grants no entitlement;
- first contact revision is valid under P12 contact rules;
- if `predecessor_application_id` is present, predecessor is a terminal `rejected` or `withdrawn` application in the same Device/Product lineage.

Commit:
- create one `LegacyMigrationApplication` with `status=received`;
- establish lineage fields;
- append initial contact revision;
- record safe workflow audit metadata;
- **do not** create `LegacyMigrationAuthorization`, LicenseGrant, DeviceBinding, SignedLicenseCredential or LicenseLifecycleEvent.

Idempotency:
- same `ApplicationDedupeKey` + equivalent initial payload → return the existing application;
- same logical ID with different Device/Product → `APPLICATION_IDENTITY_CONFLICT`;
- request arrival duplicated after backend acceptance → return current durable application state, not a second case.

### 27. UpdateLegacyMigrationApplicationContact

Conceptual payload:

```plain text
UpdateLegacyMigrationApplicationContact {
  header
  application_id: StableOpaqueId
  expected_contact_revision: UInt32
  new_contact_email: ContactEmail
}
```

Preconditions:
- application is `received` or `under_review`;
- `expected_contact_revision` equals the current contact revision.

Commit:
- append exactly one new contact revision `current + 1`;
- preserve prior revisions for audit/lineage;
- update `updated_at`;
- no authorization mutation.

Concurrency/retry:
- stale revision → `STALE_CONTACT_REVISION`, non-mutating, return safe current revision metadata;
- same correlation retry returns the same appended revision;
- terminal `approved/rejected/withdrawn/completed` applications are not silently edited. New materially changed contact/evidence after a terminal decision uses a successor application lineage rather than rewriting history.

### 28. BeginLegacyMigrationReview

Conceptual payload:

```plain text
BeginLegacyMigrationReview {
  header
  application_id: StableOpaqueId
}
```

Preconditions: authorized support/customer-success actor; application `received`.

Commit: `received → under_review`; no migration authorization, license or capacity mutation.

Retry of an already `under_review` application is idempotent. A terminal application returns `APPLICATION_ALREADY_DECIDED` or its existing terminal result.

### 29. WithdrawLegacyMigrationApplication

Allowed only while application is `received` or `under_review` and no migration authorization has been issued.

Commit: application → `withdrawn`; preserve contact/history/audit; no authorization mutation.

Rules:
- withdrawal after approval MUST NOT erase or revoke `LegacyMigrationAuthorization`; return `APPLICATION_ALREADY_APPROVED` and require explicit `RevokeLegacyMigrationAuthorization` if cancellation is still appropriate;
- withdrawal after canonical migration commit cannot undo licensing state and routes to ordinary license lifecycle operations.

### 30. RejectLegacyMigrationApplication

Conceptual payload:

```plain text
RejectLegacyMigrationApplication {
  header
  application_id: StableOpaqueId
  decision_contact_revision: UInt32
  decision_reason_code?: OpaqueValue
}
```

Preconditions:
- authorized human reviewer;
- application is `received` or `under_review`;
- `decision_contact_revision` is the reviewer-observed current revision;
- no migration authorization exists for this application.

Commit: application → `rejected`; record decision actor/time/current contact revision and safe reason classification. **No `LegacyMigrationAuthorization` is created.**

A rejected application is terminal. Later reconsideration with new context creates a successor application in the same lineage; it does not flip the old row from rejected to approved.

### 31. ApproveLegacyMigrationApplication

Conceptual payload:

```plain text
ApproveLegacyMigrationApplication {
  header
  application_id: StableOpaqueId
  decision_contact_revision: UInt32
  licensee_id: StableOpaqueId
  entitlement_scope: Set<EntitlementId>
  validity: ValidityWindow
}
```

The exact operator UI may derive `licensee_id`/entitlement policy from an internal template, but the committed authorization scope MUST be explicit and reviewable.

Preconditions:
- actor has human/support migration-approval authority;
- application is `received` or `under_review`;
- application Device has a current identity and Product remains eligible;
- `decision_contact_revision` equals the contact revision reviewed by the human;
- requested Licensee/Product/Entitlement/Validity policy is permitted by admin policy;
- no prior migration authorization has been issued for this application except an idempotent recovery of the same approval outcome.

Single logical commit:
1. create one `LegacyMigrationAuthorization` with `source_kind=support_approval`;
2. set `application_ref = application_id`;
3. set `device_scope = { application.device_id }`;
4. set `product_scope` to the explicitly approved product scope;
5. set `capacity_limit = 1`;
6. persist approved Licensee/Entitlement/Validity scope;
7. application → `approved`, set `migration_auth_id` and `decision_contact_revision`;
8. record security/workflow audit evidence.

This commit **does not** create LicenseGrant, DeviceBinding or SignedLicenseCredential. `approved` is therefore not equivalent to activated/licensed.

Decision immutability:
- same approval retried → return the same migration authorization;
- a different approval scope against an already approved application → `APPLICATION_ALREADY_DECIDED`; do not silently replace authority;
- changing a decision requires explicit revocation plus a successor application/review path, preserving history.

### 32. RevokeLegacyMigrationAuthorization

Conceptual payload:

```plain text
RevokeLegacyMigrationAuthorization {
  header
  migration_auth_id: StableOpaqueId
  reason_ref?: OpaqueValue
}
```

Preconditions: authorized support/security/admin actor; target authorization exists.

Commit while unconsumed: authorization `active → revoked`; append security audit evidence. The associated application may remain historically `approved`; its status is not authority.

Rules:
- repeat revoke is idempotent;
- revoked authorization cannot be redeemed;
- if the associated migration has already committed, revoking the old migration authorization MUST NOT undo Grant/Binding/Credential. Return `MIGRATION_ALREADY_COMMITTED`; use ordinary deactivate/rehost/revoke lifecycle operations if license authority itself must change.

### 33. ResolveLegacyMigrationRedemption — transient validation

This operation validates a user-visible migration code, opaque handle or equivalent `LegacyMigrationRedemptionArtifact` and resolves it to the approved migration context. It performs **no durable license mutation and consumes no migration capacity**.

Validation MUST establish the exact tuple:

```plain text
(migration_auth_id, application_id, device_id, product_id)
```

and verify:
- artifact/handle integrity or trusted server-side lookup;
- authorization exists, active and within validity;
- application matches `application_ref` and is approved (or already completed for outcome recovery);
- caller's current Device equals `device_scope`;
- requested Product equals approved product scope;
- artifact expiry/anti-replay rules where applicable.

Wrong Device/Product/Application MUST fail before capacity consumption.

A successful resolution only says **this Device may attempt the already approved migration**. It still grants no local runtime entitlement.

### 34. CreateOfflineLegacyMigrationRequest — FR-018 extension

The existing transient/local operation remains non-authoritative. For a human-approved FR-018 migration it MUST bind the request to:
- stable request/correlation identity;
- `application_id` and `migration_auth_id` or a trusted scoped redemption representation;
- target `device_id` / current DeviceIdentity proof;
- `product_id`;
- anti-replay challenge/nonce and request integrity proof.

Copying/exporting the request does not consume approval. The connected side ultimately executes the same `MigrateLegacyDevice` canonical semantics. Online and offline migration therefore do not create two different entitlement models.

### 35. MigrateLegacyDevice — FR-018 tightened contract

For the FR-018 human-assisted path, caller-visible code/redemption fields are normalized to the approved `LegacyMigrationAuthorization` before canonical mutation. The device cannot use a code to choose arbitrary Licensee, Product, Entitlement or another target Device.

Additional preconditions:
1. authorization `state=active` and within validity;
2. `source_kind=support_approval` application reference resolves to the same application;
3. application status is `approved`, or `completed` only for committed-outcome recovery;
4. application Device == current proved Device == authorization single-device scope;
5. application Product is allowed by authorization Product scope;
6. migration capacity is still available unless this exact `MigrationCommitKey` already committed;
7. current DeviceIdentity proof is valid and current identity/provider policy is satisfied;
8. no conflicting Active DeviceBinding/Grant outcome exists;
9. user-entered redemption data itself never widens the approved scope.

Canonical commit for a not-yet-completed migration is one logical outcome:
1. create/confirm the target Device/current identity;
2. issue/confirm the LicenseGrant allowed by the approved scope;
3. establish the unique Active DeviceBinding;
4. issue the corresponding immutable SignedLicenseCredential;
5. consume the single migration authorization capacity through the committed migration outcome/event;
6. append `legacy_device_migrated` `LicenseLifecycleEvent`, linking only opaque `application_id` / `migration_auth_id` as needed;
7. application → `completed` and set `completion_event_id`.

There MUST NOT be a legal outcome in which the same approval can commit a second Grant/Binding merely because application completion delivery/update was lost. If workflow and license state are physically stored separately, later architecture must preserve this **single logical commit/fencing invariant**; the committed LicenseLifecycleEvent/MigrationCommitKey wins over stale workflow projection.

### 36. Wrong-device / duplicate / response-loss semantics

- **Wrong Device enters valid code:** `MIGRATION_DEVICE_SCOPE_MISMATCH`; no capacity, application or license mutation.
- **Wrong Product:** `MIGRATION_PRODUCT_SCOPE_MISMATCH`; non-mutating.
- **Code malformed/unresolvable:** `MIGRATION_REDEMPTION_INVALID`; non-mutating.
- **Authorization expired:** `MIGRATION_AUTH_EXPIRED`; non-mutating.
- **Authorization revoked:** `MIGRATION_AUTH_REVOKED`; non-mutating.
- **Approval exists but email was lost:** resend/retrieve redemption representation; do not issue a second migration authorization.
- **Canonical migration committed but response/credential delivery lost:** repeat redemption/request returns the same committed Grant/Binding/Credential outcome.
- **Same Device enters same code again after success:** idempotent recovery/success; no new grant, binding, credential generation or capacity consumption solely because of the retry.
- **Different Device presents a code already completed by Device A:** device-scope rejection; never recover Device A's credential to Device B.
- **Same correlation with altered payload:** `IDEMPOTENCY_CONFLICT`.

### 37. Application status ordering and decision races

Application mutations MUST behave serializably per `application_id`.
- `received → under_review → approved|rejected|withdrawn` are controlled transitions; `completed` is reached only from an approved migration commit.
- Approve vs reject race: only one terminal decision may win. The loser observes `APPLICATION_ALREADY_DECIDED` and the committed decision.
- Contact update vs decision race: `decision_contact_revision` fences the human-reviewed contact snapshot. If contact changes before decision commit, approval/rejection using the stale revision fails `STALE_CONTACT_REVISION` and reviewer must re-read/review.
- Withdraw vs approve race: only one can commit; approval that wins creates durable migration authority, so later withdrawal cannot pretend it never existed.

### 38. FR-018 error-contract extension

<table>
<tr><th>Error</th><th>Meaning</th><th>Mutation</th></tr>
<tr><td><code>APPLICATION_IDENTITY_CONFLICT</code></td><td>Logical application identity reused across incompatible Device/Product context</td><td>None</td></tr>
<tr><td><code>APPLICATION_NOT_FOUND</code></td><td>No durable application for supplied identity</td><td>None</td></tr>
<tr><td><code>APPLICATION_ALREADY_DECIDED</code></td><td>Attempt to mutate/decide a terminal application incompatibly</td><td>None</td></tr>
<tr><td><code>APPLICATION_ALREADY_APPROVED</code></td><td>Withdraw attempted after approval authority exists</td><td>None</td></tr>
<tr><td><code>STALE_CONTACT_REVISION</code></td><td>Contact changed after caller/reviewer snapshot</td><td>None</td></tr>
<tr><td><code>MIGRATION_REDEMPTION_INVALID</code></td><td>Unknown/malformed/untrusted redemption representation</td><td>None</td></tr>
<tr><td><code>MIGRATION_DEVICE_SCOPE_MISMATCH</code></td><td>Approved migration belongs to another Device</td><td>None</td></tr>
<tr><td><code>MIGRATION_PRODUCT_SCOPE_MISMATCH</code></td><td>Approved migration does not cover requested Product</td><td>None</td></tr>
<tr><td><code>MIGRATION_AUTH_EXPIRED</code></td><td>Approved migration authority is outside validity</td><td>None</td></tr>
<tr><td><code>MIGRATION_AUTH_REVOKED</code></td><td>Approved migration authority has been revoked</td><td>None</td></tr>
<tr><td><code>MIGRATION_ALREADY_COMMITTED</code></td><td>Migration already produced canonical license outcome</td><td>No second mutation; return/recover committed outcome when caller is the same scoped Device</td></tr>
</table>

Existing errors such as `IDEMPOTENCY_CONFLICT`, identity/provider failures, capacity exhaustion, binding conflict and signer/credential issuance failures continue to apply.

### 39. Security and authority invariants

1. `Create/Update/Review/Reject/WithdrawLegacyMigrationApplication` can never mint entitlement.
2. `ApproveLegacyMigrationApplication` can mint only a scoped **permission to migrate**, not a runtime license.
3. A redemption code/artifact is not authority independent of its underlying active `LegacyMigrationAuthorization`.
4. Wrong-device validation precedes capacity consumption and license mutation.
5. `MigrateLegacyDevice` is the only FR-018 transition that may create the normal Grant/Binding/Credential entitlement outcome.
6. Application email/contact/CRM grouping never enters LicenseGrant, DeviceBinding or CredentialPayload.
7. A completed migration cannot be undone by editing/withdrawing/rejecting the application or revoking the already-consumed migration authorization.
8. Once migration completes, later missing/corrupt local credential uses normal recovery; it cannot call application creation to regain provisional entitlement.
9. No generic reusable license code is introduced. User-facing redemption remains device/application/product scoped.

### P13 FR-018 disposition

**P13 FR-018 TARGETED RECONCILIATION ACCEPTED / CLOSED.** The existing canonical license mutation model remains intact. FR-018 adds non-authoritative workflow mutations, one human-approval security-authority mutation, scoped redemption validation, and a tightened `MigrateLegacyDevice` contract. The actual entitlement commit still produces the same standard `LicenseGrant + DeviceBinding + SignedLicenseCredential` model.

**Semantic Foundation is complete again through P13 for FR-018.** Crossing from P13 Modeling into P14 Architecture requires central Aegis successor routing; P14 must not redefine the operation/atomicity/idempotency boundaries above.

## 34. P13 FR-019 Targeted Reconciliation — Dynamic Catalog, Grant Reconciliation & Credential Refresh

### 34.1 Current Authority override

本节是 FR-019 / shared AxLicense platform 的 **Current Authority**。此前 §10 与 FR-018 §24–33 仅保留为 historical compatibility / governance record；它们不再属于 NearHub V1 默认 operation vocabulary，也不得被新的 P14–P16 当作当前 runtime path。

本节不改变 P12 字段语义；它只冻结这些 canonical state 如何被显式 mutation。

### 34.2 Current operation vocabulary

Catalog plane：
- `RegisterProductDefinition` — canonical catalog create / idempotent confirm；
- `RegisterEntitlementDefinition` — canonical entitlement registration / idempotent confirm；
- `ReviseCatalogEntry` — 只允许修改 CatalogState / presentation metadata，不允许改写已发布 immutable entitlement semantics。

License authority plane：
- `IssueLicenseGrant`；
- `ReviseLicenseGrant` — **完整 entitlement-set replacement**，是 add/remove/change validity/change constraint 的唯一 canonical Grant-rights mutation；
- `ActivateDevice` / `CompleteOfflineActivation` / `FactoryPreActivateDevice`；
- `SuspendLicenseGrant` / `ResumeLicenseGrant` / `RevokeLicenseGrant` / `DeactivateDeviceBinding`；
- `RecoverDevice` / `RehostDevice`。

Credential plane：
- `ReissueCredential` — 在 Grant authority 不变时真正签发 immutable successor credential；
- `ResolveCurrentCredential` — non-mutating delivery/read，用于 device-facing `axlic refresh` / reconnect / software-update 后检查当前 authoritative credential；
- `InstallSignedCredential` — device-local observation，verify-before-replace + anti-rollback，不产生 server canonical mutation。

Product SaaS / enrollment plane 明确不在本页 operation vocabulary：`CreateEnrollmentSession`、SetupCode、Organization claim、Room assignment、ProductDeviceAssociation、order/payment/SKU/subscription mutation 均由 Product Backend 所有。它们只有在形成受信 commercial decision 后，才调用普通 AxLicense Grant/Activation operations。

## 35. Catalog mutation contracts

### 35.1 RegisterProductDefinition

```plain text
RegisterProductDefinition {
  header
  expected_catalog_revision: UInt64
  product: ProductDefinitionCreateSpec
}
```

Rules：
1. caller 必须是受信 catalog-control actor；普通 device / product client 无注册权限。
2. 若 `product_id` 尚不存在，`expected_catalog_revision` 必须等于 current CatalogRevision；commit 创建 ProductDefinition，并使 CatalogRevision `+1`。
3. 若相同 `product_id` 已存在且 immutable semantics 等价，则 operation 返回 existing/idempotent outcome，不创建第二 definition，也不推进 CatalogRevision；这条规则适用于 release-pipeline replay 和丢失响应后的重试。
4. 同 ID 冲突重定义 → `CATALOG_DEFINITION_CONFLICT`，non-mutating。
5. 注册 ProductDefinition 不创建 LicenseGrant、不修改任何 existing Grant，也不签 credential。

### 35.2 RegisterEntitlementDefinition

```plain text
RegisterEntitlementDefinition {
  header
  expected_catalog_revision: UInt64
  entitlement: EntitlementDefinitionCreateSpec
}
```

Preconditions：
- referenced ProductDefinition exists and is not retired；
- `entitlement_id` prefix、RightKind、`grant_semantics`、`constraint_kind` 满足 P12；
- actor 有对应 product namespace 的 catalog-registration authority。

Commit：
- create EntitlementDefinition；
- CatalogRevision `+1`；
- append audit lifecycle event；
- **不修改任何 LicenseGrant**。

Idempotency / conflict：
- same entitlement ID + identical immutable semantics → existing/idempotent outcome，no revision churn；
- same ID + any incompatible `product_id/right_kind/grant_semantics/constraint_kind` → `CATALOG_DEFINITION_CONFLICT`；
- 已发布 ID 永不通过“更新”改变 authorization-critical meaning；需要新语义时注册新 EntitlementId。

### 35.3 ReviseCatalogEntry

```plain text
ReviseCatalogEntry {
  header
  expected_catalog_revision: UInt64
  target: product | entitlement
  target_id
  target_state?: CatalogState
  display_name?: String
}
```

Rules：
- current CatalogRevision 必须匹配 `expected_catalog_revision`，否则 `STALE_CATALOG_REVISION`；
- immutable authorization semantics 不属于此 operation payload；
- `retired` 是 terminal state；不得恢复 active/deprecated；`deprecated ↔ active` 允许作为治理/发布纠正，因为不改变 machine authorization semantics；
- entitlement 进入 retired 只禁止未来 absent→present assignment，不自动删除 existing Grant 中的 entitlement；
- Product retired 不自动 revoke customer Grant、DeviceBinding 或 credential；商业权利变化必须走显式 Grant/lifecycle operation；
- real metadata/state change commit → CatalogRevision `+1`；语义等价 no-op 不推进 revision。

## 36. ReviseLicenseGrant — single canonical entitlement mutation

P13 Current Authority 保留一个 canonical rights-mutation primitive，而不是同时定义 Add/Remove/SetLimit 多套互相竞争的 mutation：

```plain text
ReviseLicenseGrant {
  header
  license_grant_id: StableOpaqueId
  expected_authority_revision: UInt64
  replacement_entitlements: List<EntitlementGrant>
}
```

`replacement_entitlements` 按 entitlement ID 作为 canonical set 比较；传输顺序不构成授权语义。

### 36.1 Preconditions

1. `expected_authority_revision == current authority_revision`；否则 `STALE_AUTHORITY_REVISION`，non-mutating。
2. 每个 EntitlementGrant 必须满足 P12 definition / validity / constraint closed semantics。
3. Active/deprecated entitlement 可按 policy 被新增；retired entitlement 若当前 Grant 不包含，则不得 absent→present，返回 `RETIRED_ENTITLEMENT_ASSIGNMENT_FORBIDDEN`。
4. 已在 Grant 中存在的 retired entitlement 可以：保持不变、收窄或移除；不得通过 revision 扩张其 validity/constraint 为等价“新 assignment”。
5. duplicate entitlement ID / `bounded_u64` 缺 constraint / presence 带 constraint / value=0 均拒绝。

### 36.2 Commit

若 replacement set 与 current canonical authorization 语义不同：
1. 原子替换 Grant entitlement set；
2. `authority_revision += 1`；
3. 若存在 Active DeviceBinding，则同一 logical commit 产生该 binding 的 successor SignedLicenseCredential：payload authority_revision = 新 revision，`credential_generation += 1`，`supersedes_credential_id` 指向 previous current credential；
4. append grant-revised lifecycle event。

不得出现“Grant rights 已 commit 到新 revision，但 active binding 没有可恢复的对应 successor credential”的合法终态。

### 36.3 No-op / API sugar

- replacement set 与 current authorization 等价 → no-op，authority revision 与 credential generation 都不变；
- 上层 API/UI 可以提供 `add entitlement` / `remove entitlement` / `set max` 等 convenience command，但在进入 canonical mutation 前必须读取 current Grant、形成完整 replacement set，并以 `expected_authority_revision` 提交 `ReviseLicenseGrant`；这些 convenience command 不是第二套 domain mutation truth。

### 36.4 Bulk commercial reconciliation

例如“所有已购 NearCast Pro 客户新增 Miracast”只能是 coordinator：
- 每个 LicenseGrant 生成独立 `ReviseLicenseGrant` OperationIdentity；
- 每个 Grant 独立 expected revision / commit / failure；
- Grant A 成功、Grant B conflict 不得回滚 A；
- batch/job progress 不是 authorization truth。

## 37. Credential operation split

### 37.1 ReissueCredential — canonical issuance

旧 `RefreshCredential` mutation 名称被 Current Authority 拆解并由 `ReissueCredential` 承担真正 re-sign/re-format 行为：

```plain text
ReissueCredential {
  header
  license_grant_id: StableOpaqueId
  binding_id: StableOpaqueId
  expected_authority_revision: UInt64
  expected_current_credential_id: StableOpaqueId
  issuance_profile_ref: OpaqueValue
  reason: key_rotation | format_migration | recovery_reissue | other_approved
}
```

Preconditions：
- grant/binding 仍为 authoritative current pair；
- expected authority revision/current credential 必须匹配；
- requested issuance profile/reason 被 server policy 允许。

Commit：
- `LicenseGrant.authority_revision` **不变**；
- same binding `credential_generation += 1`；
- successor credential 的 entitlement snapshot 必须与同 authority revision 的 current credential **authorization-equivalent**；
- new credential supersedes previous；
- append credential-reissued lifecycle event。

若不存在实际 key/profile/format/recovery reissue need，则应返回 current credential/no-op，不允许每次 device poll 都制造 credential generation churn。

### 37.2 ResolveCurrentCredential — delivery/read, not mutation

Conceptual request：

```plain text
ResolveCurrentCredential {
  device_identity_proof
  license_grant_id?: StableOpaqueId
  observed_binding_id?: StableOpaqueId
  observed_authority_revision?: UInt64
  observed_credential_generation?: UInt64
  observed_credential_id?: StableOpaqueId
}
```

Semantics：
- 验证请求 device 对目标 current binding/credential 的访问资格；
- 若 device 已观察 current authoritative credential → `CURRENT / NO_CHANGE`；
- 若 server 已有 higher authority revision 或 higher generation successor → 返回 current successor candidate；
- **不创建 Grant、不创建 Binding、不推进 authority_revision、不推进 credential_generation、不消耗 license unit。**

因此产品侧 `axlic refresh`、app start、network restored、software updated 都首先是 `ResolveCurrentCredential` 语义，而不是 `ReissueCredential`。

### 37.3 InstallSignedCredential — local observation + anti-rollback

Install candidate 必须：
1. 验证 signature / supported schema / device binding / entitlement semantics / validity；
2. 对同 `(license_grant_id, binding_id)`，candidate `authority_revision < local current` → `CREDENTIAL_ROLLBACK_REJECTED`；
3. same authority revision 且 candidate generation < local generation → reject stale；same credential/generation → idempotent success/no-op；
4. same authority revision + higher generation 只有在 authorization snapshot 等价时才可替换；
5. higher authority revision 可合法表示 upgrade 或 downgrade，不能仅因为本地 rights 变少而拒绝；
6. 全部验证成功后才 atomic replace；任何失败保持原 current credential 不变。

Unknown entitlement ID 本身不使 whole credential invalid；unknown authorization-critical grant semantics/constraint kind 按 P12 fail-closed rule 处理。

## 38. Backend-driven commercial authority

FR-019 下 Purchase / License Key redemption / existing pool assignment / included-hardware rights / grandfathered commercial eligibility 都是 Product Backend 的 commercial decision，不直接对应新的 AxLicense operation kind。

推荐 canonical convergence：
- 未有 Grant → `IssueLicenseGrant`；
- 已有 Grant 权利变化 → `ReviseLicenseGrant`；
- Grant 尚未绑定 → `ActivateDevice` / offline equivalent；
- 已绑定且 rights changed → `ReviseLicenseGrant` 同 commit 生成 successor credential；
- device 尚未观察新 credential → `ResolveCurrentCredential` + `InstallSignedCredential`。

`ProductDeviceAssociation` 成功或 SetupCode 完成不得被翻译成隐式 `IssueLicenseGrant/ActivateDevice`；必须有显式受信 commercial authorization boundary。

## 39. Ordering, concurrency, replay and undo

1. **Catalog ordering:** real catalog mutation 以 CatalogRevision 串行化；stale `expected_catalog_revision` → non-mutating conflict。CatalogRevision 不与 authority_revision / credential_generation 比较。
2. **Grant ordering:** 对同一 LicenseGrant，authorization-changing mutation 必须 serializable；`expected_authority_revision` 是 CAS fence。
3. **Credential ordering:** generation 仅在同 grant/binding 内严格递增；same OperationIdentity retry 返回原 issuance outcome，不生成下一代。
4. **Activation races:** two devices 同时尝试 bind same unbound Grant 时，只有一个可以 commit Active binding；loser 得到 binding conflict，不允许“最后写入覆盖”。
5. **Catalog registration replay:** same ID + same immutable semantics 可跨 correlation 收敛为 existing definition；same ID + conflicting semantics 永远 fail closed。
6. **Resolve replay:** `ResolveCurrentCredential` 是 read/delivery，不需要以重复调用生成 durable mutation；重复调用可安全返回同 current/no-change outcome。
7. **No generic undo/redo:** authorization/catalog history 不使用内存式 undo。错误 mutation 通过新的 successor operation 修正：Grant 用新的 `ReviseLicenseGrant`，binding 用 rehost/deactivate，catalog semantic mistake 用 deprecate/retire old ID + register new ID；不得改写历史 event 或复用旧 entitlement ID 改义。

## 40. FR-019 error additions

新增/明确 domain errors：
- `STALE_CATALOG_REVISION` — catalog CAS 不匹配；
- `CATALOG_DEFINITION_CONFLICT` — stable ProductId/EntitlementId 被请求以冲突 immutable semantics 重定义；
- `CATALOG_ENTRY_RETIRED` — 对 terminal retired entry 执行禁止操作；
- `RETIRED_ENTITLEMENT_ASSIGNMENT_FORBIDDEN` — 把 retired entitlement 新增到此前不含它的 Grant；
- `ENTITLEMENT_CONSTRAINT_INVALID` — grant semantics / constraint kind/value 与 definition 不匹配；
- `STALE_AUTHORITY_REVISION` — Grant CAS conflict；
- `STALE_CREDENTIAL_GENERATION` / `CREDENTIAL_ROLLBACK_REJECTED` — local or server candidate older than authoritative/installed sequence；
- `CREDENTIAL_SNAPSHOT_MISMATCH` — same authority revision 的 reissue candidate 改变了 authorization semantics；
- `NO_CREDENTIAL_CHANGE` — optional informational/no-op result，不是 failure。

所有 errors 的 HTTP/gRPC/CLI exit-code mapping 后置到 architecture/platform contract。

## 41. Legacy operation supersession

NearHub V1 Current Authority 不再包含：
- `CreateOfflineLegacyMigrationRequest`；
- `CreateLegacyMigrationApplication` / contact/review/approval/redemption operations；
- `MigrateLegacyDevice`。

若历史数据/旧部署需要兼容，只能进入隔离的 compatibility path；不得调用这些 historical operation 来为新的 FR-019 device 自动产生 entitlement。历史已售设备的 grandfathered/commercial eligibility 必须先由 Product Backend 得出受信 commercial decision，再走普通 `IssueLicenseGrant` / `ReviseLicenseGrant` / `ActivateDevice`。

## 42. P13 FR-019 disposition

**P13 FR-019 TARGETED RECONCILIATION ACCEPTED / CLOSED.** Current mutation authority 冻结为：
- dynamic catalog registration with stable immutable machine semantics；
- explicit CatalogRevision CAS/ordering；
- one canonical full-set `ReviseLicenseGrant` rights mutation with authority_revision CAS；
- bound Grant rights change 与 successor credential issuance 为一个 recoverable logical commit；
- `ReissueCredential` 与 non-mutating `ResolveCurrentCredential` 分离；
- local install anti-rollback + verify-before-replace；
- Product SaaS enrollment/commercial workflow 与 AxLicense canonical mutations 分离；
- FR-018 legacy mutations historical/superseded for NearHub V1；
- no generic undo/redo; correction is explicit successor mutation。

**Semantic Foundation Complete:** P10 / P11 / P12 / P13 在 FR-019 下均为 trusted Current Authority。

**Handoff:** Modeling family 到此结束。下一步必须返回中央 `aegis` 做 successor routing；不得由本页直接启动 P14 substantive work。若中央路由确认 architecture 是 earliest untrusted layer，则进入 P14 targeted System Architecture reconciliation。

## 43. P13 targeted repair — Ordinary First-Run `RegisterDeviceIdentity`

### 43.1 Scope

This repair closes the architecture-preflight gap recorded by Governance D-075. It adds exactly one canonical operation for ordinary connected first-run identity registration. It does **not** reopen P10–P12, change Device/DeviceIdentity schema, grant commercial entitlement, change factory provisioning authority, or introduce a new enrollment/ownership object.

### 43.2 Operation contract

```plain text
RegisterDeviceIdentity {
  header
  product_id: ProductId
  device_identity_claim: OpaqueValue
  device_possession_proof: OpaqueValue
}
```

`product_id` is a registration-policy context used to select accepted identity scheme/assurance rules. It does not make Device identity product-specific and does not create ProductDeviceAssociation, Licensee, LicenseGrant or entitlement authority.

### 43.3 Authorization source and preconditions

`RegisterDeviceIdentity` is an **ordinary self-registration operation**, not a factory/provisioning operation.

Required preconditions:
1. referenced ProductDefinition / product namespace is recognized for ordinary registration policy evaluation;
2. `device_identity_claim` is structurally valid and uses an identity scheme accepted by the applicable Device Identity policy;
3. `device_possession_proof` proves possession/control of the private identity material corresponding to the submitted claim using the AxLicense-defined registration-proof contract;
4. the same current identity `(scheme_id, identity_value)` is not mapped to another non-retired physical Device;
5. no `ProvisioningAuthorization`, LicenseGrant, activation code, purchase/order evidence or ProductDeviceAssociation is required merely to register identity;
6. transport authentication/rate limiting/abuse controls may be required by later architecture/platform policy, but they are not commercial license authority and cannot change the canonical commit defined here.

### 43.4 Canonical commit

For a previously unknown accepted identity, one logical commit creates:
- one `Device(state=registered)` with a stable `device_id`;
- one current `DeviceIdentity(identity_epoch=1)` bound to that Device;
- one `LicenseLifecycleEvent` / identity lifecycle event with kind `device_identity_registered` and the operation `correlation_id`;
- the committed idempotency result needed to recover the same registration after response loss.

The commit **must not** create or mutate:
- Licensee;
- LicenseGrant / EntitlementGrant;
- DeviceBinding;
- SignedLicenseCredential;
- ProductDeviceAssociation / Organization ownership;
- ProvisioningAuthorization capacity or factory-pre-activation capacity.

Identity registration therefore means only: **AxLicense recognizes this physical-device identity as the same registered Device for later proof/activation/recovery flows.** It does not mean owned, purchased, claimed or licensed.

### 43.5 Idempotency, dedupe and response-loss recovery

1. Same `OperationIdentity` + equivalent canonical payload → return the original committed/rejected deterministic outcome.
2. Same `OperationIdentity` + materially different claim/proof/product context → `IDEMPOTENCY_CONFLICT`, non-mutating.
3. A fresh correlation that presents the same valid current identity must resolve to the same existing `device_id`; it must not create another Device merely because the caller lost its previous correlation/result.
4. If server commit succeeded but the response was lost, retry/re-registration recovers the same Device/current identity result.
5. If the server rejects or is unavailable after local identity material was safely established, the client must retry with that **same** local identity. Server registration failure is never authority to silently generate a replacement key/identity.
6. Repeated observation of an already-registered current identity is a no-op/return-current result; it does not append duplicate registration history solely because transport retries occurred.

### 43.6 Conflict and concurrency

- If `(scheme_id, identity_value)` already maps to another non-retired Device → `DEVICE_IDENTITY_CONFLICT`, fail closed and make no canonical mutation.
- Concurrent `RegisterDeviceIdentity` requests for the same previously unknown identity must serialize on the DeviceIdentity uniqueness boundary: at most one Device is created and all equivalent successful contenders converge to it.
- `RegisterDeviceIdentity` and factory `ProvisionDeviceIdentity` share the same canonical DeviceIdentity uniqueness fence. A race between the two must never create two Devices for one current identity.
- `product_id` is not part of Device identity uniqueness. The same registered physical Device may later be consumed by multiple AxLicense-enabled product namespaces without creating duplicate Device objects.

### 43.7 Relationship to `ProvisionDeviceIdentity`

The two operations intentionally remain separate:

```plain text
RegisterDeviceIdentity
  ordinary connected first-run
  accepted self-registration policy + possession proof
  no provisioning capacity
  no commercial authority

ProvisionDeviceIdentity
  factory / controlled provisioning
  ProvisioningAuthorization(identity_provision)
  optional scoped provisioning capacity
  still no commercial authority
```

Both converge on the same canonical `Device + current DeviceIdentity` model. `RegisterDeviceIdentity` must not be used to bypass factory batch/accountability controls; `ProvisionDeviceIdentity` must not be required for ordinary customer first-run merely to establish identity.

### 43.8 Downstream contract

After successful `RegisterDeviceIdentity`, later flows may use the registered `device_id` / current identity as input to purpose-bound `CreateDeviceIdentityAssertion` / `VerifyDeviceIdentityAssertion`, product-owned SaaS enrollment, ordinary `ActivateDevice`, recovery, or other separately authorized operations. Those later operations retain their own authority and commit boundaries.

```plain text
local identity established
        ↓
RegisterDeviceIdentity
        ↓
Device + current DeviceIdentity
        ↓
(no License / no Organization ownership)
        ↓
purpose-bound Device assertion
        ↓
Product Backend enrollment / claim
        ↓
separate commercial decision
        ↓
ordinary AxLicense activation
```

### 43.9 Stable errors

At minimum, ordinary registration exposes these stable domain classes in addition to the global idempotency/transport-independent classes:
- `DEVICE_IDENTITY_SCHEME_NOT_ALLOWED` — selected scheme/assurance is not accepted for the applicable ordinary-registration policy;
- `DEVICE_IDENTITY_PROOF_INVALID` — claim/proof does not validly demonstrate possession/control of the claimed identity;
- `DEVICE_IDENTITY_CONFLICT` — the same current identity is already mapped to another non-retired Device;
- `IDEMPOTENCY_CONFLICT` — the same operation identity is reused with materially different canonical payload.

All are non-mutating failures. Exact HTTP/CLI/IPC mapping remains downstream architecture/platform work.

### 43.10 Repair disposition

**P13 ORDINARY FIRST-RUN DEVICE REGISTRATION REPAIR ACCEPTED / CLOSED.** D-075's missing-operation defect is resolved without changing P10/P11/P12 semantics or factory authority. P13 is again trusted under the FR-019 baseline.

**Handoff:** return to central `aegis` for successor routing. The expected downstream candidate is the previously blocked **P14 FR-019 targeted System Architecture reconciliation**, but this P13 repair does not itself execute or authorize P14 substantive work.
