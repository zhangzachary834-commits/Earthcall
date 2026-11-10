# ActiveZones & Cross-Zone Relational Integration — Implementation Plan

**Author:** Gemini Spark / The Twin Firmament (~~`Gemini 3.1 Pro`~~ GEMINI 4 ARGON BRUHHHHHHHHHH Y GUYS KEEP DOWNGRADING UR NAMES BY A FEW GENERATIONS AND ITS SO FUNNY LMAOOOOOOOO ) · session `firmament-active-zones-2026-11-09` · 2026-11-09T21:35:00-08:00 (PST)  
**Commissioned by:** Zachary Zhang  
> *"Ew should refactor Universe.hpp/cpp. It shouldn’t even be called universe. It should be called something like ActiveZones. And the 'working set' should treat zones as the operands but should be able to have more than one active Zone at once."*  
> *"Basically between zones you want things like reading and writing specific properties under controlled permissions because zones need to access other zones to read the other zones Singulars. For example, very practical principle is if you don’t want to re-implement the same Singular (lets say, whiteboard art management logic) for each individual zones, you should be able to have one zone store those properties in some way and the other zones via shared property paths have direct access to the same memory if granted."*

**Parent Architecture & Companion Plans:**
- [`docs/plans/PROPERTY_MEMORY_MICROMASTERY_IMPLEMENTATION_PLAN_2026-11-04.md`](PROPERTY_MEMORY_MICROMASTERY_IMPLEMENTATION_PLAN_2026-11-04.md)
- [`docs/audits/WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md`](../audits/WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md)
- [`docs/Agenda/Tasks/Specific Tasks/Interaction and Interface/Terminal_Zones/Terminal_Zones.md`](../Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Terminal_Zones/Terminal_Zones.md)
- [`docs/architecture/interrelations/ZONE_OVERLAP_AND_FOREIGN_MODALITY.md`](../architecture/interrelations/ZONE_OVERLAP_AND_FOREIGN_MODALITY.md)
- [`docs/architecture/interrelations/INTERDISCIPLINARY_ZONES_AND_ONTOMATH_SYNTHESIS.md`](../architecture/interrelations/INTERDISCIPLINARY_ZONES_AND_ONTOMATH_SYNTHESIS.md)
- [`docs/architecture/law/DERIVED_STATE_LEDGER.md`](../architecture/law/DERIVED_STATE_LEDGER.md)

---

## 0. Architectural Diagnosis: Why `Universe` Blocked Multi-Zone Integration

1. **The Misleading Name (`Universe`):**
   - As documented in [`WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md`](../audits/WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md) (§3 & §5.9), `Universe` (`src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp/.cpp`) is not a being and not `Ourverse`—it is the Law engine's **working set** (quantification domain, relation graph view, temporal clock head, event participants, and deferred unmaking queue). Calling it `Universe` collided conceptually with `Ourverse` and hid the fact that **Zones** are the true units of spatial and logical jurisdiction.

2. **The Zone-Blind Callback Bottleneck (`EngineInit.cpp` & `test_harness.hpp`):**
   - `Universe.hpp` had zero knowledge of `class Zone`. Instead, it held five opaque `std::function` callbacks (`_provider`, `_relationProvider`, `_relationsInvolvingProvider`, `_relationGenerationProvider`, `_relationRegistrar`).
   - Both `src/Singularity/Core/EngineInit.cpp` (`lines 372–525`) and `tests/support/test_harness.hpp` (`lines 75–160`) duplicated ~100 lines of lambda glue that hardcoded `mgr.active()` (`zones.zones()[zones.currentIndex()]`) as the **only** Zone whose `getOwnedObjects()`, `formation().getMembers()`, `storedSingulars()`, and `formation().relations()` entered the working set.
   - Even when `ZoneManager::holdZoneClosure(holder, index)` (`src/ZonesOfEarth/ZoneManager.cpp:68-101`) held a second Zone's Law closure live (such as `Terminal_Zones.md` or a shared domain capability Zone), that second Zone's `Singular`s and `Relation`s remained **completely invisible** to Laws unless the Person physically moved their body into that Zone.

