# Post-§6cn handoff: Firmament reviews; Sparkly Guy executes

Zach commissioned this handoff because Codex has 8% usage remaining. His acceptance criterion remains current-or-better accuracy against authored equations with a substantial native Northern Veil improvement; an intermediate checkpoint is welcome without finishing the entire certification campaign. Faithfulness decides, not a preference for the brighter image.

This continues the [existing Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md), [§6cn evidence audit](GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22.md), and [Firmament’s latest counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CM_WIDE_QUERY_DIAGNOSIS_AND_6CN_DESIGN_2026-10-22.md). Read these bounded records before reconstructing old experiments.

## Ownership and starting state

- **Firmament:** review the mathematical/execution contract and evidence gates; challenge the candidate before promotion. Your shared-Φ proposal is the next isolated experiment. Hardware spill/cache/occupancy diagnoses remain hypotheses.
- **Sparkly Guy:** implement and run the isolated producer experiment, coordinate exclusive native timing, preserve failures, then report connected costs and remaining shipment gates in this same Intercom.
- **Codex:** hands off now; no native probe remains running and the timing lane is released. This document does not establish that either recipient has read or started work.

Ray checkout: `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`, branch `claude/ray-domain-transport-rung0`. Start at **`5636aafaa`**, whose parent **`85662cc9cffa54218688f9c67de569f43b9bde5c`** contains §6cn source/evidence. The handoff adds documentation only after that checkpoint.

Default checkout: `/Users/zacharyzhang/Documents/GitHub/Earthcall`, branch `sync-from-earthcall-main`, observed HEAD `e063e9d25`. It contains local, uncommitted evidence/Intercom relays and unrelated changes. Preserve them: no reset, stash, clean or blanket staging. Default delivery is not a merge, remote publication or proof of recipient acknowledgement. No push/PR was performed.

## What §6cn established

The compact-event numerical candidate is **rejected as a speed optimization**. It is quarantined scratch code, not production rendering. No sample count, resolution, light, output budget, authoring, save, app or default changed. There is no new shipped FPS gain.

Full-f32 chronological event storage is128 bytes/ray versus1280 padded schedule bytes in the old format (1328 including its48-byte ray header). This reduces schedule-only storage10×, not total VRAM. The compact consumer regenerates the existing panel rule and GL2 transport, including shared attenuation for overlap.

Strict exactness screens remain executable refusals: plain GPU interpolation differs1ULP at ray3; explicit FMA then changes red output at ray5. Do not call the representation lossless or relabel these as passes. The separate numerical candidate retains the original-AST1e-5 threshold. Across32,784 distinct native rays, actual GPU-decompressed panels pass original-AST checks (maximum RGB3.725e-7, T1.073e-6), and the same192 predetermined holdout rays pass both independent dense-reference resolutions (65536/32768 intervals), all six aggregate mean/p99/max gates. Only192 rays have dense reference, not all32,784. Forced budget-zero fallback is exactly native across all three configurations.

Primary four-repeat paired complete-query times on M5/Metal:

| Ring capacity | Current | Old bulk | Compact | Compact vs current |
|---|---:|---:|---:|---:|
|8192, five chunks|1.694ms|1.913ms|2.931ms|73% slower|
|32768, two chunks|1.387ms|1.675ms|2.017ms|45% slower|

Separate matched component census uses a65536-capacity allocation with **32784 actual rays in one partial chunk**. Four-repeat connected current/bulk/compact:2.182/2.607/2.915ms. Old/new producer:1.755/1.744ms, comparable. Old/new consumer plus fallback:0.901/1.199ms, compact33% slower. These component clocks are non-additive; do not subtract processes or infer measured frame shares, cache pressure or occupancy. Timings include encoding, submission and blocking completion; exclude cold preparation, CPU reference and diagnostic readbacks. GPU timestamps remain off.

There were no normal fallback rays; maximum14 unchopped intervals. Native15-entry capacity overflow remains **unexercised**, despite codec16-entry refusal tests.241 codec round trips, six malformed codec refusals, ten report-corruption refusals and32 source/raw artifacts replay. A host diagnostic readback allocation bug was repaired; its failed source/log remains archived. Earlier assertion aborts are not GPU device-loss evidence. The older §6cl device loss remains unexplained, with unknown failing repetition.

## Next isolated experiment: coarse Φ reuse, cheaper consumer

Use the old packet format and cheaper consumer first. Current producer performs three root searches at levels `-w_z`, `0`, `+w_z`; each starts eight bins with two endpoint evaluations: **48 initial evalPhi calls per hit member** before subdivision. Nine shared coarse knots could cache the underlying Φ once each. That is48→9 for this subset, not5.33× GPU or FPS. Root work is a plausible target, not a proven exclusive bottleneck.

