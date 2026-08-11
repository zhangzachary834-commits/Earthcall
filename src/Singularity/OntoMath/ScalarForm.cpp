#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/OntoMath/ProbabilityForm.hpp"

// The guard is the condition calculus itself — pieces gated by law-text.
// (Include at the .cpp level only: ConditionModel.hpp includes this header.)
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace OntoMath {

namespace {

std::string formatNumber(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);
    return buf;
}

double totalDegree(const Term& t) {
    double degree = 0.0;
    for (const auto& [name, exp] : t.factors) {
        (void)name;
        degree += exp;
    }
    return degree;
}

std::string factorSignature(const Term& t) {
    std::string sig;
    for (const auto& [name, exp] : t.factors) {
        sig += name + "^" + formatNumber(exp) + ";";
    }
    for (const auto& tf : t.trans) {
        sig += tf.print() + ";";
    }
    return sig;
}

} // namespace

// ---------------------------------------------------------------------------
// TransFactor
// ---------------------------------------------------------------------------

std::optional<double> TransFactor::evaluate(double x) const {
    const double arg = scale * x + shift;
    switch (kind) {
        case Kind::Sin: return std::sin(arg);
        case Kind::Cos: return std::cos(arg);
        case Kind::Exp: return std::exp(arg);
        case Kind::Ln:
            if (arg <= 0.0) return std::nullopt;   // outside the domain: undefined
            return std::log(arg);
    }
    return std::nullopt;
}

std::string TransFactor::print() const {
    const char* name = kind == Kind::Sin   ? "sin"
                       : kind == Kind::Cos ? "cos"
                       : kind == Kind::Exp ? "exp"
                                           : "ln";
    std::string inner;
    if (scale == -1.0) inner = "-";
    else if (scale != 1.0) inner = formatNumber(scale);
    inner += variable;
    if (shift > 0.0) inner += "+" + formatNumber(shift);
    else if (shift < 0.0) inner += formatNumber(shift);
    return std::string(name) + "(" + inner + ")";
}

nlohmann::json TransFactor::toJson() const {
    nlohmann::json j{{"kind", static_cast<int>(kind)}, {"var", variable}};
    if (scale != 1.0) j["scale"] = scale;
    if (shift != 0.0) j["shift"] = shift;
    return j;
}

TransFactor TransFactor::fromJson(const nlohmann::json& j) {
    TransFactor f;
    f.kind = static_cast<Kind>(j.value("kind", 0));
    f.variable = j.value("var", std::string("x"));
    f.scale = j.value("scale", 1.0);
    f.shift = f.kind == Kind::Ln ? 0.0 : j.value("shift", 0.0);
    return f;
}

bool TransFactor::operator<(const TransFactor& o) const {
    if (variable != o.variable) return variable < o.variable;
    if (kind != o.kind) return static_cast<int>(kind) < static_cast<int>(o.kind);
    if (scale != o.scale) return scale < o.scale;
    return shift < o.shift;
}

// ---------------------------------------------------------------------------
// Term
// ---------------------------------------------------------------------------

std::optional<double> Term::evaluate(const std::map<std::string, double>& vars) const {
    double value = coefficient;
    for (const auto& [name, exp] : factors) {
        auto it = vars.find(name);
        if (it == vars.end()) return std::nullopt;   // unbound symbol: no value
        value *= std::pow(it->second, exp);
    }
    for (const auto& tf : trans) {
        auto it = vars.find(tf.variable);
        if (it == vars.end()) return std::nullopt;
        auto v = tf.evaluate(it->second);
        if (!v) return std::nullopt;                 // ln outside its domain
        value *= *v;
    }
    return value;
}

Term Term::times(const Term& other) const {
    Term product(coefficient * other.coefficient, factors);
    for (const auto& [name, exp] : other.factors) {
        product.factors[name] += exp;
        if (product.factors[name] == 0.0) product.factors.erase(name);
    }
    product.trans = trans;
    product.trans.insert(product.trans.end(), other.trans.begin(), other.trans.end());
    std::sort(product.trans.begin(), product.trans.end());
    return product;
}

void Term::addTrans(TransFactor factor) {
    trans.push_back(std::move(factor));
    std::sort(trans.begin(), trans.end());
}

bool Term::mentions(const std::string& var) const {
    if (factors.count(var)) return true;
    for (const auto& tf : trans) {
        if (tf.variable == var) return true;
    }
    return false;
}

std::string Term::print() const {
    if (factors.empty() && trans.empty()) return formatNumber(coefficient);
    std::string out;
    if (coefficient == -1.0) out = "-";
    else if (coefficient != 1.0) out = formatNumber(coefficient);
    for (const auto& [name, exp] : factors) {
        out += name;
        if (exp != 1.0) out += "^" + formatNumber(exp);
    }
    for (const auto& tf : trans) {
        if (!out.empty() && out != "-") out += "·";
        out += tf.print();
    }
    return out;
}

nlohmann::json Term::toJson() const {
    nlohmann::json f = nlohmann::json::object();
    for (const auto& [name, exp] : factors) f[name] = exp;
    nlohmann::json j{{"c", coefficient}, {"factors", f}};
    if (!trans.empty()) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& tf : trans) arr.push_back(tf.toJson());
        j["trans"] = arr;
    }
    return j;
}

Term Term::fromJson(const nlohmann::json& j) {
    Term t;
    t.coefficient = j.value("c", 0.0);
    if (j.contains("factors")) {
        for (auto it = j["factors"].begin(); it != j["factors"].end(); ++it) {
            t.factors[it.key()] = it.value().get<double>();
        }
    }
    if (j.contains("trans")) {
        for (const auto& tf : j["trans"]) t.trans.push_back(TransFactor::fromJson(tf));
        std::sort(t.trans.begin(), t.trans.end());
    }
    return t;
}

// ---------------------------------------------------------------------------
// ScalarForm
// ---------------------------------------------------------------------------

ScalarForm ScalarForm::constant(double c) {
    ScalarForm e;
    e.terms.emplace_back(c);
    return e;
}

ScalarForm ScalarForm::variable(const std::string& name, double exponent, double coefficient) {
    ScalarForm e;
    Term t(coefficient);
    if (exponent != 0.0) t.factors[name] = exponent;
    e.terms.push_back(std::move(t));
    return e;
}

ScalarForm ScalarForm::transcendental(TransFactor::Kind kind, const std::string& var,
                                      double scale, double shift, double coefficient) {
    ScalarForm e;
    Term t(coefficient);
    t.addTrans(TransFactor(kind, var, scale, shift));
    e.terms.push_back(std::move(t));
    return e;
}

ScalarForm ScalarForm::sinusoid(double amplitude, double frequency, double phase,
                                double bias, const std::string& var) {
    // bias + amplitude·sin(2π(frequency·var + phase)) — evalTrack, exactly.
    constexpr double kTau = 6.283185307179586476925286766559;
    ScalarForm e = transcendental(TransFactor::Kind::Sin, var, kTau * frequency,
                                  kTau * phase, amplitude);
    if (bias != 0.0) e = e.plus(constant(bias));
    return e;
}

