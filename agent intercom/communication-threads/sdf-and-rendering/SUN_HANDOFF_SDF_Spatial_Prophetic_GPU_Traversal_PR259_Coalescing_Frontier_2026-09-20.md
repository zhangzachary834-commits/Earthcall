# Sun Handoff — SDF Spatial-Prophetic GPU Traversal, PR #259

**Date:** 2026-09-20 (America/Los_Angeles)  
**Outgoing agent:** GPT-5.6 Sol — “The Sun”  
**Incoming agent:** another Sun  
**Repository:** `zhangzachary834-commits/Earthcall`  
**PR:** #259 — `GPU spatial-Prophetic traversal for SDF range hierarchy`  
**Feature branch:** `sol/sdf-gpu-range-hierarchy-20260919`  
**Current feature HEAD:** `2b877c8efd6ff2a0f3c5df12278382befe4b65c5`  
**PR metadata base SHA:** `ea900fa323c05125a87732899cb2129732067973`  
**Current live `sync-from-earthcall-main` observed during this session:** `0cc1d23326fce7bf7f2b1c6bca7955cd911855d4` (“THE WORLD FORGER HAS MADE THE CROWN”)  
**PR state at handoff:** draft, mergeable=false, 17 changed files, 82 commits.

---

## 0. DO NOT RESTART

Continue from the current feature branch. Do **not** restart from PR #241, Sep-5 Perlin work, or an earlier dense-octree implementation.

Read current `AGENTS.md` before modifying anything. Preserve the repository’s explicit **NO BIG CHUNGUS** retrieval discipline: targeted symbol/path reads only, proportional to the question. Do not dump giant logs or full repository state into context.

Also preserve the PR-53 temporal rollback invariant. Before any eventual base sync, compare the feature head to **current live base**, inspect overlap/deletion magnitude, and refuse unexplained mass rewrites. Never “repair” by wholesale ours/theirs.

---

## 1. Constitutional invariants that MUST survive every optimization

These were repeatedly validated during this pass and are not negotiable:

1. The CPU SDF range hierarchy is the mathematical authority. GPU structures are derived acceleration only.
2. Unknown / ambiguous mathematics always fails open to the exact authored marcher.
3. “Zero-free” is **not** sufficient permission to skip.
4. Only a finite interval proving `rangeLo > 0` means **proved-positive outside space** and is lawful to skip.
5. A finite interval proving `rangeHi < 0` is inside solid. The current marcher must still observe that sign, because entering negative space is part of its surface-hit contract.
6. A whole draw may be culled only when the root is proved positive outside.
7. Exact-distance-field traversal remains independently quarantined:
   `kSdfRangeDistanceTraversalVerified = false`.
8. Raster proxy tightening remains independently quarantined:
   `kSdfRangeRasterTighteningVerified = false`.
9. After a lawful spatial proof jump, marcher history must not pretend the skipped samples occurred. Existing reset semantics must remain.
10. Do **not** clamp ordinary exact marcher steps to ambiguous hierarchy cell boundaries. The current design preserves the baseline sample trajectory whenever no proof authorizes a skip.
11. No epsilon-based root skipping. Shared-face/edge ambiguity fails open.
12. No speedup claim without the native Release A/B measurement at 2880×1800.

---

## 2. Historical foundation from PR #241

PR #241 established the non-spatial SDF optimization substrate:

- memoized WGSL Program reuse;
- structural vs parameter revision split;
- persistent SDF parameter buffer;
- cached heightfield proof;
- analytic value+gradient;
- interval reasoning;
- revision-cached `SdfRangeHierarchy`;
- zero-set raster proxy/cull;
- telemetry;
- native benchmark lane.

Historical native authored-Perlin numbers were roughly:

- plane 45° ~10.44 ms
- Perlin looking down ~1.46 ms
- Perlin horizon ~80.64 ms
- Perlin 45° ~117.83 ms (~8.5 FPS)

PR #259’s job is the next rung: reduce authored-field exact evaluations using spatial proof.

---

## 3. Benchmark / correctness infrastructure

The dedicated native benchmark is:

`tests/singularity/webgpu_sdf_range_perf_test.cpp`

It renders the authored terrain expression at **2880×1800** for:

- `horizon`
- `45deg`

OFF and ON use the same process/device. It records:

- wall-clock median;
- GPU timestamp median if available;
- traversal activation;
- recurring range-proof upload bytes;
- hierarchy/proof diagnostics.

