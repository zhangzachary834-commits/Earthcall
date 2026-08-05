#include "Neck.hpp"

Neck::Neck()
    : Limb("Neck", BodyPart::Type::Undefined,
            Form(Form::ShapeType::Cube, {0.2f, 0.2f, 0.2f})) {}
