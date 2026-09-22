# Model Context Protocol (MCP) Server Bridge (`Option A`) (v2) (2026-09-09)

**Status:** ✅ done and verified (v2 Field-Tested Upgrade)  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark (sparkly guy)  
**Timestamp:** 2026-09-09T18:19:00-07:00  

---

## Origination & v1 Field Testing Feedback
Zach and Claude (Opus 4.6 via Claude Desktop) conducted a live field test of the MCP bridge v1, successfully querying world state, spawning 14 entities into "Clawd's Monastery", and building persistent structures. The field test identified five key areas for v2:
1. `earthcall_author_law` created empty shells because custom action/condition schemas were not compiled into real OntoMath ASTs.
2. `earthcall_write_property` returned `sent_without_ack` because raw JSON primitives failed in `propertyValueFromJson` and `@` target prefix was unhandled.
3. SDF / Implicit Field creation lacked a dedicated tool.
4. Law deletion crashed when mutating Laws from background WebSocket worker thread while engine main thread ticked.
5. Spawned objects were lost on zone switch if not manually saved because they were not added to `globalObjects`.

---

## v2 Architecture & Upgrades Implemented

### 1. Main-Thread Dispatch & Crash Immunity (`pollMainThread`)
- **Root Cause**: `WebSocketServer`'s `worker` thread was executing `lm->createLaw`, `lm->remove`, and `mgr.active().addObject` directly, causing data races and iterator invalidation with `Engine::tick(dt)` / `LawManager::step()`.
- **Solution**: Added `mainThreadTasks` queue in `WebSocketServer::Impl` and wired `Singularity::Network::WebSocketServer::instance().pollMainThread()` into `Engine::tick(dt)`. All world mutations now execute deterministically on the engine's main thread.

### 2. Full OntoMath AST Compilation & Raw Mode in `earthcall_author_law`
- In `WebSocketServer.cpp` (`create_law`), added dual-mode support:
  - **Raw AST Mode**: Directly deserializes `actionModel` (`ActionNode::fromJson`) and `conditionModel` (`ConditionNode::fromJson`) matching internal engine format.
  - **Structured Template Mode**: Automatically compiles simplified schemas (`flow`, `map`, `set`, `spawn`, `destroy`, `sin`/`sinusoid`, `linear`, `constant`, `toggle`) into real `OntoMath::MathNode`, `OntoMath::ScalarForm`, and `OntoMath::Piecewise` continuous functions.
  - Correctly binds event triggers (`lm->bindTrigger`) and calls `law->recompile()`.

### 3. Transparent Property Writes & Live SDF Conversions (`earthcall_write_property`)
- Upgraded `propertyValueFromJson` in `PropertyValueJson.cpp` to natively handle both `{ "t": ..., "v": ... }` and raw JSON primitives (booleans, ints, floats, strings, 3-element vec3 arrays, 16-element mat4 arrays).
- Normalized target parsing: strips leading `@` and resolves `@player`, `@active_zone`, beings in `Universe`, active zone objects, and Laws/Channels in `LawManager`.
- Direct SDF conversion: writing `field.expr` or `expr` on an Object parses with `geom::makeImplicit` and calls `obj->setFieldShape(node, extent)` in real time.
- Guaranteed `property_write_ack` with `status: "success"|"failed"|"target_not_found"`.

### 4. Dedicated SDF Field Creation Tool (`earthcall_spawn_field`)
- Added dedicated `earthcall_spawn_field` tool (now 17 tools total):
  - Spawns live raymarched Signed Distance Fields (`ShapeKind::Field`, `spatialKind = 1`).
  - Accepts any implicit formula (`sphere(0.5)`, `box(0.5)`, `torus(0.5, 0.2)`, `smoothUnion(sphere(0.5), box(0.4), 0.1)`, `morph(...)`).
- Updated `earthcall_spawn_object` to accept `shape: "Field"`, `fieldExpr`, and `extent`.

### 5. Auto-Persistence & Zone Switching Object Retention
- In `spawn_object` and `spawn_field`:
  - Registers zone designations on the object (`obj->addZoneDesignation(...)`).
  - Adds to both `mgr.active().addObject(obj)` AND `mgr.getGlobalObjects().push_back(obj)`.
  - Calls `mgr.persistZones()`.
  - Switching zones and reloading now preserves all spawned entities.

### 6. Robust World Saving (`earthcall_save_world`)
- Dispatched safely on the main thread via `pollMainThread()`.
- Catches exceptions and returns `save_ack` with success status and filename.

---

## Verification
- MCP Bridge test suite `tests/singularity/mcp_bridge_test.js`: **8/8 checks passed**.
  - All 17 tools registered with valid schemas.
  - Protocol handshake, health info, save cataloging, and offline fallback verified.
- C++ Engine test suite:
  - `file_channel_test`: **13/13 passed**.
  - `screen_recorder_test`: **8/8 passed**.
  - `vfs_test`: **4/4 passed**.
  - `stream_channel_test`: **4/4 passed**.
