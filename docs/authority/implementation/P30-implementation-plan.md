---
authority_id: AXL-V1-P30
stage: P30
scope: axlicense
kind: implementation-plan
version: "3"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c813ba9e6fb1396334bd7
migration_class: location-only
semantic_change: "v3 (2026-11-25, D-109): trust-keyring sourcing in the A2 slice changed from a packaging-time signing-backend export to an offline build-time derivation from the committed environment public-key files (tools/fetch_trust_floor.py removed); added the parallel slice AXL-V1-R1 — Publish Pipeline (release + test-publish GitHub Actions workflows, GitHub Releases as the distribution channel; R1 output is a pre-A8 engineering release, not the A8 release Gate) with its dependency-graph leaf, evidence-allocation row and CI rows; no A3–A8 content changed. v2 (2026-11-23, D-106): A2 revised from self-built Node/TS backend + HTTPS transport to the platform-integration library plane (registration association, credential install, trust keyring, host CLI surface); server-plane realization moved out of this repository's scope; A3-A8 server-side content superseded pending a host-bridge integration follow-up; dependency graph updated. v1 (D-091) client-first stack unchanged for the Windows client."
---

# 30 — P30 Implementation Plan — AxLicense V1 v0.1

> 🧱
>
> **Authority status: Accepted / P30 AxLicense V1 Implementation Planning + D-091 client-first language-stack reconciliation CLOSED — 2026-09-13; A2 direction revised by D-106 — 2026-11-23; trust-keyring sourcing revised and publish pipeline slice AXL-V1-R1 added by D-109 — 2026-11-25.** 本页把 trusted P02/P03 + P10–P20 Current Authority 转成 dependency-aware、evidence-gated vertical implementation plan。Current implementation direction is now **C++20/CMake for the Windows `axlic.exe` client, Node.js + TypeScript for the server, PostgreSQL for canonical persistence, with client-first sequencing**. Earlier Rust/Cargo wording on this page is superseded where it conflicts with D-091. 本页不授权 coding，也不冻结任何 P31 EXECUTION_CLOSURE_CONTRACT。
>
> **D-106 supersession note (2026-11-23):** the self-built server direction in §2.1 (server bullets) / §2.2 / §2.3 / §2.4 / §3 (`server/` topology) / §7.1 (server CI rows) is superseded as this-repository scope. The licensing platform (server canonical truth, signing backend, channel transport) and the host launcher bridge (channel consumption, RPC orchestration, receipt reporting) are owned outside this repository; axlic is consumed exclusively through the `axlic.exe` CLI contract. Those sections are retained below as historical direction and no longer authorize any in-repo server/transport implementation. The A2 slice in §5 is rewritten accordingly; A3–A8 entries retain their device-side semantics but their server-side content is superseded pending a host-bridge integration follow-up.
>
> **D-109 addendum note (2026-11-25):** the A2 slice's trust-keyring bullet no longer sources the embedded floor from a packaging-time signing-backend export: the floor is derived offline, at build time, from the environment public-key files committed under `client/keyring/`. §4 and §5 gain the parallel leaf **AXL-V1-R1 — Publish Pipeline**, and §6 / §7.1 gain the matching evidence and CI rows. R1 depends only on A2, does not re-plan A3–A8, and its output is a pre-A8 engineering release, not the A8 release Gate.

## 1. Stage contract

- **Role:** P30 Implementation Planning.
- **Authority:** P02/P03；P10–P18；P20 D-088；central routing D-089.
- **Objective:** 将 Windows-first AxLicense V1 拆成独立可审阅、可逐步 materialize P20 evidence 的 vertical slices，并明确依赖、并行边界、仓库结构、技术 realization 与 P31 package sequence。
- **Non-goals:** 不执行代码修改；不创建 P31 package；不声称 EV-01～EV-07 已 PASS；不重开 P10–P20；不把 Product Backend 的 Account/Organization/SKU/IAM 业务代码放入 AxLicense repo。
- **Required output:** implementation stack；repository topology；dependency graph；vertical slices；evidence allocation；CI/platform strategy；P31 sequencing；repository-empty bootstrap rule；terminal P30 disposition.
- **Quality gate:** 每个 slice 必须产生可运行/可审阅的纵向能力；不得只按“codec/domain/db/cli”做长期水平分层；P20 blocking evidence 必须有明确 materialization point；Windows-specific work 不得被 Linux/mock-only CI 假装覆盖。
- **Handoff:** P30 CLOSED 后 earliest untrusted layer = **P31 Task Packaging**；下一步由 `aegis-implementation` 为第一个 slice 冻结 EXECUTION_CLOSURE_CONTRACT。P30 不直接执行 P32。

