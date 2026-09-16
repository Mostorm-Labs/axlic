---
authority_id: AXL-V1-P03
stage: P03
scope: axlicense
kind: capability-traceability
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c81f48247e0af39e23326
migration_class: location-only
semantic_change: none
---

# 03 — P03 Capability Traceability — AxLicense V1 v0.1

> 🧩 **Status: Accepted capability baseline / device-identity policy reconciled 2026-09-12.** 建立 Requirement → Capability → Consumer → Platform → Verification 的 V1 追踪入口。本次 targeted reconciliation 进一步冻结 Windows-first Device Identity Provider discovery、assurance selection/fallback、no-silent-downgrade 与跨平台 provider contract 预留。Object/Behavior/Operation/Module 仍由后续 owning stage 冻结。

| Requirement | Capability | Primary consumers | Initial platforms | Verification direction |
|---|---|---|---|---|
| FR-001 | Signed License | All products | Linux / Windows / Android | Tamper corpus; valid/invalid signature oracle |
| FR-002 | Device Identity & Binding + Image Isolation | NearHub, Axiom OEM | **Windows V1**; future Linux/RK/Android via provider contract | Cross-device copy rejection; identity-free Golden Image fixture; post-clone identity uniqueness |
| FR-003 | Online Activation | NearHub, future products | Linux / Windows | Activation happy/error/idempotency tests |
| FR-004 | Offline Activation | NearHub, enterprise/OEM | Linux / Windows | Air-gap request/response golden flow |
| FR-005 | Entitlement Evaluation | NearHub, Axiom, Arc | All | Feature matrix oracle; unknown entitlement behavior |
| FR-006 | Rehost / RMA | Support, factory, customer ops | All device classes | Old→new device transfer state-machine tests |
| FR-007 | Factory Identity Provisioning + Optional Pre-Activation | ODM/EMS, inventory ops | **Windows V1**; future embedded providers reserved | Post-clone identity uniqueness; identity-only inventory state; scoped pre-activation; capacity/idempotency/replay tests |
| FR-008 | Right Separation | NearHub / Axiom commercial policy | All | Runtime vs maintenance vs cloud matrix |
| FR-009 | License Lifecycle Admin | Internal ops/support | Server | Audit completeness; authorization tests |
| FR-010 | Key / Format Rotation | All deployed clients | Client + server | Old/new key coexistence fixtures |
| FR-011 | Product Integration Contract | NearHub, Axiom/Arc, future products | C/C++/local IPC bindings TBD | Contract tests independent of product UI |
| FR-012 | Identity Persistence & Recovery | Support / field devices | **Windows V1** | Update/reinstall/factory-reset continuity; identity-loss recovery; physical-replacement boundary tests |
| FR-013 | Legacy Device Migration | Existing customers, support, admin | **Windows V1** | Online/offline migration; migration-authority oracle; cloned legacy IMG negative fixture; idempotent historical-seat claim |
| FR-014 | Device Identity Provider Discovery | AxLicense Agent / diagnostics / provisioning | **Windows V1**; provider interface cross-platform | Provider present-vs-usable matrix; create/load/sign qualification; unavailable/unsupported negative cases |
| FR-015 | Identity Assurance Selection & Fallback | AxLicense Agent / policy / support | **Windows V1** | TPM→software fallback matrix on first establishment; no-silent-downgrade test; minimum-assurance rejection |
| NFR-SEC-03 | No Silent Identity-Security Downgrade | AxLicense Agent / recovery | Windows V1 + future provider contract | Existing hardware identity with TPM unavailable must return unavailable/recovery path, never auto-create software identity |
| NFR-OPS-02 | Identity Capability Diagnostics | Support / factory / admin tooling | Windows V1 | Diagnostics contract; secret-redaction tests; stable failure classification |

## Consumer mapping seed

### NearHub Launcher

需要 `license status`、activation presentation、offline import/export、NearHub feature entitlement 与设备级 license diagnostics。对存量已售 NearHub，还需要 legacy identity bootstrap / migration presentation；对 factory 新设备，需要支持 identity-only 与 factory pre-activated 两种出厂状态。**V1 以 Windows NearHub/OPS 场景为 production reference；Linux/RK 仅保留未来 provider integration boundary。**

### Axiom / Arc

原则上只消费 entitlement/state，不拥有 activation lifecycle；未来作为 OEM SDK 独立出售时可使用相同 AxLicense semantics。

