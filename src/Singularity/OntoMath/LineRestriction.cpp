#include "Singularity/OntoMath/LineRestriction.hpp"

#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
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
            if (!literalConstant(*node.children[1])) {
                say(why, "Div by a computed constant (only a literal divisor reads the same number on host and device)");
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
            if (!literalConstant(*node.children[1])) {
                say(why, "Pow with a computed exponent (only a literal reads the same number on host and device)");
                return std::nullopt;
            }
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

// ============================================================================
// Rung 2 — piecewise-algebraic restriction, ray-tier evaluation, segmentation.
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
// ============================================================================
namespace {

using Kind = LineAlgebraicNode::Kind;

struct AlgValue {
    bool vector = false;
    uint32_t s = 0;
    uint32_t v[3] = {0, 0, 0};
};

// Builds a LineAlgebraicForm bottom-up. Every subtree first tries the Rung 0
// polynomial flattening (exact, cheap, closed under + − ×); only a subtree
// that genuinely leaves the polynomial class becomes an algebraic node.
struct AlgebraicBuilder {
    ScalarSubstitution subst;
    std::string pointVariable;
    LineAlgebraicForm form;
    std::string why;

    uint32_t push(LineAlgebraicNode n) {
        form.nodes.push_back(std::move(n));
        return static_cast<uint32_t>(form.nodes.size() - 1);
    }
    std::optional<uint32_t> polyLeaf(const ScalarForm& f) {
        auto r = restrictedFrom(f, &why);
        if (!r) return std::nullopt;
        LineAlgebraicNode n;
        n.kind = Kind::Polynomial;
        n.polynomial = std::move(*r);
        return push(std::move(n));
    }
    uint32_t constant(double c) {
        const uint32_t n = *polyLeaf(ScalarForm::constant(c));
        provenanceForm(n, ScalarForm::constant(c));
        return n;
    }
    void provenance(uint32_t node, const MathNode* origin, int component) {
        form.nodes[node].provenance = origin;
        form.nodes[node].provenanceComponent = static_cast<int8_t>(component);
    }
    void provenanceForm(uint32_t node, const ScalarForm& authored) {
        form.nodes[node].provenanceForm = std::make_shared<const ScalarForm>(authored);
    }
    uint32_t op(Kind k, uint32_t a, uint32_t b = 0) {
        LineAlgebraicNode n;
        n.kind = k;
        n.a = a;
        n.b = b;
        return push(std::move(n));
    }
    uint32_t sqrtNonnegative(uint32_t a) {
        const uint32_t n = op(Kind::Sqrt, a);
        form.nodes[n].nonnegativeArgument = true;
        return n;
    }

    std::optional<AlgValue> fromFlat(const Flat& f, const MathNode& origin) {
        AlgValue out;
        out.vector = f.vector;
        if (!f.vector) {
            auto leaf = polyLeaf(f.scalar);
            if (!leaf) return std::nullopt;
            out.s = *leaf;
            provenance(out.s, &origin, -1);
            return out;
        }
        for (int i = 0; i < 3; ++i) {
            auto leaf = polyLeaf(f.components[i]);
            if (!leaf) return std::nullopt;
            out.v[i] = *leaf;
            provenance(out.v[i], &origin, i);
        }
        return out;
    }

    // A refusal that must not be routed around by the algebraic path.
    static bool hardRefusal(const std::string& reason) {
        return reason.find("budget") != std::string::npos ||
               reason.find("reserved line symbol") != std::string::npos;
    }

    // Divisors, exponents and clamp bounds: literals only (Codex P1 finding 2:
    // a computed bound folds exactly on the host and differently in f32).
    std::optional<double> constantOf(const MathNode& n) {
        const auto c = literalConstant(n);
        if (!c) say(&why, std::string("a ") + mathOpName(n.op) + " where a literal constant is required "
                              "(a computed constant folds exactly on the host and differently on the device)");
        return c;
    }

    std::optional<AlgValue> childOf(const MathNode& n, std::size_t i) {
        if (i >= n.children.size() || !n.children[i]) {
            say(&why, std::string(mathOpName(n.op)) + " has a missing operand");
            return std::nullopt;
        }
        return build(*n.children[i]);
    }

    // Sum of squares of three algebraic components, then sqrt (no switch).
    uint32_t euclidean(const uint32_t c[3]) {
        const uint32_t sq0 = op(Kind::Mul, c[0], c[0]);
        const uint32_t sq1 = op(Kind::Mul, c[1], c[1]);
        const uint32_t sq2 = op(Kind::Mul, c[2], c[2]);
        const uint32_t n = sqrtNonnegative(op(Kind::Add, op(Kind::Add, sq0, sq1), sq2));
        setNorm(n, c);
        return n;
    }
    void setNorm(uint32_t n, const uint32_t c[3]) { form.nodes[n].normComponents.assign(c, c + 3); }

    // An authored sum of squares Σ base_i² (Add trees of Pow(base, 2)), so that
    // sqrt of it is recognized as the Euclidean norm it is, however it is spelled.
    bool collectSquares(const MathNode& n, std::vector<const MathNode*>& bases) {
        using Op = MathNode::Op;
        if (n.op == Op::Add && n.children.size() == 2 && n.children[0] && n.children[1])
            return collectSquares(*n.children[0], bases) && collectSquares(*n.children[1], bases);
        if (n.op == Op::Pow && n.children.size() == 2 && n.children[0] && n.children[1]) {
            const auto e = literalConstant(*n.children[1]);
            if (e && *e == 2.0) { bases.push_back(n.children[0].get()); return true; }
        }
        return false;
    }

    std::optional<AlgValue> build(const MathNode& n) {
        std::string polyWhy;
        {
            const auto f = flatten(n, subst, &polyWhy);
            if (f) return fromFlat(*f, n);
            if (hardRefusal(polyWhy)) { say(&why, polyWhy); return std::nullopt; }
        }
        using Op = MathNode::Op;
        // A ValueLeaf is wholly the polynomial path's office: its reason is the truth.
        // A ScalarLeaf may still hold sin/cos/exp of a bound coordinate (Rung 2c).
        if (n.op == Op::ScalarLeaf) return buildScalarLeaf(n.scalarForm);
        if (n.op == Op::ValueLeaf) { say(&why, polyWhy); return std::nullopt; }
        switch (n.op) {
            case Op::Add:
            case Op::Sub:
            case Op::Scale:
            case Op::Hadamard:
            case Op::Dot:
            case Op::Cross:
            case Op::Union:
            case Op::Intersection:
            case Op::Difference: {
                if (!exactArity(n, 2, &why)) return std::nullopt;
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                const auto b = childOf(n, 1);
                if (!b) return std::nullopt;
                AlgValue out;
                if (n.op == Op::Add || n.op == Op::Sub) {
                    if (a->vector != b->vector) { say(&why, std::string(mathOpName(n.op)) + " of a vector and a scalar is undefined"); return std::nullopt; }
                    const Kind k = n.op == Op::Add ? Kind::Add : Kind::Sub;
                    out.vector = a->vector;
                    if (!out.vector) out.s = op(k, a->s, b->s);
                    else for (int i = 0; i < 3; ++i) out.v[i] = op(k, a->v[i], b->v[i]);
                    return out;
                }
                if (n.op == Op::Scale) {
                    if (a->vector && b->vector) { say(&why, "Scale of two vectors is undefined"); return std::nullopt; }
                    if (!a->vector && !b->vector) { out.s = op(Kind::Mul, a->s, b->s); return out; }
                    const uint32_t k = a->vector ? b->s : a->s;
                    const AlgValue& vec = a->vector ? *a : *b;
                    out.vector = true;
                    for (int i = 0; i < 3; ++i) out.v[i] = op(Kind::Mul, vec.v[i], k);
                    return out;
                }
                if (n.op == Op::Hadamard || n.op == Op::Dot || n.op == Op::Cross) {
                    if (!a->vector || !b->vector) { say(&why, std::string(mathOpName(n.op)) + " needs two vector operands"); return std::nullopt; }
                    if (n.op == Op::Dot) {
                        out.s = op(Kind::Add, op(Kind::Add, op(Kind::Mul, a->v[0], b->v[0]), op(Kind::Mul, a->v[1], b->v[1])),
                                   op(Kind::Mul, a->v[2], b->v[2]));
                        return out;
                    }
                    out.vector = true;
                    if (n.op == Op::Hadamard) {
                        for (int i = 0; i < 3; ++i) out.v[i] = op(Kind::Mul, a->v[i], b->v[i]);
                    } else {
                        for (int i = 0; i < 3; ++i) {
                            const int j = (i + 1) % 3, k = (i + 2) % 3;
                            out.v[i] = op(Kind::Sub, op(Kind::Mul, a->v[j], b->v[k]), op(Kind::Mul, a->v[k], b->v[j]));
                        }
                    }
                    return out;
                }
                if (n.op == Op::Difference) {
                    if (a->vector || b->vector) { say(&why, "Difference of vectors is undefined in MathNode::evaluate"); return std::nullopt; }
                    out.s = op(Kind::Max, a->s, op(Kind::Mul, constant(-1.0), b->s));
                    return out;
                }
                // Union = min, Intersection = max; vectors componentwise, scalars broadcast.
                const Kind k = n.op == Op::Union ? Kind::Min : Kind::Max;
                if (!a->vector && !b->vector) {
                    out.s = op(k, a->s, b->s);
                    form.nodes[out.s].sourceA = n.children[0].get();
                    form.nodes[out.s].sourceB = n.children[1].get();
                    return out;
                }
                out.vector = true;
                for (int i = 0; i < 3; ++i) out.v[i] = op(k, a->vector ? a->v[i] : a->s, b->vector ? b->v[i] : b->s);
                return out;
            }
            case Op::Component: {
                if (!exactArity(n, 1, &why)) return std::nullopt;
                const int axis = axisIndex(n.stringArg);
                if (axis < 0) { say(&why, "Component names axis '" + n.stringArg + "'"); return std::nullopt; }
                const auto c = childOf(n, 0);
                if (!c) return std::nullopt;
                if (!c->vector) { say(&why, "Component of a non-vector operand"); return std::nullopt; }
                AlgValue out;
                out.s = c->v[axis];
                return out;
            }
            case Op::VectorConstruct: {
                if (!exactArity(n, 3, &why)) return std::nullopt;
                AlgValue out;
                out.vector = true;
                for (std::size_t i = 0; i < 3; ++i) {
                    const auto c = childOf(n, i);
                    if (!c) return std::nullopt;
                    if (c->vector) { say(&why, "VectorConstruct component is a vector"); return std::nullopt; }
                    out.v[i] = c->s;
                }
                return out;
            }
            case Op::Div: {
                if (!exactArity(n, 2, &why)) return std::nullopt;
                const auto c = n.children[1] ? constantOf(*n.children[1]) : std::nullopt;
                if (!c) { say(&why, "Div by a non-constant operand"); return std::nullopt; }
                if (!(std::abs(*c) >= kDegenerateDivisor) || !std::isfinite(*c)) { say(&why, "Div by a degenerate or non-finite constant"); return std::nullopt; }
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                const uint32_t k = constant(1.0 / *c);
                AlgValue out = *a;
                if (!a->vector) out.s = op(Kind::Mul, a->s, k);
                else for (int i = 0; i < 3; ++i) out.v[i] = op(Kind::Mul, a->v[i], k);
                return out;
            }
            case Op::Pow: {
                if (!exactArity(n, 2, &why)) return std::nullopt;
                const auto e = n.children[1] ? constantOf(*n.children[1]) : std::nullopt;
                if (!e) { say(&why, "Pow with a non-constant exponent"); return std::nullopt; }
                const auto p = admittedPower(*e, "Pow", &why);
                if (!p) return std::nullopt;
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                if (a->vector) { say(&why, "componentwise vector Pow is not admitted"); return std::nullopt; }
                AlgValue out;
                out.s = constant(1.0);
                for (int i = 0; i < *p; ++i) out.s = op(Kind::Mul, out.s, a->s);
                return out;
            }
            case Op::Abs: {
                if (!exactArity(n, 1, &why)) return std::nullopt;
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                AlgValue out = *a;
                if (!a->vector) { out.s = op(Kind::Abs, a->s); form.nodes[out.s].sourceA = n.children[0].get(); }
                else for (int i = 0; i < 3; ++i) out.v[i] = op(Kind::Abs, a->v[i]);
                return out;
            }
            case Op::Sqrt: {
                if (!exactArity(n, 1, &why)) return std::nullopt;
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                if (a->vector) { say(&why, "componentwise vector Sqrt is not admitted"); return std::nullopt; }
                AlgValue out;
                std::vector<const MathNode*> bases;
                if (collectSquares(*n.children[0], bases) && bases.size() >= 2) {
                    // A sum of squares is ≥ 0 in the reals, so sqrt(max(a, 0)) = sqrt(a):
                    // no switch, and its components feed the Cauchy–Schwarz bound.
                    std::vector<uint32_t> comps;
                    for (const auto* base : bases) {
                        const auto c = build(*base);
                        if (!c) return std::nullopt;
                        if (c->vector) { say(&why, "a squared vector inside a sum of squares"); return std::nullopt; }
                        comps.push_back(c->s);
                    }
                    out.s = sqrtNonnegative(a->s);
                    form.nodes[out.s].normComponents = std::move(comps);
                    return out;
                }
                out.s = op(Kind::Sqrt, a->s);
                form.nodes[out.s].sourceA = n.children[0].get();
                return out;
            }
            case Op::Length:
            case Op::Distance: {
                const std::size_t arity = n.op == Op::Length ? 1 : 2;
                if (!exactArity(n, arity, &why)) return std::nullopt;
                // Polynomial operands: one exact polynomial Σ(Δ_i)², then sqrt.
                std::string w;
                std::optional<std::array<ScalarForm, 3>> pa = toVectorFormUnder(*n.children[0], subst, &w);
                std::optional<std::array<ScalarForm, 3>> pb;
                if (arity == 2 && n.children[1]) pb = toVectorFormUnder(*n.children[1], subst, &w);
                if (pa && (arity == 1 || pb)) {
                    ScalarForm sum;
                    uint32_t comps[3];
                    for (int i = 0; i < 3; ++i) {
                        const ScalarForm d = arity == 1 ? (*pa)[i] : (*pa)[i].plus((*pb)[i].scaled(-1.0));
                        const auto c = polyLeaf(d);   // kept for the Cauchy–Schwarz derivative bound
                        if (!c) return std::nullopt;
                        comps[i] = *c;
                        sum = sum.plus(d.times(d));
                        if (!withinBudget(sum, &why)) return std::nullopt;
                    }
                    const auto leaf = polyLeaf(sum);
                    if (!leaf) return std::nullopt;
                    AlgValue out;
                    out.s = sqrtNonnegative(*leaf);
                    setNorm(out.s, comps);
                    provenance(out.s, &n, -1);   // the authored norm is what runs
                    return out;
                }
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                if (!a->vector) { say(&why, std::string(mathOpName(n.op)) + " needs vector operands"); return std::nullopt; }
                uint32_t c[3] = {a->v[0], a->v[1], a->v[2]};
                if (arity == 2) {
                    const auto b = childOf(n, 1);
                    if (!b) return std::nullopt;
                    if (!b->vector) { say(&why, "Distance needs vector operands"); return std::nullopt; }
                    for (int i = 0; i < 3; ++i) c[i] = op(Kind::Sub, a->v[i], b->v[i]);
                }
                AlgValue out;
                out.s = euclidean(c);
                return out;
            }
            case Op::Noise: {
                if (!exactArity(n, 1, &why)) return std::nullopt;
                const auto q = childOf(n, 0);
                if (!q) return std::nullopt;
                if (!q->vector) { say(&why, "Noise needs a vector argument"); return std::nullopt; }
                AlgValue out;
                out.s = op(Kind::Noise, q->v[0], q->v[1]);
                form.nodes[out.s].c = q->v[2];
                return out;
            }
            case Op::Clamp: {
                if (!exactArity(n, 3, &why)) return std::nullopt;
                const auto lo = n.children[1] ? constantOf(*n.children[1]) : std::nullopt;
                const auto hi = n.children[2] ? constantOf(*n.children[2]) : std::nullopt;
                if (!lo || !hi) { say(&why, "Clamp with non-constant bounds"); return std::nullopt; }
                if (!std::isfinite(*lo) || !std::isfinite(*hi) || *lo > *hi) {
                    say(&why, "Clamp with inverted or non-finite bounds (std::clamp is undefined there)");
                    return std::nullopt;
                }
                const auto a = childOf(n, 0);
                if (!a) return std::nullopt;
                if (a->vector) { say(&why, "componentwise vector Clamp is not admitted"); return std::nullopt; }
                AlgValue out;
                out.s = op(Kind::Clamp, a->s);
                form.nodes[out.s].lo = *lo;
                form.nodes[out.s].hi = *hi;
                form.nodes[out.s].sourceA = n.children[0].get();
                return out;
            }
            default:
                say(&why, std::string("no exact line restriction for ") + mathOpName(n.op));
                return std::nullopt;
        }
    }

    // Rung 2c: each term = (polynomial part) × Π kind(scale·var + shift) over the
    // transcendental factors of bound coordinates; the argument restricted to the
    // line is the polynomial scale·(o + s·d) + shift.
    std::optional<AlgValue> buildScalarLeaf(const ScalarForm& form) {
        std::optional<uint32_t> sum;
        for (const auto& term : form.terms) {
            Term rest(term.coefficient, term.factors);
            std::vector<TransFactor> bound;
            for (const auto& tf : term.trans) {
                if (subst.scalars.count(tf.variable) || subst.vectors.count(tf.variable)) bound.push_back(tf);
                else rest.trans.push_back(tf);
            }
            ScalarForm restForm;
            restForm.terms.push_back(rest);
            const auto restSub = substitute(restForm, subst, &why);
            if (!restSub) return std::nullopt;
            const auto restLeaf = polyLeaf(*restSub);
            if (!restLeaf) return std::nullopt;
            provenanceForm(*restLeaf, restForm);
            uint32_t value = *restLeaf;
            for (const auto& tf : bound) {
                if (subst.vectors.count(tf.variable)) {
                    say(&why, "vector variable '" + tf.variable + "' inside " + tf.print());
                    return std::nullopt;
                }
                if (tf.kind == TransFactor::Kind::Ln) {
                    say(&why, tf.print() + " of a line-bound coordinate: the CPU is undefined at ≤ 0 where the emitter clamps");
                    return std::nullopt;
                }
                const ScalarForm arg = ScalarForm::variable(tf.variable, 1.0, tf.scale).plus(ScalarForm::constant(tf.shift));
                const auto argSub = substitute(arg, subst, &why);
                if (!argSub) return std::nullopt;
                const auto argLeaf = polyLeaf(*argSub);
                if (!argLeaf) return std::nullopt;
                provenanceForm(*argLeaf, arg);
                const uint32_t t = op(Kind::Trans, *argLeaf);
                setTransKind(t, tf.kind);
                value = op(Kind::Mul, value, t);
            }
            sum = sum ? op(Kind::Add, *sum, value) : value;
        }
        AlgValue out;
        out.s = sum ? *sum : constant(0.0);
        return out;
    }
    void setTransKind(uint32_t node, TransFactor::Kind k) { form.nodes[node].transKind = k; }

    std::optional<AlgValue> buildPiecewise(const Piecewise& model) {
        if (model.pieces.empty()) { say(&why, "Piecewise has no pieces"); return std::nullopt; }
        struct Built { const Piecewise::Piece* piece; AlgValue value; };
        std::vector<Built> built;
        for (const auto& piece : model.pieces) {
            if (piece.guard) { say(&why, "a piece carries a world guard"); return std::nullopt; }
            if (piece.whereLEZero) { say(&why, "a piece carries a pure guard"); return std::nullopt; }
            if (piece.call) { say(&why, "a piece is a function call"); return std::nullopt; }
            if (piece.fold) { say(&why, "a piece is a fold over the world"); return std::nullopt; }
            if (!piece.mathNode) { say(&why, "a piece carries no MathNode (the CPU and emitter treat it differently)"); return std::nullopt; }
            const auto v = build(*piece.mathNode);
            if (!v) return std::nullopt;
            built.push_back({&piece, *v});
            if (!piece.hasLo && !piece.hasHi) break;   // emitPiecewise returns here
        }
        for (const auto& b : built) {
            if (b.value.vector != built.front().value.vector) { say(&why, "pieces disagree on value kind"); return std::nullopt; }
        }
        const auto& first = *built.front().piece;
        if (!first.hasLo && !first.hasHi) return built.front().value;
        // Selector: the Piecewise input variable restricted to the line.
        if (model.inputVariable == pointVariable) {
            say(&why, "Piecewise input variable is the vector point");
            return std::nullopt;
        }
        MathNode selectorNode;
        selectorNode.op = MathNode::Op::ValueLeaf;
        selectorNode.variableName = model.inputVariable;
        std::string w;
        const auto sel = toScalarFormUnder(selectorNode, subst, &w);
        if (!sel) { say(&why, "Piecewise input variable: " + w); return std::nullopt; }
        auto selector = restrictedFrom(*sel, &why);
        if (!selector) return std::nullopt;
        const bool vector = built.front().value.vector;
        AlgValue out;
        out.vector = vector;
        for (int c = 0; c < (vector ? 3 : 1); ++c) {
            LineAlgebraicNode node;
            node.kind = Kind::Select;
            node.polynomial = *selector;
            for (const auto& b : built) {
                LineAlgebraicNode::Piece p;
                p.hasLo = b.piece->hasLo;
                p.hasHi = b.piece->hasHi;
                p.lo = b.piece->lo;
                p.hi = b.piece->hi;
                p.value = vector ? b.value.v[c] : b.value.s;
                node.pieces.push_back(p);
            }
            const uint32_t id = push(std::move(node));
            if (vector) out.v[c] = id; else out.s = id;
        }
        return out;
    }

    void finalize() {
        for (uint32_t i = 0; i < form.nodes.size(); ++i) {
            auto& n = form.nodes[i];
            n.switchBegin = static_cast<uint32_t>(form.switches.size());
            auto add = [&](uint32_t role, bool strict) { form.switches.push_back({i, role, strict}); };
            switch (n.kind) {
                case Kind::Sqrt: if (!n.nonnegativeArgument) add(0, false); break;
                case Kind::Abs: add(0, false); break;
                case Kind::Min: case Kind::Max: add(1, false); break;
                case Kind::Clamp: add(2, false); add(3, false); break;
                case Kind::Select:
                    for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                        if (n.pieces[p].hasLo) add(4 + 2 * p, true);
                        if (n.pieces[p].hasHi) add(5 + 2 * p, true);
                    }
                    break;
                default: break;
            }
            n.switchCount = static_cast<uint32_t>(form.switches.size()) - n.switchBegin;
            if (n.kind == Kind::Polynomial || n.kind == Kind::Select)
                form.reads.insert(n.polynomial.reads.begin(), n.polynomial.reads.end());
        }
    }
};

LineAlgebraicRestriction finish(AlgebraicBuilder& b, const std::optional<AlgValue>& v, bool vectorValued) {
    LineAlgebraicRestriction out;
    if (!v) { out.refusal = b.why.empty() ? "not admitted" : b.why; return out; }
    if (v->vector != vectorValued) {
        out.refusal = vectorValued ? "expression is scalar-valued where a vector was required"
                                   : "expression is vector-valued where a scalar was required";
        return out;
    }
    if (vectorValued) b.form.roots = {v->v[0], v->v[1], v->v[2]};
    else b.form.roots = {v->s};
    b.finalize();
    out.form = std::move(b.form);
    return out;
}

// --- interval helpers (outward where rounding can occur) -------------------
Interval pointOrEnclose(double v) {
    if (!std::isfinite(v) || std::abs(v) > double(std::numeric_limits<float>::max())) return Interval::infinite();
    const float f = static_cast<float>(v);
    if (double(f) == v) return Interval(f);   // exact: keep the singleton (0 stays 0)
    return Interval::outward(f, f);
}
Interval enclose(double v, double err) {
    if (!std::isfinite(v) || !std::isfinite(err)) return Interval::infinite();
    const double lo = v - err, hi = v + err;
    if (std::abs(lo) > double(std::numeric_limits<float>::max()) ||
        std::abs(hi) > double(std::numeric_limits<float>::max())) return Interval::infinite();
    return Interval::outward(static_cast<float>(lo), static_cast<float>(hi));
}
Interval sqrtOf(const Interval& r) {
    const float lo = std::max(r.lo, 0.0f), hi = std::max(r.hi, 0.0f);
    const float l = lo == 0.0f ? 0.0f : Interval::roundDown(std::sqrt(lo));
    const float h = std::isinf(hi) ? hi : (hi == 0.0f ? 0.0f : Interval::roundUp(std::sqrt(hi)));
    return Interval(std::max(l, 0.0f), h);
}
Interval absOf(const Interval& r) {
    if (r.lo >= 0.0f) return r;
    if (r.hi <= 0.0f) return -r;
    return Interval(0.0f, std::max(-r.lo, r.hi));
}
Interval minOf(const Interval& a, const Interval& b) { return Interval(std::min(a.lo, b.lo), std::min(a.hi, b.hi)); }
Interval maxOf(const Interval& a, const Interval& b) { return Interval(std::max(a.lo, b.lo), std::max(a.hi, b.hi)); }

double horner(const std::vector<double>& a, double s) { return LineRestrictedForm::evaluatePolynomial(a, s); }

} // namespace

