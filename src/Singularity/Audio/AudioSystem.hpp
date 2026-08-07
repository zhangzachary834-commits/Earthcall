#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Core {
namespace Audio {

class AudioSystem {
public:
    static AudioSystem& instance();

    // Lifecycle
    bool init();
    void shutdown();
    void tick();

    void setupAudioEventListeners();

    // Core functionality
    // Play a short sound effect (fire and forget)
    void playSound(const std::string& filepath);

    // Play a spatialized sound effect at a specific 3D position
    void playSpatialSound(const std::string& filepath, const glm::vec3& position, float volume = 1.0f);

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