### Future consumers

NearCast、NearSync、NearStream 等如需 device/software entitlement，应复用同一 contract，而不是复制 activation implementation。

## Accepted product-policy trace

- **License unit:** per physical device → FR-002 / FR-003 / FR-006 / FR-007 / FR-013。
- **Identity establishment ≠ license consumption:** DeviceIdentity 可先于任何 LicenseGrant/Binding/Credential 存在 → FR-002 / FR-007 / FR-013。
- **Golden Image Identity-Free:** cloneable image 不携带 current identity/private identity material/binding/credential；per-device identity 在 target materialization 后建立 → FR-002 / FR-007 / NFR-SEC-02。
- **Perpetual offline runtime:** runtime 可永久离线 → FR-001 / FR-004 / FR-008 / NFR-AVL-01。
- **Rehost authority:** internal admin/support；physical replacement 与 same-device recovery 明确分离 → FR-006 / FR-009 / FR-012。
- **Factory connectivity:** online provisioning station；no delegated signing in V1 → FR-007 / NFR-SEC-01。
- **Factory lifecycle:** Identity Provisioning 可独立完成；Factory Pre-Activation 是可选 successor capability → FR-007。
- **Legacy migration authority:** historical entitlement 只能由 trusted migration evidence/claim authority 绑定；identity bootstrap 不自授权 → FR-013。
- **Right separation:** runtime / maintenance-update / cloud-service 分离 → FR-008。
- **Windows-first V1:** Windows 10/11 是 Device Identity production reference；Linux/RK/Android V1 只冻结 platform-neutral provider/capability boundary → NFR-PLAT-01 / FR-014 / FR-015。
- **Provider qualification:** presence 不等于 usable；首次 identity establishment 必须基于实际 create/load/sign capability 选择 provider → FR-014。
- **Assurance fallback:** Windows 默认 hardware-backed TPM → software-persistent → unsupported；普通 V1 activation/legacy migration 最低允许 software-persistent，future product policy 可提高 minimum assurance → FR-015。
- **No silent downgrade:** existing hardware-bound identity 的 provider 暂时不可用必须进入 unavailable/recovery，不得自动创建 software identity → NFR-SEC-03。
- **Diagnostics:** provider/scheme/assurance/persistence/probe failure 可诊断但不得泄露 private identity material → NFR-OPS-02。

## Downstream traceability policy

进入后续阶段后，重要 requirement 应继续扩展为：

`Requirement → Capability → Object → Behavior → Operation → Module → Platform → Verification`。

当前 P03 冻结到 capability / consumer / platform intent / proof direction；具体对象模型、行为、操作、模块、进程边界、协议和 crypto realization 必须由后续 owning stage 定义。

Disposition: **P03 DEVICE-IDENTITY POLICY RECONCILED / ACCEPTED.** 本轮新增 policy 只约束 platform/provider selection 与 assurance/fallback，不改变 P10–P13 已冻结的 DeviceIdentity `scheme_id + identity_epoch`、Grant/Binding/Credential separation 或 mutation semantics，因此 **无需重新打开 P10–P13**。下一 earliest untrusted layer 仍为 P14 System Architecture；P14 应按 Windows-first 分配 provider abstraction / policy ownership，并把具体 Windows platform contract 留给 P17。

## Targeted capability traceability addendum — Device Assertion & Enrollment Recovery — 2026-09-12

| Requirement | Capability | Primary consumers | Initial platforms | Verification direction |
|---|---|---|---|---|
| FR-016 | Purpose-Bound Device Identity Assertion | Launcher/NearHub backend, NearSync, support/recovery workflows, future product backends | **Windows V1** using existing DeviceIdentity provider boundary | Challenge/audience/purpose/expiry binding; replay rejection; wrong-device rejection; no arbitrary-signing oracle; private-key non-export |
| FR-017 | Device Enrollment / Association Recovery Integration | Launcher/NearHub account & organization backend, support/admin tooling | Windows device + product web/backend | Registered-but-unclaimed state; device-proof + human-auth dual gate; QR without human auth cannot disclose/rebind; ownership-transfer separation |
| NFR-SEC-04 | Replay-Safe Recovery Session | Launcher recovery UI/web flow | Device + web/backend | Opaque high-entropy token; short TTL; single-use/replay-safe; no sensitive identifiers/secrets embedded |

