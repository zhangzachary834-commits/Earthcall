#include "Chest.hpp"

Chest::Chest()
    : BodyPart("Chest", BodyPart::Type::Torso,
                Form(Form::ShapeType::Cube, {0.4f, 0.35f, 0.2f})) {} 