## 2. V1 implementation stack

### 2.1 Production language / workspace — D-091 Current Authority

V1 使用 **C++20 + CMake** 实现 Windows device-side `axlic.exe`，使用 **Node.js + TypeScript** 实现 AxLicense Server modular monolith。

Rationale:

- 用户明确把长期可读性、团队理解与维护便利性置于“单一生产语言”之上；
- `axlic.exe` 与 Windows CNG/NCrypt、machine-wide lock、ACL、protected local state/atomic replace 都是 native Windows boundary，C++ 可以直接表达这些 platform contracts；
- 后端业务/HTTP/control-plane/DB orchestration 使用 Node.js + TypeScript，保持较低的业务迭代和维护门槛；
- P17/P20 已冻结 deterministic CBOR + crypto semantics，因此 C++ 与 TypeScript 必须通过同一 golden vector corpus 得到 byte-identical authorization representation；跨语言实现不得各自发明第二套 wire semantics；
- Product 对客户端仍只依赖 `axlic.exe` CLI contract，不依赖 C++ ABI，因此 V1 仍不发布 public DLL。

Client build/dependency direction:

- C++20；CMake；Windows production target优先 MSVC toolchain；
- third-party dependencies必须 pinned/locked，exact compiler/CMake/dependency versions由首个 P31 package固化；
- canonical CBOR/crypto parser surface保持小而可审阅，优先显式 wrapper 而不是把第三方 object model扩散进 domain code。

Server direction:

- supported Node.js LTS line + TypeScript strict mode；
- package manager/runtime/dependency exact versions在首次 server P31 package固化；
- PostgreSQL、SigningPort、API carrier均通过清晰 adapter/port 边界实现；
- TypeScript server必须消费与 C++ client相同的 checked-in golden vectors/semantic registry。

### 2.2 Server persistence

V1 canonical server persistence implementation选择 **PostgreSQL**，以 transaction + uniqueness + CAS/row-version style checks 支撑 P13/P14 的 ACID canonical outcome、one-active-binding、idempotency ledger、CatalogRevision/authority_revision concurrency。

- Server 仍保持 modular monolith；选择 PostgreSQL 不改变 P14 logical boundaries。
- Product SaaS 数据不得进入该 canonical DB。
- DB schema/migrations属于 repository authority reality，必须由 scenario tests验证，不得只靠 ORM model。

### 2.3 Signing realization

- Production code只依赖 `LicenseSigningPort` / `ChallengeSigningPort`；private production signing key不进入 repo、DB、device或普通 server process config。
- test/dev 可提供 dedicated fixture signer，用于 EV-01/EV-02；必须 compile/config-gated，不能在 production mode被自动启用。
- 具体 cloud KMS/HSM vendor仍是 deployment adapter decision；P30 不把它变成 core dependency。

### 2.4 Transport / carrier

- Device/Product control transport使用 HTTPS + versioned JSON-equivalent API carrier；signed credential/challenge/proof artifact仍只以 P17 deterministic CBOR bytes作为 cryptographic representation。
- Text carrier需要时只承载 exact artifact bytes 的 lossless encoding；carrier parse永不替代 artifact verifier。

## 3. Repository topology

Target topology — D-091 Current Authority：

