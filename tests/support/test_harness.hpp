#pragma once

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <glm/glm.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

extern MaterialManager materials;
extern CategoryManager categories;

namespace TestSupport {

struct BootedEngineHarness {
    Core::Camera camera;
    MouseHandler mouseHandler;
    Soul soul;
    Body body;
    Person player;
    LawManager lawManager;
    ZoneManager zones;
    float currentColor[3]{1.0f, 1.0f, 1.0f};
    double worldTime{0.0};
    SaveContext ctx;
    Singularity::Input::InteractionChannel* interaction{nullptr};

    BootedEngineHarness(const std::string& playerName = "Player",
                        const std::string& bodyType = "humanoid")
        : soul(playerName),
          body(bodyType, "default"),
          player(std::move(soul), std::move(body), "default") {

        lawManager.connectToEventBus();

        // 1. Sync register standard channels like InteractionChannel
        Singularity::Input::InteractionChannel::syncRegister(lawManager);
        interaction = Singularity::Input::InteractionChannel::find(lawManager);
        if (interaction) {
            interaction->setEnabled(true);
        }

        // 2. Wire Universe providers matching real app boot (EngineInit.cpp)
        Universe::instance().setProvider([this](std::vector<Singular*>& beings) {
            if (zones.zones().empty()) return;
            auto active = zones.zones()[zones.currentIndex()];
            if (!active) return;
            beings.push_back(active.get());
            for (const auto& obj : active->getOwnedObjects()) {
                if (obj) beings.push_back(obj.get());
            }
            for (const auto& law : lawManager.getAll()) {
                if (law) beings.push_back(law.get());
            }
            for (const auto& rel : active->formation().relations().getAll()) {
                if (rel) beings.push_back(rel.get());
            }
            for (const auto& material : materials.getAll()) {
                if (material) beings.push_back(material.get());
            }
            for (const auto& category : ::categories.getAll()) {
                if (category) beings.push_back(category.get());
            }
            beings.push_back(&player);
        });

        Universe::instance().setRelationProvider([this](std::vector<Relation*>& relations) {
            if (zones.zones().empty()) return;
            auto active = zones.zones()[zones.currentIndex()];
            if (!active) return;
            for (const auto& rel : active->formation().relations().getAll()) {
                if (rel) relations.push_back(rel.get());
            }
        });

        Universe::instance().setRelationRegistrar([this](std::shared_ptr<Relation> relation) {
            if (zones.zones().empty()) return;
            auto active = zones.zones()[zones.currentIndex()];
            if (active) {
                active->formation().relations().add(std::move(relation));
            }
        });

        // 3. Setup SaveContext
        ctx.camera = &camera;
        ctx.mouseHandler = &mouseHandler;
        ctx.currentColor = currentColor;
        ctx.person = &player;
        ctx.lawManager = &lawManager;
        ctx.worldTime = &worldTime;
        ctx.unpackForAuthoring = false;

        // 4. Perform app boot hydration FIRST (matching Engine::initLogic boot sequence)
        zones.hydrateFromZoneStore();
    }

    void loadWorld(const std::string& filename) {
        zones.loadState(filename, ctx);
    }
};

// A test that needs an actually-authored world (chess_app.json, and
// whatever Zone identities that World names) used to point SaveSystem
// straight at the repo's real saves/ tree so the file would resolve, but
// BootedEngineHarness's constructor hydrates from that same root — every
// real Zone identity, not only the one the test cares about — and a
// subsequent load's "preserve unsaved work" write-back re-serialized ALL of
// them back to disk, silently drifting real save files by a few duplicate
// relation-event timestamps on every single test run. Confirmed and fixed
// 2026-09-07 after several `ctest` runs compounded into thousands of
// duplicate lines in the real saves/zones/Chess/zone.json.
//
// Pointing SaveSystem at a COPY of the tree instead (tried first) traded
// that bug for a different one nobody has fully explained: with an
// otherwise byte-identical copy of saves/zones, saves/homes, and
// saves/persons, chess_click_geometry_test still saw every piece at
// (0,0,0) and every Law answering conditions-failed, no matter which
// subdirectories were included. Rather than keep guessing which piece of
// state secretly depends on the tree's real absolute path, this instead
// runs the test against the REAL tree exactly as it always ran —
// unexplained differences have no surface to appear on — and protects it
// by backing up just the two directories that actually get mutated
// (zones/, homes/) beforehand and restoring them after, regardless of how
// the test's own run went.
inline std::string resolveRealWorldPath(const std::string& relativeWorldPath) {
    std::string filename = relativeWorldPath;
    if (!std::filesystem::exists(filename)) {
        const std::string alt = "../" + relativeWorldPath;
        if (std::filesystem::exists(alt)) filename = alt;
    }
    return filename;
}

// RAII: construct AFTER resolving the world path but BEFORE constructing
// BootedEngineHarness (whose constructor hydrates from whatever
// SaveSystem's root already is), so the backup captures the pre-test state.
// Calls SaveSystem::setSaveRoot to the real saves/ directory itself — same
// as the tree these tests always ran against — and restores saves/zones
// and saves/homes from the backup on destruction. Does nothing (and leaves
// SaveSystem's root untouched) if `resolvedWorldPath` is not a real
// `saves/worlds/...` file, so a CI environment where the optional fixture
// is simply absent behaves exactly as before.
struct RealSaveTreeGuard {
    std::filesystem::path realSaves;
    std::filesystem::path backup;

    explicit RealSaveTreeGuard(const std::string& resolvedWorldPath) {
        if (!std::filesystem::exists(resolvedWorldPath)) return;
        const auto p = std::filesystem::absolute(resolvedWorldPath);
        if (p.parent_path().filename() != "worlds" ||
            p.parent_path().parent_path().filename() != "saves") {
            return;
        }
        realSaves = p.parent_path().parent_path();
        backup = std::filesystem::temp_directory_path() /
            ("earthcall-save-backup-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(backup);
        std::error_code ec;
        if (std::filesystem::exists(realSaves / "zones")) {
            std::filesystem::copy(realSaves / "zones", backup / "zones",
                std::filesystem::copy_options::recursive, ec);
        }
        if (std::filesystem::exists(realSaves / "homes")) {
            std::filesystem::copy(realSaves / "homes", backup / "homes",
                std::filesystem::copy_options::recursive, ec);
        }
        SaveSystem::setSaveRoot(realSaves.string());
    }

    ~RealSaveTreeGuard() {
        if (realSaves.empty()) return;
        std::error_code ec;
        std::filesystem::remove_all(realSaves / "zones", ec);
        if (std::filesystem::exists(backup / "zones")) {
            std::filesystem::copy(backup / "zones", realSaves / "zones",
                std::filesystem::copy_options::recursive, ec);
        }
        std::filesystem::remove_all(realSaves / "homes", ec);
        if (std::filesystem::exists(backup / "homes")) {
            std::filesystem::copy(backup / "homes", realSaves / "homes",
                std::filesystem::copy_options::recursive, ec);
        }
        std::filesystem::remove_all(backup, ec);
        SaveSystem::setSaveRoot("");
    }

    RealSaveTreeGuard(const RealSaveTreeGuard&) = delete;
    RealSaveTreeGuard& operator=(const RealSaveTreeGuard&) = delete;
};

} // namespace TestSupport
