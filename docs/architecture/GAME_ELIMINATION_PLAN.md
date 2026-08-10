# Game Elimination Plan

**How to eliminate the Game God object and distribute its concerns ontologically.**

**Status:** Rungs 0-3 and 5 complete. Rung 4 partial — settings structs relocated, authored Tool beings not started (see §8). Execution ongoing.
**Author:** Mistral Vibe + Zachary Zhang
**Target:** Delete `src/Singularity/Core/Game.hpp` and all `Game*.cpp` files
**Last Updated:** 2026-08-10

---

## 0. Why Game Must Die

`Game.hpp` (26KB header + ~180KB implementation across 8 files) is a **category error**.
It treats "Game" as a C++ class that owns:
- Camera state (Screen modality)
- Player movement (Person/Body + Physics)
- Input handling (Keyboard/Mouse modalities)
- Tool/brush settings (Form/Material + authored state)
- Selection state (Formation relations)
- Save/load (Zone persistence)
- UI toggles (OurVerse authorship surface)
- Node graph visualization (Law structure)

Per `NEW_KIND_FRAMEWORK.md` Refusal 1: **"No new C++ class for a domain noun."**
"Game" is a domain noun. It should be authored in-world as a Formation of beings,
not carved into the type system as a class that owns everything.

Per `DIRECTORY_ORDERING.md` §1: **"a directory earns its place at the top level by naming
a kind of being or a mode of the machine — never by naming a tool, a language, a
process, or a team."** The Game class violates this by being a dumping ground for
all subsystems.

---

## 1. The Migration Ladder (6 Rungs)

Do not attempt to delete Game in one commit. Use this incremental ladder:

| Rung | Action | Exit Test | Files Affected |
|------|--------|-----------|----------------|
| 0 | **Freeze** — No new code in Game; all new features go to their ontological home | No new members added to Game class | All Game*.cpp files |
| 1 | **Extract Input** — Move KeyboardHandler, MouseHandler to `Singularity/Input/` | Game no longer has `_keyboardHandler`, `_mouseHandler` | Game.hpp, GameInit.cpp, GameUpdate.cpp |
| 2 | **Extract Camera** — Move CameraState to `Singularity/Screen/Camera` | Game no longer has `_camera` | Game.hpp, GameInit.cpp, GameRender.cpp, GameUpdate.cpp |
| 3 | **Migrate Player State** — Move movement to Person/Body with Physics Laws | `_playerVelY`, `_playerGrounded`, `_jumpKeyDownLast`, `_playerWasMoving` gone from Game; stepMovement moved to Person | Game.hpp, GameUpdate.cpp, Person.hpp, Person.cpp |
| 4 | **Extract Tool State** — Brush, Polyhedron, Clone, Pottery settings to Form/Material | Game no longer has `_brush`, `_polyhedron`, `_clone`, `_pottery` | Game.hpp, GameToolbar.cpp, GameUpdate.cpp |
| 5 | **Extract Save/Load** — Move to ZoneManager persistence | Game no longer has `saveState`, `loadState` | Game.hpp, GameSaveLoad.cpp, GameInit.cpp |
| 6 | **Delete Game** — Remove all Game files | Game directory gone | All Game*.cpp, Game.hpp |

**Rule:** Never proceed to rung N+1 until rung N's exit test passes and the build is green.

---

## 2. Detailed Rung Specifications

### Rung 0: Freeze (Immediate)

**Action:** 
- Add `// FROZEN: No new code. See GAME_ELIMINATION_PLAN.md` at top of Game.hpp
- Reject any PR that adds to Game
- Route all new features to their ontological home

**Exit Test:**
- Header comment present
- No new members in Game class since this plan was created

---

### Rung 1: Extract Input Handlers

**Current State:** KeyboardHandler and MouseHandler are already separate classes
but Game owns instances of them and they have back-pointers to Game.

**Target State:**
- `Singularity/Input/KeyboardHandler.hpp/cpp` — standalone, no Game dependency
- `Singularity/Input/MouseHandler.hpp/cpp` — standalone, no Game dependency  
- Input state accessed through EventBus, not direct Game members

