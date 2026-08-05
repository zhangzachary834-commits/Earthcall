#pragma once

#include "OurVerse/AdvancedFacePaint.hpp"

namespace Core {

struct FaceBrushSettings {
    float radius   = 0.1f; // relative to texture (0-1)
    float softness = 1.0f; // 1 = hard, 0 = very soft
    float uOffset  = 0.0f;
    float vOffset  = 0.0f;
    int   uAxis    = 1;    // 0=X, 1=Y, 2=Z
    int   vAxis    = 2;    // must differ from uAxis
    bool  invertU  = false;
    bool  invertV  = false;
};

struct AdvancedFacePaintState {
    bool enabled       = false;
    bool panelVisible  = false;
    AdvancedFacePaint::GradientSettings gradient;
    AdvancedFacePaint::SmudgeSettings   smudge;
};

} // namespace Core
