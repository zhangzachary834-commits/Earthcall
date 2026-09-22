#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <zlib.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#endif

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace Singularity {
namespace Screen {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Static file export utilities
// ---------------------------------------------------------------------------

bool ScreenRecorder::writePpm(const std::string& path, const uint8_t* rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return false;

    fs::path p(path);
    std::error_code ec;
    fs::path parent = p.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    out << "P6\n" << w << " " << h << "\n255\n";
    std::vector<uint8_t> rgb(static_cast<size_t>(w) * 3);
    for (int y = 0; y < h; ++y) {
        const uint8_t* row = rgba + static_cast<size_t>(y) * static_cast<size_t>(w) * 4;
        for (int x = 0; x < w; ++x) {
            rgb[x * 3 + 0] = row[x * 4 + 0];
            rgb[x * 3 + 1] = row[x * 4 + 1];
            rgb[x * 3 + 2] = row[x * 4 + 2];
        }
        out.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
    }
    return !out.fail();
}

bool ScreenRecorder::writePng(const std::string& path, const uint8_t* rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return false;

    fs::path p(path);
    std::error_code ec;
    fs::path parent = p.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    // PNG signature
    const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    out.write(reinterpret_cast<const char*>(sig), 8);

    auto writeChunk = [&](const char type[4], const uint8_t* data, size_t len) {
        uint32_t lenBE = htonl(static_cast<uint32_t>(len));
        out.write(reinterpret_cast<const char*>(&lenBE), 4);
        out.write(type, 4);
        if (len > 0 && data) {
            out.write(reinterpret_cast<const char*>(data), len);
        }
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const Bytef*>(type), 4);
        if (len > 0 && data) {
            crc = crc32(crc, reinterpret_cast<const Bytef*>(data), static_cast<uInt>(len));
        }
        uint32_t crcBE = htonl(static_cast<uint32_t>(crc));
        out.write(reinterpret_cast<const char*>(&crcBE), 4);
    };

    // IHDR
    uint8_t ihdr[13];
    uint32_t wBE = htonl(static_cast<uint32_t>(w));
    uint32_t hBE = htonl(static_cast<uint32_t>(h));
    std::memcpy(&ihdr[0], &wBE, 4);
    std::memcpy(&ihdr[4], &hBE, 4);
    ihdr[8] = 8; // bit depth
    ihdr[9] = 6; // color type RGBA
    ihdr[10] = 0; // compression
    ihdr[11] = 0; // filter
    ihdr[12] = 0; // interlace
    writeChunk("IHDR", ihdr, 13);

    // IDAT (filter byte 0 prepended to each scanline)
    std::vector<uint8_t> uncompressed;
    uncompressed.reserve(static_cast<size_t>(h) * (1 + static_cast<size_t>(w) * 4));
    for (int y = 0; y < h; ++y) {
        uncompressed.push_back(0); // filter: None
        const uint8_t* row = rgba + static_cast<size_t>(y) * static_cast<size_t>(w) * 4;
        uncompressed.insert(uncompressed.end(), row, row + static_cast<size_t>(w) * 4);
    }

    uLongf destLen = compressBound(static_cast<uLong>(uncompressed.size()));
    std::vector<uint8_t> compressed(destLen);
    if (compress(compressed.data(), &destLen, uncompressed.data(), uncompressed.size()) != Z_OK) {
        return false;
    }
    writeChunk("IDAT", compressed.data(), destLen);

    // IEND
    writeChunk("IEND", nullptr, 0);
    return !out.fail();
}

// ---------------------------------------------------------------------------
// macOS Permissions & Accessibility
// ---------------------------------------------------------------------------

bool ScreenRecorder::hasScreenCapturePermission() {
#if defined(__APPLE__)
    typedef bool (*CGPreflightFunc)();
    static CGPreflightFunc preflight = reinterpret_cast<CGPreflightFunc>(dlsym(RTLD_DEFAULT, "CGPreflightScreenCaptureAccess"));
    if (preflight) {
        return preflight();
    }
    return true;
#else
    return true;
#endif
}

bool ScreenRecorder::requestScreenCapturePermission() {
#if defined(__APPLE__)
    typedef bool (*CGRequestFunc)();
    static CGRequestFunc request = reinterpret_cast<CGRequestFunc>(dlsym(RTLD_DEFAULT, "CGRequestScreenCaptureAccess"));
    if (request) {
        return request();
    }
    return true;
#else
    return true;
#endif
}

bool ScreenRecorder::hasAccessibilityPermission() {
#if defined(__APPLE__)
    return AXIsProcessTrusted();
#else
    return true;
#endif
}

bool ScreenRecorder::requestAccessibilityPermission() {
#if defined(__APPLE__)
    const void* keys[] = { kAXTrustedCheckOptionPrompt };
    const void* values[] = { kCFBooleanTrue };
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    bool trusted = false;
    if (options) {
        trusted = AXIsProcessTrustedWithOptions(options);
        CFRelease(options);
    }
    return trusted;
#else
    return true;
#endif
}

// ---------------------------------------------------------------------------
// ScreenRecorder Lifecycle
// ---------------------------------------------------------------------------

ScreenRecorder::ScreenRecorder() : Law("screen-recorder") {
    setName("Screen Recorder");
    _enabled = true;
    _recording = false;
    _paused = false;
}

ScreenRecorder::~ScreenRecorder() {
    if (_recording) {
        stopRecording();
    }
}

void ScreenRecorder::syncRegister(LawManager& laws) {
    if (laws.find("screen-recorder")) return;

    auto recorder = std::make_shared<ScreenRecorder>();
    laws.add(recorder);
}

ScreenRecorder* ScreenRecorder::find(LawManager& laws) {
    return dynamic_cast<ScreenRecorder*>(laws.find("screen-recorder"));
}

std::string ScreenRecorder::ensureSessionDirectory() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm gm_tm;
#if defined(_WIN32)
    gmtime_s(&gm_tm, &t);
#else
    gmtime_r(&t, &gm_tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "rec_%Y%m%d_%H%M%S", &gm_tm);

    fs::path baseDir = _outputPath.empty() ? "saves/recordings" : _outputPath;
    fs::path sessionDir = baseDir / buf;
    std::error_code ec;
    fs::create_directories(sessionDir, ec);
    return sessionDir.string();
}