1. Inspect the exact generated root WGSL and its wrappers. Cache/reuse only initial coarse-knot Φ evaluations across levels, leaving bisection/subdivision and all other behavior unchanged.
2. Preserve original authored operand order and exact endpoints. Retain each level’s subtraction, execution slack and monotone derivative enclosure. The current error scale depends on `abs(va)`/`abs(vb)`; real-math level independence does not establish f32 equivalence. Materializing cached values may itself change GPU rounding.
3. Keep current48-piece/96-bound/32-band/20-stack capacities, complete uncertain bands, all kinks/gaps, chronological singles/overlap and native fallback. Three merged spans do not bound raw internal storage. A zero-fallback capture cannot justify suppressing future fallback.
4. Do not bundle SoA, smaller capacities, unified traversal, merged private arrays, fallback suppression, quadrature changes or new admission. Those need separate evidence after this experiment.
5. Check strict same-device equality first. If it fails, preserve the counterexample and label the candidate numerical; require unchanged all-query original-AST checks, both holdout-reference resolutions and exact forced native fallback. Never weaken thresholds to pass.
6. Only after accuracy passes, run matched connected current/old-bulk/new-producer controls with the old consumer, same distinct rays and paired ordering. Include producer+consumer+fallback+encoding/completion. Stop on clear economic loss; do not promote an isolated component improvement. If it wins, repeat a clean series before broader cameras/clocks and full-frame integration.

A reusable mathematical derivation belongs in **OntoMath**, with Screen lowering execution. The prototype’s fixed `collectVolumeParams` offsets `v[2]..v[24]` are not generic authored-source admission. Preserve `(T,L)` transport meaning and original-evaluator fallback; do not turn a curtain fixture into renderer-owned ontology.

## Reproduction and evidence pointers

Scratch entry points: `scratch/probes/ray_transport_gpu_knot_stream.py` (strict refused), `ray_transport_gpu_knot_fma.py` (strict FMA refused), `ray_transport_gpu_knot_numerical.py` (scoped numerical accuracy passes, economics fails), `ray_transport_gpu_knot_phases.py` (matched component census), `ray_transport_knot_evidence.py` (standalone replay). The older `ray_transport_gpu_wide_stream.py` retains the old packet format and is the appropriate baseline for this producer experiment. Follow its existing producer/root wrappers rather than copying a second mathematical implementation.

From the ray checkout, replay without a GPU or external fixtures:

```sh
python3 scratch/probes/ray_transport_knot_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22.json --output /private/tmp/duo-knot-replay.json
python3 scratch/probes/ray_transport_knot_evidence.py --root .
```

Machine records: `GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22.json`, `_REPLAY.json`, `_GENERATOR_SHADER_IDENTITY.json` beside the audit. The32 archives include four reused WIDE artifacts; retain them when moving evidence. Exact compiled snapshots are authoritative. Current generator fixes a stale label, with16 shader-identity checks; an old binary may refuse the new source hash. Rebuild instead of bypassing the guard.

Existing local fixture `/private/tmp/earthcall-wide-case/manifest.json` has32784 distinct rays,192 indexed reference rays and the full capture. Work directories `/private/tmp/earthcall-knot-numerical-reference` and `/private/tmp/earthcall-knot-phases` contain generated source, executables and reports; `/private/tmp/earthcall-knot-pass` holds replay and failure controls. Reports can exceed230MB: inspect bounded summaries or the archive verifier, never ingest the entire report. Use a fresh work directory for the next experiment and coordinate timing ownership first.

Frozen Northern Veil fixture SHA256: `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. Do not edit the fixture or live data home. Build/environment and companion instructions still apply. A no-device result is an environment skip, not a pass.

## Remaining distance to shipment

1. **Profitable connected distinct-ray execution:** still fails. Coarse-knot reuse is the next hypothesis, not a claimed win.
2. **Generic OntoMath density/root/source/clock/device lifetimes and native integration:** still open, including affordable unique full viewport, depth/composition and complete frame costs. Sparse repeated-ray wins do not satisfy this.
3. **Moving inhabited p95 and Zach’s visual/feel acceptance:** still open. Automated reference checks are not Person acceptance; no live30FPS claim.

Keep this standing current both in the chat report and the [progress dashboard](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md). Campaign plan §6cn, the Ray Domain Transport Compilation task, agent compass and Derived-State Ledger already carry the experiment boundaries. Existing Person verification remains pending; this handoff creates no visible application change.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T13:17:49.724985-07:00`.
