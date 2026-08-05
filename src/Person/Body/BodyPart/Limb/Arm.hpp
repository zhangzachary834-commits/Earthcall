#pragma once
#include "../Limb.hpp"

class Arm : public Limb {
public:
    enum class Side { Left, Right };
    Arm(Side side);
};
