// Boot hydration must not cost a Zone its relation graph (Bug #7, second door).
//
// Every other chess test loads chess_app.json into a fresh process, where the
// Chess zone is not live yet. The RUNNING APP is not like that: Engine::initLogic
// calls ZoneManager::hydrateFromZoneStore() before any world is loaded
// (EngineInit.cpp), so every Zone under saves/zones/ is already live by the time
// a Person clicks Load.
//
// Two things went wrong in that order, and only in that order:
//
//   1. Categories are WORLD data — they load in ZoneManager::loadState, not at
//      boot. So when hydration binds the Chess zone's formation relations,
//      "category.chess.piece" does not exist yet, every instance-of edge comes
//      back with an unbound endpoint, and Formation::add REFUSES it.
//   2. loadState's admitFromJson then found the Chess zone LIVE and returned
//      without merging the session's zone JSON at all, so nothing ever tried
//      again.
//
// The Chess zone ran with zero relations. "instance-of category.chess.piece" was
// false for every piece, so law-chess-click answered CONDITIONS FAILED on a pawn
// while still succeeding on the board — whose test is the isBoard PROPERTY, not
// a relation. To a Person: clicking a piece does nothing, forever.
//
// This test performs the app's sequence, not the test suite's.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

extern MaterialManager materials;
extern CategoryManager categories;

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

    Core::Camera camera;
    MouseHandler mouseHandler;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    LawManager lawManager;
    lawManager.connectToEventBus();

    ZoneManager zones;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        if (zones.zones().empty()) return;
        auto active = zones.zones()[zones.currentIndex()];
        if (!active) return;
        beings.push_back(active.get());
        for (const auto& obj : active->getOwnedObjects()) if (obj) beings.push_back(obj.get());
        for (const auto& law : lawManager.getAll()) if (law) beings.push_back(law.get());
        for (const auto& material : materials.getAll()) if (material) beings.push_back(material.get());
        for (const auto& category : categories.getAll()) if (category) beings.push_back(category.get());
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

    Singularity::Input::InteractionChannel::syncRegister(lawManager);
    auto* interaction = Singularity::Input::InteractionChannel::find(lawManager);
    assert(interaction);
    interaction->setEnabled(true);

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

    std::cout << "=== Boot hydration then load — the running app's order ===\n";

    // THE POINT OF THIS TEST. Engine::initLogic does this before any world is
    // loaded; no other chess test does, and that is why none of them could see
    // the bug this guards.
    zones.hydrateFromZoneStore();

    zones.loadState(filename, ctx);
    assert(!zones.zones().empty());
    auto active = zones.zones()[zones.currentIndex()];
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
    click(interaction, lawManager, reachable,
          pawn->getPosition() + glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    check(asBool(*pawn, "isSelected"),
          "clicking the pawn selects it (law-chess-click must not answer "
          "CONDITIONS FAILED on a piece)");

    click(interaction, lawManager, reachable,
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
