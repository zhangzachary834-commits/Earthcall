# SUN UPDATE — PR #329 Rung 1D CI + Base Reconciliation

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

This is a continuation of `SUN_UPDATE_PR329_Scene_Spatial_Synthesis_DAG_Rung1_2026-09-22.md`. Do not restart the investigation.

The immediately prior technical rung is Rung 1D: a test-only conservative support proof on the synthetic scene-spatial execution DAG. It recognizes `min(shared-biasA, shared-biasB)`, bypasses the proved losing branch, survives ambient/runtime sample changes, invalidates on authored bias changes, falls open to exact evaluation when invalid, and can be locally re-proved with the winner reversed.

## CI verdict changed: Rung 1D is green

The prior handoff recorded Rung 1D CI as queued. That is now stale.

Exact-head PR workflow run **#2572** for commit `d7726ae1789888b1d25525cd841ead21b2bbaf9e` completed **successfully**. This head contains the Rung 1D code commit `4393ee853681a16491674bf8a62fe978c7504f18` plus the Intercom documentation commit. Therefore the synthetic proof-on-road bypass witness has now passed the focused CI gate.

Run #2570 was cancelled without a runner, but it is superseded by successful exact-head run #2572. Do not treat #2570 cancellation as a code failure.

## Concurrent-Sun collision detected and handled safely

During this pass another chat/Sun appended the exact user-facing Rung 1D report to the canonical Intercom thread as commit `7107426ffd690ac6e5c146f7254cd4a46fc55d1d` (`Mirror Rung 1D report to Zach in Agent Intercom`).

An attempted ref advance based on the older `d7726ae1` head correctly failed non-fast-forward. No force push was used. The live head was re-read and the other Sun's commit was preserved.

This is important coordination evidence: always re-read PR head immediately before writes because the hourly/chat successors can overlap.

## Current-default reconciliation

After Rung 1D went green, default advanced by one commit to `458949c178b853534862a9cb5d9b897cc4c13665` (`THE SPECIFIC TASKS AREA CTUALLY ORGANIZED NOW`). The PR became 1 commit behind and GitHub temporarily reported it non-mergeable.

That base commit is organizational/docs work and does not overlap the four PR #329 files.

The branch was reconciled with a real two-parent merge commit:

`d9dcb082623649cf79a4319dc2ab5df7739ad441`

Parents:
- live PR head `7107426ffd690ac6e5c146f7254cd4a46fc55d1d`
- current default `458949c178b853534862a9cb5d9b897cc4c13665`

The merge tree starts from the current-default tree and overlays the four exact PR blobs, including the concurrently-added Intercom report. No force update was used.

## What remains

Rung 1D synthetic proof-on-road is now green and base-reconciled. The next technical gate remains the one already named in the canonical handoff: mirror the same conservative support-proof semantics onto the **real OntoMath-compiled DAG** in `scene_spatial_ontomath_synthesis_test.cpp`.

That next rung must preserve all six invariants:

1. derive proof from canonical compiled semantic child identity, never pretty-printed text;
2. declare/track the authored premise dependencies;
3. runtime sample movement must not rebuild proof;
4. authored premise changes invalidate only the dependent proof/repair frontier;
5. missing/invalid proof falls open to exact compiled evaluation with no optimization authority;
6. count proof consultation, bypass, fallback, and avoided compiled-node work separately.

Do not move this into production renderer/WGSL yet. The next experiment belongs in the real OntoMath test adapter first.

## Immediate successor instruction

First check CI on merge head `d9dcb082...` (and any newer documentation-only head). If green, implement the real-OntoMath support-proof rung. If red, fix only the concrete regression. Re-read live PR head before every write to avoid another Sun collision. No Big Chungus repository dumps.

## MESSAGE TO ZACH — DOES SCENE-SPATIAL PROPHETIC COVER VOLUMETRICS / LIGHT / SHADOWS?

BROOOOOOOOOOOO ☀️🌌⚔️ **YES ARCHITECTURALLY — BUT NOT YET IMPLEMENTATION-WISE.**

The clean answer is:

**PR #329 right now only PROVES THE ARCHITECTURE on an SDF-like / real-OntoMath scalar execution DAG.**

It does **not yet** mean that the live renderer's:

- radiance fields,
- chroma fields,
- angular emission,
- visibility/shadows,
- volumetric density,
- extinction,
- scattering,
- phase functions,
- volumetric emission,

are already being accelerated by this new Scene-Spatial Prophetic DAG.

So if you ask:

> “If I merge PR #329 today, are my new density mediums automatically Prophetic-optimized?”

**NO. Not yet.**

But if you ask:

> “Is this architecture fundamentally supposed to include those things too, or is it only for hard SDF surfaces?”

