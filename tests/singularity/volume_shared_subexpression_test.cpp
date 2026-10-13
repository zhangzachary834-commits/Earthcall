// CPU codegen witness for cross-channel shared subexpressions in volume media
// (SdfWgsl planVolumeSharing / emitVolumeSharedPrelude). A subtree shared by a
// medium's channels is computed once per sample; this pins that the plan finds
// it, that the shader then evaluates noise once, that the CPU parameter
// collector replays the same constants, and -- the silent failure -- that a
// numeric edit which makes two channels stop matching changes the structure
// signature, so the renderer recompiles instead of sharing a stale value.
//
// Claude Opus 5.5 · Claude Code · 2026-10-09. Zach asked for the media's
// equations to be unified rather than re-evaluated, and approved this on the
// measured numbers (~1.8-2x faster on Northern Veil; GPU float rounding moves
// one pixel per frame by one 8-bit level).
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "support/test_harness.hpp"

// Continued by Codex / GPT-6.1 Sol / session 01a122d7 / 2026-10-10:
// coordinate/Timeline isolation, all-noise locals, source/occluder slot order.
// Native consumer witness: webgpu_volume_shared_subexpression_test.cpp.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << "  " << (condition ? "ok" : "FAILED") << ": " << description << std::endl;
}

int count(const std::string& s, const std::string& t) {
    int n = 0;
    for (std::size_t p = 0; (p = s.find(t, p)) != std::string::npos; p += t.size()) ++n;
    return n;
}

std::string signature(const geom::FieldNode& f) {
    return sdfwgsl::inspectVolumeSharing(f.volumeDensity.get(), f.volumeExtinction.get(),
                                         f.volumeScattering.get(), f.volumeChroma.get(),
                                         f.volumePhase.get(), f.volumeEmission.get());
}

// Scale the first ScalarLeaf coefficient found under `node` (a numeric-only
// edit: no operator changes, so the layout structure alone would not notice).
bool nudgeFirstConstant(OntoMath::MathNode& node) {
    if (node.op == OntoMath::MathNode::Op::ScalarLeaf && !node.scalarForm.terms.empty()) {
        node.scalarForm.terms[0].coefficient *= 1.5;
        return true;
    }
    for (auto& c : node.children)
        if (c && nudgeFirstConstant(*c)) return true;
    return false;
}


