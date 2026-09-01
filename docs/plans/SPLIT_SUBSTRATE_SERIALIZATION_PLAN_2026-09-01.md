# Split Substrate Serialization (The Anti-Stagnation Plan)

## Architectural Context & Status Quo (Handoff Brief)
This document serves as a complete architectural handoff for migrating Earthcall's serialization layer. 

**The Status Quo**: Currently, Earthcall relies on a monolithic `.json` file to serve two inherently opposed purposes: acting as a legible, textual medium for human/LLM First Movers to author Laws and Relations, while simultaneously bearing the burden of dense, spatial geometry (Bézier patches, Polyhedron vertices, GPU face textures). 
As noted in the Agent Intercom broadcasts by GPT-4o, the save system has become "STAGNANT" because it treats worlds as bags of data rather than living graphs. When we force Earthcall's ontology (a web of Relations and Formations) into a JSON structure (a strict hierarchy/tree), we force the First Mover into administrative bureaucracy. Furthermore, to make this work, the engine relies on an overlapping "triple-write" pipeline: it dumps bloated JSON, creates a compressed MessagePack `.ecsave` mirror, and patches performance with a FlatBuffer `_delta.ecsave` chunk. 

**The Solution**: We must split the serialization substrate into two domain-appropriate mediums:
1. **`.ecmatter` (Binary/FlatBuffers)**: The wordless machine memory for physical density (transforms, geometry, pixels, physics).
2. **`.ecform` / `.eclaw` (Text)**: The lean, relational parchment for semantic intent and First Mover authoring.

This split restores End-to-End Coherence, cures the JSON bloat, and forces LLMs/First Movers to interact with the world semantically (via Concepts and Spawns) rather than by hallucinating corrupted Base64 strings or float arrays.

## Person Review Required

> [!WARNING]
> **DSL Parser Strategy**: Jumping straight from JSON to a bespoke text language (e.g., `Spawn "concept.house" at (0,0,0)`) requires building a new lexer/parser in `Singularity/Language/` that can securely hydrate the C++ object graph. Do you want to build this parser from scratch now, or temporarily use a forgiving format like YAML/JSON5 stripped of all binary/geometry blobs as a stepping stone?

> [!IMPORTANT]
> **Backward Compatibility**: We need to determine how long `SaveSystem::readSaveData` will support the legacy monolithic `.json` saves. I recommend adding a migration path on load: if a legacy `.json` is detected, it is immediately split and saved as `.ecform` + `.ecmatter` transparently, so we don't have to maintain the JSON load path indefinitely.

## Proposed Changes

### 1. Phase 1: Expand the FlatBuffer Schema (`.ecmatter`)

**Architectural Reasoning**: Physical matter (vertices, normal vectors, uncompressed pixel textures, continuous mathematical fields) has no business being serialized as ASCII text. Currently, `Serialization.cpp` uses a utility called `BinaryPack` to encode raw binary memory into Base64 strings just so it can fit inside the JSON tree. This is a severe anti-pattern. While Earthcall has introduced FlatBuffers (in `_delta.ecsave`), they are currently only used as a "dirty delta" optimization rather than the primary physical substrate. By elevating FlatBuffers to `.ecmatter`, we give spatial density a proper, natively mapped binary home, drastically reducing load times and file size while freeing the text files from administrative bloat.

#### [MODIFY] `src/Singularity/Storage/Schema/save_chunk.fbs` (or equivalent schema file)
- Add missing physical properties flagged in the `SERIALIZATION_SUBSTRATE_AUDIT` (e.g., `smoothData`, `complexData`, `_spatialRootObject`).
- Add support for `FaceTexture` pixel data directly in the FlatBuffer, eliminating the need for Base64 encoding.

#### [MODIFY] `src/Singularity/Storage/SaveSystem.hpp` & `.cpp`
- Introduce `writeMatterData(...)` and `readMatterData(...)` to exclusively handle FlatBuffer `.ecmatter` files.

---

### 2. Phase 2: Create the Semantic Text Exporter (`.ecform`)

