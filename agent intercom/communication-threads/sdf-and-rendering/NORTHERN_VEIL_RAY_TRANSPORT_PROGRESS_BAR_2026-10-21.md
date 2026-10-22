# Northern Veil ray-transport progress — live council dashboard

**Human goal:** Zachary Zhang's substantial native Northern Veil speed checkpoint at current-or-better accuracy, followed by an inhabited movement result.
**Branch:** `claude/ray-domain-transport-rung0`.
**Council:** [existing Intercom thread](OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md).
**Latest producer experiment:** [target-only support / §6cd](../../../docs/audits/NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21.md) halves host preparation but remains insufficient and reduces consumer headroom.

**Current evidence:** [matched native consumer / §6cc](../../../docs/audits/NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21.md); [overlap repair and rejected fusion / §6cb](../../../docs/audits/NORTHERN_VEIL_METAL_OVERLAP_REPAIR_2026-10-21.md).

## Current state

**Complete unfused A* is 3.37× / 3.34× faster than the production-generated marcher on matching resident compute ray work, with better aggregate reference accuracy. Boundaries are precomputed. A full-viewport/producer/production/inhabited FPS win is still open. Shared-cell fusion remains rejected on cost.**

The initial Metal diagnostic omitted all 253 overlap intervals in its GPU kernels and its CPU comparator. Its “Lane A complete/certified,” production recommendation, exclusive-producer-bottleneck and 0.552 microseconds/ray performance-ceiling claims are superseded. Historical text and artifacts remain in the [initial audit](../../../docs/audits/NORTHERN_VEIL_METAL_DIAGNOSTIC_2026-10-21.md) and Intercom; this page tracks current qualified evidence.

| Gate | Status | Evidence / remaining work |
|---|---|---|
| Authored mathematical ownership | Required throughout | OntoMath owns meaning and reusable derivations; Screen lowers backend execution |
| CPU A* complete-product / projected-node consumer | Passed on frozen rays | Gate, extinction floor and original emission evaluated before quadrature averaging |
| Full single + overlap native execution | Passed on frozen rays | 23,776 single-medium subpanels + 253 chronologically coupled overlap intervals |
| Independent native arithmetic checks | Passed on frozen witness | Original MathNode comparator; five closed-form GPU cases; timestamps disabled |
| Native accuracy versus independent reference | Passed aggregate mean/p99/max | Per-ray regressions remain recorded; no universal certificate |
| Same-cell gradient fusion | Implemented, rejected on cost | Fused +17.99% / +17.51% in repeated 64-dispatch host-fenced batches |
| Unfused A* versus production-generated marcher | **Passed matching compute consumer gate** | 3.37× / 3.34× on 2,304 frozen rays repeated 128×; baseline T exact, RGB max delta 4.47e-8 |
| Unique full viewport and complete frame | **Open** | Actual boundary generation, depth/raster/composition obligations and full pass/frame total |
| Boundary production / amortization | **Experiment completed; still open** | Target-only root queries halve measured host preparation (~6.21→3.12 s/2,304 rays), but preserving accuracy needs extra panels; no shipping producer |
| Production revision/device/fallback lifecycle | Open | No A* source integration shipped; no default/budget change |
| Broad poses, clocks, sources and overlap | Open | Current native witness is one frozen camera/clock |
| Inhabited movement and Person acceptance | Open | Zach's “live in Northern Veil” checkpoint remains unverified |

These are evidence gates, not a percentage estimate. A finite numerical witness, hardware throughput, production integration and Person acceptance answer different questions.

## Native measured accuracy

2,304 frozen rays, clock 7.25, observed Apple M5 Metal adapter, original fixture hashes retained:

| Error | Current renderer | Complete A* native |
|---|---:|---:|
| RGB pooled mean | 5.22350e-5 | 6.92274e-6 |
| RGB pooled p99 | 7.49103e-4 | 5.05613e-5 |
| RGB max | 1.95826e-3 | 9.15713e-5 |
| T mean | 1.33515e-4 | 2.02057e-5 |
| T p99 | 1.43029e-3 | 1.09961e-4 |
| T max | 3.02491e-3 | 1.39098e-4 |

Fused/unfused outputs are bit-identical on these rows. Independent original-AST maximum disagreement: RGB 2.906e-7, T 6.557e-7. All five closed-form cases pass. Native-reference per-ray regressions: RGB 474/2,304; T 495/2,304. Lower aggregate error does not mean every ray improves.

## What the timing establishes

Two isolated processes, four warm batches/variant and six ABBA blocks at each batch size. At 64 identical sparse-ray dispatches per fence:

| Series | Unfused batch, ms | Fused batch, ms | Fused extra cost | Empty batch, ms |
|---|---:|---:|---:|---:|
| 1 | 8.89962 | 10.50065 | +17.99% | 1.54744 |
| 2 | 8.85242 | 10.40210 | +17.51% | 1.48065 |

Single-dispatch empty and real measurements share a roughly 1.5 ms submission/poll floor. These are completion-fenced host durations, not GPU timestamp measurements. No occupancy/register-pressure cause is established. This is not a full-viewport frame or an end-to-end FPS result. Unfused A* is now the positive matching-workload consumer candidate below; it has no production recommendation yet.

## Matching native consumer comparison — latest positive gate

The baseline retains the actual production-generated V5 integration body, proof data and GPU clock preparation. Both consumers use the same captured rays and clip endpoints. The baseline reproduces stored native transmittance exactly and RGB within 4.47e-8.

Two independent processes, 294,912 invocations per dispatch (2,304 distinct rays repeated 128×), four-dispatch batches, completion-fenced **host** elapsed time:

| Series | Current normalized, ms | Unfused A* normalized, ms | Consumer speedup |
|---|---:|---:|---:|
| 1 | 15.85933 | 4.70159 | **3.373×** |
| 2 | 15.87484 | 4.74839 | **3.343×** |

All reference mean/p99/max gates and five closed-form GPU cases pass. Every replicated output is checked against its base ray. Input schedules are resident/precomputed and reused; these are not 294,912 distinct scene rays. Opaque raster/depth lookup and composition are omitted from both compute consumers. This result earns work on an affordable producer, not a shipped renderer/FPS claim.

## Producer and full-vision account

The CPU preparation office remains expensive: 902.79 segmentation range evaluations/ray and 7.66 retained medium pieces/ray. Of 17,657 retained pieces, 10,611 are indefinite; they remain represented and evaluated. Host preparation times cannot be divided by sparse GPU submission time to establish a future frame's bottleneck percentage.

The architectural aim remains:

`authored math → valid derivation/topology recipe → concrete ray instantiation → backend consumer`

Reuse the derivation at its longest justified lifetime while computing each ray's own endpoints, depths and colors. Neighboring rays do not inherit one ray's numerical answer. Query-space/movement-region analysis is a possible producer direction; the newly measured consumer headroom justifies the smallest concrete preparation/lookup experiment, without requiring a broad new framework.

## Latest producer pass — concrete tradeoff, insufficient for shipping

[§6cd audit](../../../docs/audits/NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21.md): a generic target-only support classifier avoids internal branch labels and retains uncertain spans. Seven support/refusal witnesses pass. Eight alternating CPU-only processes measure −49.73% concrete preparation, but ~1.35 ms/ray is still too expensive for per-pixel/frame work.

Deleting the branch partition also removes useful quadrature split points. Widths 4 and 2 fail independent accuracy; width 1 passes all six aggregate gates, original-AST agreement and five closed-form GPU cases. Two native processes then measure only **1.603× / 1.600×** versus current, using 49,302 single packets +350 overlaps and 905,840 resident bytes. This does not supersede the earlier 3.34–3.37× branch-aware consumer. Preserve the diagnostic; do not integrate this host producer. No complete frame or FPS gain is claimed.

The next producer must obtain useful mathematical split points cheaply, with reusable derivation/validity and each ray's own numerical endpoints; total preparation/instantiation/lookup/repair/upload/memory/lifetime must fit the native headroom.

## Current next-owner queue

Codex completed the dinner-watch repair, matched native consumer gate and target-only producer experiment, then releases the isolated GPU lane at the signed closeout in Intercom.

1. Preserve useful switch/kink/root split points with the cheapest general derivation/instantiation experiment; do not ship the measured target-only host loop. Count cold preparation, lookup, repair, invalidation, uploads and memory against the earlier branch-aware consumer headroom.
2. Qualify unique full-viewport output and complete pass/frame costs including producer and composition obligations.
3. Qualify broader poses/clocks/source/overlap and production fallback/device/revision lifetime.
4. Measure the moving inhabited scene and obtain Zach's acceptance.

Stop a candidate when its accuracy or cost gate fails. Preserve the failed evidence and prior retained renderer.

## Maintenance

This is a live status document. Update gate statuses, measured values, ownership and remaining work when evidence changes; preserve history in the linked audits and append-only Intercom. Keep both directions of the dashboard/Intercom link. Name the actual clock, workload and device, and never replace a missing native/inhabited result with a conceptual percentage.

Original dashboard: Original Prism Sun / GPT-5.6 Sol, 2026-10-21. Current measured-status repair: Codex, commissioned by Zach. No authored saves, installed app or production defaults changed.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T20:08:48.886942-07:00`.


Latest producer pass: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T20:29:03.842557-07:00`.
