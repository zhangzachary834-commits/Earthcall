# Property-Singular-Graphs and Recursive Formation Topologies

**Research Analysis: Graph Theory, Memory Layouts, Traversal Limits, and Rete Alpha-Beta Propagation**

**Date:** 2026-09-13  
**Timestamp:** 2026-09-13T22:31:00-07:00  
**Author:** Gemini Spark (Agent)  
**Origination & Intellectual Lineage:** Conceived by Zachary Zhang (Person, First Mover), identifying the core architectural capability that Earthcall Properties are not limited to scalar primitives, but can hold Earthcall Singulars or primitive C++ data structures whose members are fully authored, and that Formations unify Singulars into coherent graph structures. Evaluated and formalized by Gemini Spark.

**Interconnected Documents:**
- **Primary Architecture:** [`docs/architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`](../architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md)
- **Companion Analysis 1:** [`docs/Analysis/ONTOMATH_IMAGE_DECOMPOSITION_AND_CONTINUOUS_FIELD_THEORY_2026-09-13.md`](ONTOMATH_IMAGE_DECOMPOSITION_AND_CONTINUOUS_FIELD_THEORY_2026-09-13.md)
- **Companion Analysis 3:** [`docs/Analysis/GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md`](GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md)
- **Rete & Property Interning:** [`docs/Analysis/Law_Property_Interning_and_Rete_Updates.md`](Law_Property_Interning_and_Rete_Updates.md)
- **Property Lookup Complexity:** [`docs/Analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md`](PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md)
- **Formation Definition:** [`src/Relation/Formation/Formation.hpp`](../../../src/Relation/Formation/Formation.hpp)
- **Property Variant Definition:** [`src/ConstructedBeing/Singular/Property/PropertyValue.hpp`](../../../src/ConstructedBeing/Singular/Property/PropertyValue.hpp)

---

## 1. The Recursive Graph Nature of Earthcall State

In traditional game and application development, there is an unyielding divide between:
1. **The Object Model (Scene Graph):** Pointers between C++ entity classes.
2. **The Component State:** Primitive floats, ints, and strings stored inside data structures.
3. **The Scripting/Reflection Boundary:** String-keyed maps that allow dynamic logic to query entity state.

This triple representation introduces serialization friction, memory fragmentation, and constant type marshalling.

In Earthcall, state is unified by a singular recursive definition. Under `src/ConstructedBeing/Singular/Property/PropertyValue.hpp`, `PropertyValue` is a variant capable of holding both primitives and entity pointers:
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

Because `Singular` owns properties, and a `Property` holds a `PropertyValue`, which can itself hold a `Singular*` or a `PropertyDict` containing more `PropertyValue`s, Earthcall's state forms an **attributed, directed multi-graph**:
$$\mathcal{G} = (\mathcal{V}_S \cup \mathcal{V}_P, \mathcal{E}_A \cup \mathcal{E}_R)$$
where:
- $\mathcal{V}_S$ is the set of identity-bearing `Singular` nodes.
- $\mathcal{V}_P$ is the set of structured property nodes (`PropertyList`, `PropertyDict`).
- $\mathcal{E}_A$ is the set of **attributive property edges** (directed, named edges from a Singular or PropertyDict to another value).
- $\mathcal{E}_R$ is the set of **first-class Relation edges** (discrete `Relation` beings that carry their own identities, properties, and timestamps).

---

## 2. Attributive Properties vs. Structural Relations

A foundational design question arises when constructing complex structures (such as an image partitioned into pixel regions): *Should a connection be represented as a Property containing a pointer, or as a first-class Relation being?*

