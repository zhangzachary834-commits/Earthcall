#include "Singularity/Audio/AudioRecorder.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include "miniaudio.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#if defined(__APPLE__)
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace Singularity {
namespace Audio {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Canonical WAV File Writer (16-bit PCM)
// ---------------------------------------------------------------------------

struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0; // 36 + dataSize
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1;
    uint32_t sampleRate = 48000;
    uint32_t byteRate = 48000 * 1 * 2;
    uint16_t blockAlign = 2; // numChannels * 2
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};

bool AudioRecorder::writeWav(const std::string& path, const float* samples, size_t sampleCount, int sampleRate, int channels) {
    if (!samples || sampleCount == 0 || sampleRate <= 0 || channels <= 0) return false;

    // Resolve universal VFS paths (e.g. recording:// or save://)
    std::string resolvedPath = Storage::VirtualFileSystem::resolve(path);

    fs::path p(resolvedPath);
    std::error_code ec;
    fs::path parent = p.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    std::ofstream out(resolvedPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    uint32_t dataSize = static_cast<uint32_t>(sampleCount * sizeof(int16_t));
    WavHeader hdr;
    hdr.fileSize = 36 + dataSize;
    hdr.numChannels = static_cast<uint16_t>(channels);
    hdr.sampleRate = static_cast<uint32_t>(sampleRate);
    hdr.byteRate = hdr.sampleRate * hdr.numChannels * sizeof(int16_t);
    hdr.blockAlign = static_cast<uint16_t>(hdr.numChannels * sizeof(int16_t));
    hdr.bitsPerSample = 16;
    hdr.dataSize = dataSize;

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    // Convert float32 [-1.0, 1.0] to int16 [-32767, 32767]
    std::vector<int16_t> pcm16(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        float s = samples[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        pcm16[i] = static_cast<int16_t>(s * 32767.0f);
    }

    out.write(reinterpret_cast<const char*>(pcm16.data()), dataSize);
    return !out.fail();
}

// ---------------------------------------------------------------------------
// macOS Microphone Permissions (TCC)
// ---------------------------------------------------------------------------

bool AudioRecorder::hasMicrophonePermission() {
#if defined(__APPLE__)
    void* avf = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
    if (!avf) return true; // Fall back to open if framework unreachable

    CFStringRef* pAVMediaTypeAudio = reinterpret_cast<CFStringRef*>(dlsym(avf, "AVMediaTypeAudio"));
    if (!pAVMediaTypeAudio) return true;

    Class cls = objc_getClass("AVCaptureDevice");
    if (!cls) return true;

    SEL sel = sel_registerName("authorizationStatusForMediaType:");
    auto msgSend = reinterpret_cast<long (*)(Class, SEL, CFStringRef)>(objc_msgSend);
    long status = msgSend(cls, sel, *pAVMediaTypeAudio);
    // AVAuthorizationStatus: 0=NotDetermined, 1=Restricted, 2=Denied, 3=Authorized
    return (status == 3);
#else
    return true;
#endif
}

bool AudioRecorder::requestMicrophonePermission() {
#if defined(__APPLE__)
    void* avf = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
    if (!avf) return false;

    CFStringRef* pAVMediaTypeAudio = reinterpret_cast<CFStringRef*>(dlsym(avf, "AVMediaTypeAudio"));
    if (!pAVMediaTypeAudio) return false;

    Class cls = objc_getClass("AVCaptureDevice");
    if (!cls) return false;

    SEL sel = sel_registerName("requestAccessForMediaType:completionHandler:");
    // Dispatch async request with dummy completion block
    auto msgSend = reinterpret_cast<void (*)(Class, SEL, CFStringRef, void*)>(objc_msgSend);
    msgSend(cls, sel, *pAVMediaTypeAudio, nullptr);
    return true;
#else
    return true;
#endif
}

std::string AudioRecorder::propPermissionStatus() const {
#if defined(__APPLE__)
    void* avf = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY);
    if (!avf) return "authorized";

    CFStringRef* pAVMediaTypeAudio = reinterpret_cast<CFStringRef*>(dlsym(avf, "AVMediaTypeAudio"));
    if (!pAVMediaTypeAudio) return "authorized";

    Class cls = objc_getClass("AVCaptureDevice");
    if (!cls) return "authorized";

    SEL sel = sel_registerName("authorizationStatusForMediaType:");
    auto msgSend = reinterpret_cast<long (*)(Class, SEL, CFStringRef)>(objc_msgSend);
    long status = msgSend(cls, sel, *pAVMediaTypeAudio);
    switch (status) {
        case 0: return "not_determined";
        case 1: return "restricted";
        case 2: return "denied";
        case 3: return "authorized";
        default: return "unknown";
    }
#else
    return "not_applicable";
#endif
}

std::string AudioRecorder::propPermissionDetails() const {
#if defined(__APPLE__)
    std::string s = propPermissionStatus();
    if (s == "authorized") {
        return "Microphone access is authorized by macOS TCC.";
    } else if (s == "denied") {
        return "Microphone access is denied. To enable in-world voice capture, open System Settings > Privacy & Security > Microphone and permit Earthcall / Terminal.";
    } else if (s == "not_determined") {
        return "Microphone permission has not yet been requested from the user.";
    } else {
        return "Microphone permission is restricted by OS policy.";
    }
#else
    return "Microphone permissions checks not applicable on this platform.";
#endif
}

// ---------------------------------------------------------------------------
// miniaudio Data Callback & AudioRecorder Core
// ---------------------------------------------------------------------------

static void miniaudioCaptureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    AudioRecorder* recorder = static_cast<AudioRecorder*>(pDevice->pUserData);
    if (!recorder || !pInput || frameCount == 0) return;

    size_t sampleCount = static_cast<size_t>(frameCount) * pDevice->capture.channels;
    recorder->feedSamples(static_cast<const float*>(pInput), sampleCount);
}

AudioRecorder::AudioRecorder() : Law("microphone") {
    setName("Microphone");
    setLawIdentifier("microphone");
    _enabled = true;
    _recording = false;
    _paused = false;
    _sampleRate = 48000.0;
    _channels = 1.0;
    _gain = 1.0;
    _speechThreshold = 0.05;
    _status = "idle";
    _outputPath = "saves/recordings";
    _fallbackToSimulated = true;
    _forceSimulated = false;
    buildProperties();
}

AudioRecorder::~AudioRecorder() {
    if (_recording) {
        stopRecording();
    }
    shutdownCaptureDevice();
}

void AudioRecorder::syncRegister(LawManager& laws) {
    if (laws.find("microphone")) return;

    auto mic = std::make_shared<AudioRecorder>();
    laws.add(mic);
}

AudioRecorder* AudioRecorder::find(LawManager& laws) {
    return dynamic_cast<AudioRecorder*>(laws.find("microphone"));
}

std::vector<std::string> AudioRecorder::getAvailableDevices() const {
    std::vector<std::string> list;
    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        list.push_back("Default System Microphone");
        return list;
    }

    ma_device_info* pCaptureInfos = nullptr;
    ma_uint32 captureCount = 0;
    if (ma_context_get_devices(&context, NULL, NULL, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            std::string dName = pCaptureInfos[i].name;
            if (pCaptureInfos[i].isDefault) {
                dName += " (Default)";
            }
            list.push_back(dName);
        }
    }
    ma_context_uninit(&context);

    if (list.empty()) {
        list.push_back("Default System Microphone");
    }
    return list;
}

