// The endpoint register must never say "no relation holds this being" when one does.
//
// FORMATION_RETE.md §8 rung 5, 2026-09-15 (Claude Opus 5). Measuring what an
// instance-side adapter would speed up, the chess world's Scope::Everyone event
// sweeps turned out to spend 45 of every 49 µs per candidate destroying the
// transient ECA::Event that Law::conditionsSatisfied builds — and 39.5 µs of that
// in RelationManager::forgetBeingEverywhere, which walked every relation in every
// live manager on EVERY Singular destructor. Relation::mayBeEndpoint now lets it
// return in O(1) for a being no relation holds.
//
// That early return is a NARROWING: if the register ever forgets a pointer that a
// relation still holds, the being dies without its relations letting go, and the
// next aId()/bId() dereferences freed memory. So this test is an oracle against
// the thing the register summarises: after every operation, every pointer held by
// any relation must be reported as a possible endpoint.
//
// FOR FUTURE AGENTS (Jules especially): every write to Relation::Endpoint::ptr
// must go through bind()/forget() or Endpoint's special members. If you add one
// that does not, add it as a step here. The failure is a use-after-free, not a
// wrong answer — run this under ASan if you touch it.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Time/Moment/Moment.hpp"

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what); }
    else std::printf("  ok: %s\n", what);
}

class Being : public Singular {
public:
    std::string id;
    explicit Being(std::string identifier) : id(std::move(identifier)) {}
    std::string getIdentifier() const override { return id; }
protected:
    void buildProperties() override {}
};

// The oracle: everything any relation in these managers holds is a possible endpoint.
bool registerCovers(const std::vector<const RelationManager*>& managers) {
    for (const RelationManager* m : managers) {
        for (const auto& r : m->getAll()) {
            if (!r) continue;
            if (r->a() && !Relation::mayBeEndpoint(r->a())) return false;
            if (r->b() && !Relation::mayBeEndpoint(r->b())) return false;
        }
    }
    return true;
}

} // namespace

int main() {
    std::printf("--- endpoint register ---\n");
    Being a("a"), b("b"), c("c");
    check(!Relation::mayBeEndpoint(&a), "a being no relation holds is not an endpoint");

    {
        RelationManager g;
        auto ab = std::make_shared<Relation>("near", a, b, false);
        g.add(ab);
        check(Relation::mayBeEndpoint(&a) && Relation::mayBeEndpoint(&b), "construction registers both ends");
        check(!Relation::mayBeEndpoint(&c), "an unrelated being is still not an endpoint");

        ab->bind(&a, &c);
        check(Relation::mayBeEndpoint(&c), "re-binding registers the new end");
        check(!Relation::mayBeEndpoint(&b), "re-binding releases the old end");
        check(registerCovers({&g}), "oracle after re-bind");

        Relation copied = *ab;   // a copy holds the same pointers
        ab->forgetEndpoint(&a);
        check(Relation::mayBeEndpoint(&a), "a copy keeps the pointer registered after the original forgets it");
        check(registerCovers({&g}), "oracle after forget with a live copy");

        {
            RelationManager duplicate(g);
            check(registerCovers({&g, &duplicate}), "oracle across a copied manager");
        }

        Relation assigned("x", b, b, false);
        assigned = copied;
        check(Relation::mayBeEndpoint(&a) && Relation::mayBeEndpoint(&c), "assignment registers the assigned ends");
        check(registerCovers({&g}), "oracle after assignment");
    }
    check(!Relation::mayBeEndpoint(&a) && !Relation::mayBeEndpoint(&b) && !Relation::mayBeEndpoint(&c),
          "destroying every relation releases every end");

    // The behaviour the early return must not break: a held being that dies is
    // forgotten by every relation, keeping its name.
    {
        RelationManager g;
        auto dying = std::make_unique<Being>("dying");
        auto edge = std::make_shared<Relation>("near", *dying, a, false);
        g.add(edge);
        RelationManager::forgetBeingEverywhere(dying.get());
        check(edge->a() == nullptr && edge->aId() == "dying", "a held being is still forgotten, name kept");
        check(!Relation::mayBeEndpoint(dying.get()), "and is released from the register");
        dying.reset();
        check(edge->b() == &a, "the other end is untouched");
    }

    // The measurement this exists for: a transient Moment's lifetime must not
    // scale with the size of the relation graph.
    {
        std::vector<std::unique_ptr<Being>> beings;
        RelationManager big;
        // What LawManager::connectToEventBus installs, reduced to the part measured.
        Singular::setBeingReleasedCallback([](const Singular* being) {
            RelationManager::forgetBeingEverywhere(being);
        });
        for (int i = 0; i < 6000; ++i) {
            beings.push_back(std::make_unique<Being>("b" + std::to_string(i)));
            if (i > 0) big.add(std::make_shared<Relation>("next", *beings[i - 1], *beings[i], true));
        }
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 2000; ++i) { Moment transient; (void)transient; }
        const double usEach =
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 2000;
        std::printf("  transient Moment lifetime with 6000 relations live: %.2f us\n", usEach);
        // Before: a walk of every relation per destructor. Loose, shared machine.
        check(usEach < 20.0, "a transient Moment does not walk the relation graph");
        Singular::setBeingReleasedCallback(nullptr);
    }

    std::printf("%s\n", g_failures ? "endpoint_register_test: FAILURES" : "endpoint_register_test: OK");
    return g_failures ? 1 : 0;
}
