#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"

using json = nlohmann::json;

void Material::buildProperties() {
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

void Material::initFaceTextures(int numFaces) {
    if (faceTextures.size() == static_cast<size_t>(numFaces)) {
        return; // Already initialised correctly
    }
    faceTextures.clear();
    for (int i = 0; i < numFaces; ++i) {
        FaceTexture tex;
        tex.create(); // Default 64x64 white texture
        faceTextures.push_back(std::move(tex));
    }
}
