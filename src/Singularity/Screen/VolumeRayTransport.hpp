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
                                               bool analyticProposals = false);   // CPU-neutral; see segmentLine

// Runs the general lattice for samples [firstSample, firstSample + count) of the
// span, continuing `state` (updating samplesConsumed / terminated). On the GPU
// this is the existing marcher; in tests a CPU mirror.
using VolumeLatticeRunner = std::function<void(uint32_t firstSample, uint32_t count, VolumeTransportState&)>;

void executeVolumeSchedule(const VolumeSegmentSchedule& schedule, const VolumeLatticeSpan& span,
                           bool outputBudgetZero, VolumeTransportState& state,
                           const VolumeLatticeRunner& lattice);

} // namespace Rendering
