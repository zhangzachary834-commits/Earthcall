#include "Form.hpp"

#include "Rendering/Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// The unit cube and unit sphere as triangle lists. These were hand-written
// glBegin(GL_QUADS)/glBegin(GL_QUAD_STRIP) blocks; WebGPU has neither topology,
// so they are built once as triangles and handed to the boundary's drawSolid.
// Built lazily on first use and cached — the geometry is constant.
namespace {

const std::vector<glm::vec3>& unitCubeTris() {
    static const std::vector<glm::vec3> tris = draw::quadsToTris(std::vector<glm::vec3>{
        // Front
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        // Back
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
        // Left
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f},
        // Right
        { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        // Top
        {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f},
        // Bottom
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f},
    });
    return tris;
}

const std::vector<glm::vec3>& unitSphereTris() {
    static const std::vector<glm::vec3> tris = [] {
        constexpr int subdivisions = 16;
        // Each latitude band was a GL_QUAD_STRIP; a strip of 2n+2 vertices is the
        // same surface as n quads, so emit the quads and reuse the quad adapter.
        std::vector<glm::vec3> quads;
        for (int i = 0; i <= subdivisions; ++i) {
            float lat0 = M_PI * (-0.5f + (float)(i - 1) / subdivisions);
            float z0 = std::sin(lat0), zr0 = std::cos(lat0);
            float lat1 = M_PI * (-0.5f + (float)i / subdivisions);
            float z1 = std::sin(lat1), zr1 = std::cos(lat1);

            for (int j = 0; j < subdivisions; ++j) {
                float lngA = 2 * M_PI * (float)(j - 1) / subdivisions;
                float lngB = 2 * M_PI * (float)j / subdivisions;
                float xa = std::cos(lngA), ya = std::sin(lngA);
                float xb = std::cos(lngB), yb = std::sin(lngB);
                quads.push_back({xa * zr0 * 0.5f, ya * zr0 * 0.5f, z0 * 0.5f});
                quads.push_back({xb * zr0 * 0.5f, yb * zr0 * 0.5f, z0 * 0.5f});
                quads.push_back({xb * zr1 * 0.5f, yb * zr1 * 0.5f, z1 * 0.5f});
                quads.push_back({xa * zr1 * 0.5f, ya * zr1 * 0.5f, z1 * 0.5f});
            }
        }
        return draw::quadsToTris(quads);
    }();
    return tris;
}

} // namespace

void Form::draw() const {
    Renderer& r = currentRenderer();

    // These never had normals (no glNormal3f anywhere in the original), so they
    // were always drawn unlit — drawSolid preserves that exactly. White/opaque is
    // what the inherited glColor state gave them.
    const glm::vec4 kFlatWhite(1.0f, 1.0f, 1.0f, 1.0f);

    switch (shape) {
        case ShapeType::Cube:
            r.pushModel(glm::scale(glm::mat4(1.0f), dimensions));
            r.drawSolid(unitCubeTris(), kFlatWhite, Blend::Opaque, /*depthWrite=*/true);
            r.popModel();
            break;
        case ShapeType::Sphere:
            r.pushModel(glm::scale(glm::mat4(1.0f), dimensions));
            r.drawSolid(unitSphereTris(), kFlatWhite, Blend::Opaque, /*depthWrite=*/true);
            r.popModel();
            break;
        case ShapeType::Custom:
        default:
            // Custom shapes not yet implemented
            break;
    }
}
