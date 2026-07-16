// OntoMath milestone test: Earthcall is driven by math in the program.
//
// The exact symbolic core (multivariate terms, algebra, calculus, piecewise
// with open/closed bounds), and the two authorable kinds wired into laws:
// Zone conditions (input satisfies the designated zone of the function) and
// Map actions (output governed by an authored function of authored inputs).
// Every primitive — variables, bindings, coefficients, exponents, bounds —
// is model data, so authoring and modification ride the same serialization
// as all law text.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/OntoMath/Expression.hpp"
#include "Singularity/OntoMath/Operations.hpp"
#include "Form/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool neard(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }
bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "ontomath_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "ontomath_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "ontomath_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        using OntoMath::Expression;
        using OntoMath::Piecewise;
        using OntoMath::Term;
        using OntoMath::TransFactor;

        // ------------------------------------------------------------------
        // 1. Exact evaluation: f(x,y) = 3x²y + 2x − 5.
        // ------------------------------------------------------------------
        Expression f;
        f.terms.push_back(Term(3.0, {{"x", 2.0}, {"y", 1.0}}));
        f.terms.push_back(Term(2.0, {{"x", 1.0}}));
        f.terms.push_back(Term(-5.0));

        assert(neard(*f.evaluate({{"x", 2.0}, {"y", 3.0}}), 35.0));
        assert(!f.evaluate({{"x", 2.0}}));                 // unbound y: no value

        // ------------------------------------------------------------------
        // 2. Exact algebra: like terms combine; products distribute.
        // ------------------------------------------------------------------
        Expression sum = Expression::variable("x").scaled(2.0)
                             .plus(Expression::variable("x").scaled(3.0));
        assert(sum.terms.size() == 1 && neard(sum.terms[0].coefficient, 5.0));

        Expression xPlus1 = Expression::variable("x").plus(Expression::constant(1.0));
        Expression xMinus1 = Expression::variable("x").plus(Expression::constant(-1.0));
        Expression difference = xPlus1.times(xMinus1);     // x² − 1
        assert(difference.terms.size() == 2);
        assert(neard(*difference.evaluate({{"x", 7.0}}), 48.0));

        // ------------------------------------------------------------------
        // 3. Exact calculus by the power rule.
        // ------------------------------------------------------------------
        Expression dfdx = f.derivative("x");               // 6xy + 2
        assert(neard(*dfdx.evaluate({{"x", 2.0}, {"y", 3.0}}), 38.0));
        Expression dfdy = f.derivative("y");               // 3x²
        assert(neard(*dfdy.evaluate({{"x", 2.0}, {"y", 0.0}}), 12.0));

        Expression threeXsq = Expression::variable("x", 2.0, 3.0);
        auto integral = threeXsq.antiderivative("x");      // x³
        assert(integral.has_value());
        assert(neard(*integral->evaluate({{"x", 2.0}}), 8.0));
        // Fundamental round-trip: d/dx ∫ f = f.
        Expression back = integral->derivative("x");
        assert(neard(*back.evaluate({{"x", 5.0}}), *threeXsq.evaluate({{"x", 5.0}})));
        // ∫ x⁻¹ = ln(x) — the algebra holds it now (section 9 proves it);
        // once refused honestly, today answered exactly.
        assert(Expression::variable("x", -1.0).antiderivative("x").has_value());

        // ------------------------------------------------------------------
        // 4. The hyperoperation ladder (first movers of arithmetic).
        // ------------------------------------------------------------------
        assert(neard(Operations::hyperop(2, 3, 4), 12.0));      // multiplication
        assert(neard(Operations::hyperop(3, 2, 10), 1024.0));   // exponentiation
        assert(neard(Operations::hyperop(4, 2, 3), 16.0));      // tetration 2↑↑3

        // ------------------------------------------------------------------
        // 5. Piecewise with open/closed bounds; undefined outside, not zero.
        // ------------------------------------------------------------------
        Piecewise pw;
        Piecewise::Piece rising;                            // x on [0, 1)
        rising.hasLo = true; rising.lo = 0.0; rising.includeLo = true;
        rising.hasHi = true; rising.hi = 1.0; rising.includeHi = false;
        rising.expression = Expression::variable("x");
        Piecewise::Piece falling;                           // 2 − x on [1, 2]
        falling.hasLo = true; falling.lo = 1.0; falling.includeLo = true;
        falling.hasHi = true; falling.hi = 2.0; falling.includeHi = true;
        falling.expression = Expression::constant(2.0)
                                 .plus(Expression::variable("x").scaled(-1.0));
        pw.pieces = {rising, falling};

        assert(neard(*pw.evaluate({{"x", 0.5}}), 0.5));
        assert(neard(*pw.evaluate({{"x", 1.0}}), 1.0));     // open [0,1) hands 1 to [1,2]
        assert(neard(*pw.evaluate({{"x", 2.0}}), 0.0));     // closed hi included
        assert(!pw.evaluate({{"x", 2.5}}));                 // outside every piece
        assert(!pw.evaluate({{"x", -0.1}}));

        // ------------------------------------------------------------------
        // 6. Zone condition: the disk x² + z² ≤ 4 — a mathematically-defined
        //    satisfaction zone, desmos-precise, as a law's condition.
        // ------------------------------------------------------------------
        Object author;
        Expression paraboloid;
        paraboloid.terms.push_back(Term(1.0, {{"x", 2.0}}));
        paraboloid.terms.push_back(Term(1.0, {{"z", 2.0}}));

        Law diskLaw("gild-the-disk");
        diskLaw.addAuthor(author);
        diskLaw.setConditionModel(ConditionNode::zone(
            Piecewise::continuous(paraboloid),
            MathBindings{{"x", PropertyPath::parse("position.x")},
                         {"z", PropertyPath::parse("position.z")}},
            PropertyValue{},                 // no lower bound
            PropertyValue(4.0)));            // f ≤ 4: radius-2 disk
        diskLaw.setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        Object inside;
        inside.setPosition(glm::vec3(1.0f, 0.0f, 1.0f));    // f = 2
        assert(diskLaw.applyTo(inside) == Law::ApplicationResult::Applied);
        Object outside;
        outside.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));   // f = 9
        assert(diskLaw.applyTo(outside) == Law::ApplicationResult::ConditionsFailed);

        // The authored mathematics survives serialization like all law text.
        auto reborn = Law::fromJson(diskLaw.toJson());
        reborn->addAuthor(author);
        Object inside2;
        inside2.setPosition(glm::vec3(0.0f, 0.0f, -1.5f));  // f = 2.25
        assert(reborn->applyTo(inside2) == Law::ApplicationResult::Applied);
        Object outside2;
        outside2.setPosition(glm::vec3(2.5f, 0.0f, 0.0f));
        assert(reborn->applyTo(outside2) == Law::ApplicationResult::ConditionsFailed);

        // ------------------------------------------------------------------
        // 7. Map action: position.y := x² − 1, x bound to position.x.
        // ------------------------------------------------------------------
        Law liftLaw("parabolic-lift");
        liftLaw.addAuthor(author);
        liftLaw.setActionModel(ActionNode::map(
            "position.y",
            Piecewise::continuous(Expression::variable("x", 2.0)
                                      .plus(Expression::constant(-1.0))),
            MathBindings{{"x", PropertyPath::parse("position.x")}}));

        Object lifted;
        lifted.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
        assert(liftLaw.applyTo(lifted) == Law::ApplicationResult::Applied);
        assert(nearf(lifted.getPosition().y, 8.0f));

        // Piecewise map: outside the authored domain, NOTHING is written.
        Law tentLaw("tent");
        tentLaw.addAuthor(author);
        tentLaw.setActionModel(ActionNode::map(
            "position.y", pw,                                // the [0,2] tent
            MathBindings{{"x", PropertyPath::parse("position.x")}}));
        Object beyond;
        beyond.setPosition(glm::vec3(5.0f, 7.0f, 0.0f));     // x = 5: undefined
        assert(tentLaw.applyTo(beyond) == Law::ApplicationResult::Applied);
        assert(nearf(beyond.getPosition().y, 7.0f));         // untouched

        // ------------------------------------------------------------------
        // 8. Calculus in the pipeline: govern by the DERIVATIVE of an
        //    authored function (slope law: rotation.y := d/dx x³ = 3x²).
        // ------------------------------------------------------------------
        Expression cubic = Expression::variable("x", 3.0);
        Law slopeLaw("turn-with-slope");
        slopeLaw.addAuthor(author);
        slopeLaw.setActionModel(ActionNode::map(
            "rotation.y",
            Piecewise::continuous(cubic.derivative("x")),
            MathBindings{{"x", PropertyPath::parse("position.x")}}));

        Object turner;
        turner.setPosition(glm::vec3(2.0f, 0.0f, 0.0f));
        assert(slopeLaw.applyTo(turner) == Law::ApplicationResult::Applied);
        assert(nearf(turner.getRotationEulerDegrees().y, 12.0f, 1e-2f));

        // ------------------------------------------------------------------
        // 9. Transcendentals: sin/cos/exp/ln as EXACT factors — periodic and
        //    exponential change as law-text, closed under the calculus.
        // ------------------------------------------------------------------
        const double kPi = 3.14159265358979323846;

        // Exact evaluation.
        Expression wave = Expression::transcendental(
            TransFactor::Kind::Sin, "t", 2.0, 0.0, 3.0);   // 3·sin(2t)
        assert(neard(*wave.evaluate({{"t", kPi / 4.0}}), 3.0));   // sin(π/2) = 1
        Expression growth = Expression::transcendental(
            TransFactor::Kind::Exp, "t");
        assert(neard(*growth.evaluate({{"t", 0.0}}), 1.0));
        Expression logOf = Expression::transcendental(
            TransFactor::Kind::Ln, "x", 3.0);              // ln(3x)
        assert(neard(*logOf.evaluate({{"x", 1.0 / 3.0}}), 0.0));
        assert(!logOf.evaluate({{"x", -1.0}}));            // outside domain: undefined
        assert(!logOf.evaluate({{"x", 0.0}}));

        // Chain rule: d/dt 3·sin(2t) = 6·cos(2t).
        Expression dwave = wave.derivative("t");
        assert(neard(*dwave.evaluate({{"t", 0.0}}), 6.0));
        // Product rule: d/dx x·sin(x) = sin(x) + x·cos(x).
        Expression xsin = Expression::variable("x").times(
            Expression::transcendental(TransFactor::Kind::Sin, "x"));
        Expression dxsin = xsin.derivative("x");
        assert(dxsin.terms.size() == 2);
        assert(neard(*dxsin.evaluate({{"x", kPi}}),
                     std::sin(kPi) + kPi * std::cos(kPi)));
        // d/dx ln(3x) = 1/x (the scale cancels — Ln carries no shift).
        Expression dlog = logOf.derivative("x");
        assert(neard(*dlog.evaluate({{"x", 4.0}}), 0.25));
        // exp is its own derivative (times the inner scale).
        Expression dgrowth = growth.derivative("t");
        assert(neard(*dgrowth.evaluate({{"t", 1.5}}), std::exp(1.5)));

        // ∫x⁻¹ dx = ln(x): the old honest gap CLOSES.
        Expression inverse = Expression::variable("x", -1.0, 5.0);   // 5/x
        auto lnIntegral = inverse.antiderivative("x");
        assert(lnIntegral.has_value());
        assert(neard(*lnIntegral->evaluate({{"x", 2.0}}), 5.0 * std::log(2.0)));
        // ∫3·sin(2t) dt = -(3/2)·cos(2t); its derivative returns the wave.
        auto waveIntegral = wave.antiderivative("t");
        assert(waveIntegral.has_value());
        assert(neard(*waveIntegral->evaluate({{"t", 0.0}}), -1.5));
        Expression roundTrip = waveIntegral->derivative("t");
        assert(neard(*roundTrip.evaluate({{"t", 0.7}}), *wave.evaluate({{"t", 0.7}})));
        // ∫ln(3x) dx = x·ln(3x) − x, checked against the analytic value.
        auto logIntegral = logOf.antiderivative("x");
        assert(logIntegral.has_value());
        assert(neard(*logIntegral->evaluate({{"x", 2.0}}),
                     2.0 * std::log(6.0) - 2.0));
        // x·sin(x) needs integration by parts: honestly not yet held.
        assert(!xsin.antiderivative("x").has_value());

        // Like terms combine across identical transcendental shapes.
        Expression doubled = wave.plus(wave);
        assert(doubled.terms.size() == 1);
        assert(neard(doubled.terms[0].coefficient, 6.0));

        // The exact sinusoid matches evalTrack's form: bias + A·sin(2π(f·t + φ)).
        Expression track = Expression::sinusoid(2.0, 0.5, 0.25, 1.0, "t");
        assert(neard(*track.evaluate({{"t", 0.0}}), 1.0 + 2.0 * std::sin(kPi / 2.0)));

        // Law-text like everything else: survives serialization.
        Expression rebornWave = Expression::fromJson(wave.toJson());
        assert(rebornWave.terms.size() == 1 && rebornWave.terms[0].trans.size() == 1);
        assert(neard(*rebornWave.evaluate({{"t", 0.3}}), *wave.evaluate({{"t", 0.3}})));

        // And runs in the pipeline: position.y := sin(π/2 · x) on a subject.
        Law waveLaw("crest");
        waveLaw.addAuthor(author);
        waveLaw.setActionModel(ActionNode::map(
            "position.y",
            Piecewise::continuous(Expression::transcendental(
                TransFactor::Kind::Sin, "x", kPi / 2.0)),
            MathBindings{{"x", PropertyPath::parse("position.x")}}));
        Object surfer;
        surfer.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
        assert(waveLaw.applyTo(surfer) == Law::ApplicationResult::Applied);
        assert(nearf(surfer.getPosition().y, 1.0f));         // sin(π/2) = 1

        // ------------------------------------------------------------------
        // 10. EXPRESSION-GUARDED PIECES — the discrete-math fusion. A piece
        //     may be gated by a CONDITION instead of interval bounds, so
        //     min/max/abs become DEFINABLE and the SDF boolean algebra
        //     follows. Guards testify about a subject; without one they are
        //     unproven and skipped — never guessed.
        // ------------------------------------------------------------------
        Object witness;
        const MathBindings xBind{{"x", PropertyPath::parse("position.x")}};
        const auto guardLEZero = [&](Expression g) {
            // "applies where g(vars) <= 0" — the min/max workhorse, built
            // from the EXISTING Zone condition: zero new condition kinds.
            return std::make_shared<ConditionNode>(ConditionNode::zone(
                Piecewise::continuous(std::move(g)), xBind,
                PropertyValue{}, PropertyValue(0.0)));
        };

        // abs(x): where -x <= 0 (i.e. x >= 0) use x; otherwise use -x.
        Piecewise absF;
        {
            Piecewise::Piece positive;
            positive.guard = guardLEZero(Expression::variable("x", 1.0, -1.0));
            positive.expression = Expression::variable("x");
            Piecewise::Piece negative;                       // bare catch-all
            negative.expression = Expression::variable("x", 1.0, -1.0);
            absF.pieces.push_back(std::move(positive));
            absF.pieces.push_back(std::move(negative));
        }
        witness.setPosition(glm::vec3(-3.0f, 0.0f, 0.0f));
        assert(neard(*absF.evaluate({{"x", -3.0}}, &witness), 3.0));
        witness.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));
        assert(neard(*absF.evaluate({{"x", 4.0}}, &witness), 4.0));

        // min(f, g) with f = x², g = 2x + 3: where f - g <= 0 use f, else g.
        Piecewise minF;
        {
            Expression f = Expression::variable("x", 2.0);
            Expression g = Expression::variable("x", 1.0, 2.0).plus(
                Expression::constant(3.0));
            Expression fMinusG = f.plus(g.scaled(-1.0));
            Piecewise::Piece useF;
            useF.guard = guardLEZero(fMinusG);
            useF.expression = f;
            Piecewise::Piece useG;                           // bare catch-all
            useG.expression = g;
            minF.pieces.push_back(std::move(useF));
            minF.pieces.push_back(std::move(useG));
        }
        witness.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        assert(neard(*minF.evaluate({{"x", 0.0}}, &witness), 0.0));   // f wins
        witness.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
        assert(neard(*minF.evaluate({{"x", 5.0}}, &witness), 13.0)); // g wins

        // Without a subject, a guard is UNPROVEN: only unguarded pieces can
        // testify; a fully guarded function is undefined, never guessed.
        Piecewise onlyGuarded;
        {
            Piecewise::Piece lone;
            lone.guard = guardLEZero(Expression::variable("x"));
            lone.expression = Expression::constant(1.0);
            onlyGuarded.pieces.push_back(std::move(lone));
        }
        assert(!onlyGuarded.evaluate({{"x", -5.0}}).has_value());
        witness.setPosition(glm::vec3(-5.0f, 0.0f, 0.0f));
        assert(onlyGuarded.evaluate({{"x", -5.0}}, &witness).has_value());

        // Guards are law-text: they survive serialization with the function.
        Piecewise rebornAbs = Piecewise::fromJson(absF.toJson());
        witness.setPosition(glm::vec3(-7.0f, 0.0f, 0.0f));
        assert(neard(*rebornAbs.evaluate({{"x", -7.0}}, &witness), 7.0));

        // And they run inside a real law: y := |x| via a Map action.
        Law absLaw("y-becomes-abs-x");
        absLaw.addAuthor(author);
        absLaw.setActionModel(ActionNode::map("position.y", absF, xBind));
        Object dipper;
        dipper.setPosition(glm::vec3(-6.0f, 0.0f, 0.0f));
        assert(absLaw.applyTo(dipper) == Law::ApplicationResult::Applied);
        assert(nearf(dipper.getPosition().y, 6.0f));         // |−6|, legislated

        // ------------------------------------------------------------------
        // 11. NAMED FUNCTIONS — createTerm's recursion made durable. Define
        //     once, call from any piece; iteration is carried through the
        //     ARGUMENTS, so primitive recursion is expressible; divergence
        //     meets the call-depth ceiling with an honest nullopt.
        // ------------------------------------------------------------------
        using OntoMath::FunctionCall;
        using OntoMath::FunctionDef;
        using OntoMath::FunctionRegistry;
        auto& registry = FunctionRegistry::instance();

        // double(x) = 2x — and a caller composing double(x² + 1).
        FunctionDef doubler;
        doubler.name = "double";
        doubler.params = {"x"};
        doubler.body = Piecewise::continuous(Expression::variable("x", 1.0, 2.0));
        registry.define(doubler);

        Piecewise composed;
        {
            Piecewise::Piece piece;
            piece.call = std::make_shared<FunctionCall>();
            piece.call->function = "double";
            piece.call->args = {Expression::variable("x", 2.0).plus(
                Expression::constant(1.0))};                 // x² + 1
            composed.pieces.push_back(std::move(piece));
        }
        assert(neard(*composed.evaluate({{"x", 3.0}}), 20.0));   // 2·(9+1)

        // iter(x, n): n <= 0 -> x; else iter(2x, n - 1) — recursion with
        // the state carried through the arguments: iter(3, 4) = 3·2⁴ = 48.
        FunctionDef iter;
        iter.name = "iter";
        iter.params = {"x", "n"};
        iter.body.inputVariable = "n";
        {
            Piecewise::Piece base;                            // n <= 0 -> x
            base.hasHi = true;
            base.hi = 0.0;
            base.includeHi = true;
            base.expression = Expression::variable("x");
            Piecewise::Piece step;                            // else recurse
            step.call = std::make_shared<FunctionCall>();
            step.call->function = "iter";
            step.call->args = {Expression::variable("x", 1.0, 2.0),      // 2x
                               Expression::variable("n").plus(
                                   Expression::constant(-1.0))};         // n-1
            iter.body.pieces.push_back(std::move(base));
            iter.body.pieces.push_back(std::move(step));
        }
        registry.define(iter);

        Piecewise callIter;
        {
            Piecewise::Piece piece;
            piece.call = std::make_shared<FunctionCall>();
            piece.call->function = "iter";
            piece.call->args = {Expression::variable("x"), Expression::constant(4.0)};
            callIter.pieces.push_back(std::move(piece));
        }
        assert(neard(*callIter.evaluate({{"x", 3.0}}), 48.0));

        // Divergence is honest: a function with no base case hits the
        // anti-Babel depth ceiling and answers NOTHING.
        FunctionDef forever;
        forever.name = "forever";
        forever.params = {"x"};
        {
            Piecewise::Piece loop;
            loop.call = std::make_shared<FunctionCall>();
            loop.call->function = "forever";
            loop.call->args = {Expression::variable("x")};
            forever.body.pieces.push_back(std::move(loop));
        }
        registry.define(forever);
        Piecewise callForever;
        {
            Piecewise::Piece piece;
            piece.call = std::make_shared<FunctionCall>();
            piece.call->function = "forever";
            piece.call->args = {Expression::variable("x")};
            callForever.pieces.push_back(std::move(piece));
        }
        assert(!callForever.evaluate({{"x", 1.0}}).has_value());

        // Unknown words and wrong arity are refusals, not guesses.
        Piecewise callGhost;
        {
            Piecewise::Piece piece;
            piece.call = std::make_shared<FunctionCall>();
            piece.call->function = "no-such-function";
            piece.call->args = {Expression::variable("x")};
            callGhost.pieces.push_back(std::move(piece));
        }
        assert(!callGhost.evaluate({{"x", 1.0}}).has_value());
        Piecewise wrongArity;
        {
            Piecewise::Piece piece;
            piece.call = std::make_shared<FunctionCall>();
            piece.call->function = "iter";
            piece.call->args = {Expression::variable("x")};   // iter wants 2
            wrongArity.pieces.push_back(std::move(piece));
        }
        assert(!wrongArity.evaluate({{"x", 1.0}}).has_value());

        // The whole vocabulary survives serialization: registry AND call.
        const auto registryJson = registry.toJson();
        registry.loadFromJson(nlohmann::json::object());
        assert(registry.getAll().empty());
        registry.loadFromJson(registryJson);
        assert(registry.find("iter") != nullptr);
        Piecewise rebornCall = Piecewise::fromJson(callIter.toJson());
        assert(neard(*rebornCall.evaluate({{"x", 3.0}}), 48.0));

        // And in a real law: y := iter(x, 4).
        Law iterLaw("y-becomes-iterated-x");
        iterLaw.addAuthor(author);
        iterLaw.setActionModel(ActionNode::map("position.y", callIter, xBind));
        Object grower;
        grower.setPosition(glm::vec3(2.0f, 0.0f, 0.0f));
        assert(iterLaw.applyTo(grower) == Law::ApplicationResult::Applied);
        assert(nearf(grower.getPosition().y, 32.0f));         // 2·2⁴, legislated

        // ------------------------------------------------------------------
        // 12. PURE GUARDS — local mathematics gating local mathematics:
        //     "applies where g(variables) <= 0", no subject needed. This
        //     closes the known gap: recursion base cases over PARAMETERS,
        //     and with it the manifesto's Mandelbrot ambition (real slice).
        // ------------------------------------------------------------------
        // abs(x), subjectless this time: where -x <= 0 use x; where x <= 0
        // use -x. No world, no witness — just the variables.
        Piecewise pureAbs;
        {
            Piecewise::Piece positive;
            positive.whereLEZero = std::make_shared<Expression>(
                Expression::variable("x", 1.0, -1.0));            // -x <= 0
            positive.expression = Expression::variable("x");
            Piecewise::Piece negative;
            negative.whereLEZero = std::make_shared<Expression>(
                Expression::variable("x"));                       // x <= 0
            negative.expression = Expression::variable("x", 1.0, -1.0);
            pureAbs.pieces.push_back(std::move(positive));
            pureAbs.pieces.push_back(std::move(negative));
        }
        assert(neard(*pureAbs.evaluate({{"x", -9.0}}), 9.0));     // NO subject
        assert(neard(*pureAbs.evaluate({{"x", 2.5}}), 2.5));

        // Escape-time on the real axis of the Mandelbrot recurrence
        // x <- x² + c:  mand(x, c, n) =
        //   where 2 - x <= 0        -> n   (escaped; remaining budget)
        //   where n <= 0            -> 0   (never escaped: in the set)
        //   otherwise               -> mand(x² + c, c, n - 1)
        FunctionDef mand;
        mand.name = "mand";
        mand.params = {"x", "c", "n"};
        {
            Piecewise::Piece escaped;
            escaped.whereLEZero = std::make_shared<Expression>(
                Expression::constant(2.0).plus(
                    Expression::variable("x", 1.0, -1.0)));       // 2 - x <= 0
            escaped.expression = Expression::variable("n");
            Piecewise::Piece inTheSet;
            inTheSet.whereLEZero = std::make_shared<Expression>(
                Expression::variable("n"));                       // n <= 0
            inTheSet.expression = Expression::constant(0.0);
            Piecewise::Piece iterate;
            iterate.call = std::make_shared<FunctionCall>();
            iterate.call->function = "mand";
            iterate.call->args = {
                Expression::variable("x", 2.0).plus(Expression::variable("c")),
                Expression::variable("c"),
                Expression::variable("n").plus(Expression::constant(-1.0))};
            mand.body.pieces.push_back(std::move(escaped));
            mand.body.pieces.push_back(std::move(inTheSet));
            mand.body.pieces.push_back(std::move(iterate));
        }
        registry.define(mand);

        Piecewise orbit;
        {
            Piecewise::Piece seed;
            seed.call = std::make_shared<FunctionCall>();
            seed.call->function = "mand";
            seed.call->args = {Expression::constant(0.0), Expression::variable("c"),
                               Expression::constant(8.0)};
            orbit.pieces.push_back(std::move(seed));
        }
        // c = 1: 0 -> 1 -> 2, escapes with 6 of 8 iterations unspent.
        assert(neard(*orbit.evaluate({{"c", 1.0}}), 6.0));
        // c = -0.5: the orbit stays bounded — in the set, honestly 0.
        assert(neard(*orbit.evaluate({{"c", -0.5}}), 0.0));

        // The pure guard survives serialization with the function.
        Piecewise rebornPureAbs = Piecewise::fromJson(pureAbs.toJson());
        assert(neard(*rebornPureAbs.evaluate({{"x", -4.0}}), 4.0));

        registry.loadFromJson(nlohmann::json::object());      // leave it clean
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("ontomath_test: ALL OK");
    return 0;
}
