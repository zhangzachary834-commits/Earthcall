#pragma once

// GLU compatibility shim (OPENGL_MIGRATION_PLAN.md, Milestone 3: Retire GLU).
//
// GLU is the OpenGL Utility Library. It is deprecated alongside legacy OpenGL and
// has no successor in Metal or WebGPU, so every glu* call has to go before a
// backend swap is possible. These are drop-in replacements for the only GLU
// functions Earthcall actually used: the screen<->world projection helpers and
// gluLookAt.
//
// The signatures deliberately mirror the GLU originals — GLdouble[16] matrices,
// GLint[4] viewport — because the camera stores its matrices in exactly that
// shape (Game.hpp: GLdouble modelview[16] etc., captured from the GL stack via
// glGetDoublev). That makes the call-site migration a rename, not a rewrite.
//
// The math is reimplemented directly (not via glm::project/unProject) so it
// matches gluProject/gluUnProject bit-for-bit in convention: double precision,
// depth range [0,1], and failure signalled only when the clip w is zero — the
// same condition GLU used. Several call sites branch on that bool to cull, so
// preserving exactly when it fails preserves existing behavior.

#include <glm/glm.hpp>

namespace ecgl {

// Replaces gluProject. Maps an object-space point to window coordinates.
// Returns false only when the point is unprojectable (clip w == 0), matching GLU.
bool project(double objX, double objY, double objZ,
             const double model[16], const double proj[16], const int view[4],
             double* winX, double* winY, double* winZ);

// Replaces gluUnProject. Maps a window-space point (winZ in [0,1]) back to
// object space. Returns false only when the inverse is degenerate, matching GLU.
bool unProject(double winX, double winY, double winZ,
               const double model[16], const double proj[16], const int view[4],
               double* objX, double* objY, double* objZ);

// Replaces gluLookAt. Builds a look-at view matrix and multiplies it onto the
// current fixed-function matrix stack, exactly as gluLookAt did. (Fixed-function
// matrix use itself is retired later, in Milestone 5.)
void lookAtMul(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

} // namespace ecgl