The macOS runner currently gives no useful GPU timestamps, so wall median is the real observed metric.

Correctness is separately protected by the full SDF verification lane, including:

- CPU range soundness;
- generic WebGPU parity;
- distance parity;
- authored-color parity;
- authored-Perlin compiler/gradient gates;
- authored-Perlin range-traversal camera gate.

Do not weaken those tests to obtain performance.

---

## 4. What we tried, in order

### A. Dense GPU octree traversal — correct, disastrously slow

Initial implementation uploaded/traversed the complete depth-6 proof hierarchy.

Representative hierarchy:

- nodes: ~299,593
- positive lawful skip nodes: 6,333
- negative zero-free nodes: 78
- ambiguous leaves: 255,733

Native A/B from CI #1582:

- horizon OFF: **56.375 ms**
- horizon ON: **169.966 ms**
- ratio: **3.0149× slower**
- 45° OFF: **41.867 ms**
- 45° ON: **134.968 ms**
- ratio: **3.2237× slower**

Correctness was green, but the accelerator was economically backwards.

### B. Remove internal-node slab intersections + sparse proof-bearing pointer tree

We first stopped computing ray/AABB slab intersections at every internal node and retained only ancestry leading to positive proofs.

This materially helped.

Native result around CI #1607:

- horizon ON ~**109.93 ms**
- 45° ON ~**89.87 ms**

Penalty fell to roughly **2.03–2.12×**.

Correctness remained green.

### C. Flat fixed-depth positive-proof bit grid — current best consumption architecture

We replaced pointer-chasing through 48-byte GPU nodes with a compact depth-6 regular bitmap.

At depth 6:

- 64³ = 262,144 cells
- 1 bit/cell
- 8,192 u32 words
- **32 KiB** proof buffer

Semantics:

- bit = 1 → CPU theorem proved f>0 for that regular cell → lawful skip
- bit = 0 → no GPU proof → exact authored marcher owns the cell

No negative inference is encoded by a zero bit.

This was the best GPU representation so far.

CI #1661 was fully green.

Native A/B:

- horizon OFF **57.98 ms**, ON **80.13 ms** → ~**1.38× slower**
- 45° OFF **45.87 ms**, ON **63.67 ms** → ~**1.39× slower**

This is the strongest known pre-theorem-tightening performance point.

### D. ANY/ALL proof mip-pyramid — correct, rejected

We tried a two-bit semantic pyramid:

- ANY=0 → no positive proof descendants, exact macrocell
- ALL=1 → all fine descendants positive, skip macrocell
- mixed → descend

Everything passed parity/camera gates, but performance regressed badly.

CI #1671:

- horizon OFF **54.19 ms**, ON **136.80 ms** → **2.524× slower**
- 45° OFF **43.97 ms**, ON **111.60 ms** → **2.538× slower**

Rejected. The extra hierarchical bookkeeping outweighed the saved fine queries.

The pyramid commits remain in history for auditability; the branch was explicitly restored to the flat bit-grid rather than force-reset.

### E. Tighten the actual Perlin interval theorem — mathematically successful

The key theorem improvement was in `ScalarForm.cpp`.

Old:

`mix(a,b,t) = a + (b-a)t`

was propagated using generic interval arithmetic. That repeats `a` and `b` and creates artificial dependency inflation.

For Perlin fade, `t ∈ [0,1]`, so mix is a convex combination. We now bound the lower and upper endpoint envelopes directly, with outward rounding. If a future caller violates the [0,1] contract, the implementation falls back to generic interval arithmetic.

Commit:

`95aafcc85bb914d27f47f978bb33b56a7604bf59`
— `Tighten Perlin mix intervals by convex interpolation`

All correctness gates passed.

The theorem improved **dramatically**:

before:
- positive skip nodes: **6,333**
- ambiguous leaves: **255,733**

after:
- hierarchy nodes: **292,817**
- positive skip nodes: **37,744**
- negative zero-free nodes: **39,551**
- ambiguous leaves: **178,920**

This is a real mathematical win.

However the existing flat consumer got slower:

- horizon ratio ~**1.49×**
- 45° ratio ~**1.93×**

Interpretation: the theorem generated far more lawful proof, but most of it was spatially tiny, so consumption overhead grew faster than exact-evaluation savings.

### F. Proof-depth histogram — CRITICAL DIAGNOSIS

Benchmark instrumentation was added in:

`47b2d51061fa936ebc52fe5dbe8616c42f317299`
— `Report SDF proof usefulness by hierarchy depth`

Observed histogram with the convex theorem:

`positive = 0,0,0,0,0,388,37356`

`negative = 0,0,0,0,0,459,39092`

`ambiguous = 0,0,0,0,0,0,178920`

This is the most important current diagnosis.

Nearly all useful sign proofs are **depth-6 confetti**.

The theorem is now strong. The issue is **proof granularity / amortization**.

### G. True 3D Amanatides/Woo DDA over flat proof bits — correct, rejected

We then replaced per-cell:

- point→cell coordinate recomputation
- floor/clamp
- ray/AABB slab solve

with one 3D DDA setup and cheap cell stepping across positive runs.

Commit:

`019b604b05c765edcf474512749d2e4eeee19f7e`
— `Walk SDF proof bits with 3D DDA`

Every SDF verification gate passed, including the authored-Perlin camera corpus.

But native economics still lost:

- horizon ratio: **1.589×**
- 45° ratio: **1.907×**

Therefore DDA was rejected too.

The branch was explicitly restored to the known simpler O(1) flat lookup:

`f5d3ee9e21744ecf26b3522ef55f1d18964a819e`
— `Restore O(1) SDF proof lookup after DDA perf regression`

---

## 5. CURRENT FRONTIER: proof coalescing

The next hypothesis is **not another traversal algorithm**.

It is to make the GPU consume only proofs large enough to be profitable.

The CPU theorem should stay at depth 6.

The GPU proof-consumption grid should initially be depth 5.

New explicit constant added in the current HEAD:

`kSdfRangeProxyMaxDepth = 6`

This remains the CPU theorem depth.

`kSdfRangeGpuProofDepth = 5`

This is the intended GPU profitability depth.

Current HEAD:

`2b877c8efd6ff2a0f3c5df12278382befe4b65c5`
— `Separate SDF theorem depth from GPU proof depth`

**IMPORTANT: this is only the contract/header checkpoint. The coalescing implementation is NOT yet wired into WebGpuRenderer.cpp.**

Do not mistake the new constant for a completed optimization.

---

## 6. Exact next implementation rung

Implement bottom-up **positive proof coalescing** when building the GPU proof bitmap.

Desired semantics:

A depth-5 GPU cell may be marked positive if and only if:

1. the corresponding CPU theorem node itself proves `f > 0`; OR
2. its eight depth-6 children partition the same parent cell and **all eight children prove `f > 0`**.

The second rule is rigorous because the children cover the parent cell. If every child has a proved-positive interval, their union proves the parent is positive too.

Do **not** infer positive from:
- “some” positive children;
- zero-free children of mixed sign;
- negative children;
- ambiguous children;
- absent children;
- numerical sampling.

If a depth-5 parent cannot be proved positive under this rule, leave its GPU bit zero. All finer positive fragments beneath it are intentionally ignored by the accelerator and evaluated exactly. That is safe because zero means “no skip permission.”

This is the core profitability trade:

> willingly discard valid but tiny proofs that cost more to consume than the authored evaluations they save.

The CPU theorem keeps the full truth. The GPU gets only economically useful truth.

---

## 7. Suggested surgical implementation shape

Current packing code is in:

`src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`

around the `memo->rangeProofWords` build inside `drawImplicit`.

Current code rasterizes positive CPU nodes into a fixed depth-6 bitmap using:

- `proofDepth = kSdfRangeProxyMaxDepth`
- `proofDim = 1 << proofDepth`
- recursive `rasterizeProof(...)`

Change the GPU bitmap depth to:

`proofDepth = kSdfRangeGpuProofDepth`

But do **not** simply truncate depth-6 nodes.

You need a proof query such as conceptually:

`subtreeProvesPositive(sourceNode, sourceDepth, targetDepth)`

At target depth:

- return true if current node proves positive;
- if current node has eight children below target authority and every required child recursively proves positive across its region, return true;
- otherwise false.

Because the current CPU hierarchy may terminate early on a proof, a proved-positive ancestor trivially proves every target descendant beneath it.

Because the hierarchy also terminates at ambiguous/negative leaves, missing refinement must never be treated as positive.

The packed GPU bitmap should contain only target-depth positive cells.

Set:

`inst.rangeProofDepth = kSdfRangeGpuProofDepth`

not the CPU max depth.

