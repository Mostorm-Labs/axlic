---
authority_id: AXL-V1-P17
stage: P17
scope: axlicense
kind: platform-contract
version: "1"
status: current
source_type: notion
source_url: https://app.notion.com/p/3d94c57a590c8149b8e7e0838a5e46b9
migration_class: location-only
semantic_change: none
---

# 17 — P17 Platform Contract — AxLicense V1 v0.1

> 🪟
>
> **Authority status: Accepted / P17 FR-019 Platform Contract + D-084 canonical-signing targeted repair CLOSED — 2026-09-13.** 本页把 trusted P10–P16 FR-019 authority 映射到 Windows-first V1 platform realization，并补齐 `SignedLicenseCredential`、trusted challenge 与 DeviceIdentity proof 的 deterministic canonical signing-input contract。它冻结 `axlic.exe` process/CLI、Windows CNG identity-provider、machine-wide local-state/ACL/atomic-replace、transport/error normalization，以及 signing-byte/domain-separation boundary；不改变 P10–P16 已接受的 product semantics、operation vocabulary、module ownership 或 runtime ordering，也不在 P17 选择最终 signature/hash algorithm profile。

## 1. Stage contract

- **Role:** P17 Platform Contract.
- **Authority:** P02/P03 FR-019 requirements/capability baseline；trusted P10–P13 semantics；P14 D-078；P15 D-079；P16 D-080.
- **Objective:** 将 common semantics 与 Windows realization 分离，冻结实现层必须满足的 process、storage、identity-provider、transport、privilege、versioning 与 platform-failure contract。
- **Non-goals:** 不选择 server cloud/vendor/database/KMS 品牌；不冻结最终 crypto algorithm/key-size/profile、HTTP route 名称、installer technology、具体文件名/registry path、Named Mutex literal name、performance sizing 或 deployment topology；这些由 P18/P20/implementation authority 在不改变本页 contract 的前提下确定。
- **Quality gate:** platform shortcut 不得制造第二套 authority；不得把 private key 暴露给 AxLicense core/product；不得因 Windows API 失败改变 P13 idempotency；不得把 ordinary refresh 实现成 reissue；不得因无常驻 Agent 而放弃 machine-wide serialization、provider pinning 或 crash-safe local replace。
- **Handoff:** P17 CLOSED 后 earliest untrusted downstream layer = **P18 Engineering / Optimization**。P18 只可优化成本、latency、I/O、retry/backoff、cache 与 operational hardening，不得重新定义本页 contract。

## 2. Windows V1 process and deployment contract

### 2.1 Public executable boundary

- 唯一 product-facing native surface 是 machine-installed **`axlic.exe`**；V1 不发布 public `axlic.dll`，也不要求常驻 Windows Service/Agent。
- 产品必须通过直接 process creation 调用 executable；不得依赖 shell command parsing，也不得链接 AxLicense internal libraries。
- `axlic.exe` 安装目录属于 machine software boundary（默认由 installer 放在受保护的 Program Files 类目录）；普通产品进程不得修改 executable 或 internal libraries。
- 多个产品可以共享同一 machine AxLicense installation/state；Product namespace 不产生第二套 DeviceIdentity。

### 2.2 Privilege model

- **Read-only:** `status` / `entitlement` / diagnostics-safe reads / current credential evaluation 可在具有 machine-state read permission 的普通用户上下文运行。
- **Machine mutation:** first identity establishment、ordinary device registration local association commit、activation/import credential install、recovery、factory provisioning、需要安装新 credential 的 refresh 等，必须在拥有 AxLicense machine-state write authority 的管理员/System 上下文执行。
- 无写权限时，命令必须在 server/provider mutation 前尽可能 fail-fast；若某个 server authority 已因不可避免的 race 先 commit，本地结果仍按 P16 partial-success recovery 处理，不得回滚 server authority。
- V1 不为了绕过此 privilege boundary 引入隐式后台服务。消费产品可在需要 mutation 时显式请求 elevation；具体 UAC UX/launcher broker 实现不属于 AxLicense semantic contract。

