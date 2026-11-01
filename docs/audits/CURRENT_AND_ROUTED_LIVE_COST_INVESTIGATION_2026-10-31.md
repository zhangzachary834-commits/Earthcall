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


## Post-repair Person Release run and live camera capture

Zach explicitly confirms the runtime Release-style/NDEBUG label after the repaired build, then reports walking through several similar positions/angles: routed about13–15FPS, Current about11–13FPS. Do not explain this new run as simply Debug, or claim the launcher fix restored performance. These are unpaired live ranges; no exact percent gain is inferred. Both remain below the accepted checkpoint and the cause of the earlier19–21FPSobservation versus current cost remains OPEN. Cache/flags and executableSHA256c0088eb54dc2ea980c2e4de8eb9f2057e61dcd1837a2d37ad202bbb7555a4eaf remain verified Release at head50012eb6b.

Zach opened the app, parked at a slow view and opened F3. Desktop automation cannot attach to this unbundled GLFW executable (exact path/process-name targets refused as invalid apps); no camera movement or application launch was performed by the agent. The already-running engine's existing read-only MCP connection is available and unauthenticated for mutation. Its get_state response captured Northern Veil,33objects, camera position[-14.059818267822266, 5.853045463562012, 0.8317568302154541], forward[1.6862136931194982e-07, 0.5, 0.8660253882408142], yaw89.99999237060547, pitch30.000001907348633. Receipt:`/private/tmp/ec-current-drift-20261031/live-view-api.json`. The snapshot has a wall timestamp, not scene time; runtime FOV/extent/world clock and frame timings are unavailable through this read API.33objects is a current observation, not proof that9objects were newly added since the earlier24-object witness or that objects caused the slowdown.

The existing Performance Metrics panel exposes Engine::tick,3DCPUwall time, command recording, surface-acquisition wait, queue-submit wait, ImGui/present, volume CPU preparation, compiles and any available delayed GPU samples. These distinguish useful next attribution; CPU waits are not GPU timestamps. Requested a screenshot of this panel because neither supported UI attachment nor the read-only state endpoint exposes those rows. No new render code, build, shader/GPU benchmark, save edit or performance acceptance in this capture pass. Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-31T23:55:15-07:00.


## Native macOS Game Mode priority experiment — 2026-11-01

Zach asked to defer the full multi-app investigation and try macOS Game Mode (or another priority mechanism) for Earthcall. The newer Chrome observations supersede “one New Tab always triggers it”: later one-tab and a few-tab launches stayed around 21–23 FPS; bursts of mail or ordinary New Tabs produced about 15–17 FPS. His initial 18–20 FPS reply was **after closing tabs**, not a retained-tab recovery test. A later explicit mail-tab test started at 22 FPS, dipped to about 15 while opening, and held about 18.6 with tabs retained. Exact CPU/GPU/memory cause remains open. Read-only samples and corrected labels are in `/private/tmp/ec-chrome-contention-20261101/experiment-ledger.json`; some cameras/processes differ, and foreground state is not machine-verified, so those snapshots are not a matched per-frame causal proof. Broader tab experiments are deferred, not completed.

Host inspected: Apple M5, macOS 26.6, 16 GiB memory; active Low Power Mode is off. Apple documents [Game Mode CPU/GPU priority and native full-screen activation](https://support.apple.com/en-us/105118), the [LSSupportsGameMode opt-in](https://developer.apple.com/videos/play/wwdc2025/209/), and a [macOS 26 known issue: direct Terminal-spawned binaries do not activate Game Mode; launch the app through macOS instead](https://developer.apple.com/documentation/macos-release-notes/macos-26-release-notes). Earthcall's current source launcher directly runs the executable; its normal packaged plist is graphics-design and contains neither Game Mode key.

Prepared `/private/tmp/ec-gamemode-20261101/Earthcall Game Mode Test.app` using the existing feature Release executable (source binary SHA256 `dbbbb02d143d4037473e6aa2cca7f656369320a6a67a7e78c6d1a82043582759`, observed checkout HEAD `50012eb6b`). The temporary bundle has a distinct bundle ID, both Game Mode opt-in keys, and the OS games category **only in this test bundle**. Main executable is copied into Contents/MacOS, not a wrapper launching a child binary. Local ad-hoc signing and strict signature verification pass. All 29 non-zerofill Mach-O sections match the source executable after signing; no renderer or shader implementation changed. Receipt: `/private/tmp/ec-gamemode-20261101/receipt.json`. The installed app, normal project packaging and live save files were not edited by the agent. When launched, this app uses the normal data-home behavior of that same executable.

**Prepared, not activated yet:** awaiting closure of the existing unbundled Earthcall instance before LaunchServices launch, native full-screen entry and visible confirmation of Game Mode through its banner/Game Overlay. No FPS improvement, active Game Mode, Person acceptance or 28–30 FPS checkpoint is claimed.

**Comparison boundary:** compare Game Mode ON/OFF at the same full-screen extent and camera; do not compare a larger full-screen framebuffer directly with the old windowed FPS. In addition, `KernelRoutedTransport::ensure` caps its full-frame publication buffer at min(64 MiB, device binding limit), i.e. at most 4,194,304 pixels. Full screen may exceed that and safely select Current. Record actual routed-set/pixel counts; do not silently raise this guard during an OS-priority test. No buffer-limit, quality, sampling or resolution change in this pass.

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-01T01:11:00.667435-08:00.


<a id="game-mode-activation-verified"></a>

### Game Mode activation verified — 2026-11-01T03:38:54.568596-08:00

Zach closed the old instance and explicitly authorized launching the prepared app. Native app control launched `/private/tmp/ec-gamemode-20261101/Earthcall Game Mode Test.app` (PID35916); the macOS full-screen button entered native full screen, and the Window menu exposed **Exit Full Screen**. The menu is now closed and the full-screen app was raised for the Person. No renderer or save edit and no new performance measurement.

The Game Overlay shortcut did not yield a readable overlay, and native Control Center/SystemUIServer inspection timed out; no visual ON-state claim is made. Instead, a bounded read-only `gamepolicyd` log query independently verifies the actual OS policy. macOS identified this exact PID/bundle as a game from its plist, granted FullScreenGrant and FrontmostGrant, then logged **“Game mode enabled.”** and **“Game mode status is now on.”** at2026-11-01T03:19:09-08:00. The filtered policy log and machine-checked [local activation receipt](/private/tmp/ec-gamemode-20261101/activation-receipt.json) preserve those facts. Activation is therefore verified; the earlier “prepared, not activated” status is superseded. This is not a claim that mode remains active when the app loses qualifying foreground/full-screen state.

**Remaining Person check:** enter Northern Veil and assess Game Mode ON/OFF at the same full-screen size and camera, recording routed-set identity because the existing64MiBpublication guard can select Current at full-screen sizes. No FPS gain, live fidelity acceptance or28–30FPScheckpoint claimed. Broader multi-app investigation remains deferred.

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-01T03:38:54.568596-08:00.
