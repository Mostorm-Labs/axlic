---
authority_id: AXL-V1-P02
stage: P02
scope: axlicense
kind: requirement
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c81c79a58e2c7098c8648
migration_class: location-only
semantic_change: none
---

# 02 — P02 Product Requirements — AxLicense V1 v0.1

> 📋 **Status: Accepted requirement baseline / reconciled 2026-09-12.** 本页将 P00 已验证问题翻译为 AxLicense V1 FR/NFR/acceptance criteria；本次 reconciliation 纳入已售存量设备迁移、Golden Image Identity-Free、Identity Provisioning 与 License Activation 分离，以及 update/reinstall/recovery 的 identity continuity 约束。

## 1. Jobs to be done

- **Product runtime:** 我需要用一个稳定 API 判断当前设备是否拥有某项 entitlement，而不关心授权是在线、离线还是工厂完成的。
- **Customer / installer:** 我需要在有网时快速激活；无网时也能通过可理解的 request/response 流程完成激活。
- **Factory:** 我需要在统一 Golden Image 批量复制后，为每台目标物理设备建立独立身份；对指定出厂即授权产品，还需要在身份建立后批量完成可选的 Factory Pre-Activation，避免终端用户逐台激活。Identity Provisioning 本身不得隐式消费 LicenseGrant。
- **Legacy field device / customer:** 对已经售出的历史设备，我需要通过软件升级建立 AxLicense DeviceIdentity，并在有网或离线场景下迁移既有商业授权；仅生成新身份不得自动获得历史 entitlement。
- **Support:** 我需要在同一物理设备的 identity material 丢失时执行受控 recovery，在 RMA / 主板或整机更换时执行 rehost，并保留审计记录。
- **License admin:** 我需要签发、查看、撤销/失效、迁移和配置 entitlement，而不接触客户端实现细节。

## 2. Functional requirements

### FR-001 Signed License Artifact — MUST

系统必须产生版本化、可持久化、可离线验签的 license artifact。任何 payload 修改必须导致验证失败。

**Acceptance:** 已签发 license 在断网环境可验证；bit-level tamper 或未知 signing key 必须被拒绝。

### FR-002 Device Identity & Binding — MUST

系统必须将 device-bound license 绑定到稳定 physical-device identity，并阻止直接复制 license、系统镜像或 identity material 到另一设备后继续被视为同一授权设备。DeviceIdentity establishment 与 LicenseGrant consumption 是两个独立 lifecycle fact。

**Acceptance:** Device A 的 license 在 Device B 上验证失败；Golden/factory image 可重复克隆，但镜像本身不得携带 current DeviceIdentity、device private identity material、DeviceBinding 或 SignedLicenseCredential；目标设备 materialize/首次 identity provisioning 后形成独立 identity。仅建立 DeviceIdentity 不产生 entitlement。

### FR-003 Online Activation — MUST

联网设备可以向 AxLicense 服务提交 activation，并获得可本地验证的 license。

**Acceptance:** 首次 activation 成功后断网重启仍可运行；重复/非法 activation 根据 policy 拒绝。

### FR-004 Offline Activation — MUST

无外网设备可以导出 activation request，并在另一联网环境换取 response/license 后导入。

**Acceptance:** 全流程不要求目标设备连接 Internet；导入后与 online activation 使用相同 runtime verification 语义。

### FR-005 Entitlement Evaluation — MUST

License 必须表达 product/feature entitlement；产品通过稳定标识查询 capability 是否可用。

**Acceptance:** SKU 变化只需改变后台 entitlement mapping，不要求产品代码判断商业套餐名称。

### FR-006 Rehost / RMA — MUST

系统必须支持将有效授权从旧设备受控迁移到新设备，并防止旧/新设备在 policy 不允许时同时持有同一 device entitlement。V1 rehost authority 由内部 admin/support 持有，客户自助迁移不属于 V1。

**Acceptance:** 完成 rehost 后，新设备可获得授权；旧设备在其下一次可观测 lifecycle point 进入无效状态或被记录为不可再签发。

