// Regression witnesses for shape-truth loss across Earthcall's persistence
// boundary.  These cases are deliberately adversarial: each is a failure that
// can leave a save file looking plausible while the manifested form is wrong.
//
// Two paths are exercised:
//   1. semantic Object JSON alone (the independently loadable Zone identity),
//   2. semantic JSON followed by the REAL ZoneManager .ecmatter hydration path.
//
// The second path matters because the semantic reader can be perfectly correct
// and then have its complete SDF overwritten by a shallower physical sidecar.

#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Patch.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"

#include <flatbuffers/flatbuffers.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int checks = 0;
int failures = 0;

void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}

bool near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

geom::SdfNode authoredCompositeField() {
    auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.8f));
    sphere.offset = glm::vec3(-0.35f, 0.0f, 0.0f);
    auto torus = geom::SdfNode::leaf(geom::SdfPrim::Torus,
                                     glm::vec3(1.1f, 0.22f, 0.0f));
    torus.offset = glm::vec3(0.4f, 0.0f, 0.0f);
    return geom::SdfNode::binary(geom::SdfOp::SmoothUnion, sphere, torus, 0.17f);
}

std::vector<uint8_t> finishMatter(
    flatbuffers::FlatBufferBuilder& builder,
    const std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>>& entities) {
    auto chunkId = builder.CreateString("shape-integrity-adversarial");
    auto entitiesVec = builder.CreateVector(entities);
    auto chunk = Earthcall::Schema::CreateSaveChunk(builder, chunkId, entitiesVec);
    builder.Finish(chunk);
    const uint8_t* data = builder.GetBufferPointer();
    return std::vector<uint8_t>(data, data + builder.GetSize());
}

flatbuffers::Offset<Earthcall::Schema::Entity> bareMatterEntity(
    flatbuffers::FlatBufferBuilder& builder,
    const std::string& id,
    const std::string& owner,
    flatbuffers::Offset<Earthcall::Schema::PolyhedronData> poly = 0,
    flatbuffers::Offset<Earthcall::Schema::SmoothSurfaceData> smooth = 0,
    flatbuffers::Offset<Earthcall::Schema::FieldData> field = 0) {
    auto idStr = builder.CreateString(id);
    auto ownerStr = builder.CreateString(owner);
    return Earthcall::Schema::CreateEntity(
        builder,
        idStr,
        0, // name
        0, // transform
        poly,
        0, // patch
        smooth,
        field,
        0, // face textures
        0, // face colors
        0, // sdf nodes
        0, // laws
        0, // material id
        nullptr,
        nullptr,
        nullptr,
        1.0f,
        ownerStr);
}
} // namespace