```
CMakeLists.txt
cmake/
client/
  CMakeLists.txt
  include/axlic/
  src/
    wire/            # P12/P17 canonical artifact + strict deterministic CBOR/envelope
    core/            # credential verification, RuntimeLicense, command application
    windows/         # CNG/KSP, machine lock, ACL/protected-state/atomic-replace
    transport/       # HTTPS/control client adapters
  app/               # axlic.exe process/CLI entry
  tests/
server/
  package.json
  tsconfig.json
  src/
    domain/          # P10-P13 server-side domain semantics/invariants
    application/     # OperationCore orchestration
    persistence/     # PostgreSQL adapters/migrations
    signing/         # SigningPort adapters; no production private key in repo
    transport/       # HTTP/control plane
  tests/
reference/
  vectors/           # frozen semantic fixtures + exact canonical bytes shared by C++/TS
  model/             # independent transition oracle/test-only logic
  tools/             # vector/reference verification glue
integration-tests/
  lifecycle/
  windows/
  api/
bench/
  w1/
```

Rules:

- `client/src/core`不得依赖 Node.js、SQL、Product SaaS或 server implementation；
- `client/src/wire`只实现 P12/P17/P20 frozen artifact semantics，不拥有 commercial/domain mutation；
- `client/src/windows`只实现 platform ports，不决定 entitlement/commercial authority；
- `server/src/domain`不得依赖 Product SaaS IAM/Organization/SKU truth；
- C++ client 与 TypeScript server 对 authorization-critical CBOR 必须使用同一 P20 EV-01 golden corpus；任何 byte drift直接视为兼容/安全失败；
- Product-specific NearHub backend只通过 API/ref client消费 AxLicense，不进入本 repo canonical domain。

## 4. Dependency graph

```
AXL-V1-A0 C++ Local Signed-Credential Gate
        ↓
AXL-V1-A1 C++ Windows Identity + Protected Local State
        ↓
AXL-V1-A2 Platform Licensing Library Plane (D-106)
        ↓                          ↘ (D-109)
AXL-V1-A3 Online Grant Activation + Credential Install     AXL-V1-R1 Publish Pipeline
        ├──────────────┬──────────────┐
        ↓              ↓              ↓
AXL-V1-A4          AXL-V1-A5      AXL-V1-A6
Refresh/Revision   Device Assertion Offline Activation
        └──────────────┴──────────────┘
                       ↓
                AXL-V1-A7
        Recovery/Rehost/Factory/Admin
                       ↓
                AXL-V1-A8
     Windows Hardening + Release Qualification
```

**Client-first rule:** A0 与 A1 不要求真实 licensing platform即可 Gate-close；A2 (D-106 form) 也不引入任何网络/服务端代码——它是纯本地库平面，宿主桥负责通道侧。A4/A5/A6 在 A3 之后可以并行开发，但 P31 packages必须分别冻结 scope，不允许在共享 module/contract 上无协调并发改写同一 authority surface。

**Publish-pipeline leaf (D-109):** AXL-V1-R1 是只依赖 A2 的并行叶子：它打包并发布已有的 `axlic.exe`，既不被 A3–A8 阻塞，也不阻塞 A3–A8。

## 5. Vertical slices

### AXL-V1-A0 — C++ Local Signed-Credential Gate

**Outcome:** 从空 repo 得到一个真实 Windows `axlic.exe`，可在无网络条件下读取 fixture credential/state、验证 P17/P20 artifact、执行 `status` / `entitlement`，输出 P17 CLI structured result。

Includes:

- C++20/CMake/MSVC workspace + Windows CI bootstrap；
- narrow deterministic-CBOR authorization codec/parser wrapper；
- P-256/SHA-256 fixed-width low-S verifier + fixture signing/vector tooling only；
- minimum credential schema/evaluator；
- `axlic status` / `axlic entitlement`；
- fixture local-state reader；
- EV-01 golden positive/negative core subset；
- W1 benchmark harness。

Does not include CNG identity, real machine mutation, server, DB or activation.

**Evidence maturation:** EV-01 partial；EV-06 CLI partial；EV-07 first measurable client baseline.

### AXL-V1-A1 — C++ Windows Identity + Protected Local State

**Outcome:** `axlic.exe identity` can establish/load the strongest policy-allowed Windows CNG identity, pin provider/scheme, and persist machine-wide protected client state with cross-process single-writer + crash-safe atomic replace, without requiring a server.

