// Reversal — reading a law's writes BACKWARDS, exactly.
//
// A Flow authors dp/dt and a Map authors p = F(t). OntoMath's antiderivative
// is exact, so the past value of a property governed by one is COMPUTED in
// closed form — p(t−Δ) = p(t) − ∫[t−Δ,t] dp/dt — rather than replayed out of
// a log. Nothing is recorded; the law text is the record.
//
// The refusals are the half that matters. This test pins both: that the
// algebra reverses what it can hold, and that it says NO, with a reason,
// everywhere it cannot — a Set that overwrote what was there, a rate needing
// integration by parts, a piece gated on a world guard, a rate that reads
// what it writes. Each refusal is a stretch of world that is genuinely
// irreversible, which is a fact about that world and not a gap in the engine.

#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Object/Object.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }

bool mentions(const std::vector<std::string>& obstacles, const std::string& needle) {
    for (const auto& text : obstacles) {
        if (text.find(needle) != std::string::npos) return true;
    }
    return false;
}

OntoMath::Piecewise everywhere(OntoMath::ScalarForm e, const std::string& var = "t") {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(std::move(e)));
    f.inputVariable = var;
    return f;
}

OntoMath::Piecewise::Piece bounded(OntoMath::ScalarForm e, double lo, double hi) {
    OntoMath::Piecewise::Piece piece;
    piece.hasLo = true;  piece.lo = lo;
    piece.hasHi = true;  piece.hi = hi;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(std::move(e));
    return piece;
}

}   // namespace