**Migration Steps:**
1. Move `KeyboardHandler` and `MouseHandler` from `Perspective/` to `Singularity/Input/`
2. Remove `Game*` back-pointers from both handlers
3. Replace Game callbacks with EventBus subscriptions
4. Update Game to use handlers through EventBus instead of direct members
5. Delete `_keyboardHandler`, `_mouseHandler` from Game

**Files to Create:**
- `src/Singularity/Input/KeyboardHandler.hpp`
- `src/Singularity/Input/KeyboardHandler.cpp`
- `src/Singularity/Input/MouseHandler.hpp`
- `src/Singularity/Input/MouseHandler.cpp`

**Files to Modify:**
- `src/Perspective/KeyboardHandler.hpp/cpp` — redirect to new location or delete
- `src/Perspective/MouseHandler.hpp/cpp` — redirect to new location or delete
- `src/Singularity/Core/Game.hpp` — remove handler members
- `src/Singularity/Core/GameInit.cpp` — remove handler initialization
- `src/Singularity/Core/GameUpdate.cpp` — use EventBus for input

**Exit Test:**
- `Game.hpp` has no `_keyboardHandler` or `_mouseHandler` members
- All input flows through EventBus
- Build green, all tests pass

---

### Rung 2: Extract Camera State

**Current State:** Game owns `CameraState _camera` with pos, front, up, viewport,
modelview, projection matrices.

**Target State:**
- `Singularity/Screen/Camera.hpp/cpp` — the Screen modality's camera channel
- Camera state is a being that can be addressed by laws (@camera.pos, @camera.front)

**Migration Steps:**
1. Create `src/Singularity/Screen/Camera.hpp` with Camera class
2. Move camera matrices and view state from Game to Camera
3. Register Camera as a Singular being in Universe provider
4. Update all Game code that reads/writes camera to go through Camera singleton or instance
5. Remove `_camera` from Game

**Files to Create:**
- `src/Singularity/Screen/Camera.hpp`
- `src/Singularity/Screen/Camera.cpp`

**Files to Modify:**
- `src/Singularity/Core/Game.hpp` — remove CameraState include and _camera member
- `src/Singularity/Core/GameInit.cpp` — initialize Camera separately
- `src/Singularity/Core/GameRender.cpp` — use Camera instance
- `src/Singularity/Core/GameUpdate.cpp` — use Camera instance
- `src/Singularity/Core/CameraState.hpp` — may delete or move to Screen/

**Exit Test:**
- `Game.hpp` has no `_camera` member
- Camera is a first-class being addressable by laws
- Build green

---

### Rung 3: Migrate Player Movement State

**Current State:** Game owns:
- `_playerVelY` (float) — vertical velocity
- `_playerGrounded` (bool) — resting on ground
- `_jumpKeyDownLast` (bool) — edge-trigger for jump
- `_playerWasMoving` (bool) — previous frame locomotion state
- `stepMovement(float dt)` — movement integration

**Target State:**
- Player movement is handled by `Person::Body` with Physics Laws
- Velocity, grounded state are properties on the Person/Body
- Movement integration is a Law (OnEvent or WhileTrue)

**Migration Steps (per LAW_MIGRATION_FRAMEWORK.md):**

1. **Rung 1 (Shadow):** Add corresponding properties to Person/Body
   - `Body::velocity` (glm::vec3) — replaces `_playerVelY` (Y component)
   - `Body::grounded` (bool) — replaces `_playerGrounded`
   - `Body::wasMoving` (bool) — replaces `_playerWasMoving`

2. **Rung 2 (Echo):** Game writes to both old and new locations
   - In `stepMovement()`, also update `_player.getBody().velocity`
   - Sync `_playerVelY` from `Body::velocity.y`

3. **Rung 3 (Redirect):** Game reads from new location, writes to both
   - Read velocity from `Body::velocity`
   - Still write to old Game members (for other code that reads them)

4. **Rung 4 (Sole):** Game reads from new location only
   - Remove reads from Game's own members
   - All reads go through Person/Body

