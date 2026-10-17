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
