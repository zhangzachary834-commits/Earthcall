# Sun Update — V4 PR #339 native telemetry assertion fix

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #339
Branch: `sol/volumetric-v4-authored-emission-20260922`

## Do not restart

V3 is landed. V4 remains active.

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
E_v != source rho/chroma/alpha
```

## What run 35837847978 proved

The native V4 self-emission physics is working:

- no admitted external radiance sources;
- authored sigma_s = 0;
- control medium pixel = `(0,0,0)`;
- authored E_v medium pixel = `(255,45,15)`.

The numeric E_v pixel-delta assertion also passed after the prior production repair that added E_v to `collectVolumeParams`.

The remaining failure was:

```text
v4DimmedStats.volumeProgramCompiles == 0 &&
v4DimmedStats.volumeProgramCacheHits >= 1
```

## Root cause

This second conjunct misread the renderer telemetry.

A numeric authored edit changes the medium content revision, so the renderer enters the value-refresh branch and recollects the parameter block. `volumeProgramCacheHits` is incremented only when the complete content revision is unchanged.

Therefore a numeric E_v edit is correctly proven by:

1. visible pixel change, demonstrating refreshed parameters reached transport;
2. `volumeProgramCompiles == 0`, demonstrating the existing shader structure/WGSL was reused.

Requiring `volumeProgramCacheHits >= 1` on that same frame is not part of the renderer's current counter contract.

## Repair

Commit:

`7144d6a7b11a03a6ccbe0082ae52053fcd90d8c1`

Only the incorrect cache-hit assertion was removed. The no-source self-emission physics witness, numeric pixel-change witness, zero-recompile requirement, and unsupported-emission stale-output refusal remain intact.

Fresh exact-head focused CI:

`35840675136`

Do not merge until this exact head is green and canonical is reconciled again immediately before landing.
