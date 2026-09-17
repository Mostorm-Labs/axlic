---
authority_id: AXL-V1-P20
stage: P20
scope: axlicense
kind: verification-design
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c81cb8b62c8391702d839
migration_class: location-only
semantic_change: none
---

# 20 — P20 Verification Design — AxLicense V1 v0.1

> 🧪
>
> **Authority status: Accepted / P20 FR-019 Verification Design reconciliation CLOSED — 2026-09-13.** 本页基于 trusted P02/P03 + P10–P18 Current Authority，冻结 AxLicense Windows-first V1 的 verification contract：failure-mode-first evidence、V1 crypto profile、oracle/reference、fixture/corpus、exact execution、blocking/corroborative 分类与 Gate criteria。它不改变产品语义、operation、module/runtime/platform contract；只有能够独立发现尚未被其他机制充分覆盖的高影响 failure mode 的 evidence 才允许成为 blocking。

## 1. Stage contract

- **Role:** P20 Verification Design.
- **Authority:** P02/P03 reconciled requirements/capability traceability；P10–P13 semantic foundation；P14 D-078；P15 D-079；P16 D-080；P17 D-081 + D-085 canonical-signing repair；P18 D-082 + D-086 targeted impact revalidation；central routing D-087.
- **Objective:** 在实现前定义足够且稳定的证明目标，使后续 P30/P31 能把 implementation completion 与 Gate completion 冻结成明确合同，而不要求 coding agent 或 P34 临时发明新的 finish line。
- **Non-goals:** 不选择 cloud/DB/KMS vendor、HTTP route、installer、literal filesystem/mutex name；不重新定义 P10–P18 authority；不把每个 metric、每次 CI run、每个 fuzz seed 都升级成 blocking evidence；不要求 proof-of-proof 链。
- **Required analysis:** requirement/invariant → failure mode → existing independent coverage → residual proof gap → oracle/reference → fixture/corpus → exact execution → evidence artifact → blocking/corroborative → Gate criterion.
- **Required output:** V1 crypto profile；blocking Evidence Artifact set；corroborative evidence set；fixture/corpus contracts；reference environment；Gate acceptance criterion；downstream materialization requirements.
- **Quality / Evidence Gate:** 每个 blocking artifact 必须能独立发现至少一个高影响且未被其他独立机制充分覆盖的 failure mode，并记录 unique detection value；相同测试在 local/CI 重复执行不构成第二份独立 coverage。
- **Handoff:** P20 CLOSED 后返回 central `aegis` successor routing；`aegis-verification` 不直接执行 P30/P31。

## 2. Verification posture

### 2.1 Evidence classes

1. **Blocking correctness/security/compatibility evidence** — 缺失会留下明确的高影响 residual proof gap。
2. **Blocking release-interactive performance evidence** — 仅限已冻结且直接影响产品启动/交互的 reference workload；不是所有 P18 metric。
3. **Corroborative evidence** — 增强置信度但不独立关闭新的高影响 gap。
4. **Diagnostic/observability evidence** — 帮助定位问题，本身不是 PASS authority。

### 2.2 Anti-proof-recursion

以下默认不成为独立 blocking requirement：

- 为证明 golden-vector generator 正确而再建一个 generator 的 generator；
- local 与 hosted CI 各跑一次完全相同的 suite；
- 为证明 evidence manifest 正确再创建 provenance-of-provenance；
- 随机 fuzz seed 数量本身。

只有当 evidence mechanism 自身成为一个未被其他机制检测的高影响 failure source 时才允许升级。

## 3. Windows-first V1 cryptographic verification profile

P17 已冻结 exact canonical bytes；P20 冻结 V1 接受的 crypto profile。

### 3.1 Profile identifier

- Credential/challenge envelope `algorithm_id`: **`axl-ecdsa-p256-sha256-v1`**.
- Curve: **NIST P-256 / secp256r1**.
- Hash: **SHA-256**.
- Device proof Windows V1 provider profile同样使用 P-256 + SHA-256；`scheme_id` / `proof_scheme_id` 仍区分 TPM hardware-bound 与 software-persistent assurance，具体稳定 scheme literal 由 platform/implementation registry 固化，但不得改变本页算法 profile。

### 3.2 Signature representation

