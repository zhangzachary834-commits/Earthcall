// Rete compilation test — the condition tree as a network, not as a closure.
//
// `continuous_law_test` proves the SWEEP path: every continuous law there
// either has no condition model at registration time or carries an explicit
// targets Formation, so `compileToRete` never decided anything. This test
// exercises the compiled network itself, which is where the interesting
// failures live:
//
//   1. alpha and beta node ids shared one numeric namespace, so an All() join
//      was misread as its own first alpha and the law matched on one clause;
//   2. `property-state` facts existed only for law-spawned beings, so a law
//      restored from a save file found an empty match set and — taking the
//      fast path — skipped the sweep as well, firing never again;
//   4. All() kept only its first child terminal, so All(Any(b,c), d) silently
//      became b && d;
//   5. editing a condition never recompiled the network;
//   6. OnBecomeTrue laws were bound into the agenda and re-fired every tick
//      while their condition held — a level wearing an edge's clothes;
//   7. the fast path skipped `targets`, onset bookkeeping and
//      conditionsSatisfied entirely;
//   8. a multi-level beta join dropped matches depending on the ORDER its
//      facts arrived in.
//
// Sections B and D deliberately keep their subject OUT of the Universe
// provider: the sweep cannot reach it, so the ONLY way the law can apply is
// through the compiled network. That is what makes those assertions about the
// network rather than about the fallback.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

