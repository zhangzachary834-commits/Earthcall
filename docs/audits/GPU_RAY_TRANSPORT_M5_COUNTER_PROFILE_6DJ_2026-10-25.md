# §6dj — Xcode installed; measured M5 stack/cache pressure in the candidate producers

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T17:24:48.216228-07:00.

**Human commission:** Zach installed official Xcode and said “ITS OPEN NOWWWWWWW”; this continues his Prism quest commission and [the existing council directive](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Active independent checkout: `Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`, starting HEAD `75693c654`. The [§6di captures and exact source identities](GPU_RAY_TRANSPORT_PRISM_QUESTS_6DI_2026-10-25.md) supply the frozen workload; this pass profiles their actual replay rather than rebuilding a different shader. No Earthcall source/default/save/installed-app change or promotion.

## Tooling boundary is crossed

Verified `/Applications/Xcode.app`, Xcode27.1/build27A9275, active developer path `/Applications/Xcode.app/Contents/Developer`, `xcodebuild -checkFirstLaunchStatus` exit0 and `xcrun xctrace version`. Finding `metal` initially located only the shim; executing it exposed the missing component. Downloaded official `MetalToolchain` through Xcode/xcodebuild; `xcodebuild -showComponent MetalToolchain -json` reports installed build27A266a. `xcrun --no-cache metal --version` successfully executes Apple metal32023.921/metalfe32023.921.6. No phone/watch simulator runtime was intentionally requested. No license agreement was accepted by this agent.

Xcode opens all three restored §6di GPU trace documents, replays them on Apple M5/macOS26.6 and completes Full profiling in Overlapping execution mode, Medium performance state. Counter export has272 columns, including duplicate labels; the machine record preserves column order and raw rows instead of collapsing duplicates. Seven encoder rows and17 raw CSV/AX/screenshot artifacts are preserved. Compiler table statistics were captured separately. Current was profiled first; its shader statistics were revisited in a second replay.

## Observed compiler and runtime resource evidence

| Frozen pipeline role | Allocated registers / high register | Spilled bytes, Xcode display | Kernel occupancy | Occupancy manager target | L1 cache pressure influence | L1 stack residency |
|---|---:|---:|---:|---:|---:|---:|
| Current `rayMain` |252 /252|32 bytes|19.82%|30.86%|0.00%|0.61%|
| Disjoint `producerFastMain` |206 /206|1.95 KiB|15.34%|24.94%|100.00%|85.14%|
| Tile-cert `producerFastMain` |208 /208|1.95 KiB|15.35%|24.75%|100.00%|84.97%|
| Disjoint consumer `rayMain` |160 /160|0 bytes|14.29%|29.38%|0.00%|0.00%|
| Tile-cert consumer `rayMain` |160 /160|0 bytes|13.73%|29.20%|0.00%|0.00%|

Both candidate traces contain a third fallback dispatch:252 registers/32-byte spills, matching Current's compiler statistics. Its measured contribution is about0.43% in these replays, despite zero normal fallback in the §6di qualification; a dispatched guarded kernel still has launch/guard cost. The measured entry names, native pipeline labels, encoder indices and compiler-table SIMD counts are retained verbatim. Do not infer invocation or wave counts from a different profiler counter's name.

**The critical distinction:** `Register Pressure Influence` is0.00% for every exported encoder. `L1 Cache Pressure Influence` is100.00% for both producers. Therefore the measured evidence supports a stack-heavy producer and cache-pressure-driven occupancy influence in these samples; it does not support a claim that the register-pressure-influence counter is high. Current actually has MORE allocated registers than either producer, yet much smaller compiler spills and stack residency. Register count alone is an inadequate optimization objective.

Apple's [M5 occupancy guidance](https://developer.apple.com/videos/play/tech-talks/111431/) distinguishes these influence counters: nonzero L1 cache influence indicates working-set thrashing, while register influence identifies the register-cache contribution. Its [occupancy guidance](https://developer.apple.com/documentation/xcode/finding-your-metal-apps-gpu-occupancy) also warns that occupancy alone is not a performance verdict. Compiler spilled bytes, L1 stack residency, cache-pressure influence and device traffic are distinct observations, not interchangeable quantities.

The disjoint producer reports30.81% buffer-L1 misses versus1.48% Current; tile-cert producer31.38%. Exported device bytes written are33,321,856 for disjoint producer and31,953,280 for tile-cert producer versus2,624 for Current. These are profiler-counter bytes in this replay, not allocated-buffer sizes, per-ray spill traffic, or clean upload costs. Candidate consumers additionally read about6.52MB from device memory. Source inspection identifies possible scratch contributors (`stack[20]`, `pieces[48]`, `knots[96]`, `bands[32]`, `phi_knots[9]`), but no individual array has been isolated as the cause.

## Interpretation and the next bounded experiment

The prior “perhaps spilling/occupancy” hypothesis now has concrete measurements: both tested producers compile with much larger spills, show substantial stack residency and hit the L1 cache pressure influence category. **Causal attribution of the elapsed slowdown still requires an intervention.** One exported counter sample per trace does not establish run-to-run variability or a universal bottleneck. Percentages are not additive across encoders, influence categories or kernels. Per-line live-register peaks and compiled instruction/source attribution remain uncollected.

Next, preserve the faithful existing root guards, affine/apex endpoints, transport panels/shared attenuation and late whole-ray fallback while testing ONE representation change that shortens producer scratch lifetimes or removes simultaneously live copies. Use physically separate residual kernels only when they reduce the live program without changing the integrator. Reuse the §6di source/fixture identities; preserve original capacities and refusal behavior until an actual bound justifies shrinking them. Do not lower precision, remove guards, relax the Current accuracy contract, or substitute the sibling scheduler to make the resource table look better.

The discriminating check is: do compiler spills, stack working set, cache influence and complete matched native cost improve together? Include queue/certificate creation, rebuild/upload, classification, all dispatches and fallback. After correctness, run two clean paired series; profile diagnosis and clean economics are separate lanes. GPU-resident routing remains required. A low-register kernel is not automatically a win if queue traffic or extra dispatches erase its benefit.

## Evidence, reproduction and limitations

[Machine record](GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.json), [raw counter exports, native AX compiler tables and screenshots](GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.zip). To inspect: restore the §6di traces with the existing replay tool, open Current first in Xcode, enable Profile after replay, Replay, select Performance → Counters and export encoder counters using Share; Performance → Shaders exposes allocated registers/high-register/spilled-byte statistics. Repeat for disjoint/cert. Raw counter files preserve all fields even when the UI displays a selected group.

For an offline integrity check, open the zip with Python's standard `zipfile`, then compare every evidence file against `evidence_files` SHA256/byte counts in the machine record. Reparse the three CSVs and require272 columns,1/3/3encoder rows, exact header order and exact raw values. The existing §6di bundle remains the source/capture authority; its hash is pinned in this machine record. This record contains observations from current profiling; earlier qualification controls retain their §6di date and are not claimed rerun here.

Xcode's displayed replay GPU times were Current1.90ms (1.89ms on revisit), disjoint2.00ms and cert1.93ms. **These are diagnostic profiler times, not a new clean A/B result, complete-cost improvement, full-frame timing or FPS.** No source edit, new correctness test, native pixel capture or inhabited Person acceptance occurred. No new visible Earthcall behavior is expected.

**Standing:** native counter inspection for frozen Current and both candidates is executed; per-line attribution remains open. All3 shipping gates remain OPEN: profitable connected execution; generic/native/full-frame integration; inhabited native FPS/Person acceptance. Native replay lane RELEASED at close in all3 local Intercom copies. No new Person check is created by installing tools or analyzing captures; the existing Northern Veil movement/appearance acceptance remains pending.
