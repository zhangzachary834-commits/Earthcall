// Prophetic Rete (B-Time Rete) — the ahead-of-time abstract interpreter.
//
// Zach's realization (`docs/architecture/law/B-time Rete.md`): in Earthcall
// nothing changes by itself, so the engine can work out where a change could
// possibly matter BEFORE the change happens. This test guards the foundation
// layer of that: the interval algebra it reasons with, the write/read
// extraction it reasons over, and — most importantly — the direction it is
// allowed to be wrong in.
//
// SECTION F IS THE ONE THAT MATTERS. Every other section checks that the
// analysis is clever. F checks that it is safe: a filter derived from this
// index must never make a law go deaf. An over-generous answer costs
// performance; an over-narrow one costs a law, silently, and no test that
// does not exercise that exact law would notice. So F fires real laws through
// a real LawManager and asserts they still hear.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

using OntoMath::Interval;
using OntoMath::ScalarForm;

namespace {

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

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
    auto res = PropertyPath::parse(dotted).setValue(obj, PropertyValue(value));
    assert(res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);
}

// A one-piece model of `form`, bounded on `var` to [lo, hi].
OntoMath::Piecewise boundedPiece(const ScalarForm& form, const std::string& var,
                                 double lo, double hi) {
    OntoMath::Piecewise model;
    model.inputVariable = var;
    OntoMath::Piecewise::Piece piece;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(form);
    piece.hasLo = true;
    piece.hasHi = true;
    piece.lo = lo;
    piece.hi = hi;
    model.pieces.push_back(std::move(piece));
    return model;
}

