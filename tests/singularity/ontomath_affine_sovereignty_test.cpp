#include "Singularity/OntoMath/LinearAlgebra.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
bool near(float a, float b, float eps = 2e-4f) { return std::fabs(a-b) <= eps; }
bool near3(const glm::vec3& a, const glm::vec3& b, float eps = 2e-4f) {
    return near(a.x,b.x,eps) && near(a.y,b.y,eps) && near(a.z,b.z,eps);
}
bool near4(const glm::mat4& a, const glm::mat4& b, float eps = 2e-4f) {
    for (int c=0;c<4;++c) for (int r=0;r<4;++r)
        if (!near(a[c][r],b[c][r],eps)) return false;
    return true;
}
glm::vec3 legacyEulerOracle(const glm::mat4& m) {
    glm::vec3 scale(glm::length(glm::vec3(m[0])),
                    glm::length(glm::vec3(m[1])),
                    glm::length(glm::vec3(m[2])));
    if (scale.x <= 1e-6f) scale.x = 1.0f;
    if (scale.y <= 1e-6f) scale.y = 1.0f;
    if (scale.z <= 1e-6f) scale.z = 1.0f;
    glm::mat3 basis;
    basis[0] = glm::vec3(m[0]) / scale.x;
    basis[1] = glm::vec3(m[1]) / scale.y;
    basis[2] = glm::vec3(m[2]) / scale.z;
    if (glm::determinant(basis) < 0.0f) basis[0] = -basis[0];
    return glm::degrees(glm::eulerAngles(glm::normalize(glm::quat_cast(basis))));
}
}

