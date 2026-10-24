# Ray transport — Firmament counsel, bounded packet streaming and the cost of small chunks

**Default-checkout evidence relay:** implementation is on local `claude/ray-domain-transport-rung0`, commit `d4c9e07e7`. New diagnostic links below point to that ray checkout; this relay does not install or remotely publish the implementation.

Zach requested the Firmament's new counsel and the next pass after [§6ck](GPU_RAY_TRANSPORT_PREPARED_COEFFICIENTS_2026-10-22.md), local checkpoint `ada9ee762`. I found his review of §6cf–§6ck in the **Gemini application conversation**, through computer use. It was not yet in either Intercom copy; Zach confirmed he was still writing and instructed me to proceed. This audit is my implementation/review response, not a substitute author post. It continues the [existing council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md).

**Outcome:** fixed-capacity schedule storage can be reused across separate producer/consumer/fallback dispatches without changing any output bits on the two admitted ray fixtures. All four capacities pass exact normal and forced-native-fallback comparisons, including the final 16-ray chunk. Small chunks lose substantially on native cost; a 10 MiB ring allocation control passes but does not establish large-ring throughput or a viewport result. The GPU packet census also disproves the proposed general one-active-span-per-curtain compression. Production rendering, defaults, authored equations, saves/data home and installed app are unchanged. No new shipped performance claim, push or PR.

## Counsel received and the tightenings

Firmament proposed a maximum-width obligation for retained uncertainty bands, moving canonical density/root derivation into OntoMath at source lifetime, tiled two-pass execution, compact per-member knots, and tile-cone rejection. These are useful directions. Their specific claims need the following limits:

- **Uncertainty remains numerical work.** The existing diagnostic margin is not a complete Metal execution enclosure. `bow <= error` plus a four-unit panel cap does not by itself prove a universal GL2 error bound for the original noisy, multi-medium transport. A real optical-depth bound also needs all multiplicative factors and propagation through attenuation/radiance. No error certificate or native omission authority is added here.
- **Keep every retained span and kink.** A folded periodic level set can enter and leave the same ray several times. One entry/exit plus two affine kinks is not a general lossless schedule. Compress complete ordered interval/event records, with capacity refusal, rather than assuming one interval per member.
- **Cost causes are still hypotheses.** Current code uses eight initial bins per level-set search and 64-single/16-overlap packet capacities; the counsel used older 32-bin and 32/8 descriptions. Prior isolated phase clocks are non-additive and from a different conservative workload. They do not establish current reverse-view producer percentages, occupancy causality, a 2–4× tile gain, or zero register penalty. Tile cones need conservative whole-cone enclosures; checking four corner values alone is insufficient for nonlinear folds.
- **Use the real byte count.** 64,800 rays at 1,328 bytes/ray is about 86 MB before output, not under 16 MB. An 8,192-ray schedule allocation is 10 MiB; including a similarly bounded 48-byte input header and 16-byte output adds another 512 KiB. These are buffer arithmetic, not observed total process VRAM.

OntoMath still owns the generic mathematical density/root recipe; Screen should consume it. This pass tests the bounded execution mechanics while leaving that admission gate open.

## Executed storage contract

[`ray_transport_gpu_streamed_schedule.py`](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/scratch/probes/ray_transport_gpu_streamed_schedule.py) consumes the §6ck authored coefficient recipe and retains the existing generated producer, full ordered A* consumer and native fallback kernels. It changes their buffer views, not the ray equations, model margin, root work bounds, integration nodes, packet format or reference thresholds.

A fixed pair of single/overlap buffers holds one chunk at a time. Each chunk runs producer → consumer → native fallback before the next chunk reuses that pair. Separate compute passes preserve those dependencies. Input/output views select the original pixel order; 256-byte storage offsets and explicit chunk counts prevent tail overrun. Static full/tail uniforms and bindings are built before steady measurements. Work/capacity refusals still choose actual native fallback; no partially generated schedule is accepted. Global producer offsets are restored before subsequent diagnostic consumers.

Four capacities—176, 512, 1,024 and 8,192—run on **2,304 distinct inside rays** and **192 distinct spawn rays**, all captured at the authored native viewport in §6cj. Normal outputs match the accepted bulk candidate at every f32 bit. Forced budget-zero execution flags every ray and matches the current production-generated native marcher at every f32 bit. Outputs are NaN-poisoned first. The 176-capacity runs have final chunks of 16 rays in both views. All primary original-AST, closed-form, device parity and two-resolution reference gates remain green.

The 8,192-capacity case allocates **10,485,760 bytes** for the reusable schedule buffers. It has only one active chunk in these fixtures (2,304 or 192 inputs), so it is an allocation/guard/parity control, **not** a test of multiple large chunks or 8,192 simultaneous distinct rays. Comparison keeps the original bulk buffers alive too; no claim that total resident VRAM has fallen to 10 MiB follows. Input/output storage still covers the sparse query. Unique full-viewport ray generation, depth clipping, composition, complete frame resources and production lifetime are not exercised.

## Actual native cost and the rejected command-size attempt

