// Test: the Shape Generator 3D law AS THE ENGINE INSTANTIATES IT.
//
// basic_cube_law_test already covers a hand-built law of the same shape, and
// it passed green for a day while the law the engine actually booted could not
// fire at all: Engine::initLogic built its own copy inline, unauthored (so
// Law::applyTo refused it with Unauthored) and conditioned on a "type"
// property the CreationChannel does not carry (so the condition was never
// satisfiable either). A test that builds its own law cannot catch that. This
// one calls the same factory boot calls.
//
// Covers:
//   1. The law is authored -- the structural gate, not a convention.
//   2. It refuses unless spawnLawArmed: console Create is a different latch.
//      A click with active3DMode == "Create" and the law down births nothing.
//   3. It spawns when spawnLawArmed, taking the live shape kind and colour,
//      even if the console is not in Create.
//   4. Each newborn gets its OWN identifier. Concept members are named by
//      slot, and a slot is not an identity.
//   5. Being a first mover, it stays out of world saves and survives a load.

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    std::printf("Running Shape Generator 3D law (as booted) test...\n");

    Zone world("test-zone", "default");
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Singularity::Core::CreationChannel channel;

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&world);
        beings.push_back(&player);
        beings.push_back(&channel);
        for (const auto& obj : world.getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
    });

    LawManager lawManager;
    lawManager.connectToEventBus();

    // Exactly what Engine::initLogic does.
    auto law = Singularity::Core::createShapeGenerator3DLaw(player);
    lawManager.add(law);
    lawManager.bindTrigger(law->getIdentifier(), "onMouseClicked");

    const auto click = [&]() {
        Core::EventBus::instance().publish(
            ECA::Event{"onMouseClicked", &channel, nullptr, 0});
        lawManager.tick();
    };

    // ---- 1. authored -------------------------------------------------------
    check(law->isAuthored(),
          "the booted law is authored (Law::applyTo refuses Unauthored laws)");
    check(law->getIdentifier() == "shape-generator-3d-law",
          "stable identifier law text can name");
    check(ConceptRegistry::instance().find("concept-shape-3d") != nullptr,
          "concept-shape-3d is registered by the factory");

    // ---- 2. the condition is spawnLawArmed, not console Create --------------
    check(!law->conditionsSatisfied(channel),
          "condition is unsatisfied while the spawn law is down");

    channel.active3DMode = "Create";
    channel.spawnLawArmed = false;
    click();
    check(world.getOwnedObjects().empty(),
          "console Create alone does not fire the spawn law");

    channel.active3DMode = "Select";
    click();
    check(world.getOwnedObjects().empty(),
          "a click with the law down spawns nothing");

    // A property that does not exist must not read as a satisfied condition.
    // (The shipped condition compared a "type" path the channel never had.)
    PropertyValue unused;
    check(PropertyPath::parse("type").getValue(channel, unused) !=
              PropertyPath::PathResult::Ok,
          "the CreationChannel carries no 'type' property");

    // ---- 3. it spawns when armed, even outside console Create --------------
    channel.active3DMode = "Select";
    channel.spawnLawArmed = true;
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Sphere);
    channel.activeColor = glm::vec3(0.25f, 0.5f, 0.75f);
    channel.placementMode = "InFront";
    channel.cursorSpawnPos = glm::vec3(3.0f, 1.0f, -2.0f);

    check(law->conditionsSatisfied(channel),
          "condition is satisfied once spawnLawArmed, without console Create");

    click();
    check(world.getOwnedObjects().size() == 1, "an armed click births one Object");

    if (!world.getOwnedObjects().empty()) {
        const Object* born = world.getOwnedObjects().back().get();
        check(born->getShapeKind() == Object::ShapeKind::Sphere,
              "the newborn takes the channel's live shape kind, not the template's");
        check(nearf(born->getPosition().x, 3.0f) &&
                  nearf(born->getPosition().y, 1.0f) &&
                  nearf(born->getPosition().z, -2.0f),
              "the newborn is placed at cursorSpawnTransform");
    }

    // ---- 3b. the placement the law reads must be LIVE ----------------------
    // cursorSpawnTransform only composes a cached cursorSpawnPos. That cache
    // used to be written from inside Tool::ShapeGenerator3D, BELOW the
    // 2026-08-17 mutual-exclusion guard -- so arming the law made the only
    // writer return early, the law read a never-written default, and every
    // cube was born at the origin while the record said Applied. Spawn refuses
    // an unreadable placement; it cannot refuse a readable wrong one.
    //
    // Sensing is now Tool::UpdateShapeGeneratorPlacement, run every frame for
    // both paths. What this test can hold headlessly is the consequence: given
    // live placement, the law puts the newborn where the placement says.
    {
        const std::size_t before = world.getOwnedObjects().size();
        channel.placementMode = "InFront";
        channel.inFrontDistance = 2.0f;
        channel.gridSnap = false;
        const glm::vec3 camPos(0.0f, 1.6f, 3.0f);
        const glm::vec3 camFwd(0.0f, 0.0f, -1.0f);
        channel.updatePlacement(camPos, camFwd);          // what sensing does
        check(!nearf(channel.cursorSpawnPos.z, 0.0f) || !nearf(channel.cursorSpawnPos.y, 0.0f),
              "updatePlacement moves the cache off its default");
        click();
        check(world.getOwnedObjects().size() == before + 1, "the armed click still births one");
        if (world.getOwnedObjects().size() > before) {
            const glm::vec3 at = world.getOwnedObjects().back()->getPosition();
            check(nearf(at.x, 0.0f) && nearf(at.y, 1.6f) && nearf(at.z, 1.0f),
                  "the newborn lands at cameraPos + forward * distance, NOT the origin");
            const bool atOrigin = nearf(at.x, 0.0f) && nearf(at.y, 0.0f) && nearf(at.z, 0.0f);
            check(!atOrigin, "the newborn is not at the origin");
        }
    }

    // ---- 4. every birth is its own being -----------------------------------
    // Counted relatively, not against a literal, so inserting a case above does
    // not silently move this one's goalposts.
    {
        const std::size_t before = world.getOwnedObjects().size();
        click();
        click();
        check(world.getOwnedObjects().size() == before + 2, "two more clicks, two more Objects");
    }

    std::set<std::string> ids;
    for (const auto& obj : world.getOwnedObjects()) {
        if (obj) ids.insert(obj->getIdentifier());
    }
    check(ids.size() == world.getOwnedObjects().size(),
          "every newborn has its own identifier");
    for (const auto& id : ids) {
        check(id.find("concept-shape-3d") == 0,
              "newborn identifier names its concept: " + id);
    }

    // A reloaded world must not let the counter reissue a live identity.
    {
        const std::string highest = *ids.rbegin();
        ObjectIdentity::claimIdentifierAtLeast(highest);
        auto concept = ConceptRegistry::instance().find("concept-shape-3d");
        auto newborns = concept->instantiate(
            glm::mat4(1.0f), static_cast<const std::vector<Singular*>*>(nullptr));
        bool collided = false;
        for (const auto& n : newborns) {
            if (n && ids.count(n->getIdentifier())) collided = true;
        }
        check(!collided,
              "claimIdentifierAtLeast advances past a restored concept-member id");
    }

    // ---- 5. first-mover serialization --------------------------------------
    {
        const nlohmann::json saved = lawManager.toJson();
        bool serialized = false;
        for (const auto& lj : saved["laws"]) {
            if (lj.value("id", std::string()) == "shape-generator-3d-law") serialized = true;
        }
        check(!serialized, "a first mover is not written into the world save");

        lawManager.loadFromJson(saved);
        check(lawManager.find("shape-generator-3d-law") != nullptr,
              "the first mover survives a load");
    }

    std::printf(g_failures == 0 ? "shape_generator_law_test: ALL OK\n"
                                : "shape_generator_law_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