Includes:

- `IIdentityProvider` + Microsoft Platform Crypto Provider / Software KSP adapters；
- IdentityManager / IdentityPolicy / provider pinning；
- non-exportable persisted key handling；
- machine-wide protected local-state envelope；
- cross-process mutation lock；
- re-read-after-lock + temp/flush/atomic-replace baseline；
- local recovery classification for missing/unavailable pinned provider。

Does not include canonical Device registration or PostgreSQL.

**Evidence maturation:** EV-03 real-provider behavior partial；EV-04 local durability/serialization partial；EV-06 identity CLI contract partial.

### AXL-V1-A2 — Platform Licensing Library Plane (D-106 Current Authority)

**Outcome:** 在已经稳定的 C++ identity/local-state client 基础上，把 axlic 补全为**纯本地库平面**：持久化平台分配的设备引用、以信任密钥环验签并原子安装凭证（含防回滚高水位与通道侧 policyVersion 元数据）、向宿主 launcher 桥暴露只读持有版本查询面。axlic 自身保持零网络代码；通道传输、RPC 编排与回执上报归宿主桥，许可平台（台账/签发/签名后端）在本仓范围之外。

Includes:

- **平台设备引用域**：`axldev_` + 32 位小写 hex 的解析/校验专用类型，用于注册引用持久化与凭证载荷 `device_id` 的规范化比对；
- **注册关联平面**：持久化平台注册响应返回的 opaque 设备引用（非权威便利引用，P16 FR19-ID-02 语义）；对外提供双态自报值（未注册/引用丢失自报 identity_value，已注册自报平台分配引用）；终态拒绝（自报值采纳失败 / 本地引用查无主体）时丢弃本地引用回退 identity_value 的处置入口；
- **凭证安装平面**：安装 = 复用 A0 验签 → 载荷 `device_id` 与本地注册引用比对 → 按 (authority_revision, credential_generation) 字典序高水位防回滚（同版本幂等重装放行）→ 原子提交；唯一版本回退拒绝分类＝凭证陈旧（`CREDENTIAL_STALE`），线上 errorCode 字符串由宿主映射；空 entitlements 清权凭证为合法安装对象；
- **通道侧元数据持久化**：policyVersion（对账上报轴）随安装本地持久化，不进 CBOR 载荷（P17 冻结面）；
- **信任密钥环**：内嵌生产密钥环（构建期由仓库内提交的环境公钥文件离线生成名单、编译进 exe；仓库内默认名单为空、fail-closed；D-109 取代原"打包期从签名后端导出公钥构件"）+ 状态根 append 覆盖文件（同 key_id 覆盖内嵌项）；条目形态 {key_id, X‖Y 64 字节}；完整轮换/分发机制为后续专项；
- **持有版本查询面**：供宿主桥读取 (authorityRevision, credentialGeneration, policyVersion) 三元组或「无凭证」态，用于应答版本探测与发起对账；
- **并列本地状态载体**：注册引用、已装凭证/高水位与通道侧元数据落于状态根下并列状态文件（DPAPI 信封 + 原子替换 + 既有互斥锁语义），machine-state 身份文件零改动；
- **CLI 宿主消费面**：`status` / `entitlement` 从硬编码 absent 改为读取已安装凭证；新增 `device-register`（双态查询/采纳/重置）、`install`（验签+安装）、`held-version`（持有版本三元组查询），命名对齐 P17 CLI family。

Does not include: 网络代码、通道协议、RPC 编排、回执上报（宿主桥）；许可平台台账/签发/签名后端（仓外）；完整密钥轮换/分发机制。

**Evidence maturation:** EV-A2-01…05（P20 v3 新证据类，blocking）；EV-02 设备本地安装/关联子集；EV-04 local association/anti-rollback/held-version 面.

### AXL-V1-R1 — Publish Pipeline (D-109 Current Authority)

