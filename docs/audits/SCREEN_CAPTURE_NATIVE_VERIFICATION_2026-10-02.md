# Native screenshot and recording verification

**Commission:** Zach asked, “DO A LOOP WHERE U WORK ON EARTHCALLS SCREEN CAPTURE/SCREENSHOT/SCRENRECORDING MODALITY AND VERIFY IT WORKS FOR ME.” The intended result is trustworthy visible capture, not a successful-looking control acknowledgement.

**Attribution:** Codex · GPT-6 · session `01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c` · 2026-10-02 12:45 PDT. Existing recorder work is attributed to Gemini Spark and Antigravity in [the task](../Agenda/Tasks/Specific%20Tasks/Channels%20and%20Language/Screen_Recorder_in_Singularity/Screen_Recorder_in_Singularity.md). This pass supplies failure corrections and independent native witnesses.

## Result and scope

The focused suite passed **3/3**, repeated three times in a native Mac desktop session: `screen_recorder_test`, `stream_channel_test`, and `webgpu_screen_recorder_test` (nine successful executions). The production `earthcall_webgpu` target built successfully. The separate full-engine probe passed with **eight PNG recording frames**, an actual **2560×1440 chess screenshot**, and an empty recorder error.

Zach inspected an earlier eight-frame witness from the same loop and confirmed **“THATS THE ACTUAL CHESS ZONE”** and **“THE BUTTONS ARE THE RIGHT COLORS NOT RED GLITCH.”** That is a Person witness for the pictured chess world and promotion-button colors. It does not establish desktop/window capture, MP4 playback, cursor ergonomics, or every recording control.

The retained screenshot is [here](../../scratch/capture-verification/2026-10-02-01a0fe15/screenshot.png). Native recording frames and focused logs live beside it. These generated files are ignored by Git and retrieval; they are available locally, not committed artifacts.

## Defects corrected

- Failed viewport GPU readback used to save a deterministic gradient and return success. Production now refuses, exposes an error, and creates no image. Headless tests supply their own pixels explicitly.
- Same-second recording starts reused one session directory, overwriting sequence frames or appending another header to one raw stream. Atomic directory creation now selects a unique suffix; automatic snapshots also preserve separate files.
- `window` mode called full-display capture without a target window. It now refuses explicitly pending source selection and implementation.
- Recording start now checks supported mode/format and output-directory creation. Session mode/format/output/frame-rate changes require stopping; fixed-size streams refuse resize instead of corrupting their framing.
- Viewport fallback names its actual source and retains a diagnostic. Snapshot capture dimensions are exposed, and PPM selection uses the destination extension rather than a substring anywhere in its path.
- Cursor recording previously painted a pointer at the center whether or not the pointer was there. The engine now supplies the interaction channel's actual pointer coordinates, scaled from window points to framebuffer pixels. No overlay is invented when coordinates are missing. Live cursor acceptance remains a Person check.
- Dead process pipes could terminate Earthcall with SIGPIPE, including during `pclose`'s buffered flush. Descriptor-level suppression on Darwin and scoped signal handling protect writes and closure. Finalization returns failure for nonzero process exit.
- MP4 start refuses a missing ffmpeg executable; command output paths are shell-quoted, odd dimensions are padded for yuv420p, and stderr is retained as `encoder.log`. Actual successful MP4 encoding/playback remains unverified.
- WebGPU readback refuses open-pass/oversized requests before GPU validation; BGRA conversion also recognizes the sRGB format.
- MCP `earthcall_screen_record` now returns `accepted` and `capture_verified: false` for acknowledged control writes. The old `success` response overstated evidence. Callers must distinguish submission from execution/artifact verification.
- The old recorder test erased its sandbox under `saves/`, including three tracked fixtures. Its first run in this pass did that; the initially unchanged fixtures were restored byte-for-byte from HEAD, and the test now owns a unique temporary directory.

## Evidence paths

**CPU/logical path:** `tests/singularity/screen_recorder_test.cpp` exercises explicit pixel export, permission diagnostics, pause/resume, property aliases and delegation, event-triggered and serialized authored Laws, raw/session isolation, missing frames/directories, repeated snapshot names, and a deliberately dead pipe. A small write can reach a pipe buffer before a child exits, so the dead-pipe witness exceeds the buffer instead of treating that scheduling race as failure.

