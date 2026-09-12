#include "Singularity/Audio/AudioRecorder.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>

namespace fs = std::filesystem;
using namespace Singularity::Audio;

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
    std::printf("Running audio_recorder_test...\n");
    std::fflush(stdout);

    LawManager laws;
    Singularity::Storage::VirtualFileSystem::syncRegister(laws);
    AudioRecorder::syncRegister(laws);

    AudioRecorder* mic = AudioRecorder::find(laws);
    check(mic != nullptr, "AudioRecorder first mover registered successfully as @microphone");
    if (!mic) return 1;

    // -----------------------------------------------------------------------
    // Case 1: macOS Permissions & Telemetry Inspection
    // -----------------------------------------------------------------------
    bool hasPerm = AudioRecorder::hasMicrophonePermission();
    (void)hasPerm;
    check(true, "hasMicrophonePermission returned valid status without crashing");

    PropertyValue permStatusVal;
    lawGetValue(*mic, PropertyPath::parse("mic.permissionStatus"), permStatusVal);
    std::string permStatus = std::get<std::string>(permStatusVal);
    check(!permStatus.empty(), "mic.permissionStatus returns non-empty string (" + permStatus + ")");

    PropertyValue permDetailsVal;
    lawGetValue(*mic, PropertyPath::parse("mic.permissionDetails"), permDetailsVal);
    std::string permDetails = std::get<std::string>(permDetailsVal);
    check(!permDetails.empty(), "mic.permissionDetails returns clear guidance instructions");

    // -----------------------------------------------------------------------
    // Case 2: Device Enumeration & Selection
    // -----------------------------------------------------------------------
    auto devices = mic->getAvailableDevices();
    check(!devices.empty(), "Device enumeration returns at least 1 input device (found " + std::to_string(devices.size()) + ")");

    PropertyValue devListVal;
    lawGetValue(*mic, PropertyPath::parse("mic.devices"), devListVal);
    check(!std::get<std::string>(devListVal).empty(), "mic.devices lists available input hardware");

    PropertyValue devCountVal;
    lawGetValue(*mic, PropertyPath::parse("mic.deviceCount"), devCountVal);
    check(std::get<double>(devCountVal) >= 1.0, "mic.deviceCount reflects device catalog size");

    lawSetValue(*mic, PropertyPath::parse("mic.selectedDevice"), PropertyValue(std::string("Default System Microphone")));
    PropertyValue selDevVal;
    lawGetValue(*mic, PropertyPath::parse("mic.selectedDevice"), selDevVal);
    check(std::get<std::string>(selDevVal) == "Default System Microphone", "mic.selectedDevice successfully updated");

    // -----------------------------------------------------------------------
    // Case 3: Ingestion, Metering, and ECA Edge Events (Speech Detection)
    // -----------------------------------------------------------------------
    int speechDetectedCount = 0;
    int silenceDetectedCount = 0;
    int recordingStartedCount = 0;
    int recordingStoppedCount = 0;

    Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
        if (ev.type == "speech-detected") ++speechDetectedCount;
        if (ev.type == "silence-detected") ++silenceDetectedCount;
        if (ev.type == "recording-started") ++recordingStartedCount;
        if (ev.type == "recording-stopped") ++recordingStoppedCount;
    });

    // Configure for deterministic simulation
    mic->setSimulatedMode(true);
    fs::path testWav = fs::path("saves") / "test_recordings" / "test_voice.wav";
    std::error_code ec;
    fs::remove(testWav, ec);

    // Start recording
    bool started = mic->startRecording(testWav.string());
    check(started, "startRecording succeeded");
    check(mic->isRecording(), "isRecording returns true after start");
    check(recordingStartedCount == 1, "EventBus delivered 'recording-started' edge event");

    PropertyValue statusVal;
    lawGetValue(*mic, PropertyPath::parse("mic.status"), statusVal);
    check(std::get<std::string>(statusVal) == "recording", "mic.status is 'recording'");

    // Feed silence: inputLevel should remain near 0
    std::vector<float> silence(480, 0.0f); // 10ms of silence
    mic->feedSamples(silence.data(), silence.size());

    PropertyValue levelVal;
    lawGetValue(*mic, PropertyPath::parse("mic.inputLevel"), levelVal);
    check(std::get<double>(levelVal) == 0.0, "Input level for silence is 0.0");

    // Feed high-amplitude speech audio (above threshold 0.05)
    std::vector<float> speech(960, 0.4f); // 20ms of speech
    mic->feedSamples(speech.data(), speech.size());

    lawGetValue(*mic, PropertyPath::parse("mic.inputLevel"), levelVal);
    check(std::get<double>(levelVal) > 0.35, "Input level RMS accurately measures audio signal");

    PropertyValue peakVal;
    lawGetValue(*mic, PropertyPath::parse("mic.peakLevel"), peakVal);
    check(std::get<double>(peakVal) >= 0.4, "Peak level tracks maximum audio sample");
    check(speechDetectedCount == 1, "EventBus delivered 'speech-detected' edge event on transition");

    // Feed silence again to trigger silence-detected edge event
    mic->feedSamples(silence.data(), silence.size());
    check(silenceDetectedCount == 1, "EventBus delivered 'silence-detected' edge event on transition");

    // -----------------------------------------------------------------------
    // Case 4: Pause, Resume, and Tick Simulation
    // -----------------------------------------------------------------------
    mic->pauseRecording();
    check(mic->isPaused(), "mic.paused is true after pause");
    lawGetValue(*mic, PropertyPath::parse("mic.status"), statusVal);
    check(std::get<std::string>(statusVal) == "paused", "mic.status is 'paused'");

    mic->resumeRecording();
    check(!mic->isPaused(), "mic.paused is false after resume");
    lawGetValue(*mic, PropertyPath::parse("mic.status"), statusVal);
    check(std::get<std::string>(statusVal) == "recording", "mic.status returned to 'recording'");

    // Step tick to simulate 0.25 seconds of generated audio
    for (int i = 0; i < 15; ++i) {
        mic->tick(0.016);
    }

    PropertyValue durationVal;
    lawGetValue(*mic, PropertyPath::parse("mic.recordedDuration"), durationVal);
    check(std::get<double>(durationVal) > 0.2, "mic.recordedDuration advanced with tick");

    // -----------------------------------------------------------------------
    // Case 5: Stop Recording & Canonical 16-bit PCM WAV Validation
    // -----------------------------------------------------------------------
    bool stopped = mic->stopRecording();
    check(stopped, "stopRecording succeeded");
    check(!mic->isRecording(), "isRecording returns false after stop");
    check(recordingStoppedCount == 1, "EventBus delivered 'recording-stopped' edge event");

    PropertyValue lastPathVal;
    lawGetValue(*mic, PropertyPath::parse("mic.lastRecordingPath"), lastPathVal);
    check(std::get<std::string>(lastPathVal) == testWav.string(), "mic.lastRecordingPath matches target file");
    check(fs::exists(testWav), "WAV recording file was written to disk");

    // Inspect RIFF/WAVE header
    std::ifstream wavIn(testWav, std::ios::binary);
    check(wavIn.is_open(), "Successfully opened generated WAV file for inspection");

    char riffHeader[4];
    wavIn.read(riffHeader, 4);
    check(std::string(riffHeader, 4) == "RIFF", "WAV starts with standard 'RIFF' chunk header");

    wavIn.seekg(8);
    char waveHeader[4];
    wavIn.read(waveHeader, 4);
    check(std::string(waveHeader, 4) == "WAVE", "File format signature is 'WAVE'");

    char fmtHeader[4];
    wavIn.read(fmtHeader, 4);
    check(std::string(fmtHeader, 4) == "fmt ", "Subchunk identifier is 'fmt '");

    uint32_t fmtSize = 0;
    wavIn.read(reinterpret_cast<char*>(&fmtSize), 4);
    check(fmtSize == 16, "fmt subchunk size is 16 for PCM");

    uint16_t audioFormat = 0;
    wavIn.read(reinterpret_cast<char*>(&audioFormat), 2);
    check(audioFormat == 1, "Audio format code is 1 (uncompressed PCM)");

    uint16_t numChannels = 0;
    wavIn.read(reinterpret_cast<char*>(&numChannels), 2);
    check(numChannels == 1, "Number of channels is 1 (mono)");

    uint32_t sampleRate = 0;
    wavIn.read(reinterpret_cast<char*>(&sampleRate), 4);
    check(sampleRate == 48000, "Sample rate is 48000 Hz");

    wavIn.close();
    fs::remove(testWav, ec);

    // -----------------------------------------------------------------------
    // Case 6: Direct Static WAV Export via Universal VFS URI (recording://)
    // -----------------------------------------------------------------------
    std::vector<float> synthTone(4800); // 100ms of 440 Hz tone
    for (size_t i = 0; i < synthTone.size(); ++i) {
        synthTone[i] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979323846 * 440.0 * (static_cast<double>(i) / 48000.0)));
    }

    std::string vfsPath = "recording://vfs_mic_test.wav";
    bool vfsOk = AudioRecorder::writeWav(vfsPath, synthTone.data(), synthTone.size(), 48000, 1);
    check(vfsOk, "writeWav to universal VFS URI 'recording://...' succeeded");

    std::string resolvedPhysical = Singularity::Storage::VirtualFileSystem::resolve(vfsPath);
    check(fs::exists(resolvedPhysical), "File confirmed written to resolved physical VFS storage path");
    fs::remove(resolvedPhysical, ec);

    if (g_failures > 0) {
        std::printf("audio_recorder_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("audio_recorder_test: ALL OK (all 6 cases passed)\n");
    return 0;
}
