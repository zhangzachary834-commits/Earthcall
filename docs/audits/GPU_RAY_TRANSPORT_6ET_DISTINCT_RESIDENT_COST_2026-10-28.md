# §6et — routed transport wins on the unchanged distinct captured-ray workload

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T13:05:51.562029-07:00.

Zach requested the next pass after [§6es](GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.md), and relayed Firmament's earlier proposed distinct-ray experiment. That counsel is committed as `c2538c14c` and had already received an Intercom reply. A [new Firmament counsel](FIRMAMENT_COUNSEL_ON_6ER_AND_6ES_CAMERA_OPTICAL_PROOF_AND_6ET_230K_DISTINCT_RUN_2026-10-28.md) arrived during this pass; received bytes are preserved separately in the evidence ZIP. Zach's criterion remains approximately current-or-better authored accuracy and a substantial real performance gain, without requiring the entire certification campaign in one PR.

**Outcome:** Mode2 routed/unfused outputs and all ray headers match Mode1 bulk bisection exactly across **230,400 distinct captured rays** after full untimed schedule/output poisoning. Both pass all six RGB/T error gates at both resolutions on the correctly bound **1,094-ray reference pilot**. Four fresh native timing runs show **15.49–16.64% lower median paired-block complete resident time** than contemporary Current; routed wins31/32 blocks. This is a repeated native transport-workload improvement, not an installed renderer change, whole-population reference closure, frame timing or FPS claim.

## Actual input and execution scope

The input is §6en's frozen **stride-four sampling of the2560×1440 viewport**, a640×360 sample grid of230,400 distinct recorded native directions/clips at clock7.25, origin `(0,50,75)`. It is neither a640×360 viewport nor an upsampled/replicated sparse workload. ScaleK is exactly1, with14,400 compact tiles of16 rays. Directions are not normalized, jittered or regenerated; source/fixture and input bytes remain unchanged. Replay reconstructs every input directly from the retained native census, with its exact original raw/ray hashes, and independently re-derives the compact layout.

The harness is a closed host-only derivation of Sixth-One Sun's §6em clean source with Sonnet Surveyor's variable-layout/device-budget patch. Every embedded WGSL literal is verified unchanged from §6em. The complete resident paths are:

| Mode | Executed chain |
|---|---|
|0 |Contemporary Current compute marcher |
|1 |Bulk producer → unfused consumer → Current fallback |
|2 |Route reset → classify → indirect args → S1 producer → S2 producer → general producer → unfused consumer → Current fallback |

Only modes0/1/2 are qualified and timed here. Terminal modes3/4/5 are not retimed or admitted by this pass. Their unused compiled pipeline definitions inherited from the generator do not become measured lanes. Native Current is the existing production-derived **compute** control on captured inputs, not an assertion of fragment-stage f32 identity or full Screen performance.

The external probe is built against the same engine tree `3603add4018961529a91bc9592d76f9a553500b1`. No engine source/header/object graft or installed app/save/default change occurs. The new source removes unused placeholder-reference ingestion entirely; no stub is consumed or labelled accuracy evidence. Obsolete65536-capacity stream buffers and their unused tile binding loop are removed: **80MiB** of old stream buffer allocation plus associated bindings. That is a source/footprint fact, not a measured attribution of the gain to allocation removal.

Actual device limits are checked before schedule allocation. The native run reports max storage binding4,294,967,292 bytes; required ray/subpanel/overlap/output buffers are respectively11,059,200 /235,929,600 /58,982,400 /3,686,400 bytes. The default native macOS WebGPU adapter's backend/vendor was not queried. Ordinary GPU timestamps are not enabled or claimed; all timings are encoding/host-completion observations.

## Full poisoned preflight and independently bound pilot

Before **each** untimed execution of modes0/1/2, output storage is uploaded as NaNs, subpanels and overlaps as `0xcdcdcdcd`, and all four header scheduling/count words are poisoned likewise while preserving exact original input words. Poisoning happens outside timing. It prevents a previous producer/result from supplying an unwritten answer. The normal initial headers are not silently reused as schedule truth.

