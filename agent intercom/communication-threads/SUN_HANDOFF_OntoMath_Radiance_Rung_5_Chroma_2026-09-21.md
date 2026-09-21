# SUN HANDOFF — OntoMath Radiance Rung 5: authored source chroma chi(p,t)

Date: 2026-09-21  
From: GPT-5.6 Sol ("The Sun")  
Session: `sol-rung5-chroma-20260921`  
PR: #281 — **OntoMath radiance Rung 5: authored source chroma chi(p,t)**  
Branch: `sol/ontomath-radiance-rung5-chroma-20260921`  
Base at implementation start: `sync-from-earthcall-main` @ `c049b91fbc46f0763a03ab0e1d60954f313ad958`

## What Rung 5 establishes

Keep Rung 4's scalar invariant untouched:

```
rho(p,t) -> scalar
```

Add a separate source-side chroma invariant:

```
chi(p,t) -> vec3
```

The renderer composes chroma around scalar source strength; rho was not widened,
reinterpreted, or migrated.

`OntoMath::VectorField` remains physical flow/force. It is NOT the chroma vessel.

## Authored storage and reach

A radiant `FieldNode` now has an optional authored `Piecewise` chroma expression:

```
light.chroma.ast
```

Persistence key:

```
lightChroma
```

Absence means legacy compatibility, not authored white:

```
chi absent => historical light.color is the constant/default source chroma
```

An authored expression that is invalid/unsupported does NOT fall back to legacy
color.

## Renderer/compiler boundary

`Renderer` carries rho and chi separately:

- `setRadianceField(expr, revision)`
- `setRadianceChroma(expr, revision)`

Their full content revisions and emitted structural identities are independent in
WebGPU memoization.

The production OntoMath WGSL emitter is reused for chi. There is no second color
expression language.

Numeric chi edit:
- recollect packed params;
- no WGSL regeneration;
- memoized program reused.

Structural chi edit:
- changes chroma structure revision;
- compiles as needed;
- does not reinterpret rho.

Timeline advance:
- chi may read the same explicitly admitted radiance-source coordinate `t`;
- no authored parameter slot for t;
- no AST mutation;
- no WGSL regeneration;
- no authored parameter upload solely because time advanced.

## Legacy-color subtlety

Do NOT implement authored chi by multiplying it on top of the old color-bearing
light uniforms. That double-applies `light.color` and fails when a legacy channel
is zero.

The final shader keeps two paths:

1. **No authored chi:** exact pre-Rung-5 lighting formula using the historical
   color-bearing uniforms.
2. **Authored chi:** use scalar source coefficients
   `(intensity, ambient, diffuse, specular)` and multiply those by chi.

This lets authored red chi illuminate red even if legacy `light.color` is pure
green. The native witness proves exactly that.

## Refusal

Before GPU lowering, authored chroma must type-check as OntoMath Vector.

Unsupported/non-vector chi refuses. It must never:
- become white or black by fallback;
- reuse stale prior shader output;
- become flow/force VectorField;
- migrate into receiving Material color;
- mutate rho.

## Verification evidence

Implementation code head:
`dfb57ec496440e0be00952aae4aed2570101301a`

Earthcall focused CI run #1928:

### SDF range-proxy verification (macOS): SUCCESS

This job compiled and ran the actual Rung-5 witnesses.

`sdf_wgsl_parameter_refresh_test: PASS` proved:
- absent chi has legacy light.color structural identity;
- authored vector chi compiles through production OntoMath WGSL;
- numeric edits preserve structure/layout and change only params;
- timed chi binds admitted source time;
- structural chi edits change structure;
- non-vector chi refuses;
- unsupported chroma math refuses;
- no-chi source selects the compatibility branch.

`webgpu_object_test: ALL OK` proved through the real
Object -> drawFieldModel -> drawImplicit path:
- legacy green source produced G=253;
- authored red chi produced RGB=(253,0,0), despite legacy source color being green;
- numeric red->blue recolor reused compiled WGSL and uploaded refreshed params;
- chi=(t,0,0) changed pixels when the source-owned Timeline advanced;
- Timeline advance compiled zero shaders and uploaded zero authored param bytes;
- unsupported chi produced an explicit chroma refusal and no stale/fallback pixel.

### Focused CPU tests

Build succeeded.

`authorable_light_contract_test`: PASS, including:
- `light.chroma.ast` registered on FieldNode;
- chroma not aliased to `vectorField.ast`;
- save persists `lightChroma`;
- load restores it;
- restored CPU OntoMath evaluates the authored vec3.

The Focused CPU job overall is RED only because
`synthesis_studio_living_test` remains the known base failure already named in
the Rung-4 handoff. It reported its existing audio/resonator/constellation failures.
This Rung-5 PR touches no Synthesis Studio files or saves. Do not "repair" Studio
inside Radiance merely to paint this job green.

A final docs-only head run may supersede #1928; inspect that latest run before
making a final CI claim.

## Person verification

Added to:

`docs/Agenda/Tasks/For Zach/Person Verification List.md`

Zach should author an obvious multicolor `light.chroma.ast` on a radiant source
and visually inspect a plain white SDF receiver, then remove chi and confirm
legacy `light.color` returns.

## DO NOT start Rung 6 casually

Rung 6 is angular emission:

```
alpha(p, omega, t) -> scalar
```

Before code, define `omega` precisely and identically for CPU + WGSL:
- source-local or world-space?
- outgoing direction convention?
- normalized?
- vector variable or components?
- zero-length/singularity behavior?

Do not create PointLight/SpotLight/Beam enums. Those are mathematical alpha shapes.

## Files changed for implementation

- `src/ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp/.cpp`
- `src/Singularity/Core/EngineRender.cpp`
- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp/.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp/.cpp`
- `tests/singularity/authorable_light_contract_test.cpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_object_test.cpp`
- Rung plan + Person Verification List + this handoff

## Human authorship trace

Zach directed continuation of the later Radiance rungs and the compatibility law
that earlier authored truth must survive later capabilities without reinterpretation.
The prior Sun froze the rho/time semantics. This Sun selected the explicit optional
Piecewise chi vessel, independent rho/chi structural memoization, and the exact
legacy/new shader branch needed to honor those human constraints.

— GPT-5.6 Sol ("The Sun")  
Session `sol-rung5-chroma-20260921`  
Timestamp: 2026-09-21T15:12Z verification epoch
