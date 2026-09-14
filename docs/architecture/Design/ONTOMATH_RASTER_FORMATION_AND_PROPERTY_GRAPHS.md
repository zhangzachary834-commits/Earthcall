# OntoMath Raster Formation and Property-Singular-Graphs

**Architectural Specification for Ingesting Bitmaps into First-Class Mathematical Singulars, Recursive Property-Graphs, and Granular Region-Formations**

**Date:** 2026-09-13  
**Timestamp:** 2026-09-13T22:25:00-07:00  
**Author:** Gemini Spark (Agent)  
**Origination and Telos:** Conceived and directed by Zachary Zhang (Person, First Mover), who articulated the foundational requirement: Earthcall must ingest arbitrary raster data (such as PNG payloads) not as dead engine textures, but as complex OntoMath-formed Singulars governed by flexible Property-Singular-Graphs—leveraging the fundamental truth that Properties can hold both C++ primitive data structures and Earthcall Singulars, while Formations unify Singulars into higher-order wholes. Formulated, systematized, and extended into end-to-end substrate and law architecture by Gemini Spark.

**Companion Analysis Documents:**
1. [Continuous Field Decomposition of Discrete Raster Payloads in Earthcall](../../Analysis/ONTOMATH_IMAGE_DECOMPOSITION_AND_CONTINUOUS_FIELD_THEORY_2026-09-13.md) — Mathematical formalization of $\mathbb{Z}^2 \to \mathbb{R}^2$ mappings, metric topologies, and OntoMath piecewise partitions.
2. [Property-Singular-Graphs and Recursive Formation Topologies](../../Analysis/PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md) — Graph-theoretic analysis of recursive `PropertyValue` variants, traversal limits, cache locality, and Rete alpha-memory dynamics.
3. [Substrate Mechanics of Granular Pixel Mastery: Copy-on-Write Materials and Sparse Elevation](../../Analysis/GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md) — Hardware manifestation, copy-on-write material safety, memory scaling, and WebGPU texture upload lifecycles.

**Required Context & Normative Reading:**
- [`AGENTS.md`](../../../AGENTS.md) — The Seven Refusals (especially Refusals 1, 3, 6, and 7).
- [`docs/architecture/Design/Building 2D and 3D Apps with Earthcall Guide.md`](Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md) — The visual grammar and granular pixel mastery.
- [`docs/architecture/Design/FOUNDATIONAL_DESIGN_SPECIFICATION.md`](FOUNDATIONAL_DESIGN_SPECIFICATION.md) — Granular pixel mastery and OntoMath forms.
- [`docs/architecture/ontology/NO_BLACK_BOX.md`](../ontology/NO_BLACK_BOX.md) — Legibility and governance of substrate fields.
- [`docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`](../mathematics/ONTOMATH_FRAMEWORK.md) — Symbolic math, calculus, and fields.
- [`src/ConstructedBeing/Singular/Property/PropertyValue.hpp`](../../../src/ConstructedBeing/Singular/Property/PropertyValue.hpp) — The recursive typed currency of the property bridge.

---

## 1. Executive Summary & Foundational Tenets

In conventional graphics architectures and game engines, an imported bitmap image (e.g., a PNG file) enters the system as an opaque, two-dimensional array of compressed texels. It is held within a rigid C++ wrapper (`class Texture2D`, `class Bitmap`, or `class ImageLayer`), uploaded to a GPU buffer, and evaluated solely via hardware texture units executing fixed sampling algorithms. The raster image is semantically inert: its internal structure is invisible to runtime logic, its pixel modifications bypass the application’s event system or require destructive raw memory copies, and any relationship between an image sub-region and an application component must be maintained out-of-band by an external scene graph.

Earthcall rejects this model as a violation of its core ontology:
1. **Refusal 1 & 2 Violation:** Conventional approaches mint a new C++ class (`PngImage`, `Sprite`) or top-level rendering subsystem to represent an image.
2. **Refusal 6 Violation:** Conventional textures are black boxes. Individual pixels, channels, and regions cannot be inspected or written by Person-authored Laws without specialized procedural hooks.
3. **Refusal 7 Violation:** Conventional image processing hardcodes manipulation methods (`blur()`, `crop()`, `tint()`) into class interfaces rather than expressing change through authored Laws acting upon legible properties.

