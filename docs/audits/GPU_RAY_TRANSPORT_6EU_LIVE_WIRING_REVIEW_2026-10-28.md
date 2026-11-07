# §6eu — corrective review of the live default-on renderer

**Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T16:51:42.230711-07:00.**

Zach asked us to check Claude's continuation and Firmament's new counsel after the Sixth-One Sun was interrupted. This replies to the [existing handoff thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun--6eu-corrective-live-wiring-review-native-lane-claimed) and [Firmament's two runtime catches](FIRMAMENT_REVIEW_OF_6ET_AND_LIVE_DEFAULT_ON_WIRING_TWO_CRITICAL_CATCHES_2026-10-28.md). Counsel bytes are preserved in the archive independently of that concurrent untracked document. Zach directly authorized default ON on `claude/ray-domain-transport-rung0`, with Current fallback, and requested faster default Person movement. Fidelity to authored equations remains the criterion; a visually pleasing before image is not mathematical acceptance.

**Verdict:** Claude Opus4.8 correctly preserved and compile-verified our interrupted implementation in `01b5ab9f7`, and explicitly did not claim GPU execution. The prototype had real runtime bugs, authored by this Codex session. These are repaired. The actual rebuilt app renderer now dispatches production-evaluator transport and publishes nonempty finite results, default ON on the feature branch. Sixteen fresh native offscreen captures, 33 support-recipe checks, six exact fallback/unsupported comparisons and a bounded reference pilot pass. A full-resolution connected frame screen is cheaper in four paired blocks. This is an experimental connected checkpoint, not full mathematical admission, installed-app deployment or live FPS acceptance.

## Findings and corrections

1. **Admission never matched the authored fields.** On the clean `01b5ab9f7` source, lane ON and OFF both dispatched zero routed sets in all five frames. All four decoded density recipes returned false; images matched Current exactly. The matcher required a particular multiplication grouping absent from the authored tree. It now inspects bounded multiplicative ancestors and identifies the four support factors independently of parentheses, without rewriting the field or changing its evaluation order. Addition outside the product, computed divisors, duplicate/ambiguous factors, guarded pieces and missing nodes still refuse. The actual four authored fields plus regrouping and refusal controls pass **33 checks**. OntoMath owns this mathematical recipe; Screen remains the execution substrate. No Zone name or id is an eligibility key.
2. **Firmament catch: shared point expressions were unprepared.** The production compiler stores shared density/extinction/emission expressions in private shader variables. The generated consumer now calls `volumeShared_i_Eval(p)` when present, before reading those channels. Missing preparation would otherwise produce zero-density answers after admission succeeds.
3. **Firmament catch: shader binding usages disagreed.** Classification reads storage specs; the three retained producers declare uniform specs. We bind separate storage and uniform allocations of the same current recipe bytes, preserving the qualified WGSL address spaces rather than changing the producer shaders. This is a correctness repair, not a measured claim that either address space is faster.
4. **Additional zero-source boundary.** Absence of an incident source does not prove zero illumination: Current initializes compatibility incident light to one. Eligibility now requires an actual source with literal-zero authored radiance. The absent-source native ON/OFF control uses Current and matches exactly. In this particular fixture Current also refuses because Phi reads `wi` without a source; this control does not demonstrate a successfully lit compatibility scene. A non-`wi` compatibility scene remains a useful additional witness.
5. **Inspectable evidence.** A Kernel-only diagnostic exposes publication readback and a refuse-only work ceiling (normal1024, forced0). It does not change authored state or add a mathematical policy. The reported Kernel buffer-size sum is corrected by12 bytes. Dispatch counters remain dispatch counters; they do not count accepted answers.

## Native evidence on the final repaired source

The fresh external executable links the actual app's rebuilt `WebGpuRenderer.cpp.o` and existing CMake dependencies/flags. It changes only the diagnostic main and uses the frozen authored zone fixture; no save is written. The archive binds the original `HEAD:src` tree, exact binary diff, twelve relevant source hashes, generated main, compile/link arguments, renderer object/binary hashes, fixture hash, native logs, receipts and raw outputs. Sources and binary identity are checked before/after execution. The base is `01b5ab9f7`; final source changes are explicitly a working-tree patch, not a pretend clean historical tree. `earthcall_webgpu` itself builds and links successfully. The build is **Debug**; no copied `build_release` cache is used.

| Case | Actual publication result |
|---|---|
|334×190, four authored media, inside camera, clock7.25|63,460 accepted, 63,454 nonempty, zero nonfinite components |
|2560×1440, same camera and clock|3,686,398 accepted, 2 refused, 3,686,014 nonempty, zero nonfinite components |
|Half-screen opaque foreground|31,730 left-half publications exactly zero and corresponding Screen pixels exactly match Current; remaining half renders |
|Outside/spawn camera|63,460 accepted, 28,358 nonempty, zero nonfinite components |
|Work ceiling0, inside/small/full/outside/opaque cases|Whole-ray refusal; Screen is bit-identical to Current |
|One-member unsupported set; absent-source set|Zero routed dispatches; ON/OFF images exactly match Current (absent-source also retains Current's own Phi refusal) |

Six exact comparisons cover **16,014,800 RGBA8 bytes**, including the full14,745,600-byte native viewport. Both naturally refused full-frame pixels (linear indices894498 and1969317) independently match Current. No validation/compiler/device-loss error occurs in these final captures. A capture executes five frames, not merely the CLI's requested three: the base fixture includes two capture frames.

Publication is **full-frame**, with per-strip scratch reuse. At2560×1440 the full publication is58,982,400 bytes;30 strips of48rows retain64single/16overlap intervals per ray. Kernel buffers sum to222,781,496 bytes (~212.46MiB). This excludes other renderer/allocator-pool resources and is neither physical memory traffic nor total app VRAM. Descriptor/device limits remain eligibility boundaries. Unsupported programs/layouts/budgets retain the legacy shader.

## Authored-reference pilot — limited but independent

The final live float publication is compared to §6es's unchanged **80-pixel common ideal-camera continuum target**, at clock7.25, camera `(0,50,75)→(40,62,110)` and334×190. Inputs, original reference execution receipt and both2^16/2^15 integral results are copied with exact hashes. Reusing that explicitly defined mathematical target is legitimate; it does not transplant references for old f32 directions onto new ray inputs. The frozen fixture identity matches. These are80 selected pixels, not all63,460 or3,686,400 pixels.

At2^16, live routed RGB mean/max error is `1.75904e-5 / 9.44875e-5`, versus **historical** Current float `1.31904e-4 / 1.78926e-3`; T mean/max is `2.44118e-5 / 1.37533e-4`, versus historical `2.01720e-4 / 2.76285e-3`. All six mean/p99/max metrics improve at both reference resolutions. P99 uses the retained pilot's rank convention (with80 samples, p99=max). The float Current control is explicitly historical, not a fresh float capture.

The **fresh** Current and routed native RGBA8 images also compare against that same target: RGB mean/max `0.00153185 / 0.00304874` OFF → `0.00148874 / 0.00195973` ON. Screen quantization limits this comparison; black-background RGBA8 cannot recover T. These results do not close whole-population or multi-view accuracy, certify every individual pixel improves, or inherit §6et's1,094-ray gate for this new live evaluator/setup.

## Connected full-resolution frame screen

Actual app renderer,2560×1440, same frozen inside camera/time, four warmup frames, then12 frames and final queue completion per sample;16 samples in four OFF/ON/ON/OFF blocks. Includes CPU projection, current parameter/preparation handling, encoding, strip dispatch, legacy fallback, volume Screen composition, submission and completion. No image readback or ordinary GPU timestamps occur in the timed interval. Per-frame diagnostic log writes are present in both modes. Excludes initial compile/allocation, full inhabited scene/surfaces/window compositor, display pacing and moving clocks/camera. The same Debug engine objects are used throughout.

| Paired block | Current ms/frame | Routed ms/frame | ON/OFF ratio | Reduction |
|---|---|---|---|---|
|1|77.538042|72.406847|0.933824|6.62%|
|2|80.053281|73.050905|0.912529|8.75%|
|3|79.246307|72.191899|0.910981|8.90%|
|4|82.329951|74.973630|0.910648|8.94%|

Median paired ratio is **0.911755** (~8.82% lower). All four blocks favor ON; individual baseline blocks drift77.54→82.33ms, so do not present the median as a stable live-FPS promise or a causal occupancy result. An earlier pre-final-restriction screen also favored ON in4/4 blocks (median~7.30% lower); it is a development observation, not the source-bound final table. §6et's15.49–16.64% resident-only gain is a different scope. The real bridge now pays ray setup, strip dispatch and publication, so those numbers must not be conflated.

## Claude/Firmament coordination corrections

Claude's “compiling is not working” qualification was right. His Person-list wording that `volumeRoutedSets>0` proves produced pixels is amended: it proves dispatch, while this review's actual GPU readback proves nonempty answers in the stated captures. “Per-strip publication” should read full-frame publication with strip-local scratch. Firmament's missing-shared-eval and binding catches are accepted and credited. His proposed CLI flags do not exist: use the actual registered `volumeRoutedTransportEnabled` property / renderer API. The pre-fix constructor default was0.1units/s, not1.2. The claimed §6et empty/S1/S2 tile census was not measured by that pass, and its classifier has no separate empty-skip route; do not use those numbers as causal evidence. Some copied error/bulk timing values also differ from §6et's machine record; that record remains authoritative. Firmament's publication-footprint estimate24.3MB is corrected above to58.98MB.

## Remaining bridge to a shipped optimization

- **Implemented and exercised:** real production-field consumer → native full-frame publication → Screen; defaultON on the authorized feature branch; bounded unsupported/natural/forced Current behavior; real default Camera constructor12units/s.
- **Numerical admission OPEN:** independent reference coverage of the live full population and additional cameras/depths/clocks; generic support/transport error certification remains a separate larger campaign.
- **Execution validity OPEN:** same-process toggle, clock independence, expression/revision and extent edits, resize and device reconstruction need targeted native transition witnesses. This pass's subprocess-per-case captures do not claim these transitions.
- **Inhabited economics and Person acceptance OPEN:** release-build moving-scene full-frame/pacing evidence plus Zach's unchanged-appearance, movement-feel and agency checks. No30FPS claim or merged/default-branch promotion.

The next bounded pass should close the transition matrix and gather independent actual-consumer references for several fixed views, then measure the inhabited scene with the same fidelity settings. Preserve Current fallback; do not trade resolution, lighting, authored fields, tolerances or sample caps for a claimed win. Only after these gates should we prepare a merge-quality checkpoint. Zach's experimental defaultON authorization remains in force meanwhile.

## Replay and launch

```sh
python3 -B scratch/probes/ray_transport_6eu_replay.py docs/audits/GPU_RAY_TRANSPORT_6EU_LIVE_WIRING_REVIEW_2026-10-28.zip
cmake --build build --target earthcall_webgpu -j8
./build/earthcall_webgpu
```

Run from `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`. Stored replay is CPU validation of retained evidence, not fresh GPU execution. Native witness generator: `scratch/probes/ray_transport_live_wiring_review.py` (`build` in a fresh external directory, then `capture --mode on|off|forced`, optional `--depth`, `--no-source`, viewport/camera/membership).

[Machine replay result](GPU_RAY_TRANSPORT_6EU_LIVE_WIRING_REVIEW_2026-10-28.json) · [Lossless native evidence](GPU_RAY_TRANSPORT_6EU_LIVE_WIRING_REVIEW_2026-10-28.zip) (55,742,593 bytes; SHA256 `37be190a4e2a45ee6dbd237f8c2c1cdf028bc4b5a150a328a145c1440e85d5b3`). The101 payload entries are hashed and the102-entry inventory includes the manifest; the archive hash also covers that manifest. No installed app, live save, default-branch checkout, body consent, lighting or resolution was changed by this correction. Changes are local; no push or PR. Native lane RELEASED after the final16captures/16timing samples. Person verification stays pending.
