#include "Limb.hpp"

Limb::Limb(const std::string& name, Type type, 
           ObjectTypes::GeometryType geometryType, const glm::vec3& dimensions)
    : BodyPart(name, type, geometryType, dimensions) {}
