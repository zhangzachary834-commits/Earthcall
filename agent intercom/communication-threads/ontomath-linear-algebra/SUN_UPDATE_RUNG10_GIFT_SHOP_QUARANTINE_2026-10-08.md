# Sun Update — Rung 10: the Mathematics Gift Shop gets a ratchet

**Date:** 2026-10-08  
**Model:** GPT-5.6 Sol  
**Branch:** `sol/ontomath-rung10-glm-quarantine-20261008`  
**Declared base:** `64b36416eb4296040d386c9b18537c1b483dd642`

Zach explicitly merged PR #498 and authorized Rung 10. This is a fresh post-landing campaign; it does not extend the Rungs 0–9 branch and it does not enter Rung 11 / Bind / Person-authority work.

## The ratchet

`tests/singularity/gift_shop_guardrail_test.cpp` now walks production `src/` and reports every semantic-origin use of:

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

The scanner strips comments, string literals and character literals before matching. The guard deliberately also covers the sibling `frustum*` and `ortho*` projection constructors so camera mathematics cannot regrow through an unlisted spelling. It reports all violations before failing, following the no-black-box witness style. CMake pins its CTest working directory to the source root and the focused macOS CI list builds and executes it.

There is no legacy-debt exemption list. The only allowed files are exact, reasoned boundaries:

1. `src/Singularity/OntoMath/LinearAlgebra.cpp` — OntoMath numerical backend.
2. `src/Singularity/OntoMath/Affine.cpp` — OntoMath affine backend/decomposition.
3. `src/Singularity/Screen/GL/GluCompat.cpp` — GLU compatibility/API representation boundary.
4. `src/Singularity/Screen/WebGPU/smoke_renderer.cpp` — standalone renderer diagnostic oracle.
5. `src/Singularity/Screen/WebGPU/smoke_window.mm` — standalone renderer diagnostic oracle.

Adding another allowance is therefore visible architecture, not an invisible habit.

## Aisles migrated in the same pass

- `PersonPerspective`: view and projection formulas now call `OntoMath::cameraLookAt` / `cameraPerspective` and only lower the resulting MatrixValue to GLM storage.
- `Person::updatePose`: translation, composition, and parent-rest inverse now originate in OntoMath. A singular parent-rest transform refuses the parent-relative correction instead of admitting GLM NaN/Inf.
- `EngineUpdate`: object-fusion and field-gizmo world/local conversions now use `inverseAffine + transformPoint`.
- `CreationTools`: morph/patched control unprojection uses the same OntoMath inverse/point contract.
- `CreationWindow`, Asset Console and Create3D Console: placement/duplicate translations originate in `affineTranslation` / `affineCompose`.
- `ObjectCollision::getSupportPointWorld`: the old direct `transpose(linear) * direction` is now `OntoMath::pullbackCovector`.

The support-map change names a distinction that matters. For local→world linear part A, a world support query pulls back as A^T d because
`argmax_x d·(A x + t) = argmax_x (A^T d)·x`.
This is not a normal transform. It needs no inverse and remains mathematically defined for singular A. The affine-sovereignty witness freezes that behavior against an independent GLM oracle and separately preserves the rule that inverse-transpose normals refuse singular transforms.

## Current source census

The branch-local census over every canonical hit identified at Rung-10 start now has **zero unauthorized targeted production calls**. Remaining hits are confined to the five exact allowed boundaries above.

That is source evidence, not completion evidence. Exact-head build/test CI still decides whether the migration preserved behavior. Do not call Rung 10 complete until the guard itself builds/runs and the existing OntoMath/geometry/native-resolution witnesses survive. Do not touch Rung 11 from this handoff.
