#include "ForeLeg.hpp"

ForeLeg::ForeLeg(Side side)
    : Limb((side == Side::Left ? "LeftForeLeg" : "RightForeLeg"),
            BodyPart::Type::Leg,
            ObjectTypes::GeometryType::Cube, {0.15f, 0.4f, 0.15f}) {}