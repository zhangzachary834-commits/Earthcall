#include "Singularity/OntoMath/Expression.hpp"

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
    return sig;
}

} // namespace

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
    return value;
}

Term Term::times(const Term& other) const {
    Term product(coefficient * other.coefficient, factors);
    for (const auto& [name, exp] : other.factors) {
        product.factors[name] += exp;
        if (product.factors[name] == 0.0) product.factors.erase(name);
    }
    return product;
}

std::string Term::print() const {
    if (factors.empty()) return formatNumber(coefficient);
    std::string out;
    if (coefficient == -1.0) out = "-";
    else if (coefficient != 1.0) out = formatNumber(coefficient);
    for (const auto& [name, exp] : factors) {
        out += name;
        if (exp != 1.0) out += "^" + formatNumber(exp);
    }
    return out;
}

nlohmann::json Term::toJson() const {
    nlohmann::json f = nlohmann::json::object();
    for (const auto& [name, exp] : factors) f[name] = exp;
    return nlohmann::json{{"c", coefficient}, {"factors", f}};
}

Term Term::fromJson(const nlohmann::json& j) {
    Term t;
    t.coefficient = j.value("c", 0.0);
    if (j.contains("factors")) {
        for (auto it = j["factors"].begin(); it != j["factors"].end(); ++it) {
            t.factors[it.key()] = it.value().get<double>();
        }
    }
    return t;
}

// ---------------------------------------------------------------------------
// Expression
// ---------------------------------------------------------------------------

Expression Expression::constant(double c) {
    Expression e;
    e.terms.emplace_back(c);
    return e;
}

Expression Expression::variable(const std::string& name, double exponent, double coefficient) {
    Expression e;
    Term t(coefficient);
    if (exponent != 0.0) t.factors[name] = exponent;
    e.terms.push_back(std::move(t));
    return e;
}

std::optional<double> Expression::evaluate(const std::map<std::string, double>& vars) const {
    double sum = 0.0;
    for (const auto& term : terms) {
        auto v = term.evaluate(vars);
        if (!v) return std::nullopt;
        sum += *v;
    }
    return sum;
}

Expression Expression::plus(const Expression& other) const {
    Expression out = *this;
    out.terms.insert(out.terms.end(), other.terms.begin(), other.terms.end());
    return out.normalized();
}

Expression Expression::times(const Expression& other) const {
    Expression out;
    for (const auto& a : terms) {
        for (const auto& b : other.terms) {
            out.terms.push_back(a.times(b));
        }
    }
    return out.normalized();
}

Expression Expression::scaled(double k) const {
    Expression out = *this;
    for (auto& term : out.terms) term.coefficient *= k;
    return out.normalized();
}

Expression Expression::normalized() const {
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
    Expression out;
    out.terms = std::move(combined);
    return out;
}

Expression Expression::derivative(const std::string& var) const {
    // Power rule per term: d/dx c·x^n·R = c·n·x^(n-1)·R  (R free of x).
    Expression out;
    for (const auto& term : terms) {
        auto it = term.factors.find(var);
        if (it == term.factors.end()) continue;        // constant in var: drops
        Term d(term.coefficient * it->second, term.factors);
        const double newExp = it->second - 1.0;
        if (newExp == 0.0) d.factors.erase(var);
        else d.factors[var] = newExp;
        out.terms.push_back(std::move(d));
    }
    return out.normalized();
}

std::optional<Expression> Expression::antiderivative(const std::string& var) const {
    // Power rule per term: ∫ c·x^n dx = c/(n+1)·x^(n+1), n ≠ -1.
    // The n = -1 term integrates to ln|x|, which this algebra cannot yet
    // hold — report honestly. (+C is the author's to add.)
    Expression out;
    for (const auto& term : terms) {
        auto it = term.factors.find(var);
        const double n = it == term.factors.end() ? 0.0 : it->second;
        if (n == -1.0) return std::nullopt;
        Term integrated(term.coefficient / (n + 1.0), term.factors);
        integrated.factors[var] = n + 1.0;
        out.terms.push_back(std::move(integrated));
    }
    return out.normalized();
}

std::string Expression::print() const {
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

nlohmann::json Expression::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& term : terms) arr.push_back(term.toJson());
    return nlohmann::json{{"terms", arr}};
}

Expression Expression::fromJson(const nlohmann::json& j) {
    Expression e;
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

nlohmann::json Piecewise::Piece::toJson() const {
    nlohmann::json j{{"expr", expression.toJson()}};
    if (hasLo) {
        j["lo"] = lo;
        j["includeLo"] = includeLo;
    }
    if (hasHi) {
        j["hi"] = hi;
        j["includeHi"] = includeHi;
    }
    return j;
}

Piecewise::Piece Piecewise::Piece::fromJson(const nlohmann::json& j) {
    Piece p;
    if (j.contains("expr")) p.expression = Expression::fromJson(j["expr"]);
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
    return p;
}

Piecewise Piecewise::continuous(Expression e) {
    Piecewise f;
    Piece piece;
    piece.expression = std::move(e);
    f.pieces.push_back(std::move(piece));
    return f;
}

std::optional<double> Piecewise::evaluate(const std::map<std::string, double>& vars) const {
    // Find the piece whose bounds contain the designated input; a piece with
    // no bounds is everywhere-defined. First match wins (author's order).
    for (const auto& piece : pieces) {
        if (piece.hasLo || piece.hasHi) {
            auto it = vars.find(inputVariable);
            if (it == vars.end()) return std::nullopt;
            if (!piece.contains(it->second)) continue;
        }
        return piece.expression.evaluate(vars);
    }
    return std::nullopt;   // outside every piece: undefined, not zero
}

std::string Piecewise::print() const {
    if (pieces.size() == 1 && !pieces[0].hasLo && !pieces[0].hasHi) {
        return pieces[0].expression.print();
    }
    return "piecewise(" + std::to_string(pieces.size()) + " over " + inputVariable + ")";
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
