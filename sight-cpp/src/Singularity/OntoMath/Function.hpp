#pragma once
#include <cmath>
#include <string>
#include <utility>
#include <vector>

// OntoMath symbolic function primitives. A Function is a sum of Terms; each
// Term is coefficient * (product of variables) ^ exponent. This is the seed
// that grows into CurveModel (Constant / Polynomial / Sinusoid) for Law Drive
// actions — see LAW_AND_CREATION_SYSTEM.md §2c.
class Function {
public:
    struct Variable {
        std::string name;
        double value = 0.0;
    };

    struct Term {
        double coefficient = 0.0;      // the number out front
        std::vector<Variable> variables; // which variables, e.g. "x"
        double exponent = 1.0;         // the power, e.g. 3 for x³
        // expand later for even higher order operations

        Term() = default;
        Term(double c, std::vector<Variable> v, double e)
            : coefficient(c), variables(std::move(v)), exponent(e) {}

        // coefficient * value^exponent
        double evaluate(double valueOfVariable) const {
            return coefficient * std::pow(valueOfVariable, exponent);
        }

        std::string print() const {
            std::string vars;
            for (const auto& v : variables) vars += v.name;
            return std::to_string(coefficient) + vars + "^" + std::to_string(exponent);
        }
    };

    std::vector<Term> terms;

    // Sum of all terms evaluated at x.
    double evaluate(double x) const {
        double total = 0.0;
        for (const auto& t : terms) total += t.evaluate(x);
        return total;
    }
};
