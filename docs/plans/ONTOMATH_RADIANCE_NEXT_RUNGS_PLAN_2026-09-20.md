# OntoMath Radiance — Next Rungs and Compatibility Law

**Date:** 2026-09-20  
**Architectural direction:** Zach  
**Written by:** GPT-5.6 Sol  
**Status:** roadmap; Phase 2 merged; Rung 3 implemented on `sol/ontomath-radiance-next-rungs-20260920` and awaiting final branch CI evidence

## Purpose

Phase 2 established one authored scalar spatial radiance expression:

```
rho(p) -> scalar
```

where the active Zone's authored `FieldNode::field.astDefinition` reaches WebGPU SDF rendering through the existing OntoMath -> WGSL compiler.

The next rungs must preserve a hard compatibility guarantee:

> Existing authored `rho(p)` ASTs are not temporary encodings to be decomposed later. They are a durable invariant: the scalar source-strength / emission-envelope field.

Future lighting capability must be composed around that invariant rather than reinterpreting, splitting, or migrating it.

This is what makes it safe for Persons and agents to author beautiful scalar lighting fields now.

---

## Compatibility law: freeze the meaning of rho

For a source (s), define:

```
rho_s(p, t?) >= 0
```

as the authored scalar strength of that source at the sampled point.

Today the implemented subset is:

```
rho_s(p)
```

No future rung should change an existing AST's meaning from "source strength here" into any of the following:

- final surface irradiance;
- visibility / shadowing;
- material response;
- camera or eye response;
- aggregation across multiple sources;
- indirect/global illumination;
- color itself;
- a renderer-specific light kind.

Those are separate invariants.

The implementation may later rename internal helper functions for precision, but persisted authored mathematics must retain this semantic meaning.

### Migration rule

A future richer lighting model MUST be able to consume an old Phase-2 source unchanged by supplying identity/default values for every new factor.

Conceptually:

```
old:
    lightContribution(p) = rho(p)

future:
    sourceEmission = rho(p,t) * chroma(p,t,lambda) * angular(p,omega,t)
    directLight    = sourceEmission * visibility(source,p)
    shadedResult   = directLight * materialResponse(...)
    finalRadiance  = direct + indirectTransport(...)
```

For a Phase-2 source:

```
chroma    = authored light.color / legacy constant color
angular   = 1
visibility= 1 when shadows are disabled
indirect  = 0 when GI is disabled
```

Therefore the old `rho(p)` remains intact.

---

## What Gemini may safely author NOW

Gemini can author any effect whose meaning is only:

> "How strong is this source at this point in space?"

Examples that are safe and should require no future migration:

- inverse-square-like or softened inverse-distance falloff;
- concentric halos;
- bright rings or dark rings;
- radial ripples;
- spatial lobes;
- asymmetric spatial falloff using x/y/z;
- shells and bounded luminous regions;
- smooth authored gradients;
- polynomial/signomial patterns;
- supported trigonometric patterns;
- supported OntoMath noise modulation;
- Piecewise regions;
- combinations of the above.

A beautiful authored scalar field can be arbitrarily strange and still survive every rung below.

### What Gemini should NOT bake into rho

Do not encode these into the scalar source field merely to get a visual result:

- shadow tests / occlusion;
- surface normals or diffuse response;
- specular highlights;
- camera direction;
- material roughness/metalness;
- summing other light sources;
- reflection bounces;
- screen-space effects.

Doing so would collapse distinct invariants and create migration pressure later.

---

# Next rungs

## Rung 3 — Truthful visual consequence and authoring witness

Before increasing dimensionality, make the present invariant undeniable in the running world.

Goals:

- provide a known WebGPU SDF witness in the Sun Zone;
- compare the same surface at near/far sample positions;
- make edits to `field.astDefinition` visibly alter the SDF lighting without restart;
- keep the existing parameter-refresh vs structural-recompile split observable;
- expose refusal when an authored operation cannot compile.

This rung changes no ontology. It only strengthens the live Person-facing proof of Phase 2.

**Compatibility:** exact. Existing `rho(p)` unchanged.

### Rung 3 implementation — 2026-09-20

Implemented without widening or reinterpreting `rho`:

- the production OntoMath -> WGSL emitter now exposes an exact scalar-expression layout identity, so numeric authored edits refresh parameter slots while operator/tree/variable edits regenerate WGSL;
- the active radiance layout is inspected once per authored content revision and represented to per-Object SDF memos by a shared structural generation, avoiding an O(objects × AST) walk and duplicated structure strings;
- unsupported authored radiance refuses before stale shader reuse, with read-only `@screen-channel.sdfProgramRefusals` and `@screen-channel.sdfLastProgramRefusal` telemetry;
- the Sun Zone preserves the Phase-2 mesh cube as a control and adds two identical persisted Field/SDF sphere witnesses at camera-visible near/far positions;
- the save-hydration witness proves the two SDFs share geometry/material and that the actual saved `rho` evaluates stronger at the near witness;
- the CPU WGSL witness proves numeric edits preserve emitted structure/parameter layout, structural edits change it, and unsupported math refuses;
- the native macOS `webgpu_object_test` now proves the real Object -> `drawFieldModel` -> `drawImplicit` path: a numeric `rho` edit changes pixels with zero shader compiles, records a cache hit, uploads refreshed GPU parameter bytes, a structural edit recompiles, and an unsupported edit produces an explicit refusal with no stale rendered answer.