5. **Rung 5 (Purge):** Remove old Game members entirely
   - Delete `_playerVelY`, `_playerGrounded`, `_jumpKeyDownLast`, `_playerWasMoving`
   - Move `stepMovement()` logic to a Physics Law

**Files to Create:**
- Laws for player movement (gravity, jump, ground detection)

**Files to Modify:**
- `src/Person/Body/Body.hpp` — add velocity, grounded properties
- `src/Person/Body/Body.cpp` — movement integration
- `src/Singularity/Core/Game.hpp` — remove player movement members
- `src/Singularity/Core/GameUpdate.cpp` — move stepMovement to Body or Law

**Exit Test:**
- `Game.hpp` has no player movement state members
- Player movement controlled by Physics Laws
- Build green

---

### Rung 4: Extract Tool State

**Current State:** Game owns numerous tool-related structs:
- `BrushSettings _brush` — size, scale, rotation, opacity, flow, spacing, pressure, etc.
- `PolyhedronSettings _polyhedron` — primitive, shapeKind, custom vertices/faces, etc.
- `FaceBrushSettings _faceBrush` — radius, softness, offsets, axes, inversion
- `AdvancedFacePaintState _advancedFacePaint` — gradient, smudge settings
- `PlacementState _placement` — mode, manual offset/anchor
- `PotterySettings _pottery` — currentTool, strength
- `RotationSettings _rotation` — axisMode, sensitivity, smoothness
- `CloneToolState _clone` — active, sourceUV, offset
- `StrokeTracking _strokeTracking` — lastBrushTime, lastBrushUV, lastBrushFace, lastBrushObject

**Target State:**
- Tool settings are properties on Tool Objects (authored beings)
- Current tool is a reference to an active Tool being
- Tool state changes publish events that Laws can react to

**Migration Strategy:**
1. Create Tool as a first-class being type (subclass of Object or new Form)
2. Each tool instance (Brush, Polyhedron creator, etc.) is an authored Tool
3. Tool settings become properties on Tool beings
4. Game holds a pointer/reference to the current Tool, not the settings struct
5. Gradually migrate each tool type

**Order of Migration (easiest first):**
1. CloneTool — simplest state
2. Pottery — few settings
3. Rotation — few settings
4. Placement — has anchor state
5. FaceBrush — moderate complexity
6. Brush — most complex, many settings
7. Polyhedron — very complex, custom geometry
8. AdvancedFacePaint — complex nested state

**Files to Create:**
- `src/Form/Object/Tool/Tool.hpp` — base Tool class
- `src/Form/Object/Tool/BrushTool.hpp`
- `src/Form/Object/Tool/PolyhedronTool.hpp`
- etc.

**Files to Modify:**
- `src/Singularity/Core/Game.hpp` — remove tool state structs
- `src/Singularity/Core/GameToolbar.cpp` — use Tool beings
- `src/Singularity/Core/GameUpdate.cpp` — use Tool beings

**Exit Test:**
- `Game.hpp` has no tool state struct members
- Tools are authored beings with properties
- Build green

---

### Rung 5: Extract Save/Load

**Current State:** Game owns:
- `SaveLoadState _saveLoad` — UI state for save/load dialogs
- `saveState()`, `loadState()`, `saveStateWithLog()`, `updateSaveFiles()`
- `buildSaveJson()`, `loadSaveChunkFlatBuffer()`, `buildSaveChunkFlatBuffer()`
- `ensureHomeZone()`
- `_saveDirectory` string

**Target State:**
- Save/load is ZoneManager responsibility
- Each Zone serializes itself
- Global state (camera, player, materials) saved separately through EventBus
- Save UI is part of OurVerse authorship surface

**Migration Steps:**
1. Move `ensureHomeZone()` to ZoneManager (it already has access to mgr)
2. Move save/load serialization to ZoneManager
3. Move SaveLoadState UI to OurVerse
4. Remove save/load methods from Game
5. Route save/load through EventBus events

**Files to Modify:**
- `src/ZonesOfEarth/ZoneManager.hpp/cpp` — add save/load methods
- `src/OurVerse/OurVerse.hpp/cpp` — add save/load UI
- `src/Singularity/Core/Game.hpp` — remove save/load members
- `src/Singularity/Core/GameSaveLoad.cpp` — move code to ZoneManager