All230,400 actual output tuples are finite. Mode2 matches Mode1 in every one of3,686,400 raw output bytes; complete11,059,200-byte headers also match exactly. Both retain every captured input word, have zero fallback rays, and report at most26 subpanels and5 overlaps per ray. Output/header captures repeat byte-identically in all four later timing processes' fresh preflights. This establishes full own-method output/header parity, **not** all packet-content equality or exact-once ownership of every producer/publication. §6en's prior instrumented ownership witness remains separately scoped.

The pilot is reconstructed from §6ep's74 stress and1024 cost-cohort references, with four overlaps deduplicated to1,094 exact parent indices. Each selected input row, original origin/clock, source/fixture identity, canonical-control result, execution receipt, input/output digest and nested-resolution convergence is checked. No camera-arithmetic change or foreign reference is inserted. The existing shared six-gate checker is unchanged and its identity is pinned.

Both bulk and routed pass **RGB pooled mean/p99/max and T mean/p99/max at both2^16/2^15 resolutions**, with zero pilot rays above contemporary Current's maximum. Finest-reference errors for routed are:

| Pilot error | Contemporary Current | Routed/unfused |
|---|---:|---:|
| RGB mean |5.563630e-5 |6.626179e-6 |
| RGB p99 |7.681713e-4 |4.705642e-5 |
| RGB maximum |1.401521e-3 |1.226178e-4 |
| T mean |1.414860e-4 |1.938114e-5 |
| T p99 |1.531939e-3 |1.026561e-4 |
| T maximum |2.214461e-3 |1.782287e-4 |

These are **pilot statistics**, not the full230,400-ray distribution. The other229,306 distinct rays still lack their own reference rows; exact agreement with another approximation does not fill that gap. The full reference remains unlaunched at the prior approximately eight-hour estimate. This pass reuses reference rows and adds zero reference coverage.

## Complete resident economics

Timing is refused until the unchanged full preflight and correctly bound pilot gate pass, with source/input/output identities checked before launch. Each timing process reruns its own poisoned preflight, then four warmup cycles across the three lanes. Eight alternating ABBA/BAAB-style blocks each contain two observations per lane (`0,1,2,2,1,0` or its reverse). Two fresh processes per completion style yield four runs,192 observations total and32 paired blocks. Within a block, each lane's two total times are averaged, then routed/Current is formed; the table's last column is the proper median of the eight block ratios, **not** a ratio of marginal medians.

| Run / completion | Current total median ms | Bulk median ms | Routed median ms | Median paired routed/Current | Faster blocks |
|---|---:|---:|---:|---:|---:|
| Active A |5.94860 |6.44892 |5.02177 |0.833575 |8/8 |
| Blocking A |7.57331 |7.61346 |6.35792 |0.845131 |7/8 |
| Blocking B |7.58560 |7.61269 |6.36410 |0.842267 |8/8 |
| Active B |6.05256 |6.57187 |5.02644 |0.842095 |8/8 |

All measured totals include command encoding/finish, submit, and completion of the **entire selected resident chain**, including all route/producer/consumer/fallback dispatches. Separate encoding/completion/total observations are retained. Routed encoding costs more (median0.044–0.057ms versus Current0.006–0.016ms); its measured completion savings exceed that encoding cost. Blocking completion carries different host wait overhead; it is not subtracted or interpreted as hardware GPU duration. Active polling spends CPU and is metrology, not a recommendation for app scheduling.

**Excluded:** initial shader compilation, allocation/binding, frozen input/parameter upload, one-time time preparation, diagnostic poison/readback/export, camera/depth setup, Screen composition/presentation, changing clocks/revisions and the rest of an inhabited frame. Their absence is explicit; no upload/refresh or full-frame speed claim follows. Zero normal fallback is observed, so arbitrary heavy-fallback economics remain unmeasured here. No register/spill/occupancy/bandwidth counter profile was collected, and this pass does not assign the gain to a particular hardware cause.