### FR-007 Factory Identity Provisioning & Optional Pre-Activation — MUST

系统必须支持 OEM/ODM production station 在 Golden Image 已写入目标物理设备之后，为每台设备独立执行 Identity Provisioning；Identity Provisioning 只建立/登记 Device identity，不得天然创建 DeviceBinding、SignedLicenseCredential 或消耗 LicenseGrant。对明确要求“出厂即 licensed”的产品/批次，系统必须允许在 Identity Provisioning 成功后继续执行 Factory Pre-Activation，由受限 factory authorization 在其 scope 内完成 LicenseGrant/Binding/Credential outcome。工厂不得获得 master signing private key。V1 默认 production station 可联网访问 AxLicense Server；完全离线工厂 delegated signing 不属于 V1。

**Acceptance:** 同一个 Golden Image 连续复制到多台设备后，每台目标设备形成不同 current identity；只完成 Identity Provisioning 的库存设备没有 Active DeviceBinding/credential；执行 Factory Pre-Activation 的指定设备可在出厂后离线启动并获得 entitlement；factory authorization 可按 product/batch/capacity/time scope 单独撤销；任一 retry 不得重复注册 physical Device、重复消费 LicenseGrant 或重复消耗 factory capacity。

### FR-008 Runtime / Maintenance / Cloud Right Separation — MUST

永久运行权、软件升级权和云服务权必须能够分别表达。V1 默认 perpetual runtime 可永久离线运行；maintenance/update right 与 cloud-service right 独立。

**Acceptance:** maintenance 到期不能使已授权旧版本 runtime 失效；cloud entitlement 到期不应使本地 appliance 基础能力变砖。

### FR-009 License Lifecycle & Admin — MUST

后台至少支持 issue、inspect、activate/provision、deactivate/rehost、revoke/disable 与最小 audit trail。

**Acceptance:** 每个影响授权归属的操作可追溯 actor/time/device/license/result。

### FR-010 Key / Format Rotation — MUST

License 必须包含 format version 与 signing key identifier，使新旧 signing keys/formats 可在迁移期并存。

**Acceptance:** 增加新 key 后旧合法 license 仍可验证；被明确退役的 key 可按迁移策略停止接受。

### FR-011 Stable Product Integration API — MUST

NearHub、Axiom/Arc 等产品不得直接依赖后台数据库或 signing implementation；必须通过稳定状态与 entitlement contract 集成。

**Acceptance:** 产品至少能够查询 license status、license metadata 和 `hasEntitlement(id)`；activation UI 与 product feature logic 可分离。

### FR-012 Device Identity Persistence & Local State Recovery — MUST

系统必须定义 software update、OS/app reinstall、factory reset、license corruption、identity/secure-store loss 与 physical replacement 的不同语义。普通 update/reinstall/factory reset 在能够维持 physical-device identity continuity 时不得天然创建新的 Device 或消耗新的 license unit；identity material 丢失但仍能以受信 evidence 证明同一 physical Device 时走 controlled recovery；无法证明 continuity 时不得仅凭旧 license 文件恢复；physical replacement 必须走 Rehost。

**Acceptance:** 普通 update/reinstall 后仍能识别为原 Device 时保留原 binding；复制普通 identity/license 文件到另一机器不能恢复授权；identity loss 有明确 support-controlled recovery path；整机/主板替换不会被误判为 same-device recovery。

### FR-013 Legacy Device Migration — MUST

系统必须支持已售且历史版本尚无 AxLicense DeviceIdentity 的设备，通过软件升级建立新的 DeviceIdentity，并将其与已有商业授权进行受控迁移。迁移必须同时支持 online 与 offline/air-gapped 设备。Legacy identity bootstrap 可以自动发生，但 historical entitlement binding 必须由可验证的 migration authority/evidence 驱动，例如可信销售/设备记录、客户/管理员 claim capacity 或 internal support approval。

