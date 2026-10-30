// Read-only capture fixture, adapted from Claude Opus 5.5's 2026-10-09
// Northern Veil probe, which in turn adapted Codex's
// scratch/webgpu_sunlit_mist_perf_probe_2026_09_25.cpp.
// Census continuation: Codex / GPT-6.1 Sol / session 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca
// 2026-10-11 PDT. Read-only saved fixture; never an FPS benchmark.
// Built by scripts/northern_veil_fragment_demand.py into a scratch output directory.
// Usage: probe <zone.ecform> 0123 <width> <height> <fixed time> <output.rgba> 1
// EC_CAM supplies exact native eye/look coordinates; EC_CENSUS supplies output.bin.
// Baseline and diagnostic differ only in separately linked renderer instrumentation.
// Requires the four-curtain fixture, no opaque scene geometry. See the census audit.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "json.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>

#ifdef EC_FRAGMENT_CENSUS
extern "C" bool dumpFragmentCensus(WGPUDevice,WGPUInstance,WGPUQueue,const char*);
#else
static bool dumpFragmentCensus(WGPUDevice,WGPUInstance,WGPUQueue,const char*) { return false; }
#endif
namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
struct MapResult { bool done = false; bool ok = false; };
void mapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapResult*>(u);
    r->ok = s == WGPUMapAsyncStatus_Success;
    r->done = true;
}
#define REQUIRE(c) do { if (!(c)) { std::printf("FAIL: %s\n", #c); return 1; } } while (0)
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 3) { std::fprintf(stderr, "usage: probe <zone.ecform> 0123 [width] [height] [fixed-time] [output.rgba] 1\n"); return 2; }
    const char* mask = argv[2];
    const uint32_t width = argc > 3 ? std::atoi(argv[3]) : 640;
    const uint32_t height = argc > 4 ? std::atoi(argv[4]) : 360;
    const double fixedT = argc > 5 ? std::atof(argv[5]) : 0.0;
    const char* outPath = argc > 6 ? argv[6] : nullptr;
    const int frames = argc > 7 ? std::atoi(argv[7]) : 1;
    REQUIRE(width >= 64 && height >= 36 && frames == 1 && std::strcmp(mask,"0123") == 0);

    std::ifstream in(argv[1], std::ios::binary);
    REQUIRE(in);
    // One startup decode of the existing MsgPack migration envelope. No save
    // writes and no serialization/lowering in the sample or frame loops.
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    const auto envelope = nlohmann::json::from_msgpack(bytes);
    const auto saved = nlohmann::json::parse(envelope.at("MigrationRoot").get<std::string>());
    REQUIRE(saved.at("identifier") == "Northern Veil");

    auto root = geom::FieldNode::fromJson(saved.at("spatialRoot"));
    REQUIRE(root);
    std::vector<std::shared_ptr<geom::FieldNode>> curtains;
    for (const auto& f : saved.at("spatialFields")) curtains.push_back(geom::FieldNode::fromJson(f));
    REQUIRE(curtains.size() == 4);
    std::vector<geom::FieldNode*> active;
    for (const char* c = mask; *c; ++c) {
        const int i = *c - '0';
        if (i >= 0 && i < static_cast<int>(curtains.size())) active.push_back(curtains[i].get());
    }

    Rendering::AuthorableLightState light;
    REQUIRE(Rendering::readAuthorableLight(*root, light));

    wgpu::Device gpu;
    if (!gpu.init()) { std::puts("FAIL: no WebGPU device"); return 1; }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::puts("FAIL: renderer init"); return 1; }
    setCurrentRenderer(&renderer);

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
    source.producerId = root->getIdentifier();
    source.position = light.position;
    source.ambientRadiance = Rendering::lightAmbientRadiance(light);
    source.diffuseRadiance = Rendering::lightDiffuseRadiance(light);
    source.specularRadiance = Rendering::lightSpecularRadiance(light);
    source.coefficients = glm::vec4(light.intensity, light.ambient, light.diffuse, light.specular);
    source.enabled = light.enabled;
    renderer.setRadianceSources({source}, 1);
    renderer.setLight(source.position, source.ambientRadiance, source.diffuseRadiance, source.specularRadiance);
    renderer.setLightingEnabled(light.enabled);

    // Spawn dais, looking up and out at the curtains (y 48-78, z 68-140).
    glm::vec3 eye(0.0f, 2.0f, 0.0f);
    glm::vec3 look(0.0f, 60.0f, 90.0f);
    if (const char* cam = std::getenv("EC_CAM")) {   // "ex,ey,ez,lx,ly,lz"
        std::sscanf(cam, "%f,%f,%f,%f,%f,%f", &eye.x, &eye.y, &eye.z, &look.x, &look.y, &look.z);
    }
    renderer.setCamera(glm::lookAt(eye, look, glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);

    auto syncImage = [&] {
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
        const auto* px = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(readback, 0, bd.size));
        uint64_t lum = 0, lit = 0;
        for (uint32_t y = 0; y < height; ++y)
            for (uint32_t x = 0; x < width; ++x) {
                const size_t at = size_t(y) * stride + x * 4;
                const unsigned s = px[at] + px[at + 1] + px[at + 2];
                lum += s; lit += s > 6;
            }
        wgpuBufferUnmap(readback);
        return std::pair<uint64_t, uint64_t>{lum, lit};
    };

    double t = fixedT;
    auto frame = [&](double& projMs, double& submitMs) {
        const auto t0 = Clock::now();
        std::vector<Rendering::VolumeDensityBinding> media;
        std::string identity;
        for (auto* f : active) {
            Rendering::VolumeDensityBinding m;
            if (Rendering::readVolumeDensity(*f, t, 1.0 / 60.0, m)) {
                Rendering::appendVolumeSetIdentity(identity, m.producerId, m);
                media.push_back(m);
            }
        }
        renderer.setVolumeDensitySources(std::move(media), std::hash<std::string>{}(identity));
        const auto t1 = Clock::now();
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const auto t2 = Clock::now();
        projMs += ms(t0, t1); submitMs += ms(t1, t2);
    };

    { double p = 0, q = 0; frame(p, q); }
    syncImage();
    for (int i = 0; i < frames; ++i) { double p = 0, q = 0; frame(p, q); }
    syncImage();
    { double p = 0, q = 0; frame(p, q); }
    // final readback, tightly packed, written raw
    {
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
        const auto* px = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(readback, 0, bd.size));
        if (outPath) {
            FILE* f = std::fopen(outPath, "wb");
            for (uint32_t y = 0; y < height; ++y) std::fwrite(px + size_t(y) * stride, 1, width * 4, f);
            std::fclose(f);
        }
        wgpuBufferUnmap(readback);
    }
    if (const char* path=std::getenv("EC_CENSUS")) REQUIRE(dumpFragmentCensus(gpu.device,gpu.instance,gpu.queue,path));
    std::printf("DIAGNOSTIC mask=%s %ux%u t=%.2f out=%s; NOT PERFORMANCE TIMING\n", mask, width, height, fixedT, outPath ? outPath : "-");
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    return 0;
}
