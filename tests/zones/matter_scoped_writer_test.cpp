// Sol's Invariant 1 (agent intercom "Basic Pixel Changer Zone Identity Bug
// 9-7-26", 2026-09-08): "a split save contains only the same world twice,
// never the whole live registry." The real basic_pixel_changer.ecmatter
// reached 1,441 entities the first time ZoneManager::loadState's "Legacy
// JSON splitter" ran — a plain-JSON World naming exactly one Zone, migrated
// into split .ecform + .ecmatter substrate while dozens of unrelated Zones
// (pulled in by boot-time hydrateFromZoneStore(), or by any earlier load
// this session) were also live in _zones. buildMatterFlatBuffer() dumped
// every one of them into the new sidecar, because it had no notion that the
// World it was minting a matter file FOR only ever named one.
//
// The fix: the legacy splitter now computes the exact set of Zone ids the
// World's own "zones"/"zoneRefs" name, and passes that as buildMatterFlatBuffer's
// new optional scope. This test reproduces the real shape directly: a
// "Bystander" Zone is live (standing in for boot hydration) when a legacy
// World naming only "OnlyZone" is loaded. The freshly-migrated .ecmatter
// must contain OnlyZone's object and must NOT contain Bystander's.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

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

std::shared_ptr<Object> makeObject(const std::string& id) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    obj->setObjectID(id);
    return obj;
}

std::unordered_set<std::string> entityIdsInMatterFile(const std::filesystem::path& matterPath) {
    std::unordered_set<std::string> ids;
    std::ifstream in(matterPath, std::ios::binary);
    if (!in) return ids;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.empty()) return ids;
    flatbuffers::Verifier verifier(bytes.data(), bytes.size());
    if (!Earthcall::Schema::VerifySaveChunkBuffer(verifier)) return ids;
    const auto* chunk = Earthcall::Schema::GetSaveChunk(bytes.data());
    if (!chunk || !chunk->entities()) return ids;
    for (const auto* entity : *chunk->entities()) {
        if (entity && entity->id()) ids.insert(entity->id()->str());
    }
    return ids;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running matter scoped writer (Invariant 1: no whole-registry dump)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_matter_scoped_writer";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    ZoneManager mgr;

    // Stands in for boot-time hydrateFromZoneStore() pulling in every real
    // Zone under saves/zones/ before a Person ever loads a specific World —
    // exactly what made the real basic_pixel_changer.ecmatter's sidecar
    // include objects from Chess, FarLands, SynthesisStudio, and more.
    auto bystander = std::make_shared<Zone>("Bystander", "strict");
    bystander->addObject(makeObject("bystander-object"));
    mgr.addZone(bystander);

    // A legacy, pre-split plain-JSON World naming exactly one Zone.
    nlohmann::json legacyWorld;
    legacyWorld["saveFormat"] = "zone-identity-v1";
    nlohmann::json onlyZone;
    onlyZone["identifier"] = "OnlyZone";
    onlyZone["name"] = "OnlyZone";
    nlohmann::json onlyObject;
    onlyObject["objectID"] = "only-zone-object";
    onlyObject["shapeKind"] = 0;
    onlyZone["world"]["objects"] = nlohmann::json::array({onlyObject});
    legacyWorld["zones"] = nlohmann::json::array({onlyZone});

    const auto legacyPath = sandbox / "worlds" / "legacy_world.json";
    {
        std::ofstream out(legacyPath);
        out << legacyWorld.dump(2);
    }

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    mgr.loadState(legacyPath.string(), ctx);

    const auto matterPath = std::filesystem::path(legacyPath).replace_extension(".ecmatter");
    check(std::filesystem::exists(matterPath),
          "legacy JSON splitter migrated the World to a .ecmatter sidecar");

    const auto ids = entityIdsInMatterFile(matterPath);
    check(ids.count("only-zone-object") > 0,
          "the migrated matter buffer contains the World's own object");
    check(ids.count("bystander-object") == 0,
          "the migrated matter buffer does NOT contain an unrelated live Zone's object — "
          "this is the exact mechanism that grew the real basic_pixel_changer.ecmatter to "
          "1,441 entities");
    check(ids.size() == 1,
          "the migrated matter buffer's membership is exactly the World's own, nothing more");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "matter_scoped_writer_test: FAILED\n";
        return 1;
    }
    std::cout << "matter_scoped_writer_test: ALL OK\n";
    return 0;
}