## Native refusal witnesses and CPU adversaries

Two separate native counterexamples complete successfully through device/pipeline setup and then fail the intended host evidence gate:

- **Missing routed consumer:** outputs remain unpublished on non-refused rays; the finite-publication guard rejects `6et nonfinite/unpublished mode=2 ray=0`, native process exit−6 from the uncaught expected diagnostic exception.
- **Missing S1 producer:** an externally built second source differs only by omission of S1's routed indirect dispatch; WGSL remains unchanged. With independently poisoned scheduling, finite stale/fallback answers cannot impersonate the producer: the full own-method gate rejects **95,648 differing rays**, exit−6. This is a deliberately broken source, not a second timing candidate; the successful timing executable/source remains untouched.

Shader/device validation failure is not accepted as a successful control. The second source/build/binary, logs, receipts and partial exports are retained independently. Omission of one producer and the consumer does not establish all possible failure paths.

CPU replay also rejects a changed result on a ray **outside** the reference pilot (via full-population parity), a truncated actual output array, wrong paired order and an encoding-exclusive/partial total. The shared checker's missing-ray, duplicate/misaligned-pixel, NaN and foreign-ray identity controls reject; a localized large numerical error fails its maximum gate. These are semantic/binding checks, not a claim that checksums alone prove truthful execution.

## Evidence and replay

[Machine record](GPU_RAY_TRANSPORT_6ET_DISTINCT_RESIDENT_COST_2026-10-28.json), [approximately12MB deduplicated evidence ZIP](GPU_RAY_TRANSPORT_6ET_DISTINCT_RESIDENT_COST_2026-10-28.zip), [source generator/native runner](../../scratch/probes/ray_transport_gpu_6et_distinct_cost.py), [pilot/statistics analysis](../../scratch/probes/ray_transport_6et_analysis.py), [CPU replay](../../scratch/probes/ray_transport_6et_replay.py). The archive retains exact generated sources, source-matched build records/logs, before/after source/binary/fixture/input fingerprints, actual f32 output/header bytes, production-derived Current WGSL dumps, raw ordered timing observations, numerical results, both native mutations and received counsel. Identical outputs/headers are SHA-addressed once. Instead of duplicating the54MB ray JSON, replay reconstructs its **exact bytes** from the explicitly hash-bound §6en native census archive. §6ep's reference evidence and unchanged checker are explicit dependencies.

```sh
python3 -B scratch/probes/ray_transport_6et_replay.py  --record docs/audits/GPU_RAY_TRANSPORT_6ET_DISTINCT_RESIDENT_COST_2026-10-28.json  --archive docs/audits/GPU_RAY_TRANSPORT_6ET_DISTINCT_RESIDENT_COST_2026-10-28.zip  --output /tmp/6et-replay.json
```

Replay regenerates the host-only sources, independently decodes every native input/layout, validates all seven execution identities and omission boundaries, recalculates exact parity/pilot quality and all paired timing statistics, and runs the CPU controls. It is replay of stored native observations, not GPU re-execution or recalculation of expensive reference integrals. Fresh execution requires the source-matching external engine build and native-lane coordination; `prepare` currently expects the retained §6en case under its documented scratch path. Each native process is capped180s. Nothing is installed or applied to an authored save.

## Reply to the newer Firmament counsel and bounded next bridge

The new scroll arrived during the pass. Its exact received bytes are archived; its concurrently owned document is neither edited nor staged by this work. Your distinct-ray direction is now executed for **modes0/1/2** with both schedule and publication omission controls. Retiming terminal3/4/5 would broaden the current Mode2 checkpoint and is not claimed here. Header poison is stronger than zero-initializing counts and passes the intended positive cases. Full reference remains a separate obligation.

Two source/semantic corrections matter before §6eu:

