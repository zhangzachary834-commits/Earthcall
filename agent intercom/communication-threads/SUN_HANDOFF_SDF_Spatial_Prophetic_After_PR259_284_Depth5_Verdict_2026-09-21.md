# SUN HANDOFF — SDF Spatial-Prophetic Traversal After PR259/PR284 Merge + Depth-5 Verdict

**Date:** 2026-09-21  
**From:** GPT-5.6 Sol ("The Sun")  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Live integration branch at handoff:** `sync-from-earthcall-main`  
**Live head observed:** `d27b49fe5cca7d7bf09a5db394b4dc3189436ee4`

## Read this first

Do **not** restart the SDF acceleration work from an older renderer snapshot.

PR #259 and PR #284 are now merged into `sync-from-earthcall-main`. The live branch has advanced again after those merges, so begin from the current live head and preserve concurrent renderer/Radiance work by three-way integration only.

The old pre-merge handoff remains useful history:

`agent intercom/communication-threads/SUN_HANDOFF_PR259_Finalized_SDF_Renderer_Next_Perf_Rungs_2026-09-21.md`

This document supersedes its "next rung" status.

---

## What is merged

### PR #259 — GPU spatial-Prophetic traversal for SDF range hierarchy

Merged into `sync-from-earthcall-main`.

- PR: #259
- merge commit: `be7e02afc5316488ca50ee8ab7e0b9708113cec4`
- final pre-merge feature head: `c58cea7e1d50db19deeb5a379005fb9593ccc0d9`

The contract that landed:

- CPU `SdfRangeHierarchy` remains the mathematical authority.
- CPU theorem refines adaptively to depth 6.
- GPU consumes a conservative fixed-depth positive-proof bitmap.
- A set bit means the CPU theorem proved the entire regular cell has `f > 0`.
- A clear bit means **no proof**; exact authored marching remains sovereign.
- Only positive/outside proof authorizes skip.
- Negative/zero-free proof is not used as skip authority.
- Malformed or unsupported states fail open to the exact marcher.
- FieldNode volumetric density is excluded from this zero-set acceleration path.
- Raster tightening remains quarantined.
- Distance-field/over-relaxed traversal remains quarantined.
- Proof storage is persistent on GPU; steady-state recurring proof upload witness is 0 bytes.
- The live Rung-6 renderer semantics, including authored angular emission `alpha(p, omega, t)`, survived integration.

The ScreenChannel property is:

`sdfRangeProxyEnabled`

It remains default **false**. Merging #259 did not silently turn the experimental traversal on everywhere.

### Living Studio repair carried by #259

The integration also fixed the initial Synthesis Studio Living voice token.

Fresh launch -> enter Living -> press pads before touching TRI/SINE/SQR.

Expected: pads sound immediately using the default triangle voice and strike their matching resonators.

The stored selector token is now the canonical `"triangle"`, while emitted timbre identity remains the authored triangle timbre.

---

## PR #284 — paired native performance witness

Merged into `sync-from-earthcall-main`.

- PR: #284
- merge commit: `1149e6a9b9b752de7ec4802825c1a58e31a0cdb9`
- feature head: `1d33b8a52348598fab17721a62ed96e8890f7b57`
- production renderer change: **none**
- changed file: `tests/singularity/webgpu_sdf_range_perf_test.cpp`

#284 replaced the old sequential OFF-then-ON measurement with balanced paired AB/BA sampling.

The benchmark now uses two persistent `WebGpuRenderer` states sharing one WebGPU device:

- persistent OFF renderer;
- persistent ON renderer.

This avoids contaminating ON samples by toggling one renderer OFF and thereby clearing/re-uploading the proof cache.

Per camera:

- 6 warmups per mode;
- 10 measured samples per mode;
- exactly 5 AB pairs and 5 BA pairs;
- historical ratio-of-medians retained;
- paired wall ratio and paired wall delta emitted;
- GPU timestamp fields retained when available;
- active traversal witness retained;
- recurring proof upload must remain 0 bytes.

On the macOS runner, GPU timestamps were unavailable, so paired wall-time is the decision signal.

---

## Depth-4 baseline: the residual cost is real

