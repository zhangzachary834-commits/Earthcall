// Zone-native Save Zone isolation.
//
// Zach's required ordinary path is Creator Console -> Zones -> Move to Zone
// -> Save Zone. A save of one Zone must not rewrite every other Zone and must
// not mint/update a conglomerate saves/worlds session file. This test guards
// the storage boundary beneath that button, independently of ImGui.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << '\n';
    } else {
        std::cout << "  ok: " << description << '\n';
    }
}

std::shared_ptr<Object> makeCube(const std::string& id, const glm::vec3& position) {
    auto object = std::make_shared<Object>();
    object->setShape(Object::ShapeKind::Cube);
    object->setObjectID(id);
    object->setTransform(glm::translate(glm::mat4(1.0f), position));
    return object;
}

std::string readBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

bool containsObjectId(const std::filesystem::path& path, const std::string& id) {
    std::ifstream input(path);
    if (!input) return false;
    nlohmann::json document;
    input >> document;
    if (!document.contains("world") || !document["world"].is_object() ||
        !document["world"].contains("objects") || !document["world"]["objects"].is_array()) {
        return false;
    }
    for (const auto& object : document["world"]["objects"]) {
        if (object.value("objectID", std::string{}) == id ||
            object.value("id", std::string{}) == id ||
            object.value("identifier", std::string{}) == id) {
            return true;
        }
    }
    return false;
}

bool hasAnyRegularFile(const std::filesystem::path& directory) {
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec)) return false;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec) return false;
        if (entry.is_regular_file()) return true;
    }
    return false;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Zone-native Save Zone isolation...\n";
    std::cout << "============================================================\n";

    const auto sandbox =
        std::filesystem::temp_directory_path() / "earthcall_zone_native_save_isolation";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());

    ZoneManager manager;

    auto alpha = std::make_shared<Zone>("Alpha", "strict");
    alpha->addObject(makeCube("alpha-before", glm::vec3(1.0f, 0.0f, 0.0f)));
    manager.addZone(alpha);

    auto beta = std::make_shared<Zone>("Beta", "strict");
    beta->addObject(makeCube("beta-stable", glm::vec3(7.0f, 0.0f, 0.0f)));
    manager.addZone(beta);

    // Seed both identities through the compatibility bulk writer. The test's
    // subject begins after this point: one Zone changes, one Zone is saved.
    manager.persistZones();

    const auto alphaPath = sandbox / "zones" / "Alpha" / "zone.json";
    const auto betaPath = sandbox / "zones" / "Beta" / "zone.json";
    check(std::filesystem::exists(alphaPath), "bulk seed created Alpha identity");
    check(std::filesystem::exists(betaPath), "bulk seed created Beta identity");

    const std::string betaBefore = readBytes(betaPath);
    const std::string alphaBefore = readBytes(alphaPath);

    alpha->addObject(makeCube("alpha-after", glm::vec3(3.0f, 0.0f, 0.0f)));

    // _currentIndex begins at zero, so this is exactly the storage call used
    // by Creator Console's Save Zone button for active Alpha.
    check(manager.persistActiveZone(), "Save Zone succeeds for active Alpha");

    const std::string alphaAfter = readBytes(alphaPath);
    const std::string betaAfter = readBytes(betaPath);

    check(alphaAfter != alphaBefore, "Alpha identity changes after Alpha is edited and saved");
    check(containsObjectId(alphaPath, "alpha-after"),
          "Alpha's newly authored being is present in Alpha identity");
    check(betaAfter == betaBefore,
          "Beta identity remains byte-for-byte unchanged when Alpha is saved");
    check(containsObjectId(betaPath, "beta-stable"),
          "Beta's authored being remains present after Alpha-only save");

    check(!hasAnyRegularFile(sandbox / "worlds"),
          "Save Zone creates no conglomerate saves/worlds session file");
    check(!manager.persistZone(9999), "invalid Zone index is refused loudly");

    std::filesystem::remove_all(sandbox);

    std::cout << "============================================================\n";
    std::cout << (g_failures == 0 ? "PASS" : "FAIL") << ": " << g_checks
              << " checks, " << g_failures << " failure(s)\n";
    std::cout << "============================================================\n";
    return g_failures == 0 ? 0 : 1;
}