**Exit Test:**
- `Game.hpp` has no save/load related members or methods
- Save/load handled by ZoneManager and OurVerse
- Build green

---

### Rung 6: Delete Game

**Action:**
- Delete all Game files:
  - `src/Singularity/Core/Game.hpp`
  - `src/Singularity/Core/Game.cpp`
  - `src/Singularity/Core/GameInit.cpp`
  - `src/Singularity/Core/GameUpdate.cpp`
  - `src/Singularity/Core/GameRender.cpp`
  - `src/Singularity/Core/GameToolbar.cpp`
  - `src/Singularity/Core/GameSaveLoad.cpp`
  - `src/Singularity/Core/GameNodeGraph.cpp`
- Remove Game from Engine initialization
- Update all remaining callers

**Exit Test:**
- No Game files exist
- Build green
- All functionality preserved through new components

---

## 3. What Happens to Game's Remaining Responsibilities

After extraction, Game's remaining responsibilities map to:

| Game Responsibility | New Home | Rationale |
|---|---|---|
| `init()` / `shutdown()` | `Engine.hpp` | Engine already owns window, GLFW — Game init is Engine init |
| `registerCallbacks()` | `Engine.hpp` | GLFW callbacks belong to Engine (window owner) |
| `update(float dt)` | `Engine.hpp` | Main loop belongs to Engine |
| `render()` | `Engine.hpp` + `Singularity/Screen/` | Render loop belongs to Engine, screen output to Screen |
| `fuseObjects()` | `Form/Object/Object.hpp` | Object-to-Object operations belong to Object |
| `blendRail()` | `Form/Object/Object.hpp` | Field visualization belongs to Object |
| `handleFieldGizmos()` | `Form/Object/Object.hpp` | Field interaction belongs to Object |
| `renderNodeGraph()` | `ZonesOfEarth/AuthorsOfLaw/` | Node graph is Law structure visualization |
| `renderNodePanel()` | `ZonesOfEarth/AuthorsOfLaw/` | Node editing is Law authorship |
| Menu (`_mainMenu`) | `OurVerse/` | Menu is authorship surface |
| Chat (`_chat`) | `OurVerse/Chat.hpp` | Already there, just needs back-pointer removed |
| World (`_world`) | `ZonesOfEarth/ZoneManager.hpp` | World is Zone's world |
| LawManager (`_lawManager`) | `ZonesOfEarth/AuthorsOfLaw/LawManager.hpp` | Already separate, just needs Game coupling broken |
| Player (`_player`) | `Person/Person.hpp` | Already separate, needs Game-specific code removed |
| ZoneManager (`mgr`) | `ZonesOfEarth/ZoneManager.hpp` | Already separate, Game should not own reference |
| UI toggles (`_showToolbar`, `_showChatWindow`, etc.) | `OurVerse/` | UI state belongs to authorship surface |
| Creator state (`_currentTool`, `_current3DMode`, etc.) | `OurVerse/` | Creation is authorship |

---

## 4. Dependencies and Ordering

```
Rung 0: Freeze (can start immediately)
    ↓
Rung 1: Extract Input Handlers (no dependencies)
    ↓
Rung 2: Extract Camera (depends on Rung 1 for input independence)
    ↓
Rung 3: Migrate Player Movement (depends on Rung 2 for camera independence)
    ↓
Rung 4: Extract Tool State (depends on Rung 3 for stable player state)
    ↓
Rung 5: Extract Save/Load (depends on Rung 4 for tool state stability)
    ↓
Rung 6: Delete Game (depends on all above)
```

**Parallelizable:**
- Rung 1 (Input) can be done independently
- Camera extraction (Rung 2) can start once Input is done
- Player movement (Rung 3) can be designed while Camera is being implemented
- Tool state extraction (Rung 4) can be planned in parallel with earlier rungs

---

## 5. First Steps (Start Here)

### Immediate Actions (Today):

