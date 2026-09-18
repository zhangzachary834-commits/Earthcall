// Home / Zone manifesto — what one pass can hold.
//
// EarthcallOurverse.md: a Home is a Zone whose telos is dwelling; every
// Person has one they fully own (kernel-locked); they may author more;
// Relationships and Communities own local Zones; gathering stays unowned.
// Not implemented here: Second-Person will / forced entry (⚑ AUTHOR).

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Identity/SingularId.hpp"
#include "Person/Person.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include "json.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

extern MaterialManager materials;

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

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Home / Zone ontology (manifesto, one pass)...\n";
    std::cout << "============================================================\n";

    if (!glfwInit()) {
        std::cout << "zone_home_ontology_test: glfwInit failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "zone_home_ontology_test", nullptr, nullptr);
    if (!window) {
        std::cout << "zone_home_ontology_test: no GL context\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_home_ontology";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Zach");
    Body body("humanoid", "default");
    Person zach(std::move(soul), std::move(body), "default");
    const std::string zachId = zach.getIdentifier();

    ZoneManager mgr;
    mgr.bindLive();

    auto workshop = std::make_shared<Zone>("Workshop", "strict");
    workshop->setOwner(zachId, Zone::kOwnerKindPerson);
    mgr.addZone(workshop);
    check(mgr.findPrimaryHome(zachId) == nullptr,
          "owning Workshop is not having a Home");

    mgr.ensureHomeZone(zachId);
    Zone* home = mgr.findPrimaryHome(zachId);
    check(home != nullptr, "ensureHomeZone mints a primary Home even if the Person already owns a Zone");
    check(home && home->getIdentifier() == "Home", "first Person's primary Home keeps the Home slug");
    check(home && dynamic_cast<Home*>(home) != nullptr,
          "the primary dwelling is a Home, not a Zone wearing a kind string");
    check(home && home->isPrimaryHome() && home->isPersonalHome(),
          "the minted Home is kind=home and primary");
    check(home && home->owner() == zachId, "primary Home is owned by the Person");
    check(home && home->propOwnerKind() == Zone::kOwnerKindPerson, "ownerKind is person");
    check(home && !home->isDeletable(zachId), "primary Home is not deletable");
    check(sizeof(Home) > sizeof(Zone),
          "Home's memory layout is larger than Zone — dwelling state is not a label");
    auto* dwelling = dynamic_cast<Home*>(home);
    check(dwelling && dwelling->hasStake(zachId),
          "the owner holds a stake in the Home's own memory");
    check(dwelling && dwelling->entryRequiresWill() && dwelling->cannotForceStay(),
          "will-to-enter and cannot-force-stay live on Home, not on Zone");
    check(dwelling && !dwelling->admitInhabitant("stranger", false),
          "a stranger without will is refused at the Home");
    check(dwelling && dwelling->admitInhabitant("guest", true),
          "will admits a guest into the dwelling's inhabitant list");
    check(dwelling && !dwelling->releaseInhabitant("guest", false),
          "cannotForceStay refuses expulsion; the guest may leave");
    check(dwelling && dwelling->releaseInhabitant("guest", true),
          "the inhabitant may leave of their own will");

    home->setOwner("thief", Zone::kOwnerKindPerson);
    check(home->owner() == zachId, "setOwner cannot transfer a claimed primary Home");
    home->setOwner("parish", Zone::kOwnerKindCommunity);
    check(home->owner() == zachId, "a Community cannot take a Person's primary Home");
    home->setQuality("primary", "false");
    check(home->isPrimaryHome(), "setQuality cannot demote the primary Home");
    home->setQuality("kind", Zone::kCommunityHomeKind);
    check(home->isPersonalHome() && !home->isCommunityHome(),
          "setQuality cannot re-kind the primary Home");

    Ourverse ourverse;
    Zone& gathering = ourverse.ensureGatheringZone(mgr);
    gathering.setOwner(zachId, Zone::kOwnerKindPerson);
    check(gathering.owner().empty(), "gathering refuses a Person owner");
    gathering.setOwner("parish", Zone::kOwnerKindCommunity);
    check(gathering.owner().empty(), "gathering refuses a Community owner too");

    auto extra = mgr.authorZone("Garden", zachId, Zone::kHomeKind, Zone::kOwnerKindPerson);
    check(extra != nullptr, "authorZone mints an extra Home");
    check(extra && dynamic_cast<Home*>(extra.get()) != nullptr,
          "an extra Home is still a Home — dwelling, not an ordinary Zone");
    check(extra && extra->isPersonalHome() && !extra->isPrimaryHome(),
          "an extra Home is kind=home and is not the kernel lock");
    check(extra && extra->owner() == zachId, "the extra Home is owned by the Person");
    check(std::filesystem::exists(sandbox / "homes" / "Home" / "home.json"),
          "primary Home serializes under saves/homes/, not saves/zones/");
    check(std::filesystem::exists(sandbox / "homes" / "Garden" / "home.json"),
          "an extra Home also lives in the homes identity store");
    check(!std::filesystem::exists(sandbox / "zones" / "Home" / "zone.json"),
          "a Home is not written as a Zone identity file");
    check(mgr.authorZone("Home", zachId, Zone::kHomeKind, Zone::kOwnerKindPerson) == nullptr,
          "authorZone refuses the primary Home slug");
    check(mgr.authorZone("Ourverse Gathering", zachId, Zone::kGatheringKind,
                         Zone::kOwnerKindPerson) == nullptr,
          "authorZone refuses to mint a gathering place");

    Community parish("parish");
    auto parishHome = mgr.authorZone("Parish Home", parish.getIdentifier(),
                                     Zone::kCommunityHomeKind, Zone::kOwnerKindCommunity);
    check(parishHome && dynamic_cast<Home*>(parishHome.get()) != nullptr
              && parishHome->isCommunityHome(),
          "Community Home is a Home owned by a Community — not a CommunityHome class");
    check(parishHome && parishHome->owner() == parish.getIdentifier()
              && parishHome->propOwnerKind() == Zone::kOwnerKindCommunity,
          "Community Home is owned by the Community");
    parishHome->setOwner(zachId, Zone::kOwnerKindPerson);
    check(parishHome->owner() == parish.getIdentifier(),
          "a Person cannot seize a Community Home");

    auto commons = mgr.authorZone("Commons", parish.getIdentifier(),
                                  Zone::kCommunityZoneKind, Zone::kOwnerKindCommunity);
    check(commons && dynamic_cast<Home*>(commons.get()) == nullptr
              && commons->isCommunityZone() && !commons->isHome(),
          "Community Zone is a Zone, not a Home");

    auto cottage = mgr.authorZone("Cottage", "marriage-1", "", Zone::kOwnerKindRelationship);
    check(cottage && cottage->owner() == "marriage-1"
              && cottage->propOwnerKind() == Zone::kOwnerKindRelationship
              && !cottage->isHome(),
          "a Relationship may own an ordinary Zone");

    check(mgr.forkZone("Home", "Home.garden"), "primary Home can be forked as a named branch");
    Zone* garden = nullptr;
    for (auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == "Home.garden") garden = z.get();
    }
    check(garden && garden->isPersonalHome() && !garden->isPrimaryHome(),
          "a fork of the primary Home is an extra dwelling, not a second lock");
    check(home && home->isPrimaryHome(), "the original primary Home is still locked");

    ActionNode mint = ActionNode::authorZone("Law Garden", Zone::kHomeKind, "", Zone::kOwnerKindPerson);
    ActionNode restored = ActionNode::fromJson(mint.toJson());
    check(restored.kind == ActionNode::Kind::AuthorZone
              && restored.describe() == mint.describe(),
          "AuthorZone is law text and survives JSON");
    ECA::Event ev{"zone-wanted", &zach, nullptr, 0};
    restored.compile()(ev, zach);
    bool sawLawGarden = false;
    for (const auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == "Law Garden" && z->isPersonalHome()
            && z->owner() == zachId && !z->isPrimaryHome()) {
            sawLawGarden = true;
        }
    }
    check(sawLawGarden, "AuthorZone action mints an extra Home owned by the law's subject");

    {
        auto cube = std::make_shared<Object>();
        cube->setShapeKind(Object::ShapeKind::Cube);
        cube->setObjectID("painted-home-cube");
        for (int f = 0; f < cube->getFaces(); ++f)
            cube->setFaceColor(f, 0.25f, 0.50f, 0.75f);
        home->addObject(cube);
        mgr.persistZones();
        const std::string mid = cube->materialId();
        auto painted = materials.get(mid);
        check(painted && !painted->faceTextures.empty(),
              "generated/painted shape owns a Material with FaceTextures");
        const unsigned char red = painted->faceTextures[0].pixels.empty()
            ? 0 : painted->faceTextures[0].pixels[0];
        materials.loadFromJson(nlohmann::json::array());
        check(!materials.get(mid),
              "a session REPLACE (the old load) drops the Home's material");
        nlohmann::json hj = SaveSystem::readHomeIdentity("Home");
        check(hj.contains("materials") && hj["materials"].is_array() && !hj["materials"].empty(),
              "the Home identity file carries the materials its objects name");
        materials.mergeFromJson(hj["materials"]);
        auto restored = materials.get(mid);
        check(restored && !restored->faceTextures.empty() &&
                  restored->faceTextures[0].pixels[0] == red,
              "merging Home identity materials restores FaceTextures after a session wipe");

        // Keep-live Home + hydrate (the in-app "load another save, walk to Home" path).
        materials.loadFromJson(nlohmann::json::array());
        mgr.hydrateFromZoneStore();
        auto fromHydrate = materials.get(mid);
        check(fromHydrate && !fromHydrate->faceTextures.empty() &&
                  fromHydrate->faceTextures[0].pixels[0] == red,
              "hydrate of a live Home restores FaceTextures from the identity file");

        // Pre-embed identity: objects and faceColors, no materials array.
        // After a session wipe the named own-material is gone and draw
        // would resolve material.default (white). Reinstatement fills
        // from the object's still-present faceColors.
        hj.erase("materials");
        SaveSystem::writeHomeIdentity("Home", hj);
        materials.loadFromJson(nlohmann::json::array());
        check(!materials.get(mid), "identity without materials cannot merge paint");
        mgr.hydrateFromZoneStore();
        auto fromFaces = materials.get(mid);
        check(fromFaces && !fromFaces->faceTextures.empty() &&
                  fromFaces->faceTextures[0].pixels[0] == red,
              "keep-live Home with no identity materials reinstates FaceTextures from faceColors");
    }

    // P0 inhabitability witness: ownership is a Relation to the Person being,
    // survives a complete manager/process-shaped reload, and does not fall back
    // to display spelling once the Person has a stable cryptographic identity.
    {
        auto continuitySandbox = std::filesystem::temp_directory_path()
            / "earthcall_home_identity_continuity";
        std::filesystem::remove_all(continuitySandbox);
        std::filesystem::create_directories(continuitySandbox);
        SaveSystem::setSaveRoot(continuitySandbox.string());

        std::array<uint8_t, 32> keyBytes{};
        for (std::size_t i = 0; i < keyBytes.size(); ++i) {
            keyBytes[i] = static_cast<uint8_t>(i + 1);
        }
        const Identity::SingularId continuityId =
            Identity::SingularId::fromPublicKey(keyBytes);

        {
            Soul firstSoul("Continuity Person");
            Body firstBody("humanoid", "default");
            Person firstPerson(std::move(firstSoul), std::move(firstBody), "default");

            ZoneManager firstProcess;
            Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
                beings.push_back(&firstPerson);
            });
            check(firstProcess.ensureHomeZone(firstPerson),
                  "Person-aware ensureHomeZone establishes ownership by identity");
            Zone* firstHome = firstProcess.findPrimaryHome(firstPerson);
            check(firstHome && firstHome->getIdentifier() == "Home",
                  "identity-aware Home creation still uses the canonical Home identity");
            check(firstHome && firstHome->owner() == firstPerson.getDisplayName(),
                  "legacy Home starts with the display spelling only as a compatibility cache");

            const std::size_t beforeStableId = firstProcess.zones().size();
            firstPerson.setPersonId(continuityId);
            check(firstProcess.findPrimaryHome(firstPerson) == firstHome,
                  "owned-by still resolves the same Person after a stable id is assigned");
            check(firstProcess.ensureHomeZone(firstPerson),
                  "stable id assignment reuses the existing Home");
            check(firstProcess.zones().size() == beforeStableId,
                  "stable id assignment does not mint another Home");

            bool liveOwnedBy = false;
            if (firstHome) {
                for (const auto& relation : firstHome->getFormation().relations().getAll()) {
                    if (relation && relation->type == "owned-by" && relation->directed
                        && relation->a() == firstHome && relation->b() == &firstPerson) {
                        liveOwnedBy = true;
                    }
                }
            }
            check(liveOwnedBy,
                  "primary Home carries a directed Zone -> owned-by -> Person Relation");

            firstProcess.persistZones();
            const nlohmann::json savedHome = SaveSystem::readHomeIdentity("Home");
            bool savedOwnedBy = false;
            for (const auto& relation : savedHome.value("formationRelations", nlohmann::json::array())) {
                if (relation.value("type", std::string{}) == "owned-by"
                    && relation.value("entityA", std::string{}) == "Home"
                    && relation.value("entityB", std::string{}) == continuityId.toString()
                    && relation.value("directed", false)) {
                    savedOwnedBy = true;
                }
            }
            check(savedOwnedBy,
                  "owned-by persists with the Person SingularId rather than a display spelling");
            check(savedHome.value("owner", std::string{}) == continuityId.toString(),
                  "saved owner cache follows the owned-by Person identity");
            Universe::instance().setProvider({});
        }

        {
            Soul returnedSoul("Renamed Display");
            Body returnedBody("humanoid", "default");
            Person returnedPerson(std::move(returnedSoul), std::move(returnedBody), "default");
            returnedPerson.setPersonId(continuityId);

            ZoneManager returnedProcess;
            Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
                beings.push_back(&returnedPerson);
            });
            returnedProcess.hydrateFromZoneStore();
            Zone* returnedHome = returnedProcess.findPrimaryHome(returnedPerson);
            check(returnedHome && returnedHome->getIdentifier() == "Home",
                  "fresh hydration resolves the same Home through Person identity after display rename");
            const std::size_t zoneCountBeforeEnsure = returnedProcess.zones().size();
            check(returnedProcess.ensureHomeZone(returnedPerson),
                  "fresh-process ensureHomeZone accepts the hydrated identity relation");
            check(returnedProcess.zones().size() == zoneCountBeforeEnsure,
                  "fresh-process ensureHomeZone mints no duplicate Home");
            Universe::instance().setProvider({});
        }

        {
            Soul ambiguousSoul("Ambiguous Person");
            Body ambiguousBody("humanoid", "default");
            Person ambiguousPerson(std::move(ambiguousSoul), std::move(ambiguousBody), "default");
            const std::string ambiguousId = ambiguousPerson.getIdentifier();

            ZoneManager ambiguous;
            auto primaryA = std::make_shared<Home>("PrimaryA", "strict");
            primaryA->markPrimaryHome();
            primaryA->setOwner(ambiguousId, Zone::kOwnerKindPerson);
            auto primaryB = std::make_shared<Home>("PrimaryB", "strict");
            primaryB->markPrimaryHome();
            primaryB->setOwner(ambiguousId, Zone::kOwnerKindPerson);
            ambiguous.addZone(primaryA);
            ambiguous.addZone(primaryB);

            const std::size_t beforeRefusal = ambiguous.zones().size();
            check(ambiguous.findPrimaryHome(ambiguousPerson) == nullptr,
                  "two primary Homes refuse resolution instead of choosing by load order");
            check(!ambiguous.ensureHomeZone(ambiguousPerson),
                  "two primary Homes refuse ensureHomeZone loudly");
            check(ambiguous.zones().size() == beforeRefusal,
                  "duplicate-primary refusal never mints a third Home");
        }

        Universe::instance().setProvider({});
        std::filesystem::remove_all(continuitySandbox);
        SaveSystem::setSaveRoot(sandbox.string());
    }

    PropertyValue v;
    check(PropertyPath::parse("kind").getValue(*home, v) == PropertyPath::PathResult::Ok
              && std::get<std::string>(v) == Zone::kHomeKind,
          "kind is registered (refusal #6)");
    check(PropertyPath::parse("primary").getValue(*home, v) == PropertyPath::PathResult::Ok
              && std::get<bool>(v) == true,
          "primary is registered");
    check(PropertyPath::parse("ownerKind").getValue(*home, v) == PropertyPath::PathResult::Ok,
          "ownerKind is registered");
    check(PropertyPath::parse("owner").setValue(*home, PropertyValue(std::string("thief")))
              != PropertyPath::PathResult::Ok,
          "owner stays read-only on the property path");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_home_ontology_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_home_ontology_test: ALL OK\n";
    return 0;
}
