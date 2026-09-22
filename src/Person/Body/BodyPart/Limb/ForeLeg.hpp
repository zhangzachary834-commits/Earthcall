#pragma once
#include "../Limb.hpp"

class ForeLeg : public Limb {
public:
    enum class Side { Left, Right };
    ForeLeg(Side side);
}; 