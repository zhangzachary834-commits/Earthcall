# SourceRho measurement successor — first exact-head native paired checkpoint

**Date:** 2026-10-17 PDT  
**Scope:** Measurement-only successor to completed Visibility Elision Sun / SourceRho Economics Sun; historical draft PR #482 remains unmerged.  
**Measurement code tested:** `82c910793a047d772eaf4089704909d6e4d75da8`, branch `sol/sourcerho-measurement-20261016`.  
**Native evidence:** [GitHub Actions run #39010536979](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39010536979), job `SDF range-proxy verification (macOS)`, step `Verify SourceRho zero-authority native pixel A/B` **SUCCESS**; tiny [artifact #12093405630](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39010536979/artifacts/12093405630), `webgpu-source-rho-authority-perf.log` (5,027 bytes extracted). The full workflow failed later in the shared CPU-SDF witness step; this is not an observed SourceRho witness failure.  
**Runner evidence:** `RUNNER_OS=macOS`, `RUNNER_ARCH=ARM64`, `GITHUB_RUN_ATTEMPT=1`; workflow specifies `macos-14`. GPU adapter name, actual backend selection, thermal/power state, and valid GPU execution timestamps were **not recorded**. Do not claim a device identity or a GPU-duration measurement.

## Native correctness and actual work

The exact-head native SourceRho binary passed its whole dedicated step. Its existing gates cover exact/authority byte-exact pixels, visibility ON/OFF physical-occlusion falsifier, diagnostic/ordinary parity, unchanged independent live source, and hostile producer/lifetime/revision/generation/repair/fail-open/recovery scenarios for this fixture. This is bounded evidence, not proof for arbitrary worlds. No production renderer semantics were changed.

GPU-executed counters, captured **after** the 12 uninstrumented timing pairs:

| Source | Exact calls / shadow SDF steps | Authority calls / shadow SDF steps |
| --- | ---: | ---: |
| Proven-zero slot | 290,151 / **5,624,857** | 0 / **0** |
| Independent live slot | 290,151 / **6,778,135** | 290,151 / **6,778,135** |

The existing ontology boundary is unchanged: `rho_source != V_transport != D_medium`. No whole-source, ambient, receiver, authored-response, V1–V4, or density authority was introduced.

## First measurement-only paired run

The 12 exact/authority pairs alternated which arm ran first; raw per-pair wall, CPU-submit, device-poll, and difference values are preserved in the native artifact. All numbers below are milliseconds unless specified.

| Metric | Exact | Authority |
| --- | ---: | ---: |
| Median complete-frame wall | **13.420896** | **9.586021** |
| Median CPU recording/submission | 0.393813 | 0.379000 |
| Median device-poll/wait | 12.972646 | 9.315251 |
| Warmup wall median (six per arm) | 9.879396 | 9.997104 |
| Valid GPU timing samples | **0/12** | **0/12** |
| Recurring SDF program compiles | 0 | 0 |
| SDF program cache hits | 12 | 12 |
| Recurring parameter uploads (bytes) | 0 | 0 |
| Cold wall, descriptive only | 444.822334 | 214.414292 |
| Cold WGSL bytes | 46,957 | 46,875 |
| Cold parameter uploads (bytes) | 40 | 36 |

Ratio of independent arm wall medians: **1.400049x**; authority median is **28.57% below** exact median in this run. Do **not** confuse the difference of arm medians (3.834875 ms) with the **median of paired differences** (**2.530042 ms**). Paired difference is `exact - authority`, positive favoring authority. Paired median absolute deviation is **2.699041 ms**, larger than the median difference. **10 of 12** paired differences are positive, **2 negative**; range **-5.214209 to +8.994375 ms**. Exact-first median paired delta **2.906938 ms** (6 pairs); authority-first **2.329459 ms** (6 pairs). Paired CPU-submit median difference **-0.039104 ms** and paired device-poll median difference **+2.350166 ms**; this suggests the observed host-wall advantage is concentrated in the poll/wait segment, but **poll/wait is not a GPU timestamp**.

For a descriptive uncertainty check only, a two-sided exact sign test for 10/12 positives yields `p≈0.0386` under independent symmetric/no-effect sign assumptions; the corresponding conservative order-statistic 96.1%-coverage interval for the paired population median is approximately **[+0.211334, +6.094833] ms** under independent continuous observations. These assumptions are **not established** for alternating draws on a single CI runner; temporal dependence, scheduling, warmup/residency, and post-hoc selection may invalidate inferential confidence. The large dispersion and tiny sample demand independent repeat runs.

## Relation to the previous native evidence

The completed predecessor recorded the same **5,624,857** shadow steps eliminated but reported approximately **0.38%** and **15.38%** wall-frame improvements in two native runs. The new exact-head measurement reports **28.57%** by arm medians and **+2.530042 ms** by paired median. These three runs are **directionally compatible** with a real benefit but differ substantially in magnitude; the new run does not settle reproducibility or attribution. All runs lacked usable GPU timestamps. Zero-printed GPU medians mean **unavailable**, never zero GPU cost.

## Classification, limits, and next bounded action

- **PASS:** exact-head macOS ARM64 native SourceRho step, framebuffer/lifetime gates, deterministic GPU shadow-work counters, new pair/host breakdown reporting. Native run #39010536979 is real, not inferred from absent PR-triggered workflow listings.
- **UNCLASSIFIED/SEPARATE:** subsequent shared CPU-SDF step failed; `Upload aligned execution-key authority A/B` also failed. Focused CPU and Slow Adapter lanes failed in their respective steps. No evidence assigns these failures to the measurement-only change.
- **NOT YET PROVEN:** repeatable economic gain across runners/conditions; true GPU duration; adapter model/backend; per-stage pipeline compilation/residency; safe integration into canonical (canonical was `d9cd72aa3db4` while PR was `607c82b3f32f` at intake).
- **No shipping recommendation yet.** Preserve PR #482 draft/unmerged, canonical untouched, measurement branch separate. No production semantics or test correctness gates changed.

Next bounded measurement: repeat the **same exact benchmark code** on a verified macOS WebGPU/Metal runner, preserve raw 12-pair records and runner/device details, and compare paired distributions across runs without selecting only favorable outcomes. Where supported, add *measurement-only* adapter/backend/timestamp-capability reporting after verifying API compatibility; do not conflate queue-poll duration with GPU execution. Do not merge or create a PR without explicit authorization.

---

## Second independent native measurement — same benchmark code, economic non-replication

**Date:** 2026-10-17 PDT. **Run:** [#39021012172](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39021012172), native macOS SDF range-proxy job `117362656193`, SourceRho native A/B step **SUCCESS**. **Artifact:** `webgpu-object-parity-log` #12096522644, extracted `webgpu-source-rho-authority-perf.log` (5,016 bytes). **Workflow head:** `2b355383e629241b4ec43b7c8e6c91aa3889a4f9`; benchmark blob **identical** to tested measurement-code commit `82c910793a047d772eaf4089704909d6e4d75da8` (blob `9b08e6c44717d00f96ccf0f8967547a43a918705`). The only intervening measurement-branch change was this analysis document.

**Exactness:** dedicated native SourceRho step passed again, including the existing framebuffer, diagnostic, occlusion, lifetime/repair, and fail-open gates. GPU-executed shadow-loop counters reproduced **5,624,857** eliminated steps and **290,151** eliminated calls for proven-zero slot; independent live slot remained **6,778,135** steps / **290,151** calls in both arms. Recurring shader compiles 0/0, cache hits 12/12, recurring parameter uploads 0/0. Native runner reported `macOS` / `ARM64` / attempt 1. **Valid GPU timestamps 0/12 in each arm: GPU duration unavailable, not zero.**

| Metric (ms unless noted) | Exact | Authority |
| --- | ---: | ---: |
| Median completed-frame wall | 9.470521 | 9.440313 |
| Median CPU record/submit | 0.282855 | 0.279813 |
| Median device-poll/wait | 9.196125 | 9.163105 |
| Warmup median | 9.547105 | 9.489313 |
| Cold wall (descriptive) | 274.533458 | 192.677292 |

Independent arm median wall ratio `1.003200x`: approximately **0.32%** lower authority wall median. The **median of paired differences** (`exact - authority`) is **-0.002375 ms** (effectively zero; slightly favors exact), with paired MAD **0.911979 ms**. Exactly **6/12** pairs favored authority and **6/12** favored exact; range **[-3.410124, +1.935833] ms**. Exact-first median paired delta **+0.034167 ms**; authority-first **-0.040187 ms**. A few large, oppositely signed outliers dominate the dispersion. The host-side device-poll median difference is only ~0.033 ms, consistent with no detectable advantage in this particular run.

**Crucial replication finding:** two independent macOS ARM64 native runs using the **same benchmark source blob** reported materially different results:

| Native run | Independent arm median reduction | Paired median delta | Paired MAD | Positive pairs |
| --- | ---: | ---: | ---: | ---: |
| #39010536979 (commit `82c91079`) | **28.57%** | **+2.530042 ms** | **2.699041 ms** | 10/12 |
| #39021012172 (commit `2b355383`, identical benchmark blob) | **0.32%** | **-0.002375 ms** | **0.911979 ms** | 6/12 |

The second run directly **fails to reproduce** the first run's large apparent economic benefit while reproducing the exact GPU work eliminated. This is evidence of substantial completed-frame wall-time sensitivity to runner scheduling, synchronization/polling, thermal/power/residency or other unmeasured conditions; **which mechanism dominates remains unproven**. GPU adapter model/backend and valid GPU execution timestamps are absent. Do not assert that the GPU saved zero time, nor that the first run's 28.57% is representative. In particular, a large eliminated instruction count does not by itself guarantee end-to-end frame improvement.

**Bounded economic disposition:** correctness and actual work-elision remain supported, but **a reliable material economic gain has not been demonstrated and the large gain has failed a same-code independent replication**. Retain the predecessor's **economic rejection / do not ship or merge** disposition. This does **not** prove the GPU optimization intrinsically has zero benefit. It means the present measurement cannot earn promotion.

The SourceRho native step passed in run #39021012172; the later shared CPU-SDF proof step failed, as did the unrelated Focused CPU and Slow Adapter lanes. Their causes are not assigned to SourceRho. A targeted SDF job rerun was attempted for further replication but blocked by the repository tool safety gate; no rerun was launched and no bypass attempted.
