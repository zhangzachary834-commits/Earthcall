# SUN UPDATE — V5 membership/refusal native gate green; overlap numerical gate pinned

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical was re-inspected first and is now:

`67bb9d0cd446c557bedd01c7d9544d7b6f89ec2a`

That is PR #349's palette-UX merge, not a volumetric merge.

PR #343 was re-inspected at head:

`6c9d3d1a73f747c95e18fbf9a5b346d1fa2af394`

It remains open, Draft, and mergeable. V3/V4 remain completed and were not reopened.

## Membership/refusal native tribunal is GREEN

Focused CI run `35938731849` tested exact head `6c9d3d1a...`.

The decisive SDF range-proxy verification job completed SUCCESS, including:

- build of SDF proof + GPU parity witnesses;
- CPU SDF proof witnesses;
- generic WebGPU SDF parity;
- WebGPU SDF distance parity;
- WebGPU authored-color parity;
- **WebGPU object/radiance parity**.

That last step executes `webgpu_object_test.cpp`, therefore the new V5 native sequence from `3ff006d9...` passed on macOS WebGPU:

1. valid `{A,B}` baseline;
2. add visible structurally distinct C => one new fused compile;
3. remove C => zero structural compiles and restoration of prior `{A,B}` pixels;
4. identical restored frame => fused memo cache hits;
5. poison B's `E_v` with unsupported `Raycast` => named member/emission refusal and black framebuffer;
6. restore lawful B => prior pixels recover without structural recompilation.

This closes the V5 membership invalidation and refused-member/no-stale-output obligations at native renderer level.

Focused CPU tests and authored-Perlin A/B also completed SUCCESS. The Slow Adapter job was still executing its unrelated authored-world performance measurement at the time of this update; its soundness/cadence step had already passed. Do not confuse that long-running unrelated benchmark with the volumetric native verdict above.

## Targeted overlap-law audit

I then audited only the fused V5 transport seam in `SdfWgsl::compileVolumeSet` and the existing V5 tests.

The production shader law is structurally correct for overlapping active media:

- every active member contributes its own `extinction_i` to one `totalExtinction`;
- every active member contributes its own `C_i * sigma_s_i * Phi_i + E_i` to one `totalSource`;
- transmittance advances once as `T' = T exp(-sum(sigma_t_i) ds)`;
- radiance advances once as `L += totalSource * (T-T') / totalExtinction`.

This is materially stronger than sequential whole-medium alpha composition: extinction from all overlapping media acts on all source terms inside the same differential interval.

The existing compiler witness proves the accumulator structure exists and the native permutation witness proves order invariance. However, neither is yet the strongest possible counterexample to the old architecture: an accidentally symmetric but still-wrong implementation could theoretically satisfy order equality in a specially chosen case.

## Exact numerical counterexample for the next native witness

Use two completely overlapping constant media over the established centre-ray span of 2 world units, with zero scattering so self-emission isolates the transport algebra:

- A: `D=1`, `sigma_t=0.35`, `sigma_s=0`, `E_v=(0.2,0,0)`;
- B: `D=1`, `sigma_t=0.90`, `sigma_s=0`, `E_v=(0,0,0.3)`;
- same origin and half-extent.

For lawful fused transport:

`sigma_total = 1.25`

`1 - exp(-1.25 * 2) ~= 0.917915`

Therefore on black background the expected premultiplied answer is approximately:

- red: `(0.2 / 1.25) * 0.917915 ~= 0.14687` => byte ~37;
- blue: `(0.3 / 1.25) * 0.917915 ~= 0.22030` => byte ~56;
- alpha: `0.917915` => byte ~234.

Now compare the obsolete sequential whole-medium answer. Rendering A alone gives approximately red `0.2877`; rendering B alone gives approximately blue `0.2782`, with B transmittance `exp(-1.8) ~= 0.1653`. Sequential A-then-B would therefore leave red only about `0.0476` (byte ~12), while fused physics requires red ~37. The opposite ordering produces a different wrong answer.

That gap is intentionally huge. It gives the next native witness a robust discriminator instead of relying on one-byte subtleties.

## Independent contribution requirement

The same witness can close the remaining overlap-contribution obligation without adding another broad fixture:

- A owns red self-emission only;
- B owns blue self-emission only;
- fused `{A,B}` must contain both red and blue near the analytic shared-extinction answer;
- render A-only and B-only controls first, then compute the two sequential premultiplied counterexamples from those native pixels/alphas;
- assert the fused answer is near the analytic shared-integral result and materially far from BOTH sequential orders.

After that self-emission discriminator is green, add one bounded variant (or reuse an existing V2 native chroma/scattering control if its overlap semantics are sufficient) where one member's `C_v * sigma_s` contributes a distinct channel while the other member owns `E_v`. This prevents the proof from accidentally covering only V4 emission summation while leaving V2 scattering/chroma composition untested in overlap.

## No production fix in this pass

No contradiction was found in the production fused algebra. Changing the shader before a numerical native witness accuses it would be speculative churn. The next action is therefore witness-first.

## Remaining risks / landing work

- The direct native numerical counterexample to sequential whole-medium alpha is still missing.
- Independent overlap contribution should explicitly cover both the V2 scattering/chroma source term and V4 self-emission, not only WGSL symbol presence.
- PR #343 is still based on an old base SHA and must be reconciled with current canonical before landing.
- After reconciliation, run exact-head focused CI again and inspect renderer-adjacent conflicts rather than assuming the current green run transfers across the rebase/merge.

## Exact continuation point

1. Add the constant two-medium native analytic witness described above beside the current V5 permutation witness in `tests/singularity/webgpu_object_test.cpp`.
2. Render A-only, B-only, then fused `{A,B}` on black.
3. Assert fused RGB/alpha near the analytic shared-extinction answer and materially far from both sequential premultiplied reconstructions.
4. Add/retain a bounded mixed-source overlap check proving distinct scattering/chroma and emission contributions both survive the shared integral.
5. Run the WebGPU object/radiance parity gate first; if green, run exact-head focused CI.
6. Then reconcile PR #343 with canonical `67bb9d0c...`, resolve only real conflicts, and rerun exact-head CI.

Guardrails remain unchanged: no sequential whole-medium fallback, no global volume-cache flush, no V6/multiple-scattering/GI/spectral scope, no new Medium noun, no big-chungus reads.

— GPT-5.6 Sol