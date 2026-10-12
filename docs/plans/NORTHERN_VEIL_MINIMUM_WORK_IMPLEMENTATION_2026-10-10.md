# Northern Veil: toward the minimum necessary computation

Zach commissioned implementation of this plan after choosing exact preservation first, certified one-level output error later, 30 FPS movement first and then 60 FPS. Parent: [Northern Veil task](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md). Continues [the shared-checkout commission](../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-implements-the-minimum-work-plan).

## Acceptance

Preserve authored equations, native resolution, channel/clock/coordinate independence, save bytes and authority. No Zone-specific rules. Exact changes retain existing RGBA bytes and sample positions. Bounded mode is opt-in: final output error at most one 8-bit level per channel. Thirty FPS requires warmed moving-route p95 ≤33.3 ms at Zach’s ordinary framebuffer size plus Person appearance/movement acceptance; 60 FPS subsequently requires ≤16.7 ms. These are targets, not promises.

## Stages and implementation status

1. **Measurement — in progress.** Freeze the rebuilt baseline including discovery repair. Separate delayed volume timestamp support is implemented, but enabling timestamps on the native window produced Person-reported flicker and all-zero timing results. The opt-in was removed and that run rejected; ordinary measurements keep timestamps disabled and separated GPU time remains unavailable. Record this deviation explicitly; distinguish CPU command preparation, total frame time, surface acquire/submit waits, upload bytes, shader compiles and proof builds. Scratch-only route joins spawn, inside, reverse and side over 60 seconds after warming both lanes. Two sequential ABBA series, matched clock progression/cameras/framebuffer, no concurrent GPU tests; discard occluded/wrong-Zone frames. Measure the observed framebuffer and 2560×1440 separately. Record startup separately.
2. **Reusable incremental compilation — retained for preparation savings; revision work remains.** Kernel-only typed expression DAG, parameter slots, evaluation domains, reverse dependencies and positional occurrence identities. Replace JSON sharing keys with collision-checked typed identity. Retain evaluator fragments and layouts directly rather than extract them from complete standalone shaders. Revision changes repair affected nodes/ancestors/sharing groups; stable numeric edits refresh slots only. Reuse fragments across fused/single/inside/outside modules and prewarm both camera lanes. Changed WGSL can still trigger whole-module GPU-driver compilation.
3. **GPU time-only preparation retained; demand-first sharing remains pending.** Eligible scalar expressions whose only input is their own clock are compiled into a separate GPU preparation step, using the original arithmetic and parameter layout. Their derived results replace repeated sample evaluation. Native exact pixels and repeated inside-view cost pass retention; see the October 11 record below. First-use/nested sample sharing remains open after the earlier one-level parity failure. Preserve cheap zero-factor exits and source/occluder/Gradient/direction independence.
4. **Resident uploads — retained: recurring uploads eliminated.** Resident GPU parameter/proof buffers owned by existing caches, coalesced dirty-range writes, original parameter/proof offsets. Camera/time/transform data remains streamed. Proof invalidation remains density revision plus half-extents; unknown proofs take existing path. Release buffers with caches/device recreation. Stable frames and time-only changes upload zero parameter/proof bytes.
5. **Full-lattice fallback validated; certification pending.** Zach selected removal of the legacy `T < 0.01` stop in opt-in mode. The positive-budget path now takes all accepted samples. Next, certify bounds on remaining emission, scattering, source radiance/chroma/angular factors, medium chroma/phase, visibility, background and numerical error across every medium before permitting tail termination. Unknown/nonfinite/unsupported bounds continue the original sampling lattice without using the legacy opacity cutoff. Initially no adaptive resampling or general analytic integration.

## Interfaces and verification

Extend existing FrameStats with repair/lowering, uploads, CPU preparation and delayed volume timing. Keep compile/collector wrappers. Register ScreenChannel `volumeOutputErrorBudget`, default 0 for legacy behavior, positive capped at 1/255 for certified mode; use a separate GPU field, preserve `volumeControl.z`. No codec/domain enum/authority changes. Kernel cache state is named as such.

