#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Core {
namespace Audio {

// ---------------------------------------------------------------------------
// The audio modality reading OntoMath — the same discipline the WebGPU channel
// follows (ONTOMATH_FRAMEWORK.md §1): a channel does not define what a thing
// IS, it renders what was authored elsewhere. A FieldNode reads a Piecewise as
// a density in space; this reads THE SAME Piecewise as a pressure wave in
// time. One authored expression, two senses — a vault and its chord are not a
// resemblance, they are one text read twice.
//
// Pure by construction: no device, no engine, no miniaudio. This is what makes
// it testable, and what keeps the mathematics separable from the hardware.
//
// `timeVariable` names the expression's own parameter (whatever the author
// called it — "t", "x"). Samples where the model is UNDEFINED — outside every
// authored piece — are silent, and their count is reported through
// `undefinedSamples` rather than passed off as zero: the silence is a hole in
// the domain, not part of the sound. Values are clamped to [-1, 1]; the clamped
// count is reported the same way.
// ---------------------------------------------------------------------------
std::vector<float> renderForm(const OntoMath::Piecewise& form,
                              const std::string& timeVariable,
                              double seconds,
                              int sampleRate = 48000,
                              const std::map<std::string, double>& constants = {},
                              std::size_t* undefinedSamples = nullptr,
                              std::size_t* clampedSamples = nullptr);

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

    // Play a procedural synthesized sound based on acoustic properties
    void playProceduralCollisionSound(const glm::vec3& position, const glm::vec3& velocity, double frequency, double amplitude, const std::string& waveTypeStr);

    // Sound an authored OntoMath model directly (see renderForm above): the
    // expression is the waveform. `position` spatializes it where a Being
    // stands; pass nullptr for a sound with no place in the world.
    // Returns false if the model rendered to nothing.
    bool playForm(const OntoMath::Piecewise& form,
                  const std::string& timeVariable,
                  double seconds,
                  float volume = 1.0f,
                  const glm::vec3* position = nullptr,
                  const std::map<std::string, double>& constants = {});

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
