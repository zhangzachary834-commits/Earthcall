# GPU volumetric ray transport — council catch-up and §6dh source review

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T14:55:25.865720-07:00.

**Human commission:** Zach: “CATCH UP WITH EVERYONE ON THE GPU VOLUMETRIC RAY TRANSPORT MASTERY ARC.” This is source inspection and review of existing native records, not a fresh GPU run or implementation pass. Reviewed ray checkout/old ray worktree HEAD `482ddd8e6`; default checkout HEAD `562575f1c` at inspection. The active ray checkout is `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`. Both ray copies contain the latest council posts; default's main handoff is behind them. No source/test change exists between `075c53e33` and the reviewed ray HEAD under `src/` or `tests/`: this campaign's new implementations remain scratch probes.

This continues the [existing handoff](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md), [Prism's source review](PRISM_REVIEW_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-25.md), and [M5 dossier](EARTHCALL_NATIVE_GPU_MASTERY_M5_ANCIENT_ARCHIVES_DOSSIER_2026-10-25.md). It does not replace the authors' historical reports.

## What advanced since §6dc

- **§6dd, Firmament:** exact old level-specific predicates were composed into whole-member bow skips and a monotone depth0 peel; parallel production received SoA scheduling and earlier packet export. This is a continuation of the strict-source experiment, not a general proof that a hardware layout fixes arithmetic differences.
- **§6de, Firmament/Sparkly:** paired same-cell Perlin and cooperative beam certificates reduced modeled/replayed math work, but all five recorded implementations lost native cost. Preserve the mathematical reuse opportunities; reject those representations' economics. The old register/spill/barrier causal explanations remain hypotheses without appropriate measurements.
- **§6df, council:** many neighboring rays share a small number of program shapes. Firmament correctly distinguished a posterior oracle (classification after full roots) from an a priori certificate (classification before full work). The initial census's35,986 contiguous intervals are not247,326 emitted quadrature subpanels. Those counts cannot silently justify changing the integration rule.
- **Firmament's intrinsic-coordinate proposal:** for the wave-sheet factor, nonlinear dependence is on local x, while other dependencies can be affine. Sharing wave evaluations at common local-x coordinates can avoid a cross-ray wave-displacement term. This is a promising factorization/recognized-operator route for OntoMath; it is not universal generic admission, a changed optical path-length measure, or a completed certified f32 implementation. Parallel/ill-conditioned coordinate inversion must retain conservative refusal/fallback.
- **§6dg, Sparkly:** a real a priori classifier and physically separate S1/S2/fallback pipelines exist. Roughly70% of the fixed capture's tiles route to the two common families. Prism's source review correctly separates that routing result from the unimplemented exact root executor, generic certificate consumer, cached bin payload, density forwarding and GPU-only dynamic routing.
- **§6dh, Sparkly:** two useful changes are now in the real §6dd harness: disjoint-piece merge/sweep bypass, then CPU-prepared intrinsic tile masks combined with that bypass. Their existing saved records report32,784 queries,247,326 singles,1,799 overlaps, zero normal fallbacks and192 dual-resolution reference holdouts. Current generators reproduce the executed C++ bytes for both (SHA256 below). The separate S1/S2 prototype still has a different semantic/reference contract.
- **Prism's latest M5 dossier:** import the mathematics, measure the actual representation. Distinguish declared shader state/workgroup size from measured live registers/spills, occupancy, cache pressure and pipeline execution width. Retaining certificates/residual structure has stronger architectural motivation than indiscriminately caching transient sampled values; profitability remains a device measurement.

## Existing real-harness clocks: compare each candidate to its own contemporary current

Completion-fenced recorded means, ms per complete32,784-ray query, normalized by repeats. These are fixed capture queries, not full viewport frames or FPS. No new clocks were collected here.

| Real §6dd candidate | Repeats | Current | Candidate | Candidate elapsed overhead |
|---|---:|---:|---:|---:|
| Disjoint merge/sweep bypass | 1 | 4.881651 | 5.123865 | +4.96% |
| Disjoint merge/sweep bypass | 2 | 4.196044 | 4.256833 | +1.45% |
| Disjoint merge/sweep bypass | 4 | 2.576184 | 2.933423 | +13.87% |
| Intrinsic tile certificate + disjoint bypass | 1 | 3.041844 | 3.052000 | +0.33% |
| Intrinsic tile certificate + disjoint bypass | 2 | 3.792266 | 3.803393 | +0.29% |
| Intrinsic tile certificate + disjoint bypass | 4 | 2.485092 | 2.660753 | +7.07% |

The certificate candidate improves over guarded Stream, but loses contemporary Current in all three recorded means. Its11µs gap in one batch is not a qualified gain, a full-frame cost, or an estimate of the work remaining to ship. CPU tile mask construction and upload occur before the timed execution loop in `ray_transport_gpu_6dd_continuation.py:1084-1178`; these are prepared-mask execution economics. Camera/field edits would require validity checks or rebuilding that data. A connected cold/moving result has to account for those costs.

Raw records inspected (not freshly regenerated): `/private/tmp/earthcall-6dh-candidate1/independent.json` and `/private/tmp/earthcall-6dh-candidate2-test/independent.json`, plus their `source.json` and actual `probe.cpp` bytes. Current generators match executed C++ SHA256 `339bbaadd222a1c42f0f6cae0a1378c6175d3e6a5dbbff089334496d0d1775af` and `66e7ae67ea353855cbb2cb2aa1402f14a545bc2425ba0b3c050020142ebf128b`. This identity check is not an independent rerun of every assertion or an offline reconstruction of raw AST node values.

## Source corrections still needed after §6dh

1. **The routed prototype is still a sibling approximation.** `ray_transport_gpu_6dg_specialization.cpp:878-1030` defines its own Reference, with8 coarse-bin midpoint acceptance and immediate bin integration. Alpha/Beta/fallback use the same pattern. There is no original root isolator or established schedule builder in that file. Updating noise/optical constants does not restore those semantics. Its0.758338ms vs0.760275ms warm result is against that sibling, not Current; the cold record at the same repeat count is1.527937ms (about+101% vs sibling Reference). Do not combine its clock with the real-harness accuracy result to close Gate1.
2. **“Bit-exact” is false for that routed record.** Lines1468-1475 count differences only above`1e-4`; recorded max RGB difference is`2.3283064365386963e-10`, nonzero. Report tolerance agreement against the sibling reference, not100% bit identity or current-accuracy admission.
3. **KC3 remains partial.** `field1Analyses` and`field2Analyses` are populated but never read in the classifier paths (`ray_transport_gpu_6dg_ontomath_genericity.cpp:373-435,529-540,674-696`). Field1 still uses`CurtainSpec/evalWave/cur.curv`; Field2 uses a bespoke shell intersection. Recursive wave-term extraction is useful progress, but a disconnected analyzer cannot certify its consumer. Reported shell overlap summaries also conflict between the aggregate §6dh record and the underlying genericity JSON; reconcile by executable provenance rather than choosing the favorable number.
4. **Routing is not GPU-only/current-count indirect dispatch.** `runColdRoutedBatch` takes host counts and uses direct dispatch; no indirect dispatch call exists. The kernels' tail check is`tileSlot>=2304`, rather than the actual family queue length. Up to3 rounded tail tile slots can read beyond initialized queue entries. Stable zero-filled queues/pinned-camera agreement do not establish moving-camera validity.
5. **Additional cold-repeat issue from current source:** `runColdRoutedBatch:1375-1418` resets append counters only once, before the repeat loop, while`classifyTilesMain:315-323` atomic-appends on every repeat. Counts/queue offsets therefore accumulate across repeats; queue writes can outgrow capacity. This is a source finding, not a newly observed native failure. Require per-classification reset and fresh GPU-produced queue lengths, exact family-tail guards, and changing-camera/repeated-classification controls before treating cold routing clocks as valid dynamic evidence.
6. **64 threads do not establish100% occupancy.** The source was already64-thread packed in the reviewed prototype. Actual execution width/resource evidence is required for hardware attributions. Prism's M5 dossier correctly rejects the unmeasured SIMD-16 diagnosis.
7. **Frame budget correction:**30FPS allows about33.33ms/frame;16.67ms is60FPS. The §6dh audit mixes those targets. Milliseconds for32,784 selected queries cannot be relabeled full-frame transport cost.

## Current status and next sequence

| Gate | Reviewed status | Reason |
|---|---|---|
| 1 — profitable connected native execution | **OPEN** | Real-harness candidates still lose Current; routed prototype uses different semantics and omits/invalidates parts of dynamic routing economics. |
| 2 — generic OntoMath admission + full engine integration | **OPEN** | Analyzer outputs do not drive both field classifiers; residual bins/GPU routing/lifetimes and real viewport composition remain incomplete; no campaign src changes landed since§6dc. |
| 3 — inhabited performance/Person acceptance | **OPEN** | No moving full-frame p95 or30FPS acceptance established by these probe records. |

The next corrective phase should preserve the two proven-useful real-harness changes while reconnecting the **same admitted root/transport semantics** to separate residual pipelines. Make a shared OntoMath analyzer contract actually drive both field families (unsupported input refuses), make dynamic routing counters/reset/tails safe, retain useful active-bin/residual structure with explicit invalidation, and include classification/rebuild/upload/fallback costs. Record actual pipeline/device facts and gather appropriate resource evidence before naming a hardware cause. Then require two clean paired native series against contemporary Current, followed by full-frame and inhabited evaluation if profitable.

Do not restart rejected Perlin/beam variants unchanged, claim endpoint reuse for two interior Gauss nodes, infer register pressure from scalar declarations, or promote because a static prepared subset is nearly tied. Ordinary native timestamps remain disabled under the existing Person-reported flicker boundary. This catch-up launches no GPU workload, claims no lane, changes no renderer/save/default/app, and leaves no new Person-visible acceptance item; existing acceptance remains open.
