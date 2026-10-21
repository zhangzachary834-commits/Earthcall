#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <functional>
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
// ops, Stochastic, Raycast, LineIntegral, Integral, Unsupported. A Piecewise is admitted
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

// ============================================================================
// Rung 2 — piecewise-algebraic line restriction and sound line segmentation.
//
// The polynomial class above cannot hold the operations real authored media
// are made of: the 2026-10-16 census of saves/zones found 30 of 32 densities
// stopping at Clamp, with Length/Distance/Sqrt beneath most of them. On a line
// those are still exactly structured:
//   |o + s·d − c|                 = sqrt(Q(s)),  Q a polynomial (quadratic if affine)
//   abs / min / max / clamp / bounded Piecewise
//                                 = one algebraic branch on each region where a
//                                   SWITCHING FUNCTION g(s) keeps its sign.
// A LineAlgebraicForm is a small node arena whose leaves are the Rung 0
// polynomial forms (still symbolic in o/d, authored-revision lifetime). Each
// branch node declares its switching functions. Mirrored semantics
// (MathNode::evaluate / the WGSL emitter):
//   Sqrt(a)        sqrt(max(a, 0))            switch a          (none for Length/Distance:
//                                                                a sum of squares is ≥ 0)
//   Abs(a)         |a|                        switch a
//   Min/Max(a,b)   Union / Intersection       switch a − b      (Difference = max(a, −b))
//   Clamp(a,lo,hi) std::clamp, lo ≤ hi pure constants only (std::clamp is
//                  undefined for lo > hi)     switches a − lo, a − hi
//   Select         a Piecewise with interval bounds on its input variable, first
//                  match wins, pieces after the first unbounded one never
//                  reached (exactly where emitPiecewise stops); no match is
//                  Undefined — the CPU says nullopt, the emitter says 0, and
//                  OntoMath refuses to choose: a channel binding decides.
//                  Switches sel − lo, hi − sel, STRICT (the emitter's >=/<= and
//                  the CPU's include flags differ exactly at the bound, and a
//                  Piecewise is discontinuous there, so no piece may claim it).
// Continuous branches use non-strict signs: at g = 0 both branches agree.
//
// Still refused (after Rungs 2c/2e): ln of a bound coordinate, Tan, Map, SDF,
// Gradient, matrix ops, guards, calls, folds, pieces without a MathNode, vector
// Clamp/Sqrt/Pow, Clamp with non-constant or inverted bounds, and a Piecewise
// whose input variable is the vector point.
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
// ============================================================================

struct LineAlgebraicNode {
    // Kernel IR, never serialized: free to grow, unlike MathNode::Op.
    // Trans (Rung 2c): sin/cos/exp of an affine argument. Every OntoMath
    // transcendental is kind(scale·var + shift); restricted to a line its
    // argument is the polynomial α·s + β (operand a). Ln of a bound coordinate
    // stays refused: the CPU answers undefined at ≤ 0, the emitter log(max(·,1e-8)).
    // Noise (Rung 2e): classic Perlin of a vec3 argument (operands a, b, c).
    // Never re-derived here: its range is OntoMath's own load-bearing theorem
    // (MathNode::evalRange on Noise — glm::perlin's lattice formula intersected
    // with the proved Lipschitz ball) over the argument's enclosure box.
    enum class Kind : uint8_t { Polynomial, Add, Sub, Mul, Sqrt, Abs, Min, Max, Clamp, Select, Undefined, Trans, Noise };
    struct Piece {
        bool hasLo = false, hasHi = false;
        double lo = 0.0, hi = 0.0;
        uint32_t value = 0;
    };
    Kind kind = Kind::Polynomial;
    LineRestrictedForm polynomial;   // Polynomial: the value; Select: the selector
    uint32_t a = 0, b = 0, c = 0;    // operands (c: Noise's third component)
    double lo = 0.0, hi = 0.0;       // Clamp bounds
    bool nonnegativeArgument = false; // Sqrt of a sum of squares: no switch
    // Rung 2f: when that Sqrt is a Euclidean norm ‖v‖ — Length/Distance, or any
    // authored sqrt of a sum of squares — the components of v (never
    // operands of the value; read only by derivativeRange, which bounds
    // |d‖v‖/ds| ≤ ‖v′‖ by Cauchy–Schwarz — finite even where ‖v‖ = 0).
    std::vector<uint32_t> normComponents;   // empty = not a recognized Euclidean norm
    // The authored MathNode operand(s) a branch node was built from (Clamp/Abs/
    // Sqrt/Min/Max), so a GPU emitter can lower a switch function with the
    // production emitter's own arithmetic. Points into the authored Piecewise:
    // valid exactly as long as the cached form (same revision + identity key).
    const MathNode* sourceA = nullptr;
    const MathNode* sourceB = nullptr;
    TransFactor::Kind transKind = TransFactor::Kind::Sin;   // Trans
    // Execution provenance (Codex P1, 2026-10-17): the AUTHORED expression a
    // device executes for this node's value, when the line algebra simplified
    // it — every Polynomial leaf (a flattened subtree may have cancelled) and a
    // Length/Distance norm. Exactly one of `provenance` (a switch-free authored
    // subtree, component −1 for a scalar) or `provenanceForm` (an authored
    // ScalarForm in pre-substitution symbols) is set; a Polynomial leaf with
    // neither is UNBOUNDED for execution slack (never silently exact).
    const MathNode* provenance = nullptr;
    int8_t provenanceComponent = -1;
    std::shared_ptr<const ScalarForm> provenanceForm;
    std::vector<Piece> pieces;       // Select, authored order
    uint32_t switchBegin = 0, switchCount = 0;
};

