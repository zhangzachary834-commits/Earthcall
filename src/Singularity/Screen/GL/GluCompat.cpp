#include "Singularity/Screen/GL/GluCompat.hpp"

// emcc defines __EMSCRIPTEN__, not EMSCRIPTEN (a build-system environment
// variable, not a preprocessor macro) -- see AUDIT_2026-08-10.md §2.6. In
// practice __APPLE__ is never defined for the emscripten target regardless
// (it targets wasm32-unknown-emscripten, not a Darwin triple), so this
// particular guard was latently correct; fixed here anyway so it is not the
// next person's landmine.
#if defined(NO_OPENGL_RENDERER)
// No OpenGL available; provide dummy declarations for functions if needed or omit gl.h
#elif defined(__APPLE__) && !defined(__EMSCRIPTEN__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace ecgl {

// gluProject, reimplemented: clip = Proj * Model * obj, perspective divide,
// then the viewport transform. Double precision throughout to match GLU.
bool project(double objX, double objY, double objZ,
             const double model[16], const double proj[16], const int view[4],
             double* winX, double* winY, double* winZ) {
    glm::dmat4 M = glm::make_mat4(model);
    glm::dmat4 P = glm::make_mat4(proj);
    glm::dvec4 clip = P * (M * glm::dvec4(objX, objY, objZ, 1.0));
    if (clip.w == 0.0 || !std::isfinite(clip.w)) return false;

    clip /= clip.w;                       // NDC in [-1, 1]
    *winX = view[0] + view[2] * (clip.x * 0.5 + 0.5);
    *winY = view[1] + view[3] * (clip.y * 0.5 + 0.5);
    *winZ = clip.z * 0.5 + 0.5;           // depth range [0, 1]
    return true;
}

// gluUnProject, reimplemented: invert (Proj * Model), map the window point to
// NDC, transform back, perspective divide.
bool unProject(double winX, double winY, double winZ,
               const double model[16], const double proj[16], const int view[4],
               double* objX, double* objY, double* objZ) {
    glm::dmat4 M = glm::make_mat4(model);
    glm::dmat4 P = glm::make_mat4(proj);
    glm::dmat4 inv = glm::inverse(P * M);

    glm::dvec4 ndc(
        (winX - view[0]) / view[2] * 2.0 - 1.0,
        (winY - view[1]) / view[3] * 2.0 - 1.0,
        winZ * 2.0 - 1.0,
        1.0);
    glm::dvec4 obj = inv * ndc;
    if (obj.w == 0.0 || !std::isfinite(obj.w)) return false;

    obj /= obj.w;
    *objX = obj.x;
    *objY = obj.y;
    *objZ = obj.z;
    return true;
}

void lookAtMul(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
    glm::mat4 view = glm::lookAt(eye, center, up);
#ifndef NO_OPENGL_RENDERER
    glMultMatrixf(glm::value_ptr(view));
#else
    (void)view;
#endif
}

} // namespace ecgl
