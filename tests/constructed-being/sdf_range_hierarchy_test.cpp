// Regression witness for the inert conservative SDF zero-set hierarchy.
//
// Path 1 checks the hierarchy's structural contract (contiguous direct children,
// bounded construction, explicit unknown leaves).
// Path 2 independently samples the real evalSdf() inside every node that the
// hierarchy claims cannot contain zero. A proof node may be loose, but it may
// never suppress a sampled sign crossing.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfRangeProof.hpp"
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
        const auto proxy = geom::deriveZeroSetProxy(h, glm::vec3(2.0f));
        check(proxy.hasPossibleZero, "sphere proxy preserves possible surface");
        check(proxy.tightened, "sphere proxy tightens the authored cube");
        check(proxy.halfExtent.x <= 2.0f &&
              proxy.halfExtent.y <= 2.0f &&
              proxy.halfExtent.z <= 2.0f,
              "sphere proxy never expands beyond authored coverage");
        size_t positiveSkipSafe = 0;
        size_t negativeZeroFree = 0;
        for (const auto& node : h.nodes) {
            if (geom::rangeNodeProvesPositiveOutside(node)) ++positiveSkipSafe;
            if (node.boundFinite && node.rangeHi < 0.0f) {
                ++negativeZeroFree;
                check(!geom::rangeNodeProvesPositiveOutside(node),
                      "proved-negative interior cell is never traversal-skippable");
            }
        }
        check(positiveSkipSafe > 0,
              "sphere hierarchy contains proved-positive outside cells to skip");
        check(negativeZeroFree > 0,
              "sphere hierarchy contains proved-negative interior cells to retain");
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
        const auto proxy = geom::deriveZeroSetProxy(h, glm::vec3(4.0f));
        check(proxy.hasPossibleZero, "unknown ellipsoid proxy fails open");
        check(!proxy.tightened, "unknown ellipsoid retains full authored proxy");
        check(proxy.halfExtent == glm::vec3(4.0f),
              "unknown ellipsoid proxy exactly preserves authored extent");
        verifyStructure(h);
    }

    // A constant-positive field has a complete finite theorem excluding zero
    // over the root itself. The proxy may cull only this strongest case: no
    // ambiguous or unknown terminal region remains.
    {
        auto constant = std::make_shared<OntoMath::MathNode>();
        constant->op = OntoMath::MathNode::Op::ScalarLeaf;
        constant->scalarForm.terms.push_back(OntoMath::Term(5.0));
        const geom::SdfNode empty = geom::makeImplicit(constant);
        const auto h = geom::buildRangeHierarchy(
            empty, glm::vec3(3.0f), /*maxDepth=*/5, /*maxNodes=*/1000);
        const auto proxy = geom::deriveZeroSetProxy(h, glm::vec3(3.0f));

        check(h.nodes.size() == 1, "constant-positive field proves empty at root");
        check(h.nodes[0].provedNoZero, "constant-positive root excludes zero");
        check(!proxy.hasPossibleZero, "fully proved zero-free hierarchy culls proxy");
    }

    // Perlin now has a proved global amplitude enclosure plus a local Lipschitz
    // tightening rule. The y term lets the hierarchy prove outer vertical slabs
    // while unresolved cells around the zero set remain explicit.
    // y term still lets the hierarchy prove slabs beyond the maximum possible
    // noise amplitude. This is useful scaffold evidence without pretending it
    // solves horizon x/z ambiguity yet.
    {
        const geom::SdfNode perlin =
            geom::makeImplicit(perlinFloorMath(/*amplitude=*/40.0,
                                               /*frequency=*/0.008));
        // With the directly proved 2.2*sqrt(3) global amplitude bound,
        // depth 3 is intentionally not yet strong enough to certify the outer
        // y slabs for this +/-120 domain. At depth 4 the local Lipschitz bound
        // over the 0.008-scaled argument becomes tighter than the global bound
        // and the theorem can exclude those slabs without weakening safety.
        const auto h = geom::buildRangeHierarchy(
            perlin, glm::vec3(20.0f, 120.0f, 20.0f),
            /*maxDepth=*/4, /*maxNodes=*/5000);

        check(h.nodes.size() > 1, "Perlin hierarchy subdivides finite range");
        check(h.provedEmptyNodes > 0,
              "Perlin local Lipschitz hierarchy proves outer vertical slabs empty");
        check(h.ambiguousLeaves > 0,
              "Perlin hierarchy preserves unresolved terrain band");
        check(h.unknownLeaves == 0,
              "supported Perlin expression has finite conservative ranges");
        check(h.nodes.size() <= 5000, "Perlin hierarchy obeys hard node budget");
        verifyStructure(h);
        verifyProvedCellsBySampling(perlin, h);
    }

    // Real Perlin-floor scale: this is the renderer's actual non-heightfield
    // proxy extent (authored [1000,30,1000] grown by 5%). The old 8,192-node
    // renderer budget could truncate a depth-5 tree before it reached useful
    // small cells. A complete 65,536 budget plus lattice-aware Noise bounds must
    // now prove genuine empty space while preserving an ambiguous terrain band.
    {
        const geom::SdfNode perlin =
            geom::makeImplicit(perlinFloorMath(/*amplitude=*/40.0,
                                               /*frequency=*/0.008));
        const glm::vec3 realProxyExtent(1050.0f, 31.5f, 1050.0f);
        const auto h = geom::buildRangeHierarchy(
            perlin, realProxyExtent,
            /*maxDepth=*/6, /*maxNodes=*/327680);

        check(!h.nodes.empty(), "real-scale Perlin hierarchy builds");
        check(h.provedEmptyNodes > 0,
              "real-scale Perlin hierarchy proves zero-free cells");
        check(h.ambiguousLeaves > 0,
              "real-scale Perlin hierarchy preserves terrain ambiguity");
        check(h.unknownLeaves == 0,
              "real-scale supported Perlin expression stays finite");
        check(h.nodes.size() <= 327680,
              "real-scale Perlin hierarchy respects complete-tree budget");
        check(h.nodes.size() <= 299593,
              "depth-6 octree never exceeds mathematical node maximum");
        size_t positiveSkipNodes = 0;
        size_t negativeZeroFreeNodes = 0;
        for (const auto& node : h.nodes) {
            if (geom::rangeNodeProvesPositiveOutside(node)) ++positiveSkipNodes;
            if (node.boundFinite && node.rangeHi < 0.0f) ++negativeZeroFreeNodes;
        }
        check(positiveSkipNodes > 0,
              "real-scale Perlin hierarchy contains positive outside cells to skip");
        check(negativeZeroFreeNodes > 0,
              "real-scale Perlin hierarchy also preserves proved-negative interior cells");
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

    // Ahead-of-time spatial proof maintenance must repair value-premise
    // changes without throwing away already-refined topology. Moving the same
    // sphere far outside the authored domain collapses the active theorem to a
    // positive root while retaining its child block as cache topology; moving
    // it back must reactivate that exact subdivision without reallocating it.
    {
        geom::SdfNode sphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        const glm::vec3 extent(2.0f);
        auto h = geom::buildRangeHierarchy(
            sphere, extent, /*maxDepth=*/4, /*maxNodes=*/10000);

        check(!h.nodes.empty() && h.nodes[0].childCount == 8,
              "incremental witness begins with a refined ambiguous root");
        const size_t initialNodeCount = h.nodes.size();
        const uint32_t retainedRootChildren = h.nodes[0].firstChild;
        check(retainedRootChildren != 0u,
              "refined root owns a reusable child block");

        sphere.offset = glm::vec3(10.0f, 0.0f, 0.0f);
        geom::SdfRangeRefreshStats collapseStats;
        check(geom::refreshRangeHierarchy(
                  h, sphere, extent, /*maxDepth=*/4, /*maxNodes=*/10000,
                  &collapseStats),
              "parameter-only proof refresh succeeds");
        check(collapseStats.evaluatedNodes == 1,
              "proved-positive root stops incremental refresh at the root");
        check(h.nodes[0].childCount == 0,
              "proved-positive root deactivates deeper theorem topology");
        check(h.nodes[0].firstChild == retainedRootChildren,
              "collapsed proof retains its previous child block for repair");
        check(h.nodes.size() == initialNodeCount,
              "proof collapse does not discard allocated topology");

        const auto collapsedProof =
            geom::derivePositiveRangeProofGrid(h, /*targetDepth=*/4);
        check(collapsedProof.dim == 16 &&
                  collapsedProof.positiveCells == 4096,
              "active positive root fills the fixed-depth proof bitmap");

        sphere.offset = glm::vec3(0.0f);
        geom::SdfRangeRefreshStats restoreStats;
        check(geom::refreshRangeHierarchy(
                  h, sphere, extent, /*maxDepth=*/4, /*maxNodes=*/10000,
                  &restoreStats),
              "returning value premises incrementally repairs the theorem");
        check(h.nodes[0].childCount == 8 &&
                  h.nodes[0].firstChild == retainedRootChildren,
              "repair reactivates the original root child block");
        check(restoreStats.reusedChildBlocks > 0,
              "repair reuses previously-refined child blocks");
        check(restoreStats.allocatedChildBlocks == 0,
              "restoring identical premises needs no new topology allocation");
        check(h.nodes.size() == initialNodeCount,
              "incremental repair preserves the original node allocation");
        verifyStructure(h);
        verifyProvedCellsBySampling(sphere, h);
    }

    // Positive-proof coalescing is derived acceleration permission. It may
    // discard theorem knowledge, but it must never create a skip across any
    // negative, ambiguous, unknown, or structurally missing partition.
    {
        geom::SdfRangeHierarchy siblings;
        siblings.nodes.resize(9);
        auto& root = siblings.nodes[0];
        root.depth = 0;
        root.boundFinite = true;
        root.rangeLo = -1.0f;
        root.rangeHi = 1.0f;
        root.firstChild = 1;
        root.childCount = 8;

        for (uint32_t i = 1; i <= 8; ++i) {
            auto& child = siblings.nodes[i];
            child.depth = 1;
            child.boundFinite = true;
            child.rangeLo = 1.0f;
            child.rangeHi = 2.0f;
            child.provedNoZero = true;
        }

        const auto coalescedRoot =
            geom::derivePositiveRangeProofGrid(siblings, /*targetDepth=*/0);
        check(coalescedRoot.positiveCells == 1,
              "eight positive partition children coalesce into one parent proof");
        check(coalescedRoot.words.size() == 1 &&
                  (coalescedRoot.words[0] & 1u) != 0u,
              "coalesced parent proof sets the regular-grid bit");

        const auto directChildren =
            geom::derivePositiveRangeProofGrid(siblings, /*targetDepth=*/1);
        check(directChildren.positiveCells == 8,
              "target-depth positive children remain eight independent proofs");

        auto withNegative = siblings;
        withNegative.nodes[8].rangeLo = -2.0f;
        withNegative.nodes[8].rangeHi = -1.0f;
        const auto negativeRefusal =
            geom::derivePositiveRangeProofGrid(withNegative, /*targetDepth=*/0);
        check(!negativeRefusal.hasPositiveCells() &&
                  negativeRefusal.words.empty(),
              "one proved-negative child refuses positive parent coalescing");

        auto withAmbiguous = siblings;
        withAmbiguous.nodes[8].rangeLo = -1.0f;
        withAmbiguous.nodes[8].rangeHi = 1.0f;
        withAmbiguous.nodes[8].provedNoZero = false;
        const auto ambiguousRefusal =
            geom::derivePositiveRangeProofGrid(withAmbiguous, /*targetDepth=*/0);
        check(!ambiguousRefusal.hasPositiveCells(),
              "one ambiguous child refuses positive parent coalescing");

        auto withUnknown = siblings;
        withUnknown.nodes[8].boundFinite = false;
        withUnknown.nodes[8].rangeLo = 0.0f;
        withUnknown.nodes[8].rangeHi = 0.0f;
        withUnknown.nodes[8].provedNoZero = false;
        const auto unknownRefusal =
            geom::derivePositiveRangeProofGrid(withUnknown, /*targetDepth=*/0);
        check(!unknownRefusal.hasPositiveCells(),
              "one unknown child refuses positive parent coalescing");

        auto malformed = siblings;
        malformed.nodes.pop_back();
        const auto missingChildRefusal =
            geom::derivePositiveRangeProofGrid(malformed, /*targetDepth=*/0);
        check(!missingChildRefusal.hasPositiveCells(),
              "missing partition child fails open instead of inventing proof");

        geom::SdfRangeHierarchy positiveAncestor;
        positiveAncestor.nodes.resize(1);
        positiveAncestor.nodes[0].depth = 0;
        positiveAncestor.nodes[0].boundFinite = true;
        positiveAncestor.nodes[0].rangeLo = 3.0f;
        positiveAncestor.nodes[0].rangeHi = 4.0f;
        positiveAncestor.nodes[0].provedNoZero = true;

        const auto filled =
            geom::derivePositiveRangeProofGrid(positiveAncestor, /*targetDepth=*/2);
        check(filled.dim == 4 && filled.positiveCells == 64,
              "positive ancestor authorizes every target-depth descendant");
        check(filled.words.size() == 2 &&
                  filled.words[0] == 0xFFFFFFFFu &&
                  filled.words[1] == 0xFFFFFFFFu,
              "positive ancestor fills the exact descendant bitmap");
    }

    if (failures) {
        std::printf("sdf_range_hierarchy_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_range_hierarchy_test: PASS\n");
    return 0;
}
