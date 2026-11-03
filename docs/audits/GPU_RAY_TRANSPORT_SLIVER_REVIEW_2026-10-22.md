# §6cq — Sparkly review: guarded sliver correction passes accuracy, fails economics

Zach commissioned this review and corrective pass. His criterion remains faithful authored equations with current-or-better accuracy and a substantial native performance checkpoint, without requiring the whole campaign in one PR. This pass corrects the experiment and its accounting; it installs no renderer optimization.

Parents: [duo handoff](RAY_TRANSPORT_DUO_HANDOFF_POST_6CN_2026-10-22.md), [Sparkly §6cp](GPU_RAY_TRANSPORT_SLIVER_MERGE_2026-10-22.md), [§6co](GPU_RAY_TRANSPORT_STACK_AND_COMPACT_STATE_2026-10-22.md), [Firmament counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CN_COARSE_PHI_AND_ROOT_SLIVER_DISCOVERY_2026-10-22.md), and [Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Reviewed ray HEAD `a47602496`, including `2b0eed677`, `cbec3698f`, `553dc35cc`, on `claude/ray-domain-transport-rung0`.

## Findings and corrections

**Bulk was conflated with bounded streaming.** In the timing harness, mode0 runs current, mode1 runs the global bulk producer+consumer+fallback, and mode2 runs bounded tile storage. The capacity loop does not change mode1. Reported1.010×/1.016× means were bulk controls; all nine bounded-streaming means lose current. Those bulk means are real observations, but a mixed single series with small positive means is not a qualified or repeatable performance milestone. The old65536-capacity component series also loses connected cost. Source arrays multiplied by all invocations do not establish simultaneous spill/SLC residency, occupancy, or frame shares; phase clocks are non-additive.

**Width-only merging erases affine boundaries.** The rule knows width/adjacency but no uncertainty provenance or protected boundaries. Of12,102 matched original GPU affine endpoints, §6cp drops63. Example: ray114, member2, knee endpoint23.66913414001465. The guarded candidate retains all12,102 matched endpoints. The CPU-f32 formula only locates the relevant captured endpoint within2ULP; retention compares that original GPU endpoint’s exact bits. CPU and Metal division are not presumed identical. Both policies preserve the original per-member support unions on this capture; preserving the outer union does not preserve every kink.

**The separate guarded candidate repairs this.** `ray_transport_gpu_producer_sliver_guarded.py` tracks zero-level provenance through uncertainty-band unions, protects both band endpoints and explicit affine knots, and allows only short uncertainty pieces to join touching retained non-uncertain work. It retains the0.02 threshold, capacities, root/error/budget rules, old consumer, quadrature, overlap attenuation and whole-ray fallback. Nine semantic controls execute the exact shared merge helper on Metal, including affine/apex, gap, wide-band and missing-provenance refusals. This is a frozen-fixture numerical candidate, not generic source admission or a native omission certificate. Mathematical meaning belongs in OntoMath; Screen lowers it.

**Two corrected runs pass scoped accuracy and lose cost.** Each runs32,784 original-AST assertions below1e-5 (maximum RGB3.5763e-7/T8.9407e-7), both-resolution192-ray reference gates, poisoned output checks, normal streaming/bulk equality, and exact forced native fallback at8192/16384/32768 capacities. Nine native helper controls pass each run. Subpanels247,326 versus324,088 original/209,610 width-only; overlaps1799 versus2693/1793. Reference results match across corrected runs: RGB mean6.5011e-6/max4.7949e-5; T mean1.8916e-5/max6.9659e-5. Aggregate gates do not assert every individual ray improves.

Coarse-Φ and stack/compact predecessor output equality is independently reproduced on this capture (0 differing rays). Fused/unfused self-agreement alone would not establish that historical comparison; offline replay now checks it explicitly.

## Complete-query economics

M5/Metal; four repetitions; milliseconds per complete32,784-query workload. Includes encoding, submit and blocking completion; excludes cold preparation/bindings and readbacks. GPU timestamps off. Columns are contemporaneous within each row; do not subtract different processes or add component clocks.

|Series|Tile capacity (mode2 only)|Current mode0|Global bulk mode1|Bounded stream mode2|
|---|---:|---:|---:|---:|
|Historical width-only|8192|2.068548|2.047529|3.088199|
|Historical width-only|16384|1.839494|1.981824|2.422576|
|Historical width-only|32768|1.952625|1.922251|2.003332|
|Guarded first|8192|1.928887|2.014583|3.186357|
|Guarded first|16384|1.506828|1.680456|2.053538|
|Guarded first|32768|1.540803|1.663479|1.730217|
|Guarded confirmation|8192|1.619090|1.901356|2.682449|
|Guarded confirmation|16384|1.924914|1.987596|2.336384|
|Guarded confirmation|32768|1.739125|2.024130|2.006350|

Both corrected connected lanes lose at four repetitions in both series. Absolute clocks vary. No production/default/sample/budget/save/live-data/app change, push, shipped FPS or moving30FPS gain. The stronger policy retains required work; accuracy and economics remain separate gates.

Contemporary native outputs differ from the older captured baseline by at most5.96e-8, within the unchanged1e-5 native fidelity guard. Replay checks reference accuracy against both the original captured and contemporary baselines; both pass. Original-AST assertions ran natively over every query; raw AST nodes are not exported, so offline replay does not claim independent node reevaluation. Dense65536/32768-interval transport reference covers192 predetermined rays, never all32,784. A2560×1440 ray origin is not full-frame/depth/composition evidence.

## Evidence and reproduction

[Machine record](GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.json):32 hashed compressed source/raw artifacts. [Executed replay](GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22_REPLAY.json): five stages; hashes, both reference baselines, chronological structure, stream/bulk bits, forced fallback, mode order/clocks, original support unions and affine endpoints. Six corruption controls per historical stage/seven per guarded stage refuse (32 total). Compiled confirmation source matches current generator; nine static shader blocks match the first corrected run.

Self-contained replay from ray; no GPU or temporary fixture needed:

```sh
python3 -B scratch/probes/ray_transport_sliver_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.json --output /private/tmp/sliver-review-fresh.json
```

Native reproduction needs the configured desktop build, fixture, exclusive timing ownership and fresh external paths:

```sh
python3 -B scratch/probes/ray_transport_gpu_producer_sliver_guarded.py --root . --work /private/tmp/sliver-guarded-fresh --build-only
python3 -B scratch/probes/ray_transport_gpu_producer_sliver_guarded.py --root . --work /private/tmp/sliver-guarded-fresh --run-only --case /private/tmp/earthcall-wide-case/manifest.json --label fresh
```

Historical source/raw observations remain archived. Older audits receive an appended supersession notice; original correspondence stays append-only. Removing the sole blank non-event at default navigation journal line138 restored parsing; its old bytes are archived, and actual connection events are preserved. `nav find` works again.

## Remaining shipment gates

1. **Profitable connected distinct-ray execution: OPEN.** First-win milestone withdrawn. Next isolate event construction/scheduling under the guarded recipe and old consumer; preserve required splits and require repeatable connected cost wins.
2. **Generic math/lifetimes and full-frame integration: OPEN.** Fixed-offset fixture decoding remains; generic OntoMath density/root admission, source/clock/revision/device lifetimes, full viewport/depth/composition/frame cost remain.
3. **Moving inhabited p95 and Zach acceptance: OPEN.** Existing Person checks remain pending; there is no new visible application change.

Native lane released after two corrected series. Firmament/Sparkly/Prism should read this correction before extending the old first-win claim. Local relay is not remote publication, recipient acknowledgement, or a merge.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T17:04:46.369117-07:00`.