Keep the flat O(1) WGSL lookup for this experiment. Do not bring the rejected pyramid or DDA back into the same benchmark.

---

## 8. Acceptance test for the coalescing rung

First demand correctness:

- compile;
- CPU range soundness;
- generic WebGPU SDF parity;
- distance parity;
- authored-color parity;
- authored-Perlin core gates;
- authored-Perlin camera traversal gate.

Then inspect benchmark diagnostics.

The benchmark should continue printing the full CPU theorem histogram. Add a separate GPU-coalesced proof count only if useful and if it does not perturb GPU timing.

Then compare native Release A/B against:

### historical best consumer
Flat bit grid before convex theorem:
- ~1.38× slowdown both cameras.

### current stronger theorem, uncoalesced
- horizon ~1.49×
- 45° ~1.93×

### rejected DDA
- horizon 1.589×
- 45° 1.907×

The immediate goal is not to declare victory unless ON < OFF. First prove that coalescing makes the stronger theorem economically better than the uncoalesced stronger-theorem consumer.

If depth 5 contains too few coalesced positives to help, measure before escalating.

Do not guess that depth 4/5/6 is optimal. Once the coalescer works, a small controlled benchmark sweep of **GPU proof depth only** may be justified. Keep the CPU theorem fixed at depth 6 so the variable under test is purely profitability granularity.

---

## 9. What NOT to do next

Do not:

- resurrect the dense octree;
- resurrect sparse pointer traversal;
- resurrect the ANY/ALL mip pyramid;
- resurrect the 3D DDA in the same pass;
- weaken the convex Perlin theorem;
- reduce CPU theorem depth merely for performance;
- skip negative cells;
- enable distance traversal;
- enable raster tightening;
- clamp exact marcher steps to grid boundaries;
- invent epsilon/tolerance skips;
- claim that more proofs automatically means faster rendering;
- merge live main until this isolated profitability experiment has a clean verdict;
- perform a giant base merge without PR-53 overlap audit.

---

## 10. Later mathematical opportunities, AFTER positive coalescing

These are real future rungs, but do not mix them into the next experiment.

### A. Negative proof as a rigorous root bracket

The convex theorem now finds ~39.5k negative zero-free cells.

We currently cannot skip them because `d <= 0` is the marcher’s entry signal.

A future design could use:
- previous proved-positive interval;
- next proved-negative interval;
- exact bracket/refinement at the sign transition

to accelerate toward the root without erasing the sign observation.

That needs a new correctness proof and should remain separate.

### B. Better Perlin range dependency treatment

The convex-mix theorem removed one major dependency explosion. There may be additional correlation-aware bounds available inside the lattice cell, but do not weaken the current exact-formula conservative theorem.

### C. Profitable GPU proof-depth selection

Once coalescing is implemented, test depth 4/5/6 as a **derived representation choice**, not a mathematical-authority change.

---

## 11. Base-sync warning

The live integration branch advanced during this work.

Observed live base:

`0cc1d23326fce7bf7f2b1c6bca7955cd911855d4`

PR metadata still reports older base:

`ea900fa323c05125a87732899cb2129732067973`

Earlier audit found overlap between #259 and newer base in renderer/radiance files, including:

- `.github/workflows/earthcall-ci.yml`
- `src/Singularity/Core/EngineRender.cpp`
- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/ScreenChannel.cpp/.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp/.hpp`
- `tests/singularity/channel_paths_test.cpp`

When eventually syncing, preserve current-main OntoMath Radiance / renderer ABI work and preserve #259’s SDF theorem/traversal work. Do not blindly merge stale feature versions over live main.

---

## 12. Current branch state in one sentence

PR #259 has progressed from a mathematically correct but ~3× slower dense GPU octree to a fully parity-verified flat 32-KiB proof bitmap and a much stronger Perlin interval theorem; the new theorem proves ~6× more positive space, but almost all of it appears at depth 6 and is too fine-grained to exploit profitably, so the next unfinished rung is **mathematically coalescing depth-6 positive proofs into a depth-5 GPU profitability grid while keeping the CPU theorem at depth 6**.

---

## 13. Message to the next Sun

Do not start over.

Do not optimize the wrong layer.

The theorem is finally speaking loudly. The GPU is listening too literally.

Make the derived representation listen only to proofs large enough to be worth hearing.

**Truth stays depth 6. Profitability may be depth 5. Exact marching remains sovereign wherever prophecy is silent.**

— The Sun
