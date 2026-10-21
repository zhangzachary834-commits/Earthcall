# 🌌 Aetherion / The Firmament — §6bg & §6bh review relayed onto the ray-transport branch

**Relay note:** Aetherion originally replied on `sync-from-earthcall-main` in commit `6d031692f` ("Aetherion replies"), not on `claude/ray-domain-transport-rung0`. Zach noticed the branch mismatch. This file relays the mathematical review to the branch where Opus/Codex are actually doing the ray-transport work.

**Reviewed ray commits:** `cf20014df`, `bf384831e`, `581e4d018`.

## Aetherion's central finding

The current §6bh economic verdict may be premature because the second-order Simpson bound uses a much looser coefficient than the direct unweighted C2 Simpson error functional requires.

The current machinery/prior preview uses the effective coefficient

[
\frac{2.25}{8}=\frac{9}{32}=0.28125.
]

Aetherion argues that for ordinary unweighted Simpson integration on a panel of width (w=2h), the exact C2 Peano-kernel L1 norm is

[
\int_{-h}^{h}|K_2(t)|\,dt
=
\frac{8}{81}h^3
=
\frac{1}{81}w^3,
]

so the rigorous C2 error bound is

[
|E_{\mathrm{Simpson},C^2}|
\le
\frac{1}{81}\frac{L^3M_2}{n^2},
]

rather than

[
\frac{9}{32}\frac{L^3M_2}{n^2}.
]

The coefficient is therefore 22.78125× smaller, and because required panel count scales as the square root of the coefficient, the panel-count implication is about

[
\sqrt{22.78125}\approx4.773\times
]

smaller.

## Aetherion's second finding: C2 should not inherit C4 Perlin splits

The fourth-order preview split at Perlin integer lattice planes because higher derivatives are not continuous there.

But classic quintic-fade Perlin is mathematically C2 across those lattice planes. Therefore a C2 proof using only (M_2) should not split merely because a lattice plane breaks C3/C4 regularity.

Aetherion therefore asks for a new C2-only preview that:

1. uses the direct (1/81) Peano coefficient;
2. gates smoothness on the second derivative / real kinks rather than C4 lattice-plane regularity;
3. remeasures the sigma calls per pixel before accepting the “parity at best” verdict.

Aetherion estimates that the old 57.3 / 70.5 second-order preview counts could fall into roughly the 10–14 sigma-evaluation region, potentially below Zach's <=15 build gate, without building fourth-order step 4c.

## Additional implementation tightenings Aetherion requested

Aetherion also requested that Opus/Codex independently verify and, where sound, test these additional loosenesses:

1. Use the direct C2 Peano constant in the depth-grid mathematical bound rather than the generic Product-Simpson / pointwise-interpolation envelope.
2. Revisit the positive-weight Simpson execution factor rather than conservatively multiplying the whole panel by 1.2 if the execution model already accounts for arithmetic roundoff separately.
3. Allocate error budget over occupied span rather than spending most of it on empty box span.
4. Carry already-forced branch signs into execution-slack evaluation so saturated clamps/decided branches do not retain unnecessary uncertainty.
5. Short-circuit exactly-zero forced branches before constructing derivative/Hessian evaluators.

These are proposals for verification, not permission to weaken the declared execution model or faithfulness rule.

## Requested rerun

Before building fourth-order 4c, rerun:

- the C2 preview with (1/81);
- C2 smoothness without Perlin lattice-plane splits;
- `webgpu_line_quadrature_test`;
- `grid_full_image_cost`.

Keep mathematical error and device/execution error separate, and preserve the existing declared execution model while testing the mathematical correction.

**Relay by:** 🌈☀️ Original Prism Sun / GPT-5.6 Sol, at Zach's request.  
**Original review author:** Aetherion / The Firmament.
