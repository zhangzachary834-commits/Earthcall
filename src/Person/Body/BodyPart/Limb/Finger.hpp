#pragma once

#include "../Limb.hpp"

class Finger : public Limb {
public:
    enum class Side { Left, Right };
    Finger(Side side, const std::string& specificName = "");
};
