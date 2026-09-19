#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include "Singularity/OntoMath/Operations.hpp"

// The guard is the condition calculus itself — pieces gated by law-text.
// (Include at the .cpp level only: ConditionModel.hpp includes this header.)
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <glm/gtc/noise.hpp>

namespace OntoMath {

std::atomic<uint32_t> g_astEvaluations{0};


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

Interval TransFactor::evalRange(const Interval& x) const {
    const Interval arg = x * Interval(static_cast<float>(scale)) +
                         Interval(static_cast<float>(shift));
    switch (kind) {
        case Kind::Sin:
        case Kind::Cos: {
            // The honest bound, not the lazy [-1, 1]. sin over an arc shorter
            // than a full turn only reaches +1 if a peak (pi/2 + 2*pi*k) lies
            // inside it; otherwise the extrema are the endpoints. Getting this
            // right is what makes "0.5*sin(t) + 0.5 can never exceed 1" a
            // usable proof instead of a truism.
            if (!arg.bounded()) return Interval(-1.0f, 1.0f);
            const double lo = arg.lo, hi = arg.hi;
            if (hi - lo >= 2.0 * M_PI) return Interval(-1.0f, 1.0f);
            const auto f = [&](double t) {
                return kind == Kind::Sin ? std::sin(t) : std::cos(t);
            };
            double mn = std::min(f(lo), f(hi));
            double mx = std::max(f(lo), f(hi));
            // Peak/trough phases: sin peaks at pi/2, cos at 0; each repeats
            // every 2*pi, with the trough half a turn later.
            const double peak = kind == Kind::Sin ? M_PI / 2.0 : 0.0;
            const auto sweeps = [&](double phase) {
                const double k = std::ceil((lo - phase) / (2.0 * M_PI));
                return phase + k * 2.0 * M_PI <= hi;
            };
            if (sweeps(peak)) mx = 1.0;
            if (sweeps(peak + M_PI)) mn = -1.0;
            return Interval(static_cast<float>(mn), static_cast<float>(mx));
        }
        case Kind::Exp: {
            if (!arg.bounded()) {
                // exp is monotone, so an unbounded side stays unbounded on
                // that side only -- and exp is never negative.
                return Interval(std::isfinite(arg.lo)
                                    ? static_cast<float>(std::exp(arg.lo))
                                    : 0.0f,
                                std::isfinite(arg.hi)
                                    ? static_cast<float>(std::exp(arg.hi))
                                    : std::numeric_limits<float>::infinity());
            }
            return Interval(static_cast<float>(std::exp(arg.lo)),
                            static_cast<float>(std::exp(arg.hi)));
        }
        case Kind::Ln: {
            // ln is undefined at or below zero. Where part of the argument's
            // range is positive, bound only that part; where NONE of it is,
            // the term has no value at all and claiming a bound would be a
            // claim about nothing -- answer unbounded and let the caller's
            // own refusal stand.
            if (arg.hi <= 0.0f) return Interval::infinite();
            const float lo = arg.lo > 0.0f
                                 ? static_cast<float>(std::log(arg.lo))
                                 : -std::numeric_limits<float>::infinity();
            const float hi = std::isfinite(arg.hi)
                                 ? static_cast<float>(std::log(arg.hi))
                                 : std::numeric_limits<float>::infinity();
            return Interval(lo, hi);
        }
    }
    return Interval::infinite();
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

namespace {

// x^e over an interval, sound for every real exponent this algebra admits.
// For an INTEGER exponent the power is monotone on each side of zero, so the
// endpoints bound it -- except across zero, where an even power turns around
// (min 0) and a negative one crosses a pole. A NON-INTEGER exponent is
// undefined for negative x, so an interval reaching below zero is refused
// outright rather than bounded by a value the function never takes.
Interval powRange(const Interval& x, double e) {
    if (e == 0.0) return Interval(1.0f);
    const bool isInt = std::floor(e) == e && std::isfinite(e);
    const auto endpoints = [&](double a, double b) {
        const double pa = std::pow(a, e), pb = std::pow(b, e);
        if (!std::isfinite(pa) && !std::isfinite(pb)) return Interval::infinite();
        return Interval(static_cast<float>(std::min(pa, pb)),
                        static_cast<float>(std::max(pa, pb)));
    };
    const bool spansZero = x.lo <= 0.0f && x.hi >= 0.0f;
    if (!spansZero) {
        if (!isInt && x.hi < 0.0f) return Interval::infinite();   // undefined
        if (!x.bounded()) {
            // One endpoint at infinity: only the finite side is a real bound,
            // and only when the power is monotone toward it. Not worth a case
            // analysis nobody can check -- take the unbounded answer.
            return Interval::infinite();
        }
        return endpoints(x.lo, x.hi);
    }
    if (!isInt) return Interval::infinite();                      // undefined below 0
    if (e < 0.0) return Interval::infinite();                     // pole at 0
    if (!x.bounded()) return Interval::infinite();
    const long long n = static_cast<long long>(e);
    if (n % 2 == 0) {
        const double m = std::max(std::pow(std::fabs(static_cast<double>(x.lo)), e),
                                  std::pow(std::fabs(static_cast<double>(x.hi)), e));
        return Interval(0.0f, static_cast<float>(m));
    }
    return endpoints(x.lo, x.hi);
}

Interval rangeOfVar(const std::map<std::string, Interval>& vars, const std::string& name) {
    auto it = vars.find(name);
    return it == vars.end() ? Interval::infinite() : it->second;
}

} // namespace

Interval Term::evalRange(const std::map<std::string, Interval>& vars) const {
    Interval acc(static_cast<float>(coefficient));
    if (coefficient == 0.0) return Interval(0.0f);   // 0 * anything is 0
    for (const auto& [name, exp] : factors) {
        acc = acc * powRange(rangeOfVar(vars, name), exp);
    }
    for (const auto& tf : trans) {
        acc = acc * tf.evalRange(rangeOfVar(vars, tf.variable));
    }
    return acc;
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

Interval ScalarForm::evalRange(const std::map<std::string, Interval>& vars) const {
    if (terms.empty()) return Interval(0.0f);
    Interval sum(0.0f);
    for (const auto& term : terms) sum = sum + term.evalRange(vars);
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
// Polynomial & Bernstein Basis Conversions
// ---------------------------------------------------------------------------

ScalarForm ScalarForm::bernsteinBasis(int n, int i, const std::string& var) {
    if (i < 0 || i > n) return ScalarForm::constant(0.0);
    double cni = Operations::binom(n, i);
    ScalarForm form;
    for (int k = 0; k <= n - i; ++k) {
        double cnk = Operations::binom(n - i, k);
        double sign = (k % 2 == 0) ? 1.0 : -1.0;
        double coeff = cni * cnk * sign;
        if (coeff != 0.0) {
            Term t(coeff);
            int exp = i + k;
            if (exp > 0) t.factors[var] = static_cast<double>(exp);
            form.terms.push_back(std::move(t));
        }
    }
    return form.normalized();
}

ScalarForm ScalarForm::fromBernstein(int degree, const std::vector<double>& controlPoints, const std::string& var) {
    ScalarForm total = ScalarForm::constant(0.0);
    for (int i = 0; i <= degree && i < static_cast<int>(controlPoints.size()); ++i) {
        if (controlPoints[i] != 0.0) {
            total = total.plus(bernsteinBasis(degree, i, var).scaled(controlPoints[i]));
        }
    }
    return total.normalized();
}

ScalarForm ScalarForm::fromBivariateBernstein(int du, int dv, const std::vector<double>& grid,
                                             const std::string& uVar, const std::string& vVar) {
    int nu = du + 1, nv = dv + 1;
    ScalarForm total = ScalarForm::constant(0.0);
    if (static_cast<int>(grid.size()) < nu * nv) return total;
    for (int j = 0; j <= dv; ++j) {
        ScalarForm bj = bernsteinBasis(dv, j, vVar);
        for (int i = 0; i <= du; ++i) {
            double p = grid[j * nu + i];
            if (p != 0.0) {
                ScalarForm bi = bernsteinBasis(du, i, uVar);
                total = total.plus(bi.times(bj).scaled(p));
            }
        }
    }
    return total.normalized();
}

std::vector<double> ScalarForm::toBivariateBernstein(const ScalarForm& form, int du, int dv,
                                                    const std::string& uVar, const std::string& vVar) {
    int nu = du + 1, nv = dv + 1;
    std::vector<double> grid(static_cast<size_t>(nu) * nv, 0.0);
    std::vector<double> a(static_cast<size_t>(nu) * nv, 0.0);
    for (const auto& term : form.terms) {
        if (!term.trans.empty()) continue;
        double expU = 0.0, expV = 0.0;
        bool valid = true;
        for (const auto& [var, exp] : term.factors) {
            if (var == uVar) expU = exp;
            else if (var == vVar) expV = exp;
            else { valid = false; break; }
        }
        if (!valid) continue;
        int k = static_cast<int>(std::round(expU));
        int l = static_cast<int>(std::round(expV));
        if (k >= 0 && k <= du && l >= 0 && l <= dv) {
            a[l * nu + k] += term.coefficient;
        }
    }
    for (int i = 0; i <= du; ++i) {
        for (int j = 0; j <= dv; ++j) {
            double sum = 0.0;
            for (int k = 0; k <= i; ++k) {
                double denomU = Operations::binom(du, k);
                double cu = (denomU != 0.0) ? Operations::binom(i, k) / denomU : 0.0;
                for (int l = 0; l <= j; ++l) {
                    double denomV = Operations::binom(dv, l);
                    double cv = (denomV != 0.0) ? Operations::binom(j, l) / denomV : 0.0;
                    sum += cu * cv * a[l * nu + k];
                }
            }
            grid[j * nu + i] = sum;
        }
    }
    return grid;
}

// ---------------------------------------------------------------------------
// MathNode Geometric Helpers
// ---------------------------------------------------------------------------

std::unique_ptr<MathNode> MathNode::sphere(double radius, const std::string& pVar) {
    auto p = std::make_unique<MathNode>();
    p->op = Op::ValueLeaf;
    p->variableName = pVar;

    auto len = std::make_unique<MathNode>();
    len->op = Op::Length;
    len->children.push_back(std::move(p));

    auto r = std::make_unique<MathNode>();
    r->op = Op::ScalarLeaf;
    r->scalarForm = ScalarForm::constant(radius);

    auto sub = std::make_unique<MathNode>();
    sub->op = Op::Sub;
    sub->children.push_back(std::move(len));
    sub->children.push_back(std::move(r));
    return sub;
}

std::unique_ptr<MathNode> MathNode::unionOp(std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::Union;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

std::unique_ptr<MathNode> MathNode::intersectionOp(std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::Intersection;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

std::unique_ptr<MathNode> MathNode::differenceOp(std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::Difference;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

// box / cylinder / torus / smoothUnion: NOT YET EXPRESSIBLE -- they refuse.
//
// These three shipped as stubs that returned `sphere(...)`: box discarded two
// of its three half-extents, cylinder discarded its height, torus discarded its
// minor radius, and smoothUnion discarded its blend parameter and became a hard
// union. Nothing tested them, so a box silently WAS a sphere everywhere the
// AST was consulted -- and SdfNode::toMathNode consults it for exactly these
// kinds.
//
// Writing them honestly needs vocabulary this build does not have. The
// canonical SDFs are
//     box:      length(max(|p| - b, 0)) + min(max(qx, qy, qz), 0)
//     cylinder: max(length(p.xz) - r, |p.y| - h)
//     torus:    length(vec2(length(p.xz) - R, p.y)) - r
//     smin:     the polynomial blend, which needs clamp
// and MathNode::Op has no componentwise absolute value, no clamp, and no
// scalar min/max (Union/Intersection are the CSG booleans over whole distance
// fields, not general min/max). Op is APPEND-ONLY and serialized as ints, so
// widening it is a deliberate ontological act with its own tests -- not
// something to smuggle into a factory.
//
// Until then: refuse. A caller gets nullptr and knows it got nothing, which is
// the outcome a sphere-shaped lie was hiding.
std::unique_ptr<MathNode> MathNode::box(glm::vec3 halfExtents, const std::string& pVar) {
    auto p = std::make_unique<MathNode>();
    p->op = Op::ValueLeaf;
    p->variableName = pVar;

    auto absP = std::make_unique<MathNode>();
    absP->op = Op::Abs;
    absP->children.push_back(std::move(p));

    auto b = std::make_unique<MathNode>();
    b->op = Op::VectorConstruct;
    auto bx = std::make_unique<MathNode>(); bx->op = Op::ScalarLeaf; bx->scalarForm = ScalarForm::constant(halfExtents.x);
    auto by = std::make_unique<MathNode>(); by->op = Op::ScalarLeaf; by->scalarForm = ScalarForm::constant(halfExtents.y);
    auto bz = std::make_unique<MathNode>(); bz->op = Op::ScalarLeaf; bz->scalarForm = ScalarForm::constant(halfExtents.z);
    b->children.push_back(std::move(bx));
    b->children.push_back(std::move(by));
    b->children.push_back(std::move(bz));

    auto q = std::make_unique<MathNode>();
    q->op = Op::Sub;
    q->children.push_back(std::move(absP));
    q->children.push_back(std::move(b));

    auto zeroVec = std::make_unique<MathNode>();
    zeroVec->op = Op::VectorConstruct;
    auto z0 = std::make_unique<MathNode>(); z0->op = Op::ScalarLeaf; z0->scalarForm = ScalarForm::constant(0.0);
    auto z1 = std::make_unique<MathNode>(); z1->op = Op::ScalarLeaf; z1->scalarForm = ScalarForm::constant(0.0);
    auto z2 = std::make_unique<MathNode>(); z2->op = Op::ScalarLeaf; z2->scalarForm = ScalarForm::constant(0.0);
    zeroVec->children.push_back(std::move(z0));
    zeroVec->children.push_back(std::move(z1));
    zeroVec->children.push_back(std::move(z2));

    auto qClone1 = std::make_unique<MathNode>(*q);
    auto maxQ0 = std::make_unique<MathNode>();
    maxQ0->op = Op::Intersection;
    maxQ0->children.push_back(std::move(qClone1));
    maxQ0->children.push_back(std::move(zeroVec));

    auto outer = std::make_unique<MathNode>();
    outer->op = Op::Length;
    outer->children.push_back(std::move(maxQ0));

    auto qClone2 = std::make_unique<MathNode>(*q);
    auto qClone3 = std::make_unique<MathNode>(*q);
    auto qClone4 = std::make_unique<MathNode>(*q);

    auto qx = std::make_unique<MathNode>(); qx->op = Op::Component; qx->stringArg = "x"; qx->children.push_back(std::move(qClone2));
    auto qy = std::make_unique<MathNode>(); qy->op = Op::Component; qy->stringArg = "y"; qy->children.push_back(std::move(qClone3));
    auto qz = std::make_unique<MathNode>(); qz->op = Op::Component; qz->stringArg = "z"; qz->children.push_back(std::move(qClone4));

    auto maxQyz = std::make_unique<MathNode>(); maxQyz->op = Op::Intersection; maxQyz->children.push_back(std::move(qy)); maxQyz->children.push_back(std::move(qz));
    auto maxQxyz = std::make_unique<MathNode>(); maxQxyz->op = Op::Intersection; maxQxyz->children.push_back(std::move(qx)); maxQxyz->children.push_back(std::move(maxQyz));

    auto zeroScalar = std::make_unique<MathNode>(); zeroScalar->op = Op::ScalarLeaf; zeroScalar->scalarForm = ScalarForm::constant(0.0);
    auto inner = std::make_unique<MathNode>(); inner->op = Op::Union; inner->children.push_back(std::move(maxQxyz)); inner->children.push_back(std::move(zeroScalar));

    auto result = std::make_unique<MathNode>();
    result->op = Op::Add;
    result->children.push_back(std::move(outer));
    result->children.push_back(std::move(inner));
    return result;
}

std::unique_ptr<MathNode> MathNode::cylinder(double radius, double halfHeight, const std::string& pVar) {
    auto p1 = std::make_unique<MathNode>(); p1->op = Op::ValueLeaf; p1->variableName = pVar;
    auto p2 = std::make_unique<MathNode>(); p2->op = Op::ValueLeaf; p2->variableName = pVar;
    auto p3 = std::make_unique<MathNode>(); p3->op = Op::ValueLeaf; p3->variableName = pVar;

    auto px = std::make_unique<MathNode>(); px->op = Op::Component; px->stringArg = "x"; px->children.push_back(std::move(p1));
    auto py = std::make_unique<MathNode>(); py->op = Op::Component; py->stringArg = "y"; py->children.push_back(std::move(p2));
    auto pz = std::make_unique<MathNode>(); pz->op = Op::Component; pz->stringArg = "z"; pz->children.push_back(std::move(p3));

    auto zeroScalar = std::make_unique<MathNode>(); zeroScalar->op = Op::ScalarLeaf; zeroScalar->scalarForm = ScalarForm::constant(0.0);
    auto pxz = std::make_unique<MathNode>(); pxz->op = Op::VectorConstruct;
    pxz->children.push_back(std::move(px));
    pxz->children.push_back(std::move(zeroScalar));
    pxz->children.push_back(std::move(pz));

    auto lenXZ = std::make_unique<MathNode>(); lenXZ->op = Op::Length; lenXZ->children.push_back(std::move(pxz));
    auto r = std::make_unique<MathNode>(); r->op = Op::ScalarLeaf; r->scalarForm = ScalarForm::constant(radius);
    auto d_xz = std::make_unique<MathNode>(); d_xz->op = Op::Sub; d_xz->children.push_back(std::move(lenXZ)); d_xz->children.push_back(std::move(r));

    auto absPy = std::make_unique<MathNode>(); absPy->op = Op::Abs; absPy->children.push_back(std::move(py));
    auto h = std::make_unique<MathNode>(); h->op = Op::ScalarLeaf; h->scalarForm = ScalarForm::constant(halfHeight);
    auto d_y = std::make_unique<MathNode>(); d_y->op = Op::Sub; d_y->children.push_back(std::move(absPy)); d_y->children.push_back(std::move(h));

    auto result = std::make_unique<MathNode>();
    result->op = Op::Intersection;
    result->children.push_back(std::move(d_xz));
    result->children.push_back(std::move(d_y));
    return result;
}

std::unique_ptr<MathNode> MathNode::torus(double majorR, double minorR, const std::string& pVar) {
    auto p1 = std::make_unique<MathNode>(); p1->op = Op::ValueLeaf; p1->variableName = pVar;
    auto p2 = std::make_unique<MathNode>(); p2->op = Op::ValueLeaf; p2->variableName = pVar;
    auto p3 = std::make_unique<MathNode>(); p3->op = Op::ValueLeaf; p3->variableName = pVar;

    auto px = std::make_unique<MathNode>(); px->op = Op::Component; px->stringArg = "x"; px->children.push_back(std::move(p1));
    auto py = std::make_unique<MathNode>(); py->op = Op::Component; py->stringArg = "y"; py->children.push_back(std::move(p2));
    auto pz = std::make_unique<MathNode>(); pz->op = Op::Component; pz->stringArg = "z"; pz->children.push_back(std::move(p3));

    auto zeroScalar1 = std::make_unique<MathNode>(); zeroScalar1->op = Op::ScalarLeaf; zeroScalar1->scalarForm = ScalarForm::constant(0.0);
    auto pxz = std::make_unique<MathNode>(); pxz->op = Op::VectorConstruct;
    pxz->children.push_back(std::move(px));
    pxz->children.push_back(std::move(zeroScalar1));
    pxz->children.push_back(std::move(pz));

    auto lenXZ = std::make_unique<MathNode>(); lenXZ->op = Op::Length; lenXZ->children.push_back(std::move(pxz));
    auto majR = std::make_unique<MathNode>(); majR->op = Op::ScalarLeaf; majR->scalarForm = ScalarForm::constant(majorR);
    auto qX = std::make_unique<MathNode>(); qX->op = Op::Sub; qX->children.push_back(std::move(lenXZ)); qX->children.push_back(std::move(majR));

    auto zeroScalar2 = std::make_unique<MathNode>(); zeroScalar2->op = Op::ScalarLeaf; zeroScalar2->scalarForm = ScalarForm::constant(0.0);
    auto qVec = std::make_unique<MathNode>(); qVec->op = Op::VectorConstruct;
    qVec->children.push_back(std::move(qX));
    qVec->children.push_back(std::move(py));
    qVec->children.push_back(std::move(zeroScalar2));

    auto lenQ = std::make_unique<MathNode>(); lenQ->op = Op::Length; lenQ->children.push_back(std::move(qVec));
    auto minR = std::make_unique<MathNode>(); minR->op = Op::ScalarLeaf; minR->scalarForm = ScalarForm::constant(minorR);
    auto result = std::make_unique<MathNode>(); result->op = Op::Sub; result->children.push_back(std::move(lenQ)); result->children.push_back(std::move(minR));
    return result;
}

std::unique_ptr<MathNode> MathNode::smoothUnionOp(std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b, double k) {
    if (k <= 1e-5) return unionOp(std::move(a), std::move(b));
    auto aClone1 = std::make_unique<MathNode>(*a);
    auto bClone1 = std::make_unique<MathNode>(*b);
    auto aClone2 = std::make_unique<MathNode>(*a);
    auto bClone2 = std::make_unique<MathNode>(*b);

    auto bMinusA = std::make_unique<MathNode>(); bMinusA->op = Op::Sub; bMinusA->children.push_back(std::move(bClone1)); bMinusA->children.push_back(std::move(aClone1));
    auto scaleTerm = std::make_unique<MathNode>(); scaleTerm->op = Op::Scale;
    auto coeff = std::make_unique<MathNode>(); coeff->op = Op::ScalarLeaf; coeff->scalarForm = ScalarForm::constant(0.5 / k);
    scaleTerm->children.push_back(std::move(coeff));
    scaleTerm->children.push_back(std::move(bMinusA));

    auto halfVal = std::make_unique<MathNode>(); halfVal->op = Op::ScalarLeaf; halfVal->scalarForm = ScalarForm::constant(0.5);
    auto halfPlus = std::make_unique<MathNode>(); halfPlus->op = Op::Add; halfPlus->children.push_back(std::move(halfVal)); halfPlus->children.push_back(std::move(scaleTerm));

    auto lo = std::make_unique<MathNode>(); lo->op = Op::ScalarLeaf; lo->scalarForm = ScalarForm::constant(0.0);
    auto hi = std::make_unique<MathNode>(); hi->op = Op::ScalarLeaf; hi->scalarForm = ScalarForm::constant(1.0);
    auto h = std::make_unique<MathNode>(); h->op = Op::Clamp; h->children.push_back(std::move(halfPlus)); h->children.push_back(std::move(lo)); h->children.push_back(std::move(hi));

    auto hClone1 = std::make_unique<MathNode>(*h);
    auto hClone2 = std::make_unique<MathNode>(*h);
    auto hClone3 = std::make_unique<MathNode>(*h);

    auto oneVal = std::make_unique<MathNode>(); oneVal->op = Op::ScalarLeaf; oneVal->scalarForm = ScalarForm::constant(1.0);
    auto oneMinusH = std::make_unique<MathNode>(); oneMinusH->op = Op::Sub; oneMinusH->children.push_back(std::move(oneVal)); oneMinusH->children.push_back(std::move(hClone1));
    auto oneMinusHClone = std::make_unique<MathNode>(*oneMinusH);

    auto termB = std::make_unique<MathNode>(); termB->op = Op::Scale; termB->children.push_back(std::move(oneMinusH)); termB->children.push_back(std::move(bClone2));
    auto termA = std::make_unique<MathNode>(); termA->op = Op::Scale; termA->children.push_back(std::move(hClone2)); termA->children.push_back(std::move(aClone2));
    auto mixBA = std::make_unique<MathNode>(); mixBA->op = Op::Add; mixBA->children.push_back(std::move(termB)); mixBA->children.push_back(std::move(termA));

    auto hTimesOneMinusH = std::make_unique<MathNode>(); hTimesOneMinusH->op = Op::Scale; hTimesOneMinusH->children.push_back(std::move(hClone3)); hTimesOneMinusH->children.push_back(std::move(oneMinusHClone));
    auto kCoeff = std::make_unique<MathNode>(); kCoeff->op = Op::ScalarLeaf; kCoeff->scalarForm = ScalarForm::constant(k);
    auto kBlend = std::make_unique<MathNode>(); kBlend->op = Op::Scale; kBlend->children.push_back(std::move(kCoeff)); kBlend->children.push_back(std::move(hTimesOneMinusH));

    auto result = std::make_unique<MathNode>(); result->op = Op::Sub; result->children.push_back(std::move(mixBA)); result->children.push_back(std::move(kBlend));
    return result;
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
        case MathNode::Op::Div:             return "Div";
        case MathNode::Op::Pow:             return "Pow";
        case MathNode::Op::Abs:             return "Abs";
        case MathNode::Op::Clamp:           return "Clamp";
        case MathNode::Op::Sqrt:            return "Sqrt";
        case MathNode::Op::Tan:             return "Tan";
        case MathNode::Op::Noise:           return "Noise";
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
        case MathNode::Op::Div:
        case MathNode::Op::Pow:
        case MathNode::Op::Abs:
        case MathNode::Op::Clamp:
        case MathNode::Op::Sqrt:
        case MathNode::Op::Tan:
        case MathNode::Op::Noise:
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
        case Op::Intersection: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (*t0 == ValueKind::Unknown || *t1 == ValueKind::Unknown) return TypeResult::ok(ValueKind::Unknown);
            if (scalarLike(*t0) && scalarLike(*t1)) return TypeResult::ok(widerScalar(*t0, *t1));
            if (vectorLike(*t0) || vectorLike(*t1)) return TypeResult::ok(ValueKind::Vector);
            return TypeResult::ok(widerScalar(*t0, *t1));
        }
        case Op::Difference: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (!scalarLike(*t0)) return mismatch(path, op, 0, "ScalarField", *t0);
            if (!scalarLike(*t1)) return mismatch(path, op, 1, "ScalarField", *t1);
            return TypeResult::ok(widerScalar(*t0, *t1));
        }
        case Op::Div:
        case Op::Pow: {
            if (children.size() != 2) return arity(path, op, children.size(), 2);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            if (*t0 == ValueKind::Unknown || *t1 == ValueKind::Unknown) return TypeResult::ok(ValueKind::Unknown);
            if (scalarLike(*t0) && scalarLike(*t1)) return TypeResult::ok(widerScalar(*t0, *t1));
            if (vectorLike(*t0)) return TypeResult::ok(ValueKind::Vector);
            return TypeResult::ok(widerScalar(*t0, *t1));
        }
        case Op::Abs:
        case Op::Sqrt: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            auto t = sub(0); if (!t) return t;
            if (*t == ValueKind::Unknown) return TypeResult::ok(ValueKind::Unknown);
            if (vectorLike(*t)) return TypeResult::ok(ValueKind::Vector);
            return TypeResult::ok(widerScalar(*t, *t));
        }
        case Op::Clamp: {
            if (children.size() != 3) return arity(path, op, children.size(), 3);
            auto t0 = sub(0); if (!t0) return t0;
            auto t1 = sub(1); if (!t1) return t1;
            auto t2 = sub(2); if (!t2) return t2;
            if (*t0 == ValueKind::Unknown) return TypeResult::ok(ValueKind::Unknown);
            if (vectorLike(*t0)) return TypeResult::ok(ValueKind::Vector);
            return TypeResult::ok(widerScalar(*t0, widerScalar(*t1, *t2)));
        }
        case Op::Tan: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            auto t = sub(0); if (!t) return t;
            if (!scalarLike(*t)) return mismatch(path, op, 0, "Scalar", *t);
            return TypeResult::ok(widerScalar(*t, *t));
        }
        case Op::Noise: {
            if (children.size() != 1) return arity(path, op, children.size(), 1);
            auto t = sub(0); if (!t) return t;
            if (!vectorLike(*t)) return mismatch(path, op, 0, "Vector", *t);
            return TypeResult::ok(ValueKind::Scalar);
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
    thread_local uint32_t t_localCount = 0;
    t_localCount++;
    if (t_localCount >= 1024) {
        g_astEvaluations.fetch_add(t_localCount, std::memory_order_relaxed);
        t_localCount = 0;
    }
    switch(op) {
        case Op::ScalarLeaf: {
            // Bind only what this form actually reads. Term::evaluate looks
            // every variable up by name and never inspects the rest of the
            // map, and ScalarForm::evaluate only sums terms, so narrowing the
            // environment is exact -- an entry no term mentions could never
            // have been observed.
            //
            // The win is the constant form, which is the overwhelming case
            // inside a field expression: every numeric literal in the tree is
            // a ScalarLeaf with no factors, and it needs no environment at
            // all. Projecting the whole of `vars` here instead cost a
            // red-black-tree insert per bound variable per literal per
            // evaluation -- for the noise floor's tree, six literals x four
            // variables = 24 allocations on every SDF sample, of which a
            // tessellation takes millions.
            bool needsVars = false;
            for (const auto& t : scalarForm.terms) {
                if (!t.factors.empty() || !t.trans.empty()) { needsVars = true; break; }
            }
            if (!needsVars) {
                static const std::map<std::string, double> kNoVars;
                auto res = scalarForm.evaluate(kNoVars);
                if (!res) return std::nullopt;
                return PropertyValue(*res);
            }
            std::map<std::string, double> scalarVars;
            auto bind = [&](const std::string& name) {
                if (scalarVars.find(name) != scalarVars.end()) return;
                auto it = vars.find(name);
                if (it == vars.end()) return;   // unbound: Term::evaluate refuses
                double d = 0.0;
                if (propertyValueToNumber(it->second, d)) scalarVars.emplace(name, d);
            };
            for (const auto& t : scalarForm.terms) {
                for (const auto& f : t.factors) bind(f.first);
                for (const auto& tf : t.trans) bind(tf.variable);
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
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            const bool bVec = std::holds_alternative<glm::vec3>(*b);
            if (aVec && bVec) {
                return PropertyValue(std::get<glm::vec3>(*a) + std::get<glm::vec3>(*b));
            }
            if (aVec != bVec) return std::nullopt;   // vector + scalar is not addition
            // COERCING, like Op::Scale below. PropertyValue carries `float` and
            // `double` as SEPARATE alternatives, and plenty of scalars arrive as
            // float -- every PropertyRef<T, float> on a being, and (until this
            // same commit) Length/Dot/Distance, which returned glm's float. A
            // strict holds_alternative<double> pair therefore refused ordinary
            // arithmetic and returned nullopt, which poisons every enclosing
            // node silently. That is what made MathNode::sphere -- Sub(Length(p),
            // ScalarLeaf(r)) -- undefined for every input it was ever given.
            double da = 0.0, db = 0.0;
            if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) return std::nullopt;
            return PropertyValue(da + db);
        }
        case Op::Sub: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            const bool bVec = std::holds_alternative<glm::vec3>(*b);
            if (aVec && bVec) {
                return PropertyValue(std::get<glm::vec3>(*a) - std::get<glm::vec3>(*b));
            }
            if (aVec != bVec) return std::nullopt;   // vector - scalar is not subtraction
            double da = 0.0, db = 0.0;   // coercing, see Op::Add above
            if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) return std::nullopt;
            return PropertyValue(da - db);
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
            return PropertyValue(static_cast<double>(
                glm::dot(std::get<glm::vec3>(*a), std::get<glm::vec3>(*b))));
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
            return PropertyValue(static_cast<double>(glm::length(std::get<glm::vec3>(*v))));
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
            return PropertyValue(static_cast<double>(
                glm::distance(std::get<glm::vec3>(*a), std::get<glm::vec3>(*b))));
        }
        // --- The CSG booleans / min / max ----------------------------------
        case Op::Union:
        case Op::Intersection: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            const bool bVec = std::holds_alternative<glm::vec3>(*b);
            if (aVec && bVec) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                glm::vec3 vb = std::get<glm::vec3>(*b);
                return PropertyValue((op == Op::Union) ? glm::min(va, vb) : glm::max(va, vb));
            }
            if (aVec && !bVec) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                double db = 0.0;
                if (!propertyValueToNumber(*b, db)) return std::nullopt;
                float fb = static_cast<float>(db);
                return PropertyValue((op == Op::Union) ? glm::min(va, glm::vec3(fb)) : glm::max(va, glm::vec3(fb)));
            }
            if (!aVec && bVec) {
                double da = 0.0;
                if (!propertyValueToNumber(*a, da)) return std::nullopt;
                float fa = static_cast<float>(da);
                glm::vec3 vb = std::get<glm::vec3>(*b);
                return PropertyValue((op == Op::Union) ? glm::min(glm::vec3(fa), vb) : glm::max(glm::vec3(fa), vb));
            }
            double da = 0.0, db = 0.0;
            if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) return std::nullopt;
            return PropertyValue((op == Op::Union) ? std::min(da, db) : std::max(da, db));
        }
        case Op::Difference: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            double da = 0.0, db = 0.0;
            if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) return std::nullopt;
            return PropertyValue(std::max(da, -db));
        }
        case Op::Div: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            const bool bVec = std::holds_alternative<glm::vec3>(*b);
            if (aVec && bVec) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                glm::vec3 vb = std::get<glm::vec3>(*b);
                glm::vec3 res;
                for (int i = 0; i < 3; ++i) {
                    res[i] = (std::abs(vb[i]) < static_cast<float>(kDegenerateDivisor)) ? 0.0f : va[i] / vb[i];
                }
                return PropertyValue(res);
            }
            if (aVec && !bVec) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                double db = 0.0;
                if (!propertyValueToNumber(*b, db)) return std::nullopt;
                if (std::abs(db) < kDegenerateDivisor) return PropertyValue(glm::vec3(0.0f));
                return PropertyValue(va / static_cast<float>(db));
            }
            if (!aVec && !bVec) {
                double da = 0.0, db = 0.0;
                if (!propertyValueToNumber(*a, da) || !propertyValueToNumber(*b, db)) return std::nullopt;
                if (std::abs(db) < kDegenerateDivisor) return PropertyValue(0.0);
                return PropertyValue(da / db);
            }
            return std::nullopt;
        }
        case Op::Pow: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            auto b = children[1]->evaluate(vars, subject);
            if (!a || !b) return std::nullopt;
            const bool aVec = std::holds_alternative<glm::vec3>(*a);
            double db = 0.0;
            if (!propertyValueToNumber(*b, db)) return std::nullopt;
            if (aVec) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                float fb = static_cast<float>(db);
                return PropertyValue(glm::vec3(
                    (va.x < 0.0f && std::floor(fb) != fb) ? 0.0f : std::pow(va.x, fb),
                    (va.y < 0.0f && std::floor(fb) != fb) ? 0.0f : std::pow(va.y, fb),
                    (va.z < 0.0f && std::floor(fb) != fb) ? 0.0f : std::pow(va.z, fb)
                ));
            }
            double da = 0.0;
            if (!propertyValueToNumber(*a, da)) return std::nullopt;
            if (da < 0.0 && std::floor(db) != db) return PropertyValue(0.0);
            return PropertyValue(std::pow(da, db));
        }
        case Op::Abs: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            if (!a) return std::nullopt;
            if (std::holds_alternative<glm::vec3>(*a)) {
                return PropertyValue(glm::abs(std::get<glm::vec3>(*a)));
            }
            double da = 0.0;
            if (!propertyValueToNumber(*a, da)) return std::nullopt;
            return PropertyValue(std::abs(da));
        }
        case Op::Clamp: {
            if (children.size() != 3) return std::nullopt;
            auto val = children[0]->evaluate(vars, subject);
            auto lo = children[1]->evaluate(vars, subject);
            auto hi = children[2]->evaluate(vars, subject);
            if (!val || !lo || !hi) return std::nullopt;
            if (std::holds_alternative<glm::vec3>(*val)) {
                glm::vec3 v = std::get<glm::vec3>(*val);
                glm::vec3 vlo(0.0f), vhi(1.0f);
                if (std::holds_alternative<glm::vec3>(*lo)) vlo = std::get<glm::vec3>(*lo);
                else { double d = 0.0; propertyValueToNumber(*lo, d); vlo = glm::vec3(static_cast<float>(d)); }
                if (std::holds_alternative<glm::vec3>(*hi)) vhi = std::get<glm::vec3>(*hi);
                else { double d = 1.0; propertyValueToNumber(*hi, d); vhi = glm::vec3(static_cast<float>(d)); }
                return PropertyValue(glm::clamp(v, vlo, vhi));
            }
            double dval = 0.0, dlo = 0.0, dhi = 0.0;
            if (!propertyValueToNumber(*val, dval) || !propertyValueToNumber(*lo, dlo) || !propertyValueToNumber(*hi, dhi)) {
                return std::nullopt;
            }
            return PropertyValue(std::clamp(dval, dlo, dhi));
        }
        case Op::Sqrt: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            if (!a) return std::nullopt;
            if (std::holds_alternative<glm::vec3>(*a)) {
                glm::vec3 va = std::get<glm::vec3>(*a);
                return PropertyValue(glm::sqrt(glm::max(va, glm::vec3(0.0f))));
            }
            double da = 0.0;
            if (!propertyValueToNumber(*a, da)) return std::nullopt;
            return PropertyValue(std::sqrt(std::max(0.0, da)));
        }
        case Op::Tan: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            if (!a) return std::nullopt;
            double da = 0.0;
            if (!propertyValueToNumber(*a, da)) return std::nullopt;
            return PropertyValue(std::tan(da));
        }
        case Op::Noise: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evaluate(vars, subject);
            if (!a || !std::holds_alternative<glm::vec3>(*a)) return std::nullopt;
            glm::vec3 va = std::get<glm::vec3>(*a);
            return PropertyValue(static_cast<double>(glm::perlin(va)));
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

