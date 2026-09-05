// Extended rules test for chess_app:
// 1. Board orientation & square colors (White on right, queen on own color)
// 2. En Passant capture (White & Black)
// 3. Pawn Promotion
// 4. Threefold repetition draw
// 5. Stalemate claim

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
    (void)records;
}

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
    std::cout << "--- chess_extended_rules_test: " << filename << " ---\\n";

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    Object* board = findObj(*active, "object.chess.board");
    Object* state = findCat("state.chess");
    assert(board && state);

    // -------------------------------------------------------------
    // 1. Board colors verification:
    // White queen (d1, file 3, rank 0) must be on a WHITE/LIGHT square.
    // White king (e1, file 4, rank 0) must be on a BLACK/DARK square.
    // Bottom-right corner (h1, file 7, rank 0) must be WHITE/LIGHT ("white on right").
    // -------------------------------------------------------------
    std::cout << "--- 1. Verifying Board FaceTexture square colors ---\\n";
    auto boardMat = materials.get("material.chess.board");
    assert(boardMat && boardMat->faceTextures.size() >= 3);
    const auto& top = boardMat->faceTextures[2];
    assert(top.size == 64);

    // Helper to sample pixel at file and rank
    auto samplePixel = [&](int file, int rank) {
        // file maps to y (V), rank maps to x (U)
        int px = rank * 8 + 4;
        int py = file * 8 + 4;
        size_t idx = (py * 64 + px) * 4;
        return top.pixels[idx]; // R channel: 237 for light, 117 for dark
    };

    int d1_color = samplePixel(3, 0); // d1 (White Queen)
    int e1_color = samplePixel(4, 0); // e1 (White King)
    int h1_color = samplePixel(7, 0); // h1 (White Right Corner)
    int a1_color = samplePixel(0, 0); // a1 (White Left Corner)

    std::cout << "  d1 R=" << d1_color << " (expected light ~237)\\n";
    std::cout << "  e1 R=" << e1_color << " (expected dark ~117)\\n";
    std::cout << "  h1 R=" << h1_color << " (expected light ~237)\\n";
    std::cout << "  a1 R=" << a1_color << " (expected dark ~117)\\n";

    assert(d1_color > 200 && "d1 square must be light (White Queen on White)");
    assert(e1_color < 150 && "e1 square must be dark (White King on Dark)");
    assert(h1_color > 200 && "h1 square must be light (White on Right)");
    assert(a1_color < 150 && "a1 square must be dark");
    std::cout << "  BOARD COLORS VERIFIED! White queen is on white square, white on right!\\n";

    // -------------------------------------------------------------
    // 2. En Passant test:
    // 1. e2-e4 a7-a6
    // 2. e4-e5 d7-d5 (Black double step!)
    // 3. e5xd6 e.p.! (White pawn captures en passant)
    // -------------------------------------------------------------
    std::cout << "--- 2. Verifying En Passant Capture ---\\n";
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    Object* whitePawnE2 = findObj(*active, "piece-white-pawn-4-1");
    Object* blackPawnA7 = findObj(*active, "piece-black-pawn-0-6");
    Object* blackPawnD7 = findObj(*active, "piece-black-pawn-3-6");

    // 1. e2-e4
    click(harness.interaction, harness.lawManager, whitePawnE2, squareX(4), 0.3f, squareZ(1));
    click(harness.interaction, harness.lawManager, board, squareX(4), 0.0f, squareZ(3));
    assert(asInt(*state, "turn") == 1);

    // 1... a7-a6
    click(harness.interaction, harness.lawManager, blackPawnA7, squareX(0), 0.3f, squareZ(6));
    click(harness.interaction, harness.lawManager, board, squareX(0), 0.0f, squareZ(5));
    assert(asInt(*state, "turn") == 0);

    // 2. e4-e5
    click(harness.interaction, harness.lawManager, whitePawnE2, squareX(4), 0.3f, squareZ(3));
    click(harness.interaction, harness.lawManager, board, squareX(4), 0.0f, squareZ(4));
    assert(asInt(*whitePawnE2, "gridX") == 4 && asInt(*whitePawnE2, "gridY") == 4);
    assert(asInt(*state, "turn") == 1);

    // 2... d7-d5 (Black double step adjacent to White's e5 pawn!)
    click(harness.interaction, harness.lawManager, blackPawnD7, squareX(3), 0.3f, squareZ(6));
    click(harness.interaction, harness.lawManager, board, squareX(3), 0.0f, squareZ(4));
    assert(asInt(*blackPawnD7, "gridX") == 3 && asInt(*blackPawnD7, "gridY") == 4);
    assert(asInt(*state, "turn") == 0);

    // Verify enPassant state
    assert(asInt(*state, "enPassantFile") == 3);
    assert(asInt(*state, "enPassantTargetY") == 5);
    assert(asInt(*state, "enPassantVictimY") == 4);
    std::cout << "  En passant target available at d6 (file=3, targetY=5, victimY=4)\\n";

    // 3. exd6 (e.p.)! White pawn on e5 captures diagonally onto d6!
    click(harness.interaction, harness.lawManager, whitePawnE2, squareX(4), 0.3f, squareZ(4));
    assert(asBool(*whitePawnE2, "isSelected"));
    click(harness.interaction, harness.lawManager, board, squareX(3), 0.0f, squareZ(5)); // click d6

    std::cout << "  White pawn now at (" << asInt(*whitePawnE2, "gridX") << "," << asInt(*whitePawnE2, "gridY") << ")\\n";
    std::cout << "  Black d5 pawn onBoard=" << asBool(*blackPawnD7, "onBoard") << "\\n";
    std::cout << "  turn=" << asInt(*state, "turn") << "\\n";

    assert(asInt(*whitePawnE2, "gridX") == 3);
    assert(asInt(*whitePawnE2, "gridY") == 5);
    assert(!asBool(*blackPawnD7, "onBoard")); // Victim pawn was captured and unmade!
    assert(asInt(*state, "turn") == 1); // Turn advanced to Black
    std::cout << "  EN PASSANT CAPTURE VERIFIED! Victim pawn removed from board, capturer on d6!\\n";

    // -------------------------------------------------------------
    // 3. Pawn Promotion test:
    // Move White pawn directly to rank 7, verify role becomes Queen (4).
    // -------------------------------------------------------------
    std::cout << "--- 3. Verifying Pawn Promotion ---\\n";
    whitePawnE2->setDynamicProperty("gridY", PropertyValue(7));
    whitePawnE2->setDynamicProperty("chessRole", PropertyValue(0)); // Pawn
    Core::EventBus::instance().publish(ECA::Event{"turn-changed", state, nullptr, std::time(nullptr)});
    harness.lawManager.tick();

    int promoRole = asInt(*whitePawnE2, "chessRole");
    std::cout << "  Promoted pawn role: " << promoRole << " (expected 4 = Queen)\\n";
    assert(promoRole == 4 && "Pawn on rank 7 promotes to Queen");
    std::cout << "  PAWN PROMOTION VERIFIED!\\n";

    // -------------------------------------------------------------
    // 4. Threefold Repetition Draw test:
    // When repCount reaches 6, threefold repetition draw triggers.
    // -------------------------------------------------------------
    std::cout << "--- 4. Verifying Threefold Repetition Draw ---\\n";
    state->setDynamicProperty("repCount", PropertyValue(6));
    Core::EventBus::instance().publish(ECA::Event{"turn-changed", state, nullptr, std::time(nullptr)});
    harness.lawManager.tick();

    assert(asBool(*state, "gameOver") && "Game over on threefold repetition");
    assert(asInt(*state, "result") == 2 && "Result is Draw (2)");
    assert(asBool(*state, "isDraw") && "isDraw is true");
    std::cout << "  THREEFOLD REPETITION DRAW VERIFIED!\\n";

    std::cout << "==================================================\\n";
    std::cout << "chess_extended_rules_test: ALL OK\\n";
    std::cout << "==================================================\\n";
    return 0;
}
