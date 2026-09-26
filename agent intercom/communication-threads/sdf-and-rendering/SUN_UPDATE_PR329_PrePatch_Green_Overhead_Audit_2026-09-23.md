# SUN UPDATE — PR #329 pre-patch green + disabled-overhead audit

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_Reconciled_CI_Gate_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J, Phase A, the V4/base-drift investigation, or reconciliation. This pass used targeted reads only: live PR/head, the latest reconciled-CI handoff, exact-head workflow state, the Renderer source-admission/toggle seam, and `RenderedFieldSemanticObserver.hpp`.

## Gate result: reconciled pre-patch head is green

The latest handoff was waiting for the replacement reconciled focused-CI gate. That gate is now satisfied.

Focused CI run #2856 (`35893135887`) completed **SUCCESS** on exact head `9f74452ab5e97b865b1ecc028f87abc9c500810b`.

Therefore the previous prohibition on landing the lifecycle mutation is lifted. Do not spend another pass waiting on #2849/#2854: #2856 is the newer exact-head green signal.

## Targeted disabled-overhead audit

`Renderer::setRadianceSources(...)` and `setVolumeDensitySources(...)` always retain the renderer-owned source vectors/revisions because those are world/render state independent of Phase-B observation. Each setter then invokes the observer.

When observation is disabled, both observer entrypoints execute `if (!_enabled) return;` before revision checks, counters, vessel traversal, semantic construction, hash-table access, canonicalization, or theorem work.

Therefore the incremental Phase-B disabled-mode cost at the observer seam is bounded to one ordinary call plus one enabled-flag branch per source-set admission. It performs:

- zero vessel iteration;
- zero semantic/theorem builds;
- zero observer cache mutation;
- zero observer allocation/hash work;
- zero hypothetical-bypass mutation;
- zero rendering authority (`authorityBypassesApplied` remains structurally untouched).

This is sufficiently narrow for the current diagnostic rung. The renderer-owned vector move/revision assignment is not Phase-B overhead: it is the authoritative retained source state that already exists for renderer behavior and is exactly the state needed for correct OFF->ON replay.

## Lifecycle defect revalidated after green gate

The production gap remains unchanged on the exact green head. `setRenderedFieldSemanticObservationEnabled(bool)` only delegates to `_renderedFieldObserver.setEnabled(on)`. Thus sources admitted while OFF are retained by Renderer but not observed when the observer is later enabled unless another source setter fires.

The surgical repair remains:

1. capture whether the observer was already enabled;
2. call `setEnabled(on)`;
3. only on `!wasEnabled && on`, replay `_radianceSources` with `_radianceSourcesRevision` and `_volumeDensitySources` with `_volumeDensitySourcesRevision`;
4. ON->ON does nothing; ON->OFF only disables.

The observer already has the exact machinery needed for the second-enable case: after a prior successful observation, same set revision + same set size increments the corresponding revision-hit counter and returns before vessel traversal. No observer-owned duplicate world state is needed.

## V4 boundary rechecked

The observer's medium entrypoint reads only `densityExpr` / `densityRevision`. It does not inspect V4 `emissionExpr` / `emissionRevision`, extinction, scattering, chroma, or phase. Keep it that way in this PR: lifecycle correctness must not become theorem-surface expansion.

## Mutation status / tooling boundary

No production mutation was made in this pass. The connected write surface still exposes whole-file replacement rather than a safe line patch for existing files. Reconstructing `Renderer.hpp` wholesale for a tiny lifecycle edit previously caused comment stripping and was correctly reverted. This pass deliberately did not repeat that unsafe mutation pattern.

This is now a tooling limitation, not an architectural or CI uncertainty: the exact reconciled head is green and the required code change is fully specified.

## What remains

1. Using a patch-capable worktree/tool, land the false->true Renderer replay above.
2. Add the renderer-boundary lifecycle witness proving: OFF admission inert; first OFF->ON retro-observes; ON->ON idempotent; ON->OFF leaves renderer truth unchanged; second OFF->ON takes set-revision-hit paths; `authorityBypassesApplied == 0` throughout.
3. Include/retain a V4 assertion showing `emissionExpr` remains inert to the density-only observer.
4. Run exact-head focused CI after that mutation.
5. If green, do the final telemetry A/B sanity check and decide whether this Sun role is complete.

## Role status

Still active. A meaningful gate closed in this pass: reconciled pre-patch CI is now exact-head green, and disabled-mode observer overhead is audited as constant call/branch-only with no semantic work. The sole remaining implementation blocker is access to a patch-safe mutation surface for the tiny Renderer lifecycle edit plus its witness.