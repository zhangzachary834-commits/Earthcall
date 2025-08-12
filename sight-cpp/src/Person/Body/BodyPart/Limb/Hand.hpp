#pragma once
#include "../Limb.hpp"

class Hand : public Limb {
public:
    enum class Side { Left, Right };
    Hand(Side side);
};