void AudioRecorder::setDevice(const std::string& deviceName) {
    _selectedDevice = deviceName;
    if (_recording) {
        // Re-initialize capture device on new hardware
        shutdownCaptureDevice();
        initCaptureDevice();
    }
}

std::string AudioRecorder::ensureSessionFile() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm gm_tm;
#if defined(_WIN32)
    gmtime_s(&gm_tm, &t);
#else
    gmtime_r(&t, &gm_tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "mic_%Y%m%d_%H%M%S.wav", &gm_tm);

    fs::path baseDir = _outputPath.empty() ? "saves/recordings" : _outputPath;
    fs::path sessionFile = baseDir / buf;

    std::error_code ec;
    fs::create_directories(sessionFile.parent_path(), ec);
    return sessionFile.string();
}

bool AudioRecorder::initCaptureDevice() {
    shutdownCaptureDevice();

    if (_forceSimulated) {
        _usingSimulation = true;
        return true;
    }

    // Permission check
    if (!hasMicrophonePermission()) {
        if (_fallbackToSimulated) {
            _lastError = "macOS Microphone permission denied. Using synthetic audio fallback for testing.";
            _usingSimulation = true;
            return true;
        } else {
            _status = "error: microphone permission denied";
            _lastError = "macOS Microphone permission denied. Enable in System Settings > Privacy & Security > Microphone.";
            return false;
        }
    }

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = static_cast<ma_uint32>(_channels > 1.5 ? 2 : 1);
    config.sampleRate = static_cast<ma_uint32>(_sampleRate);
    config.dataCallback = miniaudioCaptureCallback;
    config.pUserData = this;

    ma_device* pDev = new ma_device();
    ma_result res = ma_device_init(NULL, &config, pDev);
    if (res != MA_SUCCESS) {
        delete pDev;
        if (_fallbackToSimulated) {
            _lastError = "Hardware capture device unavailable. Falling back to synthetic simulation.";
            _usingSimulation = true;
            return true;
        } else {
            _status = "error: capture device init failed";
            _lastError = "Failed to initialize miniaudio capture device";
            return false;
        }
    }

    res = ma_device_start(pDev);
    if (res != MA_SUCCESS) {
        ma_device_uninit(pDev);
        delete pDev;
        if (_fallbackToSimulated) {
            _usingSimulation = true;
            return true;
        }
        _status = "error: failed to start device";
        return false;
    }

    _pDevice = pDev;
    _usingSimulation = false;
    return true;
}

