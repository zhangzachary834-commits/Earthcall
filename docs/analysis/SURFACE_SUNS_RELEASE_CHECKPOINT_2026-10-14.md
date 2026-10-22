# Surface Rendering Suns — Release Checkpoint and Promotion Gate

**Prepared:** 2026-10-14. **Origin:** Zach explicitly requested a checkpoint for **Visibility Elision Sun / SourceRho / Execution Key Sun**, not the separately commissioned Northern Veil volumetric optimizations. This is a surface-source / shadow-transport engineering checkpoint, **not** a claim of a new FPS gain.

**Canonical checkpoint parent:** `3287e346a9c83a50f560b707082f8bee9a1141c3` on the actual default `sync-from-earthcall-main`, not the obsolete `main` branch.  
**Checkpoint branch:** `sol/surface-suns-release-checkpoint-20261014`, branched directly from that canonical commit.  
**Authority boundaries:** `rho_source != V_transport != D_medium`; ambient, receiver response, V1–V4 independent; authored source identity and lifetime never inferred from the numeric slot alone. Never reduce framebuffer resolution or authored fidelity to manufacture an FPS gain.

## What is already landed

**Execution Key Sun: [PR #369](https://github.com/zhangzachary834-commits/Earthcall/pull/369), merged September 28.** Its ordered admitted radiance/medium binding is an already-known execution key. An aligned artifact has fixed channel/generation/producer/revision checks; unknown/stale/replaced/removed/reordered or cross-channel state fails open. Local dirty-slot repair replaces a global relevance search. Its native A/B reported 200,000 proof reads, 800,000 fixed metadata checks, zero spatial/semantic record searches, 70 logical resident bytes, and **28.912143x** on a bounded *exact decision-vs-aligned-decision microbenchmark* (exact 430,651,167 ns versus aligned 14,895,166 ns). **The shipped mechanism grants zero production pixel authority**, evidenced by `production_authority_bypasses=0`. Thus 28.91x must NEVER be described as whole-renderer speedup or FPS.

Canonical evidence: [Execution Key analysis](already_known_execution_key_consumer_after_pr350_2026-09-28.md), `RenderedFieldSemanticObserver.hpp`, `rendered_field_piecewise_synthesis_test`, `rendered_field_direct_dispatch_test`. Production still uses exact authored SourceRho and independent medium/light transport semantics. This checkpoint **does not re-merge PR #369**.

## What has NOT earned an FPS promotion

| Experiment | State | Correctness / work | Renderer economics | Shipping disposition |
| --- | --- | --- | --- | --- |
| **PR #369** Execution Key aligned proof | **Already merged** | Native bounded direct-dispatch, hostile lifetime and independent channels PASS | **28.91x dispatch micro-path**, not full-frame timing; pixel bypass 0 | Keep existing structural infrastructure on default |
| **[PR #445](https://github.com/zhangzachary834-commits/Earthcall/pull/445)** exact literal-zero SourceRho body bypass | Closed, **not merged** | Native WebGPU/hostile-provenance parity PASS | 12 warmed pairs: 9.779542 ms exact, 9.681041 ms authority (~1%); zero valid GPU timestamps | No production authority promotion |
| **[PR #482](https://github.com/zhangzachary834-commits/Earthcall/pull/482)** direct `sourceVisibility` elision for proved-zero SourceRho | Open **draft**, **not merged** | Two native Metal exact/authority and diagnostic pixel witnesses PASS; 290,151 calls and 5,624,857 **executed shadow-loop** steps removed from zero source, with independent live source unchanged | See two reproducibility records below: directionally positive but dramatically variable wall effect; zero valid GPU timestamps | Research candidate only; do not squash/cherry-pick whole stale experiment onto current default |

### Visibility-elision native results, compared without cherry-picking

Both completed on macOS GitHub Actions [run #37973693652](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37973693652), at unmodified **render/test code** `fde0d5052350d9611ecf10f72cbcbf37e43266f9`. Later PR #482 commits are documents, not rendering changes. Each experiment uses **12 uninstrumented timing pairs**, with a separately executed opt-in GPU-atomic count draw afterward. Counter work is therefore excluded from the timed pairs.

| Native run artifact | Exact wall median | Authority wall median | Exact / authority | Approximate frame-time reduction |
| --- | ---: | ---: | ---: | ---: |
| Original #11641566316 (October 9) | 9.718958 ms | 9.682459 ms | 1.003770x | **0.38%** |
| Re-run #11725467491 (October 11/12) | 11.123417 ms | 9.412584 ms | 1.181760x | **15.38%** |

**Both recorded the identical GPU execution witness:** proven-zero source: exact **290,151 calls / 5,624,857 shadow steps**, authority **0 / 0**. Independent live source: **290,151 calls / 6,778,135 shadow steps in both arms**. Total counted shadow steps reduced **45.35%**. Both macOS `Verify SourceRho zero-authority native pixel A/B` steps succeeded, including visibility ON/OFF occlusion-boundary distinction, byte-exact framebuffer parity, diagnostic/ordinary parity and hostile producer-lifetime repair/fail-open. Both workflow attempts ended red in the later unrelated/shared CPU gate. The rerun's tiny native log is recoverable as artifact #11725467491 and the original verbatim values are preserved in [PR #482 final analysis](https://github.com/zhangzachary834-commits/Earthcall/blob/sol/sourcerho-zero-visibility-elision-20260929/docs/analysis/source_rho_zero_visibility_elision_native_economics_verdict_2026-10-10.md).

**Interpretation:** real structural savings and scene-local exactness are well evidenced. A 0.38%-vs-15.38% measured wall effect with no GPU duration samples is not a stable production FPS estimate. Cold startup deltas and anecdotal wall minima are not proof of a repeatable shader-performance win. Do not revise the prior negative economic verdict solely from the favorable rerun, nor ignore that rerun. Further promotion requires a predeclared multi-run, multi-view and device workload gate.

## What THIS checkpoint PR changes

It **moves, but does not duplicate**, the already-built canonical `rendered_field_piecewise_synthesis_test` and `rendered_field_direct_dispatch_test` into a named **`Verify surface execution-key direct-dispatch contract`** CI step immediately after the SDF witness build and before the shared `Run CPU SDF proof witnesses` step. The preexisting 7-day `aligned-authority-ab` artifact upload remains unchanged. Thus unrelated failing CPU SDF work cannot suppress the evidence that Execution Key's direct dispatch still passes on current canonical; if the structural test itself fails, that named step goes red. This is CI/evidence packaging only: **no shader, renderer, authored math, authority bit, save, or runtime behavior is changed**.

The checkpoint does **not** port #445 or #482 experimental renderer code, enable pixel authority, alter soundness/canonical shadow step, introduce new theorem matching or touch Northern Veil volumetric work.

## Release gate — separate three distinct claims

1. **Structure already shipped:** keep PR #369's direct-slot proof infrastructure as the baseline. Require the standalone `Verify surface execution-key direct-dispatch contract` named step and `aligned-authority-ab` artifact on exact checkpoint HEAD; verify production pixel bypasses remain zero. A microbenchmark PASS means the dispatch regime works, **not** that FPS improved.
2. **Integration / renderer exactness:** require the current canonical's `sdf_wgsl_parameter_refresh_test`, `webgpu_object_test`, relevant SDF/WebGPU native byte parity and hostile lifecycle checks. If the unrelated Focused CPU and Slow Adapter jobs are red, classify them by failing step; never call whole CI green or suppress failures. Production SourceRho stays exact by default.
3. **Future FPS promotion:** if a new **explicit commission** tries to ship #482, construct a *separate minimal canonical-ancestry candidate* with only literal-zero direct-transport visibility elision behind a default-OFF toggle; reconcile exact semantic dependencies deliberately (no bulk replay of 77 stale commits). Freeze test scenes, viewport, warmed frame protocol, ABBA order, independent devices/driver data and pass/fail thresholds **before** measuring. Demand repeatability and no material regressions on other views; include real native Metal ON/OFF occlusion, GPU-executed counters, byte-exact pixels, ambient/response/V1–V4, producer generation/revision/channel fail-open, reload/device repair, and measured full-frame savings. GPU timestamp unavailability must be explicit. **Only then** authorize default-ON behavior and merge.

**Already-known risk:** the surface experiment PR #482 diverges from canonical by hundreds of commits. Canonical's `SdfWgsl.cpp` and `WebGpuRenderer.cpp` now contain separately retained volumetric changes. Blanket file replacement, stale whole-branch merge, force-push or reset would erase newer authored work. The checkpoint starts on live default, so there is **no canonical reconciliation required for this checkpoint**. Future semantic overlap is assessed for an actual change, never merely because default advanced.

## Current release disposition (not a completed FPS launch)

At checkpoint creation, current default was `3287e346a9c83a50f560b707082f8bee9a1141c3`. Existing focused CI run [#38497138723](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/38497138723) had SDF range-proxy/macOS and authored-Perlin/macOS **success**, Focused CPU and Slow Adapter **failure**. Those red jobs are not presumed inherited without same-base evidence. The checkpoint PR's own exact-head tests must be read after GitHub finishes them.

**Ready to ship now:** existing merged Execution Key structural mechanism, plus a separate CI evidence-checkpoint improvement once its exact-head named step is green.  
**Not ready to ship as FPS:** SourceRho body or visibility-elision production bypasses.  
**Default merge policy:** only merge this CI/documentation checkpoint if relevant exact-head checks pass and default has not moved without review; no production FPS claim or premature SourceRho landing. Preserve this checkpoint as a clean base before new surface Sun iterations.
