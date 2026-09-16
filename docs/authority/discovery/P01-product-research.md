---
authority_id: AXL-V1-P01
stage: P01
scope: axlicense
kind: research
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c812a9442c5571a5e776f
migration_class: location-only
semantic_change: none
---

# 01 — P01 Product Research — Licensing Pattern Challenge v0.1

> 🧭 **Purpose** — 用成熟 licensing 产品验证需求是否属于行业已知问题，并挑战“自研轻量 AxLicense”的边界，而不是照搬供应商实现。

## 1. Alternatives reviewed

### Cryptlex

公开能力包含 node/device-bound licensing、feature entitlement、offline request/response 与本地 license validation。其模式验证了 `signed local artifact + device binding + entitlement + online/offline activation` 是成熟路径。

- https://cryptlex.com/docs/licensing-models/offline-licenses
- https://cryptlex.com/docs/features-and-entitlements/features-and-entitlement-sets

### LicenseSpring

明确面向 embedded / IoT / air-gapped 环境；支持离线文件交换、device-bound activation，并允许 activation 后全部本地检查。这与 NearHub / OEM appliance 的网络约束高度接近。

- https://licensespring.com/solutions/embedded-iot-licensing
- https://licensespring.com/solutions/offline-licensing

### Keygen

提供 cloud 与 self-hosted licensing API；Community Edition 可自托管。它说明“自建后台但复用成熟授权数据模型”也是可行市场路径。

- https://keygen.sh/docs/self-hosting/

### Revenera FlexNet Embedded

覆盖 embedded/device licensing、air-gapped、node-locked、subscription、floating 等大量企业模型。其范围也说明成熟商业平台的复杂度远超 AxLicense V1 所需。

- https://www.revenera.com/software-monetization/products/software-licensing/flexnet-licensing

## 2. Research findings

- **Finding R1:** 离线激活通常被拆为“activation/reconciliation”与“runtime local validation”两个问题；设备不需要在每次运行时连接许可服务器。
- **Finding R2:** Feature entitlement 是成熟抽象；商业 SKU 应由后台映射到 feature set，而不是在客户端写套餐名判断。
- **Finding R3:** Device-bound / node-locked license 是 embedded appliance 的常见基础模型。
- **Finding R4:** Offline deactivation/rehost 是完整离线生命周期的一部分，不能只做首次 activation。
- **Finding R5:** 全功能商业平台同时承担 floating、metering、billing、portal、reseller 等能力；这些并不是 AxLicense V1 的必要条件。

## 3. Build-vs-buy challenge

### Buy 的优势

- 更快拿到成熟 portal、SDK、离线流程与生命周期管理。
- 已覆盖大量边界情况。
- 供应商承担平台持续维护。

### Build 的优势

- NearHub / Axiom 的核心需求集合较窄且稳定。
- 需要跨产品共享，但不需要通用 SaaS monetization 全家桶。
- OEM 大批量设备长期按 activation 计费可能形成持续第三方成本。
- Device Identity、factory provisioning 与产品硬件/BSP 的结合最终仍需要 Auditoryworks 自己掌控。
- 可以把 license semantics 与产品代码保持在公司自己的稳定边界内。

## 4. P01 conclusion

当前研究**支持自研轻量 AxLicense**，但前提是严格保持 V1 non-goals，不把“自己做”误解为重新实现 Cryptlex/FlexNet 全部能力。

被外部成熟产品验证过、值得吸收的模式是：

`Device Identity → Activation → Signed Local License → Local Verification → Entitlements → Lifecycle/Rehost`。

不应复制的范围是：复杂 billing、floating、metering、reseller、完整 customer portal 与所有 monetization 组合。

## 5. Research status

P01 已足以支撑 V1 requirements drafting。后续在 architecture/security 前还应针对以下主题做专项 evidence：hardware-backed identity on RK/Android/Windows、secure storage、signing-key rotation、factory trust boundary 与 offline clock semantics。
