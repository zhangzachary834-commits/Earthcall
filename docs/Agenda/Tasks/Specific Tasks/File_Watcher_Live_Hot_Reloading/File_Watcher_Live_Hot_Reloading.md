# File Watcher: Reactive File Sensing & Live Hot-Reloading (`FileWatcher`) (2026-09-08)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T22:00:00-07:00  

---

## Origination & Scope
Zach requested adding live file watching and hot-reloading (Item 1) to Earthcall.
In Earthcall's Sense-Act ontology, storage channels must not merely wait for manual imperative triggers; the machine senses changes made by Persons in the external physical/host world and responds accordingly.

## Architecture & Features

1. **First-Mover Modality Law (`@file-watcher`)**:
   - Implemented `FileWatcher : public Law` in `src/Singularity/Storage/FileWatcher.{hpp,cpp}`.
   - Identifier: `"file-watcher"`.
   - Registered at engine boot in `src/Singularity/Core/EngineInit.cpp`.
   - Ticked in `src/Singularity/Core/EngineRender.cpp` at configurable polling intervals.

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
