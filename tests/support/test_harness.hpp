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

} // namespace TestSupport
