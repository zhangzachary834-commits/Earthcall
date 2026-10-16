#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

// ============================================================================
// Line restriction — exact substitution of a parametrized line into authored
// OntoMath, and collection of the result by powers of the line parameter.
//
//   f(p)  with  p = o + s*d     ==>     f(o + s*d) = Σ_k  c_k(o, d, …) · s^k
//
// This is mathematics, not rendering: OntoMath owns the meaning of every
// coefficient c_k, which is itself an ordinary ScalarForm over the line's
// origin/direction symbols plus every variable the line does NOT bind (time,
// omega, wi/wo, authored parameters). A renderer consumes the result to
// execute a camera ray's segment; it never re-derives this algebra privately.
//
// WHAT IS ADMITTED (the exact polynomial subset, mirroring MathNode::evaluate
// operation-by-operation):
//   ScalarLeaf (power factors of a bound coordinate need non-negative integer
//     exponents; transcendental factors may read only UNBOUND variables)
//   ValueLeaf  (x/y/z and the vector point p are bound to the line; anything
//     else stays a symbol the coefficients read)
//   VectorConstruct, Component, Add, Sub (exactly two operands, same kind),
//   Scale (scalar·scalar, scalar·vector, vector·scalar), Hadamard, Dot, Cross,
//   Div by an operand that flattens to a pure constant |c| >= kDegenerateDivisor,
//   Pow with a pure-constant non-negative integer exponent.
// Everything else REFUSES with the op's name: Abs, Clamp, Map, Sqrt, Length,
// Normalize, Noise, SDF, Gradient, Tan, Union/Intersection/Difference, matrix
// ops, Stochastic, Raycast, LineIntegral, Unsupported. A Piecewise is admitted
// only when its first piece answers unconditionally with a plain MathNode (no
// interval bounds, guard, pure guard, call or fold): Piecewise ray segmentation
// is a later rung, and guards/calls are not emitted identically by every
// execution channel, so lowering them would silently choose one meaning.
//
// A refusal is never a failure of the world. It means only that THIS exact
// symbolic route does not hold the mathematics; the caller keeps the existing
// exact evaluator (CPU MathNode::evaluate / the production WGSL emitter).
//
// FLOATING POINT. The restricted form is an identity over the REAL numbers.
// Evaluating it in double reorders arithmetic relative to the authored tree
// (expansion, like-term collection), so equality with MathNode::evaluate is a
// semantic claim with a rounding bound, never a byte-identity claim. Two
// further facts are not hidden: MathNode::evaluate carries vector values as
// glm::vec3 (f32), so a tree that reads `p` or builds vectors is already
// f32-rounded on the CPU; and the GPU evaluates everything in f32. Callers
// that need a bound use magnitudeBound() below. See
// docs/plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md §3.
//
// Bounds are doctrine: kMaxLineDegree and kMaxLineTerms are Kernel compile
// budgets, not authored policy. Exceeding either refuses (exact fallback);
// they are never silently enlarged to admit a form.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
// Zach's Ray Transport Canon (revived Prism, 2026-10-16): "compile world math
// into ray-domain math". Extends toScalarForm (ScalarForm.hpp), which already
// flattened Add/Sub/Scale into the exact signomial algebra.
// ============================================================================
namespace OntoMath {

// Reserved symbol names for the line. An authored form that already reads one
// of these names is REFUSED rather than captured: two meanings for one symbol
// is exactly the binding collapse the Prism separation forbids.
inline constexpr const char* kLineParameterVar = "line.s";
inline constexpr const char* kLineOriginVars[3] = {"line.o.x", "line.o.y", "line.o.z"};
inline constexpr const char* kLineDirectionVars[3] = {"line.d.x", "line.d.y", "line.d.z"};

inline constexpr int kMaxLineDegree = 16;
inline constexpr std::size_t kMaxLineTerms = 4096;

// Substitution environment for exact flattening: each named scalar variable is
// replaced by a ScalarForm; each named vector variable by three component forms.
// Variables named in neither map remain symbols, exactly as toScalarForm leaves
// them.
struct ScalarSubstitution {
    std::map<std::string, ScalarForm> scalars;
    std::map<std::string, std::array<ScalarForm, 3>> vectors;
};

// Exact substitution into a ScalarForm. Refuses (nullopt + why) when a bound
// variable carries a non-integer or negative exponent, appears inside a
// transcendental factor, or the expansion exceeds the compile budgets.
std::optional<ScalarForm> substitute(const ScalarForm& form, const ScalarSubstitution& subst,
                                     std::string* why = nullptr);

// Exact flattening of a MathNode under a substitution. The scalar entry point
// requires a Scalar result; the vector entry point a vec3 result.
std::optional<ScalarForm> toScalarFormUnder(const MathNode& node, const ScalarSubstitution& subst,
                                            std::string* why = nullptr);
std::optional<std::array<ScalarForm, 3>> toVectorFormUnder(const MathNode& node,
                                                           const ScalarSubstitution& subst,
                                                           std::string* why = nullptr);

// One scalar expression restricted to the line: value(s) = Σ coefficients[k]·s^k.
// No coefficient mentions kLineParameterVar. Empty coefficients == identically 0.
struct LineRestrictedForm {
    std::vector<ScalarForm> coefficients;
    // Every symbol the coefficients read, line origin/direction included. A
    // caller must bind all of them before evaluateCoefficients can answer.
    std::set<std::string> reads;

