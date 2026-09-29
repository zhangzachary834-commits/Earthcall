// Zone identity store — the click-through Zach named on the agenda.
//
// Creating beings in Home, saving session A, loading session B, and walking
// back to Home used to show an empty room: every "world" file carried its own
// copy of every Zone, and loadState replaced the lot. EarthcallOurverse.md
// says each Person has at least one Singularity-fixed Home; a session file
// may name, fork, and evolve that Home, not mint a new one.
//
// This test drives ZoneManager::saveState / loadState / forkZone / diffZones
// — the same offices Save As / Load call.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

std::shared_ptr<Object> makeCube(const std::string& id, const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    obj->setObjectID(id);
    obj->setTransform(glm::translate(glm::mat4(1.0f), p));
    return obj;
}

bool hasObject(const ZoneManager& mgr, const std::string& id) {
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return true;
        }
    }
    return false;
}

std::shared_ptr<Zone> findZone(ZoneManager& mgr, const std::string& id) {
    for (auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == id) return z;
    }
    return nullptr;
}

struct Harness {
    Soul soul;
    Body body;
    Person player;
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;

    Harness()
        : soul("Player"),
          body("humanoid", "default"),
          player(std::move(soul), std::move(body), "default") {
        ctx.camera = &camera;
        ctx.mouseHandler = &mouse;
        ctx.currentColor = color;
        ctx.person = &player;
        ctx.lawManager = &laws;
        ctx.worldTime = &worldTime;
    }
};

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone identity (Home persists across session files)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Harness h;
    const std::string worldA = (sandbox / "worlds" / "session_a.ecform").string();
    const std::string worldB = (sandbox / "worlds" / "session_b.ecform").string();

    {
        ZoneManager mgr;
        auto home = std::make_shared<Zone>("Home", "strict");
        home->setOwner(h.player.getIdentifier());
        home->setQuality("kind", "home");
        home->addObject(makeCube("home-cube", glm::vec3(1.0f, 0.0f, 0.0f)));
        mgr.addZone(home);
        auto workshop = std::make_shared<Zone>("Workshop", "strict");
        workshop->setOwner(h.player.getIdentifier());
        workshop->addObject(makeCube("workshop-cube", glm::vec3(4.0f, 0.0f, 0.0f)));
        mgr.addZone(workshop);
        h.worldTime = 11.0;
        mgr.saveState(worldA, h.ctx);
        mgr.saveStateWithLog("session_a", h.ctx);
    }

    const auto homePath = sandbox / "homes" / "Home" / "home.json";
    const auto workshopPath = sandbox / "zones" / "Workshop" / "zone.json";
    check(std::filesystem::exists(homePath),
          "Save writes Home to saves/homes/Home/home.json");
    check(std::filesystem::exists(workshopPath),
          "Save writes Workshop to its own directory, not only the session file");
    {
        std::ifstream in(worldA);
        nlohmann::json j;
        in >> j;
        check(j.value("saveFormat", std::string{}) == "zone-identity-v1",
              "session file names the zone-identity format");
        check(j.contains("zoneRefs") && j["zoneRefs"].is_array() && j["zoneRefs"].size() >= 2,
              "session file lists zoneRefs rather than owning the Zones");
        check(j.contains("zones") && j["zones"].is_array(),
              "session file dual-writes zones[] so existing files still load");
    }

    {
        ZoneManager other;
        auto home = std::make_shared<Zone>("Home", "strict");
        home->setOwner(h.player.getIdentifier());
        home->setQuality("kind", "home");
        other.addZone(home);
        auto plaza = std::make_shared<Zone>("Plaza", "strict");
        plaza->addObject(makeCube("plaza-cube", glm::vec3(8.0f, 0.0f, 0.0f)));
        other.addZone(plaza);
        h.worldTime = 22.0;
        other.saveState(worldB, h.ctx);
    }

    check(std::filesystem::exists(homePath), "saving session B does not delete Home's identity file");
    {
        std::ifstream in(homePath);
        nlohmann::json hj;
        in >> hj;
        bool stillHomeCube = false;
        if (hj.contains("world") && hj["world"].contains("objects")) {
            for (const auto& o : hj["world"]["objects"]) {
                if (o.value("objectID", "") == "home-cube") stillHomeCube = true;
            }
        }
        check(stillHomeCube,
              "saving a session whose live Home is empty does not wipe the stored Home");
    }

    {
        ZoneManager loaded;
        loaded.loadState(worldA, h.ctx);
        check(hasObject(loaded, "home-cube"), "loading session A restores Home's cube from the Zone store");
        check(hasObject(loaded, "workshop-cube"), "Workshop is its own identity, restored with A");
        loaded.loadState(worldB, h.ctx);
        check(hasObject(loaded, "home-cube"),
              "loading session B does not replace Home — the cube authored in A is still there");
        check(hasObject(loaded, "plaza-cube"), "session B's Plaza is admitted alongside Home");
        check(hasObject(loaded, "workshop-cube"),
              "a Person's other Zone (Workshop) is not dropped when another session loads");
        auto home = findZone(loaded, "Home");
        check(home && home->getOwnedObjects().size() >= 1, "Home is the same Zone, not a new empty one");
        check(h.worldTime == 22.0, "session pose (worldTime) still comes from the loaded session file");
        loaded.switchTo(0);
        for (size_t i = 0; i < loaded.zones().size(); ++i) {
            if (loaded.zones()[i] && loaded.zones()[i]->getIdentifier() == "Home") {
                loaded.switchTo(i);
                break;
            }
        }
        check(loaded.active().getIdentifier() == "Home", "switched into Home after loading session B");
        check(hasObject(loaded, "home-cube"),
              "walking into Home after loading the other file still shows what was created there");
    }

    {
        ZoneManager fresh;
        fresh.hydrateFromZoneStore();
        check(hasObject(fresh, "home-cube"),
              "a new manager hydrating from the Zone store sees Home without loading a session");
        check(hasObject(fresh, "workshop-cube"), "hydrate admits every stored Zone, not only Home");
    }

    {
        ZoneManager mgr;
        mgr.hydrateFromZoneStore();
        check(mgr.forkZone("Home", "Home.garden"), "forkZone copies Home under a new name");
        check(SaveSystem::homeIdentityExists("Home.garden"),
              "the fork has its own directory in the Home store");
        auto garden = findZone(mgr, "Home.garden");
        check(garden && garden->getQualities().count("forkedFrom") &&
                  garden->getQualities().at("forkedFrom") == "Home",
              "the fork records forkedFrom=Home (named, not a silent twin)");
        check(hasObject(mgr, "home-cube"), "the original Home is still here after the fork");
        nlohmann::json same = mgr.diffZones("Home", "Home.garden");
        check(same.contains("shared") && same["shared"].size() >= 1,
              "diff of a fresh fork reports the cube as shared");
        check(same["onlyInA"].empty() && same["onlyInB"].empty(),
              "a fresh fork has no object-id divergence yet");
        garden->addObject(makeCube("garden-bench", glm::vec3(2.0f, 0.0f, 0.0f)));
        mgr.persistZones();
        nlohmann::json diverged = mgr.diffZones("Home", "Home.garden");
        bool benchOnlyInB = false;
        for (const auto& id : diverged["onlyInB"]) {
            if (id.get<std::string>() == "garden-bench") benchOnlyInB = true;
        }
        check(benchOnlyInB, "after evolving the fork, diff names garden-bench as onlyInB");
        bool cubeShared = false;
        for (const auto& id : diverged["shared"]) {
            if (id.get<std::string>() == "home-cube") cubeShared = true;
        }
        check(cubeShared, "the original Home cube remains shared after the fork evolved");
    }

    {
        // Bug (2026-09-07): a Zone's identity-store snapshot is a moment in
        // time, not a diff. The store used to win on the WHOLE object, so a
        // field the World authored AFTER that snapshot was silently
        // discarded in favour of the object's raw C++ default instead of
        // the World's authored value. Fixed in ZoneSerialization.cpp
        // (mergeZoneObjectsFromJson): the merge is per object, per field,
        // not per whole object.
        //
        // `attributes` stands in for the missing field here (conditionally
        // omitted from to_json when empty, so a fresh object genuinely has
        // none to round-trip) — the original repro field, faceColors, is
        // now unconditionally serialized (see to_json's own fix) and so can
        // no longer come back empty from a live round trip; that fix is
        // covered separately below by an actual store round trip.
        ZoneManager mgr;

        nlohmann::json staleWidget;
        staleWidget["objectID"] = "widget";
        staleWidget["shapeKind"] = 0;
        staleWidget["authoredProperties"]["displayName"] = {{"t", "string"}, {"v", "Store Name"}};
        nlohmann::json staleIdentity;
        staleIdentity["identifier"] = "Sandbox";
        staleIdentity["name"] = "Sandbox";
        staleIdentity["world"]["objects"] = nlohmann::json::array({staleWidget});
        SaveSystem::writeZoneIdentity("Sandbox", staleIdentity);

        // The World, authored later: the same widget now carries an
        // attribute the snapshot above never saw, plus a brand-new sibling
        // object the store has never heard of.
        nlohmann::json worldWidget;
        worldWidget["objectID"] = "widget";
        worldWidget["shapeKind"] = 0;
        worldWidget["attributes"] = {{"mood", "cheerful"}};
        worldWidget["authoredProperties"]["displayName"] = {{"t", "string"}, {"v", "World Name"}};
        nlohmann::json newSibling;
        newSibling["objectID"] = "new-sibling";
        newSibling["shapeKind"] = 0;

        nlohmann::json worldZone;
        worldZone["identifier"] = "Sandbox";
        worldZone["name"] = "Sandbox";
        worldZone["world"]["objects"] = nlohmann::json::array({worldWidget, newSibling});
        nlohmann::json worldJson;
        worldJson["zones"] = nlohmann::json::array({worldZone});

        const auto mergePath = sandbox / "worlds" / "merge_test.json";
        {
            std::ofstream out(mergePath);
            out << worldJson.dump(2);
        }

        mgr.loadState(mergePath.string(), h.ctx);

        std::shared_ptr<Object> widget;
        for (auto& z : mgr.zones()) {
            if (!z) continue;
            for (auto& o : z->getOwnedObjects()) {
                if (o && o->getIdentifier() == "widget") widget = o;
            }
        }
        check(widget != nullptr, "the stale-snapshot widget still loads");
        if (widget) {
            check(widget->hasAttribute("mood") && widget->getAttribute("mood") == "cheerful",
                  "a field the store never captured (attributes) falls through to the "
                  "World's authored value instead of vanishing");
            Property* nameProp = widget->findProperty("displayName");
            check(nameProp && std::get<std::string>(nameProp->value()) == "Store Name",
                  "a field the store DOES capture (displayName) still wins over the "
                  "World's — a Person's in-session naming is not overwritten");
        }
        check(hasObject(mgr, "new-sibling"),
              "an object the World added after the snapshot is admitted, not dropped");
    }

    {
        // Bug (2026-09-07, second pass): the block above covers a Zone that
        // is NOT live yet, but that is not the path a real Person hits.
        // EngineInit's boot-time hydrateFromZoneStore() makes essentially
        // every previously-saved Zone live BEFORE a Person ever clicks Load
        // (see the comment at ZoneManager.cpp's findLive branch), so the
        // merge fix above never actually fired for the Basic Pixel Changer
        // canvas Zach was testing — it hit findLive instead.
        //
        // A merge was tried there too and reverted: re-running from_json on
        // an object that has been live for any length of time is not safe
        // (it corrupted Chess piece selection state in
        // chess_click_geometry_test — see ZoneSerialization.cpp's comment).
        // The actual fix is upstream of any merge: Object::to_json now
        // serializes faceColors, so the identity store captures the right
        // value on its very first write and boot hydration alone reproduces
        // it correctly — no runtime merge needed on the findLive path at all.
        //
        // This proves that: write a Zone's identity the same way a real
        // save does (zoneToJson, not hand-built JSON), hydrate a FRESH
        // ZoneManager from the store exactly as EngineInit.cpp does at
        // boot, and check the authored colour survived.
        ZoneManager writer;
        auto zone = std::make_shared<Zone>("ColorSandbox", "strict");
        auto plate = std::make_shared<Object>("color-plate");
        plate->setFaceColor(0, 0.2f, 0.6f, 0.9f);
        zone->addObject(plate);
        writer.addZone(zone);
        writer.persistZones();

        ZoneManager fresh;
        fresh.hydrateFromZoneStore();
        Object* loadedPlate = nullptr;
        for (auto& z : fresh.zones()) {
            if (!z) continue;
            for (auto& o : z->getOwnedObjects()) {
                if (o && o->getIdentifier() == "color-plate") loadedPlate = o.get();
            }
        }
        check(loadedPlate != nullptr, "the plate survives a store round trip");
        if (loadedPlate) {
            check(std::fabs(loadedPlate->faceColors[0][0] - 0.2f) < 1e-4f &&
                      std::fabs(loadedPlate->faceColors[0][1] - 0.6f) < 1e-4f &&
                      std::fabs(loadedPlate->faceColors[0][2] - 0.9f) < 1e-4f,
                  "faceColors now round-trips through the identity store (was silently "
                  "regressing to the legacy cube-red default — the actual Basic Pixel "
                  "Changer bug) because Object::to_json serializes it again");
        }
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_identity_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_identity_test: ALL OK\n";
    return 0;
}
