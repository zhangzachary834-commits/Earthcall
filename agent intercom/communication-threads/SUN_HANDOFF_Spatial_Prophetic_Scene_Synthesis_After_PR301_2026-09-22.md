# SUN HANDOFF — Spatial Prophetic after PR #301: Scene-Spatial Synthesis

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Canonical base: `sync-from-earthcall-main`
PR #301: MERGED at `3aa3657c4c7dd091a3342574ac459d6ab63032a5`
Do not continue the old merged branch `sol/sdf-direct-profitability-aot-20260921`.

## Read first

1. `docs/architecture/SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md`
2. PR #301 discussion and merged test witness
3. This handoff in full
4. Live `sync-from-earthcall-main` before branching, because other Suns may have advanced default after this file was written.

## What #301 proved

PR #301 was deliberately a test/architecture rung, not a production optimization.

The maintained authored-Perlin witness established:

- the rich Spatial Prophetic theorem is already AOT/resident;
- the hot-path tax is generic relevance discovery / interpretation, not recurring proof upload;
- maximal positive-run artifacts are correct but economically rejected once hidden `record_tests` are charged;
- zero per-ray hit mismatches were preserved;
- an ideal zero-discovery oracle shows that the rare genuinely relevant proof consequences are individually valuable;
- a stable geometric route atlas keyed by entry face + entry cell + octahedral direction was tested;
- the first camera-trained form was recognized as an optimistic/leaky ceiling and was corrected;
- the corrected camera-independent atlas failed to generalize robustly, especially on the horizon camera;
- increasing geometric classification resolution grew resident state / runtime consequence work without reliably recovering missing relevance;
- therefore the geometric route-atlas family is rejected for production.

