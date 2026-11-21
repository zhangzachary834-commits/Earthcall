// End-to-end verification test for the "Tetra-Craft" Zone (EF5 Horizon).
//
// Verifies:
// 1. Invariant 6 (directoryKey == documentIdentity == "Tetra-Craft") and
//    companion world session ("tetra_craft").
// 2. Native Zone hydration via BootedEngineHarness("Zach") + ZoneManager::switchTo
//    with full Material closure (10 shared Material roots) and Law closure
//    (10 authored Laws whose author resolves to Person "Zach").
// 3. ZERO ShapeKind::Cube fallbacks: every single Object hydrates as
//    ShapeKind::Field with SdfPrim::Convex (4 exact unit half-space planes).
// 4. ZERO (0, 0, 0) origin pileups and ZERO subterranean real estate (every
//    object has y > 0 and distinct world coordinates).
// 5. Complete WGSL compilation across all Objects, Material colorExprs,
//    spatialRoot Sun, and Beacon radiance.
// 6. Full playable Minecraft loop via Earthcall Laws:
//    - Bedrock protection (clicking non-mineable bedrock in Mine Mode keeps it).
//    - Mining (clicking a mineable voxel in Mine Mode increments blocksMined and
//      cleanly unmakes the block via ActionNode::Kind::Destroy).
//    - Mode switching (Build Totem -> mode = 1, Mine Totem -> mode = 0).
//    - Material swatch selection (Crystal Swatch -> selectedMaterial = 3).
//    - Building (clicking a voxel in Build Mode clones @tetra.proto.crystal via
//      ActionNode::Kind::Create, placing a real SdfPrim::Convex crystal
//      tetrahedron directly on top at y + H, incrementing blocksPlaced).
//    - Mining a newly placed block (proving newborns are full first-class voxels).
//    - Harmonic beacon animation (WhileTrue Law moves beacon vertically).
// 7. Round-trip persistence inside an isolated temporary SaveRoot.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

extern MaterialManager materials;

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

const std::string kZoneId = "Tetra-Craft";
const std::string kWorldId = "tetra_craft";
constexpr float kTetraH = 0.9797959f;

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const std::string& label) {
    ++g_checks;
    std::cout << (ok ? "PASS " : "FAIL ") << label << std::endl;
    if (!ok) ++g_failures;
}

double number(Singular& s, const std::string& path) {
    PropertyValue v;
    if (!lawGetValue(s, PropertyPath::parse(path), v)) return -999.0;
    if (auto d = std::get_if<double>(&v)) return *d;
    if (auto f = std::get_if<float>(&v)) return *f;
    if (auto i = std::get_if<int>(&v)) return static_cast<double>(*i);
    return -998.0;
}

bool flag(Singular& s, const std::string& path) {
    PropertyValue v;
    if (!lawGetValue(s, PropertyPath::parse(path), v)) return false;
    if (auto b = std::get_if<bool>(&v)) return *b;
    return false;
}

