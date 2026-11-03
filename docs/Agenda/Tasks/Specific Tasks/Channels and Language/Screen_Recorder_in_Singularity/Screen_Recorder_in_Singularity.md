# Screen Recorder in the Singularity (`ScreenRecorder`) (2026-09-08)

**Status:** Native viewport screenshots and frame recording verified; host display, selected-window capture, and MP4 acceptance remain open (2026-10-02).
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T16:49:00-07:00  

---

## Origination & Scope
Zach requested to give Earthcall a screen recorder in the Singularity while considering all relevant permissions and accessibility issues.
Screen output and display in Earthcall are ontologically housed under `Singularity/Screen/`.

## Architecture & Permissions Analysis

On modern macOS (10.15 Catalina through 15+ Sequoia):
1. **Screen Recording Permission (TCC)**:
   - Capturing full displays or host windows requires explicit permission in `System Settings > Privacy & Security > Screen & System Audio Recording`.
   - Calling legacy APIs without permission results in blank desktop wallpaper or null buffers.
   - `ScreenRecorder` queries authorization via `CGPreflightScreenCaptureAccess()` and provides `requestScreenCapturePermission()` to prompt the user.
2. **Accessibility Permission (`AXIsProcessTrusted`)**:
   - Inspecting global window hierarchies, global cursor positions, and window titles requires accessibility permissions in `System Settings > Privacy & Security > Accessibility`.
   - `ScreenRecorder` queries `AXIsProcessTrusted()` and prompts via `AXIsProcessTrustedWithOptions()`.