The native pixel witness is wired into the existing macOS SDF verification job. Final pass/fail evidence belongs to the branch CI run/PR; this document intentionally does not pre-claim a result.

---

## Rung 4 — Time as an optional input: rho(p,t)

**Implementation status: branch-complete, CI verdict pending.** Branch:
`sol/ontomath-radiance-rung4-time-20260920`.

Introduce a canonical authored time binding to the same OntoMath expression environment.

The model becomes:

```
rho(p, t) -> scalar
```

Existing spatial-only expressions simply do not read `t`.

The canonical temporal-coordinate name is `OntoMath::kTimeVar == "t"`.
OntoMath itself does not manufacture a clock, select a Timeline, or decree what
temporal frame `t` means. Screen/WebGPU accepts only an admitted temporal
coordinate through the Renderer boundary.

Timeline is relative: any Singular may own an independent Timeline through
ordinary Relation truth. The current production First Mover supplies Screen's
default coordinate from the Universe-selected broad Timeline for compatibility,
but the renderer does not know whether the value came from that Timeline, an
Object-owned Timeline, a Field-owned Timeline, or some future Law-selected one.
No wall clock, GPU frame counter, or renderer-local animation time is introduced.

The structure/value rule is preserved:

- `t` is not an authored parameter slot;
- advancing the admitted Timeline coordinate does not mutate the radiance AST;
- advancing the admitted Timeline coordinate does not recollect the SDF/radiance parameter buffer;
- advancing the admitted Timeline coordinate does not regenerate WGSL;
- a structural edit that introduces/removes/rewrites a `t`-reading expression
  still changes emitted shader structure in the normal way.

The Screen binding is intentionally scoped to the radiance expression context.
The first implementation briefly made `t` visible to every SDF-side OntoMath
expression; that was narrowed before review because GPU geometry would then have
gained temporal semantics before the CPU geometry evaluator had the same
binding. Canonical vocabulary is shared, but each channel must opt in
deliberately so one execution path cannot outrun the ontology.

Focused witnesses added on the branch:

- `sdf_wgsl_parameter_refresh_test`: `rho(p,t)` compiles, `t` consumes zero
  authored parameter slots, and generated WGSL reads the shared time uniform;
- `authorable_light_contract_test`: old spatial Sun `rho(p)` evaluates
  identically for different optional `t` bindings, while a CPU OntoMath
  `ValueLeaf(t)` resolves the supplied time exactly;
- native `webgpu_object_test`: the radiant Object owns an ordinary Timeline
  through an `owned-by` Relation; after one structural compile of `rho=t`,
  advancing that Object-owned Timeline changes the rendered pixel while
  requiring zero SDF program compiles, preserving the memoized program, and
  uploading zero authored SDF parameter bytes.

This unlocks:

- breathing/pulsing light;
- traveling ripples;
- rotating mathematical patterns;
- flicker authored as mathematics rather than renderer presets.

Time must be an input variable, not a new "animated light" kind.

**Compatibility:** exact by construction for old ASTs: an expression that does
not read `t` has identical output. Final verification evidence belongs to the
PR/CI run and must not be claimed here before those checks finish.

---

## Rung 5 — Chroma as a separate authored field

Do NOT widen `rho` from scalar to vec3 and thereby reinterpret old content.

Instead add a separate authored color/chroma invariant:

```
chi(p,t) -> vec3
```

and compose:

```
emissionRGB(p,t) = rho(p,t) * chi(p,t)
```

The existing `light.color` property becomes the constant/default chroma when no authored chroma field exists.

This unlocks:

- gradients through space;
- aurora-like color bands;
- rainbow halos;
- color waves independent of intensity waves;
- one intensity shape with many recolorings.

**Compatibility:** exact. Old `rho` is multiplied by a constant color.

---

## Rung 6 — Angular emission as another independent field

Add direction without turning "point / spot / directional" into ontology enums.

Define an authored angular factor:

```
alpha(p, omega, t) -> scalar
```

where `omega` is an explicitly defined direction variable.

Then:

```
sourceEmissionRGB = rho(p,t) * chi(p,t) * alpha(p,omega,t)
```

Defaults:

```
alpha = 1
```

This allows authored cones, fans, beams, lobes, anisotropic emission, and other directional effects without a `LightKind`.

The precise coordinate convention for `omega` must be documented before implementation.

**Compatibility:** exact. Existing sources use `alpha=1`.

---

## Rung 7 — Multiple authored sources as world composition

Do not make one radiance AST learn how to enumerate the world.