**Acceptance:** 已售设备升级后无需召回即可形成 DeviceIdentity；有网设备可在线完成 migration，离线设备可通过 request/response artifact 完成；拥有旧 IMG、复制旧文件或仅生成一个新 keypair 不足以获得历史 LicenseGrant；同一 migration 重试幂等且不会重复消费历史 seat/claim capacity；迁移期间允许先建立 identity 而不立即强制 license enforcement，避免升级后设备突然不可用。

### FR-014 Device Identity Provider Discovery — MUST

AxLicense 必须在建立或恢复 DeviceIdentity 前确定当前平台**实际可用**的 identity provider 与其安全能力，而不是仅根据产品型号、配置文件、TPM/TEE 名称或“硬件理论上存在”推断。Provider qualification 必须能够区分 present、usable、temporarily unavailable、unsupported，并输出稳定 provider/scheme identity 与 capability result。

**Acceptance:** Windows V1 能在运行时判定 TPM-backed provider 是否真正可用于创建/加载/使用设备身份；仅“TPM present”但 provider 不可用时不得误判为 hardware-backed。Capability probe 失败不得产生新 DeviceIdentity 或授权 mutation。

### FR-015 Identity Assurance Selection & Fallback — MUST

AxLicense 必须根据 provider capability 与 product policy 选择满足最低 assurance 的最强可用 Device Identity Provider。V1 Windows 默认优先 hardware-backed TPM provider；当 TPM 不存在或在首次 identity establishment 时不可用，允许回退到 `software_persistent`，前提是 identity material 在 cloneable image boundary 之外持久化，并满足普通 reboot/software update/支持的 reinstall/reset 流程中的 continuity 要求。若连最低 software-persistent 条件都无法满足，则 production identity establishment 必须 fail closed。

**Acceptance:** 首次建立 identity 时 TPM usable → 选择 hardware-backed；TPM unavailable/unusable → 仅在 software-persistent minimum policy 满足时 fallback；两者均不可用 → 返回 unsupported，不签发 production DeviceIdentity。已建立 hardware-backed identity 的设备后续 TPM 暂时不可用时，不得静默切换到 software provider 或创建第二个 Device。

## 3. Non-functional requirements

### NFR-SEC-01 Cryptographic safety

只使用成熟标准密码原语；master signing private key 不得进入客户端、工厂镜像或普通工程工具。

### NFR-SEC-02 Image-clone and identity isolation

Golden Image 必须是 identity-free template。Device private identity material/current DeviceIdentity/DeviceBinding/SignedLicenseCredential 不得作为可克隆母盘内容；device-specific identity material 必须位于 clone/materialization 之后建立的 per-device trust/persistence boundary。具体 TPM/TEE/Secure Element/software secure store realization 留给后续 architecture/platform authority。

### NFR-SEC-03 No silent identity-security downgrade

已经建立的 DeviceIdentity 必须保持 provider/scheme continuity。`hardware-backed → software-persistent`、受信 provider → 较低 assurance provider 的变化不得因 runtime probe failure、TPM 暂时不可用、驱动故障或软件升级而自动发生。此类变化只能由显式 controlled recovery / identity-provider migration policy 授权，并必须保持同一 physical Device 的 continuity evidence 与 audit linkage。

### NFR-AVL-01 Offline availability

对 perpetual local runtime license，AxLicense Server outage 或 Internet outage 不得阻止正常启动和 entitlement verification。

### NFR-COMPAT-01 Backward compatibility

License format/API 必须支持长期设备生命周期；升级 AxLicense 服务不能要求所有已部署设备同步升级。

### NFR-PLAT-01 Windows-first V1 with portable provider boundary

V1 **production implementation priority = Windows 10/11**。Windows 必须实现 Device Identity Provider discovery、hardware-backed TPM path、software-persistent fallback、diagnostics 与 recovery integration。Linux ARM64/RK、Android 及其他平台在 V1 只要求保留 platform-neutral provider/capability contract、scheme extensibility 与 semantic compatibility；**不要求在 V1 实现 TPM/OP-TEE/RPMB/Android Keystore provider**。后续平台实现不得改变既有 DeviceIdentity/Binding/License semantics。