std::optional<double> ScalarForm::evaluate(const std::map<std::string, double>& vars) const {
    double sum = 0.0;
    for (const auto& term : terms) {
        auto v = term.evaluate(vars);
        if (!v) return std::nullopt;
        sum += *v;
    }
    return sum;
}

ScalarForm ScalarForm::plus(const ScalarForm& other) const {
    ScalarForm out = *this;
    out.terms.insert(out.terms.end(), other.terms.begin(), other.terms.end());
    return out.normalized();
}

ScalarForm ScalarForm::times(const ScalarForm& other) const {
    ScalarForm out;
    for (const auto& a : terms) {
        for (const auto& b : other.terms) {
            out.terms.push_back(a.times(b));
        }
    }
    return out.normalized();
}

ScalarForm ScalarForm::scaled(double k) const {
    ScalarForm out = *this;
    for (auto& term : out.terms) term.coefficient *= k;
    return out.normalized();
}

ScalarForm ScalarForm::normalized() const {
    // Combine like terms (equal factor signatures), drop zeros, sort by
    // descending total degree then signature — a canonical form.
    std::vector<Term> combined;
    for (const auto& term : terms) {
        bool merged = false;
        for (auto& existing : combined) {
            if (existing.sameShape(term)) {
                existing.coefficient += term.coefficient;
                merged = true;
                break;
            }
        }
        if (!merged) combined.push_back(term);
    }
    combined.erase(std::remove_if(combined.begin(), combined.end(),
                                  [](const Term& t) { return t.coefficient == 0.0; }),
                   combined.end());
    std::sort(combined.begin(), combined.end(), [](const Term& a, const Term& b) {
        const double da = totalDegree(a), db = totalDegree(b);
        if (da != db) return da > db;
        return factorSignature(a) < factorSignature(b);
    });
    ScalarForm out;
    out.terms = std::move(combined);
    return out;
}

ScalarForm ScalarForm::derivative(const std::string& var) const {
    // Full product rule per term over every factor that mentions var:
    //   power x^n        -> n·x^(n-1) × rest
    //   sin(ax+b)        ->  a·cos(ax+b) × rest
    //   cos(ax+b)        -> -a·sin(ax+b) × rest
    //   exp(ax+b)        ->  a·exp(ax+b) × rest
    //   ln(ax)           ->  x^-1 × rest (ln factor removed; a cancels —
    //                        this is why Ln carries no shift)
    ScalarForm out;
    for (const auto& term : terms) {
        // Power-factor contribution.
        auto it = term.factors.find(var);
        if (it != term.factors.end()) {
            Term d = term;
            d.coefficient *= it->second;
            const double newExp = it->second - 1.0;
            if (newExp == 0.0) d.factors.erase(var);
            else d.factors[var] = newExp;
            out.terms.push_back(std::move(d));
        }
        // One contribution per transcendental factor on var.
        for (std::size_t k = 0; k < term.trans.size(); ++k) {
            const TransFactor& tf = term.trans[k];
            if (tf.variable != var) continue;
            Term d = term;
            switch (tf.kind) {
                case TransFactor::Kind::Sin:
                    d.coefficient *= tf.scale;
                    d.trans[k].kind = TransFactor::Kind::Cos;
                    break;
                case TransFactor::Kind::Cos:
                    d.coefficient *= -tf.scale;
                    d.trans[k].kind = TransFactor::Kind::Sin;
                    break;
                case TransFactor::Kind::Exp:
                    d.coefficient *= tf.scale;   // factor stays
                    break;
                case TransFactor::Kind::Ln:
                    d.trans.erase(d.trans.begin() + static_cast<long>(k));
                    d.factors[var] -= 1.0;
                    if (d.factors[var] == 0.0) d.factors.erase(var);
                    break;
            }
            std::sort(d.trans.begin(), d.trans.end());
            out.terms.push_back(std::move(d));
        }
    }
    return out.normalized();
}

std::optional<ScalarForm> ScalarForm::antiderivative(const std::string& var) const {
    // Exact where the algebra can hold the answer; honest nullopt where it
    // would take integration by parts. Per term (R = factors free of var):
    //   c·x^n·R, n ≠ -1   -> c/(n+1)·x^(n+1)·R
    //   c·x^-1·R          -> c·ln(x)·R            (defined on x > 0)
    //   c·sin(ax+b)·R     -> -(c/a)·cos(ax+b)·R
    //   c·cos(ax+b)·R     ->  (c/a)·sin(ax+b)·R
    //   c·exp(ax+b)·R     ->  (c/a)·exp(ax+b)·R
    //   c·ln(ax)·R        ->  c·x·ln(ax)·R − c·x·R
    //   powers AND transcendentals of var together, or several
    //   transcendentals of var -> nullopt (by parts; not yet held)
    ScalarForm out;
    for (const auto& term : terms) {
        std::vector<std::size_t> transOnVar;
        for (std::size_t k = 0; k < term.trans.size(); ++k) {
            if (term.trans[k].variable == var) transOnVar.push_back(k);
        }
        auto it = term.factors.find(var);
        const double n = it == term.factors.end() ? 0.0 : it->second;

        if (transOnVar.empty()) {
            if (n == -1.0) {
                // ∫ c·x^-1 dx = c·ln(x) — the algebra now holds it.
                Term integrated = term;
                integrated.factors.erase(var);
                integrated.addTrans(TransFactor(TransFactor::Kind::Ln, var));
                out.terms.push_back(std::move(integrated));
            } else {
                Term integrated = term;
                integrated.coefficient /= (n + 1.0);
                integrated.factors[var] = n + 1.0;
                out.terms.push_back(std::move(integrated));
            }
            continue;
        }
        if (transOnVar.size() > 1 || n != 0.0) return std::nullopt;   // by parts

        const std::size_t k = transOnVar.front();
        const TransFactor tf = term.trans[k];
        if (tf.scale == 0.0) return std::nullopt;   // degenerate: not a function of var
        Term integrated = term;
        switch (tf.kind) {
            case TransFactor::Kind::Sin:
                integrated.coefficient *= -1.0 / tf.scale;
                integrated.trans[k].kind = TransFactor::Kind::Cos;
                out.terms.push_back(std::move(integrated));
                break;
            case TransFactor::Kind::Cos:
                integrated.coefficient *= 1.0 / tf.scale;
                integrated.trans[k].kind = TransFactor::Kind::Sin;
                out.terms.push_back(std::move(integrated));
                break;
            case TransFactor::Kind::Exp:
                integrated.coefficient *= 1.0 / tf.scale;
                out.terms.push_back(std::move(integrated));
                break;
            case TransFactor::Kind::Ln: {
                // ∫ c·ln(ax) dx = c·x·ln(ax) − c·x
                Term first = term;
                first.factors[var] += 1.0;
                Term second = term;
                second.coefficient = -second.coefficient;
                second.trans.erase(second.trans.begin() + static_cast<long>(k));
                second.factors[var] += 1.0;
                if (second.factors[var] == 0.0) second.factors.erase(var);
                out.terms.push_back(std::move(first));
                out.terms.push_back(std::move(second));
                break;
            }
        }
        std::sort(out.terms.back().trans.begin(), out.terms.back().trans.end());
    }
    return out.normalized();
}

