// Fixture test: verify castling rules in chess_app
// Kingside and Queenside castling for White and Black.

#include "support/test_harness.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

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

// Convert grid (file, rank) to square world coordinates (wx, wz)
float squareX(int file) { return (file + 0.5f) * 1.0f - 4.0f; }
float squareZ(int rank) { return (rank + 0.5f) * 1.0f - 4.0f; }

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
    std::cout << "--- chess_castling_test: " << filename << " ---\n";

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    Object* board = findObj(*active, "object.chess.board");
    Object* state = findCat("state.chess");
    Object* whiteKing = findObj(*active, "piece-white-king-4-0");
    Object* whiteRookH1 = findObj(*active, "piece-white-rook-7-0");
    Object* whiteRookA1 = findObj(*active, "piece-white-rook-0-0");
    Object* blackKing = findObj(*active, "piece-black-king-4-7");
    Object* blackRookH8 = findObj(*active, "piece-black-rook-7-7");
    Object* blackRookA8 = findObj(*active, "piece-black-rook-0-7");

    assert(board && state && whiteKing && whiteRookH1 && blackKing && blackRookH8);

    Universe::instance().setClock(0.0, 1.0 / 60.0);

    // Initial state checks
    assert(asInt(*whiteKing, "gridX") == 4 && asInt(*whiteKing, "gridY") == 0);
    assert(!asBool(*whiteKing, "hasMoved"));
    assert(asInt(*whiteRookH1, "gridX") == 7 && asInt(*whiteRookH1, "gridY") == 0);
    assert(!asBool(*whiteRookH1, "hasMoved"));
    assert(asInt(*blackKing, "gridX") == 4 && asInt(*blackKing, "gridY") == 7);
    assert(!asBool(*blackKing, "hasMoved"));

    std::cout << "--- Step 1: 1. e2-e4 ---\n";
    Object* whitePawnE2 = findObj(*active, "piece-white-pawn-4-1");
    click(harness.interaction, harness.lawManager, whitePawnE2, squareX(4), 0.3f, squareZ(1));
    click(harness.interaction, harness.lawManager, board, squareX(4), 0.0f, squareZ(3));
    assert(asInt(*state, "turn") == 1);

    std::cout << "--- Step 2: 1... e7-e5 ---\n";
    Object* blackPawnE7 = findObj(*active, "piece-black-pawn-4-6");
    click(harness.interaction, harness.lawManager, blackPawnE7, squareX(4), 0.3f, squareZ(6));
    click(harness.interaction, harness.lawManager, board, squareX(4), 0.0f, squareZ(4));
    assert(asInt(*state, "turn") == 0);

    std::cout << "--- Step 3: 2. Ng1-f3 ---\n";
    Object* whiteKnightG1 = findObj(*active, "piece-white-knight-6-0");
    click(harness.interaction, harness.lawManager, whiteKnightG1, squareX(6), 0.3f, squareZ(0));
    click(harness.interaction, harness.lawManager, board, squareX(5), 0.0f, squareZ(2));
    assert(asInt(*whiteKnightG1, "gridX") == 5 && asInt(*whiteKnightG1, "gridY") == 2);
    assert(asInt(*state, "turn") == 1);

    std::cout << "--- Step 4: 2... Nb8-c6 ---\n";
    Object* blackKnightB8 = findObj(*active, "piece-black-knight-1-7");
    click(harness.interaction, harness.lawManager, blackKnightB8, squareX(1), 0.3f, squareZ(7));
    click(harness.interaction, harness.lawManager, board, squareX(2), 0.0f, squareZ(5));
    assert(asInt(*state, "turn") == 0);

    std::cout << "--- Step 5: 3. Bf1-c4 (f1 and g1 now clear!) ---\n";
    Object* whiteBishopF1 = findObj(*active, "piece-white-bishop-5-0");
    click(harness.interaction, harness.lawManager, whiteBishopF1, squareX(5), 0.3f, squareZ(0));
    click(harness.interaction, harness.lawManager, board, squareX(2), 0.0f, squareZ(3));
    assert(asInt(*whiteBishopF1, "gridX") == 2 && asInt(*whiteBishopF1, "gridY") == 3);
    assert(asInt(*state, "turn") == 1);

    std::cout << "--- Step 6: 3... d7-d6 ---\n";
    Object* blackPawnD7 = findObj(*active, "piece-black-pawn-3-6");
    click(harness.interaction, harness.lawManager, blackPawnD7, squareX(3), 0.3f, squareZ(6));
    click(harness.interaction, harness.lawManager, board, squareX(3), 0.0f, squareZ(5));
    assert(asInt(*state, "turn") == 0);

    std::cout << "--- Step 7: 4. O-O (White Kingside Castling: e1 to g1, h1 rook to f1) ---\n";
    click(harness.interaction, harness.lawManager, whiteKing, squareX(4), 0.48f, squareZ(0));
    assert(asBool(*whiteKing, "isSelected"));
    click(harness.interaction, harness.lawManager, board, squareX(6), 0.0f, squareZ(0)); // g1

    std::cout << "  white king at (" << asInt(*whiteKing, "gridX") << "," << asInt(*whiteKing, "gridY") << ")\n";
    std::cout << "  white rook h1 at (" << asInt(*whiteRookH1, "gridX") << "," << asInt(*whiteRookH1, "gridY") << ")\n";
    std::cout << "  turn=" << asInt(*state, "turn") << "\n";

    assert(asInt(*whiteKing, "gridX") == 6);
    assert(asInt(*whiteKing, "gridY") == 0);
    assert(asBool(*whiteKing, "hasMoved"));

    assert(asInt(*whiteRookH1, "gridX") == 5);
    assert(asInt(*whiteRookH1, "gridY") == 0);
    assert(asBool(*whiteRookH1, "hasMoved"));

    assert(asInt(*state, "turn") == 1); // Turn advanced to Black
    std::cout << "  WHITE KINGSIDE CASTLING VERIFIED!\n";

    std::cout << "--- Step 8: Black clears queenside: 4... Bc8-e6 ---\n";
    Object* blackBishopC8 = findObj(*active, "piece-black-bishop-2-7");
    click(harness.interaction, harness.lawManager, blackBishopC8, squareX(2), 0.4f, squareZ(7));
    click(harness.interaction, harness.lawManager, board, squareX(4), 0.0f, squareZ(5));
    assert(asInt(*state, "turn") == 0);

    std::cout << "--- Step 9: White plays a2-a3 ---\n";
    Object* whitePawnA2 = findObj(*active, "piece-white-pawn-0-1");
    click(harness.interaction, harness.lawManager, whitePawnA2, squareX(0), 0.3f, squareZ(1));
    click(harness.interaction, harness.lawManager, board, squareX(0), 0.0f, squareZ(2));
    assert(asInt(*state, "turn") == 1);

    std::cout << "--- Step 10: 5... Qd8-d7 (b8, c8, d8 now clear for Black!) ---\n";
    Object* blackQueen = findObj(*active, "piece-black-queen-3-7");
    click(harness.interaction, harness.lawManager, blackQueen, squareX(3), 0.42f, squareZ(7));
    click(harness.interaction, harness.lawManager, board, squareX(3), 0.0f, squareZ(6));
    assert(asInt(*state, "turn") == 0);

    std::cout << "--- Step 11: White plays h2-h3 ---\n";
    Object* whitePawnH2 = findObj(*active, "piece-white-pawn-7-1");
    click(harness.interaction, harness.lawManager, whitePawnH2, squareX(7), 0.3f, squareZ(1));
    click(harness.interaction, harness.lawManager, board, squareX(7), 0.0f, squareZ(2));
    assert(asInt(*state, "turn") == 1);

    std::cout << "--- Step 12: 6... O-O-O (Black Queenside Castling: e8 to c8, a8 rook to d8) ---\n";
    click(harness.interaction, harness.lawManager, blackKing, squareX(4), 0.48f, squareZ(7));
    assert(asBool(*blackKing, "isSelected"));
    click(harness.interaction, harness.lawManager, board, squareX(2), 0.0f, squareZ(7)); // c8

    std::cout << "  black king at (" << asInt(*blackKing, "gridX") << "," << asInt(*blackKing, "gridY") << ")\n";
    std::cout << "  black rook a8 at (" << asInt(*blackRookA8, "gridX") << "," << asInt(*blackRookA8, "gridY") << ")\n";
    std::cout << "  turn=" << asInt(*state, "turn") << "\n";

    assert(asInt(*blackKing, "gridX") == 2);
    assert(asInt(*blackKing, "gridY") == 7);
    assert(asBool(*blackKing, "hasMoved"));

    assert(asInt(*blackRookA8, "gridX") == 3);
    assert(asInt(*blackRookA8, "gridY") == 7);
    assert(asBool(*blackRookA8, "hasMoved"));

    assert(asInt(*state, "turn") == 0); // Turn advanced to White
    std::cout << "  BLACK QUEENSIDE CASTLING VERIFIED!\n";

    std::cout << "==================================================\n";
    std::cout << "chess_castling_test: ALL OK\n";
    std::cout << "==================================================\n";
    return 0;
}
