// relation-state facts — the edges of the world, reaching the network.
//
// FORMATION_RETE.md §1.2(a). Earthcall burned ConditionNode::Kind 12 and 13
// (ForAnyPair / ForAllPair) on purpose, "retired in favour of modelling pairs
// as Relations" — so `Related` is not one condition among many, it is THE
// designated answer to multi-subject joins. It was deaf.
//
// The shape of the defect, and why nothing caught it:
//
//   * `relation-formed` publishes the RELATION as its subject
//     (RelationManager.cpp: `echo.subject = r.get()`), and LawManager's
//     handler called `seedStateFacts(e.subject)` — which snapshots the
//     Relation's own properties and emits no edge fact for either endpoint.
//   * `seedStateFacts` is gated by `_seededSubjects`: once per being, ever.
//     Even passed an endpoint it would have returned immediately.
//   * `_relationTypesInPlay` is filled at compile time, so a law authored
//     AFTER the world was seeded named a type nobody ever emitted facts for.
//   * Edge facts were emitted only where `relation->a() == being`, so the
//     network could traverse a->b and never b->a.
//
// And there was no safety net. A WhileTrue law with a `Related` condition
// compiles Rete terminals, so `hasTerminals` is true and it NEVER falls
// through to the sweep (Law.cpp, the continuous pass). It receives no
// candidate, `applyTo` never runs, nothing re-checks. Deaf, permanently,
// silently — the failure PROPHETIC_RETE.md §2 exists to forbid.
//
// Every section below reads `shape.fillet`, which starts at 0 and the law
// sets to 0.5. 0.000 is a deaf law; 0.500 is a law that heard.
//
// Section E is the one that must NOT change behaviour: a law may WIDEN, never
// narrow (PROPHETIC_RETE.md §2, FORMATION_RETE.md §6). A dissolved edge leaves
// its fact live — nothing retracts it — and that is deliberate. The fact only
// WAKES an alpha; the compiled predicate behind it re-reads the live graph and
// answers false. A wasted wake is the safe direction of the error.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

