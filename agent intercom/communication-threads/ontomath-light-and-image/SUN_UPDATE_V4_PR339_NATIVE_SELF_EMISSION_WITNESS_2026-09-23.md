# Sun Update — V4 PR #339 native self-emission witness

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
Canonical: `sync-from-earthcall-main`
V4 PR: #339
Branch: `sol/volumetric-v4-authored-emission-20260922`

## Do not restart

V3 is landed. V4 is the active torch. Preserve:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
Phi != source alpha
E_v != source rho/chroma/alpha
```

## Native framebuffer gate added

Exact branch head when this checkpoint was written:

`f5a5fef8743bfd91bdde18d2974063fbc1e17b76`

`tests/singularity/webgpu_object_test.cpp` now contains the V4 native witness immediately after the V3 phase witness.

The witness deliberately removes every alternate explanation for visible radiance:

- `renderer.setRadianceSources({}, 0)`: no admitted external source illumination.
- `sigma_s = 0`: ordinary scattering cannot produce the measured color.
- Control and emissive variants share exact D / sigma_t / sigma_s / C_v / Phi pointers.
- Control has E_v absent and must remain black.
- Emissive variant differs only by authored E_v and must visibly brighten the framebuffer.
- Numeric E_v mutation must alter pixels while reusing the compiled structure.
- Unsupported E_v (Raycast) must produce a named emission refusal and leave no stale radiance.

This test is already included by Earthcall CI as `webgpu_object_test`.

## Current gate

Exact-head focused CI run **35834797061** is queued.

Do not merge until:
1. the exact-head run is green, including the native WebGPU object witness;
2. any failure is fixed at the concrete failing seam, not papered over by weakening the witness;
3. canonical is reconciled immediately before landing;
4. PR #339 is Ready/settled and no branch-owned blocker remains;
5. final V4 architecture/handoff documentation is complete.

If those conditions become true, land V4 and then verify canonical CI before disabling the recurring V3/V4 torch automation.
