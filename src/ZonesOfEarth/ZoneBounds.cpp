// Zones as mathematical bounds — the kernel locator (Rung 1).
//
// docs/plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md. Zach, 2026-09-23:
// "Person's currently located zone must be decoupled from whether a Zone is
// active", and the shared space is "a Dimensional Zone which basically
// represents an entire continuum of a dimension upon which values could be
// had (like 1D, 2D, 3D space)."
//
// Everything here is SENSE. It reads authored axes (dimension.*), placements
// (placement.*), and extents off Zones, and a being's coordinates off the
// being, and answers which Zones contain it. It writes nothing, so a world
// that authors none of it answers exactly as it did before: location is the
// residence chain.
//
// Claude Opus 5.5, session b0dcb70f-a02a-4081-8589-0aae3ab30551.

#include "ZonesOfEarth/ZoneManager.hpp"

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {

// `within` chains are authored text and may be wrong: a cycle, or a name
// that no Zone answers to. Neither may hang or crash the locator.
constexpr int kMaxWithinDepth = 64;

} // namespace

Zone* ZoneManager::findZone(const std::string& identifier) const {
    if (identifier.empty()) return nullptr;
    for (const auto& zone : _zones) {
        if (zone && zone->getIdentifier() == identifier) return zone.get();
    }
    return nullptr;
}

Zone* ZoneManager::withinOf(const Zone& zone) const {
    const std::string& parent = zone.getParentZone();
    if (parent.empty() || parent == zone.getIdentifier()) return nullptr;
    return findZone(parent);
}

Zone* ZoneManager::dimensionalRootOf(const Zone& zone) const {
    const Zone* current = &zone;
    std::unordered_set<const Zone*> seen{current};
    for (int depth = 0; depth < kMaxWithinDepth; ++depth) {
        Zone* parent = withinOf(*current);
        if (!parent || !seen.insert(parent).second) break;   // top, or a cycle
        current = parent;
    }
    return const_cast<Zone*>(current);
}

