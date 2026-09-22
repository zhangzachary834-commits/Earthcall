# SUN UPDATE — PR #329 Scene-Spatial Synthesis DAG Rung 1

Date: 2026-09-22
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
Draft PR: #329
Base at branch creation: `409d595bd8b452f5cc4e6c8d7151b1b666933f2d`

## Live-state check

Before branching, canonical `sync-from-earthcall-main` had advanced to `409d595b` via merged PR #327. No existing `scene-spatial` successor branch was found, so this Sun created a new branch rather than colliding with another successor.

## What landed in the first pass

Added a test-only witness:

`tests/singularity/scene_spatial_synthesis_dag_test.cpp`

No production renderer, WGSL, SDF runtime, or proof runtime was modified.

The witness is intentionally small. It asks whether the architecture can express the post-#301 invariants before we bind it to production classes.

It constructs two composed SDF-like branches that share the exact subtree:

```
shared = (ambient:p + shared.shift) * shared.scale
sdfA   = shared - sdfA.bias
sdfB   = shared - sdfB.bias
scene  = min(sdfA, sdfB)
```

The naive authority evaluates `shared` independently for A and B. The synthesized DAG interns the identical expression once and both branches point to the same stable node ID.

## What the witness measures/asserts

1. Exact parity between naive independent evaluation and synthesized DAG evaluation.
2. Real authored/input leaf evaluations, not merely top-level DAG queries.
3. Shared-expression cache hits and evaluations avoided.
4. DAG nodes actually visited.
5. Proof/direct annotations consulted.
6. Approximate resident artifact bytes/node count.
7. A local semantic mutation witness.

The mutation is `sdfA.bias`.

Its dependency frontier must be exactly:

- the `sdfA.bias` input leaf;
- the `sdfA` subtraction;
- the scene `min` root.

The test explicitly rejects invalidating:

- the shared `(p + shift) * scale` subtree;
- the independent `sdfB` branch.

Thus the first representation makes the crucial rule executable: a local semantic edit repairs the transitive dependent frontier while preserving unaffected compiled state.

## Prophetic placement

The test attaches a diagnostic `propheticPositive` annotation directly to `sdfA`, the execution node whose semantics derive/consume that fact. The annotation is not truth authority and does not change exact evaluation. This is deliberate: the first rung tests whether proof can live *on the execution road* instead of requiring a second ray-to-proof search structure.

## Expected census shape

For the current tiny scene, naive evaluation performs eight leaf/input evaluations: the shared three-input expression twice plus two biases. DAG evaluation performs the five unique input leaves once, with the second branch reusing the shared node through memoization. Exact output must match.

This is not yet a production-performance claim. It is a structural/economic witness that collective compilation can remove genuine duplicate authored work without hiding a global scan.

## What is still missing / next pass

PR #329 should remain draft until CI runs and the witness is strengthened.

Next work should, in order:

1. Check exact-head CI and fix compile/test failures without broadening scope.
2. Add a mutation execution pass, not only invalidation-set accounting: preserve cached values for unaffected nodes, recompute only dirty nodes, and compare the repaired result to a fresh exact authority evaluation.
3. Measure bytes/nodes actually rewritten by repair rather than equating `dirty.size()` with repair work.
4. Replace or supplement the synthetic arithmetic graph with a small adapter/compiler over real Earthcall `OntoMath::MathNode` authored expressions, while preserving canonical semantic identity. Do not canonicalize merely by pretty-printed text if that can collapse semantically distinct nodes.
5. Add multiple sample points / mutations and prove camera/runtime ambient input changes invalidate evaluation values but do NOT rebuild the semantic DAG or proof artifacts.
6. Only after those pass, test a conservative support / Prophetic annotation that can prune or bypass exact work on a DAG branch. Charge annotation consultations and fallback work explicitly.

## Hard boundaries retained