std::string ScalarForm::print() const {
    if (terms.empty()) return "0";
    std::string out;
    for (std::size_t i = 0; i < terms.size(); ++i) {
        const std::string piece = terms[i].print();
        if (i == 0) {
            out = piece;
        } else if (!piece.empty() && piece[0] == '-') {
            out += " - " + piece.substr(1);
        } else {
            out += " + " + piece;
        }
    }
    return out;
}

nlohmann::json ScalarForm::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& term : terms) arr.push_back(term.toJson());
    return nlohmann::json{{"terms", arr}};
}

ScalarForm ScalarForm::fromJson(const nlohmann::json& j) {
    ScalarForm e;
    if (j.contains("terms")) {
        for (const auto& t : j["terms"]) e.terms.push_back(Term::fromJson(t));
    }
    return e;
}

// ---------------------------------------------------------------------------
// Piecewise
// ---------------------------------------------------------------------------

bool Piecewise::Piece::contains(double x) const {
    if (hasLo && (includeLo ? x < lo : x <= lo)) return false;
    if (hasHi && (includeHi ? x > hi : x >= hi)) return false;
    return true;
}



void MathNode::collectDependencies(std::set<std::string>& outDeps) const {
    if (op == Op::ValueLeaf) {
        outDeps.insert(variableName);
    } else if (op == Op::ScalarLeaf) {
        // Collect variables from scalar terms
        for (const auto& term : scalarForm.terms) {
            for (const auto& [var, _] : term.factors) {
                outDeps.insert(var);
            }
            for (const auto& tr : term.trans) {
                outDeps.insert(tr.variable);
            }
        }
    }
    for (const auto& child : children) {
        child->collectDependencies(outDeps);
    }
}

const char* valueKindName(ValueKind k) {
    switch (k) {
        case ValueKind::Scalar:      return "Scalar";
        case ValueKind::Vector:      return "Vector";
        case ValueKind::ScalarField: return "ScalarField";
        case ValueKind::VectorField: return "VectorField";
        case ValueKind::Unknown:     return "Unknown";
    }
    return "Unknown";
}

const char* mathOpName(MathNode::Op op) {
    switch (op) {
        case MathNode::Op::ScalarLeaf:      return "ScalarLeaf";
        case MathNode::Op::ValueLeaf:       return "ValueLeaf";
        case MathNode::Op::VectorConstruct: return "VectorConstruct";
        case MathNode::Op::Component:       return "Component";
        case MathNode::Op::Add:             return "Add";
        case MathNode::Op::Sub:             return "Sub";
        case MathNode::Op::Scale:           return "Scale";
        case MathNode::Op::Dot:             return "Dot";
        case MathNode::Op::Cross:           return "Cross";
        case MathNode::Op::Hadamard:        return "Hadamard";
        case MathNode::Op::Normalize:       return "Normalize";
        case MathNode::Op::Length:          return "Length";
        case MathNode::Op::Map:             return "Map";
        case MathNode::Op::Stochastic:      return "Stochastic";
        case MathNode::Op::Project:         return "Project";
        case MathNode::Op::Distance:        return "Distance";
        case MathNode::Op::Raycast:         return "Raycast";
        case MathNode::Op::SDF:             return "SDF";
        case MathNode::Op::Gradient:        return "Gradient";
        case MathNode::Op::LineIntegral:    return "LineIntegral";
        case MathNode::Op::Union:           return "Union";
        case MathNode::Op::Intersection:    return "Intersection";
        case MathNode::Op::Difference:      return "Difference";
        case MathNode::Op::Unsupported:     return "Unsupported";
    }
    return "Unsupported";
}

// The stored op is an ARBITRARY int out of a file, not an Op. Casting it
// unchecked produces a scoped-enum value outside the enumeration — formally UB,
// and in practice a node no switch matches. Same shape as
// isKnownConditionKind in ConditionModel.cpp.
bool isKnownMathOp(int raw) {
    switch (static_cast<MathNode::Op>(raw)) {
        case MathNode::Op::ScalarLeaf:
        case MathNode::Op::ValueLeaf:
        case MathNode::Op::VectorConstruct:
        case MathNode::Op::Component:
        case MathNode::Op::Add:
        case MathNode::Op::Sub:
        case MathNode::Op::Scale:
        case MathNode::Op::Dot:
        case MathNode::Op::Cross:
        case MathNode::Op::Hadamard:
        case MathNode::Op::Normalize:
        case MathNode::Op::Length:
        case MathNode::Op::Map:
        case MathNode::Op::Stochastic:
        case MathNode::Op::Project:
        case MathNode::Op::Distance:
        case MathNode::Op::Raycast:
        case MathNode::Op::SDF:
        case MathNode::Op::Gradient:
        case MathNode::Op::LineIntegral:
        case MathNode::Op::Union:
        case MathNode::Op::Intersection:
        case MathNode::Op::Difference:
            return true;
        // Unsupported is where unknown ops LAND; it is never a stored value an
        // author picked, so it does not read back as known.
        case MathNode::Op::Unsupported:
            return false;
    }
    return false;
}

std::string formatTypeDiagnostic(const TypeDiagnostic& d) {
    return d.nodePath + ": " + d.message;
}

namespace {

// Unknown unifies with everything: it is "no declared signature here", not a
// claim. Every other pair must match exactly.
bool kindIs(ValueKind got, ValueKind want) {
    return got == ValueKind::Unknown || got == want;
}
// A ScalarField sampled at a point IS a scalar. In a POINTWISE AST — which is
// the only kind this calculus has, on either path — the two coincide, so CSG
// and SDF accept either and report the wider of the two.
bool scalarLike(ValueKind k) {
    return k == ValueKind::Scalar || k == ValueKind::ScalarField ||
           k == ValueKind::Unknown;
}
bool vectorLike(ValueKind k) {
    return k == ValueKind::Vector || k == ValueKind::VectorField ||
           k == ValueKind::Unknown;
}
ValueKind widerScalar(ValueKind a, ValueKind b) {
    if (a == ValueKind::ScalarField || b == ValueKind::ScalarField) return ValueKind::ScalarField;
    if (a == ValueKind::Unknown || b == ValueKind::Unknown) return ValueKind::Unknown;
    return ValueKind::Scalar;
}

TypeResult arity(const std::string& path, MathNode::Op op, std::size_t got, std::size_t want) {
    return TypeResult::error(TypeDiagnostic{
        path, std::string(mathOpName(op)) + " requires " + std::to_string(want) +
              " argument(s), got " + std::to_string(got)});
}
TypeResult mismatch(const std::string& path, MathNode::Op op, int index,
                    const char* want, ValueKind got) {
    return TypeResult::error(TypeDiagnostic{
        path, std::string(mathOpName(op)) + " argument " + std::to_string(index) +
              " must be " + want + ", got " + valueKindName(got)});
}

} // namespace