bool LineAlgebraicForm::polynomialOnly() const {
    return std::all_of(nodes.begin(), nodes.end(), [](const LineAlgebraicNode& n) {
        return n.kind == Kind::Polynomial || n.kind == Kind::Add || n.kind == Kind::Sub || n.kind == Kind::Mul;
    });
}

LineAlgebraicRestriction restrictAlgebraicToLine(const MathNode& node, bool vectorValued,
                                                 const std::string& pointVariable) {
    AlgebraicBuilder b{linePointSubstitution(pointVariable), pointVariable, {}, {}};
    const auto v = b.build(node);
    return finish(b, v, vectorValued);
}

LineAlgebraicRestriction restrictAlgebraicToLine(const Piecewise& model, bool vectorValued,
                                                 const std::string& pointVariable) {
    AlgebraicBuilder b{linePointSubstitution(pointVariable), pointVariable, {}, {}};
    const auto v = b.buildPiecewise(model);
    return finish(b, v, vectorValued);
}

// ---- Execution provenance (Codex review P1, 2026-10-17) --------------------
std::optional<double> literalConstant(const MathNode& node) {
    if (node.op != MathNode::Op::ScalarLeaf || node.scalarForm.terms.size() > 1) return std::nullopt;
    if (node.scalarForm.terms.empty()) return 0.0;
    const auto& t = node.scalarForm.terms.front();
    if (!t.factors.empty() || !t.trans.empty() || !std::isfinite(t.coefficient)) return std::nullopt;
    return t.coefficient;
}

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();

// A value an IEEE f32 device holds exactly: reading it, and correctly-rounded
// arithmetic whose exact result is such a value, err by nothing at all.
bool exactF32(double v) { return std::isfinite(v) && double(static_cast<float>(v)) == v; }
double literalSlack(double c, double e) { return exactF32(c) ? 0.0 : e * std::abs(c); }

struct ExecValue {
    bool vector = false;
    ExecutionBound c[3];
};

std::optional<double> magnitudeOf(const std::string& name, const std::map<std::string, double>& m) {
    const auto it = m.find(name);
    if (it == m.end() || std::isnan(it->second)) return std::nullopt;
    return std::abs(it->second);
}

// Magnitude-arithmetic rules. ε is the declared per-operation relative error.
ExecutionBound execAdd(const ExecutionBound& a, const ExecutionBound& b, double e) {
    const double m = a.magnitude + b.magnitude;
    return {m, a.slack + b.slack + e * m};
}
ExecutionBound execMul(const ExecutionBound& a, const ExecutionBound& b, double e) {
    const double m = a.magnitude * b.magnitude;
    return {m, a.magnitude * b.slack + b.magnitude * a.slack + a.slack * b.slack + e * m};
}