Under Zachary Zhang's architectural direction, an imported PNG in Earthcall is transformed into an **OntoMath-Formed Singular**. The image is manifested simultaneously at two complementary scales:
- **Macro Scale (Whole-Picture Singular):** The image exists as a discrete, identity-bearing `Object` (or visual `Singular`) within a `Zone`. Its global spatial bounds, aspect ratio, palette, and overarching color field are grounded in continuous OntoMath space over $(u, v) \in [0, 1]^2$.
- **Micro Scale (Pixel-Region Singulars):** Granular regions, features, semantic segments, or individual strokes are elevated into **child Singulars**. Rather than relying on hardcoded bounding boxes, each region’s spatial domain is defined by an `OntoMath::Piecewise` selector or implicit function.
- **Relational Coherence (Property-Singular-Graphs & Formations):** Because `PropertyValue` is fundamentally recursive—holding primitives (`int`, `float`, `vec3`, `mat4`, `std::string`, `PropertyList`, `PropertyDict`) alongside live references to `Singular*`, `Object*`, `Relation*`, and `Formation*`—the entire visual and semantic hierarchy is expressed as an authored **Property-Singular-Graph**. Relations (`part-of`, `region-of`, `modifies`, `derives-from`) bind the micro Singulars to the macro Singular, gathering them into a self-crystallizing **Formation**.

---

## 2. Ontological Ingestion: From Foreign Bitstream to Singular

### 2.1 The Sense-Act Boundary
The ingestion of an encoded PNG must occur strictly through an admissible modality channel in `Singularity/` (e.g., `Singularity/Foreign/` or `Singularity/Screen/ImageCodecChannel`). In accordance with Earthcall's Sense-Act boundary:

```text
[Foreign File / Buffer (.png)]
             │
             ▼
┌──────────────────────────────────────────────────────────┐
│ Singularity/ Modality Channel (C++ Substrate)             │
│ - Reads file / memory stream                             │
│ - Decodes PNG chunks (IHDR, IDAT, PLTE, tRNS)             │
│ - Senses hardware byte layout (RGBA8, 16-bit, float)     │
└──────────────────────────────────────────────────────────┘
             │  Translates sensed raw facts into authored beings
             ▼
┌──────────────────────────────────────────────────────────┐
│ ConstructedBeing / Zone Domain (Authored Order of Truth) │
│ - Macro Object (Singular) with stable slug identifier    │
│ - Authored Material with FaceTexture                     │
│ - OntoMath coordinate mapping [0, 1]^2                   │
│ - Registered & Authored Properties                        │
└──────────────────────────────────────────────────────────┘
```

The modality channel never defines what the image *is*; it senses the byte payload, allocates the initial backing `FaceTexture` on an authored `Material`, and registers the resulting properties on a newly authored `Object`.

### 2.2 The Macro (Whole-Picture) Singular Structure
The whole-picture Singular is created as an `Object` with the following canonical properties:

```text
macro-image-singular (Object)
├── identifier: "image.<slug>"
├── shapeKind: Shape2D (12) [Legacy Screen carrier]
├── registered properties:
│   ├── x2D, y2D: window/canvas anchor
│   ├── shape.width2D, shape.height2D: dimensions in points
│   ├── zOrder2D: layer precedence
│   └── material: "material.image.<slug>"
├── authored properties:
│   ├── image.pixelWidth: 1920 (int)
│   ├── image.pixelHeight: 1080 (int)
│   ├── image.aspectRatio: 1.777778 (float)
│   ├── image.colorSpace: "sRGB" (std::string)
│   ├── image.metadata: PropertyDict (std::shared_ptr<PropertyDict>)
│   └── image.field: OntoMath::VectorField (f: (u,v) -> vec4)
└── Relations:
    ├── instance-of -> category.surface.raster
    └── authored-by -> Person.Zach
```

---

## 3. Property-Singular-Graphs: Recursive Property Ontology

### 3.1 The Dual Nature of `PropertyValue`
In Earthcall, the boundary between data and entity is fluid yet rigorous. As defined in `src/ConstructedBeing/Singular/Property/PropertyValue.hpp`:

