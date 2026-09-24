#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"

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

    // Dynamic recording control delegation
    lawSetValue(*channel, PropertyPath::parse("recording"), PropertyValue(true));
    lawGetValue(*recorder, PropertyPath::parse("recorder.recording"), val);
    check(std::get<bool>(val) == true, "ScreenChannel.recording=true delegates to ScreenRecorder");
    lawGetValue(*channel, PropertyPath::parse("recording"), val);
    check(std::get<bool>(val) == true, "ScreenChannel.recording reports true when recorder is active");

    lawSetValue(*channel, PropertyPath::parse("recording"), PropertyValue(false));
    lawGetValue(*recorder, PropertyPath::parse("recorder.recording"), val);
    check(std::get<bool>(val) == false, "ScreenChannel.recording=false stops ScreenRecorder");

    // Dynamic snapshot delegation
    lawSetValue(*channel, PropertyPath::parse("snapshot"), PropertyValue(true));
    lawGetValue(*recorder, PropertyPath::parse("recorder.lastSnapshotPath"), val);
    check(!std::get<std::string>(val).empty(), "ScreenChannel.snapshot delegates to ScreenRecorder snapshot");

    // -----------------------------------------------------------------------
    // Case 9: Stream Pipe and Pending Snapshot Handling
    // -----------------------------------------------------------------------
    recorder->checkPendingSnapshot(testW, testH, frameData.data());
    check(!recorder->isSnapshotPending(), "checkPendingSnapshot clears pending snapshot state");

    lawSetValue(*recorder, PropertyPath::parse("recorder.format"), PropertyValue(std::string("pipe")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.outputPath"), PropertyValue(std::string("cat > /dev/null")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.start"), PropertyValue(true));
    stepped = recorder->stepFrame(testW, testH, frameData.data());
    check(stepped == true, "stepFrame succeeded in pipe format");
    lawSetValue(*recorder, PropertyPath::parse("recorder.stop"), PropertyValue(true));
    lawSetValue(*recorder, PropertyPath::parse("recorder.format"), PropertyValue(std::string("png_sequence")));
    lawSetValue(*recorder, PropertyPath::parse("recorder.outputPath"), PropertyValue(testDir.string()));

    // -----------------------------------------------------------------------
    // Case 10: Authored Law Condition Evaluation & Property Mutation
    // -----------------------------------------------------------------------
    std::printf("\n--- Starting Law Verification Cases ---\n");
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "player");
    player.setDynamicProperty("requestSnapshot", PropertyValue(false));
    player.setDynamicProperty("triggerArmed", PropertyValue(false));
    player.setDynamicProperty("recordArmed", PropertyValue(false));

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&player);
        beings.push_back(recorder);
        beings.push_back(channel);
        for (const auto& l : laws.getAll()) {
            if (l) beings.push_back(l.get());
        }
    });

    // Author Law: WHEN premise @player.requestSnapshot == true THEN @screen-recorder.snapshot := true
    ConditionNode snapshotCondition =
        ConditionNode::compare("@player.requestSnapshot", ConditionNode::Op::Eq, PropertyValue(true));
    ActionNode snapshotAction = ActionNode::set("@screen-recorder.snapshot", PropertyValue(true));

    auto snapshotLaw = laws.createLaw("Law: Snapshot On Request", {&player});
    snapshotLaw->setConditionModel(snapshotCondition);
    snapshotLaw->setActionModel(snapshotAction);

    // 10a: Premise is false -> condition evaluation must fail, action must NOT execute
    player.setDynamicProperty("requestSnapshot", PropertyValue(false));
    check(!snapshotLaw->conditionsSatisfied(*recorder), "Law condition is false when premise @player.requestSnapshot is false");
    auto resFalse = snapshotLaw->applyTo(*recorder);
    check(resFalse == Law::ApplicationResult::ConditionsFailed, "Law application returns ConditionsFailed when premise is false");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == false, "screen-recorder.snapshot remains false when condition fails");

    // 10b: Premise is true -> condition must pass and law must execute, changing property to true
    player.setDynamicProperty("requestSnapshot", PropertyValue(true));
    check(snapshotLaw->conditionsSatisfied(*recorder), "Law condition is true when premise @player.requestSnapshot is true");
    auto resTrue = snapshotLaw->applyTo(*recorder);
    check(resTrue == Law::ApplicationResult::Applied, "Law application returns Applied when premise is true");
    
    // Verify all alias forms reflect the true property change
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == true, "Law successfully changed property 'screen-recorder.snapshot' to true");
    lawGetValue(*recorder, PropertyPath::parse("snapshot"), val);
    check(std::get<bool>(val) == true, "Property 'snapshot' reads true");
    lawGetValue(*recorder, PropertyPath::parse("recorder.snapshot"), val);
    check(std::get<bool>(val) == true, "Property 'recorder.snapshot' reads true");
    check(recorder->isSnapshotPending(), "ScreenRecorder isSnapshotPending() reports true");

    // 10c: Author Law to reset snapshot property to false
    ActionNode resetSnapshotAction = ActionNode::set("@screen-recorder.snapshot", PropertyValue(false));
    auto resetLaw = laws.createLaw("Law: Reset Snapshot", {&player});
    resetLaw->setActionModel(resetSnapshotAction);
    auto resReset = resetLaw->applyTo(*recorder);
    check(resReset == Law::ApplicationResult::Applied, "Reset Law application returns Applied");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == false, "Law successfully changed property 'screen-recorder.snapshot' back to false");
    check(!recorder->isSnapshotPending(), "isSnapshotPending() reports false after reset");

    // -----------------------------------------------------------------------
    // Case 11: Event-Triggered Law Firing via EventBus & Agenda
    // -----------------------------------------------------------------------
    laws.connectToEventBus();

    ConditionNode eventCondition =
        ConditionNode::compare("@player.triggerArmed", ConditionNode::Op::Eq, PropertyValue(true));
    ActionNode eventAction = ActionNode::set("@screen-recorder.snapshot", PropertyValue(true));

    auto eventLaw = laws.createLaw("Law: Snapshot On Event", {&player});
    eventLaw->setConditionModel(eventCondition);
    eventLaw->setActionModel(eventAction);
    const std::string eventLawId = eventLaw->getIdentifier();
    laws.bindTrigger(eventLawId, "user-snapshot-requested");

    // 11a: Event published with triggerArmed = false -> Law must NOT fire
    player.setDynamicProperty("triggerArmed", PropertyValue(false));
    ECA::Event eventMisfire{"user-snapshot-requested", &player, nullptr, 0};
    Core::EventBus::instance().publish(eventMisfire);
    laws.tick();
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == false, "Event published while triggerArmed=false does not change snapshot property");

    // 11b: Event published with triggerArmed = true -> Law must fire and mutate property to true
    player.setDynamicProperty("triggerArmed", PropertyValue(true));
    ECA::Event eventFire{"user-snapshot-requested", &player, nullptr, 0};
    Core::EventBus::instance().publish(eventFire);
    laws.tick();
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == true, "Event published while triggerArmed=true fires Law and changes snapshot to true");
    check(recorder->isSnapshotPending(), "Snapshot is pending after event trigger");

    // 11c: Frame boundary captures snapshot and automatically clears snapshot trigger
    bool pendingCaptured = recorder->checkPendingSnapshot(testW, testH, frameData.data());
    check(pendingCaptured, "checkPendingSnapshot successfully captured pending frame");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == false, "checkPendingSnapshot reset 'screen-recorder.snapshot' back to false");
    check(!recorder->isSnapshotPending(), "isSnapshotPending() is false after capture");

    // -----------------------------------------------------------------------
    // Case 12: Recording Property Control via Authored Law
    // -----------------------------------------------------------------------
    ConditionNode recordCond =
        ConditionNode::compare("@player.recordArmed", ConditionNode::Op::Eq, PropertyValue(true));
    ActionNode startRecordAction = ActionNode::set("@screen-recorder.recording", PropertyValue(true));
    ActionNode stopRecordAction = ActionNode::set("@screen-recorder.recording", PropertyValue(false));

    auto startRecLaw = laws.createLaw("Law: Start Recording", {&player});
    startRecLaw->setConditionModel(recordCond);
    startRecLaw->setActionModel(startRecordAction);

    auto stopRecLaw = laws.createLaw("Law: Stop Recording", {&player});
    stopRecLaw->setActionModel(stopRecordAction);

    // Premise false -> cannot start
    player.setDynamicProperty("recordArmed", PropertyValue(false));
    check(startRecLaw->applyTo(*recorder) == Law::ApplicationResult::ConditionsFailed, "Start recording law rejected when recordArmed is false");
    check(!recorder->isRecording(), "Recorder is not recording");

    // Premise true -> start recording via Law
    player.setDynamicProperty("recordArmed", PropertyValue(true));
    check(startRecLaw->applyTo(*recorder) == Law::ApplicationResult::Applied, "Start recording law applied when recordArmed is true");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.recording"), val);
    check(std::get<bool>(val) == true, "Law changed 'screen-recorder.recording' to true");
    check(recorder->isRecording(), "Recorder is actively recording");

    // Stop recording via Law
    check(stopRecLaw->applyTo(*recorder) == Law::ApplicationResult::Applied, "Stop recording law applied");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.recording"), val);
    check(std::get<bool>(val) == false, "Law changed 'screen-recorder.recording' to false");
    check(!recorder->isRecording(), "Recorder stopped recording");

    // -----------------------------------------------------------------------
    // Case 13: Law Round-Trip JSON Serialization & Execution
    // -----------------------------------------------------------------------
    nlohmann::json savedJson = snapshotLaw->toJson();
    check(!savedJson.empty(), "Snapshot Law successfully serialized to JSON");
    check(savedJson.contains("conditionModel"), "Serialized Law contains conditionModel");
    check(savedJson.contains("actionModel"), "Serialized Law contains actionModel");

    auto restoredLaw = Law::fromJson(savedJson);
    check(restoredLaw != nullptr, "Law::fromJson successfully deserialized Law");
    restoredLaw->addAuthor(player);
    check(restoredLaw->isAuthored(), "Restored Law preserves author reference");
    check(restoredLaw->conditionModel() != nullptr, "Restored Law preserves conditionModel");
    check(restoredLaw->actionModel() != nullptr, "Restored Law preserves actionModel");

    // Test execution of restored law
    player.setDynamicProperty("requestSnapshot", PropertyValue(false));
    check(restoredLaw->applyTo(*recorder) == Law::ApplicationResult::ConditionsFailed, "Restored Law evaluates false condition properly");

    player.setDynamicProperty("requestSnapshot", PropertyValue(true));
    check(restoredLaw->applyTo(*recorder) == Law::ApplicationResult::Applied, "Restored Law evaluates true condition and applies");
    lawGetValue(*recorder, PropertyPath::parse("screen-recorder.snapshot"), val);
    check(std::get<bool>(val) == true, "Restored Law changed 'screen-recorder.snapshot' to true");

    // Clean up pending snapshot state and provider
    recorder->checkPendingSnapshot(testW, testH, frameData.data());
    Universe::instance().setProvider({});

    // Clean up test files
    fs::remove_all(testDir, ec);

    if (g_failures > 0) {
        std::printf("screen_recorder_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("screen_recorder_test: ALL OK (all 13 cases passed)\n");
    return 0;
}
