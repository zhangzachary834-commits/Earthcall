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
        // ∫ x⁻¹ is ln|x| — not in this algebra: honest refusal, not approximation.
        assert(!Expression::variable("x", -1.0).antiderivative("x").has_value());

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
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("ontomath_test: ALL OK");
    return 0;
}
