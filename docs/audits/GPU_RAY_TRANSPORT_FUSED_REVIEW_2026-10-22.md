# §6cw — review of fused normal execution and next diagnostic

Zach requested review, analysis and next directions after Sparkly’s latest pass. His shipping criterion remains current-or-better authored-equation accuracy with a substantial native Northern Veil gain. Reviewed ray commit `263251da8ece7ffb5d9d3a3d61fbaa274877ab76` on `claude/ray-domain-transport-rung0`. Parents: [Sparkly’s §6cv report](GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.md), [§6cu review](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md), and [the existing Intercom assignment/reply](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Latest located Firmament counsel remains [post-§6cr](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CR_LINEAR_Z_CULL_SUBDIVISION_CACHE_AND_SECANT_2026-10-22.md); no new review of this fusion pass was found in either local copy.

## What the evidence establishes

**Actual fused outputs do agree.** Independent replay compares `streamed_schedule.cases[].normal_rows`, which really come from `cpFusedNormal` in mode2, against the guarded outputs on all32,784 distinct rays at all three capacities. All four f32 output channels agree bit-for-bit. Poisoned native execution checks are present in the exact compiled source. Forced rows likewise match the contemporaneous native baseline bit-for-bit on all32,784 rays. Both65536/32768-resolution holdout gates pass on the actual fused rows; dense truth covers192 predetermined rays, not all32,784. The all-query AST assertions run on the unfused control; actual fused output equality transfers that finite-capture result. They are not newly evaluated AST node traces from the fused kernel.

**This implementation loses complete-query cost.** Replayed balanced order and encoding+completion accounting reproduce all nine observations. These are ms per complete32,784-query workload, including the fallback dispatch; cold setup/readbacks/reference remain separate. No new native timing was run after this clear loss.

|Tile capacity, repetitions|Current|Unfused bulk control|Fused stream|
|---|---:|---:|---:|
|32768,4|1.582799|1.729075|2.069671|
|16384,4|1.726825|1.908048|2.469507|
|8192,4|2.434365|2.629569|4.107941|

At the primary point, fused is30.76% slower than current and19.70% slower than the unfused bulk control. These controls have different pass/chunk organization; this does not isolate register pressure. All nine mean comparisons lose to current, so promotion and a second promotion timing series are unjustified. This says this implementation loses on this workload, not that all fusion or ray-domain transport is impossible. No full-frame/FPS inference.

## Findings and corrections

1. **Changed capacities violate the assigned isolation contract.** Host configuration is64 singles/16 overlaps per ray, while the hand-copied fused shader uses32/8 and explicitly refuses above those limits. Its guards prevent an out-of-bounds write, but introduce extra fallback conditions. The unfused capture peaks at26/4, which fits, yet its counts cannot prove the fused path’s counts. This is an admission/coverage defect, not demonstrated wrong pixels. The original generator now calls `require_contract` before compilation/native work and refuses32/8. Its post-run admission also refuses missing actual fused packet evidence and compares exported packet endpoints/counts/masks against the immutable guarded capture. Historical source/data remain archived; no shader was silently resized and no unexecuted corrected timing result is asserted.

2. **Old counters are not a fused structural witness.** `schedule_census_inputs`, total247326 singles/1799 overlaps, zero normal fallbacks, the merge-policy controls and `device_fused_unfused_parity_pass` describe earlier/global producer or older consumer execution in this same executable. Actual mode2 rows establish output parity, but the normal fused headers are overwritten by forced fallback before their readback. Local packets are never exported. Hence actual fused endpoint/mask equality, all12102 matched affine endpoints, outer support unions, normal fallback count, and synthetic capacity/budget boundaries were not independently witnessed. The new portable review labels these limits and refuses missing packet evidence rather than rebranding the old census. Packet emission must be composed from existing fragments, or explicit source identity/drift checks must accompany any copying.

3. **Allocated bytes were mislabeled as traffic.** The unfused control’s populated packets occupy3986000 logical bytes (247326+1799 packets,16 bytes each); one logical write plus read is7972000 bytes, before caches/transactions. Padded64/16 packet capacity is41963520 bytes; retained48-byte ray headers add1573632, totaling43537152. The reported43537968 is a different capacity/bookkeeping total, not an observed read/write volume. The fused kernel omits global packet accesses, but all control/tile schedule buffers remain allocated in this benchmark. Global residency was not eliminated. No memory counters measure actual device traffic.

4. **The physical diagnosis is not established.** Private arrays are source-visible state; actual register allocation, spills, occupancy, execution width and device traffic were not queried. These remain hypotheses. `execute` records every tile/repetition into ONE command buffer and performs ONE submit/blocking completion per measured batch, not one host fence per tile. Extra compute passes remain real, but the stated per-tile host-fence cause is false. `batched_fence_series` times the old unfused connected path, not `cpFusedNormal`; its phase clocks cannot explain the fused regression or be added into a lower bound. No hardware-bandwidth assumption establishes a cause.

The historical §6cv prose/machine record retain their original claims for attribution; a prominent review notice and explicit machine review field qualify them. Intercom chronology is append-only. Production renderer, defaults and authored saves are untouched. The quarantined historical runner now fails closed because its contract is incomplete; this is intentional, with a replay command below.

## Executed verification and evidence

[Machine provenance](GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.json), [executed replay](GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22_REPLAY.json), [portable bundle](GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.zip), and `scratch/probes/ray_transport_gpu_fused_review.py`. Nine documents share five large arrays (10.7MB compressed). JSON values reconstruct losslessly; source bytes are exact. Current historical shader generation matches the executed C++ SHA256 `6261f73ee99750e94775d95eb32d67290a716476502c4108abc2634d71a2f3b2`; the admission-only runner repair does not change generated shader bytes. Existing immutable guarded archives supply the comparison. Nine corruption/admission refusals pass, including changed capacities and absent actual fused packets. The original runner refuses before GPU initialization; no native lane was claimed or used this pass.

```sh
python3 -B scratch/probes/ray_transport_gpu_fused_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.json --output /private/tmp/fused-review-fresh.json
```

## Next assignment and shipping standing

Sparkly executes a **bounded work/resource census**, Firmament reviews the interpretation. First close the trace/capacity gaps without changing mathematics: restore the assigned64/16 contract in a separately named rebuild; compose existing producer/consumer fragments; export untimed per-ray packets/normal headers, compare against guarded endpoints/masks, and compare instrumented transport against the actual uninstrumented variant. Exercise budget/capacity failure paths explicitly. Instrumentation is not timing evidence.

Count actual per-ray/member `evalPhi` evaluations, coarse cells, refinement iterations, maximum stack depth, retained uncertain bands, packet counts and fallback reasons; count normal/overlap density/noise evaluations separately from the root producer. Export distributions and tails, not merely totals. Keep work counts distinct from device time. If backend resource/counter data are accessible, record actual pipeline identity/compiler settings and measurements; otherwise say unavailable. Do not turn declared private-array sizes into measured spills/occupancy, or infer SIMD width from workgroup_size64.

Use this census to choose ONE next representation change with a stated falsifiable cost hypothesis and preserved equation/uncertainty/transport contract. Do not start another culling+secant+sampling+fusion bundle. Do not change production or repeat nine losing timing settings merely to tune capacities. A new timing candidate is justified only after the diagnosis identifies an avoidable cost and a meaningful correction; then repeat the established actual-variant accuracy gates and complete connected paired timings, with a second clean series only for a possible win. Full next directions are appended to both Intercom copies.

All three shipping gates remain **OPEN**: (1) profitable repeatable connected distinct-ray execution; (2) generic OntoMath source/density/root and revision/clock/device lifetimes plus full viewport/depth/composition/frame economics; (3) inhabited moving p95 and Zach acceptance. Mathematical meaning belongs to OntoMath; Screen lowers execution. This is still frozen-fixture compute evidence. No application-visible change, FPS gain, remote delivery, push or new Person acceptance is claimed.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-22T19:31:41.653259-07:00.