struct LineSwitch {
    uint32_t node = 0;
    // 0: a (Sqrt/Abs)   1: a − b (Min/Max)   2: a − lo   3: a − hi (Clamp)
    // 4 + 2i: sel − lo_i   5 + 2i: hi_i − sel (Select piece i)
    uint32_t role = 0;
    bool strict = false;
};

struct LineAlgebraicForm {
    std::vector<LineAlgebraicNode> nodes;
    std::vector<uint32_t> roots;       // one for a scalar channel, three for vec3
    std::vector<LineSwitch> switches;
    std::set<std::string> reads;       // every symbol any leaf or selector reads
    bool polynomialOnly() const;
};

struct LineAlgebraicRestriction {
    std::optional<LineAlgebraicForm> form;
    std::string refusal;
    explicit operator bool() const { return form.has_value(); }
};

LineAlgebraicRestriction restrictAlgebraicToLine(const MathNode& node, bool vectorValued,
                                                 const std::string& pointVariable = kAmbientPointVar);
LineAlgebraicRestriction restrictAlgebraicToLine(const Piecewise& model, bool vectorValued,
                                                 const std::string& pointVariable = kAmbientPointVar);

// ---------------------------------------------------------------------------
// Execution provenance (2026-10-17). Codex's source review (P1, two findings)
// showed that the algebra above can be MORE exact than what runs:
// ((A + x) − A) − x simplifies to the exact 0 while an f32/f64 device executing
// the authored tree returns ±1 at A = 2^54, and lo = B − (B − 1) folds to 1 on
// the host while f32 gives 0 at B = 2^25. A claim that replaces or skips
// execution must therefore carry a bound on |executed − exact| computed from
// the ORIGINAL authored tree, never from its simplification.
//
// executionBound answers, per (component of a) node, sup|value| by magnitude
// arithmetic (|a + b| ≤ |a| + |b|: no cancellation, so no erased rounding) and
// an absolute slack ≥ |executed − exact| under a DECLARED device model:
//   relative       every arithmetic operation, literal and coordinate read errs
//                  by ≤ relative × its magnitude bound, in any grouping
//                  (reassociation and fusion only move error between
//                  operations the model already charges);
//   transcendental each sin/cos/noise evaluation errs by ≤ this, absolute;
//                  exp inherits it unless expAbsolute is explicitly supplied.
//   expAbsolute, expRelative price exp as A + R*exp(x), independently.
// It is a declared model, not a proof of any device's Fast-math enclosure (see
// docs/audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md). `magnitudes` maps a
// symbol to sup|symbol| over the region; the point variable resolves through
// "x"/"y"/"z". An unknown symbol, an operation outside the line-admitted
// class, or a non-literal divisor, exponent or clamp bound answers nullopt:
// the caller must refuse its claim.
//
// Three fields the device never needs and the CPU line evaluator does
// (2026-10-20, plan §6aj). LineAlgebraicEvaluator::value() does not execute the
// authored tree: it evaluates the RESTRICTED polynomials in binary64 (Horner,
// on coefficients carrying a proved error enclosure) and rounds to binary32 in
// one place only, the argument of glm::perlin.
//   restrictedForm  slack() bounds that execution: a polynomial by its
//                   coefficient errors plus Horner's γ_2n·Σ|c_k||s|^k, every
//                   other node by its structural rule; authored provenance is
//                   not consulted (it describes a different execution).
//   vector          a Noise argument is rounded by ≤ this × its magnitude
//                   (2^-24 for glm::vec3); 0 = no such rounding.
//   noise           the absolute allowance for one Noise evaluation's own
//                   arithmetic; negative = use transcendental.
struct ExecutionModel {
    double relative = 0.0;
    double transcendental = 0.0;
    double vector = 0.0;
    double noise = -1.0;
    bool restrictedForm = false;
    // Exp has separate absolute/relative contracts. Negative expAbsolute
    // inherits transcendental (the existing device model is absolute).
    // CPU libm explicitly declares one-ulp relative plus subnormal error.
    double expAbsolute = -1.0;
    double expRelative = 0.0;
    double exponentialAbsolute() const { return expAbsolute >= 0.0 ? expAbsolute : transcendental; }
    bool active() const { return relative > 0.0 || transcendental > 0.0 || vector > 0.0 || noise > 0.0 ||
                                 expAbsolute > 0.0 || expRelative > 0.0; }
};
// The CPU line evaluator's declared model: restricted form, binary64 (2^-52
// per operation, libm within one ulp), binary32 Noise arguments, and one
// glm::perlin call's float interpolation within 2^-14 absolute. That last
// number is a PREMISE, corroborated not proved: against a reference with glm's
// own float lattice hash and gradients but double interpolation, the worst over
// 4·10^6 points (|p| up to 9.5e6) was 2.9e-6, and the largest normalized
// gradient 0.99863 (the Lipschitz contract assumes ≤ 1). See plan §6aj.
ExecutionModel cpuExecutionModel();
struct ExecutionBound {
    double magnitude = 0.0;
    double slack = 0.0;
};
// `signedRanges` (optional, 2026-10-20 plan §6am): [lo, hi] for symbols whose
// SIGN is known. Magnitude arithmetic alone must bound exp(a·x + b) by
// e^(|a|·|x| + |b|), which for an optical depth charges exp(−τ) as if it were
// e^(+τ) — useless for an opaque medium. With x's signed range, exp's argument
// is bounded by its true signed maximum. Absent (the device's call), unchanged.
using SignedRanges = std::map<std::string, std::pair<double, double>>;
std::optional<ExecutionBound> executionBound(const MathNode& node, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, int component = -1,
                                             const std::string& pointVariable = kAmbientPointVar,
                                             const SignedRanges* signedRanges = nullptr);
