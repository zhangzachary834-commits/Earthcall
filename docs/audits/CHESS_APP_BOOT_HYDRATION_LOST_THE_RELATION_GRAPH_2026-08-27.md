# Chess App: boot hydration cost the Chess zone its relation graph, so no piece was a piece

**Date:** 2026-08-27
**Author:** Claude Opus 5
**Session:** https://claude.ai/code/session_01Ewyb3adQJuSmsgG2x5Yp7p
**Asked by:** Zach — *"loaded the chess app, tried to click a pawn and move it forward, still did
nothing. The event log did register the pawn being clicked (not just the generic mouse click) and
yet still nothing."* Then: *"merely clicking shows `conditions failed -> piece-white-pawn` in Recent
Applications; clicking and dragging shows sequence/publish executed on piece-white-pawn in Recent
Action Nodes Fired, and still no movement."*
**Fix:** `src/ZonesOfEarth/ZoneManager.cpp` — `admitFromJson`'s live-zone branch
**Guard:** `tests/zones/zone_boot_hydration_relations_test.cpp` (verified red without the fix)
**Follows:** [`CHESS_APP_EVERY_GESTURE_IS_A_DRAG_2026-08-26.md`](CHESS_APP_EVERY_GESTURE_IS_A_DRAG_2026-08-26.md)
— that defect was real and is fixed (`78266028`); it was not the last one standing.

---

## 1. What was wrong

**The Chess zone ran with zero formation relations, so `instance-of category.chess.piece` was false
for every piece, for the whole session.**

Two things went wrong, in this order, and only in this order:

1. **Boot hydrates zones before categories exist.** `Engine::initLogic` calls
   `ZoneManager::hydrateFromZoneStore()` (`EngineInit.cpp:193`) before any world is loaded, so every
   Zone under `saves/zones/` is already live by the time a Person clicks Load. Hydration builds the
   Chess zone from `saves/zones/Chess/zone.json` and runs `applyFormationRelations`, which tries to
   bind all 38 edges. But **categories are world data** — they load in `ZoneManager::loadState`
   (`categories.loadFromJson`, `ZoneManager.cpp:973`), never at boot. So `category.chess.piece` does
   not exist yet, every `instance-of` edge comes back with an unbound endpoint, and `Formation::add`
   refuses it:

   ```
   Relation::fromJson: unbound endpoint(s) type='instance-of' a='piece-white-rook-0-0'
                       b='category.chess.piece'.
   Formation 'formation-239': REFUSED relation 'instance-of' with unbound Singular endpoints.
   ```

2. **Nothing ever tried again.** When the Person then loads `chess_app.json`, categories load
   correctly — but `loadState`'s `admitFromJson` found the Chess zone **live** and, for a
   non-snapshot load, returned without merging the session's zone JSON at all:

   ```cpp
   if (auto live = findLive(id)) {
       if (snapshotRestore) applyZoneJson(*live, zj, true);
       return;                     // ← the session snapshot skipped entirely
   }
   ```

This is Bug #7 (*the Zone identity store lost the relation graph*) arriving through the one door its
2026-08-24 fix did not cover. That fix lifted `applyFormationRelations` out of the empty-objects
guard so it "always runs on load" — but it cannot run on a load path that never calls
`applyZoneJson` in the first place.

## 2. Why it looked exactly like what Zach reported

`law-chess-click`'s condition is `isBoard == true` **OR** `instance-of category.chess.piece`.

- Clicking the **board** passes via `isBoard`, which is an authored **property on the object** and
  survives everything. Zach's own audit log has it succeeding at 12:08:41.
- Clicking a **piece** has only the relation branch left → `CONDITIONS FAILED`. That is verbatim what
  Zach saw in the Law Author's Recent Applications: `conditions failed -> piece-white-pawn`.
