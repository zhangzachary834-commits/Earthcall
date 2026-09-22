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


## Rung 1C — multi-sample ambient stability + repeated authored repair

Successor-Sun pass on 2026-09-22 advanced the real OntoMath witness in commit `76cdc885`.

### Exact-head state before the change

PR #329 head `a8336c8` had focused CI run **#2544** green. The workflow itself explicitly builds and executes both:

- `scene_spatial_synthesis_dag_test`
- `scene_spatial_ontomath_synthesis_test`

So the earlier CI-integrity gap is closed at that head.

### Real OntoMath ambient/runtime witness

`scene_spatial_ontomath_synthesis_test.cpp` now evaluates the same compiled semantic DAG at five runtime samples:

`-13.0, -1.25, 0.0, 7.0, 42.5`

Between samples, evaluation memo values are explicitly invalidated. Every sample is compared against fresh `MathNode::evaluate()` exact authority.

The test asserts throughout that runtime sample movement changes **evaluation values only**:

- semantic node count is unchanged;
- canonical table size is unchanged;
- root compiled identity is unchanged;
- shared compiled identity is unchanged;
- independent `sdfB` identity is unchanged;
- semantic nodes rebuilt for ambient movement = 0.

This closes the real-OntoMath side of the rule that camera/runtime state must not rebuild authored semantic compilation.

### Multiple authored mutations + canonical reuse

The test still performs the local authored mutation:

`sdfA.bias: 5 -> 17`

and requires the repair frontier to remain exactly:

`biasA -> sdfA -> scene`

with three source visits and three new compiled nodes.

It now performs a second authored mutation:

`sdfA.bias: 17 -> 5`

This traverses the same three-source dependency frontier, but because the original semantic artifact is still canonicalized, the repair must create **zero** new nodes and take exactly three canonical hits. The compiled root returns to the original root ID.

The reverted graph is then checked for exact parity again across all five runtime samples.

This is an important distinction: incremental repair is not merely “append a new compiled copy every time.” Returning to previously-seen semantics can reuse the prior crystallized artifact.

### Boundary retained

This remains test-only architecture work. No renderer, WGSL, production SDF runtime, or production proof runtime was changed.

The next gate remains: exact-head CI for the new witness, followed by base reconciliation and then the first conservative proof/support annotation that can bypass exact DAG branch work while explicitly charging consultation and fallback costs.


## Rung 1D — first conservative support bypass on the execution DAG

Successor-Sun pass on 2026-09-22 advanced the synthetic execution witness in commit `4393ee85` after reconciling current default into the branch with merge commit `f4cee035`.

### Base reconciliation

Before the proof experiment, current `sync-from-earthcall-main` was `71d5d972`, eight commits beyond the old merge base. Those incoming commits touched none of PR #329's four changed files. The branch was reconciled with a real two-parent merge commit preserving both histories.

After reconciliation, GitHub reported:

- branch behind base: 0;
- branch mergeable: true;
- PR remains draft.

### First proof that changes execution

`scene_spatial_synthesis_dag_test.cpp` no longer merely counts a diagnostic proof annotation.

It now recognizes one deliberately narrow, exact algebraic shape:

```
scene = min(shared - biasA, shared - biasB)
```

The support proof is valid only when both subtraction branches point to the exact same canonical `shared` child and the bias leaves are exactly the declared authored inputs. Under that shape, the branch with the larger bias is <= the other branch for every runtime value of `shared`.

If the shape is not recognized, the proof builder refuses to prove anything.

When the proof is valid, the Min evaluator consults the proof on the execution node and evaluates only the proved winner child. The losing branch is not interpreted.

The test compares this support road against the full exact DAG authority and requires exact parity.

### Economics now charged explicitly

The witness separately counts:

- support-proof consultations;
- successful branch bypasses;
- support fallbacks;
- exact DAG nodes visited;
- support-road nodes visited;
- nodes avoided by proof;
- ordinary cache hits / leaf evaluations.

This is the first rung in this family where a proof artifact has direct measured execution profit rather than merely living beside the execution road.

### Runtime movement does not rebuild proof

The existing ambient/runtime sample movement remains value-state only.

After `p: 7 -> 8`:

- the semantic DAG is unchanged;
- the support proof remains valid;
- proof artifacts rebuilt for ambient movement = 0;
- the support road still bypasses the losing Min branch;
- exact parity remains required.

### Authored mutation invalidates, then falls open

The authored mutation remains:

`sdfA.bias: 5 -> 17`

The dirty semantic frontier is still exactly:

`biasA -> sdfA -> scene`

Because the support proof lives on `scene`, that authored semantic change invalidates the proof through the same dependency frontier.

Crucially, an invalid proof does not guess and does not retain optimization authority. The support evaluator records one consultation and one fallback, then evaluates both exact Min children using the still-valid unaffected cache entries.

This keeps exact semantics as authority.

### Targeted re-proof flips the winner

After the authored mutation is repaired, the same narrow proof builder is run again.

Before mutation:

- `biasA=5`
- `biasB=11`
- proved winner: `sdfB`

After mutation:

- `biasA=17`
- `biasB=11`
- proved winner: `sdfA`

The rebuilt support artifact therefore reverses the chosen branch and restores the branch bypass while preserving exact parity.

Only the semantic edit can require this re-proof; ambient/runtime movement still does not.

### Scope boundary retained

This experiment is still test-only.

No production renderer, WGSL, SDF runtime, or production Prophetic/proof runtime was modified.

The proof is intentionally a tiny exact shape witness rather than a general theorem system. Its purpose is to prove the architecture:

```
conservative theorem
    -> annotation on compiled execution node
    -> hot path consults one local artifact
    -> proved branch bypass
    -> stale/missing proof falls open to exact work
    -> authored premise change invalidates/rebuilds locally
```

### CI state

The pre-1D head `7de0de55` had the relevant SDF focused job green in run #2559.

The Rung 1D code head `4393ee85` triggered focused CI run **#2570**. At the time of this update its jobs were queued by GitHub Actions capacity; do not claim Rung 1D CI-green until an executor actually runs it.

The standalone witness could not be independently compiled from the local sandbox because that environment had no network route to GitHub. Treat Actions as the executable gate.

### Next decision after CI

If the synthetic support-bypass witness is green, the next research question is whether the same conservative proof shape can be expressed over the real `OntoMath::MathNode` compiled DAG without weakening identity/provenance rules.

Do not jump directly to renderer/WGSL productionization.

The next real-OntoMath rung should preserve all of these properties:

1. proof derives from canonical semantic child identity, not pretty-printed text;
2. proof has declared authored dependencies;
3. runtime samples do not rebuild it;
4. authored dependency changes invalidate only its dependent frontier;
5. invalid/missing proof falls open to exact compiled evaluation;
6. consultation and bypass/fallback economics are measured separately.
