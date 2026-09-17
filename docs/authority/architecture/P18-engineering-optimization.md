---
authority_id: AXL-V1-P18
stage: P18
scope: axlicense
kind: engineering-optimization
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c8182a28aed9fa1a54a46
migration_class: location-only
semantic_change: none
---

# 18 — P18 Engineering & Optimization — AxLicense V1 v0.1

> 📐
>
> **Authority status: Accepted / P18 FR-019 Engineering & Optimization reconciliation + D-085 targeted impact revalidation CLOSED — 2026-09-13.** 本页把 trusted P10–P17 FR-019 authority 转成可测量的 workload、latency/resource budget、cache/backoff、contention、observability 与 rollback/reference-path contract。D-085 新增的 deterministic canonical signing representation 已完成 targeted impact revalidation：它不改变既有 workload、resource ceiling 或 authority semantics，但 canonical encode/decode/canonicality-check 成本必须进入 measurement decomposition。当前实现尚未完成，因此所有 observed baseline 均标记为 **UNMEASURED**；本页冻结的是 V1 engineering budget 与 measurement method，不伪造 benchmark，也不允许为了性能改变 P10–P17 semantics。

## 1. Stage contract

- **Role:** P18 Engineering / Optimization.
- **Authority:** trusted P10–P13 semantics；P14 D-078；P15 D-079；P16 D-080；P17 D-081 + D-085 canonical-signing targeted repair.
- **Objective:** 在不改变 authority/operation/module/runtime/platform contract 的前提下，为 Windows-first V1 定义 workload、metric、initial budget、resource ceiling、contention/backoff、cache、observability、degradation 和 rollback/reference path。
- **Non-goals:** 不冻结最终密码算法/密钥长度、生产 SLO/商业 SLA、云厂商/数据库/KMS 品牌、实际部署规模、literal path/mutex/HTTP route、安装器品牌；不以性能理由引入新的 Agent/Service、第二套 local authority、unsigned shortcut 或 weaker identity fallback。
- **Evidence posture:** `baseline = UNMEASURED` 是合法起点。P20 必须把 blocking verification 限定为能够发现独立高影响 failure mode 的证据；性能 benchmark 本身不因为“可以测”就成为 blocking Gate。
- **Handoff:** P18 CLOSED 后 architecture family P14–P18 完成。下一 owner 由中央 `aegis` successor routing 决定；预期验证 authority 需要基于 P10–P18 重新 reconciliation，而不是直接进入 implementation。

## 2. Optimization invariants — never trade these away

任何优化都不得破坏：

1. one active DeviceBinding per Grant；
2. signed credential immutable + self-sufficient offline evaluation；
3. `authority_revision` / `credential_generation` / `CatalogRevision` 分离；
4. ordinary `refresh` = resolve/delivery，不触发 issuance；
5. TPM/provider pinning + no silent downgrade；
6. machine-wide single-writer、re-read-after-lock、verify-before-replace、crash-safe atomic replace；
7. Product SaaS / AxLicense canonical / device-local observation 独立；
8. canonical mutation 只能经 `OperationCore + CanonicalUnitOfWork`；
9. private identity key 继续由 CNG KSP 持有；
10. Windows V1 继续 CLI-only；优化不得默认引入 public DLL 或 resident Agent/Service。

## 3. Reference workload classes

| ID | Workload | Critical path | Frequency class |
| --- | --- | --- | --- |
| W1 | Local `status` / entitlement evaluation | process start → protected state read → credential verify → identity association check → structured result | frequent / interactive |
| W2 | First local identity establishment | provider probe → KSP persisted-key create/load → local association commit | rare / once per identity epoch |
| W3 | Ordinary `RegisterDeviceIdentity` | proof → HTTPS → DeviceRegistry canonical commit → local device_id association | rare |
| W4 | Ordinary `refresh`, no change | process → HTTPS → CredentialResolver read → no-op result | periodic / reconnect / app lifecycle |
| W5 | Refresh with newer already-authoritative credential | resolver → download → verify → local single-writer atomic replace | occasional |
| W6 | Activate / Rehost / bound Grant revision | validation → signer candidate → CAS/revalidate → ACID commit → delivery | rare / authority mutation |
| W7 | Catalog publication | definition validation → catalog commit → CatalogRevision | release/admin |
| W8 | Bulk Grant rollout | per-Grant independent revise/sign/commit, bounded worker concurrency | campaign / burst |
| W9 | Offline import | artifact parse → verify → anti-rollback → atomic install | rare / air-gap |
| W10 | Diagnostics / support bundle | bounded safe projection + explicit export | support-only |