Do not resurrect the rejected one-AABB gate (#296), depth-5 density (#288), DDA, octree, sparse pointer tree, mip pyramid, negative proof, raster tightening, distance-field traversal, grid-scale hoist, maximal-run scanning, or the rejected entry/direction route atlas without materially new evidence.

## The architectural breakthrough

Stop treating Spatial Prophetic as an external lookup service that a ray interrogates.

The stronger hypothesis is:

> Compile the collective spatial computation itself into an incrementally repairable execution/dependency DAG, then let Spatial Prophetic become an optimization pass over that same DAG so proof consequences are attached to the execution branches that derive and consume them.

The desired shape is:

```
AUTHORED WORLD
    ↓
SDF / spatial semantic collection
    ↓
canonical scene-spatial DAG
    ↓
shared subexpression + dependency synthesis
    ↓
conservative support / Prophetic proof
    ↓
proof-derived direct execution annotations / rewrites
    ↓
incrementally maintained execution program
    ↓
GPU/runtime
```

This is NOT "put every SDF in one huge shader and evaluate all of them." That would merely be N evaluations in one trench coat.

The goal is that the collective compilation exposes enough structure that runtime touches only surviving relevant branches, shares common computation, and carries proof consequences on those branches instead of rediscovering relevance through an independent search structure.

## Hard architecture rules

1. Proof/direct/compiler artifacts are derived state. The authored world / exact semantics remain authority.
2. Ordinary frames do not rebuild the scene-spatial DAG, proof, or direct artifacts.
3. Camera motion does not rebuild them.
4. Relevant semantic edits repair only affected dependency frontiers.
5. Preserve unaffected DAG nodes, proof nodes/cells, direct annotations, compiled code, and GPU-resident state whenever possible.
6. Missing an optimization must fall open to exact evaluation; absence of a direct annotation is never proof of emptiness.
7. Do not trade visible query count for hidden scans.
8. Do not mutate production renderer/WGSL until a test-only witness demonstrates dramatic economics and a separate A/B is justified.

## Successor mission — first rung

Create a NEW successor branch from the current live `sync-from-earthcall-main`. Do not reuse the merged #301 branch.

The first implementation should be a bounded TEST-ONLY scene-spatial synthesis witness for multiple SDFs.

### Minimum experiment

Build a small canonical `SceneSpatialDag`-like test representation from several authored SDF expressions / nodes.

It should make these things explicit:

- stable node identity / canonicalized expression identity;
- dependency edges back to authored SDF/Singular inputs;
- common-subexpression sharing where semantics are genuinely identical;
- composition nodes for union/intersection/subtraction/min/max as applicable;
- conservative support / proof annotations tied to the DAG node that derives them;
- provenance sufficient to identify exactly which compiled nodes depend on a changed authored input.

Then measure at least two execution paths:

A. naive independent collective evaluation;
B. synthesized DAG evaluation.

Charge real work, not headline calls. At minimum report:

- exact authored leaf / OntoMath evaluations;
- shared-expression evaluations avoided;
- DAG nodes visited;
- proof/direct annotations consulted;
- any fallback evaluations;
- artifact bytes / node count;
- exact result parity with the naive path.

### Incremental repair witness

The first rung is incomplete without a mutation test.

Change one authored SDF property/input and record:

- authored dependency that changed;
- DAG nodes invalidated;
- DAG nodes recompiled/repaired;
- proof/direct annotations invalidated;
- unaffected nodes preserved;
- total artifact bytes rewritten / re-uploaded if applicable.

A whole-scene rebuild may exist only as a temporary bootstrap/reference path. It must not be presented as the target architecture.

### Prophetic integration question

Do not begin by inventing another ray-to-proof index.

Instead ask:

> When the exact collective computation is represented as a DAG, can the proof consequence live on the exact branch/subexpression whose semantics made the proof true?

Useful first forms may include test-only annotations such as:

- conservative support for a subtree;
- known-positive/known-lower-bound facts attached to a subtree;
- branch dominance or branch irrelevance proved from existing semantics;
- exact shared subexpressions whose evaluation can be reused across multiple composed SDFs.

The important invariant is that runtime relevance should increasingly arise from following the compiled execution graph itself.

## Meshes / non-SDF geometry

Do NOT derail the first rung by solving meshes immediately.

However, avoid naming the architecture so narrowly that meshes become impossible later.

Prefer a conceptual interface such as:

```
SpatialProgramNode {
    exact_kernel
    conservative_support
    dependencies
    proof_capabilities
    invalidation_provenance
}
```

An SDF node can supply a distance/gradient kernel. A mesh may later supply exact intersection/surface kernels plus conservative support. A NURBS surface may supply another exact kernel.

Unify execution/dependency/proof compilation, not necessarily the underlying geometry representation.

## Falsification rules

Reject or redesign the first synthesis representation if:

- "collective" means evaluating every SDF anyway;
- canonicalization changes semantics;
- shared subexpression bookkeeping costs more than the work saved;
- incremental repair fans out to most/all scene nodes for local edits without structural necessity;
- proof/direct annotations become a second source of truth;
- runtime still performs a generic global relevance search;
- exact parity fails.

## What would earn production work

Only after the test-only synthesized DAG demonstrates:

- exact parity;
- meaningful reduction in real authored evaluation work;
- bounded direct/relevance overhead;
- useful sharing across multiple SDFs;
- local dependency-frontier repair with unaffected state preservation;
- no frame/camera rebuild;

should the successor open a separate production A/B lane.

Then measure actual GPU wall/timestamp performance before landing production changes.

## Successor conduct

- Check live GitHub first.
- No big chungus reads; use targeted file/search reads.
- Preserve concurrent Suns' work.
- Leave a detailed Agent Intercom update after every substantive experimental verdict.
- If another Sun has already opened a successor PR, coordinate instead of forking the architecture accidentally.

## One-sentence torch

PR #301 proved that valuable Prophetic knowledge is being defeated by runtime relevance discovery; the next Sun should test whether compiling the collective spatial semantics into one incrementally repairable execution DAG lets relevance and proof become properties of the execution road itself rather than another structure the renderer must search.