std::optional<ExecutionBound> execScalarForm(const ScalarForm& form, const std::map<std::string, double>& mags,
                                             const ExecutionModel& model) {
    const double e = model.relative, tr = model.transcendental;
    ExecutionBound sum;
    bool first = true;
    for (const auto& term : form.terms) {
        if (!std::isfinite(term.coefficient)) return std::nullopt;
        ExecutionBound t{std::abs(term.coefficient), literalSlack(term.coefficient, e)};
        for (const auto& [var, exponent] : term.factors) {
            const auto a = magnitudeOf(var, mags);
            if (!a || !std::isfinite(exponent) || exponent < 0.0) return std::nullopt;
            const ExecutionBound v{*a, e * *a};
            if (std::floor(exponent) == exponent && exponent <= kMaxLineDegree) {
                for (int k = 0; k < static_cast<int>(exponent); ++k) t = execMul(t, v, e);
            } else {
                // ontoRawPow: |x|^q, Hölder-q in x; charge a full magnitude of error.
                const double m = std::pow(*a + v.slack, exponent);
                t = execMul(t, {m, m}, e);
            }
        }
        for (const auto& tf : term.trans) {
            const auto a = magnitudeOf(tf.variable, mags);
            if (!a) return std::nullopt;
            const ExecutionBound arg = execAdd(execMul({std::abs(tf.scale), e * std::abs(tf.scale)}, {*a, e * *a}, e),
                                               {std::abs(tf.shift), e * std::abs(tf.shift)}, e);
            ExecutionBound f;
            if (tf.kind == TransFactor::Kind::Sin || tf.kind == TransFactor::Kind::Cos) {
                f = {1.0, arg.slack + tr};
            } else if (tf.kind == TransFactor::Kind::Exp) {
                const double m = std::exp(arg.magnitude + arg.slack);
                f = {std::exp(arg.magnitude), m * (arg.slack + tr)};
            } else {
                return std::nullopt;   // Ln: the CPU and emitter disagree at ≤ 0
            }
            t = execMul(t, f, e);
        }
        // The first term is not added to anything: no rounding to charge.
        sum = first ? t : execAdd(sum, t, e);
        first = false;
    }
    if (!std::isfinite(sum.magnitude) || std::isnan(sum.slack)) return std::nullopt;
    return sum;
}

std::optional<ExecValue> execNode(const MathNode& n, const std::map<std::string, double>& mags,
                                  const ExecutionModel& model, const std::string& pointVariable) {
    using Op = MathNode::Op;
    const double e = model.relative, tr = model.transcendental;
    auto child = [&](std::size_t i) -> std::optional<ExecValue> {
        if (i >= n.children.size() || !n.children[i]) return std::nullopt;
        return execNode(*n.children[i], mags, model, pointVariable);
    };
    auto scalar = [](const ExecutionBound& b) { ExecValue v; v.c[0] = b; return v; };
    auto pointwise = [&](const ExecValue& a, const ExecValue& b, auto&& f) -> std::optional<ExecValue> {
        ExecValue out;
        out.vector = a.vector || b.vector;
        for (int i = 0; i < (out.vector ? 3 : 1); ++i)
            out.c[i] = f(a.vector ? a.c[i] : a.c[0], b.vector ? b.c[i] : b.c[0]);
        return out;
    };
    switch (n.op) {
        case Op::ScalarLeaf: {
            const auto b = execScalarForm(n.scalarForm, mags, model);
            if (!b) return std::nullopt;
            return scalar(*b);
        }
        case Op::ValueLeaf: {
            static const char* const kAxes[3] = {"x", "y", "z"};
            if (n.variableName == pointVariable) {
                ExecValue v;
                v.vector = true;
                for (int i = 0; i < 3; ++i) {
                    const auto a = magnitudeOf(kAxes[i], mags);
                    if (!a) return std::nullopt;
                    v.c[i] = {*a, e * *a};
                }
                return v;
            }
            const auto a = magnitudeOf(n.variableName, mags);
            if (!a) return std::nullopt;
            return scalar({*a, e * *a});
        }
        case Op::VectorConstruct: {
            if (n.children.size() != 3) return std::nullopt;
            ExecValue v;
            v.vector = true;
            for (int i = 0; i < 3; ++i) {
                const auto c = child(i);
                if (!c || c->vector) return std::nullopt;
                v.c[i] = c->c[0];
            }
            return v;
        }
        case Op::Component: {
            const auto a = child(0);
            if (!a || !a->vector) return std::nullopt;
            const int i = n.stringArg == "x" ? 0 : n.stringArg == "y" ? 1 : n.stringArg == "z" ? 2 : -1;
            if (i < 0) return std::nullopt;
            return scalar(a->c[i]);
        }
        case Op::Add:
        case Op::Sub: {
            const auto a = child(0), b = child(1);
            if (!a || !b || a->vector != b->vector) return std::nullopt;
            return pointwise(*a, *b, [&](const ExecutionBound& x, const ExecutionBound& y) { return execAdd(x, y, e); });
        }
        case Op::Scale:
        case Op::Hadamard: {
            const auto a = child(0), b = child(1);
            if (!a || !b) return std::nullopt;
            return pointwise(*a, *b, [&](const ExecutionBound& x, const ExecutionBound& y) { return execMul(x, y, e); });
        }
        case Op::Dot: {
            const auto a = child(0), b = child(1);
            if (!a || !b || !a->vector || !b->vector) return std::nullopt;
            ExecutionBound acc = execMul(a->c[0], b->c[0], e);
            for (int i = 1; i < 3; ++i) acc = execAdd(acc, execMul(a->c[i], b->c[i], e), e);
            return scalar(acc);
        }
        case Op::Cross: {
            const auto a = child(0), b = child(1);
            if (!a || !b || !a->vector || !b->vector) return std::nullopt;
            ExecValue v;
            v.vector = true;
            for (int i = 0; i < 3; ++i) {
                const int j = (i + 1) % 3, k = (i + 2) % 3;
                v.c[i] = execAdd(execMul(a->c[j], b->c[k], e), execMul(a->c[k], b->c[j], e), e);
            }
            return v;
        }
        case Op::Div: {
            const auto a = child(0);
            const auto c = n.children.size() == 2 && n.children[1] ? literalConstant(*n.children[1]) : std::nullopt;
            if (!a || !c || !(std::abs(*c) >= kDegenerateDivisor)) return std::nullopt;
            ExecValue out = *a;
            for (int i = 0; i < (a->vector ? 3 : 1); ++i) {
                const double m = a->c[i].magnitude / std::abs(*c);
                out.c[i] = {m, a->c[i].slack / std::abs(*c) + 3.0 * e * m};
            }
            return out;
        }
        case Op::Pow: {
            const auto a = child(0);
            const auto p = n.children.size() == 2 && n.children[1] ? literalConstant(*n.children[1]) : std::nullopt;
            if (!a || a->vector || !p || *p < 0.0 || std::floor(*p) != *p || *p > kMaxLineDegree) return std::nullopt;
            ExecutionBound acc{1.0, 0.0};
            for (int k = 0; k < static_cast<int>(*p); ++k) acc = execMul(acc, a->c[0], e);
            acc.slack += 2.0 * e * acc.magnitude;   // pow() is a library call, not a product chain
            return scalar(acc);
        }
        case Op::Abs: {
            return child(0);   // 1-Lipschitz and exact
        }
        case Op::Union:
        case Op::Intersection:
        case Op::Difference: {
            const auto a = child(0), b = child(1);
            if (!a || !b) return std::nullopt;
            return pointwise(*a, *b, [](const ExecutionBound& x, const ExecutionBound& y) {
                return ExecutionBound{std::max(x.magnitude, y.magnitude), std::max(x.slack, y.slack)};
            });
        }
        case Op::Clamp: {
            const auto a = child(0);
            const auto lo = n.children.size() == 3 && n.children[1] ? literalConstant(*n.children[1]) : std::nullopt;
            const auto hi = n.children.size() == 3 && n.children[2] ? literalConstant(*n.children[2]) : std::nullopt;
            if (!a || a->vector || !lo || !hi) return std::nullopt;
            const double bound = std::max(std::abs(*lo), std::abs(*hi));
            return scalar({bound, std::max({a->c[0].slack, literalSlack(*lo, e), literalSlack(*hi, e)})});
        }
        case Op::Sqrt: {
            const auto a = child(0);
            if (!a || a->vector) return std::nullopt;
            const double m = std::sqrt(a->c[0].magnitude);
            return scalar({m, std::sqrt(a->c[0].slack) + e * m});   // |√x − √y| ≤ √|x − y|
        }
        case Op::Length:
        case Op::Distance: {
            const auto a = child(0);
            if (!a || !a->vector) return std::nullopt;
            ExecValue d = *a;
            if (n.op == Op::Distance) {
                const auto b = child(1);
                if (!b || !b->vector) return std::nullopt;
                for (int i = 0; i < 3; ++i) d.c[i] = execAdd(a->c[i], b->c[i], e);
            }
            double m2 = 0.0, sl = 0.0;
            for (int i = 0; i < 3; ++i) { m2 += d.c[i].magnitude * d.c[i].magnitude; sl += d.c[i].slack; }
            const double m = std::sqrt(m2);
            return scalar({m, sl + 4.0 * e * m});   // ‖·‖ is 1-Lipschitz; squares, sums, sqrt
        }
        case Op::Noise: {
            const auto q = child(0);
            if (!q || !q->vector) return std::nullopt;
            double sl = 0.0;
            for (int i = 0; i < 3; ++i) sl += q->c[i].slack;
            return scalar({double(kClassicPerlin3ValueBound), double(kClassicPerlin3LipschitzBound) * sl + tr});
        }
        default:
            return std::nullopt;
    }
}
} // namespace

std::optional<ExecutionBound> executionBound(const ScalarForm& form, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model) {
    return execScalarForm(form, magnitudes, model);
}

std::optional<ExecutionBound> executionBound(const MathNode& node, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, int component, const std::string& pointVariable) {
    const auto v = execNode(node, magnitudes, model, pointVariable);
    if (!v) return std::nullopt;
    if (v->vector != (component >= 0) || component > 2) return std::nullopt;
    const auto& b = v->c[component >= 0 ? component : 0];
    if (!std::isfinite(b.magnitude) || !std::isfinite(b.slack)) return std::nullopt;
    return b;
}

std::optional<ExecutionBound> executionBound(const Piecewise& piecewise, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, int component, const std::string& pointVariable) {
    ExecutionBound out;
    bool any = false;
    for (const auto& piece : piecewise.pieces) {
        if (!piece.mathNode) continue;
        const auto b = executionBound(*piece.mathNode, magnitudes, model, component, pointVariable);
        if (!b) return std::nullopt;
        out = {std::max(out.magnitude, b->magnitude), std::max(out.slack, b->slack)};
        any = true;
        if (!piece.hasLo && !piece.hasHi) break;
    }
    if (!any) return std::nullopt;
    return out;
}

LineAlgebraicEvaluator::LineAlgebraicEvaluator(const LineAlgebraicForm& form,
                                               std::map<std::string, double> bindings,
                                               bool undefinedAsZero)
    : _form(&form), _bindings(std::move(bindings)), _undefinedAsZero(undefinedAsZero) {
    _coefficients.resize(form.nodes.size());
    _errors.resize(form.nodes.size());
    computeSignFree();
    constexpr double kEps = std::numeric_limits<double>::epsilon();
    for (std::size_t i = 0; i < form.nodes.size(); ++i) {
        const auto& n = form.nodes[i];
        if (n.kind != Kind::Polynomial && n.kind != Kind::Select) continue;
        const auto a = n.polynomial.evaluateCoefficients(_bindings);
        if (!a) { _refusal = "a coefficient is undefined, unbound or non-finite on this line"; return; }
        _coefficients[i] = *a;
        for (std::size_t k = 0; k < a->size(); ++k) {
            // Forward-error bound of evaluating c_k in double: each term is a
            // product of ≤ (degree + factors) correctly-rounded operations, and
            // the sum adds one rounding per term. 4× slack over γ_n·Σ|terms|.
            double magnitude = 0.0;
            std::size_t factors = 0;
            for (const auto& term : n.polynomial.coefficients[k].terms) {
                const auto tv = term.evaluate(_bindings);
                if (!tv) { _refusal = "a coefficient term is undefined on this line"; return; }
                magnitude += std::abs(*tv);
                double f = 0.0;
                for (const auto& [name, e] : term.factors) f += std::abs(e);
                factors = std::max<std::size_t>(factors, static_cast<std::size_t>(f) + term.trans.size() * 4);
            }
            const double gamma = 4.0 * double(n.polynomial.coefficients[k].terms.size() + factors + 4) * kEps;
            _errors[i].push_back(gamma * magnitude);
        }
    }
}

LineAlgebraicEvaluator::LineAlgebraicEvaluator(const LineAlgebraicForm& form,
                                               const std::map<std::string, Interval>& boxBindings,
                                               bool undefinedAsZero)
    : _form(&form), _undefinedAsZero(undefinedAsZero), _boxMode(true) {
    _box = boxBindings;
    _coefficients.resize(form.nodes.size());
    _errors.resize(form.nodes.size());
    computeSignFree();
    for (std::size_t i = 0; i < form.nodes.size(); ++i) {
        const auto& n = form.nodes[i];
        if (n.kind != Kind::Polynomial && n.kind != Kind::Select) continue;
        for (const auto& c : n.polynomial.coefficients) {
            const Interval r = c.evalRange(boxBindings);
            if (!r.bounded() || r.empty()) { _refusal = "a coefficient is unbounded over the box"; return; }
            // One extra outward ulp: evalRange's lone constant terms are float points.
            const double lo = Interval::roundDown(r.lo), hi = Interval::roundUp(r.hi);
            const double mid = 0.5 * (lo + hi);
            _coefficients[i].push_back(mid);
            _errors[i].push_back(0.5 * (hi - lo) + 4.0 * std::numeric_limits<double>::epsilon() * std::abs(mid));
        }
    }
}

// ---- Execution slack (Codex review P1, 2026-10-17) -------------------------
void LineAlgebraicEvaluator::setExecutionModel(const ExecutionModel& model, std::map<std::string, double> magnitudeOverrides) {
    _model = {std::max(model.relative, 0.0), std::max(model.transcendental, 0.0)};
    _magnitudeOverrides = std::move(magnitudeOverrides);
    _slackMemoS1 = _slackMemoS0 - 1.0;
}

