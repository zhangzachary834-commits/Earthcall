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
// THE INFRASOUND FLOOR — a Kernel boundary, not a setting.
//
// 20 Hz is the conventional floor of human hearing. Below it a signal stops
// being heard and starts being FELT: sustained high-energy infrasound is
// associated with nausea, disorientation and chest pressure, it drives
// loudspeaker cones toward their excursion limits with no audible warning that
// anything is wrong, and 0 Hz — a DC offset — is not a sound at all but a
// constant displacement held against the driver and the ear.
//
// The manifesto's first Kernel boundary is that nothing may violate
// fundamental Person guards. A Person's body is on the other side of this
// channel. So the floor is enforced HERE, in the channel that touches the
// hardware that touches the human, and it is deliberately:
//
//   * unconditional — there is no parameter, flag, or argument that lifts it;
//   * unauthorable  — no Law and no Zone can reach it, because it is not
//                     expressed as law text at all;
//   * a REFUSAL, not a filter — the channel declines to sound the model and
//     says which frequency it refused, rather than silently high-passing a
//     Person's authored mathematics into something they did not write.
//
// What it does NOT constrain is the mathematics. A Person may author, evaluate,
// integrate, render as geometry, and reason about a 7 Hz field freely — that is
// the substrate doing its job. The guard is on the path to the body, not on the
// thought. `playProceduralCollisionSound` has always clamped to this same
// floor; this is that clamp made general, for a channel where the waveform is
// an arbitrary authored expression and there is no frequency parameter to clamp.
// ---------------------------------------------------------------------------

// The floor of human hearing. Content below this is refused, never rendered.
inline constexpr double kAudibleFloorHz = 20.0;

// How much sub-floor energy a buffer may carry before the channel refuses, as
// RMS amplitude on the [-1, 1] scale (about -34 dBFS). Absolute rather than a
// ratio, because what reaches the body is absolute: an inaudible 7 Hz term at
// RMS 0.001 is harmless, and the same term at 0.3 is what this exists to stop.
inline constexpr double kInfrasonicRmsCeiling = 0.02;

// What the channel did, and — when it refused — why.
struct SoundingReport {
    std::size_t undefinedSamples = 0;   // outside every authored piece
    std::size_t clampedSamples = 0;     // outside [-1, 1]
    bool refused = false;               // the Kernel floor stopped it
    std::string refusal;                // naming the frequency, when refused
    double infrasonicRms = 0.0;         // measured sub-floor energy
    double lowestAuthoredHz = 0.0;      // lowest sinusoid read off the TEXT (0 = none)
};

// The measurement half of the floor, exposed so any path that produces PCM can
// be held to the same line: the RMS of everything below kAudibleFloorHz.
double measureInfrasonicRms(const std::vector<float>& samples, int sampleRate);

// The symbolic half. Because an OntoMath waveform is exact TEXT rather than
// samples, the lowest sinusoid in it can be read off directly — no FFT, no
// window, no spectral leakage — and refused BEFORE a sample is rendered, with
// the offending frequency named. Returns 0 when the text carries no sinusoid
// (a polynomial or a fold says nothing about its own spectrum; that is what
// the measurement above is for). `minCoefficient` ignores terms too small to
// matter.
double lowestAuthoredFrequency(const OntoMath::Piecewise& form,
                               const std::string& timeVariable,
                               double minCoefficient = kInfrasonicRmsCeiling);

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
// authored piece — are silent, and counted in the report rather than passed off
// as zero: the silence is a hole in the domain, not part of the sound. Values
// are clamped to [-1, 1], counted the same way.
//
// Returns an EMPTY buffer when the infrasound floor refuses, with
// `report.refusal` naming the frequency and how it was found.
// ---------------------------------------------------------------------------
std::vector<float> renderForm(const OntoMath::Piecewise& form,
                              const std::string& timeVariable,
                              double seconds,
                              int sampleRate = 48000,
                              const std::map<std::string, double>& constants = {},
                              SoundingReport* report = nullptr);

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