std::unique_ptr<OntoMath::MathNode> variable(const char* name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}
std::unique_ptr<OntoMath::MathNode> noise() {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::Noise;
    n->children.push_back(variable("p"));
    return n;
}
OntoMath::Piecewise expression(std::shared_ptr<OntoMath::MathNode> node) {
    OntoMath::Piecewise pw;
    pw.inputVariable = "x";
    OntoMath::Piecewise::Piece piece;
    piece.mathNode = std::move(node);
    pw.pieces.push_back(std::move(piece));
    return pw;
}
std::string function(const std::string& wgsl, const std::string& name) {
    const auto at = wgsl.find("fn " + name + "(");
    if (at == std::string::npos) return "";
    const auto end = wgsl.find("\nfn ", at + 3);
    return wgsl.substr(at, end == std::string::npos ? end : end - at);
}
std::unique_ptr<OntoMath::MathNode> number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}
std::unique_ptr<OntoMath::MathNode> scale(double value, std::unique_ptr<OntoMath::MathNode> child) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::Scale;
    n->children.push_back(number(value));
    n->children.push_back(std::move(child));
    return n;
}
void scopeWitnesses() {
    auto density = expression(noise());
    auto extinction = expression(noise());
    auto source = expression(noise());
    auto prog = sdfwgsl::compileVolume(&density, &extinction, nullptr, nullptr, nullptr,
                                      nullptr, nullptr, &source, nullptr, nullptr);
    check(prog.ok, "equal medium and source noise expressions compile");
    check(function(prog.wgsl, "lightRadianceEval").find("cnoise3(") != std::string::npos,
          "source evaluates its own coordinates instead of a medium private slot");
    auto shifted = std::make_unique<OntoMath::MathNode>();
    shifted->op = OntoMath::MathNode::Op::Scale;
    auto two = std::make_unique<OntoMath::MathNode>();
    two->op = OntoMath::MathNode::Op::ScalarLeaf;
    two->scalarForm.terms.push_back(OntoMath::Term(2.0));
    shifted->children.push_back(std::move(two));
    shifted->children.push_back(variable("p"));
    auto rebound = std::make_unique<OntoMath::MathNode>();
    rebound->op = OntoMath::MathNode::Op::SDF;
    rebound->children.push_back(noise());
    rebound->children.push_back(std::move(shifted));
    auto scattering = expression(std::move(rebound));
    prog = sdfwgsl::compileVolume(&density, &extinction, &scattering, nullptr, nullptr,
                                 nullptr, nullptr, nullptr, nullptr, nullptr);
    check(prog.ok, "rebound SDF noise compiles");
    check(function(prog.wgsl, "volumeScatteringEval").find("cnoise3(") != std::string::npos,
          "SDF noise at 2*p does not reuse noise at p");

    auto gradient = std::make_unique<OntoMath::MathNode>();
    gradient->op = OntoMath::MathNode::Op::Gradient;
    gradient->children.push_back(noise());
    gradient->children.push_back(variable("p"));
    auto chroma = expression(std::move(gradient));
    prog = sdfwgsl::compileVolume(&density, &extinction, nullptr, &chroma, nullptr,
                                 nullptr, nullptr, nullptr, nullptr, nullptr);
    check(prog.ok && count(function(prog.wgsl, "volumeChromaEval"), "cnoise3(") == 6,
          "Gradient retains all six evaluations at its offset points");
    // A medium's matching subtree can also appear in an occluder, evaluated
    // at many shadow-ray points rather than at the current medium sample.
    auto occluder = geom::makeImplicit(noise());
    prog = sdfwgsl::compileVolume(&density, &extinction, nullptr, nullptr, nullptr,
                                 nullptr, &occluder, nullptr, nullptr, nullptr);
    check(prog.ok, "matching occluder expression compiles");
    check(function(prog.wgsl, "volumeSdfEval").find("cnoise3(") != std::string::npos,
          "occluder evaluates its own shadow-ray point");

    source = expression(scale(0.25, noise()));
    occluder = geom::makeImplicit(scale(0.75, noise()));
    prog = sdfwgsl::compileVolume(&density, &extinction, nullptr, nullptr, nullptr,
                                 nullptr, &occluder, &source, nullptr, nullptr);
    const auto params = sdfwgsl::collectVolumeParams(&density, &extinction, nullptr, nullptr,
                                                    nullptr, nullptr, &occluder, &source,
                                                    nullptr, nullptr);
    check(prog.ok && params.ok && prog.params == params.values,
          "source and occluder constants occupy the compiled slots on refresh");
    check(function(prog.wgsl, "volumeSharedG0").find("let densityFactor0 = cnoise3(") != std::string::npos,
          "an all-noise shared expression declares its referenced local");

    const auto pair = [](OntoMath::MathNode::Op code, double frequency) {
        auto n = std::make_unique<OntoMath::MathNode>(); n->op = code;
        auto a = std::make_unique<OntoMath::MathNode>(); a->op = OntoMath::MathNode::Op::Noise;
        a->children.push_back(scale(frequency, variable("p")));
        auto b = std::make_unique<OntoMath::MathNode>(); b->op = OntoMath::MathNode::Op::Noise;
        b->children.push_back(scale(0.75, variable("p")));
        n->children.push_back(std::move(a)); n->children.push_back(std::move(b));
        return expression(std::move(n));
    };
    auto sum = pair(OntoMath::MathNode::Op::Add, 0.25);
    auto product = pair(OntoMath::MathNode::Op::Scale, 0.25);
    const auto before = sdfwgsl::compileVolume(&sum, &product, nullptr, nullptr, nullptr,
                                               nullptr, nullptr, nullptr, nullptr, nullptr);
    const auto beforeSig = sdfwgsl::inspectVolumeSharing(&sum, &product, nullptr, nullptr, nullptr, nullptr);
    sum = pair(OntoMath::MathNode::Op::Add, 2.0);
    product = pair(OntoMath::MathNode::Op::Scale, 2.0);
    const auto after = sdfwgsl::compileVolume(&sum, &product, nullptr, nullptr, nullptr,
                                              nullptr, nullptr, nullptr, nullptr, nullptr);
    check(before.ok && after.ok && before.wgsl == after.wgsl && before.params != after.params &&
              beforeSig == sdfwgsl::inspectVolumeSharing(&sum, &product, nullptr, nullptr, nullptr, nullptr),
          "matched numeric edits retain slot order even when constants reverse their sort order");

}

} // namespace

