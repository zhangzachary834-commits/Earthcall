// derived_reading_law_test — a Law that reads an expression's evaluation report
// must hear it change, through the ordinary LawManager tick.
//
// The report `<prefix>.evaluation` (addendum B) is computed on read from the
// bearer's expression, its bindings record, and every bound input. No change
// fact announces it: when `rate` moves, nothing dirties the subject's `growth.*`
// facts, so a Rete alpha over `growth.evaluation.value` would never admit the
// subject — the law registered, enabled, compiled, and deaf (PROPHETIC_RETE §2).
//
// Codex's admission rule (2026-10-19 23:58, intercom thread
// OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1): until a reading declares its
// dependencies, the consumer must be told to take its general path, and must not
// claim the indexed one. LawManager now declines Rete terminals for any
// condition reading through a derived reading; the continuous sweep decides it
// every tick. His witnesses, all through mgr.tick() with no report read and no
// forced refresh in between:
//   A  the declared fallback: such a law compiles no terminals; a control does
//   B  a bound input crossing the threshold wakes it (OnBecomeTrue counts edges)
//   C  success → refusal → recovery (a removed, then restored, binding)
//   D  replacing the expression
//   E  remapping a binding to another bearer, whose own edit then wakes it
//   F  a status law hears the refusal itself
//   G  a reading registered after the law compiled makes it re-decide
// Selective dirtying (an unrelated edit costing nothing) is a separate,
// later optimization claim and is NOT made here.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-20.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/OntoMath/ExpressionEvaluation.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}

using OntoMath::MathNode;
using OntoMath::ScalarForm;
using Op = MathNode::Op;
using Node = std::unique_ptr<MathNode>;

