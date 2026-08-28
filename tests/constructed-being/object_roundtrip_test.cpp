// Does an Object survive the world file with everything it was?
//
// `serialization_compat_test` proves the PLUMBING: msgpack encodes, the
// Frontier version pipeline migrates, the file reads back. It says nothing
// about whether an object's own state made the trip, and the two failure modes
// look identical from outside — a save that round-trips perfectly through a
// loader that silently drops half the fields is still a green suite.
//
// That is not hypothetical. `faceColors` was written on every save and read on
// none: the load branch had been commented out while `setFaceColor` did not
// exist, so an object's colour did not survive a save/load at all, for months,
// with the write side making it look covered.
//
// So this test asserts field by field, and it asserts through the SAME doors
// the rest of the substrate uses — property paths where a law would read, the
// public accessors elsewhere. A field that to_json writes belongs here.

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Storage/Serialization.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

extern MaterialManager materials;   // global Material beings (globals.cpp)

namespace {

bool nearf(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }
bool nearVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}

// One save/load cycle, exactly as the world file does it.
Object reborn(const Object& src) {
    nlohmann::json j;
    to_json(j, src);
    Object out;
    from_json(j, out);
    return out;
}

} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "object_roundtrip_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "object_roundtrip_test", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "object_roundtrip_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    // ------------------------------------------------------------------
    // 1. Pose and placement: where the object is, which way it faces, and how
    //    fast it turns to where it is going.
    // ------------------------------------------------------------------
    {
        Object src;
        src.setShapeKind(Object::ShapeKind::Cube);
        src.setPosition(glm::vec3(3.5f, -1.25f, 7.0f));
        src.setCenter(glm::vec3(0.1f, 0.2f, 0.3f));
        src.setAuthoritativeAxis(glm::vec3(0.0f, 0.0f, 1.0f));
        src.setTargetRotationEulerDegrees(glm::vec3(15.0f, 30.0f, 45.0f));
        src.setRotationResponsiveness(0.35f);

        Object out = reborn(src);
        assert(nearVec3(out.getPosition(), glm::vec3(3.5f, -1.25f, 7.0f)));
        assert(nearVec3(out.getCenter(), glm::vec3(0.1f, 0.2f, 0.3f)));
        assert(nearVec3(out.getAuthoritativeAxis(), glm::vec3(0.0f, 0.0f, 1.0f)));
        assert(nearVec3(out.getTargetRotationEulerDegrees(), glm::vec3(15.0f, 30.0f, 45.0f)));
        assert(nearf(out.getRotationResponsiveness(), 0.35f));
        std::printf("  pose, centre, axis, target rotation and responsiveness survive OK\n");
    }

    // ------------------------------------------------------------------
    // 2. Shape KIND and its PARAMS. ObjectTypes.hpp promises the params are
    //    "persisted so parameterized shapes round-trip"; a sphere that comes
    //    back at the default radius is that promise quietly broken.
    // ------------------------------------------------------------------
    {
        Object src;
        Object::ShapeParams p;
        p.majorR = 0.42f;
        p.minorR = 0.09f;
        src.setShape(Object::ShapeKind::Torus, p);

        Object out = reborn(src);
        assert(out.getShapeKind() == Object::ShapeKind::Torus);
        assert(nearf(out.getShapeParams().majorR, 0.42f));
        assert(nearf(out.getShapeParams().minorR, 0.09f));
        assert(out.getSpatialKind() == Object::SpatialKind::SmoothSurface &&
               "the loaded torus was not rebuilt as a surface, only labelled one");

        // And the params still DRIVE the geometry after loading, rather than
        // being inert numbers riding alongside a default mesh.
        assert(PropertyPath::parse("shape.majorR").setValue(
            out, PropertyValue(0.8f)) == PropertyPath::PathResult::Ok);
        assert(nearf(out.getShapeParams().majorR, 0.8f));
        std::printf("  shape kind and its parameters survive, and still shape the form OK\n");
    }

    // ------------------------------------------------------------------
    // 3. Colour. The regression this file exists for.
    // ------------------------------------------------------------------
    {
        Object src;
        src.setShapeKind(Object::ShapeKind::Cube);
        for (int f = 0; f < src.getFaces(); ++f)
            src.setFaceColor(f, 0.25f, 0.5f, 0.75f);

        Object out = reborn(src);
        for (int f = 0; f < 6; ++f) {
            assert(nearf(out.faceColors[f][0], 0.25f));
            assert(nearf(out.faceColors[f][1], 0.50f));
            assert(nearf(out.faceColors[f][2], 0.75f));
        }
        // Loading must make it PAINT again, not merely restore the numbers:
        // the renderer samples the material, so a loaded world that reads
        // colours into slots nothing draws from is still a colourless world.
        auto mat = materials.get(out.materialId());
        assert(mat && !mat->faceTextures.empty() &&
               "a loaded object's colour never became paint");
        assert(mat->faceTextures[0].pixels[0] == 63 &&
               mat->faceTextures[0].pixels[2] == 191);
        std::printf("  face colours survive and become paint again OK\n");
    }

    // ------------------------------------------------------------------
    // 4. Authored vocabulary. Dynamic properties are what a Person adds to a
    //    being that the type system never knew about — the whole point of
    //    refusing new C++ classes for domain nouns. If they do not persist,
    //    authoring does not persist.
    // ------------------------------------------------------------------
    {
        Object src;
        src.setDynamicProperty("acoustic.amplitude", PropertyValue(0.5));
        src.setDynamicProperty("ritual.phase", PropertyValue(std::string("waxing")));
        src.setDynamicProperty("harvest.ready", PropertyValue(true));

        Object out = reborn(src);
        PropertyValue v;
        assert(PropertyPath::parse("acoustic.amplitude").getValue(out, v) == PropertyPath::PathResult::Ok);
        double amp = 0.0;
        assert(propertyValueToNumber(v, amp) && nearf(static_cast<float>(amp), 0.5f));
        assert(PropertyPath::parse("ritual.phase").getValue(out, v) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(v) == "waxing");
        assert(PropertyPath::parse("harvest.ready").getValue(out, v) == PropertyPath::PathResult::Ok);
        assert(std::get<bool>(v) == true);
        std::printf("  authored (dynamic) properties survive, law-readable by path OK\n");
    }

    // ------------------------------------------------------------------
    // 5. Provenance: who has a stake in which of this object's properties.
    //    Nothing enters the world without an author, and that record has to
    //    outlive the session or the claim is unverifiable next time.
    // ------------------------------------------------------------------
    {
        Object src;
        src.addStakeholder("position.y", "person-zack", "law-gravity", 1234567890);

        Object out = reborn(src);
        assert(out.stakeholders().size() == 1);
        const auto& sh = out.stakeholders()[0];
        assert(sh.propertyPath == "position.y");
        assert(sh.authorId == "person-zack");
        assert(sh.lawId == "law-gravity");
        assert(sh.timestamp == 1234567890);
        std::printf("  stakeholder provenance survives OK\n");
    }

    // ------------------------------------------------------------------
    // 6. Polyhedron geometry — variable vertex/face data, packed binary. The
    //    only shape whose form is not reproducible from a kind and params, so
    //    it is the only one that must carry its whole body through the file.
    // ------------------------------------------------------------------
    {
        Object src;
        src.createIcosahedron();
        const int faces = src.getFaces();
        const std::size_t verts = src.getPolyhedronData().vertices.size();
        assert(faces > 0 && verts > 0);

        Object out = reborn(src);
        assert(out.getShapeKind() == Object::ShapeKind::Polyhedron);
        assert(out.getFaces() == faces && "the loaded polyhedron has a different face count");
        assert(out.getPolyhedronData().vertices.size() == verts);
        for (std::size_t i = 0; i < verts; ++i) {
            assert(nearVec3(out.getPolyhedronData().vertices[i],
                            src.getPolyhedronData().vertices[i]));
        }
        std::printf("  polyhedron vertices and faces survive the binary pack OK\n");
    }

    // ------------------------------------------------------------------
    // 7. A field object: the SDF expression itself, not a mesh baked from it.
    //    A field that loads as its bounding box looks plausible and is wrong.
    // ------------------------------------------------------------------
    {
        geom::SdfNode sphere;
        sphere.op   = geom::SdfOp::Leaf;
        sphere.prim = geom::SdfPrim::Sphere;
        sphere.dims = glm::vec3(0.55f);

        Object src;
        src.setFieldShape(sphere, glm::vec3(1.5f));

        Object out = reborn(src);
        assert(out.hasField() && "the field became something other than a field");
        assert(out.getFieldData().prim == geom::SdfPrim::Sphere);
        assert(nearf(out.getFieldData().dims.x, 0.55f));
        assert(nearf(out.getFieldExtent().x, 1.5f));
        std::printf("  field expression and extent survive OK\n");
    }

    // ------------------------------------------------------------------
    // 8. The material REFERENCE, not the material. An object names its
    //    material by identifier the way a Relation names its endpoints; the
    //    Material beings are saved by their own manager. Inlining a copy per
    //    object would fork every shared surface on the next load.
    // ------------------------------------------------------------------
    {
        auto slate = materials.create("roundtrip_slate");
        slate->baseColor = glm::vec3(0.2f, 0.2f, 0.25f);

        Object src;
        src.setMaterialId("material.roundtrip_slate");
        Object out = reborn(src);
        assert(out.materialId() == "material.roundtrip_slate" &&
               "the object lost track of which material being paints it");
        std::printf("  material reference survives by identifier OK\n");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("object_roundtrip_test: ALL OK\n");
    return 0;
}
