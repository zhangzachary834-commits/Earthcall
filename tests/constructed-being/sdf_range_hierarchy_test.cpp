// Regression witness for the inert conservative SDF zero-set hierarchy.
//
// Path 1 checks the hierarchy's structural contract (contiguous direct children,
// bounded construction, explicit unknown leaves).
// Path 2 independently samples the real evalSdf() inside every node that the
// hierarchy claims cannot contain zero. A proof node may be loose, but it may
// never suppress a sampled sign crossing.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}

std::unique_ptr<OntoMath::MathNode> number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}

std::unique_ptr<OntoMath::MathNode> variable(const std::string& name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::shared_ptr<OntoMath::MathNode> perlinFloorMath(double amplitude, double frequency) {
    auto scaledPoint = std::make_unique<OntoMath::MathNode>();
    scaledPoint->op = OntoMath::MathNode::Op::Scale;
    scaledPoint->children.push_back(number(frequency));
    scaledPoint->children.push_back(variable(OntoMath::kAmbientPointVar));

    auto noise = std::make_unique<OntoMath::MathNode>();
    noise->op = OntoMath::MathNode::Op::Noise;
    noise->children.push_back(std::move(scaledPoint));

    auto scaledNoise = std::make_unique<OntoMath::MathNode>();
    scaledNoise->op = OntoMath::MathNode::Op::Scale;
    scaledNoise->children.push_back(number(amplitude));
    scaledNoise->children.push_back(std::move(noise));

    auto root = std::make_unique<OntoMath::MathNode>();
    root->op = OntoMath::MathNode::Op::Sub;
    root->children.push_back(variable("y"));
    root->children.push_back(std::move(scaledNoise));
    return std::shared_ptr<OntoMath::MathNode>(root.release());
}

void verifyStructure(const geom::SdfRangeHierarchy& h) {
    if (h.nodes.empty()) return;

    std::vector<bool> reached(h.nodes.size(), false);
    std::vector<uint32_t> stack{0u};
    while (!stack.empty()) {
        const uint32_t idx = stack.back();
        stack.pop_back();
        if (idx >= h.nodes.size()) {
            check(false, "hierarchy child index stays inside node array");
            continue;
        }
        if (reached[idx]) {
            check(false, "hierarchy node is reached exactly once");
            continue;
        }
        reached[idx] = true;

        const auto& n = h.nodes[idx];
        check(n.childCount == 0 || n.childCount == 8,
              "hierarchy node has leaf-or-octet arity");
        if (n.provedNoZero) {
            check(n.boundFinite, "proved-empty node has a finite bound");
            check(n.rangeLo > 0.0f || n.rangeHi < 0.0f,
                  "proved-empty node interval excludes zero");
            check(n.childCount == 0, "proved-empty node is terminal");
        }
        if (!n.boundFinite) {
            check(n.childCount == 0, "unknown range fails open as terminal leaf");
        }
        if (n.childCount == 8) {
            check(n.firstChild + 7u < h.nodes.size(),
                  "octet direct-child span is in bounds");
            for (uint32_t c = 0; c < 8; ++c) {
                const uint32_t child = n.firstChild + c;
                if (child < h.nodes.size()) {
                    check(h.nodes[child].depth == static_cast<uint8_t>(n.depth + 1),
                          "direct child depth increments exactly once");
                    stack.push_back(child);
                }
            }
        }
    }

    check(std::all_of(reached.begin(), reached.end(), [](bool v) { return v; }),
          "every hierarchy node is connected to the root");
}

void verifyProvedCellsBySampling(const geom::SdfNode& sdf,
                                 const geom::SdfRangeHierarchy& h) {
    for (const auto& n : h.nodes) {
        if (!n.provedNoZero) continue;

        // 3^3 deterministic samples: corners, edge/face midpoints, center.
        for (int ix = 0; ix < 3; ++ix) {
            for (int iy = 0; iy < 3; ++iy) {
                for (int iz = 0; iz < 3; ++iz) {
                    const glm::vec3 t(ix * 0.5f, iy * 0.5f, iz * 0.5f);
                    const glm::vec3 p = n.boxMin + t * (n.boxMax - n.boxMin);
                    const float v = geom::evalSdf(sdf, p);
                    const bool sameSide =
                        (n.rangeLo > 0.0f && v > 0.0f) ||
                        (n.rangeHi < 0.0f && v < 0.0f);
                    check(sameSide,
                          "sampled exact SDF agrees with proved-empty cell sign");
                }
            }
        }
    }
}

} // namespace