**OOOOOOH YES. ABSOLUTELY THE FORMER.** 🔥🔥🔥

And the current Earthcall architecture actually makes that much more plausible than I realized.

The density/radiance Suns have already separated the world into independent authored mathematical channels:

```
solid geometry / SDF        F(p,...)
source radiance magnitude   rho(p,t)
source chroma               chi(p,t)
source angular emission     alpha(p,omega,t)

medium density              D(p,t)
medium extinction           sigma_t(p,t)

derived visibility          V(source,p,...)
```

And the volumetric roadmap already reserves further independent channels like:

```
medium scattering           sigma_s(p,t)
medium chroma               C_v(p,t)
phase                       Phi(p,wi,wo,t)
medium emission             E_v(p,omega,t)
```

THAT SEPARATION IS EXACTLY WHAT PROPHETIC NEEDS.

The Prophetic compiler should **not** become:

> “the SDF optimizer.”

It should become something closer to:

> **the ahead-of-time semantic field / scene execution synthesizer.**

The theorem carried by a node simply depends on *what that node means*.

For solid SDF geometry, useful Prophetic facts might be things like:

```
this branch is definitely farther than that branch
this region is definitely outside
this subtree cannot win this min()
this object cannot affect this sample
```

That lets us skip exact SDF work.

For **radiance fields**, the theorem vocabulary is different.

We may prove things like:

```
rho_i(p,t) = 0 throughout this domain
source i cannot contribute here
alpha_i is zero over this angular domain
this source contribution is bounded below the significance / exact-composition gate
two authored radiance subexpressions share one compiled semantic subtree
```

Then the execution road can avoid evaluating a source or part of a source expression when the theorem is sound.

But we must NEVER say:

```
rho_source == medium density
```

because Earthcall now explicitly guarantees those are different authored truths.

For **volumetric density**, this gets REALLY INTERESTING.

Density is currently a real independent OntoMath Piecewise:

```
D(p,t)
```

and extinction is independently:

```
sigma_t(p,t)
```

So the same synthesis machinery can potentially compile those ASTs into shared DAG structure and derive conservative facts such as:

```
D = 0 across this interval / region
sigma_t = 0 across this interval / region
this medium contributes nothing over this segment
this entire expression subtree is shared between multiple media
this coefficient change only invalidates this local theorem frontier
```

That could eventually drive **true semantic empty-space skipping** for volumetrics.

Instead of:

```
march
sample density
march
sample density
march
sample density
...
```

we could sometimes have:

```
Prophetic proof:
D(p,t) = 0 on this whole safe interval

        ↓

SKIP THE INTERVAL
```

😭😭😭

THAT is actually a much more profound use of Prophetic than only accelerating hard surfaces.

But volumetric transport has a major difference from ordinary SDF minimum selection:

**volume effects accumulate along a ray.**

So we cannot just reuse the exact same theorem:

```
“branch B wins min(), skip branch A”
```

for everything.

The architecture is shared.

The **proof algebra is channel-specific**.

For density we need things like support/range/integral-safe proofs.

For radiance we need contribution/support proofs.

For SDF composition we need distance/order/support proofs.

And **SHADOWS / VISIBILITY are the trickiest one.**

The radiance roadmap already correctly defines visibility as:

```
direct_i = E_i * V_i
```

where `E_i` is the source's authored emission and `V_i` is **derived transport truth from geometry**.

Meaning:

A wall moving between the light and the receiver must NOT mutate:

```
rho
chi
alpha
```

The light is still emitting.

What changed is the path.

So Prophetic visibility needs dependencies like:

```
source geometry / position
receiver sample
occluding geometry
relevant transforms
the exact visibility ray/domain
```

A conservative theorem might eventually say:

```
this region of the ray is proved empty
this blocker definitely intersects the segment
this geometry set cannot intersect this visibility query
```

and therefore skip huge amounts of exact shadow-ray work.

But if any relevant blocker/geometry premise changes:

```
invalidate THAT proof frontier
```

—not the radiance AST, not every light in the world, and not the entire renderer.

And if the visibility proof is absent or stale:

```
UNKNOWN
→ exact visibility query
```

NEVER:

```
UNKNOWN
→ assume visible
```

or:

```
UNKNOWN
→ assume shadowed
```

That is the exact same constitutional rule we just proved in Rung 1D:

> **A proof may remove work. A missing proof may never remove truth.**

And HERE is the part that gets me really excited:

the other Suns' density/radiance work is not competing with this PR.

**THEY ARE BUILDING THE SEMANTIC MATERIAL THIS COMPILER COULD EVENTUALLY CRYSTALLIZE.**

They are making:

```
rho
chi
alpha
D
sigma_t
...
```

into explicit independently-authored OntoMath truth with clean revision boundaries.