## 3. CLI process / wire contract

### 3.1 Invocation contract

- command family 仍使用 P15 冻结的稳定 family：`status`, `entitlement`, `identity`, `device-register`, `device-assert`, `activate`, `refresh`, `offline`, `recover`, `diagnostics`, `support-bundle`; historical `migrate` 仅 compatibility mode。
- command selector 可以位于 argv，但结构化 request payload 必须支持从 **stdin** 输入，避免 token、proof、offline payload 或未来敏感字段出现在 process command line / process listing。
- product integration 不得依赖 human-readable help/stderr 文本。

### 3.2 Structured result

stdout 只承载一个 UTF-8 structured result document，语义等价于：

```
CommandResult {
  contract_version { major, minor }
  ok
  code
  category
  retryable
  safe_action_hint?
  correlation_ref
  data?
}
```

- stdout 不输出 progress/debug/free-form log；human/debug 信息只能进入 policy-gated stderr/diagnostics。
- `code` 是 product behavior authority；exit code 只提供 coarse process class。
- exit class 至少区分：success、contract/input error、retryable/unavailable、security/policy denied、conflict/recovery required、internal failure；具体 numeric values 可在 implementation package 冻结，但一旦发布需兼容维护。

### 3.3 Versioning

- CLI contract 使用 **major/minor**。Major mismatch 必须显式拒绝；minor 只允许 additive/ignorable evolution，不得改变既有 code/field 语义。
- unknown command / authorization-critical request semantics 必须 fail closed；unknown optional result field 可由旧产品忽略。
- `axlic.exe --version`（或等价 capability query）必须可返回 executable version + supported CLI contract range；产品不从文件 version string 推断授权行为。

## 4. Windows Device Identity provider contract

### 4.1 Provider order

Windows V1 provider realization：

1. **TPM hardware-backed CNG provider** — first preference when policy and actual usability pass；
2. **software-persistent CNG provider** — first-establishment fallback only when hardware-backed provider is absent/permanently unusable and policy allows；
3. otherwise unsupported.

A transient TPM/CNG failure is not automatically equivalent to “permanently unusable”. If the strongest provider is temporarily unavailable, return retryable provider-unavailable rather than permanently pinning weaker software identity solely because of a transient failure.

### 4.2 TPM provider realization

- Realization uses Windows CNG/KSP boundary equivalent to **Microsoft Platform Crypto Provider / NCrypt** for a persisted machine identity key.
- key must be non-exportable through AxLicense/public APIs and provider-local; AxLicense stores only opaque provider/key reference + normalized public identity claim/metadata.
- TPM presence alone is insufficient. `probe()` must establish that the required KSP/key/proof capability is usable under current policy; first-establishment commit occurs only after successful persisted key creation/load validation.

### 4.3 Software fallback realization

- Realization uses a Windows CNG software KSP equivalent to **Microsoft Software Key Storage Provider**, persisted machine-wide and marked non-exportable where the provider supports the required property.
- application code never serializes a private key into registry/file/local JSON. KSP owns private-key persistence; AxLicense stores an opaque provider/key name/reference only.
- software fallback identity has a distinct `scheme_id`/assurance class from TPM identity; server policy can distinguish it without learning private material.

### 4.4 Provider pinning

- after L0 local identity establishment commits, provider/scheme is pinned for that identity epoch;
- provider temporarily unavailable → `IDENTITY_PROVIDER_UNAVAILABLE`; no automatic TPM→software migration;
- provider identity not found/corrupt → controlled recovery path, not new ordinary first-run identity;
- only an explicit accepted recovery operation may advance identity epoch/provider realization.

## 5. Typed proof / challenge platform boundary