struct ScratchSaveRoot {
    fs::path path;
    ~ScratchSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

} // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    fs::path source = argc > 1 ? argv[1] : "saves";
    if (!fs::exists(source / "zones" / kZoneId / "zone.json")) {
        source = "../saves";
    }
    source = fs::absolute(source);

    // 1. Raw JSON files & Invariant 6
    const fs::path zonePath = source / "zones" / kZoneId / "zone.json";
    const fs::path worldPath = source / "worlds" / (kWorldId + ".json");
    check(fs::exists(zonePath), "Zone file exists at saves/zones/Tetra-Craft/zone.json");
    check(fs::exists(worldPath), "Companion World save exists at saves/worlds/tetra_craft.json");
    if (g_failures) return 1;

    const json originalZone = SaveSystem::readSaveData(zonePath.string());
    const json originalWorld = SaveSystem::readSaveData(worldPath.string());
    const size_t initialObjectCount = originalZone["world"]["objects"].size();

    check(originalZone.value("identifier", "") == kZoneId, "Invariant 6: Zone identifier matches directory key");
    check(originalZone.value("owner", "") == "Zach", "Zone owner is 'Zach'");
    check(originalZone.contains("materialRefs") && originalZone["materialRefs"].size() == 10,
          "Zone declares 10 shared materialRefs");
    check(originalZone.contains("lawRefs") && originalZone["lawRefs"].size() == 10,
          "Zone declares 10 authored lawRefs");
    check(originalZone.contains("lexemes") && originalZone["lexemes"].size() == 5,
          "Zone declares 5 extra-spatial Lexemes (4 categories + 1 commission)");
    check(originalZone.contains("formationRelations") && originalZone["formationRelations"].size() >= 200,
          "Zone declares complete formationRelations (" + std::to_string(originalZone["formationRelations"].size()) + ")");
    check(initialObjectCount >= 100, "Zone contains rich tetrahedral world (" + std::to_string(initialObjectCount) + " Objects)");
    check(originalWorld.value("identifier", "") == kWorldId, "World identifier is 'tetra_craft'");

    // Copy Zone, Materials, and Laws into an isolated temporary SaveRoot
    ScratchSaveRoot scratch{
        fs::temp_directory_path() /
        ("earthcall-tetra-craft-test-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    fs::create_directories(scratch.path / "zones" / kZoneId);
    fs::copy(source / "zones" / kZoneId, scratch.path / "zones" / kZoneId,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    for (const auto& mref : originalZone["materialRefs"]) {
        const std::string mid = mref.get<std::string>();
        const std::string stem = mid.rfind("material.", 0) == 0 ? mid.substr(9) : mid;
        fs::create_directories(scratch.path / "materials" / stem);
        fs::copy_file(source / "materials" / stem / "material.json",
                      scratch.path / "materials" / stem / "material.json",
                      fs::copy_options::overwrite_existing);
    }
    for (const auto& lref : originalZone["lawRefs"]) {
        const std::string lid = lref.get<std::string>();
        fs::create_directories(scratch.path / "laws" / lid);
        fs::copy_file(source / "laws" / lid / "law.json",
                      scratch.path / "laws" / lid / "law.json",
                      fs::copy_options::overwrite_existing);
    }

    SaveSystem::setSaveRoot(scratch.path.string());

    // 2. Native BootedEngineHarness hydration & ZoneManager::switchTo
    TestSupport::BootedEngineHarness h("Zach");
    h.zones.bindLive();
    size_t zoneIdx = 0;
    while (zoneIdx < h.zones.zones().size() &&
           h.zones.zones()[zoneIdx]->getIdentifier() != kZoneId) {
        ++zoneIdx;
    }
    check(zoneIdx < h.zones.zones().size() && h.zones.switchTo(zoneIdx),
          "Native Tetra-Craft boot and switchTo succeeds without session file");
    if (g_failures) return 1;

    auto zone = h.zones.zones()[zoneIdx];
    std::vector<Object*> allObjects;
    std::map<std::string, Object*> byId;
    int cubeCount = 0;
    int validTetraCount = 0;
    int originPileupCount = 0;
    int subterraneanCount = 0;
    bool allMaterialsResolve = true;
    std::vector<glm::vec3> positions;

    for (auto& o : zone->getOwnedObjects()) {
        if (!o) continue;
        allObjects.push_back(o.get());
        byId[o->getIdentifier()] = o.get();

        if (o->getShapeKind() == Object::ShapeKind::Cube) ++cubeCount;
        if (o->getShapeKind() == Object::ShapeKind::Field &&
            o->hasField() &&
            o->getFieldData().prim == geom::SdfPrim::Convex &&
            o->getFieldData().planes.size() == 4) {
            ++validTetraCount;
        }
        const glm::vec3 p = o->getPosition();
        if (glm::length(p) < 0.05f) ++originPileupCount;
        if (p.y <= 0.0f) ++subterraneanCount;
        positions.push_back(p);

        if (o->materialId() == "material.default" || !materials.get(o->materialId())) {
            allMaterialsResolve = false;
            std::cerr << "Invalid/default material on " << o->getIdentifier()
                      << ": " << o->materialId() << "\n";
        }
    }

    // Verify no two objects share the exact same world coordinates
    int duplicatePosCount = 0;
    for (size_t i = 0; i < positions.size(); ++i) {
        for (size_t j = i + 1; j < positions.size(); ++j) {
            if (glm::length(positions[i] - positions[j]) < 0.05f) {
                ++duplicatePosCount;
            }
        }
    }

    check(allObjects.size() == initialObjectCount, "All " + std::to_string(initialObjectCount) + " Objects survive native hydration");
    check(cubeCount == 0, "ZERO ShapeKind::Cube fallbacks (0 cubes found — for real this time)");
    check(validTetraCount == static_cast<int>(allObjects.size()),
          "100% of Objects hydrate as exact 4-plane SdfPrim::Convex regular tetrahedra");
    check(originPileupCount == 0, "ZERO Objects stacked at (0, 0, 0)");
    check(duplicatePosCount == 0, "ZERO overlapping/duplicate Object positions");
    check(subterraneanCount == 0, "ZERO subterranean Objects (every Object has y > 0)");
    check(allMaterialsResolve, "Every Object resolves to a custom authored tetra.* Material (zero material.default)");

    // 3. Verify all 10 Laws are live, enabled, and authored by Zach
    for (const auto* suffix : {
             "select-mine", "select-build", "select-material", "mine",
             "place-grass", "place-stone", "place-timber", "place-crystal",
             "place-gold", "beacon-pulse"}) {
        const std::string lawId = std::string("law-tetracraft-") + suffix;
        auto* law = h.lawManager.find(lawId);
        check(law && law->isAuthored() && law->isEnabled(), "Authored Law is live and enabled: " + lawId);
        check(law && law->authors().findMemberByIdentifier("Zach") == &h.player,
              "Law author resolves to Person 'Zach': " + lawId);
    }

    // 4. Verify WGSL compilation across all spatial fields, Material colorExprs, and Objects
    auto* root = zone->spatialRoot();
    Rendering::AuthorableLightState light;
    check(root && Rendering::readAuthorableLight(*root, light) && light.enabled,
          "Golden Sun spatialRoot resolves as an enabled authorable light source");

    {
        std::vector<const geom::FieldNode*> fieldNodes;
        if (root) fieldNodes.push_back(root);
        for (const auto& f : zone->additionalSpatialFields()) {
            if (f) fieldNodes.push_back(f.get());
        }
        check(fieldNodes.size() == 2, "One spatialRoot Sun + one Aether Beacon radiance field");
        bool allFieldsCompile = true;
        for (const auto* n : fieldNodes) {
            if (n->field && !n->field->astDefinition.pieces.empty()) {
                const auto l = sdfwgsl::inspectScalarExpression(&n->field->astDefinition, true);
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << n->getIdentifier() << " radiance WGSL error: " << l.error << "\n";
                }
            }
            if (n->lightChroma && !n->lightChroma->pieces.empty()) {
                const auto l = sdfwgsl::inspectVectorExpression(n->lightChroma.get(), true);
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << n->getIdentifier() << " chroma WGSL error: " << l.error << "\n";
                }
            }
        }
        check(allFieldsCompile, "All lighting expressions compile cleanly to WGSL");

        bool allObjectsCompile = true;
        for (Object* o : allObjects) {
            auto mat = resolveRenderMaterial(o->materialId(), {});
            auto prog = sdfwgsl::compile(
                o->getFieldData(), nullptr, mat.colorExpr.get(),
                root && root->field ? &root->field->astDefinition : nullptr);
            if (!prog.ok) {
                allObjectsCompile = false;
                std::cerr << "Object WGSL compile failure (" << o->getIdentifier() << "): " << prog.error << "\n";
            }
        }
        check(allObjectsCompile, "All " + std::to_string(allObjects.size()) + " Tetrahedra + Material colorExprs compile to WGSL");
    }

    // 5. Full Interactive Minecraft Loop Verification
    auto* stateObj = byId["tetra.state"];
    auto* mineTool = byId["tetra.tool.mine"];
    auto* buildTool = byId["tetra.tool.build"];
    auto* crystalSwatch = byId["tetra.swatch.crystal"];
    auto* bedrock = byId["tetra.voxel.bedrock.3.3"];
    auto* grassTarget = byId["tetra.voxel.surface.3.3"];
    auto* buildBase = byId["tetra.voxel.surface.4.3"];
    auto* beacon = byId["tetra.beacon.crystal"];

    check(stateObj && mineTool && buildTool && crystalSwatch && bedrock && grassTarget && buildBase && beacon,
          "Key state, tool, swatch, terrain, and beacon Objects are present");
    if (!stateObj || !mineTool || !buildTool || !crystalSwatch || !bedrock || !grassTarget || !buildBase || !beacon) {
        return 1;
    }

    auto tick = [&](double dt) {
        h.worldTime += dt;
        Universe::instance().setClock(h.worldTime, dt);
        h.lawManager.tick();
    };
    auto click = [&](Object* o) {
        Core::EventBus::instance().publish(ECA::Event{"object-clicked", o, nullptr, std::time(nullptr)});
        tick(1.0 / 60.0);
    };

    // Baseline state
    tick(1.0 / 60.0);
    check(std::abs(number(*stateObj, "mode") - 0.0) < 1e-6, "Baseline mode is 0 (MINE)");
    check(std::abs(number(*stateObj, "blocksMined") - 0.0) < 1e-6, "Baseline blocksMined is 0");
    check(std::abs(number(*stateObj, "blocksPlaced") - 0.0) < 1e-6, "Baseline blocksPlaced is 0");

    // 5A. Bedrock protection: clicking bedrock in MINE mode must NOT destroy it
    click(bedrock);
    check(zone->getOwnedObjects().size() == initialObjectCount,
          "Bedrock protection: clicking non-mineable bedrock does not destroy it");
    check(std::abs(number(*stateObj, "blocksMined") - 0.0) < 1e-6,
          "Bedrock protection: blocksMined remains 0");

    // 5B. Mining: clicking a mineable grass tetrahedron destroys it cleanly
    const std::string minedId = grassTarget->getIdentifier();
    click(grassTarget);
    check(zone->getOwnedObjects().size() == initialObjectCount - 1,
          "Mine Mode: clicking mineable grass tetrahedron cleanly unmakes it (count - 1)");
    check(std::abs(number(*stateObj, "blocksMined") - 1.0) < 1e-6,
          "Mine Mode: blocksMined increments to 1");
    bool minedStillPresent = false;
    for (const auto& o : zone->getOwnedObjects()) {
        if (o && o->getIdentifier() == minedId) minedStillPresent = true;
    }
    check(!minedStillPresent, "Mine Mode: destroyed tetrahedron is absent from Zone ownedObjects");

    // 5C. Mode switch -> BUILD + Swatch switch -> CRYSTAL
    click(buildTool);
    check(std::abs(number(*stateObj, "mode") - 1.0) < 1e-6, "Clicking Build Totem switches mode to 1 (BUILD)");
    click(crystalSwatch);
    check(std::abs(number(*stateObj, "selectedMaterial") - 3.0) < 1e-6,
          "Clicking Crystal Swatch selects material 3 (Aether Crystal)");

    // 5D. Build: clicking buildBase spawns a real SdfPrim::Convex crystal tetrahedron on top
    const glm::vec3 basePos = buildBase->getPosition();
    std::set<std::string> idsBeforeBuild;
    for (const auto& o : zone->getOwnedObjects()) {
        if (o) idsBeforeBuild.insert(o->getIdentifier());
    }
    click(buildBase);
    check(zone->getOwnedObjects().size() == initialObjectCount,
          "Build Mode: clicking a tetrahedron spawns a newborn block (count restored to initial)");
    check(std::abs(number(*stateObj, "blocksPlaced") - 1.0) < 1e-6,
          "Build Mode: blocksPlaced increments to 1");

    Object* newborn = nullptr;
    for (const auto& o : zone->getOwnedObjects()) {
        if (o && idsBeforeBuild.find(o->getIdentifier()) == idsBeforeBuild.end()) {
            newborn = o.get();
            break;
        }
    }
    check(newborn != nullptr, "Build Mode: newborn Object located in active Zone");
    if (newborn) {
        check(newborn->getShapeKind() == Object::ShapeKind::Field &&
                  newborn->hasField() &&
                  newborn->getFieldData().prim == geom::SdfPrim::Convex &&
                  newborn->getFieldData().planes.size() == 4,
              "Build Mode: newborn is a real 4-plane SdfPrim::Convex tetrahedron (NOT a Cube!)");
        check(newborn->materialId() == "material.tetra.crystal",
              "Build Mode: newborn inherits material.tetra.crystal from prototype");
        const glm::vec3 np = newborn->getPosition();
        const bool posMatch = std::abs(np.x - basePos.x) < 0.02f &&
                              std::abs(np.y - (basePos.y + kTetraH)) < 0.02f &&
                              std::abs(np.z - basePos.z) < 0.02f;
        check(posMatch, "Build Mode: newborn stacks cleanly on top of clicked block at y + H");
        check(flag(*newborn, "tetraBlock") && flag(*newborn, "mineable") && !flag(*newborn, "isPrototype"),
              "Build Mode: newborn carries tetraBlock=true, mineable=true, isPrototype=false");

        // 5E. Switch back to MINE mode and mine the newly placed crystal block!
        click(mineTool);
        check(std::abs(number(*stateObj, "mode") - 0.0) < 1e-6, "Clicking Mine Totem switches mode back to 0 (MINE)");
        click(newborn);
        check(zone->getOwnedObjects().size() == initialObjectCount - 1,
              "Round-trip gameplay: newly placed crystal tetrahedron can be mined back out cleanly");
        check(std::abs(number(*stateObj, "blocksMined") - 2.0) < 1e-6,
              "Round-trip gameplay: blocksMined increments to 2");
    }

    // 5F. Beacon harmonic pulse
    const float beaconY0 = beacon->getPosition().y;
    for (int i = 0; i < 60; ++i) tick(1.0 / 60.0);
    const float beaconY1 = beacon->getPosition().y;
    check(std::abs(beaconY1 - beaconY0) > 0.05f,
          "Living World: Aether Crystal Beacon levitates harmonically over time");

    // 6. Persistence round-trip
    check(h.zones.persistActiveZone(), "ZoneManager::persistActiveZone succeeds in isolated SaveRoot");
    const json savedZone = SaveSystem::readZoneIdentity(kZoneId);
    check(savedZone["world"]["objects"].size() == initialObjectCount - 1,
          "Round-trip persistence reflects mined block count cleanly");
    check(SaveSystem::readSaveData(zonePath.string()) == originalZone,
          "Real authored Zone file in saves/ remains untouched by the test");

    std::cout << "------------------------------------------------------------\n";
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
