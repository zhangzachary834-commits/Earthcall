#pragma once

namespace Core {

enum class RotationAxisMode {
    FreeXY = 0,
    X,
    Y,
    Z,
    AuthoritativeAxis
};

struct RotationSettings {
    RotationAxisMode axisMode    = RotationAxisMode::FreeXY;
    float            sensitivity = 0.35f;
    float            smoothness  = 10.0f;
};

} // namespace Core
