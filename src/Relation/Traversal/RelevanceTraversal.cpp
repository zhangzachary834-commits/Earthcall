#include "RelevanceTraversal.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <algorithm>
#include <deque>
#include <unordered_map>

namespace Relevance {
namespace {

// A place the walk can stand: a being when the graph binds one, otherwise the
// identifier an edge kept for it. Both halves are needed — a save loaded
// without a resolver has only names, and a being freed mid-session leaves its
// name behind on every relation that held it.
struct Node {
    Singular* being = nullptr;
    std::string id;
};

// Identifiers of the beings in the world, resolved ONCE per search and only if
// an unbound endpoint is actually met. Most worlds bind everything and never
// build this.
class WorldNames {
public:
    Singular* resolve(const std::string& id) {
        if (id.empty()) return nullptr;
        if (!_built) build();
        auto it = _byId.find(id);
        return it == _byId.end() ? nullptr : it->second;
    }

    // Edges naming an identifier the world does not bind. Same laziness.
    const std::vector<Relation*>& edgesNaming(const std::string& id) {
        if (!_edgesBuilt) buildEdges();
        static const std::vector<Relation*> none;
        auto it = _edgesById.find(id);
        return it == _edgesById.end() ? none : it->second;
    }

private:
    void build() {
        _built = true;
        for (Singular* being : Universe::instance().beings()) {
            if (!being) continue;
            const std::string id = being->getIdentifier();
            if (!id.empty()) _byId.emplace(id, being);
        }
    }
    void buildEdges() {
        _edgesBuilt = true;
        for (Relation* relation : Universe::instance().relations()) {
            if (!relation) continue;
            const std::string aId = relation->aId();
            const std::string bId = relation->bId();
            if (!aId.empty()) _edgesById[aId].push_back(relation);
            if (!bId.empty() && bId != aId) _edgesById[bId].push_back(relation);
        }
    }
    bool _built = false;
    bool _edgesBuilt = false;
    std::unordered_map<std::string, Singular*> _byId;
    std::unordered_map<std::string, std::vector<Relation*>> _edgesById;
};

// Is this endpoint the node we are standing on? Pointer when bound, kept
// identifier when not — ConditionModel's `Related` decides identity the same
// way, and the two must not drift apart.
bool isHere(const Node& here, Singular* endpointPtr, const std::string& endpointId) {
    if (endpointPtr && here.being) return endpointPtr == here.being;
    if (endpointPtr && !here.being) {
        return !here.id.empty() && endpointPtr->getIdentifier() == here.id;
    }
    return !endpointId.empty() && !here.id.empty() && endpointId == here.id;
}

std::vector<Relation*> edgesAt(const Node& here, WorldNames& names) {
    std::vector<Relation*> edges;
    if (here.being) {
        if (Universe::instance().relationsInvolving(*here.being, edges)) return edges;
        for (Relation* relation : Universe::instance().relations()) {
            if (!relation) continue;
            if (relation->a() == here.being || relation->b() == here.being) {
                edges.push_back(relation);
                continue;
            }
            if (!here.id.empty() &&
                ((!relation->a() && relation->aId() == here.id) ||
                 (!relation->b() && relation->bId() == here.id))) {
                edges.push_back(relation);
            }
        }
        return edges;
    }
    return names.edgesNaming(here.id);
}

}  // namespace

RouteResult breadthFirstRoutes(const Singular& from, const RouteQuery& query) {
    RouteResult result;
    if (query.maxHops == 0 || query.maxVisited == 0) {
        result.cappedOut = true;
        return result;
    }

    WorldNames names;
    Singular* origin = const_cast<Singular*>(&from);
    const std::string originId = origin->getIdentifier();

    std::unordered_set<const Singular*> seenBeings{origin};
    std::unordered_set<std::string> seenIds;
    if (!originId.empty()) seenIds.insert(originId);

    struct Frontier {
        Node node;
        std::vector<RouteStep> route;
    };
    std::deque<Frontier> queue;
    queue.push_back(Frontier{Node{origin, originId}, {}});

    while (!queue.empty()) {
        const Frontier current = std::move(queue.front());
        queue.pop_front();

        // The hop cap truncates the frontier rather than the answer: routes
        // already found stay, and the result says it was cut short.
        if (current.route.size() >= query.maxHops) {
            result.cappedOut = true;
            continue;
        }

        std::vector<Relation*> edges = edgesAt(current.node, names);
        // Deterministic order, so two runs of one world give one answer.
        // Pointer order is whatever the allocator did last.
        std::sort(edges.begin(), edges.end(), [](Relation* lhs, Relation* rhs) {
            if (!lhs || !rhs) return rhs != nullptr;
            if (lhs->type != rhs->type) return lhs->type < rhs->type;
            const std::string lhsFar = lhs->aId() + "\x1f" + lhs->bId();
            const std::string rhsFar = rhs->aId() + "\x1f" + rhs->bId();
            return lhsFar < rhsFar;
        });

        for (Relation* edge : edges) {
            if (!edge) continue;
            if (!query.throughKinds.empty() && !query.throughKinds.count(edge->type)) continue;

            Singular* aPtr = edge->a();
            Singular* bPtr = edge->b();
            const std::string aId = aPtr ? aPtr->getIdentifier() : edge->aId();
            const std::string bId = bPtr ? bPtr->getIdentifier() : edge->bId();

            const bool hereIsSource = isHere(current.node, aPtr, aId);
            const bool hereIsTarget = isHere(current.node, bPtr, bId);
            if (!hereIsSource && !hereIsTarget) continue;
            // A directed edge is a one-way street out of its source.
            if (query.respectDirection && edge->directed && !hereIsSource) continue;

            Node next;
            next.being = hereIsSource ? bPtr : aPtr;
            next.id = hereIsSource ? bId : aId;
            if (!next.being) next.being = names.resolve(next.id);
            if (!next.being && next.id.empty()) continue;   // an edge to nobody

            // Seen by either name: a being bound now may have been reached by
            // identifier a moment ago through an unbound edge.
            if (next.being && seenBeings.count(next.being)) continue;
            if (!next.id.empty() && seenIds.count(next.id)) continue;
            if (next.being) seenBeings.insert(next.being);
            if (!next.id.empty()) seenIds.insert(next.id);

            std::vector<RouteStep> route = current.route;
            route.push_back(RouteStep{edge, next.being, next.id});

            Route found;
            found.destination = next.being;
            found.destinationId = next.id;
            found.steps = route;
            result.routes.push_back(std::move(found));
            ++result.visited;

            if (!query.destinationId.empty() && next.id == query.destinationId) {
                // BFS reaches a being by its fewest hops first, so this is the
                // shortest route to it and the search is done.
                return result;
            }

            if (result.visited >= query.maxVisited) {
                result.cappedOut = true;
                return result;
            }

            queue.push_back(Frontier{next, std::move(route)});
        }
    }

    std::sort(result.routes.begin(), result.routes.end(),
              [](const Route& lhs, const Route& rhs) {
                  if (lhs.hops() != rhs.hops()) return lhs.hops() < rhs.hops();
                  return lhs.destinationId < rhs.destinationId;
              });
    return result;
}

}  // namespace Relevance
