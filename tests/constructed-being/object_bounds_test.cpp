#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
#include <cstdio>
#include <algorithm>

#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include <vector>

extern MaterialManager materials;

class MockRenderer : public Renderer {
public:
    glm::vec3 lastImplicitExtents = glm::vec3(0.0f);
    bool drawImplicitCalled = false;
    
    void applyCamera(const glm::mat4&, const glm::mat4&, const glm::vec3&) override {}
    void applyModel(const glm::mat4&) override {}
    void drawMesh(const geom::TessMesh&, const RenderMaterial&) override {}
    
    void drawImplicit(const geom::SdfNode& field, const glm::vec3& extents,
                      const RenderMaterial& mat, const geom::FieldNode* fieldNode,
                      uint64_t memo_id, uint32_t struct_rev, const geom::HeightGrid* grid, uint32_t param_rev) override {
        drawImplicitCalled = true;
        lastImplicitExtents = extents;
    }
    bool rendersImplicitExactly() const override { return true; }
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>&, const glm::vec4&, float, Blend) override {}
    void drawOverlay(const geom::TessMesh&, const glm::vec4&, float, bool) override {}
    void drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) override {}
    void begin2D(uint32_t width, uint32_t height) override {}
    void end2D() override {}
    void drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) override {}
    void drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) override {}
    void drawImage2D(const uint8_t*, uint32_t, uint32_t, const glm::vec4&, const glm::vec4&) override {}
    TextureHandle uploadTexture(TextureHandle h, const uint8_t*, uint32_t, uint32_t) override { return h; }
    void releaseTexture(TextureHandle) override {}
};

void checkBounds(ObjectTypes::ShapeKind kind, const ObjectTypes::ShapeParams& p, float minExpectedExtent, const char* name) {
    MockRenderer mockR;
    setCurrentRenderer(&mockR);

    Object obj;
    obj.setShape(kind, p);
    
    mockR.drawImplicitCalled = false;
    obj.drawObject();

    if (!mockR.drawImplicitCalled) {
        std::printf("FAIL: %s did not call drawImplicit (analytic fallback failed?)\n", name);
        std::exit(1);
    }
    if (mockR.lastImplicitExtents.x < minExpectedExtent) {
        std::printf("FAIL: %s bounds are too small: %f (expected >= %f)\n", name, mockR.lastImplicitExtents.x, minExpectedExtent);
        std::exit(1);
    }
    std::printf("PASS: %s bounds = %f\n", name, mockR.lastImplicitExtents.x);
}

int main() {
    ObjectTypes::ShapeParams p;

    // 1. Sphere
    p = {};
    p.r = 2.5f;
    checkBounds(ObjectTypes::ShapeKind::Sphere, p, 2.75f, "Sphere");

    // 2. Ellipsoid
    p = {};
    p.r = 1.0f; p.ry = 3.0f; p.rz = 1.5f;
    checkBounds(ObjectTypes::ShapeKind::Ellipsoid, p, 3.25f, "Ellipsoid");

    // 3. Ovoid
    p = {};
    p.r = 2.0f; p.ovoidAsym = 0.5f;
    checkBounds(ObjectTypes::ShapeKind::Ovoid, p, 2.25f, "Ovoid");

    // 4. Paraboloid
    p = {};
    p.paraboloidA = 0.1f; p.halfH = 0.5f; 
    // r = sqrt(2 * 0.5 / 0.1) = sqrt(10) ~ 3.16 -> extent ~ 3.41
    checkBounds(ObjectTypes::ShapeKind::Paraboloid, p, 3.4f, "Paraboloid");

    // 5. Torus
    p = {};
    p.majorR = 1.0f; p.minorR = 0.2f;
    // max radius = 1.2 -> extent ~ 1.45
    checkBounds(ObjectTypes::ShapeKind::Torus, p, 1.45f, "Torus");

    // 6. Cylinder (Capped)
    p = {};
    p.r = 2.0f; p.halfH = 0.5f;
    checkBounds(ObjectTypes::ShapeKind::Cylinder, p, 2.25f, "Cylinder");

    // 7. Cone (Capped)
    p = {};
    p.r = 1.0f; p.halfH = 2.5f;
    checkBounds(ObjectTypes::ShapeKind::Cone, p, 2.75f, "Cone");

    std::printf("object_bounds_test: ALL OK\n");
    return 0;
}
