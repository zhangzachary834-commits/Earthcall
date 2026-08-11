#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "json.hpp"

#include <glm/glm.hpp>
#include <string>

// ---------------------------------------------------------------------------
// A Material is a being. It owns how a surface *appears* — not a GL concept but
// authorable appearance data: an albedo tint plus a Blinn-Phong response. Like
// every Singular it registers its fields as Properties, so the Law system can
// address `material.clay.baseColor` and a Law can change a material's colour the
// same way a Law changes an Object's position. That legibility is the whole
// reason Material is a being rather than a render-layer struct.
//
// Objects reference a Material by its identifier string (the same by-name model
// Relation uses for its endpoints); MaterialManager owns the Material beings.
//
// This class holds NO OpenGL/WebGPU state. The render layer resolves a Material
// being into a flat `RenderMaterial` at draw time — that translation is where
// the backend lives, not here.
// ---------------------------------------------------------------------------
class Material : public Singular {
public:
    Material() = default;
    explicit Material(std::string name) : _name(std::move(name)) {}

    // Singular interface: identity is "material.<name>" (namespaced so it can't
    // collide with an Object identifier in the same PropertyPath space).
    std::string getIdentifier() const override { return "material." + _name; }
    const std::string& name() const { return _name; }
    void setName(std::string n) { _name = std::move(n); }

    // (De)Serialization ----------------------------------------------------
    nlohmann::json toJson() const;
    static Material fromJson(const nlohmann::json& j);

    // Authorable appearance (public data, addressable via PropertyPath). The
    // defaults reproduce the previous global ShadingSystem constants exactly, so
    // an object drawn with the default material looks identical to before.
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f}; // albedo tint; multiplies any face texture
    float opacity   = 1.0f;                // 1 = opaque
    float shininess = 32.0f;               // Blinn-Phong specular exponent (was global)
    float specular  = 1.0f;                // specular strength     (was light specular)
    float ambient   = 0.2f;                // ambient coefficient   (was light ambient)
    float diffuse   = 0.8f;                // diffuse coefficient   (was light diffuse)

private:
    // Identity, like a Relation's endpoints, is not a mutable property: renaming
    // a material is re-identifying it. Everything else is Law-addressable.
    std::string _name = "default";
    void buildProperties() override;
};
