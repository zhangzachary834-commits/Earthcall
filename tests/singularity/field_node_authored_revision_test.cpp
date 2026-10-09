// Witness for FieldNode's authored-math revision, which replaced serializing
// every authored expression to JSON on every rendered frame (~7 ms/frame in
// Northern Veil). The renderer's caches are only as truthful as this number:
// if it fails to move when the mathematics moves, Screen draws stale math.
//
// Claude Opus 5.5 · Claude Code · 2026-10-09. Zach: "Y IS IT SERIALIZING INTO
// JSON AT ALL". See FieldNode::verifiedAuthoredMathRevision.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"

#include <iostream>
#include <memory>
#include <string>

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

const char* kDensityA = R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}}]})";
const char* kDensityB = R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.75,"factors":{}}]}}}]})";

std::shared_ptr<geom::FieldNode> mist(const std::string& id) {
    nlohmann::json j;
    j["id"] = id;
    j["volumeDensity"] = nlohmann::json::parse(kDensityA);
    j["volumeEmission"] = nlohmann::json::parse(kDensityA);
    return geom::FieldNode::fromJson(j);
}

uint64_t densityRevisionOf(const geom::FieldNode& node) {
    Rendering::VolumeDensityBinding b;
    if (!Rendering::readVolumeDensity(node, 0.0, 0.0, b)) return 0;
    return b.densityRevision;
}

// Enough reads for the round-robin verifier to visit every channel once.
uint64_t settle(const geom::FieldNode& node) {
    uint64_t r = 0;
    for (int i = 0; i < 16; ++i) r = densityRevisionOf(node);
    return r;
}

} // namespace

int main() {
    std::cout << "Starting field_node_authored_revision_test...\n";

    // 1. Unchanged mathematics keeps one revision, so renderer caches hit.
    auto node = mist("mist.revision.a");
    const uint64_t first = settle(*node);
    check(first != 0, "a present density channel projects a non-zero revision");
    check(densityRevisionOf(*node) == first, "re-reading unchanged math keeps the same revision");

    Rendering::VolumeDensityBinding b;
    Rendering::readVolumeDensity(*node, 0.0, 0.0, b);
    check(b.emissionRevision != 0 && b.emissionRevision != b.densityRevision,
          "each present channel gets its own non-zero revision");
    check(b.extinctionRevision == 0 && b.scatteringRevision == 0 && b.occluderRevision == 0,
          "absent channels still project revision 0");

    // 2. A Law/Person write through the registered property path moves it.
    Property* density = node->findProperty("volume.density.ast");
    check(density != nullptr, "volume.density.ast is a registered property");
    if (density) {
        check(density->setValue(PropertyValue(std::string(kDensityB))), "bridge accepts a new density tree");
        const uint64_t afterWrite = densityRevisionOf(*node);
        check(afterWrite != first, "a bridge write changes the density revision");

        // An identical rewrite (a Law re-asserting each frame) is not a change.
        check(density->setValue(PropertyValue(std::string(kDensityB))), "bridge accepts an identical tree");
        check(densityRevisionOf(*node) == afterWrite, "an identical rewrite keeps the revision");

        check(!density->setValue(PropertyValue(std::string("{not json"))), "malformed tree is refused");
        check(densityRevisionOf(*node) == afterWrite, "a refused write keeps the revision");
    }

    // 3. Load (applyJson) moves it.
    {
        const uint64_t before = settle(*node);
        node->applyJson(node->toJson());
        check(densityRevisionOf(*node) != before, "applyJson is a write and changes the revision");
    }

    // 4. A writer that assigns in place and forgets noteAuthoredMathWritten()
    //    is caught by the verifier within one round of channels and healed.
    {
        const uint64_t before = settle(*node);
        *node->volumeDensity = OntoMath::Piecewise::fromJson(nlohmann::json::parse(kDensityA));
        uint64_t healed = before;
        int reads = 0;
        while (healed == before && reads < 32) { healed = densityRevisionOf(*node); ++reads; }
        check(healed != before, "an unrevisioned in-place write is detected and the revision bumped");
        check(reads <= 10, "detection takes at most one round of the 10 verified channels");
        const uint64_t stable = settle(*node);
        check(densityRevisionOf(*node) == stable, "after healing, unchanged math is stable again");
    }

    // 5. The MCP-style path: in-place assignment plus an explicit bump.
    {
        const uint64_t before = settle(*node);
        *node->volumeEmission = OntoMath::Piecewise::fromJson(nlohmann::json::parse(kDensityB));
        node->noteAuthoredMathWritten();
        Rendering::VolumeDensityBinding after;
        Rendering::readVolumeDensity(*node, 0.0, 0.0, after);
        check(after.densityRevision != before, "noteAuthoredMathWritten moves the revision at once");
    }

    // 6. A node rebuilt in place of a destroyed one never repeats a revision
    //    (the SourceRho producer-rebinding hazard: same address, same number).
    {
        const uint64_t old = settle(*node);
        node.reset();
        auto reborn = mist("mist.revision.a");
        check(settle(*reborn) != old, "a rebuilt node with identical math gets a never-seen revision");
    }

    // 7. Light-source channels share the mechanism (EngineRender reads them).
    {
        auto source = std::make_shared<geom::FieldNode>("light.revision.source");
        const uint64_t before = source->verifiedAuthoredMathRevision();
        Property* ast = source->findProperty("field.ast");
        check(ast && ast->setValue(PropertyValue(std::string(kDensityA))), "field.ast accepts a radiance tree");
        check(source->verifiedAuthoredMathRevision() != before, "a radiance write changes the source revision");
    }

    std::cout << "field_node_authored_revision_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
