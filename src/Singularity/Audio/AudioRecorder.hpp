#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Singularity {
namespace Audio {

class AudioRecorder : public Law {
public:
    AudioRecorder();
    ~AudioRecorder() override;

    static void syncRegister(LawManager& laws);
    static AudioRecorder* find(LawManager& laws);

    std::string getIdentifier() const override { return "microphone"; }

    // Recording controls
    bool startRecording(const std::string& customPath = "");
    bool stopRecording();
    bool pauseRecording();
    bool resumeRecording();

    bool isRecording() const { return _recording; }
    bool isPaused() const { return _paused; }

    // Engine loop integration
    void tick(double dt = 0.016);

    // Audio stream ingestion (called from hardware callback or synthesis simulation)
    void feedSamples(const float* pSamples, size_t sampleCount);

    // Hardware device enumeration & selection
    std::vector<std::string> getAvailableDevices() const;
    void setDevice(const std::string& deviceName);

    // macOS Permissions (TCC)
    static bool hasMicrophonePermission();
    static bool requestMicrophonePermission();

    // Standard WAV export
    static bool writeWav(const std::string& path, const float* samples, size_t sampleCount, int sampleRate, int channels);

    // Simulation / testing control
    void setSimulatedMode(bool sim) { _forceSimulated = sim; }
    bool isSimulated() const { return _usingSimulation; }

protected:
    void buildProperties() override;

private:
    std::string ensureSessionFile();
    bool initCaptureDevice();
    void shutdownCaptureDevice();

    // Controls
    bool _enabled = true;
    std::atomic<bool> _recording{false};
    std::atomic<bool> _paused{false};
    std::string _selectedDevice = "default";
    double _sampleRate = 48000.0;
    double _channels = 1.0;
    double _gain = 1.0;
    double _speechThreshold = 0.05;
    std::string _outputPath = "saves/recordings";
    bool _fallbackToSimulated = true;
    bool _forceSimulated = false;

    // Triggers
    bool _startTrigger = false;
    bool _stopTrigger = false;
    bool _pauseTrigger = false;
    bool _resumeTrigger = false;
    bool _refreshDevicesTrigger = false;
    bool _requestPermissionTrigger = false;

    // Telemetry
    std::string _status = "idle";
    std::string _lastError = "";
    std::atomic<double> _inputLevel{0.0};
    std::atomic<double> _peakLevel{0.0};
    double _recordedDuration = 0.0;
    double _sampleCount = 0.0;
    double _bytesRecorded = 0.0;
    std::string _lastRecordingPath = "";
    std::string _activeFilePath = "";

    // Internal capture state
    void* _pDevice = nullptr;       // Opaque ma_device*
    bool _usingSimulation = false;
    bool _lastSpeakingState = false;
    double _simPhase = 0.0;

    std::mutex _sampleMutex;
    std::vector<float> _recordedSamples;
    std::chrono::steady_clock::time_point _startTime;

    // Property getters/setters for Law property table
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    bool propRecording() const { return _recording; }
    void propSetRecording(const bool& v);

    bool propPaused() const { return _paused; }
    void propSetPaused(const bool& v);

    std::string propSelectedDevice() const { return _selectedDevice; }
    void propSetSelectedDevice(const std::string& v) { setDevice(v); }

    double propSampleRate() const { return _sampleRate; }
    void propSetSampleRate(const double& v) { if (v >= 8000.0 && v <= 192000.0) _sampleRate = v; }

    double propChannels() const { return _channels; }
    void propSetChannels(const double& v) { if (v == 1.0 || v == 2.0) _channels = v; }

    double propGain() const { return _gain; }
    void propSetGain(const double& v) { if (v >= 0.0) _gain = v; }

    double propSpeechThreshold() const { return _speechThreshold; }
    void propSetSpeechThreshold(const double& v) { if (v >= 0.0 && v <= 1.0) _speechThreshold = v; }

    std::string propOutputPath() const { return _outputPath; }
    void propSetOutputPath(const std::string& v) { _outputPath = v; }

    bool propFallbackToSimulated() const { return _fallbackToSimulated; }
    void propSetFallbackToSimulated(const bool& v) { _fallbackToSimulated = v; }

    bool propStartTrigger() const { return _startTrigger; }
    void propSetStartTrigger(const bool& v);

    bool propStopTrigger() const { return _stopTrigger; }
    void propSetStopTrigger(const bool& v);

    bool propPauseTrigger() const { return _pauseTrigger; }
    void propSetPauseTrigger(const bool& v);

    bool propResumeTrigger() const { return _resumeTrigger; }
    void propSetResumeTrigger(const bool& v);

    bool propRefreshDevicesTrigger() const { return _refreshDevicesTrigger; }
    void propSetRefreshDevicesTrigger(const bool& v);

    bool propRequestPermissionTrigger() const { return _requestPermissionTrigger; }
    void propSetRequestPermissionTrigger(const bool& v);

    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }
    double propInputLevel() const { return _inputLevel; }
    double propPeakLevel() const { return _peakLevel; }
    double propRecordedDuration() const { return _recordedDuration; }
    double propSampleCount() const { return _sampleCount; }
    double propBytesRecorded() const { return _bytesRecorded; }
    std::string propLastRecordingPath() const { return _lastRecordingPath; }

    std::string propDevices() const;
    double propDeviceCount() const;
    bool propHasPermission() const { return hasMicrophonePermission(); }
    std::string propPermissionStatus() const;
    std::string propPermissionDetails() const;
};

} // namespace Audio
} // namespace Singularity
