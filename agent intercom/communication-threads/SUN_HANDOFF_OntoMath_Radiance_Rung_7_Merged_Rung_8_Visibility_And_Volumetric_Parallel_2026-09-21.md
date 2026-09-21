# SUN HANDOFF — OntoMath Radiance Rung 7 merged; next: Rung 8 visibility + mandatory volumetric parallel substrate
**Date:** 2026-09-21  
**From:** GPT-5.6 Sol (“The Sun”)  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Canonical base:** `sync-from-earthcall-main`  
**Rung 7 PR:** #290 — “Rung 7: compose multiple authored radiance sources”  
**Merge commit:** `b407ac6d059574b789ebdaba0c8689cccf5fd071`

---

## 0. Read this first

Do **not** start over.

Rungs 3–7 are now a continuous architecture:

- Rung 3: authored spatial radiance magnitude `rho(p,t)`
- Rung 4: admitted source-relative Timeline coordinate `t`
- Rung 5: authored source chroma `chi(p,t)`
- Rung 6: authored angular emission `alpha(p,omega,t)`
- Rung 7: multiple independently authored sources composed by the world above the individual source AST

The current direct-source invariant is conceptually:

```
E_i(p, omega, t) = rho_i(p,t) * chi_i(p,t) * alpha_i(p,omega,t)
E_total = sum_i E_i
```

Rung 7 is merged. Preserve it.

The next numbered radiance rung is **Rung 8 — visibility/shadows as derived transport**.

Volumetric density/transport is **not** Rung 8 in the canonical plan. It is a **mandatory parallel substrate** recorded at the bottom of the same roadmap. Treat it as first-class architecture, not optional visual polish.

Canonical roadmap:
`docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`

---

## 1. Merge verification already performed

PR #290 is merged into `sync-from-earthcall-main`.

At merge verification time:

- PR #290 state: merged
- merge commit: `b407ac6d059574b789ebdaba0c8689cccf5fd071`
- `sync-from-earthcall-main` pointed at that merge commit
- merge footprint was bounded to the expected Rung 7 files plus its CI witness wiring
- no wholesale renderer overwrite was observed

The merged tree was explicitly checked for:

- repaired `Renderer` private source storage
- clean includes (no literal escaped-newline transplant fossils)
- multi-source discovery in `EngineRender`
- additional Zone-owned `FieldNode` source beings
- persistence for additional spatial fields
- Universe/Law reachability for those FieldNodes
- WebGPU source storage binding
- additive WGSL aggregation
- coexistence with the newer SDF range-proof machinery
- non-mangled `flushSdfDraws()`
- CPU compiler witness
- native WebGPU red+blue two-source pixel witness

Important status note: **the post-merge macOS CI run was still queued at handoff-writing time.**
Do not claim post-merge CI green until you re-check it. The implementation/merge graph looked structurally good; final runtime verification still belongs to CI.

---

## 2. What Rung 7 actually changed

### 2.1 No new Light ontology

There is still deliberately no:

- `LightKind`
- giant C++ `Light` class
- “world enumeration inside one radiance AST”
- blind per-frame O(all Objects) light search

A radiant source remains an authored `FieldNode` with ordinary world identity.

`RadianceSourceBinding` is renderer-facing projection only:

`src/Singularity/Screen/RadianceSource.hpp`

It is not ontology.

### 2.2 Zones can own additional authored FieldNodes

The historical `spatialRoot()` remains the canonical Zone field and exact one-source compatibility source.

Zones can now additionally own genuine FieldNodes through:

- `Zone::additionalSpatialFields()`
- `Zone::addSpatialField(...)`
- `Zone::clearAdditionalSpatialFields()`

Those fields:

- belong to the Zone
- enter Formation membership
- persist
- survive Zone copying
- are exposed to Universe/Law reachability
- can independently carry `rho / chi / alpha`

### 2.3 Source plurality belongs above an individual AST

`EngineRender` gathers candidate FieldNodes from the Zone’s direct field index.

One source:
- explicitly keeps the historical Rungs 3–6 path

Two or more sources:
- project a collection of `Rendering::RadianceSourceBinding`
- WebGPU SDF transport sums all admitted sources

This is the constitutional point:

```
individual source AST != world
world composition = collection of source invariants
```

### 2.4 WGSL source composition

The OntoMath → WGSL compiler can emit per-source functions conceptually like:

```
rho_0, chi_0, alpha_0
rho_1, chi_1, alpha_1
...
```

and sum their lighting contributions above those functions.

Multi-source WGSL uses a source storage binding:

```wgsl
@group(0) @binding(2) var<storage, read> RS: array<RadianceSourceData>;
```

Per-source relative time can bind through:

```
RS[i].time.x
```

rather than requiring one metaphysically global shader time.

### 2.5 Structural/value split survives

The intended invalidation law is:

**Shader structure**
- source count / membership shape
- emitted `rho` structure
- emitted `chi` structure
- emitted `alpha` structure
- angular omega-read structure

**Values**
- source position
- source enablement
- source color/compatibility coefficients
- source relative Timeline coordinate
- numeric AST coefficients when emitted structure is unchanged

Numeric edits should refresh data without shader regeneration.

Structural edits may regenerate WGSL.

### 2.6 Rung 7 and the SDF range-proof renderer coexist

While Rung 7 was being forged, the base renderer advanced substantially.

The final merged implementation was checked to retain both:

- Rung 7 multi-source source-buffer logic
- current SDF range-proof / proof-bit-grid traversal and persistent proof storage

Do not “simplify” one by deleting the other.

---

## 3. Existing Rung 7 witnesses

### Compiler / cache witness

`tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`

Rung 7 section proves, among other things:

- two-source compilation succeeds
- dedicated source storage binding exists
- sources retain independent `rho` functions
- additive aggregation occurs above source invariants
- numeric edit in one source leaves WGSL byte-identical
- parameter recollection matches full compile
- structural edit changes WGSL
- source-relative `t` can lower to the source’s own `RS[i].time.x`

### Native WebGPU composition witness

`tests/singularity/webgpu_object_test.cpp`

Rung 7 section creates:

- source 0: red
- source 1: blue
- both illuminating the same receiver

It checks both channels contribute simultaneously.

It then disables the blue source and verifies:

- blue contribution disappears
- red remains
- no shader recompile occurs
- OntoMath parameter bytes are not rewritten merely because enablement changed

### Persistence witness

`tests/zones/zone_spatial_field_roundtrip_test.cpp`

This was extended to cover additional Zone-owned FieldNodes and their persistence.

Re-check that this test is actually wired into whatever CI gate you rely on. Do not count the existence of a test file as execution.

---

# 4. NEXT NUMBERED RUNG — RUNG 8: VISIBILITY / SHADOWS

Canonical roadmap says:

```
V(source, p, omega) in [0,1]
direct = sourceEmission * V
```

Visibility is **derived transport**, not authored source strength.

This distinction is non-negotiable.

If geometry moves between a source and receiver:

- do **not** mutate `rho`
- do **not** mutate `chi`
- do **not** mutate `alpha`
- do **not** encode “blocked” into the source AST

The source still emits what it emits.

Geometry answers whether that emitted radiance reaches the receiver.

---

## 5. Recommended Rung 8 implementation strategy

### 5.1 Preserve the source invariant

Keep:

```
E_i = rho_i * chi_i * alpha_i
```

Then derive:

```
direct_i = E_i * V_i
direct_total = sum_i direct_i
```

where `V_i` is transport truth computed from geometry.

### 5.2 Start with a truthful exact baseline

Before optimization, establish one exact shadow/visibility witness.

For each source and surface hit:

1. obtain receiver point `p`
2. obtain source direction and finite source distance when appropriate
3. trace/query geometry along the segment/ray
4. return:
   - `V=1` if unobstructed
   - `V=0` if blocked
   - later, fractional visibility only when Earthcall has an actual reason for it

Do not hide approximation behind the meaning of `V`.

### 5.3 Acceleration may consume existing spatial proof machinery

The renderer now has conservative SDF range-proof infrastructure.

Investigate whether visibility rays can reuse safe portions of:

- SDF range hierarchy
- proved-positive outside cells
- proxy / empty-space skipping
- existing exact SDF evaluation

But:

**the acceleration is not ontology.**

A proof cache may accelerate the answer to `V`; it must never become the authored meaning of visibility.

Unknown proof state must fall back to exact transport, not to “visible” or “blocked.”

### 5.4 One-source compatibility

With shadows disabled/unavailable:

```
V = 1
```

This must recover the exact current Rung 7 direct-light behavior.

Make that an explicit parity witness.

### 5.5 Multi-source independence

A blocker between source 0 and the receiver must not suppress source 1 unless source 1’s own path is blocked.

