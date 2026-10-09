// CPU witness for Rendering::buildVolumeZeroProof: the shader skips density
// evaluation in every cell this proof marks, so a mark is only allowed where
// the authored density can never be positive.
//
// Part 1 uses synthetic media (nothing aurora-shaped) to pin the generic
// contract. Part 2 loads every medium in the real saves/zones Northern Veil
// save read-only and checks thousands of points inside proven cells against
// the exact CPU evaluator. Claude Opus 5.5 · Claude Code · 2026-10-09.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "support/test_harness.hpp"

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

} // namespace

int main() {
    std::cout << "Starting volume_zero_proof_test...\n";

    // ---- Part 1: the generic contract on synthetic media -------------------
    {
        const auto slab = piecewise(kSlab);
        const glm::vec3 half(10.0f);
        const auto proof = Rendering::buildVolumeZeroProof(slab, half, 4096u);
        check(proof.totalCells > 0 && proof.any(), "a clamped slab proves its empty cells");
        check(proof.provenCells < proof.totalCells, "the slab's occupied cells stay unproven");
        bool centreProven = false, edgeProven = false;
        for (uint32_t z = 0; z < proof.dims.z; ++z)
            for (uint32_t y = 0; y < proof.dims.y; ++y) {
                centreProven |= proof.proven(proof.dims.x / 2, y, z);
                edgeProven |= proof.proven(0, y, z);
            }
        check(!centreProven, "no cell on the slab's centre plane x=0 is proven");
        check(edgeProven, "cells at x=-10 (density 0) are proven");
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
    }
    {
        const auto positive = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.25,"factors":{}}]}}}]})");
        check(!Rendering::buildVolumeZeroProof(positive, glm::vec3(5.0f), 512u).any(),
              "a positive constant proves nothing");
    }
    {
        // D = t: time is never bound by the proof, so nothing may be proven.
        const auto timed = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":1,"var":"t"}}]})");
        check(!Rendering::buildVolumeZeroProof(timed, glm::vec3(5.0f), 512u).any(),
              "a time-dependent density proves nothing (fail-open)");
        const auto unknown = piecewise(R"({"input":"x","pieces":[{"mathNode":{"op":1,"var":"mystery"}}]})");
        check(!Rendering::buildVolumeZeroProof(unknown, glm::vec3(5.0f), 512u).any(),
              "an unbound variable proves nothing (fail-open)");
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
            sweep(*node->volumeDensity, proof, half, node->getIdentifier(), 4000);
        }
        check(media > 0, "the saved Zone has participating media to prove");
    }

    std::cout << "volume_zero_proof_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