### NFR-PERF-01 Startup overhead

License local verification 不应成为 appliance 启动可感知瓶颈；正常 entitlement 查询应为纯本地快速路径。

### NFR-OPS-01 Operability

Activation/provision/rehost 失败必须提供可诊断 error code 与最小安全日志，不得要求查看 signing secret 或数据库内部状态才能排障。

### NFR-OPS-02 Identity capability diagnostics

Device Identity subsystem 必须提供可诊断但不泄露秘密的 capability/probe 结果。至少可观察：selected provider/scheme、assurance class、hardware-bound 是否成立、private material 是否可导出、persistence class、probe status/failure reason，以及当前状态是 usable / temporarily unavailable / unsupported。Diagnostics 不得输出 private key、可重建 private material 的 secret 或把 transient probe result 当作新的 canonical DeviceIdentity authority。

### NFR-PRIV-01 Minimal identity collection

Device Identity 只收集授权与反克隆所需最少硬件/密钥信息；不把终端用户个人身份作为 V1 前提。

## 4. Windows-first Device Identity Assurance Policy — V1

### Assurance classes

- **`hardware_bound`** — identity private material 由 Windows hardware-backed provider 持有，目标是 non-exportable + physical-device bound；V1 优先选择。
- **`software_persistent`** — 设备本地生成 identity key/material，存储在 cloneable Golden Image 之外的 per-device persistent boundary；允许作为 Windows V1 production fallback，但 assurance 低于 hardware-bound。
- **`unsupported`** — 无法提供至少 software-persistent continuity；不得建立新的 production DeviceIdentity、Factory Pre-Activation 或新 production binding。

V1 不把 hardware attestation 作为所有授权的必需条件；接口允许未来新增 `hardware_attested` assurance，而不改变 V1 grant/binding/credential semantics。

### Provider selection policy

1. **Existing identity wins.** 若 Device 已建立 current provider/scheme，优先加载并验证该 identity；不得每次启动重新选择“当前最强 provider”。
2. **First establishment only chooses provider.** 新 Device 首次建立 identity 时执行 capability discovery。
3. **Windows preference:** usable hardware-backed TPM provider → `hardware_bound`；否则若满足 persistent-storage minimum → `software_persistent`；否则 `unsupported`。
4. **No silent downgrade.** 已建立 `hardware_bound` identity 后 TPM/provider temporarily unavailable → `IDENTITY_PROVIDER_UNAVAILABLE`，进入 retry/recovery，不自动生成 software identity。
5. **Explicit migration only.** provider/scheme 变化必须走 controlled identity-provider migration / `RecoverDevice` 语义，并在同一 physical Device continuity 可证明时增加 identity epoch；无法证明则走 Rehost/重新授权。
6. **Product minimum policy extensible.** V1 默认普通 Windows activation/legacy migration 最低允许 `software_persistent`；未来特定产品/批次可声明 minimum=`hardware_bound`，但该 policy 不能靠客户端私自降级。

### Platform boundary reserved for future

Architecture/Platform Contract 应预留统一 provider 能力，而 V1 只实现 Windows provider：`probeCapabilities`、`createIdentity`、`loadIdentity`、`signProof`、`getDiagnostics`、`recover/migrate` 的语义边界。Linux TPM、OP-TEE/RPMB、Android Keystore/StrongBox 等仅作为未来 provider，不在 V1 implementation scope。

## 5. V1 exclusions

Floating license、usage metering、payment/billing、reseller、named user、完整 customer portal、general DRM/anti-debug、复杂 SaaS subscription engine 均不进入 V1。Linux/RK/Android Device Identity provider 的具体实现、OP-TEE/RPMB/HUK qualification、Android Keystore/StrongBox attestation 也不进入 V1；仅保留接口与扩展 contract。