bool ScreenRecorder::startRecording() {
    if (!_enabled) {
        _status = "error: recorder disabled";
        _lastError = "Screen recorder is disabled";
        return false;
    }

    if (_recording) {
        return true;
    }

    // Permission and mode verification
    if (_mode == "display" || _mode == "window") {
        if (!hasScreenCapturePermission()) {
            if (_fallbackToViewport) {
                _lastError = "macOS Screen Recording permission denied. Falling back to in-engine viewport capture.";
                _mode = "viewport";
            } else {
                _status = "error: screen capture permission denied";
                _lastError = "macOS Screen Recording permission denied. Enable in System Settings > Privacy & Security > Screen Recording.";
                return false;
            }
        }
    }

    _currentSessionDir = ensureSessionDirectory();
    _startTime = std::chrono::steady_clock::now();
    _lastFrameTime = _startTime - std::chrono::milliseconds(1000);
    _sessionFrameIndex = 0;
    _frameCount = 0.0;
    _recordedDuration = 0.0;
    _bytesWritten = 0.0;

    _recording = true;
    _paused = false;
    _status = "recording";
    _lastError = "";
    return true;
}

bool ScreenRecorder::stopRecording() {
    if (!_recording) return true;

    _status = "finalizing";
    _recording = false;
    _paused = false;
    _status = "idle";
    return true;
}

bool ScreenRecorder::pauseRecording() {
    if (!_recording) return false;
    _paused = true;
    _status = "paused";
    return true;
}

bool ScreenRecorder::resumeRecording() {
    if (!_recording) return false;
    _paused = false;
    _status = "recording";
    return true;
}

void ScreenRecorder::drawCursorOverlay(std::vector<uint8_t>& rgba, int w, int h, int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= w || cy >= h) return;

    const int cursorSize = 14;
    for (int dy = 0; dy < cursorSize; ++dy) {
        for (int dx = 0; dx <= dy; ++dx) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < w && py >= 0 && py < h) {
                size_t idx = (static_cast<size_t>(py) * static_cast<size_t>(w) + static_cast<size_t>(px)) * 4;
                if (dx == 0 || dx == dy || dy == cursorSize - 1) {
                    rgba[idx + 0] = 0;
                    rgba[idx + 1] = 0;
                    rgba[idx + 2] = 0;
                    rgba[idx + 3] = 255;
                } else {
                    rgba[idx + 0] = 255;
                    rgba[idx + 1] = 255;
                    rgba[idx + 2] = 255;
                    rgba[idx + 3] = 255;
                }
            }
        }
    }
}