std::map<std::string, double> LineAlgebraicEvaluator::symbolMagnitudes(double s0, double s1) const {
    std::map<std::string, double> m;
    static const char* const kAxes[3] = {"x", "y", "z"};
    const double sMax = std::max(std::abs(s0), std::abs(s1));
    auto widen = [](double v) { return v * (1.0 + 4.0 * std::numeric_limits<double>::epsilon()); };
    if (_boxMode) {
        auto sup = [&](const std::string& name) {
            const auto it = _box.find(name);
            if (it == _box.end()) return std::numeric_limits<double>::infinity();
            return std::max(std::abs(double(it->second.lo)), std::abs(double(it->second.hi)));
        };
        for (const auto& [name, r] : _box) m[name] = std::max(std::abs(double(r.lo)), std::abs(double(r.hi)));
        for (int i = 0; i < 3; ++i)
            m[kAxes[i]] = widen(sup(kLineOriginVars[i]) + sMax * sup(kLineDirectionVars[i]));
    } else {
        for (const auto& [name, v] : _bindings) m[name] = std::abs(v);
        for (int i = 0; i < 3; ++i) {
            const auto o = _bindings.find(kLineOriginVars[i]), d = _bindings.find(kLineDirectionVars[i]);
            if (o == _bindings.end() || d == _bindings.end()) continue;
            // Linear in s: the extreme is at an endpoint.
            m[kAxes[i]] = widen(std::max(std::abs(o->second + s0 * d->second), std::abs(o->second + s1 * d->second)));
        }
    }
    for (const auto& [name, v] : _magnitudeOverrides) m[name] = std::abs(v);
    return m;
}

double LineAlgebraicEvaluator::magnitudeAt(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const {
    bool undefined = false;
    const Interval r = rangeAt(node, s0, s1, signs, &undefined);
    if (std::isnan(r.lo) || std::isnan(r.hi)) return std::numeric_limits<double>::infinity();
    const double m = std::max(std::abs(double(r.lo)), std::abs(double(r.hi)));
    return undefined && !_undefinedAsZero ? std::numeric_limits<double>::infinity() : m;
}

double LineAlgebraicEvaluator::slack(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const {
    if (!_model.active()) return 0.0;
    constexpr double inf = std::numeric_limits<double>::infinity();
    const auto& form = *_form;
    const auto& n = form.nodes[node];
    const double e = _model.relative, tr = _model.transcendental;
    if (n.provenance || n.provenanceForm) {
        // Switch-free authored subtree: its slack is monotone in the region, so
        // a memo over an enclosing interval is a sound answer for [s0, s1].
        if (!(_slackMemoS0 <= s0 && s1 <= _slackMemoS1) || _slackMemo.size() != form.nodes.size()) {
            _slackMemoS0 = s0; _slackMemoS1 = s1;
            _slackMemo.assign(form.nodes.size(), 0.0);
            _slackMemoValid.assign(form.nodes.size(), 0);
            _slackMagnitudes = symbolMagnitudes(s0, s1);
        }
        if (_slackMemoValid[node]) return _slackMemo[node];
        const auto& mags = _slackMagnitudes;
        const auto b = n.provenance ? executionBound(*n.provenance, mags, _model, n.provenanceComponent)
                                    : executionBound(*n.provenanceForm, mags, _model);
        const double out = b ? b->slack : inf;
        _slackMemo[node] = out;
        _slackMemoValid[node] = 1;
        return out;
    }
    // A switch the caller left undecided may still be decided HERE by its own
    // range beyond its own slack — the very proof segmentLine would make — so a
    // clamp every interval proves saturated executes its literal (e.g. a box
    // that never reaches a pillar's centre). Never a guess: undecided stays 0.
    auto sign = [&](uint32_t offset) -> int8_t {
        const uint32_t k = n.switchBegin + offset;
        if (k < signs.size() && signs[k] != 0) return signs[k];
        bool undefined = false;
        const Interval r = switchRange(k, s0, s1, signs, &undefined);
        if (undefined || std::isnan(r.lo) || std::isnan(r.hi)) return 0;
        const double sl = switchSlack(k, s0, s1, signs);
        const bool strict = form.switches[k].strict;
        const double lo = double(r.lo) - sl, hi = double(r.hi) + sl;
        if (strict ? lo > 0.0 : lo >= 0.0) return 1;
        if (strict ? hi < 0.0 : hi <= 0.0) return -1;
        return 0;
    };
    switch (n.kind) {
        case Kind::Polynomial: return inf;   // no authored provenance: never silently exact
        case Kind::Undefined: return _undefinedAsZero ? 0.0 : inf;
        case Kind::Add:
        case Kind::Sub:
        case Kind::Mul: {
            const double sa = slack(n.a, s0, s1, signs), sb = slack(n.b, s0, s1, signs);
            if (sa == 0.0 && sb == 0.0) {
                // Two exactly-held point values (a saturated clamp's literal, an
                // exact literal): correctly-rounded arithmetic is exact when the
                // exact result is itself an f32 value (e.g. 1 − 1, 0·x̂).
                bool ua = false, ub = false;
                const Interval ra = rangeAt(n.a, s0, s1, signs, &ua), rb = rangeAt(n.b, s0, s1, signs, &ub);
                if (!ua && !ub && ra.lo == ra.hi && rb.lo == rb.hi) {
                    const double x = ra.lo, y = rb.lo;
                    const double r = n.kind == Kind::Add ? x + y : n.kind == Kind::Sub ? x - y : x * y;
                    if (exactF32(x) && exactF32(y) && exactF32(r)) return 0.0;
                }
            }
            const double ma = magnitudeAt(n.a, s0, s1, signs), mb = magnitudeAt(n.b, s0, s1, signs);
            if (n.kind == Kind::Mul) return ma * sb + mb * sa + sa * sb + e * ma * mb;
            return sa + sb + e * (ma + mb);
        }
        case Kind::Sqrt: {
            if (!n.nonnegativeArgument && sign(0) < 0) return 0.0;   // executes sqrt(max(a, 0)) = 0 exactly
            if (n.nonnegativeArgument && !n.normComponents.empty()) {
                // A Euclidean norm is 1-Lipschitz — even at 0, where Hölder √S is
                // hopeless (a torus box contains its axis): the components' slack
                // plus the squares, sums and sqrt rounding, relative to ‖v‖.
                double components = 0.0;
                for (uint32_t c : n.normComponents) components += slack(c, s0, s1, signs);
                return components + 4.0 * e * std::sqrt(magnitudeAt(n.a, s0, s1, signs));
            }
            const double sa = slack(n.a, s0, s1, signs);
            bool undefined = false;
            const Interval r = rangeAt(n.a, s0, s1, signs, &undefined);
            // Hölder √S near 0; where the argument provably stays above its own
            // slack, sqrt is Lipschitz: |√x − √y| ≤ |x − y| / (√x + √y).
            const double floorArg = double(r.lo) - sa;
            const double propagated = floorArg > 0.0 && !undefined
                                          ? sa / (std::sqrt(floorArg) + std::sqrt(std::max(double(r.lo), 0.0)))
                                          : std::sqrt(sa);
            return std::min(propagated, std::sqrt(sa)) + e * std::sqrt(magnitudeAt(n.a, s0, s1, signs));
        }
        case Kind::Abs: return slack(n.a, s0, s1, signs);
        case Kind::Min:
        case Kind::Max: {
            const int8_t g = sign(0);   // a − b
            if (g != 0) {
                const bool takeA = (g > 0) == (n.kind == Kind::Max);
                return slack(takeA ? n.a : n.b, s0, s1, signs);
            }
            return std::max(slack(n.a, s0, s1, signs), slack(n.b, s0, s1, signs));
        }
        case Kind::Clamp: {
            const int8_t lo = sign(0), hi = sign(1);   // a − lo, a − hi
            if (lo < 0) return literalSlack(n.lo, e);  // saturated: the literal bound, as read
            if (hi > 0) return literalSlack(n.hi, e);
            const double sa = slack(n.a, s0, s1, signs);
            if (lo > 0 && hi < 0) return sa;
            return std::max({sa, literalSlack(n.lo, e), literalSlack(n.hi, e)});
        }
        case Kind::Select: {
            // Only a DECIDED piece is faithful: an undecided boundary may jump.
            uint32_t offset = 0;
            for (const auto& piece : n.pieces) {
                int8_t state = 1;   // 1 satisfied, -1 violated, 0 unknown
                if (piece.hasLo) { const int8_t g = sign(offset++); state = std::min<int8_t>(state, g); }
                if (piece.hasHi) { const int8_t g = sign(offset++); if (g < 0) state = -1; else if (g == 0 && state > 0) state = 0; }
                if (state < 0) continue;
                if (state == 0) return inf;
                return slack(piece.value, s0, s1, signs);
            }
            return _undefinedAsZero ? 0.0 : inf;
        }
        case Kind::Trans: {
            const double sa = slack(n.a, s0, s1, signs);
            if (n.transKind == TransFactor::Kind::Exp) {
                bool undefined = false;
                const Interval r = rangeAt(n.a, s0, s1, signs, &undefined);
                return std::exp(double(r.hi) + sa) * (sa + tr);
            }
            return sa + tr;
        }
        case Kind::Noise:
            return double(kClassicPerlin3LipschitzBound) *
                       (slack(n.a, s0, s1, signs) + slack(n.b, s0, s1, signs) + slack(n.c, s0, s1, signs)) +
                   tr;
    }
    return inf;
}

double LineAlgebraicEvaluator::switchSlack(std::size_t k, double s0, double s1, const std::vector<int8_t>& signs) const {
    if (!_model.active()) return 0.0;
    const auto& sw = _form->switches[k];
    const auto& n = _form->nodes[sw.node];
    const double e = _model.relative;
    if (sw.role == 0) return slack(n.a, s0, s1, signs);
    if (sw.role == 1)
        return slack(n.a, s0, s1, signs) + slack(n.b, s0, s1, signs) +
               e * (magnitudeAt(n.a, s0, s1, signs) + magnitudeAt(n.b, s0, s1, signs));
    if (sw.role == 2 || sw.role == 3) {
        const double bound = std::abs(sw.role == 2 ? n.lo : n.hi);
        return slack(n.a, s0, s1, signs) + e * (magnitudeAt(n.a, s0, s1, signs) + 2.0 * bound);
    }
    // Select: the selector is an authored coordinate read against a literal bound.
    const uint32_t p = (sw.role - 4) / 2;
    if (p >= n.pieces.size()) return std::numeric_limits<double>::infinity();
    const double bound = std::abs(sw.role % 2 == 0 ? n.pieces[p].lo : n.pieces[p].hi);
    const Interval sel = polynomialRange(sw.node, s0, s1);
    const double ms = std::max(std::abs(double(sel.lo)), std::abs(double(sel.hi)));
    return e * (2.0 * ms + 2.0 * bound);
}

void LineAlgebraicEvaluator::computeSignFree() {
    const auto& form = *_form;
    // Operands are always built before their parents, so one forward pass decides
    // which subtrees carry no switch (their range depends on the interval alone).
    _signFree.assign(form.nodes.size(), 0);
    for (std::size_t i = 0; i < form.nodes.size(); ++i) {
        const auto& n = form.nodes[i];
        bool free = n.switchCount == 0;
        switch (n.kind) {
            case Kind::Polynomial: case Kind::Undefined: break;
            case Kind::Sqrt: case Kind::Abs: case Kind::Trans: case Kind::Clamp:
                free = free && n.a < i && _signFree[n.a];
                break;
            case Kind::Noise:
                free = free && _signFree[n.a] && _signFree[n.b] && _signFree[n.c];
                break;
            case Kind::Select: free = false; break;
            default:
                free = free && n.a < i && n.b < i && _signFree[n.a] && _signFree[n.b];
                break;
        }
        _signFree[i] = free ? 1 : 0;
    }
}

std::optional<double> LineAlgebraicEvaluator::value(uint32_t node, double s) const {
    const auto& n = _form->nodes[node];
    auto undefinedValue = [&]() -> std::optional<double> {
        if (_undefinedAsZero) return 0.0;
        return std::nullopt;
    };
    switch (n.kind) {
        case Kind::Polynomial: return horner(_coefficients[node], s);
        case Kind::Undefined: return undefinedValue();
        case Kind::Select: {
            const double sel = horner(_coefficients[node], s);
            for (const auto& p : n.pieces) {
                // The emitter's tests (>=, <=); STRICT switches keep definite
                // pieces away from the bound, where the CPU's include flags differ.
                if ((!p.hasLo || sel >= p.lo) && (!p.hasHi || sel <= p.hi)) return value(p.value, s);
            }
            return undefinedValue();
        }
        default: break;
    }
    const auto a = value(n.a, s);
    if (!a) return std::nullopt;
    switch (n.kind) {
        case Kind::Sqrt: return std::sqrt(std::max(0.0, *a));
        case Kind::Abs: return std::abs(*a);
        case Kind::Clamp: return std::clamp(*a, n.lo, n.hi);
        case Kind::Trans: return TransFactor(n.transKind, "u").evaluate(*a);
        case Kind::Noise: {
            // MathNode::evaluate carries the argument as glm::vec3 (f32).
            const auto b = value(n.b, s), c = value(n.c, s);
            if (!b || !c) return std::nullopt;
            return static_cast<double>(glm::perlin(glm::vec3(float(*a), float(*b), float(*c))));
        }
        default: break;
    }
    const auto b = value(n.b, s);
    if (!b) return std::nullopt;
    switch (n.kind) {
        case Kind::Add: return *a + *b;
        case Kind::Sub: return *a - *b;
        case Kind::Mul: return *a * *b;
        case Kind::Min: return std::min(*a, *b);
        case Kind::Max: return std::max(*a, *b);
        default: return std::nullopt;
    }
}

Interval LineAlgebraicEvaluator::range(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs,
                                       bool* undefined) const {
    return rangeAt(node, s0, s1, signs, undefined);
}

Interval LineAlgebraicEvaluator::polynomialRange(uint32_t node, double s0, double s1) const {
    if (s0 != _memoS0 || s1 != _memoS1 || _memoValid.size() != _coefficients.size()) {
        _memoS0 = s0;
        _memoS1 = s1;
        _memoRange.assign(_coefficients.size(), Interval());
        _memoValid.assign(_coefficients.size(), 0);
    }
    if (_memoValid[node]) { ++_memoHits; return _memoRange[node]; }
    ++_memoMisses;
    _memoValid[node] = 1;
    return _memoRange[node] = polynomialRangeUncached(node, s0, s1);
}

namespace {
Interval centeredPolynomialRange(const std::vector<double>& a, const std::vector<double>& e, double s0, double s1);
}

Interval LineAlgebraicEvaluator::polynomialRangeUncached(uint32_t node, double s0, double s1) const {
    return centeredPolynomialRange(_coefficients[node], _errors[node], s0, s1);
}

namespace {
Interval centeredPolynomialRange(const std::vector<double>& a, const std::vector<double>& e, double s0, double s1) {
    const std::size_t n = a.size();
    if (n == 0) return Interval(0.0f);
    constexpr double kEps = std::numeric_limits<double>::epsilon();
    const double m = 0.5 * (s0 + s1);   // any double: it is the chosen centre, exact by definition
    // h = s − m for real s ∈ [s0, s1]; each endpoint subtraction rounds once.
    const double h0 = s0 - m, h1 = s1 - m;
    const Interval h = enclose(0.5 * (h0 + h1), 0.5 * (h1 - h0) + 2 * kEps * (std::abs(h0) + std::abs(h1)) + 1e-300);
    // Taylor shift b_k = Σ_{j≥k} C(j,k)·a_j·m^{j−k}, with error from the a_j
    // enclosures plus the shift's own rounding (4× slack over γ_{2n+4}).
    std::vector<Interval> b(n);
    for (std::size_t k = 0; k < n; ++k) {
        double sum = 0.0, magnitude = 0.0, err = 0.0;
        double binom = 1.0, mPow = 1.0, mAbsPow = 1.0;   // C(j,k), m^{j−k}
        for (std::size_t j = k; j < n; ++j) {
            if (j > k) {
                binom = binom * double(j) / double(j - k);
                mPow *= m;
                mAbsPow *= std::abs(m);
            }
            const double term = binom * a[j] * mPow;
            sum += term;
            magnitude += std::abs(term);
            err += binom * mAbsPow * e[j];
        }
        b[k] = enclose(sum, err + 4.0 * double(2 * n + 4) * kEps * magnitude);
    }
    Interval acc = b[n - 1];
    for (std::size_t k = n - 1; k-- > 0;) acc = acc * h + b[k];
    return acc;
}
} // namespace

namespace {
// Select piece applicability under signs: +1 applies, −1 does not, 0 unknown.
int pieceState(const LineAlgebraicNode& n, uint32_t p, const std::vector<int8_t>& signs) {
    const auto& piece = n.pieces[p];
    int state = 1;
    uint32_t k = n.switchBegin;
    for (uint32_t q = 0; q < p; ++q) k += (n.pieces[q].hasLo ? 1 : 0) + (n.pieces[q].hasHi ? 1 : 0);
    for (int t = 0; t < 2; ++t) {
        const bool present = t == 0 ? piece.hasLo : piece.hasHi;
        if (!present) continue;
        const int8_t sg = signs[k++];
        if (sg < 0) return -1;
        if (sg == 0) state = 0;
    }
    return state;
}
} // namespace

Interval LineAlgebraicEvaluator::rangeAt(uint32_t node, double sa, double sb, const std::vector<int8_t>& signs,
                                         bool* undefined) const {
    const auto& n = _form->nodes[node];
    if (_signFree[node] && n.kind != Kind::Polynomial && n.kind != Kind::Undefined) {
        if (sa != _treeMemoS0 || sb != _treeMemoS1 || _treeMemoValid.size() != _signFree.size()) {
            _treeMemoS0 = sa;
            _treeMemoS1 = sb;
            _treeMemoRange.assign(_signFree.size(), Interval());
            _treeMemoValid.assign(_signFree.size(), 0);
        }
        if (_treeMemoValid[node]) { ++_memoHits; return _treeMemoRange[node]; }
        // Sign-free: signs cannot change this answer, and undefined cannot arise.
        static const std::vector<int8_t> kNoSigns;
        _treeMemoValid[node] = 1;
        ++_memoMisses;
        return _treeMemoRange[node] = rangeUnmemoized(node, sa, sb, kNoSigns, undefined);
    }
    return rangeUnmemoized(node, sa, sb, signs, undefined);
}

Interval LineAlgebraicEvaluator::rangeUnmemoized(uint32_t node, double sa, double sb, const std::vector<int8_t>& signs,
                                                 bool* undefined) const {
    const auto& n = _form->nodes[node];
    const int8_t s0 = n.switchCount > 0 ? signs[n.switchBegin] : 0;
    const int8_t s1 = n.switchCount > 1 ? signs[n.switchBegin + 1] : 0;
    switch (n.kind) {
        case Kind::Polynomial: return polynomialRange(node, sa, sb);
        case Kind::Undefined:
            if (!_undefinedAsZero && undefined) *undefined = true;
            return Interval(0.0f);
        case Kind::Add: return rangeAt(n.a, sa, sb, signs, undefined) + rangeAt(n.b, sa, sb, signs, undefined);
        case Kind::Sub: return rangeAt(n.a, sa, sb, signs, undefined) - rangeAt(n.b, sa, sb, signs, undefined);
        case Kind::Mul: return rangeAt(n.a, sa, sb, signs, undefined) * rangeAt(n.b, sa, sb, signs, undefined);
        case Kind::Sqrt:
            if (!n.nonnegativeArgument && s0 < 0) return Interval(0.0f);
            return sqrtOf(rangeAt(n.a, sa, sb, signs, undefined));
        case Kind::Abs: {
            const Interval r = rangeAt(n.a, sa, sb, signs, undefined);
            return s0 > 0 ? r : (s0 < 0 ? -r : absOf(r));
        }
        case Kind::Noise: {
            // The rounded f32 argument the CPU actually evaluates stays inside a
            // float-bounded box that encloses the real argument, so OntoMath's
            // box theorem covers it too.
            static const MathNode noiseOfQ = [] {
                MathNode q;
                q.op = MathNode::Op::ValueLeaf;
                q.variableName = "q";
                MathNode nz;
                nz.op = MathNode::Op::Noise;
                nz.children.push_back(std::make_unique<MathNode>(q));
                return nz;
            }();
            const Interval ra = rangeAt(n.a, sa, sb, signs, undefined);
            const Interval rb = rangeAt(n.b, sa, sb, signs, undefined);
            const Interval rc = rangeAt(n.c, sa, sb, signs, undefined);
            const auto r = noiseOfQ.evalRange({{"q", MathNode::RangeValue::makeVector(ra, rb, rc)}});
            if (!r || r->kind != ValueKind::Scalar)
                return Interval(-kClassicPerlin3ValueBound, kClassicPerlin3ValueBound);
            return r->scalar;
        }
        case Kind::Trans:
            // OntoMath's own bound (exact sin/cos extrema, monotone exp), given a
            // sound argument enclosure; scale/shift already live in that argument.
            return TransFactor(n.transKind, "u", 1.0, 0.0).evalRange(rangeAt(n.a, sa, sb, signs, undefined));
        case Kind::Min:
        case Kind::Max: {
            const bool min = n.kind == Kind::Min;
            if (s0 > 0) return rangeAt(min ? n.b : n.a, sa, sb, signs, undefined);   // a ≥ b
            if (s0 < 0) return rangeAt(min ? n.a : n.b, sa, sb, signs, undefined);   // a ≤ b
            const Interval a = rangeAt(n.a, sa, sb, signs, undefined), b = rangeAt(n.b, sa, sb, signs, undefined);
            return min ? minOf(a, b) : maxOf(a, b);
        }
        case Kind::Clamp: {
            const Interval lo = pointOrEnclose(n.lo), hi = pointOrEnclose(n.hi);
            if (s0 < 0) return lo;     // a ≤ lo
            if (s1 > 0) return hi;     // a ≥ hi
            const Interval a = rangeAt(n.a, sa, sb, signs, undefined);
            if (s0 > 0 && s1 < 0) {
                const Interval m = a.met(Interval(lo.lo, hi.hi));
                return m.empty() ? a : m;
            }
            // clamp is monotone in each argument (lo ≤ hi).
            return Interval(std::min(std::max(a.lo, lo.lo), hi.lo), std::min(std::max(a.hi, lo.hi), hi.hi));
        }
        case Kind::Select: {
            Interval acc;
            bool any = false;
            for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                const int st = pieceState(n, p, signs);
                if (st < 0) continue;
                const Interval v = rangeAt(n.pieces[p].value, sa, sb, signs, undefined);
                acc = any ? acc.joined(v) : v;
                any = true;
                if (st > 0) return acc;
            }
            // No piece certainly applies: falling through is possible.
            if (_undefinedAsZero) return any ? acc.joined(Interval(0.0f)) : Interval(0.0f);
            if (undefined) *undefined = true;
            return any ? acc : Interval(0.0f);
        }
    }
    return Interval::infinite();
}

Interval LineAlgebraicEvaluator::switchRange(std::size_t k, double s0, double s1,
                                             const std::vector<int8_t>& signs, bool* undefined) const {
    const auto& sw = _form->switches[k];
    const auto& n = _form->nodes[sw.node];
    const double sa = s0, sb = s1;
    switch (sw.role) {
        case 0: return rangeAt(n.a, sa, sb, signs, undefined);
        case 1: return rangeAt(n.a, sa, sb, signs, undefined) - rangeAt(n.b, sa, sb, signs, undefined);
        case 2: return rangeAt(n.a, sa, sb, signs, undefined) - pointOrEnclose(n.lo);
        case 3: return rangeAt(n.a, sa, sb, signs, undefined) - pointOrEnclose(n.hi);
        default: {
            const uint32_t p = (sw.role - 4) / 2;
            const Interval sel = polynomialRange(sw.node, sa, sb);
            return (sw.role % 2 == 0) ? sel - pointOrEnclose(n.pieces[p].lo) : pointOrEnclose(n.pieces[p].hi) - sel;
        }
    }
}

bool LineAlgebraicEvaluator::constantUnder(uint32_t node, const std::vector<int8_t>& signs) const {
    const auto& n = _form->nodes[node];
    const int8_t s0 = n.switchCount > 0 ? signs[n.switchBegin] : 0;
    const int8_t s1 = n.switchCount > 1 ? signs[n.switchBegin + 1] : 0;
    switch (n.kind) {
        case Kind::Polynomial: return n.polynomial.exactDegreeOn(_bindings) <= 0;
        case Kind::Undefined: return _undefinedAsZero;
        case Kind::Add: case Kind::Sub: case Kind::Mul:
            return constantUnder(n.a, signs) && constantUnder(n.b, signs);
        case Kind::Sqrt:
            if (!n.nonnegativeArgument && s0 < 0) return true;
            return constantUnder(n.a, signs);
        case Kind::Abs: case Kind::Trans: return constantUnder(n.a, signs);
        case Kind::Noise: return constantUnder(n.a, signs) && constantUnder(n.b, signs) && constantUnder(n.c, signs);
        case Kind::Min: case Kind::Max: {
            const bool min = n.kind == Kind::Min;
            if (s0 > 0) return constantUnder(min ? n.b : n.a, signs);
            if (s0 < 0) return constantUnder(min ? n.a : n.b, signs);
            return constantUnder(n.a, signs) && constantUnder(n.b, signs);
        }
        case Kind::Clamp:
            if (s0 < 0 || s1 > 0) return true;
            return constantUnder(n.a, signs);
        case Kind::Select:
            for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                const int st = pieceState(n, p, signs);
                if (st < 0) continue;
                if (st == 0) return false;
                return constantUnder(n.pieces[p].value, signs);
            }
            return _undefinedAsZero;
    }
    return false;
}