1. **Add freeze comment to Game.hpp:**
   ```cpp
   // FROZEN: No new code. See docs/architecture/GAME_ELIMINATION_PLAN.md
   // This class is being eliminated. All new features must go to their
   // ontological home, not here.
   ```

2. **Create this document** (done)

3. **Start Rung 1: Input Handler Extraction**
   - Create `src/Singularity/Input/` directory
   - Move KeyboardHandler and MouseHandler there
   - Remove Game back-pointers
   - Update includes

### Week 1 Goal:
Complete Rung 1 (Input extraction) and have build green.

---

## 6. Tracking

Use the todo system to track progress:

```
[ ] Rung 0: Freeze — header comment added
[ ] Rung 1: Input — KeyboardHandler/MouseHandler moved to Singularity/Input/
[ ] Rung 2: Camera — CameraState moved to Singularity/Screen/
[ ] Rung 3: Player — movement state migrated to Person/Body
[ ] Rung 4: Tools — tool state extracted to Form/Object/Tool/
[ ] Rung 5: Save/Load — moved to ZoneManager
[ ] Rung 6: Delete — all Game files removed
```

---

## 7. Important Constraints

1. **Never break the build** — each rung must complete with green build
2. **Never lose functionality** — test after each change
3. **Follow existing patterns** — look at how other subsystems work (e.g., Physics, ZoneManager)
4. **Document each rung** — add a section to this doc after completing each rung
5. **Use EventBus** — for cross-component communication, not direct pointers
6. **Respect the five refusals** — no new domain nouns as C++ classes

---

## 8. Progress Tracking

### Completed

- **[Rung 0] Freeze** ✅
  - Added freeze comment to Game.hpp
  - Document created and committed

- **[Rung 1] Extract Input Handlers** ✅
  - Created `src/Singularity/Input/` directory
  - Moved `KeyboardHandler` and `MouseHandler` from `Perspective/` to `Singularity/Input/`
  - Removed Game back-pointers from both handlers
  - Added menu state management via public methods (`setMenuOpen`, `isMenuOpen`)
  - Updated `Game.hpp` to include from new location
  - Updated `GameInit.cpp` to use new handlers (removed `setGameInstance` calls, added menu sync)
  - Deleted old `Perspective/KeyboardHandler.*` and `Perspective/MouseHandler.*` files
  - All Game*.cpp files continue to work with new handler locations

- **[Rung 2] Extract Camera** ✅
  - Created `src/Singularity/Screen/Camera.hpp` and `Camera.cpp`
  - Camera class has same public data members as CameraState (pos, front, up, speed, modelview, projection, viewport)
  - Updated `Game.hpp` to include `Singularity/Screen/Camera.hpp` instead of `CameraState.hpp`
  - Changed `_camera` member type from `CameraState` to `Camera`
  - All Game*.cpp files use direct member access which works with both types
  - CameraState.hpp still exists but is no longer included anywhere

- **[Rung 3] Migrate Player Movement State** ✅
  - **Phase 1 (Shadow) - COMPLETED:**
    - Added `grounded`, `wasGrounded`, `wasMoving`, `jumpKeyDownLast` to Person class
    - `velocity` already existed on Person (glm::vec3)
    - Updated Person serialization to include new properties
  - **Phase 2 (Echo) - COMPLETED:**
    - Added shadow writes in `Game::stepMovement()` to sync Game state to Person
  - **Phase 3 (Redirect) - COMPLETED:**
    - Replaced all reads of `_playerVelY` with `_player.velocity.y`
    - Replaced all reads of `_playerGrounded` with `_player.grounded`
    - Replaced all reads of `_jumpKeyDownLast` with `_player.jumpKeyDownLast`
    - Replaced all reads of `_playerWasMoving` with `_player.wasMoving`
  - **Phase 4 (Sole) - COMPLETED:**
    - Removed all writes to Game's `_playerVelY`, `_playerGrounded`, `_jumpKeyDownLast`, `_playerWasMoving`
    - All movement state now written directly to Person properties
  - **Phase 5 (Purge) - COMPLETED:**
    - Removed `_playerVelY`, `_playerGrounded`, `_jumpKeyDownLast`, `_playerWasMoving` from Game.hpp
    - No remaining references to these members in the codebase
  - **Phase 6 (Law) - COMPLETED:**
    - Moved `stepMovement()` logic from Game to `Person::stepMovement()`
    - Game no longer owns movement integration
    - Person now fully owns its movement state and logic
    - Game calls `person.stepMovement(dt, window, camera, mgr, flying, canMove)`
  - **Next:** Consider making Person::stepMovement a Law (requires refactoring to use Law system)

