# Audit: World and Universe against the six refusals

**Date:** 2026-08-20
**Asked by Zach:** whether `World` and `Universe` were already retired; if not, whether they violate the six refusals.
**Answer:** **Neither is retired.** `World` violates refusals 1 and 6, and has already committed refusal 3's append-only slot. `Universe` does not violate as a kind of being — it is law-kernel mechanism wearing a name that collides with the vessel. The wrong fix for either is to register their leftovers as properties.

This audit is the map. **Postscript (same day):** the fold landed — `class World` is gone, `BeingKind::World = 6` is burned, objects live on `Zone`, save JSON still dual-reads `zones[].world.objects`, `LawContext.hpp` is deleted, Universe remains a non-being. The body of this audit describes the tree *before* that change.

**Companions:** `ontology/NEW_KIND_FRAMEWORK.md` Floor §1–§4, `ontology/NO_BLACK_BOX.md` §2b / §4 / sealed register, `ontology/AUTHORED_CATEGORIES.md` §1, `law/LAW_AND_CREATION_SYSTEM.md` §7c, `ourverse/OURVERSE.md`, `migration/GAME_ELIMINATION_PLAN.md`, [OURVERSE_GAME_ELIMINATION_AUDIT_2026-08-19.md](OURVERSE_GAME_ELIMINATION_AUDIT_2026-08-19.md). Agenda: near-term 6, property-path debt 1.

**Method:** read of the live headers, the Spawn/reaper paths, Zone formation membership, save JSON, Universe provider wiring, and the tests that construct a bare `World`. Doctrine read against those sites. **No build or in-app click this session.** Call-site claims below are from source, not from a run.

---

## 0. What Zach asked, and what this extends

Zach asked two questions, in order: *did we already retire them*, and *if not, do they violate*. The retirement of `World` was already named — the class header, near-term 6, `NO_BLACK_BOX.md`, the 2026-08-19 Ourverse audit § "Adjacent game leftovers", and the walk through the code. Those sources say *fold World into Zone; do not populate World properties*. They do not walk each refusal, they do not settle `Universe`, and they do not name the append-only trap that makes a naive delete of `class World` a serialization schism.

This audit answers Zach's two questions from the live tree, then names the three offices that currently share "the whole," so the implementation does not pick the wrong one to unseal.

Origination, named:

