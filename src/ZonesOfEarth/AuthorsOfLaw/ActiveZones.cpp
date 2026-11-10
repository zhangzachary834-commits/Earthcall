#include "ZonesOfEarth/AuthorsOfLaw/ActiveZones.hpp"

#include "ConstructedBeing/Singular/Singular.hpp"
#include "Relation/Relation.hpp"
#include "Time/timeline.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <algorithm>
#include <unordered_set>

void ActiveZones::setPrimaryZone(Zone* zone) {
    if (!zone) {
        if (!_zones.empty()) {
            _zones.erase(_zones.begin());
            bumpStructuralRevision();
        }
        return;
    }
    if (_zones.empty()) {
        _zones.push_back(zone);
        bumpStructuralRevision();
        return;
    }
    if (_zones.front() == zone) {
        return;
    }
    auto it = std::find(_zones.begin() + 1, _zones.end(), zone);
    if (it != _zones.end()) {
        _zones.erase(it);
    }
    _zones[0] = zone;
    bumpStructuralRevision();
}

void ActiveZones::setActiveZones(std::vector<Zone*> zones) {
    std::vector<Zone*> filtered;
    std::unordered_set<Zone*> seen;
    for (Zone* z : zones) {
        if (z && seen.insert(z).second) {
            filtered.push_back(z);
        }
    }
    if (_zones == filtered) return;
    _zones = std::move(filtered);
    bumpStructuralRevision();
}

bool ActiveZones::activateZone(Zone* zone) {
    if (!zone) return false;
    for (Zone* z : _zones) {
        if (z == zone) return false;
    }
    _zones.push_back(zone);
    bumpStructuralRevision();
    return true;
}

bool ActiveZones::deactivateZone(Zone* zone) {
    if (!zone) return false;
    auto it = std::find(_zones.begin(), _zones.end(), zone);
    if (it == _zones.end()) return false;
    _zones.erase(it);
    bumpStructuralRevision();
    return true;
}

void ActiveZones::clearActiveZones() {
    if (!_zones.empty()) {
        _zones.clear();
        bumpStructuralRevision();
    }
}

bool ActiveZones::isActive(const Zone* zone) const noexcept {
    if (!zone) return false;
    for (const Zone* z : _zones) {
        if (z == zone) return true;
    }
    return false;
}

std::vector<Singular*> ActiveZones::beings() const {
    std::vector<Singular*> out;
    std::unordered_set<Singular*> seen;

    auto appendBeing = [this, &out, &seen](Singular* s) {
        if (s && !isUnmade(s) && seen.insert(s).second) {
            out.push_back(s);
        }
    };

    for (Zone* zone : _zones) {
        if (!zone) continue;
        appendBeing(zone);
        if (auto* field = zone->spatialRoot()) {
            appendBeing(field);
        }
        for (const auto& field : zone->additionalSpatialFields()) {
            if (field) appendBeing(field.get());
        }
        for (const auto& obj : zone->getOwnedObjects()) {
            if (obj) appendBeing(obj.get());
        }
        for (const auto& rel : zone->formation().relations().getAll()) {
            if (rel) appendBeing(rel.get());
        }
        for (auto* member : zone->formation().getMembers()) {
            appendBeing(member);
        }
        for (const auto& stored : zone->storedSingulars()) {
            if (stored) appendBeing(stored.get());
        }
    }

    if (_provider) {
        std::vector<Singular*> extra;
        _provider(extra);
        for (Singular* s : extra) {
            appendBeing(s);
        }
    }

    if (!_unmaking.empty()) {
        out.erase(std::remove_if(out.begin(), out.end(),
                                 [this](Singular* being) { return isUnmade(being); }),
                  out.end());
    }

    return out;
}

std::vector<Relation*> ActiveZones::relations() const {
    std::vector<Relation*> out;
    std::unordered_set<Relation*> seen;

    for (Zone* zone : _zones) {
        if (!zone) continue;
        for (const auto& rel : zone->formation().relations().getAll()) {
            if (rel && seen.insert(rel.get()).second) {
                out.push_back(rel.get());
            }
        }
    }

    if (_relationProvider) {
        std::vector<Relation*> extra;
        _relationProvider(extra);
        for (Relation* r : extra) {
            if (r && seen.insert(r).second) {
                out.push_back(r);
            }
        }
    }

    return out;
}