- `signature_bytes` / P-256 proof bytes使用固定 **64 bytes**：`r || s`，每个 component 为 32-byte unsigned big-endian、left-zero-padded。
- `r`、`s` 必须满足 ECDSA range validation。
- V1 接受的 artifact signature 必须使用 **low-S** canonical form；signing adapter 若得到 high-S，必须在 artifact publication 前规范化为 `min(s, n-s)`；verifier 对 high-S equivalent signature fail closed，避免同一有效签名拥有两个被接受的 artifact byte forms。
- Production signer 不要求 deterministic nonce/signature byte equality；provider/HSM 负责安全 nonce。Golden verification vector 可以使用固定 test signature，但 production signer 只要求输出可由独立 verifier 验证且满足 fixed-width + low-S。

### 3.3 Key-purpose separation and rotation

- Credential signing key、Challenge signing key必须为不同 logical purpose/key identity；device identity key永远是 per-device provider-owned key。
- `key_id` 解析到 exact trusted public key；unknown/retired key fail closed。
- rotation window允许 verifier 同时保留 current + explicitly retained previous verification key；issuer只使用 current signing key。Key rotation不得改变 P17 canonical payload/signing-input rules。
- test corpus中的 private key只允许 dedicated fixture key，绝不得与 production KMS/HSM/device key共用。

### 3.4 Challenge digest

P17 定义的 `challenge_digest` 使用本 profile 的 **SHA-256** 对 exact `ChallengeSigningInputV1` bytes 计算。不得对 parse 后对象、JSON carrier 或 re-encoded equivalent object 计算替代 digest。

## 4. Failure-mode / evidence map

| ID | Invariant / high-impact failure mode | Primary oracle | Evidence | Class |
| --- | --- | --- | --- | --- |
| FM-01 | Tampered、forged、non-canonical、wrong-domain、unknown-key/algorithm credential/challenge 被接受 | Frozen literal canonical bytes + independent crypto verifier | EV-01 Canonical/Crypto Conformance Corpus | Blocking |
| FM-02 | same logical retry / response loss 产生第二个 Device、Grant mutation、Binding、capacity consumption 或 inconsistent result | P13 transition oracle + canonical-state invariant projection | EV-02 Domain Mutation Conformance | Blocking |
| FM-03 | one-active-binding、revision domain、refresh/reissue separation 或 catalog/grant separation 被破坏 | Reference transition sequences + post-state invariants | EV-02 Domain Mutation Conformance | Blocking |
| FM-04 | TPM 临时故障 silent downgrade；provider pin 丢失；private identity key 可被应用层导出 | Real Windows CNG/KSP observation + injected provider faults | EV-03 Windows Identity Qualification | Blocking |
| FM-05 | crash/concurrent writer/partial replace 产生半状态、credential/high-watermark 分叉或 rollback acceptance | Kill-point/crash oracle + old-or-new generation invariant | EV-04 Local Durability & Anti-Rollback | Blocking |
| FM-06 | Device proof 被跨 purpose/audience/session/challenge replay；调用者 scope 越权成 license/factory/catalog authority | Purpose-bound assertion oracle + explicit caller/operation authorization matrix | EV-05 Assertion & Control-Plane Scope | Blocking |
| FM-07 | public CLI contract drift、敏感值进入 argv/stdout/stderr/log/support bundle，导致产品兼容失败或 secret disclosure | Machine-readable contract oracle + canary leak scanner | EV-06 CLI & Diagnostics Contract | Blocking |
| FM-08 | local entitlement path严重超预算，使消费产品启动/交互被 AxLicense 本地路径阻塞 | P18 reference workload + percentile benchmark | EV-07 W1 Startup-Critical Benchmark | Blocking at release Gate |
| FM-09 | malformed/oversized CBOR触发 unbounded CPU/RAM 或 parser crash | bounded negative corpus + parser limits | EV-01 + EV-04 deterministic negatives | Blocking |

## 5. EV-01 — Canonical / Crypto Conformance Corpus

### Unique detection value

功能 happy-path test 无法独立检测 canonical-byte drift、signature encoding ambiguity、domain confusion 或 non-canonical acceptance；因此该 corpus 是 blocking。

### Corpus

至少包含：

- canonical credential：presence entitlement、bounded-u64 entitlement、perpetual validity、bounded validity、optional supersedes absent/present；
- entitlement list输入顺序不同但 canonical payload bytes必须一致；
- trusted challenge positive vectors；
- device-proof transcript positive vectors；
- one-bit / one-field tamper vectors for every authorization-critical field；
- wrong `artifact_kind` / domain string / `algorithm_id` / `key_id`；
- unknown schema major / unknown authorization-critical enum；
- non-canonical CBOR：non-minimal integer/length、indefinite-length、tag、forbidden null、duplicate map key、wrong canonical key order；
- signature negatives：wrong length、r/s out of range、high-S equivalent、wrong public key；
- bounded parser negatives：oversized artifact、excess nesting/item count.

### Independent oracle/reference

