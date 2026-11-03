# Volumetric V0 — Density Sovereignty

**Date:** 2026-09-21  
**Status:** implementation plan / constitutional boundary  
**Branch:** `sol/volumetric-v0-density-sovereignty-20260921`  
**Canonical base:** `sync-from-earthcall-main`

## Implementation status — Density Sun, 2026-09-21/22

V0a, V0b, and V0c are implemented on draft PR #299. The original plan below
remains the constitutional rationale; this status section records what the code
now actually does so later work does not restart the rung.

Landed on the Density Sun branch:

- independent `FieldNode::volumeDensity` and PropertyPath `volume.density.ast`;
- independent save/load and rho/D mutation witnesses;
- explicit `DensityInputKind::{LegacyField,None,Authored}` at the legacy SDF compiler seam;
- `volumeDensityEval(p)` and an independent density temporal coordinate;
- density-owned memo structure/content invalidation;
- Zone-owned `VolumeDensityBinding` projection independent from `light.source`;
- dedicated WebGPU participating-medium pipeline and `Renderer::composeVolumes()`;
- composition after world geometry and before HUD/2D;
- sampling of completed opaque depth to truncate the medium integral;
- no volume `frag_depth` claim;
- actual marched-interval optical-length bookkeeping;
- native volumetric-only and depth-clamped surface+volume witnesses;
- native numeric-D refresh witness;
- native `D(p,t)` Timeline witness proving an independently supplied medium-time coordinate changes pixels with no shader regeneration;
- production currently uses the broad Universe-selected Timeline as the documented compatibility/default binding; per-process Timeline selection remains with the future Law/Timeline architecture, not Screen;
- native unsupported-density refusal witness proving no stale fog survives;
- volume-specific compile/cache/refusal observability.

Prism Sun's isolated reconciliation (#304) was reviewed and merged into #299.
It closes the legacy null-pointer ambiguity: explicit absence can no longer fall
through to generic `field.ast`, so a radiant FieldNode without authored
`volume.density.ast` cannot become fog implicitly.

All eleven §10 witness categories now have executable coverage in the branch.
Focused CI is still the authority for whether this implementation is ready to
leave draft status.

Historical V0 scope note: this original V0 plan intentionally stopped before
extinction/scattering/chroma. Canonical Earthcall has since advanced beyond that
historical boundary: V1 authored extinction and V2 authored scattering + medium
chroma have landed. V3 phase, V4 volumetric emission, and later multiple-medium /
richer transport work remain future rungs. The constitutional V0 reasoning below
is preserved because it defines the sovereignty boundary those later rungs build on.

## 1. Why V0 exists

Earthcall already possesses two individually valuable mechanisms:

1. an authored scalar `FieldNode::field.astDefinition`, reachable through `field.ast`; and
2. an authored radiance source pipeline whose Rung-7 source projection currently treats that same scalar AST as source radiance `rho(p,t)`.

The WebGPU marcher also lowers `FieldNode::field.astDefinition` through `fieldEval(p)` and interprets it as volumetric density.

That means one tree can currently acquire two different meanings merely because two renderer paths look at it:

```
FieldNode::field.astDefinition
        |
        +--> source.radianceExpr  => rho_source(p,t)
        |
        +--> fieldEval(p)         => D_medium(p,t)
```

This is not a naming inconvenience. It is a semantic alias. A being that is both radiant and volumetric cannot independently answer:

- how much radiance do I emit?
- how much participating medium exists here?

V0 exists to break that alias before richer extinction, scattering, chroma, phase, or volumetric emission are built on top of it.

The invariant is:

```
rho_source != D_medium
```

Even when both functions happen to contain identical mathematics, they must be independently authored truths.

## 2. Current renderer fossils this plan must preserve truthfully, then retire later

The current marcher contains:

```wgsl
let density = fieldEval(p);
let extinction = max(density * 0.5, 1e-6);
...
volumetric_scatter += (density / extinction) * (old_t - transmittance);
```

and later composes that scalar scatter as white:

```wgsl
vec3<f32>(1.0, 1.0, 1.0) * volumetric_scatter
```

