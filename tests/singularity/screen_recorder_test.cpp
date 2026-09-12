#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace Singularity::Screen;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& desc) {
    if (!condition) {
        std::printf("  FAILED: %s\n", desc.c_str());
        ++g_failures;
    } else {
        std::printf("  ok: %s\n", desc.c_str());
    }
    std::fflush(stdout);
}

} // namespace

int main() {
    std::printf("Running screen_recorder_test...\n");
    std::fflush(stdout);

    LawManager laws;
    ScreenRecorder::syncRegister(laws);
    ScreenChannel::syncRegister(laws);

    ScreenRecorder* recorder = ScreenRecorder::find(laws);
    ScreenChannel* channel = ScreenChannel::find(laws);

    check(recorder != nullptr, "ScreenRecorder first mover registered successfully");
    check(channel != nullptr, "ScreenChannel registered successfully");

    if (!recorder || !channel) {
        return 1;
    }

    // Prepare clean test recordings sandbox under saves/
    fs::path testDir = fs::path("saves") / "test_recorder_sandbox";
    std::error_code ec;
    fs::remove_all(testDir, ec);
    fs::create_directories(testDir, ec);

    lawSetValue(*recorder, PropertyPath::parse("recorder.outputPath"), PropertyValue(testDir.string()));

    // -----------------------------------------------------------------------
    // Case 1: Permissions & Accessibility Inspection
    // -----------------------------------------------------------------------
    PropertyValue val;
    lawGetValue(*recorder, PropertyPath::parse("recorder.hasScreenCapturePermission"), val);
    bool hasScreenCapture = std::get<bool>(val);
    check(val.index() != 0, "hasScreenCapturePermission property returns a valid boolean");

    lawGetValue(*recorder, PropertyPath::parse("recorder.hasAccessibilityPermission"), val);
    bool hasAccessibility = std::get<bool>(val);
    check(val.index() != 0, "hasAccessibilityPermission property returns a valid boolean");

    lawGetValue(*recorder, PropertyPath::parse("recorder.permissionStatus"), val);
    std::string permStatus;
    if (std::holds_alternative<std::string>(val)) {
        permStatus = std::get<std::string>(val);
    }
    check(!permStatus.empty(), "permissionStatus returns non-empty diagnostic string");

    lawGetValue(*recorder, PropertyPath::parse("recorder.accessibilityDetails"), val);
    std::string details;
    if (std::holds_alternative<std::string>(val)) {
        details = std::get<std::string>(val);
    }
    check(!details.empty(), "accessibilityDetails returns non-empty guidance");

    std::printf("  Diagnostics: ScreenCapture=%d, Accessibility=%d, Status=%s\n",
                hasScreenCapture, hasAccessibility, permStatus.c_str());
    std::fflush(stdout);

    // -----------------------------------------------------------------------
    // Case 2: PPM Sequence Recording (In-Engine Viewport Mode)
    // -----------------------------------------------------------------------
    lawSetValue(*recorder, PropertyPath::parse("recorder.mode"), PropertyValue(std::string("viewport")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.format"), PropertyValue(std::string("ppm_sequence")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.fps"), PropertyValue(60.0));
    lawSetValue(*recorder, PropertyPath::parse("recorder.recordCursor"), PropertyValue(true));

    // Start recording
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));

    lawGetValue(*recorder, PropertyPath::parse("recorder.recording"), val);
    check(std::get<bool>(val) == true, "recorder.recording is true after start trigger");

    lawGetValue(*recorder, PropertyPath::parse("recorder.status"), val);
    check(std::get<std::string>(val) == "recording", "recorder.status is 'recording'");

    // Simulate 5 frames of synthetic rendering data (128x128 RGBA)
    const int testW = 128;
    const int testH = 128;
    std::vector<uint8_t> frameData(testW * testH * 4, 180);

    for (int i = 0; i < 5; ++i) {
        recorder->stepFrame(testW, testH, frameData.data());
    }

    lawGetValue(*recorder, PropertyPath::parse("recorder.frameCount"), val);
    double framesRecorded = std::get<double>(val);
    check(framesRecorded >= 1.0, "stepFrame recorded frames into active session");

    // Stop recording
    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));

    lawGetValue(*recorder, PropertyPath::parse("recorder.recording"), val);
    check(std::get<bool>(val) == false, "recorder.recording is false after stop trigger");

    // Verify session directory and PPM files on disk
    bool foundPpm = false;
    for (const auto& entry : fs::recursive_directory_iterator(testDir, ec)) {
        if (entry.path().extension() == ".ppm") {
            foundPpm = true;
            std::ifstream ppm(entry.path(), std::ios::binary);
            std::string line1, line2, line3;
            ppm >> line1 >> line2 >> line3;
            check(line1 == "P6", "Recorded PPM file has valid P6 binary header");
            break;
        }
    }
    check(foundPpm, "PPM frame file actually written to session directory");

    // -----------------------------------------------------------------------
    // Case 3: PNG Sequence Recording with ZLIB
    // -----------------------------------------------------------------------
    lawSetValue(*recorder, PropertyPath::parse("recorder.format"), PropertyValue(std::string("png_sequence")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));

    for (int i = 0; i < 3; ++i) {
        recorder->stepFrame(testW, testH, frameData.data());
    }

    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));

    bool foundPng = false;
    for (const auto& entry : fs::recursive_directory_iterator(testDir, ec)) {
        if (entry.path().extension() == ".png" && entry.path().filename().string().find("frame_") == 0) {
            foundPng = true;
            std::ifstream png(entry.path(), std::ios::binary);
            unsigned char sig[8];
            png.read(reinterpret_cast<char*>(sig), 8);
            check(sig[0] == 0x89 && sig[1] == 'P' && sig[2] == 'N' && sig[3] == 'G',
                  "Recorded PNG file has valid 8-byte PNG signature");
            break;
        }
    }
    check(foundPng, "PNG frame file successfully encoded and saved via zlib");

    // -----------------------------------------------------------------------
    // Case 4: Raw Stream Recording
    // -----------------------------------------------------------------------
    lawSetValue(*recorder, PropertyPath::parse("recorder.format"), PropertyValue(std::string("raw")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));

    for (int i = 0; i < 2; ++i) {
        recorder->stepFrame(testW, testH, frameData.data());
    }

    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));

    bool foundRaw = false;
    for (const auto& entry : fs::recursive_directory_iterator(testDir, ec)) {
        if (entry.path().filename() == "stream.raw") {
            foundRaw = true;
            std::ifstream raw(entry.path(), std::ios::binary);
            char header[8];
            raw.read(header, 8);
            check(std::string(header, 8) == "ECREC001", "Raw stream begins with ECREC001 header");
            break;
        }
    }
    check(foundRaw, "Raw binary stream file successfully generated");

    // -----------------------------------------------------------------------
    // Case 5: Pause and Resume
    // -----------------------------------------------------------------------
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));
    lawSetValue(*recorder, PropertyPath::parse("recorder.pause"), PropertyValue(true));

    lawGetValue(*recorder, PropertyPath::parse("recorder.paused"), val);
    check(std::get<bool>(val) == true, "recorder.paused is true after pause trigger");
    check(recorder->isRecording() == false, "isRecording() returns false while paused");

    lawSetValue(*recorder, PropertyPath::parse("recorder.resume"), PropertyValue(true));
    lawGetValue(*recorder, PropertyPath::parse("recorder.paused"), val);
    check(std::get<bool>(val) == false, "recorder.paused is false after resume trigger");
    check(recorder->isRecording() == true, "isRecording() returns true after resume trigger");

    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));

    // -----------------------------------------------------------------------
    // Case 6: Snapshot Capture (Instant Screenshot)
    // -----------------------------------------------------------------------
    fs::path snapshotFile = testDir / "instant_snap.png";
    recorder->captureSnapshot(snapshotFile.string());

    lawGetValue(*recorder, PropertyPath::parse("recorder.lastSnapshotPath"), val);
    check(std::get<std::string>(val) == snapshotFile.string(), "lastSnapshotPath populated with snapshot filename");
    check(fs::exists(snapshotFile), "Instant snapshot file actually created on disk");

    // -----------------------------------------------------------------------
    // Case 7: Fallback to Viewport when OS Screen Capture is Denied
    // -----------------------------------------------------------------------
    lawSetValue(*recorder, PropertyPath::parse("recorder.mode"), PropertyValue(std::string("display")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.fallbackToViewport"), PropertyValue(true));
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));

    lawGetValue(*recorder, PropertyPath::parse("recorder.recording"), val);
    check(std::get<bool>(val) == true, "Recording successfully started in display mode with viewport fallback");

    bool stepped = recorder->stepFrame(testW, testH, frameData.data());
    check(stepped == true, "stepFrame succeeded using fallback without crashing or throwing");

    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));

    // -----------------------------------------------------------------------
    // Case 8: ScreenChannel Integration & Property Delegation
    // -----------------------------------------------------------------------
    lawGetValue(*channel, PropertyPath::parse("hasScreenCapturePermission"), val);
    check(std::get<bool>(val) == hasScreenCapture, "ScreenChannel delegates hasScreenCapturePermission");

    lawGetValue(*channel, PropertyPath::parse("hasAccessibilityPermission"), val);
    check(std::get<bool>(val) == hasAccessibility, "ScreenChannel delegates hasAccessibilityPermission");

    // Clean up test files
    fs::remove_all(testDir, ec);

    if (g_failures > 0) {
        std::printf("screen_recorder_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("screen_recorder_test: ALL OK (all 8 cases passed)\n");
    return 0;
}
