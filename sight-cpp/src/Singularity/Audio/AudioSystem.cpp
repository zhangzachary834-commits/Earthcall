#include "AudioSystem.hpp"
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace Core {
namespace Audio {

struct AudioSystem::AudioState {
    ma_engine engine;
    ma_sound musicSound;
    bool hasMusic = false;
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