V0 changes only the sovereignty of **density**. The `density * 0.5` extinction rule and white scattering remain temporary compatibility execution rules until V1/V2 replace them with their own authored invariants.

Do not blur the V0 boundary by pretending those later channels are solved.

## 3. V0 authored surface

A `FieldNode` gains an independent scalar Piecewise channel:

```
volume.density.ast
```

with meaning:

```
D(p,t) -> scalar
```

This channel must be:

- whole-tree readable through PropertyPath;
- whole-tree replaceable through PropertyPath;
- refused atomically on malformed input;
- persisted independently of `field`, `lightChroma`, and `lightAngular`;
- capable of reading an admitted Timeline coordinate;
- independently structurally/value invalidated;
- compilable by the same OntoMath -> WGSL emitter used elsewhere;
- absent by default rather than secretly aliasing source radiance.

An empty `volume.density.ast` means **no explicitly authored participating-medium density channel**. It does not mean that another AST may silently acquire density semantics.

## 4. Compatibility and migration precedence

Compatibility must be explicit, named, and one-way. It must never emerge accidentally because two consumers share a pointer.

### 4.1 Explicit V0 density always wins

When `volume.density.ast` is non-empty, it is the sole authored density authority for that FieldNode:

```
D = volume.density.ast
```

Neither source `rho` nor generic `field.ast` may override it.

### 4.2 A radiant FieldNode never receives implicit density from rho

If a FieldNode is an authored source (`light.source=true`) and no explicit `volume.density.ast` exists:

```
D = 0
```

for the participating-medium channel.

The source may continue to emit through its existing Rung-3-through-Rung-8 compatibility path, but illumination is not fog merely because both are scalar functions.

This is the crucial alias-breaking rule.

### 4.3 Legacy non-radiant density may be migrated deliberately

Older non-radiant FieldNodes may have used the generic `field.ast` or procedural scalar-field parameters specifically as density.

That compatibility must be resolved **before** WGSL code generation through a named legacy-density decision, not by letting `fieldEval()` blindly inspect the generic field pointer.

For a legacy non-radiant field with no explicit `volume.density.ast`, V0 may temporarily project:

```
legacy field mathematics -> resolved density binding
```

The projection is compatibility machinery, not ontology. New saves and newly authored media should use `volume.density.ast`.

A future migration may materialize that legacy density into the explicit V0 channel. It must not rewrite source radiance.

## 5. Storage boundary

The first storage rung should add to `geom::FieldNode`:

```cpp
const std::shared_ptr<OntoMath::Piecewise> volumeDensity;
```

It is intentionally a dedicated `Piecewise`, not a second `ScalarField` container and not an alias of `field->astDefinition`.

Register it using the same whole-tree bridge pattern already proven for `light.chroma.ast` and `light.angular.ast`:

```
volume.density.ast
```

Serialization gains a separate optional key, e.g.:

```json
"volumeDensity": { ... Piecewise ... }
```

No save operation may synthesize this key merely because `field.ast` exists. Explicit authorship must remain distinguishable from legacy fallback.

## 6. Renderer-facing projection

The WGSL compiler should not decide whether an arbitrary generic FieldNode means light, matter, or density.

Resolve that question before compilation and hand the renderer a narrow projection, analogous to `RadianceSourceBinding`.

A V0 projection may contain:

```cpp
struct VolumeDensityBinding {
    const OntoMath::Piecewise* densityExpr;
    uint64_t densityRevision;
    double temporalCoordinate;
    double temporalDelta;
    bool enabled;
};
```

This is renderer-facing data, not a new ontological `MediumKind`.

The binding is the only thing allowed to tell generated `volumeDensityEval(p)` what authored density mathematics to execute.

## 7. WGSL seam

Replace the semantic role of the old generic:

```
fieldEval(p)
```

with an explicitly density-named evaluator:

```
volumeDensityEval(p)
```

whose expression comes from the resolved density binding.

The first V0 transport remains conceptually:

```
D = max(volumeDensityEval(p), 0)
sigma_t_compat = max(D * 0.5, epsilon)
```

