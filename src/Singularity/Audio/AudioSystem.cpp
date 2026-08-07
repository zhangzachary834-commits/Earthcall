#include "AudioSystem.hpp"
#include <iostream>
#include <vector>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Form/Object/Object.hpp"

namespace Core {
namespace Audio {

struct AudioSystem::AudioState {
    ma_engine engine;
    ma_sound musicSound;
    bool hasMusic = false;
    std::vector<ma_sound*> activeSpatialSounds;
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
        for (auto sound : _state->activeSpatialSounds) {
            ma_sound_uninit(sound);
            delete sound;
        }
        _state->activeSpatialSounds.clear();
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

    auto& eventBus = Core::EventBus::instance();
    eventBus.subscribe<Physics::PhysicsCollisionEvent>([this](const Physics::PhysicsCollisionEvent& event) {
        if (!event.objectA || !event.objectB) return;
        
        std::string matA = event.objectA->materialId();
        std::string matB = event.objectB->materialId();

        // Determine base sound based on material
        std::string soundFile = "data/audio/collision_light.wav";
        std::string heavySoundFile = "data/audio/collision_heavy.wav";

        if (matA.find("wood") != std::string::npos || matB.find("wood") != std::string::npos) {
            soundFile = "data/audio/wood_hit_light.wav";
            heavySoundFile = "data/audio/wood_hit_heavy.wav";
        } else if (matA.find("metal") != std::string::npos || matB.find("metal") != std::string::npos) {
            soundFile = "data/audio/metal_hit_light.wav";
            heavySoundFile = "data/audio/metal_hit_heavy.wav";
        }

        // Spatialized sound responses based on impact force
        if (event.impactForce > 5.0f) {
            playSpatialSound(heavySoundFile, event.collisionPoint);
        } else if (event.impactForce > 0.5f) {
            playSpatialSound(soundFile, event.collisionPoint, 0.5f);
        }
    });
}

void AudioSystem::tick() {
    if (!_initialized || !_state) return;

    for (auto it = _state->activeSpatialSounds.begin(); it != _state->activeSpatialSounds.end(); ) {
        if (!ma_sound_is_playing(*it)) {
            ma_sound_uninit(*it);
            delete *it;
            it = _state->activeSpatialSounds.erase(it);
        } else {
            ++it;
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
        _state->activeSpatialSounds.push_back(sound);
    } else {
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
