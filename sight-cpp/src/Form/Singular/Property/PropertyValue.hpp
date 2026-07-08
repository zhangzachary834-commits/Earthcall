#pragma once

#include <glm/glm.hpp>
#include <string>
#include <variant>

class Singular;
class Object;
class Relation;
class Formation;

// The typed currency of the property bridge: every legible property value fits
// in this variant. glm::vec3 (not a private Vec3) because the whole codebase
// speaks glm — a local vector type would mean converting at every boundary.
// glm dependency update when moving to more advanced rendering
using PropertyValue = std::variant<
    int,
    float,
    double,
    bool,
    char,
    long,
    std::string,
    glm::vec3,
    Singular*,
    Object*,
    Relation*,
    Formation*
>;
