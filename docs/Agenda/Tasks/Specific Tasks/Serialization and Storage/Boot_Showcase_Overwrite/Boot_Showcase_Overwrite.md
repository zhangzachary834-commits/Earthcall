# Boot deletes showcase saves in the data home

*Claude Code / Claude Opus 5.5 / session `f1a6b19c-d627-4c8c-a1d3-90d9a0a227d9` / 2026-11-04T16:54-08:00. Found while writing the [October monthly reflection](../../../../../Reflections%20on%20Earthcall's%20Progression/Reflections%20on%20Trajectory/Thirty_Five_Hours_After_Thirty.md#6-the-boot-path-that-deletes-your-saves), commissioned by Zach. Fixed the same evening at Zach's request ("YAASSSSSS GO FIXXXXXXXX"); see [The fix](#the-fix-2026-11-04).*

**Status:** code fixed. The data home still holds what the old code overwrote, and only Zach can decide what to do about that. See [What remains](#what-remains).

*The sections from "The defect" through "Evidence" describe the behavior before the fix.*

## The defect

`seedMissingCheckoutIdentities` (`src/Singularity/Storage/SaveSystem.cpp`) runs whenever `chooseDataRoot` selects the data home, which includes `Engine.cpp:106` and `FirstMoverTool.cpp:118`. It also ran from `listZoneIdentityRecords` whenever a checkout `saves/` was in the working directory, so the Creator Console's **Reload Store** button could trigger it mid-session. For any name on the hard-coded `isSyncedShowcase` list that already exists in the data home, it runs `fs::remove_all(dst, ec)` and then `fs::copy(...)` from the checkout's `saves/`. `ec` is not checked after either call.

The list covers `Sky-Island Sanctuary`, `SynthesisStudio`, `SynthesisStudio.LivingInstrument` and `Northern Veil`, plus material, Law and world entries with the prefixes or names `sky.sanctuary.`, `law-sky-sanctuary-`, `sky_island_sanctuary.json`, `studio.`, `law-studio-`, `synthesis_studio.json`, `synthesis_studio_living.json`, `northern_veil.`, `northern.veil.`, `law-northern-veil-` and `northern_veil.json`.

`src/ZonesOfEarth/ZoneManager.cpp:1327` also named `Sky-Island Sanctuary` and `Northern Veil` as literals that forced `applyZoneJson` over a live Zone. Boot creates only four kernel Zones before hydration, so these literals only ever fired on Reload Store, where they replaced live objects that every other Zone keeps.

**Consequence:** in-app authoring in these Zones and their materials and Laws is replaced by the checkout snapshot on the next data-root selection. This contradicts the `CLAUDE.md` non-negotiable that save files are patched, never regenerated, and that live saves belong to the data home.

## Origin

- `50c7c2b03` (2026-10-20) introduced `isSyncedShowcase`.
- `4c64b3fd8` (2026-11-02) added Northern Veil, as the resolution in the [desync audit](../../../../../audits/NORTHERN_VEIL_DATA_HOME_PERSISTENCE_DESYNC_AUDIT_2026-11-02.md). That audit was solving a real problem: Firmament's redesign in the checkout never reached the running app.

## Evidence, 2026-11-04

- `zones/SynthesisStudio/zone.ecform` is in data-home `HEAD` and deleted from the working tree; the checkout holds only `zone.json`.
- 24 tracked materials are modified. `materials/studio.A5/material.json` is byte-identical to the checkout copy.
- 54 of the 88 uncommitted paths in the data home have showcase names. The sampled showcase files carry an 11:24 timestamp from today.
- `tests/` has no reference to `isSyncedShowcase`, and `docs/BUILD_AND_ENVIRONMENT.md` does not describe the behavior.

**Unknown:** whether any human work has been lost. Committed versions are recoverable from data-home git. Uncommitted in-app edits to these Zones would not be, and nothing checked here shows whether any existed.

## The fix, 2026-11-04

- **`SaveSystem.cpp`:** `isSyncedShowcase` is gone. An entry the home holds is never touched. Missing identities are still seeded, and dotfiles are neither seeded nor reported.
- **`chooseDataRoot` note:** it now lists entries whose checkout copy has a later file date and different bytes, up to eight names plus a count of the rest. This keeps the original desync from going silent.
- **`ZoneManager.cpp`:** both Zone-name literals are removed, so these Zones now keep live objects on Reload Store like every other Zone.
- **Workflow change:** a checkout edit to an existing Zone no longer reaches the running app by itself. Patch it into the data home under `FIRST_MOVER_AUTHORING.md` §7 rule 8. The boot note will prompt you.

**The note cannot tell which copy is really newer.** It compares file dates, and git operations move those. On this machine it will list `zones/Sanctum of Beginnings`, whose checkout copy is a 9 MB Oct 5 snapshot still holding the test shapes Zach cleared from the home on Oct 17. Do not install that one. Compare before installing anything the note names.

**Evidence:**

- `data_home_test` gained witnesses that failed 7 checks on the old code, including all seven entries overwritten, and now pass. They cover the root-choosing path and the Zone-listing path.
- `zone_facetexture_test` gained a Reload Store witness. With the literal temporarily restored it fails (20/21), and with the fix it passes.
- Rebuilt and passing with fresh binaries: `save_system_error_test`, `zone_identity_test`, `zone_spatial_field_roundtrip_test`, `zone_boot_hydration_relations_test`, `home_identity_continuity_test`.
- `build/earthcall_webgpu` and `build_release/earthcall_webgpu` were rebuilt. Neither was launched, so the behavior in the live app is unverified until Zach checks it.

**Not witnessed:**

- `zone_home_ontology_test` fails two checks, but it failed the same two with an Oct 16 binary that predates this work. It expects `home.json` where the engine now writes `home.ecform`.
- `sky_island_sanctuary_test` does not link in the Debug build (undefined WebGPU renderer symbols), so it was not run.
- `/Applications/Earthcall.app` should not have been affected, from reading the code: a bundled app passes no checkout path and runs from the data home, which has no `saves/` folder. This was not tested.

## What remains

1. **Zach's decision: reconcile the data home.** 54 of its 88 uncommitted paths have showcase names, mostly what the old code wrote over the home's committed versions. For each, the choice is to keep the current bytes or restore from data-home `HEAD`. `zones/SynthesisStudio/zone.ecform` is deleted from the working tree and recoverable from `HEAD`.
2. **Any uncommitted in-app work in those Zones may already be gone.** Only Zach knows whether any existed.

**Pitfalls for whoever does the reconciliation:**

- Do not run `git add -A` in the data home first. It would record the overwritten state as history.
- `The Merry Dancers` and several other Zones are untracked in the data home. They were never on the list, and should be committed once the reconciliation is done.
