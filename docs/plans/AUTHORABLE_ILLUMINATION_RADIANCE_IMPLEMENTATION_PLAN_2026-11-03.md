# Authorable Illumination & Radiance — Renderer Deplasticization Implementation Plan

**Date:** 2026-11-03  
**Branch:** `sol/authorable-illumination-radiance-20261103`  
**Architectural direction:** Zach  
**Written by:** GPT-5.6 Sol  
**Status:** implementation plan; no completion claim

## Implementation ledger — 2026-11-03 first cut

Started on this branch.

Implemented:
- canonical receiver variable `surfaceColor` in OntoMath vocabulary;
- material-response admission of `surfaceColor : Vector`;
- WGSL response lowering binds `surfaceColor` only in the receiver context and refuses it elsewhere;
- `materialResponseEval` now receives pigment explicitly;
- authored response now owns final reflected RGB in the SDF surface path instead of being obligatorily multiplied by pigment afterward;
- one-source and multi-source paths use the same new response call;
- focused compiler witnesses cover pigment admission, authority isolation, structure/value behavior, and the sovereign-output seam.

Commits:
- `e30ad00b0c0a42387a6c01539906fe8d72a7d249` — admit surface pigment;
- `5906e14641f2b5b2be41b977768aa49d56b7c5a1` — sovereign reflected RGB;
- `8e7dd68c984d1dc4fd832997798b782aed2a0587` — focused compiler witnesses.

No CI result is claimed yet; GitHub reports no workflow run attached to the current branch head.

**Next dependency discovered:** AD-21's proposed generic `MathNode::Bind(PropertyPath)` is still not implemented in current canonical. Do not replace it with permanent renderer vocabulary such as `materialDiffuse` / `materialShininess`. The compatibility Blinn-Phong expression should become ordinary data only after it can consume ordinary Material properties through a generic binding path (or an existing equivalent that preserves the same structure/value/refusal laws).

---

## Commission

Retire Earthcall's bespoke default 3D Blinn-Phong shader as a renderer-owned law and make surface illumination a consequence of the broader authored radiance / visibility / material-response ontology.

The goal is not to replace Blinn-Phong with another privileged shader model such as GGX. The goal is to make Blinn-Phong, GGX, Lambert, Oren-Nayar, velvet, toon, glass, impossible authored matter, and future response laws all ordinary mathematical artifacts expressible through the same OntoMath substrate.

The renderer may choose execution strategy. It must not choose material metaphysics.

---

# 1. Current fossil

Current canonical already contains most of the intended Rung-9 seam:

- `Material.responseExpr` exists and persists as authored OntoMath.
- `inspectResponseExpression(...)` / `collectResponseParams(...)` distinguish structure from numeric values.
- WebGPU SDF code can emit `materialResponseEval(p,n,wi,wo)`.
- response edits already have independent revision authority.
- source strength `rho`, chroma `chi`, angular emission `alpha`, visibility, volume transport, and source collections already exist as separate concepts.

But the renderer still owns the old model in several places:

1. **Absent response means a magic renderer identity**
   `<material-response:legacy-blinn-phong>`.

2. **The WebGPU SDF path still hardcodes Blinn-Phong**
   via `L`, `V`, `H`, `diff`, `pow(dot(N,H), shininess)`, and separate ambient/diffuse/specular terms.

3. **The ordinary WebGPU mesh path has its own static Blinn-Phong shader**
   independent from the authored Rung-9 response path.

4. **The current authored response is not sovereign over the whole reflected RGB result.**
   The SDF path still multiplies `surfaceColor * receiverResponse` outside the authored expression, while legacy specular is added separately. That makes an untinted white highlight over colored pigment impossible to reproduce as a general authored response.

5. **Source state is still shaped like Blinn-Phong.**
   `AuthorableLightState` and `RadianceSourceBinding` carry ambient / diffuse / specular channels or coefficients, and `Renderer::setLight` exposes those renderer-shaped categories.

6. **`RenderMaterial` still carries renderer-owned shading coefficients**
   `ambient`, `diffuse`, `specular`, `shininess`.

The architecture is therefore authorable at the leaves while the old shader model still defines the hidden topology of the tree.

---

# 2. Governing equation

