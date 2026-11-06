#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

std::unique_ptr<OntoMath::MathNode> number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}

OntoMath::Piecewise scalarExpression(double value) {
    OntoMath::Piecewise pw;
    pw.inputVariable = "x";
    OntoMath::Piecewise::Piece piece;
    piece.mathNode = std::shared_ptr<OntoMath::MathNode>(number(value).release());
    pw.pieces.push_back(std::move(piece));
    return pw;
}

int count(const std::string& source, const std::string& needle) {
    int n = 0;
    std::size_t at = 0;
    while ((at = source.find(needle, at)) != std::string::npos) {
        ++n;
        at += needle.size();
    }
    return n;
}

} // namespace

int main() {
    auto densityA = scalarExpression(0.5);
    auto densityB = scalarExpression(0.75);

    sdfwgsl::VolumeProgramInput a;
    a.densityExpr = &densityA;
    sdfwgsl::VolumeProgramInput b;
    b.densityExpr = &densityB;
    const std::vector<sdfwgsl::VolumeProgramInput> media{a, b};

    const auto legacy = sdfwgsl::composeVolumeSet(media, {}, false);
    assert(legacy.ok);
    assert(legacy.wgsl.find("ecC5bExecutionMask") == std::string::npos);

    const std::vector<sdfwgsl::VolumeExecutionFamily> ab{
        sdfwgsl::VolumeExecutionFamily::First,
        sdfwgsl::VolumeExecutionFamily::Second
    };
    const auto grouped = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {}, ab);
    assert(grouped.ok);
    assert(grouped.wgsl.find(
        "var ecC5bExecutionMask = 0u") != std::string::npos);
    assert(grouped.wgsl.find(
        "ecC5bExecutionMask |= 1u") != std::string::npos);
    assert(grouped.wgsl.find(
        "ecC5bExecutionMask |= 2u") != std::string::npos);
    assert(grouped.wgsl.find(
        "((ecC5bExecutionMask & 1u) != 0u)") != std::string::npos);
    assert(grouped.wgsl.find(
        "((ecC5bExecutionMask & 2u) != 0u)") != std::string::npos);

    // C5b-2a is one stateful fused consumer, not several independent rays.
    assert(count(grouped.wgsl, "var transmittance = 1.0;") == 1);
    assert(count(grouped.wgsl, "var integratedRadiance = vec3<f32>(0.0);") == 1);

    // The bridge must not alter V5's authoritative segmentation/sample grid.
    // Two media still contribute exactly four authored AABB events.
    assert(legacy.wgsl.find(
        "var mediumEvents: array<f32, 4>") != std::string::npos);
    assert(grouped.wgsl.find(
        "var mediumEvents: array<f32, 4>") != std::string::npos);

    // The motivating C5b chronology is expressible physically inside one
    // generated fused consumer: First -> Second -> First. Runtime placement
    // determines which existing box-event segments carry each bit.
    auto densityC = scalarExpression(0.625);
    sdfwgsl::VolumeProgramInput cMember;
    cMember.densityExpr = &densityC;
    const std::vector<sdfwgsl::VolumeProgramInput> mediaAba{a, b, cMember};
    const std::vector<sdfwgsl::VolumeExecutionFamily> aba{
        sdfwgsl::VolumeExecutionFamily::First,
        sdfwgsl::VolumeExecutionFamily::Second,
        sdfwgsl::VolumeExecutionFamily::First
    };
    const auto groupedAba = sdfwgsl::composeVolumeSet(
        mediaAba, {}, false, {}, {}, aba);
    assert(groupedAba.ok);
    assert(count(groupedAba.wgsl, "ecC5bExecutionMask |= 1u") == 2);
    assert(count(groupedAba.wgsl, "ecC5bExecutionMask |= 2u") == 1);
    assert(groupedAba.wgsl.find(
        "var mediumEvents: array<f32, 6>") != std::string::npos);
    assert(count(groupedAba.wgsl, "var transmittance = 1.0;") == 1);
    assert(count(groupedAba.wgsl, "var integratedRadiance = vec3<f32>(0.0);") == 1);

    // C5b-2b2 activity-gated grouped program: same V5 lattice, plus one
    // conservative per-pixel First-family certificate binding.
    const auto activityGated = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {}, ab, true);
    assert(activityGated.ok);
    assert(activityGated.kernelVolumeUsesFirstActivityMask);
    assert(activityGated.wgsl.find(
        "@group(0) @binding(3) var<storage, read> c5bFirstActivity") !=
        std::string::npos);
    assert(activityGated.wgsl.find(
        "let ecC5bFirstActivityWord = c5bFirstActivity[ecC5bPixel]") !=
        std::string::npos);
    assert(activityGated.wgsl.find(
        "sampleT * (32.0 / 2048.0)") != std::string::npos);
    assert(activityGated.wgsl.find(
        "ecC5bFirstActive && all(worldP >= mediumInst0") !=
        std::string::npos);
    // Second-family work is not gated by a First-family absence certificate.
    assert(activityGated.wgsl.find(
        "ecC5bFirstActive && all(worldP >= mediumInst1") ==
        std::string::npos);
    assert(count(activityGated.wgsl, "var transmittance = 1.0;") == 1);
    assert(activityGated.wgsl.find(
        "var mediumEvents: array<f32, 4>") != std::string::npos);

    const auto illegalUngroupedActivity = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {}, {}, true);
    assert(!illegalUngroupedActivity.ok);
    assert(illegalUngroupedActivity.error.find(
        "requires grouped execution") != std::string::npos);

    // Family assignment is structural compiler state.
    const std::vector<sdfwgsl::VolumeExecutionFamily> ba{
        sdfwgsl::VolumeExecutionFamily::Second,
        sdfwgsl::VolumeExecutionFamily::First
    };
    const auto reversed = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {}, ba);
    assert(reversed.ok);
    assert(reversed.wgsl != grouped.wgsl);

    // Partial/unknown grouping is not interpreted as absence.
    const auto wrongCount = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {},
        {sdfwgsl::VolumeExecutionFamily::First});
    assert(!wrongCount.ok);
    assert(wrongCount.error.find("count does not match") != std::string::npos);

    const auto unknownTag = sdfwgsl::composeVolumeSet(
        media, {}, false, {}, {},
        {sdfwgsl::VolumeExecutionFamily::First,
         sdfwgsl::VolumeExecutionFamily::Ungrouped});
    assert(!unknownTag.ok);
    assert(unknownTag.error.find("requires First/Second") != std::string::npos);

    std::cout
        << "PASS volume_cohort_gpu_lowering_test: mixed execution families "
        << "lower into one stateful V5 segment stream without changing "
        << "the authored AABB event lattice\n";
    return 0;
}