std::string MathNode::print() const {
    // A missing child prints as "?" rather than crashing or vanishing —
    // a malformed tree should look malformed, not silently plausible.
    auto arg = [&](std::size_t i) -> std::string {
        return (i < children.size() && children[i]) ? children[i]->print() : "?";
    };
    switch (op) {
        case Op::ScalarLeaf:      return scalarForm.print();
        case Op::ValueLeaf:       return variableName;
        case Op::VectorConstruct: return "(" + arg(0) + ", " + arg(1) + ", " + arg(2) + ")";
        case Op::Component:       return arg(0) + "." + stringArg;
        case Op::Add:             return "(" + arg(0) + " + " + arg(1) + ")";
        case Op::Sub:             return "(" + arg(0) + " - " + arg(1) + ")";
        case Op::Scale:           return "(" + arg(0) + " * " + arg(1) + ")";
        case Op::Dot:             return "dot(" + arg(0) + ", " + arg(1) + ")";
        case Op::Cross:           return "cross(" + arg(0) + ", " + arg(1) + ")";
        case Op::Hadamard:        return "(" + arg(0) + " .* " + arg(1) + ")";
        case Op::Normalize:       return "normalize(" + arg(0) + ")";
        case Op::Length:          return "length(" + arg(0) + ")";
        case Op::Map:             return stringArg + "(" + arg(0) + ")";
        case Op::Stochastic: {
            std::string out = stringArg + "(";
            for (std::size_t i = 0; i < children.size(); ++i) {
                if (i) out += ", ";
                out += arg(i);
            }
            return out + ")";
        }
        case Op::Project:      return "project(" + arg(0) + " onto " + arg(1) + ")";
        case Op::Distance:     return "distance(" + arg(0) + ", " + arg(1) + ")";
        case Op::Union:        return "union(" + arg(0) + ", " + arg(1) + ")";
        case Op::Intersection: return "intersection(" + arg(0) + ", " + arg(1) + ")";
        case Op::Difference:   return "difference(" + arg(0) + ", " + arg(1) + ")";
        case Op::Div:          return "(" + arg(0) + " / " + arg(1) + ")";
        case Op::Pow:          return "(" + arg(0) + " ^ " + arg(1) + ")";
        case Op::Abs:          return "abs(" + arg(0) + ")";
        case Op::Clamp:        return "clamp(" + arg(0) + ", " + arg(1) + ", " + arg(2) + ")";
        case Op::Sqrt:         return "sqrt(" + arg(0) + ")";
        case Op::Tan:          return "tan(" + arg(0) + ")";
        case Op::Noise:        return "noise(" + arg(0) + ")";
        case Op::SDF:          return "SDF(" + arg(0) + " @ " + arg(1) + ")";
        case Op::Gradient:     return "gradient(" + arg(0) + " @ " + arg(1) + ")";
        // Declared but not implemented anywhere (see evaluate() above) —
        // say so plainly rather than printing a formula that never runs.
        case Op::Raycast:      return "raycast(" + arg(0) + ", " + arg(1) + ") [not implemented]";
        case Op::LineIntegral: return "lineIntegral(" + arg(0) + ", " + arg(1) + ") [not implemented]";
        // An op this build does not know: the original JSON rides along in
        // `unsupported` (see fromJson) — show that verbatim, never a stand-in.
        case Op::Unsupported:
            return unsupported ? ("[unsupported op: " + unsupported->dump() + "]")
                                : "[unsupported op]";
    }
    return "[unknown op]";
}

