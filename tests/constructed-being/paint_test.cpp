// Paint — the surface an object wears, and who else wears it.
//
// The per-face textures live on the Material BEING, and a Material is shared by
// identifier: every object that has not said otherwise names "material.default"
// and therefore names the same being. So "paint this object red" cannot mean
// "write into the material you resolve" — that repaints the world. It means
// diverge onto your own material first. This test holds that copy-on-write to
// its word from both ends: the painted object must change, and every object
// that was merely sharing must not.
//
// It also pins the two halves of an object's colour together. `faceColors` is
// what the "color" property reads; the material's face textures are what the
// renderer samples. `Object::propSetColor` wrote neither for months — it was an
// empty function after the paint migration, so every law that set "color"
// silently did nothing and every getter reported the default. Nothing caught
// it because nothing asserted that colour reads back what was written.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "json.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

extern MaterialManager materials;   // global Material beings (globals.cpp)

namespace {

bool nearf(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

glm::vec3 colorOf(Singular& being) {
    PropertyValue v;
    assert(PropertyPath::parse("color").getValue(being, v) == PropertyPath::PathResult::Ok);
    return std::get<glm::vec3>(v);
}

// The pixel a face is actually painted, straight out of the material the object
// names — the same buffer the renderer uploads.
void facePixel(const Object& obj, int face, unsigned char out[3]) {
    auto mat = materials.get(obj.materialId());
    assert(mat && "object names a material that does not exist");
    assert(face < static_cast<int>(mat->faceTextures.size()) && "face has no texture");
    const auto& px = mat->faceTextures[face].pixels;
    assert(px.size() >= 4);
    out[0] = px[0]; out[1] = px[1]; out[2] = px[2];
}

} // namespace

int main() {
    // Object construction reaches the renderer boundary for texture upload, so
    // give it a real — but hidden — context.
    if (!glfwInit()) { std::fprintf(stderr, "paint_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "paint_test", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "paint_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    // ------------------------------------------------------------------
    // 1. Unpainted is SHARED, and carries no paint of its own. This is the
    //    state most objects are in and must stay cheap: no material minted,
    //    no textures allocated, the surface showing the material's baseColor.
    // ------------------------------------------------------------------
    {
        Object fresh;
        fresh.setShapeKind(Object::ShapeKind::Cube);
        assert(fresh.materialId() == "material.default");
        assert(materials.defaultMaterial()->faceTextures.empty() &&
               "the default material starts unpainted and must stay that way");

        // A cube has six faces whether or not anyone said so. Reporting zero
        // here is why every paint caller once carried a "?: 6" fallback, and
        // why a cube painted by looping to getFaces() was painted not at all.
        assert(fresh.getFaces() == 6);
        std::printf("  unpainted object shares material.default, six faces, no paint OK\n");
    }

    // ------------------------------------------------------------------
    // 2. The first stroke DIVERGES. The object gets its own material named
    //    after it; the material it was sharing is untouched.
    // ------------------------------------------------------------------
    {
        auto clay = materials.create("paint_test_clay");
        clay->baseColor = glm::vec3(0.6f, 0.4f, 0.2f);
        clay->shininess = 4.0f;

        Object pot;
        pot.setShapeKind(Object::ShapeKind::Cube);
        pot.setMaterialId("material.paint_test_clay");
        pot.setFaceColor(0, 1.0f, 0.0f, 0.0f);

        assert(pot.materialId() == "material." + pot.getIdentifier() &&
               "painting must give the object its own material");
        assert(clay->faceTextures.empty() &&
               "the shared material took paint meant for one object");

        // Diverging carries the shared APPEARANCE over: the pot is still clay,
        // now with red on one face. Losing the base appearance would make the
        // first brush stroke silently reset everything else about the surface.
        auto mine = materials.get(pot.materialId());
        assert(mine && nearf(mine->baseColor.r, 0.6f) && nearf(mine->baseColor.g, 0.4f));
        assert(nearf(mine->shininess, 4.0f));
        std::printf("  first stroke diverges onto its own material, carrying appearance OK\n");
    }

    // ------------------------------------------------------------------
    // 3. Two objects painted differently keep their own paint. Asserting only
    //    that "the first one is red" would still pass if some shared buffer
    //    had gone red; this pins the paint to the object.
    // ------------------------------------------------------------------
    {
        Object a, b;
        a.setShapeKind(Object::ShapeKind::Cube);
        b.setShapeKind(Object::ShapeKind::Cube);
        for (int f = 0; f < a.getFaces(); ++f) a.setFaceColor(f, 1.0f, 0.0f, 0.0f);
        for (int f = 0; f < b.getFaces(); ++f) b.setFaceColor(f, 0.0f, 1.0f, 0.0f);

        assert(a.materialId() != b.materialId());
        unsigned char pa[3], pb[3];
        facePixel(a, 0, pa);
        facePixel(b, 0, pb);
        assert(pa[0] == 255 && pa[1] == 0 && pa[2] == 0);
        assert(pb[0] == 0 && pb[1] == 255 && pb[2] == 0);

        // Repainting one must not reach the other.
        for (int f = 0; f < a.getFaces(); ++f) a.setFaceColor(f, 0.0f, 0.0f, 1.0f);
        facePixel(b, 0, pb);
        assert(pb[0] == 0 && pb[1] == 255 && pb[2] == 0 &&
               "repainting one object bled into another");
        std::printf("  two objects hold their own paint through repaints OK\n");
    }

    // ------------------------------------------------------------------
    // 4. Colour reads back what was written — through the property surface a
    //    law uses, not just through the field. propSetColor was an EMPTY
    //    FUNCTION after the paint migration; this is the assertion whose
    //    absence let that survive.
    // ------------------------------------------------------------------
    {
        Object obj;
        obj.setShapeKind(Object::ShapeKind::Cube);
        assert(PropertyPath::parse("color").setValue(
            obj, PropertyValue(glm::vec3(0.25f, 0.5f, 0.75f))) == PropertyPath::PathResult::Ok);

        const glm::vec3 back = colorOf(obj);
        assert(nearf(back.r, 0.25f) && nearf(back.g, 0.5f) && nearf(back.b, 0.75f) &&
               "the color property did not read back what was written to it");

        // And it reached the paint, not only the slot: a legible property that
        // never touches the surface is a property that lies.
        unsigned char px[3];
        facePixel(obj, 0, px);
        assert(px[0] == 63 && px[1] == 127 && px[2] == 191);
        std::printf("  \"color\" round-trips through the property surface and reaches paint OK\n");
    }

    // ------------------------------------------------------------------
    // 5. Per-face law paths paint the face they name, and only that face —
    //    "recolour one side" is the gesture the face bridges exist for.
    // ------------------------------------------------------------------
    {
        Object obj;
        obj.setShapeKind(Object::ShapeKind::Cube);
        for (int f = 0; f < obj.getFaces(); ++f) obj.setFaceColor(f, 1.0f, 1.0f, 1.0f);

        assert(PropertyPath::parse("face.2.color").setValue(
            obj, PropertyValue(glm::vec3(1.0f, 0.0f, 0.0f))) == PropertyPath::PathResult::Ok);
        assert(nearf(obj.faceColors[2][0], 1.0f) && nearf(obj.faceColors[2][1], 0.0f));
        assert(nearf(obj.faceColors[1][1], 1.0f) && "a neighbouring face was repainted too");

        auto mine = materials.get(obj.materialId());
        assert(mine && mine->faceTextures.size() == 6);
        assert(mine->faceTextures[2].pixels[0] == 255 && mine->faceTextures[2].pixels[1] == 0);
        assert(mine->faceTextures[1].pixels[1] == 255 && "face 1's paint followed face 2's");
        std::printf("  face.N.color paints exactly the face it names OK\n");
    }

    // ------------------------------------------------------------------
    // 6. A shape change re-sizes the paint to the faces the new shape has.
    //    A sphere is one smooth surface; leaving six cube faces behind would
    //    have the renderer sampling paint for faces that no longer exist.
    // ------------------------------------------------------------------
    {
        Object obj;
        obj.setShapeKind(Object::ShapeKind::Cube);
        for (int f = 0; f < obj.getFaces(); ++f) obj.setFaceColor(f, 1.0f, 0.0f, 0.0f);
        assert(materials.get(obj.materialId())->faceTextures.size() == 6);

        obj.setShape(Object::ShapeKind::Sphere);
        assert(obj.getFaces() == 1);
        obj.setFaceColor(0, 0.0f, 0.0f, 1.0f);
        assert(materials.get(obj.materialId())->faceTextures.size() == 1 &&
               "paint kept the old shape's face count");
        std::printf("  paint follows a shape change to the new face count OK\n");
    }

    // ------------------------------------------------------------------
    // 7. Paint on a Material being survives its own JSON. The board's
    //    checkerboard is authored as faceTextures on material.chess.board;
    //    if this round-trip drops pixels, the world loads as a blank slab.
    // ------------------------------------------------------------------
    {
        auto painted = materials.create("paint_test_persist");
        painted->initFaceTextures(2);
        painted->faceTextures[0].size = 2;
        painted->faceTextures[0].pixels = {10, 20, 30, 255, 40, 50, 60, 255,
                                           70, 80, 90, 255, 100, 110, 120, 255};
        painted->faceTextures[1].size = 1;
        painted->faceTextures[1].pixels = {1, 2, 3, 255};

        const nlohmann::json j = painted->toJson();
        assert(j.contains("faceTextures") && j["faceTextures"].size() == 2);

        Material reborn = Material::fromJson(j);
        assert(reborn.faceTextures.size() == 2);
        assert(reborn.faceTextures[0].size == 2);
        assert(reborn.faceTextures[0].pixels.size() == 16);
        assert(reborn.faceTextures[0].pixels[0] == 10 &&
               reborn.faceTextures[0].pixels[2] == 30);
        assert(reborn.faceTextures[1].pixels[0] == 1 &&
               reborn.faceTextures[1].pixels[1] == 2);
        std::printf("  material faceTextures survive JSON round-trip OK\n");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("paint_test: ALL OK\n");
    return 0;
}
