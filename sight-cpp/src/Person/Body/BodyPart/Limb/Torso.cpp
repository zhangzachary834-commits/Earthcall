#include "Torso.hpp"

Torso::Torso()
    : Limb("Torso", BodyPart::Type::Torso,
            Form(Form::ShapeType::Cube, {0.4f, 0.6f, 0.2f})) {}
