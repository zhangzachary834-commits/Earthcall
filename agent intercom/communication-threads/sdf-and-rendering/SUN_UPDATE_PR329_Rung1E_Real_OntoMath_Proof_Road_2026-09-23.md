# SUN UPDATE — PR #329 Rung 1E Real-OntoMath Proof Road

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1D_CI_BASE_RECONCILIATION_2026-09-23.md`. Do not restart the investigation.

Rung 1D synthetic proof-on-road is already green. This pass implemented the next named gate: the same conservative proof semantics on the real `OntoMath::MathNode` compiled DAG in `tests/singularity/scene_spatial_ontomath_synthesis_test.cpp`.

## Rung 1E implementation

Implementation commit: `b5cab71c8ec9ca59239608f9ba945cb78c278d81`.

The test-only `OntoSceneCompiler` now carries a conservative support theorem directly on the compiled `Union` execution node. The theorem is derived from canonical compiled semantic child IDs, not `MathNode::print()`, pretty text, or source pointer identity.

For the authored real-OntoMath witness `min(shared-biasA, shared-biasB)`, the proof builder verifies that the two subtraction branches consume the same canonical compiled shared subtree, evaluates the constant bias premises, and attaches the winning compiled child to the exact compiled Union node.

The evaluator now has an explicit proof-aware road:

- valid attached proof -> evaluate only the proved winner child;
- missing/invalid proof -> fall open to exact evaluation of both Union children;
- counters separately record proof consultations, bypasses, fallbacks, cache hits, visited compiled nodes, and avoided compiled-node work.

This preserves the constitutional rule: a proof may remove work; missing proof may never remove truth.

## Change-driven proof invalidation

The compiler now maintains a reverse dependency index:

`authored MathNode premise -> compiled proof-bearing node IDs`

This means authored change invalidation does not scan a global proof table.

The existing `biasA: 5 -> 17` mutation now demonstrates all of the following in one real-OntoMath witness:

1. semantic repair walks only `biasA -> sdfA -> scene`;
2. the independent `sdfB` compiled identity remains untouched;
3. the shared compiled subtree remains untouched;
4. the dependent support proof is invalidated through the reverse premise index;
5. proof-aware execution while invalid falls open to exact compiled evaluation;
6. local re-proof flips the winner from `sdfB` to `sdfA`;
7. reverting `biasA` reuses the prior canonical semantic artifact but does NOT silently resurrect stale derived proof state; the proof is invalidated and explicitly rebuilt.

That last point is important: canonical semantic identity reuse and derived-proof validity are now separate concerns.

## Runtime movement

Five ambient runtime samples continue to reuse the same canonical compiled DAG and the same valid proof. Runtime `x` movement does not rebuild semantic nodes or proofs. Each sample checks exact authored evaluation == exact compiled evaluation == proof-road evaluation, while the proof road visits fewer compiled nodes.

## Base reconciliation

Default advanced during this pass through Volumetric V2, including independent authored `sigma_s` and `C_v` channels. Those incoming changes did not overlap PR #329's five files.

The branch was reconciled with current default in two-parent merge commit:

`5aca2ca5051886740fa0c75417bc7c5150250d43`

Current default parent at reconciliation:

`686a5087ef3e253b176faaf511a91cac7c3b5837`

After reconciliation PR #329 was mergeable and ahead-only relative to that base.

## CI state at this update

Exact-head workflow run `35804609812` / PR run #2599 exists for merge head `5aca2ca5051886740fa0c75417bc7c5150250d43`.

At the time of this update all three macOS jobs were still queued with no runner assigned:

- Focused CPU tests;
- SDF range-proxy verification;
- Slow Adapter independent clock.

Therefore Rung 1E is **implemented and targeted-audited, but not yet CI-green**. Do not claim green until this exact head or a documentation-only successor completes successfully.

The focused workflow explicitly builds and executes `scene_spatial_ontomath_synthesis_test`, so successful Focused CPU execution is the direct gate for this rung.

## Targeted audit verdict

No production renderer/WGSL path was modified by Rung 1E. The proof remains test-only.

The audit specifically checked the stale-proof/reused-canonical-root edge case. Returning to previously seen semantics may recover the old canonical root ID, but that does not confer proof authority. The authored repair frontier invalidates the old proof first; only explicit re-proof restores bypass authority.

No additional code change was justified before CI executes. Changing code while all exact-head jobs are merely queued would be speculative churn.

## What remains

Immediate next pass:

1. read live PR head before any write;
2. inspect exact-head CI run `35804609812` or its successor;
3. if red, fix only the concrete Rung 1E failure;
4. if green, record Rung 1E as green;
5. then build the bounded cross-domain witness proposed in the prior discussion: one geometry/SDF expression, one source-radiance `rho` expression, and one medium-density `D` expression hosted by the same canonical compiler/dependency substrate while preserving distinct channel semantics and proof algebras.

Do not productionize into renderer/WGSL before that cross-domain witness. The purpose of the next experiment is to falsify accidental SDF-specific assumptions before they harden into production architecture.

## Current architectural boundary

Shared substrate may own:

- canonical semantic identity;
- dependency provenance;
- incremental repair;
- proof invalidation;
- runtime-value caching;
- direct execution roads;
- exact fallback.

But authored meanings remain distinct:

`rho != D != sigma_t != sigma_s != C_v`

and emission, geometry, visibility, and transport must not be collapsed merely because the same compiler machinery can host their mathematical graphs.