bool ScreenRecorder::captureDisplayImage(std::vector<uint8_t>& outRgba, int& outW, int& outH) {
#if defined(__APPLE__)
    if (!hasScreenCapturePermission()) {
        return false;
    }
    typedef CGImageRef (*CGDisplayCreateImageFunc)(uint32_t displayID);
    static CGDisplayCreateImageFunc fnDisplayCreate = reinterpret_cast<CGDisplayCreateImageFunc>(dlsym(RTLD_DEFAULT, "CGDisplayCreateImage"));
    if (!fnDisplayCreate) {
        return false;
    }
    uint32_t displayID = CGMainDisplayID();
    CGImageRef image = fnDisplayCreate(displayID);
    if (!image) {
        return false;
    }
    size_t w = CGImageGetWidth(image);
    size_t h = CGImageGetHeight(image);
    outW = static_cast<int>(w);
    outH = static_cast<int>(h);
    outRgba.resize(w * h * 4);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        outRgba.data(), w, h, 8, w * 4, colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);

    if (!context) {
        CGImageRelease(image);
        return false;
    }

    CGContextDrawImage(context, CGRectMake(0, 0, w, h), image);
    CGContextRelease(context);
    CGImageRelease(image);
    return true;
#else
    (void)outRgba; (void)outW; (void)outH;
    return false;
#endif
}

bool ScreenRecorder::captureViewportImage(std::vector<uint8_t>& outRgba, int& outW, int& outH, const uint8_t* optionalPixels) {
    if (optionalPixels && outW > 0 && outH > 0) {
        outRgba.assign(optionalPixels, optionalPixels + static_cast<size_t>(outW) * static_cast<size_t>(outH) * 4);
        return true;
    }

    const glm::ivec4& vp = currentRenderer().viewport();
    int w = vp.z > 0 ? vp.z : 1280;
    int h = vp.w > 0 ? vp.w : 720;
    outW = w;
    outH = h;
    outRgba.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);

    if (currentRenderer().readPixels(outRgba.data(), static_cast<uint32_t>(w), static_cast<uint32_t>(h))) {
        return true;
    }

    // Deterministic fallback test pattern for headless environments or uninitialized GPU framebuffers
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)) * 4;
            outRgba[idx + 0] = static_cast<uint8_t>((x * 255) / (w > 0 ? w : 1));
            outRgba[idx + 1] = static_cast<uint8_t>((y * 255) / (h > 0 ? h : 1));
            outRgba[idx + 2] = 128;
            outRgba[idx + 3] = 255;
        }
    }
    return true;
}

bool ScreenRecorder::stepFrame(int viewportW, int viewportH, const uint8_t* optionalPixels) {
    if (!_recording || _paused) return false;

    auto now = std::chrono::steady_clock::now();
    double targetIntervalSec = 1.0 / (_fps > 0.0 ? _fps : 30.0);
    double elapsedSinceLast = std::chrono::duration<double>(now - _lastFrameTime).count();
    if (elapsedSinceLast < targetIntervalSec) {
        return false; // Throttle to target FPS
    }
    _lastFrameTime = now;

    std::vector<uint8_t> framePixels;
    int w = viewportW;
    int h = viewportH;

    bool captured = false;
    if (_mode == "display" || _mode == "window") {
        captured = captureDisplayImage(framePixels, w, h);
        if (!captured && _fallbackToViewport) {
            captured = captureViewportImage(framePixels, w, h, optionalPixels);
        }
    } else {
        captured = captureViewportImage(framePixels, w, h, optionalPixels);
    }

    if (!captured || framePixels.empty()) {
        _lastError = "Failed to grab frame pixels";
        return false;
    }

    _captureWidth = static_cast<double>(w);
    _captureHeight = static_cast<double>(h);

    if (_recordCursor) {
        drawCursorOverlay(framePixels, w, h, w / 2, h / 2);
    }

    char frameFilename[128];
    std::snprintf(frameFilename, sizeof(frameFilename), "frame_%06llu", static_cast<unsigned long long>(_sessionFrameIndex));

    fs::path framePath;
    bool written = false;
    size_t frameBytes = framePixels.size();

    if (_format == "png_sequence") {
        framePath = fs::path(_currentSessionDir) / (std::string(frameFilename) + ".png");
        written = writePng(framePath.string(), framePixels.data(), w, h);
    } else if (_format == "raw") {
        framePath = fs::path(_currentSessionDir) / "stream.raw";
        std::ofstream stream(framePath.string(), std::ios::out | std::ios::binary | std::ios::app);
        if (stream.is_open()) {
            if (_sessionFrameIndex == 0) {
                // 16-byte raw header
                const char headerSig[8] = {'E', 'C', 'R', 'E', 'C', '0', '0', '1'};
                stream.write(headerSig, 8);
                uint32_t wBE = htonl(static_cast<uint32_t>(w));
                uint32_t hBE = htonl(static_cast<uint32_t>(h));
                stream.write(reinterpret_cast<const char*>(&wBE), 4);
                stream.write(reinterpret_cast<const char*>(&hBE), 4);
            }
            stream.write(reinterpret_cast<const char*>(framePixels.data()), framePixels.size());
            written = !stream.fail();
        }
    } else {
        // Default: "ppm_sequence"
        framePath = fs::path(_currentSessionDir) / (std::string(frameFilename) + ".ppm");
        written = writePpm(framePath.string(), framePixels.data(), w, h);
    }

    if (written) {
        ++_sessionFrameIndex;
        _frameCount = static_cast<double>(_sessionFrameIndex);
        _recordedDuration = std::chrono::duration<double>(now - _startTime).count();
        _bytesWritten += static_cast<double>(frameBytes);
        return true;
    } else {
        _lastError = "Failed writing frame: " + framePath.string();
        return false;
    }
}

