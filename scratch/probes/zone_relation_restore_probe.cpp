// Probe: re-author saves/zones/Chess/zone.json's formationRelations from the
// session snapshot saves/worlds/chess_app.json, using the FIXED production
// load path (ZoneManager::loadState -> admitFromJson merge -> persistZones),
// not hand-edited JSON. This is Bug #7 repair-order step 4
// (docs/Agenda/Tasks/Specific Tasks/Zone_Relation_Graph_Loss.md): the store
// still holds formationRelations: [] on disk even after the code fix,
// because the fix heals the graph in memory on every load rather than
// retroactively rewriting what is already on disk. Running the real load
// and persist once, through the real merge + refuse-empty-overwrite guard,
// makes the recovery durable and auditable (persistZones prints what it
// wrote, and would have REFUSED here before the fix).
//
// Build (from repo root, after the ordinary cmake configure):
//   c++ -std=c++17 -UNDEBUG -Isrc -Iimgui -Ithird_party/flatbuffers/include \
//       -Ilocal_deps/include -Ibuild/_deps/asio-src/asio/include \
//       -Ibuild/_deps/websocketpp-src -DASIO_STANDALONE \
//       scratch/probes/zone_relation_restore_probe.cpp \
//       $(find build/CMakeFiles/earthcall_core.dir -name '*.o') \
//       -o scratch/probes/zone_relation_restore_probe
//
// Run from repo root: ./scratch/probes/zone_relation_restore_probe

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

extern MaterialManager materials;
extern CategoryManager categories;

int main() {
    std::string filename = "saves/worlds/chess_app.json";
    if (!std::filesystem::exists(filename)) {
        std::cerr << "run from repo root: saves/worlds/chess_app.json not found\n";
        return 1;
    }
    const auto p = std::filesystem::absolute(filename);
    assert(p.parent_path().filename() == "worlds" &&
           p.parent_path().parent_path().filename() == "saves");
    SaveSystem::setSaveRoot(p.parent_path().parent_path().string());

    Core::Camera camera;
    MouseHandler mouseHandler;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    LawManager lawManager;

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

    ZoneManager zones;
    zones.loadState(filename, ctx);

    std::shared_ptr<Zone> chess;
    for (auto& z : zones.zones()) {
        if (z && z->getIdentifier() == "Chess") { chess = z; break; }
    }
    assert(chess && "Chess zone must be present after load");
    const size_t relCount = chess->formation().relations().getAll().size();
    std::cout << "Chess zone in memory after merged load: " << relCount
              << " formation relation(s)\n";
    assert(relCount == 38 && "expected the 38 chess relations to be recovered");

    zones.persistZones();

    nlohmann::json onDisk = SaveSystem::readZoneIdentity("Chess");
    const size_t written = onDisk.value("formationRelations", nlohmann::json::array()).size();
    std::cout << "saves/zones/Chess/zone.json now holds " << written
              << " formation relation(s)\n";
    assert(written == 38);

    std::cout << "zone_relation_restore_probe: OK\n";
    return 0;
}
