// Native WebGPU witness: the zero-density proof may only remove work, never
// light. Renders the real Northern Veil media (and one medium alone, the
// single-medium pipeline) with the proof off and on, and requires the full
// RGBA framebuffer to be byte-identical while the proof actually applied.
// Timing is reported, not gated (machine load drifts; see frame_lag_test).
//
// Claude Opus 5.5 · Claude Code · 2026-10-09. Measured on an M5 at 640x360:
// four curtains 94-101 -> 34-35 ms/frame, one curtain 8.7 -> 2.8 ms/frame.
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

struct Frame { uint64_t hash = 0; uint64_t lit = 0; double ms = 0.0; uint32_t proven = 0; };

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_zero_proof_test...\n");

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

    // From the spawn dais up toward the media.
    const glm::vec3 eye(0.0f, 2.0f, 0.0f);
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0.0f, 60.0f, 90.0f), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);

    auto render = [&](const std::vector<std::shared_ptr<geom::FieldNode>>& set, bool proofOn,
                      double t, int timedFrames) {
        renderer.setVolumeZeroProofEnabled(proofOn);
        auto drawOnce = [&] {
            std::vector<Rendering::VolumeDensityBinding> bindings;
            std::string identity;
            for (const auto& f : set) {
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
        auto sync = [&]() -> std::pair<uint64_t, uint64_t> {
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
            uint64_t hash = 1469598103934665603ull, lit = 0;
            for (uint32_t y = 0; y < height; ++y)
                for (uint32_t x = 0; x < width; ++x) {
                    const size_t at = size_t(y) * stride + x * 4;
                    lit += (px[at] + px[at + 1] + px[at + 2]) > 6;
                    for (int c = 0; c < 4; ++c) { hash ^= px[at + c]; hash *= 1099511628211ull; }
                }
            wgpuBufferUnmap(readback);
            return {hash, lit};
        };
        drawOnce(); sync();   // warm: compile, build proofs
        Frame frame;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < timedFrames; ++i) drawOnce();
        sync();
        frame.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / timedFrames;
        drawOnce();
        frame.proven = renderer.frameStats().volumeZeroProofCellsProven;
        const auto [hash, lit] = sync();
        frame.hash = hash; frame.lit = lit;
        return frame;
    };

    struct Case { std::string label; std::vector<std::shared_ptr<geom::FieldNode>> set; };
    const std::vector<Case> cases = {
        {"all " + std::to_string(media.size()) + " media (fused set pipeline)", media},
        {"one medium (single pipeline)", {media.front()}},
    };
    for (const auto& c : cases) {
        for (double t : {0.0, 7.25}) {
            const Frame exact = render(c.set, false, t, 4);
            const Frame proved = render(c.set, true, t, 4);
            std::printf("       %s t=%.2f: exact %.2f ms, proved %.2f ms, lit %llu, proven cells %u\n",
                        c.label.c_str(), t, exact.ms, proved.ms,
                        (unsigned long long)exact.lit, proved.proven);
            check(exact.lit > 0, c.label + " t=" + std::to_string(t) + ": the media are visible");
            check(exact.proven == 0 && proved.proven > 0,
                  c.label + ": the proof is off, then actually applied");
            check(exact.hash == proved.hash,
                  c.label + " t=" + std::to_string(t) + ": framebuffer is byte-identical with the proof");
        }
    }

    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("webgpu_volume_zero_proof_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
