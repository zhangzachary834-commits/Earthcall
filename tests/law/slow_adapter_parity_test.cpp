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
    // ONE LawManager, toggled between phases, so the only variable between
    // arms is the adapter flag. Historically this was also a workaround for
    // EventBus listeners surviving a destroyed manager: a second connected
    // manager made the second arm count everything twice. That lifecycle bug is
    // now guarded independently by law_manager_eventbus_lifetime_test.
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
    std::uint64_t relationQueries = 0;
    Universe::instance().setRelationsInvolvingProvider(
        [&](const Singular& being, std::vector<Relation*>& out) {
            ++relationQueries;
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
    auto requiredRoad = eventLaw("required-road", ConditionNode::all({onTheRoad}), 1.0);
    eventLaw("either-way", ConditionNode::any({onTheRoad, blessed}), 10.0);
    eventLaw("off-the-road", ConditionNode::all({ConditionNode::negate(onTheRoad), blessed}), 100.0);
    auto mixedPolarity = eventLaw(
        "mixed-polarity",
        ConditionNode::all({onTheRoad, ConditionNode::negate(onTheRoad)}),
        1000.0);

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

    // This test explicitly measures both modes even though the adapter now ships
    // ON. Off must still mean off: no roads maintained and no query overhead.
    mgr.setUseSlowAdapter(false);
    ring(1);
    check(mgr.slowAdapter().roadsKnown() == 0,
          "while the adapter is off it maintains nothing");
    check(mgr.candidateTierFor(*requiredRoad) == "vocabulary",
          "with no retained road, the Law selects the vocabulary tier");
    const std::uint64_t steadySelections = mgr.candidateRouteRefreshCount();

    relationQueries = 0;
    reset();  ring(6);
    const std::string sweeping = snapshot(nullptr);
    const std::uint64_t sweepingQueries = relationQueries;
    check(mgr.candidateRouteRefreshCount() == steadySelections,
          "steady frames reuse the cached tier instead of reselecting it");

    // Turning it on re-registers the laws' roads on the SLOW CLOCK, not in
    // LawManager::tick(). Warm only through serviceSlowAdapterClock so this
    // parity test also guards the independent scheduling boundary.
    mgr.setUseSlowAdapter(true);
    mgr.setUseLawDirect(false);
    double adapterWall = 0.0;
    mgr.serviceSlowAdapterClock(adapterWall); // prime only
    for (int i = 0; i < 4; ++i) {
        adapterWall += mgr.slowAdapterClockPeriodSeconds();
        mgr.serviceSlowAdapterClock(adapterWall);
    }
    check(mgr.slowAdapter().roadsKnown() == 1,
          "turned on, it knows the one road these laws travel (the Any and Not laws yield none)");
    check(mgr.slowAdapter().ready(requiredRoad->getIdentifier()),
          "and it has walked it");
    check(mgr.candidateTierFor(*requiredRoad) == "adapter-road",
          "with Direct disabled, the current retained road reproduces the pre-Direct tier");
    relationQueries = 0;
    reset(); ring(6);
    const std::string preDirect = snapshot(nullptr);
    const std::uint64_t preDirectQueries = relationQueries;

    mgr.setUseLawDirect(true);
    check(mgr.candidateTierFor(*requiredRoad) == "law-direct",
          "the same retained road crystallizes into Law-Direct");
    check(mgr.candidateTierFor(*mixedPolarity) == "law-direct",
          "positive conjunct can go Direct while the same route under Not stays live");
    const std::uint64_t promotedSelections = mgr.candidateRouteRefreshCount();
    for (int i = 0; i < 3; ++i) {
        adapterWall += mgr.slowAdapterClockPeriodSeconds();
        mgr.serviceSlowAdapterClock(adapterWall);
        check(mgr.candidateTierFor(*requiredRoad) == "law-direct",
              "an unchanged direct route remains selected across maintenance revisits");
    }
    check(mgr.candidateRouteRefreshCount() == promotedSelections,
          "slow-clock revisits of an unchanged road do not churn tier selection");
    relationQueries = 0;
    reset();  ring(6);
    const std::string travelling = snapshot(nullptr);
    const std::uint64_t directQueries = relationQueries;
    check(directQueries < preDirectQueries,
          "Law-Direct performs fewer relation-graph queries than the immediately-pre-Direct tier");

    // Make the retained road exactly as wide as the lower vocabulary route.
    // Higher tier != better tier: equal fan-out must descend rather than pay
    // route machinery for no narrowing.
    graph.add(std::make_shared<Relation>("instance-of", outsider, target, true));
    adapterWall += mgr.slowAdapterClockPeriodSeconds();
    mgr.serviceSlowAdapterClock(adapterWall);
    check(mgr.candidateTierFor(*requiredRoad) == "law-direct",
          "equal-width Law-Direct remains useful because it removes proved condition work");

    // A member admitted mid-run, with the adapter ON: its road must be rebuilt,
    // not kept.
    Object latecomer;  latecomer.setObjectID("member.late");
    latecomer.setDynamicProperty("hits", PropertyValue(0.0));
    latecomer.setDynamicProperty("blessed", PropertyValue(0.0));
    population.push_back(&latecomer);
    graph.add(std::make_shared<Relation>("instance-of", latecomer, target, true));
    Universe::instance().bumpStructuralRevision();
    // The stale road is safe immediately: the tier query descends before the
    // independent clock is allowed to catch up.
    check(mgr.candidateTierFor(*requiredRoad) == "vocabulary",
          "a stale retained road immediately falls to the lower complete tier");
    adapterWall += mgr.slowAdapterClockPeriodSeconds();
    mgr.serviceSlowAdapterClock(adapterWall);
    reset();  ring(6);
    const std::string withLate = snapshot(&latecomer);
    mgr.setUseSlowAdapter(false);  reset();  latecomer.setDynamicProperty("hits", PropertyValue(0.0));  ring(6);
    const std::string withLateSwept = snapshot(&latecomer);

    std::printf("  adapter off       : %s (relation queries=%llu)\n", sweeping.c_str(),
                static_cast<unsigned long long>(sweepingQueries));
    std::printf("  adapter pre-direct: %s (relation queries=%llu)\n", preDirect.c_str(),
                static_cast<unsigned long long>(preDirectQueries));
    std::printf("  adapter + direct  : %s (relation queries=%llu)\n", travelling.c_str(),
                static_cast<unsigned long long>(directQueries));
    check(sweeping == preDirect && preDirect == travelling,
          "all three tiers reach the same beings the same number of times");

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

    std::printf("%s\n", g_failures ? "slow_adapter_parity_test: FAILURES"
                                   : "slow_adapter_parity_test: OK");
    return g_failures ? 1 : 0;
}