Earthcall should converge on the conceptual direct-light form

```
Lo(p, wo)
  = Le_surface(p, wo)
  + sum over sources s [
      Li_s(p, wi)
      * T_s(source -> p)
      * R_material(p, n, wi, wo, authored material state...)
    ]
  + L_volume(...)
  + L_indirect(...)   // later transport rung
```

where:

- `Li` is authored incoming source radiance derived from source invariants such as `rho * chi * alpha`;
- `T` is derived visibility / attenuation / transport truth;
- `R_material` is authored receiver response owned by Material;
- surface emission, participating media, and future indirect transport remain distinct authorities;
- the renderer compiles/evaluates/composes these facts but does not define their aesthetic law.

No privileged `BlinnPhong`, `Lambert`, `GGX`, `BRDFKind`, `LightKind`, or equivalent enum/op is to be added.

If OntoMath lacks a mathematical primitive needed by a response, add only the generic primitive.

---

# 3. Non-negotiable invariants

## 3.1 Authored mathematics, not shader-model kinds

Blinn-Phong must become an authored expression assembled from generic operations such as:

- add / multiply;
- dot;
- normalize;
- max / clamp;
- pow;
- vector construction;
- ordinary PropertyPath-bound values.

The GPU must never branch on a material-response kind.

## 3.2 Receiver sovereignty

Once resolved, a Material response owns the **complete reflected RGB contribution** for the admitted incident radiance.

Do not apply hidden albedo multiplication, hidden white specular, hidden Fresnel, hidden ambient, or another aesthetic term after the authored response.

Pigment / albedo must be an admitted input that the response may use, not a mandatory post-response multiplication imposed by the renderer.

## 3.3 Source / receiver separation

A source emits radiance. It does not emit "diffuse light" or "specular light."

Material response determines how arriving radiance is received.

Therefore source-side ambient/diffuse/specular categories are compatibility fossils to retire, not permanent source semantics.

## 3.4 Compatibility becomes authored data

Old worlds may retain their historical appearance, but no renderer branch may exist solely to preserve it.

Legacy values must be translated/resolved into an ordinary authored compatibility response and, where required, ordinary authored/default source/environment data.

After resolution, old and new worlds execute through the same path.

## 3.5 Mesh and implicit surfaces share response truth

Raster mesh intersection and SDF ray intersection are different ways to obtain a surface interaction:

```
p
n
wi
wo
material
incident radiance
```

They must not define separate lighting systems.

## 3.6 Structural/value invalidation survives

Numeric-only edits to response parameters reuse compiled structure where supported.

Topology/operator/read-set changes invalidate only the relevant response structure.

Source edits must not recompile receiver response merely because illumination changed, and receiver edits must not rewrite source math.

## 3.7 Refusal remains explicit

Unsupported authored response math must refuse. It must never silently retain stale shader code, fabricate zero as if it were authored truth, or re-enter a hidden legacy shading branch.

---

# 4. Semantic correction before deletion

The current Rung-9 response path must be widened before it can replace legacy Blinn-Phong.

Today it behaves approximately as:

```
litRgb = surfaceColor * receiverResponse * incomingDiffuseLikeRadiance
       + hiddenLegacySpecular
```

That is too narrow.

The target is:

```
reflectedRgb = responseExpr(
    p,
    n,
    wi,
    wo,
    surfaceColor,
    authored material bindings...
)

contribution = incomingRadiance * reflectedRgb
```

or an equivalent convention documented precisely by the implementation.

This enables, without renderer privilege:

- colored diffuse + white specular;
- colored metallic reflection;
- spatially varying response;
- grazing-angle effects;
- matte cloth;
- retroreflection;
- stylized/toon response;
- unusual authored matter.

The exact units/convention must be written down in code comments and tests. If the implementation chooses a throughput-like factor rather than a physically normalized BRDF, call it response/throughput and do not falsely claim physical normalization.

---

# 5. Migration phases

## Phase 0 — Freeze witnesses and inventory all hidden Blinn-Phong seams

Before changing semantics, add/retain focused witnesses for:

- current legacy default material output;
- colored surface + white legacy specular highlight;
- matte material with zero legacy specular;
- authored Rung-9 response controlling pixels;
- numeric response edit without structural recompile;
- structural response edit with relevant recompile;
- refusal without stale output;
- mesh and SDF samples under equivalent incident conditions.