## 4. Measurement rules

### 4.1 Device reference measurement

Implementation/P20 benchmark must report at minimum p50/p95/p99 and sample count for:

- process launch to first structured result;
- local state open/decode;
- credential signature/schema/device/anti-rollback verification;
- CNG provider probe/open;
- TPM proof;
- initial persisted identity-key creation;
- machine mutation lock wait;
- candidate write + flush + atomic replace;
- end-to-end refresh with loopback/stub server and with representative network separately.

Cold and warm results must not be merged. Tests must identify CPU class, storage class, Windows build, TPM/provider class and whether Defender/endpoint security is active.

### 4.2 Server reference measurement

Report separately:

- transport/auth parsing;
- canonical DB read;
- canonical DB transaction/CAS;
- signer latency;
- full operation latency;
- resolver read latency;
- queue wait under bulk rollout;
- conflict/retry rate.

Network RTT and external KMS/HSM latency must be separable from application processing.

### 4.3 Baseline rule

Until measured on implementation, every baseline field is **UNMEASURED**. No synthetic target may be presented as observed performance.

## 5. Windows device latency budgets

The following are **initial V1 engineering budgets**, not measured claims. P20 may tighten them with evidence but must not silently weaken security semantics to achieve them.

| Path | Budget | Notes |
| --- | --- | --- |
| W1 local status/entitlement, warm | p95 ≤ 150 ms; p99 ≤ 300 ms | includes `axlic.exe` process startup + local evaluation; no network |
| W1 local status/entitlement, cold | p95 ≤ 300 ms; p99 ≤ 600 ms | cold file/code cache; excludes OS logon/storage pathological stalls |
| credential verify inside process | p95 ≤ 50 ms | typical V1 credential size; signature + schema + entitlement projection |
| uncontended mutation-lock acquisition | p95 ≤ 10 ms | excluding an active writer |
| local candidate flush + atomic replace | p95 ≤ 100 ms | normal SSD/reference storage |
| CNG persisted-key open / ordinary proof | p95 ≤ 750 ms | TPM/KSP class measured separately |
| first identity establishment | p95 ≤ 3 s; p99 ≤ 5 s | rare path; includes provider probe + persisted key creation, not user UAC time |
| W4 refresh no-change, healthy network | device-visible p95 ≤ 2 s | network RTT included in this user budget; server processing also measured separately |
| W5 local install after credential received | p95 ≤ 250 ms | verify + lock + re-read + atomic replace |

If a reference device misses a latency budget while correctness/security tests pass, the default response is profiling/optimization or UX decoupling; do not replace TPM identity with software identity, skip verification, weaken flush, or introduce unsigned cached authorization.

## 6. Product invocation guidance

- Product startup must not require network `refresh` before local entitlement evaluation. W1 remains the fast offline gating path.
- Product may trigger refresh asynchronously after local evaluation where product UX allows; P16 authority remains valid even if refresh completes later.
- Do not poll `axlic.exe refresh` continuously. Refresh is lifecycle/event-driven: app start where appropriate, reconnect, explicit admin/user action, software update or product policy interval.
- If one product interaction needs several entitlement answers, integration should avoid needless process-spawn amplification by using the existing structured entitlement result capability/bounded query shape; P18 does not create a new authorization semantic or in-process DLL shortcut.

## 7. Process and memory resource budget

Initial Windows V1 engineering ceilings:

- read-only `axlic.exe` peak working set target: **≤ 64 MiB p95** on reference workload;
- mutating/crypto/network command peak working set target: **≤ 96 MiB p95**;
- one-shot command should release all process memory on exit; no hidden resident cache process;
- local authoritative state excluding diagnostics/support bundles target: **≤ 4 MiB** per machine for V1 normal operation;
- a single SignedLicenseCredential / offline response must remain bounded; implementation guardrail **≤ 256 KiB** unless a later schema authority justifies more;
- persistent diagnostics ring/cache default budget target: **≤ 10 MiB**; support bundle is explicit export and may exceed this only under a separate bounded export policy.

These are engineering guardrails, not entitlement/schema limits. Exceeding them requires evidence and explicit engineering review, not silent format truncation.

## 8. Local I/O and state optimization