// Every arity and type rule of the calculus. CALL THIS at any seam where an AST
// enters the system — deserialization, and before compiling a field AST to
// WGSL. It was dead code for one commit and that cost a whole authored envelope
// law (Scale over two scalars, silently nullopt). See AUDIT_2026-08-10 §2.8.
TypeResult MathNode::typeOf(const TypeEnv& env, const std::string& path,
                            bool allowUnbound) const {
    // A child slot that is null is a malformed tree, not a type error to
    // propagate blindly — say so before dereferencing anything.
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!children[i]) {
            return TypeResult::error(TypeDiagnostic{
                path, std::string(mathOpName(op)) + " argument " +
                      std::to_string(static_cast<int>(i)) + " is missing"});
        }
    }
    const auto sub = [&](std::size_t i) {
        return children[i]->typeOf(env, path + "." + mathOpName(op) + "[" +
                                            std::to_string(static_cast<int>(i)) + "]",
                                   allowUnbound);
    };

    switch (op) {
        case Op::Stochastic:
        case Op::ScalarLeaf: return TypeResult::ok(ValueKind::Scalar);
        case Op::ValueLeaf: {
            auto it = env.find(variableName);
            if (it == env.end()) {
                if (allowUnbound) return TypeResult::ok(ValueKind::Unknown);
                return TypeResult::error(TypeDiagnostic{
                    path, "ValueLeaf names '" + variableName +
                          "', which has no declared signature in this environment"});
            }
            return TypeResult::ok(it->second);
        }
        case Op::VectorConstruct: {
            if (children.size() != 3) return arity(path, op, children.size(), 3);
            for (std::size_t i = 0; i < 3; ++i) {
                auto t = sub(i);
                if (!t) return t;
                if (!kindIs(*t, ValueKind::Scalar))
                    return mismatch(path, op, static_cast<int>(i), "Scalar", *t);
            }
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::Component: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            if (stringArg != "x" && stringArg != "y" && stringArg != "z") {
                return TypeResult::error(TypeDiagnostic{
                    path, "Component names axis '" + stringArg + "'; expected x, y or z"});
            }
            auto t = sub(0);
            if (!t) return t;
            if (!kindIs(*t, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Add:
        case Op::Sub: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (*t0 == ValueKind::Unknown) return TypeResult::ok(*t1);
            if (*t1 == ValueKind::Unknown) return TypeResult::ok(*t0);
            if (scalarLike(*t0) && scalarLike(*t1)) return TypeResult::ok(widerScalar(*t0, *t1));
            if (*t0 != *t1) {
                return TypeResult::error(TypeDiagnostic{
                    path, std::string(mathOpName(op)) + " requires matching arguments, got " +
                          valueKindName(*t0) + " and " + valueKindName(*t1)});
            }
            return TypeResult::ok(*t0);
        }
        case Op::Scale: {
            // (Scalar, Vector) and (Vector, Scalar) scale a vector.
            // (Scalar, Scalar) is ORDINARY MULTIPLICATION — see the note in
            // evaluate(); the GPU emitter has always compiled it as `a * b`,
            // so refusing it on the CPU was a live CPU/GPU divergence.
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (*t0 == ValueKind::Unknown || *t1 == ValueKind::Unknown)
                return TypeResult::ok(ValueKind::Unknown);
            if (scalarLike(*t0) && scalarLike(*t1)) return TypeResult::ok(widerScalar(*t0, *t1));
            if ((scalarLike(*t0) && *t1 == ValueKind::Vector) ||
                (*t0 == ValueKind::Vector && scalarLike(*t1))) {
                return TypeResult::ok(ValueKind::Vector);
            }
            return TypeResult::error(TypeDiagnostic{
                path, "Scale requires (Scalar, Vector), (Vector, Scalar) or (Scalar, Scalar), got " +
                      std::string(valueKindName(*t0)) + " and " + valueKindName(*t1)});
        }
        case Op::Dot: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!kindIs(*t0, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Cross:
        case Op::Hadamard: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!kindIs(*t0, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::Normalize: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            auto t = sub(0); if (!t) return t;
            if (!kindIs(*t, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t);
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::Length: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            auto t = sub(0); if (!t) return t;
            if (!kindIs(*t, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Map: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            if (stringArg != "Round" && stringArg != "Floor") {
                return TypeResult::error(TypeDiagnostic{
                    path, "Map names function '" + stringArg +
                          "', which this build does not define (Round, Floor)"});
            }
            auto t = sub(0); if (!t) return t;
            if (!kindIs(*t, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t);
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::Project: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!kindIs(*t0, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::Distance: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!kindIs(*t0, ValueKind::Vector)) return mismatch(path, op, 0, "Vector", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Raycast: {
            if (children.size() != 3) return arity(path, op, children.size(), 3);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            auto t2 = sub(2); if (!t2) return t2;
            if (!scalarLike(*t0)) return mismatch(path, op, 0, "ScalarField", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            if (!kindIs(*t2, ValueKind::Vector)) return mismatch(path, op, 2, "Vector", *t2);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::SDF: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!scalarLike(*t0)) return mismatch(path, op, 0, "ScalarField", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Gradient: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!scalarLike(*t0)) return mismatch(path, op, 0, "ScalarField", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Vector);
        }
        case Op::LineIntegral: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!vectorLike(*t0)) return mismatch(path, op, 0, "VectorField", *t0);
            if (!kindIs(*t1, ValueKind::Vector)) return mismatch(path, op, 1, "Vector", *t1);
            return TypeResult::ok(ValueKind::Scalar);
        }
        case Op::Union:
        case Op::Intersection:
        case Op::Difference: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!scalarLike(*t0)) return mismatch(path, op, 0, "ScalarField", *t0);
            if (!scalarLike(*t1)) return mismatch(path, op, 1, "ScalarField", *t1);
            return TypeResult::ok(widerScalar(*t0, *t1));
        }
        case Op::Unsupported:
            return TypeResult::error(TypeDiagnostic{
                path, "this build does not know this operation; it was preserved "
                      "verbatim from the save and will never evaluate"});
    }
    return TypeResult::error(TypeDiagnostic{path, "unreachable: op is outside the enumeration"});
}

bool MathNode::checkTypes(const TypeEnv& env, std::string& outError,
                          ValueKind* outKind, bool allowUnbound) const {
    const TypeResult r = typeOf(env, "root", allowUnbound);
    if (!r) {
        outError = formatTypeDiagnostic(r.diagnostic);
        return false;
    }
    if (outKind) *outKind = r.kind;
    outError.clear();
    return true;
}

namespace {

// Rebind the AMBIENT POINT (see kAmbientPointVar) to q, leaving every other
// binding alone. This is what SDF/Gradient do to sample a field expression
// somewhere other than where the enclosing expression is being evaluated —
// the CPU counterpart of the WGSL emitter substituting a different point
// expression for `p`.
std::map<std::string, PropertyValue> varsAtPoint(
        const std::map<std::string, PropertyValue>& vars, const glm::vec3& q) {
    std::map<std::string, PropertyValue> out = vars;
    out[kAmbientPointVar] = PropertyValue(q);
    out["x"] = PropertyValue(static_cast<double>(q.x));
    out["y"] = PropertyValue(static_cast<double>(q.y));
    out["z"] = PropertyValue(static_cast<double>(q.z));
    return out;
}

} // namespace

std::optional<PropertyValue> MathNode::evaluate(const std::map<std::string, PropertyValue>& vars, const Singular* subject) const {
    switch(op) {
        case Op::ScalarLeaf: {
            std::map<std::string, double> scalarVars;
            for (const auto& [k, v] : vars) {
                double d = 0.0;
                if (propertyValueToNumber(v, d)) {
                    scalarVars[k] = d;
                }
            }
            auto res = scalarForm.evaluate(scalarVars);
            if (!res) return std::nullopt;
            return PropertyValue(*res);
        }
        case Op::ValueLeaf: {
            auto it = vars.find(variableName);
            if (it != vars.end()) return it->second;
            if (subject) {
                PropertyValue val;
                if (lawGetValue(*const_cast<Singular*>(subject), PropertyPath::parse(variableName), val)) {
                    return val;
                }
            }
            return std::nullopt;
        }
        case Op::VectorConstruct: {
            if (children.size() != 3) return std::nullopt;
            auto x = children[0]->evaluate(vars, subject);
            auto y = children[1]->evaluate(vars, subject);
            auto z = children[2]->evaluate(vars, subject);
            if (!x || !y || !z) return std::nullopt;
            double dx=0, dy=0, dz=0;
            if (!propertyValueToNumber(*x, dx) || !propertyValueToNumber(*y, dy) || !propertyValueToNumber(*z, dz)) return std::nullopt;
            return PropertyValue(glm::vec3(dx, dy, dz));
        }
        case Op::Component: {
            if (children.size() != 1) return std::nullopt;
            auto v = children[0]->evaluate(vars, subject);
            if (!v || !std::holds_alternative<glm::vec3>(*v)) return std::nullopt;
            auto vec = std::get<glm::vec3>(*v);
            if (stringArg == "x") return PropertyValue(vec.x);
            if (stringArg == "y") return PropertyValue(vec.y);
            if (stringArg == "z") return PropertyValue(vec.z);
            return std::nullopt;
        }
        case Op::Add: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            if (std::holds_alternative<double>(*a) && std::holds_alternative<double>(*b)) {
                return PropertyValue(std::get<double>(*a) + std::get<double>(*b));
            } else if (std::holds_alternative<glm::vec3>(*a) && std::holds_alternative<glm::vec3>(*b)) {
                return PropertyValue(std::get<glm::vec3>(*a) + std::get<glm::vec3>(*b));
            }
            return std::nullopt;
        }
        case Op::Sub: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            if (std::holds_alternative<double>(*a) && std::holds_alternative<double>(*b)) {
                return PropertyValue(std::get<double>(*a) - std::get<double>(*b));
            } else if (std::holds_alternative<glm::vec3>(*a) && std::holds_alternative<glm::vec3>(*b)) {
                return PropertyValue(std::get<glm::vec3>(*a) - std::get<glm::vec3>(*b));
            }
            return std::nullopt;
        }
        case Op::Scale: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            const bool bVec = std::holds_alternative<glm::vec3>(*b);
            double da = 0.0, db = 0.0;
            if (aVec && bVec) return std::nullopt;   // vec*vec is Hadamard, not Scale
            if (!aVec && bVec) {
                if (!propertyValueToNumber(*a, da)) return std::nullopt;
                return PropertyValue(static_cast<float>(da) * std::get<glm::vec3>(*b));
            }
            if (aVec && !bVec) {
                if (!propertyValueToNumber(*b, db)) return std::nullopt;
                return PropertyValue(std::get<glm::vec3>(*a) * static_cast<float>(db));
            }
            // WIDENED (2026-08-10): scalar * scalar is ordinary multiplication.
            // It used to fall through to nullopt, which poisoned every
            // enclosing node and silently killed authored laws (the ADSR decay
            // envelope, AUDIT §2.8) — while the WGSL emitter compiled the same
            // node to `a * b` and computed it. Refusing on one path and
            // computing on the other is the divergence, not the widening.
            // Accept any two numeric alternatives, matching ScalarLeaf's own
            // coercion rule (propertyValueToNumber).
            if (propertyValueToNumber(*a, da) && propertyValueToNumber(*b, db)) {
                return PropertyValue(da * db);
            }
            return std::nullopt;
        }
        case Op::Dot: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b || !std::holds_alternative<glm::vec3>(*a) || !std::holds_alternative<glm::vec3>(*b)) return std::nullopt;
            return PropertyValue(glm::dot(std::get<glm::vec3>(*a), std::get<glm::vec3>(*b)));
        }
        case Op::Cross: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b || !std::holds_alternative<glm::vec3>(*a) || !std::holds_alternative<glm::vec3>(*b)) return std::nullopt;
            return PropertyValue(glm::cross(std::get<glm::vec3>(*a), std::get<glm::vec3>(*b)));
        }
        case Op::Hadamard: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b || !std::holds_alternative<glm::vec3>(*a) || !std::holds_alternative<glm::vec3>(*b)) return std::nullopt;
            return PropertyValue(std::get<glm::vec3>(*a) * std::get<glm::vec3>(*b));
        }
        case Op::Normalize: {
            if (children.size() != 1) return std::nullopt;
            auto v = children[0]->evaluate(vars, subject);
            if (!v || !std::holds_alternative<glm::vec3>(*v)) return std::nullopt;
            auto vec = std::get<glm::vec3>(*v);
            if (glm::length(vec) < 1e-6f) return PropertyValue(glm::vec3(0.0f));
            return PropertyValue(glm::normalize(vec));
        }
        case Op::Length: {
            if (children.size() != 1) return std::nullopt;
            auto v = children[0]->evaluate(vars, subject);
            if (!v || !std::holds_alternative<glm::vec3>(*v)) return std::nullopt;
            return PropertyValue(glm::length(std::get<glm::vec3>(*v)));
        }
        case Op::Map: {
            if (children.size() != 1) return std::nullopt;
            auto v = children[0]->evaluate(vars, subject);
            if (!v || !std::holds_alternative<glm::vec3>(*v)) return std::nullopt;
            auto vec = std::get<glm::vec3>(*v);
            if (stringArg == "Round") {
                return PropertyValue(glm::vec3(std::round(vec.x), std::round(vec.y), std::round(vec.z)));
            } else if (stringArg == "Floor") {
                return PropertyValue(glm::vec3(std::floor(vec.x), std::floor(vec.y), std::floor(vec.z)));
            }
            return std::nullopt;
        }
        case Op::Stochastic: {
            std::vector<std::optional<PropertyValue>> evalArgs;
            for (const auto& child : children) {
                evalArgs.push_back(child->evaluate(vars, subject));
            }
            return Probability::evaluateStochastic(stringArg, evalArgs);
        }
        case Op::Project: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b || !std::holds_alternative<glm::vec3>(*a) || !std::holds_alternative<glm::vec3>(*b)) return std::nullopt;
            glm::vec3 va = std::get<glm::vec3>(*a);
            glm::vec3 vb = std::get<glm::vec3>(*b);
            // The guard is on the UNSQUARED length, exactly as Op::Normalize
            // above. Testing dot(b,b) < 1e-6 (the bug this replaces) is a
            // SQUARED length, so it zeroed every b shorter than 1e-3 and
            // returned vec3(0) where the true projection is (1,0,0).
            //
            // Below the threshold the answer is not a guess: span{0} = {0}, and
            // the orthogonal projection onto the zero subspace IS zero. The
            // WGSL emitter carries the identical guard and threshold — see
            // SdfWgsl.cpp Op::Project.
            if (glm::length(vb) < kDegenerateVectorLength) return PropertyValue(glm::vec3(0.0f));
            return PropertyValue(vb * (glm::dot(va, vb) / glm::dot(vb, vb)));
        }
        case Op::Distance: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b || !std::holds_alternative<glm::vec3>(*a) || !std::holds_alternative<glm::vec3>(*b)) return std::nullopt;
            return PropertyValue(glm::distance(std::get<glm::vec3>(*a), std::get<glm::vec3>(*b)));
        }
        // --- The CSG booleans over signed distance -------------------------
        // These ARE geom::SdfOp::Union / Intersect / Subtract (Sdf.cpp's
        // evalSdf), formula for formula. One vocabulary, two places it can be
        // written: a shape tree, or a field expression.
        case Op::Union:
        case Op::Intersection:
        case Op::Difference: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            double da = 0.0, db = 0.0;
            if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) {
                return std::nullopt;
            }
            if (op == Op::Union)        return PropertyValue(std::min(da, db));
            if (op == Op::Intersection) return PropertyValue(std::max(da, db));
            return PropertyValue(std::max(da, -db));   // Difference == Subtract
        }

        // --- Sampling a field expression at a point ------------------------
        // SDF(f, q): evaluate f with the AMBIENT POINT rebound to q. The WGSL
        // emitter does the same thing by substituting q for its point
        // expression, so the two paths compute the same number.
        case Op::SDF: {
            if (children.size() != 2) return std::nullopt;
            auto q = children[1]->evaluate(vars, subject);
            if (!q || !std::holds_alternative<glm::vec3>(*q)) return std::nullopt;
            auto at = varsAtPoint(vars, std::get<glm::vec3>(*q));
            auto v = children[0]->evaluate(at, subject);
            if (!v) return std::nullopt;
            double d = 0.0;
            if (!propertyValueToNumber(*v, d)) return std::nullopt;
            return PropertyValue(d);
        }

        // Gradient(f, q): the standard central-difference estimate, with the
        // SAME step the marcher's sdfGrad and geom::sdfNormal use. Undefined
        // wherever f is undefined at any of the six sample points — a
        // one-sided guess would be exactly the fabrication this op was fixed
        // for.
        case Op::Gradient: {
            if (children.size() != 2) return std::nullopt;
            auto q = children[1]->evaluate(vars, subject);
            if (!q || !std::holds_alternative<glm::vec3>(*q)) return std::nullopt;
            const glm::vec3 p = std::get<glm::vec3>(*q);
            const float eps = static_cast<float>(kGradientEpsilon);
            glm::vec3 grad(0.0f);
            for (int axis = 0; axis < 3; ++axis) {
                glm::vec3 step(0.0f);
                step[axis] = eps;
                auto hi = children[0]->evaluate(varsAtPoint(vars, p + step), subject);
                auto lo = children[0]->evaluate(varsAtPoint(vars, p - step), subject);
                if (!hi || !lo) return std::nullopt;
                double dh = 0.0, dl = 0.0;
                if (!propertyValueToNumber(*hi, dh) || !propertyValueToNumber(*lo, dl)) {
                    return std::nullopt;
                }
                grad[axis] = static_cast<float>((dh - dl) / (2.0 * kGradientEpsilon));
            }
            return PropertyValue(grad);
        }

        // --- Declared, not implemented -------------------------------------
        // Raycast needs a marching budget and a hit epsilon; LineIntegral needs
        // a curve parameterization and a quadrature rule. Neither is authored
        // anywhere in this tree, so neither can be made to agree across the CPU
        // and GPU paths. UNDEFINED, never a guess (ScalarForm.hpp's own rule,
        // ALGORITHMS_AS_LAW §7). The WGSL emitter REFUSES to compile them
        // rather than emitting a number — see SdfWgsl.cpp.
        case Op::Raycast:
        case Op::LineIntegral:
            return std::nullopt;

        // An op this build does not know: preserved verbatim, never evaluated.
        case Op::Unsupported:
            return std::nullopt;
    }
    return std::nullopt;
}

