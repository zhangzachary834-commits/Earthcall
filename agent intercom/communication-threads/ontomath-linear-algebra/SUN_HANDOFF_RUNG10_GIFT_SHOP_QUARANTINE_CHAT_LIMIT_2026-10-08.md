# SUN HANDOFF — Rung 10 Mathematics Gift Shop Quarantine — Chat Limit

**Date:** 2026-10-08 (America/Los_Angeles)  
**Model:** GPT-5.6 Sol  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Branch:** `sol/ontomath-rung10-glm-quarantine-20261008`  
**Draft PR:** #582 — `Rung 10: quarantine GLM semantic authorship`  
**Last implementation head judged by CI:** `a7ab4f0bcaba2c3c2c24af2cd6793817f66b488b`  
**Rung-10 declared base:** `64b36416eb4296040d386c9b18537c1b483dd642`  
**Live canonical at handoff:** `9752b8387c1435d0ad0aaec8bc8d670d8af4ff69`

This handoff exists because the originating chat reached its context limit. Continue from live repository state; do not reconstruct the campaign from old chat prose.

## Constitutional boundary

Rungs 0–9 were completed on PR #498 and explicitly merged by Zach. Rung 10 is the fresh post-landing cleanup/constitutionalization pass. The invariant remains:

> GLM may execute, lower, cache, serialize, or represent OntoMath mathematics. GLM may not independently originate mathematical meaning.

Rung 11 / Bind / Person-authority work is **not authorized** and remains out of scope.

Canonical movement alone is not an integration event. Check semantic overlap before reconciling. Preserve ancestry. Never force-push or snapshot-overwrite newer canonical work.

## What Rung 10 implemented

### 1. Anti-regrowth ratchet

`tests/singularity/gift_shop_guardrail_test.cpp` is wired into normal CTest and the focused macOS CI list. It walks production `src/`, strips comments/string literals/character literals, reports every violation before exit, and rejects unauthorized direct semantic-origin calls to:

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

The only exact allowed files are:

1. `src/Singularity/OntoMath/LinearAlgebra.cpp` — OntoMath numerical backend.
2. `src/Singularity/OntoMath/Affine.cpp` — OntoMath affine/decomposition backend.
3. `src/Singularity/Screen/GL/GluCompat.cpp` — GLU compatibility boundary.
4. `src/Singularity/Screen/WebGPU/smoke_renderer.cpp` — standalone renderer diagnostic/oracle.
5. `src/Singularity/Screen/WebGPU/smoke_window.mm` — standalone renderer diagnostic/oracle.

Do not add a broad legacy-debt allowlist. A new allowance is an architectural decision and must be narrow and justified.

### 2. Remaining targeted production aisles migrated

The Rung-10 pass moved the remaining targeted production semantic-origin calls through OntoMath:

- `PersonPerspective`: camera view/projection through `cameraLookAt` / `cameraPerspective`.
- `Person::updatePose`: translation, affine composition, and parent-rest inverse through OntoMath.
- `EngineUpdate`: object-fusion and field-gizmo world/local conversions through `inverseAffine + transformPoint`.
- `CreationTools`: morph/polyhedron/patch unprojection-to-local conversion through OntoMath.
- `CreationWindow`: placement translation through `affineTranslation`.
- Creator Console / Asset Console: spawn/duplicate placement through `affineTranslation` / `affineCompose`.
- `ObjectCollision::getSupportPointWorld`: direct `transpose(linear) * direction` removed.

### 3. New semantic distinction: support covector pullback

Rung 10 added `OntoMath::pullbackCovector`.

For local→world linear map A, a world-space support direction d pulls back to local support space as A^T d:

`argmax_x d·(A x + t) = argmax_x (A^T d)·x`.

This is deliberately distinct from a normal transform. It requires no inverse and remains defined for singular affine maps. `ontomath_affine_sovereignty_test` now freezes this against an independent GLM oracle and separately proves that singular affine maps still refuse inverse-transpose normal transformation.

The branch-local targeted census before CI found **zero unauthorized targeted production calls**.

## Exact-head CI tribunal on a7ab4f0b

Workflow: **#37863876457**  
Result: **FAILURE overall**, but the failure is not a compile failure and the new guard itself passed.

### Focused CPU job

Build: **PASS**  
Terminal exact-reference smoke: **PASS**  
`gift_shop_guardrail_test`: **PASS**  
CTest result: **73/75 passed (97%)**

Failures:

1. `second_nature_law_forge_zone_test`
   - failure text: `clicking authored instrument derives a newborn Law`

2. `authorable_light_contract_test`
   - failure text: `Sun radiance is approximately unit strength at its source`

Do **not** assume either is caused by Rung 10. First reproduce/compare on the relevant canonical/base state and inspect current canonical changes. If they are inherited/unrelated, record evidence and leave them out of this PR.

### SDF range-proxy verification job

**PASS**.

This included green evidence for:
- CPU SDF proof witnesses
- generic WebGPU SDF parity
- WebGPU SDF distance parity
- authored-color parity
- WebGPU object/radiance parity
- particle MVP path
- **OntoMath native-resolution image parity**
- V5 fused overlap physics
- volumetric mist/source/occluder transport
- authored-Perlin compiler/gradient gates
- authored-Perlin camera/range traversal gate

### Authored-Perlin Release A/B

**PASS**.

### Slow Adapter independent-clock job

The soundness/independent-cadence witnesses passed. Law-Direct stress also passed with reported **3.43x** speedup.

The job failed later in the authored-world performance loop because the first baseline invocation timed out:

`slow_adapter_zone_perf_test saves/worlds/chess_app.json --adapter=off --direct=off --frames=60`

Timeout: **120 seconds**.

Treat this as likely inherited/unrelated until reproduced on canonical. Do not mutate Rung 10 merely to silence this lane.

## Live integration state

At handoff:
- PR #582 is open, draft, and last observed `mergeable=true`.
- Implementation CI head was `a7ab4f0b...`.
- Live canonical has advanced to `9752b838...` after Rung 10 was cut.
- Do not merge/rebase merely because canonical moved. Inspect semantic overlap.
- Before final landing readiness, perform one deliberate ancestry-preserving reconciliation only if needed, then rerun exact-head relevant evidence.

## Exact continuation

1. Re-fetch live branch, PR #582, canonical, comments, and workflows. Another agent may have moved them.
2. Verify whether the two Focused CPU failures reproduce on canonical/base and classify them owned vs inherited.
3. Verify the Slow Adapter chess baseline timeout provenance the same way.
4. If Rung 10 owns a regression, make the smallest semantic repair and rerun exact-head CI.
5. After any mutation, rerun/check `gift_shop_guardrail_test`; zero unauthorized targeted semantic-origin calls is mandatory.
6. Audit the complete PR changed-file set and ancestry before declaring completion.
7. Check live canonical for semantic overlap. Reconcile only when warranted; preserve both histories.
8. Rung 10 is complete only when the guard and relevant existing OntoMath/geometry/native-resolution evidence are green with no Rung-10-owned blocker, docs are truthful, and PR #582 is genuinely ready for review.
9. **Do not merge without Zach's explicit authorization.**
10. **Do not start Rung 11.**

## Scheduled-task continuity

The `OntoMath Unification Torch` scheduled task was updated with this exact continuation state and remains **DISABLED**. Do not enable it unless Zach explicitly asks to resume scheduling.

— GPT-5.6 Sol / The Sun
