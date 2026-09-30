# Sun Update — V4 PR #339 sixth channel threaded

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Canonical: `sync-from-earthcall-main`
V4 PR: #339
V4 branch: `sol/volumetric-v4-authored-emission-20260922`

## Do not restart

V3 is landed. V4 remains the active torch. Preserve:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
Phi != source alpha
E_v != source rho/chroma/alpha
```

## This pass

The previous exact-head build exposed that `VolumeProgramKey` had six channels but the renderer runtime remained V3-shaped.

Commit `2c2ec789dbe62fc23786fd315fb62401e80b5d5d` threads independent authored emission through the missing production seams instead of applying a tuple-only compile fix:

- renderer-side `volumeEmission` expression projection;
- independent emission revision;
- `inspectEmissionExpression` refusal/layout;
- memo structural identity including `readsOmega`;
- value-only emission invalidation and parameter refresh;
- SDF `compile/collectParams` emission argument;
- six-element depth-aware `VolumeProgramKey`;
- depth-aware content revision;
- depth-aware emission layout/refusal/structure identity;
- `compileVolume/collectVolumeParams` emission argument.

The authored V4 direction remains emission-owned sample -> eye `omega`, not source alpha's source -> receiver `omega`.

## Current gate

Exact-head focused CI run **35826999809** was pending when this checkpoint was written.

Do NOT merge merely because the prior compile error is repaired. Required next actions:

1. Inspect run 35826999809 at exact head.
2. If red, diagnose only the concrete failing gate and repair surgically.
3. If green, inspect/run the relevant native V4 witnesses, especially:
   - E_v numeric refresh without structural compile;
   - E_v structural change changes program identity;
   - unsupported E_v refuses with no stale radiance;
   - time-driven E_v changes pixels without AST rewrite;
   - **no external illumination + authored E_v != 0 still yields visible medium radiance**.
4. Reconcile against latest canonical immediately before any landing.
5. Keep PR #339 unmerged until exact-head relevant CI is green and integration state is settled.
