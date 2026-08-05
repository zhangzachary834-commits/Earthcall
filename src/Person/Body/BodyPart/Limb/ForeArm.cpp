#include "ForeArm.hpp"

ForeArm::ForeArm(Side side)
    : Limb((side == Side::Left ? "LeftForeArm" : "RightForeArm"),
            BodyPart::Type::Arm,
            Form(Form::ShapeType::Cube, {0.1f, 0.4f, 0.1f})) {} 