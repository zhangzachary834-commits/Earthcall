# Screen Recorder in the Singularity (`ScreenRecorder`) (2026-09-08)

**Status:** ✅ done and verified  
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
   - If host screen recording permission is denied, `ScreenRecorder` automatically falls back to in-engine viewport recording when `recorder.fallbackToViewport` is true, ensuring recording never fails or crashes.

## Features Implemented

1. **First-Mover Sense-Act Modality (`@screen-recorder`)**:
   - Implemented `ScreenRecorder : public Law` in `src/Singularity/Screen/ScreenRecorder.{hpp,cpp}`.
   - Identifier: `"screen-recorder"`.
   - Registered under `LawManager` in `src/Singularity/Core/EngineInit.cpp`.
   - Stepped in `src/Singularity/Core/EngineRender.cpp` at the end of every rendered frame.

2. **Capture Modes**:
   - `"viewport"`: in-engine rendered frame capture (works without OS permissions).
   - `"display"`: captures host macOS display via `CGDisplayCreateImage(CGMainDisplayID())` (with dynamic resolution via `dlsym`).
   - `"window"`: captures application window bounds.

3. **Output Formats**:
   - `"ppm_sequence"`: high-speed uncompressed 24-bit RGB P6 frame sequence for real-time capture without frame drops.
   - `"png_sequence"`: lossless compressed PNG frames via `zlib` deflate compression.
   - `"raw"`: continuous binary stream (`ECREC001` header) of RGBA frame buffers.
   - `"snapshot"`: single-frame screenshot.

4. **Cursor Overlay**:
   - `recorder.recordCursor` (default true): draws an anti-aliased visual cursor pointer onto the recorded frames.

5. **Exposed Properties & Controls (Refusal #6 compliant)**:
   - Control: `recorder.recording`, `recorder.paused`, `recorder.mode`, `recorder.format`, `recorder.outputPath`, `recorder.fps`, `recorder.recordCursor`, `recorder.fallbackToViewport`.
   - Triggers: `recorder.start`, `recorder.stop`, `recorder.pause`, `recorder.resume`, `recorder.snapshot`, `recorder.requestPermission`.
   - Telemetry: `recorder.status`, `recorder.lastError`, `recorder.frameCount`, `recorder.recordedDuration`, `recorder.captureWidth`, `recorder.captureHeight`, `recorder.lastSnapshotPath`, `recorder.bytesWritten`.
   - Diagnostics: `recorder.hasScreenCapturePermission`, `recorder.hasAccessibilityPermission`, `recorder.permissionStatus`, `recorder.accessibilityDetails`.

6. **ScreenChannel Integration**:
   - `ScreenChannel` also exposes `@screen-channel.recording`, `@screen-channel.snapshot`, `@screen-channel.hasScreenCapturePermission`, and `@screen-channel.hasAccessibilityPermission`.

7. **Renderer Readback Support**:
   - Added `virtual bool readPixels(uint8_t* outRgba, uint32_t width, uint32_t height)` to `Renderer.hpp`.
   - Implemented in `OpenGLRenderer` with vertical coordinate flip.

## Verification
- Headless test suite `tests/singularity/screen_recorder_test.cpp`:
  - Verified Case 1: Permissions & Accessibility queries and diagnostic messages.
  - Verified Case 2: In-engine PPM sequence generation and P6 header validity.
  - Verified Case 3: ZLIB PNG sequence compression and 8-byte PNG signature validity.
  - Verified Case 4: Raw stream recording with `ECREC001` binary header.
  - Verified Case 5: Pause and resume states and triggers.
  - Verified Case 6: Snapshot capture and disk file creation.
  - Verified Case 7: Display mode fallback to viewport when OS screen capture is unpermitted.
  - Verified Case 8: Property delegation between `ScreenChannel` and `ScreenRecorder`.
  - Result: 8/8 tests passed (100%).
- Verified `file_channel_test`: 13/13 tests passed (100%).