nlohmann::json MathNode::toJson() const {
    // An op this build cannot read is handed back exactly as it arrived.
    // Re-serializing it from our own fields would drop every payload key we
    // have no slot for, so merely OPENING a world in this build would destroy
    // law text written by another one. (ConditionNode::toJson, same rule.)
    if (op == Op::Unsupported) {
        if (unsupported) return *unsupported;
        return nlohmann::json{{"op", static_cast<int>(Op::Unsupported)}};
    }

    nlohmann::json j;
    j["op"] = static_cast<int>(op);
    if (op == Op::ScalarLeaf) {
        j["scalarForm"] = scalarForm.toJson();
    } else if (op == Op::ValueLeaf) {
        j["var"] = variableName;
    } else if (op == Op::Component || op == Op::Map) {
        j["arg"] = stringArg;
    }
    if (!children.empty()) {
        j["children"] = nlohmann::json::array();
        for (const auto& c : children) {
            j["children"].push_back(c->toJson());
        }
    }
    return j;

}

std::unique_ptr<MathNode> MathNode::fromJson(const nlohmann::json& j) {
    auto node = std::make_unique<MathNode>();

    // THE FIRST SEAM. The stored op is an arbitrary int out of a file. Casting
    // it unchecked (what this used to do) turns "op": 99 into a scoped-enum
    // value outside the enumeration — formally UB, and in practice a node no
    // switch matches, which evaluated to a silent nullopt and re-serialized as
    // a stripped husk. Park it as Unsupported, keep the JSON verbatim, say so.
    const int rawOp = j.value("op", 0);
    if (!isKnownMathOp(rawOp)) {
        node->op = Op::Unsupported;
        node->unsupported = std::make_shared<nlohmann::json>(j);
        // "LAW", not "CONDITION": the audit logger suppresses CONDITION at
        // Summary level, and a malformed save is exactly what must not be
        // suppressed. stderr too — this is a load-time refusal, not a
        // per-frame event, so it cannot become a firehose.
        std::fprintf(stderr,
                     "[OntoMath] MathNode op %d is not known to this build; the "
                     "expression is undefined and preserved verbatim.\n", rawOp);
        ECA::LawAuditLogger::instance().log(
            "LAW",
            "MathNode op " + std::to_string(rawOp) +
                " is not supported by this build - the expression will never "
                "evaluate (undefined, not zero). The node is preserved verbatim "
                "so saving does not destroy it.",
            {{"op", rawOp}});
        return node;
    }
    node->op = static_cast<Op>(rawOp);

    if (j.contains("scalarForm")) {
        node->scalarForm = ScalarForm::fromJson(j["scalarForm"]);
    }
    if (j.contains("var")) {
        node->variableName = j["var"].get<std::string>();
    }
    if (j.contains("arg")) {
        node->stringArg = j["arg"].get<std::string>();
    }
    if (j.contains("children")) {
        for (const auto& c : j["children"]) {
            node->children.push_back(fromJson(c));
        }
    }

    // THE SECOND SEAM, and the reason typeOf exists at all. A deserializer has
    // no binding environment, so this is the LENIENT check: unbound variables
    // answer Unknown and unify with anything, while arity, axis names, function
    // names and every concrete type mismatch are caught and named. The node is
    // still returned — refusing to load law text is worse than loading it with
    // a complaint on the record — but nothing enters the world silently wrong.
    if (!node->children.empty() || node->op == Op::Component || node->op == Op::Map) {
        std::string error;
        if (!node->checkTypes(TypeEnv{}, error, nullptr, /*allowUnbound=*/true)) {
            std::fprintf(stderr, "[OntoMath] MathNode type error on load: %s\n",
                         error.c_str());
            ECA::LawAuditLogger::instance().log(
                "LAW",
                "MathNode failed its type check on load: " + error +
                    ". The expression is undefined and the law that reads it "
                    "will not fire.",
                {{"op", rawOp}, {"error", error}});
        }
    }
    return node;
}