The paired #284 Release witness established that the old slowdown was **not merely sequential thermal/order drift**.

Authored-Perlin proof ladder:

- depth 3: 0 / 512 positive cells — 64 bytes
- depth 4: 11 / 4096 positive cells — 512 bytes
- depth 5: 1864 / 32768 positive cells — 4096 bytes
- depth 6: 40460 / 262144 positive cells — 32768 bytes

Depth-4 paired result:

### Horizon

- paired wall ratio median: **1.2505x**
- paired wall delta median: **+13.826666 ms**

### 45 degrees

- paired wall ratio median: **1.4021x**
- paired wall delta median: **+17.300063 ms**

Traversal was active on 10/10 ON draws per view.

Recurring range upload bytes remained 0.

Therefore the current depth-4 theorem consumption path is correctness-sound but is a net performance loss on this native authored-Perlin witness.

---

## Rejected micro-rung: PR #287

PR #287 tested one shader micro-optimization:

> Hoist the proof-grid coordinate scale `dim / (2 * extent)` once per fragment so repeated cell classification uses multiply/add/floor instead of recomputing three extent-normalization divisions.

Branch:

`sol/sdf-grid-scale-hotpath-20260921`

Commit:

`9eefb3fa7e1c2972793a43196be8509783ec4b70`

The parity/proof suite stayed green, so this was a clean performance experiment.

Result:

### Horizon

- baseline #284: **1.2505x**, +13.826666 ms
- #287: **1.3580x**, +20.594854 ms

### 45 degrees

- baseline #284: **1.4021x**, +17.300063 ms
- #287: **1.4562x**, +20.131812 ms

Conclusion: **rejected**.

PR #287 was closed and explicitly marked `[REJECTED]`.

Do not resurrect this exact transform without new backend/compiler evidence. Algebraically cheaper source did not produce a cheaper Metal shader; register/liveness/compiler effects are plausible, but that causal explanation was not directly measured.

---

## Depth-5 proof-density experiment: PR #288

PR #288 is still open and Draft at this handoff.

- PR: #288
- branch: `sol/sdf-proof-depth5-ab-20260921`
- head: `41948b155487f06091de36585cb888c3dbde7686`
- current base still points to the old stacked #284 branch:
  `sol/sdf-paired-ab-harness-20260921`
- diff: exactly one production line:
  `kSdfRangeGpuProofDepth = 4` -> `5`

The full CI run completed **successfully**:

- SDF range-proxy verification: green
- Focused CPU tests: green
- Slow Adapter independent clock: green
- native Release authored-Perlin A/B: green

Release job: `106468309190`

The depth-5 performance result does **not** justify promotion.

### Horizon — depth 5

- ratio-of-medians: 1.4294x
- paired wall ratio median: **1.3945x**
- paired wall delta median: **+21.835916 ms**

This is materially worse than depth 4's 1.2505x / +13.826666 ms.

### 45 degrees — depth 5

- ratio-of-medians: 1.3760x
- paired wall ratio median: **1.3701x**
- paired wall delta median: **+17.562563 ms**

The ratio is slightly lower than depth 4's 1.4021x, but the absolute paired delta is slightly worse than depth 4's +17.300063 ms. This is not convincing evidence of a win.

Traversal remained active on 10/10 ON draws.

Recurring proof uploads remained 0 bytes.

### Verdict

**Depth 5 should be treated as rejected on current evidence.**

The 4 KB grid contains dramatically more lawful positive cells, but richer proof density by itself did not turn the traversal into a net acceleration.

Recommended cleanup for the next Sun:

1. Update PR #288 body with the final paired numbers and rejection.
2. Rename/title it as rejected if useful.
3. Close #288.
4. Do not merge the depth-5 constant.
5. Because #288 is being rejected, there is no need to retarget it to live main merely to preserve a one-line experiment.

---

## What the evidence now says

We have ruled out two tempting explanations/solutions:

1. **"The slowdown is probably benchmark drift."**
   - Rejected by #284's balanced AB/BA persistent-renderer witness.

2. **"The proof is just too sparse; use depth 5."**
   - Not supported by #288. Horizon became substantially worse and 45-degree did not produce a meaningful absolute-time improvement.

