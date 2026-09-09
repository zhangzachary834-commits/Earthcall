// Sol's Invariant 4 (agent intercom "Basic Pixel Changer Zone Identity Bug
// 9-7-26", 2026-09-08/09): "generations are atomic." The semantic root
// (.ecform) and physical substrate (.ecmatter) written by saveState /
// saveStateWithLog now share an opaque, content-addressed snapshot id: the
// root records that id plus the matter chunk's sha256, byte length, and
// schema version under "matterGeneration"; the matter file for that id is
// written+flushed BEFORE the root's own atomic-rename commit; the previous
// generation's file is kept until the new root commits, and removed only
// after. On load, a root that names a generation is refused (matter not
// hydrated, no crash, nothing else mutated) if the named file is missing,
// truncated, or hashes to something other than what the root recorded — it
// never silently falls back to rebuilding a fresh matter file for a root
// that explicitly named a generation, since that would paper over exactly
// the kind of silent corruption this invariant exists to catch.
//
// This test drives the real ZoneManager::saveState / loadState paths
// against a sandboxed save root (SaveSystem::setSaveRoot) — no repo save
// file is touched.

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

#include <glm/glm.hpp>
#include <openssl/sha.h>

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

std::string sha256Hex(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    static const char hexDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(SHA256_DIGEST_LENGTH * 2);
    for (unsigned char byte : hash) {
        out.push_back(hexDigits[(byte >> 4) & 0x0F]);
        out.push_back(hexDigits[byte & 0x0F]);
    }
    return out;
}

std::vector<uint8_t> readBytes(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

nlohmann::json readEcform(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in) return nlohmann::json{};
    return nlohmann::json::parse(in, nullptr, false);
}

void writeEcform(const std::filesystem::path& p, const nlohmann::json& j) {
    std::ofstream out(p);
    out << j.dump(2);
}

glm::vec3 translationOf(const glm::mat4& m) { return glm::vec3(m[3][0], m[3][1], m[3][2]); }