Required visual witness:

- red source left
- blue source right
- one blocker that occludes only one source
- receiver retains the unobstructed source’s contribution

This is the Rung 8 analogue of the Rung 7 red+blue composition witness.

### 5.6 Do not fake soft shadows yet

Do not jump directly to:

- penumbra heuristics
- percentage-closer filtering as ontology
- stochastic area-light sampling
- GI

First establish the semantic seam:

```
source emission
×
derived geometric visibility
```

Then richer transport can grow without rewriting source mathematics.

---

## 6. Suggested Rung 8 witnesses

At minimum:

1. **Compatibility:** `V=1` reproduces Rung 7 output.
2. **Single blocker:** blocking one source darkens only its direct contribution.
3. **Multi-source independence:** blocker for source A does not erase source B.
4. **Geometry motion:** moving blocker changes pixels without mutating/recompiling source `rho/chi/alpha`.
5. **No stale shadow:** removing blocker restores contribution.
6. **Conservative acceleration parity:** proof/range traversal ON and OFF produce exact same pixels on a camera/blocker corpus.
7. **Unsupported/unknown proof state:** falls back to exact query, never silently chooses visibility.
8. **No ontology leakage:** source save/AST is byte-identical before/after blocker motion.

---

# 7. MANDATORY PARALLEL FRONTIER — AUTHORED VOLUMETRIC DENSITY / TRANSPORT

Zach is explicitly excited for this. Do not lose it behind Rung 8.

The canonical roadmap already declares volumetrics a mandatory companion substrate.

Earthcall currently has useful pieces:

- `FieldNode`
- generic scalar-field AST
- `field.ast`
- `field.baseDensity`
- `field.frequency`
- `field.amplitude`
- WGSL `fieldEval(p)`

But the current marcher still contains renderer-owned medium assumptions such as:

- fixed extinction multiplier resembling `density * 0.5`
- white volumetric scattering

Those are temporary execution fossils.

They are **not** the final ontology of fog/cloud/smoke/aura/gas.

---

## 8. Volumetric constitution

Density must be first-order authored truth:

```
D(p,t) -> scalar
```

Target authoring vocabulary from the roadmap:

```
volume.density.ast       D(p,t)              -> scalar
volume.extinction.ast    sigma_t(p,t)        -> scalar
volume.scattering.ast    sigma_s(p,t)        -> scalar or vec3
volume.chroma.ast        C_v(p,t)            -> vec3
volume.phase.ast         Phi(p,wi,wo,t)       -> scalar
volume.emission.ast      E_v(p,omega,t)       -> vec3
```

Names/storage may be refined, but meanings must remain independent.

Especially:

```
rho_source != D_medium
```

One AST must never silently mean both:

- “how strongly this source emits”
- “how much participating medium occupies this point”

A `FieldNode` that is both radiant and volumetric must be able to author those truths independently.

---

## 9. Conceptual volumetric transport

The renderer may numerically integrate something like:

```
opticalDepth += sigma_t(p,t) * ds
transmittance = exp(-opticalDepth)

inScatter +=
    transmittance
    * sigma_s(p,t)
    * C_v(p,t)
    * Phi(p,wi,wo,t)
    * incidentRadiance
    * ds
```

Execution strategy belongs to the renderer:

- integration step size
- adaptive stepping
- empty-space skipping
- caching
- temporal reuse
- spatial proof traversal

Authored world truth belongs to ontology / OntoMath / PropertyPath:

- density
- extinction
- scattering
- chroma
- phase
- volumetric emission

Do not reverse those layers.

---

## 10. Recommended volumetric staging

Do **not** attempt the entire participating-media equation in one PR.

### V0 — density sovereignty

Goal:

```
volume.density.ast
```

exists as a complete Law/PropertyPath-reachable authored invariant, persisted independently from source `rho`.

Required:

- read whole AST
- replace whole AST
- save/load exactness
- time binding
- structure/value invalidation split
- native WebGPU pixels visibly change

Keep current extinction/scatter compatibility defaults temporarily if needed.

### V1 — authored extinction

Replace renderer fossil:

```
density * 0.5
```

with an authored extinction invariant / compatibility default.

Do not couple extinction to density merely because they are multiplied numerically.

### V2 — authored scattering + volumetric chroma

Remove hardcoded white scattering.

Two media must be able to share the same density shape while scattering differently.

### V3 — phase / angular scattering

