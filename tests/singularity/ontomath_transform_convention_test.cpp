#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Perspective/PersonPerspective.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

bool nearVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}

bool nearMat4(const glm::mat4& a, const glm::mat4& b, float eps = 1e-4f) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!nearf(a[c][r], b[c][r], eps)) return false;
        }
    }
    return true;
}

glm::vec3 homogenizedPoint(const glm::vec4& p) {
    assert(std::fabs(p.w) > 1e-6f);
    return glm::vec3(p) / p.w;
}

} // namespace

int main() {
    // Rung 0 freezes the transform conventions that exist BEFORE OntoMath owns
    // matrices. This is a witness, not a new source of mathematical meaning.
    //
    // GLM/Earthcall convention currently observable at runtime:
    //   * matrices are column-major and indexed [column][row];
    //   * translation lives in column 3;
    //   * column vectors are transformed as M * v;
    //   * Object Euler recomposition is T * Rx * Ry * Rz * S.

    const glm::vec3 translation(4.0f, -2.0f, 7.0f);
    const glm::vec3 scale(2.0f, 3.0f, 0.5f);
    const glm::vec3 eulerDeg(20.0f, -35.0f, 70.0f);

    glm::mat4 base = glm::translate(glm::mat4(1.0f), translation);
    base = glm::scale(base, scale);

    Object object("ontomath-rung0-transform-witness");
    object.setTransform(base);
    object.setRotationEulerDegrees(eulerDeg);

    glm::mat4 expected = glm::translate(glm::mat4(1.0f), translation);
    expected = glm::rotate(expected, glm::radians(eulerDeg.x), glm::vec3(1, 0, 0));
    expected = glm::rotate(expected, glm::radians(eulerDeg.y), glm::vec3(0, 1, 0));
    expected = glm::rotate(expected, glm::radians(eulerDeg.z), glm::vec3(0, 0, 1));
    expected = glm::scale(expected, scale);

    assert(nearMat4(object.getTransform(), expected));
    assert(nearf(object.getTransform()[3][0], translation.x));
    assert(nearf(object.getTransform()[3][1], translation.y));
    assert(nearf(object.getTransform()[3][2], translation.z));
    assert(nearf(object.getTransform()[3][3], 1.0f));


    // Rung 6 production-seam witness: repeat authored rotation on an already
    // rotated, non-uniformly-scaled Object. ObjectMotion must preserve the
    // existing translation and extracted scale while OntoMath owns the new
    // T * Rx * Ry * Rz * S recomposition.
    const glm::vec3 secondEulerDeg(-42.0f, 15.0f, 103.0f);
    object.setRotationEulerDegrees(secondEulerDeg);

    glm::mat4 secondExpected = glm::translate(glm::mat4(1.0f), translation);
    secondExpected = glm::rotate(secondExpected, glm::radians(secondEulerDeg.x), glm::vec3(1, 0, 0));
    secondExpected = glm::rotate(secondExpected, glm::radians(secondEulerDeg.y), glm::vec3(0, 1, 0));
    secondExpected = glm::rotate(secondExpected, glm::radians(secondEulerDeg.z), glm::vec3(0, 0, 1));
    secondExpected = glm::scale(secondExpected, scale);

    assert(nearMat4(object.getTransform(), secondExpected));
    assert(nearf(glm::length(glm::vec3(object.getTransform()[0])), scale.x));
    assert(nearf(glm::length(glm::vec3(object.getTransform()[1])), scale.y));
    assert(nearf(glm::length(glm::vec3(object.getTransform()[2])), scale.z));

    // Point and direction are not the same homogeneous thing. Translation
    // affects w=1 points and must not affect w=0 directions.
    const glm::vec3 localPoint(1.25f, -0.5f, 2.0f);
    const glm::vec3 localDirection(1.25f, -0.5f, 2.0f);
    const glm::mat4 affine = object.getTransform();

    const glm::vec3 worldPoint =
        glm::vec3(affine * glm::vec4(localPoint, 1.0f));
    const glm::vec3 worldDirection =
        glm::vec3(affine * glm::vec4(localDirection, 0.0f));

    const glm::mat3 linear(affine);
    assert(nearVec3(worldDirection, linear * localDirection));
    assert(nearVec3(worldPoint, linear * localPoint + translation));

    // Current world <-> local convention is inverse(M) applied to the same
    // homogeneous kind. This is the convention used by picking/raycast paths.
    const glm::mat4 inv = glm::inverse(affine);
    assert(nearVec3(glm::vec3(inv * glm::vec4(worldPoint, 1.0f)), localPoint));
    assert(nearVec3(glm::vec3(inv * glm::vec4(worldDirection, 0.0f)), localDirection));

    // Normals use inverse-transpose of the linear component. The witness uses
    // a non-uniform scale so a naive L*n is observably wrong.
    const glm::vec3 localNormal = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
    const glm::vec3 localTangent = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));
    const glm::vec3 worldTangent = linear * localTangent;
    const glm::vec3 worldNormal =
        glm::normalize(glm::transpose(glm::inverse(linear)) * localNormal);
    const glm::vec3 naiveNormal = glm::normalize(linear * localNormal);

    assert(std::fabs(glm::dot(worldNormal, worldTangent)) < 1e-4f);
    assert(std::fabs(glm::dot(naiveNormal, worldTangent)) > 1e-2f);

    // Rung 6 Body placement witness: the default avatar's authored offsets
    // remain exactly the legacy placements while OntoMath now owns translation
    // construction. Sample asymmetric and depth-bearing parts so axis/sign
    // mistakes cannot hide behind symmetry.
    Body avatar = Body::createBasicAvatar("ontomath-rung6-body");
    const BodyPart* leftShoulder = avatar.getBodyPart("LeftShoulder");
    const BodyPart* rightFoot = avatar.getBodyPart("RightFoot");
    assert(leftShoulder && rightFoot);
    assert(nearVec3(glm::vec3(leftShoulder->getTransform()[3]), glm::vec3(-0.35f, 0.6f, 0.0f)));
    assert(nearVec3(glm::vec3(rightFoot->getTransform()[3]), glm::vec3(0.15f, -1.15f, 0.1f)));

    // Rung 6 BodyPart parity: production now asks OntoMath to compose
    // dimensions and nested local offsets; these GLM expressions are the
    // frozen legacy oracle only.
    const glm::vec3 partDims(0.4f, 1.25f, 0.7f);
    glm::mat4 partWorld = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -0.5f, 1.25f));
    partWorld = glm::rotate(partWorld, glm::radians(27.0f), glm::vec3(0, 1, 0));
    BodyPart part("ontomath-rung6-bodypart", BodyPart::Type::Arm,
                  ObjectTypes::ShapeKind::Cube, partDims, partWorld);
    assert(nearMat4(part.getRaycastTransform(),
                    partWorld * glm::scale(glm::mat4(1.0f), partDims)));

    glm::mat4 localOffset =
        glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, 0.35f, 0.15f));
    Object* nested = part.addSubObject(ObjectTypes::ShapeKind::Cube, localOffset);
    assert(nested);
    assert(nearMat4(nested->getTransform(), partWorld * localOffset));

    // PersonPerspective currently defines its view by glm::lookAt and its
    // standalone projection by glm::perspective. Pin that separately from the
    // active renderer's backend-selected ZO/NO projection convention below.
    PersonPerspective perspective("ontomath-rung0-camera",
                                  PersonPerspective::PerspectiveType::FreeCamera);
    const auto& viewState = perspective.getViewState();
    const auto& settings = perspective.getSettings();
    const float aspect = 16.0f / 9.0f;

    const glm::mat4 expectedView =
        glm::lookAt(viewState.position, viewState.target, viewState.up);
    const glm::mat4 expectedProjection =
        glm::perspective(glm::radians(settings.fov), aspect,
                         settings.nearPlane, settings.farPlane);

    assert(nearMat4(perspective.getViewMatrix(), expectedView));
    assert(nearMat4(perspective.getProjectionMatrix(aspect), expectedProjection));

    // The live Renderer boundary explicitly chooses clip-depth convention:
    // OpenGL-style NO => near=-1, far=+1; WebGPU-style ZO => near=0, far=+1.
    const float n = 0.1f, f = 100.0f, top = 0.1f, right = 0.15f;
    const glm::mat4 projNO = glm::frustumNO(-right, right, -top, top, n, f);
    const glm::mat4 projZO = glm::frustumZO(-right, right, -top, top, n, f);

    const glm::vec4 nearNO = projNO * glm::vec4(0, 0, -n, 1);
    const glm::vec4 farNO  = projNO * glm::vec4(0, 0, -f, 1);
    const glm::vec4 nearZO = projZO * glm::vec4(0, 0, -n, 1);
    const glm::vec4 farZO  = projZO * glm::vec4(0, 0, -f, 1);

    assert(nearf(nearNO.z / nearNO.w, -1.0f));
    assert(nearf(farNO.z / farNO.w, 1.0f));
    assert(nearf(nearZO.z / nearZO.w, 0.0f));
    assert(nearf(farZO.z / farZO.w, 1.0f));

    // View-projection unprojection follows inverse(P*V), the convention used
    // by current cursor / interaction code.
    const glm::mat4 vp = projNO * expectedView;
    const glm::vec3 sampleWorld(0.25f, -0.4f, -2.0f);
    const glm::vec4 clip = vp * glm::vec4(sampleWorld, 1.0f);
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const glm::vec3 recovered =
        homogenizedPoint(glm::inverse(vp) * glm::vec4(ndc, 1.0f));
    assert(nearVec3(recovered, sampleWorld, 2e-4f));

    std::puts("ontomath_transform_convention_test: PASS");
    return 0;
}
