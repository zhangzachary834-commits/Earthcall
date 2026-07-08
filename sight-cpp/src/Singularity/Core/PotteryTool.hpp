#pragma once

namespace Core {

enum class PotteryTool {
    Chisel = 0,
    Expand
};

struct PotterySettings {
    PotteryTool currentTool = PotteryTool::Expand;
    float strength = 0.2f;
};

} // namespace Core