The `0.5` is intentionally marked compatibility-only until V1. White scatter is intentionally marked compatibility-only until V2.

The compiler may retain legacy helper names internally during migration, but no final semantic path may infer density from `radianceExpr`.

## 8. Timeline admission

Density is allowed to be time-dependent:

```
D(p,t)
```

The Renderer/WebGPU boundary must carry a temporal coordinate **per medium** and
must never equate density time with radiance time merely because an existing
uniform happens to exist.

Which Timeline supplies that coordinate is intentionally a higher authored
selection question. `TIME_AND_MOMENT.md` establishes that any Singular may own
ordinary Timeline beings through Relations, but the future Law <-> Timeline
architecture still owns the rule for selecting/relating one of those temporal
domains to a particular changing process. Ownership alone is not a channel
selection policy.

Therefore V0 requires:

- an independent runtime density temporal coordinate/value channel;
- native proof that an independently supplied Timeline coordinate drives
  `D(p,t)` without shader regeneration;
- production compatibility may continue to project the Universe-selected broad
  Timeline **until** authored temporal selection semantics land.

V0 must not invent `field.timelineId`, a special VolumeTimeline kind, or an
automatic "first owned Timeline wins" rule inside Screen.

Advancing the admitted coordinate is a **value change**. It must not regenerate
WGSL structure.

## 9. Structural / value invalidation

V0 follows the same recursive interpretation law as radiance:

**Structure**
- AST operator topology;
- branch/piece topology;
- presence/absence of time reads;
- supported/refused operation shape.

**Values**
- numeric constants occupying existing emitted parameter slots;
- admitted Timeline coordinate/delta;
- enablement;
- other runtime scalar values whose emitted topology is unchanged.

Use the production OntoMath emitter itself to derive structural identity. Do not add a hand-written second AST classifier.

A numeric density edit must refresh parameters while preserving WGSL bytes and pipeline identity.

## 10. Required V0 witnesses

V0 is not complete until all of these are executable witnesses:

1. **PropertyPath reachability** — read the complete `volume.density.ast`.
2. **Atomic replacement** — replace the complete AST; malformed input refuses without mutation.
3. **Save/load identity** — serialize and restore the exact density AST.
4. **rho/D independence** — one FieldNode can carry source radiance and density simultaneously; mutating one leaves the other byte-identical.
5. **No implicit radiant fog** — a `light.source=true` FieldNode with rho but no density channel does not become participating medium.
6. **Legacy non-radiant compatibility** — an old field intentionally used as density remains representable through the named migration projection.
7. **Native WebGPU pixel witness** — authored density visibly changes volumetric rendering.
8. **Numeric refresh** — changing only density numeric coefficients changes pixels/parameter bytes without regenerating WGSL.
9. **Timeline witness** — `D(p,t)` changes pixels as its admitted Timeline advances without structural recompilation.
10. **Refusal witness** — unsupported density mathematics refuses rather than silently returning zero.
11. **Volumetric-only depth/composition** — a hard-surface hit is not required merely to make authored medium visible.

## 11. Bounded implementation sequence

### V0a — sovereignty in storage

Touch only the FieldNode authoring/persistence boundary and focused tests:

- add independent `volumeDensity`;
- register `volume.density.ast`;
- persist/restore `volumeDensity`;
- prove PropertyPath replacement and rho/D byte independence.

Do not change renderer semantics in this commit.

### V0b — explicit compiler projection

- add a renderer-facing density binding;
- resolve explicit density vs named legacy compatibility before WGSL generation;
- remove direct density interpretation of `field->astDefinition` from the compiler;
- emit `volumeDensityEval`;
- add independent structure/value layout tracking.

### V0c — runtime and native pixels

- wire the binding through WebGpuRenderer;
- add density Timeline value transport;
- prove numeric/time edits do not regenerate WGSL;
- add native volumetric-only and surface+volume pixel witnesses.

Only after V0 is green should V1 replace fixed extinction and V2 replace white scatter.

## 12. Non-goals

V0 does not:

- invent a `MediumKind`;
- make density a property of a Light class;
- author extinction;
- author scattering;
- author volumetric chroma;
- author phase functions;
- author volumetric emission;
- implement multiple-media composition;
- implement GI;
- reuse source `alpha` as a scattering phase function;
- turn range/proof caches into medium ontology.

