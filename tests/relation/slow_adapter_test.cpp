// The slow adapter proposes candidates. It must never propose FEWER than exist.
//
// FORMATION_RETE.md §3.1/§3.2/§3.3, rungs 5 and 6. Zach, 2026-09-16: "The
// mechanism that creates Relations between Relations and pre-loads Law Relations
// to these Relation Formations is also supposed to be in the slow adapter rather
// than constantly rebuilt every frame."
//
// The adapter walks the graph on its own clock and remembers, for each
// `Related(kind, category)` a Law travels, which beings that road carries. A tick
// then reads the answer instead of deriving it. The danger is exact and familiar:
// a candidate set that MISSES a being is a law that silently never fires for it —
// the failure rung 0, rung 2, rung 4 and rung 7 each found in a different guise.
//
// So the oracle here is never the adapter's own bookkeeping. Every case compares
// what it proposes against a FULL SCAN of the relation graph filtered by the same
// `Related` predicate a Law compiles. Where the two may legitimately differ, they
// differ in ONE direction only: the adapter may refuse to answer (and the caller
// sweeps), never answer short.
//
// The cases are the ways a pre-loaded road goes wrong:
//   A. it answers only once built, and matches the scan
//   B. a relation formed afterwards — the road must not keep serving the old set
//   C. a relation removed afterwards
//   D. the world's shape moving (a being admitted) invalidates it
//   E. a being freed leaves no dangling pointer behind
//   F. the caps: a road larger than the cap is refused, not truncated
//   G. undirected edges, and edges pointing the wrong way
//   H. a law naming two categories is answered only when BOTH roads are current
//   I. reification: the path becomes beings (a Formation of the edges, and a
//      Relation from the Law to it)
//
// FOR FUTURE AGENTS (Jules especially): if you add a way for the relation graph
// to change that does not publish `relation-formed`/`relation-destroyed`, this
// adapter will keep serving a stale road and the law that reads it goes deaf. Add
// the announcement, not a special case here. And never relax `candidatesFor` to
// answer when it is unsure — sweeping is merely slow.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "Relation/Traversal/SlowAdapter.hpp"
#include "Relation/RelationManager.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <algorithm>
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

