#include "Foot.hpp"

Foot::Foot(Side side)
    : Limb((side == Side::Left ? "LeftFoot" : "RightFoot"),
            BodyPart::Type::Foot,
            Form(Form::ShapeType::Cube, {0.18f, 0.08f, 0.3f})) {}
