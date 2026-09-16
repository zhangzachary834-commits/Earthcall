#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    count = text.count(old)
    if count == 1:
        return text.replace(old, new, 1), True
    if new in text:
        return text, False
    raise SystemExit(f"{label}: expected one old block or an already-patched new block; old count={count}")


path = Path("tests/constructed-being/shape_hydration_integrity_test.cpp")
text = path.read_text()
changed = False

old_include = '''#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cmath>
'''
new_include = '''#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"

#include <flatbuffers/flatbuffers.h>
#include <cmath>
'''
text, did = replace_once(text, old_include, new_include, "test includes")
changed |= did

old_helper_tail = '''geom::SdfNode authoredCompositeField() {
    auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.8f));
    sphere.offset = glm::vec3(-0.35f, 0.0f, 0.0f);
    auto torus = geom::SdfNode::leaf(geom::SdfPrim::Torus,
                                     glm::vec3(1.1f, 0.22f, 0.0f));
    torus.offset = glm::vec3(0.4f, 0.0f, 0.0f);
    return geom::SdfNode::binary(geom::SdfOp::SmoothUnion, sphere, torus, 0.17f);
}
} // namespace
'''
new_helper_tail = '''geom::SdfNode authoredCompositeField() {
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
'''
text, did = replace_once(text, old_helper_tail, new_helper_tail, "matter test helpers")
changed |= did

old_end = '''    std::cout << checks - failures << "/" << checks << " shape hydration integrity checks passed\\n";
    return failures == 0 ? 0 : 1;
}
'''
new_end = '''    // ------------------------------------------------------------------
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

    std::cout << checks - failures << "/" << checks << " shape hydration integrity checks passed\\n";
    return failures == 0 ? 0 : 1;
}
'''
text, did = replace_once(text, old_end, new_end, "adversarial matter tests")
changed |= did

if changed:
    path.write_text(text)
print("patched" if changed else "already patched")
