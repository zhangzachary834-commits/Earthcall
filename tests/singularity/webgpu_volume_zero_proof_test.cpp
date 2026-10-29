// Native WebGPU witness: the zero-density proof may only remove work, never
// light. Renders the real Northern Veil media (and one medium alone, the
// single-medium pipeline) with the proof off and on, and requires the full
// RGBA framebuffer to be byte-identical while the proof actually applied.
// Timing is reported, not gated (machine load drifts; see frame_lag_test).
//
// Claude Opus 5.5 · Claude Code · 2026-10-09. Measured on an M5 at 640x360:
// four curtains 94-101 -> 34-35 ms/frame, one curtain 8.7 -> 2.8 ms/frame.
// Support, coordinate and stale-envelope witnesses: Codex / GPT-6.1 Sol /
// 01a122d7 / 2026-10-10. See the Northern_Veil_volumetric_frame_cost task.
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
#include <cstring>
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

struct Frame {
    uint64_t hash = 0;
    uint64_t lit = 0;
    double ms = 0.0;
    uint32_t proven = 0;
    uint32_t firstCompiles = 0;
    uint32_t firstProofBuilds = 0;   // zero-density proofs built by the first frame after a change
    uint32_t warmCompiles = 0;
    size_t warmParameterUploads = 0, warmProofUploads = 0;
    std::vector<unsigned char> rgba;
};

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

    // Odd dimensions include a centre ray exactly parallel to two grid axes.
    uint32_t width = 321, height = 181;
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {width, height, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);
    uint32_t stride = (width * 4 + 255) & ~255u;
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

    std::vector<unsigned char> lastRgba(width * height * 4u);
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
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(lastRgba.data() + size_t(y) * width * 4u, px + size_t(y) * stride, width * 4u);
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
        Frame frame;
        drawOnce();
        frame.firstCompiles = renderer.frameStats().volumeProgramCompiles;
        frame.firstProofBuilds = renderer.frameStats().volumeZeroProofBuilds;
        sync();   // warm: compile, build proofs
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < timedFrames; ++i) {
            drawOnce();
            frame.warmCompiles += renderer.frameStats().volumeProgramCompiles;
            frame.warmParameterUploads += renderer.frameStats().volumeParameterBytesUploaded;
            frame.warmProofUploads += renderer.frameStats().volumeProofBytesUploaded;
        }
        sync();
        frame.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / timedFrames;
        drawOnce();
        frame.proven = renderer.frameStats().volumeZeroProofCellsProven;
        frame.warmCompiles += renderer.frameStats().volumeProgramCompiles;
        const auto [hash, lit] = sync();
        frame.hash = hash; frame.lit = lit;
        frame.rgba = lastRgba;
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
            if (t == 0.0) {
                check(exact.firstCompiles + proved.firstCompiles > 0 &&
                      exact.warmCompiles == 0 && proved.warmCompiles == 0,
                      c.label + ": kernels prepare once, then reuse structure");
            }
            check(exact.warmParameterUploads == 0 && proved.warmParameterUploads == 0 &&
                  exact.warmProofUploads == 0 && proved.warmProofUploads == 0,
                  c.label + ": warmed parameters and proofs upload zero bytes");
            check(exact.lit > 0, c.label + " t=" + std::to_string(t) + ": the media are visible");
            check(exact.proven == 0 && proved.proven > 0,
                  c.label + ": the proof is off, then actually applied");
            check(exact.rgba == proved.rgba,
                  c.label + " t=" + std::to_string(t) + ": framebuffer is byte-identical with the proof");
        }
    }

    // Proofs may reject only null work. Both camera lanes retain authored
    // chord spacing while per-medium support intervals skip certified voids.
    struct View { const char* label; glm::vec3 eye; glm::vec3 target; };
    const View views[] = {
        {"inside", {0, 50, 75}, {40, 62, 110}},
        {"inside Emerald/Violet support", {-74.9f, 63, 62.72732f}, {0, 55, 90}},
        {"inside Emerald/Crimson support", {24.2272f, 50, 75.83955f}, {0, 62, 100}},
        {"reverse", {0, 60, 160}, {0, 55, 75}},
        {"axis-parallel", {0, 50, 0}, {0, 50, 100}},
        {"side", {90, 55, 90}, {-30, 55, 90}},
        {"fold crossing forward", {-72.5f, 50, 70}, {65, 60, 95}},
        {"fold crossing backward", {72.5f, 50, 95}, {-65, 60, 70}},
    };
    for (const auto& v : views) {
        renderer.setCamera(glm::lookAt(v.eye, v.target, glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), v.eye);
        for (const auto& c : cases) {
            const Frame exact = render(c.set, false, 7.25, 1);
            const Frame proved = render(c.set, true, 7.25, 1);
            check(exact.rgba == proved.rgba, c.label + " " + v.label + ": support preserves every RGBA byte");
            check(proved.proven > 0, c.label + " " + v.label + ": proof actually supplied");
        }
    }

    // Both camera lanes are warm above. Move through them with the proof still
    // enabled: returning to a lane must reuse compiled structure, not interpret
    // or regenerate the equations. Witness both single and fused caches.
    for (const auto& c : cases) {
        for (const auto& v : std::vector<View>{
                {"return outside", eye, {0, 60, 90}},
                {"return inside", {0, 50, 75}, {40, 62, 110}},
                {"return outside again", eye, {0, 60, 90}}}) {
            renderer.setCamera(glm::lookAt(v.eye, v.target, glm::vec3(0, 1, 0)),
                               glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), v.eye);
            const Frame reused = render(c.set, true, 7.25, 1);
            check(reused.firstCompiles == 0 && reused.warmCompiles == 0,
                  c.label + " " + v.label + ": cached kernels need no equation recompilation");
        }
    }

    // Keep the Piecewise address while changing all-empty density to positive.
    // Render proof-on first after the edit: an intervening proof-off frame
    // would evict the old memo and conceal a stale empty-envelope regression.
    auto edited = geom::FieldNode::fromJson(zone.at("spatialFields").front());
    const auto constant = [](double value) {
        auto j = nlohmann::json::parse(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0,"factors":{}}]}}}]})");
        j["pieces"][0]["mathNode"]["scalarForm"]["terms"][0]["c"] = value;
        return OntoMath::Piecewise::fromJson(j);
    };
    *edited->volumeDensity = constant(0.0);
    edited->noteAuthoredMathWritten();
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
    const Frame empty = render({edited}, true, 7.25, 1);
    check(empty.lit == 0 && empty.proven > 0, "zero density rejects its whole support, even with authored emission");
    *edited->volumeDensity = constant(0.25);
    edited->noteAuthoredMathWritten();
    const Frame refreshed = render({edited}, true, 7.25, 1);
    const Frame exactEdited = render({edited}, false, 7.25, 1);
    check(refreshed.lit > 0 && refreshed.rgba == exactEdited.rgba,
          "a density revision cannot retain the former empty support envelope");

    // Rebuild column/word offsets on extent changes, before any proof-off
    // frame can evict the memo and conceal stale geometry.
    *edited->volumeDensity = *media.front()->volumeDensity;
    edited->noteAuthoredMathWritten();
    render({edited}, true, 7.25, 1);
    edited->scale *= glm::vec3(1.25f, .85f, .75f);
    edited->noteAuthoredMathWritten();
    const Frame resized = render({edited}, true, 7.25, 1);
    const Frame resizedExact = render({edited}, false, 7.25, 1);
    check(resized.proven > 0 && resized.rgba == resizedExact.rgba,
          "changed half-extents cannot reuse stale column bounds or payload offsets");


    // Per-channel dependency frontier (Opus, 2026-10-12; Formation Rete
    // Incremental Maintenance §2). Through the real renderer: editing a
    // medium's emission must not rebuild its zero-density proof, and editing
    // its density must rebuild exactly one, still matching the exact path.
    {
        *edited->volumeDensity = *media.front()->volumeDensity;
        edited->noteAuthoredMathWritten();
        render({edited}, true, 7.25, 1);   // the proof for this density now exists
        Property* emission = edited->findProperty("volume.emission.ast");
        Property* density = edited->findProperty("volume.density.ast");
        const auto& other = media.size() > 1 ? media[1] : media.front();
        check(emission && emission->setValue(PropertyValue(other->volumeEmission->toJson().dump())),
              "emission accepts another medium's emission tree");
        const Frame afterEmission = render({edited}, true, 7.25, 1);
        check(afterEmission.firstProofBuilds == 0,
              "an emission edit rebuilds no zero-density proof (per-channel revision)");
        check(density && density->setValue(PropertyValue(other->volumeDensity->toJson().dump())),
              "density accepts another medium's density tree");
        const Frame afterDensity = render({edited}, true, 7.25, 1);
        const Frame afterDensityExact = render({edited}, false, 7.25, 1);
        check(afterDensity.firstProofBuilds == 1 && afterDensity.rgba == afterDensityExact.rgba,
              "a density edit rebuilds exactly one proof and stays byte-identical to exact");
    }

    // Unknown temporal support must fail open, including the inside camera.
    *edited->volumeDensity = OntoMath::Piecewise::fromJson(nlohmann::json::parse(
        R"({"input":"x","pieces":[{"mathNode":{"op":1,"var":"t"}}]})"));
    edited->noteAuthoredMathWritten();
    renderer.setCamera(glm::lookAt(glm::vec3(0, 50, 75), glm::vec3(40, 62, 110), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), {0, 50, 75});
    const Frame unknown = render({edited}, true, 0.25, 1);
    const Frame unknownExact = render({edited}, false, 0.25, 1);
    check(unknown.proven == 0 && unknown.rgba == unknownExact.rgba,
          "unknown time-dependent support retains exact inside sampling");

    // A proof-bearing member must not erase an overlapping member whose
    // time-dependent density has no support certificate. Reorder the set so
    // proof and evaluator offsets cannot be accidentally coupled to index 0.
    for (const auto& mixed : std::vector<std::vector<std::shared_ptr<geom::FieldNode>>>{
            {media.front(), edited}, {edited, media.front()}}) {
        const Frame mixedProof = render(mixed, true, 0.25, 1);
        const Frame mixedExact = render(mixed, false, 0.25, 1);
        check(mixedProof.proven > 0 && mixedProof.rgba == mixedExact.rgba,
              "mixed known/unknown support preserves each member after reordering");
    }

    // Positive-budget fallback samples the entire lattice. Support traversal
    // must stay exact there too, rather than inheriting the opacity heuristic.
    renderer.setVolumeOutputErrorBudget(1.0 / 255.0);
    const Frame fullLatticeProof = render(media, true, 7.25, 1);
    const Frame fullLatticeExact = render(media, false, 7.25, 1);
    check(fullLatticeProof.proven > 0 && fullLatticeProof.rgba == fullLatticeExact.rgba,
          "certified-zero traversal preserves the positive-budget full lattice");
    renderer.setVolumeOutputErrorBudget(0.0);

    // The support index is device-bound through the resident proof payload;
    // recreate an actual device and rebuild all views/buffers, not just WGSL.
    const auto restoreScene = [&] {
        renderer.setRadianceSources({source}, 1);
        renderer.setLight(source.position, source.ambientRadiance, source.diffuseRadiance, source.specularRadiance);
        renderer.setLightingEnabled(light.enabled);
        renderer.setCamera(glm::lookAt(glm::vec3(0, 50, 75), glm::vec3(40, 62, 110), glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), {0, 50, 75});
    };
    restoreScene();
    const Frame beforeDevice = render(media, true, 7.25, 1);
    renderer.shutdown();
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    wgpu::Device replacement;
    if (!replacement.init()) { std::printf("FAIL: replacement device\n"); return 1; }
    wgpuQueueRelease(gpu.queue); wgpuDeviceRelease(gpu.device);
    wgpuAdapterRelease(gpu.adapter); wgpuInstanceRelease(gpu.instance);
    gpu = replacement;
    if (!renderer.init(gpu)) { std::printf("FAIL: replacement renderer\n"); return 1; }
    target = wgpuDeviceCreateTexture(gpu.device, &td);
    view = wgpuTextureCreateView(target, nullptr);
    readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    restoreScene();
    const Frame afterDevice = render(media, true, 7.25, 1);
    check(afterDevice.proven > 0 && afterDevice.rgba == beforeDevice.rgba && afterDevice.warmProofUploads == 0,
          "replacement device rebuilds column payloads and retains no stale handles or pixels");

    // A cached support shortcut previously passed every small capture but
    // changed one spawn pixel at native stress size. Keep that size in the
    // existing witness: finite small images cannot certify all ray boundaries.
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    width = 2560; height = 1440; stride = (width * 4 + 255) & ~255u;
    td.size = {width, height, 1}; bd.size = uint64_t(stride) * height;
    target = wgpuDeviceCreateTexture(gpu.device, &td);
    view = wgpuTextureCreateView(target, nullptr);
    readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    lastRgba.resize(size_t(width) * height * 4);
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
    const Frame stressProof = render(media, true, 0.0, 1);
    const Frame stressExact = render(media, false, 0.0, 1);
    check(stressProof.proven > 0 && stressProof.rgba == stressExact.rgba,
          "full 2560x1440 spawn preserves every RGBA byte across proof boundaries");

    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("webgpu_volume_zero_proof_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
