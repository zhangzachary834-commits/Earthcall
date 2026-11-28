# SUN HANDOFF — Rung 10 Mathematics Gift Shop Quarantine — COMPLETE

**Date:** 2026-10-10 (America/Los_Angeles)  
**Model:** GPT-5.6 Sol  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Branch:** `sol/ontomath-rung10-glm-quarantine-20261008`  
**PR:** #582 — `Rung 10: quarantine GLM semantic authorship`  
**Tested code head:** `7fc4d092baddf439978574fad15c98a277614bca`  
**Single reconciliation merge:** `eb0c3d1e1ee2b7ebe40489a8da0777aab43104a6`  
**Current canonical audited:** `ca34af17ce150062f4b83859c2ff1a199a6ef601`

## Verdict

**Rung 10 is COMPLETE and ready for review.**

Do not merge PR #582 without Zach's explicit authorization. Do not begin Rung 11 / Bind / Person authority from this handoff.

The constitutional invariant is now enforced by code and CI:

> GLM may execute, lower, cache, serialize, or represent OntoMath mathematics. GLM may not independently originate mathematical meaning.

## Final guard state

`tests/singularity/gift_shop_guardrail_test.cpp` scans production `src/` after stripping comments, strings, and character literals. It rejects unauthorized semantic-origin calls for:

- `glm::inverse`
- `glm::transpose`
- `glm::determinant`
- `glm::translate`
- `glm::rotate`
- `glm::scale`
- `glm::lookAt`
- `glm::perspective*`
- `glm::frustum*`
- `glm::ortho*`

The exact production/oracle allowance remains only:

1. `src/Singularity/OntoMath/LinearAlgebra.cpp`
2. `src/Singularity/OntoMath/Affine.cpp`
3. `src/Singularity/Screen/GL/GluCompat.cpp`
4. `src/Singularity/Screen/WebGPU/smoke_renderer.cpp`
5. `src/Singularity/Screen/WebGPU/smoke_window.mm`

No broad debt allowlist exists.

At `7fc4d092baddf439978574fad15c98a277614bca`, the matcher was hardened from literal-token matching to C++ call-shape matching. Whitespace, line breaks, stripped comments, prefix-family variants, and optional explicit template arguments no longer create an evasion such as `glm::inverse (M)` or `glm :: transpose\n(M)`. Embedded self-probes freeze positive and negative spellings.

The production census at the tested head reports zero unauthorized targeted calls.

## Final exact-head tribunal

Earthcall focused CI **38097628367** on `7fc4d092baddf439978574fad15c98a277614bca`:

- focused build: **PASS**
- Terminal exact-reference smoke: **PASS**
- `gift_shop_guardrail_test`: **PASS** (7.59s)
- focused CTest: **77/79 PASS**
- SDF range-proxy verification: **PASS**
- OntoMath native-resolution image parity: **PASS**
- WebGPU SDF/distance/authored-color/object-radiance/particle MVP gates: **PASS**
- V5 overlap + volumetric transport gates: **PASS**
- authored-Perlin core/range-traversal gates: **PASS**
- authored-Perlin Release A/B: **PASS**

The two focused-CPU failures were:

1. `second_nature_law_forge_zone_test` — `clicking authored instrument derives a newborn Law`
2. `authorable_light_contract_test` — `Sun radiance is approximately unit strength at its source`

Slow Adapter soundness/independent-cadence and Law-Direct steps passed. Its authored-world performance step still times out on:

`slow_adapter_zone_perf_test saves/worlds/chess_app.json --adapter=off --direct=off --frames=60`

at 120 seconds.

## Baseline provenance

Current canonical `ca34af17ce150062f4b83859c2ff1a199a6ef601` independently ran push CI **38097839398** and reproduced:

- the same newborn-Law focused-CPU failure;
- the same Sun source-strength focused-CPU failure;
- the same `chess_app.json` 120-second Slow Adapter baseline timeout.

Canonical's SDF range-proxy verification and authored-Perlin Release A/B both passed.

Therefore none of those three red signals demonstrates Rung-10 ownership. Rung 10 does not mutate unrelated Law/light/Slow-Adapter code to cosmetically green the workflow.

## Reconciliation and current-canonical audit

The campaign performed exactly one deliberate ancestry-preserving reconciliation:

- first parent: Rung-10 lineage `e9eb7f945fcf2534973bbcec8a9ce4f200235c1e`
- second parent: canonical `f4f2e2aaf3216f4623efc608e36bf37f21ef3d2c`
- merge: `eb0c3d1e1ee2b7ebe40489a8da0777aab43104a6`

No force-push, rebase, or snapshot overwrite was used.

Canonical later advanced to `ca34af17ce150062f4b83859c2ff1a199a6ef601`. A fresh overlap audit found that every Rung-10 production semantic file and `ontomath_affine_sovereignty_test.cpp` remains byte-identical upstream to the reconciled base. Only `CMakeLists.txt` and `.github/workflows/earthcall-ci.yml` evolved, as additive build/test-list infrastructure. PR #582 remains mergeable. Canonical movement alone is not an integration event, so no second reconciliation was performed.

## Scope delivered

Rung 10 migrated the remaining targeted semantic-origin sites through OntoMath, including PersonPerspective camera matrices, Person pose composition/inverse, EngineUpdate local-space conversion, CreationTools unprojection, creation/asset console placement, and ObjectCollision support mapping.

`OntoMath::pullbackCovector` now names A^T d support-direction pullback independently of inverse-transpose normal transformation, including the singular-map distinction.

The anti-regrowth ratchet is wired into focused CI.

## Boundary

Rung 11 / Bind / Person authority is **not part of this work**.

PR #582 is review-ready but must remain unmerged until Zach explicitly says to merge it.

— GPT-5.6 Sol / The Sun
