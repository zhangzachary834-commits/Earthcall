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
#include "Singularity/Storage/Schema/Earthcall_generated.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include <cmath>
#include <cstdio>

extern MaterialManager materials;   // global Material beings (globals.cpp)

namespace {

bool nearf(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }
bool nearVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}

// One split-substrate cycle (.ecform semantic graph + .ecmatter FlatBuffers physical matter)
Object reborn(const Object& src) {
    // 1. Semantic parchment (.ecform)
    nlohmann::json j;
    to_json(j, src);
    Object out;
    from_json(j, out);

    // 2. Physical matter (.ecmatter)
    flatbuffers::FlatBufferBuilder builder(1024);
    glm::mat4 t = src.getTransform();
    std::vector<float> tf_data(16);
    const float* t_ptr = glm::value_ptr(t);
    for (int m = 0; m < 16; ++m) tf_data[m] = t_ptr[m];
    auto tf_vec = builder.CreateVector(tf_data);

    flatbuffers::Offset<Earthcall::Schema::PolyhedronData> poly_offset = 0;
    if (src.getShapeKind() == Object::ShapeKind::Polyhedron) {
        const auto& poly = src.getPolyhedronData();
        std::vector<Earthcall::Schema::Vec3> fbs_verts;
        for (const auto& v : poly.vertices) fbs_verts.emplace_back(v.x, v.y, v.z);
        auto verts_vec = builder.CreateVectorOfStructs(fbs_verts);
        std::vector<int> fData;
        std::vector<int> fOffsets;
        for (const auto& face : poly.faces) {
            fOffsets.push_back(static_cast<int>(fData.size()));
            for (int v_idx : face) fData.push_back(v_idx);
        }
        fOffsets.push_back(static_cast<int>(fData.size()));
        poly_offset = Earthcall::Schema::CreatePolyhedronData(
            builder, verts_vec, builder.CreateVector(fData), builder.CreateVector(fOffsets));
    }

    flatbuffers::Offset<Earthcall::Schema::BezierPatch> patch_offset = 0;
    if (src.hasPatch()) {
        const auto& patch = src.getPatchData();
        std::vector<Earthcall::Schema::Vec3> fbs_ctrl;
        for (const auto& c : patch.ctrl) fbs_ctrl.emplace_back(c.x, c.y, c.z);
        patch_offset = Earthcall::Schema::CreateBezierPatch(
            builder, patch.du, patch.dv, builder.CreateVectorOfStructs(fbs_ctrl));
    }

    flatbuffers::Offset<Earthcall::Schema::FieldData> field_offset = 0;
    if (src.hasField()) {
        const auto& fd = src.getFieldData();
        Earthcall::Schema::Vec3 f_ext(src.getFieldExtent().x, src.getFieldExtent().y, src.getFieldExtent().z);
        Earthcall::Schema::Vec3 dims(fd.dims.x, fd.dims.y, fd.dims.z);
        Earthcall::Schema::Vec3 offset(fd.offset.x, fd.offset.y, fd.offset.z);
        auto expr_str = builder.CreateString(fd.expr);
        auto root_node = Earthcall::Schema::CreateSdfNode(
            builder,
            static_cast<int>(fd.prim),
            static_cast<int>(fd.op),
            0, 0, 0,
            &dims,
            &offset,
            fd.p0,
            fd.p1,
            fd.t,
            expr_str
        );
        field_offset = Earthcall::Schema::CreateFieldData(builder, &f_ext, root_node);
    }

    std::vector<flatbuffers::Offset<Earthcall::Schema::FaceTexture>> fts;
    auto srcMat = materials.get(src.materialId());
    if (srcMat) {
        for (size_t f = 0; f < srcMat->faceTextures.size(); ++f) {
            const auto& ft = srcMat->faceTextures[f];
            if (!ft.pixels.empty()) {
                auto pix_vec = builder.CreateVector(ft.pixels);
                fts.push_back(Earthcall::Schema::CreateFaceTexture(
                    builder, static_cast<int>(f), ft.size, pix_vec));
            }
        }
    }
    auto fts_vec = fts.empty() ? 0 : builder.CreateVector(fts);

    std::vector<Earthcall::Schema::Vec3> fbs_colors;
    for (int f = 0; f < 6; ++f) {
        fbs_colors.emplace_back(src.faceColors[f][0], src.faceColors[f][1], src.faceColors[f][2]);
    }
    auto fbs_colors_vec = builder.CreateVectorOfStructs(fbs_colors);

    Earthcall::Schema::Vec3 fbs_center(src.getCenter().x, src.getCenter().y, src.getCenter().z);
    Earthcall::Schema::Vec3 fbs_axis(src.getAuthoritativeAxis().x, src.getAuthoritativeAxis().y, src.getAuthoritativeAxis().z);
    Earthcall::Schema::Vec3 fbs_target_rot(src.getTargetRotationEulerDegrees().x, src.getTargetRotationEulerDegrees().y, src.getTargetRotationEulerDegrees().z);

    auto entity = Earthcall::Schema::CreateEntity(
        builder,
        builder.CreateString(src.getIdentifier()),
        builder.CreateString(src.getObjectType()),
        tf_vec,
        poly_offset,
        patch_offset,
        0, // smooth_offset
        field_offset,
        fts_vec,
        fbs_colors_vec,
        0, // sdf_nodes
        0, // laws
        builder.CreateString(src.materialId()),
        &fbs_center,
        &fbs_axis,
        &fbs_target_rot,
        src.getRotationResponsiveness()
    );
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> ents = { entity };
    auto chunk = Earthcall::Schema::CreateSaveChunk(
        builder, builder.CreateString("single_obj"), builder.CreateVector(ents));
    builder.Finish(chunk);

    // Hydrate matter
    const auto* fbsChunk = Earthcall::Schema::GetSaveChunk(builder.GetBufferPointer());
    if (fbsChunk && fbsChunk->entities() && fbsChunk->entities()->size() > 0) {
        const auto* ent = fbsChunk->entities()->Get(0);
        if (ent->transform() && ent->transform()->size() == 16) {
            std::vector<float> tv(ent->transform()->begin(), ent->transform()->end());
            glm::mat4 m(1.0f);
            std::memcpy(glm::value_ptr(m), tv.data(), sizeof(float)*16);
            out.setTransform(m);
        }
        if (ent->center()) {
            out.setCenter(glm::vec3(ent->center()->x(), ent->center()->y(), ent->center()->z()));
        }
        if (ent->authoritative_axis()) {
            out.setAuthoritativeAxis(glm::vec3(ent->authoritative_axis()->x(), ent->authoritative_axis()->y(), ent->authoritative_axis()->z()));
        }
        if (ent->target_rotation()) {
            out.setTargetRotationEulerDegrees(glm::vec3(ent->target_rotation()->x(), ent->target_rotation()->y(), ent->target_rotation()->z()));
        }
        out.setRotationResponsiveness(ent->rotation_responsiveness());
        if (ent->polyhedron() && ent->polyhedron()->vertices() && ent->polyhedron()->face_data() && ent->polyhedron()->face_offsets()) {
            const auto* poly = ent->polyhedron();
            std::vector<glm::vec3> verts;
            for (const auto* v : *poly->vertices()) verts.emplace_back(v->x(), v->y(), v->z());
            const auto* fD = poly->face_data();
            const auto* fO = poly->face_offsets();
            std::vector<std::vector<int>> faces;
            if (fO->size() >= 2) {
                for (size_t i = 0; i + 1 < fO->size(); ++i) {
                    int start = fO->Get(i);
                    int end = fO->Get(i + 1);
                    std::vector<int> face;
                    for (int fi = start; fi < end && fi < (int)fD->size(); ++fi) face.push_back(fD->Get(fi));
                    faces.push_back(std::move(face));
                }
            }
            if (!verts.empty() && !faces.empty()) {
                out.setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
            }
        }
        if (ent->field() && ent->field()->root_node()) {
            const auto* fbsField = ent->field();
            const auto* root = fbsField->root_node();
            geom::SdfNode node;
            node.prim = static_cast<geom::SdfPrim>(root->type());
            node.op = static_cast<geom::SdfOp>(root->operation());
            if (root->dims()) node.dims = glm::vec3(root->dims()->x(), root->dims()->y(), root->dims()->z());
            if (root->offset()) node.offset = glm::vec3(root->offset()->x(), root->offset()->y(), root->offset()->z());
            node.p0 = root->p0();
            node.p1 = root->p1();
            node.t = root->t();
            if (root->expr()) node.expr = root->expr()->str();
            glm::vec3 extent(1.0f);
            if (fbsField->extent()) extent = glm::vec3(fbsField->extent()->x(), fbsField->extent()->y(), fbsField->extent()->z());
            out.setFieldShape(node, extent);
        }
        if (ent->face_textures()) {
            auto outMat = materials.get(out.materialId());
            if (outMat) {
                for (const auto* ft : *ent->face_textures()) {
                    if (!ft || !ft->pixels()) continue;
                    int fIdx = ft->face_index();
                    int sz = ft->size();
                    if (fIdx >= 0 && fIdx < static_cast<int>(outMat->faceTextures.size()) && sz > 0) {
                        auto& oft = outMat->faceTextures[fIdx];
                        oft.size = sz;
                        oft.pixels.assign(ft->pixels()->begin(), ft->pixels()->end());
                        oft.updateWholeGPU();
                    }
                }
            }
        }
        if (ent->face_colors()) {
            for (size_t f = 0; f < ent->face_colors()->size() && f < 6; ++f) {
                const auto* c = ent->face_colors()->Get(f);
                if (c) {
                    out.faceColors[f][0] = c->x();
                    out.faceColors[f][1] = c->y();
                    out.faceColors[f][2] = c->z();
                }
            }
        }
    }

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
