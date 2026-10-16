// CPU witness for Rendering::buildVolumeZeroProof: the shader skips density
// evaluation in every cell this proof marks, so a mark is only allowed where
// the authored density can never be positive.
//
// Part 1 uses synthetic media (nothing aurora-shaped) to pin the generic
// contract. Part 2 loads every medium in the real saves/zones Northern Veil
// save read-only and checks thousands of points inside proven cells against
// the exact CPU evaluator. Claude Opus 5.5 · Claude Code · 2026-10-09.
// Support-envelope extension: Codex / GPT-6.1 Sol / 01a122d7 / 2026-10-10.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "support/test_harness.hpp"
#include "support/euclidean_range_witness.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

OntoMath::Piecewise piecewise(const std::string& json) {
    return OntoMath::Piecewise::fromJson(nlohmann::json::parse(json));
}

std::map<std::string, PropertyValue> varsAt(const glm::vec3& p, double t = 0.0) {
    return {{"p", PropertyValue(p)},
            {"x", PropertyValue(static_cast<double>(p.x))},
            {"y", PropertyValue(static_cast<double>(p.y))},
            {"z", PropertyValue(static_cast<double>(p.z))},
            {OntoMath::kTimeVar, PropertyValue(t)}};
}

double valueAt(const OntoMath::Piecewise& pw, const glm::vec3& p, double t = 0.0) {
    const auto r = pw.evaluate(varsAt(p, t));
    double d = 0.0;
    if (!r || !propertyValueToNumber(*r, d)) return 1.0;   // unevaluable counts as positive
    return d;
}

// Exhaustive certificate-index witness: every unknown leaf, including its
// padded extent, must be retained in the selected column's support. This
// checks the theorem used for jumps, rather than relying on finite pixels.
bool bandsContainUnknown(const Rendering::VolumeZeroProof& proof, const glm::vec3& half) {
    if (proof.supportAxis > 2 || proof.supportBands.size() != proof.dims[proof.supportAxis]) return false;
    const auto axis = proof.supportAxis, a = (axis + 1) % 3, b = (axis + 2) % 3;
    const glm::vec3 size = 2.0f * glm::abs(half) / glm::vec3(proof.dims);
    for (uint32_t z = 0; z < proof.dims.z; ++z)
    for (uint32_t y = 0; y < proof.dims.y; ++y)
    for (uint32_t x = 0; x < proof.dims.x; ++x) {
        if (proof.proven(x, y, z)) continue;
        const glm::uvec3 c(x, y, z);
        const auto band = proof.supportBands[c[axis]];
        const auto lo = -glm::abs(half) + glm::vec3(c) * size - size * .01f;
        const auto hi = -glm::abs(half) + glm::vec3(c + glm::uvec3(1u)) * size + size * .01f;
        if (!(band.x <= lo[a] && band.y <= lo[b] && band.z >= hi[a] && band.w >= hi[b])) return false;
    }
    return true;
}