- **[Rung 5] Extract Save/Load** ✅
  - Moved `SaveLoadState`, `ensureHomeZone()`, and the save/load methods (`saveState`, `loadState`, `saveStateWithLog`, `buildSaveJson`, `buildSaveChunkFlatBuffer`, `loadSaveChunkFlatBuffer`, `updateSaveFiles`, `setSaveDirectory`, `getSaveDirectory`) onto `ZoneManager`; removed the `SaveLoadState` member and its include from Game; deleted `SaveLoadState.hpp`. Game now holds thin delegates only.
  - The move initially COPIED rather than moved: eight duplicate definitions were left behind in `GameSaveLoad.cpp` (redefinitions of `buildSaveJson`, `buildSaveChunkFlatBuffer`, `loadSaveChunkFlatBuffer`, `saveState`, `saveStateWithLog`, `loadState`, `shutdown`, `updateSaveFiles`), which did not compile. These have been removed. Earlier drafts of this document claimed "Build verified" for this rung; that claim was false when written.
  - Layer inversion resolved: `ZonesOfEarth/SaveContext.hpp` carries the seven Game members save/load actually touches (`_camera`, `_mouseHandler`, `_currentColor`, `_currentTool`, `_player`, `_lawManager`, `_worldTime`) as pointers to the live objects, so a load can still write back. `ZoneManager`'s four methods take a `SaveContext&` in place of a `Core::Game*`; `ZoneManager.cpp` no longer includes `Singularity/Core/Game.hpp`; `friend class ::ZoneManager` is gone from `Game.hpp`; and `extern ZoneManager mgr` no longer appears in any header (the delegate bodies moved out-of-line into `GameSaveLoad.cpp`). **`ZonesOfEarth` no longer depends on `Singularity/Core`.**
  - Still on Game, pending Rung 6: the save/load ImGui dialogs (`drawLoadWindow`, `drawSaveWindow`, `drawSaveManager`). These are UI surface, not persistence, and belong in `OurVerse/`.
  - Verified: target `earthcall` builds clean; ctest 35/35.

### Pending

- **[Rung 4] Extract Tool State** — PARTIAL
  - Done: relocated the seven tool settings structs (BrushSettings.hpp, CloneToolState.hpp, FacePaintSettings.hpp, PlacementState.hpp, PolyhedronSettings.hpp, PotteryTool.hpp, RotationSettings.hpp) from `Singularity/Core/` to `Form/Object/Tool/`; updated includes in Game.hpp and PolyhedronSettings.cpp; deleted the originals from Singularity/Core/
  - Not done: the stated goal of this rung — "migrate tool settings as properties on Tool beings" — has not happened. Game still owns the settings structs directly as members; they were only moved to a new file location, not turned into properties on authored beings.
  - A `Form::Tool` base class with a `Type` enum (`Brush`, `Polyhedron`, `FaceBrush`, `AdvancedFacePaint`, `Placement`, `Pottery`, `Rotation`, `Clone`, `StrokeTracking`) was created as part of this rung's earlier work. It has been deleted (`src/Form/Object/Tool/Tool.hpp`, `Tool.cpp`): a C++ class for a domain noun plus an enum of kinds-of-thing is exactly what CLAUDE.md refusals #1 and #3 forbid. It was also referenced by nothing in the codebase and contained a guaranteed-infinite-recursion bug in `getIdentifier()`. **A C++ `Tool` class with a `Type` enum is NOT an acceptable implementation of this rung** — do not recreate it.
  - Remaining work: author Tool beings in-world (per `NEW_KIND_FRAMEWORK.md` / `AUTHORED_CATEGORIES.md`), not a new subclass hierarchy. This work has not started.

