# Sun Update — V4 PR #339 native parity value-refresh fix

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #339
Branch: `sol/volumetric-v4-authored-emission-20260922`

## Do not restart

V3 is landed. V4 remains active. Preserve:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
E_v != source rho/chroma/alpha
```

## What the failing native witness proved

Reconciled exact-head run 35835364396 failed only in the WebGPU object/radiance parity step.

The V4 source-free self-emission assertion itself passed:

- control medium: `(0,0,0)`
- authored E_v medium: `(255,45,15)`
- admitted radiance sources: none
- authored sigma_s: 0

Therefore native medium self-emission is genuinely visible without external illumination.

The next assertion failed when mutating only a numeric E_v coefficient.

## Root cause

The structural volume path already passed E_v into:

`compileVolume(D, sigma_t, sigma_s, C_v, Phi, E_v)`

but the value-only refresh path still called:

`collectVolumeParams(D, sigma_t, sigma_s, C_v, Phi)`

without `medium.emissionExpr`.

That meant numeric E_v revisions were detected, but the rebuilt parameter block omitted the sixth channel.

## Repair

Commit:

`da4dde338f38686459f9829c0b9109c544647113`

The runtime refresh path now calls:

`collectVolumeParams(..., medium.phaseExpr, medium.emissionExpr)`

Do not weaken the native pixel witness. This was a production invalidation/refresh bug and the witness correctly caught it.

## Next gate

Run exact-head CI on the repaired branch. If green:

1. confirm the native no-source E_v witness passes including numeric refresh and refusal;
2. re-check canonical for any new movement;
3. finish final handoff/architecture state;
4. mark PR #339 Ready only when all relevant exact-head evidence is green;
5. land V4;
6. verify canonical CI;
7. only then disable the recurring V3/V4 torch.
