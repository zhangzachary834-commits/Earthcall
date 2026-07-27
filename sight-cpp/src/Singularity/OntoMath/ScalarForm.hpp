#pragma once

#include "json.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <set>

#include "Form/Singular/Property/PropertyValue.hpp"

class Singular;
struct ConditionNode;   // guards: the condition calculus gates pieces


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
// Representation: an ScalarForm is a sum of Terms; a Term is
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

// A transcendental factor: kind(scale·var + shift). This is what carries the
// algebra past signomials — periodic (sin/cos), exponential (exp), and
// logarithmic (ln) change become EXACT law-text, not curve approximations.
// Closure under the calculus is honest: sin/cos/exp differentiate into each
// other; Ln is restricted to ln(scale·var) — shift forced to 0 — so its
// derivative (1/x) stays inside the algebra. ln of a non-positive argument
// is undefined (nullopt), never a guess.
struct TransFactor {
    // Serialized as ints — APPEND-ONLY.
    enum class Kind { Sin = 0, Cos = 1, Exp = 2, Ln = 3 };

    Kind kind = Kind::Sin;
    std::string variable;
    double scale = 1.0;
    double shift = 0.0;   // forced to 0 for Ln

    TransFactor() = default;
    TransFactor(Kind k, std::string var, double sc = 1.0, double sh = 0.0)
        : kind(k), variable(std::move(var)), scale(sc),
          shift(k == Kind::Ln ? 0.0 : sh) {}

    std::optional<double> evaluate(double x) const;
    std::string print() const;
    nlohmann::json toJson() const;
    static TransFactor fromJson(const nlohmann::json& j);

    bool operator==(const TransFactor& o) const {
        return kind == o.kind && variable == o.variable && scale == o.scale &&
               shift == o.shift;
    }
    bool operator<(const TransFactor& o) const;   // canonical ordering
};

// One product term: coefficient * Π var^exp * Π trans(scale·var + shift).
// Power factors are kept in a sorted map and transcendental factors in a
// sorted vector so like-term comparison and printing are canonical.
struct Term {
    double coefficient = 0.0;
    std::map<std::string, double> factors;   // variable name -> real exponent
    std::vector<TransFactor> trans;          // kept sorted (canonical form)

    Term() = default;
    Term(double c, std::map<std::string, double> f = {})
        : coefficient(c), factors(std::move(f)) {}

    // Strict evaluation: every variable in the term must be bound — a law
    // must never fire on an unvalued symbol.
    std::optional<double> evaluate(const std::map<std::string, double>& vars) const;

    Term times(const Term& other) const;     // coefficients multiply, exponents add
    bool sameShape(const Term& other) const {
        return factors == other.factors && trans == other.trans;
    }
    void addTrans(TransFactor factor);       // insert + keep canonical order
    bool mentions(const std::string& var) const;

    std::string print() const;
    nlohmann::json toJson() const;
    static Term fromJson(const nlohmann::json& j);
};

// A sum of terms — the ContinuousExpression. Multivariate; polynomials are
// the natural-exponent special case.
struct ScalarForm {
    std::vector<Term> terms;

    static ScalarForm constant(double c);
    static ScalarForm variable(const std::string& name, double exponent = 1.0,
                               double coefficient = 1.0);
    // coefficient · kind(scale·var + shift) as a one-term expression.
    static ScalarForm transcendental(TransFactor::Kind kind, const std::string& var,
                                     double scale = 1.0, double shift = 0.0,
                                     double coefficient = 1.0);
    // bias + amplitude·sin(2π(frequency·var + phase)) — the exact form of
    // Automation::evalTrack / CurveModel's sinusoid (phase in turns), so
    // recorded periodic change can retire into the exact core.
    static ScalarForm sinusoid(double amplitude, double frequency, double phase = 0.0,
                               double bias = 0.0, const std::string& var = "x");

    std::optional<double> evaluate(const std::map<std::string, double>& vars) const;

    // Exact algebra.
    ScalarForm plus(const ScalarForm& other) const;
    ScalarForm times(const ScalarForm& other) const;   // distributes term products
    ScalarForm scaled(double k) const;
    // Combine like terms, drop zeros, order canonically (descending total
    // degree, then factor signature).
    ScalarForm normalized() const;

