#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

/*
 * Automation — Earthcall's term for time-driven "animation" of an Object.
 *
 * An automation is an *additive offset* layered on top of a stored rest pose.
 * Each Track drives one transform channel (position / rotation / scale on a
 * single axis) with a periodic wave. Several Tracks group into a Clip; several
 * Clips can play on one target and their offsets sum. Because everything is
 * expressed relative to a rest pose, a body can translate (walk) while its
 * limbs oscillate, and physics can keep driving any channel no Track touches.
 *
 * advance() mutates time and must be called exactly once per frame.
 * compose() is pure/idempotent and may be called many times per frame
 * (updatePose runs several times per frame), so the two are kept separate.
 */
namespace Automation {

enum class Channel : uint8_t {
    PosX, PosY, PosZ,   // world/local units, added to the rest translation
    RotX, RotY, RotZ,   // degrees, added to the rest rotation
    SclX, SclY, SclZ    // ratio: rest scale is multiplied by (1 + value)
};

enum class Wave : uint8_t { Sine, Triangle, Square, Saw };

// A single periodic drive on one channel.
struct Track {
    Channel channel   = Channel::RotY;
    Wave    wave      = Wave::Sine;
    float   amplitude = 0.0f;  // peak offset (degrees / units / scale ratio)
    float   frequency = 1.0f;  // cycles per second
    float   phase     = 0.0f;  // [0,1) turns — offset two limbs for opposition
    float   bias      = 0.0f;  // constant offset added to the channel
};

// A named bundle of Tracks advanced by a shared clock.
struct Clip {
    std::string        name;
    std::vector<Track> tracks;
    bool   loop     = true;
    float  duration = 0.0f;  // seconds; when loop==false and >0, clip stops here
    float  speed    = 1.0f;  // playback-rate multiplier
    float  weight   = 1.0f;  // blend weight (reserved for future layering)
    bool   playing  = true;
    float  time     = 0.0f;  // seconds elapsed (advanced by advance())
};

// Per-target automation state. Lives on the Object/BodyPart.
struct State {
    glm::mat4         rest{1.0f};      // baseline that animated channels build on
    bool              restValid = false;
    std::vector<Clip> clips;
};

// True if any clip is currently playing.
bool active(const State& state);

// Advance every playing clip's clock by dt. Call once per frame.
// If `finished` is non-null, the names of any non-looping clips that reach
// their end on this step are appended to it (the caller turns these into
// ClipFinished events — see AutomationEvents.hpp).
void advance(State& state, float dt, std::vector<std::string>* finished = nullptr);

// Build the animated transform: animated channels come from state.rest plus the
// summed clip offsets; every channel no Track touches passes through `base`.
// Pure — safe to call multiple times per frame.
glm::mat4 compose(const State& state, const glm::mat4& base);

} // namespace Automation
