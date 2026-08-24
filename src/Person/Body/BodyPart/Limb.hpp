#pragma once
#include "BodyPart.hpp"
#include "ConstructedBeing/Singular/Object/Object/ObjectTypes.hpp"
#include <glm/glm.hpp>

class Limb : public BodyPart {
public:
    Limb(const std::string& name,
         Type type,
         ObjectTypes::ShapeKind geometryType = ObjectTypes::ShapeKind::Cube,
         const glm::vec3& dimensions = glm::vec3(1.0f, 1.0f, 1.0f));
};
