#include "AudioSystem.hpp"
#include <iostream>
#include <vector>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Singularity/Core/Logger.hpp"

extern ZoneManager mgr;

namespace Core {
namespace Audio {

struct SpatialSoundInstance {
    ma_sound* sound = nullptr;
    ma_waveform* waveform = nullptr;
    // An OntoMath model sounded by playForm: miniaudio reads the samples for
    // as long as the sound plays, so the buffer outlives this call and is
    // released with the sound in tick().
    ma_audio_buffer* pcm = nullptr;
    std::vector<float>* samples = nullptr;
};

struct SoundEmitterInstance {
    Object* subject = nullptr;
    ma_sound* sound = nullptr;
    ma_waveform* waveform = nullptr;
    ma_waveform_type currentWaveType = ma_waveform_type_sine;
};

struct AudioSystem::AudioState {
    ma_engine engine;
    ma_sound musicSound;
    bool hasMusic = false;
    std::vector<SpatialSoundInstance*> activeSpatialSounds;
    std::vector<SoundEmitterInstance*> activeEmitters;
};

AudioSystem& AudioSystem::instance() {
    static AudioSystem s_instance;
    return s_instance;
}

bool AudioSystem::init() {
    if (_initialized) return true;

    _state = new AudioState();

    ma_result result = ma_engine_init(NULL, &_state->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "⚠️  Failed to initialize audio engine!" << std::endl;
        delete _state;
        _state = nullptr;
        return false;
    }

    _initialized = true;
    ECA::Logger::instance().log(ECA::LogCategory::Audio, "SYSTEM", "Audio System initialized");
    std::cout << "🎵 Audio System initialized." << std::endl;
    return true;
}

void AudioSystem::shutdown() {
    if (!_initialized) return;

    if (_state) {
        if (_state->hasMusic) {
            ma_sound_uninit(&_state->musicSound);
        }
        for (auto instance : _state->activeSpatialSounds) {
            ma_sound_uninit(instance->sound);
            delete instance->sound;
            if (instance->waveform) {
                ma_waveform_uninit(instance->waveform);
                delete instance->waveform;
            }
            if (instance->pcm) {
                ma_audio_buffer_uninit(instance->pcm);
                delete instance->pcm;
            }
            delete instance->samples;
            delete instance;
        }
        _state->activeSpatialSounds.clear();

        for (auto instance : _state->activeEmitters) {
            ma_sound_uninit(instance->sound);
            delete instance->sound;
            if (instance->waveform) {
                ma_waveform_uninit(instance->waveform);
                delete instance->waveform;
            }
            delete instance;
        }
        _state->activeEmitters.clear();

        ma_engine_uninit(&_state->engine);
        delete _state;
        _state = nullptr;
    }

    _initialized = false;
    ECA::Logger::instance().log(ECA::LogCategory::Audio, "SYSTEM", "Audio System shut down");
    std::cout << "🎵 Audio System shut down." << std::endl;
}

void AudioSystem::playSound(const std::string& filepath) {
    if (!_initialized || !_state) return;

    ma_engine_play_sound(&_state->engine, filepath.c_str(), NULL);
}

void AudioSystem::setupAudioEventListeners() {
    if (!_initialized) return;
    // We no longer subscribe to "audio-synthesized" - we use continuous sound-emitter objects.
}

void AudioSystem::tick() {
    if (!_initialized || !_state) return;

    glm::vec3 camPos = Integration::getEarthcallAPI().getCameraPosition();
    ma_engine_listener_set_position(&_state->engine, 0, camPos.x, camPos.y, camPos.z);

    for (auto it = _state->activeSpatialSounds.begin(); it != _state->activeSpatialSounds.end(); ) {
        if (!ma_sound_is_playing((*it)->sound)) {
            ma_sound_uninit((*it)->sound);
            delete (*it)->sound;
            if ((*it)->waveform) {
                ma_waveform_uninit((*it)->waveform);
                delete (*it)->waveform;
            }
            if ((*it)->pcm) {
                ma_audio_buffer_uninit((*it)->pcm);
                delete (*it)->pcm;
            }
            delete (*it)->samples;
            delete *it;
            it = _state->activeSpatialSounds.erase(it);
        } else {
            ++it;
        }
    }

    // Process SoundEmitters
    auto& objects = mgr.active().getOwnedObjects();
    
    // Find missing emitters (objects destroyed)
    for (auto it = _state->activeEmitters.begin(); it != _state->activeEmitters.end(); ) {
        bool found = false;
        for (const auto& up : objects) {
            if (up.get() == (*it)->subject) {
                found = true;
                break;
            }
        }
        if (!found) {
            ma_sound_uninit((*it)->sound);
            delete (*it)->sound;
            if ((*it)->waveform) {
                ma_waveform_uninit((*it)->waveform);
                delete (*it)->waveform;
            }
            delete *it;
            it = _state->activeEmitters.erase(it);
        } else {
            ++it;
        }
    }

    // Update existing or create new
    for (const auto& up : objects) {
        Object* obj = up.get();
        if (!obj) continue;

        PropertyValue isEmitterVal;
        if (!lawGetValue(*obj, PropertyPath::parse("acoustic.isSoundEmitter"), isEmitterVal)) continue;

        bool isEmitter = false;
        if (std::holds_alternative<std::string>(isEmitterVal) && std::get<std::string>(isEmitterVal) == "true") {
            isEmitter = true;
        } else if (std::holds_alternative<bool>(isEmitterVal) && std::get<bool>(isEmitterVal)) {
            isEmitter = true;
        }
        if (!isEmitter) continue;

        // It is an emitter! Read properties
        double frequency = 440.0;
        double amplitude = 1.0;
        std::string waveTypeStr = "sine";

        PropertyValue pv;
        if (lawGetValue(*obj, PropertyPath::parse("acoustic.frequency"), pv)) {
            if (std::holds_alternative<double>(pv)) frequency = std::get<double>(pv);
            else if (std::holds_alternative<int>(pv)) frequency = static_cast<double>(std::get<int>(pv));
            else if (std::holds_alternative<std::string>(pv)) {
                try { frequency = std::stod(std::get<std::string>(pv)); } catch(...) {}
            }
        }

        if (lawGetValue(*obj, PropertyPath::parse("acoustic.amplitude"), pv)) {
            if (std::holds_alternative<double>(pv)) amplitude = std::get<double>(pv);
            else if (std::holds_alternative<int>(pv)) amplitude = static_cast<double>(std::get<int>(pv));
            else if (std::holds_alternative<std::string>(pv)) {
                try { amplitude = std::stod(std::get<std::string>(pv)); } catch(...) {}
            }
        }

        if (lawGetValue(*obj, PropertyPath::parse("acoustic.waveType"), pv) && std::holds_alternative<std::string>(pv)) {
            waveTypeStr = std::get<std::string>(pv);
        }

        ma_waveform_type waveType = ma_waveform_type_sine;
        if (waveTypeStr == "triangle") waveType = ma_waveform_type_triangle;
        else if (waveTypeStr == "square") waveType = ma_waveform_type_square;
        else if (waveTypeStr == "sawtooth") waveType = ma_waveform_type_sawtooth;

        // 2. Ontological Occlusion (Muffling)
        if (lawGetValue(*obj, PropertyPath::parse("acoustic.lowpassCutoff"), pv)) {
            double cutoff = 22000.0;
            if (std::holds_alternative<double>(pv)) cutoff = std::get<double>(pv);
            else if (std::holds_alternative<int>(pv)) cutoff = static_cast<double>(std::get<int>(pv));
            
            if (cutoff < 10000.0) {
                // Simulate muffling without a real DSP filter by forcing to sine and dropping amplitude
                waveType = ma_waveform_type_sine;
                amplitude *= (cutoff / 10000.0);
            }
        }

        // Find if we already have it
        SoundEmitterInstance* instance = nullptr;
        for (auto inst : _state->activeEmitters) {
            if (inst->subject == obj) {
                instance = inst;
                break;
            }
        }

        if (!instance) {
            // Create new
            instance = new SoundEmitterInstance();
            instance->subject = obj;
            instance->currentWaveType = waveType;

            ma_waveform_config config = ma_waveform_config_init(
                _state->engine.pDevice->playback.format,
                _state->engine.pDevice->playback.channels,
                _state->engine.pDevice->sampleRate,
                waveType,
                amplitude,
                frequency
            );

            instance->waveform = new ma_waveform();
            ma_waveform_init(&config, instance->waveform);

            instance->sound = new ma_sound();
            ma_sound_init_from_data_source(&_state->engine, instance->waveform, 0, NULL, instance->sound);
            
            glm::vec3 pos = obj->getPosition();
            ma_sound_set_position(instance->sound, pos.x, pos.y, pos.z);
            ma_sound_start(instance->sound);
            
            _state->activeEmitters.push_back(instance);
        } else {
            // Update existing
            if (instance->currentWaveType != waveType) {
                ma_waveform_set_type(instance->waveform, waveType);
                instance->currentWaveType = waveType;
            }
            ma_waveform_set_amplitude(instance->waveform, amplitude);
            ma_waveform_set_frequency(instance->waveform, frequency);
            
            glm::vec3 pos = obj->getPosition();
            ma_sound_set_position(instance->sound, pos.x, pos.y, pos.z);
        }
    }
}

void AudioSystem::playSpatialSound(const std::string& filepath, const glm::vec3& position, float volume) {
    if (!_initialized || !_state) return;

    ma_sound* sound = new ma_sound();
    ma_result result = ma_sound_init_from_file(
        &_state->engine, 
        filepath.c_str(), 
        0, 
        NULL, 
        NULL, 
        sound
    );

    if (result == MA_SUCCESS) {
        ma_sound_set_position(sound, position.x, position.y, position.z);
        ma_sound_set_volume(sound, volume);
        ma_sound_start(sound);
        
        SpatialSoundInstance* instance = new SpatialSoundInstance();
        instance->sound = sound;
        instance->waveform = nullptr;
        _state->activeSpatialSounds.push_back(instance);
    } else {
        delete sound;
    }
}

void AudioSystem::playProceduralCollisionSound(const glm::vec3& position, const glm::vec3& velocity, double frequency, double amplitude, const std::string& waveTypeStr) {
    if (!_initialized || !_state) return;

    glm::vec3 camPos = Integration::getEarthcallAPI().getCameraPosition();
    float distToCam = glm::distance(camPos, position);
    glm::vec3 rayDir = camPos - position;
    if (distToCam > 0.0001f) rayDir /= distToCam;

    // 1. Doppler Shift
    float speedOfSound = 343.0f;
    float sourceVelTowardsListener = glm::dot(velocity, rayDir);
    float dopplerFactor = speedOfSound / std::max(speedOfSound - sourceVelTowardsListener, 0.1f);

    frequency *= dopplerFactor;

    // The same infrasound floor renderForm enforces, on the path that HAS a
    // frequency to clamp. One constant, so the two cannot drift apart.
    if (frequency < kAudibleFloorHz) frequency = kAudibleFloorHz;
    if (frequency > 20000.0) frequency = 20000.0;

    if (amplitude > 1.0) amplitude = 1.0;
    if (amplitude < 0.0) amplitude = 0.0;

    ma_waveform_type waveType = ma_waveform_type_sine;
    if (waveTypeStr == "triangle") {
        waveType = ma_waveform_type_triangle;
    } else if (waveTypeStr == "square") {
        waveType = ma_waveform_type_square;
    } else if (waveTypeStr == "sawtooth") {
        waveType = ma_waveform_type_sawtooth;
    }

    ma_waveform_config config = ma_waveform_config_init(
        _state->engine.pDevice->playback.format,
        _state->engine.pDevice->playback.channels,
        _state->engine.pDevice->sampleRate,
        waveType,
        amplitude,
        frequency
    );

    ma_waveform* waveform = new ma_waveform();
    ma_result result = ma_waveform_init(&config, waveform);
    if (result != MA_SUCCESS) {
        delete waveform;
        return;
    }

    ma_sound* sound = new ma_sound();
    result = ma_sound_init_from_data_source(
        &_state->engine,
        waveform,
        0,
        NULL,
        sound
    );

    if (result == MA_SUCCESS) {
        ma_sound_set_position(sound, position.x, position.y, position.z);
        ma_sound_set_velocity(sound, velocity.x, velocity.y, velocity.z);
        
        // 3. Speed-of-sound delay
        ma_uint64 engineTimeMs = ma_engine_get_time_in_milliseconds(&_state->engine);
        ma_uint64 delayMs = static_cast<ma_uint64>((distToCam / speedOfSound) * 1000.0f);
        ma_sound_set_start_time_in_milliseconds(sound, engineTimeMs + delayMs);
        
        ma_sound_start(sound);

        ma_sound_set_stop_time_with_fade_in_milliseconds(sound, engineTimeMs + delayMs + 300, 300);

        SpatialSoundInstance* instance = new SpatialSoundInstance();
        instance->sound = sound;
        instance->waveform = waveform;
        _state->activeSpatialSounds.push_back(instance);
    } else {
        ma_waveform_uninit(waveform);
        delete waveform;
        delete sound;
    }
}

// ---------------------------------------------------------------------------
// OntoMath as sound. The mathematics is not translated into audio parameters —
// no frequency, no wave type, no envelope preset. The authored expression IS
// the waveform, sampled.
// ---------------------------------------------------------------------------
namespace {

constexpr double kTau = 6.283185307179586476925286766559;

// The lowest frequency one TERM actually produces.
//
// A TransFactor is kind(scale·var + shift), and OntoMath::sinusoid builds its
// scale as 2π·frequency, so each factor's frequency is scale/2π exactly. But a
// term is a PRODUCT of factors, and a product is not a mixture: by the
// product-to-sum identities, sin(a)·sin(b) is energy at |a−b| and a+b, and at
// neither a nor b. So the frequencies a term can reach are the combinations
// ±f₁±f₂…±fₙ, and the lowest of those in absolute value is the one that
// matters here.
//
// This is what separates a 2 Hz MODULATOR — sin(440t)·sin(2t), an ordinary
// tremolo whose energy sits at 438 and 442 Hz — from 2 Hz CONTENT. It also
// catches the reverse, which a per-factor reading would miss entirely:
// sin(440t)·sin(430t) has all its factors comfortably audible and puts a
// 10 Hz difference tone into the room.
double lowestFrequencyOfTerm(const OntoMath::Term& term, const std::string& var) {
    std::vector<double> frequencies;
    for (const auto& factor : term.trans) {
        if (factor.variable != var) continue;
        if (factor.kind != OntoMath::TransFactor::Kind::Sin &&
            factor.kind != OntoMath::TransFactor::Kind::Cos) {
            continue;
        }
        const double hz = std::fabs(factor.scale) / kTau;
        if (hz > 0.0) frequencies.push_back(hz);
    }
    if (frequencies.empty()) return 0.0;

    // 2^n sign choices. Deeply nested products are not something an author
    // writes by hand, and past the ceiling the measurement pass is the honest
    // instrument anyway — so refuse to enumerate rather than take all day.
    constexpr std::size_t kMaxFactors = 12;
    if (frequencies.size() > kMaxFactors) return 0.0;

    double lowest = -1.0;
    const std::size_t combinations = std::size_t(1) << frequencies.size();
    for (std::size_t mask = 0; mask < combinations; ++mask) {
        double sum = 0.0;
        for (std::size_t i = 0; i < frequencies.size(); ++i) {
            sum += (mask & (std::size_t(1) << i)) ? -frequencies[i] : frequencies[i];
        }
        const double magnitude = std::fabs(sum);
        if (lowest < 0.0 || magnitude < lowest) lowest = magnitude;
    }
    return lowest < 0.0 ? 0.0 : lowest;
}

// The lowest sinusoid in one ScalarForm, read straight off the text — no
// spectrum estimation anywhere in the path. Terms whose coefficient is too
// small to reach the body are skipped.
double lowestSinusoidIn(const OntoMath::ScalarForm& form, const std::string& var,
                        double minCoefficient) {
    double lowest = 0.0;
    for (const auto& term : form.terms) {
        if (std::fabs(term.coefficient) < minCoefficient) continue;
        const double hz = lowestFrequencyOfTerm(term, var);
        if (hz <= 0.0) continue;
        if (lowest == 0.0 || hz < lowest) lowest = hz;
    }
    return lowest;
}

void walkForLowest(const OntoMath::MathNode& node, const std::string& var,
                   double minCoefficient, double& lowest) {
    if (node.op == OntoMath::MathNode::Op::ScalarLeaf) {
        const double hz = lowestSinusoidIn(node.scalarForm, var, minCoefficient);
        if (hz > 0.0 && (lowest == 0.0 || hz < lowest)) lowest = hz;
    }
    for (const auto& child : node.children) {
        if (child) walkForLowest(*child, var, minCoefficient, lowest);
    }
}

// Where the MEASUREMENT band ends, which is deliberately not where the
// doctrine line sits. Any Butterworth passes -3 dB at its own corner, so a
// filter cornered at 20 Hz reads a legitimate 20-25 Hz bass note as half
// infrasonic and the guard would refuse real music. The corner therefore sits
// below the line, and the two passes divide the work:
//
//   * an authored SINUSOID anywhere below 20 Hz is caught exactly, by reading
//     the text — no filter is involved and none of this applies;
//   * the filter is the backstop for energy the text cannot describe (DC, a
//     ramp, a step train, a fold), and it is aimed at the deep band where the
//     physiological effects and the cone-destroying excursion actually live.
//
// Measured behaviour of the cascade below, on a 0.9-amplitude tone: DC through
// 18 Hz is refused on energy alone, and 19.9 Hz upward passes. So the two
// passes together leave no gap — the text-reader covers every authored
// sinusoid below 20 Hz exactly, and the filter covers everything else down to
// within a hair of the same line, without touching real bass.
constexpr double kInfrasonicMeasureHz = 12.0;

// One 2nd-order low-pass section, Direct Form I.
struct Biquad {
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    static Biquad lowPass(double cutoffHz, double sampleRate, double q) {
        Biquad section;
        const double w = kTau * cutoffHz / sampleRate;
        const double cosw = std::cos(w);
        const double alpha = std::sin(w) / (2.0 * q);
        const double a0 = 1.0 + alpha;
        section.b0 = ((1.0 - cosw) * 0.5) / a0;
        section.b1 = (1.0 - cosw) / a0;
        section.b2 = section.b0;
        section.a1 = (-2.0 * cosw) / a0;
        section.a2 = (1.0 - alpha) / a0;
        return section;
    }

