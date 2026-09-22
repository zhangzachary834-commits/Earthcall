#include "Neck.hpp"

Neck::Neck()
    : Limb("Neck", BodyPart::Type::Neck,
            ObjectTypes::ShapeKind::Cube, {0.2f, 0.2f, 0.2f}) {}
