# Unmerged Branch Consolidation — 2026-09-12

This document records the curated consolidation of Earthcall's unmerged branch/PR archaeology into `integration/unmerged-superbranch-2026-09-12`, based on `sync-from-earthcall-main` at `50edc2b22dff799a14c71350d78c14944faae6bb`.

The policy was deliberately conservative: merge a branch intact only when Git can reconcile it coherently with current Earthcall; transplant an isolated file/test when the surviving delta is clearly separable; never overwrite current subsystems with an ancient branch snapshot merely because the old idea remains valuable.

## Integrated into the superbranch

- **PR #130 / `sol/person-not-object` — full integration.** Prevents registered Person identities such as `Zach` from re-entering CategoryManager as counterfeit ordinary Objects while preserving legitimate `category.*` roots. Includes focused and world-hydration regression tests plus the ontology handoff.
- **PR #128 — isolated Camera regression test.** `tests/singularity/camera_test.cpp` was absent from current default and was transplanted without importing stale branch state.
- **PR #58 — isolated BodyPart regression test.** `tests/person/body_part_test.cpp` was absent from current default and was transplanted independently.
- **PR #56 — `open_url` regression coverage.** The current Python agent test file already contained newer adjacent tests, so the branch's distinct `open_url` success/error coverage was preserved without rolling the file backward.
- **PR #59 — full semantic merge.** `EarthcallAPI::deleteDesignElement` now deletes through top-level `DesignSystem::removeShape/removeText/removeEffect`, preserving DesignSystem layer synchronization instead of bypassing it through child subsystems.
- **PR #127 — full documentation merge.** Updates `DIRECTORY_ORDERING.md` to the current source layout.
- **`sol/mcp-sdf-authoring` — full surviving delta.** Preserves `mcp_sdf_contract_test.cpp` and the Sol handoff documenting the mismatch between advertised function-like SDF expressions and the actual implicit `f(x,y,z)=0` parser. The abandoned broad WebSocket rewrite had already been removed from that branch and was not reintroduced.
- **PR #51 — full semantic merge.** Current default had an empty `src/Person/Relationship/Relationship.cpp` and no relationship test; the branch restores the implementation and regression coverage.
- **PR #31 — PaintConsole file transplanted only.** Current default still shipped the 16-line `Paint (Disabled)` stub. The complete 2D Paint tool belt was transplanted: drawing presets, shape/utility/text tools, color editing, brush dynamics, layers, and undo/redo. The separate stale `Singular.cpp` delta was *not* force-applied.

## Already landed or superseded — intentionally not merged again

- **`sol/authorable-light`** is `0` commits ahead of current default and `24` behind: its work has already been absorbed.
- **PR #120 / old Two-Path Testing Doctrine branch** is superseded by the later merged doctrine work (#123 lineage).
- **PR #83 / old logger-array optimization** is superseded by later reconciled logger work already on default; do not resurrect the old snapshot.
- **PR #77 / old CORS fix** is superseded. Current default already reads `CORS_ALLOWED_ORIGINS`, uses restricted/same-origin behavior, and has configuration tests.
- **PR #29 / old DesignElement API implementation** is substantially superseded: current `EarthcallAPI.cpp` already contains create/modify/delete/get DesignElement plumbing. The distinct deletion-layer bug from that family was salvaged separately through PR #59.
- **PR #129 / Palette no-op** contains no useful code change. Its own body says no suitable UX enhancement was found.

## Still-live ideas that must be ported surgically, not branch-merged

These are valuable, but their branches are stale enough that wholesale merging would risk replacing modern Earthcall with old subsystem snapshots.

### High priority

- **PR #24 — EarthcallAPI camera wiring.** Current default still has `getCameraPosition()` returning hardcoded `(0,0,0)` and `setCameraPosition()` not calling `Core::Camera`. The old branch is hundreds of commits behind. Port only the Camera include plus the tiny get/set implementation and update the current tests.
- **PR #45 family (#45, #39, #35, #33) — duplicate Singular dead-member cleanup.** Current default still contains `parentFormationInstances`, `childFormationInstances`, and placeholder `satisfiesKernelBounds()`. Four agent timelines independently remove essentially the same debt. Port those deletions directly against today's `Singular.hpp`; do not merge any of the four historical branches wholesale.
- **PR #53 — VM opcode / structural-revision work.** Current `NativeBytecodeVM::emitNode` still handles only a small subset and falls through to the default for complex Action kinds. The old PR spans nine core files, so its opcode coverage, Lerp/Scale semantics, and structural-revision fixes need a current-code reimplementation/review rather than a historical merge.

### Medium priority

- **PR #17 — `ZoneManager::forkZone` tests.** `tests/zones/zone_fork_test.cpp` is absent from current default, but the historical branch also touches CMake and several unrelated/compatibility files. Extract the test and register it against current CMake rather than merging the branch.
- **PR #31 residual — `Singular.cpp` property-path synchronization.** The PaintConsole portion was safely transplanted; the second file delta still needs a current-code audit before porting.
- **PR #52 — Relation/Condition performance experiment.** The branch proposed cached endpoint IDs, O(1) `Universe::findBeing`, and pointer-based related-condition comparisons. Current default does not obviously contain the `_cachedAId/_cachedBId` form, but this is hot-path architecture; re-benchmark and re-derive against current Rete/string-interning work before importing it.

## Quarantined historical divergence

- **`geminis-wild-west`** is only 3 commits ahead but 578 commits behind current default and modifies `Game.hpp`, `GameToolbar.cpp`, `Physics.cpp`, and `Physics.hpp` (plus `.DS_Store`). It should be reviewed as an archaeological feature proposal, not merged into the superbranch automatically.

## Review guidance

The superbranch is intentionally not a promise that every surviving change is correct. It is a *reviewable convergence point*: coherent unmerged work has been assembled without changing the default branch, while stale-but-live ideas are named explicitly instead of being silently lost or force-merged.

Current `CMakeLists.txt` uses `file(GLOB_RECURSE TEST_FILES "tests/*.cpp")`, so the newly preserved C++ test files are automatically configured as test targets; they do not need individual CMake registration lines.

Before merging the superbranch to default, pay particular attention to:

1. the restored `Relationship.cpp` behavior and its assumptions about Relation endpoints;
2. the resurrected PaintConsole's compatibility with today's CreationChannel/BrushSystem/CreatorConsole state;
3. the Person-not-Object hydration guard against current save fixtures;
4. CI/build results for the automatically discovered transplanted tests;
5. the DesignSystem deletion path and its layer bookkeeping.

After this superbranch review, the most valuable follow-up salvage order is: **camera API wiring → Singular dead-member cleanup → VM opcode/revision port → Zone fork test extraction → performance branch re-benchmark**.
