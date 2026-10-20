// disjunction_candidate_test — an Any must not lose an alternative from its
// Rete candidate set because that alternative could not be indexed.
//
// Suspicion raised 2026-10-20 in the ray-domain intercom thread; Codex asked for
// a bounded reproducer before any repair (2026-10-20 11:10): "An empty terminal
// vector is not proof of impossibility." ConditionNode::compileToRete's Any
// concatenates its children's terminals; an All made only of quantifiers (or an
// empty All) returns none, so if that alternative is the TRUE one, the subject
// is never proposed and the law never fires — deaf (PROPHETIC_RETE §2).
//
// Each case is a real OnBecomeTrue law through a connected LawManager, ticked
// only; its count is compared with full evaluation (conditionsSatisfied):
//   A  Any(All(ForAny(lit > 0)), x > 100): the quantified alternative true
//   B  Any(All(), x > 100): the vacuously true alternative
//   C  Any(ForAny(lit > 0), x > 100): a BARE quantifier alternative
//   D  control: Any(x > 100, y > 100), both alternatives indexable
//   E  a BARE top-level ForAny(lit > 0) targeting a subject that is not the lamp
// Owner: the shared Law/Rete condition compiler —
// docs/Agenda/Tasks/Specific Tasks/Law and Reasoning/Formation_Rete/Formation_Rete.md.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-20.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}
double number(Object& o, const char* path) {
    PropertyValue v;
    if (PropertyPath::parse(path).getValue(o, v) != PropertyPath::PathResult::Ok) return -1;
    double d = -1; propertyValueToNumber(v, d); return d;
}
void setNum(Object& o, const char* name, double v) { o.setDynamicProperty(name, PropertyValue(v)); }
using C = ConditionNode;
} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "disjunction_candidate_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "disjunction_candidate_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    std::cout << "Starting disjunction_candidate_test...\n";

    {
        Object author;
        Object lamp;  lamp.setObjectID("lamp");
        Object qa, qe, qb, qc, qf;
        qa.setObjectID("qa"); qe.setObjectID("qe"); qb.setObjectID("qb"); qc.setObjectID("qc"); qf.setObjectID("qf");
        setNum(lamp, "lit", 0.0);
        for (Object* q : {&qa, &qe, &qb, &qc, &qf}) { setNum(*q, "x", 0.0); setNum(*q, "y", 0.0); setNum(*q, "hits", 0.0); }

        std::vector<Singular*> population{&author, &lamp, &qa, &qe, &qb, &qc, &qf};
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();
        const auto lampLit = [] { return C::forAny(C::BeingKind::Object, C::compare("lit", C::Op::Gt, PropertyValue(0.0))); };
        const auto xBig = [] { return C::compare("x", C::Op::Gt, PropertyValue(100.0)); };
        const auto edgeLaw = [&](const char* name, ConditionNode condition, Object& only) {
            auto law = mgr.createLaw(name, {&author});
            law->setActivation(Law::Activation::OnBecomeTrue);
            law->setConditionModel(std::move(condition));
            law->setActionModel(ActionNode::add("hits", 1.0));
            law->addTarget(only);
            return law;
        };
        auto lawA = edgeLaw("any-all-quantifiers", C::any({C::all({lampLit()}), xBig()}), qa);
        auto lawB = edgeLaw("any-empty-all", C::any({C::all({}), xBig()}), qe);
        auto lawC = edgeLaw("any-bare-quantifier", C::any({lampLit(), xBig()}), qb);
        auto lawE = edgeLaw("bare-quantifier", lampLit(), qf);
        auto lawD = edgeLaw("any-control", C::any({xBig(), C::compare("y", C::Op::Gt, PropertyValue(100.0))}), qc);

        const auto ticks = [&](int n) { for (int i = 0; i < n; ++i) mgr.tick(); };
        const auto report = [&](const char* phase) {
            std::printf("  %-10s A=%.0f B=%.0f C=%.0f D=%.0f E=%.0f  terminals A=%zu B=%zu C=%zu D=%zu E=%zu  full A=%d B=%d C=%d\n",
                        phase, number(qa, "hits"), number(qe, "hits"), number(qb, "hits"), number(qc, "hits"), number(qf, "hits"),
                        mgr.terminalCountOf(lawA->getIdentifier()), mgr.terminalCountOf(lawB->getIdentifier()),
                        mgr.terminalCountOf(lawC->getIdentifier()), mgr.terminalCountOf(lawD->getIdentifier()),
                        mgr.terminalCountOf(lawE->getIdentifier()),
                        lawA->conditionsSatisfied(qa), lawB->conditionsSatisfied(qe), lawC->conditionsSatisfied(qb));
        };

        ticks(2);
        report("start");
        check(number(qe, "hits") == 1.0 && lawB->conditionsSatisfied(qe),
              "B: Any(All(), x > 100) is vacuously true and fires once on the first ticks");
        check(number(qa, "hits") == 0.0 && number(qb, "hits") == 0.0, "A/C: nothing lit, nothing fires");

        setNum(lamp, "lit", 1.0);
        ticks(1);
        report("lit");
        check(lawA->conditionsSatisfied(qa) && number(qa, "hits") == 1.0,
              "A: the lamp lighting makes the quantified alternative true, and the law fires");
        check(lawC->conditionsSatisfied(qb) && number(qb, "hits") == 1.0,
              "C: the same through a bare quantifier alternative");

        setNum(lamp, "lit", 0.0);
        ticks(1);
        setNum(lamp, "lit", 1.0);
        ticks(1);
        report("re-armed");
        check(number(qa, "hits") == 2.0 && number(qb, "hits") == 2.0, "A/C: false then true again fires the second edge");
        check(number(qf, "hits") == 2.0, "E: a bare top-level quantifier on a targeted subject hears both edges");
        check(number(qe, "hits") == 1.0, "B: an always-true condition fires only its one edge");

        setNum(qc, "y", 200.0);
        ticks(1);
        report("control");
        check(number(qc, "hits") == 1.0 && mgr.terminalCountOf(lawD->getIdentifier()) > 0,
              "D: an indexable disjunction keeps its terminals and hears its second alternative");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "disjunction_candidate_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
