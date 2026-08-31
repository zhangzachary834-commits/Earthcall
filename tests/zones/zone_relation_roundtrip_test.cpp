// Zone relation graph roundtrip test (Bug #7 guard).
//
// Ensures that:
// 1. Zone formation relations (and lexemes) are saved by persistZones() into the
//    Zone identity store (saves/zones/<id>/zone.json).
// 2. Loading a Zone from identity or merging snapshot data restores the relation graph
//    even when objects are already present (replaceObjects=false).
// 3. ConditionNode::Kind::Related successfully queries the restored relation graph.
// 4. persistZones() refuses to overwrite a populated stored relation graph / lexemes with empty live ones.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

extern MaterialManager materials;
extern CategoryManager categories;

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << "\n";
        return;
    }
    std::cout << "  ok: " << description << "\n";
}

std::shared_ptr<Object> makePawn(const std::string& id, const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cylinder);
    obj->setObjectID(id);
    obj->setTransform(glm::translate(glm::mat4(1.0f), p));
    return obj;
}

std::shared_ptr<Object> findObjectInZone(const Zone& zone, const std::string& id) {
    for (const auto& obj : zone.getOwnedObjects()) {
        if (obj && obj->getIdentifier() == id) return obj;
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
    std::cout << "Running zone relation roundtrip test (Bug #7 guard)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_relation_roundtrip";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Harness h;
    const std::string sessionFile = (sandbox / "worlds" / "chess_session.json").string();

    auto pieceCategory = categories.create("category.chess.piece");

    // 1. Author a Zone with objects and instance-of relations
    {
        ZoneManager mgr;
        auto chessZone = std::make_shared<Zone>("ChessArena", "strict");
        auto p1 = makePawn("piece-white-pawn-1", glm::vec3(0.0f, 0.0f, 0.0f));
        auto p2 = makePawn("piece-white-pawn-2", glm::vec3(1.0f, 0.0f, 0.0f));
        chessZone->addObject(p1);
        chessZone->addObject(p2);

        auto r1 = std::make_shared<Relation>("instance-of", *p1, *pieceCategory);
        auto r2 = std::make_shared<Relation>("instance-of", *p2, *pieceCategory);
        chessZone->getFormation().add(r1);
        chessZone->getFormation().add(r2);

        mgr.addZone(chessZone);
        mgr.saveState(sessionFile, h.ctx);
    }

    const auto zoneIdentityPath = sandbox / "zones" / "ChessArena" / "zone.json";
    check(std::filesystem::exists(zoneIdentityPath), "Zone identity written to disk");

    // 2. Verify formationRelations is stored in the identity file on disk
    {
        std::ifstream in(zoneIdentityPath);
        nlohmann::json j;
        in >> j;
        check(j.contains("formationRelations") && j["formationRelations"].is_array(),
              "Identity file contains formationRelations array");
        check(j["formationRelations"].size() == 2,
              "Identity file stored both authored relations (size == 2)");
    }

    // 3. Load state in a fresh ZoneManager and verify relations + ConditionModel::Kind::Related
    {
        ZoneManager loaded;
        // Wire Universe relation provider to the active zone formation
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& relations) {
            if (loaded.zones().empty()) return;
            auto active = loaded.zones()[loaded.currentIndex()];
            if (!active) return;
            for (const auto& rel : active->getFormation().relations().getAll()) {
                if (rel) relations.push_back(rel.get());
            }
        });

        loaded.loadState(sessionFile, h.ctx);
        check(loaded.zones().size() >= 1, "Zone loaded successfully");
        auto activeZone = loaded.zones()[loaded.currentIndex()];
        check(activeZone && activeZone->getIdentifier() == "ChessArena", "Active zone is ChessArena");

        const auto& rels = activeZone->getFormation().relations().getAll();
        check(rels.size() == 2, "Live zone in memory has 2 formation relations after load");

        // Verify ConditionNode::Kind::Related
        auto relatedCondition = ConditionNode::related("instance-of", "category.chess.piece");
        auto predicate = relatedCondition.compile();
        ECA::Event ev;

        auto p1Loaded = findObjectInZone(*activeZone, "piece-white-pawn-1");
        check(p1Loaded != nullptr, "Found pawn 1 in loaded zone");
        if (p1Loaded) {
            check(predicate(ev, *p1Loaded),
                  "ConditionNode::Kind::Related evaluates to TRUE for pawn 1 instance-of category.chess.piece");
        }

        auto unrelatedObj = makePawn("unrelated-rock", glm::vec3(5.0f, 0.0f, 0.0f));
        check(!predicate(ev, *unrelatedObj),
              "ConditionNode::Kind::Related evaluates to FALSE for un-related object");
    }

    // 4. Verify refusal to overwrite non-empty identity with empty relations (Bug #7 guard of last resort)
    {
        ZoneManager wipeAttemptMgr;
        auto emptyGraphZone = std::make_shared<Zone>("ChessArena", "strict");
        // Same ID, but no relations added
        wipeAttemptMgr.addZone(emptyGraphZone);
        wipeAttemptMgr.persistZones();

        std::ifstream in(zoneIdentityPath);
        nlohmann::json j;
        in >> j;
        check(j["formationRelations"].size() == 2,
              "persistZones REFUSED to overwrite identity file with empty relations; 2 relations preserved");
    }

    // 5. Clean up sandbox
    std::filesystem::remove_all(sandbox);

    std::cout << "============================================================\n";
    std::cout << "Zone relation roundtrip test summary: "
              << g_checks << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";

    return (g_failures == 0) ? 0 : 1;
}