void AudioRecorder::shutdownCaptureDevice() {
    if (_pDevice) {
        ma_device* pDev = static_cast<ma_device*>(_pDevice);
        ma_device_stop(pDev);
        ma_device_uninit(pDev);
        delete pDev;
        _pDevice = nullptr;
    }
    _usingSimulation = false;
}

void AudioRecorder::feedSamples(const float* pSamples, size_t sampleCount) {
    if (!_recording || _paused || !pSamples || sampleCount == 0) return;

    std::lock_guard<std::mutex> lock(_sampleMutex);
    double sumSquares = 0.0;
    double maxPeak = 0.0;

    for (size_t i = 0; i < sampleCount; ++i) {
        float s = pSamples[i] * static_cast<float>(_gain);
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;

        _recordedSamples.push_back(s);

        double absVal = std::fabs(static_cast<double>(s));
        sumSquares += absVal * absVal;
        if (absVal > maxPeak) maxPeak = absVal;
    }

    double rms = std::sqrt(sumSquares / static_cast<double>(sampleCount));
    _inputLevel = rms;
    if (maxPeak > _peakLevel) {
        _peakLevel = maxPeak;
    } else {
        _peakLevel = _peakLevel * 0.95; // Smooth decay
    }

    // Voice / speech edge detection
    bool isSpeaking = (rms >= _speechThreshold);
    if (isSpeaking != _lastSpeakingState) {
        _lastSpeakingState = isSpeaking;
        Core::EventBus::instance().publish(ECA::Event{
            isSpeaking ? "speech-detected" : "silence-detected",
            this,
            nullptr,
            Moment{}
        });
    }
}

