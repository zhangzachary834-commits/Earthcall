# SUN UPDATE — PR #329 final base reconciliation + CI classification

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_Lifecycle_Green_Structural_AB_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J, Phase A, the V4 compatibility audit, the Phase-B lifecycle implementation, or the structural OFF/ON authority audit.

This pass began from live head `d293bc94978ea3d3ce0c3497285cd19b10461ba5`.

## Lifecycle-patch CI classification

On lifecycle code head `480cc3fa1c07e1a53ebf8f3b2c25d20b5e2553b2`, the PR329-relevant execution gates passed:

- Focused CPU tests: SUCCESS, including the real base-Renderer lifecycle witness.
- SDF range-proxy verification: SUCCESS.
- SDF authored-Perlin A/B Release: SUCCESS.

The overall focused workflow #2864 was red only because the independent Slow Adapter authored-world performance step exceeded its threshold in `saves/worlds/basic_pixel_changer.json`.

Observed medians:

- adapter/direct OFF: 42.237542 ms
- adapter/direct ON: 51.537000 ms
- ratio: 1.22
- difference: +9.299458 ms

The gate allows ratio <= 1.20 OR difference <= 1.0 ms, so this run missed the ratio threshold by 0.02. Slow Adapter soundness/cadence and the dramatic Law-Direct A/B step passed before that measurement. No Scene-Spatial / rendered-field semantic witness failed.

Treat this as an independent Slow Adapter performance-tail failure unless future evidence shows overlap. Do not reopen completed Scene-DAG/lifecycle work merely because the aggregate workflow was red.

## Canonical drift found and reconciled

Canonical `sync-from-earthcall-main` advanced from `85c0bb6705d53332286e3c50093df94cf5b418b9` to `46e90911f976c24d105aead5f13fd6b0a24bce7b` by 16 commits.

The incoming changed-file set was limited to:

- Cathedral rendering Intercom notes;
- Person/Law addendum docs;
- Northern Veil generator + zone data;
- `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`;
- `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp`.

A targeted compare found **zero changed-file intersection** with PR #329.

A clean two-parent reconciliation therefore landed as:

`598fd754ffd768a19187e576f6d7703c682c42b9`

The reconciliation tree starts from current canonical and overlays the exact PR #329 blobs from the prior head. No incoming canonical file was replaced by a PR copy.

Post-merge live state:

- canonical: `46e90911f976c24d105aead5f13fd6b0a24bce7b`
- PR head: `598fd754ffd768a19187e576f6d7703c682c42b9`
- behind canonical: 0
- mergeable: true
- PR remains draft

## Current exact-head gate

Focused CI run #2880 (run id `35907328020`) is queued on reconciliation head `598fd754...`.

If the PR329-relevant CPU/SDF/lifecycle witnesses remain green on this reconciled head, there is no remaining Scene-Spatial implementation defect identified by this Sun.

If Slow Adapter alone repeats its authored-world performance variance while its soundness gates remain green, classify that independently rather than expanding PR329 scope.

## Role-close criteria

This specific Sun role may be closed when all of the following hold on live state:

1. current canonical remains an ancestor of PR #329 (0 behind);
2. PR remains mergeable;
3. the real-Renderer lifecycle witness remains execution-green after reconciliation;
4. Scene-Spatial/SDF focused witnesses remain green;
5. no observer theorem/cache state has acquired a rendering-authority path;
6. no new review/CI evidence identifies a PR329-specific defect.

Do not add new theorem families or rendering bypass authority in order to manufacture more work.

## Role status

Still active pending exact-head focused CI #2880. The production lifecycle defect is fixed, V4 remains outside the density theorem surface, structural OFF/ON authority is closed, current canonical is reconciled, and no PR329-specific CI failure is presently known.


## Successor audit — final observer consumer sweep

A successor Sun independently audited the complete 28-file PR diff for every production-facing observer symbol:

- `RenderedFieldSemanticObserver`
- `_renderedFieldObserver`
- `renderedFieldSemanticObservation*`
- `observeRadianceSources`
- `observeVolumeDensitySources`
- `authorityBypassesApplied`

Production references are confined to exactly:

1. `src/Singularity/Screen/RenderedFieldSemanticObserver.hpp`, which owns diagnostic/cache state; and
2. `src/Singularity/Screen/Renderer.hpp`, which writes source bindings into the observer and exposes only enabled/stats telemetry.

All other hits are tests or Intercom documentation. No WebGPU, OpenGL, WGSL, volumetric transport, ray-march, visibility, source-filtering, or accumulation file in PR #329 consumes observer theorem/cache state.

This independently confirms the structural OFF/ON A/B conclusion: enabling observation adds a side-channel into private diagnostic state but does not add a return edge into rendering authority.

The reconciled `Renderer.hpp` and lifecycle witness were re-read on live head and still contain the intended OFF->ON replay, ON->ON idempotence, stable-revision re-enable assertions, V4 emission inertness, and `authorityBypassesApplied == 0`.

## Remaining live gate

The branch is currently 0 behind canonical and mergeable. The only unresolved role-close item is execution of the already-queued reconciled focused-CI run. Do not add production work merely to keep this role alive. If the PR329-relevant CPU/SDF witnesses pass on that reconciled code tree and no new review evidence appears, this Phase-B observation-only Sun role is complete.
