---
title: Data Storage Type Conversion Gaps Audit
date: 2026-10-10
author: Antigravity / Gemini 3.1 Pro (High)
status: Audit / No Changes
---

# Data Storage Type Conversion Gaps Audit

**Goal:** Identify missing conversions, pipelines, and format bridges between Earthcall's various data storage schemas (`JSON`, `EcformGraph` binary, `FlatBuffers`, `MSGPack`, `.eclang` text).
**Status:** Audit only. No engine changes made.

## Framework Context

Earthcall's persistence layer (`src/Singularity/Storage/`) suffers from a fragmented taxonomy of formats. The engine's memory model is an **Ontological Graph** (Nodes/Singulars bound by Relations), but it stores data in various schemas that don't easily convert between one another:
- **JSON (`.json`)**: The legacy, bloated tree-structure format handled by `nlohmann::json`.
- **MSGPack (`.msgpack` / disguised `.ecform`)**: A binary-packed JSON, sometimes written by `SaveSystem.cpp`.
- **Ecform Binary (`.ecform` / `ECFM` magic)**: A true graph-based binary serialization handled by `BinarySerializer.cpp` (`EcformGraph v2`).
- **FlatBuffers (`Earthcall.fbs`)**: High-performance binary schemas located in `Schema/` but seemingly disconnected from the primary save pipeline.
- **EcLang Text (`.eclang`)**: A planned human-readable graph compilation format referenced in `EarthcallFmtTool.cpp`.

---

## Identified Storage Conversion Gaps

### 1. JSON ↔ EcformGraph (Tree-to-Graph Conversion)
- **Gap:** There is no dedicated pipeline to accurately transpile a legacy `JSON` save (which forces a hierarchical Tree ownership model) into a pure `EcformGraph` binary (`.ecform`). `SaveSystem.cpp` attempts to support both, but the toolchain (`EarthcallFmtTool`) lacks a `compile-json` directive to upgrade files.

### 2. Text (`.eclang`) ↔ Binary (`.ecform`)
- **Gap:** The `EarthcallFmtTool.cpp` explicitly lists `compile <input.eclang> <out.ecform>` as `"Not implemented in Phase 4 stub"`. 
- **Impact:** First Movers cannot author a clean, text-based graph (`.eclang`) and compile it into the binary engine format. They are forced to write verbose JSON to bypass the missing compiler.

### 3. JSON ↔ FlatBuffers
- **Gap:** A highly efficient schema exists (`src/Singularity/Storage/Schema/Earthcall.fbs` for `SaveChunk` and `GlobalState`), but there is no conversion bridge to migrate the vast majority of existing `.json` save data into this FlatBuffer format. The FlatBuffers schema exists in a vacuum.

### 4. MSGPack vs. Ecform (Format Confusion)
- **Gap:** `SaveSystem.cpp` frequently converts `nlohmann::json` to MSGPack (`to_msgpack`) and writes it out with an `.ecform` extension (e.g., `writeZoneIdentity`), creating a collision with `BinarySerializer`'s true `ECFM`-magic `.ecform` format. 
- **Impact:** There is no tool to identify or convert a "MSGPack disguised as `.ecform`" into a true "Graph-based `EcformGraph`". This causes fragile deserialization fallbacks.

### 5. Memory Graph ↔ Flat JSON (The Bureaucracy Gap)
- **Gap:** As previously noted in the Sept 1st Serialization Audit, there is no abstraction layer that can losslessly flatten the engine's Relational Graph into JSON without forcing the First Mover to manually manage string `entityId` pointers and nested `"entityA"`, `"entityB"` AST blocks. 

---

## Conclusion
The data storage pipeline is suffering from "format sprawl." The most critical engineering gap is the missing `.eclang` to `.ecform` compiler in `EarthcallFmtTool.cpp`, which forces First Movers to use JSON. Furthermore, the collision between `MSGPack` and true `EcformGraph` binaries sharing the `.ecform` extension needs immediate resolution to prevent save corruption.
