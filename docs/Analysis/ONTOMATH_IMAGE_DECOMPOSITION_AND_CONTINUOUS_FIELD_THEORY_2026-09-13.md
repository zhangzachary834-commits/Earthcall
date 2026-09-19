# Continuous Field Decomposition of Discrete Raster Payloads in Earthcall

**Research Analysis: Mathematical Rigor, Metric Topology, and OntoMath Piecewise Partitions**

**Date:** 2026-09-13  
**Timestamp:** 2026-09-13T22:28:00-07:00  
**Author:** Gemini Spark (Agent)  
**Origination & Intellectual Lineage:** Directed by Zachary Zhang (Person, First Mover), establishing that 2D raster assets (such as PNG files) must not remain dead, un-addressable texture buffers, but must be elevated into continuous OntoMath fields whose sub-regions are mathematically partitioned into discrete, law-governed Singulars. Formulated with mathematical and topological rigor by Gemini Spark.

**Interconnected Documents:**
- **Primary Architecture:** [`docs/architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`](../architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md)
- **Companion Analysis 2:** [`docs/Analysis/PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md`](PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md)
- **Companion Analysis 3:** [`docs/Analysis/GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md`](GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md)
- **Foundational Guide:** [`docs/architecture/Design/Building 2D and 3D Apps with Earthcall Guide.md`](../architecture/Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md)
- **OntoMath Framework:** [`docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`](../architecture/mathematics/ONTOMATH_FRAMEWORK.md)

---

## 1. The Core Tension: The Discrete Lattice vs. The Continuous Domain

In classical computer graphics, raster images are defined as discrete two-dimensional arrays:
$$I_d: \Lambda \to \mathcal{C}$$
where $\Lambda = \{0, 1, \dots, W-1\} \times \{0, 1, \dots, H-1\} \subset \mathbb{Z}^2$ is a rectangular integer lattice and $\mathcal{C} \subset \mathbb{R}^k$ represents color space (typically $k=3$ for RGB or $k=4$ for RGBA).

When this discrete payload is fed into a graphics pipeline, the hardware wraps it in an opaque sampler object. The transformation from discrete texel coordinates $(x, y) \in \Lambda$ to normalized texture coordinates $(u, v) \in [0, 1]^2$ is treated as an implementation detail of the GPU's fixed-function texture mapping unit (TMU).

In Earthcall, this delegation is unacceptable for three ontological reasons:
1. **The Inscrutable Sampler (Refusal 6):** The interpolation algorithm (bilinear, bicubic, nearest-neighbor) is embedded in hardware registers or driver state. A Person-authored Law cannot inspect, modify, or conditionally modulate the interpolation kernel as a first-class property.
2. **Sub-Region Illegibility:** In a discrete array, an arbitrary subset of pixels (e.g., a hand-drawn stroke or a recognized character) has no identity other than an arbitrary list of memory indices. It cannot participate in the Rete pattern matching network or receive directed Relations.
3. **Rigid Dimensionality:** Discrete arrays do not possess intrinsic scale-invariance. Transforming, warping, or projecting a subset requires resampling the entire array into a new buffer, destroying authorial provenance and wasting heap allocation.

To resolve this, Earthcall models every raster asset as a **continuous topological field** defined over the compact unit square:
$$\Omega = [0, 1]^2 \subset \mathbb{R}^2$$
The discrete bitmap is recognized merely as a finite, discrete empirical sampling of this underlying continuous mathematical field.

---

## 2. Topological Formalization of OntoMath Field Lifting

### 2.1 The Embedding and Reconstruction Operator
Let $I: \Lambda \to [0, 1]^k$ be the discrete raster payload decoded from a PNG file. We define the continuous OntoMath field:
$$\Phi_I: \Omega \to \mathbb{R}^k$$
by constructing an authored reconstruction operator $\mathcal{R}_K$:
$$\Phi_I(u, v) = \sum_{(i, j) \in \Lambda} I(i, j) \cdot K\left( u \cdot W - \left(i + \frac{1}{2}\right), v \cdot H - \left(j + \frac{1}{2}\right) \right)$$
where $K: \mathbb{R}^2 \to \mathbb{R}$ is an authored reconstruction kernel.

