---
authority_id: AXL-V1-A0-P35
stage: P35
scope: axlicense/A0
kind: defect-classification
version: "1"
status: historical
source_type: notion
source_url: https://app.notion.com/p/3db4c57a590c819484bfe4fcf586e30d
migration_class: location-only
semantic_change: none
---

# 35 — P35 AXL-V1-A0 Defect Classification — QCBOR Contract Divergence

> 🧭
>
> **Classification: IMPLEMENTATION_DEFECT / READY_FOR_P36 — 2026-09-14.** The Gate blocker is a direct implementation divergence from a trusted P31 frozen dependency boundary. It is not a Task Package defect, Verification Design defect, Authority conflict or evidence gap.

## 1. Finding

**ID:** `A0-P34-B1`

**Gate class:** `FROZEN_REQUIREMENT_FAILURE`

**Owning defect layer:** `IMPLEMENTATION_DEFECT`

### Frozen requirement

P31 requires QCBOR v1.6.1 commit `930708bb86481e88879eb1d87fd4d664f1d69503` as the production CBOR dependency, accessible only behind the `client/src/wire` boundary.

### Observed implementation

- No QCBOR FetchContent/linkage exists.
- Production `credential.cpp` contains a project-owned CBOR parser/encoder.
- Third-party notice confirms QCBOR is absent.

## 2. Why this is not an upstream defect

- P17 deterministic signing bytes are clear.
- P20 crypto/canonical verification obligations are clear and the frozen corpus passes.
- P31 explicitly selected and pinned QCBOR; there is no unresolved semantic choice for P32/P36.
- The current evidence independently proves functional canonical/crypto behavior, so this is not an `EVIDENCE_GAP`.

## 3. P36 authorized repair scope

Preserve valid A0 behavior and evidence design. Repair only the implementation-owned dependency boundary:

1. Add QCBOR v1.6.1 exact commit `930708bb86481e88879eb1d87fd4d664f1d69503` to the production CMake dependency graph.
2. Replace project-owned generic CBOR read/write machinery with a narrow `client/src/wire` QCBOR-backed adapter.
3. Preserve AxLicense-specific fail-closed policy above the library boundary: forbidden tags/floats/indefinite forms/nulls, exact field ordering/labels, size/item/depth limits, canonical re-encoding/equality, P17 signing input and P20 low-S/CNG verification.
4. Update third-party notices/dependency identity reporting to include QCBOR.
5. Do not alter credential schema, domain string, crypto profile, CLI contract, reference vectors, Product/Server scope, or A1 semantics.

## 4. Reverification

P36 must rerun the original frozen evidence relevant to the changed wire boundary, not invent new blocking evidence:

- T-A0-01 Release build.
- T-A0-02 complete EV-01 A0 corpus.
- T-A0-03 independent reference cross-check.
- T-A0-04 CLI process contract regression.
- T-A0-05 offline/no-server boundary regression.
- T-A0-06 W1 baseline regenerated; numeric threshold remains corroborative at A0.
- Hosted Windows run on the new exact result revision and new reviewer-accessible `AXL-V1-A0-evidence` artifact.

## 5. Preservation rule

The accepted result `09920aa…` is valid predecessor work. P36 must repair from that descendant state; it must not replay A0 from scratch or widen scope. A source-changing repair creates a new `result_revision`; the previous run/artifact remain historical evidence and cannot be reported as evidence for the new result.

## 6. Next stage

**P36 Fix / Reverification — A0-P34-B1 QCBOR production-boundary repair.**