Extend sharing, parameter-refresh, proof and quadrature witnesses: zero compile/lowering on unchanged frames and warmed crossings; numeric sharing stability and split/join repair; unrelated-media fragment reuse; coordinate/source-clock isolation; media reorder/removal/device recreation; density zero→positive invalidation. Compare complete RGBA at multiple times/four views. Bounded witnesses include bright emission behind opacity, thin structures, overlap, backgrounds and unknown fallback; use analytic fixtures and the 6144-sample reference alongside proofs.

Retain performance candidates only when repeated measurements establish ≥5% targeted-cost improvement and no repeatable >3% other-view total-frame regression. Record inconclusive/rejected experiments honestly. Update task, Derived State Ledger, Intercom and Person Verification after each retained stage. No saves, commits, pushes, stashes or subagents.

## Current observations

Final native captures match every RGBA byte at 2560×1440 across four views and t=0/7.25. Six focused targets pass; CPU sharing 55/55 and native proof 42/42. Per-medium fragments, typed positional DAG repair, admission prewarm and resident coalesced dirty buffers are implemented. Numeric refresh still traverses the changed medium through the existing collector and builds temporary emitter text; direct retained parameter-binding recipes remain open. Lazy/nested first-use sharing changed one output level in a native capture and was removed from production. The original eager sharing remains.

Zach reports approximately 15 FPS and sometimes 20 FPS after the usage interruption, with attribution uncertain. He rejected the timestamp-enabled frozen-baseline diagnostic for severe jaggedness and disappearing curtains. Fixed captures do not establish temporal coherence; ordinary live movement acceptance remains open. Matched readback-free ABBA route series are running with timestamps disabled. Raw native surface status is not exported by the scratch route; nonempty draws are its proxy, and no-draw/wrong-Zone runs are excluded. The observed diagnostic framebuffer is 2560×1440; Zach's usual dimensions need separate confirmation.

Zach answered the reference-contract question with **“remove the cutoff.”** The positive `volumeOutputErrorBudget` path therefore uses the full existing sampling lattice as its reference, including bright late emission. Zero retains the original compatibility image. Removing the legacy opacity cutoff can intentionally differ by more than one level from that older image; the one-level budget concerns certified shortcuts relative to the full lattice. The registered property, runtime propagation and separate GPU field are implemented; certified contribution bounds and early tail termination remain pending. With no certified bound, opt-in takes all accepted samples. Verification for this continuation is recorded below.


Zach reports rough surface-acquire waits ranging from 0.03 ms to commonly 120–160 ms and sometimes 300 ms, plus 6–40 ms CPU command recording; figures fluctuate and are Person estimates. The app is now closed. Surface acquire can reflect preceding GPU backpressure, so separate execution timestamps must establish its cause. On implementation entry, the compiler source had reverted to HEAD while its header/renderer retained the support-lane interface. The exact previously verified compiler SHA was recovered before baseline freezing.

Signed: Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T01:01:22.798427+00:00


## Retention evidence after the usage interruption

Seven focused CPU/native targets pass, including the existing native object consumer after its cold counts were updated for two camera variants. Eight full-resolution image comparisons remain byte-identical. Two readback-free ABBA series per fixed view show no repeatable >3% frame regression. Four-run mean frame costs A→B: spawn 77.85→79.71 ms, inside 221.09→222.46, reverse 192.05→191.86, side 168.84→164.77. None establishes a ≥5% frame improvement. Recurring parameter/proof uploads fall to zero; the first moving ABBA preparation mean falls ~19%. These satisfy targeted preparation/upload gates and justify retaining the exact groundwork. The moving window has high variance and its first log uses draw-status rather than raw surface status; its faster run is not evidence of a causal gain. The raw-status rerun is complete; movement economics remain inconclusive. No 30 FPS result or Person movement acceptance is claimed.

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T22:58:25.385007-07:00


## Completed raw-status baseline and candidate route

