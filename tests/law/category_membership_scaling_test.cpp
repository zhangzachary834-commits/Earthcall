// What a law scoped to a Category costs — FORMATION_RETE.md §3.0, §8 rung 4.
//
// §3.0: "Laws do not linear-search for targets; they query Category
// Formations." The authored form of that query is
// `Related(instance-of, category.X)`, and it is the dominant scoping idiom in
// the tree: across every saved world, 132 laws name `category.chess.piece`
// this way — two orders of magnitude more than any other category.
//
// What it costs today. `compileToRete` gives a Related leaf an alpha filtered
// on `attribute == relationType`, i.e. on the STRING "instance-of" alone. The
// far endpoint — which category the edge actually points at — is deliberately
// not read there (rung 0: a relation's far end may already be destroyed). So
// the node admits every being carrying ANY instance-of edge, whatever it is an
// instance OF, and the compiled predicate then re-checks the real question
// against the live graph, once per candidate.
//
// A law scoped to one category therefore pays for every categorised being in
// the world. This measures that: the population grows, the target category
// does not.
//
// WHAT IT FOUND (2026-09-14, Claude Opus 5). At 400 beings the Related arm cost
// ~17x the control, and the endpoint index built for rung 4 changed nothing —
// the cost was never in walking relations. PropheticRete marked every Related
// read opaque, which switched the write filter off for the whole world, so the
// law's own `add position.z` re-asserted a fact through a linear walk of ~49k
// facts every tick. With a typed Related made legible (PropheticRete.cpp;
// behaviour proved by related_prophetic_legibility_test): index 1.0x the
// control, scan 1.2x. The last assert below guards that gap from reopening.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {
double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}
double fittedExponent(const std::vector<double>& ns, const std::vector<double>& cs) {
    double sx=0,sy=0,sxx=0,sxy=0; int m=0;
    for (std::size_t i=0;i<ns.size();++i){ if(cs[i]<=1e-6) continue;
        double x=std::log(ns[i]), y=std::log(cs[i]); sx+=x; sy+=y; sxx+=x*x; sxy+=x*y; ++m; }
    if(m<2) return 0.0; double d=m*sxx-sx*sx; if(std::fabs(d)<1e-12) return 0.0;
    return (m*sxy-sx*sy)/d;
}

