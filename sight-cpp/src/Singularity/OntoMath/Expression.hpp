#pragma once

#include "json.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

// ============================================================================
// OntoMath — the substrate's own mathematics.
//
// Earthcall is driven by math in the program: laws check membership in the
// satisfaction zone of an authored function, and govern change by authored
// functions of authored inputs. This is the exact symbolic core those laws
// stand on — plain data (terms, coefficients, exponents, bounds), every
// primitive Person-modifiable, serializable like all law text, evaluated on
// demand.
//
// Representation: an Expression is a sum of Terms; a Term is
//     coefficient * Π variable_i ^ exponent_i        (real exponents)
// — the multivariate signomial algebra. Within it, algebra (+, ×, combine
// like terms) and calculus (∂/∂x, ∫dx by the power rule) are EXACT, not
// approximated by hand — the manifesto's "reasoned about by mathematical
// structure".
//
// Growth path (the spec deliberately continues): expression-valued exponents
// (createTerm's "enter a number, or a new term inside the exponential space
// itself"), transcendental factors, symbolic simplification beyond like
// terms, and the graph/relation-valued conditions of the manifesto's law
// calculus. Extend here — the ECA wiring above this does not change.
// ============================================================================
namespace OntoMath {

// One product term: coefficient * Π var^exp. Factors are kept in a sorted
// map so like-term comparison and printing are canonical.
struct Term {
    double coefficient = 0.0;
    std::map<std::string, double> factors;   // variable name -> real exponent

    Term() = default;
    Term(double c, std::map<std::string, double> f = {})
        : coefficient(c), factors(std::move(f)) {}

    // Strict evaluation: every variable in the term must be bound — a law
    // must never fire on an unvalued symbol.
    std::optional<double> evaluate(const std::map<std::string, double>& vars) const;

    Term times(const Term& other) const;     // coefficients multiply, exponents add
    bool sameShape(const Term& other) const { return factors == other.factors; }

    std::string print() const;
    nlohmann::json toJson() const;
    static Term fromJson(const nlohmann::json& j);
};

// A sum of terms — the ContinuousExpression. Multivariate; polynomials are
// the natural-exponent special case.
struct Expression {
    std::vector<Term> terms;

    static Expression constant(double c);
    static Expression variable(const std::string& name, double exponent = 1.0,
                               double coefficient = 1.0);

    std::optional<double> evaluate(const std::map<std::string, double>& vars) const;

    // Exact algebra.
    Expression plus(const Expression& other) const;
    Expression times(const Expression& other) const;   // distributes term products
    Expression scaled(double k) const;
    // Combine like terms, drop zeros, order canonically (descending total
    // degree, then factor signature).
    Expression normalized() const;

    // Exact calculus by the power rule — every term in this representation
    // is differentiable; integration fails only on the x^-1 term (its
    // antiderivative, ln|x|, does not live in this algebra yet), reported
    // honestly as nullopt rather than approximated.
    Expression derivative(const std::string& var) const;
    std::optional<Expression> antiderivative(const std::string& var) const;

    bool empty() const { return terms.empty(); }
    std::string print() const;
    nlohmann::json toJson() const;
    static Expression fromJson(const nlohmann::json& j);
};

// The manifesto's DiscreteFunctions: discrete bounds within which a
// continuous expression governs. Bounds are on one designated input variable
// and may be open or closed on each side — mathematically precise piecewise
// definition. Outside every piece the function is UNDEFINED (nullopt), never
// silently zero: discontinuity is honored, not papered over.
struct Piecewise {
    struct Piece {
        bool hasLo = false, hasHi = false;
        double lo = 0.0, hi = 0.0;
        bool includeLo = true, includeHi = true;
        Expression expression;

        bool contains(double x) const;
        nlohmann::json toJson() const;
        static Piece fromJson(const nlohmann::json& j);
    };

    std::string inputVariable = "x";   // the variable the bounds cut
    std::vector<Piece> pieces;

    // A single everywhere-defined piece — the purely continuous case.
    static Piecewise continuous(Expression e);

    std::optional<double> evaluate(const std::map<std::string, double>& vars) const;

    std::string print() const;
    nlohmann::json toJson() const;
    static Piecewise fromJson(const nlohmann::json& j);
};

} // namespace OntoMath