Node leaf(ScalarForm f) { auto n = std::make_unique<MathNode>(); n->op = Op::ScalarLeaf; n->scalarForm = std::move(f); return n; }
Node constant(double c) { return leaf(ScalarForm::constant(c)); }
Node named(const std::string& v) { auto n = std::make_unique<MathNode>(); n->op = Op::ValueLeaf; n->variableName = v; return n; }
// ∫_0^t c·k·s ds = c·k·t²/2
PropertyValue growth(double c) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::Integral;
    n->variableName = "s";
    n->children.push_back(leaf(ScalarForm::variable("k").times(ScalarForm::variable("s")).scaled(c)));
    n->children.push_back(constant(0.0));
    n->children.push_back(named("t"));
    auto f = std::make_shared<OntoMath::ScalarField>();
    f->mode = OntoMath::ScalarField::EvaluationMode::AST;
    f->astDefinition = OntoMath::Piecewise::continuous(std::move(n));
    return PropertyValue(f);
}
PropertyValue bindingsOf(std::vector<std::pair<std::string, std::string>> pairs) {
    auto d = std::make_shared<PropertyDict>();
    for (auto& [v, p] : pairs) d->elements[v] = p;
    return PropertyValue(d);
}
double number(Object& o, const char* path) {
    PropertyValue v;
    if (PropertyPath::parse(path).getValue(o, v) != PropertyPath::PathResult::Ok) return -1;
    double d = -1; propertyValueToNumber(v, d); return d;
}
void setNum(Object& o, const char* name, double v) { o.setDynamicProperty(name, PropertyValue(v)); }
} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "derived_reading_law_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "derived_reading_law_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    std::cout << "Starting derived_reading_law_test...\n";

    {
        Object author;
        Object bearer;   bearer.setObjectID("growth-bearer");
        Object other;    other.setObjectID("other-bearer");
        Object control;  control.setObjectID("control");

        // growth = ∫_0^horizon rate·s ds; rate 3, horizon 2 → 6, under the threshold 8.
        bearer.setDynamicProperty("growth.expression", growth(1.0));
        bearer.setDynamicProperty("growth.bindings", bindingsOf({{"k", "rate"}, {"t", "horizon"}}));
        setNum(bearer, "rate", 3.0);
        setNum(bearer, "horizon", 2.0);
        setNum(bearer, "hits", 0.0);
        setNum(bearer, "refusals", 0.0);
        setNum(other, "rate", 1.0);
        setNum(control, "x", 0.0);
        setNum(control, "hits", 0.0);

        std::vector<Singular*> population{&author, &bearer, &other, &control};
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        const auto edgeLaw = [&](const char* name, ConditionNode condition, const char* counter, Object& only) {
            auto law = mgr.createLaw(name, {&author});
            law->setActivation(Law::Activation::OnBecomeTrue);
            law->setConditionModel(std::move(condition));
            law->setActionModel(ActionNode::add(counter, 1.0));
            law->addTarget(only);
            return law;
        };

        // G is set up first: the law compiles BEFORE "evaluation" is registered,
        // when nothing marks its path as derived and it is indexed like any other.
        auto valueLaw = edgeLaw("growth-crosses-8",
            ConditionNode::compare("growth.evaluation.value", ConditionNode::Op::Gt, PropertyValue(8.0)), "hits", bearer);
        auto controlLaw = edgeLaw("control-crosses-0",
            ConditionNode::compare("x", ConditionNode::Op::Gt, PropertyValue(0.0)), "hits", control);
        mgr.tick();
        check(mgr.terminalCountOf(valueLaw->getIdentifier()) > 0,
              "G (before): with no reading registered, the law is indexed like any other");

        OntoMath::registerEvaluationReading();
        auto statusLaw = edgeLaw("growth-refused",
            ConditionNode::compare("growth.evaluation.status", ConditionNode::Op::Eq, PropertyValue(std::string("refused"))),
            "refusals", bearer);
        const auto ticks = [&](int n) { for (int i = 0; i < n; ++i) mgr.tick(); };
        ticks(2);

        // ---- A: the declared fallback -----------------------------------------
        check(mgr.terminalCountOf(valueLaw->getIdentifier()) == 0 && mgr.terminalCountOf(statusLaw->getIdentifier()) == 0,
              "A/G: once the reading exists, both report-reading laws re-decide and take the sweep (0 terminals)");
        check(mgr.terminalCountOf(controlLaw->getIdentifier()) > 0, "A: the control law on a stored path keeps its index");
        check(number(bearer, "hits") == 0.0 && number(bearer, "refusals") == 0.0, "at 6 nothing fires");

        // ---- B: a bound input crosses the threshold -----------------------------
        setNum(bearer, "rate", 5.0);   // 5·2²/2 = 10
        ticks(2);
        check(number(bearer, "hits") == 1.0, "B: rate 3 → 5 (value 6 → 10) wakes the law once, through tick alone");
        setNum(bearer, "rate", 3.0);   // back to 6: released
        ticks(1);
        setNum(bearer, "horizon", 3.0); // 3·9/2 = 13.5: the OTHER input re-arms it
        ticks(1);
        check(number(bearer, "hits") == 2.0, "B: falling to 6 releases it; horizon 2 → 3 (13.5) fires the second edge");
        setNum(control, "x", 1.0);
        ticks(1);
        check(number(control, "hits") == 1.0, "control: the indexed law still hears its own input");

        // ---- C and F: success → refusal → recovery ------------------------------
        bearer.setDynamicProperty("growth.bindings", bindingsOf({{"k", "rate"}}));   // t unbound
        ticks(1);
        check(number(bearer, "refusals") == 1.0, "F: removing the t binding refuses, and the status law hears it");
        bearer.setDynamicProperty("growth.bindings", bindingsOf({{"k", "rate"}, {"t", "horizon"}}));
        ticks(1);
        check(number(bearer, "hits") == 3.0, "C: restoring the binding recovers 13.5; the value law fires its third edge");

        // ---- D: replacing the expression ----------------------------------------
        bearer.setDynamicProperty("growth.expression", growth(0.1));   // 0.1·3·9/2 = 1.35
        ticks(1);
        bearer.setDynamicProperty("growth.expression", growth(2.0));   // 27
        ticks(1);
        check(number(bearer, "hits") == 4.0, "D: a replaced expression (13.5 → 1.35 → 27) is heard both ways");

        // ---- E: remapping a binding to another bearer ---------------------------
        bearer.setDynamicProperty("growth.bindings", bindingsOf({{"k", "@other-bearer.rate"}, {"t", "horizon"}}));
        ticks(1);   // 2·1·9/2 = 9 > 8: still holding, no new edge
        setNum(other, "rate", 0.5);   // 4.5: released
        ticks(1);
        setNum(bearer, "rate", 100.0);   // no longer bound: must not matter
        ticks(1);
        check(number(bearer, "hits") == 4.0, "E: after the remap, the old source is irrelevant");
        setNum(other, "rate", 2.0);   // 18: the NEW source re-arms it
        ticks(1);
        check(number(bearer, "hits") == 5.0, "E: an edit on the other bearer wakes this one's law");
        check(number(bearer, "refusals") == 1.0, "F: no spurious refusal edges along the way");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "derived_reading_law_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