- **[Rung 6] Delete Game** ⏳
  - Depends on completion of Rungs 3-5. Rung 5 is complete; Rung 4 is partial (see above). Rung 6 has not started.
  - Also folded in here: moving the save/load ImGui dialogs off Game into `OurVerse/`.

### Test Suite

ctest is **35/35**. Five test executables previously failed to compile and did not run; both causes were API drift from earlier refactors, unrelated to Rungs 4/5, and have been fixed:

- `continuous_law_test`, `law_loop_test`, `object_concept_test`, `time_flow_test` — seven alpha-node lambdas took `const ReteFact&`, but `ReteNetwork::AlphaPredicate` is `std::function<bool(const FactPtr&)>` where `FactPtr = std::shared_ptr<ReteFact>` (`src/ZonesOfEarth/AuthorsOfLaw/Law.hpp:397,412`). Lambdas now take `const FactPtr&` and dereference with `f->type`.
- `property_bridge_test` — called `Physics::getBodyFor(...)`, which no longer exists; renamed to `Physics::getFormFor(Object*, float)` (`src/ZonesOfEarth/Physics/Physics.hpp:71`), returning `RigidForm&`. The `.velocity` / `.mass` fields the test reads are unchanged.

### Files Created

- `docs/architecture/GAME_ELIMINATION_PLAN.md` - This document
- `src/ZonesOfEarth/SaveContext.hpp` - Carries save/load state across the layer boundary so `ZonesOfEarth` need not know `Core::Game` exists
- `src/Singularity/Input/KeyboardHandler.hpp`
- `src/Singularity/Input/KeyboardHandler.cpp`
- `src/Singularity/Input/MouseHandler.hpp`
- `src/Singularity/Input/MouseHandler.cpp`
- `src/Singularity/Screen/Camera.hpp`
- `src/Singularity/Screen/Camera.cpp`

### Files Modified

- `src/Singularity/Core/Game.hpp` - Freeze comment, include paths, Camera type
- `src/Singularity/Core/GameInit.cpp` - Handler initialization, menu sync
- `src/Singularity/Core/GameUpdate.cpp` - Shadow writes for player movement state
- `src/Person/Person.hpp` - Added `grounded`, `wasGrounded`, `wasMoving` properties
- `src/Person/Person.cpp` - Updated serialization for new properties
- `src/ZonesOfEarth/ZoneManager.hpp` / `.cpp` - Owns save/load state and methods; takes `SaveContext`
- `src/Singularity/Core/GameSaveLoad.cpp` - Duplicate definitions removed; delegate bodies moved here out-of-line
- `src/Singularity/Core/GameToolbar.cpp` - Save/load call sites routed through Game's delegates
- `src/Form/Object/Object.hpp` - Added `getPendingElementIds()` accessor pair for the private `_composition`
- `src/Util/Serialization.cpp` - Uses the new `getPendingElementIds()` accessor

### Files Deleted

- `src/Perspective/KeyboardHandler.hpp`
- `src/Perspective/KeyboardHandler.cpp`
- `src/Perspective/MouseHandler.hpp`
- `src/Perspective/MouseHandler.cpp`
- `src/Form/Object/Tool/Tool.hpp` - domain-noun class with a kind-enum, violated CLAUDE.md refusals #1 and #3; unreferenced; see Rung 4 above
- `src/Form/Object/Tool/Tool.cpp` - same as above

### Files To Delete (After Rung 6)

- `src/Singularity/Core/CameraState.hpp` - Replaced by Singularity/Screen/Camera
- All `Game*.cpp` and `Game.hpp` files

---

## 9. References

- `NEW_KIND_FRAMEWORK.md` — Why Game must not exist
- `DIRECTORY_ORDERING.md` — Where things belong
- `LAW_MIGRATION_FRAMEWORK.md` — How to migrate behavior to laws
- `KEYBOARD_HANDLER_REFACTOR.md` — Precedent for extracting from Game
- `EarthcallOurverse.md` — The ontology this enforces
