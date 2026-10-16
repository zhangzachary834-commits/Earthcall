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
    if (!gpu.init()) { std::printf("UNAVAILABLE: no WebGPU device\n"); return 77; }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }
    setCurrentRenderer(&renderer);

    const uint32_t width = std::getenv("REVIEW_NATIVE_SIZE") ? 2560 : 160, height = std::getenv("REVIEW_NATIVE_SIZE") ? 1440 : 120;
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
    renderer.setVolumeZeroProofEnabled(std::getenv("REVIEW_PROOF_ON") != nullptr);


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



    const glm::vec3 eye(0,0,-5), look(0,0,0);
    source.enabled = false;
    for (int count : {1,2}) {
      media.clear();
      for (int i=0;i<count;++i) {
        auto f=std::make_shared<geom::FieldNode>("review.absorber-"+std::to_string(i));
        f->scale=glm::vec3(2.0f);
        *f->volumeDensity=piece(number(1));
        auto ext=number(1.0/count);
        OntoMath::Term ripple(0.9/count);
        OntoMath::TransFactor cos;
        cos.kind=OntoMath::TransFactor::Kind::Cos;
        cos.variable="z"; cos.scale=12.0*3.14159265358979323846;
        ripple.trans.push_back(cos); ext->scalarForm.terms.push_back(ripple);
        *f->volumeExtinction=piece(std::move(ext));
        *f->volumeScattering=piece(number(0));
        f->volumeEmission->pieces.clear();
        f->noteAuthoredMathWritten(); media.push_back(f);
      }
      renderer.setVolumeOutputErrorBudget(std::numeric_limits<double>::denorm_min());
      const auto reference=render(eye,look,0,96,glm::vec4(1));
      renderer.setVolumeOutputErrorBudget(1.0/255.0);
      const auto candidate=render(eye,look,0,96,glm::vec4(1));
      const size_t center=(size_t(height/2)*width+width/2)*4;
      int maxError=0; size_t overOne=0;
      for(size_t p=0;p<reference.rgb.size();p+=4) {
        int e=0; for(int c=0;c<4;++c) e=std::max(e,std::abs(int(reference.rgb[p+c])-int(candidate.rgb[p+c])));
        maxError=std::max(maxError,e); if(e>1) ++overOne;
      }
      std::printf("REVIEW count=%d full_center=%d adaptive_center=%d max_RGBA_error=%d pixels_over_one=%zu\n",count,int(reference.rgb[center]),int(candidate.rgb[center]),maxError,overOne);
      check(maxError<=1,"adaptive absorbing medium respects one-level output budget");
    }
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    return g_failures ? 1 : 0;
}
