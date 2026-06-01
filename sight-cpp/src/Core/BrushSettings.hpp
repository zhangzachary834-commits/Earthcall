#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Core {

enum class BrushType {
    Normal = 0,
    Airbrush,
    Chalk,
    Spray,
    Smudge,
    Clone
};

struct BrushPreset {
    std::string name;
    BrushType   type;
    float       radius;
    float       softness;
    float       opacity;
    float       flow;
    float       spacing;
    float       density;
    float       strength;
};

// Fluent builder for self-documenting preset construction.
struct BrushPresetBuilder {
    BrushPreset value;

    BrushPresetBuilder(const std::string& presetName, BrushType brushType) {
        value.name     = presetName;
        value.type     = brushType;
        value.radius   = 0.1f;
        value.softness = 1.0f;
        value.opacity  = 1.0f;
        value.flow     = 1.0f;
        value.spacing  = 0.1f;
        value.density  = 0.5f;
        value.strength = 0.5f;
    }

    BrushPresetBuilder& radius(float v)   { value.radius   = v; return *this; }
    BrushPresetBuilder& softness(float v) { value.softness = v; return *this; }
    BrushPresetBuilder& opacity(float v)  { value.opacity  = v; return *this; }
    BrushPresetBuilder& flow(float v)     { value.flow     = v; return *this; }
    BrushPresetBuilder& spacing(float v)  { value.spacing  = v; return *this; }
    BrushPresetBuilder& density(float v)  { value.density  = v; return *this; }
    BrushPresetBuilder& strength(float v) { value.strength = v; return *this; }
    BrushPreset build() const { return value; }
};

struct BrushSettings {
    // Geometry / transform
    float     size = 1.0f;
    glm::vec3 scale    {1.0f};
    glm::vec3 rotation {0.0f};
    bool      gridSnap = false;
    float     gridSize = 1.0f;

    // Dynamics
    BrushType type     = BrushType::Normal;
    float     opacity  = 1.0f;
    float     flow     = 1.0f;
    float     spacing  = 0.1f;
    float     density  = 0.5f;
    float     strength = 0.5f;

    // Pressure simulation
    bool  usePressureSimulation = false;
    float pressureSensitivity   = 1.0f;
    float currentPressure       = 1.0f;

    // Stroke interpolation
    bool useStrokeInterpolation = true;

    // Cursor
    bool      showCursor    = true;
    bool      cursorVisible = false;
    glm::vec2 cursorPos     {0.0f, 0.0f};

    // Presets
    std::vector<BrushPreset> presets;
    int currentPreset = 0;

    // Preview
    bool  showPreview = true;
    float previewSize = 1.0f;

    // 2D brush system toggles
    bool useAdvanced2D = false;
    bool show2DPanel   = false;
};

} // namespace Core
