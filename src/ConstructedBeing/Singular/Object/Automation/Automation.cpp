#include "Automation.hpp"
#include "Singularity/OntoMath/LinearAlgebra.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace Automation {

namespace {

constexpr float kTwoPi = 6.28318530718f;

// Fractional part in [0,1).
float frac(float v) {
    float f = v - std::floor(v);
    return f;
}

// Evaluate a wave at phase p (already wrapped to [0,1)) -> [-1, 1].
float waveValue(Wave wave, float p) {
    switch (wave) {
        case Wave::Sine:     return std::sin(kTwoPi * p);
        case Wave::Triangle: return (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
        case Wave::Square:   return (p < 0.5f) ? 1.0f : -1.0f;
        case Wave::Saw:      return 2.0f * p - 1.0f;
    }
    return 0.0f;
}

// Track output at a given clip time.
float evalTrack(const Track& t, float time) {
    float p = frac(time * t.frequency + t.phase);
    return t.bias + t.amplitude * waveValue(t.wave, p);
}

std::optional<glm::vec3> extractScale(const glm::mat4& m) {
    auto scale = OntoMath::affineExtractScale(OntoMath::MatrixValue::fromGlmMat4(m));
    if (!scale) return std::nullopt;
    if (scale->x <= 1e-6f) scale->x = 1.0f;
    if (scale->y <= 1e-6f) scale->y = 1.0f;
    if (scale->z <= 1e-6f) scale->z = 1.0f;
    return scale;
}

// Same composition order Object uses: T * Rx * Ry * Rz * S.
// OntoMath owns the authored affine meaning; GLM is only the runtime carrier.
std::optional<glm::mat4> recompose(const glm::vec3& translation,
                                   const glm::vec3& eulerDeg,
                                   const glm::vec3& scale) {
    const auto authored = OntoMath::affineTRS(translation, eulerDeg, scale);
    if (!authored) return std::nullopt;
    return authored->toGlmMat4();
}

} // namespace

bool active(const State& state) {
    for (const auto& clip : state.clips) {
        if (clip.playing && !clip.tracks.empty()) return true;
    }
    return false;
}

void advance(State& state, float dt, std::vector<std::string>* finished) {
    if (dt <= 0.0f) return;
    for (auto& clip : state.clips) {
        if (!clip.playing) continue;
        clip.time += dt * clip.speed;
        if (!clip.loop && clip.duration > 0.0f && clip.time >= clip.duration) {
            clip.time = clip.duration;
            clip.playing = false;
            if (finished) finished->push_back(clip.name);
        }
    }
}

glm::mat4 compose(const State& state, const glm::mat4& base) {
    if (!active(state)) return base;

    // Accumulate per-channel offsets across all playing clips.
    glm::vec3 offT(0.0f);     // additive translation
    glm::vec3 offR(0.0f);     // additive rotation (degrees)
    glm::vec3 mulS(1.0f);     // multiplicative scale ratio
    bool touchedT[3] = {false, false, false};
    bool touchedR[3] = {false, false, false};
    bool touchedS[3] = {false, false, false};

    for (const auto& clip : state.clips) {
        if (!clip.playing) continue;
        for (const auto& track : clip.tracks) {
            float v = clip.weight * evalTrack(track, clip.time);
            switch (track.channel) {
                case Channel::PosX: offT.x += v; touchedT[0] = true; break;
                case Channel::PosY: offT.y += v; touchedT[1] = true; break;
                case Channel::PosZ: offT.z += v; touchedT[2] = true; break;
                case Channel::RotX: offR.x += v; touchedR[0] = true; break;
                case Channel::RotY: offR.y += v; touchedR[1] = true; break;
                case Channel::RotZ: offR.z += v; touchedR[2] = true; break;
                case Channel::SclX: mulS.x *= (1.0f + v); touchedS[0] = true; break;
                case Channel::SclY: mulS.y *= (1.0f + v); touchedS[1] = true; break;
                case Channel::SclZ: mulS.z *= (1.0f + v); touchedS[2] = true; break;
            }
        }
    }

    const auto baseScale = extractScale(base);
    const auto baseEuler = OntoMath::affineExtractEulerXYZDegrees(
        OntoMath::MatrixValue::fromGlmMat4(base));
    const glm::mat4& rest = state.restValid ? state.rest : base;
    const auto restScale = extractScale(rest);
    const auto restEuler = OntoMath::affineExtractEulerXYZDegrees(
        OntoMath::MatrixValue::fromGlmMat4(rest));
    if (!baseScale || !baseEuler || !restScale || !restEuler) return base;
    glm::vec3 baseTrans = glm::vec3(base[3]);
    glm::vec3 restTrans = glm::vec3(rest[3]);

    // Animated channels build on the rest pose; untouched channels follow the
    // live base so physics / manual edits keep flowing through.
    glm::vec3 outT(
        touchedT[0] ? restTrans.x + offT.x : baseTrans.x,
        touchedT[1] ? restTrans.y + offT.y : baseTrans.y,
        touchedT[2] ? restTrans.z + offT.z : baseTrans.z);
    glm::vec3 outR(
        touchedR[0] ? restEuler->x + offR.x : baseEuler->x,
        touchedR[1] ? restEuler->y + offR.y : baseEuler->y,
        touchedR[2] ? restEuler->z + offR.z : baseEuler->z);
    glm::vec3 outS(
        touchedS[0] ? restScale->x * mulS.x : baseScale->x,
        touchedS[1] ? restScale->y * mulS.y : baseScale->y,
        touchedS[2] ? restScale->z * mulS.z : baseScale->z);

    const auto recomposed = recompose(outT, outR, outS);
    return recomposed.value_or(base);
}

} // namespace Automation
