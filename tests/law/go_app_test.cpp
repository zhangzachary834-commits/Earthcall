// Probe: does saves/worlds/go_app.json load as a complete Go board game world
// in Earthcall — a wooden Goban prism with 19x19 FaceTexture grid, 361 intersections,
// stone bowls, player seats, and go state tracking.
//
// Witness Documentation:
// OLD TEST PROVED:
//   Static loading of the go_app.json save file: verifies board prism, 19x19
//   intersections (361 objects), bowls, seats, and initial turn string.
// OLD TEST COULD NOT PROVE:
//   Real runtime gameplay interaction: whether clicking an intersection via
//   InteractionChannel/EventBus actually triggers authored laws (law-go-click,
//   law-go-place-black, law-go-place-white), places a stone on the board,
//   updates intersection state (is_empty = false, stone_color), and advances
//   turn state between Black and White.

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

std::string asString(Singular& being, const char* name) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return "";
    if (const std::string* s = std::get_if<std::string>(&v)) return *s;
    return "";
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
    std::string filename = (argc > 1) ? argv[1] : "saves/worlds/go_app.json";
    if (argc <= 1 && !std::filesystem::exists(filename)) {
        if (std::filesystem::exists("../saves/worlds/go_app.json"))
            filename = "../saves/worlds/go_app.json";
    }
    // This test pointed SaveSystem straight at the real saves/ tree with no
    // backup/restore, the same unguarded shape found in
    // zone_boot_hydration_relations_test 2026-09-09 (it had corrupted the
    // real saves/zones/Chess/zone.json — 4,076 lines grew to 8,792 — from
    // being run directly, repeatedly, outside ctest). Guarding this one too
    // rather than waiting to find it the same way.
    TestSupport::RealSaveTreeGuard saveGuard(filename);
    std::cout << "--- go_app_probe: " << filename << " ---\n";

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    std::cout << "loaded. zones=" << harness.zones.zones().size()
              << " current=" << harness.zones.currentIndex() << "\n";
    assert(!harness.zones.zones().empty());
    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    Object* board = findObj(*active, "object.go.board");
    Object* state = findCat("go_state");
    if (!state) state = findObj(*active, "go_state");
    Object* author = findCat("grok-4.6");

    assert(board && "one board prism");
    assert(state && "go_state being");
    assert(author && "grok-4.6 first-mover identity");

    // Board geometry
    assert(board->getShapeKind() == Object::ShapeKind::Cube);
    assert(asBool(*board, "isBoard"));

    // Intersections: exactly 361 (19x19)
    int ixCount = 0;
    for (int x = 0; x < 19; ++x) {
        for (int y = 0; y < 19; ++y) {
            std::string ixId = "intersection_" + std::to_string(x) + "_" + std::to_string(y);
            Object* ix = findObj(*active, ixId);
            assert(ix && "intersection exists");
            assert(asBool(*ix, "is_empty") && "intersection starts empty");
            assert(asInt(*ix, "gridX") == x && "gridX matches");
            assert(asInt(*ix, "gridY") == y && "gridY matches");
            ixCount++;
        }
    }
    assert(ixCount == 361 && "all 19x19 intersections present");
    std::cout << "  one board, 361 intersections verified\n";

    // Check state initial turn
    assert(asString(*state, "current_turn") == "black");
    std::cout << "  initial turn: black\n";

    // Check bowls and seats
    Object* blackBowl = findObj(*active, "object.go.bowl.black");
    Object* whiteBowl = findObj(*active, "object.go.bowl.white");
    Object* blackSeat = findObj(*active, "object.go.seat.black");
    Object* whiteSeat = findObj(*active, "object.go.seat.white");

    assert(blackBowl && "black bowl exists");
    assert(whiteBowl && "white bowl exists");
    assert(blackSeat && "black player seat exists");
    assert(whiteSeat && "white player seat exists");
    std::cout << "  bowls and seats verified\n";

    // Live-path interaction test: exercise clicking intersections to place Black & White stones.
    std::cout << "--- Testing real gameplay path: clicking intersections ---\n";
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    Object* tengen = findObj(*active, "intersection_9_9");
    assert(tengen && "Tengen intersection (9, 9) exists");

    // 1. Black places a stone at Tengen (9, 9)
    click(harness.interaction, harness.lawManager, tengen, 0.0f, 0.005f, 0.0f);
    std::cout << "  after Tengen click: is_empty=" << asBool(*tengen, "is_empty")
              << " stone_color='" << asString(*tengen, "stone_color") << "'"
              << " turn='" << asString(*state, "current_turn") << "'\n";

    assert(!asBool(*tengen, "is_empty") && "Tengen intersection is no longer empty");
    assert(asString(*tengen, "stone_color") == "black" && "Tengen has black stone");
    assert(asString(*state, "current_turn") == "white" && "turn advanced to white");

    // 2. White places a stone at (10, 10)
    Object* ix10_10 = findObj(*active, "intersection_10_10");
    assert(ix10_10 && "intersection (10, 10) exists");

    click(harness.interaction, harness.lawManager, ix10_10, 0.16f, 0.005f, 0.16f);
    std::cout << "  after (10,10) click: is_empty=" << asBool(*ix10_10, "is_empty")
              << " stone_color='" << asString(*ix10_10, "stone_color") << "'"
              << " turn='" << asString(*state, "current_turn") << "'\n";

    assert(!asBool(*ix10_10, "is_empty") && "intersection (10,10) is no longer empty");
    assert(asString(*ix10_10, "stone_color") == "white" && "intersection (10,10) has white stone");
    assert(asString(*state, "current_turn") == "black" && "turn advanced back to black");

    std::cout << "go_app_test: ALL OK\n";
    return 0;
}