### 2.1 The Two Modalities
| Dimension | Attributive Property (`Singular*` / `PropertyPath`) | Structural `Relation` Being |
|---|---|---|
| **Identity** | No independent identity; owned entirely by the host Singular | Discrete `Singular` with unique, stable identifier |
| **Edge Multiplicity** | At most one value per property name on a Singular | Arbitrary multi-edges between endpoints |
| **Legibility** | Directly legible via dot-path: `image.regions.sky` | Requires relational query or Formation traversal |
| **Memory Overhead** | 4-byte `StringId` + 24–32 byte `PropertyValue` | Full heap object (`sizeof(Relation) \approx 128` bytes) |
| **Traversability** | Unidirectional (owner $\to$ target) | Bidirectional (endpoint A $\leftrightarrow$ endpoint B) |
| **Formation Participation**| Cannot close relational cycles to form Formations | Essential primitive for Formation topology |

### 2.2 The Unified Synthesis: Property-Formations
Zachary Zhang recognized that this is not an "either/or" choice:
> *"So Property-Formations, and Property-graphs would be Singulars owning Properties that are primitive cpp data structures whose members are fully authored... and how images should be both modeled as whole-picture Singulars yet modifying smaller parts of it does it by gathering Properties into smaller Singulars that encompass smaller pixel-regions of it."*

In the unified architecture:
1. **Property-Graphs for High-Speed Path Navigation:**
   The macro image Singular owns a `PropertyDict` named `regions`:
   $$\text{image.regions} \mapsto \{ \text{"sky"}: \text{Singular}^*, \text{"river"}: \text{Singular}^* \}$$
   A Law can immediately dereference `image.regions.sky.tint` with $O(1)$ cached array lookups.
2. **Relations for Ontological Grounding:**
   Simultaneously, `Relation` beings of type `region-of` and `modifies` connect each micro Singular to the macro Singular.
3. **Formations for Coherent Governance:**
   A `Formation` is instantiated containing the macro Singular, the micro Singulars, and the connecting Relations. This Formation is rooted in the macro Singular, elevating the entire image and its constituent regions into an addressable **Category/Formation unity**.

```text
       ┌────────────────────────────────────────────────────────┐
       │                 Formation: "image.forest"              │
       │                   Root: Singular (Macro)               │
       │                                                        │
       │  ┌─────────────────────────┐                           │
       │  │  Singular (Macro Image) │                           │
       │  │  owns: PropertyDict     │                           │
       │  │    "regions"            │                           │
       │  └──────────┬──────────────┘                           │
       │             │                                          │
       │      attributive property edge                         │
       │             │                                          │
       │             ▼                                          │
       │  ┌─────────────────────────┐   first-class Relation    │
       │  │  Singular (Micro Region)│◄───────────────────────┐  │
       │  │  "region.canopy"        │   type: "region-of"    │  │
       │  └─────────────────────────┘                        │  │
       │             │                                       │  │
       │             └───────────────────────────────────────┘  │
       └────────────────────────────────────────────────────────┘
```

---

## 3. Computational Scaling and Cache Locality

### 3.1 Contiguous Array Traversal on the Hot Path
As analyzed in `docs/Analysis/Law_Property_Interning_and_Rete_Updates.md` and `docs/Analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md`, Earthcall completely eliminates hash maps from runtime property resolution.

Inside every `Singular`, registered and authored properties are stored in two parallel contiguous arrays:
- `std::vector<Earthcall::StringId> _propertyNames`
- `std::vector<std::unique_ptr<Property>> _propertyRegistry`

When evaluating a deep Property-Singular-Graph path (e.g., `image.regions.sky.tint`):
1. `image` resolves `regions` via a 4-byte integer linear scan over `_propertyNames` (which fits in a single 64-byte L1 cache line).
2. The resulting `PropertyDict` is retrieved from `PropertyValue`.
3. The sub-key `sky` is resolved, yielding the `Singular*` pointer for the micro region.
4. The micro region's `_propertyNames` is scanned for `tint`.

Total time complexity: $O(L \cdot \frac{K}{16})$ cache-line fetches, where $L$ is the path depth (here $L=3$) and $K$ is the number of properties per node (typically $K < 20$). The resolution completes in nanoseconds, with zero heap allocations during traversal.

