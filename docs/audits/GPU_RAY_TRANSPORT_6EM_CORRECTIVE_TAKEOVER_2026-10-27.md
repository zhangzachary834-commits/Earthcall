# §6em — Sixth-One Sun takes the native lane: positive fallback, poisoned clean preflight, and bounded routed gain

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-27 PDT. Source reviewed: Sparkly `8bc339a85`; engine source tree `3603add4018961529a91bc9592d76f9a553500b1` matches the existing build checkout. Hardware observed through `system_profiler`: Apple M5, Metal supported.

Zach brought [the Firmament's §6el counsel](FIRMAMENT_COUNSEL_ON_1B94DE7B1_AND_6EK_MULTI_SCALE_BREAKTHROUGH_AND_6EL_QUEST_2026-10-27.md) and reported Sparkly's remaining weekly energy at 1%. Sparkly had already landed [§6el](GPU_RAY_TRANSPORT_6EL_CLEAN_MULTI_SCALE_ABLATION_2026-10-27.md) and explicitly released the GPU lane. I continued the corrective office Zach assigned this session, under current-or-better authored accuracy and the Visibility/Density validity requirements. I claimed the native lane in Intercom before this run and release it below.

## Completed native validation

The generated GPU shader literals in the new clean harness are byte-identical to §6el. Host verification now poisons output before every preflight execution, rejects nonfinite/unpublished values, records actual rows at K=1, and retains per-scale checks. Without poisoning, a missing candidate dispatch can inherit the preceding method's correct output. The targeted missing-publication negative deliberately skips Mode5: the new preflight rejects at `nonfinite/unpublished clean preflight: mode=5 ray=0`.

The positive late-refusal witness uses the existing mutation and a bounded original-pose, repeat1 case over all32,784 rays. The intended refusal set is independently derived from classifier S2 tile membership, **not** from the candidate refusal flags; this does not independently re-prove classifier geometry. Every ray's flag must agree with that intended set. Refused headers must contain zero subpanels, and every word in their allocated subpanel region must remain poisoned.

- Positive: exactly10,068 refused S2 rays,12,889 completed S1 rays; zero producer-ownership errors, final-publication errors and output differences. Refused results match Current; non-refused results match bisection; no partial refused schedule writes.
- Negative partial publication: an illegal output write before member1 refusal is caught by `routed exactly-once final output publication failure: 10068 errors` even though Current subsequently owns final output.
- Both native negatives terminate with subprocess return code **−6** and their intended error text. Do not relabel that as observed exit250.
- This is one full-population positive case and two targeted negatives, **not** a fresh30-case qualification rerun. Sparkly's broader historical report retains its original scope.

All10 normal clean runs pass poisoned, finite, bitwise own-method parity at every actual invocation: K=1/2/4/8/16 under active and blocking host polling. AtK1, all six modes' actual32,784-row outputs pass pooledRGB/T mean/p99/max at both independent reference resolutions against fresh Current. Candidate modes1–5 are exactly identical across both wait profiles. RGB max1.210615e−4 and T max1.760469e−4 (~18.5×/19.7× below Current); zero rays exceed Current's max. This is the authored projected-model reference at one pose/clock, not a universal execution certificate.

## Clean complete cost: routed unfused is the useful control

Each normal run contains two independently ordered series,16 observations per mode pooled,6 modes:960 fresh observations total. Costs below include host command encoding and submission/completion, exclude cold compilation/allocation/readbacks, and use proper medians. They are **native scratch costs over replicated sparse rays**, not viewport frames. Active polling consumes a host thread and is not a free production setting.

| K / rays | Active Current | Active routed2 | Active indirect5 | Blocking Current | Blocking routed2 | Blocking indirect5 |
|---|---:|---:|---:|---:|---:|---:|
|1 /32,784 |2.21356 |2.16165 |2.28058 |2.55294 |2.59021 |2.59640 |
|2 /65,568 |2.78987 |2.34429 |2.56606 |3.80360 |3.85740 |3.84804 |
|4 /131,136 |5.23542 |4.27100 |4.37017 |5.06854 |5.10304 |5.12394 |
|8 /262,272 |10.30990 |8.03465 |8.55406 |10.10860 |7.63504 |8.89642 |
|16 /524,544 |22.49425 |18.25579 |18.61104 |20.19300 |17.58096 |17.61883 |

All values milliseconds. AtK16 routed2 saves18.84% active and12.94% blocking in pooled complete cost. Both independent series favor routed2 over Current: active ratios0.7968/0.8006; blocking0.9291/0.8230. Indirect5 is1.95% slower than routed2 in pooled active cost and essentially tied (+0.22%) blocking, with within-series ordering changing there. The simpler routed-unfused control therefore earns priority as the bridge candidate. There is no demonstrated need to add terminal fusion or compact packets to pursue the current checkpoint.

Small workloads can tie or lose under blocking polling. Do not turn a fitted crossover into automatic runtime admission. The data establish bounded native synthetic profitability, not a generic profitable connected renderer or shipped FPS. I also replayed all960 historical §6el observations; its proper medians are consistent with the stored raw clocks. Fresh timings differ materially from historical absolute values, reinforcing the need for matched comparisons and scope.

The Firmament was correct about §6ek diagnostic work. No measured2.5–4ms diagnostic tax follows by subtracting separately compiled/cooled benchmark versions. Old Current also had diagnostic atomics; removing diagnostics can change compiler allocation as well as work. API clear bytes do not measure physical DRAM traffic. Similarly, §6el's extrapolated10–12FPS does **not** establish a frame-rate ceiling or failure to reach30FPS. No full-frame/FPS extrapolation is admitted by this pass.

## Corrections and preserved boundaries

- Repaired a dependency break: §6el expanded the diagnostic JSON writer, so §6ek's exact old output-hook match no longer generated. The historical diagnostic generator now replaces the writer's bounded return block; the160-observation CPU replay passes again. No timing rerun of the diagnostic loser.
- Route/packet census values in §6el's writer are hardcoded metadata, not freshly derived observations. This native positive witness verifies actual route counters906/707/691 and indirect args227/177/173; it does not convert every historical metadata literal into measured fact.
- K>1 here is **exact sparse-ray replication**, without §6ek's direction jitter. The repeated mathematical population is known, and native per-invocation parity is checked. It is still not a dense recorded camera frame; memory locality, traversal distribution and invocation demand can differ.
- No renderer `src`, authored save, app/default, lighting, resolution or visible world changed. The fixed Northern Veil fixture hash and ray identities were checked around every execution. No new Person-visible acceptance is claimed.

## Distance to shipping and the next bounded bridge

The safety and synthetic-cost evidence is substantially stronger. Three shipping gates remain OPEN:

1. **Profitable connected renderer:** accuracy, positive fallback and repeated clean synthetic gain now have witnesses; distinct native ray demand, full frame cost and cold/dirty economics remain unverified.
2. **Generic/full-frame connection:** current compiler/source lowering must serve the production viewport/depth/composition path, with medium clocks/revisions, validity, capacities and Current fallback preserved.
3. **Inhabited result:** matched moving frames and Zach's Northern Veil experience remain open. No projected FPS substitutes.

Next owner: start from unchanged **routed-unfused Mode2**, preserve bulk and Current controls, and test a bounded denser set of **distinct recorded native rays** with correct tile topology, finite coverage and actual reference/parity checks. Use a proven memory bound and identical clips/composition semantics; do not multiply a sparse pixel ratio into an invented frame workload. Then connect the smallest qualified transport route to the real Screen path in an isolated experiment. Keep terminal/indirect/compaction as separate optional economics work, and keep validity in shared derivation rather than a renderer-private ontology. Unsupported cases return to Current. Preserve the30-case historical controls and add actual dirty/edit/clock/depth witnesses before promotion.

Sparkly can rest; no further run is required from his session. Native GPU lane **RELEASED** by Sixth-One Sun after the recorded executions.

## Portable evidence

[Machine index](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.json), [compact archive](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.zip), [native harness](../../scratch/probes/ray_transport_gpu_6em_corrective_takeover.py), [CPU replay](../../scratch/probes/ray_transport_6em_replay.py). The archive retains exact generated sources, build fingerprints, original rays, positive/negative records and logs, all960 ordered fresh clocks and actual preflight rows. Identical rows are deduplicated only after exact comparison; original compacted-report fingerprints remain. This replay validates stored evidence, not re-execution of native publication assertions.

```sh
python3 -B scratch/probes/ray_transport_6em_replay.py \
 --record docs/audits/GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.json \
 --archive docs/audits/GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.zip \
 --output /tmp/6em-replay.json
```