Inventory and mark for removal:

- `<material-response:legacy-blinn-phong>`;
- `HAS_AUTHORED_MATERIAL_RESPONSE`;
- hardcoded `L/V/H/diff/spec` branches in `SdfWgsl.cpp`;
- the static Blinn-Phong mesh fragment shader in `WebGpuRenderer.cpp`;
- `inst.shading` / equivalent packed material quartet;
- `RenderMaterial.{ambient,diffuse,specular,shininess}` as renderer semantics;
- `Renderer::setLight(...ambient,diffuse,specular)`;
- `AuthorableLightState.{ambient,diffuse,specular}`;
- `RadianceSourceBinding.{ambientRadiance,diffuseRadiance,specularRadiance,coefficients}`;
- `lightAmbientRadiance/lightDiffuseRadiance/lightSpecularRadiance`.

Do not delete these yet. First make the new route capable of replacing every required behavior.

### Gate 0

A checked inventory exists and baseline witnesses distinguish the exact visual cases the migration must preserve for old content.

---

## Phase 1 — Define a backend-independent Surface Interaction / Response contract

Create or extract one response-lowering interface independent of SDF geometry.

The current response compiler living in `SdfWgsl.*` should be factored or generalized so mesh rendering does not depend on an SDF-specific semantic owner.

Suggested direction:

```
SurfaceResponseLayout
inspectSurfaceResponse(...)
collectSurfaceResponseParams(...)
emitSurfaceResponseWgsl(...)
```

The name is not doctrine; the separation is.

Admitted receiver variables initially:

- `p`: surface-local or documented response coordinate;
- `n`: receiver normal;
- `wi`: canonical incoming transport direction;
- `wo`: outgoing/view direction;
- surface pigment/albedo as a receiver input;
- ordinary authored material parameters through the existing Property/Bind direction when available.

Direction convention must be explicit.

Current Earthcall Rung-9 code uses `wi = source -> receiver`. If retained, every default expression must compensate accordingly; traditional receiver-to-source `L` is `-wi`.

Do not silently reverse the convention between mesh and SDF paths.

### Gate 1

The response compiler can be invoked without SDF geometry ownership and gives identical structure/value/refusal behavior for equivalent expressions.

---

## Phase 2 — Make response output sovereign over reflected RGB

Change response semantics so the authored program can reproduce the full legacy result.

Admit surface pigment/albedo into the response environment.

Do **not** perform mandatory `surfaceColor * response` after the response evaluator.

A legacy-compatible authored expression must be able to encode conceptually:

```
L = -wi
H = normalize(L + wo)

diffuse =
    surfaceColor
    * kd
    * max(dot(n, L), 0)

specular =
    specularColor
    * ks
    * pow(max(dot(n, H), 0), q)
    * step(epsilon, max(dot(n, L), 0))

response = diffuse + specular
```

Historical white Blinn-Phong specular is therefore expressible without a hidden post-step.

The current `ambient` term is handled separately in Phase 5 because ambient is incoming illumination/environment approximation, not fundamentally receiver reflection.

### Gate 2

An authored response reproduces both:

- a colored diffuse surface;
- an untinted/white legacy specular highlight over that colored surface;

without any renderer-side specular branch.

---

## Phase 3 — Material.default owns the compatibility response

Replace the magic absence identity with normal material resolution.

Target rule:

> By the time rendering begins, every visible surface has a resolved response expression.

Save data may omit `responseExpr`, but Material resolution must supply an authored/default expression from `material.default` or an explicit compatibility migration.

The renderer must not interpret null response as "use Blinn-Phong."

Author one Blinn-Phong compatibility response as ordinary OntoMath data.

Historical coefficients may remain temporarily as ordinary Material Properties:

- `ambient`;
- `diffuse`;
- `specular`;
- `shininess`.

But the compiler/renderer must not know their meanings. The compatibility expression references them as authored parameters/bindings.

Longer-term names may be generalized, but this branch need not break old save authoring syntax merely to prove the architectural seam.

### Gate 3

