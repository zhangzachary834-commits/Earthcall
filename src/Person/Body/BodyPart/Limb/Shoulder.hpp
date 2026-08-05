#pragma once
#include "../Limb.hpp"

class Shoulder : public Limb {
public:
    enum class Side { Left, Right };
    Shoulder(Side side);
}; 