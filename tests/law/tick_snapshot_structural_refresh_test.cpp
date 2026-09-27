// Regression for LawManager's per-tick Universe snapshot optimization.
//
// A structural addition made by an earlier Law in the SAME tick was visible to
// later sweeps before the optimization, because sweepSubjects() asked
// Universe::beings() at each use. The optimized path may reuse a snapshot only
// while Universe::structuralRevision() is unchanged.
//
// This witness deliberately keeps the newborn allocated from the start and
// registers it into the provider during the first Law's action. That isolates
// the LawManager contract from rendering/Object creation plumbing while
// matching real Create/Spawn behavior: Zone::addObject() bumps the same
// structural revision synchronously before later Laws run.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool near(double a, double b) {
    return std::fabs(a - b) < 1e-4;
}
} // namespace

int main() {
    Object author;
    author.setObjectID("tick-snapshot-author");

    Object seed;
    seed.setObjectID("tick-snapshot-seed");

    Object newcomer;
    newcomer.setObjectID("tick-snapshot-newcomer");
    newcomer.setDynamicProperty("newbornGate", PropertyValue(1.0));
    newcomer.setPosition(glm::vec3(0.0f));

    std::vector<Singular*> population{&author, &seed};
    bool registered = false;
    std::size_t providerCalls = 0;

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        ++providerCalls;
        out.insert(out.end(), population.begin(), population.end());
    });

    {
        LawManager manager;

        // First in manager order: simulate the synchronous structural part of
        // Create/Spawn. The target Formation keeps this law scoped to seed.
        auto registrar = manager.createLaw("register newborn", {&author});
        registrar->setLawIdentifier("tick-snapshot-register-newborn");
        registrar->setActivation(Law::Activation::WhileTrue);
        registrar->addTarget(seed);
        registrar->addCondition(
            "always",
            [](const ECA::Event&, const Singular&) { return true; });
        registrar->addAction(
            "register newborn into Universe provider",
            [&](const ECA::Event&, Singular&) {
                if (registered) return;
                population.push_back(&newcomer);
                registered = true;
                // Real Zone::addObject() does this before returning from a
                // Create/Spawn action.
                Universe::instance().bumpStructuralRevision();
            });

        // Second in manager order: must see the just-registered being during
        // this SAME tick, as the pre-optimization fresh beings() sweep did.
        auto observer = manager.createLaw("observe newborn same tick", {&author});
        observer->setLawIdentifier("tick-snapshot-observe-newborn");
        observer->setActivation(Law::Activation::WhileTrue);
        observer->setScope(Law::Scope::Everyone);
        observer->setConditionModel(ConditionNode::compare(
            "newbornGate", ConditionNode::Op::Gt, PropertyValue(0.5)));
        observer->setActionModel(ActionNode::add("position.z", 1.0));

        const auto beforeRevision = Universe::instance().structuralRevision();
        providerCalls = 0;
        manager.tick();

        assert(registered && "first Law did not register the newcomer");
        assert(Universe::instance().structuralRevision() > beforeRevision &&
               "structural addition did not move the Universe generation");
        assert(near(newcomer.getPosition().z, 1.0) &&
               "later Law did not see a being registered earlier in the same tick");
        assert(providerCalls == 2 &&
               "structural-change tick should query once initially and once after revision");

        // With no structural change, every later consumer must reuse the same
        // population snapshot. This is the deterministic performance oracle:
        // no wall-clock threshold, no machine dependence, just one provider
        // construction for the whole tick.
        providerCalls = 0;
        manager.tick();
        assert(providerCalls == 1 &&
               "quiescent tick rebuilt Universe::beings more than once");
        assert(near(newcomer.getPosition().z, 2.0) &&
               "quiescent snapshot reuse changed lawful application semantics");
    }

    Universe::instance().setProvider(nullptr);
    std::printf("tick_snapshot_structural_refresh_test: OK\n");
    return 0;
}
