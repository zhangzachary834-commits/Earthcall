// Per-Zone serialization pathway, repair boundary item 2:
// "Give each Zone its own verified, content-addressed matter generation for
//  genuinely physical/heavy geometry, and load it only through that Zone's
//  activation transaction."
//
// Proof points verified here:
//   1. Move to Zone verifies and hydrates the Zone's content-addressed .ecmatter sidecar.
//   2. Missing .ecmatter when named by matterGeneration REFUSES activation, leaving
//      the Person in the prior Zone intact.
//   3. Corrupted .ecmatter (truncated, hash mismatch, or invalid FlatBuffer) REFUSES
//      activation, leaving the Person in the prior Zone intact.
//   4. A named generation cannot fall back to an unversioned compatibility sidecar;
//      the exact <stem>.<snapshotId>.ecmatter pointer must exist.
//   5. Malformed content-address metadata, stale schema versions, and wrong-owner
//      entities REFUSE before activation.
//   6. Pure-symbolic Zone without matterGeneration switches cleanly without error.
//   7. persistZone refuses a pre-existing conflicting generation file, then writes a
//      content-addressed snapshot + post-commit compatibility mirror, updates metadata,
//      cleans its predecessor, and reuses the same generation for unchanged Matter.
//   8. Save Zone isolation: leaves other zones byte-identical and creates no conglomerate
//      saves/worlds directory.
//   9. The authored Zone/Home corpus is closed across BOTH JSON and ECForm roots:
//      every effective root that names matterGeneration names a real exact sidecar
//      with matching length/hash, a byte-identical compatibility mirror, a valid
//      FlatBuffer, and entities owned uniquely by that root's identifier.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <openssl/sha.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
int checks = 0;
int failures = 0;

void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}

std::string slurp(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
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

struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::size_t indexOf(ZoneManager& zm, const std::string& id) {
    for (std::size_t i = 0; i < zm.zones().size(); ++i) {
        if (zm.zones()[i] && zm.zones()[i]->getIdentifier() == id) return i;
    }
    return zm.zones().size();
}

Object* findObject(Zone& zone, const std::string& id) {
    for (const auto& obj : zone.getOwnedObjects()) {
        if (obj && obj->getIdentifier() == id) return obj.get();
    }
    return nullptr;
}

bool isLowerHex(const std::string& value, std::size_t width) {
    if (value.size() != width) return false;
    for (char ch : value) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return false;
    }
    return true;
}

std::vector<uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return {};
    return std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::filesystem::path effectiveIdentityRoot(const std::filesystem::path& dir,
                                            const char* stem) {
    for (const char* ext : {".ecsave", ".ecform", ".json"}) {
        const auto candidate = dir / (std::string(stem) + ext);
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec) &&
            !ec && std::filesystem::file_size(candidate, ec) > 0 && !ec) {
            return candidate;
        }
    }
    return {};
}

