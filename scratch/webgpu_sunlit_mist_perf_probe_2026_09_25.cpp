// Diagnostic only: read the authored Sanctuary save, time its volumetric path,
// and compare with an occluder-free counterfactual without writing the save.
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
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <utility>

namespace {
using Clock = std::chrono::steady_clock;
double elapsedMs(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b-a).count();
}
struct MapResult { bool done = false; bool ok = false; };
void mapped(WGPUMapAsyncStatus status, WGPUStringView, void* user, void*) {
    auto* result = static_cast<MapResult*>(user);
    result->ok = status == WGPUMapAsyncStatus_Success;
    result->done = true;
}
nlohmann::json readJson(const char* path) {
    std::ifstream in(path);
    if (!in) { std::fprintf(stderr, "cannot read %s\n", path); std::exit(2); }
    nlohmann::json value; in >> value; return value;
}
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 3) {
        std::fprintf(stderr, "usage: probe <zone.json> <authored|no-occluder> [width] [height] [frames] [blocks] [eyeZ]\n");
        return 2;
    }
    const bool authored = argv[2][0] == 'a';
    const uint32_t width = argc > 3 ? static_cast<uint32_t>(std::atoi(argv[3])) : 160;
    const uint32_t height = argc > 4 ? static_cast<uint32_t>(std::atoi(argv[4])) : 90;
    const int frames = argc > 5 ? std::atoi(argv[5]) : 30;
    const int blocks = argc > 6 ? std::atoi(argv[6]) : 3;
    const float eyeZ = argc > 7 ? std::atof(argv[7]) : 8.0f;
    assert(width >= 16 && height >= 16 && frames > 0 && blocks > 0);

    const auto saved = readJson(argv[1]);
    assert(saved.at("identifier") == "Sanctuary of Sunlit Mist");
    auto sun = geom::FieldNode::fromJson(saved.at("spatialRoot"));
    auto mist = geom::FieldNode::fromJson(saved.at("spatialFields").at(0));
    assert(sun && mist && mist->getIdentifier() == "mist.sanctuary.sunlit-mist-volume");
    Rendering::AuthorableLightState light;
    assert(Rendering::readAuthorableLight(*sun, light) && light.enabled);
    Rendering::VolumeDensityBinding initial;
    assert(Rendering::readVolumeDensity(*mist, 0, 0, initial) && initial.occluderSdf);

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
    assert(target && view && readback);

    renderer.setLight(light.position, Rendering::lightAmbientRadiance(light),
                      Rendering::lightDiffuseRadiance(light),
                      Rendering::lightSpecularRadiance(light));
    renderer.setLightingEnabled(light.enabled);
    Rendering::RadianceSourceBinding source;
    source.position = light.position;
    source.ambientRadiance = Rendering::lightAmbientRadiance(light);
    source.diffuseRadiance = Rendering::lightDiffuseRadiance(light);
    source.specularRadiance = Rendering::lightSpecularRadiance(light);
    source.coefficients = glm::vec4(light.intensity, light.ambient,
                                     light.diffuse, light.specular);
    source.enabled = light.enabled;
    renderer.setRadianceSources({source}, 1);

    const glm::vec3 eye(0.0f, 2.2f, eyeZ);
    const glm::vec3 front = glm::normalize(glm::vec3(0.0f, 0.05f, -1.0f));
    const glm::mat4 view3d = glm::lookAt(eye, eye + front, glm::vec3(0,1,0));
    const glm::mat4 proj = glm::perspectiveZO(
        glm::radians(45.0f), float(width) / height, 0.1f, 100.0f);
    renderer.setCamera(view3d, proj, eye);

    auto syncImage = [&] {
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = target;
        src.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback;
        dst.layout.bytesPerRow = stride;
        dst.layout.rowsPerImage = height;
        WGPUExtent3D extent = {width, height, 1};
        wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
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
        uint64_t lum = 0;
        uint64_t hash = 1469598103934665603ull;
        for (uint32_t y=0; y<height; ++y) {
            for (uint32_t x=0; x<width; ++x) {
                const size_t at = static_cast<size_t>(y)*stride + x*4;
                lum += pixels[at] + pixels[at+1] + pixels[at+2];
                for (int c=0;c<4;++c) {
                    hash ^= pixels[at+c];
                    hash *= 1099511628211ull;
                }
            }
        }
        wgpuBufferUnmap(readback);
        return std::pair<uint64_t,uint64_t>{lum,hash};
    };

    auto frame = [&](double& projectionMs, double& submitMs) {
        const auto t0=Clock::now();
        Rendering::VolumeDensityBinding medium;
        assert(Rendering::readVolumeDensity(*mist, 0, 0, medium));
        if (!authored) { medium.occluderSdf = nullptr; medium.occluderRevision = 0; }
        renderer.setVolumeDensitySources({medium}, Rendering::volumeContentRevision(medium));
        const auto t1=Clock::now();
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(view,width,height,glm::vec4(0,0,0,1));
        renderer.composeVolumes();
        renderer.endFrame();
        const auto t2=Clock::now();
        projectionMs += elapsedMs(t0,t1);
        submitMs += elapsedMs(t1,t2);
    };
    for (int i=0;i<12;++i) { double p=0,s=0;frame(p,s); }
    syncImage();
    std::printf("SCENE=%s mode=%s size=%ux%u eyeZ=%.1f frames=%d blocks=%d\n",
                mist->getIdentifier().c_str(), authored?"authored":"no-occluder",
                width,height,eyeZ,frames,blocks);
    for (int block=0;block<blocks;++block) {
        double projectionMs=0,submitMs=0;
        const auto wallStart=Clock::now();
        const auto cpuStart=std::clock();
        for(int i=0;i<frames;++i) frame(projectionMs,submitMs);
        const auto cpuEnd=std::clock();
        const auto [lum,hash]=syncImage();
        const auto wallEnd=Clock::now();
        std::printf("BLOCK %d projection_ms=%.4f submit_ms=%.4f process_cpu_ms=%.4f "
                    "wall_sync_ms=%.4f ring_allocs=%u ring_bytes=%zu "
                    "wgsl_compiles=%u lum_sum=%llu rgba_hash=%016llx\n",
                    block,projectionMs/frames,submitMs/frames,
                    1000.0*double(cpuEnd-cpuStart)/CLOCKS_PER_SEC/frames,
                    elapsedMs(wallStart,wallEnd)/frames,
                    renderer.frameStats().bufferSuballocations,
                    renderer.frameStats().uniformBytesWritten,
                    renderer.frameStats().volumeProgramCompiles,
                    static_cast<unsigned long long>(lum),
                    static_cast<unsigned long long>(hash));
    }
    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    return 0;
}