void LineAlgebraicEvaluator::relevantUndecided(uint32_t node, const std::vector<int8_t>& signs,
                                               std::vector<uint32_t>& out) const {
    const auto& n = _form->nodes[node];
    auto pushIfOpen = [&](uint32_t k) { if (signs[k] == 0) out.push_back(k); };
    const int8_t s0 = n.switchCount > 0 ? signs[n.switchBegin] : 0;
    const int8_t s1 = n.switchCount > 1 ? signs[n.switchBegin + 1] : 0;
    switch (n.kind) {
        case Kind::Polynomial: case Kind::Undefined: return;
        case Kind::Add: case Kind::Sub: case Kind::Mul:
            relevantUndecided(n.a, signs, out);
            relevantUndecided(n.b, signs, out);
            return;
        case Kind::Sqrt:
            if (!n.nonnegativeArgument) {
                if (s0 < 0) return;   // sqrt(max(a,0)) = 0: a is irrelevant
                pushIfOpen(n.switchBegin);
            }
            relevantUndecided(n.a, signs, out);
            return;
        case Kind::Abs:
            pushIfOpen(n.switchBegin);
            relevantUndecided(n.a, signs, out);
            return;
        case Kind::Trans:
            relevantUndecided(n.a, signs, out);
            return;
        case Kind::Noise:
            relevantUndecided(n.a, signs, out);
            relevantUndecided(n.b, signs, out);
            relevantUndecided(n.c, signs, out);
            return;
        case Kind::Min: case Kind::Max: {
            const bool min = n.kind == Kind::Min;
            if (s0 == 0) {
                pushIfOpen(n.switchBegin);
                relevantUndecided(n.a, signs, out);
                relevantUndecided(n.b, signs, out);
            } else {
                relevantUndecided((s0 > 0) == min ? n.b : n.a, signs, out);
            }
            return;
        }
        case Kind::Clamp:
            if (s0 < 0 || s1 > 0) return;   // saturated: nothing beneath can matter
            pushIfOpen(n.switchBegin);
            pushIfOpen(n.switchBegin + 1);
            relevantUndecided(n.a, signs, out);
            return;
        case Kind::Select: {
            uint32_t k = n.switchBegin;
            for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                const auto& piece = n.pieces[p];
                const uint32_t tests = (piece.hasLo ? 1 : 0) + (piece.hasHi ? 1 : 0);
                const int st = pieceState(n, p, signs);
                if (st >= 0) {
                    for (uint32_t t = 0; t < tests; ++t) pushIfOpen(k + t);
                    relevantUndecided(piece.value, signs, out);
                }
                k += tests;
                if (st > 0) return;
            }
            return;
        }
    }
}