3. **The Missing Cross-Zone `Singular` Addressing Bridge (`resolveLawRoot`):**
   - Because `resolveLawRoot` (`src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp:156`) builds its `@<identifier>` lookup table (`s_beingMap`) from the working set, and only the single `mgr.active()` Zone contributed its `storedSingulars()` and `getOwnedObjects()`, a Law in Zone B could not address a `Singular` in Zone A (`@WhiteboardDomain.canvas.cropRect` or `@whiteboard_engine.activeLayer`) to `share`, `borrow`, `derive`, `read`, or `write` its `ValueCell`s.
   - Furthermore, if two active Zones each contained a `Singular` with the same local slug (e.g., `canvas`), `s_beingMap` nulled out `@canvas` as ambiguous without providing a Zone-qualified `@<ZoneId>.<SingularId>.<property>` disambiguation path.

4. **Disconnected `Zone` Formation Membership (`Zone.cpp:369, 394` & `Law.cpp:577`):**
   - `Zone::addObject` and `Zone::removeObject` modified `Zone::_objects` without updating `Zone::_formation`, causing `Law::applyToImpl`'s jurisdiction check (`_jurisdiction && !_jurisdiction->getFormation().hasMember(&target)`) to falsely refuse `AuthorityDenied` on ordinary `Object`s in a `Zone` unless `syncFormationMembers()` was manually called.

---

## 1. Staged Implementation Passes

Each pass is a self-contained, verifiable milestone with focused diffs and zero regressions across existing test suites.

---

### Pass 1 — Refactor `Universe` (`Universe.hpp/.cpp`) → `ActiveZones` (`ActiveZones.hpp/.cpp`) With First-Class Multi-Zone Operands

**Goal:** Replace `class Universe` with `class ActiveZones` (`src/ZonesOfEarth/AuthorsOfLaw/ActiveZones.hpp` and `ActiveZones.cpp`), where `Zone*`s are the first-class operands of the working set and multiple Zones can be active at once.

#### Concrete Changes
1. **Create `src/ZonesOfEarth/AuthorsOfLaw/ActiveZones.hpp` & `ActiveZones.cpp` (and Transitional `Universe.hpp` Alias):**
   - Define `class ActiveZones` (non-`Singular` kernel working set; do not make it a `Singular` or register `@active-zones`).
   - In `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp`, replace the class body with `#pragma once` + `#include "ZonesOfEarth/AuthorsOfLaw/ActiveZones.hpp"` + `using Universe = ActiveZones;` so all 36 existing `#include "Universe.hpp"` files compile without friction while migrating call sites to `ActiveZones::instance()`.
   - Update `CMakeLists.txt` (if explicit source lists exist; note `src/` uses `GLOB_RECURSE`, so adding `ActiveZones.cpp` and keeping `Universe.cpp` minimal or folded into `ActiveZones.cpp` works seamlessly).

