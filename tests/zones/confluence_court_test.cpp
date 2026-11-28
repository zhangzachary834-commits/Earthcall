// Loads the authored Confluence Court through the same Zone and Law closure as
// the app. All saves in this witness go to an isolated copy of the five roots.
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "support/test_harness.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <tuple>

namespace {

namespace fs = std::filesystem;
constexpr const char* courtId = "confluence-court";
constexpr const char* copperId = "confluence-court-copper";
constexpr const char* indigoId = "confluence-court-indigo";
constexpr const char* copperLens = "confluence-copper-lens";
constexpr const char* indigoLens = "confluence-indigo-lens";
constexpr const char* commonLens = "confluence-common-lens";
constexpr const char* levelPath = "confluence.lampLevel";

struct Sandbox {
    std::string previous = SaveSystem::saveRoot();
    fs::path root = fs::temp_directory_path() / ("earthcall-confluence-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    Sandbox() {
        const auto seeds = fs::path(__FILE__).parent_path().parent_path().parent_path() / "saves";
        for (const auto& [group, id, file] : {
            std::tuple{"zones", courtId, "zone.json"},
            {"zones", copperId, "zone.json"},
            {"zones", indigoId, "zone.json"},
            {"laws", "confluence-turn-lamp", "law.json"},
            {"laws", "confluence-show-lamp", "law.json"}
        }) {
            const auto relative = fs::path(group) / id / file;
            fs::create_directories((root / relative).parent_path());
            fs::copy_file(seeds / relative, root / relative);
        }
        SaveSystem::setSaveRoot(root.string());
    }

    ~Sandbox() {
        SaveSystem::setSaveRoot(previous);
        std::error_code error;
        fs::remove_all(root, error);
    }
};

size_t index(ZoneManager& manager, const char* id) {
    const auto result = manager.findZoneIndex(id);
    assert(result < manager.zones().size());
    return result;
}

Object& object(Zone& zone, const char* id) {
    for (const auto& candidate : zone.objects()) {
        if (candidate && candidate->getIdentifier() == id) return *candidate;
    }
    throw std::runtime_error(std::string("Missing object ") + id);
}

bool located(ZoneManager& manager, const Object& object, const Zone& zone) {
    const auto zones = manager.locate(object);
    return std::find(zones.begin(), zones.end(), &zone) != zones.end();
}

PropertyValue read(Object& object, const char* path) {
    PropertyValue value;
    assert(PropertyPath::parse(path).getValue(object, value) == PropertyPath::PathResult::Ok);
    return value;
}

void click(TestSupport::BootedEngineHarness& app, Object& lens) {
    std::vector<Object*> reachable;
    for (const auto& candidate : app.zones.active().objects()) {
        if (candidate) reachable.push_back(candidate.get());
    }
    const auto position = lens.getPosition();
    Singularity::Input::InteractionChannel::Sense sense;
    sense.rayOrigin = {position.x, position.y, 0.0f};
    sense.left = true;
    app.interaction->observe(sense, reachable);
    assert(app.interaction->hoveredId == lens.getIdentifier());
    sense.left = false;
    app.interaction->observe(sense, reachable);
    app.lawManager.tick();
    app.lawManager.tick();
}

void checkLenses(Object& copper, Object& indigo, Object& common, double level) {
    const auto cellId = copper.getPropertyCell(levelPath);
    assert(cellId.valid() && indigo.getPropertyCell(levelPath) == cellId);
    assert(common.getPropertyCell(levelPath) == cellId);
    assert(common.getPropertyCellMode(levelPath) == CellOwnershipMode::Weak);
    const auto* cell = PropertyCellTable::instance().getCell(cellId);
    assert(cell && cell->strongCount == 2);
    for (auto* lens : {&copper, &indigo, &common}) {
        assert(std::get<double>(read(*lens, levelPath)) == level);
        const auto color = std::get<glm::vec3>(read(*lens, "color"));
        const glm::vec3 expected = level == 1.0 ?
            glm::vec3(1.0f, 0.74f, 0.30f) : glm::vec3(0.22f, 0.16f, 0.12f);
        assert(glm::length(color - expected) < 0.01f);
        const auto material = materials.get(lens->materialId());
        assert(material && !material->faceTextures.empty());
        for (const auto& face : material->faceTextures) {
            assert(face.pixels.size() >= 4);
            for (int channel = 0; channel < 3; ++channel) {
                assert(std::abs(face.pixels[channel] / 255.0f - expected[channel]) < 0.01f);
            }
        }
    }
}

void checkIndependentStores() {
    const auto original = SaveSystem::readZoneIdentity(courtId);
    const auto loadOnly = [&](const char* id) {
        auto one = original;
        one["identifier"] = id;
        auto& objects = one["world"]["objects"];
        objects.erase(std::remove_if(objects.begin(), objects.end(),
            [&](const auto& record) { return record.at("objectID") != id; }), objects.end());
        assert(objects.size() == 1);
        return makeZoneFromJson(one);
    };
    auto copper = loadOnly(copperLens);
    auto indigo = loadOnly(indigoLens);
    // The source records carry the same serialized key. Independent Zone
    // hydrations still allocate independent live cells.
    const auto west = object(*copper, copperLens).getPropertyCell(levelPath);
    const auto east = object(*indigo, indigoLens).getPropertyCell(levelPath);
    assert(west.valid() && east.valid() && west != east);
}

void admitAuthor(TestSupport::BootedEngineHarness& app) {
    TransferPolicy::instance().syncRegisterMemoryLaws(app.lawManager);
    const auto root = SaveSystem::readLawIdentity("confluence-turn-lamp");
    const auto id = root.at("law").at("authors").at(0).get<std::string>();
    assert(app.player.setPersonId(Identity::SingularId::parse(id)));
    Universe::instance().bumpStructuralRevision();
    Universe::instance().setClock(0.0, 1.0 / 60.0);
}

} // namespace

