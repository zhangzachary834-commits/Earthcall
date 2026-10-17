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
//   4. Pure-symbolic Zone without matterGeneration switches cleanly without error.
//   5. persistZone writes a content-addressed snapshot sidecar (<stem>.<snapshotId>.ecmatter),
//      writes unversioned <stem>.ecmatter, updates matterGeneration metadata (snapshotId,
//      sha256, byteLength, schemaVersion: 1), and cleans up predecessor generations.
//   6. Save Zone isolation: leaves other zones byte-identical and creates no conglomerate
//      saves/worlds directory.

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
        const std::size_t pure = indexOf(zm, "ZtPure");

        check(alpha < zm.zones().size() && corrupt < zm.zones().size() &&
              missing < zm.zones().size() && pure < zm.zones().size(),
              "fresh boot discovers all four Zones from saves/zones alone");
        if (alpha >= zm.zones().size() || corrupt >= zm.zones().size() ||
            missing >= zm.zones().size() || pure >= zm.zones().size()) return 1;

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

        // (4) Move to ZtPure: pure symbolic zone without matter switches cleanly
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

        // (5) Save Zone commits new matter snapshot and retires predecessor
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
              "unversioned zone.ecmatter content matches new generation snapshot");

        // (6) Isolation: ZtPure is byte-identical and no worlds/ directory was created
        check(slurp(pureZoneJsonPath) == pureBefore, "ZtPure remains byte-identical");
        check(!std::filesystem::exists(scratch.path / "worlds"),
              "persistZone created no conglomerate worlds/ directory");
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