## 6. Requirement freeze disposition

**P02 targeted reconciliation freeze conditions 已满足。**

- License unit 已接受：V1 默认一台 physical device 对应一个授权实例。
- Offline policy 已接受：perpetual local runtime 不要求周期联网；cloud-service right 独立。
- Rehost authority 已接受：V1 由内部 admin/support 执行。
- Factory connectivity 已接受：V1 production station 在线调用 AxLicense Server；不做 delegated offline signing。
- Factory lifecycle 已接受：Identity Provisioning 与 Factory Pre-Activation 分离；前者不天然消费 License，后者仅对明确要求出厂即 licensed 的产品/批次执行。
- Golden Image policy 已接受：image identity-free；device-specific identity 在目标物理设备 materialize 后建立。
- Legacy migration 已接受：已售设备可升级建立 identity；historical entitlement 必须经 migration authority/evidence 绑定，不能由 identity bootstrap 自我授权。
- Recovery boundary 已接受：same-device continuity → recovery；physical replacement → rehost；无法证明 continuity 时 fail closed。
- Windows-first identity policy 已接受：V1 production implementation 只要求 Windows 完整落地；其他平台只保留 provider/capability contract。
- Provider selection 已接受：首次 identity establishment 选择最强满足 policy 的 provider；Windows 默认 TPM hardware-backed → software-persistent fallback → unsupported。
- Minimum assurance 已接受：普通 Windows V1 activation/legacy migration 默认允许 software-persistent；特定产品未来可提高到 hardware-bound。
- No silent downgrade 已接受：既有 hardware-backed identity 的 provider 暂时不可用不触发自动降级或新 identity。
- Maintenance semantics 已接受：runtime、maintenance/update、cloud-service rights 分离。
- P01 未发现需要改变产品方向的外部约束。
- 所有 MUST requirements 均有可验证 acceptance criterion。
- Cryptographic profile、hardware-backed identity、secure-store realization 等仍明确留给后续 architecture/security authority。

**Disposition: P02 DEVICE-IDENTITY POLICY RECONCILED / ACCEPTED / READY FOR P03 targeted traceability reconciliation.**

## Targeted requirement reconciliation — Device Assertion & Account/Organization Recovery — 2026-09-12

### Product boundary decision

AxLicense may provide trusted **device identity proof** to product backends, but it does not become the account/password/organization ownership system. `DeviceIdentity` proves which registered physical device is participating; it does not by itself prove that the human scanning a QR code is the legitimate owner or organization administrator.

The durable `Device ↔ Account / Organization` association remains owned by the consuming product/backend (for example Launcher/NearHub management). AxLicense supplies a purpose-bound proof primitive that such a backend may use during enrollment, account recovery, binding recovery, support verification, or other high-risk device workflows.

### FR-016 Purpose-Bound Device Identity Assertion — MUST

AxLicense must allow a trusted backend to challenge a registered device and obtain a short-lived proof-of-possession bound to the existing `DeviceIdentity`. The assertion/proof must be scoped to an explicit `audience`, `purpose`, `nonce/challenge`, target device identity/reference, and expiry; it must not expose the private identity key or provide a generic arbitrary-signing oracle to callers.

**Acceptance:** a trusted Launcher/NearHub backend can prove that a recovery/enrollment request originated from the registered Device without learning or exporting its private key; replaying an expired/used challenge, changing audience/purpose, or using the proof for a different device/session is rejected; `axlic.exe` does not expose `sign arbitrary-data` or equivalent generic private-key access.

### FR-017 Device Enrollment / Association Recovery Integration — MUST

AxLicense must support a product backend using Device Identity Assertion as one input to a device enrollment or account/organization recovery workflow. Device registration/identity establishment and account/organization association are separate lifecycle facts.

The required ordering is:

1. establish/register DeviceIdentity and canonical Device record;
2. allow the device to exist in an **unclaimed / unassociated** product state;
3. start an explicit enrollment/recovery session;
4. prove current device possession through AxLicense Device Identity Assertion;
5. independently authenticate/authorize the human account or organization actor;
6. only then commit the product backend's `Device ↔ Account/Organization` association or recover its product device credential/session.

AxLicense assertion alone must never authorize ownership transfer, disclose the bound account, reset a password, or silently rebind a device to a different account/organization.

**Acceptance:** factory/inventory/first-boot devices may be registered before any customer account exists; scanning a QR code without successful account/organization verification does not reveal full account identity or mutate ownership; forgotten-credential recovery can locate the existing product association only after human authorization; rebinding to a different account/organization is treated as a separate privileged ownership-transfer flow, not ordinary password recovery.

### Enrollment timing policy — Accepted

**Do not bind the device to an account/organization at AxLicense Device registration time.** The preferred commit point is the successful completion of the first authenticated product enrollment/claim session, after both:

- device possession has been proven against the already-registered DeviceIdentity; and
- the target account/organization actor has been authenticated and authorized.

For enterprise/kiosk products, the durable primary association should normally be **Device → Organization/Tenant**; the human user is recorded as the claiming/admin actor. A direct Device → personal Account association is appropriate only for products whose ownership model is genuinely personal. This keeps device ownership stable when employees/admins change.

### NFR-SEC-04 Recovery session is not authority by itself

QR/recovery session identifiers must be opaque, high-entropy, short-lived, purpose-bound and single-use (or equivalently replay-safe). They must not embed passwords, raw DeviceIdentity, private-key material, canonical license secrets, or sufficient information to rebind/transfer ownership without independent human authorization.

### Downstream reconciliation impact

This requirement adds a new cross-product capability surface, so P15 must pause until targeted downstream reconciliation confirms semantics. Required review path:

`P10 object-model impact review → P11 assertion/enrollment/recovery behavior → P12 assertion/challenge schema → P13 non-mutating operation/command contract → P14 architecture boundary reconciliation`.

The existing LicenseGrant/DeviceBinding licensing semantics are not changed by this requirement; the new association remains product-account authority rather than AxLicense license authority.

## Targeted requirement reconciliation — Legacy Provisional Activation & Human-Assisted Migration — 2026-09-12

### Problem / accepted product policy

Historical field devices were sold before AxLicense existed and there is **no reliable historical hardware/device/account/backend record** that can distinguish those physical devices from a newly installed machine. Therefore AxLicense must not attempt to infer historical entitlement from “no record”, old files, MAC/serial, software version, or a newly generated DeviceIdentity.

The accepted migration policy is commercial and human-assisted rather than pretending there is cryptographic historical proof:

- new/factory devices remain on the normal strict activation path;
- field devices that arrive through the designated legacy software-upgrade path may enter a temporary **Legacy Provisional Mode** after AxLicense identity bootstrap;
- provisional mode is usable but visibly **unactivated/watermarked** and is not an AxLicense entitlement;
- the customer submits a migration application with a contact email; AxLicense/product backend turns the request into a support/customer case;
- human review decides whether/how to issue the historical migration authorization/license;
- once approved and redeemed, the device leaves provisional mode and becomes an ordinary AxLicense-licensed Device.

### FR-018 Legacy Provisional Activation & Human-Assisted Migration — MUST

For the first AxLicense-enabling upgrade of already deployed products, the system must support a controlled migration experience for historical devices that have no reliable prior device record.