- no-response saved materials resolve to a real response AST;
- the renderer sees no semantic distinction between compatibility response and a newly authored response;
- old material values still round-trip;
- Law/Property edits alter the compatibility response result through ordinary data dependencies.

---

## Phase 4 — Route meshes and implicit surfaces through the same response compiler

The ordinary WebGPU mesh path currently owns a static Blinn-Phong shader. Remove that semantic split.

Two acceptable implementation strategies:

### A. Shared generated fragment body

Compile/cache a response program and insert/use the same generated response evaluator in both mesh and SDF programs.

### B. Shared response module/program cache

If WebGPU pipeline constraints make literal shader sharing awkward, give both paths the same structural response identity, parameter layout, emitter, and generated semantic body, with backend-specific pipeline wrappers only.

In both cases:

- mesh supplies its rasterized world/local position and normal;
- SDF supplies its ray-hit position and gradient normal;
- both form the same canonical `wi`, `wo`, pigment, and incoming-radiance inputs;
- both call semantically identical response math.

Do not fake unification by maintaining two manually copied Blinn-Phong implementations.

### Gate 4

Equivalent mesh and SDF surfaces using the same Material under equivalent incident conditions match within an explicitly justified numerical/pixel tolerance.

Numeric material edits must not cause unnecessary full shader regeneration beyond the existing compiler's structural/value contract.

---

## Phase 5 — Purify source radiance and retire ambient/diffuse/specular light channels

Once receiver response is sovereign, collapse source state toward source truth.

Target source concept:

```
sourceEmission =
    rho(p,t)
    * chi(p,t,...)
    * alpha(p,omega,t)
```

plus source placement, enabled state, Timeline coordinate, and other truly source-owned authored inputs.

Retire source semantics shaped like receiver models:

- ambientRadiance;
- diffuseRadiance;
- specularRadiance;
- ambient/diffuse/specular coefficient triplets.

`light.color` may remain as a compatibility/default constant chroma if that is still the declared absence mapping for `chi`.

`light.intensity` may remain temporarily as ordinary authored source magnitude/default state, but it must not remain because a renderer shader expects a particular coefficient vector. Its eventual relation to `rho` must be explicit and must preserve the already-frozen meaning of authored `rho`.

Do not reinterpret existing `rho` ASTs.

### Environment / ambient compatibility

Legacy "ambient" must not remain a special receiver/source channel forever.

Migrate it to an explicit incoming environment-radiance/default-world contribution or another named authored compatibility source.

During transition, old saves may receive a compatibility environment term that reproduces historical output.

The key rule is:

> ambient compatibility may be generated data; it may not remain a hidden renderer equation.

### Gate 5

A direct source binding no longer contains diffuse/specular semantics.

Changing material response does not mutate source emission state.

Changing source emission does not rewrite material response structure.

---

## Phase 6 — Remove renderer-owned Blinn-Phong

Only after Gates 0–5 pass, delete the old law.

Delete or retire from production semantics:

- `<material-response:legacy-blinn-phong>`;
- `HAS_AUTHORED_MATERIAL_RESPONSE`;
- hidden fallback branches;
- mesh Blinn-Phong shader equations;
- SDF Blinn-Phong equations;
- packed `inst.shading` quartet;
- renderer light ambient/diffuse/specular uniforms;
- `Renderer::setLight` old shaped contract;
- `RenderMaterial` shading quartet if no longer needed by compatibility-data resolution;
- `AuthorableLightState` ambient/diffuse/specular fields once migration no longer requires reading them directly;
- `RadianceSourceBinding` ambient/diffuse/specular fields and coefficient vector;
- helper conversion functions that manufacture those channels.

Search the full repository after deletion for:

```
Blinn
Phong
shininess
ambientRadiance
diffuseRadiance
specularRadiance
lightAmbient
lightDiffuse
lightSpecular
inst.shading
HAS_AUTHORED_MATERIAL_RESPONSE
```

Every remaining occurrence must be one of:

- historical documentation;
- explicit save migration;
- authored compatibility preset/data;
- tests proving migration;
- a deliberately retained legacy OpenGL boundary with a written removal/limitation note.

No production WebGPU illumination equation may remain.

### Gate 6

A source-code audit finds no renderer-side Blinn-Phong equation in active WebGPU surface shading.

