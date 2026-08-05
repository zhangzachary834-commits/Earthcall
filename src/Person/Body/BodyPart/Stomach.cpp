#include "Stomach.hpp"

Stomach::Stomach()
    : BodyPart("Stomach", BodyPart::Type::Torso,
               Form(Form::ShapeType::Cube, {0.4f, 0.25f, 0.2f})) {} 