// The Basic Pixel Changer, held to its authored save and its actual Sense-Act
// route: Zone activation -> stable Law root -> screen-space observation ->
// object-clicked -> Law -> Screen channel -> copy-on-write Material pixels.
// This test never invokes the legacy World loader or reconstructs the changer
// Law in C++, so deleting/drifting either authored identity makes it fail.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
int failures = 0;

struct TempSaveRoot {
    std::filesystem::path path;
    ~TempSaveRoot() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

void check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::printf("  FAILED: %s\n", message);
    } else {
        std::printf("  ok: %s\n", message);
    }
}

Object* findObject(Zone& zone, const std::string& id) {
    for (const auto& object : zone.getOwnedObjects()) {
        if (object && object->getIdentifier() == id) return object.get();
    }
    return nullptr;
}

glm::vec3 texel(const FaceTexture& texture, int x, int y) {
    const std::size_t offset = static_cast<std::size_t>(y * texture.size + x) * 4;
    return glm::vec3(texture.pixels[offset], texture.pixels[offset + 1],
                     texture.pixels[offset + 2]) / 255.0f;
}

bool near(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b) < 0.01f;
}
} // namespace

int main() {
    std::printf("=== Basic Pixel Changer: authored Law and property elevation ===\n");
    std::filesystem::path sourceRoot = "saves";
    if (!std::filesystem::exists(sourceRoot / "zones/BasicPixelChanger/zone.json") &&
        std::filesystem::exists("../saves/zones/BasicPixelChanger/zone.json")) {
        sourceRoot = "../saves";
    }
    const auto sourceZone = std::filesystem::absolute(
        sourceRoot / "zones/BasicPixelChanger/zone.json");
    const auto sourceLaw = std::filesystem::absolute(
        sourceRoot / "laws/law-basic-pixel-changer/law.json");
    if (!std::filesystem::exists(sourceZone) || !std::filesystem::exists(sourceLaw)) {
        std::fprintf(stderr, "basic_pixel_changer_test: authored Zone/Law root is missing\n");
        return 1;
    }
    TempSaveRoot isolated{
        std::filesystem::temp_directory_path() /
        ("earthcall-basic-pixel-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(isolated.path / "zones/BasicPixelChanger");
    std::filesystem::create_directories(isolated.path / "laws/law-basic-pixel-changer");
    std::filesystem::copy_file(sourceZone,
                               isolated.path / "zones/BasicPixelChanger/zone.json");
    std::filesystem::copy_file(sourceLaw,
                               isolated.path / "laws/law-basic-pixel-changer/law.json");
    SaveSystem::setSaveRoot(isolated.path.string());

    const auto emptyZone = [](const std::string& id) {
        return nlohmann::json{
            {"identifier", id}, {"name", id}, {"scope", "Local"},
            {"formationRelations", nlohmann::json::array()},
            {"lexemes", nlohmann::json::array()},
            {"materials", nlohmann::json::array()},
            {"world", {{"objects", nlohmann::json::array()}}}
        };
    };
    auto missingLawZone = emptyZone("MissingLawRoot");
    missingLawZone["lawRefs"] = {"law-that-does-not-exist"};
    check(SaveSystem::writeZoneIdentity("MissingLawRoot", missingLawZone),
          "missing-root refusal fixture written in isolated store");
    check(SaveSystem::writeZoneIdentity("NoLaws", emptyZone("NoLaws")),
          "law-free departure fixture written in isolated store");

    TestSupport::BootedEngineHarness harness("Zach");
    Singularity::Core::CreationChannel::syncRegister(harness.lawManager);
    Singularity::Screen::ScreenChannel::syncRegister(harness.lawManager);

    std::shared_ptr<Zone> zone;
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        if (harness.zones.zones()[i] &&
            findObject(*harness.zones.zones()[i], "basic-pixel-canvas")) {
            check(harness.zones.switchTo(i),
                  "Creator Console Zone activation resolves the complete Law closure");
            zone = harness.zones.zones()[i];
            break;
        }
    }
    assert(zone);
    Object* canvas = findObject(*zone, "basic-pixel-canvas");
    auto* creation = Singularity::Core::CreationChannel::find(harness.lawManager);
    auto* law = harness.lawManager.find("law-basic-pixel-changer");
    check(canvas != nullptr, "authored canvas loaded");
    check(creation != nullptr, "Creation channel supplies the selected color Property");
    check(law != nullptr && law->isAuthored(), "changer Law loaded with a recorded author");
    if (!canvas || !creation || !law) return 1;

    creation->activeColor = glm::vec3(0.10f, 0.70f, 0.25f);
    std::vector<Object*> reachable{canvas};
    const auto frame = [&](float x, float y, bool left) {
        Singularity::Input::InteractionChannel::Sense sense;
        sense.pointerX = x;
        sense.pointerY = y;
        sense.left = left;
        sense.rayOrigin = glm::vec3(0.0f, 0.0f, 10.0f);
        sense.rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        harness.interaction->observe(sense, reachable);
        harness.lawManager.tick();
    };

    // Canvas is (160,100)..(672,612).  This is precisely u=.25, v=.75,
    // which addresses texel (16,48) of the lazily-created 64x64 texture.
    frame(288.0f, 484.0f, false);
    check(std::fabs(harness.interaction->hoveredU - 0.25f) < 1e-5f &&
              std::fabs(harness.interaction->hoveredV - 0.75f) < 1e-5f,
          "2D Sense reports normalized surface coordinates");
    frame(288.0f, 484.0f, true);
    frame(288.0f, 484.0f, false);

    auto material = materials.get("material.basic-pixel-canvas");
    check(material != nullptr && material->faceTextures.size() == 1,
          "first pixel write gives the canvas its own one-face Material");
    if (!material || material->faceTextures.empty()) return 1;
    FaceTexture& texture = material->faceTextures[0];
    check(near(texel(texture, 16, 48), creation->activeColor),
          "click changes exactly the addressed texel to the selected color");
    check(near(texel(texture, 15, 48), glm::vec3(1.0f)),
          "neighboring texel remains unchanged");

    // A single sample becomes an ordinary, enumerable, writable Property only
    // when authored into the being.  No 4096-property explosion is required.
    ActionNode elevateOne = ActionNode::addProperty(
        "", "surface.pixel.0.3.4", PropertyValue(glm::vec3(0.2f, 0.3f, 0.9f)));
    elevateOne.compile()(ECA::Event{"test-pixel-elevation", canvas, nullptr,
                                    std::time(nullptr)}, *canvas);
    Property* pixel = canvas->findProperty("surface.pixel.0.3.4");
    check(pixel != nullptr, "one pixel can be elevated by authored AddProperty");
    check(pixel != nullptr && near(std::get<glm::vec3>(pixel->value()),
                                   glm::vec3(0.2f, 0.3f, 0.9f)),
          "elevated pixel reads live texture state");
    check(pixel && pixel->setValue(PropertyValue(glm::vec3(0.9f, 0.1f, 0.2f))) &&
              near(texel(texture, 3, 4), glm::vec3(0.9f, 0.1f, 0.2f)),
          "Property write reaches the same texture sample");

    // A named set is selected by authored OntoMath definedness.  This selector
    // defines the left quarter (u <= .25); there is no rectangle/region enum.
    OntoMath::Piecewise selector;
    selector.inputVariable = "u";
    OntoMath::Piecewise::Piece leftQuarter;
    leftQuarter.hasHi = true;
    leftQuarter.hi = 0.25;
    leftQuarter.includeHi = true;
    leftQuarter.mathNode = OntoMath::MathNode::fromLegacyExpression(
        OntoMath::ScalarForm::constant(1.0));
    selector.pieces.push_back(std::move(leftQuarter));
    canvas->setDynamicProperty("test.face", PropertyValue(0));
    ActionNode elevateRegion;
    elevateRegion.kind = ActionNode::Kind::ElevatePixels;
    elevateRegion.propertyName = "authored.left-quarter";
    elevateRegion.pixelFacePath = PropertyPath::parse("test.face");
    elevateRegion.mapFunction = selector;
    elevateRegion.compile()(ECA::Event{"test-region-elevation", canvas, nullptr,
                                       std::time(nullptr)}, *canvas);
    Property* region = canvas->findProperty("authored.left-quarter");
    check(region != nullptr,
          "authored ElevatePixels makes an OntoMath-defined set a named Property");
    const PropertyValue regionSnapshot = region ? region->value() : PropertyValue{};
    const auto* regionValue = std::get_if<std::shared_ptr<PropertyList>>(&regionSnapshot);
    check(regionValue && *regionValue && (*regionValue)->elements.size() == 16u * 64u,
          "named Property contains exactly the samples in OntoMath's defined set");
    check(region && region->setValue(PropertyValue(glm::vec3(0.05f, 0.15f, 0.85f))) &&
              near(texel(texture, 15, 20), glm::vec3(0.05f, 0.15f, 0.85f)) &&
              near(texel(texture, 16, 20), glm::vec3(1.0f)),
          "one Property write changes the selected set and no preset shape decides its bounds");

    nlohmann::json serializedCanvas = *canvas;
    check(serializedCanvas["authoredProperties"].contains("surface.pixel.0.3.4") &&
              serializedCanvas["authoredProperties"].contains("authored.left-quarter") &&
              serializedCanvas["authoredProperties"].contains(
                  "surface.selection.authored.left-quarter"),
          "pixel and OntoMath-set elevations persist as authored Properties");

    const ActionNode roundTrip = ActionNode::fromJson(law->actionModel()->toJson());
    check(roundTrip.kind == ActionNode::Kind::WritePixel &&
              roundTrip.pixelColorPath.toString() == "@creation-channel.activeColor",
          "authored WritePixel action round-trips without hidden defaults");
    const ActionNode regionRoundTrip = ActionNode::fromJson(elevateRegion.toJson());
    check(regionRoundTrip.kind == ActionNode::Kind::ElevatePixels &&
              regionRoundTrip.propertyName == "authored.left-quarter" &&
              regionRoundTrip.mapFunction.print() == selector.print(),
          "authored OntoMath pixel-set definition round-trips");

    harness.zones.persistZones();
    const nlohmann::json persistedZone = SaveSystem::readZoneIdentity("BasicPixelChanger");
    const nlohmann::json persistedLaw =
        SaveSystem::readLawIdentity("law-basic-pixel-changer");
    check(persistedZone.value("lawRefs", nlohmann::json::array()) ==
              nlohmann::json::array({"law-basic-pixel-changer"}),
          "Zone persistence preserves its authored stable Law reference");
    check(persistedLaw.value("identifier", std::string{}) == "law-basic-pixel-changer" &&
              persistedLaw.value("authors", nlohmann::json::array()) ==
                  nlohmann::json::array({"Zach"}) &&
              persistedLaw["law"].value("authors", nlohmann::json::array()) ==
                  nlohmann::json::array({"Zach"}) &&
              persistedLaw.value("triggers", nlohmann::json::array()) ==
                  nlohmann::json::array({"object-clicked"}),
          "shared Law root persists identity, Person author, and trigger");

    const std::size_t basicIndex = harness.zones.currentIndex();
    std::size_t missingIndex = harness.zones.zones().size();
    std::size_t noLawsIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& candidate = harness.zones.zones()[i];
        if (!candidate) continue;
        if (candidate->getIdentifier() == "MissingLawRoot") missingIndex = i;
        if (candidate->getIdentifier() == "NoLaws") noLawsIndex = i;
    }
    check(missingIndex < harness.zones.zones().size() &&
              !harness.zones.switchTo(missingIndex) &&
              harness.zones.currentIndex() == basicIndex &&
              harness.lawManager.find("law-basic-pixel-changer") != nullptr,
          "missing Law root refuses atomically and leaves the current Zone working");
    check(noLawsIndex < harness.zones.zones().size() &&
              harness.zones.switchTo(noLawsIndex) &&
              harness.lawManager.find("law-basic-pixel-changer") == nullptr,
          "leaving the Zone releases its Zone-scoped Law");
    check(harness.zones.switchTo(basicIndex) &&
              harness.lawManager.find("law-basic-pixel-changer") != nullptr,
          "re-entering the Zone restores and rebinds its shared Law root");

    std::printf("%s (%d failure%s)\n", failures ? "FAIL" : "PASS", failures,
                failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
