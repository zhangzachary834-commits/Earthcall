// Codex / GPT-6.1 Sol / session 01a122d7 / 2026-10-10.
// From Zach's shared-equation goal; continues Opus 5.5's volume sharing.
// Native witness for source Timeline isolation and SDF coordinate rebinding.
// Synthetic in-memory fields only: no save loading, writing, or Engine boot.
// Readback plumbing adapted from webgpu_volume_unified_quadrature_test.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <memory>
#include <limits>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::printf("  %s: %s\n", condition ? "ok" : "FAILED", description.c_str());
}

struct MapResult { bool done = false; bool ok = false; };
void mapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapResult*>(u);
    r->ok = s == WGPUMapAsyncStatus_Success;
    r->done = true;
}

struct Image { std::vector<uint8_t> rgb; uint64_t lit = 0; uint32_t compiles = 0; size_t parameterUploads = 0; size_t proofUploads = 0; uint32_t lowerings = 0, repairedNodes = 0; };


using Node = std::unique_ptr<OntoMath::MathNode>;
Node number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}
Node variable(const char* name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}
Node timeCos(double coefficient, double frequency) {
    auto n = number(coefficient);
    OntoMath::TransFactor factor;
    factor.kind = OntoMath::TransFactor::Kind::Cos;
    factor.variable = OntoMath::kTimeVar; factor.scale = frequency;
    n->scalarForm.terms.front().trans.push_back(factor);
    return n;
}
Node op(OntoMath::MathNode::Op code, Node a, Node b = {}) {
    auto n = std::make_unique<OntoMath::MathNode>(); n->op = code;
    n->children.push_back(std::move(a));
    if (b) n->children.push_back(std::move(b));
    return n;
}
Node root(double pointScale, bool explicitOne) {
    auto point = variable("p");
    if (pointScale != 1 || explicitOne)
        point = op(OntoMath::MathNode::Op::Scale, number(pointScale), std::move(point));
    auto timeVector = std::make_unique<OntoMath::MathNode>();
    timeVector->op = OntoMath::MathNode::Op::VectorConstruct;
    for (int i = 0; i < 3; ++i)
        timeVector->children.push_back(op(OntoMath::MathNode::Op::Scale, number(0.3), variable("t")));
    auto noise = op(OntoMath::MathNode::Op::Noise,
                    op(OntoMath::MathNode::Op::Add, std::move(point), std::move(timeVector)));
    return op(OntoMath::MathNode::Op::Add, number(0.8),
              op(OntoMath::MathNode::Op::Scale, number(0.1), std::move(noise)));
}
OntoMath::Piecewise piece(Node node) {
    OntoMath::Piecewise pw; pw.inputVariable = "x";
    OntoMath::Piecewise::Piece p; p.mathNode = std::move(node);
    pw.pieces.push_back(std::move(p)); return pw;
}
OntoMath::Piecewise field(double pointScale, bool explicitOne) { return piece(root(pointScale, explicitOne)); }

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_shared_subexpression_test...\n");

    std::vector<std::shared_ptr<geom::FieldNode>> media;
    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("SKIP: no WebGPU device\n"); return 0; }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }
    setCurrentRenderer(&renderer);

    const uint32_t width = 160, height = 120;
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {width, height, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);
    const uint32_t stride = (width * 4 + 255) & ~255u;
    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bd.size = static_cast<uint64_t>(stride) * height;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &bd);

    Rendering::RadianceSourceBinding source;
    source.producerId = "test.shared-source";
    source.position = glm::vec3(4, 5, -4);
    source.ambientRadiance = glm::vec3(0.2f);
    source.diffuseRadiance = glm::vec3(2.0f);
    source.specularRadiance = glm::vec3(1.0f);
    source.coefficients = glm::vec4(2.0f, 0.2f, 0.8f, 1.0f);
    source.enabled = true;
    renderer.setLight(source.position, source.ambientRadiance, source.diffuseRadiance, source.specularRadiance);
    renderer.setLightingEnabled(true);
    renderer.setVolumeZeroProofEnabled(false);


    auto render = [&](const glm::vec3& eye, const glm::vec3& look, double t, int samples,
                      glm::vec4 background = glm::vec4(0, 0, 0, 1)) {
        renderer.setRadianceSources({source}, source.radianceRevision);
        renderer.setCamera(glm::lookAt(eye, look, glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
        renderer.setVolumeSamplesPerChord(samples);
        auto drawOnce = [&] {
            std::vector<Rendering::VolumeDensityBinding> bindings;
            std::string identity;
            for (const auto& f : media) {
                Rendering::VolumeDensityBinding b;
                if (Rendering::readVolumeDensity(*f, t, 1.0 / 60.0, b)) {
                    Rendering::appendVolumeSetIdentity(identity, b.producerId, b);
                    bindings.push_back(b);
                }
            }
            renderer.setVolumeDensitySources(std::move(bindings), std::hash<std::string>{}(identity));
            renderer.setModel(glm::mat4(1));
            renderer.beginFrameOffscreen(view, width, height, background);
            renderer.composeVolumes();
            renderer.endFrame();
        };
        drawOnce();
        Image image;
        image.compiles = renderer.frameStats().volumeProgramCompiles;
        image.parameterUploads = renderer.frameStats().volumeParameterBytesUploaded;
        image.proofUploads = renderer.frameStats().volumeProofBytesUploaded;
        image.lowerings = renderer.frameStats().volumeLowerings;
        image.repairedNodes = renderer.frameStats().volumeRepairedNodes;
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {}; src.texture = target; src.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst = {}; dst.buffer = readback;
        dst.layout.bytesPerRow = stride; dst.layout.rowsPerImage = height;
        WGPUExtent3D ext = {width, height, 1};
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd); wgpuCommandEncoderRelease(enc);
        MapResult r; WGPUBufferMapCallbackInfo cb = {};
        cb.mode = WGPUCallbackMode_AllowProcessEvents; cb.callback = mapped; cb.userdata1 = &r;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, bd.size, cb);
        while (!r.done) wgpuInstanceProcessEvents(gpu.instance);
        if (!r.ok) { std::fprintf(stderr, "FAIL: native readback\n"); std::exit(1); }
        const auto* px = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(readback, 0, bd.size));
        image.rgb.reserve(size_t(width) * height * 3);
        for (uint32_t y = 0; y < height; ++y)
            for (uint32_t x = 0; x < width; ++x) {
                const size_t at = size_t(y) * stride + x * 4;
                for (int c = 0; c < 4; ++c) image.rgb.push_back(px[at + c]);
                image.lit += (px[at] + px[at + 1] + px[at + 2]) > 6;
            }
        wgpuBufferUnmap(readback);
        return image;
    };


    const glm::vec3 eye(0, 0, -5), look(0, 0, 0);
    uint64_t revision = 1;
    for (int mediumCount : {1, 2}) {
        std::vector<uint8_t> firstSourceClockImage;
        for (double sourceTime : {0.0, 7.25}) {
            media.clear();
            for (int i = 0; i < mediumCount; ++i) {
                auto f = std::make_shared<geom::FieldNode>("test.shared-medium-" + std::to_string(i));
                f->scale = glm::vec3(2.0f);
                f->origin = glm::vec3(float(i) * 0.4f, 0, 0);
                *f->volumeDensity = field(1.0, false);
                *f->volumeExtinction = field(1.0, false);
                // f(2*p) authored through a coordinate binder.
                auto rebound = op(OntoMath::MathNode::Op::SDF, root(1.0, false),
                                  op(OntoMath::MathNode::Op::Scale, number(2), variable("p")));
                *f->volumeScattering = piece(std::move(rebound));
                f->noteAuthoredMathWritten();
                media.push_back(f);
            }
            auto authoredSource = field(1.0, false);
            source.radianceExpr = &authoredSource;
            source.radianceRevision = ++revision;
            source.temporalCoordinate = sourceTime;
            const auto original = render(eye, look, 2.5, 96);
            if (sourceTime == 0.0) firstSourceClockImage = original.rgb;
            else check(original.rgb != firstSourceClockImage,
                       "changing the source clock actually changes displayed radiance");
            // Independent equivalent lowering: f(2*p) is written directly,
            // and the source uses noise(1*p + t). Multiplication by 1 is exact
            // in f32 but prevents accidental textual equality with a medium.
            for (auto& f : media) {
                *f->volumeScattering = field(2.0, false);
                f->noteAuthoredMathWritten();
            }
            auto referenceSource = field(1.0, true);
            source.radianceExpr = &referenceSource;
            source.radianceRevision = ++revision;
            const auto reference = render(eye, look, 2.5, 96);
            int maxError = 0;
            for (std::size_t i = 0; i < original.rgb.size(); ++i)
                maxError = std::max(maxError, std::abs(int(original.rgb[i]) - int(reference.rgb[i])));
            std::printf("       %d media source t=%.2f: max RGBA error %d, lit %llu\n",
                        mediumCount, sourceTime, maxError, (unsigned long long)original.lit);
            check(original.lit > 0 && reference.lit > 0, "scattering fixture is visible");
            check(maxError <= 1, "source clock and SDF point match independent inline mathematics");
            const auto stable = render(eye, look, 2.5, 96);
            check(stable.compiles == 0 && stable.rgb == reference.rgb,
                  "unchanged fields reuse the compiled program and image");
            check(stable.parameterUploads == 0 && stable.proofUploads == 0,
                  "unchanged volume parameters remain resident without uploads");
            const auto advancing = render(eye, look, 3.5, 96);
            check(advancing.compiles == 0 && advancing.parameterUploads == 0 && advancing.proofUploads == 0,
                  "advancing medium time streams instance data without compilation or parameter uploads");
            if (mediumCount == 2 && sourceTime == 7.25) {
                for (auto* channel : {media[0]->volumeDensity.get(), media[0]->volumeExtinction.get()})
                    channel->pieces.front().mathNode->children[0]->scalarForm.terms[0].coefficient = 0.9;
                media[0]->noteAuthoredMathWritten();
                const auto edited = render(eye, look, 2.5, 96);
                check(edited.compiles == 0 && edited.lowerings == 0 && edited.repairedNodes > 0 &&
                      edited.parameterUploads > 0 && edited.proofUploads == 0 && edited.rgb != reference.rgb,
                      "a sharing-preserving numeric edit repairs slots without WGSL or unrelated-medium lowering");
                *media[0]->volumeExtinction = piece(op(OntoMath::MathNode::Op::Add,
                    root(1.0, false), number(0.04)));
                media[0]->noteAuthoredMathWritten();
                const auto split = render(eye, look, 2.5, 96);
                check(split.lowerings == 1 && split.compiles > 0,
                      "one structural/sharing edit lowers only its affected medium");
                renderer.reloadShaders();
                const auto splitFresh = render(eye, look, 2.5, 96);
                check(split.rgb == splitFresh.rgb,
                      "incrementally repaired sharing produces exactly the freshly lowered image");
                std::swap(media[0], media[1]);
                const auto reordered = render(eye, look, 2.5, 96);
                renderer.reloadShaders();
                check(reordered.rgb == render(eye, look, 2.5, 96).rgb,
                      "media reordering rebuilds offsets without stale resident values");
                media.pop_back();
                const auto removed = render(eye, look, 2.5, 96);
                renderer.reloadShaders();
                check(removed.rgb == render(eye, look, 2.5, 96).rgb,
                      "media removal rebuilds the single-medium resident layout");
                renderer.shutdown();
                wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
                wgpu::Device replacement;
                check(replacement.init(), "replacement native device acquired");
                wgpuQueueRelease(gpu.queue); wgpuDeviceRelease(gpu.device);
                wgpuAdapterRelease(gpu.adapter); wgpuInstanceRelease(gpu.instance);
                gpu = replacement;
                check(renderer.init(gpu), "renderer admits the replacement device");
                target = wgpuDeviceCreateTexture(gpu.device, &td);
                view = wgpuTextureCreateView(target, nullptr);
                readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
                renderer.setVolumeZeroProofEnabled(false);
                check(removed.rgb == render(eye, look, 2.5, 96).rgb,
                      "device recreation cannot retain old device-bound buffers or programs");
            }
        }
    }
    // Independent source and medium clocks for the GPU preparation pass.
    // Compare prepared cos(t) against the same cos evaluated in a point-bound
    // expression, which is deliberately ineligible for preparation.
    renderer.reloadShaders();
    for (int count : {1, 2}) {
        media.clear();
        for (int i = 0; i < count; ++i) {
            auto f = std::make_shared<geom::FieldNode>("test.time-preparation-" + std::to_string(i));
            f->scale = glm::vec3(2.0f);
            *f->volumeDensity = piece(number(0.5));
            *f->volumeExtinction = piece(number(0.2));
            *f->volumeScattering = piece(op(OntoMath::MathNode::Op::Add, number(0.3), timeCos(0.1, 0.32)));
            f->noteAuthoredMathWritten(); media.push_back(f);
        }
        auto preparedSource = piece(op(OntoMath::MathNode::Op::Add, number(0.6), timeCos(0.2, 0.23)));
        source.enabled = true; source.radianceExpr = &preparedSource;
        source.temporalCoordinate = 3.0; source.radianceRevision = ++revision;
        for (double time : {0.0, 7.25}) {
            const auto prepared = render(eye, look, time, 96);
            for (auto& f : media) {
                auto inlineCos = timeCos(0.1, 0.32);
                OntoMath::TransFactor pointIdentity;
                pointIdentity.kind = OntoMath::TransFactor::Kind::Cos;
                pointIdentity.variable = "x"; pointIdentity.scale = 0.0;
                inlineCos->scalarForm.terms.front().trans.push_back(pointIdentity);
                *f->volumeScattering = piece(op(OntoMath::MathNode::Op::Add, number(0.3), std::move(inlineCos)));
                f->noteAuthoredMathWritten();
            }
            auto inlineSourceCos = timeCos(0.2, 0.23);
            OntoMath::TransFactor pointIdentity;
            pointIdentity.kind = OntoMath::TransFactor::Kind::Cos;
            pointIdentity.variable = "x"; pointIdentity.scale = 0.0;
            inlineSourceCos->scalarForm.terms.front().trans.push_back(pointIdentity);
            auto inlineSource = piece(op(OntoMath::MathNode::Op::Add, number(0.6), std::move(inlineSourceCos)));
            source.radianceExpr = &inlineSource; source.radianceRevision = ++revision;
            check(prepared.rgb == render(eye, look, time, 96).rgb,
                  "GPU time-only preparation matches independent point-bound arithmetic and clocks");
            for (auto& f : media) {
                *f->volumeScattering = piece(op(OntoMath::MathNode::Op::Add, number(0.3), timeCos(0.1, 0.32)));
                f->noteAuthoredMathWritten();
            }
            source.radianceExpr = &preparedSource; source.radianceRevision = ++revision;
            (void)render(eye, look, time, 96);
            const auto advanced = render(eye, look, time + 1.0, 96);
            check(advanced.rgb != prepared.rgb && advanced.compiles == 0 && advanced.lowerings == 0 &&
                  advanced.parameterUploads == 0 && advanced.proofUploads == 0,
                  "time preparation advances medium time without compilation or CPU payload uploads");
            source.temporalCoordinate = 4.25;
            const auto sourceAdvanced = render(eye, look, time + 1.0, 96);
            check(sourceAdvanced.rgb != advanced.rgb && sourceAdvanced.compiles == 0 &&
                  sourceAdvanced.parameterUploads == 0 && sourceAdvanced.proofUploads == 0,
                  "prepared source clock advances independently without rewriting equation parameters");
            source.temporalCoordinate = 3.0;
            media[0]->volumeScattering->pieces.front().mathNode->children[1]->scalarForm.terms.front().coefficient = 0.15;
            media[0]->noteAuthoredMathWritten();
            const auto numeric = render(eye, look, time + 1.0, 96);
            check(numeric.rgb != advanced.rgb && numeric.compiles == 0 && numeric.lowerings == 0 &&
                  numeric.parameterUploads > 0 && numeric.proofUploads == 0,
                  "prepared numeric edit refreshes its inputs without shader regeneration");
            media[0]->volumeScattering->pieces.front().mathNode->children[1]->scalarForm.terms.front().coefficient = 0.1;
            media[0]->noteAuthoredMathWritten();
        }
        const auto beforeReload = render(eye, look, 3.5, 96);
        renderer.reloadShaders();
        check(beforeReload.rgb == render(eye, look, 3.5, 96).rgb,
              "reloading clears and reconstructs prepared recipes and GPU pipelines without stale values");
        if (count == 2) {
            renderer.shutdown();
            wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
            wgpu::Device replacement;
            if (!replacement.init()) return 1;
            wgpuQueueRelease(gpu.queue); wgpuDeviceRelease(gpu.device);
            wgpuAdapterRelease(gpu.adapter); wgpuInstanceRelease(gpu.instance);
            gpu = replacement;
            if (!renderer.init(gpu)) return 1;
            target = wgpuDeviceCreateTexture(gpu.device, &td);
            view = wgpuTextureCreateView(target, nullptr);
            readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
            renderer.setVolumeZeroProofEnabled(false);
            check(beforeReload.rgb == render(eye, look, 3.5, 96).rgb,
                  "device recreation cannot reuse old prepared buffers, values or compute pipelines");
        }
    }

    // Zach selected the full accepted lattice as the bounded-mode reference,
    // removing the uncertified opacity stop. The ramp emits only behind an
    // optical depth >6, after the legacy ray has already stopped at depth 4.6.
    // These equations are authored in memory; no saved world is modified.
    renderer.reloadShaders();
    source.radianceExpr = nullptr;
    source.enabled = false;
    source.radianceRevision = ++revision;
    check(renderer.volumeOutputErrorBudget() == 0.0,
          "output error budget defaults to compatibility integration");
    renderer.setVolumeOutputErrorBudget(std::numeric_limits<double>::infinity());
    check(renderer.volumeOutputErrorBudget() == 0.0,
          "nonfinite output budgets cannot enable approximate termination");
    renderer.setVolumeOutputErrorBudget(-1.0);
    check(renderer.volumeOutputErrorBudget() == 0.0,
          "negative output budget preserves compatibility integration");
    renderer.setVolumeOutputErrorBudget(1.0);
    check(renderer.volumeOutputErrorBudget() == 1.0 / 255.0,
          "output budget is capped at one 8-bit level");
    for (int count : {1, 2}) {
        media.clear();
        for (int i = 0; i < count; ++i) {
            auto f = std::make_shared<geom::FieldNode>("test.late-emission-" + std::to_string(i));
            f->scale = glm::vec3(2.0f);
            *f->volumeDensity = piece(number(1.0));
            *f->volumeExtinction = piece(number(2.0 / count));
            *f->volumeScattering = piece(number(0.0));
            auto emission = std::make_unique<OntoMath::MathNode>();
            emission->op = OntoMath::MathNode::Op::VectorConstruct;
            for (int c = 0; c < 3; ++c)
                emission->children.push_back(op(OntoMath::MathNode::Op::Scale, number(i == 0 ? 1000.0 : 0.0),
                    op(OntoMath::MathNode::Op::Intersection,
                       op(OntoMath::MathNode::Op::Sub, variable("z"), number(1.0)), number(0.0))));
            *f->volumeEmission = piece(std::move(emission));
            f->noteAuthoredMathWritten();
            media.push_back(f);
        }
        renderer.setVolumeOutputErrorBudget(0.0);
        const auto compatibility = render(eye, look, 0.0, 96);
        renderer.setVolumeOutputErrorBudget(1.0 / 255.0);
        const auto full = render(eye, look, 0.0, 96);
        const auto fine = render(eye, look, 0.0, 6144);
        const size_t center = (size_t(height / 2) * width + width / 2) * 4;
        const int analytic = int(std::lround(255.0 * 1000.0 *
            (std::exp(-6.0) - 3.0 * std::exp(-8.0)) / 4.0));
        check(compatibility.rgb[center] == 0 && full.rgb[center] > 80,
              std::to_string(count) + " media: opt-in reaches bright emission beyond the opacity cutoff");
        check(std::abs(int(fine.rgb[center]) - analytic) <= 1 &&
              std::abs(int(full.rgb[center]) - int(fine.rgb[center])) <= 1,
              std::to_string(count) + " media: full transport matches analytic ramp and 6144 reference");
        check(full.compiles == 0 && full.lowerings == 0 && full.parameterUploads == 0 && fine.compiles == 0,
              "budget and sampling changes reuse resident equations without compilation");
        renderer.setVolumeOutputErrorBudget(std::numeric_limits<double>::denorm_min());
        const auto tiny = render(eye, look, 0.0, 96);
        check(tiny.rgb == full.rgb && tiny.compiles == 0,
              "tiny positive budget still selects full lattice when its GPU budget rounds to zero");
        renderer.setVolumeOutputErrorBudget(0.0);
        check(render(eye, look, 0.0, 96).rgb == compatibility.rgb,
              "returning to zero restores the byte-identical compatibility image");

        for (auto& f : media) {
            f->volumeEmission->pieces.clear();
            f->noteAuthoredMathWritten();
        }
        const auto legacyBackground = render(eye, look, 0.0, 96, glm::vec4(1.0f));
        renderer.setVolumeOutputErrorBudget(1.0 / 255.0);
        const auto fullBackground = render(eye, look, 0.0, 96, glm::vec4(1.0f));
        check(legacyBackground.rgb[center] >= 2 && fullBackground.rgb[center] == 0,
              std::to_string(count) + " media: background attenuation also follows the full lattice");
        renderer.setVolumeOutputErrorBudget(0.0);
    }
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("webgpu_volume_shared_subexpression_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
