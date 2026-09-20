#include "Singularity/Audio/AudioChannel.hpp"

#include "Singularity/Audio/AudioSystem.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "json.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace Singularity {
namespace Audio {

namespace {

bool readString(Singular& being, const char* path, std::string& out) {
    PropertyValue value;
    if (!lawGetValue(being, PropertyPath::parse(path), value)) return false;
    const auto* text = std::get_if<std::string>(&value);
    if (!text) return false;
    out = *text;
    return true;
}

bool readNumber(Singular& being, const char* path, double& out) {
    PropertyValue value;
    return lawGetValue(being, PropertyPath::parse(path), value) &&
           propertyValueToNumber(value, out);
}

Singular* findBeing(const std::string& rawId) {
    if (rawId.empty()) return nullptr;
    const std::string id = rawId.front() == '@' ? rawId.substr(1) : rawId;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

bool isLegacyWave(const std::string& timbre) {
    return timbre.empty() || timbre == "sine" || timbre == "triangle" ||
           timbre == "square" || timbre == "sawtooth";
}

} // namespace

AudioChannel::AudioChannel() = default;

AudioChannel::~AudioChannel() {
    // The sink captures this channel. A torn-down test LawManager must not
    // leave a static callback pointing into freed memory.
    registerAudioSink(nullptr);
}

void AudioChannel::syncRegister(LawManager& laws) {
    if (auto* existing = find(laws)) {
        existing->bindSink();
        existing->applyGovernance();
        return;
    }

    auto channel = std::make_shared<AudioChannel>();
    AudioChannel* raw = channel.get();
    laws.add(channel);
    raw->bindSink();
    raw->applyGovernance();
}

AudioChannel* AudioChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<AudioChannel*>(law.get())) return channel;
    }
    return nullptr;
}

void AudioChannel::bindSink() {
    registerAudioSinkChecked(
        [this](Singular& subject, double frequency, double amplitude,
               const std::string& timbre, std::string& reason) {
            return sound(subject, frequency, amplitude, timbre, reason);
        });
}

void AudioChannel::applyGovernance() {
    auto& system = Core::Audio::AudioSystem::instance();
    system.setOutputEnabled(isEnabled());
    system.setMasterGain(_masterGain);
}

void AudioChannel::setMasterGain(const double& value) {
    if (!std::isfinite(value)) return;
    _masterGain = std::max(0.0, value);
    Core::Audio::AudioSystem::instance().setMasterGain(_masterGain);
}

int AudioChannel::actualSampleRate() const {
    return Core::Audio::AudioSystem::instance().actualSampleRate();
}

int AudioChannel::activeVoices() const {
    return Core::Audio::AudioSystem::instance().activeVoiceCount();
}

bool AudioChannel::backendInitialized() const {
    return Core::Audio::AudioSystem::instance().initialized();
}

bool AudioChannel::resolveAuthoredForm(Singular& subject,
                                       const std::string& timbre,
                                       OntoMath::Piecewise& form,
                                       std::string& timeVariable,
                                       double& referenceFrequency,
                                       double& duration,
                                       std::string& reason) {
    Singular* carrier = nullptr;
    std::string serialized;

    // A sounding being may author its own waveform directly.
    if (readString(subject, "acoustic.form", serialized) && !serialized.empty()) {
        carrier = &subject;
    } else if (!timbre.empty()) {
        // Otherwise the authored timbre token is an identity, not an enum.
        carrier = findBeing(timbre);
        if (carrier) {
            if (!readString(*carrier, "acoustic.form", serialized) ||
                serialized.empty()) {
                reason = "timbre '" + timbre +
                         "' resolves to a being with no acoustic.form";
                return false;
            }
        }
    }

    if (!carrier) {
        reason = "no authored acoustic.form resolved";
        return false;
    }

    const nlohmann::json parsed =
        nlohmann::json::parse(serialized, nullptr, false);
    if (parsed.is_discarded()) {
        reason = "authored acoustic.form is not valid OntoMath JSON";
        return false;
    }

    form = OntoMath::Piecewise::fromJson(parsed);
    timeVariable = form.inputVariable.empty() ? "phase" : form.inputVariable;
    std::string authoredTimeVar;
    if (readString(*carrier, "acoustic.timeVariable", authoredTimeVar) &&
        !authoredTimeVar.empty()) {
        timeVariable = authoredTimeVar;
    }

    referenceFrequency = 1.0;
    double authoredReference = 0.0;
    if (readNumber(*carrier, "acoustic.referenceFrequency", authoredReference)) {
        if (!std::isfinite(authoredReference) || authoredReference <= 0.0) {
            reason = "acoustic.referenceFrequency must be finite and positive";
            return false;
        }
        referenceFrequency = authoredReference;
    }

    duration = 0.35;
    double authoredDuration = 0.0;
    if (readNumber(*carrier, "acoustic.duration", authoredDuration)) {
        if (!std::isfinite(authoredDuration) || authoredDuration <= 0.0) {
            reason = "acoustic.duration must be finite and positive";
            return false;
        }
        duration = authoredDuration;
    }

    return true;
}

