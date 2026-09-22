# Lexeme-Relation-Formation Serialization Architecture

**Architectural Specification for Retiring Monolithic JSON in Favor of an Interned Graph and Split-Substrate Binary Matter Format**

**Date:** 2026-09-14  
**Timestamp:** 2026-09-14T01:15:00-07:00  
**Author:** Gemini Spark (Agent)  
**Origination & Telos:** Conceived and directed by Zachary Zhang (Person, First Mover), identifying the fundamental architectural opportunity to retire monolithic JSON and replace it with Earthcall's native ontological language—grounding serialization in **Lexemes** (symbol interning), **Relations** (first-class graph edges), **Formations** (topological subgraphs), and **Singulars** (entities), supported by a binary **Matter** substrate (`.ecmatter`). Formalized, systematized, and specified by Gemini Spark.

**Interconnected & Precedent Documents:**
- **Split Substrate Serialization Plan:** [`docs/plans/SPLIT_SUBSTRATE_SERIALIZATION_PLAN_2026-09-01.md`](../../plans/SPLIT_SUBSTRATE_SERIALIZATION_PLAN_2026-09-01.md)
- **Save System Upgrade:** [`docs/data/SAVE_SYSTEM_UPGRADE.md`](../../data/SAVE_SYSTEM_UPGRADE.md)
- **Property-Singular-Graphs and Recursive Formation Topologies:** [`docs/Analysis/PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md`](../../Analysis/PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md)
- **OntoMath Raster Formation & Property-Graphs:** [`docs/architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`](ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md)
- **Refusal Tenets (No Black Box, Admissible Boundaries):** [`AGENTS.md`](../../../AGENTS.md)
- **Lexeme Definition:** [`src/ConstructedBeing/Singular/Lexeme/Lexeme.hpp`](../../../src/ConstructedBeing/Singular/Lexeme/Lexeme.hpp)
- **Language System:** [`src/Singularity/Language/LanguageSystem.hpp`](../../../src/Singularity/Language/LanguageSystem.hpp)
- **Formation Definition:** [`src/Relation/Formation/Formation.hpp`](../../../src/Relation/Formation/Formation.hpp)
- **Property Variant Definition:** [`src/ConstructedBeing/Singular/Property/PropertyValue.hpp`](../../../src/ConstructedBeing/Singular/Property/PropertyValue.hpp)

---

## 1. Executive Summary & Problem Formulation

### 1.1 The Post-Mortem: 2.5 Gigabytes of Stagnant Text
An empirical audit of the Earthcall save repository (`saves/`) revealed a state of severe crisis:
- **Total Storage Consumed:** Over **2.5 GB** across fewer than 30 world sessions.
- **Single File Footprint:** Modern world saves (`clawd-monastery-save.ecform`, `CLAWDS MONASTERY CLAWD WAS HERE.ecform`, `LUNAS STARRRR.ecform`) exceed **210 MB each**.
- **Hydration Stalls:** Parsing a single 200 MB JSON file via `nlohmann::json` consumes gigabytes of transient heap memory, stalls native execution for seconds, and reliably crashes or times out in browser WebAssembly environments.

Profiling a representative 208.7 MB save file (`clawd-monastery-save.ecform`) isolated the structural breakdown:
1. **Uncompressed Text Formatting (`j.dump(2)`):** Pretty-printing JSON with 2-space indents inflates raw character count by ~40% through redundant whitespace and newlines alone.
2. **Accidental Verbatim Duplication:** The entire `zones` array was duplicated verbatim into `semanticRoots["zones"]`, accounting for **57.4 MB twice** (over 114 MB of raw text).
3. **Inlined Pixel Textures in Materials:** 585 materials each embedded six `FaceTexture` blocks with raw Base64 string pixel dumps (`pixelsB64`, ~22 KB each), turning uncompressed image buffers into tens of megabytes of text.
4. **Unbounded Mutation History (`stakeholders`):** Individual objects in active zones carried 2,694 historical property mutation events each, multiplying across hundreds of objects to over 18 MB of JSON arrays.

