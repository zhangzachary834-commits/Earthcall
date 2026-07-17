#pragma once

#include <string>

namespace Core {
namespace Audio {

class AudioSystem {
public:
    static AudioSystem& instance();

    // Lifecycle
    bool init();
    void shutdown();

    // Core functionality
    // Play a short sound effect (fire and forget)
    void playSound(const std::string& filepath);

    // Play background music (looped)
    void playMusic(const std::string& filepath);
    void stopMusic();

private:
    AudioSystem() = default;
    ~AudioSystem() = default;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    struct AudioState;
    AudioState* _state = nullptr;
    bool _initialized = false;
};

} // namespace Audio
} // namespace Core