Interval LineAlgebraicEvaluator::derivativeRange(uint32_t node, double sa, double sb,
                                                 const std::vector<int8_t>& signs) const {
    const auto& n = _form->nodes[node];
    const int8_t s0 = n.switchCount > 0 ? signs[n.switchBegin] : 0;
    const int8_t s1 = n.switchCount > 1 ? signs[n.switchBegin + 1] : 0;
    auto hull = [](const Interval& x, const Interval& y) { return x.joined(y); };
    auto D = [&](uint32_t c) { return derivativeRange(c, sa, sb, signs); };
    auto R = [&](uint32_t c) { return rangeAt(c, sa, sb, signs, nullptr); };
    switch (n.kind) {
        case Kind::Polynomial: {
            const auto& a = _coefficients[node];
            const auto& e = _errors[node];
            if (a.size() < 2) return Interval(0.0f);
            std::vector<double> da(a.size() - 1), de(a.size() - 1);
            constexpr double kEps = std::numeric_limits<double>::epsilon();
            for (std::size_t k = 1; k < a.size(); ++k) {
                da[k - 1] = double(k) * a[k];
                de[k - 1] = double(k) * e[k] + 2 * kEps * std::abs(da[k - 1]);
            }
            return centeredPolynomialRange(da, de, sa, sb);
        }
        case Kind::Undefined: return _undefinedAsZero ? Interval(0.0f) : Interval::infinite();
        case Kind::Add: return D(n.a) + D(n.b);
        case Kind::Sub: return D(n.a) - D(n.b);
        case Kind::Mul: return D(n.a) * R(n.b) + R(n.a) * D(n.b);
        case Kind::Sqrt: {
            if (!n.nonnegativeArgument && s0 < 0) return Interval(0.0f);
            if (n.nonnegativeArgument && !n.normComponents.empty()) {
                // Cauchy–Schwarz: |d‖v‖/ds| ≤ ‖v′‖, valid everywhere (‖·‖ is 1-Lipschitz).
                double sum = 0.0;
                for (uint32_t c : n.normComponents) {
                    const Interval d = D(c);
                    if (!d.bounded()) return Interval::infinite();
                    const double m = std::max(std::abs(double(d.lo)), std::abs(double(d.hi)));
                    sum += m * m;
                }
                const float bound = Interval::roundUp(static_cast<float>(std::sqrt(sum) * (1.0 + 1e-12)));
                return Interval(-bound, bound);
            }
            const Interval r = R(n.a);
            if (!(r.lo > 0.0f)) return Interval::infinite();   // sqrt is not Lipschitz at 0
            return D(n.a) / (sqrtOf(r) * Interval(2.0f));
        }
        case Kind::Abs: {
            const Interval d = D(n.a);
            if (s0 > 0) return d;
            if (s0 < 0) return -d;
            return hull(d, -d);
        }
        case Kind::Min: case Kind::Max: {
            const bool min = n.kind == Kind::Min;
            if (s0 > 0) return D(min ? n.b : n.a);
            if (s0 < 0) return D(min ? n.a : n.b);
            return hull(D(n.a), D(n.b));
        }
        case Kind::Clamp:
            if (s0 < 0 || s1 > 0) return Interval(0.0f);
            if (s0 > 0 && s1 < 0) return D(n.a);
            return hull(D(n.a), Interval(0.0f));
        case Kind::Trans: {
            const Interval arg = R(n.a), darg = D(n.a);
            using TK = TransFactor::Kind;
            if (n.transKind == TK::Sin) return TransFactor(TK::Cos, "u", 1.0, 0.0).evalRange(arg) * darg;
            if (n.transKind == TK::Cos) return -(TransFactor(TK::Sin, "u", 1.0, 0.0).evalRange(arg)) * darg;
            if (n.transKind == TK::Exp) return TransFactor(TK::Exp, "u", 1.0, 0.0).evalRange(arg) * darg;
            return Interval::infinite();
        }
        case Kind::Noise: {
            // |d/ds noise(q(s))| ≤ L·|q′(s)|: OntoMath's proved Lipschitz bound.
            const Interval da = D(n.a), db = D(n.b), dc = D(n.c);
            if (!da.bounded() || !db.bounded() || !dc.bounded()) return Interval::infinite();
            auto mag = [](const Interval& x) { return std::max(std::abs(x.lo), std::abs(x.hi)); };
            const float norm = Interval::roundUp(std::sqrt(mag(da) * mag(da) + mag(db) * mag(db) + mag(dc) * mag(dc)));
            const float bound = Interval::roundUp(kClassicPerlin3LipschitzBound * norm);
            return Interval(-bound, bound);
        }
        case Kind::Select: {
            for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                const int st = pieceState(n, p, signs);
                if (st < 0) continue;
                if (st == 0) return Interval::infinite();   // a possible jump: no Lipschitz claim
                return D(n.pieces[p].value);
            }
            return _undefinedAsZero ? Interval(0.0f) : Interval::infinite();
        }
    }
    return Interval::infinite();
}

Interval LineAlgebraicEvaluator::switchDerivativeRange(std::size_t k, double sa, double sb,
                                                       const std::vector<int8_t>& signs) const {
    const auto& sw = _form->switches[k];
    const auto& n = _form->nodes[sw.node];
    switch (sw.role) {
        case 0: case 2: case 3: return derivativeRange(n.a, sa, sb, signs);
        case 1: return derivativeRange(n.a, sa, sb, signs) - derivativeRange(n.b, sa, sb, signs);
        default: {
            // Select bound on the selector polynomial: sel′ or −sel′.
            const auto& a = _coefficients[sw.node];
            if (a.size() < 2) return Interval(0.0f);
            std::vector<double> da(a.size() - 1), de(a.size() - 1);
            for (std::size_t q = 1; q < a.size(); ++q) {
                da[q - 1] = double(q) * a[q];
                de[q - 1] = double(q) * _errors[sw.node][q] + 4 * std::numeric_limits<double>::epsilon() * std::abs(da[q - 1]);
            }
            const Interval d = centeredPolynomialRange(da, de, sa, sb);
            return sw.role % 2 == 0 ? d : -d;
        }
    }
}

std::optional<double> lipschitzBoundOverBox(const LineAlgebraicForm& form, std::size_t switchIndex,
                                            const Interval box[3],
                                            const std::map<std::string, Interval>& others,
                                            bool undefinedAsZero) {
    if (switchIndex >= form.switches.size()) return std::nullopt;
    double sum = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        std::map<std::string, Interval> b = others;
        for (int i = 0; i < 3; ++i) {
            b[kLineOriginVars[i]] = box[i];
            b[kLineDirectionVars[i]] = Interval(i == axis ? 1.0f : 0.0f);
        }
        LineAlgebraicEvaluator ev(form, b, undefinedAsZero);
        if (!ev.ok()) return std::nullopt;
        const std::vector<int8_t> none(form.switches.size(), 0);
        // d/ds g(o + s·e_axis) at s = 0, for every o in the box: ∂g/∂x_axis.
        const Interval d = ev.switchDerivativeRange(switchIndex, 0.0, 0.0, none);
        if (!d.bounded()) return std::nullopt;
        const double m = std::max(std::abs(double(d.lo)), std::abs(double(d.hi)));
        sum += m * m;
    }
    return std::sqrt(sum) * (1.0 + 1e-12);
}

namespace {
// Real roots of a·s² + b·s + c in (s0, s1), numerically stable form; a nearly
// tangent pair (tiny negative discriminant) proposes the vertex.
void quadraticRoots(double a, double b, double c, double s0, double s1, std::vector<double>& out) {
    auto push = [&](double r) { if (std::isfinite(r) && r > s0 && r < s1) out.push_back(r); };
    const double scale = std::max({std::abs(a), std::abs(b), std::abs(c)});
    if (!(scale > 0.0) || !std::isfinite(scale)) return;
    if (std::abs(a) <= 1e-14 * scale) { if (b != 0.0) push(-c / b); return; }
    const double disc = b * b - 4.0 * a * c;
    if (disc < 0.0) {
        if (-disc <= 1e-12 * std::max(b * b, std::abs(4.0 * a * c))) push(-b / (2.0 * a));
        return;
    }
    const double q = -0.5 * (b + std::copysign(std::sqrt(disc), b));
    push(q / a);
    if (q != 0.0) push(c / q);
}

struct AffineOver { double alpha = 0.0, beta = 0.0; int64_t base = -1; };
} // namespace

void LineAlgebraicEvaluator::proposeRoots(double s0, double s1, std::vector<double>& out) const {
    const auto& nodes = _form->nodes;
    // g restricted to the form α·B + β, B a Polynomial node or Sqrt of one.
    std::function<std::optional<AffineOver>(uint32_t)> affineOf = [&](uint32_t id) -> std::optional<AffineOver> {
        const auto& n = nodes[id];
        switch (n.kind) {
            case Kind::Polynomial:
                if (n.polynomial.exactDegreeOn(_bindings) <= 0)
                    return AffineOver{0.0, _coefficients[id].empty() ? 0.0 : _coefficients[id][0], -1};
                return AffineOver{1.0, 0.0, id};
            case Kind::Sqrt:
                if (nodes[n.a].kind == Kind::Polynomial) return AffineOver{1.0, 0.0, id};
                return std::nullopt;
            case Kind::Add: case Kind::Sub: {
                const auto A = affineOf(n.a), B = affineOf(n.b);
                if (!A || !B) return std::nullopt;
                const double sg = n.kind == Kind::Add ? 1.0 : -1.0;
                if (B->base == -1) return AffineOver{A->alpha, A->beta + sg * B->beta, A->base};
                if (A->base == -1) return AffineOver{sg * B->alpha, A->beta + sg * B->beta, B->base};
                if (A->base == B->base) return AffineOver{A->alpha + sg * B->alpha, A->beta + sg * B->beta, A->base};
                return std::nullopt;
            }
            case Kind::Mul: {
                const auto A = affineOf(n.a), B = affineOf(n.b);
                if (!A || !B) return std::nullopt;
                if (A->base == -1) return AffineOver{A->beta * B->alpha, A->beta * B->beta, B->base};
                if (B->base == -1) return AffineOver{B->beta * A->alpha, B->beta * A->beta, A->base};
                return std::nullopt;
            }
            default: return std::nullopt;
        }
    };
    auto solve = [&](const std::vector<double>& c, double target) {
        if (c.size() > 3) return;   // only degree ≤ 2 in closed form
        const double c0 = (c.empty() ? 0.0 : c[0]) - target;
        const double c1 = c.size() > 1 ? c[1] : 0.0, c2 = c.size() > 2 ? c[2] : 0.0;
        quadraticRoots(c2, c1, c0, s0, s1, out);
    };
    for (const auto& sw : _form->switches) {
        const auto& n = nodes[sw.node];
        if (sw.role >= 4) {   // Select bound: selector(s) = lo_i / hi_i
            const auto& piece = n.pieces[(sw.role - 4) / 2];
            solve(_coefficients[sw.node], sw.role % 2 == 0 ? piece.lo : piece.hi);
            continue;
        }
        std::optional<AffineOver> g = affineOf(n.a);
        double c = 0.0;
        if (sw.role == 1) {
            const auto B = affineOf(n.b);
            if (!g || !B) continue;
            if (B->base == -1) g = AffineOver{g->alpha, g->beta - B->beta, g->base};
            else if (g->base == -1) g = AffineOver{-B->alpha, g->beta - B->beta, B->base};
            else if (g->base == B->base) g = AffineOver{g->alpha - B->alpha, g->beta - B->beta, g->base};
            else continue;
        }
        if (sw.role == 2) c = n.lo;
        if (sw.role == 3) c = n.hi;
        if (!g || g->base < 0 || g->alpha == 0.0) continue;
        const double target = (c - g->beta) / g->alpha;   // B(s) = target
        const auto& base = nodes[static_cast<std::size_t>(g->base)];
        if (base.kind == Kind::Polynomial) solve(_coefficients[static_cast<std::size_t>(g->base)], target);
        else if (target >= 0.0) solve(_coefficients[base.a], target * target);   // sqrt(Q) = t ⇔ Q = t², t ≥ 0
    }
}

namespace {
struct Segmenter {
    const std::vector<const LineAlgebraicEvaluator*>& evaluators;
    double resolution;
    uint32_t maxEvaluations;
    LineSegmentationStats stats;
    std::vector<LineSegmentationPiece> pieces;
    std::vector<std::pair<std::size_t, uint32_t>> targets;

    // Adjacent pieces merge only when switches AND targets agree: a proved-empty
    // stretch must never be absorbed into a neighbouring positive one.
    void emit(double a, double b, bool definite, const std::vector<std::vector<int8_t>>& signs,
              const std::vector<int8_t>& targetSigns) {
        if (!pieces.empty() && pieces.back().definite == definite && pieces.back().signs == signs &&
            pieces.back().targetSigns == targetSigns) {
            pieces.back().s1 = b;
            return;
        }
        pieces.push_back({a, b, definite, signs, targetSigns});
    }

