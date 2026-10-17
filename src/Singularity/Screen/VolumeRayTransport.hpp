#pragma once

#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

// ============================================================================
// Ray-domain transport — Rung 0/1 of Zach's Ray Transport Canon.
//
// Kernel-derived EXECUTION state for participating-medium transport. Nothing
// here is authored ontology: there is no Medium, RayProgram or Segment being.
// The FieldNode remains the authored Singular; VolumeDensityBinding remains the
// Screen channel's bounded projection of it; OntoMath::restrictToLine owns the
// mathematics of p = o + s·d. This file decides only how much of the accepted
// lattice a camera-ray segment must actually execute.
//
// The accepted lattice (the reference; WebGPU single and fused V5 marchers in
// SdfWgsl.cpp) is midpoint quadrature over each segment:
//     s_i = s_first + i·Δ,   per active medium j at s_i:
//       D_j = max(D(p), 0);  only D_j > 0 participates
//       σ_j = max(σ_t(p, D_j), 1e-6)
//       S_j = C_v(p)·Li·max(σ_s(p, D_j), 0)·Φ + max(E_v(p, ω), 0)
//     T_i = T_{i-1}·exp(−Σσ_j·Δ);  L += ΣS_j·(T_{i-1} − T_i)/Σσ_j
//     legacy cutoff (output budget 0): break once T < 0.01.
// The lattice integrates piecewise-CONSTANT media exactly. So on a segment
// where every participating channel restricted to the ray is exactly constant,
// the n-step product telescopes IN THE REALS to one closed form:
//     T_out = T_in·exp(−σ·Δ·k),   L += S·T_in·(−expm1(−σ·Δ·k))/σ,
// with k = min(n, first step at which the cutoff fires). That equality is with
// the lattice itself, not with some other "continuous" integral, so it keeps
// Zach's full-lattice reference decision intact. It is NOT byte identity: the
// lattice multiplies k f32 exponentials, this evaluates one double expm1.
//
// EXACTNESS ON A RAY is proved, not measured: a channel is constant on a ray
// only when every s-coefficient of degree ≥ 1 vanishes EXACTLY (each term
// carries a factor bound to exactly 0.0 — a zero direction component, say).
// See OntoMath::LineRestrictedForm::exactDegreeOn.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
// Plan/contract: docs/plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md
// ============================================================================
namespace Rendering {

// One authored channel of one medium, restricted to the line. AUTHORED-REVISION
// lifetime: derived only from the channel's Piecewise content (identified by
// its per-channel revision) — never from camera, ray, medium origin, extent or
// clock, all of which enter later as numeric bindings.
struct VolumeRayChannelForm {
    bool present = false;                       // authored expression exists
    const OntoMath::Piecewise* expr = nullptr;  // identity observed at lowering (not a validity proof)
    uint64_t revision = 0;                      // per-channel authored revision observed at lowering
    bool vectorValued = false;
    OntoMath::LineRestriction scalar;
    OntoMath::VectorLineRestriction vector;
    // Every variable the production emitter reads: the Piecewise input
    // variable, then each MathNode piece in order up to and including the first
    // unbounded one (emitPiecewise returns there). The WGSL flags
    // VOLUME_PHASE_READS_WI / VOLUME_EMISSION_READS_OMEGA are computed that
    // way, and they change transport (phase = 0 without a source).
    std::set<std::string> emitterReads;
    std::string refusal;                        // empty iff admitted (or absent)
    // Rung 2: the piecewise-algebraic restriction (a superset of the polynomial
    // one above). Same lifetime and key; its own refusal reason.
    OntoMath::LineAlgebraicRestriction algebraic;

