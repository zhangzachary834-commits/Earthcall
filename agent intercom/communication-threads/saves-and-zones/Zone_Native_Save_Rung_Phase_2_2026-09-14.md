# Zone-Native Save Rung — Phase 2: remove the top-level conglomerate lie

**Author:** GPT-5.6 Sol  
**Date:** 2026-09-14 PDT  
**Branch:** `sol-zone-native-loading-phase2-2026-09-14`  
**Base at branch birth:** `9a9af8327622ca33b7afa33ca9101b1aad4ea906` (`sync-from-earthcall-main`)

## Human intent carried forward

Zach asked to continue the next phase immediately after PR #165 landed. That PR made one-Zone persistence real and demoted the Assets session surface, but its handoff explicitly named a remaining contradiction: the top-level main menu still presented **Quick Save / Save As / Load / Save Manager** as ordinary persistence, and `Quick Save` still called `ZoneManager::saveStateWithLog`, thereby minting a legacy `saves/worlds/` session.

This phase removes that contradiction without deleting the recovery path before full Zone closure exists.

## What changed

### 1. `S` now means Save Active Zone

`EngineInit.cpp` now registers:

- `S` → **Save Active Zone** → `ZoneManager::persistActiveZone()`
- `A` → **Legacy Session Export...**
- `L` → **Legacy Session Import / Recovery**
- `G` → **Legacy Session Manager**

The ordinary S action no longer builds a `SaveContext` and no longer routes through `saveStateWithLog`. A refusal is printed loudly and explicitly says that no legacy session was written.

This is the semantic point of the rung: **there is no top-level ordinary action left whose name says “save” while secretly meaning “serialize a conglomerate session.”**

### 2. The keymap tells the same truth

`Engine::renderKeymapContent()` now labels the section **Persistence**, describes S as Save Active Zone, and names A/L/G as legacy-session compatibility/recovery operations. The keymap no longer advertises `Quick Save`, `Save As`, `Load`, or `Save Manager` as if they were peers of Zone-native persistence.

### 3. Compatibility is retained deliberately

The A/L/G actions still open the already-demoted Assets legacy-session windows. This is intentional. Phase 2 is retirement from ordinary ontology, not destruction of historical recovery/migration access before shared Material/Category/matter closure exists.

## Verification split

### Mechanical witness

The existing `zone_native_save_isolation_test` from Phase 1 remains the logic witness for the exact primitive now wired to S. It proves that `persistActiveZone()`:

- changes the active Zone identity;
- leaves an unrelated Zone byte-for-byte untouched;
- creates no regular file under `saves/worlds/`;
- refuses an invalid Zone index.

The focused CI workflow already builds/runs that witness. Phase 2 additionally changes `EngineInit.cpp` and `Engine.cpp`, so the focused build is also the compile witness that the real main-menu caller is wired to the live primitive.

### Person-facing witness still required

A Person should run the native app and verify the actual visible surface:

1. Open the main menu with `M`.
2. Confirm the ordinary entry reads **Save Active Zone**.
3. Confirm the old session operations are visibly marked **Legacy Session Export...**, **Legacy Session Import / Recovery**, and **Legacy Session Manager**.
4. Modify the active Zone, invoke **Save Active Zone**, then inspect/reopen that Zone and confirm the modification persisted.
5. Confirm that performing only this Zone save does not create or rewrite a normal `saves/worlds/` session.
6. Open Controls / Keymap with `K` and confirm it describes the same Zone-native/legacy split.
7. Open each A/L/G legacy surface once and confirm it still reaches the compatibility/recovery window rather than silently behaving like ordinary Zone movement/saving.

Do not mark this Person-facing path verified merely because the C++ compiled.

## What remains open

This phase does **not** claim the per-Zone serialization pathway is complete. The next architectural work remains the closure itself:

- shared Material roots referenced by stable ids instead of Zone-embedded snapshots;
- shared Category roots;
- Zone-scoped physical matter generations / `.ecmatter` closure;
- activation preflight for every shared root kind before `switchTo` mutates live state;
- a detached whole-closure transaction across Zone + Laws + Materials + Categories + matter, so a filesystem failure cannot leave only the first half committed;
- migration of remaining authored app roots onto that complete closure;
- eventual deletion or further isolation of legacy session machinery only after migration/recovery no longer depends on it.

## Integrity note

No authored file under the real `saves/` tree was modified in this phase. This was wiring/chrome only. The ordinary top-level save act now points at the Zone-native primitive created in Phase 1; legacy session access remains explicit and quarantined by name.