// Walking the relation graph: shortest in hops, bounded, and honest about both.
//
// FORMATION_RETE.md §3.1/§3.2, rungs 5 and 6. Zach, 2026-09-15, answering §9.1:
// the slow adapter runs broader bounded searches across the graph and keeps what
// it finds as Relation structure. Asked which search to build first: "I think its
// fine to just implement BFS for now we can always adjust it to dijkstras later
// if I decide weight is something like strength."
//
// A traversal only proposes where to look; the Law's conditions still decide, and
// the sweep is still the correctness floor (§6). That is what makes a capped,
// approximate search admissible. But "approximate" is not "arbitrary": the three
// things below must hold, or nothing can be built on top of it.
//
//   1. SHORTEST IN HOPS. BFS's whole claim. If it can return a 3-hop route to a
//      being it can reach in 2, a cost model later layered on it starts from a
//      wrong answer.
//   2. BOUNDED, AND IT SAYS SO. `cappedOut` distinguishes "no route exists" from
//      "none within these bounds". Conflating them is how a bounded search starts
//      lying about the world.
//   3. IDENTITY, NOT JUST POINTERS. An endpoint keeps its identifier when it has
//      no pointer (a save loaded without a resolver, a being freed and reborn).
//      A walk that only follows pointers goes deaf exactly where the Related
//      predicate did before rung 4 fixed it.
//
// FOR FUTURE AGENTS (Jules especially): when this becomes Dijkstra, every case
// here must still pass with unit costs — hop count IS Dijkstra with every edge
// weighing 1. If you add a way to cross an edge (a new kind filter, a
// relation-of-relations hop), add a case. If you make routes persist, that is
// governed by ontology/PRIMARY_AND_SUB_RELATIONS.md, not by this file.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "Relation/Traversal/RelevanceTraversal.hpp"
#include "Relation/RelationManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "json.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what.c_str()); }
    else std::printf("  ok: %s\n", what.c_str());
}

const Relevance::Route* routeTo(const Relevance::RouteResult& result, const std::string& id) {
    for (const auto& route : result.routes) {
        if (route.destinationId == id) return &route;
    }
    return nullptr;
}

std::string reached(const Relevance::RouteResult& result) {
    std::string out;
    for (const auto& route : result.routes) {
        if (!out.empty()) out += " ";
        out += route.destinationId + "(" + std::to_string(route.hops()) + ")";
    }
    return out;
}

} // namespace

