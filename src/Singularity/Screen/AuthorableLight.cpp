#include "Singularity/Screen/AuthorableLight.hpp"

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

namespace Rendering {
namespace {

bool readBool(const geom::FieldNode& field, const char* path, bool& out) {
    PropertyValue value;
    if (!field.getDynamicProperty(path, value)) return false;
    if (const bool* v = std::get_if<bool>(&value)) {
        out = *v;
        return true;
    }
    return false;
}

bool readFloat(const geom::FieldNode& field, const char* path, float& out) {
    PropertyValue value;
    if (!field.getDynamicProperty(path, value)) return false;
    double number = 0.0;
    if (!propertyValueToNumber(value, number)) return false;
    out = static_cast<float>(number);
    return true;
}

bool readVec3(const geom::FieldNode& field, const char* path, glm::vec3& out) {
    PropertyValue value;
    if (!field.getDynamicProperty(path, value)) return false;
    if (const glm::vec3* v = std::get_if<glm::vec3>(&value)) {
        out = *v;
        return true;
    }
    return false;
}

} // namespace

bool readAuthorableLight(const geom::FieldNode& field, AuthorableLightState& out) {
    AuthorableLightState resolved;
    resolved.position = field.origin;

    bool source = false;
    if (!readBool(field, "light.source", source) || !source) {
        out = resolved;
        return false;
    }
    resolved.source = true;

    // Optional authored vocabulary. Wrongly typed values are ignored rather
    // than coerced by guessing; the property itself remains visible for a Law
    // or authoring UI to correct.
    readBool(field, "light.enabled", resolved.enabled);
    readVec3(field, "light.color", resolved.color);
    readFloat(field, "light.intensity", resolved.intensity);
    readFloat(field, "light.ambient", resolved.ambient);
    readFloat(field, "light.diffuse", resolved.diffuse);
    readFloat(field, "light.specular", resolved.specular);
    readFloat(field, "light.attenuation.constant", resolved.attenuationConstant);
    readFloat(field, "light.attenuation.linear", resolved.attenuationLinear);
    readFloat(field, "light.attenuation.quadratic", resolved.attenuationQuadratic);

    out = resolved;
    return true;
}

glm::vec3 lightAmbientRadiance(const AuthorableLightState& light) {
    return light.color * (light.intensity * light.ambient);
}

glm::vec3 lightDiffuseRadiance(const AuthorableLightState& light) {
    return light.color * (light.intensity * light.diffuse);
}

glm::vec3 lightSpecularRadiance(const AuthorableLightState& light) {
    return light.color * (light.intensity * light.specular);
}

} // namespace Rendering