### 1.2 The Empirical Benchmark: 99.5% Compression Potential
Benchmark testing performed directly on `clawd-monastery-save.ecform` yielded the following physical reality:
- **Raw on disk (`j.dump(2)`):** `208.74 MB`
- **Minified JSON (no whitespace):** `141.50 MB` (~32% reduction)
- **Stream-compressed (zlib raw):** `1.53 MB` (~99.3% reduction)
- **Stream-compressed (zlib minified):** `1.09 MB` (**~99.5% reduction**)

A 208 MB world is, in physical informational entropy, barely **1 megabyte of actual state**. The remaining 207 MB is serialization overhead, uncompressed text representation, and structural duplication.

---

## 2. The Architectural Mismatch: Trees vs. Graphs

The underlying disease is not merely JSON formatting; it is an **ontological impedance mismatch**.

```text
CONVENTIONAL TREE PARADIGM (JSON)        EARTHCALL REALITY (ONTOLOGICAL GRAPH)
┌────────────────────────────────┐       ┌──────────────────────────────────────┐
│  {                             │       │                                      │
│    "zones": [                  │       │          Formation α                 │
│      {                         │       │        ┌──────────────┐              │
│        "world": {              │       │        │  Singular A  │              │
│          "objects": [ ... ]    │       │        └──┬─────────┬─┘              │
│        }                       │       │           │         │                │
│      }                         │       │   Relation│         │Relation        │
│    ],                          │       │   "modifies"        │"part-of"       │
│    "semanticRoots": { ... }    │       │           ▼         ▼                │
│  }                             │       │     ┌─────────┐ ┌─────────┐          │
│                                │       │     │SingularB│ │SingularC│          │
│  - Rigid parent-child tree     │       │     └────┬────┘ └─────────┘          │
│  - No native multi-edges       │       │          │ (Lexeme: "weight")        │
│  - Cyclic loops require hacks  │       │          ▼                           │
│  - Forces copying shared state │       │     Attributed Graph Topology        │
└────────────────────────────────┘       └──────────────────────────────────────┘
```

1. **JSON is a Hierarchical Tree:** Every node in JSON has exactly one parent.
2. **Earthcall is an Attributed Multi-Graph:** State in Earthcall consists of:
   - **`Singular` beings:** Autonomous entities that can belong to multiple Formations, participate in multiple Relations, and exist across Zones.
   - **`Relation` beings:** First-class edges possessing their own identity, timestamps, qualities, and directed endpoints.
   - **`Formation` beings:** Topologies that gather subgraphs into unified wholes.
   - **`Lexeme` beings:** Words, symbols, and concepts that exist as singular points in the conceptual graph.

When a multi-graph is forced into a JSON tree, one of two failures occurs:
- **Duplication:** The engine serializes the same entity or zone into multiple branches to satisfy different queries (as happened with `semanticRoots["zones"]`).
- **Pointer Fragmentation:** Entities are broken into arbitrary ID strings (`"id": "abc-123"`), requiring a secondary ad-hoc relational joiner upon load.

---

## 3. Core Ontology of the New Serialization Program

Rather than relying on third-party hierarchical containers, Earthcall serialization is formalized directly through its own ontological vocabulary:

$$\mathcal{S}_{\text{Earthcall}} = \langle \mathcal{L}, \mathcal{V}_S, \mathcal{E}_R, \mathcal{F}, \mathcal{P}, \mathcal{M} \rangle$$