int main() {
    std::cout << "Starting volume_shared_subexpression_test...\n";
    scopeWitnesses();
    const std::string path = TestSupport::resolveRealWorldPath("saves/zones/Northern Veil/zone.ecform");
    if (!std::filesystem::exists(path)) {
        std::cout << "SKIP: Northern Veil save not present\n";
        return 0;
    }
    std::ifstream in(path, std::ios::binary);
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto zone = nlohmann::json::parse(
        nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());

    int media = 0;
    for (const auto& fj : zone.at("spatialFields")) {
        auto f = geom::FieldNode::fromJson(fj);
        if (!f || !f->volumeDensity || f->volumeDensity->pieces.empty()) continue;
        ++media;
        const std::string id = f->getIdentifier();
        const std::string sig = signature(*f);
        std::cout << "       " << id << " plan: " << sig;

        // 1. The shape shared by density (c0), extinction (c1), scattering (c2), emission (c5).
        check(sig.find("c0@") != std::string::npos && sig.find("c1@") != std::string::npos &&
                  sig.find("c2@") != std::string::npos && sig.find("c5@") != std::string::npos,
              id + ": the curtain shape is shared by density, extinction, scattering and emission");

        // 2. Noise is evaluated once in the generated shader.
        const auto prog = sdfwgsl::compileVolume(f->volumeDensity.get(), f->volumeExtinction.get(),
                                                 f->volumeScattering.get(), f->volumeChroma.get(),
                                                 f->volumePhase.get(), f->volumeEmission.get(),
                                                 nullptr, nullptr, nullptr, nullptr);
        check(prog.ok, id + ": the medium compiles");
        // One cnoise3 definition plus one call site.
        check(count(prog.wgsl, "cnoise3(") == 2, id + ": the shader evaluates Perlin noise once");
        check(count(prog.wgsl, "volumeSharedEval(p);") == 1, id + ": shared values are filled once per sample");

        // 3. CPU parameter collection replays the shader's constants exactly.
        const auto params = sdfwgsl::collectVolumeParams(f->volumeDensity.get(), f->volumeExtinction.get(),
                                                         f->volumeScattering.get(), f->volumeChroma.get(),
                                                         f->volumePhase.get(), f->volumeEmission.get(),
                                                         nullptr, nullptr, nullptr, nullptr);
        check(params.ok && params.values == prog.params,
              id + ": collectVolumeParams matches the compiled parameter block");

        // 4. A numeric edit that breaks the match must change the signature.
        // Extinction is 0.10 * shape (plan path 0/1). Editing the 0.10 keeps the
        // shape identical, so sharing correctly survives it; editing a constant
        // inside extinction's copy of the shape must end the sharing.
        auto& extinction = *f->volumeExtinction->pieces.front().mathNode;
        check(extinction.children.size() == 2 && extinction.children[0] &&
                  nudgeFirstConstant(*extinction.children[0]),
              id + ": extinction's own 0.10 factor can be edited");
        check(signature(*f) == sig, id + ": editing extinction's own factor keeps the shape shared");
        check(extinction.children[1] && nudgeFirstConstant(*extinction.children[1]),
              id + ": extinction's copy of the shape has a constant to edit");
        const std::string after = signature(*f);
        check(after != sig, id + ": extinction diverging from the shared shape changes the sharing signature");
        check(after.find("c1@") == std::string::npos, id + ": extinction no longer shares the stale value");
    }
    check(media > 0, "the saved Zone has participating media");

    std::cout << "volume_shared_subexpression_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