    void run(std::vector<double>& signal) const {
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        for (double& sample : signal) {
            const double x0 = sample;
            const double y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x0;
            y2 = y1; y1 = y0;
            sample = y0;
        }
    }
};

}   // namespace

double measureInfrasonicRms(const std::vector<float>& samples, int sampleRate) {
    if (samples.empty() || sampleRate <= 0) return 0.0;

    const auto rms = [](const std::vector<double>& signal) {
        double sum = 0.0;
        for (const double sample : signal) sum += sample * sample;
        return std::sqrt(sum / static_cast<double>(signal.size()));
    };

    std::vector<double> band(samples.begin(), samples.end());
    const double fs = static_cast<double>(sampleRate);

    const double denominator = static_cast<double>(band.size() - 1);
    const auto hann = [denominator](std::size_t i) {
        return 0.5 * (1.0 - std::cos(kTau * static_cast<double>(i) / denominator));
    };

    // One cycle of the measurement corner is the shortest buffer in which
    // sub-corner content is even a meaningful question — a filter cannot see a
    // period it never contains. Below that the only honest low-frequency
    // statistic is the offset, which is also the one that matters, since a
    // short buffer's dangerous failure mode is DC holding the cone off centre.
    //
    // Weighted by the same window, deliberately. A raw mean of a short slice
    // is dominated by whichever partial cycle the slice happens to end on, so
    // an ordinary 110 Hz blip reads as a DC offset it does not have; weighting
    // suppresses the ragged edges while leaving a true offset untouched.
    const std::size_t cycle = static_cast<std::size_t>(fs / kInfrasonicMeasureHz);
    if (band.size() <= cycle) {
        if (band.size() < 2) return std::fabs(band.front());
        double weighted = 0.0, weight = 0.0;
        for (std::size_t i = 0; i < band.size(); ++i) {
            weighted += hann(i) * band[i];
            weight += hann(i);
        }
        return weight > 0.0 ? std::fabs(weighted / weight) : 0.0;
    }

    // Taper the buffer to zero at both ends before filtering (Hann). A buffer
    // is a slice out of a longer sound, and its abrupt edges are an artifact
    // of the slicing, not content: fed to a filter starting from rest, that
    // step injects far more low-frequency energy than the signal contains —
    // enough to read a clean 25 Hz bass note as infrasound. Reflecting the
    // edges instead is worse, because reflecting about a non-zero endpoint
    // lays down a DC pedestal the filter then smears back inward. A window
    // MODULATES rather than adds, so a 4 kHz tone stays at 4 kHz and a DC
    // offset stays at DC — which is exactly the distinction being measured.
    for (std::size_t i = 0; i < band.size(); ++i) band[i] *= hann(i);

    // Isolate the deep band. Two Butterworth sections (4th order) run forward
    // and then backward — zero phase, 8th order effective, ~48 dB/octave — so
    // a 25 Hz bass note is not mistaken for infrasound and a 7 Hz one cannot
    // slip through a gentle rolloff. Doubles throughout: a 12 Hz corner at
    // 48 kHz has coefficients far too small to survive in float.
    const Biquad first = Biquad::lowPass(kInfrasonicMeasureHz, fs, 0.541196100146197);
    const Biquad second = Biquad::lowPass(kInfrasonicMeasureHz, fs, 1.306562964876377);
    for (const Biquad& section : {first, second}) {
        section.run(band);
        std::reverse(band.begin(), band.end());
        section.run(band);
        std::reverse(band.begin(), band.end());
    }

    // The window removed a known fraction of the signal's power (a Hann window
    // has coherent power 3/8), so put it back — the answer must be the band's
    // RMS in the original signal, not in the windowed copy.
    constexpr double kHannPowerGain = 0.6123724356957945;   // sqrt(3/8)
    return rms(band) / kHannPowerGain;
}

double lowestAuthoredFrequency(const OntoMath::Piecewise& form,
                               const std::string& timeVariable,
                               double minCoefficient) {
    double lowest = 0.0;
    for (const auto& piece : form.pieces) {
        if (piece.mathNode) {
            walkForLowest(*piece.mathNode, timeVariable, minCoefficient, lowest);
        }
    }
    return lowest;
}

std::vector<float> renderForm(const OntoMath::Piecewise& form,
                              const std::string& timeVariable,
                              double seconds,
                              int sampleRate,
                              const std::map<std::string, double>& constants,
                              SoundingReport* report) {
    SoundingReport local;
    SoundingReport& out = report ? *report : local;
    out = SoundingReport{};

    std::vector<float> samples;
    if (seconds <= 0.0 || sampleRate <= 0) return samples;

    // ------------------------------------------------------------------
    // The floor, first pass: read the TEXT. An authored infrasonic sinusoid
    // is refused before a single sample exists, and named exactly, because
    // the waveform is symbolic rather than sampled.
    // ------------------------------------------------------------------
    out.lowestAuthoredHz = lowestAuthoredFrequency(form, timeVariable);
    if (out.lowestAuthoredHz > 0.0 && out.lowestAuthoredHz < kAudibleFloorHz) {
        out.refused = true;
        out.refusal = "refused: the model authors a " +
                      std::to_string(out.lowestAuthoredHz) +
                      " Hz component, below the " + std::to_string(kAudibleFloorHz) +
                      " Hz floor of human hearing. This channel does not sound "
                      "infrasound; the mathematics is untouched.";
        return samples;
    }

    const std::size_t count =
        static_cast<std::size_t>(seconds * static_cast<double>(sampleRate));
    samples.reserve(count);

    // The constants are bound once; only the time variable moves.
    std::map<std::string, PropertyValue> vars;
    for (const auto& [name, value] : constants) vars[name] = PropertyValue(value);

    for (std::size_t i = 0; i < count; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(sampleRate);
        vars[timeVariable] = PropertyValue(t);

        // No subject: this is pure mathematics being sounded, so a piece that
        // needs the world to testify is unproven and the model is undefined
        // there. Undefined is silence THAT IS COUNTED, never silence passed
        // off as a value the author wrote.
        const auto value = form.evaluate(vars, nullptr);
        double amplitude = 0.0;
        if (!value || !propertyValueToNumber(*value, amplitude)) {
            ++out.undefinedSamples;
            samples.push_back(0.0f);
            continue;
        }
        if (amplitude > 1.0 || amplitude < -1.0) {
            ++out.clampedSamples;
            amplitude = amplitude > 1.0 ? 1.0 : -1.0;
        }
        samples.push_back(static_cast<float>(amplitude));
    }

    // ------------------------------------------------------------------
    // The floor, second pass: measure what was actually produced. The
    // symbolic pass sees sinusoids; this sees EVERYTHING — a DC offset, a
    // slow ramp, a piecewise step train, a recursive function call, a fold
    // over the world. Whatever the author's route to the speaker, the energy
    // below the floor is measured on the samples themselves, so nothing
    // reaches a Person's body by being authored in a form the text-reader
    // could not interpret.
    // ------------------------------------------------------------------
    out.infrasonicRms = measureInfrasonicRms(samples, sampleRate);
    if (out.infrasonicRms > kInfrasonicRmsCeiling) {
        out.refused = true;
        out.refusal = "refused: the rendered signal carries " +
                      std::to_string(out.infrasonicRms) +
                      " RMS below the " + std::to_string(kAudibleFloorHz) +
                      " Hz floor of human hearing (ceiling " +
                      std::to_string(kInfrasonicRmsCeiling) +
                      "). This channel does not sound infrasound; the "
                      "mathematics is untouched.";
        samples.clear();
    }
    return samples;
}

bool AudioSystem::playForm(const OntoMath::Piecewise& form,
                           const std::string& timeVariable,
                           double seconds,
                           float volume,
                           const glm::vec3* position,
                           const std::map<std::string, double>& constants) {
    if (!_initialized || !_state) return false;

    const ma_uint32 sampleRate = _state->engine.pDevice->sampleRate;
    SoundingReport report;
    auto* samples = new std::vector<float>(renderForm(
        form, timeVariable, seconds, static_cast<int>(sampleRate), constants, &report));
    if (samples->empty()) {
        // The floor refuses out loud. A guard that stops a Person's authored
        // sound without saying so is indistinguishable from a broken channel.
        if (report.refused) std::cerr << "AudioSystem::playForm " << report.refusal << std::endl;
        delete samples;
        return false;
    }

    // One channel: a waveform authored as f(t) is mono by construction, and
    // where it sits in the world is the spatializer's business, not the
    // expression's.
    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_f32, 1, samples->size(), samples->data(), NULL);
    config.sampleRate = sampleRate;

