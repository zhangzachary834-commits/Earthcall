#pragma once

// SDF -> WGSL codegen (Milestone 6).
//
// Earthcall's shapes are signed-distance expression TREES (geom::SdfNode), kept as
// plain data precisely so they can be introspected and compiled. Fixed-function
// OpenGL could only ever tessellate them into triangles — an approximation with a
// resolution. WebGPU can evaluate the field itself, so a field renders exactly:
// this turns the tree into a WGSL `sdfEval` function and raymarches it.
//
// THE KEY SPLIT, and the reason this is not just string concatenation:
//   * TREE STRUCTURE becomes generated code — which primitives, which operators.
//   * NUMERIC PARAMETERS become entries in a buffer the shader reads.
// So dragging a radius or a blend slider reuses the same compiled pipeline and
// only rewrites a few floats. Baking the numbers into the source instead would
// recompile a shader every frame you moved a slider.
//
// Consequence: `wgsl` is a complete cache key for the pipeline, and `params` is
// per-instance data. Two spheres of different radii share one pipeline.

#include <cstddef>
#include <string>
#include <vector>

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/RadianceSource.hpp"

namespace geom { struct SdfNode; class FieldNode; }

namespace sdfwgsl {

// Density input is a resolved compiler fact, not a null-pointer convention.
// LegacyField preserves old generic FieldNode behavior; None is explicit absence;
// Authored means densityExpr is the sole D(p,t) authority.
enum class DensityInputKind {
    LegacyField,
    None,
    Authored
};

struct ParameterBlock {
    std::vector<float> values;
    bool ok = true;
    std::string error;
};

// Isolated authored scalar-expression layout. This deliberately reuses the
// production emitter: numeric values become parameter slots, while every choice
// that changes generated WGSL remains in `structure`. That makes it an exact
// structural identity for a Piecewise under this compiler rather than a second
// hand-maintained AST classifier.
struct ScalarExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool ok = true;
    std::string error;
};

// Same structure/value identity for an authored vec3 expression. Kept distinct
// from ScalarExpressionLayout so rho and chi remain independently observable.
struct VectorExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool ok = true;
    std::string error;
};

// Rung 6 source angular factor. It stays distinct from the generic scalar
// layout so alpha can expose whether its authored structure actually reads
// omega; the shader needs that fact to refuse the source singularity without
// suppressing direction-independent alpha.
struct AngularExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool readsOmega = false;
    bool ok = true;
    std::string error;
};

// V3 medium phase is deliberately not source angular alpha. It admits two
// independently named directions with transport semantics:
//   wi = world-space normalized source -> sample propagation direction
//   wo = world-space normalized sample -> receiver/eye propagation direction.
struct PhaseExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool readsWi = false;
    bool readsWo = false;
    bool ok = true;
    std::string error;
};

// Rung 9 receiving-surface response. It intentionally resembles the phase
// layout only at the generic mathematical level: n/wi/wo are admitted under a
// distinct receiver-owned context, and no Material Timeline is invented here.
struct ResponseExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool readsNormal = false;
    bool readsWi = false;
    bool readsWo = false;
    bool ok = true;
    std::string error;
};

struct EmissionExpressionLayout {
    std::string structure;
    std::size_t parameterCount = 0;
    bool readsOmega = false;
    bool ok = true;
    std::string error;
};

struct Program {
    std::string        wgsl;    // full shader source; identical for same-shaped trees
    std::vector<float> params;  // the numbers this instance needs, in emitted order

    // True when the tree contains an implicit f(x,y,z)=0 leaf. Such a field is an
    // ISO-SURFACE VALUE, not a distance: it can be arbitrarily larger than the
    // true distance, so a sphere-tracing step of `f` tunnels straight through the
    // surface. (For x^2+y^2+z^2-0.3 viewed from z=3, f is 8.7 while the surface is
    // 2.45 away — even a halved step overshoots the shape entirely.) When set, the
    // marcher steps by f/|grad f| instead, which is a conservative distance
    // estimate. It costs a gradient per step, so it is only enabled where needed.
    bool needsGradientStep = false;

    // THE REFUSAL. False when the field's authored OntoMath AST contains
    // something this compiler will not turn into WGSL: an operation with no
    // implementation (Raycast, LineIntegral), an op this build does not know, a
    // variable with no binding in a field expression, a piece guarded by a
    // world condition the GPU has no subject to testify about, or a tree that
    // fails its type check.
    //
    // CALLERS MUST CHECK THIS BEFORE CREATING A PIPELINE. It replaces the old
    // behaviour, which was to emit the literal string "0.0" for every
    // unimplemented op — a number that reads as a real answer (for a signed
    // distance, "on the surface"; for a density, "empty here") and cannot be
    // told apart downstream from an authored zero. Refusing to draw is
    // recoverable; drawing the wrong thing silently is not.
    //
    // When !ok, `wgsl` is a comment-only source with no entry points, so any
    // pipeline built from it fails too — the refusal cannot be ignored by
    // accident, only deliberately.
    bool        ok = true;
    std::string error;   // one line, naming the op and why
};

