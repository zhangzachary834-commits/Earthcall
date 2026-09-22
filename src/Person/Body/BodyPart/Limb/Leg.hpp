#pragma once
#include "../Limb.hpp"

class Leg : public Limb {
public:
    enum class Side { Left, Right };
    Leg(Side side);
};
