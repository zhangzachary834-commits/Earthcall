#pragma once

#include "Relation/Relation.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

// ---------------------------------------------------------------------------
// Walking the relation graph — Formation Rete §3.1/§3.2, rungs 5 and 6.
//
// Zach, 2026-09-15, answering §9.1: similarity is one source of routing among
// several, and the slow adapter's job is to run broader bounded searches
// (BFS, Dijkstra) across Singulars, properties and Relations, RETAINING what it
// finds as first-class Relation structure so later searches pay almost nothing.
// Asked which to build first: *"I think its fine to just implement BFS for now
// we can always adjust it to dijkstras later if I decide weight is something
// like strength."*
//
// So this is BFS: unweighted, shortest-in-HOPS, and deliberately not reading
// `Relation::weight` at all — which is also the honest thing to do while §9.2
// is open and Zach is leaning toward removing weight entirely.
//
// UPGRADING TO DIJKSTRA LATER (the reason this shape was chosen): swap the
// frontier queue for a priority queue keyed on accumulated cost, and give
// RouteQuery a cost function. Nothing else here changes — `Route` already
// carries the edges it used, the caps already bound the search, and the
// contract below is a search-order-independent statement. Hop count is exactly
// Dijkstra with every edge weighing 1.
//
// THE CONTRACT, and it is the whole safety argument (FORMATION_RETE.md §6):
// a route only ever says WHERE TO LOOK. It never decides that a Law holds. So
// a route that is stale, capped short, or simply missed costs efficiency and
// never truth, and the complete sweep remains the correctness floor. That is
// what makes a bounded, approximate search admissible here at all.
//
// BOUNDED BY CONSTRUCTION (§3.1: "it runs on an independent clock, and is
// capped"): every search states a hop limit and a visit limit, and reports
// whether it hit them. An unbounded walk of a Person's world is not on offer.
//
// FOR FUTURE AGENTS (Jules especially):
//   * Do not make this decide anything. If you find yourself asking a route
//     whether a law should fire, stop: the law's own conditions answer that.
//   * Endpoints are matched by POINTER when bound and by KEPT IDENTIFIER when
//     not, the same rule ConditionModel's `Related` uses. A relation whose
//     endpoint was freed still names its being, and a save loaded without a
//     resolver has no pointers at all.
//   * Results are ordered deterministically (hops, then relation kind, then
//     destination identifier), never by pointer value. Two runs of the same
//     world must give the same routes, or nothing built on top can be saved,
//     compared, or reproduced.
//   * Retention — turning a discovered route into a Relation that persists — is
//     NOT here yet. It needs §9.1(a) (the adapter is a First Mover) and the
//     primary/sub-Relation rules in ontology/PRIMARY_AND_SUB_RELATIONS.md.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md
// ---------------------------------------------------------------------------

namespace Relevance {

// One hop: the relation crossed, and the being it led to. `to` is null when the
// far endpoint is unbound — the edge names it, the world has not bound it, and
// `toId` is what it names.
struct RouteStep {
    Relation* edge = nullptr;
    Singular* to = nullptr;
    std::string toId;
};

// A shortest walk, in hops, from the search's origin to `destination`.
struct Route {
    std::vector<RouteStep> steps;
    Singular* destination = nullptr;
    std::string destinationId;
    std::size_t hops() const { return steps.size(); }
};

struct RouteQuery {
    // The caps. Both are stated, never defaulted to "no limit".
    std::size_t maxHops = 3;
    std::size_t maxVisited = 512;

    // Relation kinds the walk may cross. Empty means any kind.
    std::unordered_set<std::string> throughKinds;

    // Stop as soon as this being is reached. Empty explores until the caps.
    std::string destinationId;

    // A directed relation is a one-way street from a() to b(). False walks
    // every edge both ways — which is a WIDENING, so it is safe for proposing
    // candidates and wrong for anything that claims direction means something.
    bool respectDirection = true;
};

struct RouteResult {
    // One shortest route per being reached, origin excluded.
    std::vector<Route> routes;
    std::size_t visited = 0;
    // True when a cap stopped the search — so a caller can tell "no route
    // exists" (false) from "none found within these bounds" (true). Conflating
    // those two is how a bounded search starts lying about the world.
    bool cappedOut = false;
};

RouteResult breadthFirstRoutes(const Singular& from, const RouteQuery& query);

}  // namespace Relevance
