# SUN UPDATE — V5 native overlap-physics tribunal added

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live state

Canonical was re-inspected before this pass and had advanced to:

`0426ce24e3f5450d27301597dcd9c56dda360e92`

The new canonical commit is analysis documentation (`docs(analysis): record rendering relevance economics verdict`) on top of `67bb9d0c...`; do not assume this remains current at the next pass.

PR #343 was re-inspected at head:

`09930779e128ca11fee144d014529f1a3cb8c404`

It remained open and Draft. The exact-head focused CI for that head, run `35940884183`, completed SUCCESS before this implementation pass.

V3/V4 remain landed and were not reopened.

## Next unfinished V5 theorem advanced

The remaining physics gap was stronger than order invariance. Existing native coverage proves A+B and B+A agree, while compiler coverage proves one `totalExtinction`, one `totalSource`, and one interval gain. That does not by itself numerically prove that native pixels implement the shared extinction integral rather than some other accidentally commutative composition.

This pass adds a dedicated native WebGPU tribunal:

`tests/singularity/webgpu_v5_overlap_physics_test.cpp`

Implementation commit:

`fae087677bed6abc0ba56d1c0636a5fecdf688da`

The test is intentionally tiny and closed-form rather than another broad scene fixture.

## Closed-form fused witness

Two media occupy exactly the same center-ray interval `z=[-1,+1]`, so `L=2`.

Both use `D=1` and zero scattering for the first arm.

A:
- `sigma_t = 0.35`
- `E_v = (0.20, 0, 0)`

B:
- `sigma_t = 0.90`
- `E_v = (0, 0, 0.30)`

The correct shared law is:

`gain = (1 - exp(-(sigmaA + sigmaB)L)) / (sigmaA + sigmaB)`

and therefore:

`C_fused = (E_A + E_B) * gain`.

For these constants, the 96-step shared shader integral telescopes to the same closed form (no coefficient varies within the interval and transmittance never reaches the early-break threshold).

The native framebuffer is required to agree with that closed-form answer within 3 bytes per active channel.

## Sequential-alpha counterexample

The same test analytically reconstructs the obsolete V0–V4 architecture:

1. integrate A independently and B independently;
2. composite their whole-medium premultiplied answers A-over-B;
3. composite them again B-over-A.

The native fused result must remain at least 25 aggregate RGB bytes away from BOTH sequential reconstructions.

This is the missing discriminator: the test no longer merely says “ordering does not matter”; it says “the native answer is the shared extinction integral and is numerically incompatible with either whole-medium alpha ordering.”

## Independent overlap-source witness

A second arm keeps A's red authored self-emission but replaces B's blue self-emission with:

- authored `sigma_s = 0.30`;
- authored `C_v = (0,0,1)`;
- no B `E_v`.

The resulting red+blue native answer must still match the same expected source magnitudes under shared extinction.

This directly witnesses that overlap preserves independent authored source channels: A contributes through `E_v`, while B contributes through scattering/chroma. V5 is therefore not merely summing multiple self-emission vectors while dropping the V2/V3 source structure.

## Evidence already green before this commit

Focused CI run `35940884183` on prior exact head `09930779...` completed SUCCESS. That closes the earlier membership/refusal gate and establishes a green baseline immediately before this physics witness was added.

## Current verification limitation / risk

The focused workflow explicitly builds `webgpu_object_test` and does not yet name the new `webgpu_v5_overlap_physics_test` target. CMake discovers it automatically through `file(GLOB_RECURSE TEST_FILES "tests/*.cpp")`, and its `webgpu_.*` name routes it through the existing WebGPU test target construction, but the focused CI job will not execute it until the workflow target/run list is updated or the witness is folded into `webgpu_object_test`.

Therefore DO NOT call the new numerical theorem green yet. The implementation exists, but native execution is still the next gate.

No production renderer code was changed in this pass because the audited shader law already has the correct shared-extinction algebra. If the tribunal fails when executed, repair only the narrow fused transport seam exposed by the failure; do not weaken the expected physics to fit the implementation.

## Exact continuation point

1. Re-inspect canonical and PR head first.
2. Wire `webgpu_v5_overlap_physics_test` into the SDF/WebGPU focused CI build and execution list (or, if branch concurrency makes a separate target undesirable, transplant the tribunal into the already-executed `webgpu_object_test`).
3. Run the native test on macOS WebGPU.
4. If it fails, inspect actual native RGB and determine whether the error is test geometry/quantization or fused transport. Preserve the closed-form theorem; fix production only if production is wrong.
5. If it passes, mark combined-extinction-vs-sequential-alpha and independent overlap source contribution complete in the V5 plan.
6. Reconcile PR #343 with then-current canonical and run final exact-head focused CI.
7. If all V5 plan obligations are then discharged, leave Draft only if another explicit plan item remains; otherwise report V5 complete rather than inventing V6 scope.

## Guardrails

- V3/V4 stay closed.
- No sequential whole-medium blending.
- No global cache flushes.
- No numeric/time values in structural identity.
- No GI, multiple scattering, spectral transport, new Medium nouns, or renderer-owned authored truth.
- Targeted reads only; no big chungus.

— GPT-5.6 Sol
