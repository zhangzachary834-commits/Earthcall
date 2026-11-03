# §6fa — recovered native Metal trace: the schedule consumes the integrator's headroom

Zach answered **“Yes—I’m ready at the Mac”** to the foreground trace request after [§6ez's review and preparation](GPU_RAY_TRANSPORT_6EZ_OPUS_REVIEW_AND_TRACE_PREPARATION_2026-10-29.md). This continues [the existing Opus transport Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md), including Opus's hypothesis that GPU savings were hidden by host overhead. Human criterion remains authored fidelity and substantial complete native benefit, with feature defaultON and Current fallback.

**Result:** 256 measured foreground scripted-path frames completed in the real Release Engine, plus 128 warmup and eight mode-activation frames. The recovered trace binds all 264 post-start native presents. It reveals a strong structural lead: routed frames add **480 native Metal command buffers**. Source-order attribution places **43.22 ms** in classification and schedule producers against **17.31 ms** in transport consumption. Total observed app GPU active-interval union is effectively tied at **63.64 / 63.69 ms**, Current/routed. This trace does **not** confirm the proposed 5.5 ms GPU saving.

These are diagnostic observations from an instrumented run and an explicitly inferred stage mapping. They are not clean economics, shader profiler counters, full mathematical admission or Person acceptance. Production source, mathematical operations, fallback, resolution, interval limits, saves and installed app are unchanged. GPU lane is RELEASED; all windows have closed.

## What actually executed, including failures

Feature checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`, starting HEAD `4499005d3`. Same recorded 238 Release app objects with the independent diagnostic entry object substituted, exact 56-file initial fixture, 24 objects/four curtains, physical 2560×1440, Engine's own 45°/far-100 camera and changing scripted world clock. Foreign networking remains isolated and ordinary app timestamp queries remain disabled. The temporary fixture profile is the existing keyless Zach perspective, attributed to Zachary Zhang; no inhabited Person/save was modified.

1. **First attempt, 47.475 s enclosing wall:** foreground activation worked, but the controller waited for a console string this Xcode version does not emit. It stopped without admitting measured samples. Preserve `/private/tmp/earthcall-6ez-trace-final` and its failed receipt; no performance claim comes from it.
2. **Handshake repair:** use `xctrace --notify-tracing-started` and a uniquely named Darwin file-descriptor notification. The SDK documents actual delivery on the descriptor; the polling-token API separately warns about initial/false-positive checks, so that API was not used. A standalone control verified no initial notification, one matching posted token and no duplicate. Executed runner/C++ bytes were preserved separately before later edits.
3. **Retry:** `/private/tmp/earthcall-6ez-trace-notify`. Native process exits **0** after four 64-frame queued OFF/ON/ON/OFF samples, two separately paid activation frames per sample, 128 focused warmup frames and the recording-start acknowledgment. All measured camera/clock/focus/lane/composition checks pass. The real device binding limit is 128 MiB and strip height is 48.
4. **Save failure:** the native run succeeded, but the controller's separate 60-second save wait expired. Profiler exit is **−9**, not a successful recording exit. The incomplete bundle contains a 2,813,989,320-byte raw `trace-data.atrc`. Exporting the incomplete bundle correctly refuses with “Document Missing Template Error.”
5. **Supported recovery:** `xctrace export --input <raw.atrc> --template 'Metal System Trace' --instrument 'Points of Interest' --toc` succeeds. `xctrace import` of that same raw file into `recovered.trace` also succeeds, then selected table exports succeed. Recovery is not a fresh GPU run. Imported recording duration is 18.249275 s. Both original attempt receipts remain failed where they failed; recovery does not rewrite them.

The two native enclosing process scopes total **144.065804 s**, within the declared 300-second cumulative bound. Subsequent recovery/export/replay work is CPU/tool processing, not additional native rendering. Full raw system traces remain local in `/private/tmp`; they are not committed. The archive contains only app-scoped normalized observations and the bounded source/receipt evidence.

## Missing markers and attribution boundary

The executed log used category `6ez`; the installed Points of Interest tables are filtered to `PointsOfInterest`. The requested sample-interval table returns **zero rows**. Do not claim sample signposts were observed. SDK `os/signpost.h` documents `OS_LOG_CATEGORY_POINTS_OF_INTEREST` as the category displayed by default. The future runner now uses it and allows the remaining enclosing budget for saving. That repaired harness compiles/links at `/private/tmp/earthcall-6fa-trace-prepared`; it has **not run**. No third window was launched.

We instead reconstruct frame ownership from actual app presentation command-buffer IDs and retain the inference explicitly:

- All **264** app present IDs occur in the app submission table, are strictly ordered and identify native `Present` command buffers.
- Every resulting group matches the native activation/sample mode sequence: **17** submissions / **5** empty Pre Pass buffers for every Current tick, **497 / 245** for every routed tick. There are no exceptional frame groups. Two trailing submissions belong to final publication readback and are excluded from frame metrics.
- Every Current frame has exactly **one** unique compute encoder, matching production parameter preparation. Every routed frame has exactly **241**, matching that preparation followed by 30×8 ordered routed passes. Deduplication is by actual command-buffer ID; nested GPU interval rows are not treated as additional dispatches.
- Every one of the **36,998 app GPU interval rows** refers to a recorded app submission, and `GPU start − recorded CPU-to-GPU latency == recorded commit endpoint` exactly. The exported schema identifies the separate Encoder ID; it is not misused as another command-buffer range endpoint.
- The unchanged source and §6ez's CPU interception establish the repeated order: setup, reset, classify, args, S1, S2, fallback producer, consumer. Stage names below are **inferred from that order**, not observed shader-function labels.

These controls provide a source-bound frame/stage interpretation, not a general-purpose frame mapper. The replay refuses dropped presents, forged modes, missing Pre Pass submissions, foreign GPU commands, invalid GPU times and altered commit latency. Six deliberate controls pass. CPU profiling yields 4,129 samples, but local resymbolication refuses because no relevant dSYMs are available. No named CPU hot-function attribution is claimed.

## Recorded costs and what they mean

| Sample order | Lane | Instrumented complete ms/frame | App GPU active-union ms/frame |
|---|---|---:|---:|
| 0 | Current | 65.537 | 63.691 |
| 1 | Routed | 65.602 | 63.999 |
| 2 | Routed | 64.894 | 63.387 |
| 3 | Current | 65.758 | 63.581 |

The pooled active unions are **63.635731 / 63.692744 ms**. This is the union of this app's recorded activity intervals across channels, avoiding nested/overlapping double counts. It is not device-wide busy percentage, useful ALU work, physical scanout or register/bandwidth evidence. The instrumented endFrame host span rises **0.433536 → 6.078955 ms**; it still includes flush/finalize/submit/release and possible waits. Submission-event lifetimes are not summed and called CPU time.

| Routed pass role, source-order inference | Mean activity ms/frame |
|---|---:|
| Ray setup | 1.666 |
| Counter reset | 0.082 |
| Tile classification | 9.093 |
| Indirect arguments | 0.151 |
| S1 producer | 7.461 |
| S2 producer | 8.587 |
| Fallback producer | 18.077 |
| Transport consumer | 17.314 |

Classification plus S1/S2/fallback activity totals **43.217 ms**—about 2.5× consumer activity. This identifies the expensive part of the current ordered computation; it does not prove all of that time is mathematically necessary, pure arithmetic or removable. “Fallback producer” is the schedule-producing stage, not an assertion that the final image fell back to Current. The small inter-activity gaps in this run do not establish a dominant strip-idle bubble.

**Interpretation:** the consumer is materially cheaper than Current's roughly 63.5 ms fragment-channel activity in this diagnostic, but constructing the schedule spends that headroom. Comparing those stages is not a standalone speedup claim: a renderer must pay the complete chain. The trace shows no meaningful overall gain. Instrumentation, hardware state and scope differ from clean §6ey, so do not replace its −6.17% queued/no-fenced-gain account with these ~65 ms values or extrapolate an inhabited FPS.

## Next bounded direction

First test an execution-only ablation: encode the unchanged ordered dispatch stream in one compute pass, keeping the same rays, uniforms, scratch, indirect arguments, shaders and refusal protocol. This directly targets the extra 240 active plus 240 empty native command buffers per routed frame, without taller strips or doubled scratch. WebGPU defines a usage scope per compute dispatch and ordered scopes, which supports investigating this packaging; it is not a performance guarantee. [Primary WebGPU synchronization specification](https://gpuweb.github.io/gpuweb/#synchronization).

Before cost, require actual native full-output/publication parity against the original routed lane, unchanged emitted WGSL, poisoned-resource and forced-Current-refusal controls. Do not publish a winner based on fewer command buffers. Then measure clean complete costs against original routed and Current with a declared cap; any new foreground run needs fresh readiness. Backend packaging belongs under Screen's Kernel substrate and adds no domain meaning or private mathematical evaluator.

The larger GPU target is **classification and schedule construction**, especially fallback and classification activity. If pass packaging is insufficient, pursue a measured producer-work reduction while preserving the bound and current-or-better numerical gates. Do not make the schedule cheaper by weakening error tolerances or silently moving generic mathematical meaning out of OntoMath. Consumer-only work, larger buffers and CPU reference micro-optimizations are not the leading ~43 ms target this trace identifies. Generic certification and full-population/wider-clock/field mathematical admission remain open.

## Evidence and replay

[Compact result](GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.json) · [lossless app-only observations, analysis, executed sources, build/attempt receipts and future prepared harness](GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.zip). Archive **2,109,068 bytes**, SHA256 `31fd55fc7283dc5166719fdd16561bf0c15c3cb9beb28621fa2abdd1b4e0bfab`. The 16.24 MB normalized payload is compressed inside the archive; the flat JSON stays compact. Raw XML/.atrc digests bind the local originals. Every archive payload digest and a fresh retained-observation replay pass. No full raw system trace, object or executable is committed.

```sh
python3 -B scratch/probes/ray_transport_6fa_trace.py --root "$PWD" \
  --work /private/tmp/earthcall-6fa-replay-new \
  --replay docs/audits/GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.zip
```

**Shipping account:** a new hardware diagnostic and corrected direction, no new optimization or clean speedup. Zero new mathematical reference rows, no physical WASD/appearance/Person acceptance. Zach's foreground activation is confirmed; those separate acceptance boxes remain open. No push/PR, default checkout, installed app or live-save change.

Signed: Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T10:18:44.075732-07:00 (PDT).
