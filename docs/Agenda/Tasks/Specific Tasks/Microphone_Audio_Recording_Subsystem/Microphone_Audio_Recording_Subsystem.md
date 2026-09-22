# Microphone Audio Capture & Streaming Recording Subsystem (`AudioRecorder` / `@microphone`) (2026-09-09)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark (sparkly guy)  
**Timestamp:** 2026-09-09T19:35:00-07:00  

---

## Origination & Scope
Zach requested the ability for the Singularity substrate to record microphone audio directly on macOS, connecting to whatever audio hardware the Mac is connected to (built-in microphone, USB audio interfaces, AirPods/Bluetooth headsets) as permitted by OS permissions.

In Earthcall's Sense-Act ontology, audio input is not an isolated utility function; it is an ontological hardware modality first mover (`@microphone` / `@audio-recorder`), exposing continuous acoustic sensing, reactive speech detection, and audio file recording into the universal virtual file system.

---

## Architecture & Features Implemented

1. **First-Mover Modality Law (`@microphone` / `@audio-recorder`)**:
   - Implemented `AudioRecorder : public Law` in `src/Singularity/Audio/AudioRecorder.{hpp,cpp}`.
   - Identifier: `"microphone"` (registered at boot in `src/Singularity/Core/EngineInit.cpp` and stepped in `src/Singularity/Core/EngineRender.cpp`).
   - Senses host audio input hardware in real time, streams PCM audio, calculates RMS audio level and peak volume, and writes standard 16-bit uncompressed PCM `.wav` files.

2. **macOS CoreAudio Capture & Device Enumeration**:
   - Uses `miniaudio.h`'s native CoreAudio capture pipeline (`ma_device_type_capture`).
   - Dynamic device enumeration: discovers all connected input hardware on the Mac:
     - Built-in Microphone
     - USB audio interfaces / microphones (Yeti, Shure, Rode, etc.)
     - Bluetooth headsets / AirPods
     - 3.5mm line-in
   - Dynamic switching between capture devices via `mic.selectedDevice`.

3. **macOS TCC Microphone Permissions**:
   - Dynamically inspects `AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio` via Objective-C runtime dlopen.
   - Telemetry properties:
     - `mic.hasPermission` (boolean)
     - `mic.permissionStatus` (`"authorized"`, `"denied"`, `"not_determined"`, `"restricted"`)
     - `mic.permissionDetails` (informative user guidance pointing to System Settings > Privacy & Security > Microphone)
   - Built-in simulation fallback (`mic.fallbackToSimulated`) allowing headless tests, automated pipelines, and environments without microphone authorization to run deterministically.

4. **Live Audio Ingestion & Metering**:
   - Real-time `dataCallback` receives floating-point audio frames.
   - Computes RMS audio level (`mic.inputLevel`) and peak level (`mic.peakLevel`) with smooth exponential decay.
   - Configurable gain (`mic.gain`), sample rate (`mic.sampleRate`, default 48 kHz), and channels (`mic.channels`, 1=mono, 2=stereo).

5. **Past-Tense Noun-Verbed ECA Edge Events**:
   - Follows Earthcall's core rule from `AGENTS.md` (events must be past-tense noun-verbed edges rather than continuous levels):
     - `recording-started`: published when audio capture begins.
     - `recording-stopped`: published when recording finishes and file is written.
     - `speech-detected`: published on edge transition when input level crosses `mic.speechThreshold`.
     - `silence-detected`: published on edge transition when speech drops back to silence.

6. **Canonical 16-bit PCM WAV File Exporter**:
   - Generates standard 44-byte RIFF/WAVE header (uncompressed PCM format code 1).
   - Writes timestamped recordings to `saves/recordings/mic_%Y%m%d_%H%M%S.wav` (or custom path).
   - Seamlessly integrates with universal Virtual File System (`recording://my_voice.wav`).

7. **Exposed Law Properties (Refusal #6 Compliant)**:
   - Controls: `mic.enabled`, `mic.recording`, `mic.paused`, `mic.selectedDevice`, `mic.sampleRate`, `mic.channels`, `mic.gain`, `mic.speechThreshold`, `mic.outputPath`, `mic.fallbackToSimulated`.
   - Triggers: `mic.start`, `mic.stop`, `mic.pause`, `mic.resume`, `mic.refreshDevices`, `mic.requestPermission`.
   - Telemetry: `mic.status`, `mic.lastError`, `mic.inputLevel`, `mic.peakLevel`, `mic.recordedDuration`, `mic.sampleCount`, `mic.bytesRecorded`, `mic.lastRecordingPath`, `mic.devices`, `mic.deviceCount`, `mic.hasPermission`, `mic.permissionStatus`, `mic.permissionDetails`.

---

## Verification
- Headless test suite `tests/singularity/audio_recorder_test.cpp`: **6/6 tests passed (100%)**.
  - Case 1: macOS permissions & telemetry inspection.
  - Case 2: Device enumeration & selection.
  - Case 3: Audio ingestion, real-time RMS/peak metering, and `speech-detected`/`silence-detected` ECA events.
  - Case 4: Pause, resume, and tick duration tracking.
  - Case 5: Stop recording, WAV file creation, and canonical RIFF/WAVE 44-byte header validation.
  - Case 6: Direct static WAV export to universal VFS URI (`recording://...`).
- Full engine regression test suite passed cleanly:
  1. `file_channel_test`: 13/13 passed
  2. `screen_recorder_test`: 8/8 passed
  3. `vfs_test`: 4/4 passed
  4. `stream_channel_test`: 4/4 passed
  5. `file_watcher_test`: 5/5 passed
  6. `mcp_bridge_test`: 8/8 passed
  7. `audio_recorder_test`: 6/6 passed
