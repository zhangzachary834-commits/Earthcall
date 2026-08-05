#include "Shoulder.hpp"

Shoulder::Shoulder(Side side)
    : Limb((side == Side::Left ? "LeftShoulder" : "RightShoulder"),
            BodyPart::Type::Arm, // categorize as arm for now
            Form(Form::ShapeType::Cube, {0.18f, 0.18f, 0.18f})) {} 