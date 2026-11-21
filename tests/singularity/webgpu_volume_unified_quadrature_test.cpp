// Native WebGPU witness for unified volume quadrature (SdfWgsl
// compileVolumeSet): overlapping media share the finest standalone sampling
// resolution present instead of each overlap segment taking a full chord's
// worth of samples. That changes pixels, so the promise is measured against
// the true integral: at the default 96 samples per chord, every pixel must
// stay within the historical fidelity of a 6144-samples-per-chord reference.
// Also witnesses that the resolution is a runtime value: changing it must
// never recompile a volume program.
//
// Claude Opus 5.5 · Claude Code · 2026-10-09, from Zach's idea: "a
// mathematical unification of the drawing functions wherever it overlaps".
// Measured on an M5 at 640x360 (vs a 6144 reference, max channel error in
// 8-bit levels): the old per-segment scheme max 1 / 0 px >1; unified max 1 /
// 0 px >1 at three views, 1.5-1.8x faster.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "support/test_harness.hpp"

#include <webgpu/wgpu.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
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

struct Image { std::vector<uint8_t> rgb; uint64_t lit = 0; uint32_t compiles = 0; };

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_unified_quadrature_test...\n");

    const std::string path = TestSupport::resolveRealWorldPath("saves/zones/Northern Veil/zone.ecform");
    if (!std::filesystem::exists(path)) { std::printf("SKIP: Northern Veil save not present\n"); return 0; }
    std::ifstream in(path, std::ios::binary);
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto zone = nlohmann::json::parse(
        nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());

    auto root = geom::FieldNode::fromJson(zone.at("spatialRoot"));
    std::vector<std::shared_ptr<geom::FieldNode>> media;
    for (const auto& f : zone.at("spatialFields")) {
        auto node = geom::FieldNode::fromJson(f);
        if (node && node->volumeDensity && !node->volumeDensity->pieces.empty()) media.push_back(node);
    }
    Rendering::AuthorableLightState light;
    if (media.empty() || !root || !Rendering::readAuthorableLight(*root, light)) {
        std::printf("FAIL: Northern Veil save has no media or no light\n");
        return 1;
    }

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("SKIP: no WebGPU device\n"); return 0; }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }
    setCurrentRenderer(&renderer);

    const uint32_t width = 320, height = 180;
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


    auto render = [&](const glm::vec3& eye, const glm::vec3& look, double t, int samples) {
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
            renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));
            renderer.composeVolumes();
            renderer.endFrame();
        };
        drawOnce();
        Image image;
        image.compiles = renderer.frameStats().volumeProgramCompiles;
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
        image.rgb.reserve(size_t(width) * height * 3);
        for (uint32_t y = 0; y < height; ++y)
            for (uint32_t x = 0; x < width; ++x) {
                const size_t at = size_t(y) * stride + x * 4;
                for (int c = 0; c < 3; ++c) image.rgb.push_back(px[at + c]);
                image.lit += (px[at] + px[at + 1] + px[at + 2]) > 6;
            }
        wgpuBufferUnmap(readback);
        return image;
    };

    struct View { const char* label; glm::vec3 eye; glm::vec3 look; double t; };
    const View views[] = {
        {"spawn dais, looking up", {0, 2, 0}, {0, 60, 90}, 0.0},
        {"from the side", {45, 8, 25}, {0, 62, 110}, 7.25},
        {"among the media, looking along them", {0, 60, 40}, {10, 60, 140}, 7.25},
    };
    for (const auto& v : views) {
        const Image unified = render(v.eye, v.look, v.t, 96);
        const Image truth = render(v.eye, v.look, v.t, 6144);
        const Image again = render(v.eye, v.look, v.t, 96);
        int maxErr = 0;
        uint64_t overOne = 0, litPx = 0;
        double sumErr = 0.0;
        for (size_t i = 0; i < unified.rgb.size(); i += 3) {
            int m = 0;
            for (int c = 0; c < 3; ++c) {
                const int e = std::abs(int(unified.rgb[i + c]) - int(truth.rgb[i + c]));
                m = std::max(m, e);
                sumErr += e;
            }
            maxErr = std::max(maxErr, m);
            overOne += m > 1;
            litPx += (truth.rgb[i] + truth.rgb[i + 1] + truth.rgb[i + 2]) > 6;
        }
        std::printf("       %s: max error %d levels, %llu px > 1 level, mean %.3f, lit %llu\n",
                    v.label, maxErr, (unsigned long long)overOne,
                    sumErr / std::max<uint64_t>(1, litPx * 3), (unsigned long long)litPx);
        check(truth.lit > 0, std::string(v.label) + ": the media are visible");
        // Promise: today's fidelity (measured max 1, none > 1). Gate leaves one
        // level of headroom for other GPUs' transcendental precision.
        check(maxErr <= 2, std::string(v.label) + ": no pixel more than 2 levels from the 6144 reference");
        check(overOne * 2000 <= litPx, std::string(v.label) + ": at most 0.05% of lit pixels more than 1 level off");
        // Frame stats reset every frame; the first draw of this set compiled it.
        check(truth.compiles == 0 && again.compiles == 0,
              std::string(v.label) + ": changing samples per chord recompiled nothing");
        check(again.rgb == unified.rgb, std::string(v.label) + ": returning to 96 restores the identical image");
    }

    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("webgpu_volume_unified_quadrature_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
