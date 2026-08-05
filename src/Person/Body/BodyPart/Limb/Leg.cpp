#include "Leg.hpp"

Leg::Leg(Side side)
    : Limb((side == Side::Left ? "LeftLeg" : "RightLeg"),
            BodyPart::Type::Leg,
            Form(Form::ShapeType::Cube, {0.15f, 0.5f, 0.15f})) {}
