#include "Shoulder.hpp"

Shoulder::Shoulder(Side side)
    : Limb((side == Side::Left ? "LeftShoulder" : "RightShoulder"),
            BodyPart::Type::Shoulder,
            ObjectTypes::ShapeKind::Cube, {0.18f, 0.18f, 0.18f}) {}