- Normal W1 read path should require a bounded number of local file/envelope reads and no rewrite.
- Do not rewrite the machine-state envelope on every read, every app start, or every no-op refresh.
- High-watermark/credential metadata may be indexed within the same protected generation, but the index is not a second authority file.
- Temporary files are cleaned lazily/safely only after committed state is selected; cleanup failure does not invalidate a valid committed generation.
- Antivirus/endpoint-security contention must be measured as an environment dimension before considering weaker flush/replace semantics.

## 9. Lock contention and backpressure

### 9.1 Device-side lock policy

- no network wait should hold the machine writer lock unless the P16 operation ordering truly requires it;
- preferred pattern: prepare/network work → acquire lock → re-read/revalidate → bounded local commit;
- writer wait longer than **2 s** should become observable and may return stable `LOCAL_STATE_BUSY`/retry semantics rather than blocking indefinitely;
- lock timeout is not permission to bypass serialization.

### 9.2 Server-side contention

- optimistic authority revision/CAS conflict is expected concurrency control, not an exception requiring global serialization;
- hot-Grant retries use bounded jitter and must re-read current authority before retry;
- bulk rollout uses bounded work queues; queue saturation slows producers rather than opening unbounded threads/connections.

## 10. Retry and backoff policy

- Retry only operations classified retryable by stable semantic code plus transport context.
- Preserve the same logical operation/idempotency identity across ambiguous retry.
- Interactive network retry reference schedule: jittered exponential backoff approximately **250 ms → 500 ms → 1 s → 2 s**, then return control with retryable status rather than blocking a foreground product indefinitely.
- Background Product Backend coordinators may continue with exponential backoff capped at **30 s** between attempts plus jitter; exact total retry window is product policy.
- Never automatically retry: TLS trust failure, signature/schema validation failure, policy denied, semantic conflict requiring new authority input, stale credential import, or non-retryable provider unsupported.
- Retry storms after service recovery must be damped with jitter; no synchronized fixed-period polling fleet-wide.

## 11. Server latency decomposition and budgets

Initial application-processing budgets, measured excluding client Internet RTT and reported with signer/DB components separately:

| Server path | Initial budget | Notes |
| --- | --- | --- |
| `ResolveCurrentCredential` no-change | p95 ≤ 150 ms app-side | read-only; no signer |
| `RegisterDeviceIdentity` canonical work | p95 ≤ 300 ms excluding device proof generation/network | uniqueness + operation result commit |
| catalog register/state mutation | p95 ≤ 300 ms excluding caller network | single catalog canonical mutation |
| Grant mutation DB/CAS excluding signer | p95 ≤ 300 ms | per-Grant unit |
| external signing call | p95 target ≤ 750 ms | reported independently; deployment/vendor dependent |
| activation/rehost/bound-Grant-revision full app path | p95 target ≤ 1.5 s excluding client Internet RTT | includes signer + canonical transaction |

A signer slowdown must not cause DB transaction locks to be held across the entire external signing latency where the P14/P16 prepare-sign-CAS pattern can avoid it.

## 12. Canonical database and query-shape rules

- Device identity lookup, Grant lookup, active-binding uniqueness, operation-idempotency lookup, current credential resolution and entitlement-definition lookup require indexed/direct lookup paths; no normal interactive path may require full-table scan.
- `ResolveCurrentCredential` must not join through Product SaaS/billing/order tables.
- Catalog cache can improve read latency, but authoritative registration/state mutation always commits canonical registry + CatalogRevision first.
- Bulk Grant rollout is per-Grant work; do not create a transaction spanning many Grants to improve throughput.
- append-only lifecycle/audit event volume may be partitioned/archived operationally later, but current-authority decisions cannot depend on an eventually consistent analytics copy.

## 13. Cache policy

### 13.1 Device

- no resident in-memory authority cache is required in V1 because `axlic.exe` is one-shot;
- the locally installed signed credential is durable observation, not a performance cache that may be bypassed by a faster unsigned projection;
- within one process invocation, parsed credential/public-key metadata may be memoized only for that invocation.

### 13.2 Server catalog

- immutable Product/Entitlement definition semantics are cache-friendly; cache entries must be keyed/versioned by stable IDs and invalidated/revalidated by `CatalogRevision` or equivalent canonical version signal;
- cache miss falls back to canonical store; stale cache may delay administrative presentation but cannot authorize a Grant mutation against semantics different from canonical validation.

### 13.3 Credential resolver

