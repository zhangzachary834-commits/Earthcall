# Current/routed live-cost investigation after P11

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-31T23:41:22-07:00.

Zach reported that both requested renderers had fallen from roughly19–21FPS to13–17FPS in similar live views after P1–P11, with some comparisons about14FPSfor both and the latest about13Current/17routed. He confirmed OFF counters0sets/0pixels and ON1set/3,686,400pixels. This continues the [ray-domain Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) and responds to the [P10 closeout](FIRMAMENT_REVIEW_OF_SPARKLY_P10_64_OF_64_CLEAN_SWEEP_AND_CLOSEOUT_2026-10-31.md). [Machine-readable facts](CURRENT_AND_ROUTED_LIVE_COST_INVESTIGATION_2026-10-31.json).

## Verdict and boundaries

**The actual cause of the live Current FPS drop is not established.** The toggle was functioning according to Zach's counters; this is not the earlier zero-dispatch ambiguity. A positive dispatch count does not establish the accepted-ray/fallback fraction. The latest13/17report is compatible with a routed benefit at that view while both remain slower than an earlier observation; similar views/different runs are not frozen camera/clock/power equivalence.

Found and repaired a real reproducibility bug: the standard launch helper unconditionally selected Debug unless EARTHCALL_BUILD_TYPE was supplied, even when the existing cache was Release. The pre-repair cache/flags were Debug but the executable contained the Release-style label. **Do not label Zach's slow run Debug:** he did not record the label, the binary label describes one compiled translation unit, and configuration/object/executable provenance disagreed. Preserved original executable, cache, flags and helper before rebuilding at `/private/tmp/ec-current-drift-20261031/pre-rebuild-64b2bcd9938a` (original binary SHA25664b2bcd9938a3a93e7f88074e417c46f23bb409ff9c1f014fee3bda19e6847ff).

The helper now preserves the existing CMake profile unless explicitly overridden, prints the selected profile, and retains the old Debug default for a fresh build. Eight actual-shell checks pass using a mocked cmake/app: fresh, cached Release, cached Debug, explicit Debug/Release, RelWithDebInfo, empty cache and normal .command launcher. Fixtures include a checkout path with spaces. These are launcher tests, not renderer performance tests.

Explicit Release build and then a second normal-helper build with EARTHCALL_BUILD_TYPE unset both pass. The current cache names this exact source checkout and Release; flags contain-O3-DNDEBUG; the executable contains the Release-style label and no Debug-style label. Current binary SHA256c0088eb54dc2ea980c2e4de8eb9f2057e61dcd1837a2d37ad202bbb7555a4eaf. Build receipts/logs and original executable remain under `/private/tmp/ec-current-drift-20261031`. The actual app has not been launched or retimed by this pass; a faster live result is not claimed.

## What changed in Current's production source

Compared feature HEAD6911446c3 with the earlier074f4075a checkpoint. The Current shader generator (`SdfWgsl.cpp`), core render loop, Renderer.hpp, ScreenChannel.hpp and EngineInit.cpp are byte-identical. The broader production diff is five files: developer comparison chrome, routed kernel/header/implementation, and20lines in WebGpuRenderer.cpp. The changed fragment-publication insertion is within `if(routed)`; the dedicated spec-buffer write is within the routed pipeline branch. P11 itself changes only VolumeRoutedKernels.hpp and VolumeRoutedTransport.inc. **No direct Current-equation/core-loop edit was found in this comparison.** This is source evidence, not a proof that runtime Current costs are unchanged: resource residency, shared driver/host state and changed inputs still require controlled measurement.

Live data-home and checkout Northern Veil zone.ecform are both97,635bytes and byte-hash-identical: SHA2566f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496. No save was edited. This excludes a difference in that zone file at inspection, not every runtime binding/other world object/clock.

## Why promising benchmark deltas did not establish a faster live app

Independently read the committed P8/P9/P10 JSON summaries:

| Phase | Current mean ms | Routed mean ms | Routed minus Current |
|---|---:|---:|---:|
| P8 | 87.852 | 88.268 | +0.416 |
| P9 | 91.731 | 87.849 | -3.882 |
| P10 | 105.443 | 91.982 | -13.461 |

P10 has a stronger within-run relative win while both absolute costs exceed P8. This does **not** prove code regression or thermal causality across runs, but it invalidates treating the widening delta as proof of continuous absolute acceleration. Keep paired relative economics separate from stable historical-baseline comparison. The P10 JSON lacks source/binary/object/build identities; future controlled runs need those receipts to distinguish build drift from algorithm/device effects.

The actual `positive_e2e_metal_verify.cpp` is an **offscreen volume-renderer harness**, using setCamera/beginFrameOffscreen/composeVolumes/endFrame and a per-frame completion fence. It does not run the live Engine/world/UI/window presentation loop. Camera is60degrees, near0.1/far1000,64scripted poses with fixed times7.25+i/60; the earlier full Engine witness used45degrees/far100 and the Person's actual view is not captured here. The timing includes renderPose preparation plus queue completion and busy-polls wgpuInstanceProcessEvents while waiting. It is not isolated GPU time or the user's inhabited-frame timing. These scope differences do not discard the harness's matched within-run result; they prevent promoting it into a live FPS guarantee.

## FPS display and environment

ImGui's Framerate is a reciprocal60-frame average in this vendored source, not instantaneous FPS. At13–20FPS, a new state takes roughly3–5seconds to fill the window. Corrected DeveloperToolsWindow and PerformanceMetricsWindow labels accordingly; no rendering mathematics changed. This can blur short comparisons, not explain a sustained slowdown alone.

At inspection no Earthcall/build/benchmark process was running. The Mac was on battery at92%; pmset had no recorded thermal/performance warning or CPU power status. The retrieved low-power-mode field was under AC Power, not proof of the active battery policy. These are after-the-fact observations and cannot establish conditions during the slow tests or exclude GPU/thermal/background contention.

## Next controlled diagnostic and checkpoint

1. Relaunch this verified Release executable; allow the60-frame readout to settle per mode. Capture exact camera/view/time/extent and actual dispatch/fallback identity. Do not compare “similar” views across builds as a causal baseline.
2. Bind the old and current comparison to the same real-app workload, build profile, power/load and completed timing scope; compare both per-frame baseline and routed contribution. Check fresh Current versus Current after routed activation if resident/shared-state overhead remains suspected. Source identity alone does not eliminate that possibility.
3. Only after attribution choose a source correction or revert. Do not change authored lighting/resolution/sample accuracy or benchmark acceptance to recover FPS. No kernel optimization/revert, merged branch, installation or push was performed here.
4. Zach's checkpoint remains about28–30FPSin ANY ONE of the three accepted views, with standing accuracy/Current fallback; these reports do not meet it. The larger campaign and all-view target are not new prerequisites.

Visible changes: launching preserves cached Release, prints the chosen profile, and FPS labels accurately state60-frame averaging. Person confirmation of sustained live cost remains open; build/test success does not check that box.