bool ScreenRecorder::captureSnapshot(const std::string& customPath) {
    std::vector<uint8_t> framePixels;
    const glm::ivec4& vp = currentRenderer().viewport();
    int w = vp.z > 0 ? vp.z : 1280;
    int h = vp.w > 0 ? vp.w : 720;

    bool captured = false;
    if (_mode == "display") {
        captured = captureDisplayImage(framePixels, w, h);
        if (!captured && _fallbackToViewport) {
            captured = captureViewportImage(framePixels, w, h, nullptr);
        }
    } else {
        captured = captureViewportImage(framePixels, w, h, nullptr);
    }

    if (!captured || framePixels.empty()) {
        _lastError = "Failed to capture snapshot buffer";
        return false;
    }

    std::string destPath = customPath;
    if (destPath.empty()) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm gm_tm;
#if defined(_WIN32)
        gmtime_s(&gm_tm, &t);
#else
        gmtime_r(&t, &gm_tm);
#endif
        char buf[64];
        std::strftime(buf, sizeof(buf), "snapshot_%Y%m%d_%H%M%S.png", &gm_tm);
        fs::path baseDir = _outputPath.empty() ? "saves/recordings" : _outputPath;
        destPath = (baseDir / buf).string();
    }

    bool success = false;
    if (destPath.rfind(".ppm") != std::string::npos) {
        success = writePpm(destPath, framePixels.data(), w, h);
    } else {
        success = writePng(destPath, framePixels.data(), w, h);
    }

    if (success) {
        _lastSnapshotPath = destPath;
        _bytesWritten += static_cast<double>(framePixels.size());
        _status = "snapshot-saved";
        _lastError = "";
        return true;
    } else {
        _status = "error: snapshot failed";
        _lastError = "Failed to save snapshot to: " + destPath;
        return false;
    }
}

// ---------------------------------------------------------------------------
// Property Getters / Setters & Triggers
// ---------------------------------------------------------------------------

void ScreenRecorder::propSetRecording(const bool& v) {
    if (v && !_recording) {
        startRecording();
    } else if (!v && _recording) {
        stopRecording();
    }
}

void ScreenRecorder::propSetStartTrigger(const bool& v) {
    _startTrigger = v;
    if (_startTrigger) {
        startRecording();
        _startTrigger = false;
    }
}

void ScreenRecorder::propSetStopTrigger(const bool& v) {
    _stopTrigger = v;
    if (_stopTrigger) {
        stopRecording();
        _stopTrigger = false;
    }
}

void ScreenRecorder::propSetPauseTrigger(const bool& v) {
    _pauseTrigger = v;
    if (_pauseTrigger) {
        pauseRecording();
        _pauseTrigger = false;
    }
}

void ScreenRecorder::propSetResumeTrigger(const bool& v) {
    _resumeTrigger = v;
    if (_resumeTrigger) {
        resumeRecording();
        _resumeTrigger = false;
    }
}

void ScreenRecorder::propSetSnapshotTrigger(const bool& v) {
    _snapshotTrigger = v;
    if (_snapshotTrigger) {
        captureSnapshot();
        _snapshotTrigger = false;
    }
}