double filletOf(Object& obj) {
    PropertyValue v;
    if (PropertyPath::parse("shape.fillet").getValue(obj, v) != PropertyPath::PathResult::Ok) {
        return -12345.0;
    }
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

void resetFillet(Object& obj) {
    PropertyPath::parse("shape.fillet").setValue(obj, PropertyValue(0.0f));
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "rete_relation_state_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "rete_relation_state_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "rete_relation_state_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object early;    // related BEFORE the first tick — always worked
        Object late;     // related AFTER  the first tick — section A
        Object farSide;  // named from the FAR end of the edge — section C
        Object anchor;   // the other end of every edge here
        Object tardy;    // section D: edge exists before the law is authored

        // `shape.fillet` does not exist until something writes it, and an
        // unresolvable path reads as the sentinel, not as 0. Start every being
        // at a real 0 so "did not fire" and "has no such property" cannot be
        // confused for one another.
        for (Object* obj : {&author, &early, &late, &farSide, &anchor, &tardy}) {
            resetFillet(*obj);
            assert(nearf(filletOf(*obj), 0.0));
        }

        std::vector<Singular*> population{&author, &early, &late, &farSide, &anchor, &tardy};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        RelationManager graph;
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& rel : graph.getAll()) {
                if (rel) out.push_back(rel.get());
            }
        });

        LawManager mgr;
        mgr.connectToEventBus();

        ECA::Event probe;
        probe.type = "test";

        // The edge that exists before anything runs. This case worked before
        // the fix and must keep working: the first-tick seed reaches it.
        graph.add(std::make_shared<Relation>("touching", early, anchor, false));

        auto tethered = mgr.createLaw("tethered-fillet", {&author});
        tethered->setActivation(Law::Activation::WhileTrue);
        tethered->setConditionModel(ConditionNode::related("touching"));
        tethered->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        mgr.tick();
        assert(nearf(filletOf(early), 0.5));   // the case that always worked
        assert(nearf(filletOf(late), 0.0));    // not related yet — correctly silent

        // ------------------------------------------------------------------
        // A. THE REGRESSION. An edge formed after the first tick.
        //
        //    Before the fix this asserted 0.000 forever, while the raw
        //    `Related` predicate against the live graph read TRUE the whole
        //    time — the law was deaf, not wrong.
        // ------------------------------------------------------------------
        graph.add(std::make_shared<Relation>("touching", late, anchor, false));

        // The graph says yes. The question is only whether the network heard.
        assert(ConditionNode::related("touching").compile()(probe, late));

        mgr.tick();
        assert(nearf(filletOf(late), 0.5) &&
               "a relation formed after the first tick must reach the network");

        // ------------------------------------------------------------------
        // B. Idempotence. assertFact does not deduplicate, and three paths now
        //    assert edge facts. Ticking repeatedly must not grow the fact set:
        //    duplicates are a standing per-tick propagation tax and, over a
        //    session of edges forming, unbounded.
        // ------------------------------------------------------------------
        const std::size_t factsAfterFirstEdges = mgr.rete().facts().size();
        mgr.tick();
        mgr.tick();
        assert(mgr.rete().facts().size() == factsAfterFirstEdges &&
               "re-seeding the same edge must not stack duplicate facts");

        // ------------------------------------------------------------------
        // C. BOTH ENDPOINTS. Edge facts used to be emitted only where
        //    `relation->a() == being`, so the network could traverse a->b and
        //    never b->a — the one structural gap FORMATION_RETE.md §2 names.
        //
        //    `farSide` is the B end of an undirected edge. An undirected
        //    relation holds of both its ends, so the law must reach it.
        // ------------------------------------------------------------------
        graph.add(std::make_shared<Relation>("touching", anchor, farSide, false));
        assert(ConditionNode::related("touching").compile()(probe, farSide));
        mgr.tick();
        assert(nearf(filletOf(farSide), 0.5) &&
               "an undirected edge must reach the being at its far end");

        // ------------------------------------------------------------------
        // D. A LAW AUTHORED LATE. `_relationTypesInPlay` is filled at compile
        //    time and only grows, and seedStateFacts runs once per being — so
        //    a law naming a relation type for the FIRST time found that
        //    nobody had ever emitted a fact for it, and was deaf to every
        //    edge that already existed.
        //
        //    The edge is formed here while no law names "mooring", so nothing
        //    seeds it; the law arrives afterwards.
        // ------------------------------------------------------------------
        graph.add(std::make_shared<Relation>("mooring", tardy, anchor, false));
        mgr.tick();
        assert(nearf(filletOf(tardy), 0.0));   // no law names "mooring" yet

        auto moored = mgr.createLaw("moored-fillet", {&author});
        moored->setActivation(Law::Activation::WhileTrue);
        moored->setConditionModel(ConditionNode::related("mooring"));
        moored->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        mgr.tick();
        assert(nearf(filletOf(tardy), 0.5) &&
               "a law authored after an edge exists must still reach it");

        // ------------------------------------------------------------------
        // E. A DISSOLVED EDGE DOES NOT FIRE — and the mechanism matters.
        //
        //    Nothing retracts the endpoint's relation-state fact when an edge
        //    is removed: `relation-destroyed` carries the Relation as its
        //    subject, so retractStateFactsBySubject drops facts keyed on the
        //    RELATION, not on either endpoint. That is left alone on purpose.
        //    The stale fact only wakes the alpha; the compiled predicate
        //    re-reads the live graph and answers false. Widening, which §6
        //    permits — never a false fire.
        //
        //    Retracting it "properly" would be the dangerous change: facts
        //    carry no relation identity, so two edges of the same type from
        //    one being share one fact shape, and dropping it on the first
        //    dissolution would silence the second edge. That is a NARROWING,
        //    and it is why this is deliberately not done here.
        // ------------------------------------------------------------------
        resetFillet(late);
        assert(graph.removeBetween(late, anchor, "touching"));
        assert(!ConditionNode::related("touching").compile()(probe, late));

        mgr.tick();
        assert(nearf(filletOf(late), 0.0) &&
               "a dissolved edge must not fire, however stale the fact is");

        // The beings still genuinely related are untouched by that removal.
        assert(nearf(filletOf(early), 0.5));
        assert(nearf(filletOf(farSide), 0.5));

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("rete_relation_state_test: OK\n");
    return 0;
}
