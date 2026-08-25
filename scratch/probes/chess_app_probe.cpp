// Probe: does saves/worlds/chess_app.json load as a chess world a Person
// can play — one board, 32 pieces on their squares, queens on their colours,
// object-clicked walking a pawn, capture unmaking onto a rack, an illegal
// self-check reverted.
//
// Build (from repo root, after the ordinary cmake configure):
//   c++ -std=c++17 -UNDEBUG -Isrc -Iimgui -Ithird_party/flatbuffers/include \
//       -Ilocal_deps/include -Ibuild/_deps/asio-src/asio/include \
//       -Ibuild/_deps/websocketpp-src -DASIO_STANDALONE \
//       scratch/probes/chess_app_probe.cpp \
//       $(find build/CMakeFiles/earthcall_core.dir -name '*.o') \
//       -o scratch/probes/chess_app_probe

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

extern MaterialManager materials;
extern CategoryManager categories;

namespace {

int asInt(Singular& being, const char* name, int fallback = -999) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return fallback;
    if (const int* i = std::get_if<int>(&v)) return *i;
    double n = 0.0;
    if (propertyValueToNumber(v, n)) return static_cast<int>(n);
    return fallback;
}

bool asBool(Singular& being, const char* name) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return false;
    if (const bool* b = std::get_if<bool>(&v)) return *b;
    double n = 0.0;
    if (propertyValueToNumber(v, n)) return n != 0.0;
    return false;
}

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

Object* findCat(const std::string& id) {
    auto c = categories.get(id);
    return c ? c.get() : nullptr;
}

