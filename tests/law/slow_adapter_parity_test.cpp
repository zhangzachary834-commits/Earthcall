// Turning the slow adapter on must change WHEN work happens, never WHO is reached.
//
// FORMATION_RETE.md §8 rungs 5-6. The adapter pre-loads the road a Law travels
// (`Related(kind, category)`) on its own clock, and `sweepSubjects` reads that
// instead of deriving candidates per frame. Zach, 2026-09-16: "The mechanism that
// creates Relations between Relations and pre-loads Law Relations to these
// Relation Formations is also supposed to be in the slow adapter rather than
// constantly rebuilt every frame."
//
// A candidate source is only ever allowed to WIDEN (PROPHETIC_RETE.md §2). So
// this test runs the same world twice — adapter off, adapter on — and demands
// the same beings are reached, tick for tick. The interesting cases are the ones
// where a road is NOT the set of beings that satisfy the law:
//
//   * `Any(Related(instance-of, X), Compare(...))` — a being can qualify through
//     the OTHER arm without being on the road at all. Proposing only the road
//     would silence it.
//   * `Not(Related(instance-of, X))` — being ON the road is what DISqualifies.
//     The candidates are everyone else.
//   * `ForAny(...)` — the inner condition is about the instances it ranges over,
//     not about the law's subject.
//
// `ConditionNode::collectCategoryRoutes` is what keeps those safe: it only walks
// `All` chains, so a Related under Any/Not/a quantifier yields no road and the
// law keeps sweeping. If you widen that collector, this test is what tells you
// the law went deaf.
//
// FOR FUTURE AGENTS (Jules especially): every law shape below must reach the same
// beings with the adapter on and off. If you add a condition kind that can carry
// a `Related`, add it here in both an All and an Any.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
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


double hits(Object& o) {
    PropertyValue v;
    if (PropertyPath::parse("hits").getValue(o, v) != PropertyPath::PathResult::Ok) return -1;
    double d = -1; propertyValueToNumber(v, d); return d;
}

} // namespace


