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

namespace geom { struct SdfNode; }

namespace sdfwgsl {

struct Program {
    std::string        wgsl;    // full shader source; identical for same-shaped trees
    std::vector<float> params;  // the numbers this instance needs, in emitted order
};

// Compile a field into a raymarching shader plus its parameter block.
// `colorFormatIsSrgb` is unused for now; kept out of the signature deliberately —
// the fragment output convention lives with the pipeline, not the codegen.
//
// An empty/degenerate tree still yields valid WGSL that reports "no surface", so
// callers never have to special-case it.
Program compile(const geom::SdfNode& root);

} // namespace sdfwgsl