int main() {
    const MathBindings clockBinding{{"t", PropertyPath::parse("time")}};

    // =====================================================================
    // 1. The definite integral is EXACT, not sampled.
    // =====================================================================
    {
        // ∫[0,π] sin t dt = 2, to the arithmetic, from -cos t.
        const auto sine = everywhere(
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "t"));
        const auto area = OntoMath::definiteIntegral(sine, "t", 0.0, M_PI, {});
        assert(area && near(*area, 2.0, 1e-12));

        // ∫[0,2] 3t² dt = 8, from t³.
        OntoMath::ScalarForm cubicRate;
        cubicRate.terms.push_back(OntoMath::Term(3.0, {{"t", 2.0}}));
        const auto cubic = OntoMath::definiteIntegral(everywhere(cubicRate), "t", 0.0, 2.0, {});
        assert(cubic && near(*cubic, 8.0, 1e-12));

        // Backwards runs backwards: ∫[2,0] = −∫[0,2].
        const auto reversed = OntoMath::definiteIntegral(everywhere(cubicRate), "t", 2.0, 0.0, {});
        assert(reversed && near(*reversed, -8.0, 1e-12));

        // A constant of the OTHER bound variables rides along untouched.
        OntoMath::ScalarForm scaled;
        scaled.terms.push_back(OntoMath::Term(1.0, {{"t", 1.0}, {"k", 1.0}}));
        const auto withK = OntoMath::definiteIntegral(everywhere(scaled), "t", 0.0, 2.0, {{"k", 5.0}});
        assert(withK && near(*withK, 10.0, 1e-12));   // 5 · t²/2 over [0,2]
    }

    // =====================================================================
    // 2. The piecewise structure is honoured — and a hole in the domain is
    //    refused rather than counted as zero.
    // =====================================================================
    {
        OntoMath::Piecewise steps;
        steps.inputVariable = "t";
        steps.pieces.push_back(bounded(OntoMath::ScalarForm::constant(1.0), 0.0, 1.0));
        steps.pieces.push_back(bounded(OntoMath::ScalarForm::constant(2.0), 1.0, 3.0));

        // 1·(1−0) + 2·(3−1) = 5, integrated across the breakpoint.
        const auto across = OntoMath::definiteIntegral(steps, "t", 0.0, 3.0, {});
        assert(across && near(*across, 5.0, 1e-12));

        // Past the last piece the rate is UNDEFINED. Undefined is not zero.
        std::string why;
        const auto beyond = OntoMath::definiteIntegral(steps, "t", 0.0, 4.0, {}, &why);
        assert(!beyond);
        assert(why.find("undefined") != std::string::npos);
    }

    // =====================================================================
    // 3. The honest refusals of the algebra itself.
    // =====================================================================
    {
        // t·sin(t) needs integration by parts. The algebra does not hold it,
        // and says so instead of approximating.
        OntoMath::Term byParts(1.0, {{"t", 1.0}});
        byParts.addTrans(OntoMath::TransFactor(OntoMath::TransFactor::Kind::Sin, "t"));
        OntoMath::ScalarForm product;
        product.terms.push_back(byParts);
        std::string why;
        assert(!OntoMath::integrable(everywhere(product), "t", &why));
        assert(why.find("by parts") != std::string::npos);

        // A piece gated on a WORLD guard cannot be integrated over the past:
        // answering whether it applied then needs the past being computed.
        OntoMath::Piecewise guarded = everywhere(OntoMath::ScalarForm::constant(1.0));
        guarded.pieces[0].guard = std::make_shared<ConditionNode>(ConditionNode::compare(
            "position.x", ConditionNode::Op::Gt, PropertyValue(1.0)));
        why.clear();
        assert(!OntoMath::integrable(guarded, "t", &why));
        assert(why.find("guard") != std::string::npos);

        // Bounds that cut a DIFFERENT variable than the one being integrated.
        OntoMath::Piecewise crossed;
        crossed.inputVariable = "u";
        crossed.pieces.push_back(bounded(OntoMath::ScalarForm::constant(1.0), 0.0, 1.0));
        why.clear();
        assert(!OntoMath::integrable(crossed, "t", &why));
        assert(why.find("bounds cut") != std::string::npos);

        // An op with no scalar closed form refuses by NAME, so an author can
        // see which node stopped it.
        auto gradientNode = std::make_unique<OntoMath::MathNode>();
        gradientNode->op = OntoMath::MathNode::Op::Gradient;
        why.clear();
        assert(!OntoMath::toScalarForm(*gradientNode, &why));
        assert(why.find("Gradient") != std::string::npos);
    }

    // =====================================================================
    // 4. The judgement on a law's TEXT — no subject, no values, nothing run.
    // =====================================================================
    {
        // A rate that is a pure function of the clock: reversible.
        const auto cosine = everywhere(
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "t"));
        const auto rising = ActionNode::flow("position.y", cosine, clockBinding);
        assert(rising.reversibility().exact);
        assert(rising.reversibility().summary() == "exactly reversible");

        // Set destroys what was there, and the law text does not keep it.
        const auto overwrite = ActionNode::set("position.y", PropertyValue(3.0));
        const auto setJudgement = overwrite.reversibility();
        assert(!setJudgement.exact);
        assert(mentions(setJudgement.obstacles, "overwrote"));

        // Add is algebraically invertible but the firing count is nowhere in
        // the text — so the past is not recoverable from the law alone.
        const auto bump = ActionNode::add("position.y", 1.0);
        assert(!bump.reversibility().exact);
        assert(mentions(bump.reversibility().obstacles, "how many times it fired"));

        // A rate that READS what it WRITES is an ODE, not a quadrature.
        const MathBindings selfReading{{"t", PropertyPath::parse("time")},
                                       {"y", PropertyPath::parse("position.y")}};
        OntoMath::ScalarForm decay;
        decay.terms.push_back(OntoMath::Term(-1.0, {{"y", 1.0}}));
        const auto exponential = ActionNode::flow("position.y", everywhere(decay), selfReading);
        const auto odeJudgement = exponential.reversibility();
        assert(!odeJudgement.exact);
        assert(mentions(odeJudgement.obstacles, "differential equation"));

        // No clock among the bindings: there is no axis to travel along.
        const MathBindings spaceOnly{{"t", PropertyPath::parse("position.x")}};
        const auto placeless = ActionNode::flow("position.y", cosine, spaceOnly);
        assert(!placeless.reversibility().exact);
        assert(mentions(placeless.reversibility().obstacles, "not a function of time"));

        // Map needs only to be re-evaluable at the past time — a weaker
        // demand than Flow's, and t·sin(t) meets it though its integral does
        // not exist in this algebra.
        OntoMath::Term byParts(1.0, {{"t", 1.0}});
        byParts.addTrans(OntoMath::TransFactor(OntoMath::TransFactor::Kind::Sin, "t"));
        OntoMath::ScalarForm product;
        product.terms.push_back(byParts);
        assert(ActionNode::map("position.y", everywhere(product), clockBinding)
                   .reversibility().exact);
        assert(!ActionNode::flow("position.y", everywhere(product), clockBinding)
                    .reversibility().exact);

        // Drive's CurveModel is symbolic, so it reverses — when its input is
        // the clock. Driven by anything else, the input's own past is the
        // unknown, one level out.
        assert(ActionNode::drive("position.y", CurveModel::sinusoid(1.0, 1.0), "time")
                   .reversibility().exact);
        const auto byDistance =
            ActionNode::drive("position.y", CurveModel::sinusoid(1.0, 1.0), "position.x");
        assert(!byDistance.reversibility().exact);
        assert(mentions(byDistance.reversibility().obstacles, "not the clock"));

        // A composite is reversible only if every branch is, and the obstacle
        // names the branch that spoiled it.
        const auto mixed = ActionNode::sequence({rising, overwrite});
        const auto mixedJudgement = mixed.reversibility();
        assert(!mixedJudgement.exact);
        assert(mentions(mixedJudgement.obstacles, "overwrote"));
        // Publishing an event writes no property, so it is no obstacle here.
        assert(ActionNode::sequence({rising, ActionNode::publish("chord-sounded")})
                   .reversibility().exact);
    }

    // =====================================================================
    // 5. The past, computed off a live being.
    // =====================================================================
    {
        Object bell;
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&bell);
        });

        // dy/dt = cos t, so y(t) = sin t. Stand the world at t = 5 with the
        // property holding exactly what the mathematics says it should.
        const auto cosine = everywhere(
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "t"));
        const auto rising = ActionNode::flow("position.y", cosine, clockBinding);

        Universe::instance().setClock(5.0, 0.1);
        bell.setPosition(glm::vec3(0.0f, static_cast<float>(std::sin(5.0)), 0.0f));

        // Two seconds ago the world clock read 3, and y was sin(3) — reached
        // by subtracting exactly what flowed over [3,5], with no log of it.
        const auto twoAgo = rising.valueSecondsAgo(bell, 2.0);
        assert(twoAgo);
        double past = 0.0;
        assert(propertyValueToNumber(*twoAgo, past));
        assert(near(past, std::sin(3.0), 1e-6));

        // Five seconds ago: back to the origin the model started from.
        const auto fiveAgo = rising.valueSecondsAgo(bell, 5.0);
        assert(fiveAgo && propertyValueToNumber(*fiveAgo, past));
        assert(near(past, 0.0, 1e-6));

        // Zero is the present, and the present is what the being holds.
        const auto now = rising.valueSecondsAgo(bell, 0.0);
        assert(now && propertyValueToNumber(*now, past));
        assert(near(past, std::sin(5.0), 1e-6));

        // The Map counterpart travels the same road from the other side:
        // y = sin t evaluated at t−Δ, no integration needed, same answer.
        const auto sine = everywhere(
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "t"));
        const auto position = ActionNode::map("position.y", sine, clockBinding);
        const auto mapped = position.valueSecondsAgo(bell, 2.0);
        assert(mapped && propertyValueToNumber(*mapped, past));
        assert(near(past, std::sin(3.0), 1e-9));

        // An irreversible action refuses to answer rather than guessing.
        assert(!ActionNode::set("position.y", PropertyValue(1.0)).valueSecondsAgo(bell, 1.0));

        Universe::instance().setProvider(nullptr);
    }

    std::printf("law_reversal_test: OK\n");
    return 0;
}