std::shared_ptr<MathNode> MathNode::fromLegacyExpression(const ScalarForm& expr) {
    auto node = std::make_unique<MathNode>();
    node->op = Op::ScalarLeaf;
    node->scalarForm = expr;
    return node;
}


bool Piecewise::Piece::applies(const std::map<std::string, PropertyValue>& vars,
                               const std::string& inputVariable,
                               const Singular* subject) const {
    // The PURE guard first: local mathematics gating local mathematics.
    // Undefined g is unproven — skipped, never guessed.
    if (whereLEZero) {
        const auto g = whereLEZero->evaluate(vars, subject);
        double val = 0.0;
        if (!g || !propertyValueToNumber(*g, val) || val > 0.0) return false;
    }
    if (guard) {
        // A world guard testifies about the SUBJECT: without one it is
        // unproven — the piece is skipped, never guessed.
        if (!subject) return false;
        if (!guardCompiled) {
            const ECA::ConditionPredicate predicate = guard->compile();
            guardCompiled = [predicate](const Singular& s) {
                ECA::Event probe;
                probe.type = "piece-guard";
                return predicate(probe, s);
            };
        }
        return guardCompiled(*subject);
    }
    if (whereLEZero) return true;   // the pure guard already decided
    if (hasLo || hasHi) {
        auto it = vars.find(inputVariable);
        if (it == vars.end()) return false;
        double val = 0.0;
        if (propertyValueToNumber(it->second, val)) return contains(val);
        return false;
    }
    return true;   // everywhere-defined
}

