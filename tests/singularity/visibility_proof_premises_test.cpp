#include "Singularity/Screen/WebGPU/VisibilityProofPremises.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <string>

using namespace sdfwgsl;

static FieldVmDependencySnapshot dependencySnapshot(
    float radius, const std::string& authority) {
    FieldVmDependencyManifest manifest;
    manifest.dependencies.push_back(
        {FieldVmDependencyKind::PropertyPath, "radius"});

    std::map<std::string, PropertyValue> values{
        {"radius", PropertyValue(radius)}
    };
    std::map<std::string, std::string> authorities{
        {"radius", authority}
    };
    FieldVmDependencyContext context;
    context.propertyBindings = &values;
    context.propertyBindingAuthorities = &authorities;
    return snapshotFieldVmDependencies(manifest, context);
}

static VisibilityProofPremises basePremises() {
    VisibilityProofPremises p;
    p.executorFrame = VisibilityExecutorFrame::SdfRaymarch;
    p.scope = VisibilityProofScope::CompleteScene;
    p.admittedProgramIdentity = "field-vm-program:v17:ast-rev-42";
    p.receiverEvidenceIdentity = "receiver:sdf-cohort:7:rev-11";
    p.sceneEvidenceIdentity = "scene-blockers:rev-991";
    p.traversalPolicyIdentity =
        "complete-scene-march:v3:maxSteps=24:eps=f32:358637bd:gradient=v1";
    p.dependencySnapshot = dependencySnapshot(0.5f, "material.radius@rev18");
    p.sourcePosition = glm::vec3(1.0f, 2.0f, 3.0f);
    p.receiverPosition = glm::vec3(-2.0f, 0.25f, 4.0f);
    return p;
}

static void expectDifferent(
    const VisibilityProofPremiseIdentity& a,
    const VisibilityProofPremiseIdentity& b,
    const char* why) {
    if (!a.valid || !b.valid || a.identity == b.identity) {
        std::fprintf(stderr, "FAIL: %s\n", why);
        std::abort();
    }
}

int main() {
    const auto base = basePremises();
    const auto a = makeVisibilityProofPremiseIdentity(base);
    const auto a2 = makeVisibilityProofPremiseIdentity(base);
    assert(a.valid && a2.valid && !a.identity.empty());
    assert(a.identity == a2.identity);

    {
        auto changed = base;
        changed.dependencySnapshot =
            dependencySnapshot(0.5f, "material.radius@rev19");
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "same f32 value from a different authority reused the old proof premise");
    }
    {
        auto changed = base;
        changed.dependencySnapshot =
            dependencySnapshot(0.9f, "material.radius@rev18");
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "changed dependency execution bits reused the old proof premise");
    }
    {
        auto changed = base;
        changed.sourcePosition.x += 0.125f;
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "moved source reused the old visibility query premise");
    }
    {
        auto changed = base;
        changed.receiverPosition.z -= 0.25f;
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "moved receiver reused the old visibility query premise");
    }
    {
        auto changed = base;
        changed.sceneEvidenceIdentity = "scene-blockers:rev-992";
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "changed blocker evidence reused the old proof premise");
    }
    {
        auto changed = base;
        changed.traversalPolicyIdentity =
            "complete-scene-march:v4:maxSteps=32:eps=f32:358637bd:gradient=v1";
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "changed numerical traversal policy reused the old proof premise");
    }
    {
        auto changed = base;
        changed.admittedProgramIdentity = "field-vm-program:v17:ast-rev-43";
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "changed admitted program reused the old proof premise");
    }
    {
        auto changed = base;
        changed.receiverEvidenceIdentity = "receiver:sdf-cohort:8:rev-11";
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "changed receiver evidence reused the old proof premise");
    }
    {
        auto changed = base;
        changed.executorFrame = VisibilityExecutorFrame::MeshRaster;
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "mesh and SDF execution frames aliased without an explicit reconciliation");
    }
    {
        auto changed = base;
        changed.scope = VisibilityProofScope::LocalReceiverGeometry;
        expectDifferent(
            a, makeVisibilityProofPremiseIdentity(changed),
            "local and CompleteScene proof scopes aliased");
    }

    // Exact f32 identity is deliberately conservative: signed-zero differences
    // are not normalized by Parallax because this layer does not own numerical
    // equivalence proofs.
    {
        auto changed = base;
        changed.sourcePosition.y = -0.0f;
        auto positive = base;
        positive.sourcePosition.y = +0.0f;
        expectDifferent(
            makeVisibilityProofPremiseIdentity(positive),
            makeVisibilityProofPremiseIdentity(changed),
            "signed-zero execution bits were normalized without authority");
    }

    // Invalid subordinate evidence cannot be promoted into a proof premise.
    {
        auto changed = base;
        changed.dependencySnapshot = {};
        const auto invalid = makeVisibilityProofPremiseIdentity(changed);
        assert(!invalid.valid && invalid.identity.empty());
    }
    {
        auto changed = base;
        changed.sceneEvidenceIdentity.clear();
        const auto invalid = makeVisibilityProofPremiseIdentity(changed);
        assert(!invalid.valid && invalid.identity.empty());
    }
    {
        auto changed = base;
        changed.sourcePosition.x = std::numeric_limits<float>::infinity();
        const auto invalid = makeVisibilityProofPremiseIdentity(changed);
        assert(!invalid.valid && !invalid.nonFinite.empty());
    }

    std::puts("visibility proof premise identity: PASS");
    return 0;
}