    auto* pcm = new ma_audio_buffer();
    if (ma_audio_buffer_init(&config, pcm) != MA_SUCCESS) {
        delete pcm;
        delete samples;
        return false;
    }

    auto* sound = new ma_sound();
    if (ma_sound_init_from_data_source(&_state->engine, pcm, 0, NULL, sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(pcm);
        delete pcm;
        delete samples;
        delete sound;
        return false;
    }

    ma_sound_set_volume(sound, volume);
    if (position) {
        ma_sound_set_position(sound, position->x, position->y, position->z);
    } else {
        ma_sound_set_spatialization_enabled(sound, MA_FALSE);
    }
    ma_sound_start(sound);

    auto* instance = new SpatialSoundInstance();
    instance->sound = sound;
    instance->pcm = pcm;
    instance->samples = samples;
    _state->activeSpatialSounds.push_back(instance);
    return true;
}

void AudioSystem::playMusic(const std::string& filepath) {
    if (!_initialized || !_state) return;

    stopMusic();

    ma_result result = ma_sound_init_from_file(
        &_state->engine, 
        filepath.c_str(), 
        MA_SOUND_FLAG_STREAM, 
        NULL, 
        NULL, 
        &_state->musicSound
    );

    if (result == MA_SUCCESS) {
        ma_sound_set_looping(&_state->musicSound, MA_TRUE);
        ma_sound_start(&_state->musicSound);
        _state->hasMusic = true;
    } else {
        std::cerr << "⚠️  Failed to load music: " << filepath << std::endl;
    }
}

void AudioSystem::stopMusic() {
    if (!_initialized || !_state) return;

    if (_state->hasMusic) {
        ma_sound_stop(&_state->musicSound);
        ma_sound_uninit(&_state->musicSound);
        _state->hasMusic = false;
    }
}

} // namespace Audio
} // namespace Core