int main() {
    // ------------------------------------------------------------------
    // 1. LIVE split-substrate path: semantic JSON reconstructs a complete
    //    SmoothUnion, then legacy .ecmatter presents only the root operator.
    //    Matter is cache/physical density; it must not erase authored math.
    // ------------------------------------------------------------------
    {
        auto sourceZone = std::make_shared<Zone>("shape-integrity-zone", "strict");
        auto source = std::make_shared<Object>("shape-integrity-field");
        source->setFieldShape(authoredCompositeField(), glm::vec3(3.0f, 2.0f, 3.0f));
        sourceZone->addObject(source);

        ZoneManager writer;
        writer.addZone(sourceZone);
        const std::vector<uint8_t> matter = writer.buildMatterFlatBuffer();
        check(!matter.empty(), "matter sidecar was produced by the real ZoneManager writer");

        nlohmann::json semantic;
        to_json(semantic, *source);
        auto hydrated = std::make_shared<Object>("shape-integrity-field");
        from_json(semantic, *hydrated);
        check(hydrated->hasField() && hydrated->getFieldData().children.size() == 2,
              "semantic hydration reconstructs the complete SmoothUnion before matter");

        auto destinationZone = std::make_shared<Zone>("shape-integrity-zone", "strict");
        destinationZone->addObject(hydrated);
        ZoneManager reader;
        reader.addZone(destinationZone);
        reader.applyMatterFlatBuffer(matter);

        check(hydrated->hasField(), "matter hydration keeps the Object a Field");
        check(hydrated->getFieldData().op == geom::SdfOp::SmoothUnion,
              "matter hydration keeps the authored field operator");
        check(hydrated->getFieldData().children.size() == 2,
              "shallow .ecmatter root cannot erase authored SDF children");
        if (hydrated->getFieldData().children.size() == 2) {
            check(hydrated->getFieldData().children[0] &&
                      hydrated->getFieldData().children[0]->prim == geom::SdfPrim::Sphere,
                  "left SDF child survives matter hydration");
            check(hydrated->getFieldData().children[1] &&
                      hydrated->getFieldData().children[1]->prim == geom::SdfPrim::Torus,
                  "right SDF child survives matter hydration");
        }
        check(near(hydrated->getFieldData().t, 0.17f),
              "SmoothUnion blend survives matter hydration");
    }

    // ------------------------------------------------------------------
    // 2. Historical law-spawn malformed implicit form: prim=Sphere plus an
    //    expression string. setFieldShape must understand the expression as
    //    the form, promote the leaf to Expr, and compile its executable RPN.
    // ------------------------------------------------------------------
    {
        geom::SdfNode malformed =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f));
        malformed.expr = "x*x+y*y+z*z-0.25";

        Object implicit("implicit-normalization-test");
        implicit.setFieldShape(malformed, glm::vec3(1.0f));

        check(implicit.getFieldData().prim == geom::SdfPrim::Expr,
              "an authored expression cannot masquerade as a Sphere primitive");
        check(!implicit.getFieldData().rpn.empty(),
              "implicit expression is compiled when it enters the Object");
        check(geom::evalSdf(implicit.getFieldData(), glm::vec3(0.0f)) < 0.0f &&
                  geom::evalSdf(implicit.getFieldData(), glm::vec3(1.0f, 0.0f, 0.0f)) > 0.0f,
              "compiled implicit field has the expected inside/outside semantics");
    }

    // ------------------------------------------------------------------
    // 3. Temporal merge poison: an old base record still has a `field`
    //    payload, while the newer record declares Sphere. Payload presence
    //    must NEVER outrank the explicit current shape discriminant.
    // ------------------------------------------------------------------
    {
        Object oldField("stale-payload-test");
        oldField.setFieldShape(authoredCompositeField(), glm::vec3(2.0f));
        nlohmann::json mergedLookingRecord;
        to_json(mergedLookingRecord, oldField);

        mergedLookingRecord["shapeKind"] = static_cast<int>(Object::ShapeKind::Sphere);
        mergedLookingRecord["geometryType"] = static_cast<int>(Object::ShapeKind::Sphere);
        mergedLookingRecord["shape"]["kind"] = static_cast<int>(Object::ShapeKind::Sphere);
        mergedLookingRecord["shape"]["params"]["r"] = 0.73f;
        // Deliberately leave mergedLookingRecord["field"] behind, exactly as
        // JSON merge-patch does when the overlay simply omits an obsolete key.

        Object current("stale-payload-test");
        from_json(mergedLookingRecord, current);
        check(current.getShapeKind() == Object::ShapeKind::Sphere,
              "declared current shape wins over a stale Field payload");
        check(!current.hasField(), "stale Field payload is not resurrected");
        check(near(current.getShapeParams().r, 0.73f),
              "named current shape parameters survive stale-payload hydration");
    }

    // ------------------------------------------------------------------
    // 4. Shape ordinals are persisted integers. A future/corrupt ordinal is
    //    an ordinary load event; it must have a deterministic safe answer.
    // ------------------------------------------------------------------
    {
        nlohmann::json bad{
            {"objectID", "invalid-shape-test"},
            {"shapeKind", 99999},
            {"shapeParams", {0.5, 0.32, 0.5, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0}}
        };
        Object hydrated("invalid-shape-test");
        from_json(bad, hydrated);
        check(hydrated.getShapeKind() == Object::ShapeKind::Cube,
              "invalid serialized ShapeKind refuses to become an invalid enum value");
    }

    // ------------------------------------------------------------------
    // 5. Named shape parameters are the human/agent-readable authority while
    //    the append-only positional array remains a legacy compatibility arm.
    // ------------------------------------------------------------------
    {
        Object torus("named-shape-params-test");
        Object::ShapeParams p;
        p.majorR = 0.91f;
        p.minorR = 0.07f;
        torus.setShape(Object::ShapeKind::Torus, p);
        nlohmann::json j;
        to_json(j, torus);

        check(j.contains("shape") && j["shape"].contains("params") &&
                  near(j["shape"]["params"]["majorR"].get<float>(), 0.91f),
              "writer emits self-describing named shape parameters");
        j["shapeParams"][4] = 0.11f;            // stale legacy value
        j["shape"]["params"]["majorR"] = 0.91f; // current semantic value

        Object back("named-shape-params-test");
        from_json(j, back);
        check(near(back.getShapeParams().majorR, 0.91f),
              "named semantic parameter overrides stale positional compatibility data");
    }

    // ------------------------------------------------------------------
    // 6. A Zone identity is independently complete for sculpted topology.
    //    Patch and custom Polyhedron data may not require .ecmatter to exist.
    // ------------------------------------------------------------------
    {
        Object patchSource("semantic-patch-test");
        geom::BezierPatch patch = geom::makeBezierGrid(3, 3, 0.5f);
        patch.ctrl[0] = glm::vec3(-2.0f, 0.25f, 1.0f);
        patchSource.setBezierPatch(patch);

        nlohmann::json j;
        to_json(j, patchSource);
        check(j.contains("patch"), "semantic Object JSON carries the Bezier control net");
        Object patchBack("semantic-patch-test");
        from_json(j, patchBack);
        check(patchBack.hasPatch() && patchBack.getPatchData().ctrl.size() == patch.ctrl.size(),
              "Bezier patch survives semantic-only round trip");
        if (patchBack.hasPatch() && !patchBack.getPatchData().ctrl.empty()) {
            check(near(patchBack.getPatchData().ctrl[0].x, -2.0f),
                  "Bezier control point survives semantic-only round trip");
        }

        Object polySource("semantic-polyhedron-test");
        polySource.createIcosahedron();
        nlohmann::json pj;
        to_json(pj, polySource);
        check(pj.contains("polyhedron"), "semantic Object JSON carries custom polyhedron topology");
        Object polyBack("semantic-polyhedron-test");
        from_json(pj, polyBack);
        check(polyBack.getShapeKind() == Object::ShapeKind::Polyhedron &&
                  polyBack.getPolyhedronData().vertices.size() ==
                      polySource.getPolyhedronData().vertices.size(),
              "custom polyhedron survives semantic-only round trip");
    }

    // ------------------------------------------------------------------
    // 7. ObjectConcept has its own shape persistence boundary. Its historical
    //    `params` reader required EXACTLY nine slots, so appending two entries
    //    would break old readers. Keep that array nine-wide and carry the two
    //    later 2D dimensions as additive named fields instead.
    // ------------------------------------------------------------------
    {
        ObjectConcept::MemberTemplate member;
        member.kind = Object::ShapeKind::Shape2D;
        member.params.r = 0.41f;
        member.params.fillet = 0.13f;
        member.params.width2D = 321.0f;
        member.params.height2D = 123.0f;

        const nlohmann::json j = member.toJson();
        check(j.contains("params") && j["params"].is_array() && j["params"].size() == 9,
              "ObjectConcept keeps the historical nine-slot params arm stable");
        check(j.contains("width2D") && j.contains("height2D") &&
                  near(j["width2D"].get<float>(), 321.0f) &&
                  near(j["height2D"].get<float>(), 123.0f),
              "ObjectConcept adds 2D dimensions without changing the legacy array width");
        const auto back = ObjectConcept::MemberTemplate::fromJson(j);
        check(back.kind == Object::ShapeKind::Shape2D &&
                  near(back.params.width2D, 321.0f) && near(back.params.height2D, 123.0f),
              "ObjectConcept preserves Shape2D width and height");

        nlohmann::json legacy = j;
        legacy.erase("width2D");
        legacy.erase("height2D");
        const auto legacyBack = ObjectConcept::MemberTemplate::fromJson(legacy);
        check(near(legacyBack.params.r, 0.41f) && near(legacyBack.params.fillet, 0.13f),
              "historical nine-slot ObjectConcept members remain readable");

        nlohmann::json transitional = legacy;
        transitional["params"].push_back(321.0f);
        transitional["params"].push_back(123.0f);
        const auto transitionalBack = ObjectConcept::MemberTemplate::fromJson(transitional);
        check(near(transitionalBack.params.width2D, 321.0f) &&
                  near(transitionalBack.params.height2D, 123.0f),
              "brief eleven-slot development ObjectConcept records remain readable");

        nlohmann::json corrupt = j;
        corrupt["kind"] = 99999;
        const auto corruptBack = ObjectConcept::MemberTemplate::fromJson(corrupt);
        check(corruptBack.kind == Object::ShapeKind::Cube,
              "ObjectConcept refuses an invalid persisted ShapeKind ordinal");
        corrupt["kind"] = "not-an-integer";
        const auto wrongTypeBack = ObjectConcept::MemberTemplate::fromJson(corrupt);
        check(wrongTypeBack.kind == Object::ShapeKind::Cube,
              "ObjectConcept refuses a non-integer persisted ShapeKind without throwing");
    }

    // ------------------------------------------------------------------
    // 8. BodyParts are a second persisted ShapeKind reader. They used to
    //    direct-cast arbitrary integers for both the primary shape and nested
    //    sub-objects; a corrupt/future ordinal must not enter the enum.
    // ------------------------------------------------------------------
    {
        BodyPart part("body-shape-validation", BodyPart::Type::Arm,
                      ObjectTypes::ShapeKind::Sphere, glm::vec3(1.0f));
        nlohmann::json bodyJson = bodyPartToJson(part);
        bodyJson["geometryType"] = 99999;
        bodyPartFromJson(bodyJson, part);
        check(part.getPrimaryShape() == ObjectTypes::ShapeKind::Cube,
              "BodyPart refuses an invalid primary ShapeKind ordinal");

        nlohmann::json sub = nlohmann::json::object();
        sub["shapeKind"] = 99999;
        sub["geometryType"] = 99999;
        sub["transform"] = std::vector<float>{
            1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
        };
        bodyJson["subObjects"] = nlohmann::json::array({sub});
        bodyPartFromJson(bodyJson, part);
        check(part.getSubObjectCount() == 1 && part.getSubObject(0) &&
                  part.getSubObject(0)->getShapeKind() == Object::ShapeKind::Cube,
              "BodyPart refuses an invalid sub-object ShapeKind ordinal");
    }

    // ------------------------------------------------------------------
    // 9. Matter is a legacy recovery/cache substrate, never a competing
    //    semantic topology authority. Exercise all four topology families
    //    through the REAL writer/reader boundary, including positive legacy
    //    recovery where semantic JSON genuinely carries only a shell.
    // ------------------------------------------------------------------
    {
        const std::string zoneId = "matter-authority-zone";
        auto sourceZone = std::make_shared<Zone>(zoneId, "strict");

        auto stalePatch = std::make_shared<Object>("stale-patch-matter");
        geom::BezierPatch stalePatchData = geom::makeBezierGrid(3, 3, 0.5f);
        stalePatchData.ctrl[0] = glm::vec3(7.0f, 0.0f, 0.0f);
        stalePatch->setBezierPatch(stalePatchData);
        sourceZone->addObject(stalePatch);

        auto stalePoly = std::make_shared<Object>("stale-poly-matter");
        stalePoly->createIcosahedron();
        sourceZone->addObject(stalePoly);

        auto staleSmooth = std::make_shared<Object>("stale-smooth-matter");
        Object::ShapeParams sourceSphereParams;
        sourceSphereParams.r = 0.23f;
        staleSmooth->setShape(Object::ShapeKind::Sphere, sourceSphereParams);
        sourceZone->addObject(staleSmooth);

        auto staleField = std::make_shared<Object>("stale-field-matter");
        staleField->setFieldShape(authoredCompositeField(), glm::vec3(9.0f));
        sourceZone->addObject(staleField);

        auto legacyPatchSource = std::make_shared<Object>("legacy-patch-recovery");
        geom::BezierPatch legacyPatchData = geom::makeBezierGrid(3, 3, 0.5f);
        legacyPatchData.ctrl[0] = glm::vec3(4.25f, 0.0f, 0.0f);
        legacyPatchSource->setBezierPatch(legacyPatchData);
        sourceZone->addObject(legacyPatchSource);

        auto legacyPolySource = std::make_shared<Object>("legacy-poly-recovery");
        legacyPolySource->createTetrahedron();
        sourceZone->addObject(legacyPolySource);

        auto legacyFieldSource = std::make_shared<Object>("legacy-field-recovery");
        legacyFieldSource->setFieldShape(
            geom::SdfNode::leaf(geom::SdfPrim::Box, glm::vec3(0.31f, 0.42f, 0.53f)),
            glm::vec3(4.0f, 5.0f, 6.0f));
        sourceZone->addObject(legacyFieldSource);

        auto legacySmoothSource = std::make_shared<Object>("legacy-smooth-recovery");
        Object::ShapeParams legacySphereParams;
        legacySphereParams.r = 0.62f;
        legacySmoothSource->setShape(Object::ShapeKind::Sphere, legacySphereParams);
        sourceZone->addObject(legacySmoothSource);

        ZoneManager writer;
        writer.addZone(sourceZone);
        const std::vector<uint8_t> matter = writer.buildMatterFlatBuffer();
        check(!matter.empty(), "authority-test matter sidecar was produced");

        auto destinationZone = std::make_shared<Zone>(zoneId, "strict");

        auto currentPatchId = std::make_shared<Object>("stale-patch-matter");
        Object::ShapeParams sphereParams;
        sphereParams.r = 0.77f;
        currentPatchId->setShape(Object::ShapeKind::Sphere, sphereParams);
        destinationZone->addObject(currentPatchId);

        auto currentPolyId = std::make_shared<Object>("stale-poly-matter");
        currentPolyId->setShape(Object::ShapeKind::Sphere, sphereParams);
        destinationZone->addObject(currentPolyId);

        auto currentSmoothId = std::make_shared<Object>("stale-smooth-matter");
        Object::ShapeParams torusParams;
        torusParams.majorR = 0.91f;
        torusParams.minorR = 0.07f;
        currentSmoothId->setShape(Object::ShapeKind::Torus, torusParams);
        destinationZone->addObject(currentSmoothId);

        auto currentFieldId = std::make_shared<Object>("stale-field-matter");
        currentFieldId->setFieldShape(authoredCompositeField(), glm::vec3(3.0f, 2.0f, 3.0f));
        destinationZone->addObject(currentFieldId);

        auto legacyPatch = std::make_shared<Object>("legacy-patch-recovery");
        legacyPatch->setShape(Object::ShapeKind::Patch);
        destinationZone->addObject(legacyPatch);

        auto legacyPoly = std::make_shared<Object>("legacy-poly-recovery");
        legacyPoly->setShape(Object::ShapeKind::Polyhedron);
        destinationZone->addObject(legacyPoly);

        auto legacyField = std::make_shared<Object>("legacy-field-recovery");
        legacyField->setShape(Object::ShapeKind::Field);
        destinationZone->addObject(legacyField);

        auto legacySmooth = std::make_shared<Object>("legacy-smooth-recovery");
        legacySmooth->setShape(Object::ShapeKind::Sphere, legacySphereParams);
        legacySmooth->clearTopologyModel(); // simulate a pre-topology semantic record
        destinationZone->addObject(legacySmooth);

        ZoneManager reader;
        reader.addZone(destinationZone);
        reader.applyMatterFlatBuffer(matter);

        check(currentPatchId->getShapeKind() == Object::ShapeKind::Sphere &&
                  currentPatchId->hasSmoothSurface() && !currentPatchId->hasPatch(),
              "stale Patch matter cannot reclassify a newer semantic Sphere");
        check(currentPolyId->getShapeKind() == Object::ShapeKind::Sphere &&
                  currentPolyId->hasSmoothSurface() &&
                  currentPolyId->getPolyhedronData().vertices.empty(),
              "stale Polyhedron matter cannot reclassify a newer semantic Sphere");
        check(currentSmoothId->getShapeKind() == Object::ShapeKind::Torus &&
                  currentSmoothId->hasSmoothSurface() &&
                  currentSmoothId->getSmoothData().model == geom::SmoothSurfaceData::Model::Parametric &&
                  currentSmoothId->getSmoothData().pkind == geom::SmoothSurfaceData::ParametricKind::Torus &&
                  !currentSmoothId->getSmoothData().params.empty() &&
                  near(currentSmoothId->getSmoothData().params[0], 0.91f),
              "stale SmoothSurface matter cannot split Torus identity from runtime topology");
        check(currentFieldId->hasField() &&
                  currentFieldId->getFieldData().children.size() == 2 &&
                  near(currentFieldId->getFieldExtent().x, 3.0f) &&
                  near(currentFieldId->getFieldExtent().y, 2.0f) &&
                  near(currentFieldId->getFieldExtent().z, 3.0f),
              "stale Field matter cannot replace either semantic tree or semantic extent");

        check(legacyPatch->hasPatch() &&
                  !legacyPatch->getPatchData().ctrl.empty() &&
                  near(legacyPatch->getPatchData().ctrl[0].x, 4.25f),
              "valid legacy Patch matter can recover a semantic Patch shell");
        check(legacyPoly->getShapeKind() == Object::ShapeKind::Polyhedron &&
                  legacyPoly->getPolyhedronData().vertices.size() == 4 &&
                  !legacyPoly->getPolyhedronData().faces.empty(),
              "valid legacy Polyhedron matter can recover missing topology");
        check(legacyField->hasField() &&
                  legacyField->getFieldData().prim == geom::SdfPrim::Box &&
                  near(legacyField->getFieldExtent().x, 4.0f) &&
                  near(legacyField->getFieldExtent().y, 5.0f) &&
                  near(legacyField->getFieldExtent().z, 6.0f),
              "valid leaf Field matter can recover a legacy semantic Field shell");
        check(legacySmooth->getShapeKind() == Object::ShapeKind::Sphere &&
                  legacySmooth->hasSmoothSurface() &&
                  near(legacySmooth->getSmoothData().axes.x, 0.62f),
              "valid SmoothSurface matter can recover missing topology when semantic kind agrees");
    }

    // ------------------------------------------------------------------
    // 10. The Object-level lossy-field guard is also a semantic-extent guard.
    //     This protects non-ZoneManager callers that still present an old
    //     root-only operator shell.
    // ------------------------------------------------------------------
    {
        Object field("field-extent-authority");
        field.setFieldShape(authoredCompositeField(), glm::vec3(2.0f, 3.0f, 4.0f));
        geom::SdfNode shallowOperator;
        shallowOperator.op = geom::SdfOp::SmoothUnion;
        shallowOperator.prim = geom::SdfPrim::Sphere;
        field.setFieldShape(shallowOperator, glm::vec3(99.0f));
        check(field.getFieldData().children.size() == 2 &&
                  near(field.getFieldExtent().x, 2.0f) &&
                  near(field.getFieldExtent().y, 3.0f) &&
                  near(field.getFieldExtent().z, 4.0f),
              "refused lossy Field shells cannot mutate semantic extent as a side effect");
    }

    // ------------------------------------------------------------------
    // 11. FlatBuffers verification proves memory structure, not semantic
    //     geometry. Feed structurally valid but hostile Polyhedron offsets,
    //     face indices, Field enums, and SmoothSurface enums through the real
    //     reader and require refusal without reclassification or allocation
    //     from an unproven range.
    // ------------------------------------------------------------------
    {
        const std::string zoneId = "malformed-matter-zone";
        flatbuffers::FlatBufferBuilder builder(2048);
        std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> entities;

        std::vector<Earthcall::Schema::Vec3> triVerts{
            {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
        };
        auto vertsA = builder.CreateVectorOfStructs(triVerts);
        auto faceDataA = builder.CreateVector(std::vector<int>{0, 1, 2});
        auto offsetsA = builder.CreateVector(std::vector<int>{0, -1, 3});
        auto badOffsetsPoly = Earthcall::Schema::CreatePolyhedronData(
            builder, vertsA, faceDataA, offsetsA);
        entities.push_back(bareMatterEntity(
            builder, "bad-poly-offsets", zoneId, badOffsetsPoly));

        auto vertsB = builder.CreateVectorOfStructs(triVerts);
        auto faceDataB = builder.CreateVector(std::vector<int>{0, 1, 99});
        auto offsetsB = builder.CreateVector(std::vector<int>{0, 3});
        auto badIndexPoly = Earthcall::Schema::CreatePolyhedronData(
            builder, vertsB, faceDataB, offsetsB);
        entities.push_back(bareMatterEntity(
            builder, "bad-poly-index", zoneId, badIndexPoly));

        Earthcall::Schema::Vec3 dims(0.5f, 0.5f, 0.5f);
        Earthcall::Schema::Vec3 offset(0.0f, 0.0f, 0.0f);
        Earthcall::Schema::Vec3 extent(1.0f, 1.0f, 1.0f);
        auto emptyExpr = builder.CreateString("");
        auto badSdf = Earthcall::Schema::CreateSdfNode(
            builder,
            99999, // invalid SdfPrim
            static_cast<int>(geom::SdfOp::Leaf),
            0, 0, 0,
            &dims,
            &offset,
            0.0f, 0.0f, 0.5f,
            emptyExpr);
        auto badField = Earthcall::Schema::CreateFieldData(builder, &extent, badSdf);
        entities.push_back(bareMatterEntity(
            builder, "bad-field-enum", zoneId, 0, 0, badField));

        std::vector<float> identityQ{
            1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
        };
        auto q = builder.CreateVector(identityQ);
        auto smoothParams = builder.CreateVector(std::vector<float>{});
        Earthcall::Schema::Vec3 axes(0.5f, 0.5f, 0.5f);
        auto badSmooth = Earthcall::Schema::CreateSmoothSurfaceData(
            builder,
            true, true, false, false,
            99999, // invalid Model
            q,
            static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere),
            static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus),
            &axes,
            -0.5f, 0.5f,
            smoothParams);
        entities.push_back(bareMatterEntity(
            builder, "bad-smooth-enum", zoneId, 0, badSmooth));

        const std::vector<uint8_t> badMatter = finishMatter(builder, entities);
        auto zone = std::make_shared<Zone>(zoneId, "strict");

        auto badOffsetsTarget = std::make_shared<Object>("bad-poly-offsets");
        badOffsetsTarget->setShape(Object::ShapeKind::Polyhedron);
        zone->addObject(badOffsetsTarget);

        auto badIndexTarget = std::make_shared<Object>("bad-poly-index");
        badIndexTarget->setShape(Object::ShapeKind::Polyhedron);
        zone->addObject(badIndexTarget);

        auto badFieldTarget = std::make_shared<Object>("bad-field-enum");
        badFieldTarget->setShape(Object::ShapeKind::Field);
        zone->addObject(badFieldTarget);

        auto badSmoothTarget = std::make_shared<Object>("bad-smooth-enum");
        badSmoothTarget->setShape(Object::ShapeKind::Sphere);
        badSmoothTarget->clearTopologyModel();
        zone->addObject(badSmoothTarget);

        ZoneManager reader;
        reader.addZone(zone);
        reader.applyMatterFlatBuffer(badMatter);

        check(badOffsetsTarget->getPolyhedronData().vertices.empty() &&
                  badOffsetsTarget->getPolyhedronData().faces.empty(),
              "negative/non-monotone matter face offsets are rejected before allocation");
        check(badIndexTarget->getPolyhedronData().vertices.empty() &&
                  badIndexTarget->getPolyhedronData().faces.empty(),
              "out-of-range matter face vertex indices are rejected");
        check(badFieldTarget->getShapeKind() == Object::ShapeKind::Field &&
                  !badFieldTarget->hasField(),
              "invalid matter SdfPrim/SdfOp ordinals never enter runtime geometry");
        check(badSmoothTarget->getShapeKind() == Object::ShapeKind::Sphere &&
                  !badSmoothTarget->hasSmoothSurface(),
              "invalid matter SmoothSurface enum ordinals never enter runtime geometry");
    }

    std::cout << checks - failures << "/" << checks << " shape hydration integrity checks passed\n";
    return failures == 0 ? 0 : 1;
}