**Outcome:** 让已经 Gate-closed 的 `axlic.exe` 具备可重复、可追溯的出厂通道：一条正式 `release` 工作流与一条仅手动触发的测试发布工作流，共用一条可复用流水线（构建 job 只读令牌 → 发布 job 可写令牌），在构建期用本环境提交在仓库内的公钥离线生成信任名单并编译进 exe，产物连同校验和与构建清单发布到本仓库的 GitHub Releases。验签代码、名单文档格式与默认本地构建（空名单）均不变。

Includes:

- **流水线结构**：两个薄入口（`release.yml`、`test-publish.yml`）调用 `axlic-publish.yml`（发布身份校验 → 离线名单生成 → 完整构建 → 全量测试与既有 A2 证据步骤 → 嵌入核对 → 打包 → 校验和复核后发布）；执行分支上的 `r1-windows.yml` 以双环境矩阵提供托管证据；
- **信任名单来源**：`client/keyring/license-test.pub.pem` 与 `license-prod.pub.pem` 提交在仓库内，`tools/build_trust_floor.py` 离线生成名单并核对指纹；每个产物只嵌入本环境的一把钥匙；
- **版本与发布规则**：正式版由推送 `vX.Y.Z` 标签触发，标签版本必须等于该提交 `CMakeLists.txt` 的项目版本且提交须在 `main` 上；测试版版本为 `<项目版本>-test.<yyyyMMdd>.<run_number>`，发为 Pre-release；正式版支持手动指定已存在标签重发并覆盖同名附件；
- **产物**：`axlic-<版本>-win-x64.zip`（`axlic.exe`、随包 VC++ 运行库、第三方声明与完整许可证文本、`SHA256SUMS.txt`、`build-manifest.json`）。

Does not include: 验签代码或名单格式改动、安装包/MSIX、Authenticode 签名、运行时覆盖写入侧与密钥轮换、W1 性能门禁、CI 自动触发测试发布。

**Positioning:** R1 产物是 **A8 之前的工程发布**，不是 A8 release Gate：不执行 EV-07 门禁；安装/升级契约、真实 TPM 资格认证与 EV-01～07 发布级证据仍归 AXL-V1-A8。

**Evidence maturation:** EV-R1-01…06（P20 v4 §18 新证据类，blocking）；既有 A2 证据步骤在待发布提交上复跑.

> **A3–A8 supersession note (D-106):** 下列 A3–A8 条目保留其设备侧语义（activation/refresh/assertion/offline/recovery/hardening 的本地半边），但其 server-side content（Node/TS 服务端、HTTPS transport adapter、PostgreSQL canonical store、fixture signer port 等）已随 D-106 撤销为本仓范围外，待宿主桥集成专项重新规划；在新的专项权威落地前，这些条目不授权任何 in-repo server/transport 实现。

### AXL-V1-A3 — Online Grant Activation + Credential Install

**Outcome:** 一个 admin/test commercial intent可以 Issue Grant → Activate Device → external signing port → canonical commit → C++ `axlic activate` / resolve delivery → verify-before-replace → local entitlement visible。

Includes:

- minimal Product/Entitlement catalog seed/register path；
- GrantAuthority / Binding / CredentialIssuance；
- PostgreSQL canonical unit-of-work；
- fixture signer port；
- CLI/network activation path；
- CredentialManager anti-rollback + atomic install；
- TypeScript server encoder必须逐向量匹配 EV-01 canonical bytes。

**Evidence maturation:** EV-01 cross-language signer/encoder integration；EV-02 activation/atomicity major subset；EV-04 credential/high-watermark coupling.

### AXL-V1-A4 — Dynamic Catalog + Grant Revision + Refresh

**Outcome:** register entitlement → explicit Grant revision → successor credential；ordinary `refresh` only resolves current committed credential and repeated no-change refresh has zero canonical mutation/generation churn。

Includes:

- CatalogRevision registry；
- ReviseLicenseGrant；
- CredentialResolver；
- explicit ReissueCredential separate path；
- C++ `axlic refresh`；
- 100x unchanged refresh invariant test。

**Evidence maturation:** EV-02 revision-domain / catalog-vs-grant / resolve-vs-reissue coverage becomes near-complete.