---

## Phase 7 — Replace the visual default, separately from compatibility

Architecture migration and aesthetic default selection are separate decisions.

After legacy appearance can be preserved for old worlds as authored compatibility data, define a new `material.default` for newly authored content that does **not** produce the conspicuous plastic Blinn-Phong look.

Initial visual direction:

- mostly matte / diffuse;
- broad, restrained rough reflection rather than a tight white plastic blob;
- environment-aware illumination rather than flat universal ambient;
- no claim that this is the one physically correct or ontologically privileged material model.

If GGX, Oren-Nayar, or another familiar response is used as inspiration, encode it as an authored expression/preset, not a renderer kind.

Suggested authored presets may eventually include:

```
material.response.legacy-blinn-phong
material.response.matte
material.response.rough-dielectric
material.response.metal
material.response.velvet
material.response.toon
```

These are authored artifacts/templates, never engine branches.

### Gate 7

New default objects visibly avoid the historical "everything is polished plastic" appearance while legacy worlds can still intentionally request the compatibility preset.

---

# 6. Bind / PropertyPath direction

The clean end-state is aligned with AD-21:

```
OntoMath::Bind(PropertyPath)
```

or the adopted equivalent.

A response expression should not require the compiler to maintain a growing hardcoded catalog such as:

```
materialDiffuse
materialSpecular
materialShininess
materialFoo
```

Instead, the expression declares dependencies on ordinary authored properties, and lowering maps those dependencies to parameter slots or inlined field expressions according to their type/authority.

This branch may stage the work if AD-21 is not yet complete, but every temporary receiver-specific binding must be designed as a bridge toward generic PropertyPath binding, not a new permanent vocabulary.

Unresolvable bindings must refuse by name.

---

# 7. Cache and invalidation law

Response caching should key on truths already possessed by the ontology/execution path:

- Material identity;
- response structural identity;
- admitted binding layout;
- backend/compiler profile where relevant.

Separate:

1. **structure identity**
   operations, topology, admitted variable reads, bound property layout;

2. **numeric/value state**
   scalar/vector leaf values and bound material values;

3. **incident/source state**
   radiance values and transport state;

4. **surface interaction state**
   p/n/wi/wo per fragment.

A change in (2) should update parameter data without rebuilding (1) where possible.

A change in source radiance must not invalidate receiver structure.

A changed Material response must not rewrite source programs.

Do not key semantic caches by raw pointers or incidental draw order.

---

# 8. Persistence and migration

Old material JSON currently stores legacy coefficients. Preserve read compatibility.

Migration policy:

1. Loading old content may keep the scalar properties so Laws and authoring tools do not break.
2. Missing `responseExpr` resolves to the legacy authored compatibility response, not a renderer branch.
3. Saving may either:
   - persist the resolved compatibility expression explicitly; or
   - preserve omission while Material resolution deterministically supplies the same authored default.
4. Choose one canonical policy and test save -> load -> save stability.
5. Do not silently rewrite authored `rho`, `chi`, or `alpha` ASTs.
6. Do not migrate source emission into material response or vice versa.

If source-side ambient compatibility requires a synthetic environment source, its provenance and deterministic migration rule must be visible.

---

# 9. Tests and tribunals

Minimum focused witnesses before merge:

## OntoMath / compiler

- response with `p/n/wi/wo`;
- response reading pigment;
- generic operations needed by legacy expression;
- numeric-only edit keeps emitted WGSL structure byte-identical where expected;
- structural edit changes identity;
- unsupported operation refuses;
- unresolved binding refuses by name;
- no stale program after refusal.

## Material

- `responseExpr` persistence;
- default response resolution;
- legacy coefficient PropertyPath compatibility;
- copy-on-write preserves response ownership;
- response revision / binding-value revision behaves correctly.

## Source

- source radiance no longer needs ambient/diffuse/specular output channels;
- old `rho` CPU/GPU meaning unchanged;
- old `chi` absence/default meaning unchanged;
- old `alpha` absence remains multiplicative identity;
- source edit does not recompile material response.

## Native WebGPU

