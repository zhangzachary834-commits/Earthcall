#pragma once

#include <glm/glm.hpp>
#include <string>

// ---------------------------------------------------------------------------
// RenderMaterial is the flat, GPU-facing view of a Material being. The ontology
// (Material : Singular, Law-addressable, owned by MaterialManager) is resolved
// down to this POD once per draw. The renderer consumes ONLY this — it never
// sees the being — which is what lets the same draw paths back onto OpenGL today
// and WebGPU later (where this struct becomes a uniform buffer).
//
// `textureId` is the per-face albedo (the paint the Face Brush writes, kept on
// the Object's faceTextures). baseColor is the material's tint; final surface
// colour is baseColor * texture. 0 means "no texture, use baseColor alone".
// ---------------------------------------------------------------------------
struct RenderMaterial {
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f};
    float opacity   = 1.0f;
    float shininess = 32.0f;
    float specular  = 1.0f;   // realized in the WGSL shader (M5); see OpenGLRenderer
    float ambient   = 0.2f;   // "
    float diffuse   = 0.8f;   // "
    unsigned int textureId = 0; // per-face albedo as a GL texture id (OpenGL backend)
    // Portable albedo: the same RGBA8 pixels as `textureId`, straight from the
    // FaceTexture CPU buffer. WebGPU can't use a GL id, so it uploads these. Non-
    // owning — valid only for the duration of the draw call. null = untextured.
    const unsigned char* albedoPixels = nullptr;
    int albedoSize = 0;         // width == height == albedoSize (square RGBA8)
    bool doubleSided = false;   // open surfaces (Bezier patches): light both faces.
                                // GL → two-sided light model; WebGPU → cull none.
};

// One face's albedo — the paint the Face Brush writes — described BOTH ways,
// because the two backends consume it differently and neither form is portable:
//   handle — a backend-owned texture (under OpenGL, the GL texture name).
//   pixels — the CPU-side RGBA8 buffer it was uploaded from.
// A backend that holds GPU textures samples `handle`; one that re-uploads per
// draw reads `pixels`. Passing only the handle is what left every WebGPU surface
// white: WebGPU keeps no handles, so it had nothing to sample.
struct FaceAlbedo {
    unsigned int         handle = 0;
    const unsigned char* pixels = nullptr;
    int                  size   = 0; // width == height (square RGBA8)
};

// Resolve an Object's material identifier against the global MaterialManager and
// flatten it to a RenderMaterial, stamping in the given per-face albedo.
// A dangling identifier resolves to material.default, so this never fails.
RenderMaterial resolveRenderMaterial(const std::string& materialId, const FaceAlbedo& albedo);