- Core modules provide a **typed `ProofInput`**, not arbitrary caller bytes, to `IIdentityProvider.prove(...)`.
- platform provider may internally use CNG signing primitives, but there is no public `sign(bytes)` command/ABI.
- proof transcript must domain-separate purpose/audience and bind the current identity context plus freshness material supplied by trusted challenge semantics; a proof for enrollment/assertion/registration cannot be replayed as license/commercial authority.
- Device Assertion challenge must be verified as trusted, purpose-bound and unexpired before provider proof is requested.
- exact signature/hash algorithm profile and key-rotation test vectors are frozen by P20 security/verification authority; P17 requires only standard CNG-supported primitives, non-exportability, purpose separation and stable scheme-version identification.

## 6. Machine-wide local state contract

### 6.1 State root and ownership

- V1 local state is **machine-wide**, under a protected ProgramData-class AxLicense state root, not per-user AppData and not HKCU.
- installer creates state root/security descriptor. Administrators/System own mutation; ordinary product users may receive minimum read access required for local status/entitlement evaluation.
- no private identity key bytes are stored here. State may contain provider reference, registered opaque `device_id`, pinned scheme/provider metadata, signed credential artifact, trust metadata, local generation/high-watermark metadata and bounded recovery metadata.
- app-controlled registry keys are not the private-key store. Registry may be used only for non-authoritative install/discovery metadata if implementation needs it.

### 6.2 Protected envelope

- security-sensitive local metadata must be stored inside a versioned protected machine-state envelope with Windows OS protection/ACL defense-in-depth (DPAPI/DPAPI-NG-equivalent may be used); the SignedLicenseCredential remains independently signature-verifiable and never derives authority from local encryption.
- local protection is not treated as protection against a machine administrator with full control. Security goal is to prevent ordinary process/user tampering/disclosure and accidental corruption from becoming authorization.

### 6.3 Atomic replace

For a local authoritative observation update:

1. acquire machine-wide mutation ownership;
2. re-read committed state;
3. build complete candidate in the same volume/state root;
4. verify credential/identity/schema/anti-rollback;
5. write candidate temp, flush data, close;
6. commit through an OS same-volume atomic replace primitive with write-through/flush semantics;
7. only then publish success/release mutation ownership.

Startup/recovery must reject incomplete temp artifacts and select only a fully valid committed state. Crash outcome must be old committed state or new committed state, never a partially parsed candidate.

### 6.4 Anti-rollback

- `CredentialManager` compares candidate `authority_revision` / `credential_generation` against the committed local high-watermark before replace.
- lower authority revision or lower generation at the same authority revision is stale and rejected；higher authority revision may legally reduce rights.
- local high-watermark and credential bytes must be committed in the same machine-state generation so ordinary file replacement cannot intentionally create two independent truths.
- exact resistance against malicious local Administrator rollback is out of V1 trust scope; P20 verifies standard-user/file-corruption/power-loss rollback barriers.

## 7. Machine-wide mutation serialization

- Windows V1 must use a machine-wide cross-process lock primitive (named kernel mutex or equivalent protected OS lock) with an explicit security descriptor, not an in-process mutex.
- mutating commands acquire lock before committed local-state mutation and **re-read after acquisition**.
- a second writer waits or receives stable busy/retry semantics; it must never bypass locking.
- network calls should not hold the local writer longer than necessary when P16 ordering allows prepare → revalidate → commit; any lock release/reacquire requires re-reading/revalidating the current committed state before replace.
- read-only commands may run concurrently but may observe only an old or new complete snapshot.

## 8. Network / control transport contract

### 8.1 Device client transport

- Windows device transport uses HTTPS over the Windows/system TLS validation boundary (WinHTTP-equivalent realization). Certificate/hostname validation is mandatory; production has no `--insecure` bypass.
- device transport preserves logical correlation/idempotency identifiers across retry. Timeout/socket/TLS errors never generate a new logical operation identity by themselves.
- HTTP status is transport mapping only; final semantic behavior comes from versioned AxLicense result/error code.
- ordinary `ResolveCurrentCredential` is a read/delivery call and must not invoke signing/issuance merely because it was retried.

