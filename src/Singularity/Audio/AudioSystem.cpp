#include "AudioSystem.hpp"
#include <iostream>
#include <vector>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Foreign/EarthcallAPI.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

extern ZoneManager mgr;

namespace Core {
namespace Audio {

struct SpatialSoundInstance {
    ma_sound* sound = nullptr;
    ma_waveform* waveform = nullptr;
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
            delete *it;
            it = _state->activeSpatialSounds.erase(it);
        } else {
            ++it;
        }
    }

    // Process SoundEmitters
    auto& objects = mgr.active().world().getOwnedObjects();
    
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

    if (frequency < 20.0) frequency = 20.0;
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