std::optional<MathNode::RangeValue> MathNode::evalRange(const std::map<std::string, RangeValue>& vars) const {
    auto retInf = []() { return RangeValue::makeScalar(Interval::infinite()); };
    auto retVecInf = []() { return RangeValue::makeVector(Interval::infinite(), Interval::infinite(), Interval::infinite()); };
    
    switch (op) {
        case Op::ScalarLeaf: {
            // A non-constant form used to answer [-inf, +inf] outright, which
            // made every bound downstream of an authored formula useless. The
            // signomial algebra can bound itself: hand ScalarForm::evalRange
            // the SCALAR variables in scope (a vector variable is never a
            // ScalarForm symbol) and let it do the interval arithmetic.
            std::map<std::string, Interval> scalars;
            for (const auto& [name, rv] : vars) {
                if (rv.kind == ValueKind::Scalar) scalars.emplace(name, rv.scalar);
            }
            return RangeValue::makeScalar(scalarForm.evalRange(scalars));
        }
        case Op::ValueLeaf: {
            auto it = vars.find(variableName);
            if (it != vars.end()) return it->second;
            return retInf();
        }
        case Op::VectorConstruct: {
            if (children.size() != 3) return std::nullopt;
            auto x = children[0]->evalRange(vars);
            auto y = children[1]->evalRange(vars);
            auto z = children[2]->evalRange(vars);
            if (!x || !y || !z) return std::nullopt;
            return RangeValue::makeVector(x->scalar, y->scalar, z->scalar);
        }
        case Op::Component: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            if (!a || a->kind != ValueKind::Vector) return std::nullopt;
            if (stringArg == "x") return RangeValue::makeScalar(a->vec[0]);
            if (stringArg == "y") return RangeValue::makeScalar(a->vec[1]);
            if (stringArg == "z") return RangeValue::makeScalar(a->vec[2]);
            return std::nullopt;
        }
        case Op::Add: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) return RangeValue::makeScalar(a->scalar + b->scalar);
            if (a->kind == ValueKind::Vector && b->kind == ValueKind::Vector) return RangeValue::makeVector(a->vec[0] + b->vec[0], a->vec[1] + b->vec[1], a->vec[2] + b->vec[2]);
            return std::nullopt;
        }
        case Op::Sub: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) return RangeValue::makeScalar(a->scalar - b->scalar);
            if (a->kind == ValueKind::Vector && b->kind == ValueKind::Vector) return RangeValue::makeVector(a->vec[0] - b->vec[0], a->vec[1] - b->vec[1], a->vec[2] - b->vec[2]);
            return std::nullopt;
        }
        case Op::Scale: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) return RangeValue::makeScalar(a->scalar * b->scalar);
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Vector) return RangeValue::makeVector(a->scalar * b->vec[0], a->scalar * b->vec[1], a->scalar * b->vec[2]);
            if (a->kind == ValueKind::Vector && b->kind == ValueKind::Scalar) return RangeValue::makeVector(a->vec[0] * b->scalar, a->vec[1] * b->scalar, a->vec[2] * b->scalar);
            return std::nullopt;
        }
        case Op::Div: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) return RangeValue::makeScalar(a->scalar / b->scalar);
            if (a->kind == ValueKind::Vector && b->kind == ValueKind::Scalar) return RangeValue::makeVector(a->vec[0] / b->scalar, a->vec[1] / b->scalar, a->vec[2] / b->scalar);
            return std::nullopt;
        }
        case Op::Abs: {
            if (children.size() != 1) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            if (!a) return std::nullopt;
            auto absInt = [](Interval i) {
                float minAbs = (i.lo <= 0.0f && i.hi >= 0.0f) ? 0.0f : std::min(std::abs(i.lo), std::abs(i.hi));
                float maxAbs = std::max(std::abs(i.lo), std::abs(i.hi));
                return Interval(minAbs, maxAbs);
            };
            if (a->kind == ValueKind::Scalar) return RangeValue::makeScalar(absInt(a->scalar));
            if (a->kind == ValueKind::Vector) return RangeValue::makeVector(absInt(a->vec[0]), absInt(a->vec[1]), absInt(a->vec[2]));
            return std::nullopt;
        }
        case Op::Union: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) {
                return RangeValue::makeScalar(Interval(std::min(a->scalar.lo, b->scalar.lo), std::min(a->scalar.hi, b->scalar.hi)));
            }
            return std::nullopt;
        }
        case Op::Intersection: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) {
                return RangeValue::makeScalar(Interval(std::max(a->scalar.lo, b->scalar.lo), std::max(a->scalar.hi, b->scalar.hi)));
            }
            return std::nullopt;
        }
        case Op::Difference: {
            if (children.size() != 2) return std::nullopt;
            auto a = children[0]->evalRange(vars);
            auto b = children[1]->evalRange(vars);
            if (!a || !b) return std::nullopt;
            if (a->kind == ValueKind::Scalar && b->kind == ValueKind::Scalar) {
                return RangeValue::makeScalar(Interval(std::max(a->scalar.lo, -b->scalar.hi), std::max(a->scalar.hi, -b->scalar.lo)));
            }
            return std::nullopt;
        }
        case Op::Noise: {
            // This bound is LOAD-BEARING, not decorative: geom::evalRange feeds it to
            // tessellateSdf's subdivision, which DISCARDS any cell whose interval does
            // not straddle zero. Claim a range the noise can leave and the marcher
            // deletes cells that really do contain surface -- holes in the mesh a
            // Person falls through, with nothing logged.
            //
            // Op::Noise evaluates to glm::perlin, whose 3D form returns 2.2 * n, with
            // n a fade-weighted convex blend of unit-gradient dot products. The
            // classical supremum for N-dimensional classic Perlin is sqrt(N)/2, so
            // |glm::perlin| <= 2.2 * sqrt(3)/2 = 1.905. It is NOT 1.0: sampling
            // 8e6 random points measured [-1.127, +1.123], so the [-1, 1] this read
            // for one campaign was already unsound at the values the noise floor
            // actually reaches.
            constexpr float kPerlinBound = 1.905255f;   // 2.2 * sqrt(3)/2
            return RangeValue::makeScalar(Interval(-kPerlinBound, kPerlinBound));
        }
        // Fallback for everything else
        default:
            return retInf();
    }
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

