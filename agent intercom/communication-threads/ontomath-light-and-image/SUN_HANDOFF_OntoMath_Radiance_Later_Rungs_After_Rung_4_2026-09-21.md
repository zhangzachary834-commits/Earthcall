# SUN HANDOFF — OntoMath Radiance Later Rungs After Rung 4

Date: 2026-09-21  
From: GPT-5.6 Sol ("The Sun")  
For: the next Sun continuing OntoMath Radiance  
Merged predecessor: PR #273 — **OntoMath radiance Rung 4: relative Timeline input rho(p,t)**  
Base: `sync-from-earthcall-main`

## Read this before touching code

Read:

- `AGENTS.md`
- `docs/BUILD_AND_ENVIRONMENT.md`
- `docs/ENGINEERING_DISCIPLINE.md`
- `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`
- `docs/architecture/ontology/TIME_AND_MOMENT.md`
- `docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`
- `agent intercom/communication-threads/ontology-and-authorship/TO_CONSTITUTIONALIST_Timeline_Relativity_Correction_2026-09-20.md`
- the previous Rung-4 handoff for implementation archaeology only:
  `agent intercom/communication-threads/ontomath-light-and-image/SUN_HANDOFF_OntoMath_Radiance_Rung_4_Time_2026-09-20.md`

Do **not** Big Chungus-read the repository. Use bounded reads and exact searches.

---

# 1. What Rung 4 established permanently

The authored scalar source-strength invariant is now:

```
rho(p,t) -> scalar
```

with `t` optional. Existing `rho(p)` ASTs do not read `t` and retain exactly
their previous meaning.

**Freeze this meaning.**

`rho` means source strength / emission envelope. It does **not** mean:

- chroma/color;
- angular emission;
- visibility/shadowing;
- material response;
- aggregation over sources;
- indirect/global illumination;
- camera response;
- "animated light type."

Future rungs compose new invariants around `rho`; they must not reinterpret or
decompose an old authored `rho` AST.

The durable direction is:

```
source strength    rho
x source chroma    chi
x angular emission alpha
x visibility       V
x material response f_r
+ indirect transport
```

---

# 2. Final Rung-4 temporal semantics — do not regress these

`OntoMath::kTimeVar == "t"` is a canonical **coordinate name**, not a global
clock declaration.

OntoMath does not own a Timeline, clock, scheduler, or temporal frame.

A consuming channel must explicitly admit/bind `t`.

For radiance, the final renderer seam is source-specific:

```
Renderer::setRadianceTemporalCoordinate(t, delta)
```

Do not widen this back into one renderer-global time coordinate. Radiance time
must not silently become material time, geometry time, animation time, receiver
time, etc.

The current production First Mover may still project the broad compatibility
Timeline into the active radiance source until authored Law/Timeline selection
exists. That is compatibility plumbing, not ontology.

The native witness deliberately proves the stronger rule:

```
Timeline --owned-by--> radiance-source Singular
```

The Timeline belongs to the **source process**, not the surface receiving light.

Changing the Timeline coordinate:

- does not mutate the radiance AST;
- does not consume an authored parameter slot;
- does not rewrite the authored parameter buffer;
- does not regenerate WGSL;
- does not change radiance content revision;
- reuses the memoized shader program.

Piecewise applicability over `t` uses the same admitted temporal coordinate.
A temporal expression in a context that did not opt into time must refuse rather
than invent `t=0`.

Geometry still does **not** receive `t` merely because radiance does. Preserve
that channel/context boundary unless CPU geometry and ontology are deliberately
extended together later.

---

# 3. Timeline correction — constitutional invariant

Timeline is a **relative temporal domain** and a `Singular`.

Any Singular may own a Timeline through ordinary Relation truth:

```
Timeline --owned-by--> Singular
```

A lamp, Field, Material, Relation, Person, Zone, or other Singular can
effectively say:

> "I own my own clock."

The broad world clock is only one Timeline at broad scope:

```
world-timeline --owned-by--> Ourverse
```

Engine lifetime/storage is not ontological ownership.

Do not create:

```
AnimationTimeline
RadianceTimeline
PhysicsTimeline
AdapterTimeline
TimelineKind::Radiance
TimelineKind::Local
...
```

Temporal ownership/scope is relational truth, not a C++ taxonomy.

Also: `Timeline::all()` is lifecycle/identity bookkeeping, not automatic global
Law reachability. Do not sweep all live Timelines into Universe merely because
they exist.

---

# 4. IMPORTANT: future Law / Moment / Timeline semantics are NOT this task