Two visible native ABBA series completed at 2560×1440, with matched renderer clock/camera route and raw status 1 on every warmed admitted frame. Compiles/lowerings/proof builds remain zero; candidate parameter/proof uploads remain zero. Series #1 mean frame 125.30→126.29 ms (+0.79% cost), preparation ~18% lower. Series #2 111.93→134.30 ms (+19.99% cost), preparation ~10% worse. Both versions exhibit an unusual acquire/submit wait-pattern shift, so causal movement gains and a uniform CPU speedup are not established. Candidate route p95 197–298 ms fails the 33.3 ms inhabitation target. Retain the exact zero-upload/compiler groundwork on the separate fixed-view gate; do not call it a verified FPS improvement. Raw-status and pixel/image witnesses remain separate. The first occluded raw attempt is excluded. The public frame reset was already correct and no redundant reset was retained. Actual usual-size and Person coherence acceptance remain open; no new certified mode exists.

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:24:19.962220-07:00


## Cutoff reference decision implemented

Zach's “remove the cutoff” settles the bounded reference as the full accepted lattice. `ScreenChannel.volumeOutputErrorBudget` is registered, defaults to zero and reaches the renderer each frame. Positive finite values are capped at 1/255; a separately named GPU field preserves `volumeControl.z` and sample-count control. Both single and fused medium marchers bypass their opacity stops in opt-in mode, including both inner/outer fused-loop exits. Tiny positive budgets retain opt-in even when their f32 budget rounds to zero. No contribution proof is claimed: every opt-in ray continues all accepted nonzero samples, with the existing independent exact zero-density acceleration.

The app builds and eight focused targets pass: property paths, parameter refresh, CPU sharing/proof, native sharing/proof, unified quadrature and object rendering. Native sharing passes 46/46, including single/overlapping bright late emission, background attenuation, analytic ramp, 6144-sample reference, tiny-positive budgets, no recompilation/upload on budget changes and returning to the identical zero-budget image. The property test's first sandboxed GLFW initialization hung; that process was stopped and desktop-enabled rerun passed in 0.58 seconds. Eight separate 2560×1440 default RGBA captures (four views, t=0 and 7.25) match the frozen baseline byte for byte. Scratch records: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/cutoff-default-results.json`.

This is fidelity/reference work, with no new performance claim. Positive mode can cost more until certified tails are implemented. Full-lattice output may intentionally differ by more than one level from the old cutoff image. The real Northern Veil save remains SHA256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. No saves, probe source, commits or pushes were changed. Certified bound construction/numerical uncertainty, direct parameter recipes, exact demand sharing/GPU time preparation and live inhabitation remain unfinished.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:49:10-07:00


## Independent high-value step retained: GPU time-only preparation

Zach asked to choose the highest likely value remaining item that did not depend on unfinished predecessors. Selected stage 3's GPU time-only preparation: it can remove repeated transcendental evaluation at native resolution without waiting for contribution bounds or changing the lattice. The compiler emits retained recipes directly, with the same scalar arithmetic/order and parameter-slot inputs. Initial eligibility is a ScalarForm with at least one transcendental factor and no variable except its own `t`; mixed spatial/directional expressions remain in their existing evaluators. This is a generic compiler decision, not a Northern Veil rule.

Prepared outputs occupy explicitly derived slots in existing resident GPU storage. A compute pass evaluates them once per participating medium per frame before volume rendering; source recipes retain `u.sourceTime.x`, medium/occluder recipes retain the medium instance clock. Camera variants share the preparation pipeline. Ordered GPU commands supply synchronization without ordinary-frame CPU waits. Numeric edits preserving layout refresh inputs without WGSL regeneration. Reload/device teardown clears preparation pipelines with the existing caches. No new authored switch, save field, codec or authority mechanism.

**Native fidelity:** all eight 2560×1440 RGBA comparisons (four views, t=0/7.25) match the frozen cutoff-compatible baseline bytes. CPU sharing and parameter refresh pass; four final native targets pass. Native sharing 65/65 now covers prepared versus independent inline arithmetic, separate clocks, time-only zero CPU uploads/compiles, numeric edits, reload and device replacement. Native proof 42/42, quadrature 15/15 and the existing native object consumer pass. App rebuilt. A test-only call to a nonexistent device teardown method was corrected to the existing device-replacement pattern before the final green run.