namespace {
// The actual authored formula for one piece — never a stand-in like
// "[MathNode]". Every way a piece can carry its value (a call, a fold, or
// a math tree) and every way it can be gated (a guard, a pure inequality,
// an interval) is rendered honestly, so the Map/Flow editor never shows
// less than what will actually run.
std::string printPiece(const Piecewise::Piece& piece, const std::string& inputVariable) {
    std::string value;
    if (piece.call) {
        value = piece.call->function + "(";
        for (std::size_t i = 0; i < piece.call->args.size(); ++i) {
            if (i) value += ", ";
            value += piece.call->args[i].print();
        }
        value += ")";
    } else if (piece.fold) {
        static const char* foldNames[] = {"sum", "mean", "min", "max", "count"};
        const int idx = static_cast<int>(piece.fold->op);
        const char* name = (idx >= 0 && idx < 5) ? foldNames[idx] : "fold";
        value = std::string(name) + "(" + piece.fold->path + ")";
    } else if (piece.mathNode) {
        value = piece.mathNode->print();
    } else {
        value = "[empty]";
    }

    std::vector<std::string> conditions;
    if (piece.guard) conditions.push_back(piece.guard->describe());
    if (piece.whereLEZero) conditions.push_back(piece.whereLEZero->print() + " <= 0");
    if (piece.hasLo || piece.hasHi) {
        const std::string lo = piece.hasLo ? formatNumber(piece.lo) : "-inf";
        const std::string hi = piece.hasHi ? formatNumber(piece.hi) : "+inf";
        conditions.push_back(inputVariable + " in " + (piece.includeLo ? "[" : "(") + lo +
                              ", " + hi + (piece.includeHi ? "]" : ")"));
    }
    if (conditions.empty()) return value;
    std::string out = value + "  [if ";
    for (std::size_t i = 0; i < conditions.size(); ++i) {
        if (i) out += " and ";
        out += conditions[i];
    }
    return out + "]";
}
} // namespace