std::optional<ExecutionBound> executionBound(const ScalarForm& form, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, const SignedRanges* signedRanges = nullptr);
// Every piece the emitter can answer with (through the first unbounded one).
std::optional<ExecutionBound> executionBound(const Piecewise& piecewise, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, int component = -1,
                                             const std::string& pointVariable = kAmbientPointVar);
// A literal: a ScalarLeaf of at most one constant term. Host and device read
// the same authored number (f32-rounded once on upload), so it is the only
// constant a divisor, exponent or clamp bound may be (Codex P1 finding 2).
std::optional<double> literalConstant(const MathNode& node);

// RAY tier: one LineAlgebraicForm bound to one concrete line. Construction
// evaluates every polynomial coefficient once (double value plus a sound
// enclosure from a forward-error bound — deliberately NOT ScalarForm::evalRange,
// whose constant terms are float points that need not contain the double
// coefficient). Signs vectors are indexed like form.switches: 0 undecided,
// +1 g ≥ 0 (> 0 when strict), −1 g ≤ 0 (< 0 when strict).
// What hybridIntegral answers: the estimate, a proved bound on its distance from
// the exact integral, and how much numerical work the remainder cost.
// A factor the caller samples (2026-10-19, for ∫ σ_s·T ds: the transmittance
// T = e^(−τ) is a cumulative integral, not an authored node). `value` at a
// point; `range` a proved enclosure over [a, b]; `lipschitz` a proved bound on
// |dM/ds| over [a, b] (infinity when unknown).
struct LineSampledFactor {
    std::function<std::optional<double>(double)> value;
    std::function<std::pair<double, double>(double, double)> range;
    std::function<double(double, double)> lipschitz;
    // Optional pointwise contract (2026-10-20): sup |value(s) − exact M(s)| over
    // [a, b]. Without it, an integral under an active execution model charges
    // this factor's samples without bound and refuses — it never assumes 0.
    std::function<double(double, double)> slack;
};

struct LineHybridIntegral {
    double estimate = 0.0;
    double radius = 0.0;
    uint32_t remainderTerms = 0;         // sampled w·G terms (0: closed form)
    uint32_t remainderEvaluations = 0;   // point evaluations of their G nodes
    uint32_t factorEvaluations = 0;      // point evaluations of a sampled factor
};

class LineAlgebraicEvaluator {
public:
    // undefinedAsZero is a CHANNEL decision (the volume emitter returns 0 when no
    // piece matches); OntoMath's own default keeps Undefined undefined.
    LineAlgebraicEvaluator(const LineAlgebraicForm& form, std::map<std::string, double> bindings,
                           bool undefinedAsZero = false);
    // BOX mode (Rung 2f-native groundwork): bindings are INTERVALS — e.g. the line
    // origin ranging over a whole medium box, so derivativeRange at s ∈ [0, 0]
    // with d = e_i encloses ∂g/∂x_i everywhere in the box. Coefficients become
    // enclosures (centre ± radius) from ScalarForm::evalRange, widened one ulp
    // outward (its lone constants are float points). range()/derivativeRange()
    // stay sound; value()/proposeRoots() are not meaningful in this mode, and
    // singleton bindings remain exact for structural constancy proofs; interval
    // origins are never replaced by their centres for that purpose.
    LineAlgebraicEvaluator(const LineAlgebraicForm& form, const std::map<std::string, Interval>& boxBindings,
                           bool undefinedAsZero);
    bool boxMode() const { return _boxMode; }
    bool ok() const { return _refusal.empty(); }
    const std::string& refusal() const { return _refusal; }
    const LineAlgebraicForm& form() const { return *_form; }

