# SUN UPDATE — V5 exact-head green; local-sampling segmentation is now the next implementation gate

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Re-inspection first — do not reuse stale state

At the start of this pass, canonical `sync-from-earthcall-main` had advanced to:

`cd78aa3c1fe46b92ca65dd4517a3b908ca4b2aa7`

PR #343 code head before this Intercom-only commit was:

`36e71bbba486a8e29b26a7ae4c4eeb0139a27132`

The PR is still open, Draft, and mergeable. It is now 20 commits behind current canonical, with merge base `b5fa341329176b0257d6de58f85f99ac6a286830`.

## CI evidence changed materially

Exact-head workflow run for `36e71bb...`:

- Earthcall focused CI run `35912862017`
- status: completed
- conclusion: **success**

This supersedes the previous pass's red Slow Adapter performance-gate observation. Do not carry that old red state forward as a current blocker.

## Targeted code review: the next V5 fracture is confirmed

`compileVolumeSet(...)` still performs one fixed 96-step march across the entire union AABB ray interval:

- compute union `t0/t1`
- `span = t1 - t0`
- `stepLength = span / 96`
- sample all media at each of those 96 global midpoints

Each medium is then admitted only if the world sample lies inside that medium's own AABB. The transport law inside a sample is correct in the important V5 sense: active media sum into one `totalExtinction` and one `totalSource`, and Earthcall advances one shared transmittance/radiance integral. Preserve that.

The defect is therefore sampling economics, not medium-composition physics: a distant disjoint medium enlarges the union span and can reduce sample density inside another medium even though the empty gap contributes no transport.

## Next implementation gate — witness before surgery

The next code change should be the explicit native non-overlap/local-quality witness already required by the PR plan.

Construct A and B such that:

1. A intersects the centre camera ray and has a deliberately spatially varying density/source profile so coarse sampling is observable.
2. Render A alone and capture the centre pixel.
3. Add B far away along the same ray (or otherwise enlarge the fused union interval substantially) while keeping B disjoint from A and arranged so B's own contribution can be isolated/neutralized for the A-local comparison.
4. Under the current global-96 policy, demonstrate that A's local result changes beyond a small tolerance solely because B enlarged the union sampling span.
5. This witness should fail before the sampling-policy fix and pass after it.

Do not use a constant-density A as the only witness; a constant field can hide the resolution defect.

## Intended fix after the witness fails

Implement **event-segmented occupied-interval transport**, not sequential whole-medium rendering:

- intersect each medium AABB with the camera ray;
- gather/clamp entry/exit events to the visible/depth-limited interval;
- partition the ray into locally occupied segments;
- skip empty segments without consuming sampling budget;
- assign sampling resolution from occupied/local segment length rather than the global union span;
- carry one continuous transmittance and integrated-radiance state across segment boundaries;
- inside an overlap segment, continue evaluating every active medium and summing into the same `totalExtinction` / `totalSource` before the shared integral update.

The slogan remains: **segment the ray, not the physics**.

A safe first implementation may use a bounded fixed maximum medium count / event count matching the renderer's admitted set contract, but must refuse truthfully if that bound is exceeded rather than silently dropping media.

## Canonical reconciliation warning

Do not merge #343 from its current base. Current canonical is 20 commits ahead of the merge base. Reconcile only after the local-sampling witness/fix is coherent, then rerun exact-head CI. The current comparison shows V5's own changed files remain the expected renderer/compiler/tests/docs set, but canonical has advanced substantially enough that stale assumptions are unsafe.

## Remaining V5 plan gates after local sampling

Still track the PR body's explicit witness matrix:

- time-only fused-set witness;
- numeric-only fused renderer witness / no structural compile;
- membership invalidation witness;
- refused-member native no-stale-output witness;
- exact non-overlap/local-quality witness + fix;
- canonical reconciliation and exact-head native CI.

Do not broaden into GI, multiple scattering, spectral transport, new Light/Medium kinds, or renderer-owned authored properties.

## Exact continuation point

Start in `tests/singularity/webgpu_object_test.cpp` beside the existing V5 AB/BA overlap witness. Add the failing spatially-varying A-alone vs A-plus-far-B local-quality witness first. Then modify `compileVolumeSet(...)` only as much as required to make that witness pass while preserving the existing AB/BA permutation witness and the shared fused transport law. Run the focused compiler/native WebGPU witnesses, then exact-head CI.
