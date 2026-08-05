#pragma once
#include "../Limb.hpp"

class Foot : public Limb {
public:
    enum class Side { Left, Right };
    Foot(Side side);
};
