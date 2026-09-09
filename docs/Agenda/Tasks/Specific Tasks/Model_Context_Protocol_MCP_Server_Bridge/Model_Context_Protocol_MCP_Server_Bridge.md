# Model Context Protocol (MCP) Server Bridge (`Option A`) (2026-09-09)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-09T13:17:00-07:00  

---

## Origination & Scope
Zach requested implementing Option A: TypeScript/Node Model Context Protocol (MCP) Bridge (`scripts/mcp-server.js` and `src/Singularity/Foreign/mcp/`) to allow external AI models and developer tools (Claude Desktop, Cursor, Gemini CLI, OpenAI, etc.) to perceive and author Earthcall directly over standard MCP stdio JSON-RPC.

Option C (Native C++ MCP Server directly in the engine) is tracked on the To-do list for subsequent implementation.

## Architecture & Communication

1. **Placement (Refusal #2 Compliant)**:
   - Built under `src/Singularity/Foreign/mcp/earthcall-mcp-server.js`.
   - Executable wrapper at `scripts/mcp-server.js` (`chmod +x`).
   - Integrated with `package.json` under `"scripts": { "mcp": "node scripts/mcp-server.js" }` and `"bin": { "earthcall-mcp": "scripts/mcp-server.js" }`.

2. **Bidirectional WebSocket Bridge**:
   - Maintains an asynchronous bridge to Earthcall's C++ WebSocket server (`ws://localhost:8080`, configurable via `EARTHCALL_WS_URL`).
   - Translates MCP tool calls into Earthcall JSON protocol messages (`property_write`, `spawn_object`, `transform_object`, `create_law`, `switch_zone`, etc.) and returns real-time responses.
   - Automatically reconnects with exponential backoff if the engine restarts.

3. **Graceful Offline Fallback**:
   - If Earthcall's live engine is not currently running, inspection tools (`earthcall_get_state`, `earthcall_list_saves`) automatically fall back to reading `saves/worlds/` and `saves/zones/` on disk, allowing models to inspect world states even offline.
   - Status tool (`earthcall_get_connection_status`) reports connection health and provides clear instructions for starting the engine.

## Tools Exposed to AI Models (16 Tools)

1. `earthcall_get_state`: Retrieves full active world state (active zone, objects, positions, colors, shapes, laws, and player coordinates).
2. `earthcall_spawn_object`: Spawns 3D shapes (Cube, Sphere, Cylinder, Cone, Torus, Plane) with custom transform, scale, color, and name.
3. `earthcall_transform_object`: Modifies an existing entity's position, rotation, dimensions, color, shape, or material.
4. `earthcall_delete_object`: Removes an object from the active zone by identifier.
5. `earthcall_write_property`: Sets any property path on any being (e.g. `@player.position.y`, `@screen-recorder.recording`, `@active_zone.gravity`, or object ID). Refusal #6 compliant.
6. `earthcall_author_law`: Authors or modifies a When-Condition-Action Law (supports built-in presets like zero-g, color-pulse, orbit, bounce, or custom ASTs).
7. `earthcall_toggle_law`: Enables or disables an existing Law by identifier.
8. `earthcall_delete_law`: Removes an authored Law from Earthcall's LawManager.
9. `earthcall_switch_zone`: Switches active Zone by name or index.
10. `earthcall_create_zone`: Creates a new Zone in the Ourverse.
11. `earthcall_teleport_player`: Teleports the player to 3D world coordinates.
12. `earthcall_speak`: Emits an utterance into Earthcall's Language modality and EventBus.
13. `earthcall_save_world`: Triggers a world save to disk with optional custom name.
14. `earthcall_screen_record`: Controls the Singularity Screen Recorder (start, stop, pause, resume, snapshot).
15. `earthcall_list_saves`: Catalogs saved worlds, zones, and homes in `saves/`.
16. `earthcall_get_connection_status`: Verifies live WebSocket bridge connectivity.

## Resources & Prompts

- **Resources**:
  - `earthcall://status`: Live engine and WebSocket connection health.
  - `earthcall://world/snapshot`: Real-time JSON snapshot of active zone and entities.
  - `earthcall://saves/catalog`: Catalog of saved worlds and zones.
- **Prompts**:
  - `earthcall-first-mover`: System instructions for guiding AI agents as Earthcall First Movers respecting the Seven Refusals.

## Verification
- Test harness `tests/singularity/mcp_bridge_test.js`:
  - Verified MCP protocol handshake (`initialize`).
  - Verified `tools/list` exposes all 16 tools with valid JSON schemas.
  - Verified `earthcall_get_connection_status` returns structured status.
  - Verified `earthcall_list_saves` catalogs saved files on disk.
  - Verified `earthcall_get_state` retrieves world state with offline fallback.
  - Verified `resources/read` for `earthcall://status`.
  - Verified `prompts/get` for `earthcall-first-mover`.
  - Result: **7/7 checks passed (100%)**.
