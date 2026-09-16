---
authority_id: AXL-V1-P00
stage: P00
scope: axlicense
kind: problem
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d74c57a590c81f8b4a3ee31b093afd0
migration_class: location-only
semantic_change: none
---

# 00 — P00 Problem Discovery — AxLicense v0.1

> 🔎 **Authority status: Accepted / P00 closed.** 问题、约束、成功标准与 V1 non-goals 已确认；本页不冻结架构实现。

## 1. Problem statement

Auditoryworks 已出现多个需要软件授权的独立产品/运行时，包括 NearHub Launcher、Axiom / Arc，并预期后续 NearCast、NearSync 等也会出现相同需求。如果各产品分别实现激活、设备绑定、离线授权、功能开关和 RMA 转移，将形成重复实现、策略漂移、密钥风险与长期维护成本。

同时，目标设备包含 Linux/Android/Windows appliance、会议室与教育场景、OEM/ODM 量产和可能长期离线的企业网络，因此“每次运行必须联网验证”的 SaaS 式方案不适合作为基础运行条件。

## 2. Root constraints

- 一套跨产品共享的 license / entitlement 语义，而不是每个产品一套激活系统。
- 永久或长期授权的本地运行不能依赖 AxLicense Server 在线。
- License 必须可离线验证完整性与来源，客户端不得持有签发私钥。
- 必须支持设备绑定，同时允许合法 RMA / 主板更换后的授权迁移。
- 必须支持完全离线的首次激活路径。
- 必须支持 OEM/ODM 工厂批量 provisioning，而不是让终端客户逐台人工激活。
- 产品代码只消费 license state / entitlement，不应耦合签发后台内部结构。
- V1 必须刻意控制范围，避免演化成完整 billing / CRM / floating license 平台。

## 3. Affected scenarios

### S1 — NearHub 正常出厂

工厂烧录系统并完成授权 provisioning；最终用户开机后 NearHub Launcher 已处于 licensed 状态，不需要输入激活码。

### S2 — NearHub 在线激活

未预激活设备通过网络完成首次 device-bound activation；之后可断网启动并本地校验。

### S3 — 企业 / 政府离线环境

设备生成 activation request，通过 USB/其他介质转移至联网环境签发，再导入 signed license；后续无需持续联网。

### S4 — Axiom OEM Runtime

第三方产品集成 Axiom/Arc，通过标准 entitlement API 判断 `axiom.runtime`、`arc.fastink` 等能力，不依赖 NearHub。

### S5 — RMA / 主板更换

旧设备不可继续使用授权；后台或受控流程将 entitlement 从旧 device identity 转移到新设备。

### S6 — Feature / SKU 差异

商业 SKU 映射为 entitlement set；客户端不硬编码 `Standard/Pro/Ultra` 等销售套餐名。

## 4. V1 seed scope — accepted

- **Signed License** — 版本化、可离线验签的授权文档。
- **Device Identity** — 稳定设备身份与 device binding。
- **Activation** — online + offline request/response。
- **Entitlement** — 产品/feature 权限查询与评估。
- **Rehost** — deactivate / replace / RMA transfer。
- **Factory Provisioning** — OEM/ODM 批量预授权。

## 5. Supporting requirements discovered during P00

以下不是新产品方向，而是上述六项能力成立所必需的 supporting concerns：

- Signing key lifecycle / key rotation。
- License format versioning 与 backward compatibility。
- Secure local storage 与 factory-image cloning protection。
- Runtime right、maintenance/update right、cloud-service right 的分离。
- Minimal audit trail：issue / activate / deactivate / rehost / provisioning。
- Failure-safe behavior：server outage、clock anomaly、corrupt license、device identity unavailable。
- Stable integration contract：产品只能查询状态/entitlement，不直接操作 signing secrets。

## 6. Success criteria

- NearHub、Axiom/Arc 能使用同一 license semantics 与同一份客户端核心能力。
- Online 与 offline activation 最终产生同一种 locally verifiable license artifact。
- 对 perpetual runtime license，切断 Internet 或关闭 AxLicense Server 后设备仍能正常运行。
- 修改 license payload、复制到不匹配设备或使用未知 signing key 时，验证必须失败。
- 工厂镜像克隆不能导致多台设备共享同一个有效 device identity / license state。
- RMA 可以通过明确受控流程迁移，而不是要求工程师手工改数据库。
- 新 signing key / 新 license format 可以在不使全部旧设备失效的情况下逐步滚动。

## 7. Explicit V1 non-goals

- Floating / concurrent-seat license。
- Usage metering / consumption billing。
- Payment、invoice、checkout、CRM。
- Reseller hierarchy / channel settlement。
- Named-user license 与用户账号体系。
- 完整 customer self-service portal。
- 通用 DRM、代码混淆或反调试平台。
- 远程“瞬时”吊销永久离线设备。
- 复杂租户计费与 SaaS subscription engine。

## 8. Product policy disposition

以下五项 V1 product policy 已由 Product Owner 于 2026-09-10 明确接受：**per physical device、perpetual runtime 可永久离线、RMA/rehost 由内部 admin/support 控制、工厂 provisioning station 联网、runtime/maintenance/cloud rights 分离**。其余条目属于 supporting policy 或后续 architecture/security 选择，不阻塞 P00/P02 收敛。

| Decision | Proposed V1 default | Why it matters |
|---|---|---|
| License unit | **ACCEPTED — per physical device**；SDK/OEM 特殊合同后续扩展 | 决定 activation/rehost 模型 |
| Perpetual offline policy | **ACCEPTED — Runtime perpetual license 可永久离线**；云服务单独在线 entitlement | 避免会议室/教室因服务器故障变砖 |
| Device identity assurance | V1 接口抽象稳定；首版允许 robust fingerprint，硬件可用时升级 hardware-backed key | 平衡开发速度与克隆防护 |
| Trial license | V1 支持 trial，但高可信 offline trial 不作为首要目标 | 无 trusted clock 时离线试用容易被时间回滚绕过 |
| Factory connectivity | **ACCEPTED — 工厂 provisioning station 在线**；完全离线工厂 delegated signing 暂不做 | 避免把 signing authority 下放到工厂 |
| Rehost authority | **ACCEPTED — V1 由内部 admin/support 执行**；客户自助后置 | 减少滥用与 portal 范围 |
| OS reinstall | 同一 device identity 可恢复/重新导入；不以文件系统安装实例作为唯一身份 | 影响售后与 secure store |
| Maintenance | **ACCEPTED — 运行权与升级权分离**：perpetual runtime + optional maintenance_until；cloud-service right 独立 | 支持 OEM 长期设备生命周期 |
| Revocation | 在线设备可同步 revoke；永久离线设备不承诺远程即时 revoke | 这是离线模型的物理边界 |
| Signing algorithm | 实现阶段在标准算法中选择；不得自定义密码算法 | 属于后续 security/architecture decision |

## 9. P00 disposition

当前 evidence 足以确认“需要一个跨产品、离线优先、device-bound 的轻量授权基础设施”这一问题是真实且重复出现的。五项核心 product policy 已接受，P00 不再存在阻塞 P02 freeze 的产品未知项。cryptographic profile、Device Identity assurance realization、secure storage 与模块实现继续留在 architecture/security 阶段，不在 P00 提前冻结。