### AXL-V1-A5 — Purpose-Bound Device Assertion + Control-Plane Scope

**Outcome:** Product Backend reference harness可以取得 trusted challenge，通过 C++ `axlic device-assert` 产生 purpose-bound proof，Node/TS server verifier验证 audience/purpose/session/current identity；scope matrix阻止 device/product/release/factory actor越权。

Includes:

- ChallengeAuthority；
- DeviceAssertionCore/Verifier；
- service/admin/device logical principal abstraction；
- reference Product Backend client/harness only；不实现 Organization/IAM/ProductDeviceAssociation truth。

**Evidence maturation:** EV-05 materializable；EV-01 challenge/proof cross-language vectors complete.

### AXL-V1-A6 — Offline Activation / Import

**Outcome:** air-gap request export → connected server completion → signed response → offline import → verify/install；duplicate import idempotent、tamper/stale/non-canonical fail closed。

Includes:

- offline request/response transport envelope；
- CompleteOfflineActivation；
- C++ import/export CLI；
- bounded artifact parsing。

**Evidence maturation:** EV-01 offline negatives complete；EV-04 import atomicity/rollback coverage strengthened.

### AXL-V1-A7 — Recovery / Rehost / Factory / Administrative Lifecycle

**Outcome:** 完成 V1 剩余 lifecycle：RecoverDevice、RehostDevice、ProvisionDeviceIdentity、FactoryPreActivateDevice、Deactivate/Suspend/Resume/Revoke，以及 catalog/admin paths。

Includes:

- same-device recovery vs physical replacement separation；
- one-active-binding rehost transition；
- factory identity-only vs preactivation capacity separation；
- lifecycle event/audit atomicity。

**Evidence maturation:** EV-02 lifecycle corpus complete；EV-05 control-plane scopes expanded to factory/admin/release cases.

### AXL-V1-A8 — Windows Hardening + Release Qualification

**Outcome:** 将“功能实现”升级为 Windows V1 可独立 review 的 release candidate evidence surface。

Includes:

- installer/upgrade state preservation contract；
- real ACL/UAC/machine-write behavior；
- real TPM 2.0 qualification；
- reboot persistence / golden-image identity-free check；
- deterministic kill-point crash suite；
- cross-process writer contention；
- CLI contract/version/exit-family compatibility；
- diagnostics/support-bundle synthetic canary leak scan；
- release W1 benchmark；
- full EV-01～EV-07 evidence compilation inputs。

**Evidence maturation:** EV-03/EV-04/EV-06/EV-07 reach release-materialized form；EV-01/02/05 rerun against release candidate.

## 6. Evidence allocation

| Evidence | First useful materialization | Release-complete point |
| --- | --- | --- |
| EV-01 Canonical/Crypto | A0 | A6/A8 release rerun |
| EV-02 Domain Mutation | A2 (device-local install/association subset per D-106) | A7 + A8 rerun |
| EV-03 Windows Identity | A1 | A8 real TPM qualification |
| EV-04 Durability/Anti-Rollback | A1/A3 | A8 kill-point/concurrency |
| EV-05 Assertion/Scope | A5 | A7/A8 |
| EV-06 CLI/Diagnostics | A0 | A8 |
| EV-07 W1 Performance | A0 baseline | A8 release build |
| EV-R1 Publish pipeline | R1 | R1 (pipeline self-evidence; not an A8 release-Gate family) |

Rule: evidence在早期 slice出现并不自动代表 P20 blocking family最终 PASS；release Gate只能使用满足 P31/P34 exact binding 的最终 materialized evidence。

## 7. CI / execution environments

### 7.1 Hosted CI

Minimum matrix：