| Claim | From |
|---|---|
| Retire World into Zone; Worlds are Zones with spatial properties and objects | `World.hpp` comment (already in-tree) |
| Do not populate World properties — fold, then expose Ourverse | near-term 6, Zach's agenda |
| `_playerEyeHeight` is state, not Kernel, and `buildProperties()` is `{}` | `NO_BLACK_BOX.md` §2b, `no_black_box_test` sealed register |
| Two object lists at boot (Ourverse bag vs Zone's World) | 2026-08-19 Ourverse audit |
| The six refusals as the test | Zach, this prompt |
| Three offices for "the whole"; `BeingKind::World = 6` cannot be deleted; `Universe` is kernel not a being; `LawContext.hpp` is a duplicate; Mode / eyeHeight / camera on World are dead writes | this audit |

---

## 1. Status: not retired

### World

`src/ZonesOfEarth/World/World.hpp` still declares `class World : public Singular`. The retirement sentence is a comment above the class, not a deletion. Each `Zone` still constructs `std::unique_ptr<World>` and admits that World into its Formation (`Zone.cpp` constructor and `syncFormationMembers`).

The live object list Persons see, save, spawn into, and destroy from is `World::_objects`. Engine ticks `mgr.active().world().update(dt)`. `Spawn` in `ActionModel.cpp` resolves a `World*` as the womb (`resolveWorld`, and the same `dynamic_cast<World*>` inline in `Kind::Spawn`). `reapUnmadeBeings` walks `Universe::beings()` for `World*` and calls `World::removeObject`.

`ConditionNode::BeingKind::World = 6` is still matched by `dynamic_cast<const World*>`. The Law Graph picker still offers the string `"World"` (`LawGraphWindow.cpp`). Concept instantiation still refuses `"a World is not instantiated"`.

Tests that construct a bare `World` (not a Zone) still exist: `action_spawn_test`, `basic_cube_law_test`, `shape_generator_law_test`, `bezier_patch_law_test`, `law_creation_test`, `law_audit_test`, `singular_set_to_set_test`, `object_concept_test`, `ground_plane_test`, `no_black_box_test`.

Save format still has a World-shaped key. `ZoneManager::buildSaveJson` writes `zj["world"] = z->world()`, and `Serialization.cpp` writes `{ "objects": [...] }`. That key is load-bearing for existing Person saves. Folding the class must dual-read it. It must not invent a new ontology for the file.

### Universe

`src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp` is a process-wide singleton, not a `Singular`. There is no retirement note, no agenda item, no `BeingKind::Universe`. `EngineInit.cpp` still installs the provider, the relation provider, and the relation registrar. `Law.cpp` still sweeps `Universe::instance().beings()`, arms `OnsetScope` / `EventScope`, and reaps through `requestUnmaking`.

`LawContext.hpp` is a byte-identical copy of `Universe.hpp` (both 274 lines, both dated 2026-08-12). Nothing in `src/` includes it. An older Zone/Ourverse audit mentions the filename as if it were a law-jurisdiction type. It is not. It is a stray duplicate.

### The three offices

| Office | What it is in the running engine | Retired? |
|---|---|---|
| `World` | Zone's object list + leftover `Mode` / camera / eyeHeight | **No.** Marked. Live. |
| `Ourverse` | vessel of unity (registered) **and** Engine's unused `ownedObjects` bag | Liturgy unsealed 2026-08-18. Game bag not retired. See the 2026-08-19 audit. |
| `Universe` | law's working set: quantification domain, clock, unmaking, event participants, relation graph | **No**, and it should not be. It is not a being. |

`Engine::getWorld()` returns the `Ourverse` and has no callers in `src/`. The name is the old Game accessor. Render, tools, save, locomotion, audio, and spawn all go through `mgr.active().world()` — the `World` class.

Doctrine already picked the mapping: Ourverse is the vessel; Zone is the spatial and jurisdictional container; World disappears into Zone; Universe stays as kernel under a name that does not pretend to be a being.

---

## 2. World against the six refusals

### Refusal 1 — no C++ class for a domain noun

**Violates.** Kind Floor §1 still lists `World` next to `Person`, `Zone`, `Law` as an ontological category. The class contradicts its own listing: a World is a Zone's object vector plus `enum class Mode { Creative, Survival, Spectator }`. Those modes are game nouns. Admission test Q1 for a *new* World class would be NO (no hardware channel) → author it, write no C++.

The header's retirement sentence is the correction. Leaving `World` on the Kind Floor while the header says it is not a kind is two offices for one fact.

`drawGround()` is a hardcoded green quad. `World::update` still contains `if (mode == Mode::Survival && Physics::getFlying()) Physics::setFlying(false)`. `setMode` / `getMode` have **no callers in `src/`**. `load()` forces `Mode::Creative`. Survival is unreachable. That is Game leftover occupying a `Singular`.

### Refusal 2 — no new top-level directory

**Does not violate.** `src/ZonesOfEarth/World/` is the right region. After the fold the directory should leave; until then placement is correct.

### Refusal 3 — no new enum value for a kind of thing

**Already committed, and that is the trap.** `BeingKind::World = 6` is append-only, serialized as an integer (`FIRST_MOVER_AUTHORING.md` §5b). Deleting the class cannot delete 6. Aliasing 6 onto `Zone` would make two enum values mean one kind — a schism with a version number, the thing Floor §3 exists to forbid. The honest move is the same one already used for `ConditionNode::Kind` 12 and 13: **burn 6**. `matchesKind(World)` answers false, the picker stops offering it, the integer is never reused.

`World::Mode` is a second enum of kinds of play. It has no callers that set it. Delete it with the class; do not migrate it into Zone and do not author it as a category. It is Game.

`Zone::Scope::World` is a different word: jurisdiction scale (`Global / World / Regional / Local / UI`). It is not the class. Leave it.

### Refusal 4 — Body is for Persons / Refusal 5 — Person means Human

**Does not violate.** World claims neither Body nor Person.

### Refusal 6 — no black box

**This is the live violation.** `World::buildProperties()` is `{}` with no sealed-register comment on the method. `no_black_box_test` holds the reason on `kSealedRegister`:

> `_playerEyeHeight` and `Mode` are a Person's viewpoint and their world's posture — state, not mechanism. The header itself says to retire World into Zone; until then this is unregistered debt, not an exemption.

Fields today:

| Field | Could a Person mean something by it? | Registered? | Live? |
|---|---|---|---|
| `_objects` | yes — the beings in the Zone | no | **yes** — save, render, spawn, destroy, locomotion, audio, interaction |
| `_playerEyeHeight` | yes — `NO_BLACK_BOX.md` names it | no | **written every frame** (`EngineRender.cpp`) and **never read** (`World::update` no longer simulates the player) |
| `_cameraPos` | the camera is the Person's | no | written every frame, never read on World |
| `mode` | a world's posture, if it were real | no | written to Creative on load; Survival branch dead |

`getIdentifier()` returns the literal `"World"` for every instance. Law text cannot tell two Zones' worlds apart. The Universe provider only pushes the *active* Zone's World, so `@World` happens to mean "the bag in front of you." That accident is not a stable slug. Zone already answers with its name (`"Sanctum of Beginnings"`). That is the address Spawn should use.

Populating `World` properties to clear the sealed register would make the Game leftover into ontology. Near-term 6 already forbids that. The 2026-08-19 audit forbids the same move on `Ourverse::ownedObjects`. Same rule, same being-shaped bag.

---

## 3. Universe against the six refusals

### Refusal 1 — no C++ class for a domain noun

**Name collides; the class does not claim a kind.** Universe is not a `Singular`. It does not inherit, does not register, does not serialize as a being. Floor §4's smell is real — a singleton that "tracks all instances" looks like a `*Manager` for the domain *everything* — but this one does not own beings and does not tick them. `LawManager` ticks. `Zone` owns. Universe answers "what exists for this sweep?" and holds the clock / unmaking queue / event participants that a sweep cannot live without.

That function is kernel. The admission test does not apply: this is not a proposal for a kind of thing. Calling it Universe is what collides with World and Ourverse as "the whole."

Do not make it a `Singular` to "fix" refusal 6. That would be Game-elimination run backwards: promoting a working set into the ontology, the failure `REFUSALS_AUDIT_2026-08-11.md` §0 named for Ourverse.

### Refusal 2 — no new top-level directory

**Does not violate.** It lives in `AuthorsOfLaw/`, which is where Law's kernel belongs.

### Refusal 3 — no new enum value

**Does not violate.** There is no `BeingKind::Universe`. Keep it that way.

### Refusal 4 / 5

**Does not violate.**

### Refusal 6 — no black box

**Not in the same way.** A non-being has no `buildProperties()` to seal. The clock is already reserved paths (`time` / `time.delta` / `time.sinceApplied` via `MathBinding.hpp`). The beings *are* the beings. Unmaking, event scopes, and providers are law-kernel mechanism: a Person cannot mean something by the raw pointer queue that keeps `Destroy` from freeing mid-`applyTo`.

The remaining gap is addressability of the *domain*, not hidden state. Laws range over Universe without naming it. That is acceptable for kernel. If a Person needs to ask "who is in the sweep," they already have `ForAny` / `ForAll` over Zone, Object, Law. Adding `@universe` would mint a being for a working set. Refuse that unless Zach authors it.

`LawContext.hpp` as a silent twin is housekeeping, not a black box. Delete the copy. Do not invent a jurisdiction type in its place under that filename — Zone jurisdiction is architectural actualization 11, a different task.

---

## 4. What must not be done

These are the moves this audit exists to prevent. Each has a precedent in this tree.

1. **Do not register properties on `World`.** That takes it off `kSealedRegister` by crystallizing the bag. Near-term 6; `NO_BLACK_BOX.md` sealed register is a debt ledger, not an allowlist that you fill.
2. **Do not register `Ourverse::ownedObjects` as the "then unseal Ourverse" half of near-term 6.** Liturgy is already unsealed. The bag is Engine leftover. 2026-08-19 audit: "Do not register `ownedObjects` as a property as a fix."
3. **Do not alias `BeingKind::World` to `Zone`.** Two integers, one kind. Burn 6.
4. **Do not make `Universe` a `Singular`.** Kernel is not a being.
5. **Do not put object ownership on Ourverse.** Ourverse is the vessel of unity, not the womb. Spawn's target is the Zone that receives the newborn.
6. **Do not change the save key without dual-read.** Zach's standing critical: the save system must not erase developer worlds. `zones[].world.objects` is what existing `.json` / `.ecsave` files carry.
7. **Do not migrate `Mode { Creative, Survival, Spectator }` into Zone or into an authored category.** Delete it. It has no live callers.

---

## 5. What "retired" would look like (exit tests)

World is retired when all of these are true. Universe is settled when the last two are true.

1. `class World` is gone. `src/ZonesOfEarth/World/` is gone. Reconfigure required (sources are globbed).
2. `Zone` owns the object vector. `addObject` / `removeObject` / `update` live on Zone (or on a non-being helper that is not a `Singular`). Formation membership is the objects and the spatial root — not a nested World being.
3. `Spawn` and `reapUnmadeBeings` take the Zone as womb. `resolveWorld` is `resolveZone` (or the target is the Zone, falling back to `mgr.active()`).
4. Save still loads `zones[].world.objects` from existing files. New writes may keep that key (the file is not the ontology) or write `objects` beside it and dual-read both.
5. `BeingKind::World = 6` is burned: `matchesKind` is false, the picker does not offer it, the integer is never reused. `BeingKind::Zone` remains 7.
6. `no_black_box_test` no longer constructs a `World` and no longer lists it on `kSealedRegister`. Zone's object list, if a Person can mean something by it, is registered on **Zone** (read-only identifiers is enough; the objects are themselves addressable).
7. Universe provider no longer pushes a World being. It already pushes every Zone. The active Zone is the womb.
8. `LawContext.hpp` is deleted.
9. Universe remains a non-`Singular` singleton in `AuthorsOfLaw/`. Rename is Zach's, not an agent's. Default: keep the name, document it as the law working set, stop calling it a container of being.

A probe or test that constructs `World world;` and `executor(event, world)` must become a Zone (identifier stable, objects on the Zone). `ground_plane_test` already speaks the Zone contract (`baseline=ground`); it should stop going through a detached World.

---

## 6. Call-site ledger (live `src/`, not scratch)

These are the files an implementation has to touch. Scratch/legacy still names World; it is not the running engine.

**Ownership and tick**

- `ZonesOfEarth/Zone/Zone.hpp` / `Zone.cpp` — `unique_ptr<World>`, Formation admits World, `load`/`unload` forward
- `ZonesOfEarth/World/World.hpp` / `World.cpp` — the class
- `Singularity/Core/EngineUpdate.cpp` — `world().update(dt)`, `world().removeObject`
- `Singularity/Core/EngineRender.cpp` — `setCamera` / `setPlayerEyeHeight` (dead writes), then iterates `getOwnedObjects`
- `Singularity/Core/EngineInit.cpp` — Universe provider pushes `&mgr.active().world()` then its objects
- `Singularity/Core/Engine.cpp` — `renderCreationWindow(..., mgr.active().world())`

**Law**

- `AuthorsOfLaw/ActionModel.cpp` — `resolveWorld`, `Kind::Spawn`
- `AuthorsOfLaw/Law.cpp` — `reapUnmadeBeings`
- `AuthorsOfLaw/ConditionModel.cpp` — `BeingKind::World`
- `ConstructedBeing/Object/Creation/ObjectConcept.cpp` — birth refusal for World
- `Singularity/Screen/LawGraphWindow.cpp` / `MathEditors.cpp` — picker label `"World"`

**Save**

- `ZonesOfEarth/ZoneManager.cpp` — `zj["world"] = z->world()`, load `from_json(..., z->world())`
- `Singularity/Storage/Serialization.hpp` / `.cpp` — `to_json` / `from_json` for `World`

**Readers of the object list** (become Zone methods, same names are fine)

- `Singularity/Input/Locomotion/LocomotionChannel.cpp`
- `Singularity/Input/Interaction/InteractionChannel.cpp`
- `Singularity/Audio/AudioSystem.cpp`
- `Singularity/FirstMoverOntology/FirstMoverWindowTools/{Tool,CreationTools,CursorTools,CreatorConsole/Create3DConsole}.cpp`

**Universe (keep; delete the twin)**

- `AuthorsOfLaw/Universe.hpp` — the singleton
- `AuthorsOfLaw/LawContext.hpp` — identical unused copy
- `EngineInit.cpp` — provider / relation provider / registrar
- `ZoneManager.cpp` — `Universe::instance().setClock` on load
- tests and probes that `setProvider` for quantification

---

## 7. Doctrine that goes stale the day the fold lands

Update these in the same change, or the router will keep listing World as a kind:

- `NEW_KIND_FRAMEWORK.md` Floor §1 and §3 — drop World from the closed list of subclasses; keep 6 burned in the BeingKind paragraph
- `AUTHORED_CATEGORIES.md` §1 table — same
- `FIRST_MOVER_AUTHORING.md` §5b — `6 World` marked burned
- `LAW_AND_CREATION_SYSTEM.md` §7c — Spawn's target is the Zone
- `NO_BLACK_BOX.md` §2b example and §7 sealed-register paragraph (still names Ourverse / Formation / Soul as sealed as of 2026-08-13; those three already left)
- `DIRECTORY_ORDERING.md` / `AGENTS.md` tree — `ZonesOfEarth/` no longer lists World as a being
- `LawGraphWindow.cpp` BeingKind labels

`Zone::Scope::World` and save-file names (`my_world.ecsave`) are not this class. Do not rename either as part of the fold.

---

## 8. Verdict

| Question | World | Universe |
|---|---|---|
| Retired? | No | No (and should not be, as a being) |
| Refusal 1 | **Yes** — bag + game modes as a `Singular` | Name only |
| Refusal 2 | No | No |
| Refusal 3 | Slot 6 committed; `Mode` is a game enum | No |
| Refusal 4 / 5 | No | No |
| Refusal 6 | **Yes** — empty vocabulary, live object list hidden | Kernel, not a being |
| Right next move | Fold into Zone; burn 6; dual-read saves | Delete `LawContext.hpp`; do not mint a being |

Near-term 6's "then unseal Ourverse" half is already done for the vessel. The remaining Ourverse work is deleting the Engine bag, not registering it. That is the 2026-08-19 audit, not this one. Do not couple the two in one PR beyond "stop writing cube+ground onto Ourverse" if a baseline ground is being moved onto the Zone where `ground_plane_test` already looks for it.

Zach's save-system critical governs the fold: a World retirement that cannot reload `my_world` is not a retirement.
