# SUN UPDATE — PR #329 Rung 1H Typed Chroma + Timeline

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1G_Piecewise_Multi_Channel_Adapter_2026-09-23.md`. Do not restart Rungs 1A–1G.

Rung 1G's authoritative exact-head workflow run #2651 on `5df2c20edd5306e6f44e6d5b1cd0ab6bcdd27aae` completed successfully. All four jobs were green. In particular, `SDF range-proxy verification (macOS)` successfully built the SDF proof/GPU parity witnesses and ran the CPU SDF proof witnesses containing `rendered_field_piecewise_synthesis_test`.

## This pass

Implementation commit:

`fecdd46fd1d6dc33ff2747db157388917b2b4133`

Extended the existing real `OntoMath::Piecewise` rendered-field witness instead of inventing a parallel adapter.

### Typed C_v lane

The Piecewise adapter now has an explicit value kind:

- scalar for `rho`, `D`, `sigma_t`, `sigma_s`;
- vec3 for `C_v`.

`C_v` accepts only a real `MathNode::VectorConstruct` root with three scalar children. Its compiled vessel records `ValueKind::Vec3`; the vector root and its scalar component calculations enter the same canonical math substrate without scalar coercion.

The witness evaluates real `Piecewise::evaluate()` output and requires the resulting `PropertyValue` to contain `glm::vec3(1.0, 0.5, 0.25)`.

The inverse type-safety witness is also present: a scalar Piecewise submitted as `MediumChroma` is refused even though the scalar compiler could otherwise evaluate its math. This keeps type/channel meaning authoritative over mathematical convenience.

### Timeline is now a real authored premise

Rung 1G supplied `t` at runtime but did not consume it. Rung 1H closes that limitation with a real authored expression:

`D(x,t) = 2 * (x + t)`

The density Piecewise is compiled once. It is then evaluated at fixed `x=3` with:

- `t=0` -> `6`;
- `t=5` -> `16`.

The witness requires both the Piecewise topology-build count and compiled math ID to remain unchanged across Timeline movement.

This establishes the intended distinction:

`runtime Timeline value movement != authored semantic mutation`

Time may change the value of a rendered field without invalidating its compiled semantic artifact merely because time passed.

## Preserved boundaries

The pass still preserves:

`rho != D != sigma_t != sigma_s != C_v`

while sharing the mathematical execution substrate where structure is genuinely identical.

No production renderer/WGSL code changed.

## CI state

The previous Rung 1G gate is green. The new implementation commit `fecdd46f` requires its own exact-head focused CI before Rung 1H can be called green. A documentation-only successor may become the current head; judge the run whose tested tree contains `fecdd46f`.

## Next bounded actions

1. Inspect exact-head CI for the typed vec3 + real Timeline witness; fix only concrete failures if red.
2. If green, record Rung 1H as green.
3. Add channel-scoped proof records at the Piecewise vessel level. The key invariant remains: shared math identity does not imply shared theorem authority.
4. Give those proof records dependency provenance over Piecewise topology and authored child premises, with exact fallback while invalid.
5. Measure compile/proof cost versus repeated evaluation before choosing the first production A/B lane.
6. Keep the full rendered-field implementation plan authoritative; do not narrow the architecture back to SDF-only work.

This Sun role is not finished yet: typed values and real Timeline semantics are now represented, but Piecewise-level proof authority/invalidation and the production-lane selection gate remain.