SaveContext makeCtx(Core::Camera& camera, MouseHandler& mouse, LawManager& laws,
                     Person& player, float* color, double* worldTime) {
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = worldTime;
    return ctx;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running matter generation commit (Invariant 4: atomic generations)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_matter_generation_commit";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx = makeCtx(camera, mouse, laws, player, color, &worldTime);

    const auto ecformPath = sandbox / "worlds" / "gen_test.ecform";
    const auto legacyFixedMatterPath = sandbox / "worlds" / "gen_test.ecmatter";

    // ---- Round 1: fresh save, verify generation metadata + file. ----
    ZoneManager mgr1;
    auto zone1 = std::make_shared<Zone>("GenZone", "strict");
    auto obj1 = std::make_shared<Object>();
    obj1->setShape(Object::ShapeKind::Cube);
    obj1->setObjectID("gen-object");
    obj1->setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    zone1->addObject(obj1);
    mgr1.addZone(zone1);

    mgr1.saveState(ecformPath.string(), ctx);

    nlohmann::json rootV1 = readEcform(ecformPath);
    check(!rootV1.is_discarded() && rootV1.contains("matterGeneration"),
          "saveState's .ecform names a matterGeneration");
    const std::string genId1 = rootV1.value("matterGeneration", nlohmann::json{}).value("snapshotId", std::string{});
    const std::string hash1 = rootV1["matterGeneration"].value("sha256", std::string{});
    const std::size_t len1 = rootV1["matterGeneration"].value("byteLength", std::size_t{0});
    check(!genId1.empty() && !hash1.empty() && len1 > 0,
          "matterGeneration carries a non-empty snapshotId, sha256, and byteLength");

    const auto genPath1 = sandbox / "worlds" / ("gen_test." + genId1 + ".ecmatter");
    check(std::filesystem::exists(genPath1), "the generation-named .ecmatter file exists on disk");
    const auto bytes1 = readBytes(genPath1);
    check(bytes1.size() == len1, "the generation file's size matches the recorded byteLength");
    check(sha256Hex(bytes1) == hash1, "the generation file's sha256 matches what the root recorded");
    check(!std::filesystem::exists(legacyFixedMatterPath),
          "a new generation-coupled save does not also write the legacy fixed-name .ecmatter");

    // ---- Round trip: load into a fresh ZoneManager, transform hydrates. ----
    {
        ZoneManager mgr2;
        mgr2.loadState(ecformPath.string(), ctx);
        Object* found = nullptr;
        for (const auto& z : mgr2.zones()) {
            if (!z) continue;
            for (const auto& o : z->getOwnedObjects()) {
                if (o && o->getObjectID() == "gen-object") { found = o.get(); break; }
            }
        }
        check(found != nullptr, "round-trip load finds the object by id");
        if (found) {
            glm::vec3 t = translationOf(found->getTransform());
            check(std::abs(t.x - 1.0f) < 1e-3f && std::abs(t.y - 2.0f) < 1e-3f && std::abs(t.z - 3.0f) < 1e-3f,
                  "round-trip load hydrates the verified generation's transform (1,2,3)");
        }
    }

    // ---- Round 2: mutate + re-save to the SAME path. New generation, old cleaned up. ----
    obj1->setPosition(glm::vec3(4.0f, 5.0f, 6.0f));
    mgr1.saveState(ecformPath.string(), ctx);
    nlohmann::json rootV2 = readEcform(ecformPath);
    const std::string genId2 = rootV2["matterGeneration"].value("snapshotId", std::string{});
    check(genId2 != genId1, "changed matter content commits under a new generation id");
    check(!std::filesystem::exists(genPath1),
          "the superseded generation file is removed once the new root commits");
    const auto genPath2 = sandbox / "worlds" / ("gen_test." + genId2 + ".ecmatter");
    check(std::filesystem::exists(genPath2), "the new generation file exists");

    // ---- Round 3: save again with NO change. Content-addressed dedup. ----
    mgr1.saveState(ecformPath.string(), ctx);
    nlohmann::json rootV3 = readEcform(ecformPath);
    const std::string genId3 = rootV3["matterGeneration"].value("snapshotId", std::string{});
    check(genId3 == genId2, "saving unchanged matter content reuses the same content-addressed generation id");
    check(std::filesystem::exists(genPath2), "the reused generation file is still present, untouched");

    // ---- Tamper: corrupt the root's recorded hash. Refuse, don't crash, ----
    // ---- and the real (correct) generation file remains intact and       ----
    // ---- independently loadable — "the old generation still loadable."   ----
    {
        nlohmann::json tampered = rootV3;
        const std::string correctHash = tampered["matterGeneration"]["sha256"].get<std::string>();
        std::string badHash = correctHash;
        badHash[0] = (badHash[0] == '0') ? '1' : '0';
        tampered["matterGeneration"]["sha256"] = badHash;
        writeEcform(ecformPath, tampered);

        ZoneManager mgr3;
        mgr3.loadState(ecformPath.string(), ctx);
        Object* found = nullptr;
        for (const auto& z : mgr3.zones()) {
            if (!z) continue;
            for (const auto& o : z->getOwnedObjects()) {
                if (o && o->getObjectID() == "gen-object") { found = o.get(); break; }
            }
        }
        bool hydrated = false;
        if (found) {
            glm::vec3 t = translationOf(found->getTransform());
            hydrated = std::abs(t.x - 4.0f) < 1e-3f && std::abs(t.y - 5.0f) < 1e-3f && std::abs(t.z - 6.0f) < 1e-3f;
        }
        check(!hydrated, "a root naming a hash-mismatched generation refuses to hydrate matter");
        check(!std::filesystem::exists(legacyFixedMatterPath),
              "refusal does not fall through to rebuilding a legacy fixed-name .ecmatter");

        // The actual on-disk generation file was never touched by the refusal.
        const auto stillThere = readBytes(genPath2);
        check(!stillThere.empty() && sha256Hex(stillThere) == correctHash,
              "the real generation file is untouched and still hashes correctly after a refused load");

        // Restore the untampered root for the next case.
        writeEcform(ecformPath, rootV3);
    }

    // ---- Missing file: root names a generation whose file was deleted. ----
    {
        std::filesystem::remove(genPath2);
        ZoneManager mgr4;
        mgr4.loadState(ecformPath.string(), ctx);
        Object* found = nullptr;
        for (const auto& z : mgr4.zones()) {
            if (!z) continue;
            for (const auto& o : z->getOwnedObjects()) {
                if (o && o->getObjectID() == "gen-object") { found = o.get(); break; }
            }
        }
        bool hydrated = false;
        if (found) {
            glm::vec3 t = translationOf(found->getTransform());
            hydrated = std::abs(t.x - 4.0f) < 1e-3f && std::abs(t.y - 5.0f) < 1e-3f && std::abs(t.z - 6.0f) < 1e-3f;
        }
        check(!hydrated, "a root naming a missing generation file refuses to hydrate matter");
        check(!std::filesystem::exists(legacyFixedMatterPath),
              "a missing named generation does not trigger a silent legacy re-migration either");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "matter_generation_commit_test: FAILED\n";
        return 1;
    }
    std::cout << "matter_generation_commit_test: ALL OK\n";
    return 0;
}