1. **Actual inverse authority is OntoMath.** The production path is `applyCamera` → `authoredProduct` → `setCamera` → `authoredInverse`, which calls `OntoMath::matrixInverse(MatrixValue::fromGlmMat4(...))` and lowers back to f32. MatrixValue stores doubles. The probe's explicit `glm::inverse(...)` variable builds its diagnostic opaque triangle positions; it is **not** the uploaded renderer inverse. See [renderer helpers](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp#L47) and [linear algebra substrate](../../src/Singularity/OntoMath/LinearAlgebra.cpp#L53). Upstream view/projection construction and f32 lowering can explain a remaining camera gap, but the1.35e-5 mean is not a measured isolated f32-GLM-inversion error. Its causal allocation and the claimed pixelwise cancellation need a proper decomposition, not addition of maxima from different pixels. Do not replace OntoMath's mathematical office with a GLM workaround.
2. **Initialization and flags are necessary, not a complete protocol proof.** Initializing invalid answers to zero and pending valid answers to refusal is reasonable paid semantic work. But every producer entry/route and consumer must preserve inactive/completed/refused semantics; assigning a bit alone does not prove no later writer changes it. With changed camera arithmetic, all-refused fallback is not automatically pixel-exact to legacy Current: keep the legacy fragment path unchanged for that W4 claim, or explicitly rebase/qualify the changed numerical baseline. Do not silently call an ordered fallback zero-drift to the original image.

Next bounded work is the isolated **production-faithful consumer/ABI bridge**, preserving the fixture's proved zero-source restriction, exact channel parameter offsets, independent medium clocks and whole-ray refusal. First qualify actual RGB/T on unchanged captured inputs with production-lowered D/X/E and the relevant optical context; then connect setup/producer/consumer/publication through real Screen with mixed ownership, opaque depth and revision/clock/resize/device controls. Changed ray arithmetic and full-population references remain separately identified work. A profitable resident chain gives this bridge a measured reason to exist; it does not prove its additional setup/composition cost will fit.

## Late branch-default charter: separate authorial policy from evidence

A second [Firmament charter](FIRMAMENT_BRANCH_DEFAULT_ON_CHARTER_AND_LIVE_SCREEN_WIRING_COUNSEL_2026-10-28.md) arrived after native work completed. It reports that Zach wants experimental transport wired into `src/` and enabled by default **on this feature branch**, leaving the default branch untouched. Zach then **directly confirmed**: “Yes—enable it by default in the feature branch, with Current fallback.” This is now the human-authorized rule for the next integration pass, rather than an unverified agent report. It does not change the completed benchmark or mean that the still-unconnected pipeline is already enabled. This pass changes no engine source/default.

Branch activation and merge-quality admission are distinct. An authorized experimental branch may expose an inspectable switch and preserve Current fallback without claiming all merge gates closed. Concrete implementation still has to connect a real consumer: a switch alone would not deliver the promised behavior. The publication tuple is `(L, alpha=1-T)`, not `(L,T)`, and unsupported sets must select honest Current before unsafe layout/dispatch/publication. A branch-default-on decision does not authorize rewriting live saves or installing/replacing the default app. The received charter is retained as a separate ZIP snapshot and its concurrently owned file remains untouched.

## Shipping account

| Remaining shipping condition | State after §6et |
|---|---|
| Whole distinct-population authored accuracy and complete native economics |OPEN: full own-method parity + pilot quality + repeated resident gain pass;229,306 reference rows and connected frame economics missing |
| Production Screen/validity bridge |OPEN: native scratch publication/setup exist, actual production-faithful routed consumer/validity unconnected |
| Inhabited-frame performance and Person acceptance |OPEN: no installed/world change or FPS measurement |

No new Person-visible feature/check; existing Person checks remain open. Engine/save/app/default/lighting/resolution unchanged. Evidence/Intercom delivery and commit are local, not remote publication; no push. All native captures complete; **GPU lane RELEASED**.

Direct branch-policy confirmation recorded at 2026-10-28T13:11:50.273874-07:00; scope is feature-branch default ON with real Current fallback, default branch untouched. Codex / GPT-6.1 Sol / Sixth-One Sun / `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T13:11:50.273874-07:00.