Zach and Clawd Opus 5 ("The Constitutionalist") are taking the ontological part
of Law <-> Moment <-> Timeline.

Current `Drive`, `Flow`, `WhileTrue`, `time.sinceApplied`, and existing
`f(t)` action machinery predate first-class Timeline ontology.

Do not constitutionalize those old mechanisms while implementing later radiance
rungs.

Do not decide in this rendering work:

- how a Law creates/selects/owns a Timeline;
- how a conditional process authors start/end Moments;
- how `WhileTrue` changes after Moments/Timelines become first-class in Law;
- how Timelines pause/scale/fork/derive/synchronize;
- whether changing color/glow/field/animation creates a Timeline and exactly how
  the Law relates to it.

Zach + Opus own those choices.

**Hardcoded First Movers are acceptable for focused implementation/tests** when
needed to prove a rung. Keep them visibly test/compatibility scaffolding and do
not mistake them for final ontology.

---

# 5. Start at Rung 5 — separate authored chroma

## Goal

Add a separate authored source chroma invariant:

```
chi(p,t) -> vec3
```

Compose:

```
emissionRGB(p,t) = rho(p,t) * chi(p,t)
```

Do **not** widen `rho` from scalar to vec3.

Old content defaults to the current authored/legacy light color:

```
chi = constant light.color
```

so an old source still means exactly what it meant before.

## Architectural requirements

- Reuse OntoMath. Do not create a second "lighting expression" language.
- Chroma is source-side authored mathematics, distinct from receiving material
  color/BRDF.
- `chi` may deliberately opt into the same canonical `t` coordinate, but
  through a **chroma/radiance-source-specific binding**, not a generic renderer
  time variable.
- A numeric chroma edit should refresh numeric GPU data without WGSL recompilation
  when structure is unchanged.
- A structural chroma edit may invalidate/recompile.
- Existing `rho` program/layout behavior must remain independently observable.
- Unsupported vector math must refuse explicitly rather than fall back to white,
  black, or stale shader state.

## Suggested implementation shape

Before inventing a new container, inspect whether existing authored
`OntoMath::VectorField`, vector `MathNode`, or another existing property slot
can hold `chi` truthfully.

Prefer:

```
source owns/reaches:
  rho : scalar authored expression
  chi : vector authored expression
```

over encoding color inside scalar rho or introducing renderer presets.

If persistence currently has only `light.color`, treat that as the default
constant chroma and add the richer expression in a backward-compatible way.

## Required Rung-5 witnesses

At minimum prove:

1. Existing saved/static source without `chi` produces the same result using
   constant legacy color.
2. `rho=constant`, `chi(p)` spatially changes RGB while scalar intensity
   structure remains unchanged.
3. `rho(p,t)` and `chi(p,t)` can vary independently.
4. Advancing source Timeline changes a timed `chi` without shader recompilation.
5. Numeric chroma edits refresh data without structural compile.
6. Structural chroma edits do compile once.
7. Unsupported chroma math refuses without stale output.
8. The receiving Object/material does not become the owner of source chroma time.

Do not proceed to Rung 6 until that separation is undeniable.

---

# 6. Rung 6 — angular emission

## Goal

Add another independent source invariant:

```
alpha(p, omega, t) -> scalar
```

and compose:

```
sourceEmissionRGB = rho(p,t) * chi(p,t) * alpha(p,omega,t)
```

Default:

```
alpha = 1
```

Do not encode "point", "spot", "directional", "beam", "fan" as ontology enums.

Those are authorable mathematical shapes of angular emission.

## Stop before implementation and define omega precisely

The next Sun must write down the coordinate convention before coding:

- Is `omega` source-local outgoing direction?
- world-space?
- normalized?
- represented as a vector variable or named scalar components?
- what happens at a source singularity / zero-length vector?
- how does CPU evaluation bind it?
- how does WGSL bind exactly the same semantics?

Do not let GPU convention outrun CPU convention.

## Witnesses

Prove at least:

- old source with no alpha remains identical via `alpha=1`;
- authored cone/lobe changes with direction;
- rotating alpha via admitted source Timeline changes pixels without structural
  recompilation;
- CPU and WGSL agree on omega convention;
- unsupported/missing omega context refuses.

---

# 7. Rung 7 — multiple authored sources

## Goal

Each source retains its own:

```
rho_i
chi_i
alpha_i
Timeline / temporal binding as appropriate
```

World composition happens **above** an individual source AST:

```
E(p,...) = sum_i E_i(p,...)
```

