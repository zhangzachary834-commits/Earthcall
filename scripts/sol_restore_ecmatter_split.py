#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


# -----------------------------------------------------------------------------
# 1. Restore the original split-substrate contract in Object semantic JSON.
#    Dense Patch/Polyhedron topology belongs to .ecmatter; semantic JSON keeps
#    identity/shape intent and remains backward-compatible with older embedded
#    topology records on READ.
# -----------------------------------------------------------------------------
obj_path = Path("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp")
obj = obj_path.read_text()

obj = replace_once(
    obj,
'''    // Sculpted / authored topology is SEMANTIC truth, not a disposable matter
    // cache.  The old writer only persisted Field payloads here; Patch and
    // custom Polyhedron payloads survived solely because .ecmatter happened to
    // be present.  A Zone identity must be independently complete.
    if (obj.hasPatch()) {
        const auto& patch = obj.getPatchData();
        nlohmann::json pj{
            {"du", patch.du},
            {"dv", patch.dv},
            {"ctrl", nlohmann::json::array()}
        };
        for (const auto& c : patch.ctrl) pj["ctrl"].push_back({c.x, c.y, c.z});
        j["patch"] = std::move(pj);
    }

''',
'''    // Split-substrate contract: semantic text says WHAT representation this
    // being has; dense Patch control nets and custom Polyhedron vertex/face
    // arrays are physical geometry and remain in .ecmatter.  The reader below
    // still accepts historical/transitional semantic payloads for backward
    // compatibility, but new writes intentionally do not duplicate them here.

''',
    "remove semantic Patch write")

obj = replace_once(
    obj,
'''    if (obj.getShapeKind() == Object::ShapeKind::Polyhedron) {
        const auto& poly = obj.getPolyhedronData();
        if (!poly.vertices.empty() && !poly.faces.empty()) {
            nlohmann::json pj{
                {"vertices", nlohmann::json::array()},
                {"faces", nlohmann::json::array()}
            };
            for (const auto& v : poly.vertices) pj["vertices"].push_back({v.x, v.y, v.z});
            for (const auto& face : poly.faces) pj["faces"].push_back(face);
            j["polyhedron"] = std::move(pj);
        }
    }

''',
'''    // Patch and Polyhedron density is deliberately omitted from the semantic
    // record.  Their ShapeKind is semantic intent; their control-net / vertex
    // buffers are hydrated from the matching .ecmatter entity after the
    // semantic skeleton exists.

''',
    "remove semantic Polyhedron write")

obj = replace_once(
    obj,
'''    // Custom polyhedron geometry belongs to the semantic identity too.  Only
    // apply it when the discriminant says this Object is still a Polyhedron;
    // an obsolete payload may coexist in a merged legacy record and must not
    // overturn the newer declared shape.
''',
'''    // Backward compatibility only: older/transitional semantic records may
    // embed custom Polyhedron geometry. New writers keep this dense topology
    // in .ecmatter, but an embedded legacy payload is still accepted when the
    // discriminant says this Object is a Polyhedron. An obsolete payload may
    // never overturn a newer declared shape.
''',
    "polyhedron reader comment")

obj = replace_once(
    obj,
'''    if (obj.hasField()) {
        // A Field's ShapeKind is only its low-level representation label; the
        // SDF/OntoMath tree is the authored form. Persist the complete tree and
        // its evaluation extent so a reload cannot produce a bare Field shell.
''',
'''    if (obj.hasField()) {
        // A Field's compact SDF/OntoMath recipe is semantic authoring intent,
        // not a dense sampled/compiled buffer. Keep that recipe and its domain
        // here until .ecmatter can encode the full recursive form losslessly;
        // dense compiled/sampled field data belongs in Matter, not in text.
''',
    "field semantic recipe comment")

obj_path.write_text(obj)


# -----------------------------------------------------------------------------
# 2. Make the regression witness enforce the split instead of semantic
#    duplication for Patch/Polyhedron.
# -----------------------------------------------------------------------------
test_path = Path("tests/constructed-being/shape_hydration_integrity_test.cpp")
test = test_path.read_text()

