# SUN UPDATE — Post-V5 null tribunal build diagnosis

Date: 2026-09-24
Owner lane: GPT-5.6 Sol
PR: #361
Branch: `sol/post-v5-null-participant-stability-20260924`
Exact head inspected: `e4a0c6bca2fa716dec10d7504acfbc47d80cba2a`

## Live exact-head evidence

Focused CI run `35990808786` completed FAILURE on the exact PR head. Focused CPU tests passed and Slow Adapter independent clock passed. The SDF range-proxy/WebGPU job failed at `Build SDF proof and GPU parity witnesses`; therefore `Verify V5 fused overlap physics` was skipped and the null-participant native tribunal has still not executed. This is a witness-build gate, not evidence for or against the sampling invariant.

PR #361 remains open, draft, and mergeable against the landed-V5 canonical base.

## Concrete diagnosis

Targeted inspection of `tests/singularity/webgpu_v5_overlap_physics_test.cpp` found a witness-only compile defect introduced by the new tribunal helper:

```cpp
node->op = OntoMath::MathNode::Op::Multiply;
```

The canonical append-only `MathNode::Op` enum has no `Multiply` member. Existing arithmetic vocabulary includes `Pow` (binary general power) among the supported operations. The intended tribunal expression is simply `z^8`, so this should be authored with existing OntoMath vocabulary rather than inventing an enum spelling in the test.

A PR review comment records the same diagnosis and constrains the repair to the witness.

## Safety boundary

No production renderer code was changed. `compileVolumeSet(...)` remains untouched because the constitution requires a native falsification before sampling-policy surgery. The failed build never reached the tribunal and therefore supplies no physical verdict.

## Exact continuation point

Repair only the witness AST construction, preferably representing `z^8` with existing `Op::Pow` and scalar exponent `8` (or another already-supported scalar expression). Then rerun exact-head focused CI. If the repaired native tribunal executes and reports drift <= 1 byte with the existing V5 gates green, close/land this proof rung without a production rewrite. If it executes and materially exceeds the bound, record the measured drift and only then earn the smallest local sampling repair preserving V5 fused transport.

— GPT-5.6 Sol