- Freeze literal semantic fixture + expected canonical `payload_bytes` + exact P17 signing-input bytes as checked-in golden data.
- Generate/freeze corpus with a small **non-production reference encoder** that directly implements the P17 field map/restrictions; production encoder code must not be reused by the oracle.
- Signature verification cross-check uses an independent standards implementation such as OpenSSL-equivalent P-256/SHA-256 verifier; implementation signer output is accepted when that independent verifier validates it and fixed-width/low-S rules hold. Exact production signature bytes are not compared for equality.

### Exact execution

1. Production encoder consumes semantic fixture → byte-for-byte compare with golden payload/signing input.
2. Production verifier consumes fixed signed golden artifacts → positives accepted, every negative rejected with fail-closed family.
3. Production signer/test signing adapter signs golden input → independent verifier validates signature; assert 64-byte low-S.
4. Repeated encode of same semantic object must be byte-identical.

### Evidence artifact

`EV-01` report must include corpus version/hash, implementation revision, pass/fail per vector and independent-verifier result. Raw private fixture key need not be emitted into the report.

### Gate criterion

All blocking positive/negative vectors PASS. Any accepted non-canonical/tampered/wrong-domain/wrong-key artifact is Gate BLOCKED.

## 6. EV-02 — Domain Mutation / Lifecycle Conformance

### Unique detection value

Crypto tests不能发现 duplicate binding、revision churn、response-loss replay、catalog/grant coupling 或 cross-operation atomicity failures；因此独立 blocking。

### Reference model

建立与 production storage/application code分离的 table-driven transition oracle，只表达 P13 frozen outcomes和 invariants，不复用 production domain implementation。

### Mandatory scenario families

- `RegisterDeviceIdentity`: new、same-operation retry、fresh-correlation same identity、response lost after commit、concurrent same identity、identity conflict；
- `ActivateDevice` / `CompleteOfflineActivation`: first bind、same-device replay、different-device conflict、response loss；
- `RehostDevice`: old binding closed + exactly one new active binding + successor credential；
- `ReviseLicenseGrant`: rights change increments `authority_revision`; bound grant emits successor credential in same logical outcome; no-op no churn；
- `ResolveCurrentCredential`: repeat **100 times** against unchanged state with zero canonical mutation / zero generation change；
- `ReissueCredential`: same authorization revision, generation +1, authorization-equivalent snapshot；
- Catalog registration/state change only advances CatalogRevision, never customer Grant/credential；
- Factory identity-only vs pre-activation capacity separation；
- RecoverDevice same-device continuity vs rehost boundary；
- offline import duplicate/idempotent install.

### Exact execution

Run scenario corpus against actual application service + production persistence adapter. After each sequence, collect canonical state projection and operation result; compare with reference transition outcome and run invariant queries for active binding uniqueness, revision monotonicity, lifecycle-event atomicity and operation-result idempotency.

Concurrency cases must use real concurrent transactions/processes where the production deployment supports them, not only sequential mocks.

### Evidence artifact

`EV-02` = scenario manifest + implementation revision + final-state projections + invariant-query results. DB dumps containing secrets are not required.

### Gate criterion

No divergence from frozen outcomes; one-active-binding/revision/idempotency/atomicity invariants all PASS.

## 7. EV-03 — Windows Device Identity Provider Qualification

### Unique detection value

Fake-provider/unit tests不能证明真实 Windows CNG/KSP non-exportability、persistence/provider selection或真实 TPM integration；因此需要 real-platform evidence。

### Reference matrix

- one Windows 10 V1 reference environment and one Windows 11 V1 reference environment；
- at least one physical TPM 2.0 machine using Microsoft Platform Crypto Provider-equivalent realization；
- software-persistent fallback environment using Microsoft Software Key Storage Provider-equivalent realization；
- transient-provider-failure cases may use deterministic injected adapter faults when destructive real-TPM fault injection is impractical, but positive provider behavior/non-exportability must be real-platform.

### Mandatory checks

- usable TPM wins first establishment；
- absent/permanently unusable TPM + policy allow → software fallback；
- transient TPM/provider unavailable → retryable failure, **no software identity creation**；
- after L0 provider/scheme remains pinned across process restart/OS restart；
- persisted public identity remains stable；
- app-facing/provider API cannot export private key bytes；explicit private-key export attempt fails under the configured non-exportable key policy；
- missing/corrupt pinned key routes to recovery, not ordinary new identity；
- golden image/template contains no machine identity key/device association/credential.

### Evidence artifact

`EV-03` records Windows build, TPM/provider class, policy mode, observed provider/scheme, key persistence/non-export test result and fault-injection scenario result. Do not log key handle/name when policy marks it sensitive.