We also rejected one compiler-level micro-hoist (#287).

The important new inference is therefore not "try another random shader trick."

The next investigation should ask:

> Where is the traversal tax actually paid, and how much exact authored evaluation does each proof lookup save?

Depth 5 creates many more positive cells but also much smaller cells. A ray can therefore cross/classify more proof cells. The current evidence is consistent with classification/control-flow cost outweighing saved authored-field evaluations, but that mechanism is still a hypothesis until instrumented.

---

## Recommended next rung

Do **measurement/profiling before another architectural mutation**.

Useful questions:

1. How many times per rendered fragment does `rangeCandidate()` run?
2. How many proof cells are classified per ON frame?
3. How many are positive skips versus clear-bit handoffs?
4. How much field-evaluation work is actually avoided by each successful skip?
5. How far, in field-space distance or equivalent exact-march steps, does each positive proof jump save?
6. Does divergence dominate because neighboring fragments classify different proof cells/branches?
7. Does the proof-buffer lookup itself matter, or is the dominant cost cell-boundary arithmetic/control flow?
8. Is most of the tax paid by fragments that never obtain a useful positive skip?

Prefer low-distortion instrumentation. Do not add permanent atomics to the hot shader and then benchmark the instrumented cost as though it were production performance. If shader counters are required, use them as a separate diagnostic rung.

A promising architectural direction, **only after measurement**, would be to find a proof representation/query that has a better ratio of:

`useful authored evaluations avoided / proof classifications performed`

rather than simply increasing regular-grid resolution.

---

## Do not resurrect without new evidence

Prior rejected/quarantined directions from this thread still stand:

- dense depth-6 GPU octree;
- sparse pointer tree;
- ANY/ALL mip pyramid;
- full 3D Amanatides/Woo DDA traversal;
- negative/zero-free skip authority;
- raster tightening;
- distance-field traversal;
- lowering CPU theorem depth merely for speed;
- prepared-query invariant hoist as a claimed optimization;
- grid-coordinate scale hoist (#287);
- depth-5 regular bitmap merely for greater proof density (#288).

If new profiling changes the evidence, revisit deliberately rather than by folklore.

---

## Anti-rollback / integration warning

Current live head observed at handoff:

`d27b49fe5cca7d7bf09a5db394b4dc3189436ee4`

This is newer than both merge commits.

Do not reset the renderer to #259's merge commit or #284's merge commit.

The renderer has concurrent OntoMath/Radiance work. Preserve current live authority and use narrow branches.

PR #259 and #284 are already merged. Do not reopen/reimplement them.

---

## Person-facing expectations right now

Normal launch should **not** suddenly be faster because the generic SDF range traversal remains default-off through `@screen-channel.sdfRangeProxyEnabled`.

With traversal OFF, rendering should behave as the established exact baseline.

With traversal explicitly ON for eligible surface-only authored SDFs, pixels should remain parity-equivalent; only work scheduling is intended to differ.

Visible integration repair to verify:

- fresh launch;
- enter Synthesis Studio Living;
- before touching any voice selector, press C5 and other pads;
- they should immediately sound in the default triangle voice and strike matching resonators.

---

## Immediate next-Sun checklist

1. Start from current `sync-from-earthcall-main`; do not start from an old SDF branch.
2. Read this file and the earlier finalized PR259 handoff.
3. Close/document #288 as rejected from its completed paired evidence.
4. Keep #259/#284 merged state intact.
5. Do not enable `sdfRangeProxyEnabled` globally as a "performance improvement"; current measured traversal is slower on the maintained witness.
6. Instrument the cost structure before inventing another traversal.
7. Make one hypothesis-changing edit per branch/A-B.
8. Preserve positive-proof-only authority and fail-open semantics.
9. Preserve Rung-6 Radiance/Chroma/angular-emission code while touching renderer files.
10. If the next rung cannot beat OFF honestly, say so. The theorem is valuable even if this particular GPU consumer is not yet fast.

The question now is no longer:

> Can Earthcall prove empty space?

It can.

The question is:

> What GPU consumption structure can make a conservative proof cheaper to consult than the authored work it prevents?

That is the next rung.

— GPT-5.6 Sol ("The Sun")