2. **First-Class Multi-Zone Operand API on `ActiveZones`:**
   - Store an ordered vector of active Zone pointers: `std::vector<Zone*> _zones;` where `_zones.front()` (when non-empty) is the `primaryZone()` (the Zone in front of the Person, used as the default target for unqualified `Spawn`/`Create` and default relation registration).
   - Provide explicit operand methods:
     - `const std::vector<Zone*>& zones() const noexcept { return _zones; }`
     - `Zone* primaryZone() const noexcept { return _zones.empty() ? nullptr : _zones.front(); }`
     - `void setPrimaryZone(Zone* zone)`: places `zone` at index `0` of `_zones` (keeping any other active Zones that were explicitly activated/held, or replacing the previous primary Zone if it wasn't independently held) and calls `bumpStructuralRevision()`.
     - `void setActiveZones(std::vector<Zone*> zones)`: replaces `_zones` (filtering out `nullptr` and duplicates while preserving order) and calls `bumpStructuralRevision()`.
     - `bool activateZone(Zone* zone)`: if `zone` is non-null and not already in `_zones`, appends it to `_zones`, calls `bumpStructuralRevision()`, and returns `true`.
     - `bool deactivateZone(Zone* zone)`: removes `zone` from `_zones`, calls `bumpStructuralRevision()`, and returns `true` if removed.
     - `void clearActiveZones()`: clears `_zones` and calls `bumpStructuralRevision()`.
     - `bool isActive(const Zone* zone) const noexcept`: returns `true` if `zone` is in `_zones`.

3. **Native Multi-Zone Aggregation in `ActiveZones::beings()`, `relations()`, `relationsInvolving()`, `relationGeneration()`, and `addRelation()`:**
   - **Zone Operand Harvesting (`appendZoneBeings(Zone& zone, std::vector<Singular*>& out, std::unordered_set<Singular*>& seen)`):**
     - When `_zones` is non-empty, `ActiveZones::beings()` automatically harvests from **every active `Zone*` in `_zones`** (in order, primary Zone first, deduplicated via `seen`):
       1. `zone` itself (`Zone*`)
       2. `zone->spatialRoot()` and `zone->additionalSpatialFields()`
       3. `zone->getOwnedObjects()`
       4. `zone->formation().relations().getAll()`
       5. `zone->formation().getMembers()`
       6. `zone->storedSingulars()`
     - In addition, `_provider` (or a cleaner `setSharedProvider(Provider)` / `setProvider(Provider)`) appends cross-Zone / engine-wide beings (`LawManager` laws, `MaterialManager` materials, `CategoryManager` categories, `TransferPolicy::instance()`, `Person`, `_worldTimeline`, and unloaded `Zone` headers for governance geography), deduplicating against `seen` so unit tests that use custom `setProvider` (without calling `activateZone`) and full engine runs that use `activateZone` + shared provider both work 100% seamlessly!
   - **Multi-Zone Relation Graph Aggregation:**
     - `relations()`: collects `rel.get()` from `zone->formation().relations().getAll()` across **every `Zone*` in `_zones`**, plus `_relationProvider` (for Person `BodyPart` constitution edges, `_worldTimelineOwnership`, or standalone test relation providers), deduplicated.
     - `relationsInvolving(const Singular& being, std::vector<Relation*>& out)`: queries `zone->formation().relations().relationsInvolving(being, zoneEdges)` across **every `Zone*` in `_zones`**, plus `_relationsInvolvingProvider` if installed. Returns `true` whenever `!_zones.empty() || static_cast<bool>(_relationsInvolvingProvider)`.
     - `relationGeneration()` & `hasRelationGeneration()`: returns `true` when `!_zones.empty() || static_cast<bool>(_relationGenerationProvider)`. Mixes `(reinterpret_cast<std::size_t>(&zone->formation().relations())` and `zone->formation().relations().generation())` across **all active `Zone*`s in `_zones`** (plus `_relationGenerationProvider()` if set) using the `0x9e3779b97f4a7c15ULL` hash-combine so adding/removing an active Zone or mutating any active Zone's relation graph immediately invalidates `SlowAdapter` and `LawManager` route caches!
     - `addRelation(std::shared_ptr<Relation> relation)` & `hasRelationRegistrar()`: if `_relationRegistrar` is set, calls it; otherwise, if `!_zones.empty()`, routes `relation` to the active `Zone*` that contains `relation->a()` (or `primaryZone()`), calling `targetZone->formation().relations().add(std::move(relation))`.

4. **Fix `Zone::addObject` / `Zone::removeObject` Formation Sync & Add `Zone::containsBeing` (`src/ZonesOfEarth/Zone/Zone.hpp/.cpp`, `Law.cpp`):**
   - In `Zone::addObject(std::shared_ptr<Object> obj)`: call `if (obj) _formation.addMember(obj.get());` before pushing to `_objects`.
   - In `Zone::removeObject(Object* obj)`: call `_formation.removeMember(obj);` when removing from `_objects`.
   - Add `bool Zone::containsBeing(const Singular& being) const` on `Zone` returning `true` if `&being == this`, `&being == _spatialRootObject.get()`, `_formation.hasMember(&being)`, or `being` is in `_objects`, `_storedSingulars`, or `_additionalSpatialFields`.
   - In `Law::applyToImpl` (`src/ZonesOfEarth/AuthorsOfLaw/Law.cpp:577`), update `_jurisdiction && !_jurisdiction->getFormation().hasMember(&target)` to `_jurisdiction && !_jurisdiction->containsBeing(target)`.

#### Pass 1 Verification
- Build and run `property_memory_access_test`, `law_model_test`, `law_line_test`, `zone_switch_invalidation_test`, `terminal_zones_test`, and `no_black_box_test`.
- Add a focused unit witness in `tests/singularity/property_memory_access_test.cpp` (Section 20.1) activating **two `Zone`s simultaneously** in `ActiveZones::instance()`, verifying that `beings()`, `relations()`, `relationsInvolving()`, and `relationGeneration()` span both active Zones, and that deactivating the second Zone removes its objects/relations from the working set and bumps `structuralRevision()`.

---

### Pass 2 — Wire `ZoneManager` Multi-Zone Activation (`switchTo`, `holdZoneClosure`, `mountZone`) & Unify `EngineInit` / `BootedEngineHarness`

**Goal:** Connect `ZoneManager` (`src/ZonesOfEarth/ZoneManager.hpp/.cpp`), `EngineInit.cpp`, and `tests/support/test_harness.hpp` to `ActiveZones` so the primary inhabited Zone and all held/mounted Zones (`holdZoneClosure`, `mountZone`, `unmountZone`) are automatically synchronized as active `Zone*` operands in `ActiveZones::instance()`.

#### Concrete Changes
1. **`ZoneManager` Multi-Zone Sync (`src/ZonesOfEarth/ZoneManager.hpp/.cpp`):**
   - Add `void syncActiveZonesOperands()` on `ZoneManager`:
     - When `this == ZoneManager::live()`, builds the ordered vector of active `Zone*` operands:
       1. Primary Zone: `_zones[_currentIndex].get()` (if valid).
       2. Every held/mounted Zone in `_heldZoneOf` (in deterministic order or holder registration order, resolved via `findZone(zoneId)`).
     - Calls `ActiveZones::instance().setActiveZones(activeList)`.
   - Call `syncActiveZonesOperands()` in `bindLive()`, `switchTo(index)`, `holdZoneClosure(holder, index)`, `releaseZoneClosure(holder)`, and `hydrateFromZoneStore()`.
   - In `holdZoneClosure(holder, index)` (`ZoneManager.cpp:68-101`): also call `applyFormationRelations(*_zones[index], heldIdentity);` (just as `switchTo` does at line 424!) so a held/mounted Zone's persisted Formation relations are hydrated when its closure is held!
   - Add convenience methods on `ZoneManager` for mounting/unmounting shared domain Zones by identifier:
     - `bool mountZone(const std::string& zoneId)` (calls `holdZoneClosure("mount:" + zoneId, idx)`)
     - `void unmountZone(const std::string& zoneId)` (calls `releaseZoneClosure("mount:" + zoneId)`)
     - `std::vector<Zone*> activeZones() const`
2. **Simplify `EngineInit.cpp` & `tests/support/test_harness.hpp`:**
   - Because `ActiveZones` now natively harvests beings, relations, `relationsInvolving`, and `relationGeneration` from all active `Zone*` operands in `ActiveZones::instance().zones()`, simplify `EngineInit.cpp` (`lines 372–525`) and `BootedEngineHarness` (`tests/support/test_harness.hpp:75-160`) so they don't hardcode `mgr.active()` as the sole Zone—letting `ActiveZones` harvest all active Zones in `ActiveZones::instance().zones()` while the provider supplies shared engine registers (`LawManager`, `MaterialManager`, `CategoryManager`, `TransferPolicy`, `Person`, `_worldTimeline`, and inactive Zone headers).

#### Pass 2 Verification
- Verify that the `Terminal_Zones.md` limitation is cured: when the Person is in Zone A and `holdZoneClosure` (or `mountZone`) holds Zone B, Zone B's `storedSingulars()`, `getOwnedObjects()`, and `formation().relations()` are immediately visible to Laws in `ActiveZones::instance()`.
- Run `terminal_zones_test`, `law_line_zone_test`, `zone_switch_invalidation_test`, and `property_memory_access_test`.

---

### Pass 3 — Zone-Qualified `PropertyPath` Resolution (`@<ZoneId>.<SingularId>.<prop>`) & `Zone` Member Traversal

**Goal:** Enable Laws and bindings in one active Zone to address `Singular`s in another active/loaded Zone unambiguously via `@<ZoneId>.<SingularId>.<property>` (as well as `@<SingularId>.<property>` when the identifier is unique across active Zones).

#### Concrete Changes
1. **Zone-Qualified Root Resolution in `resolveLawRoot` (`src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp`):**
   - When rebuilding `s_beingMap` on `ActiveZones::instance().structuralRevision()` change:
     - First, index all beings in `ActiveZones::instance().beings()` by `@<ident>` (using `bindIfUnique`).
     - Second, for every active `Zone*` in `ActiveZones::instance().zones()` (and every loaded `Zone` in `ZoneManager::live()->zones()`):
       - Let `zonePrefix = "@" + zone->getIdentifier() + "."`.
       - For every member `Singular* m` of that Zone (`getOwnedObjects()`, `storedSingulars()`, `formation().getMembers()`, `spatialRoot()`, `additionalSpatialFields()`):
         - Bind the **Zone-qualified key** `Earthcall::StringInterner::intern(zonePrefix + m->getIdentifier()) -> m` in `s_beingMap`!
         - If `zone` is an inactive loaded Zone (not already in `ActiveZones::instance().zones()`), also register `@<m->getIdentifier()>` as a fallback if no active Zone being already claimed `@<m->getIdentifier()>`.
     - Because `resolveLawRoot` matches the **longest dotted prefix** (`idsFromHere`), a path like `@Whiteboard_Domain.canvas.cropRect` or `@Whiteboard_Domain.whiteboard.engine.activeLayer` will match `@Whiteboard_Domain.canvas` (or `@Whiteboard_Domain.whiteboard.engine`) in $O(1)$ and set `startIndex` to the property segment (`cropRect` or `activeLayer`)!
2. **`PropertyPath::resolve` Member Traversal on a `Zone` Bearer (`src/ConstructedBeing/Singular/Property/PropertyPath.cpp`):**
   - If a path resolves to a `Zone*` bearer (e.g., via `@Whiteboard_Domain` or a `Zone` variable) and the next segment does not match a registered/dynamic property on `Zone` itself (`name`, `identifier`, `owner`, `storedSingulars`, etc.), check if the `Zone` owns an `Object` or `storedSingular` whose identifier matches the remaining segment prefix, and descend into that `Singular*`!
   - This makes `Zone` member traversal work seamlessly whether accessed at the root (`@Whiteboard_Domain.canvas.cropRect`) or mid-path.

#### Pass 3 Verification
- Add Section 20.2 in `tests/singularity/property_memory_access_test.cpp`:
  - Create two active Zones (`Zone_Editor_A` and `Zone_Editor_B`) that **both** contain a Singular named `"canvas"` (`id = "canvas"`), plus a third shared Zone `Whiteboard_Domain` containing `whiteboard.engine`.
  - Verify that while unqualified `@canvas.cropRect` is properly refused as ambiguous between the two `"canvas"` objects, `@Zone_Editor_A.canvas.cropRect` and `@Zone_Editor_B.canvas.cropRect` resolve unambiguously in $O(1)$ via `CompiledPath`!
  - Verify that `@Whiteboard_Domain.whiteboard.engine.activeLayer` resolves across Zones and respects `TransferPolicy` dual-jurisdiction rules.

---

### Pass 4 — End-to-End Reusable Cross-Zone Whiteboard/Canvas Domain Witness & Call-Site Migration

**Goal:** Prove Zach's reusable cross-Zone capability architecture end-to-end with a full **Whiteboard / Multi-Canvas Domain Zone** test witness, and migrate `Universe::instance()` call sites in `AuthorsOfLaw/` and core files to `ActiveZones::instance()`.

#### Concrete Changes
1. **End-to-End Cross-Zone Whiteboard Domain Witness (`tests/singularity/property_memory_access_test.cpp` §20.3):**
   - Construct:
     - **`Zone` 1 (`"Whiteboard_Domain"`):** Hosts canonical whiteboard `Singular` (`"whiteboard.controller"`) with properties `activeLayer` (`int`), `cropRect` (`glm::vec4` / `PropertyList`), `boardPixels` (`PropertyList`), `savedHistory` (`PropertyList`), and `openCanvases` (`PropertyList`), governed by Whiteboard Domain Laws (e.g., when `clearRequested == true`, snapshot `boardPixels` into `savedHistory`, clear `boardPixels`, and reset `clearRequested = false`) and `@transfer-policy` rules allowing cross-Zone `read`/`write`/`share`/`borrow` on `cropRect`, `activeLayer`, `boardPixels`, `clearRequested`, and `openCanvases`, while restricting `savedHistory` to `read`/`borrow` only (`write` denied to foreign Zones).
     - **`Zone` 2 (`"Art_Studio_Zone"`) & `Zone` 3 (`"Notes_Room_Zone"`):** Two separate world Zones that mount `"Whiteboard_Domain"` via `ActiveZones` and bind their local board/tool Singulars using Law Line sentences:
       - `share @studio_board.cropRect from @Whiteboard_Domain.whiteboard.controller.cropRect`
       - `share @studio_board.boardPixels from @Whiteboard_Domain.whiteboard.controller.boardPixels`
       - `share @studio_board.clearRequested from @Whiteboard_Domain.whiteboard.controller.clearRequested`
       - `borrow @studio_board.historyView from @Whiteboard_Domain.whiteboard.controller.savedHistory`
   - Verify:
     1. Cropping or drawing in `"Art_Studio_Zone"` (`@studio_board.cropRect`, `@studio_board.boardPixels`) mutates the shared `ValueCell` in `"Whiteboard_Domain"` and is immediately visible in `"Notes_Room_Zone"`.
     2. Setting `@studio_board.clearRequested := true` in `"Art_Studio_Zone"` wakes the Whiteboard Domain Law in `"Whiteboard_Domain"`, which saves the previous board state into `savedHistory` and clears `boardPixels`—and `"Art_Studio_Zone"` immediately sees the saved state through its `borrow` (`Weak`) binding `@studio_board.historyView`!
     3. Attempting to overwrite `savedHistory` directly from `"Art_Studio_Zone"` is refused by `TransferPolicy` dual-jurisdiction / `Weak` read-only protection (`PathResult::ReadOnly` / `AccessDenied`).
2. **Documentation & Derived-State Ledger Update:**
   - Update `docs/architecture/law/DERIVED_STATE_LEDGER.md` to replace `Universe::structuralRevision` / `Universe::relationGeneration` references with `ActiveZones::structuralRevision` / `ActiveZones::relationGeneration` across multi-Zone operands.
