#include "Leg.hpp"

Leg::Leg(Side side)
    : Limb((side == Side::Left ? "LeftLeg" : "RightLeg"),
            BodyPart::Type::Leg,
            ObjectTypes::GeometryType::Cube, {0.15f, 0.5f, 0.15f}) {}