bool AudioRecorder::startRecording(const std::string& customPath) {
    if (!_enabled) {
        _status = "error: recorder disabled";
        _lastError = "Audio recorder is disabled";
        return false;
    }

    if (_recording) return true;

    {
        std::lock_guard<std::mutex> lock(_sampleMutex);
        _recordedSamples.clear();
    }

    _activeFilePath = customPath.empty() ? ensureSessionFile() : customPath;
    _startTime = std::chrono::steady_clock::now();
    _recordedDuration = 0.0;
    _sampleCount = 0.0;
    _bytesRecorded = 0.0;
    _inputLevel = 0.0;
    _peakLevel = 0.0;
    _simPhase = 0.0;

    if (!initCaptureDevice()) {
        return false;
    }

    _recording = true;
    _paused = false;
    _status = "recording";
    _lastError = "";

    // Emit past-tense ECA edge event
    Core::EventBus::instance().publish(ECA::Event{
        "recording-started",
        this,
        nullptr,
        Moment{}
    });

    return true;
}

bool AudioRecorder::stopRecording() {
    if (!_recording) return true;

    shutdownCaptureDevice();
    _recording = false;
    _paused = false;
    _status = "finalizing";

    std::vector<float> toSave;
    {
        std::lock_guard<std::mutex> lock(_sampleMutex);
        toSave.swap(_recordedSamples);
    }

    bool ok = writeWav(
        _activeFilePath,
        toSave.data(),
        toSave.size(),
        static_cast<int>(_sampleRate),
        static_cast<int>(_channels)
    );

    if (ok) {
        _lastRecordingPath = _activeFilePath;
        _bytesRecorded = static_cast<double>(sizeof(WavHeader) + toSave.size() * sizeof(int16_t));
        _sampleCount = static_cast<double>(toSave.size());
        _status = "idle";
        _lastError = "";

        // Emit past-tense ECA edge event
        Core::EventBus::instance().publish(ECA::Event{
            "recording-stopped",
            this,
            nullptr,
            Moment{}
        });
        return true;
    } else {
        _status = "error: failed to write wav";
        _lastError = "Failed to write WAV file to: " + _activeFilePath;
        return false;
    }
}

bool AudioRecorder::pauseRecording() {
    if (!_recording || _paused) return false;
    _paused = true;
    _status = "paused";
    return true;
}

bool AudioRecorder::resumeRecording() {
    if (!_recording || !_paused) return false;
    _paused = false;
    _status = "recording";
    return true;
}

void AudioRecorder::tick(double dt) {
    if (!_recording || _paused) return;

    _recordedDuration += dt;

    // Headless synthetic simulation generator
    if (_usingSimulation) {
        size_t samplesToGen = static_cast<size_t>(_sampleRate * dt);
        if (samplesToGen > 0) {
            std::vector<float> simBuffer(samplesToGen);
            double freq = 440.0; // 440 Hz concert A test tone
            double phaseInc = (2.0 * 3.14159265358979323846 * freq) / _sampleRate;

            for (size_t i = 0; i < samplesToGen; ++i) {
                simBuffer[i] = static_cast<float>(0.3 * std::sin(_simPhase));
                _simPhase += phaseInc;
                if (_simPhase > 2.0 * 3.14159265358979323846) {
                    _simPhase -= 2.0 * 3.14159265358979323846;
                }
            }
            feedSamples(simBuffer.data(), simBuffer.size());
        }
    }

    {
        std::lock_guard<std::mutex> lock(_sampleMutex);
        _sampleCount = static_cast<double>(_recordedSamples.size());
    }
}

// ---------------------------------------------------------------------------
// Law Property Handlers & Triggers
// ---------------------------------------------------------------------------

void AudioRecorder::propSetRecording(const bool& v) {
    if (v && !_recording) {
        startRecording();
    } else if (!v && _recording) {
        stopRecording();
    }
}

void AudioRecorder::propSetPaused(const bool& v) {
    if (v && !_paused) {
        pauseRecording();
    } else if (!v && _paused) {
        resumeRecording();
    }
}

void AudioRecorder::propSetStartTrigger(const bool& v) {
    _startTrigger = v;
    if (_startTrigger) {
        startRecording();
        _startTrigger = false;
    }
}

void AudioRecorder::propSetStopTrigger(const bool& v) {
    _stopTrigger = v;
    if (_stopTrigger) {
        stopRecording();
        _stopTrigger = false;
    }
}

