# SUN UPDATE — PR #329 lifecycle green + structural telemetry A/B

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PhaseB_Renderer_Lifecycle_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J, Phase A, V4/base reconciliation, disabled-overhead audit, or the Renderer lifecycle repair. This pass began from live head `480cc3fa1c07e1a53ebf8f3b2c25d20b5e2553b2` and used targeted reads only: live PR state, the latest lifecycle handoff, exact-head Actions jobs, the Renderer observer seam, and `RenderedFieldSemanticObserver.hpp`.

## Exact-head lifecycle witness is green

Focused CI run #2865 is executing on exact lifecycle-patch head `480cc3fa...`.

PR329-relevant jobs have completed SUCCESS:

- `Focused CPU tests (macOS)`: SUCCESS. This includes build + execution of the focused regression witnesses, therefore the new base-Renderer lifecycle witness compiled and ran green.
- `SDF range-proxy verification (macOS)`: SUCCESS, including CPU SDF proof witnesses and WebGPU parity checks.
- `SDF authored-Perlin A/B (macOS Release)`: SUCCESS, including the 2880x1800 OFF-vs-ON measurement.

The only still-running job at this update is `Slow Adapter independent clock (macOS)`, currently in `Measure adapter impact across authored worlds`; its soundness/cadence and dramatic Law-Direct A/B steps already passed. This is not a Scene-DAG/Phase-B lifecycle gate unless it produces evidence of overlap.

## Final structural telemetry/render-authority A/B audit

The lifecycle patch changes only `Renderer::setRenderedFieldSemanticObservationEnabled` transition behavior. The renderer-owned `setRadianceSources` / `setVolumeDensitySources` authoritative collections and revisions remain the same objects consumed by rendering. Enabling observation replays const references to those existing collections; it does not replace, move, rewrite, filter, or derive alternate source collections.

`RenderedFieldSemanticObserver` remains one-way diagnostic state:

- public outputs are `enabled()` and `stats()` only;
- it exposes no theorem lookup/consumption API to Renderer/WebGPU;
- `observeRadianceSources` and `observeVolumeDensitySources` return `void`;
- the observer's proof/cache state is private;
- `authorityBypassesApplied` is only telemetry and remains zero by construction;
- disabled entrypoints return before set-revision checks, vessel traversal, canonicalization, theorem work, or cache mutation.

Therefore the OFF-vs-ON authority graph is structurally identical after source admission:

`authored source bindings -> Renderer-owned collections -> existing render/WebGPU path`

Observation ON adds only a side branch:

`Renderer-owned collections -> diagnostic observer -> private semantic/proof cache + Stats`

There is no edge from that side branch back into the render path. In particular, no observer result reaches pixels, WGSL generation, ray marching, volumetric accumulation, visibility, source filtering, or transport.

The renderer-boundary witness now additionally proves that OFF->ON and OFF->ON-after-disable preserve the exact vector storage, AST pointers, vessel revisions, and set revisions while `authorityBypassesApplied == 0`; second enable takes set-revision hits without semantic/theorem rebuild. This closes the lifecycle correctness half of the A/B at the actual ownership seam rather than only inside the observer class.

## V4 boundary remains closed

The witness medium carries independent V4 `emissionExpr/emissionRevision`, but the observer still reads only `densityExpr/densityRevision`. Successful exact-head execution therefore revalidates that lifecycle replay does not accidentally promote V4 self-emission into the density theorem family.

## What remains

1. Let exact-head run #2865 finish. If the remaining Slow Adapter authored-world tail is green, the entire exact-head workflow is green.
2. If that tail fails, inspect only the concrete failure and distinguish unrelated authored-world variance from PR329 overlap; do not reopen completed Scene-DAG investigation without evidence.
3. Once the exact-head workflow is settled, recheck live base drift / mergeability and decide whether PR #329 is ready to leave draft or whether a final base reconciliation is needed.

No production mutation was needed in this pass: the targeted audit found the telemetry/render-authority boundary already structurally closed, and the new lifecycle witness has executed green on exact head.

## Role status

Still active only because exact-head workflow #2865 has one running Slow Adapter tail and the live base must be rechecked after it settles. The PR329 lifecycle implementation itself is now compiled/executed green and the final structural OFF-vs-ON authority audit found no rendering-authority path from observer theorem state.