    // Exact calculus — power rule plus the full product/chain rule over
    // transcendental factors; every term in this representation is
    // differentiable. Integration is exact where the algebra can hold the
    // answer: powers (∫x⁻¹ = ln x now included, defined on x > 0), a single
    // sin/cos/exp/ln factor per term (∫ln(ax) = x·ln(ax) − x). Products
    // needing integration by parts (x·sin x, sin·cos, ...) are reported
    // honestly as nullopt rather than approximated.
    ScalarForm derivative(const std::string& var) const;
    std::optional<ScalarForm> antiderivative(const std::string& var) const;

    bool empty() const { return terms.empty(); }
    std::string print() const;
    nlohmann::json toJson() const;
    static ScalarForm fromJson(const nlohmann::json& j);
};

// A CALL to a named, authored function (see FunctionRegistry below): the
// arguments are full Expressions of the CALLER's variables, evaluated first
// and bound to the definition's parameters — pure functions, composable and
// recursive. Iteration is carried through the arguments ("f(x², n−1)"), so
// primitive recursion — escape-time fractals included — is expressible,
// bounded by an anti-Babel call-depth ceiling (divergence is honest nullopt).
struct FunctionCall {
    std::string function;              // name in the FunctionRegistry
    std::vector<ScalarForm> args;      // evaluated in the caller's variables

    nlohmann::json toJson() const;
    static FunctionCall fromJson(const nlohmann::json& j);
};

// A FOLD over the world: aggregate one property across every being of a
// kind — the discrete-math Σ/mean/min/max/count, as a piece's value.
// "y := the mean height of all Objects" is one fold. Empty sums are 0 and
// empty counts are 0 (honest identities); empty mean/min/max are undefined
// (nullopt). This is the aggregation half of pair quantification.
struct Fold {
    // Serialized as ints — APPEND-ONLY.
    enum class Op { Sum = 0, Mean = 1, Min = 2, Max = 3, Count = 4 };

    Op op = Op::Sum;
    int beingKind = 1;                  // ConditionNode::BeingKind (1 = Object)
    std::string path;                   // property read on each being
    std::vector<std::string> exceptIds; // "...with possible exceptions"

    nlohmann::json toJson() const;
    static Fold fromJson(const nlohmann::json& j);
};

// ============================================================================
// MathNode — Typed AST over ScalarForm
// ============================================================================

enum class ValueKind { Scalar, Vector };

struct TypeDiagnostic {
    std::string nodePath;
    std::string message;
};

struct TypeResult {
    bool success;
    ValueKind kind;
    TypeDiagnostic diagnostic;

    static TypeResult ok(ValueKind k) { return {true, k, {}}; }
    static TypeResult error(TypeDiagnostic d) { return {false, ValueKind::Scalar, std::move(d)}; }
    
    explicit operator bool() const { return success; }
    ValueKind operator*() const { return kind; }
};

using TypeEnv = std::map<std::string, ValueKind>;

struct MathNode {
    // Serialized as ints — APPEND-ONLY
    enum class Op {
        ScalarLeaf = 0,
        ValueLeaf = 1,
        VectorConstruct = 2,
        Component = 3,
        Add = 4,
        Sub = 5,
        Scale = 6,
        Dot = 7,
        Cross = 8,
        Hadamard = 9,
        Normalize = 10,
        Length = 11,
        Map = 12 // Unary componentwise function (e.g. Round, Floor)
    };

    Op op;
    ScalarForm scalarForm;
    std::string variableName;
    std::string stringArg; // For Component index or Map func name
    std::vector<std::unique_ptr<MathNode>> children;
    
    TypeResult typeOf(const TypeEnv& env, const std::string& path = "root") const;
    void collectDependencies(std::set<std::string>& outDeps) const;

    std::optional<PropertyValue> evaluate(const std::map<std::string, PropertyValue>& vars, const Singular* subject = nullptr) const;

    nlohmann::json toJson() const;
    static std::unique_ptr<MathNode> fromJson(const nlohmann::json& j);
    static std::shared_ptr<MathNode> fromLegacyExpression(const ScalarForm& expr);
};

