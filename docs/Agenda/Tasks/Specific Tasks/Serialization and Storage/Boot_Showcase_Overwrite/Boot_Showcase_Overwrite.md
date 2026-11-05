# Boot deletes showcase saves in the data home

*Claude Code / Claude Opus 5.5 / session `f1a6b19c-d627-4c8c-a1d3-90d9a0a227d9` / 2026-11-04T16:54-08:00. Found while writing the [October monthly reflection](../../../../../Reflections%20on%20Earthcall's%20Progression/Reflections%20on%20Trajectory/Thirty_Five_Hours_After_Thirty.md#6-the-boot-path-that-deletes-your-saves), commissioned by Zach. Nothing was changed: this records the finding only.*

**Status:** open, unfixed. Found by reading source and inspecting the data home. The app was not launched for this and no overwrite was triggered on purpose.

## The defect

`seedMissingCheckoutIdentities` (`src/Singularity/Storage/SaveSystem.cpp:117`) runs whenever `chooseDataRoot` selects the data home, which includes `Engine.cpp:106` and `FirstMoverTool.cpp:118`. For any name on the hard-coded `isSyncedShowcase` list that already exists in the data home, it runs `fs::remove_all(dst, ec)` and then `fs::copy(...)` from the checkout's `saves/`. `ec` is not checked after either call.

The list covers `Sky-Island Sanctuary`, `SynthesisStudio`, `SynthesisStudio.LivingInstrument` and `Northern Veil`, plus material, Law and world entries with the prefixes or names `sky.sanctuary.`, `law-sky-sanctuary-`, `sky_island_sanctuary.json`, `studio.`, `law-studio-`, `synthesis_studio.json`, `synthesis_studio_living.json`, `northern_veil.`, `northern.veil.`, `law-northern-veil-` and `northern_veil.json`.

`src/ZonesOfEarth/ZoneManager.cpp:1327` also names `Sky-Island Sanctuary` and `Northern Veil` as literals that force `applyZoneJson` over a live Zone.

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

## For the agent who fixes this

1. Add a failing witness first, in an isolated data root: seed, edit a showcase Zone in the data home, run `chooseDataRoot` again, and assert that the edit survives.
2. Remove the `isSyncedShowcase` overwrite and the `ZoneManager` literals. No Zone name belongs in C++.
3. Deliver intended updates as explicit patches under `docs/architecture/law/FIRST_MOVER_AUTHORING.md` §7 rule 8. Keep the original desync problem solved: a checkout update must not silently fail to arrive, so report the divergence loudly instead of resolving it by erasure.

**Pitfalls:**

- Run tests only against an isolated root. Do not launch the unfixed app to reproduce this, because launching is the overwrite.
- Do not commit the data home to "clean it up" before reconciling. `git add -A` there would record the overwritten state as history. Those 54 paths need a Person-authorized reconciliation, and `zones/SynthesisStudio/zone.ecform` should be looked at before anything decides it is obsolete.
- `The Merry Dancers` and several other Zones are untracked in the data home. They are not on the list, so this defect does not touch them, but they should be committed once the reconciliation is done.
