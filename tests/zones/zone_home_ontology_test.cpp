// Home / Zone manifesto — what one pass can hold.
//
// EarthcallOurverse.md: a Home is a Zone whose telos is dwelling; every
// Person has one they fully own (kernel-locked); they may author more;
// Relationships and Communities own local Zones; gathering stays unowned.
// Not implemented here: Second-Person will / forced entry (⚑ AUTHOR).

#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <filesystem>
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

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Home / Zone ontology (manifesto, one pass)...\n";
    std::cout << "============================================================\n";

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
    check(home && home->isPrimaryHome() && home->isPersonalHome(),
          "the minted Zone is kind=home and primary");
    check(home && home->owner() == zachId, "primary Home is owned by the Person");
    check(home && home->propOwnerKind() == Zone::kOwnerKindPerson, "ownerKind is person");
    check(home && !home->isDeletable(zachId), "primary Home is not deletable");

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
    check(extra && extra->isPersonalHome() && !extra->isPrimaryHome(),
          "an extra Home is kind=home and is not the kernel lock");
    check(extra && extra->owner() == zachId, "the extra Home is owned by the Person");
    check(mgr.authorZone("Home", zachId, Zone::kHomeKind, Zone::kOwnerKindPerson) == nullptr,
          "authorZone refuses the primary Home slug");
    check(mgr.authorZone("Ourverse Gathering", zachId, Zone::kGatheringKind,
                         Zone::kOwnerKindPerson) == nullptr,
          "authorZone refuses to mint a gathering place");

    Community parish("parish");
    auto parishHome = mgr.authorZone("Parish Home", parish.getIdentifier(),
                                     Zone::kCommunityHomeKind, Zone::kOwnerKindCommunity);
    check(parishHome && parishHome->isCommunityHome(), "Community Home is an authored kind, not a C++ class");
    check(parishHome && parishHome->owner() == parish.getIdentifier()
              && parishHome->propOwnerKind() == Zone::kOwnerKindCommunity,
          "Community Home is owned by the Community");
    parishHome->setOwner(zachId, Zone::kOwnerKindPerson);
    check(parishHome->owner() == parish.getIdentifier(),
          "a Person cannot seize a Community Home");

    auto commons = mgr.authorZone("Commons", parish.getIdentifier(),
                                  Zone::kCommunityZoneKind, Zone::kOwnerKindCommunity);
    check(commons && commons->isCommunityZone() && !commons->isHome(),
          "Community Zone belongs to a Community and is not a Home");

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

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_home_ontology_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_home_ontology_test: ALL OK\n";
    return 0;
}