    // Structural degree in s (−1 for the identically-zero form).
    int degree() const { return static_cast<int>(coefficients.size()) - 1; }

    // The structural degree on ONE concrete line: the highest k whose
    // coefficient does not vanish EXACTLY. Exact vanishing is proved, never
    // measured: every term of c_k carries a power factor of a symbol whose
    // bound value is exactly 0.0 (a zero direction component, say). A
    // coefficient that merely evaluates to a tiny or zero double through
    // cancellation is NOT treated as vanishing — that would be a numerical
    // guess about a real-number identity.
    int exactDegreeOn(const std::map<std::string, double>& bindings) const;

    // Evaluate every c_k. nullopt if any symbol is unbound or a value is
    // undefined (e.g. ln of a non-positive argument) or non-finite.
    std::optional<std::vector<double>> evaluateCoefficients(
        const std::map<std::string, double>& bindings) const;

    // Σ |each term of each c_k| · |s|^k at s — the magnitude scale against
    // which a rounding bound is stated (forward error of a sum/product chain
    // is O(eps · Σ|terms|), not O(eps · |result|)).
    std::optional<double> magnitudeBound(const std::map<std::string, double>& bindings,
                                         double s) const;

    // Horner evaluation of Σ a_k s^k.
    static double evaluatePolynomial(const std::vector<double>& a, double s);

    std::string print() const;
};

struct LineRestriction {
    // Engaged iff admitted. When disengaged, `refusal` names the first
    // operation or structure that kept the exact symbolic route from holding.
    std::optional<LineRestrictedForm> form;
    std::string refusal;
    explicit operator bool() const { return form.has_value(); }
};

struct VectorLineRestriction {
    std::optional<std::array<LineRestrictedForm, 3>> form;
    std::string refusal;
    explicit operator bool() const { return form.has_value(); }
};

// The point binding: x/y/z and the vector `pointVariable` (default "p") become
// line.o.c + line.s·line.d.c. This matches varsAtPoint() on the CPU and
// pointComponent() in the WGSL emitter, which treat x/y/z as the point's
// components.
ScalarSubstitution linePointSubstitution(const std::string& pointVariable = kAmbientPointVar);

LineRestriction restrictToLine(const MathNode& node,
                               const std::string& pointVariable = kAmbientPointVar);
LineRestriction restrictToLine(const Piecewise& model,
                               const std::string& pointVariable = kAmbientPointVar);
VectorLineRestriction restrictVectorToLine(const MathNode& node,
                                           const std::string& pointVariable = kAmbientPointVar);
VectorLineRestriction restrictVectorToLine(const Piecewise& model,
                                           const std::string& pointVariable = kAmbientPointVar);

// Collect a ScalarForm by powers of `parameter`. Refuses when the parameter has
// a non-integer/negative exponent or appears in a transcendental factor.
std::optional<std::vector<ScalarForm>> collectPowers(const ScalarForm& form,
                                                     const std::string& parameter,
                                                     std::string* why = nullptr);

} // namespace OntoMath