// `n` beings, ALL categorised, but only `inTarget` in the category the law names.
double costPerTick(int n, int inTarget, int ticks, bool viaRelation, bool indexed = false) {
    std::vector<std::unique_ptr<Object>> owned;
    std::vector<Singular*> population;
    auto target = std::make_unique<Object>();  target->setObjectID("category.target");
    auto other  = std::make_unique<Object>();  other->setObjectID("category.other");
    Object author;

    RelationManager graph;
    for (int i=0;i<n;++i) {
        auto o = std::make_unique<Object>();
        o->setObjectID("being-" + std::to_string(i));
        PropertyPath::parse("shape.fillet").setValue(*o, PropertyValue(0.25f));
        // The CONTROL's discriminator: a plain property carried by exactly the
        // same beings the target category contains. Same selectivity, same
        // firing set — the only difference is whether membership is asked of
        // the relation graph or of the being itself.
        o->setDynamicProperty("inTarget", PropertyValue(i < inTarget ? 1.0 : 0.0));
        population.push_back(o.get());
        owned.push_back(std::move(o));
    }
    population.push_back(target.get());
    population.push_back(other.get());

    Universe::instance().setProvider([&](std::vector<Singular*>& b){
        for (Singular* s : population) b.push_back(s); });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out){
        for (const auto& r : graph.getAll()) if (r) out.push_back(r.get()); });
    // The INDEXED arm installs the endpoint index exactly as EngineInit does —
    // after setRelationProvider, which clears it. Without this line the Related
    // condition scans every relation, which is the pre-rung-4 engine.
    if (indexed) {
        Universe::instance().setRelationsInvolvingProvider(
            [&](const Singular& being, std::vector<Relation*>& out) {
                graph.relationsInvolving(being, out);
            });
    }
    Universe::instance().setClock(100.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    // Every being is an instance of SOMETHING, so the relation-type filter
    // cannot separate them. Only the far endpoint distinguishes the target
    // category's members, and that is exactly what the index does not hold.
    for (int i=0;i<n;++i) {
        Object* being = static_cast<Object*>(population[i]);
        graph.add(std::make_shared<Relation>(
            "instance-of", *being, (i < inTarget ? *target : *other), true));
    }

    auto law = mgr.createLaw("category-scoped", {&author});
    law->setActivation(Law::Activation::WhileTrue);
    law->setConditionModel(
        viaRelation
            ? ConditionNode::related("instance-of", "category.target")
            : ConditionNode::compare("inTarget", ConditionNode::Op::Gt, PropertyValue(0.5)));
    law->setActionModel(ActionNode::add("position.z", 1.0));

    mgr.tick();
    {   // The law must actually reach its category, or the timing means nothing.
        PropertyValue before, after; double b=0,a=0;
        PropertyPath::parse("position.z").getValue(*population[0], before);
        mgr.tick();
        PropertyPath::parse("position.z").getValue(*population[0], after);
        propertyValueToNumber(before,b); propertyValueToNumber(after,a);
        assert(a > b && "the category-scoped law never fired; timing is meaningless");
    }

    const double t0 = nowMs();
    for (int t=0;t<ticks;++t) mgr.tick();
    const double elapsed = nowMs() - t0;

    Universe::instance().setRelationProvider(nullptr);
    Universe::instance().setProvider(nullptr);
    return elapsed / ticks;
}
} // namespace

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(64,64,"category_membership_scaling_test",nullptr,nullptr);
    if (!w) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(w);

    const int kInTarget = 8;
    const int ticks = 8;
    std::vector<double> ns, viaRel, viaIdx, viaProp;
    std::printf("\nCATEGORY-SCOPED LAW — only %d beings match, however membership is asked\n", kInTarget);
    std::printf("  %8s  %13s  %13s  %13s  %9s  %9s\n", "beings", "Related scan", "Related index",
                "Compare prop", "scan/prop", "index/prop");
    for (int n : {50, 100, 200, 400}) {
        // All three arms back to back, so machine load hits them alike and the
        // RATIOS are the trustworthy numbers — absolute times swing with
        // whatever else is building.
        const double prop = costPerTick(n, kInTarget, ticks, false);
        const double scan = costPerTick(n, kInTarget, ticks, true, false);
        const double idx  = costPerTick(n, kInTarget, ticks, true, true);
        ns.push_back(n); viaRel.push_back(scan); viaIdx.push_back(idx); viaProp.push_back(prop);
        std::printf("  %8d  %13.4f  %13.4f  %13.4f  %8.1fx  %8.1fx\n", n, scan, idx, prop,
                    prop > 1e-9 ? scan / prop : 0.0, prop > 1e-9 ? idx / prop : 0.0);
    }
    const double kRel = fittedExponent(ns, viaRel);
    const double kIdx = fittedExponent(ns, viaIdx);
    const double kProp = fittedExponent(ns, viaProp);
    std::printf("\n  fitted k against POPULATION (matching set held at %d):\n", kInTarget);
    std::printf("    Related, scanning every relation  = %.3f   <- before rung 4\n", kRel);
    std::printf("    Related, endpoint index           = %.3f   <- rung 4\n", kIdx);
    std::printf("    Compare(own property)             = %.3f   <- the control\n", kProp);
    std::printf("\n  Same selectivity, same firing set. The scan arm pays for walking every\n"
                "  relation per candidate; the index arm pays for the subject's own edges.\n"
                "  Both once paid ~17x for an opaque Prophetic read; see the header.\n\n");
    assert(kProp < 1.75 && "the control is not the shape it should be");
    // Rung 4's claim, stated loosely because this machine is shared: an indexed
    // category-scoped law must not cost MORE than scanning every relation.
    assert(viaIdx.back() <= viaRel.back() * 1.25 &&
           "the endpoint index made a category-scoped law slower than scanning");
    // Loose on purpose (shared machine): measured 1.0x. It was 17x while a
    // Related condition made the Prophetic index incomplete.
    assert(viaIdx.back() <= viaProp.back() * 4.0 &&
           "a category-scoped Related law costs far more than the same law reading a "
           "property -- is a Related read making the Prophetic index incomplete again?");
    glfwDestroyWindow(w); glfwTerminate();
    std::printf("category_membership_scaling_test: OK\n");
    return 0;
}