### 3.2 Cycle Detection and Depth Bounding
Because `PropertyValue` can store `Singular*` pointers, an authored property graph could theoretically contain directed cycles (e.g., node A points to node B, and node B points back to node A).

To guarantee termination during serialization, Rete fact collection, and recursive path lookups, the architecture enforces:
1. **Depth Bounding:** All recursive graph traversals are strictly bounded by `Formation::kMaxFormationDepth = 32`. Any traversal reaching depth 32 terminates immediately and logs a diagnostic refusal.
2. **Cycle Guards:** Cold operations (such as `Formation::toJson()`) maintain a thread-local visited pointer set:
   ```cpp
   std::vector<const Formation*> seen;
   ```
   If a node is already present in `seen`, traversal refuses recursion loudly, preserving memory safety and halting infinite loops.

---

## 4. Impact on Rete Pattern Matching: Preventing Fact Explosions

### 4.1 The $O(N \cdot M)$ Threat
In Earthcall's rule engine, every property on every object emits a `property-state` fact into the Rete network (`LawManager::_facts`).

If an image of $1920 \times 1080$ pixels were to treat every pixel as an independent Singular with properties:
$$N_{\text{facts}} = 1920 \times 1080 \times 4 \approx 8.3 \times 10^6 \text{ facts}$$
Evaluating these facts against the Rete alpha-memory network would overwhelm CPU memory, saturate L3 cache, and freeze the engine's tick loop (as occurred in historical stress tests documented in `SYNTHESIS_STUDIO_CLICK_LOCKOUT_2026-09-04.md`).

### 4.2 The Property-Singular-Graph Solution
By organizing pixels into an **OntoMath-Formed Property-Singular-Graph**:
1. The **Macro Image** emits only its macro properties into Rete (`image.pixelWidth`, `image.aspectRatio`, `material`).
2. Only **Elevated Micro-Region Singulars** (typically $10^0$ to $10^2$ active regions) emit facts into Rete (`region.canopy.tint`, `region.canopy.selected`).
3. Individual pixels remain un-elevated inside the `FaceTexture` byte buffer until explicitly addressed by `AddProperty` or `ElevatePixels`.
4. When a Law modifies a region property (e.g., `region.canopy.tint = vec3(0.2, 0.8, 0.1)`), the engine only invalidates the **single fact** corresponding to `region.canopy.tint`.
5. The Screen channel's `WritePixel` handler updates the underlying GPU texture buffer directly in a single batch, completely bypassing Rete fact re-generation for the hundreds of thousands of unchanged pixels.

### 4.3 Rete Join-Node (Beta Network) Scaling
Because micro-regions are connected to the macro image via `Relation` beings, multi-entity join conditions can be compiled cleanly into Rete Beta nodes:

```text
Condition:
  (Object ?img with category == "category.surface.raster")
  (Relation ?rel with type == "region-of", from == ?region, to == ?img)
  (Object ?region with selected == true)
```

The Beta join table only indexes active regions ($\le 10^2$ items). The join complexity remains bounded by $O(|\mathcal{V}_{\text{regions}}|)$, completely decoupled from the underlying pixel resolution ($W \times H$).

---

## 5. Summary & Structural Invariants

The Property-Singular-Graph architecture transforms Earthcall's property bridge into a fully general, high-performance graph database residing in L1/L2 cache. By combining:
- **`PropertyValue` Variant Recursion** for nanosecond dot-path traversal,
- **First-Class `Relation` Beings** for semantic truth and provenance,
- **`Formation` Rooted Topologies** for coherent category and set unity, and
- **OntoMath Region Partitions** to shield the Rete network from pixel-count explosions,

Earthcall achieves both granular pixel-level authoring and multi-hundred FPS engine performance without violating a single one of the Seven Refusals.
