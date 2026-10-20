#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <array>
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
//   transcendental each sin/cos/exp/noise evaluation errs by ≤ this, absolute.
// It is a declared model, not a proof of any device's Fast-math enclosure (see
// docs/audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md). `magnitudes` maps a
// symbol to sup|symbol| over the region; the point variable resolves through
// "x"/"y"/"z". An unknown symbol, an operation outside the line-admitted
// class, or a non-literal divisor, exponent or clamp bound answers nullopt:
// the caller must refuse its claim.
struct ExecutionModel {
    double relative = 0.0;
    double transcendental = 0.0;
    bool active() const { return relative > 0.0 || transcendental > 0.0; }
};
struct ExecutionBound {
    double magnitude = 0.0;
    double slack = 0.0;
};
std::optional<ExecutionBound> executionBound(const MathNode& node, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model, int component = -1,
                                             const std::string& pointVariable = kAmbientPointVar);
std::optional<ExecutionBound> executionBound(const ScalarForm& form, const std::map<std::string, double>& magnitudes,
                                             const ExecutionModel& model);
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
    // constantUnder() is conservative (no binding is exactly zero).
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