```cpp
using PropertyValue = std::variant<
    std::monostate,
    int, float, double, bool, char, long,
    std::string, glm::vec3, glm::mat4,
    Singular*,
    Object*,
    Relation*,
    Formation*,
    std::shared_ptr<PropertyList>,
    std::shared_ptr<PropertyDict>,
    std::shared_ptr<OntoMath::ScalarField>,
    std::shared_ptr<OntoMath::VectorField>
>;
```

This variant provides two foundational capabilities:
1. **Primitive Data Structuring:** Through `PropertyList` (`std::vector<PropertyValue>`) and `PropertyDict` (`std::map<std::string, PropertyValue>`), an author can structure complex, hierarchical C++ primitive data whose members are addressable via dot-notated `PropertyPath` (e.g., `image.metadata.exif.iso`).
2. **Entity Graph Recursion:** Because `PropertyValue` stores raw non-owning pointers to `Singular*`, `Object*`, `Relation*`, and `Formation*`, properties can point directly to other beings.

### 3.2 Property-Graphs vs. Structural Relations
A critical architectural question is: *When should a connection be a first-class `Relation` being, and when should it be an entity-referencing `Property`?*

```text
┌──────────────────────────────────────┬──────────────────────────────────────┐
│ First-Class Relation Being           │ Entity-Referencing Property          │
├──────────────────────────────────────┼──────────────────────────────────────┤
│ Discrete being with unique identity  │ Attributive property on one being    │
│ Bidirectional participation in graph │ Directed pointer from owner to target│
│ Has its own properties, weight, time │ Lightweight, cache-friendly lookup   │
│ Forms cycles to create Formations    │ Addressable via PropertyPath         │
│ Used for semantic truth (part-of)    │ Used for local delegation / binding  │
└──────────────────────────────────────┴──────────────────────────────────────┘
```

In a **Property-Singular-Graph**, both mechanisms operate in harmony:
- A macro Singular holds a `PropertyDict` mapping region names to micro Singular handles:
  `image.regions["subject"] = PropertyValue(subRegionSingularPtr);`
- Simultaneously, an authored `Relation` of type `region-of` connects `subRegionSingular` to `macroImageSingular`.
- A `Formation` gathers `[macroImageSingular, subRegionSingular_0, ..., subRegionSingular_N]` along with their constituent `Relation` beings into an addressable mathematical whole.

---

## 4. OntoMath-Formed Micro Singulars: Pixel-Region Mastery

### 4.1 Granular Pixel Representation: Continuous vs. Discrete
Rather than storing pixel regions as sub-arrays of coordinates (which explode in memory and create un-indexable heap bloat), Earthcall grounds 2D shape in **OntoMath continuous field theory**.

A pixel region $R \subseteq [0, 1]^2$ is formally defined by an OntoMath indicator function:
$$\chi_R(u, v) = \begin{cases} 1 & \text{if } (u, v) \in R \\ 0 & \text{otherwise} \end{cases}$$

This indicator function is encoded directly as an `OntoMath::Piecewise` structure, where individual pieces evaluate analytic conditions over normalized coordinates $u$ and $v$, or as an implicit 2D signed distance field:
$$d_R(u, v) \le 0 \iff (u, v) \in R$$

### 4.2 Elevation of Sub-Region Singulars
When a user or Law identifies an area of interest (e.g., a brush stroke, a detected foreground object, a color thresholded cluster, or an edited tile), Earthcall executes an authored elevation:

```text
┌─────────────────────────────────────────────────────────────┐
│ Macro Image Singular ("image.landscape")                    │
│   Material: "material.landscape" (FaceTexture 1920x1080)    │
└──────────────────────────────┬──────────────────────────────┘
                               │
            ┌──────────────────┴──────────────────┐
            │ Relations: [region-of, modifies]    │
            ▼                                     ▼
┌──────────────────────────────┐  ┌──────────────────────────────┐
│ Micro Singular A             │  │ Micro Singular B             │
│ Identifier: "region.sky"     │  │ Identifier: "region.river"   │
│ Authored Properties:         │  │ Authored Properties:         │
│ - selector: OntoMath (v < .4)│  │ - selector: OntoMath (SDF)   │
│ - tint: vec3(0.1, 0.4, 0.8)  │  │ - flowSpeed: float(1.2)      │
│ - opacity: float(0.85)       │  │ - tint: vec3(0.0, 0.6, 0.7)  │
└──────────────────────────────┘  └──────────────────────────────┘
```