Apple M5/Metal, timestamps off. Each timing covers command encoding plus submission-to-blocking-completion; those two components are also recorded separately. A matched sequence alternates current marcher, retained bulk connected candidate and streamed connected candidate in ABCCBA / CBAABC order. Four blocks supply eight observations per mode, at 1/2/4 repeated complete query batches. Cold buffer/bind-group setup and correctness poison/readback are separate. All raw observations are retained; no fence-floor subtraction is applied.

Final four-repetition inside means, normalized per complete 2,304-ray query:

| Capacity | Chunks | Schedule allocation | Matched bulk connected ms | Streamed connected ms |
|---|---:|---:|---:|---:|
| 176 | 14 | 220 KiB | 0.648 | 4.358 |
| 512 | 5 | 640 KiB | 0.739 | 2.125 |
| 1,024 | 3 | 1.25 MiB | 0.825 | 1.741 |
| 8,192 | 1 | 10 MiB | 0.959 | 0.790 |

The multi-chunk cases cost about **6.7× / 2.9× / 2.1×** their matched bulk controls. Encoding and completion both grow; kernel/pass count is explicit, but the precise driver/hardware cause is not measured. The last row executes only one chunk and cannot establish a streaming improvement or full-viewport economic gain. Small-query fence sensitivity remains substantial. The outside case similarly adds cost for two chunks; its one-chunk controls vary around the bulk cost. No new candidate is promoted on these measurements. The earlier 1.57–1.58× longer-batch sparse throughput remains its own evidence, not an FPS promise or a comparator with the same cost scope as this table.

The initial 1/16/64-repetition streaming screen passed exactness and forced fallback before a timing-loop command encoder failed with **Parent device is lost**, child exit -6 and no report. The exact failing repetition was not logged, so do not assign it to 16 or 64 or claim a specific Metal fence/register cause. That failed log and exact C++ are archived. Final runs cap the command screen at 1/2/4 and pass; this bounds the successful witness, **it does not repair or explain the device-loss limit**. A full viewport could have many more passes and needs an explicit bounded submission strategy and its own native stress witness.

## Independent schedule census: the compression counterexample

The report archives all original GPU-produced single/overlap packets and ray flags. An independent Python reconstruction unions only touching/overlapping intervals for each ray/member, retaining all positive gaps and every overlap member. It exactly reproduces the C++ census:

- Inside: 9,216 ray/member pairs; 5,881 empty, 3,127 with one retained span, 197 with two, **11 with three**. Thus **208 pairs have multiple disjoint spans**.
- Spawn: 768 pairs; 589 empty and 179 single-span in this selected fixture. This view-specific observation does not generalize the one-span assumption.

For pixel (260,20), member 3 retains [22.2565002,47.5980644] and [49.1091118,64.4714508]. A one-span descriptor bridges a positive gap and changes the integration schedule. These are complete retained numerical packets, including uncertainty, not a universal statement that every retained point has positive authored density. Future compression must preserve their order, gaps, all internal knots and fallback semantics, then pass the same output/reference and native cost gates.

## Reproduction and remaining bridge

[Machine record](GPU_RAY_TRANSPORT_BOUNDED_STREAMING_2026-10-22.json) pins ten compressed source/raw/failure archives. [`ray_transport_streamed_schedule_evidence.py`](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/scratch/probes/ray_transport_streamed_schedule_evidence.py) independently replays both primary numerical reports, eight normal/forced streaming cases, strict float-bit output, the full packet census, memory/tail arithmetic and complete timing accounting. Nine corruption controls refuse missing tail output, changed pixel order, nonfinite output, inexact/missing fallback, wrong allocation, missing timing pairs, invalid clocks and a false span summary.

Restore the preceding §6cj fixtures with its verifier, then use fresh paths:

```sh
python3 scratch/probes/ray_transport_streamed_schedule_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_BOUNDED_STREAMING_2026-10-22.json --fixtures /private/tmp/your-restored-pose-fixtures
python3 scratch/probes/ray_transport_gpu_streamed_schedule.py --root . --work /private/tmp/new-stream --build-only
python3 scratch/probes/ray_transport_gpu_streamed_schedule.py --root . --work /private/tmp/new-stream --run-only --case /private/tmp/your-restored-pose-fixtures/inside-full-control/manifest.json --label inside
```

This is scratch native compilation against the existing configured app objects; no broad suite/app rebuild or Person result is claimed.

**Standing shipment distance:** fixed-buffer schedule reuse and exact tail/fallback mechanics now have finite evidence. Profitable large-chunk streaming does not. Next capture more than 8,192 **distinct** native queries to test actual large-ring reuse/amortization, with original depth/clip semantics and independent held-out references; do not substitute more repeats of these 2,304 rays. Keep the generic density/root execution recipe and production source/revision/device lifetime as explicit parallel gates. Then complete unique full-viewport/depth/composition/frame costs (including empties and command-size safety), followed by moving inhabited p95 and Zach's appearance acceptance. The Person checks already on the list remain open; no new app-visible change exists.

All probes finished; scratch/native ownership released at closeout. Implementation stays on the local ray branch; default receives this signed audit/evidence and the council reply. No push/PR or authored/default/app change.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T11:16:00.866645-07:00`.
