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

#include <string>
#include <vector>

namespace geom { struct SdfNode; class FieldNode; }

namespace sdfwgsl {

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
// An empty/degenerate tree still yields valid WGSL that reports "no surface", so
// callers never have to special-case it.
Program compile(const geom::SdfNode& root, const geom::FieldNode* fieldNode = nullptr);

} // namespace sdfwgsl