// The manifesto's DiscreteFunctions: discrete bounds within which a
// continuous expression governs. Bounds are on one designated input variable
// and may be open or closed on each side — mathematically precise piecewise
// definition. Outside every piece the function is UNDEFINED (nullopt), never
// silently zero: discontinuity is honored, not papered over.
//
// EXPRESSION-GUARDED pieces (the discrete-math fusion): a piece may carry a
// full ConditionNode GUARD instead of interval bounds — "use this formula
// wherever f - g <= 0" (Zone guards), or wherever any condition of the law
// calculus holds (IsKind, Related, Overlaps...). min/max/abs and the SDF
// boolean algebra become DEFINABLE, and mathematics can branch on ontology.
// Guarded pieces need a SUBJECT to testify about the world; evaluated
// without one they are unproven and skipped — never guessed.
struct Piecewise {
    struct Piece {
        bool hasLo = false, hasHi = false;
        double lo = 0.0, hi = 0.0;
        bool includeLo = true, includeHi = true;
        std::shared_ptr<MathNode> mathNode;

        // When set, the guard DECIDES applicability (interval bounds are
        // ignored). Shared across copies of the model — edits go through
        // the usual copy-edit-commit flow.
        std::shared_ptr<ConditionNode> guard;
        // Compiled lazily on first evaluation (tree -> closure, once).
        mutable std::function<bool(const Singular&)> guardCompiled;

        // The PURE guard: applies where whereLEZero(variables) <= 0 —
        // evaluated against the LOCAL variables alone, no subject needed.
        // This is what recursion base cases over parameters use ("where
        // x - 2 > 0, escape"), and it composes with the world guard above
        // (when both are set, both must hold). Undefined g = unproven.
        std::shared_ptr<MathNode> whereLEZero;

        // When set, the piece's VALUE is a call to a named function
        // (the expression is ignored) — composition and recursion.
        std::shared_ptr<FunctionCall> call;

        // When set (and no call), the piece's VALUE is a fold over the
        // world's beings — Σ/mean/min/max/count of a property by kind.
        std::shared_ptr<Fold> fold;

        bool contains(double x) const;
        bool applies(const std::map<std::string, PropertyValue>& vars,
                     const std::string& inputVariable, const Singular* subject) const;
        nlohmann::json toJson() const;
        static Piece fromJson(const nlohmann::json& j);
    };

    std::string inputVariable = "x";   // the variable interval bounds cut
    std::vector<Piece> pieces;

    // A single everywhere-defined piece — the purely continuous case.
    static Piecewise continuous(std::shared_ptr<MathNode> node);

    // Without a subject, guarded pieces are unproven (skipped); interval
    // pieces behave as always. Pass the subject wherever one exists.
    // `depth` is the recursion budget consumed by function calls.
    std::optional<PropertyValue> evaluate(const std::map<std::string, PropertyValue>& vars) const;
    std::optional<PropertyValue> evaluate(const std::map<std::string, PropertyValue>& vars,
                                   const Singular* subject, int depth = 0) const;

    std::string print() const;
    nlohmann::json toJson() const;
    static Piecewise fromJson(const nlohmann::json& j);
};

// ============================================================================
// FunctionRegistry — the words of mathematics. A Person names a function
// once (parameters + a piecewise body, guards and calls included) and every
// expression may call it: createTerm's recursion made durable. Bodies are
// PURE — they see only their parameters (plus the subject, for guards) —
// so definitions compose and recurse safely. Self-reference is legal;
// divergence is answered by the call-depth ceiling with an honest nullopt.
// ============================================================================
struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    Piecewise body;

    nlohmann::json toJson() const;
    static FunctionDef fromJson(const nlohmann::json& j);
};

class FunctionRegistry {
public:
    static FunctionRegistry& instance();

    // The anti-Babel ceiling on recursive calls.
    static constexpr int kMaxCallDepth = 32;

    void define(FunctionDef def);              // replaces an existing name
    bool remove(const std::string& name);
    const FunctionDef* find(const std::string& name) const;
    const std::vector<FunctionDef>& getAll() const { return _functions; }

    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j);   // replace-all

private:
    FunctionRegistry() = default;
    std::vector<FunctionDef> _functions;
};

} // namespace OntoMath
