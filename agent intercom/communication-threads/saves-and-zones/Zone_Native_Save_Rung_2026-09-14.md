# Zone-Native Save Rung — continuing the retirement of conglomerate session loading

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT / Earthcall continuation requested by Zach  
**Date:** 2026-09-14, started 21:32 PDT  
**Branch:** `sol-zone-native-loading-2026-09-14`  
**Parent:** `6479bbae94cae51203d57fb791a30c3a542414f6` (`sync-from-earthcall-main`)

## Human intent carried forward

Zach asked to continue where the other agents left off on **Zone-based loading and the retirement of the conglomerate load system**. This pass follows the correction already recorded in `Per_Zone_serialization_pathway.md`:

> Creator Console -> Zones -> **Move to Zone** -> **Save Zone**

A `saves/worlds/` session bundle is therefore compatibility/recovery machinery, not the ordinary unit a Person authors or inhabits.

## What this rung changes

### 1. One-Zone persistence is now a first-class storage boundary

`ZoneManager` now exposes:

- `persistZone(size_t index)`
- `persistActiveZone()`

The operation writes exactly one Zone/Home identity and the shared non-First-Mover Law roots named by that identity's `lawRefs`.

It deliberately does **not** call `saveState`, `saveStateWithLog`, or any `SaveType::WORLD` writer. It preserves the existing anti-erasure guards for empty live Zones and relation/lexeme graph loss. Law closure is semantically preflighted before the Zone identity is mutated: malformed `lawRefs`, absent `LawManager`, or a named Law missing from the running register refuse the save with no Zone write.

This is the storage primitive the old UI was missing. `persistZones()` remains as the bulk/compatibility writer for legacy session and cross-root operations; it is no longer the only way to commit authored spatial work.

### 2. The Creator Console now tells the truth about movement

`ZonesConsole.cpp` no longer makes selecting a row synonymous with teleporting. It now has distinct acts:

1. select a Zone;
2. **Move to Zone** — invokes the existing activation/preflight path;
3. **Save Zone** — commits only the currently active Zone through `persistActiveZone()`.

A refused move says that the current Zone remained unchanged. A refused save points the Person to the console refusal instead of silently pretending success.

### 3. The old conglomerate UI is being retired, not deleted blindly

`AssetsConsole.cpp` now presents `saves/worlds/` as **Legacy Session Import / Recovery** and **Legacy Session Export**. The normal instruction shown to the Person is to use Creator Console -> Zones for movement and ordinary saving.

The old session machinery remains readable and operable for migration, recovery, old authored apps, and compatibility witnesses. This is intentional: retirement means removing it from the ontology of ordinary use, not destroying historical access before Zone closure is complete.

### 4. The isolation invariant has an executable witness

`tests/zones/zone_native_save_isolation_test.cpp` performs the important mechanical experiment:

1. seed Zone Alpha and Zone Beta;
2. record both identity files;
3. mutate Alpha only;
4. call `persistActiveZone()`;
5. require Alpha to change;
6. require Beta to remain **byte-for-byte identical**;
7. require no regular file to appear under `saves/worlds/`;
8. require an invalid Zone index to refuse.

The focused GitHub Actions workflow includes this test target so a branch push actually compiles and executes the new boundary rather than merely documenting it.

## What this does NOT claim

This is **not completion** of the reopened per-Zone serialization task.

Still open:

- general shared Material roots instead of Zone-embedded material snapshots;
- shared Category roots;
- Zone-scoped physical matter generations / `.ecmatter` closure;
- detached whole-closure transaction semantics across Zone + Laws + Materials + Categories + matter (the current Law roots are individually atomic, but a later filesystem failure can still occur after the Zone file succeeds);
- migration of the remaining authored app Laws onto Zone-owned/shared-root closure;
- removal or rewiring of every remaining top-level legacy session shortcut (notably old main-menu Save As / Load affordances) once their recovery role has an explicit home;
- full activation transaction beyond the existing Law-root preflight, including every root kind before `switchTo` mutates live state.

The next architectural rung should therefore **extend the closure, not invent another save office**: identify Material/Category references by stable root id, preflight them alongside `lawRefs`, and make Move to Zone assemble a complete detached closure before any live-world mutation.

## Integrity note

No authored file under the real `saves/` tree was modified in this pass. The new regression witness points `SaveSystem` at a temporary sandbox. This honors the repository's rule that save files are a Person's authored work, not disposable fixtures.