- colored diffuse receiver;
- white specular over colored pigment reproduced by authored legacy preset;
- matte response with no specular;
- two receivers under same source differ only by response;
- same receiver under changed source differs without response recompile;
- visibility change alters contribution without rewriting source or response math;
- mesh/SDF parity witness;
- new default visually distinct from legacy plastic preset;
- unsupported response produces explicit refusal / no stale image.

## Regression

Preserve:

- Rungs 3–8 source-radiance witnesses;
- Rung-9 response ownership/invalidation witnesses, updated for no-fallback semantics;
- volumetric V1–V5 / routed transport gates;
- Cathedral / Sunlit Mist / Northern Veil focused tests relevant to radiance transport;
- save/load and Law PropertyPath tests.

Do not declare the branch complete because a standalone compiler test passes. The production mesh and implicit render paths must both be proven.

---

# 10. Performance constraints

The ontology transition must not become a shader-per-object catastrophe.

Required behavior:

- materials sharing structural response identity share compiled response structure/pipelines when backend constraints permit;
- numeric material variation uses parameter data, not source regeneration;
- source changes do not multiply response compilation count by number of surfaces;
- mesh and SDF response compilation share structural identity machinery;
- no per-frame AST serialization or full tree traversal for unchanged materials;
- no recompilation on camera motion merely because `wo` changes;
- no recompilation on source motion merely because `wi` changes;
- no recompilation on Timeline advance when time is a runtime binding rather than structural edit.

Instrument response compiles/cache hits/parameter uploads if existing SDF counters cannot truthfully distinguish the new shared path.

---

# 11. Explicit non-goals

This branch does **not** need to:

- implement full path tracing;
- solve global illumination;
- become spectrally complete;
- enforce physical energy conservation on every authored response;
- provide a giant library of PBR presets;
- delete every old material property on day one;
- redesign the entire Material ontology;
- introduce a `BRDF` class hierarchy;
- make OpenGL feature-equivalent if WebGPU is the canonical production target.

It **does** need to ensure those future features can arrive as authored mathematics/transport without another renderer-owned shader-model throne.

---

# 12. Recommended implementation sequence

```
0. baseline witnesses + fossil inventory
        |
1. extract/generalize response compiler from SDF-only ownership
        |
2. widen response semantics to sovereign reflected RGB
        |
3. author legacy Blinn-Phong as ordinary Material.default compatibility expression
        |
4. route mesh + SDF through one response semantic/compiler path
        |
5. purify RadianceSourceBinding and migrate ambient compatibility to explicit data
        |
6. delete renderer-owned Blinn-Phong and shaped light channels
        |
7. author a restrained non-plastic new default material
        |
8. full native/regression/performance tribunal
```

Do not reverse steps 2 and 6. The old shader may only be deleted after the authored response can reproduce every compatibility behavior that still matters.

Do not reverse steps 3 and 7. Legacy compatibility and the new aesthetic default are separate artifacts.

---

# 13. Completion criteria

This branch is complete only when all of the following are true:

1. No active WebGPU surface path contains a hardcoded Blinn-Phong lighting equation.
2. No null/absent Material response means "renderer chooses Blinn-Phong."
3. Mesh and SDF surfaces consume the same authored response semantics.
4. The response owns the full reflected RGB contribution, including pigment and untinted highlights when authored.
5. Source bindings no longer define incoming radiance as ambient/diffuse/specular channels.
6. Existing authored `rho`, `chi`, and `alpha` retain their frozen meanings.
7. Old visual behavior, where intentionally preserved, is supplied by authored compatibility data rather than a renderer branch.
8. Numeric response edits reuse structure where supported; structural edits invalidate only relevant response programs.
9. Unsupported response math refuses explicitly without stale output.
10. New default material appearance no longer imposes the historical polished-plastic Blinn-Phong aesthetic.
11. Focused native WebGPU witnesses prove both mesh and implicit paths.
12. Existing radiance, visibility, volume-transport, persistence, and Law regressions remain green or any unrelated pre-existing failures are explicitly separated with evidence.

---

# Final architectural maxim

> **A renderer may execute illumination. It may not legislate what matter is.**

Blinn-Phong survives only as an authored historical/material preset if a Person wants it.

Radiance is source truth.  
Visibility/transport is path truth.  
Material response is receiver truth.  
The Screen is their rendered consequence.

That is the branch.
