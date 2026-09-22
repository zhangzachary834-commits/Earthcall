#include "SlowAdapter.hpp"

#include "Relation/Formation/Formation.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <algorithm>
#include <unordered_set>

namespace Relevance {
namespace {

Singular* resolveById(const std::string& id) {
    if (id.empty()) return nullptr;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

// The same question asked many times in one build gets a map instead of a scan.
// A road over unbound edges resolves one name per edge, and resolveById walks
// every being each time — O(edges x beings) for exactly the case the road exists
// to make cheap.
class NameCache {
public:
    Singular* resolve(const std::string& id) {
        if (id.empty()) return nullptr;
        if (!_built) {
            _built = true;
            for (Singular* being : Universe::instance().beings()) {
                if (!being) continue;
                const std::string name = being->getIdentifier();
                if (!name.empty()) _byId.emplace(name, being);
            }
        }
        auto it = _byId.find(id);
        return it == _byId.end() ? nullptr : it->second;
    }
private:
    bool _built = false;
    std::unordered_map<std::string, Singular*> _byId;
};

std::vector<Relation*> edgesAt(Singular& being) {
    std::vector<Relation*> edges;
    if (Universe::instance().relationsInvolving(being, edges)) return edges;
    const std::string id = being.getIdentifier();
    for (Relation* relation : Universe::instance().relations()) {
        if (!relation) continue;
        if (relation->a() == &being || relation->b() == &being) {
            edges.push_back(relation);
            continue;
        }
        if (!id.empty() && ((!relation->a() && relation->aId() == id) ||
                            (!relation->b() && relation->bId() == id))) {
            edges.push_back(relation);
        }
    }
    return edges;
}

// The being on the far side of `edge` from the category, or null when this edge
// does not put anyone in that category.
//
// This must agree EXACTLY with ConditionModel's `Related(kind, other)`: the
// subject is related when it is the source of a directed edge whose far end is
// `other`, or either end of an undirected one. A candidate set built from a
// different rule either wastes work or hides a being from its law.
Singular* memberThrough(Relation& edge, const std::string& categoryId, NameCache& names) {
    const std::string aId = edge.a() ? edge.a()->getIdentifier() : edge.aId();
    const std::string bId = edge.b() ? edge.b()->getIdentifier() : edge.bId();

    if (bId == categoryId) {
        Singular* member = edge.a();
        if (!member) member = names.resolve(aId);
        return member;
    }
    if (!edge.directed && aId == categoryId) {
        Singular* member = edge.b();
        if (!member) member = names.resolve(bId);
        return member;
    }
    return nullptr;
}

}  // namespace

void SlowAdapter::noteLaw(const std::string& lawId,
                          const std::vector<std::pair<std::string, std::string>>& routes) {
    std::vector<RouteKey> keys;
    keys.reserve(routes.size());
    for (const auto& route : routes) {
        RouteKey key{route.first, route.second};
        keys.push_back(key);
        if (_roads.find(key) == _roads.end()) {
            _roads.emplace(key, Road{});
            _pending.push_back(key);
        }
    }
    if (keys.empty()) {
        _lawRoutes.erase(lawId);
        return;
    }
    _lawRoutes[lawId] = std::move(keys);
}

void SlowAdapter::forgetLaw(const std::string& lawId) {
    _lawRoutes.erase(lawId);
}

bool SlowAdapter::current(const Road& road) const {
    if (!road.built) return false;
    // A capped road is INCOMPLETE, and an incomplete candidate set is a deaf
    // law. It is kept (so the cap is visible and the revisit rate can try
    // again) but never served.
    if (road.cappedOut) return false;
    // Nobody is counting the graph's changes, so nothing here can be shown to be
    // current. Refuse, and let the caller sweep.
    if (!Universe::instance().hasRelationGeneration()) return false;
    // BOTH numbers, and both are needed. The graph's generation catches a
    // relation formed, dissolved or loaded; the structural revision catches a
    // being arriving or leaving, which changes who an unbound edge names without
    // touching the graph at all (slow_adapter_test's "a being arrives" case).
    return road.builtAtStructural == Universe::instance().structuralRevision() &&
           road.builtAtGraph == Universe::instance().relationGeneration();
}

void SlowAdapter::build(const RouteKey& key, const Budget& budget) {
    Road& road = _roads[key];
    const bool wasBuilt = road.built;
    const bool wasCapped = road.cappedOut;
    const std::uint64_t oldStructural = road.builtAtStructural;
    const std::size_t oldGraph = road.builtAtGraph;

    road.members.clear();
    road.edges.clear();
    road.cappedOut = false;
    road.built = true;
    road.builtAtStructural = Universe::instance().structuralRevision();
    road.builtAtGraph = Universe::instance().relationGeneration();
    road.lastVisited = _steps;

    // The category's own edges when it is here; otherwise every relation in the
    // world, because AN EDGE CAN NAME A CATEGORY THAT IS NOT PRESENT. A relation
    // keeps its endpoint's identifier when it has no pointer (a save loaded
    // without a resolver, a being freed and not yet reborn), and `Related` holds
    // of such an edge by that name — so returning an empty road here would have
    // been a candidate set missing every being on it, which is a deaf law.
    // Found by review 2026-09-16, before it could be shipped; the last case of
    // slow_adapter_test guards it.
    Singular* category = resolveById(key.categoryId);
    std::vector<Relation*> edges;
    if (category) {
        edges = edgesAt(*category);
    } else {
        edges = Universe::instance().relations();
    }

    NameCache names;
    for (Relation* edge : edges) {
        if (!edge || edge->type != key.relationKind) continue;
        Singular* member = memberThrough(*edge, key.categoryId, names);
        if (!member) continue;
        if (std::find(road.members.begin(), road.members.end(), member) != road.members.end()) {
            continue;
        }
        if (road.members.size() >= budget.maxMembers) {
            road.cappedOut = true;
            break;
        }
        road.members.push_back(member);
        road.edges.push_back(edge);
    }

    // Exact per-road currency: routine revisits of an unchanged road remain
    // invisible to the hot-path tier cache; eligibility-changing rebuilds do not.
    if (!wasBuilt || oldStructural != road.builtAtStructural ||
        oldGraph != road.builtAtGraph || wasCapped != road.cappedOut) {
        road.currencyRevision = ++_currencyClock;
    }
}

std::size_t SlowAdapter::step() { return step(Budget{}); }

void SlowAdapter::observe() {
    // Kept as the place a consumer says "I am watching now". Nothing is
    // subscribed: currency is decided by the two counters `current()` reads.
    //
    // An earlier version subscribed to `relation-formed`/`relation-destroyed`
    // instead. That was not enough, and the hole was invisible:
    // RelationManager::loadFromJson replaces the WHOLE graph and publishes
    // nothing at all (it only calls touch()), so a road built before a save was
    // loaded would have been served against a graph that no longer existed.
    // RelationManager::generation() already counts every write to `relations` —
    // it is what the rung 4 endpoint index keys on — so reading it is both
    // simpler and stricter than listening for announcements that do not cover
    // every path.
    _observing = true;
}

void SlowAdapter::forgetBeing(const Singular* being) {
    if (!being) return;
    for (auto& entry : _roads) {
        auto& road = entry.second;
        bool involvesBeing = false;
        auto mIt = std::find(road.members.begin(), road.members.end(), being);
        if (mIt != road.members.end()) {
            involvesBeing = true;
        }
        if (!involvesBeing) {
            for (Relation* rel : road.edges) {
                if (rel && (rel->a() == being || rel->b() == being)) {
                    involvesBeing = true;
                    break;
                }
            }
        }
        if (involvesBeing) {
            road.built = false;
            road.currencyRevision = ++_currencyClock;
            road.edges.clear();
            road.members.clear();
            _pending.push_back(entry.first);
        }
    }
}

std::size_t SlowAdapter::step(const Budget& budget) {
    // ONE clock, ticked here. `lastVisited` used to be stamped from a counter
    // that only moved when a road was BUILT, so "how long since this road was
    // looked at" was measured in builds rather than in steps — the revisit rate
    // drifted with how much work happened to be done.
    ++_steps;
    std::size_t touched = 0;

    // IMPROVE: build roads nobody has walked yet.
    for (std::size_t i = 0; i < budget.improve && !_pending.empty(); ++i) {
        const RouteKey key = _pending.front();
        _pending.pop_front();
        if (_roads.find(key) == _roads.end()) continue;
        build(key, budget);
        ++touched;
    }

    // REVISIT: re-walk the road least recently checked. §4B — an index built
    // once and never re-checked "found better routes once", and this one also
    // goes stale when the world's shape moves under it.
    for (std::size_t i = 0; i < budget.revisit; ++i) {
        const RouteKey* oldest = nullptr;
        std::uint64_t oldestVisit = 0;
        for (const auto& entry : _roads) {
            if (!entry.second.built) continue;
            if (current(entry.second) && entry.second.lastVisited + _roads.size() > _steps) {
                continue;   // checked recently enough
            }
            if (!oldest || entry.second.lastVisited < oldestVisit) {
                oldest = &entry.first;
                oldestVisit = entry.second.lastVisited;
            }
        }
        if (!oldest) break;
        build(*oldest, budget);
        ++touched;
    }

    return touched;
}

bool SlowAdapter::ready(const std::string& lawId) const {
    auto it = _lawRoutes.find(lawId);
    if (it == _lawRoutes.end() || it->second.empty()) return false;
    for (const RouteKey& key : it->second) {
        auto road = _roads.find(key);
        if (road == _roads.end() || !current(road->second)) return false;
    }
    return true;
}

bool SlowAdapter::candidateViewFor(
    const std::string& lawId, const std::vector<Singular*>*& out) const {
    out = nullptr;
    auto law = _lawRoutes.find(lawId);
    if (law == _lawRoutes.end() || law->second.size() != 1) return false;

    auto road = _roads.find(law->second.front());
    if (road == _roads.end() || !current(road->second)) return false;

    out = &road->second.members;
    return true;
}

std::uint64_t SlowAdapter::candidateGenerationFor(const std::string& lawId) const {
    auto law = _lawRoutes.find(lawId);
    if (law == _lawRoutes.end() || law->second.size() != 1) return 0;

    auto road = _roads.find(law->second.front());
    if (road == _roads.end()) return 0;
    return road->second.currencyRevision;
}

bool SlowAdapter::candidatesFor(const std::string& lawId, std::vector<Singular*>& out) const {
    out.clear();
    auto it = _lawRoutes.find(lawId);
    if (it == _lawRoutes.end() || it->second.empty()) return false;

    // Every road this law travels must be current. A law naming two categories
    // is answered only when BOTH are known: serving the union of one known and
    // one unknown road would drop the beings the unknown one carries.
    for (const RouteKey& key : it->second) {
        auto road = _roads.find(key);
        if (road == _roads.end() || !current(road->second)) return false;
    }

    // The UNION, deliberately. Two `Related` leaves in one condition may be
    // conjoined or disjoined, and this structure does not know which — so it
    // proposes everyone either road carries and lets the condition decide.
    // Widen where uncertain, never narrow (PROPHETIC_RETE.md §2).
    std::unordered_set<const Singular*> seen;
    for (const RouteKey& key : it->second) {
        for (Singular* member : _roads.at(key).members) {
            if (!member || Universe::instance().isUnmade(member)) continue;
            if (seen.insert(member).second) out.push_back(member);
        }
    }
    return true;
}

SlowAdapter::Reified SlowAdapter::reify(const std::string& lawId, const RouteKey& route,
                                        Singular& author) {
    Reified made;
    auto road = _roads.find(route);
    if (road == _roads.end() || !road->second.built) return made;

    // §3.2: "the discovered paths become beings." The gathering is a Formation
    // OF RELATIONS — the edges that carry this category's membership — and the
    // Law is related to the gathering, so a traverser walks Law -> gathering ->
    // edges -> members instead of asking the graph.
    auto gathering = std::make_unique<Formation>(std::vector<Singular*>{});
    gathering->setIdentifier("formation.route." + route.relationKind + "." + route.categoryId);
    for (Relation* edge : road->second.edges) {
        if (edge) gathering->addMember(edge);
    }
    made.gathering = std::move(gathering);

    // Relations between Relations: the gathering holds each edge, and says so
    // in the graph rather than only in its member list.
    for (Relation* edge : road->second.edges) {
        if (!edge) continue;
        auto held = std::make_shared<Relation>("gathers", *made.gathering, *edge, true);
        made.gatheringToEdges.push_back(held);
    }

    if (Singular* law = resolveById(lawId)) {
        made.lawToGathering = std::make_shared<Relation>("routes-through", *law, *made.gathering, true);
    }
    (void)author;   // recorded by the caller that admits these into a world
    return made;
}

void SlowAdapter::clear() {
    _roads.clear();
    _lawRoutes.clear();
    _pending.clear();
    _steps = 0;
    _currencyClock = 0;
}

}  // namespace Relevance
