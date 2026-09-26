#pragma once

#include "Relation/Relation.hpp"
#include "Relation/Traversal/RelevanceTraversal.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// THE SLOW ADAPTER — FORMATION_RETE.md §3.1, §3.2, §3.3, §8 rungs 5 and 6.
//
// Zach, 2026-09-16, on where this belongs:
//
//   "The mechanism that creates Relations between Relations and pre-loads Law
//    Relations to these Relation Formations is also supposed to be in the slow
//    adapter rather than constantly rebuilt every frame."
//
// That is the whole design. A Law whose condition says `Related(instance-of,
// category.chess.piece)` travels the same road every tick. Today the engine
// re-derives that road per frame — the vocabulary index rebuilds on structural
// change (measured at 132-208 ms in Synthesis Studio before it was made cheap),
// and the Rete re-proposes candidates from fact memories. The adapter instead
// **pre-loads the road**: once, slowly, off the frame path, it walks the graph
// to find the Relations that carry a category's membership, gathers them, and
// connects the Law to that gathering. Ticks then traverse what is already there.
//
// §3.1's constraints, kept literally:
//   * an INDEPENDENT CLOCK: `step()` does a bounded amount of work when the
//     caller has time, never "rebuild everything now";
//   * CAPPED: every step states how many routes it may build and refresh;
//   * TWO RATES (§4B, HNSW rots under update): an IMPROVE rate that builds
//     roads not yet known, and a REVISIT rate that re-checks the oldest known
//     one, because a world that never re-checks "found better routes once".
//   * IT DOES NOT EVALUATE TRUTH. It proposes candidates. The Law's own
//     conditions still decide, and the sweep stays the correctness floor (§6).
//
// WHY THAT LAST LINE IS NOT A FORMALITY HERE. A candidate set that MISSES a
// being is a silently deaf law — the failure this whole document family exists
// to prevent. So the adapter refuses to answer unless it can show its work is
// current: it tracks the world's structural revision and the relation graph's
// own generation, and `candidatesFor` returns false the moment either has moved
// past what it maintained. A caller that gets false sweeps, exactly as before.
// Guarded by tests/relation/slow_adapter_test.cpp, which checks every answer
// against a full scan.
//
// REIFICATION (Relations between Relations, in the world) is implemented and
// deliberately NOT run by the engine — see `reify()`. It writes Relations into
// the Person's world, which a save then carries, and save files are sacred.
//
// FOR FUTURE AGENTS (Jules especially):
//   * Never make `candidatesFor` answer optimistically. If you are unsure
//     whether the membership is current, return false: sweeping is slow, a deaf
//     law is a bug nobody sees.
//   * Every new way the relation graph can change must either bump
//     RelationManager's generation (it does, through touch()) or be observed
//     here. A change the adapter cannot see is a route it will keep serving.
//   * Keep `step()` bounded. The moment it does unbounded work it is a per-frame
//     rebuild wearing a different name, which is the thing Zach's note above
//     rules out.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md
// ---------------------------------------------------------------------------


// Zach: The adapter makes Chess takes longer. I'm going to personally audit to investigate why.

class Formation;

namespace Relevance {

// One road: "beings related to <categoryId> by <relationKind>".
struct RouteKey {
    std::string relationKind;
    std::string categoryId;
    bool operator==(const RouteKey& other) const {
        return relationKind == other.relationKind && categoryId == other.categoryId;
    }
};

struct RouteKeyHash {
    std::size_t operator()(const RouteKey& key) const {
        return std::hash<std::string>{}(key.relationKind) ^
               (std::hash<std::string>{}(key.categoryId) << 1);
    }
};

class SlowAdapter {
public:
    struct Budget {
        std::size_t improve = 1;   // roads built per step
        std::size_t revisit = 1;   // roads re-checked per step
        std::size_t maxMembers = 4096;   // cap on one road's membership
    };

    // What a Law travels: its `Related(kind, category)` leaves. Called when law
    // text changes, not per tick.
    void noteLaw(const std::string& lawId, const std::vector<std::pair<std::string, std::string>>& routes);
    void forgetLaw(const std::string& lawId);

    // One bounded unit of work on the adapter's own clock. Returns how many
    // roads it touched, so a caller can tell a working step from an idle one.
    std::size_t step(const Budget& budget);
    std::size_t step();

    // Declare that this adapter is in use. Currency is decided by two counters
    // the world already keeps: Universe::structuralRevision (beings arriving,
    // leaving, gaining properties) and RelationManager::generation through
    // Universe::relationGeneration (every write to the relation graph). A road
    // built under different numbers is refused, never served stale.
    void observe();

    // A being is being freed. Its pointer leaves every road immediately, and the
    // roads that held it are marked for rebuilding. Called from the one callback
    // that hears about every death (LawManager's, via Singular).
    void forgetBeing(const Singular* being);

    // Candidates for a Law, or false when the adapter cannot show its work is
    // current. False is not "no candidates": it is "ask the sweep".
    bool candidatesFor(const std::string& lawId, std::vector<Singular*>& out) const;

    // True when every road this Law travels is built and current.
    bool ready(const std::string& lawId) const;

    // Reification — §3.2's "the discovered paths become beings". Creates, in the
    // world: a Formation of the Relations that carry one road's membership, and
    // a Relation from the Law to that Formation. NOT called by the engine; the
    // Person's saved world is not written to on the adapter's initiative.
    // `author` is recorded as the author of what is made.
    struct Reified {
        // Owned by the caller: reification MAKES beings, and who admits them to
        // a world (and records their author) is that caller's decision, not
        // this structure's. Returned owning rather than raw so the mechanism
        // cannot leak while it waits for a world to be admitted into.
        std::unique_ptr<Formation> gathering;
        std::shared_ptr<Relation> lawToGathering;
        std::vector<std::shared_ptr<Relation>> gatheringToEdges;
    };
    Reified reify(const std::string& lawId, const RouteKey& route, Singular& author);

    // Counters, for the report and for tests.
    std::size_t roadsKnown() const { return _roads.size(); }
    std::size_t roadsPending() const { return _pending.size(); }
    void clear();

private:
    struct Road {
        std::vector<Singular*> members;      // beings on the far side of the edges
        std::vector<Relation*> edges;        // the Relations that carry them
        std::uint64_t builtAtStructural = 0;
        std::size_t builtAtGraph = 0;
        bool built = false;
        bool cappedOut = false;
        std::uint64_t lastVisited = 0;       // the revisit rate's clock
    };

    void build(const RouteKey& key, const Budget& budget);
    bool current(const Road& road) const;

    bool _observing = false;

    std::unordered_map<RouteKey, Road, RouteKeyHash> _roads;
    std::unordered_map<std::string, std::vector<RouteKey>> _lawRoutes;
    std::deque<RouteKey> _pending;
    std::uint64_t _steps = 0;
};

}  // namespace Relevance
