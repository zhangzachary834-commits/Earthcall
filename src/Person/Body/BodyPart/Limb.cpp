#include "Limb.hpp"

Limb::Limb(const std::string& name, Type type, 
           ObjectTypes::ShapeKind geometryType, const glm::vec3& dimensions)
    : BodyPart(name, type, geometryType, dimensions) {}