// THE ORACLE: who a `Related(kind, category)` condition actually holds of,
// asked of every being in the world through the compiled predicate itself.
std::vector<std::string> scanned(const std::string& kind, const std::string& category) {
    ECA::Event probe;
    const auto predicate = ConditionNode::related(kind, category).compile();
    std::vector<std::string> out;
    for (Singular* being : Universe::instance().beings()) {
        if (being && predicate(probe, *being)) out.push_back(being->getIdentifier());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> named(const std::vector<Singular*>& beings) {
    std::vector<std::string> out;
    for (Singular* being : beings) {
        if (being) out.push_back(being->getIdentifier());
    }
    std::sort(out.begin(), out.end());
    return out;
}

// The adapter may propose MORE than the condition holds of (the condition
// decides), never fewer.
bool covers(const std::vector<std::string>& proposed, const std::vector<std::string>& truth) {
    return std::includes(proposed.begin(), proposed.end(), truth.begin(), truth.end());
}

std::string join(const std::vector<std::string>& ids) {
    std::string out;
    for (const auto& id : ids) { if (!out.empty()) out += ","; out += id; }
    return out.empty() ? "(none)" : out;
}

} // namespace

int main() {
    {
        Object target;   target.setObjectID("category.target");
        Object other;    other.setObjectID("category.other");
        Object one;      one.setObjectID("being.one");
        Object two;      two.setObjectID("being.two");
        Object stranger; stranger.setObjectID("being.stranger");
        Object lawBeing; lawBeing.setObjectID("law.travels");

        std::vector<Singular*> population{&target, &other, &one, &two, &stranger, &lawBeing};
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
        Universe::instance().setRelationGenerationProvider([&]() { return graph.generation(); });

        Relevance::SlowAdapter adapter;
        adapter.observe();
        adapter.noteLaw("law.travels", {{"instance-of", "category.target"}});

        graph.add(std::make_shared<Relation>("instance-of", one, target, true));

        std::vector<Singular*> candidates;

        // --------------------------------------------------------------
        // A. Nothing is served until the road is walked, and then it matches.
        // --------------------------------------------------------------
        check(!adapter.candidatesFor("law.travels", candidates),
              "before the adapter has done its work it refuses to answer (the caller sweeps)");
        check(adapter.step() > 0, "one step builds the road");
        check(adapter.candidatesFor("law.travels", candidates), "and then it answers");
        check(named(candidates) == scanned("instance-of", "category.target"),
              "the pre-loaded road matches a full scan: " + join(named(candidates)));

        // --------------------------------------------------------------
        // B. A relation formed afterwards. Relation changes do NOT move the
        //    structural revision, so an adapter that only watched that counter
        //    would keep serving the old set forever.
        // --------------------------------------------------------------
        graph.add(std::make_shared<Relation>("instance-of", two, target, true));
        const bool answeredStale = adapter.candidatesFor("law.travels", candidates) &&
                                   !covers(named(candidates), scanned("instance-of", "category.target"));
        check(!answeredStale, "a road made stale by a new relation is never served stale");
        adapter.step();
        check(adapter.candidatesFor("law.travels", candidates) &&
                  named(candidates) == scanned("instance-of", "category.target"),
              "and the next step picks the new member up: " + join(named(candidates)));

        // --------------------------------------------------------------
        // C. A relation removed afterwards.
        // --------------------------------------------------------------
        check(graph.removeBetween(one, target, "instance-of"), "precondition: the edge is removed");
        const bool servedRemoved = adapter.candidatesFor("law.travels", candidates) &&
                                   named(candidates) != scanned("instance-of", "category.target");
        check(!servedRemoved, "a road made stale by a removal is not served stale");
        adapter.step();
        check(adapter.candidatesFor("law.travels", candidates) &&
                  named(candidates) == scanned("instance-of", "category.target"),
              "and the next step drops the member: " + join(named(candidates)));

        // --------------------------------------------------------------
        // D. The world's shape moves: a being admitted after the road was built
        //    could be a member the road never saw.
        // --------------------------------------------------------------
        Object latecomer;  latecomer.setObjectID("being.latecomer");
        population.push_back(&latecomer);
        graph.add(std::make_shared<Relation>("instance-of", latecomer, target, true));
        Universe::instance().bumpStructuralRevision();   // what Zone admission does
        check(!adapter.candidatesFor("law.travels", candidates),
              "a road built before the world's shape moved is refused");
        adapter.step();
        check(adapter.candidatesFor("law.travels", candidates) &&
                  named(candidates) == scanned("instance-of", "category.target"),
              "and rebuilding includes the new being: " + join(named(candidates)));

        // --------------------------------------------------------------
        // D2. The world's shape moving with NO relation event at all — which is
        //     the only case that tests the structural-revision check on its own.
        //     An edge loaded from a save names a being that is not in the world
        //     yet (rung 4: an unbound endpoint keeps its identifier). The road is
        //     built while it names nobody; then the being ARRIVES. No relation is
        //     formed or destroyed, so nothing announces it — only the world's
        //     shape moves. A road that ignores that keeps proposing a set the
        //     newcomer is missing from, and the law never fires for it.
        // --------------------------------------------------------------
        {
            // The edge outlives the being it named: exactly what a save loaded
            // without a resolver holds, and what a Zone reload leaves behind.
            std::shared_ptr<Relation> unbound;
            {
                auto ghost = std::make_unique<Object>();
                ghost->setObjectID("being.arriving");
                unbound = std::make_shared<Relation>("instance-of", *ghost, target, true);
                graph.add(unbound);
                RelationManager::forgetBeingEverywhere(ghost.get());
            }
            check(unbound->a() == nullptr && unbound->aId() == "being.arriving",
                  "precondition: the edge kept the name of a being that is not here");
            Universe::instance().bumpStructuralRevision();
            adapter.step();
            check(adapter.candidatesFor("law.travels", candidates) &&
                      named(candidates) == scanned("instance-of", "category.target"),
                  "an edge naming a being that is not here yet carries nobody");

            Object arriving;  arriving.setObjectID("being.arriving");
            population.push_back(&arriving);
            Universe::instance().bumpStructuralRevision();   // admission; no relation event
            std::vector<Singular*> proposed;
            const bool served = adapter.candidatesFor("law.travels", proposed);
            check(!served || covers(named(proposed), scanned("instance-of", "category.target")),
                  "a road built before a being arrived is refused rather than served short");
            adapter.step();
            check(adapter.candidatesFor("law.travels", candidates) &&
                      named(candidates) == scanned("instance-of", "category.target"),
                  "and once rebuilt it carries the newly arrived being: " + join(named(candidates)));
            population.pop_back();
            graph.remove(unbound);
            Universe::instance().bumpStructuralRevision();
            adapter.step();
        }

        // --------------------------------------------------------------
        // E. A being freed. Its pointer must leave the road at once — the road
        //    holds raw pointers, and the next read would be a dangling one.
        // --------------------------------------------------------------
        {
            auto dying = std::make_unique<Object>();
            dying->setObjectID("being.dying");
            population.push_back(dying.get());
            graph.add(std::make_shared<Relation>("instance-of", *dying, target, true));
            Universe::instance().bumpStructuralRevision();
            adapter.step();
            check(adapter.candidatesFor("law.travels", candidates) &&
                      named(candidates) == scanned("instance-of", "category.target"),
                  "the dying being is a member while it lives");

            adapter.forgetBeing(dying.get());
            std::vector<Singular*> afterDeath;
            const bool served = adapter.candidatesFor("law.travels", afterDeath);
            const bool holdsPointer =
                std::find(afterDeath.begin(), afterDeath.end(), dying.get()) != afterDeath.end();
            check(!holdsPointer, "a freed being's pointer is gone from what the adapter proposes");
            check(!served, "and the road it was on is refused until rebuilt");
            population.pop_back();
            RelationManager::forgetBeingEverywhere(dying.get());
            dying.reset();
            Universe::instance().bumpStructuralRevision();
            adapter.step();
        }

        // --------------------------------------------------------------
        // F. The cap. A road bigger than the budget allows is REFUSED, not
        //    truncated — half a candidate set is a deaf law, and §3.1 says the
        //    adapter is capped, not that it may lie about what it holds.
        // --------------------------------------------------------------
        {
            Relevance::SlowAdapter tiny;
            tiny.noteLaw("law.travels", {{"instance-of", "category.target"}});
            Relevance::SlowAdapter::Budget budget;
            budget.maxMembers = 1;
            tiny.step(budget);
            std::vector<Singular*> none;
            check(!tiny.candidatesFor("law.travels", none),
                  "a road that hit its member cap is refused rather than served short");
        }

        // --------------------------------------------------------------
        // G. Direction and undirected edges, against the same oracle.
        // --------------------------------------------------------------
        graph.add(std::make_shared<Relation>("instance-of", target, stranger, true));   // wrong way
        graph.add(std::make_shared<Relation>("instance-of", other, target, false));     // undirected
        Universe::instance().bumpStructuralRevision();
        adapter.step();
        check(adapter.candidatesFor("law.travels", candidates) &&
                  named(candidates) == scanned("instance-of", "category.target"),
              "direction is honoured exactly as the condition honours it: " + join(named(candidates)));

        // --------------------------------------------------------------
        // H. Two categories: answered only when both roads are current.
        // --------------------------------------------------------------
        adapter.noteLaw("law.travels",
                        {{"instance-of", "category.target"}, {"instance-of", "category.other"}});
        check(!adapter.candidatesFor("law.travels", candidates),
              "a law naming a second category is not answered from the first road alone");
        adapter.step();
        adapter.step();
        if (adapter.candidatesFor("law.travels", candidates)) {
            auto truth = scanned("instance-of", "category.target");
            const auto second = scanned("instance-of", "category.other");
            truth.insert(truth.end(), second.begin(), second.end());
            std::sort(truth.begin(), truth.end());
            truth.erase(std::unique(truth.begin(), truth.end()), truth.end());
            check(covers(named(candidates), truth),
                  "and once both are walked it covers every being either road carries");
        } else {
            check(false, "both roads should be current after two steps");
        }

        // --------------------------------------------------------------
        // I. Reification — §3.2: the discovered path becomes beings.
        // --------------------------------------------------------------
        {
            Relevance::RouteKey route{"instance-of", "category.target"};
            auto made = adapter.reify("law.travels", route, lawBeing);
            check(made.gathering != nullptr, "reification gathers the road's Relations into a Formation");
            // The gathering is OWNED by the caller: making beings and admitting
            // them to a world are two decisions, and only the second is a
            // Person's to authorise.
            if (made.gathering) {
                check(made.gathering->getIdentifier() == "formation.route.instance-of.category.target",
                      "the gathering is a being with a stable name");
                check(!made.gathering->getMembers().empty() &&
                          made.gathering->getMembers().size() == made.gatheringToEdges.size(),
                      "it holds one Relation per edge the road carries");
                check(!made.gatheringToEdges.empty() &&
                          made.gatheringToEdges[0]->type == "gathers",
                      "and says so in the graph: Relations BETWEEN Relations");
            }
            check(made.lawToGathering != nullptr && made.lawToGathering->type == "routes-through",
                  "the Law is related to the gathering it travels");
        }

        // --------------------------------------------------------------
        // J. THE CATEGORY ITSELF IS NOT IN THE WORLD. An edge keeps the name of
        //    an endpoint it has no pointer to, and `Related(kind, name)` holds
        //    of such an edge — so a road whose category is absent still carries
        //    beings. Building it from "the category's own edges" finds nothing
        //    to start from; the road must fall back to the whole graph rather
        //    than report an empty membership, which would be a deaf law.
        // --------------------------------------------------------------
        {
            Object orbiter;  orbiter.setObjectID("being.orbiter");
            population.push_back(&orbiter);
            {
                auto ghost = std::make_unique<Object>();
                ghost->setObjectID("category.absent");
                graph.add(std::make_shared<Relation>("instance-of", orbiter, *ghost, true));
                RelationManager::forgetBeingEverywhere(ghost.get());
            }
            Universe::instance().bumpStructuralRevision();

            Relevance::SlowAdapter absent;
            absent.noteLaw("law.absent", {{"instance-of", "category.absent"}});
            absent.step();
            std::vector<Singular*> proposed;
            const bool served = absent.candidatesFor("law.absent", proposed);
            const auto truth = scanned("instance-of", "category.absent");
            check(!truth.empty(), "precondition: a being IS related to the absent category");
            check(served && named(proposed) == truth,
                  "a road whose category is not in the world still carries its beings: " +
                      join(named(proposed)));
            population.pop_back();
        }

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }

    std::printf("%s\n", g_failures ? "slow_adapter_test: FAILURES" : "slow_adapter_test: OK");
    return g_failures ? 1 : 0;
}