nlohmann::json Piecewise::Piece::toJson() const {
    nlohmann::json j;
    if (mathNode) j["mathNode"] = mathNode->toJson();
    if (hasLo) {
        j["lo"] = lo;
        j["includeLo"] = includeLo;
    }
    if (hasHi) {
        j["hi"] = hi;
        j["includeHi"] = includeHi;
    }
    if (guard) j["guard"] = guard->toJson();
    if (whereLEZero) j["where"] = whereLEZero->toJson();
    if (call) j["call"] = call->toJson();
    if (fold) j["fold"] = fold->toJson();
    return j;
}

Piecewise::Piece Piecewise::Piece::fromJson(const nlohmann::json& j) {
    Piece p;
    if (j.contains("mathNode")) {
        p.mathNode = MathNode::fromJson(j["mathNode"]);
    } else if (j.contains("expr")) {
        p.mathNode = MathNode::fromLegacyExpression(ScalarForm::fromJson(j["expr"]));
    }
    if (j.contains("lo")) {
        p.hasLo = true;
        p.lo = j["lo"].get<double>();
        p.includeLo = j.value("includeLo", true);
    }
    if (j.contains("hi")) {
        p.hasHi = true;
        p.hi = j["hi"].get<double>();
        p.includeHi = j.value("includeHi", true);
    }
    if (j.contains("guard")) {
        p.guard = std::make_shared<ConditionNode>(ConditionNode::fromJson(j["guard"]));
    }
    if (j.contains("where")) {
        p.whereLEZero = MathNode::fromJson(j["where"]);
    }
    if (j.contains("call")) {
        p.call = std::make_shared<FunctionCall>(FunctionCall::fromJson(j["call"]));
    }
    if (j.contains("fold")) {
        p.fold = std::make_shared<Fold>(Fold::fromJson(j["fold"]));
    }
    return p;
}

Piecewise Piecewise::continuous(std::shared_ptr<MathNode> e) {
    Piecewise f;
    Piece piece;
    piece.mathNode = std::move(e);
    f.pieces.push_back(std::move(piece));
    return f;
}

std::optional<PropertyValue> Piecewise::evaluate(const std::map<std::string, PropertyValue>& vars) const {
    return evaluate(vars, nullptr);
}

