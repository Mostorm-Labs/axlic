---
authority_id: AXL-V1-A0-P36
stage: P36
scope: axlicense/A0
kind: fix-reverification
version: "1"
status: historical
source_type: notion
source_url: https://app.notion.com/p/3db4c57a590c8101b0d2ebb9e653cfec
migration_class: location-only
semantic_change: none
---

# 36 — P36 AXL-V1-A0 Fix / Reverification — QCBOR Boundary Repair

> 🛠️
>
> **P36 status: READY_FOR_P34_REREVIEW — 2026-09-14.** `A0-P34-B1` 的 implementation-owned QCBOR production-boundary defect 已按 P35 frozen repair scope 修复，并在新 exact result revision 上完成原冻结证据的 hosted reverification。P36 不签发或暗示 P34 PASS。

## 1. Repair identity

- Repository: `github/Mostorm-Labs/axlic`
- Governing P31 package: `6190288cf7d83bcbe588a4e19fda8d6354335e6f:.aegis/packages/AXL-V1-A0-P31.md`
- P35 finding: `A0-P34-B1`
- Valid predecessor result: `09920aa719e93f076a04cfc62717104c547956d3`
- New result revision: `13d16ebb34913dafbddc83222cf29187e92c8053`
- Materialized ref: `refs/heads/codex/axl-v1-a0@13d16ebb34913dafbddc83222cf29187e92c8053`
- Repair tree: `93b3d969d6593b68b2e415e9e5fd205686690c6a`
- Note: predecessor chain contains content-identical noop commit `24350f499d8f1f9fb932b97d34279f29199760cf`; it changes no tree bytes and is preserved in ancestry rather than force-rewritten.

## 2. Implemented repair

1. Added frozen QCBOR v1.6.1 exact commit `930708bb86481e88879eb1d87fd4d664f1d69503` through CMake and linked `qcbor::qcbor` into `axlic_core`.
2. Replaced project-owned generic `CborReader` / `CborWriter` machinery with narrow `client/src/wire/qcbor_codec.*` backed by QCBOR.
3. QCBOR is compiled with floats, tags, indefinite-length arrays/strings, non-integer labels and exp/mantissa support disabled where applicable.
4. Preserved AxLicense-owned bounds and fail-closed policy: 64 KiB artifact bound, bounded item/nesting/text/entitlement counts, UTF-8 validation, exact field schema, deterministic re-encode equality, P17 domain-separated signing input, low-S validation and Windows CNG P-256/SHA-256 verification.
5. Updated `THIRD_PARTY_NOTICES.md` and build/toolchain evidence identity to record QCBOR exact commit.
6. No golden vector, CLI, RuntimeLicense, server/backend, A1 identity or product semantic change was made.

## 3. Repository delta audit

Compared with `09920aa…`, substantive repair touches only:

- `CMakeLists.txt`
- `client/CMakeLists.txt`
- `client/src/wire/credential.cpp`
- `client/src/wire/qcbor_codec.cpp`
- `client/src/wire/qcbor_codec.hpp`
- `THIRD_PARTY_NOTICES.md`
- `reference/tools/toolchain_report.py`

No authorized-scope expansion was observed.

## 4. Hosted reverification identity

- GitHub Actions run: `34824491876`, attempt 1 — SUCCESS
- Job: `103913345075` — SUCCESS
- Evidence artifact: `10340300160`, `AXL-V1-A0-evidence`
- Artifact digest: `sha256:e1c696436a1377a5ce1d3671abe5aab6c4a5470400516fc7d7d5a41cee87cfe7`
- Artifact head SHA: `13d16ebb34913dafbddc83222cf29187e92c8053`
- Artifact expiry: 2026-10-14

## 5. Frozen regression results

- T-A0-01 Release configure/build: PASS; QCBOR C target built as `qcbor.lib`, `axlic.exe` built successfully.
- CTest: 11/11 PASS.
- T-A0-02 / EV-01 production corpus: PASS, all frozen positive and negative vectors.
- T-A0-03 independent Python reference / `cryptography==50.0.1`: PASS.
- T-A0-04 / EV-06 CLI process contract: PASS.
- T-A0-05 offline/no-server boundary: PASS; no forbidden server/database imports.
- T-A0-06 W1 baseline: produced from 20 warmups + 200 measurements; p50 `4.918 ms`, p95 `5.574 ms`, p99 `5.875 ms`.
- Toolchain evidence: CMake 4.4.2; MSVC 19.44/toolset 14.44; Windows SDK 10.0.26100.0; QCBOR dependency identity equals frozen commit.

## 6. P36 closure judgment

```yaml
p36_reverification:
  finding: A0-P34-B1
  defect_class: IMPLEMENTATION_DEFECT
  predecessor_result: 09920aa719e93f076a04cfc62717104c547956d3
  result_revision: 13d16ebb34913dafbddc83222cf29187e92c8053
  frozen_repair_scope_satisfied: true
  required_regressions: PASS
  hosted_exact_result_evidence: PASS
  new_blocking_findings: []
  status: READY_FOR_P34_REREVIEW
```

## 7. Next stage

Return to **`aegis-gate-review → P34 AXL-V1-A0 rereview`**. P34 must independently bind the new result/run/job/artifact and decide Gate PASS/BLOCKED. P36 itself does not issue Gate closure.