- `law-chess-select` requires the same relation, so nothing was ever selected — no lift, no move.
- **Dragging** was the confusing part, and it is explained too: `law-chess-drag-drop`'s condition is
  `@interaction-channel.hoveredId != ""` — no piece test — so its action nodes genuinely fired
  (Zach saw "sequence executed / publish executed on piece-white-pawn" in Recent Action Nodes Fired).
  It published `square-clicked` faithfully. But nothing was selected, so no move law matched, and the
  piece did not move.

Every symptom, including the two that looked contradictory, is this one cause.

## 3. Why every test passed

**No test had ever booted the way the app boots.** A test process starts with no zones, so
`admitFromJson` takes the store-hit branch, merges the session JSON, and by then categories are
loaded. `chess_app_test`, `chess_click_geometry_test`, `zone_relation_roundtrip_test`, and my own
`chess_app_full_loop_probe` all did it in that order and all passed.

Adding one line — `zones.hydrateFromZoneStore()` before `zones.loadState(...)` — flips the probe
from green to the Person's exact failure:

```
### fresh (what every test does) ###
formation relations: 38 total, 35 instance-of, 35 with both endpoints bound
  law-chess-click  -> piece-white-pawn-4-1 applied
  law-chess-select -> piece-white-pawn-4-1 applied
RESULT: pawn walked e2-e4.

### boot-hydrated (what the app does) ###
formation relations: 0 total, 0 instance-of, 0 with both endpoints bound
  law-chess-click  -> piece-white-pawn-4-1 conditions-failed
RESULT: **pawn did not move**
```

## 4. The fix

`ZoneManager::admitFromJson` now merges the session's zone JSON into a **live** zone too, with
`replaceObjects` on only for a snapshot restore:

```cpp
applyZoneJson(*live, zj, /*replaceObjects=*/snapshotRestore);
```

Safe by construction: `replaceObjects=false` never clears the live objects, `applyZoneJson` only
fills objects when the zone is empty, and `applyFormationRelations` is idempotent by
`type + aId + bId`. It is the same remedy the store-hit branch already applies, extended to the
branch that was returning early.

After the fix, the app's own sequence gives 38 relations, all 35 `instance-of` edges bound, and the
pawn walks e2–e4 through a real `observe()`-driven click.

## 5. Verified

- `tests/zones/zone_boot_hydration_relations_test.cpp` — new. Hydrates from the store first, then
  loads, then clicks. **Confirmed red without the fix** (4 of 6 checks fail, relations = 0) and green
  with it.
- Full suite: **72 / 73**. The one failure is `smooth_tessellation_cache_test`, the pre-existing
  failure `CLAUDE.md` and Bugs.md #11 already name; it touches no load path.
- Probe matrix after the fix: fresh ✓, boot ✓, boot + 7 px of click travel ✓ (the 08-26 gesture fix
  holds), boot + **cursor locked** ✗ — still picks at the viewport centre, see §6.

## 6. Still open

- **Cursor locked still picks at the viewport centre.** `78266028` made this legible by drawing a
  reticle, which is the right half of the answer; the behaviour is unchanged, so a Person who plays
  with the cursor locked still clicks the crosshair square rather than the piece under the (invisible)
  pointer. Not Zach's current failure — he was unlocked, which is why his pawn click registered at
  all — but it will be somebody's.
- **The law audit log is unusable, and it is a lag suspect.** `ourverse-gathering-unowned` is a
  continuous `Everyone`-scope law with no action model, and it logs one `NO ACTIONS` application per
  being per tick. In Zach's session it burned the logger's entire 200,000-line budget in **29
  seconds** — the run went silent seven seconds after the chess world finished loading, which is why
  his clicks are not in `logs/law_audit.log` at all. `ControlPatterns::createHoverResponseLaw` already
  solves this correctly for the same shape of law: it ships `setEnabled(false)` precisely because
  *"a law with nothing to do should not be sweeping the world every tick."* `ourverse-gathering-unowned`
  should do the same, and an application with no action model should not reach the audit log at
  Summary level. This blinded three sessions of diagnostics and is a plausible slice of the standing
  chess-frame cost.