### 8.2 Product Backend / admin control plane

- Product Backend, Release/Admin and Factory callers use separate scoped service/admin authentication from ordinary device bootstrap/identity proof.
- Product Backend cannot directly access Canonical Store or Signing Authority; Release/Admin catalog publication cannot mutate customer Grants; Factory scopes cannot silently authorize ordinary commercial entitlement.
- exact OAuth/mTLS/vendor credential realization is deployment/security hardening and may be selected later as long as scope/audience separation is preserved.

### 8.3 Signing authority platform port

- AxLicense Server calls an external `LicenseSigningPort`/challenge-signing port backed by KMS/HSM-equivalent custody; private signing keys are not loaded into product/device processes or canonical database rows.
- license credential signing and challenge signing use distinct logical key purpose/authorization even if one physical HSM/KMS service hosts both.

## 9. Offline artifact platform boundary

- OfflineActivationRequest/Response use a versioned transport envelope with explicit artifact kind, schema/contract version, correlation/idempotency reference and bounded payload.
- request files/QR/USB are untrusted transport; server canonical validation remains authoritative.
- response carries/verifiably references the signed credential result and is always routed through `CredentialManager` verify-before-replace.
- V1 does not require one encoding to fit both file and QR. A compact encoding/QR fragmentation strategy may be added later without changing activation semantics.

## 10. Windows platform error normalization

Platform-specific Win32/CNG/TPM/TLS/file errors must be converted at the adapter boundary into stable AxLicense categories before reaching product code. Minimum stable mappings include:

| Platform condition | Stable semantic family |
| --- | --- |
| insufficient machine-state write privilege / ACL denied | `LOCAL_WRITE_PRIVILEGE_REQUIRED` / local access denied |
| machine mutation lock unavailable/busy | `LOCAL_STATE_BUSY` retryable |
| pinned KSP/TPM temporarily unavailable | `IDENTITY_PROVIDER_UNAVAILABLE` retryable |
| required provider/algorithm permanently unsupported | `IDENTITY_PROVIDER_UNSUPPORTED` |
| pinned key missing/corrupt | `IDENTITY_RECOVERY_REQUIRED` |
| local state parse/protection failure | `LOCAL_STATE_CORRUPT` / recovery required |
| TLS certificate/hostname validation failure | `TRANSPORT_TRUST_FAILED` |
| timeout/DNS/network unavailable | `TRANSPORT_UNAVAILABLE` retryable when safe |
| candidate lower revision/generation | `CREDENTIAL_STALE` |
| credential signature/schema/device mismatch | credential validation / fail-closed family |

Raw HRESULT/NTSTATUS/Win32/provider strings may appear only in bounded policy-gated diagnostics, never as the product-facing semantic contract and never with secret/provider-private payloads.

## 11. Installer / upgrade compatibility contract

- installer upgrade must preserve machine state/provider-managed key material; uninstall/reinstall must not silently delete physical DeviceIdentity unless an explicit destructive reset path is authorized.
- golden image / deployment template must exclude machine identity key, current DeviceIdentity association, signed credential and machine-state high-watermark.
- executable upgrades must be able to read the previous supported local-state contract or fail with an explicit migration-required classification; they must not reset state because a parser changed.
- older executable encountering newer authorization-critical local/credential semantics must fail closed for affected evaluation rather than reinterpret them.
- rollback/downgrade policy is explicit: an older executable may run only if its supported local-state/credential major versions cover the installed state; otherwise it returns incompatible-version without rewriting state.

## 12. Deferred / explicitly not frozen at P17

The following remain downstream decisions and are not required to close P17:

- final credential/challenge signing algorithm/key sizes and algorithm-agility test vectors;
- exact Windows installer/MSI/MSIX technology and literal filesystem/registry/mutex names;
- exact REST paths, JSON field spelling beyond already-frozen semantic envelopes, compression and QR encoding;
- server cloud/region/DB product/KMS/HSM vendor;
- Product Backend service-auth vendor/mechanism details;
- benchmark targets, cache/backoff tuning, server sizing and observability budgets;
- malicious local Administrator/physical-attacker hardening beyond V1 trust scope.

## 13. P17 consistency review and disposition

The targeted platform reconciliation finds **no new upstream semantic/product-authority gap**. Windows realization can satisfy P10–P16 while preserving:

1. CLI-only Windows V1 with no mandatory Agent/Service;
2. machine-wide identity/state and explicit privilege boundary;
3. TPM-first initial establishment with software fallback only before pinning and only when policy permits;
4. no private-key export and no generic public signing primitive;
5. ordinary registration independent of Factory ProvisioningAuthorization;
6. machine-wide single-writer + re-read + crash-safe atomic replace;
7. signed credential self-sufficient offline runtime;
8. `refresh` resolve/delivery separated from explicit reissue;
9. Product SaaS/AxLicense/canonical/local states remain independent;
10. stable platform-error normalization without leaking raw sensitive diagnostics.

**P17 FR-019 TARGETED PLATFORM CONTRACT RECONCILIATION ACCEPTED / CLOSED.** Earliest untrusted downstream layer advances to **P18 Engineering / Optimization**.

## D-084 targeted repair — canonical signing input / deterministic wire contract

### Repair scope

This section closes only the verification-blocking gap identified by D-084. P10–P16 semantics remain unchanged. P17 freezes **what exact bytes are signed or proved**; P20 still owns the accepted signature/hash algorithm profile, key-size/profile, public-key fixtures and golden cryptographic vectors.

### 1. AxLicense Canonical Binary Encoding v1

All V1 authorization-critical signing payloads use **RFC 8949 Core Deterministic CBOR** as the sole canonical binary representation.

Additional AxLicense restrictions:

- only definite-length CBOR items are allowed;
- signed semantic payloads use maps with fixed **unsigned-integer field labels**;
- integers use the shortest valid deterministic representation;
- floating-point values are forbidden in V1 signing payloads;
- CBOR tags are forbidden in V1 signing payloads;
- duplicate map keys are invalid;
- `null` is forbidden unless a future schema explicitly defines null as a semantic value; current optional fields are represented by **key absence**, not null;
- byte material (`OpaqueBytes`, nonce, digest, signature) is encoded as CBOR byte string;
- string-valued IDs/enums that are represented as text use exact UTF-8 text. ProductId/EntitlementId remain lowercase ASCII as frozen by P12; no Unicode normalization transform is performed by the signing encoder;
- `SchemaVersion` is encoded as array `[major, minor]` using unsigned integers;
- `Instant` in signing payloads is encoded as signed integer **Unix UTC seconds**; no floating time representation is accepted;
- a parser may be variation-tolerant for non-authoritative transport data, but **signed AxLicense artifacts MUST be rejected when their protected payload bytes are not already the canonical deterministic encoding**. The verifier must not silently parse a non-canonical form, re-encode it and then accept it as equivalent.

This yields the invariant:

```
one accepted logical signed payload
        ↓
exactly one AxLicense Canonical Binary Encoding v1 byte sequence
```

Changing these encoding rules incompatibly requires a new canonical-format/signing-input version; it is not a P12 minor-schema change.

### 2. Signed envelope v1

Credential and trusted-challenge artifacts use the same outer deterministic-CBOR envelope shape but different artifact kinds and different domain-separated signing inputs.

```
SignedEnvelopeV1 {
  1: artifact_kind       // UInt: credential=1, challenge=2
  2: envelope_version    // UInt, MUST be 1
  3: algorithm_id        // UTF-8 text; accepted values frozen by P20 crypto profile
  4: key_id              // UTF-8 text; resolves trusted verification key
  5: payload_bytes       // bstr; already-canonical payload bytes
  6: signature_bytes     // bstr
}
```