## 13. Architectural result

After V0, a radiant fog can say two different true things without one equation impersonating the other:

```
rho(p,t) = how this being emits
D(p,t)   = how much participating substance is here
```

They may be mathematically equal by choice. They may diverge radically. A Law may rewrite one while leaving the other untouched.

That separation is the doorway through which the later medium stack—extinction, scattering, chroma, phase, emission—can become authored physics rather than renderer folklore.


---

## 14. 2026-09-22 full-stack continuation after V0/V1/V2

The original document above is the constitutional V0 density-separation plan. The renderer has since advanced through independent authored extinction and V2 scattering/chroma, so the continuation is now coordinated by:

[`RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md`](RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md)

The medium stack is now planned as one explicit sequence of independent authored meanings:

```text
V0  D(p,t)               density
V1  sigma_t(p,t)         extinction
V2  sigma_s(p,t)         scattering magnitude
V2  C_v(p,t)             medium chroma
V3  Phi(p,wi,wo,t)       phase / angular scattering
V4  E_v(p,omega,t)       volumetric emission
V5+ multiple media and richer transport composition
```

The full renderer plan also connects these medium channels to, without conflating them with:

```text
F(p,...)                  geometry / SDF
rho(p,t)                  source radiance
chi(p,t)                  source chroma
alpha(p,omega,t)          source angular emission
V(source,p,...)           visibility/shadows
material response
indirect transport / GI
Screen presentation
```

### Prophetic / semantic-synthesis integration

The post-V2 performance architecture is not “make density use the SDF proof cache.”

Instead, the shared semantic compiler may provide common machinery:

- canonical mathematical execution;
- common-subexpression sharing;
- authored-source provenance;
- incremental repair;
- runtime value caching;
- reverse proof dependency indexes;
- backend lowering identity.

Each medium meaning keeps its own theorem algebra.

Examples:

```text
D:
    exact zero-density support
    conservative density bounds
    empty interval

sigma_t:
    exact zero extinction
    optical-depth bounds
    identity-transmittance interval

sigma_s:
    exact zero scattering
    scattering support

C_v:
    exact zero component/support where composition permits it

Phi:
    angular support/exclusion

E_v:
    emission support / exact zero emission
```

An interval theorem for one channel has no authority over another.

For example:

```text
D = 0 on [u0,u1]
```

may justify skipping medium occupancy work over that interval, but does not imply:

```text
rho = 0
sigma_t = 0
E_v = 0
```

unless those facts are independently proved.

### First volumetric Prophetic target

The safest first production volumetric optimization remains an **exact zero-density/support interval proof**.

Conceptually:

```text
prove D(p(u),t) = 0 for every u in [u0,u1]
        ↓
skip density/medium sampling over [u0,u1]
        ↓
resume exact transport at u1
```

This must be conservative over the entire interval. A few zero point samples are not sufficient proof.

The next targets are exact-zero extinction/scattering support and conservative optical-depth bounds, always with proof-disabled exact A/B parity.

### Change law

Runtime query state such as camera position, ray coordinate, and admitted Timeline value does not rebuild a theorem when that theorem already quantifies over the changing variable.

Authored premise changes invalidate only the dependent medium proof frontier.

Changing `sigma_s` must not invalidate a `D` theorem unless that theorem explicitly depends on `sigma_s`.

Changing source `rho` must not invalidate medium density merely because the source and medium happen to share identical OntoMath calculation nodes.

### Required cross-domain regression

Before the shared compiler receives production optimization authority, retain a regression where geometry, source radiance, and medium density use identical mathematics.

The expected result is:

```text
shared compiled mathematical identity
+
separate semantic-channel proof authority
```

A density edit may split/rebuild the density execution mapping and invalidate density proofs while leaving geometry/radiance proof overlays valid.

This is the bridge from V0's original sovereignty law to the broader semantic-synthesis architecture.

— full-stack continuation directed by Zach, recorded by GPT-5.6 Sol, 2026-09-22