void auditAuthoredMatterRoot(const std::filesystem::path& rootPath) {
    nlohmann::json root = SaveSystem::readSaveData(rootPath.string());
    check(root.is_object(), "authored semantic root parses: " + rootPath.string());
    if (!root.is_object() || !root.contains("matterGeneration")) return;

    const std::string id = root.value("identifier", std::string{});
    check(!id.empty(), "matter-bearing root has identifier: " + rootPath.string());

    const auto& gen = root["matterGeneration"];
    check(gen.is_object(), "matterGeneration is an object: " + rootPath.string());
    if (!gen.is_object()) return;

    const std::string snapshotId = gen.value("snapshotId", std::string{});
    const std::string expectedHash = gen.value("sha256", std::string{});
    const std::size_t expectedLength = gen.value("byteLength", std::size_t{0});
    const int schemaVersion = gen.value("schemaVersion", 0);

    check(isLowerHex(snapshotId, 16),
          "snapshotId is 16 lowercase hex: " + rootPath.string());
    check(isLowerHex(expectedHash, 64),
          "sha256 is 64 lowercase hex: " + rootPath.string());
    check(expectedHash.size() >= snapshotId.size() &&
              expectedHash.substr(0, snapshotId.size()) == snapshotId,
          "snapshotId is sha256 prefix: " + rootPath.string());
    check(expectedLength > 0, "matter byteLength is non-zero: " + rootPath.string());
    check(schemaVersion == 1, "matter schemaVersion is exactly 1: " + rootPath.string());
    if (!isLowerHex(snapshotId, 16) || !isLowerHex(expectedHash, 64) ||
        expectedHash.substr(0, 16) != snapshotId || expectedLength == 0 ||
        schemaVersion != 1 || id.empty()) {
        return;
    }

    const std::string stem = rootPath.stem().string();
    const auto exact =
        rootPath.parent_path() / (stem + "." + snapshotId + ".ecmatter");
    const auto compatibility =
        rootPath.parent_path() / (stem + ".ecmatter");

    check(std::filesystem::exists(exact),
          "exact content-addressed generation exists: " + exact.string());
    check(std::filesystem::exists(compatibility),
          "compatibility matter mirror exists: " + compatibility.string());
    if (!std::filesystem::exists(exact)) return;

    const auto bytes = readBytes(exact);
    check(bytes.size() == expectedLength,
          "generation byteLength matches semantic root: " + exact.string());
    check(!bytes.empty() && sha256Hex(bytes) == expectedHash,
          "generation sha256 matches semantic root: " + exact.string());

    if (std::filesystem::exists(compatibility)) {
        const auto compatibilityBytes = readBytes(compatibility);
        check(compatibilityBytes == bytes,
              "compatibility mirror is byte-identical to authoritative generation: " +
                  compatibility.string());
    }

    if (bytes.empty()) return;
    flatbuffers::Verifier verifier(bytes.data(), bytes.size());
    const bool flatbufferOk = Earthcall::Schema::VerifySaveChunkBuffer(verifier);
    check(flatbufferOk, "generation is a valid SaveChunk FlatBuffer: " + exact.string());
    if (!flatbufferOk) return;

    const auto* chunk = Earthcall::Schema::GetSaveChunk(bytes.data());
    check(chunk && chunk->entities(),
          "generation contains an entity vector: " + exact.string());
    if (!chunk || !chunk->entities()) return;

    std::unordered_set<std::string> seen;
    bool ownershipOk = true;
    for (flatbuffers::uoffset_t i = 0; i < chunk->entities()->size(); ++i) {
        const auto* entity = chunk->entities()->Get(i);
        if (!entity || !entity->id() || entity->id()->size() == 0 ||
            !entity->owner_identifier() ||
            entity->owner_identifier()->str() != id ||
            !seen.insert(entity->id()->str()).second) {
            ownershipOk = false;
            break;
        }
    }
    check(ownershipOk,
          "generation ids are unique and every entity is owned by semantic root '" +
              id + "': " + exact.string());
}

void auditAuthoredMatterCorpus(const std::filesystem::path& saves) {
    std::size_t effectiveRoots = 0;
    std::size_t generatedRoots = 0;

    const auto auditFamily = [&](const std::filesystem::path& family,
                                 const char* stem) {
        std::error_code ec;
        if (!std::filesystem::is_directory(family, ec) || ec) return;
        for (const auto& entry : std::filesystem::directory_iterator(family)) {
            if (!entry.is_directory()) continue;
            const auto root = effectiveIdentityRoot(entry.path(), stem);
            if (root.empty()) continue;
            ++effectiveRoots;
            const auto parsed = SaveSystem::readSaveData(root.string());
            if (parsed.is_object() && parsed.contains("matterGeneration")) {
                ++generatedRoots;
            }
            auditAuthoredMatterRoot(root);
        }
    };

    auditFamily(saves / "zones", "zone");
    auditFamily(saves / "homes", "home");

    check(effectiveRoots > 0, "authored corpus exposes effective Zone/Home roots");
    check(generatedRoots > 0,
          "authored corpus includes verified content-addressed matter generations");
    std::cout << "  corpus: " << effectiveRoots << " effective roots, "
              << generatedRoots << " with matterGeneration\n";
}