The outer envelope itself MUST also be encoded using AxLicense Canonical Binary Encoding v1.

Credential signing input:

```
CredentialSigningInputV1 = DCBOR([
  "axlicense.credential",
  1,
  algorithm_id,
  key_id,
  payload_bytes
])
```

Challenge signing input:

```
ChallengeSigningInputV1 = DCBOR([
  "axlicense.challenge",
  1,
  algorithm_id,
  key_id,
  payload_bytes
])
```

`algorithm_id` and `key_id` are therefore integrity-bound to the signature input and cannot be substituted outside the signed transcript. `signature_bytes` is excluded from its own signing input by definition.

### 3. SignedLicenseCredential protected payload v1

`payload_bytes` for `artifact_kind=credential` is deterministic CBOR of this fixed integer-label map:

```
CredentialPayloadV1 {
  1: schema_version
  2: credential_id
  3: license_grant_id
  4: authority_revision
  5: binding_id
  6: device_id
  7: credential_generation
  8: issued_at
  9: supersedes_credential_id?   // absent when not present
  10: entitlements
}
```

`entitlements` is an array of `CredentialEntitlementV1`. Because entitlement order is not authorization semantics, the signing encoder MUST sort entries by ascending bytewise UTF-8 encoding of `entitlement_id` before encoding. Duplicate `entitlement_id` is invalid.

```
CredentialEntitlementV1 {
  1: entitlement_id
  2: product_id
  3: right_kind
  4: grant_semantics
  5: validity
  6: constraint?                 // absent when not applicable
}

ValidityV1 {
  1: kind
  2: not_before?                 // Instant
  3: not_after?                  // Instant
}

ConstraintV1 {
  1: kind
  2: value
}
```

Wire enum codes are frozen for canonical bytes:

- `right_kind`: `runtime=1`, `maintenance_update=2`, `cloud_service=3`;
- `grant_semantics`: `presence=1`, `bounded_u64=2`;
- `ValidityV1.kind`: `perpetual=1`, `bounded=2`;
- `ConstraintV1.kind`: `max_u64=1`.

P12 validation still owns semantic legality. Examples: `presence` must not carry a constraint; `bounded_u64` requires `max_u64 >= 1`; bounded validity requires `not_after`; perpetual validity must not carry `not_after`. P17 only freezes how a valid P12 object becomes deterministic bytes.

### 4. Trusted Device Assertion Challenge payload v1

For `artifact_kind=challenge`, `payload_bytes` is deterministic CBOR of:

```
DeviceAssertionChallengePayloadV1 {
  1: schema_version
  2: challenge_id
  3: issuer_id
  4: audience
  5: purpose
  6: session_ref
  7: nonce
  8: expected_device_id?         // absent when discovery is allowed
  9: issued_at
  10: expires_at
}
```

Every P12 challenge semantic that constrains replay/retargeting is therefore inside `payload_bytes` and covered by `ChallengeSigningInputV1`: audience, purpose, session, nonce, optional expected Device, and time bounds.

### 5. Challenge digest and DeviceIdentity proof transcript

`DeviceIdentityAssertion.challenge_digest` is defined as:

```
challenge_digest = PROFILE_HASH(ChallengeSigningInputV1 bytes)
```

`PROFILE_HASH` is selected by the P20 cryptographic verification profile. The digest is over the deterministic **challenge signing input**, not over JSON, UI fields, transport framing, or the possibly non-deterministic signature bytes.

The device private identity proves a second domain-separated transcript. Core constructs the typed semantic payload; the provider is never given caller-selected arbitrary bytes.