int main() {
    const glm::vec3 t(4.0f,-2.0f,7.0f), r(20.0f,-35.0f,70.0f), s(2.0f,3.0f,0.5f);
    auto m=OntoMath::affineTRS(t,r,s); assert(m);
    auto gm=m->toGlmMat4(); assert(gm);
    glm::mat4 ref=glm::translate(glm::mat4(1.0f),t);
    ref=glm::rotate(ref,glm::radians(r.x),glm::vec3(1,0,0));
    ref=glm::rotate(ref,glm::radians(r.y),glm::vec3(0,1,0));
    ref=glm::rotate(ref,glm::radians(r.z),glm::vec3(0,0,1));
    ref=glm::scale(ref,s);
    assert(near4(*gm,ref));

    auto extractedEuler = OntoMath::affineExtractEulerXYZDegrees(*m);
    assert(extractedEuler && near3(*extractedEuler, legacyEulerOracle(ref)));

    const glm::vec3 reflectedScale(-2.0f, 3.0f, 0.5f);
    glm::mat4 reflected = glm::translate(glm::mat4(1.0f), t);
    reflected = glm::rotate(reflected, glm::radians(r.x), glm::vec3(1,0,0));
    reflected = glm::rotate(reflected, glm::radians(r.y), glm::vec3(0,1,0));
    reflected = glm::rotate(reflected, glm::radians(r.z), glm::vec3(0,0,1));
    reflected = glm::scale(reflected, reflectedScale);
    auto reflectedEuler = OntoMath::affineExtractEulerXYZDegrees(
        OntoMath::MatrixValue::fromGlmMat4(reflected));
    assert(reflectedEuler && near3(*reflectedEuler, legacyEulerOracle(reflected)));

    const glm::vec3 p(1.25f,-0.5f,2.0f);
    auto wp=OntoMath::transformPoint(*m,p);
    auto wd=OntoMath::transformDirection(*m,p);
    assert(wp && wd);
    assert(near3(*wp,glm::vec3(ref*glm::vec4(p,1))));
    assert(near3(*wd,glm::vec3(ref*glm::vec4(p,0))));
    assert(!near3(*wp,*wd));

    auto inv=OntoMath::inverseAffine(*m); assert(inv);
    auto rp=OntoMath::transformPoint(*inv,*wp);
    auto rd=OntoMath::transformDirection(*inv,*wd);
    assert(rp && rd && near3(*rp,p) && near3(*rd,p));

    const glm::vec3 rayOriginWorld(8.0f, -3.5f, 4.0f);
    const glm::vec3 rayDirectionWorld = glm::normalize(glm::vec3(-0.8f, 0.25f, -0.4f));
    const auto rayOriginLocal = OntoMath::transformPoint(*inv, rayOriginWorld);
    const auto rayDirectionLocal = OntoMath::transformDirection(*inv, rayDirectionWorld);
    assert(rayOriginLocal && rayDirectionLocal);
    const glm::mat4 legacyInverse = glm::inverse(ref);
    assert(near3(*rayOriginLocal, glm::vec3(legacyInverse * glm::vec4(rayOriginWorld, 1.0f))));
    assert(near3(*rayDirectionLocal, glm::vec3(legacyInverse * glm::vec4(rayDirectionWorld, 0.0f))));

    const glm::vec3 n=glm::normalize(glm::vec3(1,1,0));
    auto wn=OntoMath::transformNormal(*m,n); assert(wn);
    glm::vec3 refn=glm::transpose(glm::inverse(glm::mat3(ref)))*n;
    assert(near3(*wn,refn));

    // Rung-7 collision-normal witness: non-uniform scale must preserve the
    // frozen inverse-transpose oracle, while singular transforms must refuse.
    const glm::vec3 collisionLocalNormal =
        glm::normalize(glm::vec3(0.35f, -0.8f, 0.47f));
    const auto collisionWorldNormal =
        OntoMath::transformNormal(*m, collisionLocalNormal);
    assert(collisionWorldNormal);
    const glm::vec3 collisionOracle =
        glm::transpose(glm::inverse(glm::mat3(ref))) * collisionLocalNormal;
    assert(near3(*collisionWorldNormal, collisionOracle));

    // CollisionDispatcher uses the same authored inverse once per scan direction.
    // Witness repeated world probes against the frozen legacy inverse oracle.
    const auto dispatcherInverse = OntoMath::inverseAffine(*m);
    assert(dispatcherInverse);
    const glm::vec3 dispatcherProbeA(5.5f, -1.0f, 3.25f);
    const glm::vec3 dispatcherProbeB(-2.0f, 4.5f, 1.0f);
    const auto dispatcherLocalA = OntoMath::transformPoint(*dispatcherInverse, dispatcherProbeA);
    const auto dispatcherLocalB = OntoMath::transformPoint(*dispatcherInverse, dispatcherProbeB);
    assert(dispatcherLocalA && dispatcherLocalB);
    assert(near3(*dispatcherLocalA,
                 glm::vec3(legacyInverse * glm::vec4(dispatcherProbeA, 1.0f))));
    assert(near3(*dispatcherLocalB,
                 glm::vec3(legacyInverse * glm::vec4(dispatcherProbeB, 1.0f))));

    // Rung-8 camera witnesses: OntoMath owns view/projection formulas while
    // preserving the frozen GLM execution oracle.
    const glm::vec3 cameraEye(4.0f, 3.0f, 8.0f);
    const glm::vec3 cameraTarget(-1.0f, 0.5f, 0.0f);
    const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
    const auto cameraView = OntoMath::cameraLookAt(cameraEye, cameraTarget, cameraUp);
    assert(cameraView);
    const auto cameraViewGlm = cameraView->toGlmMat4();
    assert(cameraViewGlm);
    const glm::mat4 cameraViewOracle = glm::lookAt(cameraEye, cameraTarget, cameraUp);
    assert(near4(*cameraViewGlm, cameraViewOracle));

    const double cameraFov = glm::radians(61.0);
    const double cameraAspect = 16.0 / 10.0;
    const auto cameraProjection =
        OntoMath::cameraPerspective(cameraFov, cameraAspect, 0.2, 600.0, false);
    assert(cameraProjection);
    const auto cameraProjectionGlm = cameraProjection->toGlmMat4();
    assert(cameraProjectionGlm);
    const glm::mat4 cameraProjectionOracle =
        glm::perspectiveRH_NO(static_cast<float>(cameraFov),
                              static_cast<float>(cameraAspect), 0.2f, 600.0f);
    assert(near4(*cameraProjectionGlm, cameraProjectionOracle));

    // Rung-8 world -> clip witness: composition belongs to OntoMath; compare
    // clip coordinates against the frozen independent GLM projection * view oracle.
    const auto cameraViewProjection =
        OntoMath::matrixMultiply(*cameraProjection, *cameraView);
    assert(cameraViewProjection);
    const auto cameraViewProjectionGlm = cameraViewProjection->toGlmMat4();
    assert(cameraViewProjectionGlm);
    const glm::vec4 cameraWorldPoint(1.25f, -0.75f, 2.5f, 1.0f);
    const glm::vec4 cameraClip =
        *cameraViewProjectionGlm * cameraWorldPoint;
    const glm::vec4 cameraClipOracle =
        cameraProjectionOracle * cameraViewOracle * cameraWorldPoint;
    assert(glm::all(glm::epsilonEqual(cameraClip, cameraClipOracle, 1e-5f)));

    // Rung-8 clip -> world witness: feed NDC derived from the frozen clip
    // oracle back through OntoMath's canonical inverse(P * V) authority.
    assert(std::abs(cameraClipOracle.w) > 1e-6f);
    const glm::vec3 cameraNdc = glm::vec3(cameraClipOracle) / cameraClipOracle.w;
    const auto cameraWorldRoundTrip =
        OntoMath::unprojectNdcPoint(*cameraView, *cameraProjection, cameraNdc);
    assert(cameraWorldRoundTrip);
    assert(near3(*cameraWorldRoundTrip, glm::vec3(cameraWorldPoint), 1e-4f));

    assert(!OntoMath::cameraLookAt(cameraEye, cameraEye, cameraUp));
    assert(!OntoMath::cameraLookAt(cameraEye, cameraTarget,
                                   glm::normalize(cameraTarget - cameraEye)));
    assert(!OntoMath::cameraPerspective(cameraFov, 0.0, 0.2, 600.0, false));
    assert(!OntoMath::cameraPerspective(cameraFov, cameraAspect, 1.0, 0.5, false));

    // Rung-7 picking witness: OntoMath projective unprojection must preserve
    // the frozen legacy inverse(P * V) + homogeneous-divide formula.
    const glm::mat4 pickView = glm::lookAt(
        glm::vec3(3.0f, 2.0f, 7.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0,1,0));
    const glm::mat4 pickProjection =
        glm::perspective(glm::radians(58.0f), 16.0f / 9.0f, 0.1f, 250.0f);
    const auto authoredPickView = OntoMath::MatrixValue::fromGlmMat4(pickView);
    const auto authoredPickProjection = OntoMath::MatrixValue::fromGlmMat4(pickProjection);
    const glm::vec3 pickNdc(0.37f, -0.22f, -1.0f);
    const auto pickWorld =
        OntoMath::unprojectNdcPoint(authoredPickView, authoredPickProjection, pickNdc);
    assert(pickWorld);
    glm::vec4 pickOracle =
        glm::inverse(pickProjection * pickView) * glm::vec4(pickNdc, 1.0f);
    assert(std::abs(pickOracle.w) > 1e-6f);
    pickOracle /= pickOracle.w;
    assert(near3(*pickWorld, glm::vec3(pickOracle)));

    const auto singularProjection =
        OntoMath::MatrixValue::fromGlmMat4(glm::mat4(0.0f));
    assert(!OntoMath::unprojectNdcPoint(
        authoredPickView, singularProjection, glm::vec3(0.0f)));

    // Preserve ObjectEvents raw homogeneous hover-origin convention.
    const auto hoverVp = OntoMath::matrixMultiply(authoredPickProjection, authoredPickView);
    assert(hoverVp);
    const auto hoverInv = OntoMath::matrixInverse(*hoverVp);
    assert(hoverInv);
    const auto hoverGlm = hoverInv->toGlmMat4();
    assert(hoverGlm);
    const glm::mat4 hoverOracle = glm::inverse(pickProjection * pickView);
    const glm::vec4 hoverH(0,0,0,1);
    assert(near3(glm::vec3(*hoverGlm * hoverH), glm::vec3(hoverOracle * hoverH)));

        auto noT=OntoMath::affineTRS(glm::vec3(0),r,s); assert(noT);
    auto noTn=OntoMath::transformNormal(*noT,n); assert(noTn && near3(*wn,*noTn));

    auto rz=OntoMath::affineAxisAngle(glm::vec3(0,0,1),glm::radians(90.0)); assert(rz);
    auto x=OntoMath::transformDirection(*rz,glm::vec3(1,0,0));
    assert(x && near3(*x,glm::vec3(0,1,0)));

    assert(!OntoMath::affineAxisAngle(glm::vec3(0),1.0));
    auto singular=OntoMath::affineScale(glm::vec3(1,0,2)); assert(singular);
    assert(!OntoMath::inverseAffine(*singular));
    assert(!OntoMath::transformNormal(*singular,n));
    auto nonAffine=OntoMath::affineIdentity(); assert(nonAffine);
    nonAffine->at(3,2)=0.25;
    assert(!OntoMath::inverseAffine(*nonAffine));
    assert(!OntoMath::transformPoint(*nonAffine,p));

    std::puts("ontomath_affine_sovereignty_test: PASS");
    return 0;
}