void click(Singularity::Input::InteractionChannel* interaction,
           LawManager& lawManager,
           Object* subject,
           float wx, float wy, float wz) {
    interaction->pointerWorld = glm::vec3(wx, wy, wz);
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", subject, nullptr, std::time(nullptr)});
    auto records = lawManager.tick();
    std::cout << "  tick records: " << records.size() << "\n";
    for (const auto& r : records) {
        std::cout << "    " << r.lawId << " -> " << r.targetId
                  << " " << Law::resultName(r.result) << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "saves/worlds/chess_app.json";
    if (argc <= 1 && !std::filesystem::exists(filename)) {
        if (std::filesystem::exists("../saves/worlds/chess_app.json"))
            filename = "../saves/worlds/chess_app.json";
    }
    {
        const auto p = std::filesystem::absolute(filename);
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }
    std::cout << "--- chess_app_probe: " << filename << " ---\n";

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

    Singularity::Input::InteractionChannel::syncRegister(lawManager);
    auto* interaction = Singularity::Input::InteractionChannel::find(lawManager);
    assert(interaction);
    interaction->setEnabled(true);

    ZoneManager zones;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        if (zones.zones().empty()) return;
        auto active = zones.zones()[zones.currentIndex()];
        if (!active) return;
        beings.push_back(active.get());
        for (const auto& obj : active->getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
        for (const auto& rel : active->formation().relations().getAll()) {
            if (rel) beings.push_back(rel.get());
        }
        for (const auto& law : lawManager.getAll()) {
            if (law) beings.push_back(law.get());
        }
        for (const auto& material : materials.getAll()) {
            if (material) beings.push_back(material.get());
        }
        for (const auto& category : categories.getAll()) {
            if (category) beings.push_back(category.get());
        }
        beings.push_back(&player);
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& relations) {
        if (zones.zones().empty()) return;
        auto active = zones.zones()[zones.currentIndex()];
        if (!active) return;
        for (const auto& rel : active->formation().relations().getAll()) {
            if (rel) relations.push_back(rel.get());
        }
    });

    zones.loadState(filename, ctx);
    std::cout << "loaded. zones=" << zones.zones().size()
              << " current=" << zones.currentIndex() << "\n";
    assert(!zones.zones().empty());
    auto active = zones.zones()[zones.currentIndex()];
    assert(active);
    assert(active->getIdentifier() == "Chess");

    Object* board = findObj(*active, "object.chess.board");
    Object* state = findCat("state.chess");
    Object* author = findCat("grok-4.6");
    Object* whiteQueen = findObj(*active, "piece-white-queen-3-0");
    Object* blackQueen = findObj(*active, "piece-black-queen-3-7");
    Object* whitePawnE2 = findObj(*active, "piece-white-pawn-4-1");
    Object* blackPawnD7 = findObj(*active, "piece-black-pawn-3-6");
    Object* whiteRookA1 = findObj(*active, "piece-white-rook-0-0");
    Object* whiteBishopC1 = findObj(*active, "piece-white-bishop-2-0");
    Object* whiteKnightB1 = findObj(*active, "piece-white-knight-1-0");

    assert(board && "one board prism");
    assert(state && "state.chess extra-spatial being");
    assert(author && "grok-4.6 first-mover identity");
    assert(whiteQueen && blackQueen && whitePawnE2);

    int boardCount = 0, pieceCount = 0;
    for (const auto& o : active->getOwnedObjects()) {
        if (!o) continue;
        const std::string id = o->getIdentifier();
        if (id == "object.chess.board") boardCount++;
        if (id.rfind("piece-", 0) == 0) pieceCount++;
        assert(id.rfind("board-", 0) != 0 && "no 64-square board objects");
    }
    assert(boardCount == 1);
    assert(pieceCount == 32);
    std::cout << "  one board, 32 pieces\n";

    // Queens on their colours: d1 is light (3+0 odd), d8 is dark (3+7 even).
    assert(asInt(*whiteQueen, "gridX") == 3 && asInt(*whiteQueen, "gridY") == 0);
    assert(asInt(*blackQueen, "gridX") == 3 && asInt(*blackQueen, "gridY") == 7);
    std::cout << "  white queen on d1 (light), black queen on d8 (dark)\n";

    auto boardMat = materials.get("material.chess.board");
    assert(boardMat && boardMat->faceTextures.size() >= 3);
    const auto& top = boardMat->faceTextures[2];
    assert(top.size == 64 && top.pixels.size() == 64u * 64u * 4u);
    // a1 (file 0, rank 0) is dark: pixel x=0 (rank), y=0 (file).
    assert(top.pixels[0] == 117 && top.pixels[1] == 69 && top.pixels[2] == 33);
    // d1 (file 3, rank 0) is light: x=0, y = 3*8 = 24.
    const size_t d1 = (24u * 64u + 0u) * 4u;
    assert(top.pixels[d1] == 237 && top.pixels[d1 + 1] == 214);
    std::cout << "  board FaceTexture is a checkerboard; a1 dark, d1 light\n";

    Law* clickLaw = lawManager.find("law-chess-click");
    assert(clickLaw);
    assert(!clickLaw->authors().getMembers().empty());

    Universe::instance().setClock(0.0, 1.0 / 60.0);

    // Starting pose: e2 pawn at (0.5, restY, -2.5)
    assert(std::fabs(whitePawnE2->getPosition().x - 0.5f) < 1e-3f);
    assert(std::fabs(whitePawnE2->getPosition().z + 2.5f) < 1e-3f);

    std::cout << "--- e2 pawn clicked, then e4 ---\n";
    click(interaction, lawManager, whitePawnE2, 0.5f, 0.3f, -2.5f);
    assert(asBool(*whitePawnE2, "isSelected"));
    assert(asInt(*state, "selectedX") == 4);
    assert(asInt(*state, "selectedY") == 1);

    click(interaction, lawManager, board, 0.5f, 0.0f, -0.5f);
    std::cout << "  e2 now (" << asInt(*whitePawnE2, "gridX") << ","
              << asInt(*whitePawnE2, "gridY") << ") turn="
              << asInt(*state, "turn") << " pos=("
              << whitePawnE2->getPosition().x << ","
              << whitePawnE2->getPosition().z << ")\n";
    assert(asInt(*whitePawnE2, "gridX") == 4);
    assert(asInt(*whitePawnE2, "gridY") == 3);
    assert(asBool(*whitePawnE2, "hasMoved"));
    assert(asInt(*state, "turn") == 1);
    assert(std::fabs(whitePawnE2->getPosition().x - 0.5f) < 1e-3f);
    assert(std::fabs(whitePawnE2->getPosition().z + 0.5f) < 1e-3f);
    std::cout << "  pawn walked e2-e4, anchored, turn is black\n";

    std::cout << "--- black e7-e5 ---\n";
    Object* blackPawnE7 = findObj(*active, "piece-black-pawn-4-6");
    assert(blackPawnE7);
    click(interaction, lawManager, blackPawnE7, 0.5f, 0.3f, 2.5f);
    click(interaction, lawManager, board, 0.5f, 0.0f, 0.5f);
    assert(asInt(*blackPawnE7, "gridY") == 4);
    assert(asInt(*state, "turn") == 0);
    std::cout << "  black pawn e7-e5\n";

    std::cout << "--- white d2-d4, black captures exd4 ---\n";
    Object* whitePawnD2 = findObj(*active, "piece-white-pawn-3-1");
    assert(whitePawnD2);
    click(interaction, lawManager, whitePawnD2, -0.5f, 0.3f, -2.5f);
    click(interaction, lawManager, board, -0.5f, 0.0f, -0.5f);
    assert(asInt(*whitePawnD2, "gridY") == 3);

    click(interaction, lawManager, blackPawnE7, 0.5f, 0.3f, 0.5f);
    click(interaction, lawManager, whitePawnD2, -0.5f, 0.3f, -0.5f);
    std::cout << "  black e5 at (" << asInt(*blackPawnE7, "gridX") << ","
              << asInt(*blackPawnE7, "gridY") << ") onBoard="
              << asBool(*blackPawnE7, "onBoard") << "\n";
    std::cout << "  white d4 onBoard=" << asBool(*whitePawnD2, "onBoard")
              << " x=" << whitePawnD2->getPosition().x << "\n";
    assert(asInt(*blackPawnE7, "gridX") == 3);
    assert(asInt(*blackPawnE7, "gridY") == 3);
    assert(asBool(*blackPawnE7, "onBoard"));
    assert(!asBool(*whitePawnD2, "onBoard"));
    assert(whitePawnD2->getPosition().x < -4.0f || whitePawnD2->getPosition().x > 4.0f);
    std::cout << "  capture unmade the white pawn onto the rack, not y=-100\n";

    std::cout << "--- illegal: white rook tries to leap its own pawn ---\n";
    // a2 pawn is still there. a1 rook cannot go to a3.
    Object* whitePawnA2 = findObj(*active, "piece-white-pawn-0-1");
    assert(whitePawnA2);
    assert(asBool(*whitePawnA2, "onBoard"));
    click(interaction, lawManager, whiteRookA1, -3.5f, 0.4f, -3.5f);
    click(interaction, lawManager, board, -3.5f, 0.0f, -1.5f); // a3
    assert(asInt(*whiteRookA1, "gridX") == 0);
    assert(asInt(*whiteRookA1, "gridY") == 0);
    std::cout << "  rook still on a1 (path blocked)\n";

    std::cout << "--- legal: white bishop c1 moves to e3 (slope +1 diagonal) ---\n";
    // d2 pawn is unmade, so c1 bishop has open path to e3 (2,0 -> 4,2)
    click(interaction, lawManager, whiteBishopC1, -1.5f, 0.4f, -3.5f);
    click(interaction, lawManager, board, 0.5f, 0.0f, -1.5f); // e3
    assert(asInt(*whiteBishopC1, "gridX") == 4);
    assert(asInt(*whiteBishopC1, "gridY") == 2);
    assert(asInt(*state, "turn") == 1);
    std::cout << "  bishop moved c1-e3\n";

    std::cout << "--- legal: black queen d8 moves to a5 (slope -1 diagonal) giving distant check to white king at e1 ---\n";
    click(interaction, lawManager, blackQueen, -0.5f, 0.4f, 3.5f); // d8
    click(interaction, lawManager, board, 3.5f, 0.0f, -0.5f); // h4
    assert(asInt(*blackQueen, "gridX") == 7);
    assert(asInt(*blackQueen, "gridY") == 3);
    assert(asInt(*state, "turn") == 0);
    std::cout << "  black queen moved d8-h4 (clear slope -1 diagonal across board)\n";

    std::cout << "--- distant check test: black queen on h4 (7,3) attacks white king on e1 (4,0) ---\n";
    Object* whitePawnF2 = findObj(*active, "piece-white-pawn-5-1");
    assert(whitePawnF2);
    click(interaction, lawManager, whitePawnF2, 1.5f, 0.3f, -2.5f); // f2
    click(interaction, lawManager, board, 1.5f, 0.0f, -0.5f); // f4 (vacates diagonal to king!)
    assert(asInt(*whitePawnF2, "gridX") == 5);
    assert(asInt(*whitePawnF2, "gridY") == 1);
    assert(asInt(*state, "turn") == 0); // Still white turn, move reverted!
    std::cout << "  f2 pawn move reverted because it exposed king to distant diagonal check from queen at h4!\n";

    (void)whiteKnightB1;
    (void)blackPawnD7;

    std::cout << "==================================================\n";
    std::cout << "chess_app_probe: ALL OK\n";
    std::cout << "==================================================\n";
    return 0;
}
