// Per-Zone serialization pathway, proof items 4 and 5 (Material half) and the
// Save Zone isolation clause of proof 3.
//   Claude Code / Sonnet 5.5, session 01GxayCUN2nc7DDaeg33kXhZ, 2026-10-07.
//   Authority: Zach, 2026-10-07 ("do the rest of it") on the 2026-09-09
//   correction that a Zone must be independently complete and saveable.
//
// A scratch SaveRoot holds three Zones cloned from the real visible_cube
// identity: Alpha and Beta both use ONE shared Material root
// (saves/materials/zt_shared_red/material.json, named by `materialRefs`);
// Gamma names a Material that is defined nowhere. There is no worlds/
// directory and this test never calls loadState().
//
//   1. Move to Alpha resolves the shared Material from its root alone.
//   2. Move to Gamma REFUSES (dangling Material) with the current Zone intact,
//      even though the live register holds unrelated Materials.
//   3. A shared root deleted from disk makes Beta refuse, not render white.
//   4. Save Zone on Alpha leaves Beta's identity and the unchanged shared root
//      byte-identical, creates no worlds/ directory, keeps `materialRefs`, and
//      does not embed a private copy of the shared Material.
//   5. Changing the shared Material and saving Alpha rewrites the ONE root.
//   6. Material::fromJson honours an explicit "id" (the Prism Cathedral fault).

#include "support/test_harness.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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
} // namespace

int main() {
    // (6) needs no engine.
    {
        const auto m = Material::fromJson(nlohmann::json{
            {"id", "material.zt_prism_like"}, {"name", "Display Label"},
            {"baseColor", {0.1, 0.2, 0.3}}});
        check(m.name() == "zt_prism_like" && m.getIdentifier() == "material.zt_prism_like",
              "Material::fromJson takes its identity from an explicit \"id\"");
        const auto plain = Material::fromJson(nlohmann::json{{"name", "plain"}});
        check(plain.getIdentifier() == "material.plain", "without \"id\", name is still the identity");
    }

    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/visible_cube/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    const auto template_ = saves / "zones/visible_cube/zone.json";
    check(std::filesystem::exists(template_), "visible_cube template identity exists");
    if (!std::filesystem::exists(template_)) return 1;
    const nlohmann::json base = SaveSystem::readSaveData(std::filesystem::absolute(template_).string());

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_zone_material_closure_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};

    const auto makeZone = [&](const std::string& id, const std::string& materialId,
                              bool refShared) {
        nlohmann::json j = base;
        j["identifier"] = id;
        j["name"] = id;
        j.erase("materials");
        j["world"]["objects"][0]["objectID"] = id + "-cube";
        j["world"]["objects"][0]["materialId"] = materialId;
        if (refShared) j["materialRefs"] = nlohmann::json::array({"material.zt_shared_red"});
        const auto dir = scratch.path / "zones" / id;
        std::filesystem::create_directories(dir);
        std::ofstream(dir / "zone.json") << j.dump(2);
    };
    makeZone("ZtAlpha", "material.zt_shared_red", true);
    makeZone("ZtBeta", "material.zt_shared_red", true);
    makeZone("ZtGamma", "material.zt_ghost", false);

    {
        Material red("zt_shared_red");
        red.baseColor = glm::vec3(0.9f, 0.1f, 0.1f);
        const auto dir = scratch.path / "materials/zt_shared_red";
        std::filesystem::create_directories(dir);
        std::ofstream(dir / "material.json")
            << nlohmann::json{{"identifier", "material.zt_shared_red"}, {"material", red.toJson()}}.dump(2);
    }

    SaveSystem::setSaveRoot(scratch.path.string());
    check(!std::filesystem::exists(scratch.path / "worlds"), "scratch root has no worlds/ directory");

    {
        TestSupport::BootedEngineHarness harness;
        auto& zm = harness.zones;
        const std::size_t alpha = indexOf(zm, "ZtAlpha");
        const std::size_t beta = indexOf(zm, "ZtBeta");
        const std::size_t gamma = indexOf(zm, "ZtGamma");
        check(alpha < zm.zones().size() && beta < zm.zones().size() && gamma < zm.zones().size(),
              "fresh boot discovers all three Zones from saves/zones alone");
        if (alpha >= zm.zones().size() || beta >= zm.zones().size() || gamma >= zm.zones().size()) return 1;

        // (1) Alpha resolves the shared Material from its root.
        check(zm.switchTo(alpha), "Move to Zone commits Alpha with a shared Material root");
        auto live = materials.get("material.zt_shared_red");
        check(live != nullptr, "shared Material is live after the move");
        check(live && live->baseColor.r > 0.89f && live->baseColor.g < 0.11f,
              "shared Material carries the root's authored colour");

        // (2) Gamma names a Material defined nowhere: refuse, stay in Alpha.
        const std::size_t before = zm.currentIndex();
        check(!zm.switchTo(gamma), "Move to a Zone with a dangling Material REFUSES");
        check(zm.currentIndex() == before && before == alpha,
              "the refusal leaves the Person in Alpha");

        // (4) Save Zone isolation.
        const auto betaFile = scratch.path / "zones/ZtBeta/zone.json";
        const auto gammaFile = scratch.path / "zones/ZtGamma/zone.json";
        const auto rootFile = scratch.path / "materials/zt_shared_red/material.json";
        const std::string betaBytes = slurp(betaFile);
        const std::string gammaBytes = slurp(gammaFile);
        const std::string rootBytes = slurp(rootFile);
        check(zm.persistZone(alpha), "Save Zone commits Alpha");
        check(slurp(betaFile) == betaBytes, "Save Zone leaves Beta's identity byte-identical");
        check(slurp(gammaFile) == gammaBytes, "Save Zone leaves Gamma's identity byte-identical");
        check(slurp(rootFile) == rootBytes, "an unchanged shared Material root is not rewritten");
        check(!std::filesystem::exists(scratch.path / "worlds"), "Save Zone created no worlds/ directory");
        const nlohmann::json alphaSaved =
            SaveSystem::readSaveData((scratch.path / "zones/ZtAlpha/zone.json").string());
        check(alphaSaved.contains("materialRefs") && alphaSaved["materialRefs"].size() == 1,
              "Alpha still names the shared Material");
        bool embedded = false;
        if (alphaSaved.contains("materials")) {
            for (const auto& m : alphaSaved["materials"]) {
                if (Material::fromJson(m).name() == "zt_shared_red") embedded = true;
            }
        }
        check(!embedded, "Alpha did not embed a private copy of the shared Material");

        // (5) Changing the shared Material rewrites the ONE root.
        live->baseColor = glm::vec3(0.1f, 0.9f, 0.1f);
        check(zm.persistZone(alpha), "Save Zone commits Alpha after the shared Material changed");
        check(slurp(rootFile) != rootBytes, "the shared root now holds the change");
        check(slurp(betaFile) == betaBytes, "Beta is still byte-identical");
        const nlohmann::json root = SaveSystem::readSaveData(rootFile.string());
        check(root.value("identifier", std::string{}) == "material.zt_shared_red",
              "the root keeps its stable identifier");

        // (3) Delete the root: Beta must refuse instead of rendering white.
        std::filesystem::remove(rootFile);
        check(!zm.switchTo(beta), "Move to Beta REFUSES when its named Material root is gone");
        check(zm.currentIndex() == alpha, "the refusal leaves the Person in Alpha");
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
