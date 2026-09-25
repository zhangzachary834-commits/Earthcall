// Regression test for Bug #25: Promotion button must only promote the piece at (targetX, targetY).
#include "support/test_harness.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include <cassert>
#include <iostream>

namespace {
int asInt(Singular& being, const char* name, int fallback = -999) {
    PropertyValue v;
    if (!lawGetValue(being, PropertyPath::parse(name), v)) return fallback;
    if (const int* i = std::get_if<int>(&v)) return *i;
    double n = 0.0;
    if (propertyValueToNumber(v, n)) return static_cast<int>(n);
    return fallback;
}

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}
} // namespace

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : TestSupport::resolveRealWorldPath("saves/worlds/chess_app.json");
    TestSupport::RealSaveTreeGuard saveGuard(filename);
    std::cout << "--- chess_promotion_isolation_test ---\n";

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    Object* state = categories.get("state.chess").get();
    Object* whiteKing = findObj(*active, "piece-white-king-4-0");
    Object* whiteRook0 = findObj(*active, "piece-white-rook-0-0");
    Object* blackRook7 = findObj(*active, "piece-black-rook-7-7");
    Object* whitePawn = findObj(*active, "piece-white-pawn-4-1");
    Object* btnKnight = findObj(*active, "hud.chess.promo.knight");
    assert(state && whiteKing && whiteRook0 && blackRook7 && whitePawn && btnKnight);

    // Position whitePawn at (4, 7) as if promoted
    whitePawn->setDynamicProperty("gridX", PropertyValue(4));
    whitePawn->setDynamicProperty("gridY", PropertyValue(7));
    state->setDynamicProperty("targetX", PropertyValue(4));
    state->setDynamicProperty("targetY", PropertyValue(7));

    // Verify initial roles before promotion selection
    assert(asInt(*whiteKing, "chessRole") == 5 && "White King must start as King (5)");
    assert(asInt(*whiteRook0, "chessRole") == 1 && "White Rook must start as Rook (1)");
    assert(asInt(*blackRook7, "chessRole") == 1 && "Black Rook must start as Rook (1)");

    // Simulate clicking the Knight promotion HUD button
    Core::EventBus::instance().publish(ECA::Event{"object-clicked", btnKnight, nullptr, std::time(nullptr)});
    harness.lawManager.tick();

    // Verify that the target piece at (4,7) promoted to Knight (2)
    assert(asInt(*whitePawn, "chessRole") == 2 && "Target piece at (4,7) must promote to Knight (2)");

    // Verify that non-target pieces on rank 0 and 7 were NOT promoted!
    assert(asInt(*whiteKing, "chessRole") == 5 && "White King on rank 0 must remain King (5)");
    assert(asInt(*whiteRook0, "chessRole") == 1 && "White Rook on rank 0 must remain Rook (1)");
    assert(asInt(*blackRook7, "chessRole") == 1 && "Black Rook on rank 7 must remain Rook (1)");

    std::cout << "  chess_promotion_isolation_test: PASSED!\n";
    return 0;
}
