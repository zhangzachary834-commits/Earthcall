# SUN UPDATE — Rung 9 authored material response enters production SDF lighting
**Date:** 2026-09-24
**Repository:** `zhangzachary834-commits/Earthcall`
**Canonical re-read:** `sync-from-earthcall-main@2c581e751c8d53a273257bd088efadb9f358d4ea`
**PR:** #375 — `Rung 9: authored material response invariant`
**Exact implementation/witness head before this document:** `114a5ebc3592bc882850e04f057dc833e3f03797`
**Focused CI:** run `36082272524` / #3202, queued at this update

## What became real

Rung 9 is no longer only an owned AST channel at the renderer boundary. The SDF/WebGPU compiler now admits the Material-owned `responseExpr` into the actual receiving-surface lighting program.

The compiler emits a distinct receiver function:

```
materialResponseEval(p, n, wi, wo) -> vec3
```

This reuses generic OntoMath directional grammar while preserving semantic authority:
- `n` is receiver normal;
- `wi` keeps Earthcall's established transport convention: normalized world-space **source -> receiver** propagation direction;
- `wo` is normalized world-space **receiver -> eye** propagation direction;
- `t` remains refused because no honest Material-owned Timeline has been established.

Volume phase and material response remain separate predicates even though both can use `wi/wo`.

## Authored response composition law in this rung

Source emission and visibility stay outside receiver response.

For an authored response, direct surface contribution is compositionally of the form:

```
albedo
* sourceChromaIntensity
* f_r(p,n,wi,wo)
* rho
* alpha
* V
```

where source-owned `rho/chi/alpha` and derived visibility are evaluated independently before/around the receiver contribution.

The historical Material coefficients `ambient/diffuse/specular/shininess` do **not** secretly multiply authored response. When `responseExpr` is present, those compatibility coefficients remain stored and Law-addressable state but are bypassed by the authored-response direct path.

When `responseExpr` is absent, the existing Blinn-Phong compatibility path remains the rendering authority.

This first production lowering targets the raymarched SDF receiving-surface path. It does not claim mesh parity yet.

## Cache / invalidation law now wired

`WebGpuRenderer::MemoizedProgram` now carries receiver-response identity separately:
- `responseRevision` — value/content revision;
- `responseStructure`;
- structural read flags for `n`, `wi`, and `wo`.

The draw path inspects `responseExpr` before cache selection.

Therefore:
- unchanged response structure + changed response revision -> recollect packed params and reuse WGSL when parameter shape agrees;
- changed response structure/read layout -> relevant SDF program recompiles;
- unsupported response -> explicit program refusal before stale authored output can render.

Source, visibility, and volume revisions remain independent.

## Production witness added

`tests/singularity/sdf_wgsl_parameter_refresh_test.cpp` now requires:
1. absent response advertises `HAS_AUTHORED_MATERIAL_RESPONSE = false` and retains the explicit legacy shading branch;
2. authored response emits `materialResponseEval` and is called after source transport/visibility;
3. numeric response edit changes packed params while preserving WGSL byte-for-byte;
4. structural response edit changes WGSL and records `wi` dependence.

This complements the already-landed Rung-9 admission tribunal proving receiver-only `n/wi/wo`, numeric-vs-structural response layout, `t` refusal, authority separation, PropertyPath/persistence, and unsupported-math refusal.

## What is NOT yet proven

Do not call Rung 9 complete yet.

Still required:
- exact-head CI for the production lowering above;
- native WebGPU/pixel tribunal showing two receivers under the same incident source/visibility differ solely due to authored response;
- renderer-cache witness at the frame-stat level that numeric response edits avoid a shader compile in the real draw path;
- structural response edit invalidates only the relevant response program;
- source and blocker changes do not rewrite response structure;
- mesh receiving-surface parity or an explicit bounded statement of backend scope before completion;
- current-canonical reconciliation if base advances;
- final exact-head CI and landing/review readiness.

## Exact continuation

First inspect exact-head CI for `114a5ebc...` / run `36082272524`.

If it fails, repair only the concrete Rung-9 regression. If green, build the smallest native SDF receiver tribunal: same geometry/source/visibility, two authored response functions, materially different pixels; then add frame-stat cache evidence for numeric response reuse before widening to mesh parity.

Do not begin Rung 10 or Rung 11.
