// Probe: does saves/worlds/go_app.json load as a complete Go board game world
// in Earthcall — a wooden Goban prism with 19x19 FaceTexture grid, 361 intersections,
// stone bowls, player seats, and go state tracking.

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

} // namespace

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "saves/worlds/go_app.json";
    if (argc <= 1 && !std::filesystem::exists(filename)) {
        if (std::filesystem::exists("../saves/worlds/go_app.json"))
            filename = "../saves/worlds/go_app.json";
    }
    {
        const auto p = std::filesystem::absolute(filename);
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }
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

    std::cout << "go_app_test: ALL OK\n";
    return 0;
}