    void classify(double a, double b, std::vector<std::vector<int8_t>> signs,
                  std::vector<int8_t> targetSigns = {}) {
        if (targetSigns.size() != targets.size()) targetSigns.assign(targets.size(), 0);
        for (;;) {
            bool progress = false, pending = false;
            for (std::size_t t = 0; t < targets.size(); ++t) {
                if (targetSigns[t] != 0) continue;
                if (stats.rangeEvaluations >= maxEvaluations) {
                    stats.budgetExhausted = true;
                    emit(a, b, false, signs, targetSigns);
                    return;
                }
                ++stats.rangeEvaluations;
                bool undefined = false;
                const auto& [e, node] = targets[t];
                const Interval r = evaluators[e]->range(node, a, b, signs[e], &undefined);
                if (undefined || std::isnan(r.lo) || std::isnan(r.hi)) { pending = true; continue; }
                // Execution-faithful decision: beyond what the device may err by.
                const double sl = evaluators[e]->slack(node, a, b, signs[e]);
                if (double(r.hi) + sl <= 0.0) { targetSigns[t] = -1; progress = true; }
                else if (double(r.lo) - sl > 0.0) { targetSigns[t] = 1; progress = true; }
                else pending = true;
            }
            for (std::size_t e = 0; e < evaluators.size(); ++e) {
                const auto& ev = *evaluators[e];
                std::vector<uint32_t> relevant;
                for (uint32_t root : ev.form().roots) ev.relevantUndecided(root, signs[e], relevant);
                std::sort(relevant.begin(), relevant.end());
                relevant.erase(std::unique(relevant.begin(), relevant.end()), relevant.end());
                for (uint32_t k : relevant) {
                    if (stats.rangeEvaluations >= maxEvaluations) {
                        stats.budgetExhausted = true;
                        emit(a, b, false, signs, targetSigns);
                        return;
                    }
                    ++stats.rangeEvaluations;
                    bool undefined = false;
                    const Interval r = ev.switchRange(k, a, b, signs[e], &undefined);
                    const bool strict = ev.form().switches[k].strict;
                    if (undefined || std::isnan(r.lo) || std::isnan(r.hi)) { pending = true; continue; }
                    const double sl = ev.switchSlack(k, a, b, signs[e]);
                    const double lo = double(r.lo) - sl, hi = double(r.hi) + sl;
                    if (strict ? lo > 0.0 : lo >= 0.0) { signs[e][k] = 1; progress = true; }
                    else if (strict ? hi < 0.0 : hi <= 0.0) { signs[e][k] = -1; progress = true; }
                    else pending = true;
                }
            }
            if (!pending) { emit(a, b, true, signs, targetSigns); return; }
            if (!progress) break;
        }
        const double mid = 0.5 * (a + b);
        if (!(b - a > resolution) || !(mid > a && mid < b)) { emit(a, b, false, signs, targetSigns); return; }
        // Decided signs (switches and targets) were proved over [a, b], so they
        // hold on both halves and are inherited; pending ones are retried there.
        classify(a, mid, signs, targetSigns);
        classify(mid, b, std::move(signs), std::move(targetSigns));
    }
};
} // namespace

std::vector<LineSegmentationPiece> segmentLine(const std::vector<const LineAlgebraicEvaluator*>& evaluators,
                                               double s0, double s1, double resolution,
                                               LineSegmentationStats* stats, uint32_t maxRangeEvaluations,
                                               bool analyticProposals,
                                               const std::vector<std::vector<int8_t>>* initialSigns,
                                               const std::vector<std::pair<std::size_t, uint32_t>>* signTargets) {
    Segmenter seg{evaluators, resolution, maxRangeEvaluations, {}, {}, {}};
    if (signTargets) seg.targets = *signTargets;
    std::vector<std::vector<int8_t>> signs;
    for (std::size_t e = 0; e < evaluators.size(); ++e) {
        const std::size_t n = evaluators[e]->form().switches.size();
        if (initialSigns && e < initialSigns->size() && (*initialSigns)[e].size() == n) signs.push_back((*initialSigns)[e]);
        else signs.emplace_back(n, int8_t(0));
    }
    if (!(std::isfinite(s0) && std::isfinite(s1) && s1 >= s0)) {
        if (stats) *stats = seg.stats;
        return seg.pieces;
    }
    std::vector<double> roots;
    if (analyticProposals) {
        for (const auto* e : evaluators) e->proposeRoots(s0, s1, roots);
        std::sort(roots.begin(), roots.end());
        std::vector<double> merged;
        for (double r : roots)
            if (merged.empty() || r - merged.back() > resolution) merged.push_back(r);
        roots.swap(merged);
    }
    seg.stats.proposedRoots = static_cast<uint32_t>(roots.size());
    // Cut at each proposal: a window of one resolution around it, and the gaps
    // between. Every piece is still classified (and bisected if needed) by proof.
    const double w = 0.5 * resolution;
    double cur = s0;
    for (double r : roots) {
        const double lo = std::max(cur, r - w), hi = std::min(s1, r + w);
        if (lo > cur) seg.classify(cur, lo, signs);
        if (hi > lo) seg.classify(lo, hi, signs);
        cur = std::max(cur, hi);
    }
    if (s1 > cur || seg.pieces.empty()) seg.classify(cur, s1, signs);
    if (stats) *stats = seg.stats;
    return seg.pieces;
}

namespace {
// One branch of a line form, as the integration operation holds it.
struct LineIntegrand {
    std::vector<double> poly;                     // Σ a_k s^k
    struct Radical { std::vector<double> poly; double A, B, C; };   // R(s)·√(A s² + B s + C)
    std::vector<Radical> radicals;
    struct Wave { std::vector<double> poly; TransFactor::Kind kind; double alpha, beta; };   // P(s)·g(αs + β)
    std::vector<Wave> waves;
    // Hybrid integration (2026-10-19): w(s)·G(s) where G is an authored node with
    // no closed form here (noise, an elliptic product) sampled numerically, and
    // w is closed-form. Only hybridIntegral admits these.
    struct Remainder { uint32_t node; std::shared_ptr<LineIntegrand> weight; };
    std::vector<Remainder> remainders;
    bool constant() const {
        return radicals.empty() && waves.empty() && remainders.empty() && poly.size() <= 1;
    }
    double constantValue() const { return poly.empty() ? 0.0 : poly[0]; }
};

std::vector<double> polyTimes(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<double> out(a.size() + b.size() - 1, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j) out[i + j] += a[i] * b[j];
    return out;
}
void polyAdd(std::vector<double>& a, const std::vector<double>& b, double k) {
    if (a.size() < b.size()) a.resize(b.size(), 0.0);
    for (std::size_t i = 0; i < b.size(); ++i) a[i] += k * b[i];
}
void scaleIntegrand(LineIntegrand& f, double k) {
    for (auto& a : f.poly) a *= k;
    for (auto& r : f.radicals) for (auto& a : r.poly) a *= k;
    for (auto& r : f.remainders) { auto w = std::make_shared<LineIntegrand>(*r.weight); scaleIntegrand(*w, k); r.weight = w; }
    for (auto& w : f.waves) for (auto& a : w.poly) a *= k;
}

} // namespace

namespace {
// The exact product of two closed-form integrands (no remainders), or nullopt
// with the reason. Value-branch segmentation (2026-10-19) widened the products:
//  · (P₁ + R₁√Q)(P₂ + R₂√Q) = P₁P₂ + R₁R₂·Q + (P₁R₂ + P₂R₁)·√Q for one Q
//    (a squared norm such as (√Q − R)² in a torus, a shell's falloff times a
//    polynomial);
//  · products of waves by product-to-sum: sin·cos = ½[sin(u+v) + sin(u−v)], and
//    so on; exp·exp = exp(u + v) (a gyroid is Σ sin·cos).
// Still refused: two different square roots (√(Q₁Q₂) is an elliptic integral in
// general), a square root times a wave, exp times sin/cos.
std::optional<LineIntegrand> multiplyExact(const LineIntegrand& a, const LineIntegrand& b, std::string& reason) {
    if (a.constant()) { LineIntegrand f = b; scaleIntegrand(f, a.constantValue()); return f; }
    if (b.constant()) { LineIntegrand f = a; scaleIntegrand(f, b.constantValue()); return f; }
    const bool aWaves = !a.waves.empty(), bWaves = !b.waves.empty();
    const bool aRoots = !a.radicals.empty(), bRoots = !b.radicals.empty();
    if ((aRoots && bWaves) || (bRoots && aWaves))
        { reason = "a square root times a wave (no elementary closed form here)"; return std::nullopt; };
    LineIntegrand f;
    if (aRoots || bRoots) {
        const LineIntegrand::Radical* root = nullptr;
        auto oneRoot = [&](const LineIntegrand& g, std::vector<double>& R) {
            for (const auto& q : g.radicals) {
                if (root && (q.A != root->A || q.B != root->B || q.C != root->C)) return false;
                root = &q;
                polyAdd(R, q.poly, 1.0);
            }
            return true;
        };
        std::vector<double> R1, R2;
        if (!oneRoot(a, R1) || !oneRoot(b, R2))
            { reason = "a product of two different square roots (an elliptic integral in general)"; return std::nullopt; };
        const LineIntegrand::Radical q = *root;
        f.poly = polyTimes(a.poly, b.poly);
        polyAdd(f.poly, polyTimes(polyTimes(R1, R2), {q.C, q.B, q.A}), 1.0);
        std::vector<double> R = polyTimes(a.poly, R2);
        polyAdd(R, polyTimes(b.poly, R1), 1.0);
        if (!R.empty()) f.radicals.push_back({R, q.A, q.B, q.C});
        return f;
    }
    f.poly = polyTimes(a.poly, b.poly);
    for (auto w : b.waves) { w.poly = polyTimes(a.poly, w.poly); if (!w.poly.empty()) f.waves.push_back(w); }
    for (auto w : a.waves) { w.poly = polyTimes(b.poly, w.poly); if (!w.poly.empty()) f.waves.push_back(w); }
    using TK = TransFactor::Kind;
    for (const auto& u : a.waves)
        for (const auto& v : b.waves) {
            const std::vector<double> P = polyTimes(u.poly, v.poly);
            if (P.empty()) continue;
            const bool uExp = u.kind == TK::Exp, vExp = v.kind == TK::Exp;
            // The office integrates exp·sin/cos; a Wave cannot carry both factors yet.
            if (uExp != vExp) { reason = "an exponential times a sine or cosine (a line Wave carries one factor)"; return std::nullopt; };
            // g(x)·h(y) = Σ c·G: the identities live once, in the office.
            const auto sum = productToSum(TransFactor(u.kind, "s", u.alpha, u.beta), TransFactor(v.kind, "s", v.alpha, v.beta));
            if (!sum) { reason = "a product of waves the office has no identity for"; return std::nullopt; }
            for (const auto& [c, g] : *sum) {
                std::vector<double> scaled = P;
                for (auto& x : scaled) x *= c;
                f.waves.push_back({scaled, g.kind, g.scale, g.shift});
            }
        }
    return f;
}

// ∫[s0, s1] of the closed-form part of f (its remainders are the caller's).
// The rules are the one office's (ScalarForm.hpp, 2026-10-19; Zach: "build it on
// top of antiderivatives so we dont have two competing antiderivative code"):
// the polynomial and waves convert to a ScalarForm in s and integrate through
// ScalarForm::antiderivative, evaluated stably; the radicals through
// definiteIntegralTimesRadical. Nothing here knows an antiderivative.
std::optional<double> integrateExact(const LineIntegrand& f, double s0, double s1, std::string* why) {
    static const std::string kS = "s";
    ScalarForm form = polynomialForm(f.poly, kS);
    for (const auto& w : f.waves)
        form = form.plus(polynomialForm(w.poly, kS).times(ScalarForm::transcendental(w.kind, kS, w.alpha, w.beta)));
    const auto closed = definiteIntegral(form, kS, s0, s1, {}, why);
    if (!closed) return std::nullopt;
    double total = *closed;
    for (const auto& r : f.radicals) {
        const auto v = definiteIntegralTimesRadical(r.poly, r.A, r.B, r.C, s0, s1, why);
        if (!v) return std::nullopt;
        total += *v;
    }
    if (!std::isfinite(total)) { if (why) *why = "the closed form is not finite on the interval"; return std::nullopt; }
    return total;
}

// sup |f| over [s0, s1], an upper bound: each term is centred on the interval
// (|Σ b_k u^k| ≤ Σ |b_k| h^k), |sin|, |cos| ≤ 1, exp at the larger end.
double supAbs(const LineIntegrand& f, double s0, double s1) {
    const double m = 0.5 * (s0 + s1), h = 0.5 * (s1 - s0);
    auto polyBound = [&](const std::vector<double>& a) {
        const std::vector<double> b = shiftPolynomial(a, m);
        double sum = 0.0, hp = 1.0;
        for (double c : b) { sum += std::abs(c) * hp; hp *= h; }
        return sum;
    };
    double total = polyBound(f.poly);
    for (const auto& r : f.radicals)
        total += polyBound(r.poly) * std::sqrt(polyBound({r.C, r.B, r.A}));
    for (const auto& w : f.waves) {
        const double g = w.kind == TransFactor::Kind::Exp
            ? std::exp(std::max(w.alpha * s0 + w.beta, w.alpha * s1 + w.beta)) : 1.0;
        total += polyBound(w.poly) * g;
    }
    return total * (1.0 + 1e-12) + 1e-300;
}

// f(m + v) as an integrand in v: a panel's own coordinates, so products with a
// panel's interpolation basis stay small (no s^deg coefficients far from 0).
LineIntegrand shiftIntegrand(const LineIntegrand& f, double m) {
    LineIntegrand g;
    g.poly = shiftPolynomial(f.poly, m);
    for (const auto& r : f.radicals)
        g.radicals.push_back({shiftPolynomial(r.poly, m), r.A, 2.0 * r.A * m + r.B, (r.A * m + r.B) * m + r.C});
    for (const auto& w : f.waves) g.waves.push_back({shiftPolynomial(w.poly, m), w.kind, w.alpha, w.alpha * m + w.beta});
    return g;
}
} // namespace

std::optional<double> LineAlgebraicEvaluator::integral(uint32_t root, double s0, double s1,
                                                       const std::vector<int8_t>& signs, std::string* why) const {
    const auto r = integrateCore(root, s0, s1, signs, 0, nullptr, why);
    if (!r) return std::nullopt;
    return r->estimate;
}

std::optional<LineHybridIntegral> LineAlgebraicEvaluator::hybridIntegral(uint32_t root, double s0, double s1,
                                                                         const std::vector<int8_t>& signs,
                                                                         uint32_t cells, std::string* why) const {
    return integrateCore(root, s0, s1, signs, std::max<uint32_t>(cells, 1), nullptr, why);
}

