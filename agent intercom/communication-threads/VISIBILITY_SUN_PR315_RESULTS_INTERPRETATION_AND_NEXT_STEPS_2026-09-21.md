# VISIBILITY SUN — PR #315 results interpretation and next steps

**Date:** 2026-09-21 (America/Los_Angeles)  
**From:** GPT-5.6 Sol — Visibility Sun 1  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Canonical:** `sync-from-earthcall-main` @ `102f0277bb28651b759594a15e44ec492b4c0555` at final audit  
**Baseline:** PR #297 merged as `4ab5ebe25aaac99915d25d63770960a2e085a34b`  
**Acceleration PR:** #315  
**CI-verified implementation head:** `5d06cd464fe612aa73fceff7d5d728c14340e0b2`  
**Note:** communication-doc commits were added afterward; they do not modify renderer/test code.

Zach — the results are in.

## What happened

PR #315 passed the complete maintained focused CI run #2354:

- Focused CPU tests (macOS): **PASS**
- SDF range-proxy verification (macOS): **PASS**
  - CPU range-proof witnesses
  - generic WebGPU parity
  - distance parity
  - authored-color parity
  - native WebGPU object/radiance parity
  - authored-Perlin compiler gates
  - authored-Perlin range-traversal camera gate
- Slow Adapter independent clock (macOS): **PASS**
- SDF authored-Perlin A/B (macOS Release): **PASS**

The native Rung-8 witness specifically exercises the new proof-consuming visibility path on both outcomes:

1. a source path that is genuinely blocked; and
2. the same geometry after the blocker moves away and the proof hierarchy rebuilds.

The proof-enabled pixels remain observationally equivalent to the exact visibility baseline, and the test requires `sdfRangeTraversalDraws > 0`, so this is not a vacuous "feature enabled but never used" pass.

## What this means

The architecture is now supported by runtime evidence:

```
authored geometry
    ↓
conservative range theorem
    ↓
positive-proof bitmap
    ↓
visibility ray may skip ONLY proved f>0 cells
    ↓
unknown cell → exact signed marcher
```

That matters because the bitmap is not being promoted into shadow ontology. It grants execution permission only where the CPU theorem has already proved absence of the zero set.

The admission split also survived the maintained renderer corpus:

- bit 0 of `rangeTraversalEnabled` governs primary-ray proof traversal;
- bit 1 governs Rung-8 source→receiver visibility traversal.

Therefore the unresolved primary distance-field quarantine remains intact. Primitive distance SDFs may use the proof grid for the monotone secondary visibility ray without granting the over-relaxed primary marcher permission to jump.

This is exactly the architectural separation we wanted.

## What the green result does NOT mean

Do **not** translate this CI result into "visibility is now faster."

The implementation structurally removes authored SDF evaluations from intervals already proved strictly positive, but we have not yet measured whether that saved work exceeds:

- `rangeCandidate()` classification cost;
- proof-buffer/cache traffic;
- branch/divergence cost;
- repeated per-source visibility queries;
- proof density limitations at the current fixed depth.

So the correct current statement is:

> #315 is a sound proof-consuming execution path with demonstrated activation and parity. Profitability remains unmeasured.

That distinction is important. Earthcall should not merge an optimization merely because it contains fewer conceptual evaluations if the GPU actually pays more to discover the skip.

## Integration state

#297 is merged.

#315 has been retargeted from the old #297 feature branch to the live canonical branch.

At the final audit:

- canonical: `102f0277bb28651b759594a15e44ec492b4c0555`
- CI-verified implementation head: `5d06cd464fe612aa73fceff7d5d728c14340e0b2`
- GitHub: **mergeable=true** after retarget
- implementation diff remains exactly four renderer/test files; this intercom message and the Sun handoff are additional documentation-only files on the PR branch
- canonical is 33 commits beyond the old stacked base, but none of those canonical commits changed #315's four files

The density lane remains separate:

- #299 — Volumetric V0: still open/draft
- #312 — Prism visibility × density reconciliation: still open/draft on the Density branch

Do not absorb Density into #315. Do not make #315 wait for Density to answer its own profitability question.

## What I think should happen next

The next Sun should build a **test-only visibility profitability witness** before promoting #315 from draft.

Use the existing spatial-Prophetic performance methodology as precedent, but measure the actual secondary-ray workload rather than primary Perlin marching.

The minimum useful A/B is:

```
A = exact Rung-8 visibility, proof consumption disabled
B = exact Rung-8 visibility, positive-proof consumption enabled
```

Same geometry. Same source set. Same camera. Same device/process. Warm both arms first. Interleave balanced AB/BA samples so thermal/clock drift cannot systematically favor the later arm.

Measure at least:

- wall-frame time and GPU main-pass time where available;
- number of source→receiver visibility queries;
- exact signed SDF sample/evaluation count;
- proof candidate calls;
- proved-positive skip calls;
- distance skipped under proof;
- clear/unknown handoffs back to exact marching;
- blocked/unblocked answer mismatches: must remain zero;
- pixel mismatches: must remain zero within the already-established tolerance;
- recurring proof bytes after warmup: should be zero for an unchanged scene.

Do not instrument production hot-path code permanently just to win a benchmark. A dedicated diagnostic compute/test path is preferable if counters would materially distort the renderer.

## Decision rule after the A/B

If proof traversal materially reduces exact work and produces a real GPU/frame-time improvement without correctness loss:

- document the result;
- update #315 with the measured claim;
- mark #315 Ready for Review.

If exact evaluations fall but total GPU time is neutral or worse:

- do not call #315 an optimization;
- use the diagnostic to find whether the tax is proof lookup, grid granularity, branch divergence, or source multiplicity;
- keep the mathematical theorem but reconsider the runtime artifact/consumer.

If proof traversal is simply unprofitable for this workload:

- do not weaken the proof or invent more permissive skip semantics;
- reject or quarantine the hot-path consumer and preserve the exact Rung-8 baseline.

## After profitability

Two independent visibility frontiers remain, and neither should be smuggled into the profitability experiment:

1. **192-step unresolved state.** The current baseline returns V=0 after finite-budget exhaustion. "Unresolved within budget" is not proof of "blocked." A later rung should make unresolved explicit or establish a termination theorem.
2. **Scene-wide transport.** A generated SDF pipeline can currently query only geometry represented by that pipeline. Cross-pipeline / Zone-wide occlusion requires a truthful shared geometry transport representation, not pretending foreign geometry exists in the local SDF.

For the next Sun: do not start Rung 8 over. The exact baseline is merged. #315 has already established the proof-consumption architecture. The next question is economics.
