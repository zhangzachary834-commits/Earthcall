// A typed `Related` law must behave identically whether the Prophetic write
// filter is on or forced open.
//
// FORMATION_RETE.md §8 rung 4, 2026-09-14 (Claude Opus 5). The rung was built to
// stop `Related` walking every relation per candidate. Built and oracle-tested
// (relation_endpoint_index_test), the endpoint index bought nothing: a
// category-scoped law still cost ~10x the same law reading a property. Profiled,
// the cost was not in the graph at all. Prophetic analysis marked every Related
// read OPAQUE, which made the index incomplete, which turned
// LawManager::propheticHears off for every property write in the world — so each
// unrelated write (the law's own `add position.z`) went through markFactDirty ->
// evaluateDirty -> retractFact, a linear walk of ~49k facts at 400 beings.
//
// The fix declares what a typed Related actually hears (PropheticRete.cpp,
// `case ConditionNode::Kind::Related`). That is a NARROWING of the filter, and
// PROPHETIC_RETE.md §2 allows a narrowing only where it is proved. This test is
// the proof we can run: every step of a relation-graph scenario is played twice —
//   LEGIBLE: only Related laws, so the filter is live;
//   OPEN:    the same, plus one Overlaps law, which is opaque and forces the
//            filter to hear everything (the old behaviour);
// and every observable must agree after every step. A disagreement is a Related
// law that went deaf because the filter stopped passing a write it depended on.
//
// FOR FUTURE AGENTS (Jules especially): if you change what a Related Rete node
// wakes on (ConditionNode::compileToRete's attribute filter), or add a field to
// Relation that the Related predicate reads, update the Related case in
// PropheticRete.cpp and add a step here that writes it.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

