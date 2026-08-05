#include "Form/Material/Material.hpp"

#include "Form/Singular/Property/PropertyRef.hpp"

using json = nlohmann::json;

void Material::buildProperties() {
    // Every field a Law may want to read or drive is registered here. baseColor
    // is a vec3 (RGB); opacity is separate so the whole appearance stays within
    // the PropertyValue variant (float / vec3 / string / bool).
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, glm::vec3>>(
        "baseColor", this, &Material::baseColor));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, float>>(
        "opacity", this, &Material::opacity));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, float>>(
        "shininess", this, &Material::shininess));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, float>>(
        "specular", this, &Material::specular));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, float>>(
        "ambient", this, &Material::ambient));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Material, float>>(
        "diffuse", this, &Material::diffuse));
}

json Material::toJson() const {
    return json{
        {"name", _name},
        {"baseColor", {baseColor.r, baseColor.g, baseColor.b}},
        {"opacity", opacity},
        {"shininess", shininess},
        {"specular", specular},
        {"ambient", ambient},
        {"diffuse", diffuse},
    };
}

Material Material::fromJson(const json& j) {
    Material m(j.value("name", std::string("default")));
    if (j.contains("baseColor") && j["baseColor"].is_array() && j["baseColor"].size() == 3) {
        m.baseColor = glm::vec3(j["baseColor"][0].get<float>(),
                                j["baseColor"][1].get<float>(),
                                j["baseColor"][2].get<float>());
    }
    m.opacity   = j.value("opacity", 1.0f);
    m.shininess = j.value("shininess", 32.0f);
    m.specular  = j.value("specular", 1.0f);
    m.ambient   = j.value("ambient", 0.2f);
    m.diffuse   = j.value("diffuse", 0.8f);
    return m;
}
