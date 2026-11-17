# File Watcher: Reactive File Sensing & Live Hot-Reloading (`FileWatcher`) (2026-09-08)

**Status:** Working-tree async repair; Zach reports Sanctum at 60 FPS, while watcher correctness and wider live verification remain open.
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T22:00:00-07:00  

---

## 2026-11-16 performance continuation

**Working-tree continuation:** the periodic scan now runs on one joinable worker with copied scan configuration and generation checks; delivery runs on the main thread before Laws. The Release executable and watcher test build, and the existing watcher suite passes including periodic callback thread identity. Zach reports that after this change Sanctum of Beginnings returned to **60 FPS**, with the 3D command-recording reading around **0.1 ms**. This is his live observation, not a controlled benchmark or verification of file events. Deterministic stale-result, in-flight shutdown, scan-failure/partial traversal, cold-baseline, large-result frame-cost, full regression, and populated-Zone/live hot-reload witnesses remain open. Snapshot diff and event publication remain on the main thread and can themselves be large. The sandbox state-root change is not implemented: existing session discovery must be preserved. Codex / GPT-6.1 Terra / `01a1e2e9-2234-7510-8e51-f98034f60923` / 2026-11-16T13:26:10-08:00.

Zach measured under 1 FPS and 4,000–5,000 ms in the 3D phase before and after the Active Zones work; a Release main-thread sample found the file watcher in 877 of 900 samples. The performance panel separates post-render sensing and watcher time from other 3D CPU work. Preserve authored recursive watch coverage and the synchronous explicit `checkNow` contract; generation changes, shutdown, baseline semantics, edge delivery, and large completion batches need witnesses. The Firmament's advice is in the [conversation](../../../../../../agent%20intercom/communication-threads/performance-and-lag/FIRMAMENT_SUN_EARTH_ACTIVE_ZONES_AND_FILE_WATCHER_2026-11-16.md); its cited watcher implementation differs from this checkout, which already tracks mtime and size and does not contain the cited dot-directory pruning. Do not silently prune authored paths or delete sandbox data. Codex / GPT-6.1 Terra / `01a1e2e9-2234-7510-8e51-f98034f60923` / 2026-11-16T12:52:45-08:00.

## Origination & Scope
Zach requested adding live file watching and hot-reloading (Item 1) to Earthcall.
In Earthcall's Sense-Act ontology, storage channels must not merely wait for manual imperative triggers; the machine senses changes made by Persons in the external physical/host world and responds accordingly.

## Architecture & Features

1. **First-Mover Modality Law (`@file-watcher`)**:
   - Implemented `FileWatcher : public Law` in `src/Singularity/Storage/FileWatcher.{hpp,cpp}`.
   - Identifier: `"file-watcher"`.
   - Registered at engine boot in `src/Singularity/Core/EngineInit.cpp`.
   - Ticked before Laws in `src/Singularity/Core/Engine.cpp` at configurable polling intervals in the current working tree.

2. **Past-Tense Noun-Verbed Edge Events**:
   - Emits events strictly on transitions (edges, not continuous levels), adhering to the core event rules in `AGENTS.md`:
     * `file-modified`: when a tracked file's timestamp or size changes.
     * `file-created`: when a new file appears in the watched directory.
     * `file-deleted`: when a previously tracked file disappears.
   - Published directly into `Core::EventBus::instance().publish(ECA::Event{...})`.

3. **In-Engine Hot-Reloading Callbacks**:
   - C++ listener support via `FileWatcher::addCallback(cb)`.
   - Allows rendering subsystems (e.g. WGSL shaders in `WebGpuRenderer`), asset caches, and save monitors to reload assets in real time without restarting the engine.

4. **Universal VFS Integration**:
   - Target path in `watcher.watchPath` supports standard disk paths as well as universal VFS schemes (e.g. `save://`, `zone://`, `recording://`), resolving automatically via `VirtualFileSystem::resolve`.

5. **Configurable Scope & Filters**:
   - `watcher.watchPath`: target directory or single file.
   - `watcher.recursive`: recursive vs top-level directory traversal.
   - `watcher.filterExtension`: filter for specific assets (e.g. `".wgsl"`, `".json"`, `".obj"`).
   - `watcher.pollIntervalMs`: configurable scan period (default: 250ms) to prevent disk thrashing.

6. **Exposed Law Properties (Refusal #6 Compliant)**:
   - Controls: `watcher.enabled`, `watcher.watchPath`, `watcher.recursive`, `watcher.filterExtension`, `watcher.pollIntervalMs`, `watcher.autoReload`.
   - Triggers: `watcher.checkNow` (forces an immediate synchronous scan), `watcher.reloadShaders` (triggers manual shader pipeline recompilation).
   - Telemetry: `watcher.lastModifiedFile`, `watcher.lastEventType`, `watcher.lastEventTimestamp`, `watcher.totalEventsPublished`, `watcher.filesTracked`, `watcher.reloadCount`, `watcher.lastReloadTarget`, `watcher.status`, `watcher.lastError`.

## Verification
- Headless test suite `tests/singularity/file_watcher_test.cpp`:
  - Verified Case 1: File modification detection (`file-modified` ECA event delivered on transition edge, accurate path reporting, EventBus delivery).
  - Verified Case 2: New file creation detection (`file-created` event, tracked file counter increment).
  - Verified Case 3: File deletion detection (`file-deleted` event, tracked file counter decrement).
  - Verified Case 4: File extension filtering (`.wgsl` filter restricts tracking to matching files).
  - Verified Case 5: Automatic shader/rule hot-reloading (`watcher.lastReloadTarget`, `watcher.reloadCount`, and manual `watcher.reloadShaders` trigger).
  - Result: 5/5 tests passed (100%).