double read(Singular& being, const char* path) {
    PropertyValue v;
    if (PropertyPath::parse(path).getValue(being, v) != PropertyPath::PathResult::Ok) return -12345.0;
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

void write(Singular& being, const char* path, double value) {
    PropertyPath::parse(path).setValue(being, PropertyValue(static_cast<float>(value)));
}

// One full playthrough. Returns every observable, step by step.
std::vector<double> play(bool forceOpen) {
    std::vector<double> trace;

    Object author;
    Object a; a.setObjectID("being-a");
    Object b; b.setObjectID("being-b");
    Object c; c.setObjectID("being-c");
    Object target; target.setObjectID("category.target");
    std::vector<Object*> subjects{&a, &b, &c};
    for (Object* o : subjects) { write(*o, "position.z", 0.0); write(*o, "shape.fillet", 0.0); }

    std::vector<Singular*> population{&author, &a, &b, &c, &target};
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        for (Singular* s : population) out.push_back(s);
    });
    RelationManager graph;
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& r : graph.getAll()) if (r) out.push_back(r.get());
    });
    Universe::instance().setRelationsInvolvingProvider(
        [&](const Singular& being, std::vector<Relation*>& out) { graph.relationsInvolving(being, out); });
    Universe::instance().setClock(100.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    graph.add(std::make_shared<Relation>("instance-of", a, target, true));

    auto level = mgr.createLaw("member-level", {&author});
    level->setActivation(Law::Activation::WhileTrue);
    level->setConditionModel(ConditionNode::related("instance-of", "category.target"));
    level->setActionModel(ActionNode::add("position.z", 1.0));

    auto edge = mgr.createLaw("member-edge", {&author});
    edge->setActivation(Law::Activation::OnBecomeTrue);
    edge->setConditionModel(ConditionNode::related("instance-of", "category.target"));
    edge->setActionModel(ActionNode::add("shape.fillet", 1.0));

    if (forceOpen) {
        auto opaque = mgr.createLaw("force-open", {&author});
        opaque->setActivation(Law::Activation::WhileTrue);
        opaque->setConditionModel(ConditionNode::overlaps("@event.object"));
        opaque->setActionModel(ActionNode::set("force-open-marker", PropertyValue(1.0f)));
    }

    const auto snapshot = [&](const char* step) {
        for (Object* o : subjects) {
            trace.push_back(read(*o, "position.z"));
            trace.push_back(read(*o, "shape.fillet"));
        }
        (void)step;
    };
    const auto ticks = [&](int n, const char* step) {
        for (int i = 0; i < n; ++i) mgr.tick();
        snapshot(step);
    };

    ticks(3, "edge formed before the first tick");

    // The gate is really in the state this arm claims.
    if (forceOpen) {
        assert(!mgr.prophetic().complete());
        assert(mgr.propheticHears("position"));
    } else {
        assert(mgr.prophetic().complete() && "a typed Related must leave the index complete");
        assert(!mgr.propheticHears("position") && "an unrelated write must be filtered");
        assert(mgr.propheticHears("instance-of") && mgr.propheticHears("instance-of.weight") &&
               "the relation type is heard by root");
        assert(mgr.propheticHears("type") && mgr.propheticHears("directed"));
    }

    graph.add(std::make_shared<Relation>("instance-of", b, target, true));
    ticks(3, "edge formed after the first tick");

    for (Object* o : subjects) write(*o, "position.y", 7.0);   // unrelated writes
    ticks(2, "unrelated writes");

    assert(graph.removeBetween(a, target, "instance-of"));
    ticks(3, "edge removed");

    graph.add(std::make_shared<Relation>("instance-of", a, target, true));
    ticks(3, "edge re-formed (OnBecomeTrue must fire again)");

    auto wrongKind = std::make_shared<Relation>("near", c, target, true);
    graph.add(wrongKind);
    ticks(2, "edge of another type");
    write(*wrongKind, "weight", 0.5);
    PropertyPath::parse("type").setValue(*wrongKind, PropertyValue(std::string("instance-of")));
    ticks(3, "edge retyped through its property");

    // Retyped through its Lexeme (Relation::setTypeLexeme), which wrote `type`
    // directly and announced nothing until 2026-09-14. Appended after the
    // six-slot snapshots as two extra values: [d.z, d.fillet].
    Object d; d.setObjectID("being-d");
    write(d, "position.z", 0.0); write(d, "shape.fillet", 0.0);
    population.push_back(&d);
    auto lexemeKind = std::make_shared<Relation>("near", d, target, true);
    graph.add(lexemeKind);
    mgr.tick();
    Singularity::Language::Lexeme instanceOf("instance of", "instance-of");
    lexemeKind->setTypeLexeme(&instanceOf);
    assert(lexemeKind->type == "instance-of");
    for (int i = 0; i < 3; ++i) mgr.tick();
    trace.push_back(read(d, "position.z"));
    trace.push_back(read(d, "shape.fillet"));
    graph.remove(lexemeKind);   // before `instanceOf` leaves scope
    population.pop_back();

    Universe::instance().setRelationProvider(nullptr);
    Universe::instance().setProvider(nullptr);
    return trace;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "related_prophetic_legibility_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "related_prophetic_legibility_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "related_prophetic_legibility_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const std::vector<double> legible = play(false);
    const std::vector<double> open = play(true);
    assert(legible.size() == open.size());

    bool agree = true;
    for (std::size_t i = 0; i < legible.size(); ++i) {
        const bool same = std::fabs(legible[i] - open[i]) < 1e-4;
        std::printf("  [%2zu] legible %8.3f  open %8.3f %s\n", i, legible[i], open[i], same ? "" : "<-- DIFFERS");
        agree = agree && same;
    }
    assert(agree && "the write filter changed what a typed Related law did");

    // Agreement alone could be agreement on a wrong answer — the first run of
    // this test showed exactly that for the last two steps, identically in both
    // arms. So the answers are pinned too. Each snapshot is six values:
    // [a.z, a.fillet, b.z, b.fillet, c.z, c.fillet].
    const auto at = [&](int step, int slot) { return legible[step * 6 + slot]; };
    assert(at(0, 0) > 0.5 && "being-a's WhileTrue law never fired");
    assert(at(0, 1) == 1.0 && "OnBecomeTrue fires once while the edge holds");
    assert(at(1, 2) > 0.5 && at(1, 3) == 1.0 && "an edge formed later reaches both laws");
    {
        const double settled = at(3, 0);
        assert(at(4, 0) > settled && "the WhileTrue law resumes when the edge re-forms");
    }
    // THE RETRACTION HALF (Law.hpp, _relationStateToRevalidate). Before it, a
    // removed edge's fact stayed behind, the subject was never released, and
    // re-forming the edge produced no false->true transition: fillet stayed 1.
    assert(at(4, 1) == 2.0 && "OnBecomeTrue must fire again when a removed edge re-forms");
    assert(at(5, 4) == 0.0 && "an edge of another type is not membership");
    // A Relation retyped through its `type` property. Before, its endpoint never
    // received a fact of the new type and the law stayed deaf to it.
    assert(at(6, 4) > 0.5 && at(6, 5) == 1.0 &&
           "an edge retyped into the category must reach the laws");
    assert(legible[7 * 6] > 0.5 && legible[7 * 6 + 1] == 1.0 &&
           "an edge retyped through its Lexeme must reach the laws");

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("related_prophetic_legibility_test: OK\n");
    return 0;
}