```
DeviceProofPayloadV1 {
  1: schema_version
  2: assertion_id
  3: challenge_id
  4: challenge_digest
  5: device_id
  6: identity_epoch
  7: scheme_id
  8: audience
  9: purpose
  10: session_ref
  11: issued_at
  12: expires_at
}

DeviceProofSigningInputV1 = DCBOR([
  "axlicense.device-proof",
  1,
  proof_scheme_id,
  DeviceProofPayloadV1
])
```

`proof_scheme_id` is thereby bound to the proof transcript. `proof_bytes` in `DeviceIdentityProofEnvelope` is the provider-produced proof over this logical transcript according to the P20-selected scheme/profile. A platform adapter may internally hash the transcript before calling CNG when required by the selected signature primitive, but the externally defined message being proved is exactly `DeviceProofSigningInputV1`.

This preserves the P17 prohibition on a public/generic `sign(bytes)` interface: callers provide typed challenge/assertion intent; AxLicense core owns serialization and domain separation; the IdentityProvider only executes the approved proof operation.

### 6. Verification order and non-canonical rejection

Credential verification MUST perform, in order:

1. parse outer envelope and require canonical deterministic CBOR;
2. require `artifact_kind=credential` and supported `envelope_version`;
3. require accepted `algorithm_id`/`key_id` under the current P20 trust profile;
4. require `payload_bytes` to parse as the supported CredentialPayload schema and to be byte-for-byte equal to its required canonical encoding;
5. reconstruct `CredentialSigningInputV1` from the **received canonical payload bytes** and envelope metadata;
6. verify signature;
7. only then evaluate P12 schema/device/binding/revision/entitlement semantics and P17 local anti-rollback rules.

Challenge verification uses the equivalent sequence with `artifact_kind=challenge`, then checks issuer/audience/purpose/session/time policy.

Device proof verification MUST first verify the trusted challenge, reconstruct its exact `ChallengeSigningInputV1`, compute `challenge_digest`, compare it with the assertion, reconstruct `DeviceProofSigningInputV1`, and verify the proof against the registered current DeviceIdentity context.

A non-canonical but semantically parseable authorization artifact is **invalid**. A verifier must not repair/reorder/re-normalize it into a form that verifies.

### 7. Schema and crypto evolution boundaries

Three version domains remain distinct:

- P12 `schema_version` controls semantic-field compatibility;
- P17 `envelope_version` / signing-input version controls deterministic binary/signing representation;
- P20 crypto profile controls allowed signature/hash algorithms, key parameters and trust/key-rotation vectors.

A P12 minor schema addition may be ignored by an older verifier only when P12 declares it authorization-neutral/ignorable. It is nevertheless covered by the signed payload bytes. Unknown authorization-critical semantics or unsupported schema major fail closed.

P20 may add/rotate an accepted algorithm or verification key without changing P17 canonical field mapping. Conversely, changing canonical field labels, canonical sort rules, Instant representation, domain-separation strings or signing-input structure requires a new P17 signing-input/envelope version even if the cryptographic primitive remains unchanged.

### 8. Transport separation

JSON, CLI stdin/stdout, HTTP bodies, QR, Base64/Base64URL or file wrappers may be used as **carrier encodings** where already allowed by P17. They are not alternative signing representations.

Before issuance, semantic state is serialized into the canonical CBOR payload above. A signed artifact carries the canonical payload bytes. Text transports may encode the whole artifact as Base64URL or an equivalent lossless carrier, but decoding MUST reproduce the exact original artifact bytes before verification.

### 9. D-084 repair disposition

**P17 D-084 targeted canonical-signing contract repair: ACCEPTED / CLOSED.**

The missing upstream contract is now explicit enough for P20 to create independent credential/challenge/device-proof golden vectors, parser-differential/tamper fixtures and cross-implementation verification evidence without inventing serialization semantics inside Verification Design.

No P10–P16 semantic, operation, module, runtime or Windows privilege/provider boundary changed. P18 requires only targeted impact revalidation for encoding-size/verification-latency/resource assumptions before central successor routing resumes P20.