bool ActiveZones::relationsInvolving(const Singular& being, std::vector<Relation*>& out) const {
    if (_zones.empty() && !_relationsInvolvingProvider) return false;
    out.clear();
    std::unordered_set<Relation*> seen;
    std::vector<Relation*> buffer;

    for (Zone* zone : _zones) {
        if (!zone) continue;
        buffer.clear();
        zone->formation().relations().relationsInvolving(being, buffer);
        for (Relation* r : buffer) {
            if (r && seen.insert(r).second) {
                out.push_back(r);
            }
        }
    }

    if (_relationsInvolvingProvider) {
        buffer.clear();
        _relationsInvolvingProvider(being, buffer);
        for (Relation* r : buffer) {
            if (r && seen.insert(r).second) {
                out.push_back(r);
            }
        }
    }

    return true;
}

bool ActiveZones::hasRelationGeneration() const {
    return !_zones.empty() || static_cast<bool>(_relationGenerationProvider);
}

std::size_t ActiveZones::relationGeneration() const {
    if (!hasRelationGeneration()) return 0;
    std::size_t stamp = 0;
    auto mix = [&stamp](std::size_t v) {
        stamp ^= v + 0x9e3779b97f4a7c15ULL + (stamp << 6) + (stamp >> 2);
    };

    for (Zone* zone : _zones) {
        if (!zone) continue;
        mix(reinterpret_cast<std::size_t>(&zone->formation().relations()));
        mix(zone->formation().relations().generation());
    }

    if (_relationGenerationProvider) {
        mix(_relationGenerationProvider());
    }

    return stamp;
}

bool ActiveZones::hasRelationRegistrar() const {
    return static_cast<bool>(_relationRegistrar) || !_zones.empty();
}

void ActiveZones::addRelation(std::shared_ptr<Relation> relation) {
    if (!relation) return;
    if (_relationRegistrar) {
        _relationRegistrar(std::move(relation));
        return;
    }
    if (!_zones.empty()) {
        Zone* targetZone = nullptr;
        if (relation->a()) {
            for (Zone* z : _zones) {
                if (z && z->containsBeing(*relation->a())) {
                    targetZone = z;
                    break;
                }
            }
        }
        if (!targetZone) {
            targetZone = primaryZone();
        }
        if (targetZone) {
            targetZone->formation().relations().add(std::move(relation));
        }
    }
}

void ActiveZones::setTimeline(Timeline* timelineBeing) {
    if (_timeline == timelineBeing) return;

    // Preserve the last readable clock if authority is explicitly removed.
    // Binding a NEW Timeline does not overwrite that being with stale fallback
    // state: the selected Timeline is authoritative by virtue of being selected.
    if (!timelineBeing && _timeline && _timeline->hasClock()) {
        _now = _timeline->now();
        _dt = _timeline->delta();
        _clockSet = true;
    }

    _timeline = timelineBeing;
}

void ActiveZones::setClock(double nowValue, double deltaValue) {
    if (_timeline) {
        (void)_timeline->setClock(nowValue, deltaValue);
        return;
    }
    _now = nowValue;
    _dt = deltaValue;
    _clockSet = true;
}

bool ActiveZones::hasClock() const {
    return _timeline ? _timeline->hasClock() : _clockSet;
}

double ActiveZones::now() const {
    return _timeline ? _timeline->now() : _now;
}

double ActiveZones::dt() const {
    return _timeline ? _timeline->delta() : _dt;
}

bool ActiveZones::isRelationSuperseded(const Relation& r) const {
    const std::string myId = r.getIdentifier();
    for (const auto* rel : relations()) {
        if (rel && rel->type == "superseded-by") {
            if ((rel->a() == &r) || (rel->aId() == myId)) {
                return true;
            }
        }
    }
    return false;
}

bool ActiveZones::isRelationCurrent(const Relation& r) const {
    return !isRelationSuperseded(r);
}