1. **Upgrade candidate classification is not entitlement authority.** A device may be marked `legacy_upgrade_candidate` only because it arrived through the designated product upgrade/migration entry path. This classification does not by itself create a LicenseGrant, DeviceBinding, SignedLicenseCredential, or historical entitlement.
2. **Provisional usage.** When a legacy-upgrade candidate has established a DeviceIdentity but does not yet have a valid AxLicense credential, the consuming product may continue to operate under `legacy_provisional` rollout policy while showing a persistent, clearly visible **Unactivated / 未激活** watermark or equivalent product indication.
3. **Activation remains required.** Provisional mode is a migration accommodation, not a permanent AxLicense license. Normal factory/new-install devices that do not enter through the designated legacy-upgrade migration path must not gain provisional rights merely because they have no AxLicense record.
4. **Migration application.** The product must let the user enter a contact email and create a stable `LegacyMigrationApplication` / equivalent request tied to the current AxLicense Device/request context. Submission itself grants no entitlement.
5. **Offline outbox.** If there is no network when the user submits the application, the request must be stored durably in a local pending outbox and retried when connectivity returns. While pending, the product may continue in watermarked provisional mode. A queued local request is not migration authority and may be safely retried/deduplicated.
6. **Server-side case creation.** When connectivity exists, the backend receives the request, associates it with a stable request/device reference, deduplicates repeated submissions, and creates the customer/support workflow. Email delivery/notification is a backend responsibility rather than requiring the device to operate an SMTP/email-sending stack.
7. **Customer-contact workflow.** Requests may be grouped for human review using non-authoritative heuristics such as verified contact email/domain and request counts. A high-volume pattern may trigger direct customer contact to understand organization size, deployment quantity, use scenario, operational problems, and management needs. Such grouping is a sales/support signal only; it does not establish Organization ownership or license authority.
8. **Single-device workflow.** For an ordinary single-device case, support may approve the migration and send the customer a migration/activation code or equivalent response, together with a recommendation to enroll the device in the product management backend.
9. **Activation response safety.** Any issued migration activation code/response must be scoped to the approved migration request/device/product, single-use or replay-safe, and unsuitable as a reusable generic license code. Successful redemption must flow through the existing controlled `MigrateLegacyDevice` / activation semantics and produce the normal SignedLicenseCredential.
10. **Offline completion.** If the target device is still air-gapped after approval, the existing offline request/response migration semantics may be used; the customer-support approval does not require the target device itself to be online.
11. **Post-migration normality.** After successful migration, the device is no longer treated as a historical special case. Future update/reinstall/credential recovery follows ordinary AxLicense identity-continuity and recovery rules and should not require the customer to repeat the historical migration application merely because the application software is upgraded again.

### Acceptance criteria

- A historical device upgrading to the first AxLicense-aware software can continue working without immediate hard lock while clearly displaying an unactivated watermark.
- A disconnected device can submit its email/request locally, remain usable in provisional mode, and upload the same durable request after connectivity returns without duplicate cases or licenses.
- A newly produced/fresh-installed device cannot obtain full entitlement merely by being “unknown” to AxLicense or by recreating a legacy-looking local state.
- Email/request submission alone never creates LicenseGrant/Binding/Credential.
- Support approval results in a device/request-scoped migration authorization/code; copying that code to another Device fails.
- Repeated request submission/retry is idempotent and does not generate multiple commercial entitlements.
- Large clusters of migration requests can be surfaced for human customer-development follow-up without those heuristics becoming authorization truth.
- Once a migration is committed, subsequent normal software upgrades use the ordinary AxLicense path and do not re-enter provisional migration.

### Explicit risk acceptance

Because there is no historical device authority, `legacy_upgrade_candidate` cannot cryptographically prove that the hardware was sold before AxLicense. A technically capable party may attempt to install/imitate an old software state. V1 mitigates this by ensuring provisional mode remains visibly unactivated and **full AxLicense entitlement is never auto-issued**: human/server migration approval remains the gate. This is an explicit commercial migration-risk tradeoff, not a claim of perfect historical-device attestation.

### P02 disposition

**FR-018 ACCEPTED / targeted requirement reconciliation complete.** This requirement materially changes legacy migration behavior and runtime states, so downstream P10–P16 require targeted reconciliation before P16 can close.

## Targeted Requirement Reconciliation — Unified First-Run Enrollment & Backend Licensing — 2026-09-12

### FR-019 Software-Distribution Independence & Unified Device Onboarding — MUST