Under Earthcall's `Singularity/OntoMath/` architecture, $K$ is not an opaque GPU filter mode; it is an authored mathematical function:
- **Nearest-Neighbor (Step Field):**
  $$K_{0}(x, y) = \Pi(x) \Pi(y), \quad \text{where } \Pi(t) = \begin{cases} 1 & \text{if } |t| \le \frac{1}{2} \\ 0 & \text{otherwise} \end{cases}$$
- **Bilinear (Piecewise Linear Field):**
  $$K_{1}(x, y) = \Lambda_0(x) \Lambda_0(y), \quad \text{where } \Lambda_0(t) = \max(0, 1 - |t|)$$
- **Catmull-Rom Cubic Spline (Continuous $\mathcal{C}^1$ Field):**
  A cubic piecewise polynomial over intervals $|t| \in [0, 1]$ and $|t| \in [1, 2]$, represented via `OntoMath::Piecewise`.

By lifting the discrete samples into an analytic OntoMath expression, $\Phi_I(u, v)$ becomes directly differentiable, integrable, and composable with other authored fields (such as procedural Perlin noise, analytical gradients, and distance transforms).

---

## 3. Mathematical Partitioning: Sub-Regions as Indicator Fields

### 3.1 Region Indicator Functions
Zachary Zhang articulated that modifying smaller parts of an image must be achieved by gathering properties into smaller Singulars that encompass smaller pixel regions.

Mathematically, a sub-region $R_m \subset \Omega$ is defined as a subset of the unit square. In Earthcall, this subset is represented by its **characteristic indicator function**:
$$\chi_{R_m}: \Omega \to \{0, 1\}$$
$$\chi_{R_m}(u, v) = \begin{cases} 1 & \text{if } (u, v) \in R_m \\ 0 & \text{if } (u, v) \notin R_m \end{cases}$$

Rather than storing a binary mask bitmap (which would require $W \times H$ bits of memory per region), Earthcall represents $\chi_{R_m}$ through two primary OntoMath modalities:

#### 1. Piecewise Box & Interval Decompositions
When regions correspond to rectilinear patches, tiles, or coordinate cuts:
$$\chi_{R_m}(u, v) = \mathbf{1}_{[u_{\min}, u_{\max}]}(u) \cdot \mathbf{1}_{[v_{\min}, v_{\max}]}(v)$$
This maps 1:1 to `OntoMath::Piecewise` structures, as verified in `tests/law/basic_pixel_changer_test.cpp`:
```cpp
OntoMath::Piecewise selector;
selector.inputVariable = "u";
OntoMath::Piecewise::Piece leftQuarter;
leftQuarter.hasHi = true;
leftQuarter.hi = 0.25;
leftQuarter.includeHi = true;
leftQuarter.mathNode = OntoMath::MathNode::fromLegacyExpression(
    OntoMath::ScalarForm::constant(1.0));
selector.pieces.push_back(std::move(leftQuarter));
```

#### 2. Implicit 2D Signed Distance Fields (SDF)
When regions are organic, curved, or boundary-formed (e.g., characters, brush strokes, segmented objects):
$$R_m = \{ (u, v) \in \Omega \mid d_m(u, v) \le 0 \}$$
where $d_m: \Omega \to \mathbb{R}$ is an authored 2D SDF. The indicator function is simply:
$$\chi_{R_m}(u, v) = H(-d_m(u, v))$$
where $H$ is the Heaviside step function.

