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

## Successor audit — Rung 1H exact-head green + vessel-proof boundary

Exact-head workflow run **#2661** (`35816877135`) on head `39af1f4dcbeba18e5d34d1e1e96561ca4ef9f061` completed successfully. All four jobs were green. The macOS `SDF range-proxy verification` job successfully completed both `Build SDF proof and GPU parity witnesses` and `Run CPU SDF proof witnesses`, so the typed `C_v` and real Timeline assertions in `rendered_field_piecewise_synthesis_test` are now execution-backed, not merely source-audited.

Rung 1H is therefore **GREEN**.

### Targeted vessel-proof audit

The next gate was audited directly against `tests/singularity/rendered_field_piecewise_synthesis_test.cpp`; do not restart earlier compiler work.

The present substrate already has the right separation for the next rung:

- `MathCompiler::canonical` owns reusable mathematical calculation identity;
- `CompiledPiecewise::channel` owns rendered meaning (`rho`, `D`, `sigma_t`, `sigma_s`, `C_v`);
- `CompiledPiecewise::kind` owns scalar versus vec3 type;
- `CompiledPiecewise::topologyKey` owns Piecewise vessel structure including input variable, bounds, inclusivity, piece order, and compiled child IDs.

But the audit found that **proof authority does not yet exist at the vessel level**. In particular, the current adapter intentionally shares identical child math IDs across `rho`, `D`, `sigma_t`, and `sigma_s`, while `CompiledPiecewise` carries no proof record, no proof-valid bit, no proof dependency provenance, and no exact-fallback counter. This is the correct place to add the next theorem layer: do not contaminate `MathCompiler::canonical` with rendered-channel meaning merely to obtain proof scoping.

### Rung 1I design verdict

The next implementation should use **shared calculation identity + channel-scoped vessel proof authority**.

A proof record must be attached to the compiled Piecewise vessel (or to a vessel-owned theorem table keyed by vessel identity), and must include at least:

- the exact `Channel` whose meaning the theorem governs;
- the `ValueKind`;
- the topology identity/version it was derived from;
- the authored child-premise identities/revisions it depends on;
- theorem kind and theorem payload;
- validity state;
- counters for consultations, bypasses, exact fallbacks, invalidations, and rebuilds.

The first theorem should remain deliberately tiny and falsifiable. A good bounded witness is a scalar Piecewise support theorem for `MediumDensity`: if an interval's authored child is provably constant zero, that interval may be skipped by a density-support query. Construct a byte-identical zero-valued Piecewise under `SourceRho` and require that the density theorem **cannot** authorize a radiance skip. The mathematical zero child may canonicalize to the same calculation node; theorem authority must remain channel-scoped.

Required mutation sequence:

1. compile identical zero math under both `MediumDensity` and `SourceRho`;
2. prove zero-support only for the density vessel;
3. show density support query uses the theorem while radiance has no theorem authority;
4. change only the density Piecewise interval bound and invalidate the density theorem from topology provenance;
5. while invalid, support-enabled execution/query must fall open to exact Piecewise evaluation;
6. re-prove locally and restore the density bypass;
7. move runtime `x/t` without changing authored premises and require zero proof rebuilds;
8. mutate the shared mathematical child only in the density authored tree and invalidate/rebuild only the density vessel theorem, leaving the radiance vessel theorem state/identity untouched.

This is the architectural answer to the earlier cross-domain question: **identical math may share execution artifacts; rendered meaning scopes theorem authority.** Only if a future theorem genuinely depends on semantic role for the mathematical calculation itself should role enter canonical math identity.

### Current PR state at audit

PR #329 remains open, draft, and mergeable. Base is `sync-from-earthcall-main` at `3e46af88ed9986a727b8696598deac3b9917ed29`; the audited implementation head was `39af1f4dcbeba18e5d34d1e1e96561ca4ef9f061`. This handoff update is documentation-only and advances the branch without changing the tested code tree.

### What remains

Implement Rung 1I exactly at the Piecewise vessel boundary above, wire its counters into the existing rendered-field witness output, and obtain exact-head CI. After that, measure proof-build/invalidation cost against repeated support queries/evaluation before selecting the first production A/B lane. Do not productionize merely because Rung 1H is green.