### Gate criterion

No silent downgrade, no exportable device private key through AxLicense/public/provider path, and stable pinned identity behavior PASS.

## 8. EV-04 — Local Durability / Concurrency / Anti-Rollback

### Unique detection value

Domain/unit tests不能证明 Windows machine-wide state 在真实 process crash、cross-process concurrency、filesystem replace 与 ACL 下保持 old-or-new atomicity；因此独立 blocking。

### Mandatory fault points

Test-only failpoints around local mutation：before temp write、after temp write、after flush、immediately before atomic replace、immediately after replace before success publication. At each point terminate the writer process abruptly and restart read path.

### Required invariants

- observed state after restart is exactly old committed generation or new committed generation；never partial/mixed；
- credential bytes and local authority/generation high-watermark never diverge；
- lower authority revision or same-revision lower generation rejected；same revision/generation different bytes fail closed；
- two concurrent mutating `axlic.exe` processes serialize through machine-wide lock; loser re-reads committed state；
- ordinary read sees old/new complete snapshot only；
- standard user without write authority cannot mutate protected state；
- malformed/oversized/non-canonical local/offline artifact fails before unbounded allocation/expensive crypto when practical.

### Evidence artifact

`EV-04` = kill-point matrix + concurrent-process matrix + anti-rollback negative corpus + ACL result.

### Gate criterion

Every deterministic crash/concurrency/rollback case satisfies the invariants; any partial state or rollback acceptance BLOCKS.

## 9. EV-05 — Device Assertion / Control-Plane Scope Separation

### Unique detection value

Correct signatures alone不能证明 proof purpose binding 与 caller authorization scope；这是独立的 privilege-escalation/replay failure mode，因此 blocking。

### Device assertion matrix

For a valid challenge/proof pair, independently alter `purpose`, `audience`, `session_ref`, nonce/challenge digest, expiry, expected device and device identity context. Every retargeted/replayed form must reject. Proof for one challenge must not authenticate another challenge even for the same Device.

### Caller-scope matrix

At minimum model callers: ordinary device/bootstrap、Product Backend service、Release/Admin catalog publisher、Factory identity-provision scope、Factory pre-activation scope、Support/Rehost authority、License admin. Verify allow/deny for their relevant P13 operations.

Required negatives include：

- device/bootstrap cannot Issue/Revise Grant；
- Product Backend runtime cannot publish catalog by merely possessing device proof；
- Release/Admin catalog scope cannot mutate customer Grant；
- factory `identity_provision` scope cannot pre-activate/license；
- factory pre-activation scope cannot become general license-admin authority；
- Product Backend/service caller cannot access Canonical Store or Signing Authority directly through public control-plane contract.

### Transport trust negatives

Windows device transport must reject invalid certificate/hostname trust and must not expose production insecure bypass. HTTP/transport error cannot be interpreted as semantic success.

### Evidence artifact

`EV-05` = assertion replay/tamper matrix + caller/operation allow-deny matrix + transport trust negative results.

### Gate criterion

Zero unauthorized operation succeeds; all purpose/audience/session/challenge retargeting fails closed.

## 10. EV-06 — CLI / Diagnostics Public Contract

### Unique detection value

Domain tests不能检测产品集成 wire drift、argv disclosure 或 diagnostic leakage；这是独立 compatibility/security surface，因此 blocking。

### CLI contract cases

- direct process creation, no shell dependency；
- structured request accepted through stdin；
- stdout contains exactly one versioned structured result document, no progress/free-form logs；
- product behavior maps from stable `code/category/retryable`, not stderr wording；
- major contract mismatch rejects；supported minor additive fields remain ignorable；
- unknown authorization-critical request semantics fail closed；
- read-only command runs without machine-write privilege；mutating command without required privilege returns stable semantic family.

### Minimum-disclosure canary test

Inject unique synthetic canaries representing activation/license secret、bearer/service token、proof payload、offline artifact body、raw identity claim、provider key reference/name. Exercise success and failure paths, diagnostics and support-bundle generation. Scan argv/process command line, stdout, stderr, log files and exported support bundle.

Forbidden canaries must not appear outside an explicitly allowlisted test-only location. Stable error code/correlation ref/safe provider class are allowed; raw secret/private material is not.

### Evidence artifact

`EV-06` = CLI compatibility matrix + process-command-line capture + canary scan report + support-bundle allowlist result.

### Gate criterion

Public contract cases PASS and secret-canary scan reports zero forbidden disclosures.

## 11. EV-07 — W1 Startup-Critical Performance Gate