// A slab density clamp(1 - |x| / 2, 0, 1): positive only for |x| < 2.
const char* kSlab = R"({"input":"x","pieces":[{"mathNode":{"op":26,"children":[
  {"op":5,"children":[{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},
    {"op":23,"children":[{"op":25,"children":[{"op":1,"var":"x"}]},
      {"op":0,"scalarForm":{"terms":[{"c":2.0,"factors":{}}]}}]}]},
  {"op":0,"scalarForm":{"terms":[{"c":0.0,"factors":{}}]}},
  {"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}]}}]})";

// Soundness sweep: random points inside proven cells must evaluate <= 0.
void sweep(const OntoMath::Piecewise& pw, const Rendering::VolumeZeroProof& proof,
           const glm::vec3& halfExtent, const std::string& label, uint32_t samples) {
    std::mt19937 rng(20261009u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const glm::vec3 cell = 2.0f * halfExtent / glm::vec3(proof.dims);
    uint32_t tested = 0, positive = 0;
    for (uint32_t i = 0; i < samples * 8 && tested < samples; ++i) {
        const glm::uvec3 c(rng() % proof.dims.x, rng() % proof.dims.y, rng() % proof.dims.z);
        if (!proof.proven(c.x, c.y, c.z)) continue;
        const glm::vec3 p = -halfExtent +
            (glm::vec3(c) + glm::vec3(unit(rng), unit(rng), unit(rng))) * cell;
        ++tested;
        if (valueAt(pw, p) > 0.0) ++positive;
    }
    check(tested > 0 && positive == 0,
          label + ": " + std::to_string(tested) + " points in proven cells all have density <= 0");
}

// Preserve the independent host optimizations while refusing invalid inputs.
// These are range/type witnesses, never permission for a new native skip.
void rangeOptimizationWitness() {
    using N = OntoMath::MathNode;
    using I = OntoMath::Interval;
    using RV = N::RangeValue;
    auto number = [](double v) { auto n = std::make_unique<N>();
        n->op = N::Op::ScalarLeaf; n->scalarForm.terms.push_back(OntoMath::Term(v)); return n; };
    auto variable = [](const char* v) { auto n = std::make_unique<N>();
        n->op = N::Op::ValueLeaf; n->variableName = v; return n; };
    auto noise = [&](const char* v) { auto n = std::make_unique<N>();
        n->op = N::Op::Noise; n->children.push_back(variable(v)); return n; };
    auto scale = [](std::unique_ptr<N> a, std::unique_ptr<N> b) {
        auto n = std::make_unique<N>(); n->op = N::Op::Scale;
        n->children.push_back(std::move(a)); n->children.push_back(std::move(b)); return n; };
    const std::map<std::string, RV> vars{{"p", RV::makeVector(I(-.25f,.25f),I(0),I(0))},
                                          {"s", RV::makeScalar(I(3))}};
    for (bool reverse : {false, true}) {
        auto expr = reverse ? scale(noise("p"), number(0)) : scale(number(0), noise("p"));
        auto r = expr->evalRange(vars);
        check(r && r->kind == OntoMath::ValueKind::Scalar && r->scalar.exactZero(),
              "zero annihilates valid noise in either Scale operand order");
        expr = reverse ? scale(variable("p"), number(0)) : scale(number(0), variable("p"));
        r = expr->evalRange(vars);
        check(r && r->kind == OntoMath::ValueKind::Vector && r->vec[0].exactZero() &&
              r->vec[1].exactZero() && r->vec[2].exactZero(),
              "zero scaling retains vector type and all three zero components");
        expr = reverse ? scale(noise("s"), number(0)) : scale(number(0), noise("s"));
        check(!expr->evalRange(vars), "zero scaling still refuses a scalar Noise argument");
        auto malformed = noise("p"); malformed->children.clear();
        expr = reverse ? scale(std::move(malformed), number(0)) : scale(number(0), std::move(malformed));
        check(!expr->evalRange(vars), "zero scaling still refuses malformed Noise arity");
        expr = reverse ? scale(nullptr, number(0)) : scale(number(0), nullptr);
        check(!expr->evalRange(vars), "zero scaling refuses null operands without dereferencing them");
    }
    auto expr = scale(number(2), noise("p"));
    const auto r = expr->evalRange(vars);
    bool enclosed = r && r->kind == OntoMath::ValueKind::Scalar;
    for (float x : {-.25f, -.125f, 0.0f, .125f, .25f}) {
        const auto v = expr->evaluate({{"p", glm::vec3(x,0,0)}});
        enclosed &= v && r && r->scalar.lo <= std::get<double>(*v) && std::get<double>(*v) <= r->scalar.hi;
    }
    check(enclosed, "nonzero Scale keeps its ordinary noise range and encloses host values");
    auto scalar = number(1); scalar->scalarForm.terms.front().factors["s"] = 1;
    const auto a = scalar->evalRange(vars);
    auto changed = vars; changed["s"] = RV::makeScalar(I(7));
    const auto b = scalar->evalRange(changed);
    check(a && b && a->scalar.lo <= 3 && a->scalar.hi >= 3 && b->scalar.lo <= 7 && b->scalar.hi >= 7 &&
          a->scalar.hi < b->scalar.lo, "per-query scalar-map reuse respects fresh variable bindings");
    const auto positive = piecewise(R"({"pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1}]}}}]})");
    uint32_t ranges = 0;
    check(!Rendering::densityProvenNonPositive(positive, glm::vec3(-1), glm::vec3(1), &ranges) && ranges == 0,
          "positive midpoint only rejects a proof and bypasses interval construction");
}

} // namespace

