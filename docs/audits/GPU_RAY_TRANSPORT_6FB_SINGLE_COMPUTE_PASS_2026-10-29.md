# §6fb — one compute pass for the ordered routed dispatch stream: exact parity, small clean gain over original routed, no gain over Current

Continues [§6fa](GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.md) (which found 240 extra encoder-bearing + 240 empty Pre-Pass Metal command buffers per routed frame) and the [Opus transport Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Origin: Sixth-One Sun's §6fa "next bounded direction" and Zach's commission of this bounded experiment; the Firmament counsel endorsed the same step. WebGPU usage scopes are per dispatch ([spec](https://gpuweb.github.io/gpuweb/#synchronization)); that licensed trying it, and the native results below are what decide it.

## What changed (execution packaging only)
`KernelRoutedTransport::encode` (`VolumeRoutedTransport.inc`) opens one compute pass for the whole 30-strip stream when `coalescePasses` is set; otherwise it opens a pass per dispatch as before. WGSL, dispatches, order, workgroup counts, uniforms, scratch, indirect args, publication ownership and whole-ray refusal are untouched. Kernel diagnostics `encodedPasses/encodedDispatches/dispatchTrace` (FNV-1a over pipeline, workgroups/indirect offset, kind) record what was encoded. **`coalescePasses` now defaults ON**; `setRoutedCoalescedPassesForDiagnostics(false)` restores the original for A/B. The default flip itself was compile-verified, not separately run natively (the tested path is identical to `coalescePasses=true`). Current fallback and feature-branch default-ON routing are unchanged.

## Correctness (native Release Engine, 2560×1440, wgpu-native/Metal, one process, 32.7 s)
Poses 0 and 32; per pose nine captures: Current, original, original repeat, coalesced, poisoned original, poisoned coalesced, forced-refusal original/coalesced/poisoned-coalesced.
- **Execution counts:** original 240 passes / 240 dispatches; coalesced **1 pass / 240 dispatches**; identical dispatch-order hash `0c4d53948f4aa585` in every routed capture.
- **Publication and rendered snapshots bit-identical** coalesced vs original (3,686,400 / 2,553,614 non-empty pixels at poses 0/32, zero refused, zero nonfinite). Original-vs-original-repeat identical (determinism control).
- **Poison control:** publication, rays, singles, overlaps, counts, tile lists and args filled with 0xFF; readback before each frame proved all 14,745,600 publication words NaN. Both lanes still produced the clean result.
- **Forced refusal (work limit 0):** zero published transport, all pixels refused, snapshot equals Current's in original, coalesced and poisoned-coalesced. Routed ≠ Current when limit is 1024.
- Native log: no validation, uncaptured-error, device-lost or compiler messages. The harness refuses those, stale/nonfinite words, wrong counts and unexpected set counts.

## Timing (separate screen run, 117.1 s; cumulative native total 149.8 s of declared 600 s cap)
Lanes Current / original / coalesced, 4 rotated blocks per series, 64 paid warmup frames per lane, 2 separately paid activation frames per sample, 64 measured frames. Queued = 64 frames + terminal drain; fenced = queue completion awaited every frame. App GPU timestamps off. Instrumented by nothing beyond the wall clocks.

| ms/frame | Current | Original | Coalesced | Coalesced vs Original | Coalesced vs Current |
|---|---:|---:|---:|---:|---:|
| Queued complete | 62.092 | 63.360 | 62.421 | **−1.48%** | +0.53% |
| Fenced complete | 66.493 | 69.335 | 66.685 | **−3.82%** | +0.29% |

Coalesced was below original in all 8 paired block samples. Host `submit_ms` falls 3.235→0.298 (queued) and 2.927→0.222 (fenced); it was the cost visible in §6fa's endFrame growth. **Unfavorable and not hidden:** in this protocol original routed is *slower* than Current (+2.04% / +4.27%), unlike §6ey's −6.17% queued (different protocol: OFF/ON/ON/OFF order, 2,048 frames). Coalesced merely restores routed to Current's cost (+0.5% / +0.3%, within sample spread: Current samples span 61.26–63.89). **No speedup over Current, no inhabited 30 FPS claim.**

## Decision and what remains
Correct and measurably better than original routed → retained, default ON, limits above. It removes packaging overhead; it does not buy the ~45 ms headroom the §6fa trace shows the consumer saves. Next directions, in the trace's order: (1) classification + schedule producers (≈43 ms): the Firmament counsel's idea that `kClassifyWgsl` routes empty and non-curtain-0 single tiles into the 4-curtain fallback producer is **unverified by me** — check the cited lines before acting; any producer change needs the same bitwise-parity gates and no tolerance loosening; (2) hoisting per-strip producer bind groups is now a small host-side residue; (3) re-run the §6ey-protocol comparison once producers shrink. Person acceptance of movement/appearance remains open.

Evidence: [results JSON](GPU_RAY_TRANSPORT_6FB_SINGLE_COMPUTE_PASS_2026-10-29.json) (parity + compact timing); witness `scratch/probes/ray_transport_6ey_engine.{cpp,py}` modes `parity`/`packaging`; executed in `/private/tmp/earthcall-6fb-engine`. No push/PR; no live save, default checkout or installed app touched. GPU lane RELEASED.

Signed: Claude Code / Sonnet 5.5 / session `78cc5135-a805-4233-84ab-54fb16681c11` / 2026-10-29 (PDT).


**Independent §6fc continuation:** [All1536timingframes and206,438,400decodedRGBA comparison bytes replay; exact-route baseline/accuracy distinction and corrections](GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.md). Raw publication arrays were not retained, so independent publication replay is limited to the recorded full-readback FNV digests. The exact pre-default-flip header bytes are unavailable and explicitly attested. Original executed tools are archived; future packaging builds use `ray_transport_6fb_engine.cpp/.py`, with historical6eyfilesrestored. Ordinary FNV work is now opt-in; Releasebuild and CPUcommand/bindingidentity pass, no new foreground/native Engine gain claimed. Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T11:19:46.729976-07:00.
