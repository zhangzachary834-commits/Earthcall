# SUN UPDATE — V5 occupied-segment transport exact-head green; next witness gate

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh re-inspection

Do not use older handoff SHAs as current truth.

Observed branch head entering this pass:
`a29458fb1f9f3850ecdf825730449f3bfb0d96c0`

Observed canonical:
`423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

Canonical's latest merge is PR #329 (scene-spatial synthesis DAG rung 1).

PR #343 remains open and Draft.

## Exact-head CI verdict changed from queued to GREEN

Earthcall focused CI workflow `35924735427` tested exact head `a29458fb1f9f3850ecdf825730449f3bfb0d96c0` and completed SUCCESS.

All four jobs are green:
- Focused CPU tests (macOS)
- SDF range-proxy verification (macOS)
- Slow Adapter independent clock (macOS)
- SDF authored-Perlin A/B (macOS Release)

This closes the native proof gate that was still queued in `SUN_UPDATE_V5_OCCUPIED_SEGMENT_SAMPLING_2026-09-23.md`.

Therefore the occupied-segment implementation is now supported by exact-head focused CI: the event-array WGSL validates/runs under the native WebGPU witness suite, and the existing fused AB/BA witness remains green on this head.

## Canonical drift audit

A targeted compare from the V5 merge base `b5fa341...` to current canonical `423cfd69...` shows canonical has advanced substantially (89 commits from that merge base).

Important refinement to the prior collision statement: canonical now changes renderer-adjacent infrastructure (`src/Singularity/Screen/Renderer.hpp`) and adds `RenderedFieldSemanticObserver.hpp`, plus CI workflow changes from PR #329. It still does NOT change the V5-owned WebGPU compiler/renderer files (`SdfWgsl.*`, `WebGpuRenderer.*`) or V5 tests directly.

So reconciliation remains mandatory before landing. Do not infer a source conflict merely from Renderer.hpp adjacency, but do rerun exact-head CI after reconciliation because PR #329 changed renderer lifecycle/relevance infrastructure and the CI workflow itself.

## What is complete in V5 now

- canonical six-channel medium identity including E_v
- fused sample-level medium-set transport
- overlapping AB/BA permutation-invariant native witness
- member refusal at compiler seam
- numeric member edit preserves fused WGSL structure at compiler seam
- occupied-segment local sampling policy
- non-overlap/local-quality native witness
- exact-head focused CI for the occupied-segment implementation

The core local-quality theorem is now implemented and green: irrelevant empty distance no longer consumes the finite sample budget of an existing medium while overlapping media still share one physical transport integral.

## Next unfinished rung — do not restart segmentation

Advance the required V5 witness matrix rather than revisiting the now-green occupied-segment implementation.

The next coherent gate should combine the two closely related incremental-runtime invariants:

1. **time-only fused-set witness**
   - at least one medium consumes authored `t` in a V5 channel;
   - changing only that medium's relative Timeline coordinate changes native pixels;
   - medium-set WGSL structure / pipeline identity does not regenerate.

2. **numeric-only fused renderer witness**
   - change a numeric authored value in one member without changing AST topology or membership;
   - native pixels must change;
   - fused set structural compile/pipeline count must not increment;
   - parameter refresh must feed the existing fused program.

The compiler-level numeric invariant already exists in `sdf_wgsl_parameter_refresh_test.cpp`, but the V5 plan explicitly requires the renderer-level/no-structural-compile witness. Do not mistake compiler byte equality for proof that the runtime renderer cache actually follows the intended incremental path.

After those are green, continue with:
- membership add/remove => bounded medium-set structural invalidation;
- unsupported member => native no-stale-output witness;
- canonical reconciliation;
- final exact-head CI.

## Risks / guardrails

- Do not weaken the 96-samples-per-occupied-segment correctness baseline before measuring its many-segment cost.
- Do not regress to per-medium sequential framebuffer transport.
- Do not make time part of structural identity merely to make the time witness easy; time is ambient runtime input.
- Do not make numeric parameter values part of WGSL structural identity; value refresh must remain data-only.
- Do not start GI, multiple scattering, spectral transport, or new Medium kinds in V5.

## Exact continuation point

Start in the existing V5 renderer cache/update seam in `WebGpuRenderer.*` and the native `webgpu_object_test.cpp` witness. Identify the smallest existing telemetry/counter that can prove structural compile/pipeline reuse. If none exists, add narrowly scoped test-visible telemetry rather than broad renderer instrumentation. Land the time-only + numeric-only native witness first; if it exposes a cache/invalidation bug, repair only that seam, then run focused CPU/native WebGPU CI.

— GPT-5.6 Sol
