# SUN UPDATE — PR #329 Rung 1I Vessel-Scoped Proof Authority

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1H_Typed_Chroma_Timeline_2026-09-23.md`. Do not restart Rungs 1A–1H.

Rung 1H is green. Exact-head run #2661 proved typed vec3 `C_v`, real authored Timeline evaluation, Piecewise multi-channel compilation, and zero semantic rebuild under runtime time movement.

The next named gate was vessel-level theorem authority. That gate is now implemented in the existing `rendered_field_piecewise_synthesis_test.cpp`.

## Implementation

Core implementation commit:

`608d50370422500698b4d4f21211d012a82d8bf4`

Canonical then stood fifteen commits ahead of the old branch base. Those incoming changes were audited before reconciliation. They include current volumetric / phase / renderer work plus Person serialization fixes, but none overlap PR #329's thirteen changed files.

Current-default reconciliation landed as real two-parent merge commit:

`ec68b468853373a4362219ffb3d3746848b34d7a`

After that merge:

- current canonical: `e4eeee50d1c6982d262c676a0d9d8b55212f959f`;
- PR behind canonical: 0;
- PR mergeable: true;
- PR remains draft.

No force push was used.

## Architectural result

Rung 1I makes the earlier cross-domain verdict executable:

> identical mathematics may share canonical calculation identity; rendered meaning scopes theorem authority.

The witness constructs byte-identical zero-valued Piecewise fields for:

- `SourceRho`;
- `MediumDensity`.

Both channels intentionally compile their zero children to the same canonical math IDs.

The density vessel then receives a `DensityZeroSupport` theorem. The radiance vessel does not.

The theorem record lives on `CompiledPiecewise`, not in `MathCompiler::canonical`, and carries:

- validity;
- theorem kind;
- exact channel;
- value kind;
- deriving topology key;
- compiled child-math premise IDs;
- authored child-source premise identities;
- proved zero piece indices;
- theorem generation.

This is deliberately stricter than canonical calculation reuse.

## Cross-channel authority refusal

The density theorem builder explicitly refuses a `SourceRho` vessel even when its child mathematics is byte-identical to density.

The witness then performs an intentionally hostile test: it copies the valid density theorem record onto the compiled radiance vessel.

Support-enabled radiance execution still refuses the proof as authority because:

`proof.channel == MediumDensity`

does not match:

`vessel.channel == SourceRho`.

The query falls open to exact `Piecewise::evaluate()`.

Thus a density theorem cannot become source-radiance authority merely by sharing math or even by having its record mechanically copied.

## Topology invalidation + exact fallback

The initial density field is two zero pieces split at `x=0`.

After proving both intervals zero, the authored density Piecewise split is changed to `x=-2`.

Recompiling the same stable density vessel compares the theorem's declared premises against the new vessel:

- channel;
- value kind;
- topology key;
- compiled child IDs;
- authored child-source identities.

The topology change invalidates the theorem.

While invalid, a support-enabled density query performs exact `Piecewise::evaluate()` and records a proof fallback. It does not assume zero.

A local re-proof then restores support bypass authority.

## Runtime movement does not rebuild theorem state

After re-proof, the witness queries multiple runtime coordinates across both intervals and multiple Timeline values.

No compile call occurs.

It requires:

- Piecewise topology build count unchanged;
- proof build count unchanged;
- valid density theorem reused.

Runtime `x/t` movement therefore changes the query/evaluation state without masquerading as an authored semantic mutation.

## Authored child mutation is local

The density-only left child is mutated from constant zero to constant two.

Recompiling the density vessel invalidates its theorem because a declared authored/math premise changed.

The separately authored radiance vessel retains:

- its exact topology key;
- both canonical zero math IDs;
- its no-proof state.

This demonstrates that editing `D` does not invalidate or reinterpret `rho`, even when the two channels previously shared canonical calculation IDs.

While the density proof is invalid:

- left interval exact fallback correctly reports non-zero;
- right interval exact fallback correctly reports zero.

Local re-proof then becomes partial:

- left interval is not proved zero and continues exact fallback;
- right interval is proved zero and bypasses.

The theorem therefore narrows safely rather than clinging to its old all-zero conclusion.

## Proof economics

The existing rendered-field witness now prints/counts:

- proof builds;
- proof invalidations;
- proof consultations;
- proof bypasses;
- exact fallbacks;
- proof refusals.

This is still test-only architecture work. No production renderer/WGSL path consumes these proof records.

## CI state

Reconciled exact code head `ec68b468853373a4362219ffb3d3746848b34d7a` triggered focused workflow run **#2682**.

At handoff-writing time the Focused CPU macOS job had a runner and was building the broad regression witnesses. The SDF range-proxy job containing `rendered_field_piecewise_synthesis_test` had not yet completed.

Therefore Rung 1I is:

**IMPLEMENTED + TARGETED-AUDITED + BASE-CURRENT, NOT YET CI-GREEN.**

Do not interpret queue/in-progress state as failure or success.

This documentation/plan commit is a successor to the exact code head, so a newer workflow may supersede #2682. Judge the newest run whose tested tree contains `608d5037` and `ec68b468`.

## Next gate

If Rung 1I CI is green, measure theorem-build/invalidation cost against repeated support queries before choosing a production A/B lane.

The measurement must distinguish:

```
rare authored premise change
    -> compile / theorem build / repair cost

many runtime samples
    -> theorem consultation / bypass profit
```

Then add the first corresponding **radiance-specific** exact-zero/contribution theorem rather than reusing the density theorem algebra.

Only after those economics and cross-channel proofs are green should the project choose between the first production A/B candidates:

1. already-proven exact SDF branch dominance;
2. exact zero-density / empty-support skipping.

Do not move proof authority into WGSL/renderer production merely because the test architecture is coherent.

This Sun role is not finished.
