#include "Neck.hpp"

Neck::Neck()
    : Limb("Neck", BodyPart::Type::Undefined,
            ObjectTypes::GeometryType::Cube, {0.2f, 0.2f, 0.2f}) {}
