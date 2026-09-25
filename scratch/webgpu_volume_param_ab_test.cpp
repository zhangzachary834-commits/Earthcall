// Temporary identical-source A/B probe. Run alone on exact parent and candidate.
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
struct MapResult { bool done = false; bool ok = false; };
void mapped(WGPUMapAsyncStatus status, WGPUStringView, void* user, void*) {
    auto* result = static_cast<MapResult*>(user);
    result->ok = status == WGPUMapAsyncStatus_Success;
    result->done = true;
}
std::shared_ptr<OntoMath::MathNode> scalar(double value) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::ScalarLeaf;
    node->scalarForm.terms.push_back(OntoMath::Term(value));
    return node;
}
std::shared_ptr<OntoMath::MathNode> vector(double x, double y, double z) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::VectorConstruct;
    for (double component : {x, y, z}) {
        node->children.push_back(std::make_unique<OntoMath::MathNode>(*scalar(component)));
    }
    return node;
}
}

int main(int argc, char** argv) {
    const uint32_t width = argc > 1 ? static_cast<uint32_t>(std::atoi(argv[1])) : 64;
    const int frames = argc > 2 ? std::atoi(argv[2]) : 100;
    const int blocks = argc > 3 ? std::atoi(argv[3]) : 5;
    const bool edit = argc > 4 && argv[4][0] == 'e';
    const int mediaCount = argc > 5 ? std::atoi(argv[5]) : 2;
    assert(width >= 16 && width <= 512 && frames > 0 && blocks > 0 &&
           (mediaCount == 2 || mediaCount == 4));
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::puts("FAIL: no WebGPU device"); return 1; }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::puts("FAIL: renderer init"); return 1; }
    setCurrentRenderer(&renderer);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {width, width, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);
    const uint32_t stride = (width * 4 + 255) & ~255u;
    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bd.size = static_cast<uint64_t>(stride) * width;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    assert(target && view && readback);

    auto syncPixel = [&] {
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = target;
        src.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback;
        dst.layout.bytesPerRow = stride;
        dst.layout.rowsPerImage = width;
        WGPUExtent3D extent = {width, width, 1};
        wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);
        MapResult result;
        WGPUBufferMapCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowProcessEvents;
        callback.callback = mapped;
        callback.userdata1 = &result;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, bd.size, callback);
        while (!result.done) wgpuInstanceProcessEvents(gpu.instance);
        assert(result.ok);
        const auto* pixels = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, bd.size));
        const size_t at = static_cast<size_t>(width / 2) * stride + (width / 2) * 4;
        const unsigned rgb = (unsigned(pixels[at]) << 16) |
                             (unsigned(pixels[at + 1]) << 8) | pixels[at + 2];
        wgpuBufferUnmap(readback);
        return rgb;
    };

    const glm::vec3 eye(0, 0, 2);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    const glm::mat4 view3d = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
    renderer.setCamera(view3d, proj, eye);

    auto densityNode = scalar(1.0);
    auto extinctionANode = scalar(0.35);
    auto extinctionBNode = scalar(0.90);
    auto extinctionCNode = scalar(0.22);
    auto extinctionDNode = scalar(0.47);
    auto scatterNode = scalar(0.0);
    auto emissionANode = vector(0.20, 0.0, 0.0);
    auto emissionBNode = vector(0.0, 0.0, 0.30);
    auto emissionCNode = vector(0.04, 0.01, 0.0);
    auto emissionDNode = vector(0.0, 0.03, 0.02);
    auto density = OntoMath::Piecewise::continuous(densityNode);
    auto extinctionA = OntoMath::Piecewise::continuous(extinctionANode);
    auto extinctionB = OntoMath::Piecewise::continuous(extinctionBNode);
    auto extinctionC = OntoMath::Piecewise::continuous(extinctionCNode);
    auto extinctionD = OntoMath::Piecewise::continuous(extinctionDNode);
    auto scatter = OntoMath::Piecewise::continuous(scatterNode);
    auto emissionA = OntoMath::Piecewise::continuous(emissionANode);
    auto emissionB = OntoMath::Piecewise::continuous(emissionBNode);
    auto emissionC = OntoMath::Piecewise::continuous(emissionCNode);
    auto emissionD = OntoMath::Piecewise::continuous(emissionDNode);

    Rendering::VolumeDensityBinding a;
    a.origin = glm::vec3(0);
    a.scale = glm::vec3(1);
    a.densityExpr = &density;
    a.densityRevision = 5801;
    a.extinctionExpr = &extinctionA;
    a.extinctionRevision = 5802;
    a.scatteringExpr = &scatter;
    a.scatteringRevision = 5803;
    a.emissionExpr = &emissionA;
    a.emissionRevision = 5804;
    Rendering::VolumeDensityBinding b = a;
    b.extinctionExpr = &extinctionB;
    b.extinctionRevision = 5811;
    b.emissionExpr = &emissionB;
    b.emissionRevision = 5812;
    Rendering::VolumeDensityBinding c = a;
    c.extinctionExpr = &extinctionC;
    c.extinctionRevision = 5813;
    c.emissionExpr = &emissionC;
    c.emissionRevision = 5814;
    Rendering::VolumeDensityBinding d = a;
    d.extinctionExpr = &extinctionD;
    d.extinctionRevision = 5815;
    d.emissionExpr = &emissionD;
    d.emissionRevision = 5816;
    std::vector<Rendering::VolumeDensityBinding> sources = {a, b};
    if (mediaCount == 4) {
        sources.push_back(c);
        sources.push_back(d);
    }
    uint64_t revision = 5820;
    renderer.setVolumeDensitySources(sources, revision);

    auto draw = [&] {
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(view, width, width, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
    };
    for (int i = 0; i < 30; ++i) draw();
    syncPixel();
    std::printf("BENCH width=%u frames=%d blocks=%d mode=%s media=%d\n",
                width, frames, blocks, edit ? "edit" : "stable", mediaCount);
    for (int block = 0; block < blocks; ++block) {
        double cpuMs = 0;
        uint64_t ringSuballocations = 0;
        uint64_t ringBytes = 0;
        const auto wallStart = Clock::now();
        const std::clock_t processStart = std::clock();
        for (int i = 0; i < frames; ++i) {
            const auto t0 = Clock::now();
            if (edit) {
                emissionANode->children[0]->scalarForm.terms[0].coefficient =
                    ((block * frames + i) & 1) ? 0.20 : 0.25;
                a.emissionRevision = ++revision;
                sources[0] = a;
                renderer.setVolumeDensitySources(sources, revision);
            }
            draw();
            const auto t1 = Clock::now();
            cpuMs += ms(t0, t1);
            ringSuballocations += renderer.frameStats().bufferSuballocations;
            ringBytes += renderer.frameStats().uniformBytesWritten;
        }
        const std::clock_t processEnd = std::clock();
        const unsigned rgb = syncPixel();
        const auto wallEnd = Clock::now();
        std::printf("BLOCK %d cpu_ms_per_frame=%.6f process_cpu_ms_per_frame=%.6f "
                    "wall_sync_ms_per_frame=%.6f "
                    "ring_allocs_per_frame=%.2f ring_bytes_per_frame=%.1f rgb=%06x\n",
                    block, cpuMs / frames,
                    1000.0 * double(processEnd - processStart) / CLOCKS_PER_SEC / frames,
                    ms(wallStart, wallEnd) / frames,
                    double(ringSuballocations) / frames, double(ringBytes) / frames, rgb);
    }

    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    return 0;
}