namespace {

// The chain from `zone` up to (not including) `root`, innermost first — the
// Zones whose placements carry a coordinate from `zone`'s frame into the
// root's.
std::vector<const Zone*> chainBelowRoot(const ZoneManager& mgr, const Zone& zone,
                                        const Zone* root) {
    std::vector<const Zone*> chain;
    const Zone* current = &zone;
    std::unordered_set<const Zone*> seen;
    for (int depth = 0; current && current != root && depth < kMaxWithinDepth; ++depth) {
        if (!seen.insert(current).second) break;
        chain.push_back(current);
        current = mgr.withinOf(*current);
    }
    return chain;
}

// The residence chain, outermost first: what "where is it" meant before
// Zones had bounds, and what it still means where none are authored.
std::vector<Zone*> residenceChain(const ZoneManager& mgr, Zone* residence) {
    std::vector<Zone*> chain;
    std::unordered_set<Zone*> seen;
    for (Zone* z = residence; z && chain.size() < static_cast<size_t>(kMaxWithinDepth);
         z = mgr.withinOf(*z)) {
        if (!seen.insert(z).second) break;
        chain.push_back(z);
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

} // namespace

Zone* ZoneManager::residenceOf(const Singular& being) const {
    if (const auto* zone = dynamic_cast<const Zone*>(&being)) {
        return withinOf(*zone);
    }
    if (dynamic_cast<const Person*>(&being)) {
        // The Person is present where the Person is (Rung 2 decouples this
        // from "active"; for now they coincide).
        return _zones.empty() ? nullptr : _zones[_currentIndex].get();
    }

    const double now = Universe::instance().now();
    const auto rebuild = [&]() {
        _residenceIndex.clear();
        for (const auto& zone : _zones) {
            if (!zone) continue;
            for (const auto& obj : zone->objects()) {
                if (!obj) continue;
                // A child Zone's view includes its parents' objects
                // (switchTo), so a being can sit in several stores. Its
                // residence is the store it is DESIGNATED to, when there is
                // one; otherwise the first store that holds it.
                auto it = _residenceIndex.find(obj.get());
                if (it == _residenceIndex.end()) {
                    _residenceIndex.emplace(obj.get(), zone.get());
                } else if (!obj->belongsToZone(it->second->getIdentifier()) &&
                           obj->belongsToZone(zone->getIdentifier())) {
                    it->second = zone.get();
                }
            }
        }
        _residenceStamp = now;
        _residenceBuilt = true;
    };
    if (!_residenceBuilt || now != _residenceStamp) rebuild();
    auto it = _residenceIndex.find(&being);
    if (it == _residenceIndex.end()) {
        rebuild();   // a being born this tick
        it = _residenceIndex.find(&being);
    }
    return it == _residenceIndex.end() ? nullptr : it->second;
}

bool ZoneManager::coordinatesIn(const Singular& being, const Zone& frame,
                                std::map<std::string, PropertyValue>& out) const {
    out.clear();
    Zone* residence = residenceOf(being);
    if (!residence) return false;
    Zone* root = dimensionalRootOf(*residence);
    if (!root || dimensionalRootOf(frame) != root) return false;   // another continuum
    const auto axes = root->dimensionAxes();
    if (axes.empty()) return false;

    // Local coordinates, carried up into the root's frame by the residence's
    // placements, then down into `frame` by subtracting frame's.
    const auto up = chainBelowRoot(*this, *residence, root);
    const auto down = chainBelowRoot(*this, frame, root);
    for (const auto& [axis, path] : axes) {
        PropertyValue v;
        double x = 0.0;
        if (!lawGetValue(const_cast<Singular&>(being), PropertyPath::parse(path), v) ||
            !propertyValueToNumber(v, x)) {
            out.clear();
            return false;
        }
        for (const Zone* z : up) x += z->placementAlong(axis);
        for (const Zone* z : down) x -= z->placementAlong(axis);
        out[axis] = PropertyValue(x);
    }
    return true;
}

std::vector<Zone*> ZoneManager::locate(const Singular& being) const {
    Zone* residence = residenceOf(being);
    if (!residence) return {};
    if (dynamic_cast<const Zone*>(&being)) {
        // A Zone is located wherever it is within, by definition.
        return residenceChain(*this, residence);
    }
    Zone* root = dimensionalRootOf(*residence);
    std::map<std::string, PropertyValue> probe;
    if (!coordinatesIn(being, *root, probe)) {
        // No continuum axes, or the being has no coordinate on one: location
        // cannot be computed from bounds, so it is what it always was.
        return residenceChain(*this, residence);
    }

    // Containment nests: Z contains the being iff Z's extent holds in Z's
    // own frame AND Z's parent contains it. Memoized per call.
    std::unordered_map<const Zone*, bool> memo;
    std::function<bool(const Zone&, int)> contains = [&](const Zone& z, int depth) -> bool {
        auto hit = memo.find(&z);
        if (hit != memo.end()) return hit->second;
        memo[&z] = false;   // cycle guard while this frame is open
        bool inside = false;
        if (depth < kMaxWithinDepth) {
            const Zone* parent = (&z == root) ? nullptr : withinOf(z);
            if (&z == root || (parent && contains(*parent, depth + 1))) {
                std::map<std::string, PropertyValue> coords;
                inside = coordinatesIn(being, z, coords) && z.extentHolds(coords);
            }
        }
        memo[&z] = inside;
        return inside;
    };

    std::vector<std::pair<size_t, Zone*>> found;
    for (const auto& zone : _zones) {
        if (!zone || dimensionalRootOf(*zone) != root) continue;
        if (contains(*zone, 0)) {
            found.emplace_back(chainBelowRoot(*this, *zone, root).size(), zone.get());
        }
    }
    std::stable_sort(found.begin(), found.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<Zone*> located;
    located.reserve(found.size());
    for (const auto& entry : found) located.push_back(entry.second);
    return located;
}

void ZoneManager::installZoneReadings() {
    static bool installed = false;
    if (installed) return;
    installed = true;

    // Answered through live() at read time, never a captured pointer: a
    // ZoneManager that is replaced (tests, reloads) must not leave readings
    // pointing at a dead one. No live manager = undefined, and an undefined
    // reading fails rather than answering "" (MathBinding.hpp).
    registerWorldReading("@world.zoneId", [](Singular& subject, PropertyValue& out) {
        const ZoneManager* mgr = ZoneManager::live();
        if (!mgr) return false;
        const auto located = mgr->locate(subject);
        out = PropertyValue(located.empty() ? std::string() : located.back()->getIdentifier());
        return true;
    });
    registerWorldReading("@world.zonePath", [](Singular& subject, PropertyValue& out) {
        const ZoneManager* mgr = ZoneManager::live();
        if (!mgr) return false;
        std::string path;
        for (const Zone* z : mgr->locate(subject)) {
            if (!path.empty()) path += "/";
            path += z->getIdentifier();
        }
        out = PropertyValue(path);
        return true;
    });
    registerWorldReading("@world.dimensionalZoneId", [](Singular& subject, PropertyValue& out) {
        const ZoneManager* mgr = ZoneManager::live();
        if (!mgr) return false;
        Zone* residence = mgr->residenceOf(subject);
        Zone* root = residence ? mgr->dimensionalRootOf(*residence) : nullptr;
        out = PropertyValue(root ? root->getIdentifier() : std::string());
        return true;
    });
}