### Binding timing trace

`DeviceIdentity registration` establishes only trusted physical-device identity. It does **not** establish customer ownership.

Preferred lifecycle:

`Device registered → Product state unclaimed/unassociated → authenticated enrollment/claim session → Device Identity Assertion + human/org authorization → Product backend commits Device↔Organization/Account association`.

For enterprise/kiosk consumers, trace the durable ownership association primarily to `Organization/Tenant`; the user account is the claimant/admin actor. Personal-account binding remains a product-policy option for truly personal products.

### Ownership boundary

AxLicense owns DeviceIdentity and proof-of-possession semantics. The consuming product backend owns account/organization identity, password/SSO recovery, Device↔Account/Organization association, and ownership transfer policy. AxLicense assertion is evidence consumed by those workflows, not ownership authority by itself.

### Downstream routing impact

Because FR-016 introduces a new public cross-product device-trust capability, the earliest untrusted downstream layer reopens to **P10 targeted impact review** before P15. P10–P14 should reconcile only the new assertion/enrollment boundary；existing license Grant/Binding/Credential semantics remain current unless a concrete conflict is found.

## Targeted capability traceability addendum — Legacy Provisional Activation & Human-Assisted Migration — 2026-09-12

| Requirement | Capability | Primary consumers | Initial platforms | Verification direction |
|---|---|---|---|---|
| FR-018 | Legacy Provisional Mode + Migration Application Outbox + Human-Assisted Migration Approval | Existing field devices, Launcher migration UI, support/customer-success/admin backend | **Windows V1**  • product backend/email workflow | Legacy-upgrade candidate enters watermarked provisional mode without entitlement; offline request survives restart and uploads once; duplicate submission dedupes; new/fresh device cannot auto-gain legacy entitlement; issued migration code is device/request scoped and replay-safe; post-migration device follows ordinary AxLicense recovery |

### Capability decomposition

`FR-018` traces into the following capability set:

1. **Legacy Upgrade Candidate Classification** — identifies that the product arrived through the designated legacy upgrade entry path. This is rollout classification only, never entitlement authority.
2. **Legacy Provisional Runtime Policy** — product can remain usable while visibly marked `Unactivated/未激活`; this is explicitly outside SignedLicenseCredential authority.
3. **Migration Application Capture** — user supplies a contact email and the product binds the application to stable request/device context.
4. **Durable Offline Migration Outbox** — no-network submissions persist and retry later with the same logical request identity.
5. **Migration Case Intake / Deduplication** — backend converts uploaded applications into a durable support/customer workflow and prevents duplicate cases from duplicate device retries.
6. **Customer-Development Aggregation** — backend may group requests by verified email/domain and device/request counts to flag likely fleet/enterprise customers for human outreach. This is non-authoritative business intelligence only.
7. **Human-Assisted Migration Approval** — support/customer-success/admin can approve a historical migration after customer contact/review.
8. **Device/Request-Scoped Migration Response** — approved code/artifact is single-use or replay-safe and bound to the current migration request/device/product; it cannot act as a generic reusable license key.
9. **Standardized Migration Completion** — approved response enters existing `MigrateLegacyDevice`/offline migration authority path and produces normal Grant/Binding/Credential state.

### New-vs-legacy routing trace

```plain text
First AxLicense-aware execution
        |
        +-- Factory/new-install authority path
        |      -> normal identity provisioning/activation
        |      -> no Legacy Provisional Mode
        |
        +-- Designated legacy software-upgrade entry path
               -> establish DeviceIdentity
               -> legacy_upgrade_candidate
               -> if no valid credential: Legacy Provisional Mode + watermark
               -> migration application/email
               -> human approval
               -> MigrateLegacyDevice
               -> normal AxLicense device
```

Critical invariant: **absence of an AxLicense record is never legacy eligibility.** The legacy path is a designated rollout entry path plus human migration approval; neither old software presence nor `legacy_upgrade_candidate` alone grants entitlement.

### Customer-contact trace

The email/application system has two different meanings that must remain separate:

- **Contact/business signal:** allows support/customer-success to reach the user, identify large deployments, learn deployment/use problems, and recommend management enrollment.
- **Authorization:** only an explicit approved migration authorization/code may permit `MigrateLegacyDevice` to commit commercial rights.

Email ownership, email domain, request count, or inferred organization size are not LicenseGrant authority by themselves.

