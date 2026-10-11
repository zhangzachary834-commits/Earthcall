# SourceRho literal-zero visibility elision — native A/B evidence and economic verdict

**Date:** 2026-10-10 (PDT)  
**Commission:** successor to PR #445; existing draft [PR #482](https://github.com/zhangzachary834-commits/Earthcall/pull/482)  
**Experimental branch:** `sol/sourcerho-zero-visibility-elision-20260929`  
**Exact tested CODE commit:** `fde0d5052350d9611ecf10f72cbcbf37e43266f9`  
**Canonical observed in this verdict pass:** `ca34af17ce150062f4b83859c2ff1a199a6ef601` (`sync-from-earthcall-main`)  
**Native evidence:** GitHub Actions [run #37973693652](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37973693652), macOS job `SDF range-proxy verification`, step `Verify SourceRho zero-authority native pixel A/B` **success**. Recovered and inspected the tiny `webgpu-object-parity-log` artifact #11641566316 (663-byte ZIP), containing `webgpu-source-rho-authority-perf.log` (1,250 bytes). The job's shared CPU-SDF step failed later; **the SourceRho step itself passed**.

## Final bounded verdict

**Native renderer exactness / hostile-lifetime gates: PASS for the tested scene and code.**  
**Actual GPU shadow-work elimination: PASS.**  
**Material completed-frame wall-time economics: NOT DEMONSTRATED — economic rejection; do not promote or merge the experimental elision on these results.**

The proven literal-zero SourceRho eliminates **5,624,857 actually executed shadow-loop signed-distance steps** and **290,151 `sourceVisibility` calls** attributable to source slot 0. This is a genuine per-source GPU execution reduction, *not* a CPU-side authority-opportunity estimate. Nevertheless the paired uninstrumented run shows only a **0.036499 ms**, approximately **0.38%**, difference in median measured wall time. There is no usable GPU timestamp measurement to independently attribute a GPU-time win. The 12-pair medians alone do not establish statistical reliability or a material frame benefit. Under the handoff's explicit instruction to stop when the expensive consequence does not pay, the narrow experiment ends **without economic promotion**.

## Deterministic GPU execution evidence

The diagnostic uses an opt-in fragment storage/atomic group and separately copied GPU counter buffers **after** the 12 uninstrumented paired timings. The counters include *entered shadow-loop iterations only*: the preliminary receiver surface-bias probe is excluded.

| Actual executed per-source work | Exact | Theorem authority | Difference |
| --- | ---: | ---: | ---: |
| Proven-dark source (slot 0) `sourceVisibility` calls | 290,151 | 0 | -290,151 |
| Proven-dark source (slot 0) shadow-loop SDF steps | 5,624,857 | 0 | -5,624,857 |
| Independent live source (slot 1) `sourceVisibility` calls | 290,151 | 290,151 | 0 |
| Independent live source (slot 1) shadow-loop SDF steps | 6,778,135 | 6,778,135 | 0 |
| Total counted shadow-loop SDF steps | **12,402,992** | **6,778,135** | **-45.35%** |

This is **not** whole-source erasure, density authority, response simplification, or an inference about other workloads. The independent live source's measured visibility work is exactly conserved.

## Economics from the UNINSTRUMENTED 12-pair timing run

| Reported metric | Exact | Authoritative |
| --- | ---: | ---: |
| Warm median CPU/completed-frame wall (ms) | 9.718958 | 9.682459 |
| `exact / authority` warmed wall ratio | | **1.003770×** |
| Cold wall (ms, descriptive, order/cache confounded) | 312.054542 | 249.499500 |
| Cold emitted WGSL bytes | 46,957 | 46,875 |
| Cold parameter upload bytes | 40 | 36 |
| Recurring shader compiles | 0 | 0 |
| Program cache hits | 12 | 12 |
| Recurring parameter upload bytes | 0 | 0 |
| GPU timestamp valid samples | **0** | **0** |
| GPU time | **unavailable** | **unavailable** |

Reported `exact_gpu_median_ms=0.000000` and `authority_gpu_median_ms=0.000000` are **not valid zero-duration samples**: the artifact explicitly reports zero valid GPU timing samples. The 1.003770 ratio is *wall*, not GPU-time speedup. No variance distribution or statistical uncertainty was available in the artifact; a tiny difference must not be marketed as a robust performance win.

Authority-only lifecycle and memory: semantic artifact **152 bytes** plus **2-byte source authority mask**; semantic setup **12,291 ns**; local repair **9,000 ns**; post-repair draw **13.893792 ms** (descriptive; not compared against a paired exact repair draw). Cumulative observed accounting: 20 vessel observations; 3 semantic builds / 17 semantic cache hits; 2 slot builds / 14 slot repairs; 22 handle publications, validations and proof reads each; 104 metadata tests; 16 proof fallbacks; 7 authority applications. The artifact does not independently report a GPU-executed rho-body evaluation count or a reliable pipeline-specific GPU duration; do not manufacture either. This absence narrows attribution but cannot turn the observed immaterial wall result into earned promotion.

## Native semantic and lifetime proof (tested, not universalized)

The dedicated native macOS WebGPU test returned success with the current **visibility-ON** witness, a physical occluder, visibility-OFF vs visibility-ON pixel falsifier, byte-exact exact-vs-authoritative output, diagnostic-vs-ordinary pixel equality, and GPU counter assertions above. Its complete success path also exercises zero-to-nonzero repair; producer same-revision replacement and exact first-draw quarantine; experiment OFF/ON; removal/re-add; fresh-generation handling; slot reorder/reuse; and separate positive recovery. Ambient contribution, independent material response and V1–V4 are **not authorized to disappear**. This test is strong native evidence for its particular bounded fixture; it is not a general source, scene, or floating-point theorem.

The compiled code also preserves the canonical transport denominator `s.raw / max(gradLen, 1.0)` in `sourceTransportSignedStep`. SourceRho remains distinct from transport visibility and medium density: `rho_source != V_transport != D_medium`. A recorded source theorem belongs to the already-selected ordered slot with stable producer identity, authored revision, artifact generation, and channel/lifetime admission gates. All unknown/stale states fail open to exact `sourceVisibility`, with dirty-slot repair and no hidden hot-path search.

## CI classification and provenance limits

For the *same tested code head* `fde0d505...`, run #37973693652 is **overall red** due to other jobs, despite successful SourceRho native step and successful build of SDF proof/GPU parity witnesses.

- `Focused CPU tests (macOS)`: overall failure, previous targeted inspection identified `law_line_test` at line 252 (50/51 other focused tests passed). This is not demonstrated as caused by PR #482.
- `SDF range-proxy verification (macOS)`: dedicated SourceRho native step **success**; later `Run CPU SDF proof witnesses` **failure**; exact failed CPU executable not securely classified because the full job log was approximately 258.8 MB and beyond safe retrieval. The next generic WebGPU steps were **skipped**, not failed. The subsequent artifact upload succeeded and contains the SourceRho evidence.
- `Slow Adapter independent clock (macOS)`: authored-world performance step failed, previously identified timeout on `chess_app.json`, not demonstrated SourceRho-owned.
- `SDF authored-Perlin A/B (macOS Release)`: skipped due dependency.

At this pass, exact-head workflow and status queries for `fde0d505...` returned no PR-triggered matches, but explicit run #37973693652 was retrieved by run ID with the job and artifact above. Empty query results do not negate a real native step nor imply a new PASS.

The canonical and experimental branches are substantially diverged. The prior concrete `sourceTransportSignedStep` denominator semantic overlap was already reconciled in one bounded successor commit; targeted canonical movement alone does not justify a broad merge. This negative promotion verdict calls for **no canonical landing**. Preserve ancestry, do not force-push or snapshot-overwrite newer canonical, and do not merge PR #482 absent explicit authorization.

## Interpretation, refusals, and terminal disposition

1. **Rejected:** “millions of SDF steps eliminated implies a material frame win.” Work falls by 45.35% in the counted visibility loop, while measured median wall changes by only about 0.38%. GPU-duration attribution is unavailable.
2. **Rejected:** “0.000 GPU timestamp means free GPU work.” There were zero valid samples.
3. **Rejected:** “overall red CI means SourceRho parity failed.” The dedicated native step succeeded; other jobs failed afterward or elsewhere.
4. **Rejected:** “a SourceRho literal-zero theorem licenses MediumDensity, general expressions, ambient/source deletion, receiver-response rewriting, GI or Scene-DAG relevance.” None are in scope.
5. **Rejected:** artificially altering source semantics or widening the experiment until a favorable headline appears.

**Disposition:** stop this successor as a *bounded negative economic result with native semantic/work validation*, leave PR #482 draft/unmerged, publish Agent Intercom final verdict, and disable the existing Visibility Elision Sun automation. No final canonical reconciliation is warranted for an optimization that has not earned landing. Further performance exploration would require a **new explicit human commission**, not a silent successor rung.