std::string Piecewise::print() const {
    if (pieces.empty()) return "[undefined]";
    std::string out;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i) out += ";  ";
        out += printPiece(pieces[i], inputVariable);
    }
    return out;
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

// ===========================================================================
// Exact integration over the AST and the piecewise model.
// ===========================================================================
namespace {

void say(std::string* why, const std::string& text) {
    if (why) *why = text;
}

// Every breakpoint a piecewise model has inside (a, b), so the interval can
// be cut into stretches over which ONE piece governs.
void collectBreakpoints(const Piecewise& model, double a, double b,
                        std::vector<double>& out) {
    for (const auto& piece : model.pieces) {
        if (piece.hasLo && piece.lo > a && piece.lo < b) out.push_back(piece.lo);
        if (piece.hasHi && piece.hi > a && piece.hi < b) out.push_back(piece.hi);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

}   // namespace

std::optional<ScalarForm> toScalarForm(const MathNode& node, std::string* why) {
    switch (node.op) {
        case MathNode::Op::ScalarLeaf:
            return node.scalarForm;

        // A named value is a scalar symbol here. If it is bound to a vector at
        // evaluation time it simply will not appear in the double-valued
        // variable map, and evaluation fails honestly rather than guessing.
        case MathNode::Op::ValueLeaf:
            if (node.variableName.empty()) {
                say(why, "ValueLeaf with no variable name");
                return std::nullopt;
            }
            return ScalarForm::variable(node.variableName);

        case MathNode::Op::Add: {
            if (node.children.empty()) {
                say(why, "Add with no children");
                return std::nullopt;
            }
            ScalarForm sum;
            for (const auto& child : node.children) {
                if (!child) { say(why, "Add with a null child"); return std::nullopt; }
                const auto term = toScalarForm(*child, why);
                if (!term) return std::nullopt;
                sum = sum.plus(*term);
            }
            return sum.normalized();
        }

        case MathNode::Op::Sub: {
            if (node.children.size() < 2) {
                say(why, "Sub needs two children");
                return std::nullopt;
            }
            const auto head = toScalarForm(*node.children[0], why);
            if (!head) return std::nullopt;
            ScalarForm diff = *head;
            for (std::size_t i = 1; i < node.children.size(); ++i) {
                if (!node.children[i]) { say(why, "Sub with a null child"); return std::nullopt; }
                const auto term = toScalarForm(*node.children[i], why);
                if (!term) return std::nullopt;
                diff = diff.plus(term->scaled(-1.0));
            }
            return diff.normalized();
        }

        // Scalar·Scalar is ordinary multiplication (see evaluate()); the
        // vector cases have no scalar form and fall out through the algebra
        // when their operands do.
        case MathNode::Op::Scale: {
            if (node.children.size() != 2) {
                say(why, "Scale needs two children");
                return std::nullopt;
            }
            const auto lhs = toScalarForm(*node.children[0], why);
            if (!lhs) return std::nullopt;
            const auto rhs = toScalarForm(*node.children[1], why);
            if (!rhs) return std::nullopt;
            return lhs->times(*rhs).normalized();
        }

        default:
            say(why, std::string("no scalar closed form for ") + mathOpName(node.op));
            return std::nullopt;
    }
}

std::optional<ScalarForm> antiderivative(const MathNode& node, const std::string& var,
                                         std::string* why) {
    const auto form = toScalarForm(node, why);
    if (!form) return std::nullopt;
    auto integrated = form->antiderivative(var);
    if (!integrated) {
        say(why, "∫" + form->print() + " d" + var +
                     " needs integration by parts: not held in this algebra");
        return std::nullopt;
    }
    return integrated;
}

bool integrable(const Piecewise& model, const std::string& var, std::string* why) {
    if (model.pieces.empty()) {
        say(why, "no pieces: nothing to integrate");
        return false;
    }
    // Interval bounds cut ONE designated variable. If that is not the variable
    // being integrated, the bounds decide applicability by something held
    // constant here — answerable, but not by this quadrature, and guessing
    // which piece governs is exactly the kind of guess this algebra refuses.
    bool anyBounded = false;
    for (const auto& piece : model.pieces) {
        if (piece.hasLo || piece.hasHi) { anyBounded = true; break; }
    }
    if (anyBounded && model.inputVariable != var) {
        say(why, "the bounds cut '" + model.inputVariable + "' but the integration is in '" +
                     var + "'");
        return false;
    }
    for (const auto& piece : model.pieces) {
        // A guarded piece asks the world whether it applies. Integrating over
        // a past interval would need that world's past to answer it, which is
        // the very thing being computed.
        if (piece.guard) {
            say(why, "a piece is gated on a world guard: its past applicability is unknown");
            return false;
        }
        if (piece.whereLEZero) {
            say(why, "a piece is gated on a pure guard: its past applicability is unknown");
            return false;
        }
        if (piece.call) {
            say(why, "a piece's value is a function call: not integrated symbolically");
            return false;
        }
        if (piece.fold) {
            say(why, "a piece's value is a fold over the world: its past is not in the law text");
            return false;
        }
        if (!piece.mathNode) {
            say(why, "a piece has no expression");
            return false;
        }
        if (!antiderivative(*piece.mathNode, var, why)) return false;
    }
    return true;
}

std::optional<double> definiteIntegral(const Piecewise& model, const std::string& var,
                                       double a, double b,
                                       const std::map<std::string, double>& vars,
                                       std::string* why) {
    if (!integrable(model, var, why)) return std::nullopt;
    if (a == b) return 0.0;
    if (b < a) {
        const auto forward = definiteIntegral(model, var, b, a, vars, why);
        if (!forward) return std::nullopt;
        return -*forward;
    }

    std::vector<double> cuts;
    collectBreakpoints(model, a, b, cuts);
    cuts.insert(cuts.begin(), a);
    cuts.push_back(b);

    double total = 0.0;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
        const double lo = cuts[i], hi = cuts[i + 1];
        if (hi <= lo) continue;
        const double mid = lo + (hi - lo) * 0.5;

        // First applicable piece wins — the same rule evaluate() follows, so
        // the integral is of the function the world actually ran.
        const Piecewise::Piece* governing = nullptr;
        for (const auto& piece : model.pieces) {
            const bool bounded = piece.hasLo || piece.hasHi;
            if (!bounded || piece.contains(mid)) { governing = &piece; break; }
        }
        if (!governing) {
            say(why, "the rate is undefined on [" + std::to_string(lo) + ", " +
                         std::to_string(hi) + "]: undefined is not zero");
            return std::nullopt;
        }

        const auto F = antiderivative(*governing->mathNode, var, why);
        if (!F) return std::nullopt;

        std::map<std::string, double> at = vars;
        at[var] = hi;
        const auto upper = F->evaluate(at);
        at[var] = lo;
        const auto lower = F->evaluate(at);
        if (!upper || !lower) {
            say(why, "the antiderivative does not evaluate at the interval's ends "
                     "(an unbound variable, or outside its domain)");
            return std::nullopt;
        }
        total += *upper - *lower;
    }
    return total;
}

} // namespace OntoMath