// Compile a field into a raymarching shader plus its parameter block.
// `colorFormatIsSrgb` is unused for now; kept out of the signature deliberately —
// the fragment output convention lives with the pipeline, not the codegen.
//
// An empty/degenerate tree still yields valid WGSL that reports "no surface".
// fieldNode remains only as a LEGACY generic-field compatibility input. New
// participating-medium authorship enters through densityExpr so density never
// needs to borrow source-radiance or generic-field identity by accident.
// colorExpr is optional; if provided, it replaces the uniform base color.
// radianceExpr is optional; if provided, it supplies authored spatial light radiance.
// densityKind is the authority for interpreting this input: LegacyField admits
// the old generic FieldNode fallback, None means no participating medium, and
// Authored makes densityExpr the sole V0 D(p,t) authority.
Program compile(const geom::SdfNode& root,
                const geom::FieldNode* fieldNode = nullptr,
                const OntoMath::Piecewise* colorExpr = nullptr,
                const OntoMath::Piecewise* radianceExpr = nullptr,
                const OntoMath::Piecewise* chromaExpr = nullptr,
                const OntoMath::Piecewise* angularExpr = nullptr,
                const std::vector<Rendering::RadianceSourceBinding>* radianceSources = nullptr,
                const OntoMath::Piecewise* densityExpr = nullptr,
                DensityInputKind densityKind = DensityInputKind::LegacyField,
                const OntoMath::Piecewise* extinctionExpr = nullptr,
                const OntoMath::Piecewise* scatteringExpr = nullptr,
                const OntoMath::Piecewise* volumeChromaExpr = nullptr,
                const OntoMath::Piecewise* phaseExpr = nullptr,
                const OntoMath::Piecewise* emissionExpr = nullptr);

// Re-collect numeric parameter values in the exact order used by compile()
// without assembling the complete WGSL module. This is the value-revision path:
// tree structure is already compiled and only the storage-buffer contents changed.
ParameterBlock collectParams(const geom::SdfNode& root,
                             const geom::FieldNode* fieldNode = nullptr,
                             const OntoMath::Piecewise* colorExpr = nullptr,
                             const OntoMath::Piecewise* radianceExpr = nullptr,
                             const OntoMath::Piecewise* chromaExpr = nullptr,
                             const OntoMath::Piecewise* angularExpr = nullptr,
                             const std::vector<Rendering::RadianceSourceBinding>* radianceSources = nullptr,
                             const OntoMath::Piecewise* densityExpr = nullptr,
                             DensityInputKind densityKind = DensityInputKind::LegacyField,
                             const OntoMath::Piecewise* extinctionExpr = nullptr,
                             const OntoMath::Piecewise* scatteringExpr = nullptr,
                             const OntoMath::Piecewise* volumeChromaExpr = nullptr,
                             const OntoMath::Piecewise* phaseExpr = nullptr,
                             const OntoMath::Piecewise* emissionExpr = nullptr);

// Inspect one authored scalar Piecewise with the SAME emission rules compile()
// uses, but with its parameter numbering starting at zero. Equal structure means
// a value-only edit can refresh the shared parameter block without regenerating
// WGSL; unequal structure means the shader source can have changed. Unsupported
// mathematics refuses here for the same reason it refuses in compile().
// bindTime admits the canonical temporal coordinate "t"; it says nothing
// about which Timeline or Singular owner supplied that coordinate.
ScalarExpressionLayout inspectScalarExpression(const OntoMath::Piecewise* expr,
                                               bool bindTime = false);

// Inspect authored participating-medium density D(p,t)->scalar through the same
// production emitter, but with its OWN temporal coordinate (u.volumeTime.x).
// Absence is a real structural state and means no explicit V0 density channel.
ScalarExpressionLayout inspectDensityExpression(const OntoMath::Piecewise* expr);

// V1 authored participating-medium extinction sigma_t(p,t). Absence is a
// compatibility state, not a refusal: sigma_t falls back to 0.5 * D.
ScalarExpressionLayout inspectExtinctionExpression(const OntoMath::Piecewise* expr);

// V2 authored scattering coefficient sigma_s(p,t). Absence preserves the
// historical coefficient sigma_s = D.
ScalarExpressionLayout inspectScatteringExpression(const OntoMath::Piecewise* expr);

// V2 authored medium chroma C_v(p,t)->vec3. Absence preserves neutral white.
// This is deliberately distinct from source/light chroma.
VectorExpressionLayout inspectVolumeChromaExpression(const OntoMath::Piecewise* expr);