- **Windows / MSVC:** CMake configure/build、C++ unit tests、`axlic.exe` CLI/local-state/CNG software-provider tests、EV-01 client vectors、W1 benchmark smoke；
- **Linux:** Node.js/TypeScript server build/test、PostgreSQL integration/lifecycle tests、reference model/tools；
- **Cross-language conformance:** C++ verifier/encoder-facing fixtures and TypeScript server encoder/verifier must consume the same checked-in EV-01 golden vectors；
- formatting/lint/static checks：C++ formatting/static analysis + TypeScript formatting/lint/typecheck；
- exact compiler/CMake/Node/package/dependency versions pinned by the owning P31 package/lockfiles；no floating `latest` in Gate evidence。
- **Windows / MSVC publish pipeline (D-109, AXL-V1-R1):** 可复用流水线 `axlic-publish.yml`（发布身份校验、离线名单生成、完整构建与 A2 证据、嵌入核对、打包；发布 job 在校验和复核后发布到 GitHub Releases）；入口 `release.yml`（推 `vX.Y.Z` 标签 + 手动重发）与 `test-publish.yml`（仅手动）；仅使用已按 commit 固定的 `actions/checkout`、`actions/setup-python`、`actions/upload-artifact`，其余用 runner 自带的 `gh`；工作流不使用任何 secret（自动 `GITHUB_TOKEN` 除外）。

Hosted Windows runner不能替代 EV-03 physical TPM qualification。

Hosted Windows runner不能替代 EV-03 physical TPM qualification。

### 7.2 Controlled Windows qualification

至少一台 Windows 11 + TPM 2.0；在 V1 支持窗口内保留 Windows 10 qualification。该环境负责 EV-03、EV-04 real-platform subset、EV-07 release benchmark；结果必须能被 reviewer通过 exact revision + evidence ref解析。

### 7.3 Database test environment

PostgreSQL schema/invariant tests必须使用 real PostgreSQL instance，不允许 SQLite mock 被当作 canonical-store Gate evidence。

## 8. Empty-repository bootstrap rule

`Mostorm-Labs/axlic` 当前没有 commit。Aegis P32 repository execution要求 non-null revision-based `task_anchor`，因此第一个 P31 package之前必须先建立一个 **authority-neutral repository seed revision**。

P31 control-plane bootstrap规则：

1. 在 `main` 创建最小 non-product seed commit（例如 README / repository metadata；不得包含 A0 substantive implementation）；
2. 记录该 exact commit SHA；
3. 后续 materialize A0 package，并将 seed revision作为 `task_anchor` ancestor；
4. P32 从 package-defined branch/worktree开始 substantive mutation。

该 seed commit只解决 repository identity/ancestry，不成为 product implementation evidence，也不授权 coding。

## 9. P31 package strategy

每个 A0～A8 默认独立执行：

```
P31 package
→ P32 implementation
→ P34 Gate review
→ repository integration closure
→ successor routing / next package
```

不要一次冻结一个覆盖 A0～A7 的巨型 EXECUTION_CLOSURE_CONTRACT；后续 slice必须在前一 slice的 accepted repository baseline上包装。

每个 P31 package必须：

- 精确引用 Current Authority + P20 obligation；
- freeze scope/files/module ownership；
- freeze required vs forbidden changes；
- freeze exact tests/oracles；
- freeze blocking vs corroborative evidence；
- 给出 non-null task_anchor；
- 定义 reviewer-accessible materialized result；
- `continue_until_terminal_state: true`。

## 10. Parallelization policy

允许：

- A3 / A4 / A5 在 A2 accepted baseline之后并行准备；
- reference oracle/vector tooling与production implementation在不共享实现代码的条件下并行；
- Linux server tests与Windows adapter tests并行。

禁止：

- 两个未协调 package同时修改同一 canonical schema/domain mutation contract；
- 为赶并行进度复制一套 credential schema/codec到 server和device；
- Product Backend直接写 canonical DB；
- Windows adapter自行创造新的 semantic error/operation语义。

## 11. P30 disposition

P30 finds no remaining Authority or Verification gap preventing implementation packaging.

**P30 AxLicense V1 IMPLEMENTATION PLANNING ACCEPTED / CLOSED.**

Earliest untrusted layer becomes **P31 Task Packaging**.

Recommended first package: **AXL-V1-A0 — Local Signed-Credential Gate**.

P31 must first establish the authority-neutral seed revision required by §8, then freeze the A0 EXECUTION_CLOSURE_CONTRACT. No substantive coding is authorized until that package exists.
