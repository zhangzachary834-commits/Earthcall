#include "Finger.hpp"

Finger::Finger(Side side, const std::string& specificName)
    : Limb((specificName.empty() ? (side == Side::Left ? "LeftFinger" : "RightFinger") : specificName),
            BodyPart::Type::Finger,
            ObjectTypes::ShapeKind::Cube, {0.05f, 0.2f, 0.05f}) {}