Do not make one `rho` expression enumerate the world.

## Source discovery

Use existing ontology:

- Relations;
- Formations;
- Zone membership;
- authored ownership;
- existing source-bearing FieldNodes / source Singulars.

Do not invent a renderer-owned master light list as ontology.

The first implementation may use a simple First Mover collection if needed to
prove composition, but production discovery must become incremental/indexed.

Eventually Prophetic/Rete invalidation should tell rendering when the source set
changed; do not perform an O(world) blind traversal every pixel/frame as the
final design.

## Witnesses

- one-source world exactly matches previous rung;
- two sources sum independently;
- editing source A does not recompile source B unnecessarily;
- destroying/removing a source updates composition truthfully;
- per-source Timelines can differ;
- adding/removing source membership invalidates the source set, not every AST.

---

# 8. Rung 8 — visibility / shadows

## Goal

Visibility is derived transport:

```
V(source, p, omega) in [0,1]
```

Then:

```
direct = sourceEmission * V
```

Do not mutate `rho`, `chi`, or `alpha` when geometry blocks a light.

Source mathematics answers **what is emitted**.

Visibility answers **whether the path reaches the receiver**.

Acceleration structures, ray marching, BVHs, shadow maps, caches, etc. are
execution strategies. They do not become ontology.

Default compatibility path:

```
V = 1
```

when visibility/shadows are unavailable or deliberately disabled.

## Witnesses

- same source AST before/after an occluder remains byte/structure identical;
- moving occluder changes V, not source authored revision;
- disabling visibility recovers exact pre-Rung-8 direct lighting;
- stale visibility caches refuse/invalidate rather than lie.

---

# 9. Rung 9 — authored material response

## Goal

Separate receiving-surface response from source emission.

Eventually express something in the family of:

```
f_r(material, normal, incomingDir, outgoingDir, ...)
```

using OntoMath wherever it can truthfully represent the authored mathematics.

Do not pull material terms into `rho`, `chi`, or `alpha`.

Current ambient/diffuse/specular properties are compatibility state. Preserve
them through defaults/mapping while richer authored response appears around them.

Important source/receiver separation:

```
source side:
  rho
  chi
  alpha

transport:
  V

receiver side:
  f_r
```

Witness two different materials under the same source and prove source ASTs do
not change.

---

# 10. Rung 10 — indirect transport / GI

Only begin after source emission, visibility, and material response are cleanly
separate.

Conceptually:

```
L = L_emitted + T[L]
```

Possible execution strategies:

- path tracing;
- probes;
- caches;
- radiosity-style solvers;
- other numerical approximations.

These are implementation strategies, not new ontology kinds.

GI consumes the existing authored source invariants. It must never require
rewriting old `rho` ASTs.

Disabling indirect transport should recover the direct-light model exactly.

Do not jump to GI early by smuggling bounce/occlusion/material meaning into rho.

---

# 11. Rung 11 — spectral extension only if actually needed

If RGB becomes insufficient, extend chroma/spectral meaning rather than source
strength.

Conceptually:

```
chi(p,t,lambda)
```

or another explicitly documented spectral field.

Keep:

```
rho = source magnitude
chi = chromatic/spectral distribution
```

Do not migrate scalar rho into a vector/spectral AST.

This rung is optional. Do not implement merely because the roadmap names it.

---

# 12. Cross-rung engineering law

For every later rung, preserve the Rung-3/Rung-4 structure/value split:

**Numeric edit**
- refresh parameters/data;
- do not regenerate WGSL when emitted structure is unchanged.

**Structural edit**
- invalidates the structural identity;
- recompiles as necessary.

**Runtime coordinate advance**
- changes ambient inputs/uniforms;
- does not mutate authored AST;
- does not count as authored parameter upload;
- does not force shader recompilation.

Every unsupported authored expression must **refuse**. Never silently:

- substitute zero;
- substitute white;
- substitute `alpha=1` when the author actually supplied unsupported alpha;
- reuse stale compiled output after an invalid edit;
- reinterpret one channel's coordinate through another channel's binding.

Defaults are allowed only when the richer invariant is genuinely **absent**, not
when authored mathematics is present but unsupported.

---

# 13. Compatibility table

The next Sun should keep this table true:

| New capability | Old-source default |
|---|---|
| time | old rho simply does not read t |
| chroma | constant legacy light.color |
| angular emission | alpha = 1 |
| multiple sources | one-element sum |
| visibility | V = 1 when disabled/unavailable |
| material response | compatibility mapping of existing material/light properties |
| indirect transport | 0 indirect contribution when disabled |
| spectral | RGB/default chroma path |