double readNumber(Object& obj, const char* dotted) {
    PropertyValue v;
    if (PropertyPath::parse(dotted).getValue(obj, v) != PropertyPath::PathResult::Ok) {
        return -12345.0;
    }
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

void writeFloat(Object& obj, const char* dotted, float value) {
    assert(PropertyPath::parse(dotted).setValue(obj, PropertyValue(value)) ==
           PropertyPath::PathResult::Ok);
}

// Hand-assert one `property-state` fact, the way the world seeding pass does.
// Used to control the ORDER facts reach the network (section D) and to make a
// being visible to the network but not to the sweep (sections B and D).
void assertStateFact(LawManager& mgr, Object& subject, const std::string& attribute) {
    Property* prop = subject.findProperty(attribute);
    assert(prop != nullptr);
    auto fact = std::make_shared<ReteFact>();
    fact->type = "property-state";
    fact->subject = &subject;
    fact->subjectId = subject.getIdentifier();
    fact->attribute = attribute;
    fact->value = propertyValueToJson(prop->value());
    fact->isState = true;
    fact->dirty = false;
    mgr.rete().assertFact(fact);
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "rete_compile_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "rete_compile_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "rete_compile_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object both;        // satisfies BOTH clauses
        Object onlyFirst;   // satisfies only the first
        Object edgeSubject; // section C
        Object disj;        // section B — deliberately outside the Universe
        Object ordered;     // section D — deliberately outside the Universe

        author.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        both.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        onlyFirst.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        edgeSubject.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
        disj.setPosition(glm::vec3(-20.0f, 0.0f, 0.0f));
        ordered.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

        std::vector<Singular*> population{&author, &both, &onlyFirst, &edgeSubject};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        // ------------------------------------------------------------------
        // A. A saved WhileTrue law with a two-clause All(): it must fire for
        //    the being that satisfies BOTH clauses, and stay silent for the
        //    one that satisfies only the first.
        //
        //    Loading is the point. `add()` compiles the condition, so the law
        //    arrives with a live network and takes the fast path — which is
        //    exactly the path that used to bind the join to its own first
        //    alpha (bug 1), find an unseeded network (bug 2), and skip the
        //    verification the sweep does (bug 7).
        // ------------------------------------------------------------------
        std::string bothClausesId, edgeLawId;
        {
            Law bothClauses("both-clauses");
            bothClauses.setActivation(Law::Activation::WhileTrue);
            bothClauses.setConditionModel(ConditionNode::all({
                ConditionNode::compare("position.x", ConditionNode::Op::Lt,
                                       PropertyValue(1.0)),
                ConditionNode::compare("position.y", ConditionNode::Op::Lt,
                                       PropertyValue(1.0))}));
            bothClauses.setActionModel(ActionNode::set("position.z", PropertyValue(7.0)));
            bothClausesId = bothClauses.getIdentifier();

            Law edgeLaw("edge-once");
            edgeLaw.setActivation(Law::Activation::OnBecomeTrue);
            edgeLaw.setConditionModel(ConditionNode::compare(
                "position.y", ConditionNode::Op::Lt, PropertyValue(1.0)));
            edgeLaw.setActionModel(ActionNode::add("position.z", 0.5));
            edgeLaw.setEnabled(false);          // armed in section C
            edgeLaw.addTarget(edgeSubject);     // reattaches by identifier
            edgeLawId = edgeLaw.getIdentifier();

            nlohmann::json saved;
            saved["laws"] = nlohmann::json::array();
            saved["laws"].push_back(bothClauses.toJson());
            saved["laws"].push_back(edgeLaw.toJson());
            saved["triggers"] = nlohmann::json::object();
            // Authorship is a covenant reattached BY IDENTIFIER on load.
            saved["laws"][0]["authors"] = nlohmann::json::array({author.getIdentifier()});
            saved["laws"][1]["authors"] = nlohmann::json::array({author.getIdentifier()});
            mgr.loadFromJson(saved);
        }

        Law* bothClauses = mgr.find(bothClausesId);
        Law* edgeLaw = mgr.find(edgeLawId);
        assert(bothClauses != nullptr);
        assert(edgeLaw != nullptr);
        assert(bothClauses->isAuthored());       // else it could never fire at all
        assert(edgeLaw->targets().getMembers().size() == 1);

        mgr.tick();

        // The being that satisfies both clauses was reached...
        assert(nearf(static_cast<float>(readNumber(both, "position.z")), 7.0f));
        // ...and the one that satisfies only the first was NOT.
        assert(!nearf(static_cast<float>(readNumber(onlyFirst, "position.z")), 7.0f));
        // A law scoped by conjunction must not reach a being that fails a clause
        // no matter how the network narrowed the candidate set.
        assert(nearf(static_cast<float>(readNumber(onlyFirst, "position.z")), 0.0f));

        bothClauses->setEnabled(false);

        // ------------------------------------------------------------------
        // B. All(Any(b, c), d) must match a subject satisfying c && d.
        //
        //    `disj` is not in the Universe, so the sweep cannot reach it: if
        //    the compiled network does not hold the c-branch, nothing fires.
        //    The law is authored in-session and its condition set AFTER
        //    registration, so this also proves that editing a condition
        //    recompiles the network (bug 5).
        // ------------------------------------------------------------------
        assertStateFact(mgr, disj, "position");

        auto disjunctive = mgr.createLaw("either-far-side", {&author});
        disjunctive->setActivation(Law::Activation::WhileTrue);
        disjunctive->setConditionModel(ConditionNode::all({
            ConditionNode::any({
                ConditionNode::compare("position.x", ConditionNode::Op::Gt,
                                       PropertyValue(10.0)),
                ConditionNode::compare("position.x", ConditionNode::Op::Lt,
                                       PropertyValue(-10.0))}),
            ConditionNode::compare("position.y", ConditionNode::Op::Lt,
                                   PropertyValue(1.0))}));
        disjunctive->setActionModel(ActionNode::set("position.z", PropertyValue(9.0)));

        mgr.tick();
        assert(nearf(static_cast<float>(readNumber(disj, "position.z")), 9.0f));

        disjunctive->setEnabled(false);

        // ------------------------------------------------------------------
        // C. OnBecomeTrue fires ONCE while its condition stays true. Edges,
        //    not levels: the agenda must not re-fire a continuous law every
        //    tick just because its terminal keeps matching.
        // ------------------------------------------------------------------
        writeFloat(edgeSubject, "position.z", 0.0f);
        edgeLaw->setEnabled(true);
        // The being announces itself, exactly as a law-spawned one does — the
        // announcement is what used to queue the law onto the agenda.
        Core::EventBus::instance().publish(
            ECA::Event{"object-created", &edgeSubject, nullptr, std::time(nullptr)});

        mgr.tick();
        assert(nearf(static_cast<float>(readNumber(edgeSubject, "position.z")), 0.5f));
        mgr.tick();
        mgr.tick();
        // Still true, three ticks later: an edge fires once, not once per tick.
        assert(nearf(static_cast<float>(readNumber(edgeSubject, "position.z")), 0.5f));

        // Release and re-hold: the edge RE-ARMS.
        edgeSubject.setPosition(glm::vec3(5.0f, 4.0f, 0.0f));
        mgr.tick();
        edgeSubject.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
        mgr.tick();
        assert(nearf(static_cast<float>(readNumber(edgeSubject, "position.z")), 1.0f));

        edgeLaw->setEnabled(false);

        // ------------------------------------------------------------------
        // D. A three-clause All() must match whatever ORDER its facts arrive
        //    in. The law is compiled first (one tick with no facts), then the
        //    three facts are asserted deepest-clause-first — the order that
        //    used to leave the second join's memory empty.
        // ------------------------------------------------------------------
        writeFloat(ordered, "shape.r", -1.0f);
        writeFloat(ordered, "shape.ry", -1.0f);
        writeFloat(ordered, "shape.rz", -1.0f);
        writeFloat(ordered, "position.z", 0.0f);

        auto threeClause = mgr.createLaw("three-clause", {&author});
        threeClause->setActivation(Law::Activation::WhileTrue);
        threeClause->setConditionModel(ConditionNode::all({
            ConditionNode::compare("shape.r", ConditionNode::Op::Lt, PropertyValue(-0.5)),
            ConditionNode::compare("shape.ry", ConditionNode::Op::Lt, PropertyValue(-0.5)),
            ConditionNode::compare("shape.rz", ConditionNode::Op::Lt, PropertyValue(-0.5))}));
        threeClause->setActionModel(ActionNode::set("position.z", PropertyValue(3.0)));

        mgr.tick();   // compiles the network; `ordered` is still unknown to it
        assert(nearf(static_cast<float>(readNumber(ordered, "position.z")), 0.0f));

        // LAST clause first: the join order is the reverse of the compile order.
        assertStateFact(mgr, ordered, "shape.rz");
        assertStateFact(mgr, ordered, "shape.ry");
        assertStateFact(mgr, ordered, "shape.r");

        mgr.tick();
        assert(nearf(static_cast<float>(readNumber(ordered, "position.z")), 3.0f));

        threeClause->setEnabled(false);

        // ------------------------------------------------------------------
        // E. An empty All() is vacuously true — it must never compile to a
        //    dangling terminal that matches nobody.
        // ------------------------------------------------------------------
        auto vacuous = mgr.createLaw("vacuously-true", {&author});
        vacuous->setActivation(Law::Activation::WhileTrue);
        vacuous->setConditionModel(ConditionNode::all({}));
        vacuous->setActionModel(ActionNode::set("position.z", PropertyValue(-4.0)));
        vacuous->addTarget(both);

        mgr.tick();
        assert(nearf(static_cast<float>(readNumber(both, "position.z")), -4.0f));
        vacuous->setEnabled(false);

        Universe::instance().setProvider({});   // leave no dangling refs
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("rete_compile_test: ALL OK");
    return 0;
}