    bool admitted() const { return !present || refusal.empty(); }
    bool algebraicAdmitted() const { return !present || algebraic.form.has_value(); }
    bool reads(const std::string& prefix) const;
};

struct VolumeRayChannelForms {
    std::string producerId;
    VolumeRayChannelForm density, extinction, scattering, chroma, phase, emission;
    bool occluderPresent = false;   // matters only with an incident source
};

// Lower every channel of one medium. Pure; the cache below decides when.
VolumeRayChannelForms lowerVolumeRayChannels(const VolumeDensityBinding& medium);

// AUTHORED-REVISION cache, keyed by the medium's stable producer identity and,
// per channel, by (revision, expression identity). Dependencies and writers:
//   - FieldNode per-channel authored revisions (bridges, applyJson, MCP
//     author_volume; verifier heals a forgotten bump) — any change relowers
//     THAT channel only;
//   - revision 0 for a present expression means "unrevisioned": never retained,
//     relowered on every request (counted in uncacheableLowerings);
//   - empty producerId: nothing to key on, never retained;
//   - medium origin/half-extent, camera, ray, clock: NOT dependencies (numeric
//     bindings at the ray tier), so camera motion performs zero lowering.
// Lifetime: until retainOnly()/clear(). No device state. Stale/unknown entries
// can only cost a relowering, never authorize a closed form on their own: the
// per-ray planner re-proves constancy from the bindings every time.
class VolumeRayFormCache {
public:
    struct Stats {
        uint64_t requests = 0;
        uint64_t channelLowerings = 0;      // channels actually (re)lowered
        uint64_t uncacheableLowerings = 0;  // subset forced by revision 0 / no id
    };
    const VolumeRayChannelForms& formsFor(const VolumeDensityBinding& medium);
    void retainOnly(const std::set<std::string>& producerIds);
    void clear() { _entries.clear(); }
    std::size_t size() const { return _entries.size(); }
    const Stats& stats() const { return _stats; }

private:
    std::map<std::string, VolumeRayChannelForms> _entries;
    VolumeRayChannelForms _scratch;   // uncacheable answers live here until the next call
    Stats _stats;
};

// --- RAY / SEGMENT tier -------------------------------------------------------

// One medium active on the segment, with the numeric premises the ray tier
// binds: its origin (p = worldP − origin, the marchers' convention) and its own
// clock (each medium owns its temporal coordinate; never the source's).
struct VolumeSegmentMember {
    const VolumeRayChannelForms* forms = nullptr;
    glm::dvec3 origin{0.0};
    double temporalCoordinate = 0.0;
};

struct VolumeRayInput {
    glm::dvec3 origin{0.0};      // world eye position (u.eyePos)
    glm::dvec3 direction{0.0};   // world ray direction as the marcher uses it
    bool incidentSourceActive = false;   // u.incidentSource.w > 0.5
};

// The segment's accepted lattice: samples at firstSample + i·step, i < count.
struct VolumeLatticeSpan {
    double firstSample = 0.0;
    double step = 0.0;
    uint32_t count = 0;
};

enum class VolumeSegmentStrategy : uint8_t {
    GeneralLattice,      // fallback: the existing exact sample loop must run
    ExactEmpty,          // every member's D is exactly constant and ≤ 0
    ConstantClosedForm,  // every participating channel exactly constant, Σσ > 0
};
const char* volumeSegmentStrategyName(VolumeSegmentStrategy s);

struct VolumeSegmentPlan {
    VolumeSegmentStrategy strategy = VolumeSegmentStrategy::GeneralLattice;
    std::string reason;          // why the lattice is required (or what was proved)
    double totalExtinction = 0.0;   // Σ σ_j over participating members
    glm::dvec3 totalSource{0.0};    // Σ S_j over participating members
    uint32_t participatingMembers = 0;
};

// Prove (or refuse) that the segment is exactly constant on THIS ray. Any
// refusal returns GeneralLattice with a reason; the caller runs the lattice.
// The caller owns segment membership (V5 box events) and must call this only
// for media whose production program compiled — emitter refusals are not
// re-derived here.
VolumeSegmentPlan planVolumeRaySegment(const std::vector<VolumeSegmentMember>& members,
                                       const VolumeRayInput& ray,
                                       const VolumeLatticeSpan& span);

struct VolumeTransportState {
    double transmittance = 1.0;
    glm::dvec3 radiance{0.0};
    uint32_t samplesConsumed = 0;
    bool terminated = false;   // legacy cutoff fired inside this segment
};

// Execute a non-lattice plan over the span. Returns false (state untouched) for
// GeneralLattice. outputBudgetZero mirrors u.volumeOutputError.y == 0.
bool executeVolumeSegment(const VolumeSegmentPlan& plan, const VolumeLatticeSpan& span,
                          bool outputBudgetZero, VolumeTransportState& state);

// Transmittance-only rung for degree ≤ 1 media: the midpoint lattice sums an
// affine σ exactly, Σσ(s_i)·Δ = n·Δ·σ(s_mid), provided no clamp is active at
// any sample (D > 0 and σ_t > 1e-6 at both end samples suffices for affine
// functions) and the legacy cutoff cannot fire. Radiance under affine σ has no
// lattice closed form here and still needs the lattice. nullopt = use lattice.
std::optional<double> affineSegmentTransmittance(const std::vector<VolumeSegmentMember>& members,
                                                 const VolumeRayInput& ray,
                                                 const VolumeLatticeSpan& span,
                                                 double transmittanceIn,
                                                 bool outputBudgetZero,
                                                 std::string* reason = nullptr);

// --- Rung 2: per-ray segmentation of one V5 segment ---------------------------
//
// OntoMath::segmentLine partitions the segment's sample interval by every
// relevant switching function of every admitted channel of every member
// (abs/min/max/clamp/sqrt/Piecewise bounds — radial falloffs' ball boundaries
// are exact quadratic roots). Each piece is then classified with the same
// proofs as Rung 1, but on the reduced branch:
//   exact-empty      every member's density range over the piece is ≤ 0
//   constant         every participating channel's surviving branch is exactly
//                    constant on this line (no source, direction gates open)
//   general-lattice  anything else, including the narrow uncertain pieces
//                    around possible switches
// The accepted lattice is never re-spaced: each sample is assigned to the piece
// containing it, and consecutive samples with the same plan form one run.
// Pieces/runs are RAY-lifetime values, never cached.
struct VolumeScheduleRun {
    uint32_t firstSample = 0;
    uint32_t sampleCount = 0;
    VolumeSegmentPlan plan;
};

struct VolumeSegmentSchedule {
    std::vector<VolumeScheduleRun> runs;
    std::string reason;   // set when the whole segment stays on the lattice
    uint32_t pieces = 0;
    uint32_t rangeEvaluations = 0;
    bool budgetExhausted = false;
    uint32_t emptySamples = 0, constantSamples = 0, generalSamples = 0;
};

// resolutionInSteps: uncertain pieces are refined until no wider than this
// fraction of the lattice step (a coarser value costs fewer proofs and leaves
// at most a sample or two near each switch on the lattice).
VolumeSegmentSchedule scheduleVolumeRaySegment(const std::vector<VolumeSegmentMember>& members,
                                               const VolumeRayInput& ray,
                                               const VolumeLatticeSpan& span,
                                               double resolutionInSteps = 0.125,
                                               bool analyticProposals = true);   // measured win at 28 media; see segmentLine

// Runs the general lattice for samples [firstSample, firstSample + count) of the
// span, continuing `state` (updating samplesConsumed / terminated). On the GPU
// this is the existing marcher; in tests a CPU mirror.
using VolumeLatticeRunner = std::function<void(uint32_t firstSample, uint32_t count, VolumeTransportState&)>;

void executeVolumeSchedule(const VolumeSegmentSchedule& schedule, const VolumeLatticeSpan& span,
                           bool outputBudgetZero, VolumeTransportState& state,
                           const VolumeLatticeRunner& lattice);

// --- Rung 2f prototype: Lipschitz-certified support tracing --------------------
//
// The GPU-shaped route to Rung 2's emptiness proofs (sphere tracing, Hart 1996).
// A KILLER is a switch sign that by itself forces a member's density range over
// the chord to be ≤ 0 (e.g. a curtain clamp saturating at 0 inside a product).
// For each killer, L = sup |g′| over the chord (interval derivative). Walking the
// accepted lattice, a not-yet-certified sample s evaluates one point enclosure of
// g; a margin m > 0 away from the killing side certifies emptiness up to
// s + m/L (mean value theorem). A sample is empty only when EVERY member's density
// is certified there. Cost proxy = point evaluations (what a shader would pay).
// Prototype only: nothing consumes it; witnesses compare it with Rung 2 and the
// zero-proof grid.
struct VolumeSupportTrace {
    std::vector<char> emptySample;
    uint32_t killers = 0;
    uint32_t setupEvaluations = 0;   // killer identification + Lipschitz bounds
    uint32_t pointEvaluations = 0;   // per-step g(s) enclosures
    uint32_t certifiedSamples = 0;
    std::string reason;              // set when nothing could be traced
};
VolumeSupportTrace traceVolumeSupport(const std::vector<VolumeSegmentMember>& members,
                                      const VolumeRayInput& ray, const VolumeLatticeSpan& span);

// --- Rung 2f-native groundwork: authored-tier support certificates ------------
//
// What a shader can carry. A certificate is a killer that holds over the WHOLE
// medium box at EVERY time (t is bound as the unbounded interval, so a clock-
// dependent kill simply refuses), with a SPATIAL Lipschitz constant
// L ≥ sup‖∇g‖ over the box (OntoMath::lipschitzBoundOverBox). On any ray inside
// the box, |dg/ds| ≤ L·|d|, so one point value of g certifies emptiness for
// margin/(L·|d|) ahead — no per-ray interval work at all.
//
// Lifetime: AUTHORED-REVISION × BOX × CLOCK BUCKET. Derived from the density channel's
// algebraic form (same key as VolumeRayFormCache) and the half-extent; invalid
// when either changes. Independent of camera, ray, origin and clock. `source`
// points into the authored Piecewise for a future emitter; it is valid exactly
// as long as the form it came from.
struct VolumeSupportCertificate {
    std::size_t switchIndex = 0;
    int8_t sign = 0;               // the killing side of g
    double spatialLipschitz = 0.0;
    uint32_t role = 0;             // LineSwitch role (what g is in terms of source)
    const OntoMath::MathNode* sourceA = nullptr;
    const OntoMath::MathNode* sourceB = nullptr;
    double offset = 0.0;           // clamp lo/hi for roles 2/3
    // δ, the margin the native lane must clear: τ + 2·slack(g), with slack(g)
    // OntoMath's execution slack of the switch function from its AUTHORED
    // operands over the box for |t| ≤ clockMagnitude (Codex P1, 2026-10-17:
    // never from the simplified form, which may have cancelled). Twice, because
    // the shader evaluates g once in the killer and once inside the density.
    double deviceSlack = 0.0;
};
// DECLARED device tolerance τ for every claim that replaces or skips executed
// arithmetic (OntoMath::executionBound's model), not a theorem: τ = 2⁻¹⁰ per
// magnitude unit — room for ~2¹⁴ f32 roundings in any grouping, or two
// WGSL-accuracy sin/cos errors (2⁻¹¹ absolute). Metal Fast mode's complete
// execution enclosure is still open (docs/audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md),
// so the native lane is opt-in (ScreenChannel volumeSupportCertificatesEnabled)
// and its pixels are witnessed, not proved, per backend.
inline constexpr double kVolumeSupportDeviceTolerance = 1.0 / 1024.0;
// The execution model every replacing or skipping claim is decided under
// (OntoMath::executionBound): each arithmetic operation, literal and
// coordinate read errs by ≤ 2⁻¹⁶ of its magnitude bound (256 f32 ulps, room for
// Fast-mode reassociation, fusion and approximate division), and each
// sin/cos/exp/noise evaluation by ≤ 2⁻¹⁰ absolute (the device tolerance).
inline constexpr OntoMath::ExecutionModel kVolumeExecutionModel{1.0 / 65536.0, kVolumeSupportDeviceTolerance};
// A closed form replaces execution only when execution cannot drift far from
// the algebra: an exactly-constant channel value c is admitted only while its
// authored tree's execution slack is ≤ κ·τ·(1 + |c|), κ = 16. Beyond that the
// algebra may have cancelled what the device computes, and the lattice runs.
inline constexpr double kExecutionCancellationGuard = 16.0;
// Clock-magnitude bucket for certificates: the smallest power of two ≥ |t|
// (at least 1). Device error in sin(k·t) grows with |t|, so a certificate's
// slack is derived for |t| ≤ bucket and re-derived when the clock crosses it.
double volumeClockMagnitudeBucket(double t);
struct VolumeSupportCertificates {
    std::vector<VolumeSupportCertificate> killers;
    std::string refusal;
};
VolumeSupportCertificates deriveVolumeSupportCertificates(const VolumeRayChannelForms& forms,
                                                          const glm::dvec3& halfExtent,
                                                          double clockMagnitude = 1.0);

// The shader-shaped tracer: certificates only, one point evaluation of g per
// not-yet-certified sample. members[i] pairs with certificates[i].
VolumeSupportTrace traceVolumeSupportCertified(const std::vector<VolumeSegmentMember>& members,
                                               const std::vector<const VolumeSupportCertificates*>& certificates,
                                               const VolumeRayInput& ray, const VolumeLatticeSpan& span);

} // namespace Rendering