### Unique detection value

Correctness tests cannot detect a severe local startup/entitlement latency regression that directly blocks consumer-product startup. This is the only P18 performance family frozen as a mandatory release-blocking benchmark by default.

### Exact execution

On the P31-frozen Windows reference device, Release build, representative signed credential and no network dependency:

1. warm-up 20 independent `axlic.exe` invocations；
2. measure at least **200** independent W1 `status`/entitlement invocations；
3. report p50/p95/p99 and sample count；
4. separately instrument credential parse/canonicality/signature/schema/entitlement verify segment.

### Gate criterion

- W1 warm: **p95 ≤ 150 ms, p99 ≤ 300 ms**；
- credential verify segment: **p95 ≤ 50 ms**；
- benchmark environment and build identity must be recorded.

P18 cold W1、W5 install、memory ceilings、server latency、signer latency、bulk throughput remain measured/corroborative unless a later explicit release requirement promotes one due to a distinct uncovered high-impact failure mode. Missing corroborative benchmark alone does not block.

## 12. Corroborative / non-blocking evidence

Unless promoted by a new explicit high-impact uncovered failure mode, the following remain non-blocking confidence/hardening evidence：

- random/coverage-guided fuzzing beyond EV-01/EV-04 deterministic negative corpora；
- local + hosted CI duplicate runs of the same oracle；
- cold-start extended benchmark, W5 install latency, full memory/resource profiling；
- server load/soak, bulk rollout throughput and queue-age studies before production capacity/SLA is frozen；
- alternate DB/KMS/HSM vendor matrix；
- manual support-bundle review in addition to EV-06 canary scanner；
- static analysis/sanitizer coverage when it does not uniquely close one of the frozen blocking failure modes.

A P34 reviewer may record failures here as findings/hardening work, but cannot retroactively make them blocking merely because more confidence would be useful.

## 13. Fixture / corpus organization contract

Downstream implementation package should materialize logical equivalents of：

```
verification/
  vectors/
    credential/
    challenge/
    device-proof/
    negative-canonical/
  scenarios/
    domain/
    assertion-authz/
  local-state/
    crash-matrix/
    anti-rollback/
  cli-diagnostics/
  performance/
```

Exact repository path may change in P30/P31, but fixture identity/version/hash must be stable once frozen for an execution package.

Fixture data must use synthetic IDs and test-only keys. Production customer/device identity, license keys, tokens or private keys are forbidden from evidence corpora.

## 14. Gate evidence manifest

Every blocking evidence bundle must record：

```yaml
artifact_id: EV-xx
implementation_revision: <exact source revision>
authority_revision: P20-FR019-v0.1
fixture_or_corpus_id: <stable id/hash>
execution_environment: <platform/build/provider/db as relevant>
command_or_runner: <exact reproducible entrypoint>
result: PASS | FAIL
failures: []
```

For each blocking artifact, P31/P34 must preserve its P20 qualification：

```yaml
failure_mode: <FM-id>
impact: <why material>
existing_independent_coverage: <what else detects / does not detect it>
new_evidence_unique_detection_value: <residual gap closed>
blocking_justification: <why absence changes Gate decision>
classification: blocking
```

Do not add a separate blocking artifact merely to prove this manifest exists unless manifest corruption itself becomes a distinct material undetected failure mode.

## 15. Reference environment and evidence materialization

- Exact source revision and test fixture identity are mandatory for blocking evidence.
- EV-03/EV-04/EV-06/EV-07 require Windows execution; EV-03 additionally requires real CNG/KSP and at least one physical TPM 2.0 qualification.
- Hosted CI is useful for reproducibility, but a hosted rerun of the same mechanism is the same detection coverage. Real-platform evidence may come from controlled self-hosted Windows runner or recorded release qualification machine.
- No current repository implementation exists; therefore P20 freezes the proof contract, not observed PASS claims. Evidence becomes materialized only after the relevant implementation exists.

## 16. P20 closure / downstream Gate contract

P20 finds **no remaining upstream authority gap** after D-085/D-086. Important V1 requirements now have a defined credible proof path, and every blocking Evidence Artifact has an explicit unique high-impact detection rationale.

**P20 FR-019 VERIFICATION DESIGN RECONCILIATION ACCEPTED / CLOSED.**

Before P31 authorizes code as complete, downstream authority must be able to freeze the seven blocking evidence families above into its `EXECUTION_CLOSURE_CONTRACT`; implementation does not need those artifacts to already PASS before coding, but it must know exactly what terminal PASS requires.

**Successor routing:** return to central `aegis`. P20 does not directly invoke P30/P31.