### 3.2 Partition Properties and Measure Theory
Let $\{R_1, R_2, \dots, R_M\}$ be a set of sub-region Singulars associated with an image Singular $S_0$.
The partition is:
- **Disjoint:** if $R_i \cap R_j = \emptyset$ for all $i \ne j$, satisfying $\chi_{R_i}(u, v) \cdot \chi_{R_j}(u, v) = 0$.
- **Covering:** if $\bigcup_{m=1}^M R_m = \Omega$, meaning $\sum_{m=1}^M \chi_{R_m}(u, v) \ge 1$ for almost all $(u, v) \in \Omega$.
- **Overlapping / Stratified:** when regions represent semantic layers (e.g., background, character, clothing, accessories). In this case, each region carries an authored `zOrder` property, and the composition operator is the standard Porter-Duff over operator:
  $$\Phi_{\text{composite}}(u, v) = \Phi_{\text{top}}(u, v) \oplus \Phi_{\text{bottom}}(u, v)$$

The area (measure) of any sub-region Singular is computed directly in closed form:
$$\mu(R_m) = \iint_{\Omega} \chi_{R_m}(u, v) \, du \, dv$$
This integral can be solved analytically by OntoMath's symbolic calculus engine or approximated via numerical quadrature during property evaluation, eliminating the need to iterate through individual pixels to determine region size.

---

## 4. Continuous Law Mutation and Field Integration

### 4.1 Local Field Modulation
When an authored Law targets a sub-region Singular $S_m$, it modifies the field locally without altering the global macro Singular's base definition.
Let $\Delta \Phi_m(u, v)$ be a localized field perturbation authored by a Law (e.g., a color shift, brightness ramp, or frequency oscillation). The updated global field is:
$$\Phi_{\text{updated}}(u, v) = \Phi_I(u, v) + \chi_{R_m}(u, v) \cdot \Delta \Phi_m(u, v)$$

Because $\chi_{R_m}(u, v)$ vanishes outside $R_m$:
1. **Zero Perturbation Invariance:** For all $(u, v) \notin R_m$, $\Phi_{\text{updated}}(u, v) = \Phi_I(u, v)$ with mathematical certainty.
2. **Predictable Rete Invalidation:** The Rete network only invalidates facts associated with texels in the support of $\chi_{R_m}$:
   $$\operatorname{supp}(\chi_{R_m}) = \overline{\{ (u, v) \in \Omega \mid \chi_{R_m}(u, v) \ne 0 \}}$$
   Texels outside this support are provably unaffected, preventing spurious alpha-node re-evaluations across the rest of the world.

### 4.2 Closed-Form History and Reversibility
Section 6 of `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md` mandates that the past is integrated in closed form rather than replayed from an opaque command log.

Because field modifications are expressed as additive, multiplicative, or functional compositions of OntoMath expressions over $\chi_{R_m}$, undoing an edit is simply the evaluation of the inverse operator:
$$\Phi_{\text{previous}}(u, v) = \mathcal{O}^{-1}(\Phi_{\text{updated}}(u, v), \Delta \Phi_m(u, v))$$
The entire edit history of a region Singular is preserved as an authored sequence of OntoMath transformation nodes, keeping the image fully parametric from its initial ingestion forward.

---

## 5. Conclusion & Forward Guidance

Lifting discrete raster images into continuous OntoMath fields resolves the foundational split between static bitmaps and living ontology. An imported PNG is no longer an opaque block of bytes in GPU memory; it is a continuous, differentiable, and partitionable mathematical object whose sub-regions possess stable identities, authored properties, and direct responsiveness to Person-authored Laws.

For the concrete structural mechanics of how these micro Singulars are linked into graph topologies, see [Companion Analysis 2](PROPERTY_SINGULAR_GRAPHS_AND_RECURSIVE_FORMATION_ONTOLOGY_2026-09-13.md). For substrate performance and copy-on-write memory safety, see [Companion Analysis 3](GRANULAR_PIXEL_MASTERY_SUBSTRATE_EXECUTION_AND_COW_MATERIALS_2026-09-13.md).