std::vector<uint8_t> makeTestMatterBuffer(const std::string& objId,
                                         const std::string& ownerId,
                                         float x, float y, float z) {
    flatbuffers::FlatBufferBuilder builder(1024);
    auto idStr = builder.CreateString(objId);
    auto nameStr = builder.CreateString("TestObject");
    std::vector<float> tf = {1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1};
    auto tfVec = builder.CreateVector(tf);
    auto ownerStr = builder.CreateString(ownerId);

    auto ent = Earthcall::Schema::CreateEntity(
        builder, idStr, nameStr, tfVec,
        0, 0, 0, 0,
        0, 0, 0, 0, 0,
        nullptr, nullptr, nullptr,
        1.0f,
        ownerStr);

    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> ents = {ent};
    auto chunk = Earthcall::Schema::CreateSaveChunk(
        builder, builder.CreateString("matter_" + ownerId), builder.CreateVector(ents));
    builder.Finish(chunk);
    const uint8_t* buf = builder.GetBufferPointer();
    return std::vector<uint8_t>(buf, buf + builder.GetSize());
}
} // namespace

int main() {
    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/visible_cube/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    const auto templatePath = saves / "zones/visible_cube/zone.json";
    check(std::filesystem::exists(templatePath), "visible_cube template identity exists");
    if (!std::filesystem::exists(templatePath)) return 1;

    // Audit the actual authored corpus BEFORE redirecting SaveSystem into the
    // synthetic scratch root below. This walks effective .ecsave/.ecform/.json
    // identities with the same priority as SaveSystem's identity resolver.
    auditAuthoredMatterCorpus(std::filesystem::absolute(saves));

    nlohmann::json base = SaveSystem::readSaveData(std::filesystem::absolute(templatePath).string());

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_zone_matter_closure_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};

    // Prepare zones in scratch:
    // 1. ZtAlpha: valid matter generation + verified .ecmatter sidecars
    // 2. ZtCorrupt: matterGeneration with hash mismatch / corrupted content
    // 3. ZtMissing: matterGeneration pointing to non-existent .ecmatter file
    // 4. ZtPure: no matterGeneration (pure symbolic/transformless)

    // ZtAlpha
    {
        const std::string zoneId = "ZtAlpha";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        std::vector<uint8_t> matterBytes = makeTestMatterBuffer("alpha-matter-obj", zoneId, 1.0f, 2.0f, 3.0f);
        const std::string hash = sha256Hex(matterBytes);
        const std::string snapId = hash.substr(0, 16);

        // Write content-addressed and unversioned matter
        std::ofstream(dir / ("zone." + snapId + ".ecmatter"), std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());
        std::ofstream(dir / "zone.ecmatter", std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "alpha-matter-obj";
        j["matterGeneration"] = {
            {"snapshotId", snapId},
            {"sha256", hash},
            {"byteLength", matterBytes.size()},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtCorrupt
    {
        const std::string zoneId = "ZtCorrupt";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        std::vector<uint8_t> matterBytes = makeTestMatterBuffer("corrupt-obj", zoneId, 0, 0, 0);
        const std::string realHash = sha256Hex(matterBytes);
        const std::string snapId = realHash.substr(0, 16);

        // Write corrupt bytes into the sidecar
        std::vector<uint8_t> badBytes = matterBytes;
        badBytes[10] ^= 0xFF; // corrupt FlatBuffer header
        std::ofstream(dir / ("zone." + snapId + ".ecmatter"), std::ios::binary).write(
            reinterpret_cast<const char*>(badBytes.data()), badBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "corrupt-obj";
        j["matterGeneration"] = {
            {"snapshotId", snapId},
            {"sha256", realHash},
            {"byteLength", badBytes.size()},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtMissing
    {
        const std::string zoneId = "ZtMissing";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "missing-obj";
        j["matterGeneration"] = {
            {"snapshotId", "0123456789abcdef"},
            {"sha256", "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"},
            {"byteLength", 1234},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtFallbackOnly: semantic root names a content-addressed generation,
    // but only the legacy unversioned sidecar exists. Even byte-identical
    // fallback bytes must NOT satisfy a generation pointer: the named
    // snapshot file is the durable generation witness.
    {
        const std::string zoneId = "ZtFallbackOnly";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        std::vector<uint8_t> matterBytes =
            makeTestMatterBuffer("fallback-only-obj", zoneId, 7.0f, 8.0f, 9.0f);
        const std::string hash = sha256Hex(matterBytes);
        const std::string snapId = hash.substr(0, 16);

        std::ofstream(dir / "zone.ecmatter", std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "fallback-only-obj";
        j["matterGeneration"] = {
            {"snapshotId", snapId},
            {"sha256", hash},
            {"byteLength", matterBytes.size()},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtBadMetadata: snapshotId must be the prefix of the declared full hash.
    {
        const std::string zoneId = "ZtBadMetadata";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        const auto matterBytes =
            makeTestMatterBuffer("bad-metadata-obj", zoneId, 2.0f, 3.0f, 4.0f);
        const std::string hash = sha256Hex(matterBytes);
        std::string badSnap = hash.substr(0, 16);
        badSnap[0] = badSnap[0] == '0' ? '1' : '0';
        std::ofstream(dir / ("zone." + badSnap + ".ecmatter"), std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "bad-metadata-obj";
        j["matterGeneration"] = {
            {"snapshotId", badSnap},
            {"sha256", hash},
            {"byteLength", matterBytes.size()},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtOldSchema: generation schema compatibility is explicit, not guessed.
    {
        const std::string zoneId = "ZtOldSchema";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        const auto matterBytes =
            makeTestMatterBuffer("old-schema-obj", zoneId, 3.0f, 4.0f, 5.0f);
        const std::string hash = sha256Hex(matterBytes);
        const std::string snapId = hash.substr(0, 16);
        std::ofstream(dir / ("zone." + snapId + ".ecmatter"), std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "old-schema-obj";
        j["matterGeneration"] = {
            {"snapshotId", snapId},
            {"sha256", hash},
            {"byteLength", matterBytes.size()},
            {"schemaVersion", 0}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtWrongOwner: new per-Zone generations may not use the legacy
    // unambiguous-bare-id fallback to hydrate matter authored for another Zone.
    {
        const std::string zoneId = "ZtWrongOwner";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        const auto matterBytes =
            makeTestMatterBuffer("wrong-owner-obj", "SomeOtherZone", 4.0f, 5.0f, 6.0f);
        const std::string hash = sha256Hex(matterBytes);
        const std::string snapId = hash.substr(0, 16);
        std::ofstream(dir / ("zone." + snapId + ".ecmatter"), std::ios::binary).write(
            reinterpret_cast<const char*>(matterBytes.data()), matterBytes.size());

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = "wrong-owner-obj";
        j["matterGeneration"] = {
            {"snapshotId", snapId},
            {"sha256", hash},
            {"byteLength", matterBytes.size()},
            {"schemaVersion", 1}
        };
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    // ZtPure
    {
        const std::string zoneId = "ZtPure";
        const auto dir = scratch.path / "zones" / zoneId;
        std::filesystem::create_directories(dir);

        nlohmann::json j = base;
        j["identifier"] = zoneId;
        j["name"] = zoneId;
        j.erase("materials");
        j.erase("matterGeneration");
        j["world"]["objects"][0]["objectID"] = "pure-obj";
        std::ofstream(dir / "zone.json") << j.dump(2);
    }

    SaveSystem::setSaveRoot(scratch.path.string());
    check(!std::filesystem::exists(scratch.path / "worlds"), "scratch root has no worlds/ directory");

    {
        TestSupport::BootedEngineHarness harness;
        auto& zm = harness.zones;

        const std::size_t alpha = indexOf(zm, "ZtAlpha");
        const std::size_t corrupt = indexOf(zm, "ZtCorrupt");
        const std::size_t missing = indexOf(zm, "ZtMissing");
        const std::size_t fallbackOnly = indexOf(zm, "ZtFallbackOnly");
        const std::size_t badMetadata = indexOf(zm, "ZtBadMetadata");
        const std::size_t oldSchema = indexOf(zm, "ZtOldSchema");
        const std::size_t wrongOwner = indexOf(zm, "ZtWrongOwner");
        const std::size_t pure = indexOf(zm, "ZtPure");

        check(alpha < zm.zones().size() && corrupt < zm.zones().size() &&
              missing < zm.zones().size() && fallbackOnly < zm.zones().size() &&
              badMetadata < zm.zones().size() && oldSchema < zm.zones().size() &&
              wrongOwner < zm.zones().size() && pure < zm.zones().size(),
              "fresh boot discovers all eight Zones from saves/zones alone");
        if (alpha >= zm.zones().size() || corrupt >= zm.zones().size() ||
            missing >= zm.zones().size() || fallbackOnly >= zm.zones().size() ||
            badMetadata >= zm.zones().size() || oldSchema >= zm.zones().size() ||
            wrongOwner >= zm.zones().size() || pure >= zm.zones().size()) return 1;

        // (1) Move to ZtAlpha: valid matter generation commits and hydrates
        check(zm.switchTo(alpha), "Move to ZtAlpha commits with valid matter generation");
        check(zm.currentIndex() == alpha, "active zone is ZtAlpha");

        // Verify hydrated object
        auto* alphaObj = findObject(zm.active(), "alpha-matter-obj");
        check(alphaObj != nullptr, "alpha-matter-obj exists in active Zone");
        if (alphaObj) {
            const auto tf = alphaObj->getTransform();
            check(tf[3][0] == 1.0f && tf[3][1] == 2.0f && tf[3][2] == 3.0f,
                  "alpha-matter-obj carries hydrated transform from .ecmatter sidecar");
        }

        // (2) Move to ZtCorrupt: hash/buffer failure REFUSES, leaving Person in ZtAlpha
        check(!zm.switchTo(corrupt), "Move to ZtCorrupt REFUSES due to hash/corrupt buffer");
        check(zm.currentIndex() == alpha, "Person remains safely in ZtAlpha after corrupt refusal");

        // (3) Move to ZtMissing: missing sidecar REFUSES, leaving Person in ZtAlpha
        check(!zm.switchTo(missing), "Move to ZtMissing REFUSES due to missing sidecar file");
        check(zm.currentIndex() == alpha, "Person remains safely in ZtAlpha after missing refusal");

        // (4) Named generation cannot be satisfied by an unversioned compatibility
        // copy, even when that copy has the exact expected hash and length.
        check(!zm.switchTo(fallbackOnly),
              "Move to ZtFallbackOnly REFUSES when named snapshot file is absent");
        check(zm.currentIndex() == alpha,
              "Person remains safely in ZtAlpha after generation-pointer refusal");

        // (5) Strict generation metadata and ownership are part of admission.
        check(!zm.switchTo(badMetadata),
              "Move to ZtBadMetadata REFUSES snapshot/hash identity mismatch");
        check(zm.currentIndex() == alpha,
              "Person remains in ZtAlpha after malformed generation metadata");
        check(!zm.switchTo(oldSchema),
              "Move to ZtOldSchema REFUSES unsupported schema version");
        check(zm.currentIndex() == alpha,
              "Person remains in ZtAlpha after schema refusal");
        check(!zm.switchTo(wrongOwner),
              "Move to ZtWrongOwner REFUSES matter owned by another Zone");
        check(zm.currentIndex() == alpha,
              "Person remains in ZtAlpha after wrong-owner refusal");

        // (6) Move to ZtPure: pure symbolic zone without matter switches cleanly
        check(zm.switchTo(pure), "Move to ZtPure succeeds cleanly without matter sidecar");
        check(zm.currentIndex() == pure, "active zone is ZtPure");

        // Switch back to Alpha to test persistZone and predecessor retirement
        check(zm.switchTo(alpha), "Switch back to ZtAlpha succeeds");

        // Mutate Alpha's object and persist
        if (alphaObj) {
            auto tf = alphaObj->getTransform();
            tf[3][0] = 42.0f;
            alphaObj->setTransform(tf);
        }

        const auto alphaZoneJsonPath = scratch.path / "zones/ZtAlpha/zone.json";
        const auto alphaZoneEcformPath = scratch.path / "zones/ZtAlpha/zone.ecform";
        const auto pureZoneJsonPath = scratch.path / "zones/ZtPure/zone.json";
        const std::string pureBefore = slurp(pureZoneJsonPath);

        // Read prior snapshot id before save
        nlohmann::json priorDoc = SaveSystem::readSaveData(alphaZoneJsonPath.string());
        const std::string priorSnapId = priorDoc["matterGeneration"].value("snapshotId", std::string{});
        const auto priorMatterFile = scratch.path / "zones/ZtAlpha" / ("zone." + priorSnapId + ".ecmatter");
        check(std::filesystem::exists(priorMatterFile), "prior matter snapshot exists before save");

        // (7a) A pre-existing file under the would-be content address must
        // never be overwritten if its bytes disagree.  Because Matter bytes
        // are deterministic, we can predict the exact generation path here.
        const auto intendedMatter =
            zm.buildMatterFlatBuffer(std::unordered_set<std::string>{"ZtAlpha"});
        const std::string intendedHash = sha256Hex(intendedMatter);
        const std::string intendedSnapId = intendedHash.substr(0, 16);
        const auto conflictingMatterFile =
            scratch.path / "zones/ZtAlpha" / ("zone." + intendedSnapId + ".ecmatter");
        const std::string identityBeforeCollision = slurp(alphaZoneJsonPath);
        {
            std::ofstream poison(conflictingMatterFile, std::ios::binary);
            poison << "not-the-addressed-content";
        }
        check(!zm.persistZone(alpha),
              "persistZone REFUSES a conflicting pre-existing content-addressed filename");
        check(slurp(alphaZoneJsonPath) == identityBeforeCollision,
              "collision refusal leaves semantic Zone identity byte-identical");
        check(slurp(conflictingMatterFile) == "not-the-addressed-content",
              "collision refusal does not overwrite the conflicting generation");
        std::filesystem::remove(conflictingMatterFile);

        // (7b) Save Zone commits the new generation and retires predecessor.
        check(zm.persistZone(alpha), "persistZone commits ZtAlpha");

        nlohmann::json newDoc = SaveSystem::readSaveData(alphaZoneJsonPath.string());
        check(newDoc.contains("matterGeneration"), "saved ZtAlpha identity contains matterGeneration");
        const std::string newSnapId = newDoc["matterGeneration"].value("snapshotId", std::string{});
        const std::string newHash = newDoc["matterGeneration"].value("sha256", std::string{});
        const std::size_t newByteLength = newDoc["matterGeneration"].value("byteLength", std::size_t{0});
        const int newVersion = newDoc["matterGeneration"].value("schemaVersion", 0);

        check(!newSnapId.empty() && newSnapId != priorSnapId,
              "new snapshotId minted for mutated matter");
        check(newVersion == 1, "schemaVersion is 1");
        check(newByteLength > 0, "byteLength is non-zero");

        const auto newMatterFile = scratch.path / "zones/ZtAlpha" / ("zone." + newSnapId + ".ecmatter");
        const auto unversionedMatter = scratch.path / "zones/ZtAlpha/zone.ecmatter";
        check(std::filesystem::exists(newMatterFile), "new content-addressed matter snapshot exists on disk");
        check(std::filesystem::exists(unversionedMatter), "unversioned zone.ecmatter written beside it");
        check(!std::filesystem::exists(priorMatterFile), "predecessor matter snapshot file cleaned up");

        // Verify unversioned file matches new content-addressed file
        check(slurp(newMatterFile) == slurp(unversionedMatter),
              "post-commit unversioned zone.ecmatter mirrors the authoritative generation");

        // Content-addressing means unchanged physical state reuses the same
        // generation rather than minting a clock-derived pseudo-generation.
        check(zm.persistZone(alpha), "persistZone can save unchanged Matter again");
        nlohmann::json repeatedDoc = SaveSystem::readSaveData(alphaZoneJsonPath.string());
        check(repeatedDoc["matterGeneration"].value("snapshotId", std::string{}) == newSnapId,
              "unchanged Matter reuses exactly the same content-addressed generation");

        // (8) Isolation: ZtPure is byte-identical and no worlds/ directory was created
        check(slurp(pureZoneJsonPath) == pureBefore, "ZtPure remains byte-identical");
        check(!std::filesystem::exists(scratch.path / "worlds"),
              "persistZone created no conglomerate worlds/ directory");
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
