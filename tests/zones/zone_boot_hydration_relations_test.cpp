// Boot hydration must not cost a Zone its relation graph (Bug #7, second door).
//
// Every other chess test loads chess_app.json into a fresh process, where the
// Chess zone is not live yet. The RUNNING APP is not like that: Engine::initLogic
// calls ZoneManager::hydrateFromZoneStore() before any world is loaded
// (EngineInit.cpp), so every Zone under saves/zones/ is already live by the time
// a Person clicks Load.
//
// This test uses the shared TestSupport::BootedEngineHarness to perform the app's
// sequence (boot hydration then load), ensuring all tests model true boot order.

#include "support/test_harness.hpp"
#include "Relation/Relation.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

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

// A press frame and a release frame on whatever the ray hits, then one law tick
// — the shape a Person's mouse actually drives.
void click(Singularity::Input::InteractionChannel* interaction, LawManager& laws,
           const std::vector<Object*>& reachable, const glm::vec3& origin,
           const glm::vec3& dir) {
    Singularity::Input::InteractionChannel::Sense sense;
    sense.rayOrigin = origin;
    sense.rayDirection = dir;
    sense.left = true;
    interaction->observe(sense, reachable);
    sense.left = false;
    interaction->observe(sense, reachable);
    laws.tick();
}

int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  ok   " : "  FAIL ") << what << "\n";
    if (!ok) ++failures;
}

} // namespace

int main() {
    std::string filename = "saves/worlds/chess_app.json";
    if (!std::filesystem::exists(filename) &&
        std::filesystem::exists("../saves/worlds/chess_app.json")) {
        filename = "../saves/worlds/chess_app.json";
    }
    {
        const auto p = std::filesystem::absolute(filename);
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }

    std::cout << "=== Boot hydration then load — the running app's order ===\n";

    // BootedEngineHarness runs hydrateFromZoneStore() in its constructor
    // matching Engine::initLogic boot sequence.
    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    assert(!harness.zones.zones().empty());
    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);
    check(active->getIdentifier() == "Chess",
          "active zone is Chess (got '" + active->getIdentifier() + "')");

    // The relation graph must have survived — bound to beings, not left as
    // name-strings on a refused edge.
    std::size_t total = 0, instanceOf = 0, bound = 0;
    for (const auto& rel : active->formation().relations().getAll()) {
        if (!rel) continue;
        ++total;
        if (rel->type != "instance-of") continue;
        ++instanceOf;
        if (rel->a() && rel->b()) ++bound;
    }
    std::cout << "  formation relations: " << total << " total, " << instanceOf
              << " instance-of, " << bound << " with both endpoints bound\n";
    check(total >= 38, "the Chess formation kept its relation graph");
    check(instanceOf == 35 && bound == instanceOf,
          "every instance-of edge is bound to real beings");

    Universe::instance().setClock(0.0, 1.0 / 60.0);
    std::vector<Object*> reachable;
    for (const auto& obj : active->getOwnedObjects()) if (obj) reachable.push_back(obj.get());

    Object* pawn = findObj(*active, "piece-white-pawn-4-1");
    check(pawn != nullptr, "the e2 pawn is in the world");
    if (!pawn) return 1;

    // Straight down onto the e2 pawn, then onto e4 — a real press/release each,
    // through InteractionChannel::observe(), not a hand-published event.
    click(harness.interaction, harness.lawManager, reachable,
          pawn->getPosition() + glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    check(asBool(*pawn, "isSelected"),
          "clicking the pawn selects it (law-chess-click must not answer "
          "CONDITIONS FAILED on a piece)");

    click(harness.interaction, harness.lawManager, reachable,
          glm::vec3(0.5f, 5.0f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f));
    const int gx = asInt(*pawn, "gridX"), gy = asInt(*pawn, "gridY");
    std::cout << "  pawn is at (" << gx << "," << gy << ")\n";
    check(gx == 4 && gy == 3, "clicking e4 walks the pawn e2-e4");

    std::cout << "==================================================\n";
    std::cout << (failures == 0 ? "zone_boot_hydration_relations_test: ALL OK\n"
                                : "zone_boot_hydration_relations_test: FAILURES\n");
    std::cout << "==================================================\n";
    return failures == 0 ? 0 : 1;
}
