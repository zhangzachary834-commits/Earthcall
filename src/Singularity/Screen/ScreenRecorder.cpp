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
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#endif

namespace Singularity {
namespace Screen {

namespace fs = std::filesystem;

// Subprocess I/O is Kernel substrate. A closed encoder pipe must refuse a
// frame, never terminate the Person's whole engine with SIGPIPE.
static bool writeProcessFrame(FILE* pipe, const std::vector<uint8_t>& pixels) {
#if defined(__APPLE__)
    // Darwin can deliver a pipe signal to another engine thread. Suppress it
    // on this descriptor, rather than changing the process's signal policy.
    if (fcntl(fileno(pipe), F_SETNOSIGPIPE, 1) == -1) return false;
#endif
#if !defined(_WIN32)
    sigset_t blocked, previous, pending;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &blocked, &previous) != 0) return false;
    sigpending(&pending);
    bool alreadyPending = sigismember(&pending, SIGPIPE);
#endif
    bool ok = std::fwrite(pixels.data(), 1, pixels.size(), pipe) == pixels.size();
    ok = std::fflush(pipe) == 0 && ok;
#if !defined(_WIN32)
    sigpending(&pending);
    if (!alreadyPending && sigismember(&pending, SIGPIPE)) {
        int signal;
        sigwait(&blocked, &signal);
    }
    pthread_sigmask(SIG_SETMASK, &previous, nullptr);
#endif
    return ok;
}

static int closeProcessPipe(FILE* pipe) {
#if defined(__APPLE__)
    fcntl(fileno(pipe), F_SETNOSIGPIPE, 1);
#endif
#if !defined(_WIN32)
    sigset_t blocked, previous, pending;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &blocked, &previous) != 0) return -1;
    sigpending(&pending);
    bool alreadyPending = sigismember(&pending, SIGPIPE);
#endif
    // pclose also flushes stdio's buffered tail, so it has the same broken-pipe
    // boundary as fwrite, even after a failed explicit fflush.
    int result = pclose(pipe);
#if !defined(_WIN32)
    sigpending(&pending);
    if (!alreadyPending && sigismember(&pending, SIGPIPE)) {
        int signal;
        sigwait(&blocked, &signal);
    }
    pthread_sigmask(SIG_SETMASK, &previous, nullptr);
#endif
    return result;
}

static std::string shellPath(const std::string& path) {
    std::string result = "'";
    for (char c : path) result += c == '\'' ? "'\\''" : std::string(1, c);
    return result + "'";
}

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

static ScreenRecorder* s_activeScreenRecorder = nullptr;

ScreenRecorder* ScreenRecorder::activeInstance() {
    return s_activeScreenRecorder;
}

ScreenRecorder::ScreenRecorder() : Law("screen-recorder") {
    setName("Screen Recorder");
    _enabled = true;
    _recording = false;
    _paused = false;
    s_activeScreenRecorder = this;
}

