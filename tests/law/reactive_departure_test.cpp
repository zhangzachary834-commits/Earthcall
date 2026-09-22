// A being that stops satisfying a law must be RELEASED on the reactive path.
//
// FORMATION_RETE.md §8 rung 7, "departure reporting on the reactive path",
// 2026-09-15 (Claude Opus 5; Zach: "NOW DO THE NEXT PHASE").
//
// A connected LawManager decides which subjects hold a WhileTrue/OnBecomeTrue law
// from its Rete terminal memory. That memory keeps a subject for as long as ANY
// fact about it once passed the alpha. But the alpha's predicate reads the whole
// subject, and its attribute filter names only one attribute:
//
//   Compare(x > y)       filters on x   -> y turning it false re-evaluates nothing
//   InRegion(sphere)     no filter      -> the facts that passed while inside stay
//   Zone(a - b >= 0)     no filter      -> the same
//   Related(T, farEnd)   filters on T   -> the far end changing keeps the T fact
//
// So the subject was never released. An OnBecomeTrue law never re-armed: it fired
// once in its lifetime, however often its condition went false and true again.
// A WhileTrue law's onset (time.sinceApplied) never reset. Nothing reported it.
// Measured before the fix: each red case below stayed at 1 (Related at 1 too);
// `All` of two plain compares was already correct, because retracting the changed
// fact drops the join token — kept as the control.
//
// Fix: LawManager::tick verifies terminal candidates against the live world
// before counting them as holding. Membership proposes; the condition decides.
//
// FOR FUTURE AGENTS (Jules especially): if you make that verification
// conditional ("only for inexact laws") for speed, every case here must stay
// green, and you must PROVE the exactness claim per condition kind — a wrong
// "exact" is this bug again. Add a case for any new condition kind.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {
double hits(Object& o) {
    PropertyValue v;
    if (PropertyPath::parse("hits").getValue(o, v) != PropertyPath::PathResult::Ok) return -1;
    double d = -1; propertyValueToNumber(v, d); return d;
}
void setNum(Object& o, const char* name, double v) { o.setDynamicProperty(name, PropertyValue(v)); }
void place(Object& o, float x) { PropertyPath::parse("position").setValue(o, PropertyValue(glm::vec3(x, 0.0f, 0.0f))); }
} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "reactive_departure_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "reactive_departure_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    {
        Object author;
        // Every subject starts far from the region, so only its own law can reach it.
        Object byPath, byRegion, byZone, byRelated, control, level;
        for (Object* o : {&byPath, &byRegion, &byZone, &byRelated, &control, &level}) {
            setNum(*o, "hits", 0.0);
            place(*o, 50.0f);
        }
        byRelated.setObjectID("byRelated");
        Object target;  target.setObjectID("category.target");  place(target, 50.0f);
        Object other;   other.setObjectID("category.other");    place(other, 50.0f);

        setNum(byPath, "x", 10.0);    setNum(byPath, "y", 0.0);
        place(byRegion, 0.0f);
        setNum(byZone, "a", 5.0);     setNum(byZone, "b", 1.0);
        setNum(control, "hp", 10.0);  setNum(control, "gate", 1.0);
        setNum(level, "lx", 10.0);    setNum(level, "ly", 0.0);

        std::vector<Singular*> population{&author, &byPath, &byRegion, &byZone, &byRelated,
                                          &control, &level, &target, &other};
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
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

        graph.add(std::make_shared<Relation>("instance-of", byRelated, target, true));
        // A second edge of the SAME type, to a different category: when the
        // target edge goes, the relation-state fact for `instance-of` rightly stays.
        graph.add(std::make_shared<Relation>("instance-of", byRelated, other, true));

        const auto edgeLaw = [&](const char* name, ConditionNode condition, Object& only) {
            auto law = mgr.createLaw(name, {&author});
            law->setActivation(Law::Activation::OnBecomeTrue);
            law->setConditionModel(std::move(condition));
            law->setActionModel(ActionNode::add("hits", 1.0));
            law->addTarget(only);
            return law;
        };

        using OntoMath::ScalarForm; using OntoMath::Term;
        ScalarForm aMinusB;
        aMinusB.terms.push_back(Term(1.0, {{"a", 1.0}}));
        aMinusB.terms.push_back(Term(-1.0, {{"b", 1.0}}));

        edgeLaw("by-path", ConditionNode::comparePaths("x", ConditionNode::Op::Gt, "y"), byPath);
        edgeLaw("by-region", ConditionNode::inRegion(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f))), byRegion);
        edgeLaw("by-zone", ConditionNode::zone(
            OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(aMinusB)),
            MathBindings{{"a", PropertyPath::parse("a")}, {"b", PropertyPath::parse("b")}},
            PropertyValue(0.0), PropertyValue{}), byZone);
        edgeLaw("by-related", ConditionNode::related("instance-of", "category.target"), byRelated);
        edgeLaw("control", ConditionNode::all({
            ConditionNode::compare("hp", ConditionNode::Op::Gt, PropertyValue(0.0)),
            ConditionNode::compare("gate", ConditionNode::Op::Gt, PropertyValue(0.0))}), control);

        auto levelLaw = mgr.createLaw("level", {&author});
        levelLaw->setActivation(Law::Activation::WhileTrue);
        levelLaw->setConditionModel(ConditionNode::comparePaths("lx", ConditionNode::Op::Gt, "ly"));
        levelLaw->setActionModel(ActionNode::add("hits", 1.0));
        levelLaw->addTarget(level);

        const auto ticks = [&](int n) { for (int i = 0; i < n; ++i) mgr.tick(); };
        const auto report = [&](const char* phase) {
            std::printf("  %-12s path=%.0f region=%.0f zone=%.0f related=%.0f control=%.0f level=%.0f\n",
                        phase, hits(byPath), hits(byRegion), hits(byZone), hits(byRelated),
                        hits(control), hits(level));
        };

        // 1. Holds: every edge law fires exactly once.
        ticks(3);
        report("holds");
        for (Object* o : {&byPath, &byRegion, &byZone, &byRelated, &control}) assert(hits(*o) == 1.0);
        assert(hits(level) == 3.0);

        // 2. Each condition goes false through the attribute its filter does NOT name.
        setNum(byPath, "y", 20.0);
        place(byRegion, 5.0f);
        setNum(byZone, "b", 9.0);
        assert(graph.removeBetween(byRelated, target, "instance-of"));
        setNum(control, "gate", 0.0);
        setNum(level, "ly", 20.0);
        ticks(3);
        report("false");
        for (Object* o : {&byPath, &byRegion, &byZone, &byRelated, &control}) assert(hits(*o) == 1.0);
        assert(hits(level) == 3.0 && "a WhileTrue law must stop applying when its condition fails");
        assert(!levelLaw->lastConditionState(&level) &&
               "a WhileTrue subject whose condition failed must be released (its onset resets)");

        // 3. True again: every edge law fires exactly once more.
        setNum(byPath, "y", 0.0);
        place(byRegion, 0.0f);
        setNum(byZone, "b", 1.0);
        graph.add(std::make_shared<Relation>("instance-of", byRelated, target, true));
        setNum(control, "gate", 1.0);
        setNum(level, "ly", 0.0);
        ticks(3);
        report("true again");
        assert(hits(control) == 2.0 && "control: a joined condition re-arms");
        assert(hits(byPath) == 2.0 && "Compare(x > y): departure through the operand must re-arm");
        assert(hits(byRegion) == 2.0 && "InRegion: leaving and re-entering must re-arm");
        assert(hits(byZone) == 2.0 && "Zone: leaving the satisfaction zone must re-arm");
        assert(hits(byRelated) == 2.0 && "Related: losing the named far end must re-arm");
        assert(hits(level) == 6.0 && "WhileTrue resumes");

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("reactive_departure_test: OK\n");
    return 0;
}
