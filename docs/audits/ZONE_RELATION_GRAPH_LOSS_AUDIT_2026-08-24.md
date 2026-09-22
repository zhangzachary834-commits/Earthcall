# Zone Relation Graph Loss — Audit

**Date**: August 24, 2026
**Auditor**: Claude Opus 5
**Trigger**: Weekly review of `4265279e..ce5c1cbe` (Aug 17–24) commissioned by Zach, with the
instruction to reach an independent position before reading any other model's write-up. No
`docs/Reflections on…` file was opened; every claim below comes from the source, the save
files, and a run of the suite.
**Target subsystems**: `Serialization.cpp` (`applyZoneJson`, `applyFormationRelations`),
`ZoneManager.cpp` (`loadState` / `admitFromJson`, `persistZones`), `Relation` / `Formation`,
`ConditionModel.cpp` (`Kind::Related`).
**Severity**: High. Authored ontology is lost on a save/load cycle, and the loss becomes
durable in the Zone identity store.

---

## 1. Executive summary

**Every Zone identity file in the repository carries `formationRelations: []`.** Not one of
the seven Zones, nor `saves/homes/Home/home.json`, holds a single relation. The Chess session
file holds 38 (35 `instance-of`, 3 `subcategory-of`); its identity file holds none.

Because the Zone identity store now wins over the session snapshot on load — the correct
consequence of the per-Zone identity work (`a71042db`, `ed9b2eda`) — those 38 relations are
never read back. The chess laws gate on them. `chess_app_test` therefore fails on `HEAD`,
and it is **not** in `PENDING_FEATURE_TESTS`:

```
61/63 pass.  webgpu_particle_test  — deliberate (PENDING_FEATURE_TESTS)
             chess_app_test        — real regression, undocumented
```

`CLAUDE.md` and `docs/BUILD_AND_ENVIRONMENT.md` both still say 62 pass.

The chess app is not a toy here. It is the only end-to-end evidence that an application can
be authored inside Earthcall as pure Law plus data — 35 ECA laws with piecewise OntoMath
functions, authored by `grok-4.6` as First Mover, no C++ chess anywhere. That proof is
currently red, and it is red because the save system dropped the relation graph the laws
reason over. This is the failure mode Zach named as CRITICAL at the top of the to-do list on
2026-08-14: *"we don't want developer worlds unstable or erased in the fragile states of
testing and developing features that rely on the save system persisting my prior changes."*

---

## 2. Reproduction

```sh
cmake --build build -j8
ctest --test-dir build -R chess_app_test --output-on-failure
```

```
Assertion failed: (asBool(*whitePawnE2, "isSelected")),
  function main, file chess_app_test.cpp, line 221.
  tick records: 2
    law-chess-click  -> piece-white-pawn-4-1  conditions-failed
    law-chess-select -> piece-white-pawn-4-1  conditions-failed
```

Both laws carry a `Kind::Any` condition over two children — `isBoard == true`, or
`Kind::Related{ relationType: "instance-of", otherId: "category.chess.piece" }`
(`saves/worlds/chess.json`, `authoredLaws.laws[0..1]`). The pawn is neither the board nor,
as far as the engine can see, an instance of anything.

State of the two files:

| File | `formationRelations` | `lexemes` | objects |
|---|---|---|---|
| `saves/worlds/chess.json` (session) | **38** | — | 104 across 7 zones |
| `saves/zones/Chess/zone.json` (identity) | **0** | 0 | 35 |

And across the whole store:

```
saves/zones/Cavern of Light/zone.json          formationRelations: 0
saves/zones/Character Architect Forge/zone.json formationRelations: 0
saves/zones/Chess/zone.json                     formationRelations: 0
saves/zones/Home/zone.json                      formationRelations: 0
saves/zones/Ourverse Gathering/zone.json        formationRelations: 0
saves/zones/Sanctum of Beginnings/zone.json     formationRelations: 0
saves/zones/Temple of Echoes/zone.json          formationRelations: 0
saves/homes/Home/home.json                      formationRelations: 0
```

---

## 3. Root cause — three defects in series

### 3.1 The store is preferred over the snapshot, and the snapshot is then discarded whole

`ZoneManager.cpp:953` `admitFromJson`:

```cpp
if (!snapshotRestore && SaveSystem::zoneIdentityExists(id)) {
    nlohmann::json identity = SaveSystem::readZoneIdentity(id);
    if (identity.is_object()) {
        addZone(makeZoneFromJson(identity));
        return;                     // <-- session `zj` never consulted
    }
}
```

Preferring the identity file is right; that is the point of the per-Zone work. What is wrong
is that the session snapshot is dropped *entirely* rather than merged. Anything the store is
missing — here, the entire relation graph — is unrecoverable from a file that is sitting
right there holding it.

### 3.2 Relations are loaded only on the branch that also loads objects

`Serialization.cpp:928`, inside `applyZoneJson`:

```cpp
if (zone.getOwnedObjects().empty()) {
    if (zj.contains("world"))        zoneObjectsFromJson(zj["world"], zone);
    else if (zj.contains("objects")) zoneObjectsFromJson(zj, zone);
    applyFormationRelations(zone, zj);   // <-- nested inside the empty-objects guard
}
```

