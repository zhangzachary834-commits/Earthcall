#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main(int argc, char** argv) {
    const std::string filename = (argc > 1) ? argv[1] : "saves/worlds/chess_first_mover.json";
    std::cout << "--- Testing Chess First Mover Save: " << filename << " ---" << std::endl;
    
    try {
        Core::Camera camera;
        MouseHandler mouseHandler;
        Soul soul("Player");
        Body body("humanoid", "default");
        Person player(std::move(soul), std::move(body), "default");
        LawManager lawManager;
        lawManager.connectToEventBus();
        
        float currentColor[3] = {1.0f, 1.0f, 1.0f};
        double worldTime = 0.0;
        
        SaveContext ctx;
        ctx.camera = &camera;
        ctx.mouseHandler = &mouseHandler;
        ctx.currentColor = currentColor;
        ctx.player = &player;
        ctx.lawManager = &lawManager;
        ctx.worldTime = &worldTime;
        ctx.unpackForAuthoring = false;
        
        // Sync InteractionChannel into lawManager
        Singularity::Input::InteractionChannel::syncRegister(lawManager);
        auto* interaction = Singularity::Input::InteractionChannel::find(lawManager);
        assert(interaction != nullptr);
        interaction->setEnabled(true);
        
        ZoneManager zones;
        
        // Wire Universe providers to the active zone (same as EngineInit does)
        Universe::instance().setProvider([&zones, &lawManager, &player](std::vector<Singular*>& beings) {
            auto activeZone = zones.zones()[zones.currentIndex()];
            if (activeZone) {
                beings.push_back(activeZone.get());
                for (const auto& obj : activeZone->getOwnedObjects()) {
                    if (obj) beings.push_back(obj.get());
                }
                for (const auto& rel : activeZone->formation().relations().getAll()) {
                    if (rel) beings.push_back(rel.get());
                }
            }
            for (const auto& law : lawManager.getAll()) {
                if (law) beings.push_back(law.get());
            }
            beings.push_back(&player);
        });
        
        Universe::instance().setRelationProvider([&zones](std::vector<Relation*>& relations) {
            auto activeZone = zones.zones()[zones.currentIndex()];
            if (activeZone) {
                for (const auto& rel : activeZone->formation().relations().getAll()) {
                    if (rel) relations.push_back(rel.get());
                }
            }
        });
        
        zones.loadState(filename, ctx);
        
        std::cout << "✓ ZoneManager loaded save successfully." << std::endl;
        
        auto activeZone = zones.zones()[zones.currentIndex()];
        Object* board = nullptr;
        Object* stateChess = nullptr;
        Object* whitePawnE2 = nullptr;
        
        for (const auto& o : activeZone->getOwnedObjects()) {
            if (!o) continue;
            if (o->getIdentifier() == "object.chess.board") board = o.get();
            if (o->getIdentifier() == "state.chess") stateChess = o.get();
            if (o->getIdentifier() == "piece-white-pawn-4-1") whitePawnE2 = o.get();
        }
        
        assert(board != nullptr);
        assert(stateChess != nullptr);
        assert(whitePawnE2 != nullptr);
        
        // 1. Check Authored Laws count & Authors
        Law* clickLaw = lawManager.find("law-chess-click");
        assert(clickLaw != nullptr);
        std::cout << "✓ clickLaw author count: " << clickLaw->authors().getMembers().size() << std::endl;
        
        // 2. Simulate Click on White Pawn at e2 (x=4, y=1)
        // World pos: (4 - 3.5, 0.5, 1 - 3.5) = (0.5, 0.5, -2.5)
        std::cout << "--- Simulating Click on White Pawn (e2: x=4, y=1) ---" << std::endl;
        interaction->pointerWorld = glm::vec3(0.5f, 0.0f, -2.5f);
        
        Core::EventBus::instance().publish(ECA::Event{"object-clicked", whitePawnE2, nullptr, std::time(nullptr)});
        auto records = lawManager.tick();
        
        std::cout << "Tick 1 records count: " << records.size() << std::endl;
        for (const auto& r : records) {
            std::cout << "   Record: law=" << r.lawId << ", subject=" << r.targetId << ", result=" << Law::resultName(r.result) << std::endl;
        }
        
        PropertyValue isSelVal, selXVal, selYVal, actVal, txVal, tyVal;
        whitePawnE2->getDynamicProperty("isSelected", isSelVal);
        stateChess->getDynamicProperty("selectedX", selXVal);
        stateChess->getDynamicProperty("selectedY", selYVal);
        stateChess->getDynamicProperty("selectionActive", actVal);
        stateChess->getDynamicProperty("targetX", txVal);
        stateChess->getDynamicProperty("targetY", tyVal);
        
        bool isSel = std::holds_alternative<bool>(isSelVal) && std::get<bool>(isSelVal);
        std::cout << "White pawn e2 isSelected: " << (isSel ? "TRUE" : "FALSE") << std::endl;
        assert(isSel == true);
        assert(std::get<int>(selXVal) == 4);
        assert(std::get<int>(selYVal) == 1);
        assert(std::get<bool>(actVal) == true);
        
        // 3. Simulate Click on Destination Square e4 (x=4, y=3)
        // World pos: (4 - 3.5, 0.0, 3 - 3.5) = (0.5, 0.0, -0.5)
        std::cout << "--- Simulating Click on Board Destination (e4: x=4, y=3) ---" << std::endl;
        interaction->pointerWorld = glm::vec3(0.5f, 0.0f, -0.5f);
        
        Core::EventBus::instance().publish(ECA::Event{"object-clicked", board, nullptr, std::time(nullptr)});
        auto records2 = lawManager.tick();
        
        std::cout << "Tick 2 records count: " << records2.size() << std::endl;
        for (const auto& r : records2) {
            std::cout << "   Record: law=" << r.lawId << ", subject=" << r.targetId << ", result=" << Law::resultName(r.result) << std::endl;
        }
        
        // 4. Verify Pawn Moved!
        PropertyValue gxVal, gyVal, movedVal, turnVal;
        whitePawnE2->getDynamicProperty("gridX", gxVal);
        whitePawnE2->getDynamicProperty("gridY", gyVal);
        whitePawnE2->getDynamicProperty("hasMoved", movedVal);
        stateChess->getDynamicProperty("turn", turnVal);
        
        int newGx = std::get<int>(gxVal);
        int newGy = std::get<int>(gyVal);
        bool hasMoved = std::get<bool>(movedVal);
        int newTurn = std::get<int>(turnVal);
        
        std::cout << "White pawn e2 new grid: (" << newGx << ", " << newGy << ")" << std::endl;
        std::cout << "White pawn e2 hasMoved: " << (hasMoved ? "TRUE" : "FALSE") << std::endl;
        std::cout << "White pawn e2 3D Pos: (" << whitePawnE2->getPosition().x << ", " 
                  << whitePawnE2->getPosition().y << ", " << whitePawnE2->getPosition().z << ")" << std::endl;
        std::cout << "state.chess turn: " << (newTurn == 1 ? "BLACK (1)" : "WHITE (0)") << std::endl;
        
        assert(newGx == 4);
        assert(newGy == 3);
        assert(hasMoved == true);
        assert(std::fabs(whitePawnE2->getPosition().x - 0.5f) < 1e-3f);
        assert(std::fabs(whitePawnE2->getPosition().z - (-0.5f)) < 1e-3f);
        assert(newTurn == 1);
        
        std::cout << "==================================================" << std::endl;
        std::cout << "🎯 PAWN WALKED! ALL FIRST MOVER LAWS VERIFIED! 🎯" << std::endl;
        std::cout << "==================================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "FAILED WITH EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
