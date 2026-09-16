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

    std::cout << checks - failures << "/" << checks << " shape hydration integrity checks passed\n";
    return failures == 0 ? 0 : 1;
}
