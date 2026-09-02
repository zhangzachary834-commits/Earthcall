#include "HomesOfEarth/Home.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>

Home::Home(const std::string& name, const std::string& foundationSymbol, Scope scope)
    : Zone(name, foundationSymbol, scope)
{
    setQuality("kind", kHomeKind);
    _stakes.setIdentifier(name + ".stakes");
}

void Home::buildProperties() {
    Zone::buildProperties();
    // `primary` is already registered on Zone via virtual isPrimaryHome.
    // These are Home-only memory — they do not exist on Zone.
    registerProperty(std::make_unique<ComputedProperty<Home, bool>>(
        "entryRequiresWill", this, &Home::propEntryRequiresWill));
    registerProperty(std::make_unique<ComputedProperty<Home, bool>>(
        "cannotForceStay", this, &Home::propCannotForceStay));
    registerProperty(std::make_unique<ComputedProperty<Home, std::string>>(
        "stakes", this, &Home::propStakes, nullptr));
    registerProperty(std::make_unique<ComputedProperty<Home, int>>(
        "stakeCount", this, &Home::propStakeCount));
    registerProperty(std::make_unique<ComputedProperty<Home, int>>(
        "inhabitantCount", this, &Home::propInhabitantCount));
}

bool Home::isPrimaryHome() const {
    if (isCommunityHome()) return false;
    return _primary;
}

void Home::markPrimaryHome() {
    if (isOurverseGathering()) {
        std::fprintf(stderr,
            "Home '%s': REFUSED to become primary. A gathering place is not a "
            "dwelling (EarthcallOurverse.md).\n",
            name().c_str());
        return;
    }
    _primary = true;
    Zone::markPrimaryHome();
}

void Home::markCommunityHome() {
    if (_primary) {
        std::fprintf(stderr,
            "Home '%s': REFUSED community-home. A Person's primary Home "
            "cannot be re-kinded.\n",
            name().c_str());
        return;
    }
    _primary = false;
    Zone::markCommunityHome();
}

void Home::setOwner(const std::string& ownerId) {
    std::string kind = propOwnerKind();
    if (kind.empty() && !ownerId.empty()) {
        kind = isCommunityHome() ? kOwnerKindCommunity : kOwnerKindPerson;
    }
    setOwner(ownerId, kind);
}

void Home::setOwner(const std::string& ownerId, const std::string& ownerKind) {
    if (_primary) {
        if (ownerKind == kOwnerKindRelationship || ownerKind == kOwnerKindCommunity) {
            std::fprintf(stderr,
                "Home '%s': REFUSED owner '%s' (%s). A Person's primary Home "
                "is fully owned by that Person (EarthcallOurverse.md).\n",
                name().c_str(), ownerId.c_str(), ownerKind.c_str());
            return;
        }
        if (!owner().empty() && ownerId != owner()) {
            std::fprintf(stderr,
                "Home '%s': REFUSED to transfer primary Home from '%s' to "
                "'%s'. Highest ownership priority is kernel-locked.\n",
                name().c_str(), owner().c_str(), ownerId.c_str());
            return;
        }
    }
    if (isCommunityHome() && !ownerId.empty()
        && ownerKind != kOwnerKindCommunity) {
        std::fprintf(stderr,
            "Home '%s': REFUSED owner '%s' (%s). A Community Home is owned "
            "by a Community — shared stakes, not a Person seizure.\n",
            name().c_str(), ownerId.c_str(), ownerKind.c_str());
        return;
    }
    Zone::setOwner(ownerId, ownerKind);
    if (!ownerId.empty()) addStake(ownerId);
    if (!ownerId.empty()) admitInhabitant(ownerId, true);
}

void Home::setQuality(const std::string& key, const std::string& value) {
    if (_primary && (key == "primary" || key == "kind")) {
        const bool demotePrimary = (key == "primary" && value != "true");
        const bool demoteKind = (key == "kind" && value != kHomeKind);
        if (demotePrimary || demoteKind) {
            std::fprintf(stderr,
                "Home '%s': REFUSED to change %s. Primary Home memory is "
                "kernel-locked.\n",
                name().c_str(), key.c_str());
            return;
        }
    }
    Zone::setQuality(key, value);
}

void Home::setDeletable(const std::string& person, bool flag) {
    if (_primary && flag) {
        std::fprintf(stderr,
            "Home '%s': REFUSED deletable. A Person's primary Home is not "
            "erased (EarthcallOurverse.md).\n",
            name().c_str());
        return;
    }
    Zone::setDeletable(person, flag);
}

bool Home::isDeletable(const std::string& person) const {
    if (_primary) return false;
    return Zone::isDeletable(person);
}

bool Home::hasStake(const std::string& beingId) const {
    return std::find(_stakeIds.begin(), _stakeIds.end(), beingId) != _stakeIds.end();
}

void Home::addStake(const std::string& beingId) {
    if (beingId.empty() || hasStake(beingId)) return;
    _stakeIds.push_back(beingId);
}

void Home::loadStakeIds(std::vector<std::string> ids) {
    _stakeIds = std::move(ids);
}

bool Home::admitInhabitant(const std::string& personId, bool will) {
    if (personId.empty()) return false;
    if (std::find(_inhabitants.begin(), _inhabitants.end(), personId) != _inhabitants.end())
        return true;
    const bool ownerOrStake = (personId == owner()) || hasStake(personId);
    if (_entryRequiresWill && !will && !ownerOrStake) {
        std::fprintf(stderr,
            "Home '%s': REFUSED inhabitant '%s'. Nobody forces themselves "
            "into another's Home apart from will (EarthcallOurverse.md).\n",
            name().c_str(), personId.c_str());
        return false;
    }
    _inhabitants.push_back(personId);
    return true;
}

bool Home::releaseInhabitant(const std::string& personId, bool bySelf) {
    auto it = std::find(_inhabitants.begin(), _inhabitants.end(), personId);
    if (it == _inhabitants.end()) return true;
    if (_cannotForceStay && !bySelf) {
        std::fprintf(stderr,
            "Home '%s': REFUSED to expel '%s'. A Home's laws cannot treat "
            "someone's relation to their own dwelling as expendable.\n",
            name().c_str(), personId.c_str());
        return false;
    }
    _inhabitants.erase(it);
    return true;
}

void Home::loadInhabitantIds(std::vector<std::string> ids) {
    _inhabitants = std::move(ids);
}

void Home::welcome() const {
    const std::string& who = owner();
    if (!who.empty()) {
        std::cout << "Welcome to " << who << "'s home." << std::endl;
    } else {
        std::cout << "Welcome home." << std::endl;
    }
}