int main() {
    std::printf("Running SDF range hierarchy test...\n");

    // Exact sphere: hierarchy should prune large empty regions while retaining
    // ambiguous cells around the surface.
    {
        const geom::SdfNode sphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        const auto h = geom::buildRangeHierarchy(
            sphere, glm::vec3(2.0f), /*maxDepth=*/4, /*maxNodes=*/10000);

        check(!h.nodes.empty(), "sphere hierarchy builds");
        check(h.provedEmptyNodes > 0, "sphere hierarchy proves empty cells");
        check(h.ambiguousLeaves > 0, "sphere hierarchy retains surface ambiguity");
        check(h.unknownLeaves == 0, "exact sphere hierarchy needs no unknown leaves");
        check(h.nodes.size() <= 10000, "sphere hierarchy obeys hard node budget");
        verifyStructure(h);
        verifyProvedCellsBySampling(sphere, h);
    }

    // Approximate ellipsoid range is deliberately unknown after the soundness
    // fix. The hierarchy must NOT recursively subdivide unknown into fake
    // confidence; it stops at one fail-open root.
    {
        const geom::SdfNode ellipsoid =
            geom::SdfNode::leaf(geom::SdfPrim::Ellipsoid,
                                glm::vec3(3.0f, 1.0f, 1.0f));
        const auto h = geom::buildRangeHierarchy(
            ellipsoid, glm::vec3(4.0f), /*maxDepth=*/8, /*maxNodes=*/10000);

        check(h.nodes.size() == 1, "unknown ellipsoid range does not subdivide");
        check(h.unknownLeaves == 1, "unknown ellipsoid is explicit fail-open leaf");
        check(h.provedEmptyNodes == 0, "unknown ellipsoid proves no empty space");
        verifyStructure(h);
    }

    // The current Perlin interval is global rather than spatially tight, but the
    // y term still lets the hierarchy prove slabs beyond the maximum possible
    // noise amplitude. This is useful scaffold evidence without pretending it
    // solves horizon x/z ambiguity yet.
    {
        const geom::SdfNode perlin =
            geom::makeImplicit(perlinFloorMath(/*amplitude=*/40.0,
                                               /*frequency=*/0.008));
        const auto h = geom::buildRangeHierarchy(
            perlin, glm::vec3(20.0f, 120.0f, 20.0f),
            /*maxDepth=*/3, /*maxNodes=*/2000);

        check(h.nodes.size() > 1, "Perlin hierarchy subdivides finite range");
        check(h.provedEmptyNodes > 0,
              "Perlin hierarchy proves outer vertical slabs empty");
        check(h.ambiguousLeaves > 0,
              "Perlin hierarchy preserves unresolved terrain band");
        check(h.unknownLeaves == 0,
              "supported Perlin expression has finite conservative ranges");
        check(h.nodes.size() <= 2000, "Perlin hierarchy obeys hard node budget");
        verifyStructure(h);
        verifyProvedCellsBySampling(perlin, h);
    }

    // Budget exhaustion is a correctness mode, not a partial-tree authority:
    // construction stops with ambiguous leaves while preserving coverage.
    {
        const geom::SdfNode sphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        const auto h = geom::buildRangeHierarchy(
            sphere, glm::vec3(2.0f), /*maxDepth=*/10, /*maxNodes=*/9);
        check(h.nodes.size() <= 9, "tiny hierarchy obeys node budget");
        check(h.ambiguousLeaves > 0,
              "budget exhaustion remains explicit ambiguity");
        verifyStructure(h);
    }

    if (failures) {
        std::printf("sdf_range_hierarchy_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_range_hierarchy_test: PASS\n");
    return 0;
}