Each source keeps its own invariants:

```
rho_i
chi_i
alpha_i
```

The world/Zone provides the collection of source-bearing FieldNodes through existing ontology (Relations/Formations/Zone membership as appropriate).

Direct source emission is aggregated:

```
E(p,...) = sum_i E_i(p,...)
```

The aggregation mechanism belongs above an individual source AST.

Important performance requirement:

- source discovery must become incremental/indexed rather than a blind full-world traversal;
- Prophetic/Rete invalidation should eventually tell rendering when the relevant source set changed.

**Compatibility:** exact. A one-source world is the one-element sum.

---

## Rung 8 — Visibility and shadows are derived transport, not authored source strength

Introduce:

```
V(source, p, omega) in [0,1]
```

as a derived visibility/occlusion term produced from geometry.

Then:

```
direct = sourceEmission * V
```

Do not modify or rewrite `rho` when something moves in front of the source.

This separation is essential:

- source mathematics says what the source emits;
- geometry says whether a path is blocked.

The renderer may accelerate visibility however it wants, but the acceleration cannot become ontology.

**Compatibility:** exact. When shadowing is unavailable/disabled, `V=1`.

---

## Rung 9 — Material response becomes its own authored mathematics

The current legacy ambient/diffuse/specular vocabulary is useful compatibility state, but it must not become part of the source field.

Introduce a surface/material response invariant, eventually capable of expressing something like:

```
f_r(material, normal, incomingDir, outgoingDir, ...)
```

The exact representation should reuse OntoMath where possible and remain independent of source emission.

The transition should preserve current authored material/light properties as defaults or a compatibility mapping.

Do not migrate Gemini's `rho` fields into BRDF logic.

**Compatibility:** source ASTs unchanged.

---

## Rung 10 — Indirect radiance / global illumination as a transport operator

Only after source emission, visibility, and material response are distinct should Earthcall add indirect transport.

Conceptually:

```
L = L_emitted + T[L]
```

where `T` is the transport induced by geometry and material response.

Possible implementations may include path tracing, probes, caches, radiosity-like approximations, or other numerical strategies.

Those are execution strategies, not ontology.

The authored source invariant stays:

```
rho_s(...)
```

GI consumes it. GI does not replace it.

**Compatibility:** exact. Disabling indirect transport recovers the direct-light model.

---

## Rung 11 — Spectral / wavelength dimension, only if Earthcall needs it

If RGB eventually becomes too narrow, add wavelength/spectral meaning without rewriting scalar source strength.

Conceptually:

```
chi(p,t,lambda)
```

or another explicitly documented spectral field.

Again:

```
rho = source magnitude
chi = spectral/chromatic distribution
```

Do not turn old scalar radiance ASTs into vector/spectral ASTs by migration.

**Compatibility:** scalar intensity field remains unchanged.

---

# The decomposition boundary

There IS more decomposition ahead, but it should be decomposition of the *larger lighting equation around rho*, not decomposition of the current authored rho AST itself.

That distinction is the reason Gemini can safely author scalar source effects now.

The durable factorization is:

```
source strength    rho
x source chroma    chi
x angular emission alpha
x visibility       V
x material response f_r
+ indirect transport
```

Each term answers a different question.

If a proposed implementation requires taking an existing authored `rho` AST apart and deciding which nodes "really meant color", "really meant shadow", or "really meant BRDF", the proposal violates this roadmap.

---

# Gemini Light Wizard contract

Gemini may be asked to author custom Phase-2 lighting now under this contract:

1. Work only in the existing authored source-strength `field.astDefinition`.
2. Treat its output as scalar source strength at position.
3. Use only OntoMath operations already supported by both CPU and WGSL paths.
4. Do not add shader-only equations or a second expression language.
5. Do not encode visibility, surface/material response, camera response, or other sources into the field.
6. Preserve First-Mover authorship/provenance when editing saves.
7. Add a CPU evaluation witness and, when practical, a WebGPU compiler/parameter witness for new operator combinations.
8. Prefer parameter changes over structural rewrites when an effect can be exposed as authored numeric values.
9. If the desired effect fundamentally needs color-varying, angular, visibility, material, or GI semantics, stop at the relevant future rung rather than smuggling it into `rho`.

Under that contract, the authored scalar lighting work is intended to survive the future lighting architecture unchanged.

---

# Implementation order

Recommended order:

```
Phase 2 merged
  -> Rung 3 live SDF visual witness
  -> Rung 4 canonical time binding
  -> Rung 5 chroma field
  -> Rung 6 angular emission
  -> Rung 7 multiple sources
  -> Rung 8 visibility/shadows
  -> Rung 9 authored material response
  -> Rung 10 indirect transport / GI
  -> Rung 11 spectral extension if needed
```

Do not jump to GI by making `rho` secretly answer transport questions.

The architectural objective is not merely prettier light. It is a lighting system where every new capability adds one inspectable invariant while preserving the mathematics Persons already authored.