old_section6 = '''    // ------------------------------------------------------------------
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
'''
new_section6 = '''    // ------------------------------------------------------------------
    // 6. ORIGINAL SPLIT-SUBSTRATE CONTRACT. Semantic text carries the
    //    representation/intent; dense Bezier control points and Polyhedron
    //    vertex/face arrays live in .ecmatter. Hydration is therefore:
    //       semantic shell first -> matching physical Matter second.
    //    New semantic writes must not duplicate those dense arrays.
    // ------------------------------------------------------------------
    {
        const std::string zoneId = "split-substrate-shape-test";
        auto sourceZone = std::make_shared<Zone>(zoneId, "strict");

        auto patchSource = std::make_shared<Object>("split-patch-test");
        geom::BezierPatch patch = geom::makeBezierGrid(3, 3, 0.5f);
        patch.ctrl[0] = glm::vec3(-2.0f, 0.25f, 1.0f);
        patchSource->setBezierPatch(patch);
        sourceZone->addObject(patchSource);

        auto polySource = std::make_shared<Object>("split-polyhedron-test");
        polySource->createIcosahedron();
        sourceZone->addObject(polySource);

        ZoneManager writer;
        writer.addZone(sourceZone);
        const std::vector<uint8_t> matter = writer.buildMatterFlatBuffer();
        check(!matter.empty(), "split-substrate matter carries dense shape topology");

        nlohmann::json patchJson;
        nlohmann::json polyJson;
        to_json(patchJson, *patchSource);
        to_json(polyJson, *polySource);
        check(patchJson["shapeKind"].get<int>() == static_cast<int>(Object::ShapeKind::Patch) &&
                  !patchJson.contains("patch"),
              "semantic Patch record keeps identity but does not duplicate its control net");
        check(polyJson["shapeKind"].get<int>() == static_cast<int>(Object::ShapeKind::Polyhedron) &&
                  !polyJson.contains("polyhedron"),
              "semantic Polyhedron record keeps identity but does not duplicate vertices/faces");

        auto patchBack = std::make_shared<Object>("split-patch-test");
        auto polyBack = std::make_shared<Object>("split-polyhedron-test");
        from_json(patchJson, *patchBack);
        from_json(polyJson, *polyBack);
        check(patchBack->getShapeKind() == Object::ShapeKind::Patch && !patchBack->hasPatch(),
              "semantic Patch hydration creates the shell before Matter supplies density");
        check(polyBack->getShapeKind() == Object::ShapeKind::Polyhedron &&
                  polyBack->getPolyhedronData().vertices.empty(),
              "semantic Polyhedron hydration creates the shell before Matter supplies density");

        auto destinationZone = std::make_shared<Zone>(zoneId, "strict");
        destinationZone->addObject(patchBack);
        destinationZone->addObject(polyBack);
        ZoneManager reader;
        reader.addZone(destinationZone);
        reader.applyMatterFlatBuffer(matter);

        check(patchBack->hasPatch() &&
                  patchBack->getPatchData().ctrl.size() == patch.ctrl.size() &&
                  near(patchBack->getPatchData().ctrl[0].x, -2.0f),
              "matching .ecmatter fleshes out the semantic Patch shell");
        check(polyBack->getShapeKind() == Object::ShapeKind::Polyhedron &&
                  polyBack->getPolyhedronData().vertices.size() ==
                      polySource->getPolyhedronData().vertices.size() &&
                  !polyBack->getPolyhedronData().faces.empty(),
              "matching .ecmatter fleshes out the semantic Polyhedron shell");
    }
'''
test = replace_once(test, old_section6, new_section6, "section 6 split-substrate test")

test = replace_once(
    test,
'''    // 9. Matter is a legacy recovery/cache substrate, never a competing
    //    semantic topology authority. Exercise all four topology families
    //    through the REAL writer/reader boundary, including positive legacy
    //    recovery where semantic JSON genuinely carries only a shell.
''',
'''    // 9. Matter is the physical geometry substrate, but semantic identity
    //    constrains what Matter is allowed to flesh out. Exercise all four
    //    topology families through the REAL writer/reader boundary, including
    //    positive hydration of matching semantic shells and stale refusal.
''',
    "section 9 matter role comment")