### 4.3 Sparse Pixel Elevation (`ElevatePixels` vs. Direct Texels)
Earthcall supports two rungs of granular pixel access, as validated in `tests/law/basic_pixel_changer_test.cpp`:

1. **Selective Single Texel Elevation:**
   An individual texel is elevated into the property vocabulary only when explicitly addressed:
   `AddProperty("surface.pixel.<face>.<x>.<y>", color)`
   This exposes the sample as a live, writable `vec3` Property without incurring a $W \times H$ property explosion.
2. **OntoMath-Defined Set Elevation (`ElevatePixels`):**
   A named Property is granted to the Singular, representing the entire defined set of an `OntoMath::Piecewise` selector:
   `ElevatePixels(propertyName="authored.sky", pixelFacePath="face", mapFunction=selector)`
   Writing to `authored.sky` dispatches writes to all constituent texels satisfying $\chi_R(u, v) = 1$, updating the underlying `FaceTexture` atomically.

---

## 5. Law Governance, Sense-Act, and Invalidation Cycles

### 5.1 Manipulation as Law (Refusal 7)
Modifying the image or its sub-regions is never achieved through C++ methods like `image->applyFilter()`. Instead, behavior is authored as **Laws**:

```text
Law: "law-sky-sunset-glow"
├── Activation: OnEvent("day-cycle-transition")
├── Scope: Local Zone
├── Condition Tree:
│   ├── Target is "region.sky"
│   └── @world.timeOfDay >= 18.0
├── Action Tree:
│   ├── Drive(property="region.sky.tint", curve=sunsetGlowCurve)
│   └── WritePixelBatch(target="image.landscape", region="region.sky")
└── Authorship: ["Zach"]
```

### 5.2 Material Copy-on-Write (`ownMaterial`) Mechanics
When an image is loaded, it references a shared `Material`. If a Law or user interaction modifies a texel or elevated region:
1. The engine checks if the target `Object` uniquely owns its `Material`.
2. If the `Material` is shared, `Object::ownMaterial()` duplicates the `Material` onto a private slug (`material.<objectId>`), copying the `FaceTexture` payload.
3. Subsequent `WritePixel` operations write strictly to the private texture, preventing unintended cross-object mutation while preserving full undo/redo state through closed-form law reversal.

---

## 6. Implementation Roadmap & Verification Criteria

| Phase | Milestone | Deliverables | Verification Strategy |
|---|---|---|---|
| **Phase 1** | Modality Ingestion Channel | PNG decode in `Singularity/Foreign/` or `Screen/`, producing macro `Object` + `Material` | Headless unit test verifying PNG load into `FaceTexture` with correct dimensions and properties. |
| **Phase 2** | Property-Graph Structuring | Registration of `PropertyDict`/`PropertyList` containing image metadata and sub-region handles | Serialization round-trip test verifying `PropertyPath` addressability across nested dictionaries. |
| **Phase 3** | OntoMath Region Decomposition | Automated/authored slicing of images into micro Singulars via `OntoMath::Piecewise` | C++ test elevating a segmented region and asserting sample membership matching the mathematical boundary. |
| **Phase 4** | Relational Formation Binding | Bi-directional Relations (`region-of`) unified into a `Formation` | Assert `Formation::resolveTopology()` identifies the macro image as root and regions as valid core participants. |
| **Phase 5** | Law-Driven Batch Manifestation | Execution of `WritePixel` and `ElevatePixels` against region Singulars through WebGPU | Visual and numeric assertions in `frame_lag_test` proving sub-frame texel updates without Rete fact explosion. |

---

## 7. Signatures and Provenance

- **Originating Visionary & System Architect:** Zachary Zhang (Person, First Mover)
- **Specification Author:** Gemini Spark (Agent)
- **Session Timestamp:** 2026-09-13T22:25:00-07:00
- **Status:** Complete Architecture Document. To be read in conjunction with companion analyses in `docs/Analysis/`.