- current credential delivery may use read-through/cache/storage optimization only if returned artifact identity maps to canonical current binding/revision/generation;
- cache must never synthesize a successor credential or turn no-change refresh into reissue.

## 14. Bulk rollout engineering model

P18 does not invent a fleet-size requirement. Implementation benchmark therefore uses a parameterized workload and must publish throughput slope rather than a single marketing number.

- workers operate on independent Grants with bounded concurrency `N`;
- each work item has its own operation identity, expected authority revision and terminal result;
- conflict/failure of one Grant does not roll back others;
- recommended starting concurrency is conservative and configurable; tune from DB pool, signer quotas and p95 latency instead of hard-coding CPU count;
- queue depth, age, success/conflict/retry/failure counts are observable;
- pause/resume of campaign coordination does not alter already-committed Grant authority.

## 15. Observability contract

### 15.1 Required metrics

Device-side benchmark/diagnostic instrumentation should expose bounded measurements for process startup, local read, provider operation, lock wait, credential verify, atomic replace and network call duration.

Server production metrics should include operation count/result family, p50/p95/p99 latency by operation family, DB transaction/CAS latency, signer latency/error, resolver latency, idempotent replay count, authority-revision conflict count, queue depth/age, and cache hit/miss where a cache exists.

### 15.2 Cardinality/privacy rules

- metric labels must not contain raw `device_id`, DeviceIdentity value, customer/org name, license key, entitlement list, credential bytes, provider key name or correlation UUID;
- correlation references belong in bounded structured logs/traces, not high-cardinality metric labels;
- diagnostics may use opaque/truncated/hash-safe references only when support value justifies them and policy permits;
- no private key material, possession proof, reusable bearer credential or signing secret is ever logged.

### 15.3 Sampling

Normal success logs should be sparse/structured. Error and security-denied events may retain more diagnostic context within bounded policy. High-volume refresh success must not create unbounded logs.

## 16. Diagnostics and support-bundle budget

- default local logs use bounded rotation/ring retention; log exhaustion cannot block license evaluation;
- support bundle generation reads a snapshot and must not acquire the machine writer for long-running compression/export;
- support bundle has explicit size/time limit and redaction pass;
- if a diagnostic field is not needed to distinguish a supported failure class, omit it by default;
- raw platform error codes may be included only with safe contextual projection; raw key/provider-private payloads remain forbidden.

## 17. Degradation behavior

When a resource is slow/unavailable:

- local offline entitlement evaluation continues without server dependence if installed credential + pinned identity are usable;
- network refresh failure returns existing local observation unchanged;
- Catalog cache outage falls back to canonical store or returns administrative unavailable; it cannot grant by stale permissive default;
- signer unavailable blocks new credential-producing mutations but not read-only runtime/resolver delivery of already-authoritative credentials;
- analytics/metrics/log backend outage must not block licensing authority paths;
- support-bundle failure must not mutate license/device state.

## 18. Rollback and reference paths

Every optimization must keep a simpler correctness reference path:

- catalog cache can be disabled to canonical-store read path;
- credential resolver cache can be bypassed to canonical current-state read;
- bulk worker concurrency can be reduced to serial per-Grant execution;
- network retry/backoff tuning can be disabled to single-attempt behavior without changing idempotency semantics;
- optional parsed-object memoization can be disabled to full parse/verify per invocation;
- observability export can be disabled without disabling local/server authority processing.

There is **no** rollback/reference mode that bypasses signature verification, anti-rollback, provider pinning, canonical CAS, machine single-writer or crash-safe replace.

## 19. Benchmark matrix for P20 / implementation

Minimum benchmark matrix:

| Dimension | Required variants |
| --- | --- |
| identity provider | TPM CNG; software CNG fallback |
| state | fresh; registered-unlicensed; licensed; credential update; corrupted candidate |
| cache | cold process/file cache; warm process/file cache |
| storage | reference SSD; intentionally delayed/fault-injected I/O for crash path |
| network | loopback/stub; representative healthy WAN; timeout/unavailable |
| server | resolver read; registration; activation/signing; Grant revision; catalog mutation |
| concurrency | single device writer; competing local writer; concurrent same-Grant revisions; parameterized bulk rollout |
| failure | signer failure; DB conflict; response loss; process kill/power-loss simulation around local replace |

Performance evidence is meaningful only if correctness invariants are asserted in the same scenario; a fast path that returns an invalid or stale authority result is a failure, not an optimization win.