for old, new in [
    ("valid legacy Patch matter can recover a semantic Patch shell",
     "valid Patch matter hydrates a matching semantic Patch shell"),
    ("valid legacy Polyhedron matter can recover missing topology",
     "valid Polyhedron matter hydrates a matching semantic Polyhedron shell"),
    ("valid leaf Field matter can recover a legacy semantic Field shell",
     "valid leaf Field matter can hydrate a matching semantic Field shell"),
    ("valid SmoothSurface matter can recover missing topology when semantic kind agrees",
     "valid SmoothSurface matter can hydrate missing topology when semantic kind agrees"),
]:
    if old not in test:
        raise SystemExit(f"test message missing: {old}")
    test = test.replace(old, new, 1)

test_path.write_text(test)


# -----------------------------------------------------------------------------
# 3. Correct ZoneManager's terminology. Matter is a real physical substrate,
#    not merely a cache; semantic kind/recipe controls admission.
# -----------------------------------------------------------------------------
zone_path = Path("src/ZonesOfEarth/ZoneManager.cpp")
zone = zone_path.read_text()
zone = replace_once(
    zone,
'''    // IMPORTANT: authored semantic topology is already hydrated before this
    // sidecar is applied. Matter may recover topology that an old semantic
    // record genuinely lacks, but it may never redefine an already-authored
    // representation. The checks below therefore ask both WHAT the semantic
    // ShapeKind says the being is and WHETHER that representation is already
    // complete before admitting legacy matter as a compatibility fill.
''',
'''    // IMPORTANT: semantic identity is hydrated before physical Matter.
    // .ecmatter is the binary substrate for dense geometry, so a semantic
    // Patch/Polyhedron shell is EXPECTED to receive its control-net/vertices
    // here. What Matter may never do is redefine the semantic representation.
    // The checks below therefore ask WHAT the semantic ShapeKind says the
    // being is and whether topology is already present before injecting the
    // matching physical payload. Older embedded-topology saves remain valid.
''',
    "ZoneManager matter role comment")

zone = zone.replace("legacy Polyhedron matter", "Polyhedron matter")
zone = zone.replace("legacy Patch matter", "Patch matter")
zone = zone.replace("legacy SmoothSurface matter", "SmoothSurface matter")
zone = zone.replace("legacy Field matter", "Field matter")

zone = replace_once(
    zone,
'''        // 2. Polyhedron. The sidecar may fill a legacy Polyhedron whose
        // semantic identity names Polyhedron but has no topology. It may not
        // turn some other authored shape INTO a Polyhedron, nor replace a
        // semantic Polyhedron that already carries vertices/faces.
''',
'''        // 2. Polyhedron. Dense vertices/faces live in Matter. Inject them
        // only into a semantic Polyhedron shell that does not already carry
        // topology (for example an older embedded-topology record). Matter may
        // not turn some other authored shape INTO a Polyhedron.
''',
    "Polyhedron matter comment")

zone = replace_once(
    zone,
'''        // 3. Bezier Patch. Same compatibility rule: only fill a semantic
        // Patch shell. A stale patch payload is never permission to reclassify
        // the current authored being.
''',
'''        // 3. Bezier Patch. Dense control points live in Matter. Fill only a
        // semantic Patch shell; a stale patch payload is never permission to
        // reclassify the current authored being.
''',
    "Patch matter comment")

zone = replace_once(
    zone,
'''        // 4. Smooth Surface. Named analytic kinds rebuild their semantic
        // surface from ShapeParams during JSON hydration. Matter is therefore
        // only a legacy recovery source when the ShapeKind itself agrees AND
        // the semantic topology is actually missing.
''',
'''        // 4. Smooth Surface. Named analytic kinds currently rebuild their
        // surface deterministically from semantic ShapeParams, so Matter is
        // normally redundant for them today. It may hydrate missing physical
        // topology only when the exact semantic analytic kind agrees.
''',
    "Smooth matter comment")

zone_path.write_text(zone)

print("Restored .ecmatter split-substrate intent in serialization code, tests, and matter-boundary comments.")
