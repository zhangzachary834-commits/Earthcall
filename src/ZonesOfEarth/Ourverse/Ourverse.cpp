#include "Ourverse.hpp"
#include <iostream>
#include <cstdio>
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "Singularity/Language/JoyHierarchy.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <unordered_map>
#include <unordered_set>

extern ZoneManager mgr;

Ourverse::Ourverse() {
    _joys.setIdentifier("ourverse.joys");
    Singularity::Language::seedJoyHierarchy(
        _joys, Singularity::Language::LanguageSystem::instance().foundation().get());
    if (_joys.root()) setTelosId(_joys.root()->getIdentifier());
    _filaments.setIdentifier("ourverse.filaments");
    _filaments.setRelationTypeTag("filaments");
    _metalaws.setIdentifier("ourverse.metalaws");
}

void Ourverse::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Ourverse, std::string>>(
        "gatheringZone", this, &Ourverse::propGatheringZone, nullptr));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Ourverse, std::string>>(
        "joys", this, &Ourverse::propJoys, nullptr));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Ourverse, int>>(
        "filamentCount", this, &Ourverse::propFilamentCount, nullptr));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Ourverse, std::string>>(
        "metalaws", this, &Ourverse::propMetalaws, nullptr));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Ourverse, std::string>>(
        "convenesToward", this, &Ourverse::propConvenesToward, nullptr));
}

std::string Ourverse::propGatheringZone() const {
    return _gatheringZone ? _gatheringZone->getIdentifier() : std::string{};
}

int Ourverse::propFilamentCount() const {
    int n = 0;
    for (const auto& rel : _filaments.relations().getAll()) {
        if (rel && rel->type == kFilamentType) ++n;
    }
    return n;
}

void Ourverse::setPrimaryGatheringZone(std::shared_ptr<Zone> zone) {
    if (!zone) {
        _gatheringZone.reset();
        return;
    }
    if (zone->isHome()) {
        std::fprintf(stderr,
            "Ourverse: REFUSED gathering Zone '%s' — a Home is a dwelling, "
            "not a gathering place (EarthcallOurverse.md / OURVERSE.md).\n",
            zone->name().c_str());
        return;
    }
    if (!zone->owner().empty() && !zone->isOurverseGathering()) {
        std::fprintf(stderr,
            "Ourverse: REFUSED gathering Zone '%s' — it is already owned. "
            "No one owns the gathering place (OURVERSE.md).\n",
            zone->name().c_str());
        return;
    }
    zone->markOurverseGathering();
    _gatheringZone = std::move(zone);
}

Zone& Ourverse::ensureGatheringZone(ZoneManager& zones) {
    if (_gatheringZone) return *_gatheringZone;
    for (auto& zone : zones.zones()) {
        if (zone && zone->isOurverseGathering()) {
            _gatheringZone = zone;
            return *_gatheringZone;
        }
    }
    auto gathering = std::make_shared<Zone>("Ourverse Gathering", "default");
    gathering->markOurverseGathering();
    gathering->setScope(Zone::Scope::Global);
    zones.addZone(gathering);
    _gatheringZone = gathering;
    return *_gatheringZone;
}

bool Ourverse::ensureCommunityGathering(Community& community) {
    if (!_gatheringZone) return false;
    _filaments.addMember(_gatheringZone.get());
    _filaments.addMember(&community);
    auto rel = std::make_shared<Relation>(
        kGathersType, *_gatheringZone, community, false);
    return _filaments.addRelation(rel);
}

bool Ourverse::mayWeave(const Zone& a, const Zone& b) const {
    if (&a == &b) return false;
    if (a.getIdentifier() == b.getIdentifier()) return false;
    return true;
}

bool Ourverse::weave(Zone& a, Zone& b) {
    if (!mayWeave(a, b)) {
        std::fprintf(stderr,
            "Ourverse: REFUSED filament %s <-> %s. A Zone does not filament "
            "itself (OURVERSE.md).\n",
            a.getIdentifier().c_str(), b.getIdentifier().c_str());
        return false;
    }
    _filaments.addMember(&a);
    _filaments.addMember(&b);
    auto rel = std::make_shared<Relation>(kFilamentType, a, b, false);
    return _filaments.addRelation(rel);
}

void Ourverse::registerMetalaws(LawManager& laws) {
    auto addIfMissing = [&](const std::string& id, const std::string& name) {
        if (laws.find(id)) return;
        auto law = std::make_shared<FirstMoverLaw>(name);
        law->setLawIdentifier(id);
        law->addAuthor(*this);
        law->setActivation(Law::Activation::WhileTrue);
        // OFF until one of these actually does something. Both are declarations
        // with no conditions and no actions -- they exist to make a ceiling the
        // Kernel already enforces legible and metalaw-addressable. But a
        // WhileTrue law with no compiled Rete terminals takes the FULL SWEEP
        // path in LawManager::tick(), over every being, every frame, to decide
        // nothing. Zach measured the pair at 20-30 ms together, which is what
        // was capping the frame rate at 20-40 fps; disabling them let it reach
        // 200-600 (or 60, vsync-bound). Rendering was never the cost -- a whole
        // optimization campaign chased this, see
        // docs/audits/RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md.
        //
        // This sets the DEFAULT only. A world that recorded a choice in its
        // firstMoverEnabled map still wins on load, in either direction: the
        // Person's answer outranks ours. Turn this back to enabled when these
        // laws carry conditions and actions of their own.
        law->setEnabled(false);
        laws.add(law);
        _metalaws.addMember(law.get());
    };
    // Kernel already refuses gathering ownership and directed filaments.
    // These first-movers make that ceiling legible and metalaw-addressable.
    addIfMissing("ourverse-gathering-unowned", "ourverse: gathering unowned");
    addIfMissing("ourverse-filaments-mutual", "ourverse: filaments are mutual");
}

