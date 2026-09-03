#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Substrate Split (.ecform + .ecmatter) Test...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_substrate_split_test";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    camera.pos = glm::vec3(5.0f, 5.0f, 5.0f);
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {0.8f, 0.2f, 0.1f};
    double worldTime = 12.34;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    ZoneManager mgr;
    auto testZone = std::make_shared<Zone>("SubstrateSanctum", "default");

    // 1. Create a polyhedron object with custom vertices, attributes, and dynamic properties
    auto polyObj = std::make_shared<Object>();
    polyObj->setObjectID("poly-test-1");
    std::vector<glm::vec3> polyVerts = {
        {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 2.0f}
    };
    std::vector<std::vector<int>> polyFaces = {
        {0, 1, 2, 3}, {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4}
    };
    polyObj->setPolyhedronData(PolyhedronData::createCustomPolyhedron(polyVerts, polyFaces));
    polyObj->setAttribute("lore", "ancient_pyramid");
    polyObj->addTag("sacred");
    polyObj->setDynamicProperty("resonance", PropertyValue(432.0));
    polyObj->setFaceColor(0, 0.9f, 0.8f, 0.1f);
    testZone->addObject(polyObj);

    // 2. Create a Bezier patch object
    auto patchObj = std::make_shared<Object>();
    patchObj->setObjectID("patch-test-1");
    geom::BezierPatch bp;
    bp.du = 1; bp.dv = 1;
    bp.ctrl = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.5f},
        {0.0f, 1.0f, 0.5f}, {1.0f, 1.0f, 1.0f}
    };
    patchObj->setBezierPatch(bp);
    patchObj->setAttribute("material_class", "cloth");
    testZone->addObject(patchObj);

    // 3. Create a smooth quadric object
    auto sphereObj = std::make_shared<Object>();
    sphereObj->setObjectID("sphere-test-1");
    sphereObj->setShape(Object::ShapeKind::Sphere);
    sphereObj->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)));
    testZone->addObject(sphereObj);

    mgr.addZone(testZone);

    // Test 1: Save state with split substrate
    mgr.saveStateWithLog("split_test_world", ctx);
    check(mgr.getSaveLoadState().lastSaveReport.find("SAVE FAILED") == std::string::npos,
          "saveStateWithLog completed successfully");

    const auto formPath = sandbox / "worlds" / "split_test_world.ecform";
    const auto matterPath = sandbox / "worlds" / "split_test_world.ecmatter";
    const auto jsonPath = sandbox / "worlds" / "split_test_world.json";

    check(std::filesystem::exists(formPath), ".ecform semantic text file exists");
    check(std::filesystem::exists(matterPath), ".ecmatter physical binary file exists");

    // Test 2: Inspect .ecform JSON structure (ensure it's lean, readable, no binary base64)
    {
        std::ifstream formIn(formPath);
        nlohmann::json fj;
        formIn >> fj;

        check(fj.contains("objects") && fj["objects"].is_array() && fj["objects"].size() == 3,
              ".ecform has 3 objects");

        // Verify poly-test-1 in .ecform has attributes and dynamic properties, but NO binary dumps
        bool foundPoly = false;
        for (const auto& o : fj["objects"]) {
            if (o.value("objectID", "") == "poly-test-1") {
                foundPoly = true;
                check(o.contains("attributes") && o["attributes"].value("lore", "") == "ancient_pyramid",
                      ".ecform preserves custom attribute 'lore'");
                check(o.contains("tags") && o["tags"].is_array() && o["tags"][0] == "sacred",
                      ".ecform preserves tag 'sacred'");
                check(o.contains("authoredProperties") && o["authoredProperties"].contains("resonance"),
                      ".ecform preserves authored dynamicProperty 'resonance'");
                check(!o.contains("verticesBinary") && !o.contains("facesDataBinary"),
                      ".ecform is stripped of BinaryPack vertices");
            }
        }
        check(foundPoly, "Found poly-test-1 in .ecform");
    }

    // Test 3: Hydrate into a clean ZoneManager from split files
    {
        ZoneManager loadMgr;
        loadMgr.loadState(formPath.string(), ctx);

        check(loadMgr.zones().size() >= 1, "loadState restored zones");
        const auto& z = loadMgr.active();
        check(z.getOwnedObjects().size() == 3, "loadState restored 3 objects");

        std::shared_ptr<Object> restoredPoly = nullptr;
        std::shared_ptr<Object> restoredPatch = nullptr;
        std::shared_ptr<Object> restoredSphere = nullptr;

        for (const auto& o : z.getOwnedObjects()) {
            if (o->getIdentifier() == "poly-test-1") restoredPoly = o;
            else if (o->getIdentifier() == "patch-test-1") restoredPatch = o;
            else if (o->getIdentifier() == "sphere-test-1") restoredSphere = o;
        }

        check(restoredPoly != nullptr, "poly-test-1 restored");
        if (restoredPoly) {
            check(restoredPoly->getAttribute("lore") == "ancient_pyramid",
                  "poly-test-1 restored attribute 'lore'");
            check(restoredPoly->hasTag("sacred"),
                  "poly-test-1 restored tag 'sacred'");
            check(restoredPoly->hasDynamicProperty("resonance"),
                  "poly-test-1 restored dynamic property 'resonance'");
            
            // Physical reinstatement from .ecmatter
            const auto& pd = restoredPoly->getPolyhedronData();
            check(pd.vertices.size() == 5, "poly-test-1 restored 5 vertices from .ecmatter");
            check(pd.faces.size() == 5, "poly-test-1 restored 5 faces from .ecmatter");
            if (!pd.vertices.empty()) {
                check(std::abs(pd.vertices[4].z - 2.0f) < 1e-4f, "poly-test-1 apex Z vertex matches");
            }
        }

        check(restoredPatch != nullptr, "patch-test-1 restored");
        if (restoredPatch) {
            check(restoredPatch->hasPatch(), "patch-test-1 hasPatch is true");
            const auto& pData = restoredPatch->getPatchData();
            check(pData.ctrl.size() == 4, "patch-test-1 restored 4 control points from .ecmatter");
        }

        check(restoredSphere != nullptr, "sphere-test-1 restored");
        if (restoredSphere) {
            check(std::abs(restoredSphere->getPosition().y - 10.0f) < 1e-4f,
                  "sphere-test-1 restored position from .ecmatter transform");
        }
    }

    // Test 4: Legacy JSON Splitter / Migration on Load
    {
        // Construct a legacy monolithic JSON file without .ecmatter
        nlohmann::json legacySave = nlohmann::json::object();
        legacySave["worldTime"] = 99.0;
        legacySave["currentZone"] = 0;
        nlohmann::json legObj = nlohmann::json::object();
        legObj["objectID"] = "legacy-cube";
        legObj["shapeKind"] = static_cast<int>(Object::ShapeKind::Cube);
        legObj["attributes"] = {{"origin", "pre-split-era"}};
        legObj["transform"] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 10,20,30,1};
        legacySave["objects"] = nlohmann::json::array({legObj});
        legacySave["zones"] = nlohmann::json::array({
            {{"name", "LegacyZone"}, {"ownerId", "default"}, {"world", {{"objects", nlohmann::json::array({legObj})}}}}
        });

        std::filesystem::path legPath = sandbox / "worlds" / "legacy_world.json";
        {
            std::ofstream legOut(legPath);
            legOut << legacySave.dump(2);
        }

        const auto migratedForm = sandbox / "worlds" / "legacy_world.ecform";
        const auto migratedMatter = sandbox / "worlds" / "legacy_world.ecmatter";
        check(!std::filesystem::exists(migratedMatter), "No .ecmatter prior to loading legacy save");

        ZoneManager legacyMgr;
        legacyMgr.loadState(legPath.string(), ctx);

        check(legacyMgr.active().getOwnedObjects().size() == 1, "Legacy save objects hydrated");
        check(std::filesystem::exists(migratedForm), "Legacy loader transparently wrote .ecform");
        check(std::filesystem::exists(migratedMatter), "Legacy loader transparently wrote .ecmatter");
        check(SaveSystem::readMatterData(migratedMatter.string()).size() > 0, ".ecmatter contains non-empty binary payload");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "substrate_split_test: FAILED\n";
        return 1;
    }
    std::cout << "substrate_split_test: ALL OK\n";
    return 0;
}