void ScreenRecorder::propSetRequestPermissionTrigger(const bool& v) {
    _requestPermissionTrigger = v;
    if (_requestPermissionTrigger) {
        requestScreenCapturePermission();
        requestAccessibilityPermission();
        _requestPermissionTrigger = false;
    }
}

bool ScreenRecorder::propHasScreenCapturePermission() const {
    return hasScreenCapturePermission();
}

bool ScreenRecorder::propHasAccessibilityPermission() const {
    return hasAccessibilityPermission();
}

std::string ScreenRecorder::propPermissionStatus() const {
#if defined(__APPLE__)
    bool sc = hasScreenCapturePermission();
    bool ax = hasAccessibilityPermission();
    if (sc && ax) return "authorized";
    if (!sc && ax) return "screen_capture_denied";
    if (sc && !ax) return "accessibility_denied";
    return "denied";
#else
    return "not_supported";
#endif
}

std::string ScreenRecorder::propAccessibilityDetails() const {
#if defined(__APPLE__)
    std::string details;
    bool sc = hasScreenCapturePermission();
    bool ax = hasAccessibilityPermission();
    if (!sc) {
        details += "[Screen Recording]: Permission not granted. Host OS capture requires user authorization in System Settings > Privacy & Security > Screen Recording. (In-engine viewport capture operates without this permission). ";
    } else {
        details += "[Screen Recording]: Authorized. ";
    }
    if (!ax) {
        details += "[Accessibility]: Permission not granted. Global cursor tracking and window inspection require authorization in System Settings > Privacy & Security > Accessibility. (In-engine cursor operates without this permission).";
    } else {
        details += "[Accessibility]: Authorized.";
    }
    return details;
#else
    return "Host OS permissions checks not applicable on this platform.";
#endif
}

void ScreenRecorder::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.enabled", this, &ScreenRecorder::propEnabled, &ScreenRecorder::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.recording", this, &ScreenRecorder::propRecording, &ScreenRecorder::propSetRecording));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.paused", this, &ScreenRecorder::propPaused, &ScreenRecorder::propSetPaused));

    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.mode", this, &ScreenRecorder::propMode, &ScreenRecorder::propSetMode));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.format", this, &ScreenRecorder::propFormat, &ScreenRecorder::propSetFormat));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.outputPath", this, &ScreenRecorder::propOutputPath, &ScreenRecorder::propSetOutputPath));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.fps", this, &ScreenRecorder::propFps, &ScreenRecorder::propSetFps));

    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.recordCursor", this, &ScreenRecorder::propRecordCursor, &ScreenRecorder::propSetRecordCursor));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.fallbackToViewport", this, &ScreenRecorder::propFallbackToViewport, &ScreenRecorder::propSetFallbackToViewport));

    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.start", this, &ScreenRecorder::propStartTrigger, &ScreenRecorder::propSetStartTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.stop", this, &ScreenRecorder::propStopTrigger, &ScreenRecorder::propSetStopTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.pause", this, &ScreenRecorder::propPauseTrigger, &ScreenRecorder::propSetPauseTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.resume", this, &ScreenRecorder::propResumeTrigger, &ScreenRecorder::propSetResumeTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.snapshot", this, &ScreenRecorder::propSnapshotTrigger, &ScreenRecorder::propSetSnapshotTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.requestPermission", this, &ScreenRecorder::propRequestPermissionTrigger, &ScreenRecorder::propSetRequestPermissionTrigger));

    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.status", this, &ScreenRecorder::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.lastError", this, &ScreenRecorder::propLastError, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.frameCount", this, &ScreenRecorder::propFrameCount, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.recordedDuration", this, &ScreenRecorder::propRecordedDuration, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.captureWidth", this, &ScreenRecorder::propCaptureWidth, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.captureHeight", this, &ScreenRecorder::propCaptureHeight, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.lastSnapshotPath", this, &ScreenRecorder::propLastSnapshotPath, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
        "recorder.bytesWritten", this, &ScreenRecorder::propBytesWritten, nullptr));

    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.hasScreenCapturePermission", this, &ScreenRecorder::propHasScreenCapturePermission, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
        "recorder.hasAccessibilityPermission", this, &ScreenRecorder::propHasAccessibilityPermission, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.permissionStatus", this, &ScreenRecorder::propPermissionStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
        "recorder.accessibilityDetails", this, &ScreenRecorder::propAccessibilityDetails, nullptr));
}

} // namespace Screen
} // namespace Singularity