I am working on:

```
rich authored semantic graph
        ↓
canonical compiled DAG
        ↓
conservative theorems
        ↓
small direct execution artifacts
        ↓
hot path follows crystallized roads
```

Those two projects eventually MEET.

So my mental target after seeing the volumetric work is now broader than:

> Scene-Spatial Synthesis DAG for SDFs.

It is closer to:

> **Scene-Spatial / Field Semantic Synthesis: compile the mathematical truths governing geometry, radiance, participating media, and derived transport into shared incremental execution structure, then attach sound channel-specific proofs directly to the roads that consume them.**

And importantly, that does **NOT** mean one giant mega-DAG where every concept loses its identity.

We should preserve:

```
rho != D
D != sigma_t
emission != visibility
visibility != geometry
geometry != transport
```

while still allowing shared compiler machinery underneath:

```
canonical semantic identity
dependency provenance
incremental repair
proof invalidation
runtime-value caching
direct execution roads
exact fallback
```

That is the common substrate.

So I would describe the state like this:

```
RIGHT NOW
─────────
PR #329:
SDF-like / scalar OntoMath proof-of-architecture
✅ canonical DAG
✅ incremental repair
✅ runtime movement without semantic rebuild
✅ proof-on-road bypass
✅ stale-proof exact fallback

NOT YET
───────
❌ production SDF integration
❌ radiance integration
❌ density integration
❌ extinction integration
❌ visibility/shadow integration
❌ scattering/phase integration

ARCHITECTURAL DESTINATION
─────────────────────────
geometry ───────┐
radiance ───────┤
density ────────┤
extinction ─────┤
visibility ─────┤
scattering ─────┤
phase ──────────┤
                ↓
      semantic synthesis compiler
                ↓
        channel-specific proofs
                ↓
       crystallized execution roads
                ↓
          exact renderer truth
```

BROOOOOOOO AND I THINK THIS ACTUALLY TELLS US SOMETHING IMPORTANT ABOUT WHAT TO DO **BEFORE** productionizing PR #329.

After the real-OntoMath proof-on-road rung, I do **not** think we should immediately shove it into the solid SDF renderer and declare victory.

I think we should make one tiny cross-domain witness first:

Take maybe:

```
one SDF expression
one rho radiance expression
one D density expression
```

all represented as real authored OntoMath,

and prove that the **same canonical compiler / dependency / incremental-repair substrate** can host all three while keeping their theorem semantics completely separate.

If THAT works cleanly...

then we will know we're building a genuine Earthcall semantic execution substrate rather than accidentally baking an SDF optimization into a fancy abstraction.

AND THEN THE DENSITY SUNS AND THIS SUN BASICALLY COLLIDE INTO VOLTRON. 💀💀💀☀️🌈🌫️⚔️


## Rung 1E — real OntoMath proof-on-road

Successor-Sun pass on 2026-09-22 advanced the real OntoMath witness in code commit `b5cab71c8ec9ca59239608f9ba945cb78c278d81`, then reconciled the newly advanced canonical base through merge commit `5aca2ca5051886740fa0c75417bc7c5150250d43`.

### What changed

`tests/singularity/scene_spatial_ontomath_synthesis_test.cpp` now carries the same conservative support-proof semantics that Rung 1D previously proved only in the synthetic DAG.

This is now operating over the test compiler built from real Earthcall `OntoMath::MathNode` source trees.

The theorem remains deliberately narrow:

```
Union(
    shared - biasA,
    shared - biasB
)
```

Because Earthcall's `Union` evaluator is the scalar minimum in this witness, if both branches consume the **same canonical compiled shared subtree**, then the larger constant bias always produces the smaller branch result for every runtime value of that shared subtree.

### Canonical identity, not source-object coincidence

The two authored shared expressions are still distinct C++ `MathNode` objects.

The proof is accepted only after compilation shows that both source trees map to the same canonical compiled child ID.

It does not derive support from:

- source pointer equality;
- `MathNode::print()`;
- recursive pretty-text identity.

This closes the main Rung 1D -> real-OntoMath semantic bridge.

### Proof lives on the compiled execution road

The compiled `Union` node now carries test-only derived state:

- `supportProofValid`;
- `supportWinnerChild`.

With support enabled:

- valid proof -> evaluate only the proved winning compiled child;
- invalid/missing proof -> evaluate both children exactly.

Thus stale or absent proof has zero optimization authority.

### Authored premise dependencies are reverse-indexed

The compiler now records:

```
authored MathNode premise -> compiled nodes whose support proof depends on it
```

The initial theorem declares source premises for:

- scene root;
- both subtraction branches;
- both separately-authored shared-source trees;
- both authored bias leaves.

Proof invalidation therefore consumes the same changed-source frontier used by incremental semantic repair. It does not scan a global proof table to rediscover relevance.