bool AudioChannel::sound(Singular& subject, double frequency, double amplitude,
                         const std::string& timbre, std::string& reason) {
    applyGovernance();

    if (!isEnabled()) {
        reason = "audio channel disabled";
        _lastTimbreStatus = reason;
        return false;
    }
    if (!std::isfinite(frequency) || frequency <= 0.0) {
        reason = "frequency must be finite and positive";
        _lastTimbreStatus = reason;
        return false;
    }
    if (!std::isfinite(amplitude) || amplitude < 0.0) {
        reason = "amplitude must be finite and non-negative";
        _lastTimbreStatus = reason;
        return false;
    }

    auto& system = Core::Audio::AudioSystem::instance();
    if (!system.initialized()) {
        reason = "audio backend is not initialized";
        _lastTimbreStatus = reason;
        return false;
    }

    OntoMath::Piecewise form;
    std::string timeVariable;
    double referenceFrequency = 1.0;
    double duration = 0.35;
    std::string authoredReason;

    if (resolveAuthoredForm(subject, timbre, form, timeVariable,
                            referenceFrequency, duration, authoredReason)) {
        ++_authoredFormParses;

        glm::vec3 position(0.0f);
        const glm::vec3* positionPtr = nullptr;
        if (auto* object = dynamic_cast<Object*>(&subject)) {
            position = object->getPosition();
            positionPtr = &position;
        }

        // The authored form describes its own reference-rate waveform. Pitch
        // is a Law-supplied parameter that retimes that same mathematics rather
        // than selecting a different engine preset.
        const double timeScale = frequency / referenceFrequency;
        const bool sounded = system.playForm(
            form, timeVariable, duration, static_cast<float>(amplitude),
            positionPtr, {}, timeScale);

        if (!sounded) {
            reason = "authored timbre '" + timbre +
                     "' was refused or rendered no samples";
            _lastTimbreStatus = reason;
            return false;
        }

        ++_authoredTimbresSounded;
        _lastTimbreStatus = timbre.empty() ? "authored-local" :
                                             "authored:" + timbre;
        return true;
    }

    // Compatibility floor for old saves only. Unknown words do NOT become
    // sine. A new timbre either resolves to authored structure or refuses.
    if (!isLegacyWave(timbre)) {
        ++_unresolvedTimbres;
        reason = authoredReason.empty()
                     ? "unresolved authored timbre '" + timbre + "'"
                     : "unresolved authored timbre '" + timbre + "': " +
                           authoredReason;
        _lastTimbreStatus = reason;
        return false;
    }

    glm::vec3 position(0.0f);
    if (auto* object = dynamic_cast<Object*>(&subject)) {
        position = object->getPosition();
    }

    const bool sounded = system.playProceduralCollisionSound(
        position, glm::vec3(0.0f), frequency, amplitude, timbre);
    if (!sounded) {
        reason = "legacy oscillator path refused '" +
                 (timbre.empty() ? std::string("sine") : timbre) + "'";
        _lastTimbreStatus = reason;
        return false;
    }

    ++_legacyTimbresSounded;
    _lastTimbreStatus = "legacy:" +
                        (timbre.empty() ? std::string("sine") : timbre);
    return true;
}

void AudioChannel::buildProperties() {
    registerEnabledProperty();

    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, double>>(
            "masterGain", this, &AudioChannel::masterGain,
            &AudioChannel::setMasterGain));

    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "actualSampleRate", this, &AudioChannel::actualSampleRate, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "activeVoices", this, &AudioChannel::activeVoices, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, bool>>(
            "backendInitialized", this, &AudioChannel::backendInitialized,
            nullptr));

    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "authoredFormParses", this, &AudioChannel::authoredFormParses,
            nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "authoredTimbresSounded", this,
            &AudioChannel::authoredTimbresSounded, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "legacyTimbresSounded", this,
            &AudioChannel::legacyTimbresSounded, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, int>>(
            "unresolvedTimbres", this, &AudioChannel::unresolvedTimbres,
            nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<AudioChannel, std::string>>(
            "lastTimbreStatus", this, &AudioChannel::lastTimbreStatus, nullptr));
}

} // namespace Audio
} // namespace Singularity