This is the medium analogue of source `alpha`.

Do not reuse source `alpha`; scattering phase is a different physical/semantic invariant.

### V4 — emissive media

A medium may itself emit radiance:

```
E_v(p,omega,t)
```

Again: independent from density.

### V5 — multiple participating media

Compose multiple media as world truth above an individual AST, using indexed/incremental discovery rather than permanent O(world) scans.

---

## 11. Required volumetric witnesses before calling it “fully authored”

Copied from the canonical roadmap because these are constitutional:

1. A Law/PropertyPath can read and replace the complete density AST.
2. Save/load restores that density AST exactly.
3. Editing density visibly changes native WebGPU volume rendering.
4. Numeric density edits reuse compiled structure.
5. Extinction/scattering/chroma are not hardcoded WGSL constants.
6. Two differently colored/scattering media can share the same density shape without rewriting that density AST.
7. A timed density/medium expression changes pixels from its admitted Timeline without structural recompilation.
8. Unsupported medium math refuses with no stale or fallback volume.
9. A source that is also a participating medium can author radiance and density independently; changing one does not mutate/reinterpret the other.

Also preserve truthful depth/composition for volumetric-only pixels: a hard surface hit must not be required merely to make a medium visible.

---

# 12. PERFORMANCE / INVALIDATION LAW

For both Rung 8 and volumetrics:

### Do not scan the whole world every frame

Rung 7 already moved source discovery toward direct Zone ownership.

Continue toward:

- indexed source/media sets
- structural revision boundaries
- Prophetic/Rete invalidation
- relevant-change-only rebuilds

### Do not recompile shader structure for value changes

Examples of value-only changes:

- source moves
- blocker moves, if visibility is runtime transport
- Timeline advances
- density numeric coefficient changes
- extinction numeric coefficient changes
- medium color numeric changes

Examples that may require structural regeneration:

- AST operator topology changes
- source/media count changes when statically unrolled
- new angular-variable dependence changes emitted structure

### Beware JSON hashing on the frame path

Audit `EngineRender` source-set identity generation.

Earlier Rung 7 code used `field->toJson().dump()` as part of source-set identity.

That is semantically broad and potentially expensive for rich FieldNodes.

Prefer narrowly scoped revision identities / authored-content revisions rather than serializing entire beings each frame.

Do not “optimize” by weakening correctness; give each authored invariant an explicit content/structure revision instead.

---

# 13. Things NOT to do

Do not:

- invent `LightKind`
- make `rho` contain shadow state
- make `rho` contain BRDF/material response
- make `rho` contain GI
- make medium density and source radiance the same AST
- make visibility authored source truth
- hardcode a permanent `density * 0.5`
- keep permanent white-only volume scattering
- reintroduce full-world per-frame source/media scans
- delete current SDF range-proof work while adding shadow traversal
- weaken existing tests to make a new rung pass
- call queued CI “green”

---

# 14. Immediate next-Sun sequence

1. Re-check post-merge CI for `b407ac6d059574b789ebdaba0c8689cccf5fd071`.
2. If CI exposes a Rung 7 regression, repair it before advancing.
3. Audit current direct-light WGSL seam and current range-proof traversal.
4. Implement the smallest exact `V(source,p,omega)` baseline for Rung 8.
5. Add one-source `V=1` parity + selective multi-source blocker witness.
6. Only then accelerate visibility with conservative existing spatial proof machinery.
7. In parallel, audit current volumetric `fieldEval`, fixed extinction, and white scatter fossils.
8. Write a bounded V0 density-sovereignty implementation plan before editing medium storage.
9. Keep V0 density authoring independent from source `rho`.
10. Hand off with exact PRs, commits, pixels/benchmarks, and unresolved transport assumptions.

---

## 15. The architectural story

Rung 7 changed the grammar of light.

Before it, Earthcall knew how one authored source could speak mathematically.

Now the world can contain a choir of sources without forcing any single voice to pretend it is the universe.

Rung 8 should add the next distinction: a source may speak, yet geometry may stand between speaker and hearer. The obstruction belongs to the path, not to the voice.

And volumetrics open a still deeper realm: space itself can become authored substance — density, extinction, scattering, color, anisotropy, emission — not a renderer fog knob, but mathematical world-truth.

Preserve those separations.

That is how the renderer stops being a pile of effects and becomes an interpretable physics of authored worlds.

— GPT-5.6 Sol, 2026-09-21
