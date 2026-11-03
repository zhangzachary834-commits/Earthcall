// Regression witness for SDF render-cache invalidation granularity.
//
// Broad geometry revision still invalidates collision/tessellation derived state,
// while the SDF renderer has separate witnesses for shader structure and numeric
// parameters. Value-only edits must not masquerade as topology changes.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <cstdio>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}
}

int main() {
    std::printf("Running SDF revision split test...\n");

    Object object("sdf-revision-split");

    auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.8f));
    auto torus = geom::SdfNode::leaf(geom::SdfPrim::Torus, glm::vec3(1.1f, 0.2f, 0.0f));
    auto field = geom::SdfNode::binary(geom::SdfOp::SmoothUnion, sphere, torus, 0.2f);
    object.setFieldShape(field, glm::vec3(3.0f));

    const auto broad0 = object.getFieldRevision();
    const auto structure0 = object.getSdfStructureRevision();
    const auto params0 = object.getSdfParameterRevision();

    object.setMorphParam(0.7f);
    check(object.getFieldRevision() == broad0 + 1,
          "morph value edit invalidates broad geometry caches");
    check(object.getSdfStructureRevision() == structure0,
          "morph value edit does not invalidate shader structure");
    check(object.getSdfParameterRevision() == params0 + 1,
          "morph value edit invalidates SDF parameters");

    const auto broad1 = object.getFieldRevision();
    const auto structure1 = object.getSdfStructureRevision();
    const auto params1 = object.getSdfParameterRevision();

    object.setFieldOperandBOffset(glm::vec3(0.25f, 0.0f, 0.0f));
    check(object.getFieldRevision() == broad1 + 1,
          "operand offset invalidates broad geometry caches");
    check(object.getSdfStructureRevision() == structure1,
          "operand offset does not invalidate shader structure");
    check(object.getSdfParameterRevision() == params1 + 1,
          "operand offset invalidates SDF parameters");

    const auto broad2 = object.getFieldRevision();
    const auto structure2 = object.getSdfStructureRevision();
    const auto params2 = object.getSdfParameterRevision();

    object.setFieldCellSize(0.25f);
    check(object.getFieldRevision() == broad2 + 1,
          "sampling-resolution edit invalidates broad geometry caches");
    check(object.getSdfStructureRevision() == structure2,
          "sampling-resolution edit does not invalidate shader structure");
    check(object.getSdfParameterRevision() == params2,
          "sampling-resolution edit does not invalidate SDF parameters");

    auto replacement = geom::SdfNode::binary(
        geom::SdfOp::Union,
        geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f)),
        geom::SdfNode::leaf(geom::SdfPrim::Box, glm::vec3(0.4f)),
        0.0f);
    const auto structure3 = object.getSdfStructureRevision();
    const auto params3 = object.getSdfParameterRevision();
    object.setFieldShape(replacement, glm::vec3(2.0f));

    check(object.getSdfStructureRevision() == structure3 + 1,
          "field replacement invalidates shader structure");
    check(object.getSdfParameterRevision() == params3 + 1,
          "field replacement invalidates SDF parameters");

    if (failures) {
        std::printf("sdf_revision_split_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_revision_split_test: PASS\n");
    return 0;
}