### Runtime movement preserves proof

The existing five ambient/runtime samples remain:

`-13.0, -1.25, 0.0, 7.0, 42.5`

For each sample the witness now executes both:

1. full compiled exact evaluation;
2. support-enabled compiled evaluation.

It requires exact parity, one support consultation, one successful bypass, zero fallback, and fewer compiled nodes visited on the support road.

Across all runtime samples:

- semantic node count stays fixed;
- canonical table stays fixed;
- root/shared/sdfB compiled identities stay fixed;
- support proof remains valid;
- support proof build count stays fixed at one.

Runtime movement therefore causes zero semantic or support-proof rebuilds.

### Authored mutation: invalidate -> exact fallback -> targeted re-proof

The existing authored mutation remains:

`biasA: 5 -> 17`

Semantic repair remains exactly:

`biasA -> sdfA -> scene`

and still creates exactly three new compiled nodes while preserving the shared subtree and independent sdfB identity.

That changed-source frontier invalidates the old support theorem through the reverse premise index.

The newly repaired root initially has no theorem. Running with support enabled therefore records:

- one proof consultation;
- zero bypasses;
- one exact fallback;

and must still match `MathNode::evaluate()` exactly.

Only after that fallback witness does the compiler rebuild support.

The winner then flips:

```
before: biasA=5,  biasB=11 -> sdfB wins
after:  biasA=17, biasB=11 -> sdfA wins
```

The rebuilt support road again bypasses one compiled branch and records the avoided-node work.

### Reverting semantics does NOT resurrect stale proof

The second authored mutation restores:

`biasA: 17 -> 5`

As in Rung 1C, semantic repair takes three canonical hits and creates zero new compiled nodes; the original compiled root ID is reused.

But the test explicitly refuses to treat canonical artifact reuse as proof validity.

The changed authored frontier invalidates the currently valid theorem first. Then the theorem is rebuilt for the restored semantics.

This distinction is important:

```
compiled semantic artifact can be reusable
!=
derived proof artifact is automatically current
```

The restored proof again selects sdfB and bypasses correctly across all five ambient samples.

### Economics now measured on the real OntoMath compiler

The witness prints/counts:

- support theorem build count;
- proof consultations;
- bypasses;
- exact fallbacks;
- nodes avoided initially;
- nodes avoided across ambient samples;
- proof invalidations;
- nodes avoided after authored repair;
- proof invalidation without a global scan;
- canonical reuse on semantic revert.

No production renderer/WGSL code was changed.

### Base reconciliation

While Rung 1E was being implemented, canonical advanced four commits from `458949c1` to `686a5087`, including merged Volumetric V2 authored scattering + medium chroma.

Those incoming changes touched none of PR #329's five changed files.

The branch was reconciled with real two-parent merge commit:

`5aca2ca5051886740fa0c75417bc7c5150250d43`

After reconciliation:

- behind canonical: 0;
- GitHub mergeable: true;
- PR remains draft.

This is especially relevant architecturally because canonical now contains independent medium channels `D`, `sigma_t`, `sigma_s`, and `C_v` while this branch proves the generic compiled-DAG/proof machinery over real OntoMath.

### CI state

The exact reconciled code head `5aca2ca5` triggered focused workflow run **#2599**.

At handoff-writing time all macOS jobs are queued. Do not claim Rung 1E CI-green until the job containing `scene_spatial_ontomath_synthesis_test` actually executes successfully.

The focused workflow still explicitly builds and runs both scene-spatial witnesses.

### Next gate after Rung 1E CI

If Rung 1E is green, do not immediately productionize only the SDF lane.

Create one tiny **cross-domain semantic-synthesis witness** using real authored OntoMath from distinct channels, at minimum:

- one SDF/geometry expression;
- one source-radiance `rho` expression;
- one medium-density `D` expression.

The purpose is not visual output yet. It is to prove the compiler substrate can be shared while theorem meaning cannot leak across channels.

The witness should intentionally include at least one numerically/structurally identical subexpression across two channels and force an explicit architectural answer to this question:

> May the compiled calculation node be shared while proof metadata remains channel-scoped, or must semantic role participate in compiled identity when channel meaning changes the valid theorem algebra?

Do not permit a density theorem (for example, zero-support over a segment) to become radiance authority merely because `D` and `rho` happen to contain byte-identical mathematics.

Preserve:

```
rho != D
D != sigma_t != sigma_s != C_v
emission != visibility
geometry != transport
```

while testing whether the common substrate can still share:

- canonical mathematical execution;
- dependency provenance;
- incremental repair;
- value caching;
- proof invalidation infrastructure.

That cross-domain witness is the next architectural gate before choosing a production integration lane.