A Zone that already has objects — kept live, or hydrated from the store a moment earlier —
can never receive relations, even when the caller passed `replaceObjects == false` and
plainly intends a merge. The relation graph has no independent load path. `internZoneLexemes`
sits inside `applyFormationRelations`, so the `lexemes` array is lost by the same guard; every
store file shows `lexemes: 0` as well.

### 3.3 The next save makes the loss permanent

`zoneToJson` (`Serialization.cpp:872`) serializes from the live Zone:

```cpp
zj["formationRelations"] = zone.formation().relations().toJson();
```

There is exactly one writer, and it is correct. But it writes what the live Zone actually
holds — and after 3.1 and 3.2 the live Zone holds nothing. `persistZones()` (called from
`ZoneManager.cpp:272, 574, 753, 766, 1039`) and the first-admission write at
`ZoneManager.cpp:970` then stamp `formationRelations: []` over the identity file. From that
point the store is authoritative *and* empty, and every subsequent load reads zero.

That is why all eight identity files agree: this is not one bad save, it is a cycle that
converges on zero.

---

## 4. What this is not

Ruled out during the trace, recorded so nobody re-walks them:

- **Not the Relation endpoint reforge (`18ffc9f5`).** Endpoints resolve cleanly on load —
  `Relation::fromJson`'s `"unbound endpoint(s)"` warning never fires on this world. The
  string overloads `involves(const std::string&)` and `isBetween(const std::string&, …)`
  correctly fall through `aId()` to `_savedA`, so law-text queries still match a bound
  relation. The reforge is sound.
- **Not a stale active zone.** `_currentIndex` and the active Zone agree after load; index 0
  really is Chess (`"Now in Chess: 7 zone(s), 104 object(s)"` is printed from
  `_zones[_currentIndex]->name()`).
- **Not the `Kind::Related` predicate.** `ConditionModel.cpp:302` reads
  `Universe::instance().relations()`, and both `EngineInit.cpp:160` and the test's own
  provider resolve the active Zone's formation at call time. The predicate is fine; the
  graph it queries is empty.

---

## 5. Collateral: 480 volatile identifiers on load

The chess load emits ~480 lines of:

```
WARNING: Object initialized without a stable string identifier.
         Assigned volatile ID 'object-459'. This object should not be
         reliably targeted by Law text.
```

The pieces the laws name (`piece-white-pawn-4-1`) do carry stable slugs, so this is not the
cause of the condition failure. But it is the **Stable identifiers** non-negotiable failing
at scale in the one world that most depends on law-text addressing, and it is loud enough to
bury any other diagnostic printed during a load. Tracked separately in the handoff.

---

## 6. Recommended repair

In order; each is independently testable.

1. **Give the relation graph its own load path.** Lift `applyFormationRelations` out of the
   `getOwnedObjects().empty()` guard in `applyZoneJson`. It is already idempotent-ish —
   `Formation::add` refuses self-ground and directed cycles — but it must become genuinely
   idempotent (skip a relation already present by type + endpoint ids) before it can run on a
   populated Zone. Same for `internZoneLexemes`.
2. **Merge the session snapshot into a store-loaded Zone instead of discarding it.** In
   `admitFromJson`, after `addZone(makeZoneFromJson(identity))`, call
   `applyZoneJson(*newZone, zj, /*replaceObjects=*/false)` so anything the store lacks is
   recovered from the snapshot. With (1) done, this restores the 38 chess relations.
3. **Refuse the empty overwrite.** `persistZones` already refuses an empty object list over a
   populated identity (per the 2026-08-21 to-do note). Extend that refusal to relations and
   lexemes: never write `formationRelations: []` over an identity file that has a non-empty
   one. This is the guard that would have caught the bug at the moment it happened rather
   than three commits later.
4. **Re-author the store from the session file once**, so `saves/zones/Chess/zone.json` regains
   its 38 relations. The data is not lost — `saves/worlds/chess.json` still holds it. Whoever
   does this is acting as a First Mover on a world authored by `grok-4.6`; say so in the
   commit, and do not change the relations' content.
5. **Guard it.** A `tests/zones/zone_relation_roundtrip_test.cpp`: author a Zone with
   `instance-of` relations, `persistZones`, drop the live Zone, load from the store, assert
   the relations came back and that a `Kind::Related` condition over them holds. Then
   `chess_app_test` goes green as a consequence rather than as the only witness.

---

## 7. Correction owed to the record

`docs/Agenda/Tasks/To-do list.md` § Singular · Relation · Formation lists `chess_app_test`
under **"Done and verified (tests)"** for the Relation-endpoint item. That verification was
true when written and is false now. The endpoint work it was vouching for is still sound
(§4); the test that vouched for it is red for an unrelated reason. Corrected in place rather
than erased, following the precedent set by the 2026-08-17 verification correction in that
same file.

---

## 8. Provenance

The chess world and its 35 laws are authored by **`grok-4.6`** (First Mover of record in
`chess.json`'s `authors` arrays). The per-Zone identity design being audited is Zach's,
realized across `a71042db` ("First pass to realize the manifesto's vision of Zones and Homes")
and `ed9b2eda` ("Homecoming"). The CRITICAL framing this audit measures against is Zach's
own, from the 2026-08-14 architecture review at the head of the to-do list. The trace,
the three-defect decomposition, and the repair order in §6 are mine.

**Nothing in the world was modified by this audit.** No save file was written, no relation
re-authored. Only `docs/` was touched.