**Economics:** two sequential readback-free ABBA series per view, 24 timed frames per run after warmup, fixed t=7.25 and 2560×1440. No concurrent GPU tests. The table is total offscreen batch cost per frame, not a separately measured GPU execution timestamp or a windowed movement FPS claim.

| View | Baseline ms | Prepared ms | Mean cost reduction |
|---|---:|---:|---:|
| Spawn | 87.485 | 84.3625 | 3.57% |
| Inside | 219.3225 | 207.075 | 5.58% |
| Reverse | 183.6925 | 180.0025 | 2.01% |
| Side | 159.0075 | 155.46 | 2.23% |

Inside's individual series improve 4.14% and 7.01%; pooled repeated measurements exceed the 5% targeted-cost gate. Both series improve every view, so no repeatable >3% regression. Smaller other-view gains remain below the retention target. Warm runs show zero lowering, shader compilation and CPU parameter/proof uploads. Retain the time-only stage on that limited evidence. Preparation itself still runs each rendered frame, even if the clock happens to be unchanged; avoiding that tiny dispatch is subsequent work.

Records live under `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/`: `time-preparation-cost-results.json`, `time-preparation-cost-summary.json`, `time-preparation-default-results.json`, `time-preparation-provenance.json`. Baseline/candidate binaries were frozen with matching constructor layouts; benchmark GPU modules are unchanged by subsequent startup generated-byte accounting. Ordinary timestamps remain disabled. Real save SHA256 stays `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. No commits/pushes. Live temporal coherence, ordinary-framebuffer movement and 30 FPS remain unverified; certified tails, direct parameter recipes and demand-first sample sharing remain unfinished.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:15:29-07:00


## Independent high-value continuation: exact support traversal

Zach selected implementation before phase-by-phase diagnosis. Retained fused traversal computes each medium's proof support once per ray, skips certified voids inside authored boxes and reuses the leaf proof for density demand. Box events, segment spacing, sample positions and arithmetic are preserved. Unknown/near-parallel inputs fail open; both resident camera lanes carry support bounds with zero warm uploads/recompilation. Eight captures match bytes and all six focused targets pass, including native proof 54/54.

| View | Baseline mean ms | Candidate mean ms | Cost reduction | Separate series |
|---|---:|---:|---:|---|
| Spawn | 100.49 | 65.99 | 34.33% | 36.47% / 32.03% |
| Inside | 257.77 | 172.79 | 32.97% | 34.06% / 31.90% |
| Reverse | 231.98 | 158.28 | 31.77% | 32.86% / 30.68% |
| Side | 194.47 | 102.54 | 47.27% | 46.58% / 47.95% |


Final protocol is two sequential readback-free ABBA series per view at 2560×1440, 24 timed frames/run. Means include all runs; ordinary GPU timestamps remain disabled. The candidate exceeds retention gates in all views. [Full evidence, proof dependencies, artifacts and outlier disclosure](../audits/NORTHERN_VEIL_SUPPORT_TRAVERSAL_2026-10-11.md). App rebuilt; no saves/commits/pushes. Live temporal coherence, usual-size movement p95 and 30 FPS remain Person/unverified work; exact noise-cell reuse, direct parameter recipes, demand-first sharing and certified tails remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T02:25:43-07:00


## Perlin reuse retention decision — October 11

Six generic candidates were implemented under Zach's next-step commission. All three exact ray caches were slower in repeated paired measurements; all three immutable GPU lookup variants changed the inside image despite exact native compute comparisons. No candidate is retained. The previously retained compiler/residency/time/support implementation is rebuilt, seven focused checks pass in Perlin ABC mode and eight restored captures match. The original full Perlin terrain-upload gate has a reproduced baseline failure and is not included in that green selector result. [Complete evidence and future-agent cautions](../audits/NORTHERN_VEIL_PERLIN_REUSE_EXPERIMENTS_2026-10-11.md). Exact demand-aware sharing, direct parameter recipes, certified tails and live movement acceptance remain open; these rejected forms of Perlin reuse are no longer an untested presumed win.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T10:29:00-07:00


## Selective folded-support index retained — October 11

Zach commissioned the next structure-specific step. Generic column envelopes reuse every unknown leaf of the existing density proof; outside ray eligibility is computed once, then reached empty leaves can jump farther through certified void. Original inside traversal, contributing samples, equations and lattice remain unchanged. Proof/index residency uses existing revision/extents/camera/membership/device lifetime gates; no warm upload or equation compilation.

Eight full-resolution captures match; all six focused targets pass, including native proof 65/65. Final two ABBA/view plus two predeclared inside follow-ups retain the stage: spawn 61.35→55.36 ms (−9.77%), reverse −2.47%, side −0.27%; inside **124.81→127.67 ms (+2.29%)**, with series +7.76/+1.02/+0.57/−0.13%, no repeatable >3% regression and no inside speedup claim. All initial runs remain included. Direct-everywhere and interval-cache variants are rejected; per-leaf cost selection remains below retention. [Full implementation/evidence](../audits/NORTHERN_VEIL_FOLDED_SUPPORT_2026-10-11.md).

Ordinary app rebuilt; no save/codec/domain/authority changes, commits or pushes. Live movement, usual framebuffer 30 FPS acceptance, tighter mathematical support contraction, demand-aware sharing, direct parameter recipes and certified tails remain open. This index improves traversal of existing proofs; it does not prove new unknown cells empty.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T13:27:06-07:00


## Astra continuation: sample-local scheduling retained — October 11

Zach commissioned counsel, implementation and a report back. The first-ranked projected support guard remains deferred pending a complete backend numerical enclosure; restricting Astra’s diagnostic model to principal trig domains lowers its opportunity to 21.86% of folds, still no GPU certificate. The second-ranked fused schedule is implemented: one medium proof/contribution sweep, original direction arithmetic after positive density, no cross-sample cache. All six focused targets and eight exact 2560×1440 images pass. Two ABBA series/view retain inside −9.77%, spawn −11.53%, reverse −6.44%, side −8.03%; zero warm uploads/lowering/compiles. [Complete mechanism, all run means and unsupported claims](../audits/NORTHERN_VEIL_SAMPLE_LOCAL_SCHEDULING_2026-10-11.md). Live usual-framebuffer movement and 30 FPS remain unverified. No saves, commits or pushes.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T17:22:37-07:00 (PDT).


## Next testing pass — diagnostic retained, transport candidate rejected

Implemented a reproducible read-only GPU fragment census, with paired pixels and counter-accounting checks. Its counters repeat, but instrumentation changes RGBA by at most one level; treat them as diagnostic rather than exact original-fragment evidence. Tested an independent sample-local empty-transport skip: eight exact full-size captures pass, but inside ABBA series are +8.02%/+1.04% slower, so it is rejected and production scheduling stays unchanged. No new support guard, cutoff, cache or save edit. [Full evidence and reproducible tool](../audits/NORTHERN_VEIL_FRAGMENT_DEMAND_2026-10-11.md). Actual live-view context, sound numerical support certificates and the movement milestone remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T20:53:25-07:00 (PDT).


## Backend certificate research — October 12, runtime candidate deferred

The rational scalar-support model and two-endpoint band pass 18 named witness groups. Only 157/1,024 fixture bins meet the model's intrinsic-domain restriction; its conditional grid coverage is not demanded-ray coverage. Complete production transformations/coordinate arithmetic are not enclosed, so production-admitted opportunity is zero and no runtime guard is built or timed. AD-30–32 remain OPEN; this is scalar research, not certified tail termination or new whole-contribution skipping. [Derivation, refusals, artifacts and next boundary](../audits/VOLUME_SUPPORT_NUMERICAL_CONTRACT_2026-10-12.md). Retained scheduling remains the baseline.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T00:43:07-07:00 (PDT).


## Actual Metal profile observed — October 12 follow-up

The native option profile is now known: retained volume MSL uses Fast arithmetic and Fast functions. Complete execution enclosure under that profile remains unresolved. Implemented a scratch-only source/options observer and conditional local-coordinate/refusal witnesses; 24 groups pass and eight observed/plain 320×180 images match. These do not admit the guard or establish native-size correctness/performance. No new production behavior or FPS gain. [Reproduction, assumptions and next boundary](../audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T11:27:35-07:00 (PDT).
