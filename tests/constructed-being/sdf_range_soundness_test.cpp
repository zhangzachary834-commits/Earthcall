// Regression witness for conservative SDF interval bounds.
//
// evalRange() is not an approximation contract: callers use it to PROVE that
// a cell cannot contain the zero set and skip work. Therefore a finite interval
// must contain every evalSdf() value in the queried AABB. Unknown mathematics
// must return an unbounded interval rather than a guessed finite enclosure.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <cmath>
#include <cstdio>

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}

bool contains(const OntoMath::Interval& r, float v) {
    return v >= r.lo && v <= r.hi;
}
} // namespace

int main() {
    std::printf("Running SDF range soundness test...\n");

    // Exact sphere: the 1-Lipschitz center ± half-diagonal enclosure should be
    // finite and contain representative interior/corner samples.
    {
        const geom::SdfNode sphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        const glm::vec3 lo(-0.3f, -0.2f, -0.1f);
        const glm::vec3 hi( 0.5f,  0.4f,  0.2f);
        const auto range = geom::evalRange(sphere, lo, hi);

        check(std::isfinite(range.lo) && std::isfinite(range.hi),
              "exact sphere receives a finite conservative range");

        for (int ix = 0; ix <= 4; ++ix) {
            for (int iy = 0; iy <= 4; ++iy) {
                for (int iz = 0; iz <= 4; ++iz) {
                    const glm::vec3 t(ix / 4.0f, iy / 4.0f, iz / 4.0f);
                    const glm::vec3 p = lo + t * (hi - lo);
                    check(contains(range, geom::evalSdf(sphere, p)),
                          "sphere range contains sampled field value");
                }
            }
        }
    }

    // Eccentric ellipsoid: Earthcall uses the fast k0*(k0-1)/k1 ellipsoid
    // approximation. It is not globally 1-Lipschitz. This tiny AABB is a
    // concrete falsifier for the old center±R assumption: with axes (3,1,1),
    // the center is near (-2.7477), while nearby points in the same box reach
    // roughly -2.17 and -2.77 -- far outside a radius-0.001732 enclosure.
    //
    // The sound answer until a proved ellipsoid bound exists is UNKNOWN,
    // represented by the infinite interval. A future tighter implementation is
    // allowed, but only if it remains conservative for the explicit witness.
    {
        const geom::SdfNode ellipsoid =
            geom::SdfNode::leaf(geom::SdfPrim::Ellipsoid,
                                glm::vec3(3.0f, 1.0f, 1.0f));
        const glm::vec3 center(0.02f, 0.001f, 0.0f);
        const glm::vec3 half(0.001f);
        const auto range = geom::evalRange(ellipsoid, center - half, center + half);

        const glm::vec3 witnessA(0.019f, 0.002f, -0.001f);
        const glm::vec3 witnessB(0.021f, 0.0f, -0.001f);
        check(contains(range, geom::evalSdf(ellipsoid, witnessA)),
              "ellipsoid range contains high directional witness");
        check(contains(range, geom::evalSdf(ellipsoid, witnessB)),
              "ellipsoid range contains low directional witness");
        check(!std::isfinite(range.lo) && !std::isfinite(range.hi),
              "unproved ellipsoid range fails open instead of asserting 1-Lipschitz");
    }

    // CSG must preserve fail-open knowledge: an unknown child may loosen the
    // result, but it must never become a false exclusion. Sample the combined
    // field to ensure its propagated interval remains conservative.
    {
        const geom::SdfNode ellipsoid =
            geom::SdfNode::leaf(geom::SdfPrim::Ellipsoid,
                                glm::vec3(3.0f, 1.0f, 1.0f));
        const geom::SdfNode sphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f));
        const geom::SdfNode combined =
            geom::SdfNode::binary(geom::SdfOp::Union, ellipsoid, sphere);

        const glm::vec3 lo(-0.1f), hi(0.1f);
        const auto range = geom::evalRange(combined, lo, hi);
        for (int ix = 0; ix <= 4; ++ix) {
            for (int iy = 0; iy <= 4; ++iy) {
                for (int iz = 0; iz <= 4; ++iz) {
                    const glm::vec3 t(ix / 4.0f, iy / 4.0f, iz / 4.0f);
                    const glm::vec3 p = lo + t * (hi - lo);
                    check(contains(range, geom::evalSdf(combined, p)),
                          "CSG range containing unknown child remains conservative");
                }
            }
        }
    }

    if (failures) {
        std::printf("sdf_range_soundness_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_range_soundness_test: PASS\n");
    return 0;
}
