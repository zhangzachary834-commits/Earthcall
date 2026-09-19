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

    // VectorConstruct requires scalar children at runtime. A vector
    // child must not have its default scalar RangeValue slot mistaken for a
    // finite number by abstract interpretation.
    {
        auto p = std::make_unique<OntoMath::MathNode>();
        p->op = OntoMath::MathNode::Op::ValueLeaf;
        p->variableName = OntoMath::kAmbientPointVar;

        auto zeroY = std::make_unique<OntoMath::MathNode>();
        zeroY->op = OntoMath::MathNode::Op::ScalarLeaf;
        zeroY->scalarForm.terms.push_back(OntoMath::Term(0.0));
        auto zeroZ = std::make_unique<OntoMath::MathNode>();
        zeroZ->op = OntoMath::MathNode::Op::ScalarLeaf;
        zeroZ->scalarForm.terms.push_back(OntoMath::Term(0.0));

        OntoMath::MathNode malformed;
        malformed.op = OntoMath::MathNode::Op::VectorConstruct;
        malformed.children.push_back(std::move(p));
        malformed.children.push_back(std::move(zeroY));
        malformed.children.push_back(std::move(zeroZ));

        std::map<std::string, OntoMath::MathNode::RangeValue> vars{
            {OntoMath::kAmbientPointVar,
             OntoMath::MathNode::RangeValue::makeVector(
                 OntoMath::Interval(-1.0f, 1.0f),
                 OntoMath::Interval(-2.0f, 2.0f),
                 OntoMath::Interval(-3.0f, 3.0f))}
        };
        const auto range = malformed.evalRange(vars);
        check(range && range->kind == OntoMath::ValueKind::Vector,
              "malformed VectorConstruct range remains explicitly vector-valued");
        if (range && range->kind == OntoMath::ValueKind::Vector) {
            check(!std::isfinite(range->vec[0].lo) &&
                      !std::isfinite(range->vec[0].hi),
                  "vector-valued child makes VectorConstruct range fail open");
        }
    }

    // Guarded division: runtime returns 0 when |denominator| is below
    // kDegenerateDivisor. Ordinary interval division over a tiny positive
    // denominator would exclude that real zero and become an unsound proof.
    // The range path must therefore fail open across the guard band.
    {
        auto numerator = std::make_unique<OntoMath::MathNode>();
        numerator->op = OntoMath::MathNode::Op::ScalarLeaf;
        numerator->scalarForm.terms.push_back(OntoMath::Term(1.0));

        auto denominator = std::make_unique<OntoMath::MathNode>();
        denominator->op = OntoMath::MathNode::Op::ValueLeaf;
        denominator->variableName = "x";

        auto div = std::make_shared<OntoMath::MathNode>();
        div->op = OntoMath::MathNode::Op::Div;
        div->children.push_back(std::move(numerator));
        div->children.push_back(std::move(denominator));

        const geom::SdfNode field = geom::makeImplicit(div);
        const float tiny = static_cast<float>(OntoMath::kDegenerateDivisor * 0.25);
        const glm::vec3 lo(tiny, 0.0f, 0.0f);
        const glm::vec3 hi(tiny * 2.0f, 0.0f, 0.0f);
        const auto range = geom::evalRange(field, lo, hi);
        const float exact = geom::evalSdf(field, glm::vec3(tiny, 0.0f, 0.0f));

        check(exact == 0.0f, "degenerate Div runtime witness returns guarded zero");
        check(contains(range, exact),
              "degenerate Div range contains runtime guarded zero");
        check(!std::isfinite(range.lo) && !std::isfinite(range.hi),
              "degenerate Div range fails open across guard band");
    }

    // Classic Perlin: the global amplitude theorem remains the outer guard,
    // but the proved Lipschitz constant should tighten a sufficiently small
    // input AABB. Independently sample the exact CPU evaluator inside that box.
    {
        auto p = std::make_unique<OntoMath::MathNode>();
        p->op = OntoMath::MathNode::Op::ValueLeaf;
        p->variableName = OntoMath::kAmbientPointVar;

        auto noise = std::make_shared<OntoMath::MathNode>();
        noise->op = OntoMath::MathNode::Op::Noise;
        noise->children.push_back(std::move(p));

        const geom::SdfNode field = geom::makeImplicit(noise);
        const glm::vec3 centre(0.23f, -0.37f, 0.41f);
        const glm::vec3 half(0.001f);
        const auto range = geom::evalRange(field, centre - half, centre + half);

        check(std::isfinite(range.lo) && std::isfinite(range.hi),
              "Perlin local range is finite");
        check((range.hi - range.lo) <
                  2.0f * OntoMath::kClassicPerlin3ValueBound,
              "Perlin Lipschitz theorem tightens a small box below global amplitude");

        for (int ix = 0; ix <= 4; ++ix) {
            for (int iy = 0; iy <= 4; ++iy) {
                for (int iz = 0; iz <= 4; ++iz) {
                    const glm::vec3 t(ix / 4.0f, iy / 4.0f, iz / 4.0f);
                    const glm::vec3 q = centre - half + t * (2.0f * half);
                    check(contains(range, geom::evalSdf(field, q)),
                          "tightened Perlin interval contains sampled exact value");
                }
            }
        }
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