int main() {
    Sandbox sandbox;
    checkIndependentStores();
    {
        TestSupport::BootedEngineHarness app("Zach");
        admitAuthor(app);
        auto& manager = app.zones;
        const auto rootIndex = index(manager, courtId);
        const auto copperIndex = index(manager, copperId);
        const auto indigoIndex = index(manager, indigoId);
        auto& root = *manager.zones()[rootIndex];
        auto& copper = *manager.zones()[copperIndex];
        auto& indigo = *manager.zones()[indigoIndex];
        auto& common = object(root, commonLens);
        auto& west = object(root, copperLens);
        auto& east = object(root, indigoLens);

        assert(manager.switchTo(rootIndex));
        // Ordinary entry loads the shared Law closure without a manual mount.
        assert(manager.activeZones().size() == 1);
        app.lawManager.tick();
        click(app, common);
        checkLenses(west, east, common, 0.0);
        click(app, west);
        checkLenses(west, east, common, 1.0);
        click(app, east);
        checkLenses(west, east, common, 0.0);
        assert(manager.mountZone(copperId) && manager.mountZone(indigoId));
        assert(manager.activeZones().size() == 3);
        assert(manager.residenceOf(common) == &root);
        assert(located(manager, common, copper) && located(manager, common, indigo));
        assert(located(manager, west, copper) && !located(manager, west, indigo));
        assert(located(manager, east, indigo) && !located(manager, east, copper));
        app.lawManager.tick();
        checkLenses(west, east, common, 0.0);

        assert(manager.switchTo(copperIndex));
        assert(&object(copper, commonLens) == &common);
        click(app, west);
        checkLenses(west, east, common, 1.0);
        assert(PropertyPath::parse(levelPath).setValue(common, 0.0) ==
            PropertyPath::PathResult::ReadOnly);

        assert(manager.switchTo(indigoIndex));
        assert(&object(indigo, commonLens) == &common);
        const auto beings = Universe::instance().beings();
        assert(std::count(beings.begin(), beings.end(), &common) == 1);
        for (const auto& zone : {copperId, indigoId}) {
            const auto path = PropertyPath::parse(std::string("@") + zone + "." + commonLens + "." + levelPath);
            size_t start = 0;
            assert(resolveLawRoot(common, path, start) == &common);
        }

        // Explicit child membership keeps an Object definition in the child.
        auto& explicitlyShared = object(root, "confluence-common-plinth");
        explicitlyShared.addZoneDesignation(copperId);
        const auto designatedSnapshot = zoneToJson(copper);
        assert(designatedSnapshot.at("world").at("objects").size() == 1);
        assert(designatedSnapshot.at("world").at("objects")[0].at("objectID") ==
            explicitlyShared.getIdentifier());
        explicitlyShared.removeZoneDesignation(copperId);

        // Saving a child with an inherited viewport must never fork the cell
        // or mint a second copy of any of the parent definitions.
        auto local = std::make_shared<Object>("confluence-local-save-control");
        copper.addObject(local);
        assert(manager.persistZone(copperIndex));
        const auto localSnapshot = SaveSystem::readZoneIdentity(copperId);
        const auto& localObjects = localSnapshot.at("world").at("objects");
        assert(localObjects.size() == 1 && localObjects[0].at("objectID") == local->getIdentifier());
        assert(copper.removeObject(local.get()));
        assert(!manager.persistZone(copperIndex)); // inherited objects cannot mask erasure
        assert(manager.persistActiveZone());
        for (const auto& zone : {copperId, indigoId}) {
            const auto stored = SaveSystem::readZoneIdentity(zone);
            const size_t expected = std::string(zone) == copperId ? 1 : 0;
            assert(stored.at("world").at("objects").size() == expected);
            assert(stored.at("extent").at("hi") == 4.0);
        }
        const auto before = SaveSystem::readZoneIdentity(courtId);
        assert(manager.persistZone(rootIndex));
        const auto stored = SaveSystem::readZoneIdentity(courtId);
        assert(stored.at("world").at("objects").size() == root.objects().size());
        assert(stored.at("authors") == before.at("authors"));
        assert(stored.at("injected_by") == before.at("injected_by"));
    }
    {
        TestSupport::BootedEngineHarness app("Zach");
        admitAuthor(app);
        auto& manager = app.zones;
        assert(manager.switchTo(index(manager, courtId)));
        auto& root = manager.active();
        auto& west = object(root, copperLens);
        auto& east = object(root, indigoLens);
        auto& common = object(root, commonLens);
        assert(located(manager, common, *manager.zones()[index(manager, copperId)]));
        assert(located(manager, common, *manager.zones()[index(manager, indigoId)]));
        app.lawManager.tick();
        checkLenses(west, east, common, 1.0);
        assert(manager.switchTo(index(manager, indigoId)));
        assert(&object(manager.active(), commonLens) == &common);
        click(app, east);
        checkLenses(west, east, common, 0.0);
    }
    std::cout << "Confluence Court passed Zone hydration, overlap, pointer input and save/reload.\n";
}