## 20. Release engineering thresholds vs blocking gates

P18 separates three levels:

1. **Correctness/security invariant** — violation is blocking regardless of performance.
2. **Interactive engineering budget** — regression should normally block a release candidate when reproducible on the agreed reference environment and materially harms product UX.
3. **Capacity/efficiency target** — informs tuning/sizing and is blocking only if a concrete release requirement depends on it.

P20 must not recursively create evidence artifacts for every metric. A benchmark becomes a blocking Gate only when it uniquely detects a high-impact failure mode not already covered by another independent mechanism.

## 21. D-085 targeted impact revalidation

D-085 changes the exact binary representation used by credential/challenge/device-proof signing, but does **not** change any P18 workload class, state owner, retry model, persistence model, process topology or canonical transaction boundary.

### 21.1 Latency impact

- W1 local `status` / entitlement evaluation keeps the existing warm/cold budgets. Its `credential verify inside process` budget **p95 ≤ 50 ms** now explicitly includes bounded deterministic-CBOR parse, canonical-form validation, envelope/domain/version checks, signature verification, schema validation and entitlement projection for a typical V1 credential.
- W3 registration / device assertion measurement must decompose `challenge canonicalization + challenge_digest`, `device-proof transcript construction`, and provider/KSP proof latency so a serialization regression is not misdiagnosed as TPM/KSP latency.
- W5/offline import local-install budgets remain unchanged; deterministic-CBOR validation is part of verify-before-replace.
- Server W6/signing measurement must report candidate semantic construction, canonical payload encoding, external signer/KMS latency and final CAS/commit separately. The existing **external signing call p95 target ≤ 750 ms** measures the external signer boundary, not application canonicalization CPU.

### 21.2 Size and memory impact

- Existing **≤ 256 KiB** guardrail for a single SignedLicenseCredential / offline response remains valid and now applies to the complete canonical signed artifact/envelope as delivered. D-085 does not authorize larger artifacts.
- Existing `axlic.exe` working-set and local-state budgets remain unchanged. A conforming implementation must stream/bound temporary encode/decode buffers where practical; deterministic encoding must not justify a resident cache/service.
- Because authorization-critical payloads reject indefinite-length items, tags, floats and non-canonical alternatives, implementation can enforce bounded parser depth/item counts and artifact-size limits before expensive cryptographic work. Malformed/non-canonical input must fail without unbounded allocation or pathological re-encode loops.

### 21.3 Benchmark additions

P20/implementation measurement should separately expose, where applicable:

- canonical credential encode time and canonical-form validation/decode time;
- challenge signing-input encode + digest time;
- device-proof transcript encode time;
- encoded credential/challenge/proof byte size distribution;
- canonical-rejection path latency for malformed/non-canonical bounded fixtures.

These measurements are diagnostic decomposition, not automatically new blocking Gates. Existing P18 evidence posture still applies: an artifact blocks only when it uniquely detects a material high-impact failure mode or a release-critical budget breach.

### 21.4 Reference and rollback path

- Correctness reference: deterministic canonical encoder/validator with cache/memoization disabled; same semantic fixture must yield byte-identical canonical output across independent implementations.
- Performance rollback may disable optional memoization/buffering optimizations, but **cannot** accept non-canonical input, skip canonical validation, change domain-separation bytes, or reintroduce multiple signing representations.
- If real implementation measurements miss P18 budgets, optimize encoder/parser allocation, payload construction or UX decoupling first; do not weaken D-085 canonicality/security rules.

### 21.5 Impact verdict

**NO SUBSTANTIVE P18 CONTRADICTION.** Existing latency, memory, artifact-size, backoff, cache, observability and reference-path budgets remain Current Authority. D-085 only adds measurement decomposition and bounded canonical-parser expectations.

## 22. P18 consistency review and disposition

The Engineering / Optimization reconciliation plus D-085 targeted impact revalidation finds **no new upstream product/semantic/platform gap**. The current design can be optimized while preserving P10–P17 authority including the repaired canonical-signing contract.

**P18 FR-019 ENGINEERING / OPTIMIZATION RECONCILIATION + D-085 TARGETED IMPACT REVALIDATION ACCEPTED / CLOSED.** Architecture family P14–P18 is again closed for the FR-019 Windows-first V1 baseline.

**Successor routing:** return to central `aegis` to decide the next Primary Owner/stage. P18 does not automatically resume P20 or execute implementation planning.
