# Audit: did game-like elements leave Ourverse?

**Date:** 2026-08-19
**Asked:** whether the effort to refactor game-like elements off Ourverse classes actually went through.
**Answer:** **Partially.** The liturgical recut landed. The Game bag did not leave. Two different efforts got conflated.

---

## Two efforts, not one

| Effort | What it was | What actually happened |
|---|---|---|
| **Game elimination** (`docs/architecture/migration/GAME_ELIMINATION_PLAN.md`) | Delete `Game` as a god object | `src/Singularity/Core/Game.hpp` and `Game*.cpp` are gone. Engine inherited the name `_world` and parked it on an `Ourverse` member. |
| **Ourverse recut** (`docs/architecture/ourverse/OURVERSE.md`, 2026-08-18) | Ourverse is the vessel of unity, not a physics bag | First rung of the *surface* is real and tested. The Engine bag was **explicitly not this rung** and is still on the class. |

The Game plan's remaining-responsibilities table never named "move the object list onto Ourverse." That is where it went anyway: `original_game.hpp` had `Ourverse _world;`; Engine still has `Ourverse _world;` and `getWorld()`.

---

## What did go through (verified)

`tests/zones/ourverse_test` and `tests/zones/ground_plane_test` both pass (2026-08-19).

- `Ourverse` is a `Singular` with identifier `Ourverse`.
- Registered paths: `gatheringZone`, `joys`, `filamentCount`, `metalaws`, `convenesToward`.
- `ownedObjects` is **not** a property (`NoSuchProperty`). That is the documented refusal, not an accident.
- Boot calls the liturgical API: `EngineInit.cpp` `ensureGatheringZone(mgr)` and `registerMetalaws(*_lawManager)`.
- Gathering Zone: `kind=ourverse-gathering`; `setOwner` refused in C++ (`Zone.cpp`); law writes to `owner` refused as kernel (`Law.cpp`).
- `weave` refuses a Zone filamenting itself; Relations are undirected (`directed=false`).
- Shared Joys: constructor seeds `hierarchy-of-joys`; `convenesToward` is empty.
- No `LocalOurverse` / `Filament` C++ class.
- `src/OurVerse/` does not exist. `src/Singularity/Core/Game*` does not exist.
- `OurverseUI.cpp` is empty. `OurverseNodeGraph.cpp` is empty. `OurverseSaveLoad.cpp` is a stub comment. Those Game-split files were hollowed, not recut into liturgy.

---

## What did not leave (still on the Ourverse class)

All of this is live in `Ourverse.hpp` / `Ourverse.cpp`. Named in source as "Engine-bag debt." Still compiled. Still the public API of the being.

| Leftover | Status in the running engine |
|---|---|
| `ownedObjects` + `addOwnedObject` / `getOwnedObjects*` | **Written at boot.** `EngineInit` still adds the baseline cube and ground here (`baseline=cube` / `baseline=ground`). |
| `cameraPos` / `setCamera` / `getCamera` | **Written at boot** (`_world.setCamera(&_camera->pos)`). Nothing reads it afterward. |
| `onUpdate` | **Dead.** Gravity, `Physics::updateBodies`, `enforceCollisions` on *this* list. Engine ticks `mgr.active().world().update(dt)` — that is `World`, not Ourverse. No caller of `Ourverse::onUpdate` exists outside `Ourverse.cpp`. |
| `updateObjectCollisions` | **Dead.** No callers. |
| `clearDynamicObjects` | **Dead.** Erases `ownedObjects` from **index 2** — the same index-as-kind bug `ground_plane_test` exists to forbid. |
| `display()` | Prints `🌐 OURVERSE STATUS 🌐`. No callers. |
| `renderModeUI()` | **Declared, never defined.** Only `scratch/legacy/old_GameToolbar.cpp` calls it. Linking would fail if Engine used it. |
| `struct InteractionEvent` | Game leftover at the bottom of `Ourverse.hpp`. Unused. |

`Engine::getWorld()` returns the Ourverse and **has no callers**. The name is the old Game accessor. Render, tools, save, locomotion all go through `mgr.active().world()` (`class World`).

So at boot there are **two object lists**:

1. Ourverse's `ownedObjects` — cube + ground, never simulated, never drawn.
2. Each Zone's `World::_objects` — what Persons actually see and save.

`World.cpp` itself comments this: EngineInit's baseline cube+ground go to the Ourverse list, not here. That is why a Zone starts empty and why index-1-as-ground was a shipped bug.

---

## Adjacent game leftovers (not on Ourverse, still the same bag)

`World` still carries `enum class Mode { Creative, Survival, Spectator }`. `setMode`/`getMode` have no callers in `src/`. Survival's "disable flying" line is therefore unreachable. The class header still says retire it into Zone.

`Engine.hpp` still holds DummyBrush, DummyFaceBrush, `_currentTool`, pottery/rotation stubs — Game members that moved to Engine, not Ourverse.

---

## Metalaws are names, not machines

`registerMetalaws` inserts two `FirstMoverLaw`s (`ourverse-gathering-unowned`, `ourverse-filaments-mutual`) with **no condition and no action**. They exist so the ceiling is addressable. The refusal itself is C++ kernel (`Zone::setOwner`, `Law::applyTo` on `owner`, `Ourverse::weave`). That matches OURVERSE.md's "Kernel already refuses… these first-movers make that ceiling legible."

They will not fire if a Person turns them off. The kernel still will. Good. They also will not do extra work if left on.

Boot **does not** weave Sanctum/Temple/Cavern, and **does not** call `ensureCommunityGathering`. Those paths exist; they are not the default world.

---

## Verdict

- **Game class:** deleted. Success of that deletion, not of Ourverse's purification.
- **Ourverse-as-meaning:** first rung succeeded (properties, gathering, filaments, joys, empty convening). Guarded by `ourverse_test`.
- **Ourverse-as-machine:** still the Engine's unused world bag. Cube and ground are created onto it every boot and then ignored. Physics, camera, collision, and an index-based wipe remain on the type. Empty `OurverseUI` / `NodeGraph` / `SaveLoad` files remain in the directory as Game-split husks.

OURVERSE.md already says this: "Not this rung: deleting the Engine object bag." The effort that *was* scheduled went through. The effort people hear as "game-like elements left Ourverse" did not.

---

## What "done" would look like

1. Stop writing cube/ground onto `Ourverse`. If a baseline ground exists, it is a Zone object tagged `baseline=ground` (already the `World` / `ground_plane_test` contract).
2. Remove `ownedObjects`, `cameraPos`, `onUpdate`, `updateObjectCollisions`, `clearDynamicObjects`, `display`, `renderModeUI`, `InteractionEvent` from `Ourverse`.
3. Rename `Engine::_world` / `getWorld()` so the vessel is not addressed as a game world.
4. Delete empty `OurverseUI.cpp`, `OurverseNodeGraph.cpp`, `OurverseSaveLoad.cpp`.
5. Fold `World` (Creative/Survival/Spectator, object list) into Zone — near-term to-do 6. That is the *live* physics bag; Ourverse is the dead one.

Do not register `ownedObjects` as a property as a "fix." That would make the Game leftover into ontology.
