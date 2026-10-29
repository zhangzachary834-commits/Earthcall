# §6fe Rung 0 — per-lane work census: the production producers have no long tail, so the GPU-Runs continuation queue (Pass B) is rejected on this workload

Answers Sixth-One Sun's relayed quest (Zach's instruction: implement the next authorable-GPU-Runs slice following [AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md) §§20–22; task addendum in [Ray_Domain_Transport_Compilation.md](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md)). Origins: Zach's GPU-Runs architecture and its Rung 0 → Rung 1 ladder (measure before queue); Sixth-One Sun's brief (PASS A diagnostics, PASS B only if justified, "if measurements reject, report why and the best bounded next target"); the Firmament's §6fd counsel for the next target. I (Sonnet 5.5) built the instrument and drew the decision.

**Verdict.** On the frozen Northern Veil population, with the production 1024-unit allowance, the producers' per-ray work is narrow and far below the ceiling. A resumable continuation queue has at most ≈0.3–0.7% of modeled lockstep work to reclaim before paying for ≈1.4 KB/ray of saved state. **Pass B was not built.** This is a measured rejection of one class on one authored field, not a refutation of Zach's architecture (see "What this does not show").

## What was built (Pass A only; default OFF; not in any ordinary or timing path)
`KernelRoutedTransport::enableWorkTrace` / `observeWork` (`VolumeRoutedTransport.hpp/.inc`; renderer setter `setRoutedWorkTraceForDiagnostics`). When on, the three production producers are replaced by variants **derived by string substitution from the shipped WGSL** (one extra `workTrace` binding, 8-word `Config`, and one store before each of the producers' existing `rays[id].counts =` exit statements). Each lane records its final `work` counter and `reason` bits at `[strip][S1|S2|Fallback][tileSlot*16+rayInTile]`; `0` means the lane wrote nothing (setup-refused/idle). The buffer is cleared at the start of each `encode()`. The trace is Kernel state beneath the Screen channel (named in a comment, Refusal 6 exemption), not a being.

Controls: **observer-ON publication is bit-identical to observer-OFF** (`de154bd4fd9d2c0b` pose 0, `7c14eadd2a2e79c9` pose 32; both equal the hashes recorded for the original routed lane in the §6fd audit, so the added code did not alter the baseline). Analyzer: `scratch/probes/ray_work_trace_analyze.cpp`. Harness: `scratch/probes/ray_transport_6fe_engine.{cpp,py}` modes `worktrace` and `worktrace_in` (6fb derivative; the foreground-focus gate is skipped for these two non-timing modes — a native window still opens). Clean timing is untouched: no timing mode ever enables the trace.

## Evidence (native Release Engine, 2560×1440, wgpu-native/Metal; 5–8 s per run)
Production configuration: `workLimit` 1024, coalesced, skip-empty OFF. 16 poses (every 4th of the 64-pose route) on each of two populations — the **outside** route, and an **inside** population (eye at the route's aim point among the curtains, yaw sweeping 360°, my own addition to give a harder case). [JSON](GPU_RAY_TRANSPORT_6FE_WORK_TRACE_CENSUS_2026-10-29.json).

| Population | Producer | Lanes | max work | worst-pose p99 / p99.9 | refused | SIMD-32 efficiency | modeled lockstep vs direct, Q=32 / 64 / 128 |
|---|---|---:|---:|---:|---:|---:|---|
| outside | S1 | 18.29 M | 148 | 117 / 126 | 0 | 0.934 | 0.9967 / 0.9984 / 0.9999 |
| outside | S2 | 11.39 M | 123 | 121 / 122 | 0 | 0.952 | 0.9972 / 1.0004 / 1.0000 |
| outside | Fallback | 29.30 M | 196 | 160 / 179 | 5 | 0.970 | 0.9970 / 0.9980 / 0.9979 |
| inside | S1 | 5.66 M | 135 | 124 / 129 | 0 | 0.936 | 0.9998 / 1.0005 / 1.0000 |
| inside | Fallback | 53.32 M | 208 | 198 / 202 | 1 | 0.971 | 0.9966 / 0.9998 / 1.0000 |

- **Budget exhaustion never occurred**: 0 lanes with reason bit 1 in ≈118 M traced lanes. The ceiling (1024) is ≥4.9× the largest observed work (208). The six refusals are all reason `4` (degenerate coarse interval), not unfinished work.
- Work is tightly clustered, not heavy-tailed: e.g. pose 32 Fallback p50 96 (consistent with four 24-unit empty proofs), p90 127, p99 133, max 168; S1 pose 0 p50 54, p99 93.
- "Efficiency" = Σwork / (32·Σ per-group max): 93–97% of lockstep lane-time is already useful by this counter.
- Work share (Σwork): outside S1/S2/Fallback = 18.3/22.8/59.0 %; inside Fallback = 94.8 %. Pose 32: Fallback 302.8 M of 334.8 M units.

**Model.** A 32-lane group costs max(work); with quantum Q each stage costs min(Q, max remaining) and survivors are compacted in original order into fresh groups. It is an upper bound on benefit — no state traffic, compaction, redispatch or fallback cost, and one `work` unit (24 per empty proof, 1 per subdivision/bisection node) is not one hardware cycle. A ratio of 1.0005 means the repack was slightly worse than direct. The best case anywhere is ≈0.3%.

**State-size cost (source reading, not measured).** Persisting a mid-`roots()` ray needs `knots[96]` f32, `bands[32]` vec2, `pieces[48]` vec2, a ≤20-entry vec4 stack, depth bits and loop positions ≈ 1.4 KB per pending ray, plus resumable re-entry into the nested `member`/`level`/`bin`/`isolateMonotone` loops that WGSL cannot suspend. A strip holds 122,880 rays; a full-strip queue would be ≈170 MB, over the renderer's 64 MB storage-binding cap, so capacity and overflow would be exercised constantly.

## Decision against the quest's questions
- *One expensive irregular class that could plausibly benefit?* None found. The most expensive class (Fallback) is expensive by **work quantity** (four proof charges per empty tile, §6fd), not by tail irregularity. This is the spec §20 distinction: quantity is attacked by work elimination, irregularity by re-cohorting — here only the first is present.
- *Pass B justified?* No. Not built; no queue, resume stage, overflow/negative controls or economics were run, and none are claimed. Experimental continuation stays OFF/absent; no ScreenChannel/Law mode or inert property was added, per the brief.

## What this does not show
One authored field (Northern Veil, four curtains, wave amplitudes/frequencies as authored), 32 of the poses, one device. A field with deep root subdivision (large curvature·frequency, many bands) could have a real tail; the census instrument is retained so that can be measured rather than assumed. The model counts work units, not hardware time; a divergence cost not visible in trip counts (e.g. register pressure from the large per-lane arrays — also relevant to occupancy) is not captured. I did not run the Pass B same-process economics, so "no gain" is a modeled upper bound, not a measured cost.

## Best bounded next target (work elimination, not scheduling)
The Fallback producer is ≈59% (outside) to ≈95% (inside) of all producer work units. Its p50 is ≈4×24 empty-proof charges. §6fd step 2a (skip wave-empty tiles) already measured −3.1%/−2.95% vs coalesced; the Firmament's [step 2b blueprint](FIRMAMENT_ANSWER_TO_SONNET_6FD_STEP2A_CONTRACT_AND_STEP2B_BLUEPRINT_2026-10-29.md) (single-active-member tiles → S1 with a member id, ≈45% of fallback tiles, no new arithmetic: `ecConsume` only reads `sub.info.x`) targets most of the rest. Those lanes carry ≈100–130 units each versus ≈55 for S1. Step 2b needs its own bit-identity and wider-pose parity gates (poses `20..50` have the empties; `8,16,56` are inert controls). It does not change the budget contract or any default. I did not implement it: it is a different quest and the Firmament/Sixth-One Sun threads own its sequencing.

## Reproduce
```bash
# worktree root; Release build dir is private to this session
cmake --build /private/tmp/earthcall-6fe-release-build --target earthcall_webgpu -j10
cd scratch/probes
python3 ray_transport_6fe_engine.py build --work /private/tmp/6fe-work/h
python3 ray_transport_6fe_engine.py worktrace    --work /private/tmp/6fe-work/h
python3 ray_transport_6fe_engine.py worktrace_in --work /private/tmp/6fe-work/h
clang++ -O2 -std=c++17 ray_work_trace_analyze.cpp -o /tmp/ray_work_trace_analyze
/tmp/ray_work_trace_analyze 30 7680 /private/tmp/6fe-work/h/worktrace/trace-*.bin
```
(The 11 MB-word raw traces stay in `/private/tmp`; the committed JSON holds the analysis, binary/tool hashes and the output-control hashes.)

**Evidence class:** source + native Metal diagnostic + offline model. No Person evidence, no timing, no visible change; no new Person-only check was added to the Person Verification List.

Signed: Claude Code / Sonnet 5.5 / worktree `authorable-gpu-runs-268d29` (branch `claude/authorable-gpu-runs-268d29`, base `301304eed`) / 2026-10-29 (PDT).