int main() {
    std::cout << "Starting volume_zero_proof_test...\n";

    EuclideanRangeWitness::run(check);
    rangeOptimizationWitness();

    // ---- Part 1: the generic contract on synthetic media -------------------
    {
        const auto slab = piecewise(kSlab);
        const glm::vec3 half(10.0f);
        const auto proof = Rendering::buildVolumeZeroProof(slab, half, 4096u);
        check(proof.totalCells > 0 && proof.any(), "a clamped slab proves its empty cells");
        check(bandsContainUnknown(proof, half), "column bounds contain every unproven slab cell");
        check(proof.provenCells < proof.totalCells, "the slab's occupied cells stay unproven");
        bool centreProven = false, edgeProven = false;
        for (uint32_t z = 0; z < proof.dims.z; ++z)
            for (uint32_t y = 0; y < proof.dims.y; ++y) {
                centreProven |= proof.proven(proof.dims.x / 2, y, z);
                edgeProven |= proof.proven(0, y, z);
            }
        check(!centreProven, "no cell on the slab's centre plane x=0 is proven");
        check(edgeProven, "cells at x=-10 (density 0) are proven");
        check(proof.supportMin.x <= -2.0f && proof.supportMax.x >= 2.0f &&
              proof.supportMin.x > -half.x && proof.supportMax.x < half.x,
              "slab support encloses every positive x while removing empty prefixes/tails");
        check(proof.supportMin.y <= -half.y && proof.supportMax.y >= half.y &&
              proof.supportMin.z <= -half.z && proof.supportMax.z >= half.z,
              "support retains the full unconstrained y/z extent");
        sweep(slab, proof, half, "slab", 2000);
        check(proof.rangeEvaluations < proof.totalCells,
              "coarse-to-fine proves empty regions in fewer evaluations than cells (" +
              std::to_string(proof.rangeEvaluations) + " < " + std::to_string(proof.totalCells) + ")");
    }
    {
        const auto zero = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.0,"factors":{}}]}}}]})");
        const auto proof = Rendering::buildVolumeZeroProof(zero, glm::vec3(5.0f), 512u);
        check(proof.provenCells == proof.totalCells && proof.rangeEvaluations == 1,
              "identically-zero density is proven everywhere in one evaluation");
        check(glm::any(glm::greaterThan(proof.supportMin, proof.supportMax)),
              "all-empty density has an explicitly empty support envelope");
        bool emptyBands = true;
        for (const auto& band : proof.supportBands) emptyBands &= band.x > band.z && band.y > band.w;
        check(emptyBands && !proof.supportBands.empty(), "zero density has explicitly empty columns");
    }
    {
        const auto positive = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.25,"factors":{}}]}}}]})");
        const auto proof = Rendering::buildVolumeZeroProof(positive, glm::vec3(5.0f), 512u);
        check(!proof.any(),
              "a positive constant proves nothing");
        check(bandsContainUnknown(proof, glm::vec3(5.0f)), "positive density retains all unknown cells in columns");
        check(glm::all(glm::lessThanEqual(proof.supportMin, glm::vec3(-5.0f))) &&
              glm::all(glm::greaterThanEqual(proof.supportMax, glm::vec3(5.0f))),
              "unproven positive density retains its entire support");
    }
    {
        // D = t: time is never bound by the proof, so nothing may be proven.
        const auto timed = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":1,"var":"t"}}]})");
        check(!Rendering::buildVolumeZeroProof(timed, glm::vec3(5.0f), 512u).any(),
              "a time-dependent density proves nothing (fail-open)");
        const auto unknown = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":1,"var":"mystery"}}]})");
        check(!Rendering::buildVolumeZeroProof(unknown, glm::vec3(5.0f), 512u).any(),
              "an unbound variable proves nothing (fail-open)");
        check(bandsContainUnknown(Rendering::buildVolumeZeroProof(timed, glm::vec3(5.0f), 512u), glm::vec3(5.0f)) &&
              bandsContainUnknown(Rendering::buildVolumeZeroProof(unknown, glm::vec3(5.0f), 512u), glm::vec3(5.0f)),
              "unbound clocks and unknown variables cannot disappear from column support");
    }
    {
        // Conditional piece positive for x >= 3, else falls through to 0.
        const auto gated = piecewise(R"({"input":"x","pieces":[{"lo":3.0,"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}}]})");
        check(!Rendering::buildVolumeZeroProof(gated, glm::vec3(5.0f), 512u).any(),
              "a reachable positive conditional piece defeats the proof (no piece-bound reasoning claimed)");
    }

    // ---- Part 2: every medium of the real Northern Veil save ---------------
    const std::string path = TestSupport::resolveRealWorldPath("saves/zones/Northern Veil/zone.ecform");
    if (!std::filesystem::exists(path)) {
        std::cout << "  (Northern Veil save not present; real-save sweep skipped)\n";
    } else {
        std::ifstream in(path, std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                         std::istreambuf_iterator<char>());
        const auto wrapper = nlohmann::json::from_msgpack(bytes);
        const auto zone = nlohmann::json::parse(wrapper.at("MigrationRoot").get<std::string>());
        int media = 0;
        for (const auto& fj : zone.at("spatialFields")) {
            auto node = geom::FieldNode::fromJson(fj);
            if (!node || !node->volumeDensity || node->volumeDensity->pieces.empty()) continue;
            ++media;
            const glm::vec3 half = glm::abs(node->scale);   // renderer: halfExtent = |scale|
            const auto proof = Rendering::buildVolumeZeroProof(*node->volumeDensity, half);
            std::cout << "       " << node->getIdentifier() << ": " << proof.provenCells << "/"
                      << proof.totalCells << " cells proven empty, " << proof.rangeEvaluations
                      << " range evaluations\n";
            check(proof.any(), node->getIdentifier() + " has provably empty cells");
            check(bandsContainUnknown(proof, half), node->getIdentifier() + ": columns enclose EVERY unknown cell");
            std::cout << "       column axis=" << proof.supportAxis << " count=" << proof.supportBands.size() << '\n';
            sweep(*node->volumeDensity, proof, half, node->getIdentifier(), 4000);
        }
        check(media > 0, "the saved Zone has participating media to prove");
    }

    std::cout << "volume_zero_proof_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
