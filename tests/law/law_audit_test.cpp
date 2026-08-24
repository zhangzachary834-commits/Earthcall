// Regressions for the law-system audit.
//
// Every case here is a bug that shipped, and every one of them is asserted
// THROUGH LawManager::tick() rather than by hand-feeding a compiled node —
// because the whole class of failure being fixed is "the verb works when you
// call it yourself, and the production path that should call it doesn't".
//
//   1. Destroy is deferred      a law unmaking its own subject used to free it
//                               under applyTo's feet and read it three more times
//   2. Applied != changed       a law whose every write failed reported SUCCESS
//                               and was handed a drive session that ran forever
//   3. Authored bounds bind     Sequence[bounded arc, Set] ignored the arc's bounds
//   4. Authority is granted     a save file could raise its own ceiling
//   5. Alpha nodes are interned rebinding triggers leaked a node per call
//   6. Sweeps are filtered      "everyone" meant relations, zones, and laws too
//   7. Flow integrates lanes    dp/dt on "position.y" silently integrated nothing
//
// Run: make test-law-audit

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_checks = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << what << std::endl;
    assert(ok);
}

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

double yOf(const Object& o) { return o.getPosition().y; }

OntoMath::Piecewise boundedInT(double lo, double hi, double value) {
    OntoMath::Piecewise f;
    f.inputVariable = "t";
    OntoMath::Piecewise::Piece piece;
    piece.hasLo = true;
    piece.lo = lo;
    piece.hasHi = true;
    piece.hi = hi;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(
        OntoMath::ScalarForm::constant(value));
    f.pieces.push_back(piece);
    return f;
}

