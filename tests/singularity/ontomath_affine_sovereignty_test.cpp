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

    const glm::vec3 n=glm::normalize(glm::vec3(1,1,0));
    auto wn=OntoMath::transformNormal(*m,n); assert(wn);
    glm::vec3 refn=glm::transpose(glm::inverse(glm::mat3(ref)))*n;
    assert(near3(*wn,refn));

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
