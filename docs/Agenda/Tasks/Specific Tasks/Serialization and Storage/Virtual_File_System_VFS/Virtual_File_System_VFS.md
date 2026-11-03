# Virtual File System (VFS) with Universal URI Addressing (`VirtualFileSystem`) (2026-09-08)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T21:53:00-07:00  

---

## Origination & Scope
Zach requested adding Universal Addresses / Virtual File System (VFS) to Earthcall.
In Earthcall, world saves and assets should not depend on brittle, machine-specific absolute file paths (`/Users/zacharyzhang/...`), nor should temporary computational scratchpads require writing physical disk files.

## Architecture & Features

1. **First-Mover Modality Law (`@vfs`)**:
   - Implemented `VirtualFileSystem : public Law` in `src/Singularity/Storage/VirtualFileSystem.{hpp,cpp}`.
   - Identifier: `"vfs"`.
   - Registered at engine boot in `src/Singularity/Core/EngineInit.cpp`.

2. **Universal URI Resolution Scheme**:
   - `save://<relative-path>`: dynamically resolves against `SaveSystem::saveRoot()` (e.g. `save://worlds/my_world.json`).
   - `zone://<zone-id>/<relative-path>`: resolves to the specific Zone's storage root (`SaveSystem::zoneDirectory(id)`).
   - `home://<person-id>/<relative-path>`: resolves to the Person's dwelling memory directory (`SaveSystem::homeDirectory(id)`).
   - `recording://<relative-path>`: resolves to `saves/recordings/`.
   - `file://<path>`: clean canonical local file path stripping.

3. **In-Memory Ephemeral Virtual Files (`memory://`)**:
   - Zero-copy, RAM-resident volatile storage (`memory://<id>`).
   - Allows Laws, tests, and temporary pipelines to write and read virtual files without hitting the physical SSD or leaving disk remnants.
   - Fully tracks memory usage: `vfs.memoryFilesCount`, `vfs.memoryBytesAllocated`.

4. **Dynamic Mount Table**:
   - Custom mount points (e.g. `vfs->mount("assets://", "saves/assets/")`).
   - Prefix replacement maps virtual directories to concrete paths.
   - Telemetry property: `vfs.mountTable` lists all active mounts.

5. **Transparent `FileChannel` Integration**:
   - `FileChannel` seamlessly accepts VFS URIs in `@file-channel.path`:
     - Writing to `memory://...` stores directly in RAM.
     - Reading from `memory://...` reads back from RAM.
     - Writing to `save://...` or `zone://...` resolves to concrete paths while preserving sandbox security bounds.

6. **Exposed Law Properties (Refusal #6 Compliant)**:
   - Control: `vfs.enabled`, `vfs.queryUri`, `vfs.mountPrefix`, `vfs.mountTarget`.
   - Triggers: `vfs.mount`, `vfs.unmount`.
   - Telemetry & Diagnostics: `vfs.resolvedPath`, `vfs.isMemory`, `vfs.exists`, `vfs.mountTable`, `vfs.memoryFilesCount`, `vfs.memoryBytesAllocated`.

## Verification
- Headless test suite `tests/singularity/vfs_test.cpp`:
  - Verified Case 1: Standard URI resolution across `save://`, `zone://`, `home://`, `recording://`, and `file://`.
  - Verified Case 2: Custom mount table registration, path mapping, and unmounting.
  - Verified Case 3: In-memory file creation, data integrity, existence checks, byte sizes, and memory allocation metrics.
  - Verified Case 4: Complete integration with `FileChannel` (writing/reading `memory://` payloads, JSON validation over memory files, and physical disk persistence via `save://`).
  - Result: 4/4 tests passed (100%).