std::optional<PropertyValue> Piecewise::evaluate(const std::map<std::string, PropertyValue>& vars,
                                          const Singular* subject, int depth) const {
    // First applicable piece wins (author's order): guarded pieces ask
    // their condition (about the subject); interval pieces ask their
    // bounds; a bare piece is everywhere-defined.
    for (const auto& piece : pieces) {
        if (!piece.applies(vars, inputVariable, subject)) continue;
        if (piece.call) {
            // Composition/recursion: evaluate the args in the CALLER's
            // variables, bind them to the definition's parameters, and
            // evaluate the pure body. Divergence meets the ceiling.
            if (depth >= FunctionRegistry::kMaxCallDepth) return std::nullopt;
            const FunctionDef* def =
                FunctionRegistry::instance().find(piece.call->function);
            if (!def || def->params.size() != piece.call->args.size()) {
                return std::nullopt;   // unknown word or wrong arity: honest
            }
            std::map<std::string, PropertyValue> bound;
            for (std::size_t i = 0; i < def->params.size(); ++i) {
                std::map<std::string, double> dVars;
                for (const auto& [k,v] : vars) {
                    double num;
                    if (propertyValueToNumber(v, num)) dVars[k] = num;
                }
                const auto value = piece.call->args[i].evaluate(dVars);
                if (!value) return std::nullopt;
                bound[def->params[i]] = *value;
            }
            return def->body.evaluate(bound, subject, depth + 1);
        }
        if (piece.fold) {
            // The discrete Σ: aggregate one property across every being of
            // the kind, with possible exceptions. Empty sum/count are their
            // honest identities; empty mean/min/max are undefined.
            const Fold& fold = *piece.fold;
            const PropertyPath foldPath = PropertyPath::parse(fold.path);
            double sum = 0.0;
            double mn = 0.0, mx = 0.0;
            int counted = 0;
            for (Singular* being : Universe::instance().beings()) {
                if (!being) continue;
                if (!ConditionNode::matchesKind(
                        *being, static_cast<ConditionNode::BeingKind>(fold.beingKind))) {
                    continue;
                }
                if (std::find(fold.exceptIds.begin(), fold.exceptIds.end(),
                              being->getIdentifier()) != fold.exceptIds.end()) {
                    continue;
                }
                if (fold.op == Fold::Op::Count && fold.path.empty()) {
                    ++counted;   // counting BEINGS, no property needed
                    continue;
                }
                PropertyValue value;
                double x = 0.0;
                if (!lawGetValue(*being, foldPath, value) ||
                    !propertyValueToNumber(value, x)) {
                    continue;   // this being cannot testify to the property
                }
                if (counted == 0) {
                    mn = mx = x;
                } else {
                    mn = std::min(mn, x);
                    mx = std::max(mx, x);
                }
                sum += x;
                ++counted;
            }
            switch (fold.op) {
                case Fold::Op::Sum: return sum;                     // empty Σ = 0
                case Fold::Op::Count: return static_cast<double>(counted);
                case Fold::Op::Mean:
                    if (counted == 0) return std::nullopt;
                    return sum / counted;
                case Fold::Op::Min:
                    if (counted == 0) return std::nullopt;
                    return mn;
                case Fold::Op::Max:
                    if (counted == 0) return std::nullopt;
                    return mx;
            }
            return std::nullopt;
        }
        // A piece with no value at all (no mathNode, no call, no fold) is
        // malformed law text, not a value of zero — and dereferencing it was a
        // null deref one bad save away.
        if (!piece.mathNode) return std::nullopt;
        return piece.mathNode->evaluate(vars, subject);
    }
    return std::nullopt;   // outside every piece: undefined, not zero
}

// ---------------------------------------------------------------------------
// Fold
// ---------------------------------------------------------------------------

nlohmann::json Fold::toJson() const {
    nlohmann::json j{{"op", static_cast<int>(op)},
                     {"kind", beingKind},
                     {"path", path}};
    if (!exceptIds.empty()) j["except"] = exceptIds;
    return j;
}

Fold Fold::fromJson(const nlohmann::json& j) {
    Fold f;
    f.op = static_cast<Op>(j.value("op", 0));
    f.beingKind = j.value("kind", 1);
    f.path = j.value("path", std::string());
    if (j.contains("except")) {
        f.exceptIds = j["except"].get<std::vector<std::string>>();
    }
    return f;
}

// ---------------------------------------------------------------------------
// FunctionCall / FunctionDef / FunctionRegistry
// ---------------------------------------------------------------------------

nlohmann::json FunctionCall::toJson() const {
    nlohmann::json argsJson = nlohmann::json::array();
    for (const auto& arg : args) argsJson.push_back(arg.toJson());
    return nlohmann::json{{"fn", function}, {"args", argsJson}};
}

FunctionCall FunctionCall::fromJson(const nlohmann::json& j) {
    FunctionCall c;
    c.function = j.value("fn", std::string());
    if (j.contains("args")) {
        for (const auto& a : j["args"]) c.args.push_back(ScalarForm::fromJson(a));
    }
    return c;
}

nlohmann::json FunctionDef::toJson() const {
    return nlohmann::json{{"name", name}, {"params", params}, {"body", body.toJson()}};
}

FunctionDef FunctionDef::fromJson(const nlohmann::json& j) {
    FunctionDef def;
    def.name = j.value("name", std::string());
    if (j.contains("params")) {
        def.params = j["params"].get<std::vector<std::string>>();
    }
    if (j.contains("body")) def.body = Piecewise::fromJson(j["body"]);
    return def;
}

FunctionRegistry& FunctionRegistry::instance() {
    static FunctionRegistry registry;
    return registry;
}

void FunctionRegistry::define(FunctionDef def) {
    if (def.name.empty()) return;
    for (auto& existing : _functions) {
        if (existing.name == def.name) {
            existing = std::move(def);   // redefinition: the word is renewed
            return;
        }
    }
    _functions.push_back(std::move(def));
}

bool FunctionRegistry::remove(const std::string& name) {
    const auto before = _functions.size();
    _functions.erase(std::remove_if(_functions.begin(), _functions.end(),
                                    [&](const FunctionDef& def) {
                                        return def.name == name;
                                    }),
                     _functions.end());
    return _functions.size() != before;
}

const FunctionDef* FunctionRegistry::find(const std::string& name) const {
    for (const auto& def : _functions) {
        if (def.name == name) return &def;
    }
    return nullptr;
}

nlohmann::json FunctionRegistry::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& def : _functions) arr.push_back(def.toJson());
    return nlohmann::json{{"functions", arr}};
}

void FunctionRegistry::loadFromJson(const nlohmann::json& j) {
    _functions.clear();
    if (!j.contains("functions")) return;
    for (const auto& d : j["functions"]) define(FunctionDef::fromJson(d));
}

std::string Piecewise::print() const {
    if (pieces.size() == 1 && !pieces[0].hasLo && !pieces[0].hasHi &&
        !pieces[0].guard) {
        return pieces[0].mathNode ? "[MathNode]" : "[Empty]";
    }
    bool anyGuard = false;
    for (const auto& piece : pieces) anyGuard = anyGuard || piece.guard != nullptr;
    return "piecewise(" + std::to_string(pieces.size()) +
           (anyGuard ? " guarded" : " over " + inputVariable) + ")";
}

nlohmann::json Piecewise::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& piece : pieces) arr.push_back(piece.toJson());
    return nlohmann::json{{"input", inputVariable}, {"pieces", arr}};
}

Piecewise Piecewise::fromJson(const nlohmann::json& j) {
    Piecewise f;
    f.inputVariable = j.value("input", std::string("x"));
    if (j.contains("pieces")) {
        for (const auto& p : j["pieces"]) f.pieces.push_back(Piece::fromJson(p));
    }
    return f;
}

} // namespace OntoMath