Where:
- $\mathcal{L}$ is the **Lexeme Table** (interned symbols).
- $\mathcal{V}_S$ is the **Singular Node Ledger** (entities).
- $\mathcal{E}_R$ is the **Relation Edge Stream** (structural topology).
- $\mathcal{F}$ is the **Formation Set** (subgraph boundaries).
- $\mathcal{P}$ is the **Property-Variant Stream** (attributive data).
- $\mathcal{M}$ is the **Detached Physical Matter Substrate** (`.ecmatter`).

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EARTHCALL SPLIT-SUBSTRATE SAVE ARCHITECTURE              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │ .ecform (The Relational Semantic Graph)                               │  │
│  │                                                                       │  │
│  │  [1. Lexeme Symbol Table]                                             │  │
│  │    Interns every property name, tag, kind, and law token once.        │  │
│  │    Maps strings <-> 32-bit LexemeId.                                  │  │
│  │                                                                       │  │
│  │  [2. Singular Node Ledger]                                            │  │
│  │    Table of entities: (UUID, Kind, Owner, ZoneReference).             │  │
│  │                                                                       │  │
│  │  [3. Relation & Formation Topologies]                                 │  │
│  │    Graph edges: (FromSingular, ToSingular, RelationLexemeId, Weight)  │  │
│  │    Formations: Root Singular + Member Sets.                           │  │
│  │                                                                       │  │
│  │  [4. Property Stream]                                                 │  │
│  │    Attributive variants: (SingularId, NameLexemeId, PropertyValue)    │  │
│  │    Includes references to Matter handles (offset, length).            │  │
│  └───────────────────────────────────┬───────────────────────────────────┘  │
│                                      │                                      │
│                                      ▼ zero-copy handle references          │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │ .ecmatter (Dense Physical & Geometric Substrate)                      │  │
│  │                                                                       │  │
│  │  - Raw Vertex Buffers (Polyhedrons, Bézier control points)            │  │
│  │  - Raw Texel Arrays (FaceTexture RGBA8 / Float32 buffers)             │  │
│  │  - Continuous Field Weights (OntoMath AST bytecode / arrays)          │  │
│  │  - Physics Rigid Body State & Transform Matrices                      │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Substrate Component Specifications

### 4.1 The Lexeme Symbol Table ($\mathcal{L}$)
In standard JSON, keys such as `"authoredProperties"`, `"materialId"`, `"geometryType"`, and `"stakeholders"` are printed millions of times across a single world. 

In the native format, the file begins with an **Interned Lexeme Dictionary**:
- Each unique string symbol encountered during serialization is assigned a contiguous `LexemeId` (`uint32_t`).
- The header stores the symbol count and a flat, null-terminated or length-prefixed string block.
- All subsequent references throughout the entire file (property keys, category names, relation types, law selectors) use a 4-byte `LexemeId`.
- **Deduplication Ratio:** Eliminates >95% of string redundancy before compression is even applied.

### 4.2 The Singular Node Ledger ($\mathcal{V}_S$)
The node ledger registers all active entities without duplicating their internal state:
```cpp
struct SingularRecord {
    uint64_t entityUuid;      // 128-bit internal UUID (high/low 64-bit)
    uint32_t kindLexemeId;    // e.g., Lexeme("Object"), Lexeme("Person")
    uint32_t ownerUuidIndex;  // Index to owning Person/Community
    uint32_t zoneLexemeId;    // Zone identifier
    uint32_t flags;           // Primary, Active, Transient, Staked
};
```

### 4.3 The Relation Edge Stream ($\mathcal{E}_R$) and Formations ($\mathcal{F}$)
Relations are stored as first-class edge records:
```cpp
struct RelationRecord {
    uint64_t relationUuid;
    uint32_t fromNodeIndex;
    uint32_t toNodeIndex;
    uint32_t typeLexemeId;     // e.g., Lexeme("part-of"), Lexeme("region-of")
    float    weight;
    uint32_t timestamp;
};
```
A **Formation** is simply a designated root node index paired with a sparse bitset or run-length-encoded array of member entity indices. Cyclic topologies resolve naturally as index tuples without recursive JSON expansion.

