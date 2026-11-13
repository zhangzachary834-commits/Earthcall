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

    // 3. Load (applyJson) moves a channel exactly when its content changes
    //    (2026-10-12: per-channel deltas; identical reloads keep every cache).
    {
        const uint64_t before = settle(*node);
        node->applyJson(node->toJson());
        check(densityRevisionOf(*node) == before, "re-applying identical JSON keeps the revision");
        auto j = node->toJson();
        j["volumeDensity"] = nlohmann::json::parse(kDensityA);
        node->applyJson(j);
        check(densityRevisionOf(*node) != before, "applyJson with changed density changes its revision");
        j["volumeDensity"] = nlohmann::json::parse(kDensityB);
        node->applyJson(j);   // back to the content case 4 expects
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

    // 8. Per-channel independence (Formation Rete Incremental Maintenance §2:
    //    "semantic deltas, not global revision panic"). Editing one channel
    //    must not invalidate caches that read another.
    {
        auto medium = mist("mist.revision.channels");
        Rendering::VolumeDensityBinding b0, b1, b2, b3, b4;
        settle(*medium);
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b0);

        Property* emission = medium->findProperty("volume.emission.ast");
        check(emission && emission->setValue(PropertyValue(std::string(kDensityB))),
              "emission bridge accepts a new tree");
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b1);
        check(b1.emissionRevision != b0.emissionRevision, "an emission edit moves the emission revision");
        check(b1.densityRevision == b0.densityRevision,
              "an emission edit leaves the density revision (and its proof) alone");

        Property* density = medium->findProperty("volume.density.ast");
        check(density && density->setValue(PropertyValue(std::string(kDensityB))),
              "density bridge accepts a new tree");
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b2);
        check(b2.densityRevision != b1.densityRevision, "a density edit moves the density revision");
        check(b2.emissionRevision == b1.emissionRevision, "a density edit leaves the emission revision alone");

        // An unrevisioned in-place emission write heals only emission.
        settle(*medium);
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b3);
        *medium->volumeEmission = OntoMath::Piecewise::fromJson(nlohmann::json::parse(kDensityA));
        Rendering::VolumeDensityBinding healed = b3;
        for (int i = 0; i < 32 && healed.emissionRevision == b3.emissionRevision; ++i)
            Rendering::readVolumeDensity(*medium, 0.0, 0.0, healed);
        check(healed.emissionRevision != b3.emissionRevision, "the verifier heals the unrevisioned emission write");
        check(healed.densityRevision == b3.densityRevision, "healing emission leaves density alone");

        // A reload that changes only emission moves only emission.
        settle(*medium);
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b4);
        auto j = medium->toJson();
        j["volumeEmission"] = nlohmann::json::parse(kDensityB);
        medium->applyJson(j);
        Rendering::VolumeDensityBinding b5;
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b5);
        check(b5.emissionRevision != b4.emissionRevision && b5.densityRevision == b4.densityRevision,
              "applyJson changing only emission moves only emission");

        // A writer that cannot name its channel still moves every channel.
        medium->noteAuthoredMathWritten();
        Rendering::VolumeDensityBinding b6;
        Rendering::readVolumeDensity(*medium, 0.0, 0.0, b6);
        check(b6.densityRevision != b5.densityRevision && b6.emissionRevision != b5.emissionRevision,
              "noteAuthoredMathWritten still moves every channel (conservative)");
    }

    std::cout << "field_node_authored_revision_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