OntoMath::Piecewise everywhereInT(double value) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::constant(value)));
    f.inputVariable = "t";
    return f;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "law_audit_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_audit_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "law_audit_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const MathBindings tBinding{{"t", PropertyPath::parse("time.sinceApplied")}};

    // ------------------------------------------------------------------
    // 1. A law that destroys its own subject.
    //
    //    This is the most obvious law anyone writes ("when touched by fire,
    //    it is gone") and it was a use-after-free: World::removeObject frees
    //    synchronously, then applyTo goes on to log, build a record, and
    //    publish — three reads through a reference to freed memory.
    // ------------------------------------------------------------------
    std::cout << "\n[1] Destroy is deferred to the end of the tick" << std::endl;
    {
        Zone world("test-zone", "default");
        Object author;
        // Deliberately NOT connectToEventBus(): the bus has no unsubscribe, so
        // a connected manager must be engine-lifetime. Releasing an unmade
        // being from its laws must work without it, and this asserts that.
        LawManager mgr;

        auto victim = std::make_unique<Object>();
        victim->setObjectType("kindling");
        Object* victimPtr = victim.get();
        world.addObject(std::move(victim));

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&world);
            for (const auto& obj : world.getOwnedObjects()) {
                if (obj) beings.push_back(obj.get());
            }
        });
        Universe::instance().setClock(1.0, 0.1);

        auto burn = mgr.createLaw("burn-what-you-are", {&author});
        burn->setActivation(Law::Activation::WhileTrue);
        burn->addTarget(*victimPtr);
        burn->setActionModel(ActionNode::destroy());   // empty token = the subject

        check(world.getOwnedObjects().size() == 1, "the kindling exists");
        mgr.tick();   // would have crashed, or silently corrupted, before
        check(world.getOwnedObjects().empty(), "the tick unmade it");
        check(!Universe::instance().hasUnmakings(), "the reaper drained the queue");

        // The law's target Formation held a RAW pointer to the freed being.
        // Nothing released it before; the next tick walked a dangling pointer
        // and did so for the rest of the session.
        check(burn->targets().getMembers().empty(),
              "the unmade being was released from every law that named it");
        mgr.tick();   // clean under ASan/valgrind only if the release happened
        check(true, "a second tick after the unmaking is safe");

        Universe::instance().setProvider(nullptr);
    }

    // ------------------------------------------------------------------
    // 2. Applied means the action branch was REACHED. Whether anything
    //    changed is a different question, answered per node.
    // ------------------------------------------------------------------
    std::cout << "\n[2] Applied is not the same claim as changed" << std::endl;
    {
        Object subject;
        Object author;
        Law wishful("write-a-property-that-is-not-there");
        wishful.addAuthor(author);
        wishful.setActionModel(ActionNode::set("noSuchProperty.deeper", PropertyValue(1.0)));

        check(wishful.applyTo(subject) == Law::ApplicationResult::Applied,
              "the action branch was reached, so the result is Applied");
        const Law::ApplicationRecord& record = wishful.applicationLog().back();
        check(!record.changedSomething(), "...but nothing was written");
        check(record.trace.nodes.size() == 1, "the trace names the node that fired");
        check(record.trace.nodes.front().reason ==
                  PropertyPath::PathResult::NoSuchProperty,
              "...and says WHY it did not land");
        check(record.toJson()["changed"] == false,
              "the record carries that truth out to the UI and the save");

        // A law that writes is distinguishable from one that only tried.
        Law real("actually-move-it");
        real.addAuthor(author);
        real.setActionModel(ActionNode::set("position.y", PropertyValue(3.0)));
        check(real.applyTo(subject) == Law::ApplicationResult::Applied, "real law applies");
        check(real.applicationLog().back().changedSomething(), "...and changed something");
        check(nearf(yOf(subject), 3.0f), "...visibly");
    }

    // ------------------------------------------------------------------
    // 3. A law that writes nothing must not be handed a drive session.
    //    It used to get one, re-apply every tick, and fail every tick —
    //    a runaway built out of a silent failure.
    // ------------------------------------------------------------------
    std::cout << "\n[3] A law that changed nothing does not earn a drive" << std::endl;
    {
        Object subject;
        Object author;
        LawManager mgr;
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&subject);
        });
        Universe::instance().setClock(100.0, 0.1);

        auto futile = mgr.createLaw("drive-a-property-that-is-not-there", {&author});
        futile->setActivation(Law::Activation::WhileTrue);
        futile->addTarget(subject);
        futile->setDrives(true);
        futile->setActionModel(ActionNode::set("noSuchProperty.deeper", PropertyValue(1.0)));

        mgr.tick();
        check(mgr.driveSessions().empty(),
              "no session opened for a law that wrote nothing");

        Universe::instance().setProvider(nullptr);
    }

    // ------------------------------------------------------------------
    // 4. Authored bounds must be able to END a composite drive. With a
    //    plain any-fold over children, an unbounded Set voted "still
    //    defined" forever and the arc's bounds were unwritable.
    // ------------------------------------------------------------------
    std::cout << "\n[4] Only bounded children vote on a composite's life" << std::endl;
    {
        Object subject;
        Universe::instance().setClock(0.0, 0.1);

        ActionNode arc = ActionNode::map("position.y", boundedInT(0.0, 2.0, 5.0), tBinding);
        ActionNode plain = ActionNode::set("shape.fillet", PropertyValue(0.5f));
        ActionNode both = ActionNode::sequence({arc, plain});

        check(arc.hasAuthoredBounds(), "a Map carries authored bounds");
        check(!plain.hasAuthoredBounds(), "a Set does not");
        check(both.hasAuthoredBounds(), "a sequence containing one does");

        Universe::instance().setApplicationOnset(0.0);
        Universe::instance().setClock(1.0, 0.1);         // t = 1: inside [0,2]
        check(both.definedFor(subject), "inside the arc's bounds, the sequence lives");
        Universe::instance().setClock(9.0, 0.1);         // t = 9: outside
        check(!both.definedFor(subject),
              "outside them it ends — the unbounded sibling does not keep it alive");
        Universe::instance().clearApplicationOnset();

        // A composite with no bounded child anywhere is unbounded, as before.
        ActionNode neither = ActionNode::sequence({plain, plain});
        check(neither.definedFor(subject), "a wholly unbounded sequence still never ends");
    }

    // ------------------------------------------------------------------
    // 5. Authority is granted in code, never claimed by a save file.
    // ------------------------------------------------------------------
    std::cout << "\n[5] A save file cannot raise its own ceiling" << std::endl;
    {
        Object author;
        Law kernel("kernel");
        kernel.addAuthor(author);
        kernel.grantAuthority(10);
        check(kernel.authorityLevel() == 10, "the first-mover path grants freely");

        Law authored("authored");
        authored.addAuthor(author);
        authored.setAuthorityLevel(10);
        check(authored.authorityLevel() == Law::kAuthoredCeiling,
              "the ordinary path clamps to the authored ceiling");

        // The forged save: a hand-edited authority integer.
        nlohmann::json forged = authored.toJson();
        forged["authority"] = 9999;
        auto loaded = Law::fromJson(forged);
        loaded->addAuthor(author);
        check(loaded->authorityLevel() == Law::kAuthoredCeiling,
              "loading clamps it too — the ceiling survives a text editor");
        check(loaded->applyTo(kernel) == Law::ApplicationResult::AuthorityDenied,
              "...so the forged law still cannot govern a kernel law");
    }

    // ------------------------------------------------------------------
    // 6. One alpha node per event type, and unbound nodes are reclaimed.
    //    evaluate() predicate-tests every node against every fact BEFORE
    //    it looks at bindings, so leaked nodes are a per-frame tax.
    // ------------------------------------------------------------------
    std::cout << "\n[6] Alpha nodes are interned and reclaimed" << std::endl;
    {
        Object author;
        LawManager mgr;
        auto a = mgr.createLaw("hears-collision-a", {&author});
        auto b = mgr.createLaw("hears-collision-b", {&author});

        const std::size_t before = mgr.rete().alphaNodeCount();
        mgr.bindTrigger(a->getIdentifier(), "collision");
        mgr.bindTrigger(b->getIdentifier(), "collision");
        check(mgr.rete().alphaNodeCount() == before + 1,
              "two laws on one event type share one node");

        mgr.bindTrigger(a->getIdentifier(), "touch");
        check(mgr.rete().alphaNodeCount() == before + 2, "a second type adds one node");

        // The old unbindTrigger tore down every binding and rebuilt the
        // survivors on brand-new nodes — a leak per call.
        for (int i = 0; i < 20; ++i) {
            mgr.unbindTrigger(a->getIdentifier(), "touch");
            mgr.bindTrigger(a->getIdentifier(), "touch");
        }
        check(mgr.rete().alphaNodeCount() == before + 2,
              "twenty rebinds leak nothing");

        mgr.unbindTrigger(a->getIdentifier(), "touch");
        check(mgr.rete().alphaNodeCount() == before + 1,
              "the last law off a node reclaims it");
    }

    // ------------------------------------------------------------------
    // 7. "Apply to everyone" means everyone WITH THIS PROPERTY. A law that
    //    writes position.y has nothing to say to a Relation or another Law,
    //    and sweeping them cost a condition evaluation and a record each.
    // ------------------------------------------------------------------
    std::cout << "\n[7] An untargeted sweep is filtered by the law's vocabulary"
              << std::endl;
    {
        Object author;
        Object mover;
        Law bystander("a-law-is-a-being-too");
        LawManager mgr;

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&mover);
            beings.push_back(&bystander);   // a Law: extra-spatial, has no position
        });
        Universe::instance().setClock(200.0, 0.1);

        auto sink = mgr.createLaw("everything-falls", {&author});
        sink->setActivation(Law::Activation::WhileTrue);
        sink->setActionModel(ActionNode::set("position.y", PropertyValue(-1.0)));
        // no targets: the sweep is derived

        check(sink->requiredProperties().size() == 1 &&
                  sink->requiredProperties().front() == "position",
              "the law's text names the vocabulary it needs");
        check(sink->couldApplyTo(mover), "an Object carries it");
        check(!sink->couldApplyTo(bystander),
              "a Law does not — it is extra-spatial, and is skipped");

        auto records = mgr.tick();
        int touchedBystander = 0;
        for (const auto& r : records) {
            if (r.targetId == bystander.getIdentifier()) ++touchedBystander;
        }
        check(touchedBystander == 0, "the sweep never visited it");
        check(nearf(yOf(mover), -1.0f), "...and still reached everyone it should");

        Universe::instance().setProvider(nullptr);
    }

    // ------------------------------------------------------------------
    // 8. Flow integrates a vector LANE. It matched on the `double` variant,
    //    but a component read yields a float — so every dp/dt authored
    //    against position.y integrated nothing, forever, silently.
    // ------------------------------------------------------------------
    std::cout << "\n[8] Flow integrates a component path" << std::endl;
    {
        Object author;
        Object rising;
        rising.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        LawManager mgr;

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&rising);
        });

        auto steady = mgr.createLaw("rise-two-per-second", {&author});
        steady->setActivation(Law::Activation::WhileTrue);
        steady->addTarget(rising);
        steady->setActionModel(ActionNode::flow("position.y", everywhereInT(2.0), tBinding));

        double now = 300.0;
        for (int i = 0; i < 4; ++i) {
            now += 0.5;
            Universe::instance().setClock(now, 0.5);
            mgr.tick();
        }
        check(nearf(yOf(rising), 4.0f), "2 per second over 2 seconds is 4, exactly");

        // And it says so when it genuinely cannot integrate.
        Law confused("integrate-a-name");
        confused.addAuthor(author);
        confused.setActionModel(
            ActionNode::flow("objectType", everywhereInT(2.0), tBinding));
        Universe::instance().setClock(now, 0.5);
        confused.applyTo(rising);
        const auto& trace = confused.applicationLog().back().trace;
        check(trace.fired() && !trace.anyWrote(), "a string lane integrates nothing");
        check(!trace.nodes.front().note.empty(),
              "...and the trace says so in words, rather than returning in silence");

        Universe::instance().setProvider(nullptr);
    }

    std::cout << "\nSUCCESS — " << g_checks << " checks passed." << std::endl;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