int main() {
    // A small world: a chain a -> b -> c -> d, a shortcut a -> d, a branch off b,
    // and an island nothing reaches.
    Object a;       a.setObjectID("a");
    Object b;       b.setObjectID("b");
    Object c;       c.setObjectID("c");
    Object d;       d.setObjectID("d");
    Object branch;  branch.setObjectID("branch");
    Object island;  island.setObjectID("island");

    std::vector<Singular*> population{&a, &b, &c, &d, &branch, &island};
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        for (Singular* s : population) out.push_back(s);
    });

    RelationManager graph;
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& r : graph.getAll()) if (r) out.push_back(r.get());
    });
    Universe::instance().setRelationsInvolvingProvider(
        [&](const Singular& being, std::vector<Relation*>& out) {
            graph.relationsInvolving(being, out);
        });

    graph.add(std::make_shared<Relation>("next", a, b, true));
    graph.add(std::make_shared<Relation>("next", b, c, true));
    graph.add(std::make_shared<Relation>("next", c, d, true));
    graph.add(std::make_shared<Relation>("beside", b, branch, false));

    // ---------------------------------------------------------------
    // 1. Shortest in hops, and everything reachable is reported once.
    // ---------------------------------------------------------------
    {
        Relevance::RouteQuery q;  q.maxHops = 5;
        const auto result = Relevance::breadthFirstRoutes(a, q);
        std::printf("  from a: %s\n", reached(result).c_str());
        check(result.routes.size() == 4, "reaches b, c, d and branch, each once");
        const auto* toC = routeTo(result, "c");
        const auto* toD = routeTo(result, "d");
        check(toC && toC->hops() == 2, "c is two hops away");
        check(toD && toD->hops() == 3, "d is three hops away along the chain");
        check(routeTo(result, "island") == nullptr, "an unreachable being is not reported");
        check(!result.cappedOut, "an exhausted search is not reported as capped");
        if (toD) {
            check(toD->steps.size() == 3 && toD->steps[0].toId == "b" &&
                      toD->steps[1].toId == "c" && toD->steps[2].toId == "d",
                  "the route carries the beings it passed through, in order");
            check(toD->steps[0].edge && toD->steps[0].edge->type == "next",
                  "and the relations it crossed");
        }
    }

    // ---------------------------------------------------------------
    // 2. A SHORTCUT must win. Added after the long way round exists, which is
    //    the case a search that keeps the first route it built gets wrong.
    // ---------------------------------------------------------------
    graph.add(std::make_shared<Relation>("leap", a, d, true));
    {
        Relevance::RouteQuery q;  q.maxHops = 5;
        const auto result = Relevance::breadthFirstRoutes(a, q);
        const auto* toD = routeTo(result, "d");
        check(toD && toD->hops() == 1, "the one-hop shortcut replaces the three-hop chain");
    }

    // ---------------------------------------------------------------
    // 3. The caps, and saying so.
    // ---------------------------------------------------------------
    {
        Relevance::RouteQuery q;  q.maxHops = 1;
        const auto result = Relevance::breadthFirstRoutes(a, q);
        check(routeTo(result, "c") == nullptr, "a hop cap stops the walk");
        check(result.cappedOut, "and the result says the walk was cut short");

        Relevance::RouteQuery small;  small.maxHops = 5;  small.maxVisited = 2;
        const auto limited = Relevance::breadthFirstRoutes(a, small);
        check(limited.routes.size() == 2 && limited.cappedOut,
              "a visit cap stops the walk and is reported");

        Relevance::RouteQuery far;  far.maxHops = 5;
        const auto exhausted = Relevance::breadthFirstRoutes(island, far);
        check(exhausted.routes.empty() && !exhausted.cappedOut,
              "no route at all is NOT reported as capped — the caller can tell them apart");
    }

    // ---------------------------------------------------------------
    // 4. Direction, and crossing only chosen kinds.
    // ---------------------------------------------------------------
    {
        Relevance::RouteQuery q;  q.maxHops = 5;
        const auto fromD = Relevance::breadthFirstRoutes(d, q);
        check(fromD.routes.empty(), "a directed edge is not walked backwards");

        Relevance::RouteQuery both;  both.maxHops = 5;  both.respectDirection = false;
        const auto ignoring = Relevance::breadthFirstRoutes(d, both);
        check(routeTo(ignoring, "a") != nullptr,
              "ignoring direction widens the walk (a candidate proposal, never a claim)");

        Relevance::RouteQuery kinds;  kinds.maxHops = 5;  kinds.throughKinds = {"next"};
        const auto onlyNext = Relevance::breadthFirstRoutes(a, kinds);
        check(routeTo(onlyNext, "branch") == nullptr, "a kind filter keeps the walk off other edges");
        check(routeTo(onlyNext, "c") != nullptr, "and still crosses the kind it was given");
    }

    // ---------------------------------------------------------------
    // 5. Cycles terminate, and a named destination stops the search early.
    // ---------------------------------------------------------------
    graph.add(std::make_shared<Relation>("next", d, a, true));   // closes the loop
    {
        Relevance::RouteQuery q;  q.maxHops = 10;
        const auto result = Relevance::breadthFirstRoutes(a, q);
        check(result.routes.size() == 4, "a cycle does not revisit beings or loop forever");

        Relevance::RouteQuery stop;  stop.maxHops = 10;  stop.destinationId = "c";
        const auto found = Relevance::breadthFirstRoutes(a, stop);
        const auto* toC = routeTo(found, "c");
        check(toC && toC->hops() == 2, "a named destination is found by its shortest route");
        // It returned the moment it found c, rather than finishing the walk:
        // the destination is the LAST thing it recorded, and it saw fewer
        // beings than the same walk with no destination named.
        const auto full = Relevance::breadthFirstRoutes(a, q);
        check(!found.routes.empty() && found.routes.back().destinationId == "c",
              "and the search returns at the destination instead of exploring on");
        check(found.visited <= full.visited,
              "and never visits more than the full walk would");
    }

    // ---------------------------------------------------------------
    // 6. IDENTITY. A save loaded without a resolver has no pointers at all, and
    //    the walk must still cross those edges by the names they kept.
    // ---------------------------------------------------------------
    {
        RelationManager loaded;
        loaded.loadFromJson(nlohmann::json::array({
            {{"type", "next"}, {"entityA", "a"}, {"entityB", "b"}, {"directed", true}},
            {{"type", "next"}, {"entityA", "b"}, {"entityB", "c"}, {"directed", true}}}));
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : loaded.getAll()) if (r) out.push_back(r.get());
        });   // clears the involving provider: the scan path, as a loaded save has
        check(loaded.getAll()[0]->a() == nullptr, "precondition: the loaded edges are unbound");

        Relevance::RouteQuery q;  q.maxHops = 5;
        const auto result = Relevance::breadthFirstRoutes(a, q);
        const auto* toC = routeTo(result, "c");
        check(toC && toC->hops() == 2, "an unbound edge is still walked, by the identifier it kept");
        check(toC && toC->destination == &c,
              "and the being it names is resolved back out of the world");
    }

    // ---------------------------------------------------------------
    // 7. BREADTH, not depth — the claim the whole thing rests on.
    //    `z` sits 2 hops away through `r`, and 3 hops away through p -> q.
    //    The kinds are named so the LONG branch is explored first: a search
    //    that follows one branch to its end before trying the other (a stack
    //    instead of a queue) reaches z at 3 hops and reports that. Every other
    //    case in this file passes under such a search — this is the one that
    //    fails, so it is the one that guards BFS itself.
    // ---------------------------------------------------------------
    {
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : graph.getAll()) if (r) out.push_back(r.get());
        });
        Universe::instance().setRelationsInvolvingProvider(
            [&](const Singular& being, std::vector<Relation*>& out) {
                graph.relationsInvolving(being, out);
            });

        Object start;  start.setObjectID("start");
        Object p;      p.setObjectID("p");
        Object q;      q.setObjectID("q");
        Object r;      r.setObjectID("r");
        Object z;      z.setObjectID("z");
        for (Object* o : {&start, &p, &q, &r, &z}) population.push_back(o);

        // "alpha" sorts before "omega", so the short branch is queued first and
        // the long one is on top of the stack a depth-first search would pop.
        graph.add(std::make_shared<Relation>("alpha", start, r, true));
        graph.add(std::make_shared<Relation>("omega", start, p, true));
        graph.add(std::make_shared<Relation>("omega", p, q, true));
        graph.add(std::make_shared<Relation>("omega", q, z, true));
        graph.add(std::make_shared<Relation>("alpha", r, z, true));

        Relevance::RouteQuery deep;  deep.maxHops = 6;
        const auto result = Relevance::breadthFirstRoutes(start, deep);
        std::printf("  from start: %s\n", reached(result).c_str());
        const auto* toZ = routeTo(result, "z");
        check(toZ && toZ->hops() == 2,
              "z is found at 2 hops through r, not at 3 through the branch explored first");
        check(toZ && toZ->steps.size() == 2 && toZ->steps[0].toId == "r",
              "and the route it reports is the short one");
    }

    Universe::instance().setRelationProvider(nullptr);
    Universe::instance().setProvider(nullptr);

    std::printf("%s\n", g_failures ? "relevance_traversal_test: FAILURES"
                                   : "relevance_traversal_test: OK");
    return g_failures ? 1 : 0;
}
