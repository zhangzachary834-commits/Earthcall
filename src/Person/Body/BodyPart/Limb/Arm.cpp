#include "Arm.hpp"

Arm::Arm(Side side)
    : Limb((side == Side::Left ? "LeftArm" : "RightArm"),
            BodyPart::Type::Arm,
            ObjectTypes::GeometryType::Cube, {0.1f, 0.5f, 0.1f}) {}