**Architectural Reasoning**: According to Earthcall's First Mover tenets (Refusal 6: No Black Box), the save file is the ultimate parchment of authority. If an LLM or human wants to dictate a new Law or Formation, they must be able to write it directly into the substrate. JSON fails at this because it forces them to write AST (Abstract Syntax Tree) integer opcodes instead of declarative logic. Furthermore, the current `to_json` implementation acts as a "Temporal Black Box," silently dropping registered properties (like `smoothData` or `_attributes`) because it doesn't know how to serialize them. By stripping out all geometry and reducing this file to *pure semantic intent*, we create an environment where First Movers can safely author worlds without corrupting physical memory arrays.

#### [MODIFY] `src/Singularity/Storage/Serialization.cpp`
- **Strip the bloat:** Remove `patch`, `polyhedron`, `faceColors`, and `BinaryPack` logic from `to_json(Object)`. 
- **Preserve the intent:** Ensure `authoredProperties`, `stakeholders`, and identifiers remain. 
- Ensure all properties registered in `buildProperties()` that are *not* physical geometry are properly routed to the text exporter to eliminate the "Temporal Black Box".

---

### 3. Phase 3: The Loader Unification

**Architectural Reasoning**: In a split-substrate system, the loading mechanism must follow a strict ontological order: Intent precedes Matter. You cannot inject vertices into a shape that hasn't been conceptually spawned yet. Therefore, the loader must first read the `.ecform` to construct the "skeleton" of the world (instantiating the `Object` identities, the `Relation` graph, and the `Law`s). Once the graph is stable and memory is allocated, the loader reads `.ecmatter` to "flesh out" those identities with their heavy physical attributes (transform matrices, vertex buffers, pixel data). 

#### [MODIFY] `src/ZonesOfEarth/ZoneManager.cpp`
- Update `loadState()` to execute a two-step hydration process.
- **Step 1:** Parse `.ecform` to instantiate `Object`s, `Relation`s, and `Law`s based on the semantic graph.
- **Step 2:** Parse `.ecmatter` to inject the raw vertices, `transform` matrices, and `faceTextures` into the corresponding objects by matching `identifier`.

---

### 4. Phase 4: Sunset the Triple-Write

**Architectural Reasoning**: `ZoneManager::saveState` is currently trapped in a transitional phase. When a user hits save, the engine writes the same world three times: a monolithic `.json`, a compressed `.ecsave` (MessagePack), and a `_delta.ecsave` (FlatBuffers). This was a symptom of attempting to patch the JSON bloat problem without committing to the architectural split. Once `.ecform` and `.ecmatter` are unified, the legacy formats are dead weight. Sunsetting them finalizes the migration and permanently removes the "Stagnant" bag-of-data paradigm.

#### [MODIFY] `src/ZonesOfEarth/ZoneManager.cpp`
- Remove the monolithic `nlohmann::json` dump from `ZoneManager::saveState`.
- Delete the `_delta.ecsave` chunking logic, as the new `.ecmatter` pipeline will natively handle rapid binary saves (since it maps directly to memory) without interfering with the First Mover text files.

#### [DELETE] `src/Singularity/Storage/BinaryPack.hpp` & `.cpp`
- Base64 encoding and JSON binary packing are structurally obsolete and must be purged from the codebase.

## Verification Plan

### Automated Tests
- `ctest -R save_roundtrip_test`: Must pass with the new split substrate, proving that properties are preserved across a save cycle.
- `ctest -R no_black_box_test`: Ensure the refactored `Serialization.cpp` properly saves everything registered in `buildProperties()`.
- Add a new `tests/storage/substrate_split_test.cpp` to verify that an injected `.ecform` file can successfully spawn concepts without needing `.ecmatter`.

### Manual Verification
1. Open Earthcall, sculpt a complex shape with the 3D Face Brush, and save the world.
2. Verify that `saves/worlds/<name>.ecform` is tiny, human-readable, and free of Base64 blobs.
3. Verify that `saves/worlds/<name>.ecmatter` contains the dense binary data.
4. Reload the save and verify the sculpture looks identical.

---
*Signed: Antigravity, Session ID 97115016-4730-43e8-aba7-87b7bad34e1a, 2026-09-01T13:45*
