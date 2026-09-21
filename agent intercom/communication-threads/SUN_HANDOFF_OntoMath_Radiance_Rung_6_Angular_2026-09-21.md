# SUN HANDOFF — OntoMath Radiance Rung 6: authored angular emission alpha(p,omega,t)

Date: 2026-09-21  
From: GPT-5.6 Sol ("The Sun")  
PR: #283 — **OntoMath radiance Rung 6: authored angular emission alpha(p,omega,t)**  
Branch: `sol/ontomath-radiance-rung6-angular-20260921`  
Base: `sync-from-earthcall-main` @ `3d167584bc05a229e10225cd26e56e44399859f2`

## What Rung 6 establishes

Rung 6 composes a third independently authored source invariant around the already-landed scalar radiance and chroma fields:

```
rho(p,t) -> scalar
chi(p,t) -> vec3
alpha(p,omega,t) -> scalar

sourceEmissionRGB = rho * chi * alpha
```

Absence of alpha means the exact multiplicative identity:

```
alpha = 1
```

No PointLight / SpotLight / Beam / DirectionalLight ontology enum was introduced. Directionality is authored mathematics.

## Omega constitution

The coordinate convention was written before implementation in:

`docs/plans/ONTOMATH_RADIANCE_RUNG6_OMEGA_CONVENTION_2026-09-21.md`

Canonical meaning:

```
delta = receiverWorldPoint - sourceWorldOrigin
omega = delta / |delta|
```

Thus omega is:

- world-space;
- normalized;
- outgoing source -> receiver;
- represented to authored scalar math as `omega.x`, `omega.y`, `omega.z`;
- admitted only in the angular-radiance expression context;
- not an authored parameter slot.

At the source singularity `|delta| <= kDirectionEpsilon`, omega is undefined. The GPU path contributes zero for a direction-dependent angular sample instead of inventing an axis, camera direction, or cached direction. Direction-independent alpha remains evaluable without omega.

CPU callers bind the same named scalar components explicitly.

## Authored storage and reach

`FieldNode` carries an independent authored `Piecewise` angular expression:

```
light.angular.ast
```

Persistence key:

```
lightAngular
```

Renderer boundary:

- `setRadianceField(expr, revision)` for rho;
- `setRadianceChroma(expr, revision)` for chi;
- `setRadianceAngular(expr, revision)` for alpha.

rho, chi, and alpha have independent content/structural revisions.

## Production WGSL

The existing OntoMath emitter now lowers:

```
fn lightAngular(p: vec3<f32>, omega: vec3<f32>) -> f32
```

The production lighting path derives `sourceDelta` in world space, normalizes it for directional alpha, and composes:

```
shapedRadiance = radialRadiance * angularRadiance
```

Both legacy-chroma and authored-chroma diffuse/specular branches consume that same shaped scalar source term.

An authored omega variable outside the angular expression context refuses.

## Structure/value/time laws

Numeric alpha edit:
- preserves emitted structure;
- refreshes packed parameters;
- reuses the memoized shader.

Structural alpha edit:
- advances angular structure identity;
- recompiles as required;
- does not reinterpret rho or chi.

Timeline advance:
- alpha may read the same explicitly admitted source-side `t`;
- no AST mutation;
- no authored parameter slot for time;
- no shader regeneration solely because the Timeline advanced;
- no authored parameter upload solely because time advanced.

Unsupported alpha refuses; it does not fall back to alpha=1 or stale illumination.

## Witnesses added

### CPU / persistence

`authorable_light_contract_test` now proves:
- `light.angular.ast` is PropertyPath/Law reachable;
- save persists `lightAngular`;
- load restores it;
- CPU evaluation of alpha reads normalized world-space source->receiver omega.

### Compiler / refresh

`sdf_wgsl_parameter_refresh_test` now proves:
- absent alpha has explicit legacy identity;
- authored alpha lowers through production WGSL;
- omega admission is angular-context-only;
- numeric alpha edits preserve WGSL and parameter layout;
- recollected params equal a full compile;
- alpha may read both omega and t;
- unsupported alpha refuses.

### Native WebGPU pixels

`webgpu_object_test` now proves:
- a directional alpha lobe visibly changes received illumination;
- numeric alpha edits change pixels with zero WGSL recompiles and refreshed parameter bytes;
- a rotating lobe

```
alpha = -(omega.x*sin(t) + omega.z*cos(t))
```

changes pixels as a source-owned Timeline advances;
- that Timeline advance requires zero shader compiles and zero authored parameter uploads;
- unsupported alpha produces an explicit angular refusal and no stale illuminated pixel.

## Mandatory companion: fully authored density fields and volumetrics

Do NOT inherit this radiance handoff and leave participating media behind as renderer black boxes.

Earthcall already has generic scalar-field authorship:
- `field.ast`;
- `field.baseDensity`;
- `field.frequency`;
- `field.amplitude`.

Those are PropertyPath/Law reachable. But the live marcher still contains hardcoded medium assumptions including fixed `density * 0.5` extinction and white scatter. Those are unfinished execution fossils.

The roadmap now explicitly requires volumetric density and transport to become first-order authored/property-exposed truth. Target semantic surface:

```
volume.density.ast
volume.extinction.ast
volume.scattering.ast
volume.chroma.ast
volume.phase.ast
volume.emission.ast
```

Exact storage types/names may be refined before implementation, but the invariant is fixed: density, extinction, scattering, medium chroma, and later phase/emission mathematics must be persisted, inspectable, Law-reachable authored state rather than anonymous WGSL constants.

CRITICAL: if a radiant FieldNode is also a participating medium, source radiance `rho` and volumetric density `D` MUST be independent authored invariants. Never make one `field.ast` silently mean both "how strongly this source emits" and "how much medium exists here."

Carry forward the full requirement from:
`docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`
section **Mandatory parallel substrate — authored volumetric density and transport**.

## Current verification state

PR #283 is Ready for Review and GitHub reports it mergeable.

The documentation addenda advanced the PR head after the first CI run had started, so authoritative verification belongs to the newest PR-head workflow, not the superseded earlier run.

At handoff-writing time the newest run is:
- Earthcall focused CI #1965
- head `c26e95a16775400fbf9d55e64819667e401c5d5b`
- status: pending/queued

Do not claim Rung 6 complete until the newest head's SDF/native WebGPU witness job has passed. If a failure appears, inspect the exact failing build/test before changing architecture.

## Next architectural rung after Rung 6

The radiance roadmap's next numbered rung is multiple authored sources:

```
E(p,...) = sum_i rho_i * chi_i * alpha_i
```

World composition belongs above individual source ASTs. Source discovery should become indexed/incremental rather than a permanent O(world) scan, eventually using Prophetic/Rete invalidation to update the relevant source set.

Do not let that next rung postpone the separate volumetric-authorability obligation recorded above.

— GPT-5.6 Sol ("The Sun"), 2026-09-21
