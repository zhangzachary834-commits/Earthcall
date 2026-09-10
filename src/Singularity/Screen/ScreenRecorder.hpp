#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace Singularity {
namespace Screen {

// First-mover modality channel and engine for Screen Recording in the Singularity.
//
// Refusal #1 & Refusal #6 compliant:
// All state and metrics are exposed to the Law system (@screen-recorder.*).
//
// Capabilities:
// - Multi-mode capture:
//     * "viewport": in-engine framebuffer capture (100% self-contained, no OS permissions needed).
//     * "display": host OS display capture via CoreGraphics (requires Screen Recording permission).
//     * "window": host OS window capture (requires Screen Recording permission).
// - Multi-format output:
//     * "ppm_sequence": high-speed uncompressed 24-bit RGB P6 stream for real-time capture.
//     * "png_sequence": lossless compressed PNG frames via zlib.
//     * "raw": binary RGBA stream with Earthcall recording header.
//     * "snapshot": instant single-frame screenshot.
// - macOS Permissions & Accessibility awareness:
//     * Queries CGPreflightScreenCaptureAccess and AXIsProcessTrusted.
//     * Gracefully falls back to in-engine viewport recording if OS permissions denied.
//     * Informs the Person with diagnostic accessibility and privacy details.
//     * Supports optional cursor overlay tracking in recorded frames.
class ScreenRecorder : public Law {
public:
    ScreenRecorder();
    ~ScreenRecorder() override;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "screen-recorder"; }

    static void syncRegister(LawManager& laws);
    static ScreenRecorder* find(LawManager& laws);

    // Recording session control
    bool startRecording();
    bool stopRecording();
    bool pauseRecording();
    bool resumeRecording();
    bool isRecording() const { return _recording && !_paused; }

    // Step a frame in the active recording session
    bool stepFrame(int viewportW, int viewportH, const uint8_t* optionalPixels = nullptr);

    // Capture an instantaneous snapshot (PNG or PPM)
    bool captureSnapshot(const std::string& customPath = "");

    // macOS permissions and accessibility helpers
    static bool hasScreenCapturePermission();
    static bool requestScreenCapturePermission();
    static bool hasAccessibilityPermission();
    static bool requestAccessibilityPermission();

    // Frame export utilities
    static bool writePpm(const std::string& path, const uint8_t* rgba, int w, int h);
    static bool writePng(const std::string& path, const uint8_t* rgba, int w, int h);

    // Frame capture backends
    bool captureDisplayImage(std::vector<uint8_t>& outRgba, int& outW, int& outH);
    bool captureViewportImage(std::vector<uint8_t>& outRgba, int& outW, int& outH, const uint8_t* optionalPixels);

private:
    void buildProperties() override;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    bool propRecording() const { return _recording; }
    void propSetRecording(const bool& v);

    bool propPaused() const { return _paused; }
    void propSetPaused(const bool& v) { _paused = v; }

    std::string propMode() const { return _mode; }
    void propSetMode(const std::string& v) { _mode = v; }

    std::string propFormat() const { return _format; }
    void propSetFormat(const std::string& v) { _format = v; }

    std::string propOutputPath() const { return _outputPath; }
    void propSetOutputPath(const std::string& v) { _outputPath = v; }

    double propFps() const { return _fps; }
    void propSetFps(const double& v) { _fps = v > 0.0 ? v : 30.0; }

    bool propRecordCursor() const { return _recordCursor; }
    void propSetRecordCursor(const bool& v) { _recordCursor = v; }

    bool propFallbackToViewport() const { return _fallbackToViewport; }
    void propSetFallbackToViewport(const bool& v) { _fallbackToViewport = v; }

    // Triggers
    bool propStartTrigger() const { return _startTrigger; }
    void propSetStartTrigger(const bool& v);

    bool propStopTrigger() const { return _stopTrigger; }
    void propSetStopTrigger(const bool& v);

    bool propPauseTrigger() const { return _pauseTrigger; }
    void propSetPauseTrigger(const bool& v);

    bool propResumeTrigger() const { return _resumeTrigger; }
    void propSetResumeTrigger(const bool& v);

    bool propSnapshotTrigger() const { return _snapshotTrigger; }
    void propSetSnapshotTrigger(const bool& v);

    bool propRequestPermissionTrigger() const { return _requestPermissionTrigger; }
    void propSetRequestPermissionTrigger(const bool& v);

    // Telemetry & diagnostics
    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }
    double propFrameCount() const { return _frameCount; }
    double propRecordedDuration() const { return _recordedDuration; }
    double propCaptureWidth() const { return _captureWidth; }
    double propCaptureHeight() const { return _captureHeight; }
    std::string propLastSnapshotPath() const { return _lastSnapshotPath; }
    double propBytesWritten() const { return _bytesWritten; }

    // Permissions & Accessibility
    bool propHasScreenCapturePermission() const;
    bool propHasAccessibilityPermission() const;
    std::string propPermissionStatus() const;
    std::string propAccessibilityDetails() const;

    // Internal helpers
    void drawCursorOverlay(std::vector<uint8_t>& rgba, int w, int h, int cx, int cy);
    std::string ensureSessionDirectory();

    // State
    bool _enabled = true;
    bool _recording = false;
    bool _paused = false;
    std::string _mode = "viewport";       // "viewport", "display", "window"
    std::string _format = "ppm_sequence"; // "ppm_sequence", "png_sequence", "raw"
    std::string _outputPath = "saves/recordings";
    double _fps = 30.0;
    bool _recordCursor = true;
    bool _fallbackToViewport = true;

    bool _startTrigger = false;
    bool _stopTrigger = false;
    bool _pauseTrigger = false;
    bool _resumeTrigger = false;
    bool _snapshotTrigger = false;
    bool _requestPermissionTrigger = false;

    std::string _status = "idle";
    std::string _lastError;
    double _frameCount = 0.0;
    double _recordedDuration = 0.0;
    double _captureWidth = 0.0;
    double _captureHeight = 0.0;
    std::string _lastSnapshotPath;
    double _bytesWritten = 0.0;

    // Kernel timing and session state
    std::string _currentSessionDir;
    std::chrono::steady_clock::time_point _startTime;
    std::chrono::steady_clock::time_point _lastFrameTime;
    uint64_t _sessionFrameIndex = 0;
};

} // namespace Screen
} // namespace Singularity