### Downstream routing impact

FR-018 adds a product-visible provisional runtime state, a migration-application lifecycle, a durable offline outbox, a human-approval boundary, and a device/request-scoped migration response. Therefore the earliest untrusted downstream layer is **P10 targeted impact review**, followed by P11/P12/P13 and P14–P16 targeted reconciliation. P16 remains open until that pass is complete.

Disposition: **P03 FR-018 TRACEABILITY ACCEPTED.** Next Primary Owner after Discovery handoff: `aegis-modeling`, beginning at **P10 targeted reconciliation**.

## Targeted Capability Traceability Reconciliation — FR-019 Unified First-Run Enrollment & Backend Licensing — 2026-09-12

| Requirement | Capability | Primary consumers | Initial platforms | Verification direction |
|---|---|---|---|---|
| FR-019 | Unified first-run DeviceIdentity establishment + SaaS enrollment + backend license acquisition/assignment | NearHub Launcher, NearHub Product Backend/SaaS, AxLicense Server, support/commercial admin | **Windows V1** device + product web/backend | Legacy/new device path equivalence; copied-installer grants no entitlement; setup/enrollment-code expiry/replay/wrong-device rejection; Device proof + human/Organization authorization dual gate; claimed-but-unlicensed intermediate state; backend license assignment produces ordinary Grant/Binding/Credential; local credential verification before feature enablement |

### Capability decomposition

1. **Unrestricted Software Distribution** — installer/update provenance is not license authority; copied software can establish local identity but cannot self-license.
2. **Unified First-Run Identity Bootstrap** — both already-sold and newly shipped devices establish protected per-device identity material on first AxLicense-aware execution after materialization/install.
3. **SaaS Enrollment Session** — connected device proves current DeviceIdentity possession and obtains a short-lived setup/enrollment session handle for human-assisted Organization claim.
4. **Product Organization Claim** — authenticated/authorized customer Organization actor consumes the enrollment session and commits ProductDeviceAssociation. Device proof alone is insufficient.
5. **Commercial License Acquisition** — after enrollment, Product Backend may expose trial/purchase/redeem/assign/included-hardware/support allocation policy. These are commercial/product-backend decisions, not setup-code authority.
6. **AxLicense Activation Orchestration** — a valid backend commercial decision resolves/creates the applicable LicenseGrant and invokes the ordinary device activation path; canonical authority still converges on Grant + Binding + SignedLicenseCredential.
7. **Claimed-but-Unlicensed Recovery** — ProductDeviceAssociation and license activation are separate commits; partial failure leaves the device managed/claimed but unlicensed and backend/device retry converges without distributed rollback.
8. **Historical Customer Accommodation** — already-sold device entitlement, if granted, is decided from trusted backend sales/customer/support evidence and ordinary license issuance/assignment; no legacy-client provenance is required.
9. **Air-Gap Exception** — devices that cannot join SaaS use standard offline activation request/response; this does not reintroduce a separate legacy migration authority model.

### Supersession trace

For NearHub V1, `FR-013 Legacy Device Migration` and `FR-018 Legacy Provisional Activation & Human-Assisted Migration` no longer define required product journeys. Their legacy-specific client/workflow capabilities are superseded by FR-019 unless another accepted product requirement independently needs them. Existing downstream objects/modules/operations such as legacy candidate classification, migration application/outbox, migration-review authority and device-side migration redemption must therefore be revalidated rather than automatically preserved.

### Updated NearHub primary lifecycle trace

`software install/update → first-run local private identity establishment → optional network setup → SaaS enrollment session/setup code → authenticated Organization claim → managed but possibly unlicensed device → purchase/trial/redeem/assign/included entitlement decision → ordinary AxLicense activation → SignedLicenseCredential verify/install → licensed feature availability`.

### Routing impact

This requirement changes product semantics rather than only P16 presentation. **Earliest untrusted downstream layer = P10 Product Object Model.** P10 must determine which legacy-specific objects cease to be V1 authority and whether a new product-side enrollment/license-assignment concept is needed; P11–P16 must then reconcile behavior, schema/operations, architecture/modules and runtime flows in order. Existing Grant/Binding/Credential separation, DeviceIdentity proof rules and offline verification semantics remain authoritative unless a concrete conflict is found.

**P03 disposition:** FR-019 capability trace accepted. P16 remains blocked from closure until downstream targeted reconciliation completes.