// Inspect authored source chroma chi(p,t)->vec3 with the same production
// emitter. Absent chi has a distinct legacy identity; an authored expression
// must type-check as Vector and unsupported GPU semantics refuse.
VectorExpressionLayout inspectVectorExpression(const OntoMath::Piecewise* expr,
                                               bool bindTime = false);

// Inspect alpha(p,omega,t)->scalar through the production emitter. This is the
// only source-radiance scalar context that admits omega.x/y/z.
AngularExpressionLayout inspectAngularExpression(const OntoMath::Piecewise* expr);

// V3 medium phase Phi(p,wi,wo,t)->scalar. Absence is exact identity Phi=1.
// wi/wo are transport bindings, never aliases of source alpha's omega.
PhaseExpressionLayout inspectPhaseExpression(const OntoMath::Piecewise* expr);

// Rung 9 receiver-owned f_r-like channel. Authored responses are vec3-valued,
// may read n and existing wi/wo directional coordinates, and deliberately do
// not admit t until an honest Material-owned Timeline exists.
ResponseExpressionLayout inspectResponseExpression(const OntoMath::Piecewise* expr);

// Re-collect only response numeric parameters using the same admission/lowering
// path as inspectResponseExpression. This is the future value-only cache path.
ParameterBlock collectResponseParams(const OntoMath::Piecewise* expr);

// V4 medium self-emission E_v(p,omega,t)->vec3. Absence is no self-emitted
// radiance. omega is world-space sample -> eye in an emission-owned context.
EmissionExpressionLayout inspectEmissionExpression(const OntoMath::Piecewise* expr);

// Inspect participating-medium occluder geometry S(p) -> signed distance.
ScalarExpressionLayout inspectOccluderLayout(const geom::SdfNode* root);

// Volumetric V0c: compile one authored density structure into a dedicated
// depth-aware volume-composite shader. This is intentionally separate from
// drawImplicit: a participating medium is not a hard surface and must not own
// frag_depth merely because both paths use OntoMath.
Program compileVolume(const OntoMath::Piecewise* densityExpr,
                      const OntoMath::Piecewise* extinctionExpr = nullptr,
                      const OntoMath::Piecewise* scatteringExpr = nullptr,
                      const OntoMath::Piecewise* volumeChromaExpr = nullptr,
                      const OntoMath::Piecewise* phaseExpr = nullptr,
                      const OntoMath::Piecewise* emissionExpr = nullptr,
                      const geom::SdfNode* occluderSdf = nullptr,
                      const OntoMath::Piecewise* lightRadianceExpr = nullptr,
                      const OntoMath::Piecewise* lightChromaExpr = nullptr,
                      const OntoMath::Piecewise* lightAngularExpr = nullptr);

// V5 world-medium program input. This is compiler transport data only; it does
// not create a Medium ontology. Each entry preserves the independent authored
// channels of one projected FieldNode.
struct VolumeProgramInput {
    const OntoMath::Piecewise* densityExpr = nullptr;
    const OntoMath::Piecewise* extinctionExpr = nullptr;
    const OntoMath::Piecewise* scatteringExpr = nullptr;
    const OntoMath::Piecewise* volumeChromaExpr = nullptr;
    const OntoMath::Piecewise* phaseExpr = nullptr;
    const OntoMath::Piecewise* emissionExpr = nullptr;
    const geom::SdfNode* occluderSdf = nullptr;
    const OntoMath::Piecewise* lightRadianceExpr = nullptr;
    const OntoMath::Piecewise* lightChromaExpr = nullptr;
    const OntoMath::Piecewise* lightAngularExpr = nullptr;
};

// V5 exact overlap baseline. Reuses compileVolume() for every member's evaluator
// semantics, then composes those evaluators into one shared ray integral. The
// caller supplies one VolumeInstanceData record per medium after a union-bounds
// header record; each medium's paramOffset points at its own parameter segment.
Program compileVolumeSet(const std::vector<VolumeProgramInput>& media);

// Value-only companion to compileVolume(). Recollects numeric parameter slots
// without regenerating shader source when structure is unchanged.
ParameterBlock collectVolumeParams(const OntoMath::Piecewise* densityExpr,
                                   const OntoMath::Piecewise* extinctionExpr = nullptr,
                                   const OntoMath::Piecewise* scatteringExpr = nullptr,
                                   const OntoMath::Piecewise* volumeChromaExpr = nullptr,
                                   const OntoMath::Piecewise* phaseExpr = nullptr,
                                   const OntoMath::Piecewise* emissionExpr = nullptr,
                                   const geom::SdfNode* occluderSdf = nullptr,
                                   const OntoMath::Piecewise* lightRadianceExpr = nullptr,
                                   const OntoMath::Piecewise* lightChromaExpr = nullptr,
                                   const OntoMath::Piecewise* lightAngularExpr = nullptr);

} // namespace sdfwgsl
