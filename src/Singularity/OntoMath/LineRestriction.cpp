#include "Singularity/OntoMath/LineRestriction.hpp"

#include <cmath>
#include <sstream>

// Exact line restriction over OntoMath's own signomial algebra. Every algebraic
// step below is ScalarForm::plus / times / scaled / normalized — the same exact
// operations toScalarForm uses — so this file adds a substitution walk, not a
// second algebra. See LineRestriction.hpp for the admitted subset and the
// floating-point contract.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
namespace OntoMath {
namespace {

void say(std::string* why, const std::string& reason) {
    if (why && why->empty()) *why = reason;
}

bool withinBudget(const ScalarForm& form, std::string* why) {
    if (form.terms.size() <= kMaxLineTerms) return true;
    say(why, "expansion exceeds the line-restriction term budget (" +
                 std::to_string(kMaxLineTerms) + " terms)");
    return false;
}

// A pure constant: no term reads any symbol. Normalized forms hold at most one
// such term; an empty form is the constant 0.
std::optional<double> pureConstant(const ScalarForm& form) {
    double value = 0.0;
    for (const auto& term : form.terms) {
        if (!term.factors.empty() || !term.trans.empty()) return std::nullopt;
        value += term.coefficient;
    }
    return value;
}

// A non-negative integer exponent within the degree budget, or refusal.
std::optional<int> admittedPower(double exponent, const std::string& what, std::string* why) {
    if (!std::isfinite(exponent) || std::floor(exponent) != exponent || exponent < 0.0) {
        say(why, what + " carries exponent " + std::to_string(exponent) +
                     "; only non-negative integer powers of a line-bound coordinate stay polynomial");
        return std::nullopt;
    }
    if (exponent > kMaxLineDegree) {
        say(why, what + " exceeds the line-restriction degree budget (" +
                     std::to_string(kMaxLineDegree) + ")");
        return std::nullopt;
    }
    return static_cast<int>(exponent);
}

std::optional<ScalarForm> power(const ScalarForm& base, int n, std::string* why) {
    ScalarForm acc = ScalarForm::constant(1.0);
    for (int i = 0; i < n; ++i) {
        acc = acc.times(base);
        if (!withinBudget(acc, why)) return std::nullopt;
    }
    return acc.normalized();
}

bool isReserved(const ScalarSubstitution& subst, const std::string& name) {
    (void)subst;
    if (name == kLineParameterVar) return true;
    for (int i = 0; i < 3; ++i) {
        if (name == kLineOriginVars[i] || name == kLineDirectionVars[i]) return true;
    }
    return false;
}

// One flattened value: a scalar form or three component forms.
struct Flat {
    bool vector = false;
    ScalarForm scalar;
    std::array<ScalarForm, 3> components;
};

int axisIndex(const std::string& axis) {
    if (axis == "x") return 0;
    if (axis == "y") return 1;
    if (axis == "z") return 2;
    return -1;
}

std::optional<Flat> flatten(const MathNode& node, const ScalarSubstitution& subst, std::string* why);

std::optional<Flat> child(const MathNode& node, std::size_t i, const ScalarSubstitution& subst,
                          std::string* why) {
    if (i >= node.children.size() || !node.children[i]) {
        say(why, std::string(mathOpName(node.op)) + " has a missing operand");
        return std::nullopt;
    }
    return flatten(*node.children[i], subst, why);
}

bool exactArity(const MathNode& node, std::size_t n, std::string* why) {
    // Mirrors MathNode::evaluate, the denotational authority: Add/Sub/Scale
    // with any other operand count are undefined there, so they are not
    // admitted here even though toScalarForm accepts n-ary Add.
    if (node.children.size() == n) return true;
    say(why, std::string(mathOpName(node.op)) + " needs exactly " + std::to_string(n) +
                 " operands, got " + std::to_string(node.children.size()));
    return false;
}

Flat scalarFlat(ScalarForm s) {
    Flat f;
    f.scalar = std::move(s);
    return f;
}

Flat vectorFlat(std::array<ScalarForm, 3> v) {
    Flat f;
    f.vector = true;
    f.components = std::move(v);
    return f;
}

std::optional<Flat> flatten(const MathNode& node, const ScalarSubstitution& subst, std::string* why) {
    using Op = MathNode::Op;
    switch (node.op) {
        case Op::ScalarLeaf: {
            const auto s = substitute(node.scalarForm, subst, why);
            if (!s) return std::nullopt;
            return scalarFlat(*s);
        }
        case Op::ValueLeaf: {
            const std::string& name = node.variableName;
            if (name.empty()) { say(why, "ValueLeaf with no variable name"); return std::nullopt; }
            if (isReserved(subst, name)) {
                say(why, "authored mathematics already reads reserved line symbol '" + name + "'");
                return std::nullopt;
            }
            if (auto it = subst.vectors.find(name); it != subst.vectors.end()) return vectorFlat(it->second);
            if (auto it = subst.scalars.find(name); it != subst.scalars.end()) return scalarFlat(it->second);
            return scalarFlat(ScalarForm::variable(name));
        }
        case Op::VectorConstruct: {
            if (!exactArity(node, 3, why)) return std::nullopt;
            std::array<ScalarForm, 3> v;
            for (std::size_t i = 0; i < 3; ++i) {
                const auto c = child(node, i, subst, why);
                if (!c) return std::nullopt;
                if (c->vector) { say(why, "VectorConstruct component is a vector"); return std::nullopt; }
                v[i] = c->scalar;
            }
            return vectorFlat(std::move(v));
        }
        case Op::Component: {
            if (!exactArity(node, 1, why)) return std::nullopt;
            const int axis = axisIndex(node.stringArg);
            if (axis < 0) { say(why, "Component names axis '" + node.stringArg + "'"); return std::nullopt; }
            const auto c = child(node, 0, subst, why);
            if (!c) return std::nullopt;
            if (!c->vector) { say(why, "Component of a non-vector operand"); return std::nullopt; }
            return scalarFlat(c->components[axis]);
        }
        case Op::Add:
        case Op::Sub: {
            if (!exactArity(node, 2, why)) return std::nullopt;
            const auto a = child(node, 0, subst, why);
            if (!a) return std::nullopt;
            const auto b = child(node, 1, subst, why);
            if (!b) return std::nullopt;
            if (a->vector != b->vector) {
                say(why, std::string(mathOpName(node.op)) + " of a vector and a scalar is undefined");
                return std::nullopt;
            }
            const double sign = node.op == Op::Add ? 1.0 : -1.0;
            if (!a->vector) {
                auto s = a->scalar.plus(b->scalar.scaled(sign));
                if (!withinBudget(s, why)) return std::nullopt;
                return scalarFlat(std::move(s));
            }
            std::array<ScalarForm, 3> v;
            for (int i = 0; i < 3; ++i) {
                v[i] = a->components[i].plus(b->components[i].scaled(sign));
                if (!withinBudget(v[i], why)) return std::nullopt;
            }
            return vectorFlat(std::move(v));
        }
        case Op::Scale: {
            if (!exactArity(node, 2, why)) return std::nullopt;
            const auto a = child(node, 0, subst, why);
            if (!a) return std::nullopt;
            const auto b = child(node, 1, subst, why);
            if (!b) return std::nullopt;
            if (a->vector && b->vector) {
                say(why, "Scale of two vectors is undefined (Hadamard is the componentwise product)");
                return std::nullopt;
            }
            if (!a->vector && !b->vector) {
                auto s = a->scalar.times(b->scalar);
                if (!withinBudget(s, why)) return std::nullopt;
                return scalarFlat(std::move(s));
            }
            const ScalarForm& k = a->vector ? b->scalar : a->scalar;
            const auto& vec = a->vector ? a->components : b->components;
            std::array<ScalarForm, 3> v;
            for (int i = 0; i < 3; ++i) {
                v[i] = vec[i].times(k);
                if (!withinBudget(v[i], why)) return std::nullopt;
            }
            return vectorFlat(std::move(v));
        }
        case Op::Hadamard:
        case Op::Dot:
        case Op::Cross: {
            if (!exactArity(node, 2, why)) return std::nullopt;
            const auto a = child(node, 0, subst, why);
            if (!a) return std::nullopt;
            const auto b = child(node, 1, subst, why);
            if (!b) return std::nullopt;
            if (!a->vector || !b->vector) {
                say(why, std::string(mathOpName(node.op)) + " needs two vector operands");
                return std::nullopt;
            }
            const auto& u = a->components;
            const auto& w = b->components;
            if (node.op == Op::Dot) {
                auto s = u[0].times(w[0]).plus(u[1].times(w[1])).plus(u[2].times(w[2]));
                if (!withinBudget(s, why)) return std::nullopt;
                return scalarFlat(std::move(s));
            }
            std::array<ScalarForm, 3> v;
            if (node.op == Op::Hadamard) {
                for (int i = 0; i < 3; ++i) v[i] = u[i].times(w[i]);
            } else {
                v[0] = u[1].times(w[2]).plus(u[2].times(w[1]).scaled(-1.0));
                v[1] = u[2].times(w[0]).plus(u[0].times(w[2]).scaled(-1.0));
                v[2] = u[0].times(w[1]).plus(u[1].times(w[0]).scaled(-1.0));
            }
            for (const auto& c : v) if (!withinBudget(c, why)) return std::nullopt;
            return vectorFlat(std::move(v));
        }
        case Op::Div: {
            if (!exactArity(node, 2, why)) return std::nullopt;
            const auto a = child(node, 0, subst, why);
            if (!a) return std::nullopt;
            const auto b = child(node, 1, subst, why);
            if (!b) return std::nullopt;
            if (b->vector) {
                say(why, "Div by a vector is componentwise-guarded; not a polynomial");
                return std::nullopt;
            }
            const auto c = pureConstant(b->scalar);
            if (!c) {
                say(why, "Div by a non-constant operand leaves the polynomial algebra");
                return std::nullopt;
            }
            // MathNode::evaluate answers 0 below the degenerate threshold. That
            // is an authored guard, not a reciprocal; refuse rather than encode it.
            if (!(std::abs(*c) >= kDegenerateDivisor) || !std::isfinite(*c)) {
                say(why, "Div by a degenerate or non-finite constant");
                return std::nullopt;
            }
            const double k = 1.0 / *c;
            if (!a->vector) return scalarFlat(a->scalar.scaled(k));
            return vectorFlat({a->components[0].scaled(k), a->components[1].scaled(k),
                               a->components[2].scaled(k)});
        }
        case Op::Pow: {
            if (!exactArity(node, 2, why)) return std::nullopt;
            const auto a = child(node, 0, subst, why);
            if (!a) return std::nullopt;
            const auto b = child(node, 1, subst, why);
            if (!b) return std::nullopt;
            if (a->vector || b->vector) { say(why, "componentwise vector Pow is not admitted"); return std::nullopt; }
            const auto e = pureConstant(b->scalar);
            if (!e) { say(why, "Pow with a non-constant exponent"); return std::nullopt; }
            const auto n = admittedPower(*e, "Pow", why);
            if (!n) return std::nullopt;
            auto s = power(a->scalar, *n, why);
            if (!s) return std::nullopt;
            return scalarFlat(std::move(*s));
        }
        default:
            say(why, std::string("no exact line restriction for ") + mathOpName(node.op));
            return std::nullopt;
    }
}

const MathNode* answeringNode(const Piecewise& model, std::string* why) {
    // Admit only an unconditional first answer: the CPU evaluator and the WGSL
    // emitter then agree on which piece speaks. Bounds become ray boundaries in
    // the Piecewise-segmentation rung; guards/calls/folds are not emitted
    // identically by every execution channel and are refused here.
    if (model.pieces.empty()) { say(why, "Piecewise has no pieces"); return nullptr; }
    const auto& piece = model.pieces.front();
    if (!piece.mathNode) { say(why, "first piece carries no MathNode"); return nullptr; }
    if (piece.hasLo || piece.hasHi) { say(why, "first piece is interval-bounded (Piecewise ray segmentation is a later rung)"); return nullptr; }
    if (piece.guard) { say(why, "first piece carries a world guard"); return nullptr; }
    if (piece.whereLEZero) { say(why, "first piece carries a pure guard"); return nullptr; }
    if (piece.call) { say(why, "first piece is a function call"); return nullptr; }
    if (piece.fold) { say(why, "first piece is a fold over the world"); return nullptr; }
    return piece.mathNode.get();
}

std::optional<LineRestrictedForm> restrictedFrom(const ScalarForm& form, std::string* why) {
    auto powers = collectPowers(form, kLineParameterVar, why);
    if (!powers) return std::nullopt;
    if (static_cast<int>(powers->size()) - 1 > kMaxLineDegree) {
        say(why, "restricted form exceeds the line-restriction degree budget (" +
                     std::to_string(kMaxLineDegree) + ")");
        return std::nullopt;
    }
    LineRestrictedForm out;
    out.coefficients = std::move(*powers);
    for (const auto& c : out.coefficients) {
        for (const auto& term : c.terms) {
            for (const auto& [name, exponent] : term.factors) out.reads.insert(name);
            for (const auto& tf : term.trans) out.reads.insert(tf.variable);
        }
    }
    return out;
}

} // namespace

std::optional<ScalarForm> substitute(const ScalarForm& form, const ScalarSubstitution& subst,
                                     std::string* why) {
    ScalarForm out;
    for (const auto& term : form.terms) {
        Term residual(term.coefficient);
        ScalarForm expanded = ScalarForm::constant(1.0);
        for (const auto& [name, exponent] : term.factors) {
            if (isReserved(subst, name)) {
                say(why, "authored mathematics already reads reserved line symbol '" + name + "'");
                return std::nullopt;
            }
            if (subst.vectors.count(name)) {
                say(why, "vector variable '" + name + "' used as a scalar power factor");
                return std::nullopt;
            }
            auto it = subst.scalars.find(name);
            if (it == subst.scalars.end()) {
                residual.factors[name] = exponent;
                continue;
            }
            const auto n = admittedPower(exponent, "factor " + name, why);
            if (!n) return std::nullopt;
            const auto p = power(it->second, *n, why);
            if (!p) return std::nullopt;
            expanded = expanded.times(*p);
            if (!withinBudget(expanded, why)) return std::nullopt;
        }
        for (const auto& tf : term.trans) {
            if (isReserved(subst, tf.variable)) {
                say(why, "authored mathematics already reads reserved line symbol '" + tf.variable + "'");
                return std::nullopt;
            }
            if (subst.scalars.count(tf.variable) || subst.vectors.count(tf.variable)) {
                // sin/cos/exp/ln of a bound coordinate is not polynomial in s.
                // A per-line affine-argument class is a later rung.
                say(why, "transcendental factor " + tf.print() + " reads a line-bound coordinate");
                return std::nullopt;
            }
            residual.trans.push_back(tf);
        }
        ScalarForm residualForm;
        residualForm.terms.push_back(residual);
        out = out.plus(expanded.times(residualForm));
        if (!withinBudget(out, why)) return std::nullopt;
    }
    return out.normalized();
}

std::optional<ScalarForm> toScalarFormUnder(const MathNode& node, const ScalarSubstitution& subst,
                                            std::string* why) {
    const auto f = flatten(node, subst, why);
    if (!f) return std::nullopt;
    if (f->vector) { say(why, "expression is vector-valued where a scalar was required"); return std::nullopt; }
    return f->scalar;
}

std::optional<std::array<ScalarForm, 3>> toVectorFormUnder(const MathNode& node,
                                                           const ScalarSubstitution& subst,
                                                           std::string* why) {
    const auto f = flatten(node, subst, why);
    if (!f) return std::nullopt;
    if (!f->vector) { say(why, "expression is scalar-valued where a vector was required"); return std::nullopt; }
    return f->components;
}

std::optional<std::vector<ScalarForm>> collectPowers(const ScalarForm& form,
                                                     const std::string& parameter,
                                                     std::string* why) {
    std::vector<ScalarForm> out;
    for (const auto& term : form.terms) {
        for (const auto& tf : term.trans) {
            if (tf.variable == parameter) {
                say(why, "parameter '" + parameter + "' appears inside " + tf.print());
                return std::nullopt;
            }
        }
        Term rest = term;
        int k = 0;
        if (auto it = rest.factors.find(parameter); it != rest.factors.end()) {
            const auto n = admittedPower(it->second, "parameter " + parameter, why);
            if (!n) return std::nullopt;
            k = *n;
            rest.factors.erase(it);
        }
        if (static_cast<int>(out.size()) <= k) out.resize(k + 1);
        out[k].terms.push_back(std::move(rest));
    }
    for (auto& c : out) c = c.normalized();
    while (!out.empty() && out.back().terms.empty()) out.pop_back();
    return out;
}

ScalarSubstitution linePointSubstitution(const std::string& pointVariable) {
    ScalarSubstitution subst;
    std::array<ScalarForm, 3> point;
    static const char* const kAxes[3] = {"x", "y", "z"};
    for (int i = 0; i < 3; ++i) {
        // o_i + s·d_i, as an exact two-term form.
        point[i] = ScalarForm::variable(kLineOriginVars[i]).plus(
            ScalarForm::variable(kLineParameterVar).times(ScalarForm::variable(kLineDirectionVars[i])));
        subst.scalars[kAxes[i]] = point[i];
    }
    subst.vectors[pointVariable] = point;
    return subst;
}

LineRestriction restrictToLine(const MathNode& node, const std::string& pointVariable) {
    LineRestriction result;
    const auto form = toScalarFormUnder(node, linePointSubstitution(pointVariable), &result.refusal);
    if (!form) return result;
    result.form = restrictedFrom(*form, &result.refusal);
    if (result.form) result.refusal.clear();
    return result;
}

LineRestriction restrictToLine(const Piecewise& model, const std::string& pointVariable) {
    LineRestriction result;
    const MathNode* node = answeringNode(model, &result.refusal);
    if (!node) return result;
    return restrictToLine(*node, pointVariable);
}

VectorLineRestriction restrictVectorToLine(const MathNode& node, const std::string& pointVariable) {
    VectorLineRestriction result;
    const auto v = toVectorFormUnder(node, linePointSubstitution(pointVariable), &result.refusal);
    if (!v) return result;
    std::array<LineRestrictedForm, 3> forms;
    for (int i = 0; i < 3; ++i) {
        auto r = restrictedFrom((*v)[i], &result.refusal);
        if (!r) return result;
        forms[i] = std::move(*r);
    }
    result.form = std::move(forms);
    result.refusal.clear();
    return result;
}

VectorLineRestriction restrictVectorToLine(const Piecewise& model, const std::string& pointVariable) {
    VectorLineRestriction result;
    const MathNode* node = answeringNode(model, &result.refusal);
    if (!node) return result;
    return restrictVectorToLine(*node, pointVariable);
}

int LineRestrictedForm::exactDegreeOn(const std::map<std::string, double>& bindings) const {
    for (int k = degree(); k >= 0; --k) {
        bool vanishes = true;
        for (const auto& term : coefficients[k].terms) {
            bool termZero = term.coefficient == 0.0;
            for (const auto& [name, exponent] : term.factors) {
                if (termZero) break;
                auto it = bindings.find(name);
                // 0^e is exactly 0 only for e > 0 (e < 0 is a pole, e == 0 is 1).
                if (it != bindings.end() && it->second == 0.0 && exponent > 0.0) termZero = true;
            }
            if (!termZero) { vanishes = false; break; }
        }
        if (!vanishes) return k;
    }
    return -1;
}

std::optional<std::vector<double>> LineRestrictedForm::evaluateCoefficients(
        const std::map<std::string, double>& bindings) const {
    std::vector<double> a;
    a.reserve(coefficients.size());
    for (const auto& c : coefficients) {
        const auto v = c.evaluate(bindings);
        if (!v || !std::isfinite(*v)) return std::nullopt;
        a.push_back(*v);
    }
    return a;
}

std::optional<double> LineRestrictedForm::magnitudeBound(const std::map<std::string, double>& bindings,
                                                         double s) const {
    double total = 0.0;
    double sk = 1.0;
    for (const auto& c : coefficients) {
        for (const auto& term : c.terms) {
            const auto v = term.evaluate(bindings);
            if (!v || !std::isfinite(*v)) return std::nullopt;
            total += std::abs(*v) * sk;
        }
        sk *= std::abs(s);
    }
    return total;
}

double LineRestrictedForm::evaluatePolynomial(const std::vector<double>& a, double s) {
    double acc = 0.0;
    for (auto it = a.rbegin(); it != a.rend(); ++it) acc = acc * s + *it;
    return acc;
}

std::string LineRestrictedForm::print() const {
    if (coefficients.empty()) return "0";
    std::ostringstream out;
    for (std::size_t k = 0; k < coefficients.size(); ++k) {
        if (k) out << " + ";
        out << "(" << (coefficients[k].empty() ? std::string("0") : coefficients[k].print()) << ")";
        if (k == 1) out << "·s";
        else if (k > 1) out << "·s^" << k;
    }
    return out.str();
}

} // namespace OntoMath