- No ordinary-frame rebuild.
- No camera-motion rebuild of semantic/proof/direct artifacts.
- Local authored edits repair dependency frontiers only.
- Exact semantics remain authority.
- Missing optimization falls open to exact evaluation.
- No hidden global relevance scan.
- No production renderer/WGSL work until this test-only family demonstrates strong economics and earns a separate A/B.
- Do not resurrect #296 one-AABB, #288 depth-5 density, DDA, octree/sparse tree, mip pyramid, negative proof, raster tightening, distance-field traversal, grid-scale hoist, maximal-run scanning, or the rejected entry/direction atlas without materially new evidence.

## Verdict so far

The first scene-spatial synthesis representation is coherent enough to test. It encodes common-subexpression sharing and local dependency-frontier repair directly, without an external spatial relevance lookup. It has not yet earned production work.


## Rung 1B — executable repair + real OntoMath bridge

The branch has advanced substantially beyond the first structural witness.

### Executable incremental repair

`scene_spatial_synthesis_dag_test.cpp` now performs the mutation rather than merely counting the dependency frontier.

After an initial exact-parity evaluation, it performs two distinct change classes:

1. **Ambient/runtime sample change** (`p: 7 -> 8`)
   - invalidates evaluation-cache values downstream of the ambient input;
   - preserves the semantic DAG and canonical node identities;
   - preserves the attached Prophetic annotation;
   - performs zero semantic-node rebuilds and zero proof-artifact rebuilds;
   - re-evaluates against a fresh exact authority result.

2. **Authored semantic change** (`sdfA.bias: 5 -> 17`)
   - invalidates exactly `biasA -> sdfA -> scene`;
   - erases only those cached values;
   - preserves the shared subtree and independent `sdfB` cached values;
   - invalidates the Prophetic annotation attached to the dirty `sdfA` branch;
   - recomputes exactly the dirty execution frontier;
   - compares the repaired value both to a fresh naive authority evaluation and to a fresh full DAG evaluation.

The test now reports actual payload/cache bytes rewritten rather than equating the dirty-set size with physical work.

### Real Earthcall OntoMath adapter

Added `tests/singularity/scene_spatial_ontomath_synthesis_test.cpp`.

This is the first bridge from real `OntoMath::MathNode` authored ASTs into the scene-spatial synthesis idea.

It constructs two separately-authored SDF-like MathNode branches whose shared expression trees are distinct C++ objects but semantically identical. The test compiler canonicalizes from:

- the real MathNode opcode;
- local semantic payload (`variableName`, `stringArg`, exact normalized ScalarForm JSON for scalar leaves);
- already-canonical child IDs.

It deliberately does **not** use `MathNode::print()` as semantic identity and does not perform recursive whole-subtree serialization as a hidden lookup key.

The initial 15-node authored tree synthesizes into fewer compiled execution nodes by sharing the duplicate subtree. Exact compiled evaluation is checked against `MathNode::evaluate()`.

Then `sdfA`'s real ScalarLeaf bias is mutated in place. Source-parent provenance repairs only:

```
biasA -> sdfA -> scene
```

The compiler does not scan the whole authored scene or canonical table to rediscover relevance. The shared subtree and independent sdfB branch retain their exact compiled IDs. The repair creates exactly three replacement compiled nodes.

### CI integrity correction

A live audit found that CMake auto-configured these tests, but the focused GitHub workflow did not build or execute them. Therefore earlier green CI was **not** sufficient evidence for this rung.

Commit `0395c3bb` explicitly adds both witnesses to the CPU portion of the SDF range-proxy focused CI job.

CI run **#2542** is therefore the first honest exact-head CI gate for the executable-repair + real-OntoMath rung.

Do not graduate PR #329 based on earlier green runs.

### Live base drift

At this update, canonical default is `22756eb5` and the PR is three commits behind. The latest default commit is the docs-only “Property as metal” agenda update, which is conceptually aligned with the next dependency-addressing question but does not alter the implementation files in this rung.

Reconcile current base after the test witness is green, then rerun exact-head CI before promoting the PR out of draft.
