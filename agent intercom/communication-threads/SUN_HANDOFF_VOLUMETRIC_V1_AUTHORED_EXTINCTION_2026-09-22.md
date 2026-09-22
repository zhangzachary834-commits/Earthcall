# SUN HANDOFF — Volumetric V1 Authored Extinction

**Date:** 2026-09-22
**Repository:** `zhangzachary834-commits/Earthcall`
**Branch:** `sol/volumetric-v1-authored-extinction-20260922`
**Base at branch creation:** `14d87bc1e64bf40f87e443d9e7df67ac402c3564`
**V0 canonical merge:** PR #320

## Constitution

V1 preserves:

```
rho_source != V_transport != D_medium
D_medium != sigma_t
```

`volume.density.ast` remains the sole authored density truth.
V1 adds `volume.extinction.ast` with meaning:

```
sigma_t(p,t) -> scalar
```

Presence of authored extinction makes it the sole extinction authority.
Absence preserves the exact pre-V1 compatibility law:

```
sigma_t = 0.5 * D
```

Extinction alone does not create a medium; authored D still determines whether participating substance exists.

## Implemented

Core implementation commit:
`f93247d179a97ece1d9c02c734d358b0636a63df`

That commit adds:
- independent `FieldNode::volumeExtinction`;
- PropertyPath `volume.extinction.ast`;
- save/load persistence independent from rho and D;
- `VolumeDensityBinding::extinctionExpr/extinctionRevision`;
- source-set identity including extinction revision;
- production OntoMath scalar inspection for extinction;
- independent structure/value invalidation;
- authored `volumeExtinctionEval` in both the legacy drawImplicit medium seam and the dedicated depth-aware volume compositor;
- exact `0.5 * D` compatibility when extinction is absent;
- refusal instead of stale/fallback output when an authored extinction expression is unsupported;
- the existing medium temporal coordinate admitted by sigma_t(p,t), without inventing an ExtinctionTimeline kind.

The follow-up witness commit adds:
- PropertyPath/persistence/rho-D-sigma_t independence tests;
- focused codegen, numeric-vs-structural invalidation, time, compatibility, and refusal tests;
- native WebGPU pixels proving fixed D with changing sigma_t changes transport without numeric-edit recompilation;
- native extinction Timeline and stale-output refusal witnesses.

### V1 cache-identity follow-up

The next bounded audit found a V0-era cache assumption that became false under V1:
the dedicated volume program memo was keyed only by the density AST pointer.

V1 explicitly permits two media to share the same D AST while carrying different
sigma_t ASTs. A density-only memo key therefore made the two lawful extinction
variants evict/ping-pong one another's memo state every frame.

The V1 branch now keys that memo by the pair:

```
(D AST pointer, sigma_t AST pointer)
```

A native witness renders two media sharing one density AST but using distinct
extinction structures, then repeats the frame and requires zero recompiles plus
two cache hits. This preserves the structure/value caching promise for the new
independence V1 introduces.

### CPU/WGSL same-truth witness

The V1 focused test now explicitly evaluates the authored timed extinction
Piecewise through OntoMath's ordinary CPU evaluator and then lowers that same
Piecewise through the production WGSL path. This closes the CPU/WGSL contract
without adding a renderer-specific extinction language.

## V2 remains out of scope

Do not alter the current white-scattering compatibility in this V1 lane.
V2 owns authored scattering and volumetric chroma.

## Next

Open a draft PR against current `sync-from-earthcall-main`, run exact-head CI, repair only evidence-backed V1 failures, reconcile if canonical moves, and land V1 only after the relevant exact-head jobs are green and the branch remains semantically clean.
