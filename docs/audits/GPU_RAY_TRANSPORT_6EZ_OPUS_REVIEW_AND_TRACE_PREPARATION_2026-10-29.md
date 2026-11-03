# §6ez — Opus continuation review and bounded Metal trace preparation

Zach asked: “Opus 5.5 got here and continued from where you left off. Review his work and do next pass.” This continues [Opus's reply in the existing transport Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md), headed “Opus takes the torch after §6ey,” and the [§6ey handoff](OPUS55_HANDOFF_AFTER_6EY_2026-10-29.md). Human criterion: current-or-better authored accuracy plus substantial complete native benefit, with experimental feature defaultON and Current fallback. No reduced resolution, diminished interval budget or prettier-picture substitution.

**Reviewed HEAD:** `687dd98e27bd8537de0c18600ee5550881a3ef8b`, branch `claude/ray-domain-transport-rung0`, checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`. Opus's commits `0b35e3b84` and `687dd98e2` preserve the completed §6ey artifacts and update the progress account. Production `src/` is byte-unchanged from `04c7d4032` through this review. No subagents, GPU execution, live-save/default-branch/install change, push or PR in §6ez.

## Verdict and corrections

**Retain Opus's checkpoint and the trace-first direction.** The stored §6ey archive independently replays successfully again: 2,048 measured frames, six captures and six checker refusals. This is replay of dated native observations, not new Metal execution. Both timing scopes and the open numerical/Person gates are preserved. There is no implementation regression to repair in his two commits.

Four interpretations need tightening before selecting an optimization:

1. **The residual is not measured GPU time.** Independently recomputing the retained fenced means gives complete-frame delta −0.116435 ms, host-tick delta +5.372375 ms and residual delta −5.488810 ms. That arithmetic is correct. The residual can include GPU completion, presentation/queue/driver effects and measurement-boundary overhead. Opus's Intercom heading calls this a “GPU saving”; its body appropriately calls it a hypothesis. The heading must not become an established result. Absence of an explicit steady `wgpuDevicePoll` does not exclude waits inside native API calls.
2. **This executable uses wgpu-native, not Dawn.** Its Release link command binds `third_party/wgpu/lib/libwgpu_native.a`; CMake confirms that route. Dawn is used by a separate WASM configuration. The next CPU trace should inspect this binary's wgpu-native/Metal and driver stacks. “Dawn validation costs several milliseconds” is not an attribution for this build.
3. **The recorded “submit” span includes more than submission.** `EngineRender.cpp` measures the entire `currentRenderer().endFrame()`. That includes mesh/SDF draw flushing, pass closure, command-buffer finalization, queue submission and releases. The +4.61 ms delta is a valid host span, not an isolated `wgpuQueueSubmit` duration or proof of host computation rather than waiting.
4. **A higher requested binding limit alone cannot enlarge the strips.** `ensure()` uses `min(48, limit/(width*64*16))`, rounded down to four rows. The 48-row cap must also change in any taller-strip experiment. Larger limits are capabilities to query/request, not proven M5 capability or a cost-free change. Double buffering introduces more scratch and removes some reuse hazards; it does not by itself prove concurrent scheduling or a gain. Some producer→consumer and consumer→next-strip reuse dependencies are real, but the exact hardware stalls remain unmeasured.

## Executed CPU check of actual host encoding

`scratch/probes/ray_transport_6ez_review.py` extracts the **verbatim production** `ensure`, `releaseBuffers` and `encode` definitions and bind-group helpers. It compiles them against the real headers with intercepted WebGPU calls. Shader compilation is refused; no GPU library is linked or device acquired. The interception records allocations, pass order, direct/indirect dispatches, bind-group contents and the actual per-strip uniform windows. An independent checker verifies expected stage order, coverage, unchanged 64/16 interval ceilings and shared setup/consumer scratch handles.

| Mocked dimensions / binding limit | Rows/strip | Strips | Compute passes | Bind groups | Allocated buffer bytes |
|---|---:|---:|---:|---:|---:|
| 2560×1440 / 128 MiB | 48 | 30 | 240 | 125 | 222,781,496 |
| 2560×1440 / 1 GiB | 48 | 30 | 240 | 125 | 222,781,496 |
| 2560×1440 / 64 MiB | 24 | 60 | 480 | 245 | 140,881,976 |
| 320×181 / 128 MiB | 48 | 4 | 32 | 21 | 21,401,656 |
| 7680×4320 / 1 GiB | refused | — | — | — | 0 |
| 0×1440 / 128 MiB | refused | — | — | — | 0 |

The 2560×1440 case executes 150 direct and 90 indirect dispatch encodings. Five deliberate observation mutations refuse: missing pass, wrong producer order, repeated/lost window, diminished interval budget and rebound scratch. These are checker controls, not GPU omission controls. Allocation bytes are requested sizes from intercepted calls, not measured resident memory, bandwidth traffic or device overhead. The mocked 1 GiB value is not a hardware capability claim. These checks confirm Opus's counts and expose the cap; they cannot establish what costs time.

Reproduce without opening a window:

```sh
python3 -B scratch/probes/ray_transport_6ez_review.py \
  --root "$PWD" --work /private/tmp/earthcall-6ez-review-reproduction
```

## Next trace prepared and linked; not executed

`scratch/probes/ray_transport_6ez_trace.py prepare --work <fresh-directory>` builds a diagnostic main against the same 238 recorded Release app objects, replacing only the entry object. The generated main retains §6ey's Engine/scene/rendering path and adds `os_signpost` frame/sample intervals. The copied initial fixture matches all **56 files** of the successful §6ey execution. Its temporary `Zach.json` is the same isolated, keyless human-perspective fixture, attributed to Zachary Zhang by the existing fixture tool; the 24-object/four-curtain authored scene is copied unchanged. No inhabited Person or save is edited.

Preparation and linking **passed** at `/private/tmp/earthcall-6ez-trace-final`. Derived C++ SHA256 `63fda23777dd04ec7360deb43d3bcdc8e73e6bf6d96d2d74d8fc2eb5b4eae9e5`; executable SHA256 `7664bbcea3d63ad0780e7c35977384b730fbc0e7793631830309df2a6553f832`. These are preparation evidence only. Native execution, signpost visibility, attach permissions and export schemas remain unverified.

Installed `xctrace` reports Metal System Trace, Points of Interest and Time Profiler. The prepared runner uses Metal System Trace plus Points of Interest, attaches after 128 untraced warmup frames, waits for recording acknowledgment, then admits four 64-frame queued OFF/ON/ON/OFF samples. Eight mode-activation frames are outside the sample intervals. The trace is capped at 45 seconds; the single process attempt has a 240-second enclosing cap including cleanup. Ordinary app GPU timestamp queries remain disabled. Trace instrumentation and signposts distinguish this binary from the clean §6ey harness; instrumented costs cannot replace clean economics.

The user was asked whether they can keep the temporary window foreground; no new readiness answer arrived during this pass. **No window has been launched.** Existing “im ready now” belonged to the completed §6ey run. Do not reuse it to surprise-launch a later trace.

Once the user is ready, the concrete command is:

```sh
python3 -B scratch/probes/ray_transport_6ez_trace.py run \
  --work /private/tmp/earthcall-6ez-trace-final
```

The app must stay foreground until it closes. The runner creates a fresh receipt even on failure and stops its own processes. If attachment/recording fails, preserve the attempt; do not treat a saved empty trace or compiled harness as profiling evidence. Inspect the exported table of contents, target identity, sample/frame signs, focus/dispatch and source/binary binding before deriving GPU/CPU costs.

The trace must distinguish useful GPU work, producer/consumer intervals and reuse gaps, drawable/present pacing, and native finalization/submit CPU work versus waits. Then choose one intervention. Taller strips require explicit capability **and** cap/memory work; scratch banking requires ownership and aliasing checks. Do not combine them before an ablation. Preserve mathematical operations and Current refusal; same-ray scheduling changes first need complete native parity, not invented fresh reference coverage. If rays/math change, bind fresh references to the changed inputs. Wider mathematical admission remains open either way.

## Shipping account and artifacts

**No new speedup this pass.** Last actual complete-frame evidence remains queued −6.17% (94.54→88.73 ms), with no fenced benefit (~100 ms). The reciprocal queued batch cost is about 11.3 frames/s, not an observed physical scanout or accepted inhabited FPS. Reaching 33.3 ms requires about a 2.66× improvement in that measured scope; this is distance, not a forecast. No new reference rows, no full-population/wider-clock/field closure, no physical WASD/appearance/Person acceptance. Feature defaultON and Current fallback remain intact.

[CPU result and preparation identity](GPU_RAY_TRANSPORT_6EZ_OPUS_REVIEW_AND_TRACE_PREPARATION_2026-10-29.json) · [lossless observations, generated sources, build/replay logs and receipts](GPU_RAY_TRANSPORT_6EZ_OPUS_REVIEW_AND_TRACE_PREPARATION_2026-10-29.zip). ZIP 59,332 bytes, SHA256 `a71bbe1d4580a6305ebe5afa987aa63dcbd8ed2107cc122438e7b8d2be76515e`; every payload digest was checked after creation. Executables/objects are represented by hashes, not committed payloads. No `.trace` exists because none was recorded. GPU lane remains RELEASED.

Signed: Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T05:44:06.776097-07:00 (PDT).


**Later continuation — §6fa:** Zach subsequently confirmed readiness; [actual native execution, tooling failures, supported raw-trace recovery and attribution](GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.md) supersede the pending-readiness status above. The old prepared-directory command is historical and that directory has a receipt; do not rerun it. Future corrected preparation is `/private/tmp/earthcall-6fa-trace-prepared`, not executed. No further foreground run without fresh readiness. Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T10:23:43.755139-07:00.
