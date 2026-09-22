#pragma once
#include "../Limb.hpp"

class ForeArm : public Limb {
public:
    enum class Side { Left, Right };
    ForeArm(Side side);
}; 