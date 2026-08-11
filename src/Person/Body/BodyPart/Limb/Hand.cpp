#include "Hand.hpp"

Hand::Hand(Side side)
    : Limb((side == Side::Left ? "LeftHand" : "RightHand"),
            BodyPart::Type::Hand,
            ObjectTypes::GeometryType::Cube, {0.12f, 0.12f, 0.12f}) {}