NearHub V1 must not treat control of the software installer/update package as the license-security boundary. The software may be redistributed, copied or installed independently of commercial entitlement; installation and first execution alone must never create a LicenseGrant, DeviceBinding or licensed entitlement. This unrestricted distribution is an accepted product/acquisition posture rather than a migration-security defect.

For both already-sold field devices receiving the first AxLicense-aware software and newly shipped/factory devices, the default lifecycle is unified:

1. On first AxLicense-aware execution after installation/materialization, the device establishes device-local private identity material outside the cloneable image boundary and derives its DeviceIdentity. Private identity material remains local/protected and is not uploaded.
2. Identity establishment does not consume a license and does not require the device to already belong to a customer Organization.
3. The device may subsequently connect to the NearHub Product Backend and enter the ordinary SaaS enrollment/claim flow. A short-lived setup/enrollment code may be used as a human-friendly session handle, but a static DeviceId/serial/setup code is not ownership or license authority by itself.
4. After authenticated Organization claim, the Product Backend may offer purchase, trial, license-key redemption, existing-license-pool assignment, included-hardware entitlement or support/admin allocation according to commercial policy.
5. Only an explicit backend-authorized LicenseGrant assignment/activation may create DeviceBinding + SignedLicenseCredential. The device verifies and installs the signed credential before licensed entitlements become available locally.
6. The consuming product may expose an unlicensed/basic/promotional experience before a LicenseGrant is assigned, but licensed/advanced entitlements must remain unavailable until a valid credential is installed.
7. Existing sold devices and new factory devices do not require different client-side activation semantics solely because of shipment date or software provenance.
8. Truly air-gapped devices remain supported by the standard ACT-02 offline activation request/response path; this is an exception transport, not a separate commercial-license model.

**Acceptance:**

- Copying/installing the software on another compatible machine causes that machine to establish its own local identity and grants no commercial entitlement.
- An already-sold device and a newly shipped device with no prior AxLicense state can both reach the same `identity-established / unclaimed / unlicensed` product state.
- SaaS enrollment/Organization claim can succeed without silently licensing the device.
- License purchase/redemption/assignment can occur after enrollment and results in the standard AxLicense `LicenseGrant → DeviceBinding → SignedLicenseCredential` outcome.
- Failure or delay of license assignment must not corrupt ProductDeviceAssociation; the device can remain claimed but unlicensed while backend retry/support completes the license workflow.
- Setup/enrollment code replay, wrong Organization actor, wrong Device proof or expired session must fail without creating ProductDeviceAssociation or LicenseGrant.

### Supersession of legacy-specific NearHub requirements

For **NearHub V1**, the previous dedicated `FR-013 Legacy Device Migration` and `FR-018 Legacy Provisional Activation & Human-Assisted Migration` are **SUPERSEDED as mandatory product journeys** by FR-019. The old distinction `legacy sold device vs factory/new device` is no longer required at the client product-flow layer.

Consequences:

- `legacy_upgrade_candidate`, special legacy watermark-as-migration-control, migration email application/outbox, human migration approval and device-side migration redemption are no longer required for the NearHub V1 default lifecycle.
- Historical/customer commercial accommodation, including grandfathered rights for already-sold devices, becomes a **backend commercial entitlement decision** using trusted sales/customer/support evidence and ordinary license issuance/assignment. It must not be inferred merely from software provenance or local DeviceIdentity.
- Existing generic migration machinery may only remain in V1 if another accepted product requirement independently needs it; otherwise it should be removed/deferred during downstream reconciliation rather than preserved for historical reasons.
- `FR-007 Factory Identity Provisioning & Optional Pre-Activation` remains an optional specialized capability for batches that explicitly require factory-side identity/pre-activation, but it is no longer the default NearHub shipment path.

**P02 disposition:** FR-019 is accepted as the new NearHub V1 product requirement. This materially invalidates the legacy-specific assumptions previously propagated into P03 and P10–P16; downstream reconciliation is required from the earliest affected layer.
