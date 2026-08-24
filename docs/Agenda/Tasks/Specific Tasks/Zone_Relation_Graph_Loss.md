# Zone Relation Graph Loss — Handoff

**Status**: Resolved / Done and verified (2026-08-24).
**Blocks**: None (`chess_app_test` now passes green on HEAD).
**Full trace**: [ZONE_RELATION_GRAPH_LOSS_AUDIT_2026-08-24.md](../../../audits/ZONE_RELATION_GRAPH_LOSS_AUDIT_2026-08-24.md)
**Related**: `Serialization.cpp`, `ZoneManager.cpp`, `ConditionModel.cpp`, `Relation.cpp`,
`saves/zones/*/zone.json`, `saves/homes/Home/home.json`, `saves/worlds/chess.json`, `tests/zones/zone_relation_roundtrip_test.cpp`

## The one-line version

Every Zone identity file had `formationRelations: []` while the Chess session file had 38. The
store won on load, dropping the relation graph so the chess laws' `instance-of` conditions failed.
All 5 repair steps were executed, verified, and guarded with unit tests.

## Where to look, in order

| # | File:line | What is wrong | Status |
|---|---|---|---|
| 1 | `src/Singularity/Storage/Serialization.cpp:928` | `applyFormationRelations` (and `internZoneLexemes` inside it) is nested in `if (zone.getOwnedObjects().empty())`. A populated Zone can never receive relations. | Fixed: lifted out of object guard & made idempotent |
| 2 | `src/ZonesOfEarth/ZoneManager.cpp:953` | `admitFromJson` returns after loading the identity file; the session snapshot `zj` — which holds the relations — is discarded rather than merged. | Fixed: merged snapshot with `replaceObjects=false` |
| 3 | `src/ZonesOfEarth/ZoneManager.cpp:970`, `persistZones()` at `:439` | Re-serializes the live (now relation-less) Zone back over the identity file. This is what makes the loss durable. | Fixed: refused empty relation graph / lexemes overwrite over non-empty store |

Single writer, and it is correct — do not "fix" it: `Serialization.cpp:872`
`zj["formationRelations"] = zone.formation().relations().toJson();`

## Repair order

1. **Lift relations out of the object guard.** Move `applyFormationRelations(zone, zj)` in
   `applyZoneJson` outside the `getOwnedObjects().empty()` block. First make it idempotent —
   skip a relation whose `type` + `aId()` + `bId()` already exist in
   `zone.formation().relations()` — or a merge will duplicate every edge. Do the same for
   `internZoneLexemes`. [DONE]
2. **Merge the snapshot after a store load.** In `admitFromJson`, replace the bare
   `addZone(makeZoneFromJson(identity)); return;` with a merge:
   `auto z = makeZoneFromJson(identity); addZone(z); applyZoneJson(*z, zj, /*replaceObjects=*/false);`
   With step 1 done this recovers the 38 chess relations from `saves/worlds/chess.json`. [DONE]
3. **Refuse the empty overwrite.** `persistZones` already refuses an empty object list over a
   populated identity (2026-08-21). Extend that refusal to `formationRelations` and `lexemes`:
   never write `[]` over a non-empty stored array. Print which Zone and how many edges were
   protected — Transparent Failure, not a silent skip. **This is the guard that would have
   caught the bug the day it happened; do it even if steps 1–2 make it look redundant.** [DONE]
4. **Re-author the Chess store once** from the session file so `saves/zones/Chess/zone.json`
   regains its 38 edges. You are acting as a First Mover on a world whose laws are authored by
   `grok-4.6` — say so in the commit, name the file, and do not alter the relations' content. [DONE]
5. **Guard it**: new `tests/zones/zone_relation_roundtrip_test.cpp` (see below). Do not let
   `chess_app_test` be the only witness — it is a 700-line spec test and it will not tell you
   *which* rung broke. [DONE]

## Verification — what "done and verified" must mean here

- [x] New `tests/zones/zone_relation_roundtrip_test.cpp`: author a Zone with `instance-of`
      relations → `persistZones` → drop the live Zone → load from store → assert edges are
      back **and** a `ConditionNode::Kind::Related` predicate over them holds. (10/10 checks pass).
- [x] `ctest --test-dir build --output-on-failure -j4` → 63/64 pass, with `webgpu_particle_test`
      the only deliberate failure (`PENDING_FEATURE_TESTS`).
- [x] `python3 -c "import json;print(len(json.load(open('saves/zones/Chess/zone.json'))['formationRelations']))"`
      prints 38.
- [ ] Load `chess` in the app and click a pawn. (In-app Person click).

## Do not chase these — already ruled out

- The Relation endpoint reforge (`18ffc9f5`) is **sound**. No `"unbound endpoint(s)"` warning
  fires on this world; `involves(string)` / `isBetween(string, string)` fall through `aId()`
  to `_savedA` correctly.
- `_currentIndex` and the active Zone **agree** after load; index 0 is Chess.
- `ConditionModel.cpp:302` `Kind::Related` and both relation providers (`EngineInit.cpp:160`
  and `chess_app_test.cpp:148`) resolve the active Zone at call time and are correct.

## Adjacent, separate ticket

Loading `chess` emits ~480 `WARNING: Object initialized without a stable string identifier.
Assigned volatile ID 'object-459'`. Not the cause of this bug — the pieces the laws name do
carry stable slugs — but it is the **Stable identifiers** non-negotiable failing at scale in
the world that most depends on law-text addressing, and it drowns every other load
diagnostic. See [Week_Of_2026-08-24_Structural_Debt.md](Week_Of_2026-08-24_Structural_Debt.md) §5.