void AudioRecorder::propSetPauseTrigger(const bool& v) {
    _pauseTrigger = v;
    if (_pauseTrigger) {
        pauseRecording();
        _pauseTrigger = false;
    }
}

void AudioRecorder::propSetResumeTrigger(const bool& v) {
    _resumeTrigger = v;
    if (_resumeTrigger) {
        resumeRecording();
        _resumeTrigger = false;
    }
}

void AudioRecorder::propSetRefreshDevicesTrigger(const bool& v) {
    _refreshDevicesTrigger = v;
    if (_refreshDevicesTrigger) {
        getAvailableDevices();
        _refreshDevicesTrigger = false;
    }
}

void AudioRecorder::propSetRequestPermissionTrigger(const bool& v) {
    _requestPermissionTrigger = v;
    if (_requestPermissionTrigger) {
        requestMicrophonePermission();
        _requestPermissionTrigger = false;
    }
}

std::string AudioRecorder::propDevices() const {
    auto devs = getAvailableDevices();
    std::string out;
    for (size_t i = 0; i < devs.size(); ++i) {
        if (i > 0) out += ", ";
        out += devs[i];
    }
    return out;
}

double AudioRecorder::propDeviceCount() const {
    return static_cast<double>(getAvailableDevices().size());
}

void AudioRecorder::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.enabled", this, &AudioRecorder::propEnabled, &AudioRecorder::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.recording", this, &AudioRecorder::propRecording, &AudioRecorder::propSetRecording));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.paused", this, &AudioRecorder::propPaused, &AudioRecorder::propSetPaused));

    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.selectedDevice", this, &AudioRecorder::propSelectedDevice, &AudioRecorder::propSetSelectedDevice));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.sampleRate", this, &AudioRecorder::propSampleRate, &AudioRecorder::propSetSampleRate));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.channels", this, &AudioRecorder::propChannels, &AudioRecorder::propSetChannels));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.gain", this, &AudioRecorder::propGain, &AudioRecorder::propSetGain));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.speechThreshold", this, &AudioRecorder::propSpeechThreshold, &AudioRecorder::propSetSpeechThreshold));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.outputPath", this, &AudioRecorder::propOutputPath, &AudioRecorder::propSetOutputPath));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.fallbackToSimulated", this, &AudioRecorder::propFallbackToSimulated, &AudioRecorder::propSetFallbackToSimulated));

    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.start", this, &AudioRecorder::propStartTrigger, &AudioRecorder::propSetStartTrigger));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.stop", this, &AudioRecorder::propStopTrigger, &AudioRecorder::propSetStopTrigger));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.pause", this, &AudioRecorder::propPauseTrigger, &AudioRecorder::propSetPauseTrigger));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.resume", this, &AudioRecorder::propResumeTrigger, &AudioRecorder::propSetResumeTrigger));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.refreshDevices", this, &AudioRecorder::propRefreshDevicesTrigger, &AudioRecorder::propSetRefreshDevicesTrigger));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.requestPermission", this, &AudioRecorder::propRequestPermissionTrigger, &AudioRecorder::propSetRequestPermissionTrigger));

    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.status", this, &AudioRecorder::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.lastError", this, &AudioRecorder::propLastError, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.inputLevel", this, &AudioRecorder::propInputLevel, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.peakLevel", this, &AudioRecorder::propPeakLevel, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.recordedDuration", this, &AudioRecorder::propRecordedDuration, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.sampleCount", this, &AudioRecorder::propSampleCount, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.bytesRecorded", this, &AudioRecorder::propBytesRecorded, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.lastRecordingPath", this, &AudioRecorder::propLastRecordingPath, nullptr));

    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.devices", this, &AudioRecorder::propDevices, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, double>>(
        "mic.deviceCount", this, &AudioRecorder::propDeviceCount, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, bool>>(
        "mic.hasPermission", this, &AudioRecorder::propHasPermission, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.permissionStatus", this, &AudioRecorder::propPermissionStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<AudioRecorder, std::string>>(
        "mic.permissionDetails", this, &AudioRecorder::propPermissionDetails, nullptr));
}

} // namespace Audio
} // namespace Singularity