OntoMath::Piecewise everywhere(const ScalarForm& form, const std::string& var) {
    OntoMath::Piecewise model;
    model.inputVariable = var;
    model.pieces.push_back(
        OntoMath::Piecewise::Piece{false, false, 0.0, 0.0, true, true,
                                   OntoMath::MathNode::fromLegacyExpression(form),
                                   nullptr, {}, nullptr, nullptr, nullptr});
    return model;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "prophetic_rete_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "prophetic_rete_test", nullptr, nullptr);
    if (window) {
        glfwMakeContextCurrent(window);
    }

    // ======================================================================
    // A. The abstract value lattice.
    // ======================================================================
    {
        using Prophetic::Range;

        assert(Range::ofValue(PropertyValue(5.0)).kind == Range::Kind::Number);
        assert(Range::ofValue(PropertyValue(true)).kind == Range::Kind::Boolean);
        assert(Range::ofValue(PropertyValue(std::string("clay"))).kind == Range::Kind::Text);
        // A value the lattice cannot hold answers Top, never a guess.
        assert(Range::ofValue(PropertyValue(glm::vec3(1.0f))).isTop());
        assert(Range::ofValue(PropertyValue()).isTop());

        const Range low = Range::fromInterval(Interval(0.0f, 10.0f));
        const Range high = Range::fromInterval(Interval(100.0f, 1000.0f));
        const Range mid = Range::fromInterval(Interval(5.0f, 50.0f));
        assert(!low.mayIntersect(high));   // the whole point
        assert(low.mayIntersect(mid));
        assert(low.mayIntersect(Range::top()));
        assert(!low.mayIntersect(Range::bottom()));

        assert(Range::text({"a"}).mayIntersect(Range::text({"a", "b"})));
        assert(!Range::text({"a"}).mayIntersect(Range::text({"b"})));

        // met NARROWS a demand, so every case it cannot compute must widen to
        // Top rather than collapse to Bottom. A demand accidentally met down
        // to Bottom is a law ruled out that should not have been.
        assert(low.met(high).isBottom());
        assert(low.met(mid).kind == Range::Kind::Number);
        assert(nearf(low.met(mid).number.lo, 5.0) && nearf(low.met(mid).number.hi, 10.0));
        assert(low.met(Range::text({"a"})).isTop());        // kinds disagree -> Top
        assert(low.met(Range::top()).kind == Range::Kind::Number);

        // joined WIDENS, so Top is contagious.
        assert(low.joined(high).kind == Range::Kind::Number);
        assert(nearf(low.joined(high).number.hi, 1000.0));
        assert(low.joined(Range::top()).isTop());
        assert(low.joined(Range::bottom()).kind == Range::Kind::Number);

        // Bool relates to number, because bool IS arithmetic in this codebase.
        assert(Range::boolean(true, false)
                   .mayIntersect(Range::fromInterval(Interval(0.5f, 2.0f))));
        assert(!Range::boolean(false, true)
                    .mayIntersect(Range::fromInterval(Interval(0.5f, 2.0f))));
        std::puts("  A. lattice OK");
    }

    // ======================================================================
    // B. The interval algebra over OntoMath — Pass 3's engine.
    //    Before this, ScalarForm::evalRange did not exist and MathNode's
    //    answer for any non-constant formula was [-inf, +inf], which made
    //    every bound downstream of authored mathematics worthless.
    // ======================================================================
    {
        // Luna's worked example, §7 Pass 3: x in [0, 10] -> 2x + 5 -> [5, 25].
        const ScalarForm affine =
            ScalarForm::variable("x", 1.0, 2.0).plus(ScalarForm::constant(5.0));
        std::map<std::string, Interval> vars{{"x", Interval(0.0f, 10.0f)}};
        const Interval r = affine.evalRange(vars);
        assert(nearf(r.lo, 5.0) && nearf(r.hi, 25.0));

        // ...and the condition it is asked about: x > 100 is IMPOSSIBLE here.
        assert(!Prophetic::Range::fromInterval(r).mayIntersect(
            Prophetic::Range::fromInterval(Interval(100.0f, 1e30f))));

        // An even power turns around at zero; the endpoints alone would say
        // [-27, 4] for x^2 over [-3, 2], which is wrong on BOTH sides.
        const ScalarForm square = ScalarForm::variable("x", 2.0);
        std::map<std::string, Interval> sq{{"x", Interval(-3.0f, 2.0f)}};
        const Interval sqr = square.evalRange(sq);
        assert(nearf(sqr.lo, 0.0) && nearf(sqr.hi, 9.0));

        // A sinusoid is bounded no matter what its input does — the case that
        // makes "this glow law can never exceed 1" provable.
        const ScalarForm wave = ScalarForm::sinusoid(0.5, 1.0, 0.0, 0.5, "t");
        const Interval w = wave.evalRange({});   // t entirely unbound
        assert(nearf(w.lo, 0.0) && nearf(w.hi, 1.0));

        // ...and the bound is the honest one, not the lazy [-1, 1]: over a
        // short arc that contains no peak, sin is bounded by its endpoints.
        const ScalarForm plain = ScalarForm::transcendental(
            OntoMath::TransFactor::Kind::Sin, "t", 1.0, 0.0, 1.0);
        const Interval arc = plain.evalRange({{"t", Interval(0.0f, 1.0f)}});
        assert(arc.hi < 0.9f);   // sin(1) ~ 0.841, and no peak is reached
        assert(nearf(arc.lo, 0.0));

        // Unbound variables stay unbounded — soundness, not cleverness.
        assert(!ScalarForm::variable("x", 1.0).evalRange({}).bounded());

        // 0 * inf must not poison an interval with NaN. This one is
        // load-bearing beyond this file: geom::evalRange feeds MathNode
        // intervals to the tessellator's straddle test, and a NaN bound there
        // deletes cells that really do contain surface.
        const Interval poison = Interval::infinite() * Interval(0.0f);
        assert(!std::isnan(poison.lo) && !std::isnan(poison.hi));
        assert(nearf(poison.lo, 0.0) && nearf(poison.hi, 0.0));
        std::puts("  B. OntoMath interval algebra OK");
    }

    // ======================================================================
    // C. Write effects — what a law's action tree can put where.
    // ======================================================================
    {
        Prophetic::LawFacts facts;
        facts.lawId = "writer";
        Prophetic::analyzeAction(
            ActionNode::sequence({ActionNode::set("health", PropertyValue(5.0)),
                                  ActionNode::add("score", 1.0)}),
            facts);
        assert(facts.writes.size() == 2);
        assert(facts.writes[0].path == "health");
        assert(facts.writes[0].range.kind == Prophetic::Range::Kind::Number);
        assert(nearf(facts.writes[0].range.number.lo, 5.0));
        // Add composes with a value this analysis never saw: Top, not a guess.
        assert(facts.writes[1].path == "score" && facts.writes[1].range.isTop());
        assert(!facts.opaqueWrites);

        // A Map carries the authored mathematics into the write range.
        Prophetic::LawFacts mapped;
        mapped.lawId = "mapper";
        Prophetic::analyzeAction(
            ActionNode::map("glow",
                            boundedPiece(ScalarForm::variable("x", 1.0, 2.0)
                                             .plus(ScalarForm::constant(5.0)),
                                         "x", 0.0, 10.0),
                            {{"x", PropertyPath::parse("position.y")}}),
            mapped);
        assert(mapped.writes.size() == 1);
        assert(mapped.writes[0].range.kind == Prophetic::Range::Kind::Number);
        assert(nearf(mapped.writes[0].range.number.lo, 5.0));
        assert(nearf(mapped.writes[0].range.number.hi, 25.0));

        // Creation changes the fact base, not a value in it.
        Prophetic::LawFacts creator;
        creator.lawId = "creator";
        Prophetic::analyzeAction(ActionNode::create(0, "rock", {}), creator);
        assert(creator.opaqueWrites);
        std::puts("  C. write effects OK");
    }

    // ======================================================================
    // D. Read demands — what would satisfy a law's condition.
    // ======================================================================
    {
        const auto demandOn = [](const ConditionNode& c, const std::string& path) {
            Prophetic::LawFacts f;
            f.lawId = "reader";
            Prophetic::analyzeCondition(c, f);
            for (const auto& r : f.reads) {
                if (r.path == path) return r.satisfying;
            }
            return Prophetic::Range::bottom();   // "no demand recorded"
        };

        auto gt = demandOn(ConditionNode::compare("hp", ConditionNode::Op::Gt,
                                                 PropertyValue(100.0)),
                           "hp");
        assert(gt.kind == Prophetic::Range::Kind::Number && nearf(gt.number.lo, 100.0));

        // All() MEETS: both clauses must hold, so the demand narrows.
        auto both = demandOn(
            ConditionNode::all({ConditionNode::compare("hp", ConditionNode::Op::Gt,
                                                       PropertyValue(3.0)),
                                ConditionNode::compare("hp", ConditionNode::Op::Lt,
                                                       PropertyValue(7.0))}),
            "hp");
        assert(both.kind == Prophetic::Range::Kind::Number);
        assert(nearf(both.number.lo, 3.0) && nearf(both.number.hi, 7.0));

        // Any() JOINS: either clause suffices, so the demand widens.
        auto either = demandOn(
            ConditionNode::any({ConditionNode::compare("hp", ConditionNode::Op::Lt,
                                                       PropertyValue(1.0)),
                                ConditionNode::compare("hp", ConditionNode::Op::Gt,
                                                       PropertyValue(9.0))}),
            "hp");
        assert(either.isTop());   // (-inf,1] joined [9,inf) covers everything

        // A path only ONE arm of an Any constrains is unconstrained overall —
        // the other arm can satisfy the law without it. Getting this backwards
        // is the difference between a filter and a bug.
        {
            Prophetic::LawFacts f;
            f.lawId = "either-side";
            Prophetic::analyzeCondition(
                ConditionNode::any({ConditionNode::compare("a", ConditionNode::Op::Gt,
                                                           PropertyValue(1.0)),
                                    ConditionNode::compare("b", ConditionNode::Op::Gt,
                                                           PropertyValue(1.0))}),
                f);
            for (const auto& r : f.reads) assert(r.satisfying.isTop());
            // ...but BOTH are still recorded as read. Dropping the demand must
            // never drop the read.
            assert(f.readNames.count("a") && f.readNames.count("b"));
        }

        // Negation is a hole, and this lattice holds intervals, not holes.
        assert(demandOn(ConditionNode::negate(ConditionNode::compare(
                            "hp", ConditionNode::Op::Gt, PropertyValue(5.0))),
                        "hp")
                   .isTop());
        // Ne likewise.
        assert(demandOn(ConditionNode::compare("hp", ConditionNode::Op::Ne,
                                               PropertyValue(5.0)),
                        "hp")
                   .isTop());
        // A right-hand side read live off another property has no bound, and
        // the right-hand path is itself recorded as a read.
        {
            Prophetic::LawFacts f;
            f.lawId = "live-rhs";
            Prophetic::analyzeCondition(
                ConditionNode::comparePaths("hp", ConditionNode::Op::Gt, "maxHp"), f);
            assert(f.readNames.count("hp") && f.readNames.count("maxHp"));
            for (const auto& r : f.reads) assert(r.satisfying.isTop());
        }

        // Overlaps and Related consult machinery this walk cannot name, so
        // the whole law's reads go opaque rather than half-enumerated.
        {
            Prophetic::LawFacts f;
            f.lawId = "toucher";
            Prophetic::analyzeCondition(ConditionNode::overlaps("@event.object"), f);
            assert(f.opaqueReads);
        }
        {
            Prophetic::LawFacts f;
            f.lawId = "related";
            Prophetic::analyzeCondition(ConditionNode::related("holds", ""), f);
            assert(f.opaqueReads);
        }

        // A quantifier's inner reads are about the INSTANCES, and are filed as
        // such — but they are still filed. A condition that sweeps every
        // Object's height still has to hear a height change.
        {
            Prophetic::LawFacts f;
            f.lawId = "quantified";
            Prophetic::analyzeCondition(
                ConditionNode::forAny(ConditionNode::BeingKind::Object,
                                      ConditionNode::compare("height", ConditionNode::Op::Gt,
                                                             PropertyValue(3.0))),
                f);
            assert(f.readNames.count("height"));
            bool sawInstanceRead = false;
            for (const auto& r : f.reads) {
                if (r.path == "height") {
                    assert(r.aboutInstances);
                    sawInstanceRead = true;
                }
            }
            assert(sawInstanceRead);
        }
        std::puts("  D. read demands OK");
    }

    // ======================================================================
    // E. Path names — the sound over-approximation.
    //
    // A PropertyPath resolves through nested Singulars, and the property-change
    // callback reports the name on WHICHEVER Singular ends up owning the leaf,
    // not the path the author wrote. So "body.head.position" changing arrives
    // as ("position") and the filter must recognise it. Anything narrower than
    // every contiguous sub-run is unsound.
    // ======================================================================
    {
        std::unordered_set<std::string> names;
        Prophetic::collectPathNames(PropertyPath::parse("body.head.position"), names);
        assert(names.count("body"));
        assert(names.count("position"));            // the leaf owner's own name
        assert(names.count("head.position"));       // a flat registration
        assert(names.count("body.head.position"));

        // A referent segment names WHOSE property, not which one.
        std::unordered_set<std::string> qualified;
        Prophetic::collectPathNames(PropertyPath::parse("@event.subject.position.y"),
                                    qualified);
        assert(qualified.count("position"));
        assert(qualified.count("subject.position.y"));
        assert(!qualified.count("@event"));
        std::puts("  E. path names OK");
    }

    // ======================================================================
    // F. THE SAFETY SECTION. A filter derived from this index must never make
    //    a law go deaf, and must fail open every way it can be wrong.
    // ======================================================================
    {
        Object subject;
        subject.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        std::vector<Singular*> population{&subject};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        Object author;
        LawManager mgr;
        mgr.connectToEventBus();

        auto listens = mgr.createLaw("listens-to-y", {&author});
        listens->setActivation(Law::Activation::WhileTrue);
        listens->setConditionModel(
            ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(4.0)));
        listens->setActionModel(ActionNode::set("position.z", PropertyValue(9.0)));

        mgr.tick();   // syncs the index off the law text
        assert(mgr.prophetic().complete());

        // The law reads position; a change to position must always get through.
        assert(mgr.propheticHears("position"));

        // A property NO condition anywhere reads is provably irrelevant.
        // `objectType` is a real registered property that this law's text
        // never mentions.
        assert(!mgr.propheticHears("objectType"));

        mgr.resetPropheticCounters();
        Singular::notifyPropertyChanged(&subject, "objectType");
        assert(mgr.propheticCounters().asked == 1);
        assert(mgr.propheticCounters().filtered == 1);

        // ...and the law still fires on the property it does read. This is
        // the assertion the whole optimization has to survive.
        writeFloat(subject, "position.y", 5.0f);
        mgr.tick();
        assert(nearf(readNumber(subject, "position.z"), 9.0));

        // --- fail open (1): the index is stale. ---------------------------
        // Adding a law bumps the register's text revision. Until the next
        // sync, the index describes a different law set than the live one, and
        // every answer must be "possibly".
        auto later = mgr.createLaw("added-later", {&author});
        later->setActivation(Law::Activation::WhileTrue);
        later->setConditionModel(
            ConditionNode::compare("objectType", ConditionNode::Op::Eq,
                                   PropertyValue(std::string("rock"))));
        assert(mgr.propheticHears("objectType"));   // stale -> open
        mgr.tick();
        // ...and once synced, the newly authored read is honoured on its own
        // merits, not because the gate gave up.
        assert(mgr.propheticHears("objectType"));
        assert(mgr.prophetic().anyConditionReads("objectType"));

        // --- fail open (2): a law whose reads are not enumerable. ---------
        auto opaque = mgr.createLaw("touches-things", {&author});
        opaque->setActivation(Law::Activation::WhileTrue);
        opaque->setConditionModel(ConditionNode::overlaps("@event.object"));
        mgr.tick();
        assert(!mgr.prophetic().complete());
        // Every property is now audible, including ones nothing names.
        assert(mgr.propheticHears("a-name-no-law-mentions"));
        assert(mgr.propheticHears("position"));

        assert(mgr.remove(opaque->getIdentifier()));
        mgr.tick();
        assert(mgr.prophetic().complete());
        assert(!mgr.propheticHears("a-name-no-law-mentions"));

        // --- fail open (3): an opaque alpha node bound straight to the
        //     network. A hand-written predicate matches on whatever it likes.
        std::size_t opaqueAlpha = mgr.rete().addAlphaNode(
            "anything at all", [](const FactPtr&) { return true; });
        mgr.rete().bindLawToAlpha(listens->getIdentifier(), opaqueAlpha);
        assert(mgr.rete().hasOpaqueBoundAlpha());
        assert(mgr.propheticHears("a-name-no-law-mentions"));
        mgr.rete().unbindLawFromAlpha(listens->getIdentifier(), opaqueAlpha);

        Universe::instance().setProvider({});
        std::puts("  F. safety / fail-open OK");
    }

    // ======================================================================
    // G. What the abstract interpretation actually concludes — the prophecy.
    // ======================================================================
    {
        // (1) Self-impossible, proved from one law's own text: an authored
        //     Zone function bounded to [0, 1] whose satisfaction window starts
        //     at 100. No world can satisfy this, ever.
        auto impossible = std::make_shared<Law>("never-satisfiable");
        impossible->setConditionModel(ConditionNode::zone(
            everywhere(ScalarForm::sinusoid(0.5, 1.0, 0.0, 0.5, "t"), "t"),
            {{"t", PropertyPath::parse("position.y")}},
            PropertyValue(100.0), PropertyValue(1000.0)));

        Prophetic::Index index;
        index.rebuild({impossible});
        bool sawSelf = false;
        for (const auto& u : index.unreachable()) {
            if (u.lawId == impossible->getIdentifier() && u.selfImpossible) sawSelf = true;
        }
        assert(sawSelf);

        // (2) No lawful driver: one law can only ever write hp := 1, another
        //     demands hp > 100. Nothing in this law set can bridge them.
        auto writer = std::make_shared<Law>("sets-hp-to-one");
        writer->setActionModel(ActionNode::set("hp", PropertyValue(1.0)));
        auto reader = std::make_shared<Law>("wants-hp-over-100");
        reader->setConditionModel(
            ConditionNode::compare("hp", ConditionNode::Op::Gt, PropertyValue(100.0)));

        Prophetic::Index pair;
        pair.rebuild({writer, reader});
        bool sawDriver = false;
        for (const auto& u : pair.unreachable()) {
            if (u.lawId == reader->getIdentifier() && !u.selfImpossible) sawDriver = true;
        }
        assert(sawDriver);

        // (3) ...and the claim is scoped to AUTHORED LAW, not to the world.
        //     Add any opaque writer — a Create, a First Mover — and the
        //     cross-law finding must vanish: the union of authored writes is
        //     no longer the union of everything laws can do.
        auto maker = std::make_shared<Law>("makes-things");
        maker->setActionModel(ActionNode::create(0, "rock", {}));
        Prophetic::Index withMaker;
        withMaker.rebuild({writer, reader, maker});
        for (const auto& u : withMaker.unreachable()) {
            assert(u.selfImpossible);   // only text-local proofs survive
        }

        // (4) A property NOBODY writes is not a finding. Most properties a
        //     condition reads are moved by First Movers and tools, which this
        //     analysis cannot see — Zach's §20/§21 edge case.
        auto lonely = std::make_shared<Law>("reads-something-unwritten");
        lonely->setConditionModel(ConditionNode::compare("mood", ConditionNode::Op::Gt,
                                                         PropertyValue(3.0)));
        Prophetic::Index alone;
        alone.rebuild({lonely});
        assert(alone.unreachable().empty());

        // (5) And a satisfiable pair produces no finding at all.
        auto generous = std::make_shared<Law>("sets-hp-high");
        generous->setActionModel(ActionNode::set("hp", PropertyValue(500.0)));
        Prophetic::Index fine;
        fine.rebuild({generous, reader});
        assert(fine.unreachable().empty());

        // The report is legible, because nothing here is a black box.
        const nlohmann::json report = pair.toJson();
        assert(report["complete"] == true);
        assert(report["lawCount"] == 2);
        assert(!report["unreachable"].empty());
        std::puts("  G. prophecy OK");
    }

    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("prophetic_rete_test: ALL OK");
    return 0;
}