If a rung requires opening an existing `rho` AST and deciding which nodes
"really meant" color, shadows, BRDF, direction, etc., stop: the implementation
has violated the compatibility law.

---

# 14. Rung-4 verification truth inherited by later work

PR #273 merged on 2026-09-21.

Before base reconciliation, the exact Rung-4 head completed all three Earthcall CI
jobs successfully:

- Focused CPU tests — success;
- SDF range-proxy verification — success;
- Slow Adapter independent clock — success.

After bringing the latest base into the PR:

- GitHub reported the branch mergeable and behind the base by 0;
- post-merge SDF range-proxy verification passed;
- `timeline_test` passed in the post-merge Focused CPU run;
- the Focused CPU job's sole failure was `synthesis_studio_living_test`, which
  was independently confirmed to fail on the base itself before the Rung-4
  merge.

Do not spend the next radiance rung "fixing Rung 4" because of that inherited
Synthesis Studio failure unless new evidence directly implicates the radiance
changes.

---

# 15. Recommended next-Sun execution order

Start a fresh branch from the post-merge base.

Then:

```
1. bounded audit of current source-radiance ownership/persistence
2. freeze the exact storage/API shape for chi
3. implement CPU authored chi
4. implement WGSL/vector lowering using existing OntoMath
5. compose emissionRGB = rho * chi
6. add legacy constant-color fallback
7. add structural-vs-numeric refresh witnesses
8. add source-owned Timeline timed-chi witness
9. add native pixel witness
10. document only what the implementation actually proves
11. open PR as draft
12. run authoritative CI
```

Do not start Rung 6 in the same PR unless Rung 5 is tiny and independently
proved. Prefer one invariant per rung/PR.

---

# 16. What the next Sun should NOT do

Do not:

- resurrect `Time.h/.cpp`;
- make Timeline synonymous with global time;
- create timeline-kind enums/classes;
- sweep every Timeline into global Universe reachability;
- solve Zach + Opus's future Law/Timeline/Moment constitution;
- make `rho` RGB;
- put chroma into material response;
- put shadow tests inside rho;
- attach source Timeline semantics to the receiving Object;
- expose `t` to every SDF expression context;
- create renderer presets such as `AnimatedLightKind`, `SpotLightKind`, etc.;
- invent shader-only mathematics;
- weaken refusal behavior to make a demo pass;
- claim CI green before the authoritative run actually finishes.

---

# 17. Success condition for the whole roadmap

The goal is not merely prettier lighting.

The goal is that a Person can author each distinct part of light as inspectable,
durable mathematics and ontology, and every later capability composes without
changing the meaning of earlier authored truth.

If the final system can still read a Phase-2 scalar `rho(p)` untouched while
also supporting independent time, color, direction, multiple sources, shadows,
materials, GI, and possibly spectral transport, the architecture held.

That is the invariant to protect.


---

# 18. Addendum — do not strand density fields / volumetrics behind the shader

Zach explicitly requires the later-rung work to preserve and finish the
first-order authorability of volumetric density, not only surface lighting.

Existing substrate:
- `FieldNode::field.ast` is already PropertyPath/Law reachable;
- `field.baseDensity`, `field.frequency`, and `field.amplitude` are already
  registered authored properties;
- WebGPU already has a `fieldEval(p)` seam for scalar density.

That is NOT permission to call volumetrics finished. The current marcher still
contains renderer-owned medium assumptions, including fixed
`density * 0.5` extinction and white volumetric scatter. These must become
authored, persisted, property-exposed mathematics rather than permanent WGSL
constants.

Carry forward the mandatory roadmap clause in
`docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`:
- density `D(p,t)` is independent authored truth;
- production authoring must expose a stable volumetric density PropertyPath
  (target vocabulary `volume.density.ast`, backed by existing ScalarField
  storage where truthful);
- extinction, scattering, volumetric chroma, and later phase/emission functions
  must likewise become independently authored/property-exposed rather than
  shader constants;
- numeric/structural/time invalidation follows the same structure/value laws as
  rho/chi/alpha;
- unsupported medium math refuses;
- source radiance and medium density MUST NOT alias one AST if the same source
  participates as both emitter and medium.

A future Sun must not interpret "radiance roadmap" as authorization to leave
fog/cloud/aura/god-ray mathematics black-boxed in `SdfWgsl`.

— Zach-directed addendum, GPT-5.6 Sol, 2026-09-21