int main() {
    if (!glfwInit()) { std::fprintf(stderr, "slow_adapter_parity_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "slow_adapter_parity_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    // ONE LawManager, toggled between phases — not two managers run back to
    // back. The EventBus has no unsubscribe (Law.hpp says so: "a connected
    // LawManager must outlive all publishing"), so a second connected manager in
    // one process is heard by the first one's still-live handler. Measured while
    // writing this test: whichever arm ran second counted everything twice, with
    // the adapter playing no part in it.
    Object author;
    Object target;   target.setObjectID("category.target");
    Object member1;  member1.setObjectID("member.one");
    Object member2;  member2.setObjectID("member.two");
    Object outsider; outsider.setObjectID("outsider");   // qualifies only via the Any arm
    std::vector<Object*> watched{&member1, &member2, &outsider};
    outsider.setDynamicProperty("blessed", PropertyValue(1.0));
    member1.setDynamicProperty("blessed", PropertyValue(0.0));
    member2.setDynamicProperty("blessed", PropertyValue(0.0));
    for (Object* o : watched) o->setDynamicProperty("hits", PropertyValue(0.0));

    std::vector<Singular*> population{&author, &target, &member1, &member2, &outsider};
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
    Universe::instance().setClock(100.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    graph.add(std::make_shared<Relation>("instance-of", member1, target, true));
    graph.add(std::make_shared<Relation>("instance-of", member2, target, true));

    // EVENT laws scoped to EVERYONE — the shape that actually reaches
    // `sweepSubjects`, and the shape chess is written in (283 of the 353 laws in
    // the saved worlds are OnEvent, 92 of them Scope::Everyone). A CONTINUOUS law
    // carrying a `Related` conjunct compiles Rete terminals from that very leaf
    // and takes the reactive path instead, so it never sweeps and the adapter
    // never sees it — which the first draft of this test got wrong, and why it
    // passed with the soundness rule mutated away.
    const auto onTheRoad = ConditionNode::related("instance-of", "category.target");
    const auto blessed = ConditionNode::compare("blessed", ConditionNode::Op::Gt, PropertyValue(0.5));
    const auto eventLaw = [&](const char* name, ConditionNode condition, double amount) {
        auto law = mgr.createLaw(name, {&author});
        law->setActivation(Law::Activation::OnEvent);
        law->setScope(Law::Scope::Everyone);
        law->setConditionModel(std::move(condition));
        law->setActionModel(ActionNode::add("hits", amount));
        mgr.bindTrigger(law->getIdentifier(), "the-bell");
        return law;
    };
    eventLaw("required-road", ConditionNode::all({onTheRoad}), 1.0);
    eventLaw("either-way", ConditionNode::any({onTheRoad, blessed}), 10.0);
    eventLaw("off-the-road", ConditionNode::all({ConditionNode::negate(onTheRoad), blessed}), 100.0);

    const auto ring = [&](int times) {
        for (int i = 0; i < times; ++i) {
            ECA::Event bell{"the-bell", &author, nullptr, 0};
            Core::EventBus::instance().publish(bell);
            mgr.tick();
        }
    };
    const auto reset = [&]() { for (Object* o : watched) o->setDynamicProperty("hits", PropertyValue(0.0)); };
    const auto snapshot = [&](Object* late) {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "one=%.0f two=%.0f outsider=%.0f late=%.0f",
                      hits(member1), hits(member2), hits(outsider), late ? hits(*late) : 0.0);
        return std::string(buffer);
    };

    // OFF is the default, and off means off: no roads are noted and none walked,
    // so the adapter costs nothing at all until someone turns it on.
    ring(1);
    check(mgr.slowAdapter().roadsKnown() == 0,
          "while the adapter is off it maintains nothing");

    mgr.setUseSlowAdapter(false);  reset();  ring(6);
    const std::string sweeping = snapshot(nullptr);

    // Turning it on re-registers the laws' roads; warm it so the phase below
    // really reads a pre-loaded road rather than quietly falling back to the
    // sweep and proving nothing.
    mgr.setUseSlowAdapter(true);
    ring(1);
    for (int i = 0; i < 4; ++i) mgr.slowAdapter().step();
    check(mgr.slowAdapter().roadsKnown() == 1,
          "turned on, it knows the one road these laws travel (the Any and Not laws yield none)");
    check(mgr.slowAdapter().ready("law-1"),
          "and it has walked it");
    reset();  ring(6);
    const std::string travelling = snapshot(nullptr);

    // A member admitted mid-run, with the adapter ON: its road must be rebuilt,
    // not kept.
    Object latecomer;  latecomer.setObjectID("member.late");
    latecomer.setDynamicProperty("hits", PropertyValue(0.0));
    latecomer.setDynamicProperty("blessed", PropertyValue(0.0));
    population.push_back(&latecomer);
    graph.add(std::make_shared<Relation>("instance-of", latecomer, target, true));
    Universe::instance().bumpStructuralRevision();
    reset();  ring(6);
    const std::string withLate = snapshot(&latecomer);
    mgr.setUseSlowAdapter(false);  reset();  latecomer.setDynamicProperty("hits", PropertyValue(0.0));  ring(6);
    const std::string withLateSwept = snapshot(&latecomer);

    std::printf("  adapter off: %s\n", sweeping.c_str());
    std::printf("  adapter on : %s\n", travelling.c_str());
    check(sweeping == travelling, "the same beings are reached, the same number of times");

    // Not vacuous: each law must actually have fired, or the comparison above is
    // two identical zeroes agreeing with each other.
    check(sweeping.find("one=0 ") == std::string::npos, "the road law fired");
    check(sweeping.find("outsider=0 ") == std::string::npos,
          "the being that qualifies only through the OTHER arm was reached");
    std::printf("  mid-run member, adapter on : %s\n", withLate.c_str());
    std::printf("  mid-run member, adapter off: %s\n", withLateSwept.c_str());
    check(withLate == withLateSwept, "and the same after a member is admitted mid-run");
    check(withLate.find("late=0") == std::string::npos, "the being added mid-run was reached");

    Universe::instance().setRelationProvider(nullptr);
    Universe::instance().setProvider(nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("%s\n", g_failures ? "slow_adapter_parity_test: FAILURES"
                                   : "slow_adapter_parity_test: OK");
    return g_failures ? 1 : 0;
}