ScreenRecorder::~ScreenRecorder() {
    if (_recording) {
        stopRecording();
    }
    if (_pipeProcess) {
        closeProcessPipe(_pipeProcess);
        _pipeProcess = nullptr;
    }
    if (s_activeScreenRecorder == this) {
        s_activeScreenRecorder = nullptr;
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
    if (_format == "pipe") {
        return "";
    }
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
    std::error_code ec;
    fs::create_directories(baseDir, ec);
    if (ec) {
        _lastError = "Cannot create recording directory: " + ec.message();
        return "";
    }
    // A new session must never truncate an earlier session's frame_000000 or
    // append a second header to its raw stream, even within the same second.
    for (uint64_t suffix = 0; ; ++suffix) {
        fs::path sessionDir = baseDir / (std::string(buf) +
            (suffix ? "_" + std::to_string(suffix) : ""));
        if (fs::create_directory(sessionDir, ec)) return sessionDir.string();
        if (ec) {
            _lastError = "Cannot create recording session: " + ec.message();
            return "";
        }
    }
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

    _lastError.clear();
    if (_mode != "viewport" && _mode != "display") {
        _status = "error: unsupported capture mode";
        _lastError = _mode == "window"
            ? "Host window capture is not implemented; choose viewport for Earthcall or display for the host screen."
            : "Unknown capture mode: " + _mode;
        return false;
    }
    if (_format != "ppm_sequence" && _format != "png_sequence" &&
        _format != "raw" && _format != "pipe" && _format != "mp4") {
        _status = "error: unsupported recording format";
        _lastError = "Unknown recording format: " + _format;
        return false;
    }
    if (_format == "mp4" && std::system("command -v ffmpeg > /dev/null 2>&1") != 0) {
        _status = "error: video encoder unavailable";
        _lastError = "MP4 requires ffmpeg on PATH; choose png_sequence, ppm_sequence, or raw.";
        return false;
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
    if (_format != "pipe" && _currentSessionDir.empty()) {
        _status = "error: recording directory unavailable";
        return false;
    }
    _startTime = std::chrono::steady_clock::now();
    _lastFrameTime = _startTime - std::chrono::milliseconds(1000);
    _sessionFrameIndex = 0;
    _frameCount = 0.0;
    _recordedDuration = 0.0;
    _bytesWritten = 0.0;

    _recording = true;
    _paused = false;
    _status = "recording";
    return true;
}

bool ScreenRecorder::stopRecording() {
    if (!_recording) return true;

    _status = "finalizing";
    bool finalized = true;
    if (_pipeProcess) {
        finalized = closeProcessPipe(_pipeProcess) == 0;
        _pipeProcess = nullptr;
    }
    _recording = false;
    _paused = false;
    _status = finalized ? "idle" : "error: encoder failed";
    if (!finalized) _lastError = "Recording subprocess failed; output is not verified. Inspect encoder.log for MP4.";
    return finalized;
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
        _lastError = "Host screen capture permission is not granted";
        return false;
    }
    typedef CGImageRef (*CGDisplayCreateImageFunc)(uint32_t displayID);
    static CGDisplayCreateImageFunc fnDisplayCreate = reinterpret_cast<CGDisplayCreateImageFunc>(dlsym(RTLD_DEFAULT, "CGDisplayCreateImage"));
    if (!fnDisplayCreate) {
        _lastError = "Host display capture API unavailable";
        return false;
    }
    uint32_t displayID = CGMainDisplayID();
    CGImageRef image = fnDisplayCreate(displayID);
    if (!image) {
        _lastError = "Host display capture returned no image";
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
    _lastError = "Host display capture is unsupported on this platform";
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

    // Injected test pixels belong in the test caller. A failed GPU readback is
    // never a screenshot, and must not publish a synthetic image as evidence.
    outRgba.clear();
    _lastError = "Viewport readback unavailable; no rendered frame was captured";
    return false;
}

bool ScreenRecorder::stepFrame(int viewportW, int viewportH, const uint8_t* optionalPixels,
                               int cursorX, int cursorY) {
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
            if (captured) {
                _mode = "viewport";
                _lastError = "Host display capture unavailable; captured the Earthcall viewport instead.";
            }
        }
    } else {
        captured = captureViewportImage(framePixels, w, h, optionalPixels);
    }

    if (!captured || framePixels.empty()) {
        if (_lastError.empty()) _lastError = "Failed to grab frame pixels";
        return false;
    }

    if ((_format == "raw" || _format == "mp4" || _format == "pipe") &&
        _sessionFrameIndex > 0 && (_captureWidth != w || _captureHeight != h)) {
        _lastError = "Capture dimensions changed during a fixed-size stream; stop and start a new recording.";
        return false;
    }

    _captureWidth = static_cast<double>(w);
    _captureHeight = static_cast<double>(h);

    if (_recordCursor && _mode == "viewport") {
        drawCursorOverlay(framePixels, w, h, cursorX, cursorY);
    }

    char frameFilename[128];
    std::snprintf(frameFilename, sizeof(frameFilename), "frame_%06llu", static_cast<unsigned long long>(_sessionFrameIndex));

    fs::path framePath;
    bool written = false;
    size_t frameBytes = framePixels.size();

    if (_format == "png_sequence") {
        framePath = fs::path(_currentSessionDir) / (std::string(frameFilename) + ".png");
        written = writePng(framePath.string(), framePixels.data(), w, h);
    } else if (_format == "mp4") {
        if (!_pipeProcess && _sessionFrameIndex == 0) {
            fs::path videoPath = fs::path(_currentSessionDir) / "recording.mp4";
            std::string cmd = "ffmpeg -y -f rawvideo -pix_fmt rgba -s " +
                              std::to_string(w) + "x" + std::to_string(h) +
                              " -r " + std::to_string(_fps) +
                              " -i - -vf 'pad=ceil(iw/2)*2:ceil(ih/2)*2' -c:v libx264 -pix_fmt yuv420p " +
                              shellPath(videoPath.string()) + " 2>" +
                              shellPath((fs::path(_currentSessionDir) / "encoder.log").string());
            _pipeProcess = popen(cmd.c_str(), "w");
            if (!_pipeProcess) {
                _lastError = "Failed to launch ffmpeg";
                return false;
            }
        }
        if (_pipeProcess) {
            written = writeProcessFrame(_pipeProcess, framePixels);
        } else {
            framePath = fs::path(_currentSessionDir) / (std::string(frameFilename) + ".png");
            written = writePng(framePath.string(), framePixels.data(), w, h);
        }
    } else if (_format == "pipe") {
        if (!_pipeProcess && _sessionFrameIndex == 0) {
            std::string cmd = _outputPath.empty() ? "cat" : _outputPath;
            _pipeProcess = popen(cmd.c_str(), "w");
        }
        if (_pipeProcess) {
            written = writeProcessFrame(_pipeProcess, framePixels);
        }
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

bool ScreenRecorder::captureSnapshot(const std::string& customPath, int viewportW, int viewportH, const uint8_t* optionalPixels) {
    if (!_enabled || (_mode != "viewport" && _mode != "display")) {
        _status = "error: snapshot refused";
        _lastError = !_enabled ? "Screen recorder is disabled" : "Unsupported capture mode: " + _mode;
        return false;
    }
    std::vector<uint8_t> framePixels;
    int w = viewportW;
    int h = viewportH;
    if (w <= 0 || h <= 0) {
        const glm::ivec4& vp = currentRenderer().viewport();
        w = vp.z > 0 ? vp.z : 1280;
        h = vp.w > 0 ? vp.w : 720;
    }

    bool captured = false;
    bool usedFallback = false;
    if (_mode == "display" || _mode == "window") {
        captured = captureDisplayImage(framePixels, w, h);
        if (!captured && _fallbackToViewport) {
            captured = captureViewportImage(framePixels, w, h, optionalPixels);
            usedFallback = captured;
            if (captured) _mode = "viewport";
        }
    } else {
        captured = captureViewportImage(framePixels, w, h, optionalPixels);
    }

    if (!captured || framePixels.empty()) {
        _status = "error: snapshot capture failed";
        if (_lastError.empty()) _lastError = "Failed to capture snapshot buffer";
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
        std::error_code ec;
        fs::create_directories(baseDir, ec);
        fs::path candidate = baseDir / buf;
        for (uint64_t suffix = 1; fs::exists(candidate, ec); ++suffix) {
            candidate = baseDir / (fs::path(buf).stem().string() + "_" +
                std::to_string(suffix) + ".png");
        }
        destPath = candidate.string();
    }

    bool success = false;
    if (fs::path(destPath).extension() == ".ppm") {
        success = writePpm(destPath, framePixels.data(), w, h);
    } else {
        success = writePng(destPath, framePixels.data(), w, h);
    }

    if (success) {
        _captureWidth = w;
        _captureHeight = h;
        _lastSnapshotPath = destPath;
        _bytesWritten += static_cast<double>(framePixels.size());
        _status = "snapshot-saved";
        _lastError = usedFallback
            ? "Host display capture unavailable; saved the Earthcall viewport instead."
            : "";
        return true;
    } else {
        _status = "error: snapshot failed";
        _lastError = "Failed to save snapshot to: " + destPath;
        return false;
    }
}

bool ScreenRecorder::checkPendingSnapshot(int viewportW, int viewportH, const uint8_t* optionalPixels) {
    if (!_pendingSnapshot) return false;
    _pendingSnapshot = false;
    _snapshotTrigger = false;
    std::string path = _pendingSnapshotPath;
    _pendingSnapshotPath.clear();
    return captureSnapshot(path, viewportW, viewportH, optionalPixels);
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
    _pendingSnapshot = v;
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
    auto registerBool = [this](const std::string& name,
                               bool (ScreenRecorder::*getter)() const,
                               void (ScreenRecorder::*setter)(const bool&)) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            "recorder." + name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            "screen-recorder." + name, this, getter, setter));
    };

    auto registerReadOnlyBool = [this](const std::string& name,
                                       bool (ScreenRecorder::*getter)() const) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            "recorder." + name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, bool>>(
            "screen-recorder." + name, this, getter, nullptr));
    };

    auto registerString = [this](const std::string& name,
                                 std::string (ScreenRecorder::*getter)() const,
                                 void (ScreenRecorder::*setter)(const std::string&)) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            "recorder." + name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            "screen-recorder." + name, this, getter, setter));
    };

    auto registerReadOnlyString = [this](const std::string& name,
                                         std::string (ScreenRecorder::*getter)() const) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            "recorder." + name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, std::string>>(
            "screen-recorder." + name, this, getter, nullptr));
    };

    auto registerDouble = [this](const std::string& name,
                                 double (ScreenRecorder::*getter)() const,
                                 void (ScreenRecorder::*setter)(const double&)) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            "recorder." + name, this, getter, setter));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            "screen-recorder." + name, this, getter, setter));
    };

    auto registerReadOnlyDouble = [this](const std::string& name,
                                         double (ScreenRecorder::*getter)() const) {
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            "recorder." + name, this, getter, nullptr));
        registerProperty(std::make_unique<ComputedProperty<ScreenRecorder, double>>(
            "screen-recorder." + name, this, getter, nullptr));
    };

    registerBool("enabled", &ScreenRecorder::propEnabled, &ScreenRecorder::propSetEnabled);
    registerBool("recording", &ScreenRecorder::propRecording, &ScreenRecorder::propSetRecording);
    registerBool("paused", &ScreenRecorder::propPaused, &ScreenRecorder::propSetPaused);

    registerString("mode", &ScreenRecorder::propMode, &ScreenRecorder::propSetMode);
    registerString("format", &ScreenRecorder::propFormat, &ScreenRecorder::propSetFormat);
    registerString("outputPath", &ScreenRecorder::propOutputPath, &ScreenRecorder::propSetOutputPath);
    registerDouble("fps", &ScreenRecorder::propFps, &ScreenRecorder::propSetFps);

    registerBool("recordCursor", &ScreenRecorder::propRecordCursor, &ScreenRecorder::propSetRecordCursor);
    registerBool("fallbackToViewport", &ScreenRecorder::propFallbackToViewport, &ScreenRecorder::propSetFallbackToViewport);

    registerBool("start", &ScreenRecorder::propStartTrigger, &ScreenRecorder::propSetStartTrigger);
    registerBool("stop", &ScreenRecorder::propStopTrigger, &ScreenRecorder::propSetStopTrigger);
    registerBool("pause", &ScreenRecorder::propPauseTrigger, &ScreenRecorder::propSetPauseTrigger);
    registerBool("resume", &ScreenRecorder::propResumeTrigger, &ScreenRecorder::propSetResumeTrigger);
    registerBool("snapshot", &ScreenRecorder::propSnapshotTrigger, &ScreenRecorder::propSetSnapshotTrigger);
    registerBool("requestPermission", &ScreenRecorder::propRequestPermissionTrigger, &ScreenRecorder::propSetRequestPermissionTrigger);

    registerReadOnlyString("status", &ScreenRecorder::propStatus);
    registerReadOnlyString("lastError", &ScreenRecorder::propLastError);
    registerReadOnlyDouble("frameCount", &ScreenRecorder::propFrameCount);
    registerReadOnlyDouble("recordedDuration", &ScreenRecorder::propRecordedDuration);
    registerReadOnlyDouble("captureWidth", &ScreenRecorder::propCaptureWidth);
    registerReadOnlyDouble("captureHeight", &ScreenRecorder::propCaptureHeight);
    registerReadOnlyString("lastSnapshotPath", &ScreenRecorder::propLastSnapshotPath);
    registerReadOnlyDouble("bytesWritten", &ScreenRecorder::propBytesWritten);

    registerReadOnlyBool("hasScreenCapturePermission", &ScreenRecorder::propHasScreenCapturePermission);
    registerReadOnlyBool("hasAccessibilityPermission", &ScreenRecorder::propHasAccessibilityPermission);
    registerReadOnlyString("permissionStatus", &ScreenRecorder::propPermissionStatus);
    registerReadOnlyString("accessibilityDetails", &ScreenRecorder::propAccessibilityDetails);
}

} // namespace Screen
} // namespace Singularity