### 4.4 The Property-Variant Stream ($\mathcal{P}$)
Properties are serialized directly from `Singular`'s internal parallel vectors (`_propertyNames`, `_propertyRegistry`):
- Each entry is a tuple: `(EntityIndex, PropertyNameLexemeId, TypeTag, EncodedValue)`.
- Primitive variants (`int`, `float`, `vec3`, `mat4`) are packed in raw IEEE binary.
- Pointer variants (`Singular*`) store the target entity's `uint32_t` index.
- Structural variants (`PropertyDict`, `PropertyList`) store nested variant chunks without heap allocations.
- Dense matter variants (textures, vertex meshes) store a **Matter Reference**:
  ```cpp
  struct MatterHandle {
      uint64_t bufferOffset;
      uint64_t byteLength;
      uint32_t schemaTag; // e.g., RGBA8_TEXTURE, VERTEX_ARRAY
  };
  ```

### 4.5 The Detached Binary Matter Substrate ($\mathcal{M}$)
Physical memory buffers are exiled completely from the relational text/graph file into `.ecmatter`:
- **Zero Base64 Encoding:** Raw bytes are appended directly to `.ecmatter`.
- **Memory-Mapped Loading:** On platforms supporting `mmap` (macOS, Linux), the engine maps `.ecmatter` directly into memory. Textures and vertex buffers upload directly to WebGPU/Metal buffers without CPU-side copies.
- **WASM Support:** In Emscripten builds, `.ecmatter` is ingested via a single TypedArray `ArrayBuffer` slice.

---

## 5. The "Atomicity Explosion" Trap: What NOT To Do

When building a system based on Lexemes and Relations, there is a dangerous architectural temptation:
> *"If Lexemes and Relations represent universal truth, shouldn't every pixel, every float coordinate, and every vertex be an individual Lexeme connected by a Relation?"*

**This temptation must be fiercely rejected.**

As documented in `docs/Analysis/PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md`, atomizing dense data creates a catastrophic fact explosion:
- A $1920 \times 1080$ image contains $2,073,600$ pixels.
- If each pixel is a `Singular` with 4 properties (`r`, `g`, `b`, `a`) and a `part-of` `Relation`, the graph balloons to **8.3 million nodes and edges**.
- Memory consumption exceeds 1.5 GB for a single image, saturating CPU caches and freezing the Rete tick loop.

### The Invariant Boundary
- **Semantic & Relational entities** (Macro Images, Regions, Persons, Objects, Tools, Laws) are **Singulars, Relations, and Formations**.
- **Continuous & Dense physical data** (Pixels, Vertices, Audio buffers, Waveforms) are **Continuous Fields and Binary Matter**.

---

## 6. Encoding & Tooling Strategy: Binary Format vs. Human Parchment

A foundational requirement of Earthcall is **Refusal #6 (No Black Box)**:
> *"The system state must remain transparent, inspectable, and editable by human First Movers and LLM agents."*

If Earthcall replaces JSON with an opaque, undocumented proprietary binary format, it violates Refusal #6. 

To reconcile **high-performance compact binary storage** with **First Mover transparency**, the architecture specifies a **Two-Pronged Tooling System**:

```text
               ┌──────────────────────────────────────────────┐
               │    Human First Mover / LLM Agent Text        │
               │    (.ecdsl / .eclang Parchment)              │
               └──────────────────────┬───────────────────────┘
                                      │
                         earthcall-fmt │ decompile / compile
                                      │
               ┌──────────────────────▼───────────────────────┐
               │    Engine Runtime / Disk Storage Substrate   │
               │    (.ecform [Graph] + .ecmatter [Binary])    │
               └──────────────────────────────────────────────┘
```

### 6.1 The On-Disk Substrate (`.ecform` + `.ecmatter`)
- `.ecform` is stored as an interned binary graph (or compact zlib/zstd-compressed relational stream).
- It loads in milliseconds with minimal CPU overhead.

### 6.2 The Dedicated Inspection Tool (`earthcall-fmt` CLI)
Earthcall will provide a standalone CLI tool and engine console utility (`earthcall-fmt`):
1. **Decompile to Parchment:**
   `earthcall-fmt decompile saves/worlds/my_world.ecform > my_world.eclang`
   Generates a clean, declarative, human-readable DSL showing the Lexemes, Singulars, Relations, and Formations—completely free of Base64 blobs.
