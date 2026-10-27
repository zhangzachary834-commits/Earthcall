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
#include <chrono>
#include <stdexcept>
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


struct SanctuarySaveSandbox {
    std::filesystem::path root;
    std::filesystem::path worldPath;
    std::string previousRoot;

    explicit SanctuarySaveSandbox(const std::filesystem::path& sourceWorld) {
        const auto sourceZone = sourceWorld.parent_path().parent_path() /
            "zones" / "Sanctuary of Sunlit Mist";
        if (!std::filesystem::is_regular_file(sourceWorld) ||
            !std::filesystem::is_directory(sourceZone)) {
            throw std::runtime_error("sanctuary save fixtures missing");
        }

        // Each invocation owns a fresh directory. Never remove a shared,
        // fixed-name scratch directory or write into the authored save tree.
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto candidate = std::filesystem::temp_directory_path() /
                ("earthcall-sanctuary-" + std::to_string(stamp) + "-" +
                 std::to_string(attempt));
            std::error_code ec;
            if (std::filesystem::create_directory(candidate, ec)) {
                root = candidate;
                break;
            }
            if (ec) throw std::filesystem::filesystem_error(
                "creating sanctuary scratch directory", candidate, ec);
        }
        if (root.empty()) throw std::runtime_error("no unique sanctuary scratch directory");

        try {
            std::filesystem::create_directories(root / "worlds");
            std::filesystem::create_directories(root / "zones");
            worldPath = root / "worlds" / sourceWorld.filename();
            std::filesystem::copy_file(sourceWorld, worldPath);
            std::filesystem::copy(sourceZone, root / "zones" / sourceZone.filename(),
                                  std::filesystem::copy_options::recursive);
            previousRoot = SaveSystem::saveRoot();
            SaveSystem::setSaveRoot(root.string());
        } catch (...) {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
            throw;
        }
    }

    ~SanctuarySaveSandbox() {
        SaveSystem::setSaveRoot(previousRoot);
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
        if (ec) std::terminate(); // Never silently leave a polluted scratch tree.
    }

    SanctuarySaveSandbox(const SanctuarySaveSandbox&) = delete;
    SanctuarySaveSandbox& operator=(const SanctuarySaveSandbox&) = delete;
};

} // namespace

int main() {
    std::cout << "Starting sanctuary_of_sunlit_mist_test...\n";

    std::string worldSavePath = TestSupport::resolveRealWorldPath("saves/worlds/sanctuary_of_sunlit_mist.json");
    if (!std::filesystem::exists(worldSavePath)) {
        std::cout << "sanctuary_of_sunlit_mist_test: FAILED (file not found: " << worldSavePath << ")\n";
        return 1;
    }

    // Load the real authored fixture only from a private, checked scratch tree.
    // The guard outlives ZoneManager and restores the previous global save root.
    SanctuarySaveSandbox sandbox(std::filesystem::absolute(worldSavePath));
    check(SaveSystem::saveRoot() == sandbox.root.string(),
          "SaveSystem points at the private sandbox");
    check(std::filesystem::exists(sandbox.worldPath),
          "sandbox world fixture copied successfully");

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

    mgr.loadState(sandbox.worldPath.string(), ctx);

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