**Native pixel path:** `tests/singularity/webgpu_screen_recorder_test.cpp` creates a real GLFW/CAMetalLayer surface and uses production `Renderer::beginFrame/endFrame`, `ScreenRecorder::checkPendingSnapshot/stepFrame`, and presentation. It decodes each saved PNG and compares every RGBA byte with GPU readback. Red at the top-left and blue at the bottom-right independently establish orientation and BGRA conversion. The framebuffer row is not 256-byte aligned, so the test exercises staging padding. It verifies three sequence frames and refuses readback after presentation and beyond surface bounds. A repeated run caught an invalid first-frame readiness assumption; the test now permits at most ten frame attempts and still requires three actual captures. The failed first-attempt log is retained as `earthcall-capture-surface-race.log`. It does not replace a Person's judgment of an inhabited scene.

**Full production engine path:** `scratch/probes/screen_recorder_engine_probe.py` compiles a small driver and links the existing production WebGPU app object files. The driver boots `Engine::init`, loads the existing `saves/worlds/chess_app.json` through `ZoneManager::loadTestObservation`, requests capture through registered PropertyPaths, and advances `Engine::tick`. The source chess save is read only; output goes into an isolated first-seed temporary root with no identity keys. No Person/First Mover authentication is forged and no MCP grant is bypassed: the driver is a local developer test of the engine's own registered channel.

Final engine result:

```text
ENGINE_CAPTURE_RESULT PASS frames=8
snapshot=.../earthcall-engine-capture-5eamb69m/capture/snapshot_20261002_194538.png
error=
```

The inspected Person-witnessed screenshot came from `earthcall-engine-capture-xr0ir23s/capture/snapshot_20261002_194016.png`; the final rerun also passed. A first empty-root capture rendered only the clear background, so the inhabited witness was repeated with the chess save rather than treating an empty frame as visual acceptance.

## Reproduction

Use the configure flags in [BUILD_AND_ENVIRONMENT](../BUILD_AND_ENVIRONMENT.md). A new test source was added, so reconfigure before building:

```sh
cmake --build build --target screen_recorder_test stream_channel_test webgpu_screen_recorder_test earthcall_webgpu -j8
ctest --test-dir build -R '^(screen_recorder_test|stream_channel_test|webgpu_screen_recorder_test)$' --output-on-failure
python3 scratch/probes/screen_recorder_engine_probe.py
node --check src/Singularity/Foreign/mcp/earthcall-mcp-server.js
git -c core.fsmonitor=false diff --check
```

GPU/surface checks must run in a real desktop session. Initial unrelated `channel_paths_test` execution stalled and was interrupted; no verdict is claimed for it. The final focused suite above completed. The configured count is 261 from `ctest -N`; **the full suite was not run**. Existing compiler warnings were retained.

## Boundaries and next work

Viewport capture happens after scene submission and before the ImGui overlay. It captures authored world geometry and in-world 2D controls, including the promotion buttons, but excludes Creator Console/ImGui panels. Whole-app-window capture is a distinct source choice.

The permission diagnostic in the sandboxed CPU run reported ScreenCapture=0 and Accessibility=0. That is evidence about that executable/context, not a claim that every launch process has the same TCC grant. Native host-display capture via the existing legacy CoreGraphics implementation was not verified. Next work must verify permission and API availability on the ordinary launch, and choose/implement window source semantics; never silently widen a window request to the entire desktop.

MP4 has no successful encoding witness in this pass; ffmpeg was absent from the initial shell PATH. No dependency was installed. PNG/PPM frame sequences and raw export remain the available verified recording mechanisms. Requested-vs-delivered cadence, long recording endurance, and cursor placement need further acceptance; the eight-frame witness is not a sustained performance benchmark.

Follow-up is indexed in [the task](../Agenda/Tasks/Specific%20Tasks/Channels%20and%20Language/Screen_Recorder_in_Singularity/Screen_Recorder_in_Singularity.md) and [Person Verification List](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#screen-recorder--law-authored-snapshot--recording-controls). Existing human home/Person/identity edits and concurrent Intercom edits were left alone. No inhabited beings were injected or saved by this pass.