3. **In-Engine Viewport Fallback**:
   - Critically, internal viewport recording (reading back Earthcall's own rendered frame buffer via `Renderer::readPixels`) **does not require macOS OS-level screen capture permissions**.
   - If host capture is unavailable, `recorder.fallbackToViewport` permits an explicit viewport fallback with diagnostics; an unavailable viewport fails rather than generating substitute pixels.

## Features Implemented

1. **First-Mover Sense-Act Modality (`@screen-recorder`)**:
   - Implemented `ScreenRecorder : public Law` in `src/Singularity/Screen/ScreenRecorder.{hpp,cpp}`.
   - Identifier: `"screen-recorder"`.
   - Registered under `LawManager` in `src/Singularity/Core/EngineInit.cpp`.
   - Stepped in `src/Singularity/Core/EngineRender.cpp` at the end of every rendered frame.

2. **Capture Modes**:
   - `"viewport"`: in-engine rendered frame capture (works without OS permissions).
   - `"display"`: captures host macOS display via `CGDisplayCreateImage(CGMainDisplayID())` (with dynamic resolution via `dlsym`).
   - `"window"`: currently refuses with a diagnostic; the old implementation captured the entire display and did not select a window.

3. **Output Formats**:
   - `"ppm_sequence"`: uncompressed 24-bit RGB P6 frame sequence, throttled to the requested capture cadence.
   - `"png_sequence"`: lossless compressed PNG frames via `zlib` deflate compression.
   - `"raw"`: continuous binary stream (`ECREC001` header) of RGBA frame buffers.
   - `"snapshot"`: single-frame screenshot.

4. **Cursor Overlay**:
   - `recorder.recordCursor` (default true): draws a simple pointer at the engine's sensed viewport coordinates, scaled to framebuffer pixels; no cursor is invented when coordinates are unavailable.

5. **Exposed Properties & Controls (Refusal #6 compliant)**:
   - Control: `recorder.recording`, `recorder.paused`, `recorder.mode`, `recorder.format`, `recorder.outputPath`, `recorder.fps`, `recorder.recordCursor`, `recorder.fallbackToViewport`.
   - Triggers: `recorder.start`, `recorder.stop`, `recorder.pause`, `recorder.resume`, `recorder.snapshot`, `recorder.requestPermission`.
   - Telemetry: `recorder.status`, `recorder.lastError`, `recorder.frameCount`, `recorder.recordedDuration`, `recorder.captureWidth`, `recorder.captureHeight`, `recorder.lastSnapshotPath`, `recorder.bytesWritten`.
   - Diagnostics: `recorder.hasScreenCapturePermission`, `recorder.hasAccessibilityPermission`, `recorder.permissionStatus`, `recorder.accessibilityDetails`.

6. **ScreenChannel Integration**:
   - `ScreenChannel` exposes `@screen-channel.recording`, `@screen-channel.snapshot`, `@screen-channel.hasScreenCapturePermission`, and `@screen-channel.hasAccessibilityPermission`.
   - Wired bidirectional property delegation: setting `@screen-channel.recording` or `@screen-channel.snapshot` dynamically triggers and reflects `@screen-recorder`'s active state.

7. **Renderer Readback Support & WebGPU Parity**:
   - Added `virtual bool readPixels(uint8_t* outRgba, uint32_t width, uint32_t height)` to `Renderer.hpp`.
   - Implemented in `OpenGLRenderer` with vertical coordinate flip.
   - Implemented in `WebGpuRenderer`: added `WGPUTextureUsage_CopySrc` to surface configurations, 256-byte aligned row staging buffer readback, asynchronous mapping polling, and BGRA-to-RGBA conversion without coordinate flip.
   - Added frame-boundary `checkPendingSnapshot()` in `EngineRender.cpp` so snapshots requested between frames execute synchronously before surface presentation.
   - Added streaming pipe (`format: "pipe"`) and direct H.264 video encoding (`format: "mp4"` via lightweight `popen` pipe to `ffmpeg`).

## Verification

### 2026-10-02 correction and native witnesses

Zach commissioned a work/fix/verify loop for screenshot and screen recording. Codex found that the old headless snapshot test passed on a generated gradient after GPU readback failed, and that the window mode called full-display capture. The historical results below therefore do not establish native capture or selected-window support.

- Removed synthetic production fallback. Missing frames and captures outside the surface lifetime refuse without creating a screenshot.
- Added unique recording directories and automatic snapshot names, preventing same-second overwrites and multiple raw headers in one session.
- Validated recording modes, formats, and output directories; mode/format/path/frame-rate changes require stopping the current recording. Fixed-size streams refuse dimension changes.
- Preserved fallback diagnostics, corrected snapshot dimensions and extension selection, and guarded WebGPU readback size and BGRA formats.
- Guarded subprocess writes **and closure** against SIGPIPE; nonzero encoder exit reports failure. MP4 refuses a missing ffmpeg dependency, quotes output paths, pads odd dimensions, and retains encoder stderr. Successful MP4 encoding is **not verified** on this Mac.
- MCP recording responses now say `accepted`, with `capture_verified: false`, because a control-write acknowledgement is not capture evidence. Consumers expecting the old `success` response must use `accepted` for submission and inspect execution/artifacts for completion.
- `screen_recorder_test`: explicit test pixels, serialized/event-triggered Laws, repeated raw sessions and screenshots, missing GPU/output-path refusal, and a dead subprocess pipe.
- `webgpu_screen_recorder_test`: real GLFW/Metal surface, property-triggered capture before presentation, decoded PNG equality against every GPU pixel, red/blue orientation and BGRA anchors, row padding, recording frames, oversized/stale capture refusal.
- `scratch/probes/screen_recorder_engine_probe.py`: links production WebGPU app objects, boots an isolated first-seed save root without identity keys, loads the existing chess save through `loadTestObservation`, and captures through `Engine::tick`. The inspected witness contains the chessboard and authored promotion buttons at 2560×1440, with eight PNG recording frames.
- Zach's Person witness: **“THATS THE ACTUAL CHESS ZONE”** and **“THE BUTTONS ARE THE RIGHT COLORS NOT RED GLITCH”** after inspecting the captured image. This verifies those visible features, not every capture mode or recording control.

Viewport capture occurs after the scene pass and before the ImGui overlay. It includes authored in-world 2D controls; it is not a whole-app/desktop screenshot. Host display permission was denied in the headless diagnostic, and native host-display capture remains unverified. Choose the window-source semantics before implementing selected-window capture; never substitute an entire desktop for a requested window.

Full evidence and commands: [native capture audit](../../../../../audits/SCREEN_CAPTURE_NATIVE_VERIFICATION_2026-10-02.md). Person follow-up: [verification list](../../../For%20Zach/Person%20Verification%20List.md#screen-recorder--law-authored-snapshot--recording-controls).

*Codex · GPT-6 · session `01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c` · 2026-10-02 12:40 PDT.*

### Historical verification (scope corrected above)
- Headless test suite `tests/singularity/screen_recorder_test.cpp`:
  - Verified Case 1: Permissions & Accessibility queries and diagnostic messages.
  - Verified Case 2: In-engine PPM sequence generation and P6 header validity.
  - Verified Case 3: ZLIB PNG sequence compression and 8-byte PNG signature validity.
  - Verified Case 4: Raw stream recording with `ECREC001` binary header.
  - Verified Case 5: Pause and resume states and triggers.
  - Verified Case 6: Snapshot capture and disk file creation.
  - Verified Case 7: Display mode fallback to viewport when OS screen capture is unpermitted.
  - Verified Case 8: Dynamic property delegation between `ScreenChannel` and `ScreenRecorder` (`recording`, `snapshot`, permissions).
  - Verified Case 9: Stream pipe execution (`format: "pipe"`) and pending snapshot latch clearing.
  - Verified Case 10: Authored Law condition evaluation (premise false -> `ConditionsFailed` and snapshot stays false; premise true -> `Applied` and `screen-recorder.snapshot` changes to true, verified on all alias forms `snapshot`, `recorder.snapshot`, `screen-recorder.snapshot`). Verified Law resetting snapshot back to false.
  - Verified Case 11: Event-triggered Law firing via `EventBus` (`"user-snapshot-requested"`) + `laws.tick()` with condition gating (misfire when premise false; fires and mutates `screen-recorder.snapshot` to true when premise true; frame-boundary `checkPendingSnapshot` saves snapshot and clears trigger).
  - Verified Case 12: Continuous recording control via Law (starts recording when premise is true, stops recording via reset Law).
  - Verified Case 13: Round-trip JSON persistence and serialization (`toJson()` / `fromJson()`), verifying that deserialized Laws evaluate conditions and execute property mutations.
  - Result: 13/13 tests passed (100%).
- Verified `tests/singularity/channel_paths_test.cpp`:
  - Added `recorderPrototype` probe to `Rendering::knownPathOptions()` under group `"Channel — Screen Recorder"`.
  - Probed all ScreenRecorder properties in Creator Console's Law Authoring Window dropdown.
  - Result: 524/524 advertised channel paths resolved (100% passed).
- Verified `earthcall_webgpu`: compiled and linked cleanly.
- Verified test suite: `webgpu_heightfield_sweep_test`, `audio_channel_test`, `audio_recorder_test`, `audio_system_test`, `file_channel_test` all passed (100%).