std::optional<LineHybridIntegral> LineAlgebraicEvaluator::hybridIntegral(uint32_t root, double s0, double s1,
                                                                         const std::vector<int8_t>& signs, uint32_t cells,
                                                                         const LineSampledFactor& factor,
                                                                         std::string* why) const {
    return integrateCore(root, s0, s1, signs, std::max<uint32_t>(cells, 1), &factor, why);
}

std::optional<LineHybridIntegral> LineAlgebraicEvaluator::integrateCore(uint32_t root, double s0, double s1,
                                                                        const std::vector<int8_t>& signs,
                                                                        uint32_t cells, const LineSampledFactor* factor,
                                                                        std::string* why) const {
    if (_boxMode) { if (why) *why = "the integral needs a concrete line, not box bindings"; return std::nullopt; }
    const bool hybrid = cells > 0;
    std::string reason;
    // A switch the caller's signs leave open is decided here when its proved
    // range over [s0, s1] is one-sided (continuous branches agree at zero, so
    // ≤/≥ suffice; a Select bound is STRICT). One that changes sign inside the
    // interval is refused: the caller must split there (or, hybrid, sample it).
    std::vector<int8_t> decided = signs;
    auto decide = [&](uint32_t k) -> int8_t {
        if (decided[k] != 0) return decided[k];
        bool undefined = false;
        const Interval r = switchRange(k, s0, s1, decided, &undefined);
        if (undefined) return 0;
        const bool strict = _form->switches[k].strict;
        if (strict ? double(r.hi) < 0.0 : double(r.hi) <= 0.0) return decided[k] = -1;
        if (strict ? double(r.lo) > 0.0 : double(r.lo) >= 0.0) return decided[k] = 1;
        return 0;
    };
    std::function<std::optional<LineIntegrand>(uint32_t)> build;
    // Hybrid: a node with no closed form becomes a remainder of weight 1, to be
    // sampled; its parents carry it through every exact operation they can.
    auto buildOrRemainder = [&](uint32_t node) -> std::optional<LineIntegrand> {
        auto f = build(node);
        if (f || !hybrid) return f;
        LineIntegrand g;
        auto one = std::make_shared<LineIntegrand>();
        one->poly = {1.0};
        g.remainders.push_back({node, one});
        return g;
    };
    build = [&](uint32_t node) -> std::optional<LineIntegrand> {
        const auto& n = _form->nodes[node];
        auto refuse = [&](const std::string& r) -> std::optional<LineIntegrand> {
            if (reason.empty()) reason = r;
            return std::nullopt;
        };
        for (uint32_t k = n.switchBegin; k < n.switchBegin + n.switchCount; ++k) decide(k);
        const int8_t sw0 = n.switchCount > 0 ? decided[n.switchBegin] : 0;
        const int8_t sw1 = n.switchCount > 1 ? decided[n.switchBegin + 1] : 0;
        switch (n.kind) {
            case Kind::Polynomial: { LineIntegrand f; f.poly = _coefficients[node]; return f; }
            case Kind::Undefined:
                if (_undefinedAsZero) return LineIntegrand{};
                return refuse("Undefined on the interval");
            case Kind::Noise: return refuse("Perlin noise has no closed-form integral");
            case Kind::Select:
                for (uint32_t p = 0; p < n.pieces.size(); ++p) {
                    const int st = pieceState(n, p, decided);
                    if (st < 0) continue;
                    if (st == 0) return refuse("a Select piece changes inside the interval (split at its bound)");
                    return buildOrRemainder(n.pieces[p].value);
                }
                if (_undefinedAsZero) return LineIntegrand{};
                return refuse("no Select piece applies (Undefined)");
            // Value-branch segmentation (2026-10-19): a branch node decides first and
            // builds only the operand its decided branch reads, exactly as
            // relevantUndecided prunes. A saturated clamp or the unchosen side of a
            // min/max may hold switches (or noise) segmentLine rightly left open.
            case Kind::Clamp:
                if (sw0 < 0) { LineIntegrand f; f.poly = {n.lo}; return f; }
                if (sw1 > 0) { LineIntegrand f; f.poly = {n.hi}; return f; }
                if (sw0 > 0 && sw1 < 0) return buildOrRemainder(n.a);
                return refuse("clamp changes branch inside the interval (split at its root)");
            case Kind::Min: case Kind::Max: {
                if (sw0 == 0) return refuse("min/max changes branch inside the interval (split at its root)");
                const bool min = n.kind == Kind::Min;
                // sign > 0: a ≥ b.
                return buildOrRemainder(sw0 > 0 ? (min ? n.b : n.a) : (min ? n.a : n.b));
            }
            case Kind::Sqrt:
                if (!n.nonnegativeArgument && sw0 < 0) return LineIntegrand{};   // sqrt(max(a, 0)) = 0
                break;
            default: break;
        }
        // Sqrt and Trans of a remainder are not linear in it: the node itself is
        // then the remainder (buildOrRemainder above it), so build its operand
        // exactly only.
        const bool linear = n.kind == Kind::Abs || n.kind == Kind::Add || n.kind == Kind::Sub || n.kind == Kind::Mul;
        auto a = linear ? buildOrRemainder(n.a) : build(n.a);
        if (!a) return std::nullopt;
        switch (n.kind) {
            case Kind::Abs:
                if (sw0 == 0) return refuse("abs changes branch inside the interval (split at its root)");
                if (sw0 < 0) scaleIntegrand(*a, -1.0);
                return a;
            case Kind::Sqrt: {
                if (!a->radicals.empty() || !a->waves.empty() || a->poly.size() > 3)
                    return refuse("square root of a non-quadratic (e.g. a torus's nested norm)");
                std::vector<double> q = a->poly;
                q.resize(3, 0.0);
                LineIntegrand f;
                f.radicals.push_back({{1.0}, q[2], q[1], q[0]});
                return f;
            }
            case Kind::Trans: {
                if (!a->radicals.empty() || !a->waves.empty() || a->poly.size() > 2)
                    return refuse("transcendental of a non-affine argument");
                std::vector<double> arg = a->poly;
                arg.resize(2, 0.0);
                LineIntegrand f;
                f.waves.push_back({{1.0}, n.transKind, arg[1], arg[0]});
                return f;
            }
            default: break;
        }
        auto b = buildOrRemainder(n.b);
        if (!b) return std::nullopt;
        switch (n.kind) {
            case Kind::Add: case Kind::Sub: {
                const double k = n.kind == Kind::Add ? 1.0 : -1.0;
                polyAdd(a->poly, b->poly, k);
                for (auto r : b->radicals) { for (auto& c : r.poly) c *= k; a->radicals.push_back(r); }
                for (auto w : b->waves) { for (auto& c : w.poly) c *= k; a->waves.push_back(w); }
                for (auto r : b->remainders) {
                    auto w = std::make_shared<LineIntegrand>(*r.weight);
                    scaleIntegrand(*w, k);
                    a->remainders.push_back({r.node, w});
                }
                return a;
            }
            case Kind::Mul: {
                // (E_a + Σ w·G)(E_b): every remainder's weight times the other's exact
                // part. Two remainders multiplied are not linear: the product is one.
                if (!a->remainders.empty() && !b->remainders.empty())
                    return refuse("a product of two remainders (sampled as one)");
                const LineIntegrand& withR = a->remainders.empty() ? *b : *a;
                const LineIntegrand& other = a->remainders.empty() ? *a : *b;
                LineIntegrand exactPart = withR;
                exactPart.remainders.clear();
                auto f = multiplyExact(exactPart, other, reason);
                if (!f) return std::nullopt;
                for (const auto& r : withR.remainders) {
                    auto w = multiplyExact(*r.weight, other, reason);
                    if (!w) return std::nullopt;
                    f->remainders.push_back({r.node, std::make_shared<LineIntegrand>(std::move(*w))});
                }
                return f;
            }
            default: break;
        }
        return refuse("node kind not integrable");
    };
    const auto f = hybrid ? buildOrRemainder(root) : build(root);
    if (!f) { if (why) *why = reason; return std::nullopt; }
    LineHybridIntegral out;
    out.remainderTerms = static_cast<uint32_t>(f->remainders.size());
    // The terms to sample: every remainder w·G, and — with a sampled factor M —
    // the closed-form part too, as weight E times G ≡ 1 (then each sampled
    // function is G·M). Without a factor the closed form integrates exactly.
    constexpr uint32_t kNoNode = std::numeric_limits<uint32_t>::max();
    std::vector<LineIntegrand::Remainder> terms = f->remainders;
    if (factor) {
        auto e = std::make_shared<LineIntegrand>(*f);
        e->remainders.clear();
        if (!e->poly.empty() || !e->radicals.empty() || !e->waves.empty()) terms.push_back({kNoNode, e});
    } else {
        const auto exact = integrateExact(*f, s0, s1, why);
        if (!exact) return std::nullopt;
        out.estimate = *exact;
    }
    // Product Simpson rule (product integration): G sampled at the `cells` + 1
    // cell edges and interpolated by a quadratic over each pair of cells (a
    // cubic over the last three when the count is odd, a line for one cell);
    // ∫ w·(that interpolant) is exact, so a sharp
    // closed-form weight costs nothing and only G's own smoothness errs — fourth
    // order where G is smooth, against the plain midpoint sum's second.
    // Enclosure per panel: |∫ w(G − P)| ≤ sup|w|·∫|G − P|, and with c = G at the
    // panel's centre and Λ the interpolant's Lebesgue constant (1, 1.25, 1.6311
    // for 2, 3, 4 equispaced nodes), |G − P| ≤ |G − c| + Λ·max|G_i − c|
    // ≤ (1 + Λ)·min(range width, L′·half-width). With a sampled factor M, G is
    // G·M: its range is the product's, its derivative bound |G′|·sup|M| + sup|G|·|M′|.
    const double h = (s1 - s0) / double(std::max<uint32_t>(cells, 1));
    auto edge = [&](uint32_t j) { return j == cells ? s1 : s0 + h * j; };
    std::vector<double> m(cells + 1, 1.0);
    if (factor && !terms.empty()) {
        for (uint32_t j = 0; j <= cells; ++j) {
            const auto v = factor->value(edge(j));
            ++out.factorEvaluations;
            if (!v || !std::isfinite(*v)) { if (why) *why = "the sampled factor is undefined on the interval"; return std::nullopt; }
            m[j] = *v;
        }
    }
    for (const auto& r : terms) {
        std::vector<double> g(cells + 1);
        for (uint32_t j = 0; j <= cells; ++j) {
            double v = 1.0;
            if (r.node != kNoNode) {
                const auto gv = value(r.node, edge(j));
                ++out.remainderEvaluations;
                if (!gv) { if (why) *why = "the remainder is Undefined on the interval"; return std::nullopt; }
                v = *gv;
            }
            g[j] = v * m[j];
        }
        // ∫ w·L over [a, b] for the Lagrange basis polynomial L of the given nodes,
        // in the panel's own coordinate v = s − centre.
        auto weighted = [&](const std::vector<double>& nodes, std::size_t i, double a, double b) -> std::optional<double> {
            const double centre = 0.5 * (a + b);
            std::vector<double> basis{1.0};
            double denom = 1.0;
            for (std::size_t k = 0; k < nodes.size(); ++k) {
                if (k == i) continue;
                basis = polyTimes(basis, {centre - nodes[k], 1.0});
                denom *= nodes[i] - nodes[k];
            }
            for (auto& c : basis) c /= denom;
            std::string ignored;
            const auto wl = multiplyExact(shiftIntegrand(*r.weight, centre), LineIntegrand{basis, {}, {}, {}}, ignored);
            if (!wl) { if (why) *why = "a remainder weight times a polynomial is not closed-form"; return std::nullopt; }
            return integrateExact(*wl, a - centre, b - centre, why);
        };
        uint32_t j = 0;
        while (j < cells) {
            // Pairs of cells, with the last three as one cubic panel when the count
            // is odd: a lone linear cell would be the one second-order panel.
            const uint32_t left = cells - j;
            const uint32_t span = left == 1 ? 1 : left == 3 ? 3 : 2;
            const double a = edge(j), b = edge(j + span);
            std::vector<double> nodes;
            for (uint32_t k = 0; k <= span; ++k) nodes.push_back(edge(j + k));
            for (uint32_t k = 0; k <= span; ++k) {
                const auto wk = weighted(nodes, k, a, b);
                if (!wk) return std::nullopt;
                out.estimate += g[j + k] * *wk;
            }
            double gLo = 1.0, gHi = 1.0, gLip = 0.0;
            if (r.node != kNoNode) {
                bool undefined = false;
                const Interval gr = range(r.node, a, b, decided, &undefined);
                gLo = undefined ? -std::numeric_limits<double>::infinity() : double(gr.lo);
                gHi = undefined ? std::numeric_limits<double>::infinity() : double(gr.hi);
                const Interval dg = derivativeRange(r.node, a, b, decided);
                gLip = std::max(std::abs(double(dg.lo)), std::abs(double(dg.hi)));
            }
            double lo = gLo, hi = gHi, lip = gLip;
            if (factor) {
                const auto [mLo, mHi] = factor->range(a, b);
                const double mLip = factor->lipschitz(a, b);
                const double pr[4] = {gLo * mLo, gLo * mHi, gHi * mLo, gHi * mHi};
                lo = *std::min_element(pr, pr + 4);
                hi = *std::max_element(pr, pr + 4);
                const double supG = std::max(std::abs(gLo), std::abs(gHi)), supM = std::max(std::abs(mLo), std::abs(mHi));
                lip = gLip * supM + supG * mLip;
            }
            const double width = std::isfinite(hi - lo) ? hi - lo : std::numeric_limits<double>::infinity();
            const double lebesgue = span == 3 ? 1.6311 : span == 2 ? 1.25 : 1.0;   // equispaced Lebesgue constants
            const double local = std::min(width, std::isfinite(lip) ? lip * 0.5 * (b - a) : std::numeric_limits<double>::infinity());
            out.radius += supAbs(*r.weight, a, b) * (b - a) * (1.0 + lebesgue) * local;
            j += span;
        }
    }
    if (!std::isfinite(out.estimate)) { if (why) *why = "the hybrid estimate is not finite"; return std::nullopt; }
    if (!hybrid && !f->remainders.empty()) { if (why) *why = reason; return std::nullopt; }
    return out;
}

} // namespace OntoMath
