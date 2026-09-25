#pragma once

#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "json.hpp"

#include "ConstructedBeing/Singular/Object/Object/FaceTexture.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <glm/glm.hpp>
#include <string>
#include <memory>

// ---------------------------------------------------------------------------
// Material is the bridge between Singulars and raw Singularity metal.
// It gathers the Singularity into a defined shape and value order, calls it a Material,
// and then a Singular is comprised of it.
//
// A Material is a being. It owns how a surface *appears* — not a GL concept but
// authorable appearance data: albedo plus optional authored receiver-response math. Like
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

    // OntoMath-driven color evaluation for SDFs. If present, the WGSL backend
    // compiles this directly into the shader instead of using baseColor.
    std::shared_ptr<OntoMath::Piecewise> colorExpr;

    // Rung 9: independently authored receiving-surface response. This is the
    // Material being's f_r-like truth; absence preserves exact legacy shading.
    // It deliberately does not reuse volume phase or source-radiance ownership.
    std::shared_ptr<OntoMath::Piecewise> responseExpr;

    // Historical revision remains the color-expression content revision.
    uint32_t getRevision() const { return _revision; }
    void bumpRevision() { ++_revision; }
    uint32_t getResponseRevision() const { return _responseRevision; }
    void bumpResponseRevision() { ++_responseRevision; }

    int textureResolution = 64;
    int textureWidth = 64;
    int textureHeight = 64;

    int getTextureResolution() const;
    void setTextureResolution(const int& res);
    int getTextureWidth() const;
    void setTextureWidth(const int& w);
    int getTextureHeight() const;
    void setTextureHeight(const int& h);

private:
    // Identity, like a Relation's endpoints, is not a mutable property: renaming
    // a material is re-identifying it. Everything else is Law-addressable.
    std::string _name = "default";
    uint32_t _revision = 0;
    uint32_t _responseRevision = 0;
    void buildProperties() override;

public:
    // Per-face texture painting support
    std::vector<struct FaceTexture> faceTextures;
    
    // Initialise or reinitialise textures after geometry type set/changed
    void initFaceTextures(int numFaces, int defaultWidth = -1, int defaultHeight = -1);
};
