#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Material/PaintToolSurface.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

extern MaterialManager materials;

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
    std::cout << "============================================================" << std::endl;
    std::cout << "Running zone face texture persistence test..." << std::endl;
    std::cout << "============================================================" << std::endl;

    if (!glfwInit()) {
        std::cout << "zone_facetexture_test: glfwInit failed" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "zone_facetexture_test", nullptr, nullptr);
    if (!window) {
        std::cout << "zone_facetexture_test: no GL context" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_facetexture_test";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "zones");
    SaveSystem::setSaveRoot(sandbox.string());

    const std::string zoneId = "PainterWorkshop";

    ZoneManager mgr;
    auto zone = std::make_shared<Zone>(zoneId, "strict");
    auto cube = std::make_shared<Object>();
    cube->setShape(Object::ShapeKind::Cube);
    cube->setObjectID("painted-cube-1");
    cube->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)));

    cube->setFaceColor(0, 1.0f, 0.0f, 0.0f);
    cube->setFaceColor(1, 0.0f, 1.0f, 0.0f);
    if (auto mat = cube->ownMaterial()) {
        PaintToolSurface pts(*mat);
        pts.paintFace(2, glm::vec2(0.5f, 0.5f), 0.0f, 0.0f, 1.0f, 0.2f, 0.5f);
    }

    zone->addObject(cube);
    mgr.addZone(zone);

    auto mat = materials.get(cube->materialId());
    check(mat != nullptr, "cube has its own diverged material before save");
    check(mat && mat->faceTextures.size() == 6, "material has 6 face textures");
    check(mat && mat->faceTextures[0].pixels[0] == 255, "face 0 is red in memory");
    check(mat && mat->faceTextures[1].pixels[1] == 255, "face 1 is green in memory");

    mgr.persistZones();

    const auto zoneFilePath = sandbox / "zones" / zoneId / "zone.json";
    check(std::filesystem::exists(zoneFilePath), "zone.json written to disk");

    // Verify zone.json contains materials array with faceTextures
    {
        std::ifstream in(zoneFilePath);
        nlohmann::json zj;
        in >> zj;
        check(zj.contains("materials") && zj["materials"].is_array(), "zone.json contains materials array");
        check(!zj["materials"].empty(), "materials array is not empty");
        bool foundCubeMat = false;
        for (const auto& mj : zj["materials"]) {
            if (mj.value("name", "") == "painted-cube-1") {
                foundCubeMat = true;
                check(mj.contains("faceTextures") && mj["faceTextures"].size() == 6,
                      "saved material contains 6 faceTextures in zone.json");
            }
        }
        check(foundCubeMat, "found painted-cube-1 material in zone.json");
    }

    // Cold boot: empty MaterialManager, hydrate from the identity file.
    {
        materials = MaterialManager();
        ZoneManager freshMgr;
        freshMgr.hydrateFromZoneStore();

        auto loadedZone = freshMgr.zones().empty() ? nullptr : freshMgr.zones()[0];
        check(loadedZone != nullptr && loadedZone->getIdentifier() == zoneId, "hydrated zone correctly");

        if (loadedZone) {
            check(loadedZone->getOwnedObjects().size() == 1, "loaded zone has 1 object");
            auto loadedCube = loadedZone->getOwnedObjects()[0];
            check(loadedCube->getIdentifier() == "painted-cube-1", "loaded cube has correct id");

            auto loadedMat = materials.get(loadedCube->materialId());
            check(loadedMat != nullptr, "loaded cube material was restored into MaterialManager");
            if (loadedMat) {
                check(loadedMat->faceTextures.size() == 6, "restored material has 6 face textures");
                if (loadedMat->faceTextures.size() >= 2) {
                    check(loadedMat->faceTextures[0].pixels[0] == 255 &&
                          loadedMat->faceTextures[0].pixels[1] == 0,
                          "face 0 red color restored from zone save");
                    check(loadedMat->faceTextures[1].pixels[1] == 255 &&
                          loadedMat->faceTextures[1].pixels[0] == 0,
                          "face 1 green color restored from zone save");
                }
            }
        }
    }

    // Keep-live objects + session REPLACE (the in-app "load another save" wipe).
    {
        materials.loadFromJson(nlohmann::json::array());
        check(!materials.get(cube->materialId()),
              "session REPLACE drops the live zone's material");
        mgr.hydrateFromZoneStore();
        auto kept = materials.get(cube->materialId());
        check(kept && kept->faceTextures.size() == 6 &&
                  kept->faceTextures[0].pixels[0] == 255,
              "hydrate of a live zone restores FaceTextures from identity");
    }

    // Pre-embed identity: objects and faceColors, no materials array.
    {
        nlohmann::json stripped;
        {
            std::ifstream in(zoneFilePath);
            in >> stripped;
        }
        stripped.erase("materials");
        {
            std::ofstream out(zoneFilePath);
            out << stripped.dump(2);
        }
        materials.loadFromJson(nlohmann::json::array());
        mgr.hydrateFromZoneStore();
        auto fromFaces = materials.get(cube->materialId());
        check(fromFaces && !fromFaces->faceTextures.empty() &&
                  fromFaces->faceTextures[0].pixels[0] == 255 &&
                  fromFaces->faceTextures[1].pixels[1] == 255,
              "keep-live zone with no identity materials reinstates FaceTextures from faceColors");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed" << std::endl;
    if (g_failures > 0) {
        std::cout << "zone_facetexture_test: FAILED" << std::endl;
        return 1;
    }
    std::cout << "zone_facetexture_test: ALL OK" << std::endl;
    return 0;
}