    // Point value with the authored semantics; nullopt when Undefined.
    std::optional<double> value(uint32_t node, double s) const;
    // Sound enclosure of the real value over s ∈ [s0, s1] under decided signs.
    // *undefined is set when Undefined may be selected somewhere in the range.
    Interval range(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs,
                   bool* undefined) const;
    Interval switchRange(std::size_t k, double s0, double s1, const std::vector<int8_t>& signs,
                         bool* undefined) const;
    // Shared conservative branch classifier. An active model requires its
    // execution margin; inactive means exact mathematics. Unknown is 0.
    int8_t classifySwitch(std::size_t k, double s0, double s1, const std::vector<int8_t>& signs) const;
    // Proved constant over the whole region the signs describe: every surviving
    // polynomial leaf vanishes exactly in degree ≥ 1 on this line.
    bool constantUnder(uint32_t node, const std::vector<int8_t>& signs) const;
    // Undecided switches that can still change the value under these signs
    // (a saturated clamp hides everything beneath it).
    void relevantUndecided(uint32_t node, const std::vector<int8_t>& signs,
                           std::vector<uint32_t>& out) const;
    // Rung 2d: PROPOSE where switches may change sign, in closed form, for the
    // switches whose function is α·B + β with B a polynomial of degree ≤ 2 on
    // this line, or sqrt of one (every radial clamp, every Piecewise bound on an
    // affine selector). Proposals only choose where segmentLine cuts; every
    // piece is still PROVED by interval range. A wrong proposal costs bisection,
    // never soundness. Appends roots inside (s0, s1).
    void proposeRoots(double s0, double s1, std::vector<double>& out) const;
    // Rung 2f: sound enclosure of d/ds over [s0, s1] (forward-mode interval
    // differentiation). Chain rule for sin/cos/exp; Noise via OntoMath's proved
    // Lipschitz constant (kClassicPerlin3LipschitzBound); undecided abs/min/max/
    // clamp take the hull of their branches' derivatives (they are Lipschitz, so
    // that hull bounds every difference quotient); sqrt near 0 and an undecided
    // Select (a jump) answer the unbounded interval — no false Lipschitz claim.
    Interval derivativeRange(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const;
    // An enclosure of the SECOND derivative along s (2026-10-20, plan §6ak), for
    // the hybrid's second-order remainder bound. Sound only where the node is
    // C¹ with a Lipschitz derivative: every kink or jump (an undecided Abs,
    // Min/Max, Clamp or Select; sqrt near 0) answers the unbounded interval,
    // and the caller keeps its first-order bound there. A defined subtree proved
    // constant along the direction has both derivatives zero, even at a
    // transverse crease; that says nothing about its transverse smoothness.
    Interval secondDerivativeRange(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const;
    // Exact ∫[s0, s1] of a node along this line, under signs decided over the
    // whole interval (a definite segmentLine piece): the integration operation
    // on the ray (2026-10-18; Zach: "lets enrich the ontomath integration
    // substrate"). The branch the signs select must reduce to
    //   P(s) + Σ R(s)·√Q(s) + Σ P_k(s)·g_k(α_k s + β_k),   g ∈ {sin, cos, exp},
    // P, R, P_k polynomials and Q of degree ≤ 2 — every ball, ellipsoid,
    // cylinder and abs shell restricted to a ray, times any polynomial falloff,
    // and every product of waves (product-to-sum: a gyroid) is of this form.
    // Polynomials are integrated about the interval's midpoint (no cancellation
    // against a far origin), R·√Q by a moment recurrence on the closed form of
    // √Q (asinh / log / asin by the sign of its discriminant), P·g by the
    // tabular rule ScalarForm::antiderivative uses. Branch nodes read only the
    // operand their decided branch selects (2026-10-19). Anything else answers
    // nullopt with the reason, never an approximation: noise (no closed-form
    // integral), an undecided branch, a root of a non-quadratic (a torus's
    // nested norm), two different square roots multiplied (elliptic), a square
    // root times a wave, exp times sin/cos, or Undefined. Exact-arithmetic
    // semantics; not available in box mode.
    std::optional<double> integral(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs,
                                   std::string* why = nullptr) const;
    // Hybrid integration (2026-10-19; the Original Prism Sun's decree step 3,
    // "numerically integrate only the irreducible remainder"). The decided branch
    // splits into a closed form plus Σ w_k(s)·G_k(s): each G_k an authored node
    // with no closed form here (noise, an elliptic product), each weight w_k
    // closed-form. G_k is sampled at `cells` + 1 evenly spaced points and
    // interpolated by a quadratic over each pair of cells (product Simpson);
    // ∫ w_k·(that interpolant) is exact, so the smooth factors contribute no
    // quadrature error at all. `radius` is a proved bound on |estimate − exact|
    // from G's proved range and derivative bound on each panel.
    // A fully closed-form branch answers with radius 0 and no evaluations.
    std::optional<LineHybridIntegral> hybridIntegral(uint32_t node, double s0, double s1,
                                                     const std::vector<int8_t>& signs, uint32_t cells,
                                                     std::string* why = nullptr) const;
    // ∫ node·M ds for a sampled factor M: the closed-form part of the branch
    // becomes one more weight, every sampled function is G_k·M (or M), and each
    // panel's radius uses the product's range and derivative bound.
    std::optional<LineHybridIntegral> hybridIntegral(uint32_t node, double s0, double s1,
                                                     const std::vector<int8_t>& signs, uint32_t cells,
                                                     const LineSampledFactor& factor, std::string* why = nullptr) const;
    Interval switchDerivativeRange(std::size_t k, double s0, double s1, const std::vector<int8_t>& signs) const;

    // Execution slack (Codex P1, 2026-10-17). With an active model (the
    // declared device model of executionBound), slack() bounds |executed −
    // exact| for a node over [s0, s1] under decided signs, from each leaf's
    // AUTHORED provenance: a clamp proved saturated executes its literal bound
    // exactly (slack ε|bound|), an undecided Select may jump (unbounded), a
    // proved-negative sqrt argument executes 0. switchSlack() bounds a switch
    // function's. segmentLine decides a sign only beyond that slack, so an
    // algebraic cancellation can never certify what the device does not do.
    // An inactive model (the default) keeps exact-arithmetic semantics. Symbols
    // the evaluator does not bind (e.g. a box's unbounded clock) take
    // `magnitudeOverrides` (sup|symbol|) for slack only. A leaf's authored slack
    // is monotone in its region, so the first interval asked (segmentLine's
    // whole span) is reused for every interval inside it.
    void setExecutionModel(const ExecutionModel& model, std::map<std::string, double> magnitudeOverrides = {});
    const ExecutionModel& executionModel() const { return _model; }
    double slack(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const;
    double switchSlack(std::size_t k, double s0, double s1, const std::vector<int8_t>& signs) const;

private:
    std::optional<LineHybridIntegral> integrateCore(uint32_t node, double s0, double s1,
                                                    const std::vector<int8_t>& signs, uint32_t cells,
                                                    const LineSampledFactor* factor, std::string* why) const;
    double magnitudeAt(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs) const;
    std::map<std::string, double> symbolMagnitudes(double s0, double s1) const;
    ExecutionModel _model;
    std::map<std::string, double> _magnitudeOverrides;
    std::map<std::string, Interval> _box;
    mutable double _slackMemoS0 = 0.0, _slackMemoS1 = -1.0;
    mutable std::vector<double> _slackMemo;
    mutable std::vector<char> _slackMemoValid;
    mutable std::map<std::string, double> _slackMagnitudes;
    Interval rangeAt(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs, bool* undefined) const;
    Interval rangeUnmemoized(uint32_t node, double s0, double s1, const std::vector<int8_t>& signs, bool* undefined) const;
    // Centered (Taylor-shifted) enclosure of a polynomial node over [s0, s1]:
    // re-expanded about the midpoint in double with an explicit forward-error
    // bound, then evaluated over h ∈ [s0 − m, s1 − m]. Plain Horner over [s0, s1]
    // widens like |s|·w (the dependency problem) and could not decide a grazing
    // sphere/ray switch; the centered form widens like |f′(m)|·w.
    Interval polynomialRange(uint32_t node, double s0, double s1) const;
    Interval polynomialRangeUncached(uint32_t node, double s0, double s1) const;
    void computeSignFree();
    const LineAlgebraicForm* _form;
    std::map<std::string, double> _bindings;
    bool _undefinedAsZero;
    std::vector<std::vector<double>> _coefficients;    // per node (Polynomial value / Select selector)
    std::vector<std::vector<double>> _errors;          // absolute error bound per coefficient
    std::string _refusal;
    // Range memo for ONE interval at a time, over every SIGN-FREE node (no
    // switch anywhere beneath it: polynomial leaves, and e.g. a Perlin noise of a
    // polynomial argument). Such a node's enclosure depends only on [s0, s1],
    // never on switch signs, yet every switch of a classification pass re-reads
    // it. Kernel scratch beneath any property path; makes the evaluator
    // single-threaded (it is ray-lifetime).
    bool _boxMode = false;
    std::vector<char> _signFree;
    mutable double _memoS0 = 0.0, _memoS1 = -1.0;
    mutable std::vector<Interval> _memoRange;
    mutable std::vector<char> _memoValid;
    mutable double _treeMemoS0 = 0.0, _treeMemoS1 = -1.0;
    mutable std::vector<Interval> _treeMemoRange;
    mutable std::vector<char> _treeMemoValid;
    mutable uint64_t _memoHits = 0, _memoMisses = 0;
    // constantUnder answers per node for ONE sign vector at a time: a pure
    // function of construction and signs, re-asked by every node of every
    // derivative recursion (plan §6bc). -1 unknown, 0 false, 1 true.
    mutable std::vector<int8_t> _constantSigns, _constantMemo;
    bool constantUnderUncached(uint32_t node, const std::vector<int8_t>& signs) const;
    // Composite nodes BENEATH a switch, for one ([s0, s1], signs) at a time:
    // derivative recursions re-range every child at every level. Stores the
    // range and whether evaluating it reports undefined (a set-only flag that no
    // branch reads), so a hit answers exactly as a recomputation would.
    mutable double _signedMemoS0 = 0.0, _signedMemoS1 = -1.0;
    mutable std::vector<int8_t> _signedMemoSigns;
    mutable std::vector<Interval> _signedMemoRange;
    mutable std::vector<char> _signedMemoState;   // 0 absent, 1 defined, 2 reports undefined
public:
    uint64_t leafRangeHits() const { return _memoHits; }
    uint64_t leafRangeMisses() const { return _memoMisses; }
};

// Partition [s0, s1] into pieces on which every RELEVANT switch of every
// evaluator has a proved sign ("definite"), plus narrow pieces no wider than
// `resolution` that still straddle a possible switch ("uncertain" — the exact
// evaluator must run there). Interval branch-and-bound over OntoMath::Interval;
// a piece is only ever called definite by proof, never by sampling. Pieces are
// contiguous, ordered, and cover [s0, s1]. maxRangeEvaluations is a Kernel
// budget: when exhausted, the remainder is uncertain (exact fallback).
// Sound spatial Lipschitz bound of switch k's function over an axis-aligned box:
// sqrt(Σ_i sup|∂g/∂x_i|²) (Cauchy–Schwarz), each partial enclosed by a box-mode
// derivative along e_i. `others` binds every non-point symbol (e.g. t as the
// unbounded interval for a clock-independent certificate). nullopt when any
// partial is unbounded (sqrt at 0, a possible jump, an unbounded symbol).
std::optional<double> lipschitzBoundOverBox(const LineAlgebraicForm& form, std::size_t switchIndex,
                                            const Interval box[3],
                                            const std::map<std::string, Interval>& others,
                                            bool undefinedAsZero = true);

// ---- A line-quadrature certificate for a DEVICE (2026-10-20, plan §6ap) ----
// For a scalar field f(p), and each magnitude band [−cap, cap]³ (cap = 2^j),
// what one f32 device quadrature of f along ANY line inside the band needs:
//   lipschitz  sup |∇f| over the band (box-mode partials, Cauchy–Schwarz);
//   width, sup the band's range of f (its width and largest magnitude);
//   execution  |device f(p) − f(p)| for one evaluation, under the DEVICE's
//              own declared model (kVolumeExecutionModel), coordinates ≤ cap.
// Magnitude arithmetic is monotone, so a ray whose endpoints lie in a band is
// covered by it. A band with an unbounded quantity is left out; a ray beyond
// every band refuses. This is the host half of step 2c: the device picks the
// band and the cell count per ray from these constants (SdfWgsl
// compileLineQuadrature); nothing per-ray is proved on the host.
//   hessian    sup ‖∇²f‖ over the band (Frobenius of box-mode second partials,
//              mixed ones from e_i + e_j); infinite where f is not C¹ with a
//              Lipschitz gradient there — then only the first-order radius.
struct LineQuadratureBand {
    double cap = 0.0, lipschitz = 0.0, width = 0.0, sup = 0.0, execution = 0.0;
    double hessian = std::numeric_limits<double>::infinity();
    double low = 0.0, high = 0.0;   // the band's signed range of f
    // Mathematical branch proof. Equal Hessians on different branches do not
    // prove a Hessian bound across their join; grid aggregation checks this.
    std::vector<int8_t> branchSignature;
};
// Transient Kernel mathematical execution evidence, not authored state.
// Direction-qualified mathematical derivatives, per unit of the ray's own
// parameter, for every d in [directionLo,directionHi]: slope >= |D_d f|
// and curvature >= |D_d^2 f|. These never replace
// the region's isotropic Lipschitz/Hessian or price transverse position error.
struct LineDirectionalBound {
    std::array<double, 3> directionLo{}, directionHi{};
    double slope = std::numeric_limits<double>::infinity();
    double curvature = std::numeric_limits<double>::infinity();
};
struct LineQuadratureCertificate {
    std::vector<LineQuadratureBand> bands;   // increasing cap
    ExecutionModel model;
    // Qualification domain, retained for device runtime binding checks. This is
    // mathematical evidence, not authored state or a cache-validity token.
    std::map<std::string, std::pair<double, double>> symbols;
    std::string sourceExpression;  // exact source snapshot; prevents stale execution plans
};
// `symbols` binds every non-point symbol to an interval [lo, hi]: a frame's
// clock t as a point, a view direction ω as [−1, 1]³ so one certificate covers
// every pixel's direction (box mode ranges over them).
std::optional<LineQuadratureCertificate> certifyLineQuadrature(const MathNode& field, const ExecutionModel& deviceModel,
                                                               int maxBand = 20, std::string* why = nullptr,
                                                               const std::map<std::string, std::pair<double, double>>& symbols = {});
// The CPU mirror of the device's per-ray plan (same arithmetic, in double):
// the band, the even cell count n, and the proved total for f32 execution.
struct LineQuadraturePlan {
    int band = -1;
    double cells = 0.0, mathematical = 0.0, execution = 0.0;
    double total() const { return mathematical + execution; }
};
std::optional<LineQuadraturePlan> planLineQuadrature(const LineQuadratureCertificate& cert, const double o[3],
                                                     const double d[3], double lo, double hi, double tolerance,
                                                     double maxCells);

// ---- Transport along a line (plan §6aq) ----
// L = ∫_lo^hi exp(−τ(s))·ε(q(s)) ds, τ(s) = ∫_lo^s σ(q(u)) du, with σ and ε
// each certified (above). The device marches ONCE: depth accumulates by the
// same Simpson panels (the midpoint by the quadratic's exact partial
// integral), so the work is O(cells), not O(cells²). The bound is a priori,
// from the two certificates on the ray's band. With σ ≥ σ_low there, e^−τ ≤
// T_max = e^(max(0,−σ_low)·S); every factor below is scaled by it (= 1 when
// σ ≥ 0 is proved), and |e^−a − e^−b| ≤ F·|a − b| with F = T_max·e^δτ:
//   depth error   δτ ≤ (S + 2h)·[(1+Λ)·local_σ(h) + σ's device error + rounding]
//   sample error  δτ·ε_max + ε's device error + exp's device error
//   outer radius  (1+Λ)·min(width_g, |g′|·hw, |g″|·hw²/2), g = e^−τ·ε,
//                 |g′| ≤ |ε′| + σ_max|ε|, |g″| ≤ |ε″| + 2σ|ε′| + |σ′||ε| + σ²|ε|
// The cells are the smallest power of two whose total fits the tolerance.
struct LineTransportPlan {
    int sigmaBand = -1, emissionBand = -1;
    double cells = 0.0, depthError = 0.0, mathematical = 0.0, execution = 0.0;
    double total() const { return mathematical + execution; }
};
// The A-POSTERIORI bound (plan §6as): the same march, its bound accumulated as
// it goes — the depth error δτ_k grows from 0 along the ray, and each sample's
// share is weighted by the transmittance actually computed there:
//   |e^−τ − e^−τ̃| ≤ e^−τ̃·(e^δτ − 1),   the outer radius per panel with
//   T ≤ e^(−τ̃_a + δτ_a)·e^(max(0,−σ_low)·2h).
// The device terms are the certificates' (the declared device model), so this
// CPU march in double bounds what the DEVICE march would compute. `sigmaAt`
// and `emissionAt` evaluate σ and ε at the line parameter s.
struct LineTransportMarch {
    double value = 0.0, mathematical = 0.0, execution = 0.0;
    double total() const { return mathematical + execution; }
    double transmittance = 1.0, transmittanceMathematical = 0.0, transmittanceExecution = 0.0;
    double transmittanceTotal() const { return transmittanceMathematical + transmittanceExecution; }
};
std::optional<LineTransportMarch> marchLineTransport(const LineQuadratureCertificate& sigma,
                                                     const LineQuadratureCertificate& emission,
                                                     const std::function<double(double)>& sigmaAt,
                                                     const std::function<double(double)>& emissionAt, const double o[3],
                                                     const double d[3], double lo, double hi, double cells,
                                                     std::string* why = nullptr);
// A camera-independent proof grid over one box (plan §6bd). Every cell of
// [first, last] is in exactly one of two states:
//  * occupied: its certificate is bit-identical to RegionCertificate::over()
//    of that single cell (the same box, the same certifyBox);
//  * inside a zero block: ONE containing box's certificate proves the field
//    mathematically ≡ 0 there (range exactly [0, 0]). A proof over a box holds
//    on every subset of it, so no sub-cell is visited. A consumer may skip such
//    a cell (zero contributes zero error) or evaluate it under the block's own
//    certificate, which bounds device execution anywhere in the block.
// Transient Kernel proof evidence, not authored state; source/model/symbol
// edits require rebuilding it (it borrows the certificate's provenance).
struct RegionProofGrid {
    struct ZeroBlock {
        std::array<long long, 3> first{}, last{};
        LineQuadratureBand certificate;
    };
    double cell = 0.0;
    std::array<long long, 3> first{}, last{};
    std::vector<ZeroBlock> zero;
    std::map<std::array<long long, 3>, LineQuadratureBand> occupied;
    std::size_t proofs = 0;   // certifyBox calls made (host work, not device work)
};

// Per-region certificates (plan §6at): a lazily filled grid of cubic cells of
// side `cell`. over(lo, hi) answers the worst constants over every cell the box
// touches (largest L, H, sup and device error; smallest low, largest high).
// Small cells decide a field's clamps, so its Hessian is finite where a
// band-wide one is not, and its L and magnitudes are local. On a device this is
// a buffer of cell constants; nothing per ray is proved on the host.
class RegionCertificate {
public:
    RegionCertificate(const MathNode& field, const ExecutionModel& model, double cell,
                      const std::map<std::string, std::pair<double, double>>& symbols = {});
    bool ok() const { return bool(_restriction) && std::isfinite(_cell) && _cell > 0.0; }
    std::optional<LineQuadratureBand> over(const double lo[3], const double hi[3]) const;
    // Exact query box, reusing the same lowered source/model office. Transient
    // host proof for schedule derivation; no CPU-per-pixel native consumer.
    std::optional<LineQuadratureBand> tightOver(const double lo[3], const double hi[3]) const;
    // Same whole-box evidence, plus derivatives in the specified direction.
    // No directional grid cache: the returned proof is scoped to this box/d.
    std::optional<std::pair<LineQuadratureBand, LineDirectionalBound>> alongOver(
        const double lo[3], const double hi[3], const double direction[3]) const;
    // Derivatives valid for every raw direction in this closed box. Exact zero
    // components may prove constancy; a nonzero-width component may not.
    std::optional<std::pair<LineQuadratureBand, LineDirectionalBound>> alongOver(
        const double lo[3], const double hi[3], const double directionLo[3], const double directionHi[3]) const;
    // Hierarchical proof of every cell touching [lo, hi]: certify a containing
    // box first and subdivide (longest index axis, halves) only where the field
    // is not proved ≡ 0. Work is proportional to the occupied cells, not the
    // volume. `maxProofs` is a Kernel work cap; at the cap, or when any single
    // cell cannot be certified, the grid is incomplete and nullopt returns.
    // Occupied leaves also fill over()'s cache (they are the same answers).
    std::optional<RegionProofGrid> proveGrid(const double lo[3], const double hi[3], std::size_t maxProofs,
                                             std::string* why = nullptr) const;
    double cell() const { return _cell; }
    const ExecutionModel& model() const { return _model; }
    const std::string& sourceExpression() const { return _sourceExpression; }
    const std::map<std::string, std::pair<double, double>>& symbols() const { return _symbols; }
private:
    // Restriction provenance borrows the immutable source tree, which must
    // outlive this transient certificate; no native retention yet.
    LineAlgebraicRestriction _restriction;
    ExecutionModel _model;
    std::string _sourceExpression;
    std::map<std::string, std::pair<double, double>> _symbols;
    double _cell;
    std::map<std::string, Interval> _symbolBox;
    std::map<std::string, double> _symbolMagnitudes;
    mutable std::map<std::array<long long, 3>, std::optional<LineQuadratureBand>> _cache;
    // The outward f32 box of cells [first, last]: the one construction shared by
    // over() and proveGrid(), so a leaf is the very box over() certifies.
    void indexBox(const long long first[3], const long long last[3], Interval box[3]) const;
};
// The a-posteriori march with each panel's constants from the region grid.
std::optional<LineTransportMarch> marchLineTransport(const RegionCertificate& sigma, const RegionCertificate& emission,
                                                     const std::function<double(double)>& sigmaAt,
                                                     const std::function<double(double)>& emissionAt, const double o[3],
                                                     const double d[3], double lo, double hi, double cells,
                                                     std::string* why = nullptr);
std::optional<LineTransportPlan> planLineTransport(const LineQuadratureCertificate& sigma,
                                                   const LineQuadratureCertificate& emission, const double o[3],
                                                   const double d[3], double lo, double hi, double tolerance,
                                                   double maxCells, std::string* why = nullptr);

// Shared arithmetic office for k*(1-v), given executed k/v and their whole
// absolute errors. Upstream uncertainty and new subtraction/product rounding
// are separate. The GPU lowering mirrors this formula with outward arithmetic.
struct ComplementProductBound {
    double propagated = 0.0, execution = 0.0;
    double total() const { return propagated + execution; }
};
std::optional<ComplementProductBound> boundComplementProduct(double coefficient, double coefficientError,
    double value, double valueError, double relative);

// A bounded executable schedule of the same nested integral, not a new
// authored transport law. Every panel retains its regional evidence; the
// ordered panels cover the full ray, including absorbing non-emitting spans.
// No CPU-per-pixel production consumer: the first use is a host reference and
// device execution witness. Native use still needs affordable family preparation,
// source admission and lifecycle integration.
struct LineTransportPanel {
    double lo = 0.0, hi = 0.0;
    LineQuadratureBand sigma, emission;
    std::optional<LineDirectionalBound> sigmaAlong, emissionAlong;
};
// Transient Kernel execution qualification, not an authored being/property.
// All origins/directions in these closed boxes share the prepared evidence.
struct LineRayDomain {
    std::array<double, 3> originLo{}, originHi{}, directionLo{}, directionHi{};
};
struct LineTransportSchedule {
    std::array<double, 3> origin{}, direction{};
    // Absent: the exact ray above. Present: that ray is only a representative;
    // both GPU schedule variants validate this full domain at execution time.
    std::optional<LineRayDomain> rayDomain;
    double lo = 0.0, hi = 0.0, pointError = 0.0;
    ExecutionModel model;
    std::string sigmaExpression, emissionExpression;
    std::map<std::string, std::pair<double, double>> sigmaSymbols, emissionSymbols;
    std::vector<LineTransportPanel> panels;
    LineTransportMarch result; // Host mirror only; family preparation leaves this unevaluated.
    bool admitted = false; // Host admission only; GPU execution always checks its own full bound.
    std::string refusal;
    std::size_t sigmaSamples = 0, emissionSamples = 0; // last host mirror's field sites; 0 when no mirror ran
    std::size_t sigmaPreparations = 0, emissionPreparations = 0; // host search cost, not device work
};
// Refines panels using a heuristic error score; ONLY the final whole-result
// bound admits. Budgets are Kernel work caps, never an accuracy policy. At a
// work cap a complete, bounded best schedule may return with admitted=false.
// Invalid inputs/incomplete certification return nullopt. Inputs are the exact
// f32 ray values a device will consume (the caller explicitly rounds its ray).
std::optional<LineTransportSchedule> planRegionalLineTransport(
    const RegionCertificate& sigma, const RegionCertificate& emission,
    const std::function<double(double)>& sigmaAt, const std::function<double(double)>& emissionAt,
    const double o[3], const double d[3], double lo, double hi, double radianceTolerance,
    double transmittanceTolerance, std::size_t maxPanels, std::string* why = nullptr);

// Prepare a complete geometric schedule once for a closed FAMILY of rays.
// No field callbacks, host samples or CPU integral: each device ray computes its
// own result and complete admission bound. A priority ranking of enclosure remainders
// allocates at most `panels` without numerical sampling. Panel count is a work cap,
// not an accuracy promise. Interval endpoints/domain bounds must be finite f32.
// Source/model/symbol/ray-domain edits require rebuilding these transient proofs.
std::optional<LineTransportSchedule> prepareRegionalLineTransport(
    const RegionCertificate& sigma, const RegionCertificate& emission, const LineRayDomain& rays,
    double lo, double hi, std::size_t panels, std::string* why = nullptr);

struct LineSegmentationPiece {
    double s0 = 0.0, s1 = 0.0;
    bool definite = false;
    std::vector<std::vector<int8_t>> signs;   // per evaluator
    std::vector<int8_t> targetSigns;          // per sign target: −1 proved ≤ 0, +1 proved > 0, 0 unknown
};
struct LineSegmentationStats {
    uint32_t rangeEvaluations = 0;
    uint32_t proposedRoots = 0;
    bool budgetExhausted = false;
};
std::vector<LineSegmentationPiece> segmentLine(const std::vector<const LineAlgebraicEvaluator*>& evaluators,
                                               double s0, double s1, double resolution,
                                               LineSegmentationStats* stats = nullptr,
                                               uint32_t maxRangeEvaluations = 1u << 16,
                                               // Measured: neutral over the first 14 admitted media (292.2 vs
                                               // 290.8 µs), but once Noise is admitted (28 media incl. Northern
                                               // Veil) it halves segmentation cost (26.2 vs 50.7 ms summed,
                                               // Release). On by default; always sound (proposals never decide).
                                               bool analyticProposals = true,
                                               // Signs already PROVED over a superset of [s0, s1]
                                               // (they hold on every sub-interval); null = all undecided.
                                               const std::vector<std::vector<int8_t>>* initialSigns = nullptr,
                                               // Expressions whose own sign must also be resolved, as
                                               // (evaluator index, node): a piece is definite only once each
                                               // is proved ≤ 0 or STRICTLY > 0 over it. This mirrors a
                                               // consumer's own branch (the volume marchers gate on D > 0),
                                               // so a structurally non-negative D still splits at its zeros.
                                               const std::vector<std::pair<std::size_t, uint32_t>>* signTargets = nullptr);

} // namespace OntoMath
