#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "support/test_harness.hpp"

#include <iostream>
#include <memory>
#include <filesystem>
#include <cassert>

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
    std::cout << "Starting sanctuary_of_sunlit_mist_test...\n";

    std::string worldSavePath = TestSupport::resolveRealWorldPath("saves/worlds/sanctuary_of_sunlit_mist.json");
    if (!std::filesystem::exists(worldSavePath)) {
        std::cout << "sanctuary_of_sunlit_mist_test: FAILED (file not found: " << worldSavePath << ")\n";
        return 1;
    }

    // Ensure SaveSystem saveRoot points to the containing saves directory
    const auto absWorld = std::filesystem::absolute(worldSavePath);
    const auto savesDir = absWorld.parent_path().parent_path();
    SaveSystem::setSaveRoot(savesDir.string());

    ZoneManager mgr;
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;

    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    mgr.loadState(worldSavePath, ctx);

    check(!mgr.zones().empty(), "zones loaded from sanctuary save");
    if (mgr.zones().empty()) {
        std::cout << "sanctuary_of_sunlit_mist_test: FAILED (no zones loaded)\n";
        return 1;
    }

    Zone& activeZone = mgr.active();
    check(activeZone.getIdentifier() == "Sanctuary of Sunlit Mist",
          "active zone identifier is 'Sanctuary of Sunlit Mist'");

    // Check camera initialization from world save
    check(std::fabs(camera.pos.x - 0.0f) < 1e-3f &&
          std::fabs(camera.pos.y - 2.2f) < 1e-3f &&
          std::fabs(camera.pos.z - 8.0f) < 1e-3f,
          "camera loaded at [0.0, 2.2, 8.0]");

    // Check spatial root (Sun light source)
    const geom::FieldNode* sunRoot = activeZone.spatialRoot();
    check(sunRoot != nullptr, "sanctuary has spatialRoot");
    if (sunRoot) {
        PropertyValue lightSource;
        check(sunRoot->getDynamicProperty("light.source", lightSource) &&
              std::get<bool>(lightSource),
              "spatialRoot is authored as radiant light source");
        PropertyValue name;
        check(sunRoot->getDynamicProperty("displayName", name) &&
              std::get<std::string>(name) == "Sanctuary Celestial Sun",
              "spatialRoot carries display name 'Sanctuary Celestial Sun'");
    }

    // Check mist spatial field
    const auto& fields = activeZone.additionalSpatialFields();
    check(!fields.empty(), "sanctuary contains additional spatial fields");
    geom::FieldNode* mistField = nullptr;
    for (const auto& f : fields) {
        if (f && f->getIdentifier() == "mist.sanctuary.sunlit-mist-volume") {
            mistField = f.get();
            break;
        }
    }
    check(mistField != nullptr, "found 'mist.sanctuary.sunlit-mist-volume' field");

    if (mistField) {
        check(mistField->volumeDensity && !mistField->volumeDensity->pieces.empty(),
              "mist field carries volume density AST");
        check(mistField->volumeExtinction && !mistField->volumeExtinction->pieces.empty(),
              "mist field carries volume extinction AST");
        check(mistField->volumeScattering && !mistField->volumeScattering->pieces.empty(),
              "mist field carries volume scattering AST");
        check(mistField->volumeChroma && !mistField->volumeChroma->pieces.empty(),
              "mist field carries volume chroma AST");

        // Verify occluder SDF
        check(geom::isSdfActive(mistField->volumeOccluder.get()),
              "mist field carries active volume occluder geometry");
        if (geom::isSdfActive(mistField->volumeOccluder.get())) {
            check(mistField->volumeOccluder->op == geom::SdfOp::Subtract,
                  "volume occluder root is CSG Subtract");
            check(mistField->volumeOccluder->children.size() == 2,
                  "volume occluder has base slab child and aperture cutouts child");
        }

        // Test projection via readVolumeDensity
        Rendering::VolumeDensityBinding binding;
        check(Rendering::readVolumeDensity(*mistField, 0.0, 0.0, binding),
              "readVolumeDensity successfully projects the mist field");
        check(binding.occluderSdf != nullptr,
              "binding carries occluderSdf pointer");
        check(binding.occluderRevision != 0,
              "binding computes non-zero occluderRevision fingerprint");
    }

    // Check objects in zone
    const auto& objects = activeZone.getOwnedObjects();
    check(!objects.empty(), "sanctuary zone contains world objects");
    std::cout << "  Sanctuary object count: " << objects.size() << std::endl;

    auto findObj = [&](const std::string& id) -> const Object* {
        for (const auto& obj : objects) {
            if (obj && obj->getObjectID() == id) return obj.get();
        }
        return nullptr;
    };

    check(findObj("mist.sanctuary.floor") != nullptr, "floor object exists");
    check(findObj("mist.sanctuary.altar_base") != nullptr, "altar base object exists");
    check(findObj("mist.sanctuary.altar_table") != nullptr, "altar table object exists");
    check(findObj("mist.sanctuary.clerestory_lintel") != nullptr, "clerestory lintel object exists");
    check(findObj("mist.sanctuary.stele") != nullptr, "physics stele object exists");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "sanctuary_of_sunlit_mist_test: FAILED\n";
        return 1;
    }
    std::cout << "sanctuary_of_sunlit_mist_test: ALL OK\n";
    return 0;
}