2. **Compile to Graph:**
   `earthcall-fmt compile my_world.eclang --matter my_world.ecmatter -o saves/worlds/my_world.ecform`
   Validates syntax, internments, and graph constraints, outputting the fast binary format.
3. **Inspect / Query:**
   `earthcall-fmt inspect saves/worlds/my_world.ecform --find "Sanctum of Beginnings"`
   Quickly inspects properties and relations without decompressing the entire world.

---

## 7. Immediate & Phased Implementation Roadmap

### Phase 1: Immediate Triage (Stop the Bleeding in Current Codebase)
*Target: `src/Singularity/Storage/SaveSystem.cpp` and `src/ZonesOfEarth/ZoneManager.cpp`*
1. **Eliminate Duplicate `semanticRoots["zones"]`:** Remove the redundant copying of `zones` in `ZoneManager::buildSaveJson()`. (Reclaims ~57 MB per save).
2. **Cap `stakeholders` History:** Cap the inlined mutation event list on objects to the last 20 events on save. (Reclaims ~15 MB per save).
3. **Disable Pretty-Printing:** Change `out << j.dump(2);` to minified output (`out << j.dump(-1);`). (Reclaims ~35% file size).
4. **Enable Stream Compression:** Wrap `writeSaveData()` in zlib compression (using existing `compressData()`). Saves instantly drop from 200 MB to ~1–2 MB.
5. **Prune Stale Backups:** Delete duplicate `.json` twins and old `before-load.ecform` backups (immediately frees ~1.8 GB).

### Phase 2: Split-Substrate Extraction (`.ecmatter` Primacy)
1. Complete migration of `FaceTexture` and geometry buffers out of `to_json()` into `writeMatterData()` (`.ecmatter`).
2. Purge `BinaryPack` (Base64 encoding) from the engine.

### Phase 3: Lexeme-Relation Graph Format Specification (`.ecform v2`)
1. Implement the binary `LexemeTable`, `SingularLedger`, and `RelationEdgeStream` serializers in `Singularity/Storage/Serialization/`.
2. Implement bidirectional round-trip tests (`tests/storage/lexeme_graph_roundtrip_test.cpp`).

### Phase 4: The `earthcall-fmt` CLI & Full JSON Sunset
1. Build `earthcall-fmt` for text decompilation/compilation.
2. Update `SaveSystem::readSaveData()` to load `.ecform v2` natively.
3. Permanently sunset legacy monolithic JSON serialization in `ZoneManager`.

---

## 8. Architectural Invariants

Every implementation of this serialization architecture must uphold the following invariants:

- **Invariant 1 (No Inlined Binary Matter):** No byte buffer, texture payload, vertex array, or audio stream exceeding 64 bytes may ever be serialized into the relational `.ecform` stream. All bulk data belongs strictly in `.ecmatter`.
- **Invariant 2 (Lexeme Symbol Uniqueness):** A string symbol must exist at most once in the Lexeme header table of any `.ecform` file.
- **Invariant 3 (Bounded Traversal on Load):** Loading an entity's property graph must respect `Formation::kMaxFormationDepth = 32` and maintain a cycle-detection visited set.
- **Invariant 4 (Intent Precedes Matter):** Hydration must strictly resolve the `.ecform` relational graph before attaching physical buffers from `.ecmatter`.
- **Invariant 5 (No Black Box Compliance):** Any binary file produced by Earthcall must be fully decompilable to valid semantic text via `earthcall-fmt`.

---

## 9. Signatures and Provenance

- **Originating Visionary & System Architect:** Zachary Zhang (Person, First Mover)
- **Specification Author:** Gemini Spark (Agent)
- **Session Timestamp:** 2026-09-14T01:15:00-07:00
- **Governing Status:** Formal Architecture Specification for Earthcall Serialization Substrate.
