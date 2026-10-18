// Native WebGPU witness for ray-domain transport Rung 2f-native: support
// certificates (Rendering::deriveVolumeSupportCertificates, lowered by
// sdfwgsl::lowerVolumeSupportKillers) may only remove work, never light. Real
// Northern Veil media render with the certificates off and on -- alone (grid
// proof off, so the certificates carry every skip) and together with the
// production grid proof -- and the full RGBA framebuffer must be byte-identical
// while the certificates actually applied.
//
// Evidence class: same-backend pixel parity on whatever adapter runs it. On the
// cloud lavapipe lane that is llvmpipe, NOT Metal; timings are printed, never
// gated, and are never a frame-time claim. The device tolerance the shader uses
// (kVolumeSupportDeviceTolerance) is declared, not proved.
//
// Claude Opus 5.5 · Claude Code · session 01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
// Zach: "KEEEEPP GOINGGNNGG". Harness pattern: webgpu_volume_zero_proof_test
// (Opus 2026-10-09; Codex / GPT-6.1 Sol 2026-10-10).
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "support/test_harness.hpp"

#include <webgpu/wgpu.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
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

// Count of differing bytes and the largest channel difference: a failed
// parity check reports how far it fell, never just that it fell.
std::string byteDiff(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    if (a.size() != b.size()) return "sizes differ";
    size_t count = 0; int worst = 0;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) { ++count; worst = std::max(worst, std::abs(int(a[i]) - int(b[i]))); }
    return std::to_string(count) + " bytes differ, max " + std::to_string(worst);
}

struct Frame {
    uint64_t lit = 0;
    double ms = 0.0;
    uint32_t proven = 0;
    uint32_t killers = 0;
    uint32_t eligible = 0;   // media whose certificates are attached (auto eligibility)
    uint32_t firstCertificateBuilds = 0;
    uint32_t firstCompiles = 0;
    uint32_t warmCompiles = 0;
    size_t warmParameterUploads = 0;
    std::vector<unsigned char> rgba;
};

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_support_certificates_test...\n");

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
    // Parity sections A/B EVERY medium (auto eligibility off); the eligibility
    // section below turns the Kernel's per-medium cost ranking back on.
    bool autoEligibility = false;
    auto render = [&](const std::vector<std::shared_ptr<geom::FieldNode>>& set, bool proofOn,
                      bool certificatesOn, double t, int timedFrames) {
        renderer.setVolumeZeroProofEnabled(proofOn);
        renderer.setVolumeSupportCertificatesEnabled(certificatesOn);
        renderer.setVolumeSupportCertificatesAutoEligibility(autoEligibility);
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
        auto sync = [&]() -> uint64_t {
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
            uint64_t lit = 0;
            for (uint32_t y = 0; y < height; ++y) {
                std::memcpy(lastRgba.data() + size_t(y) * width * 4u, px + size_t(y) * stride, width * 4u);
                for (uint32_t x = 0; x < width; ++x) {
                    const size_t at = size_t(y) * stride + x * 4;
                    lit += (px[at] + px[at + 1] + px[at + 2]) > 6;
                }
            }
            wgpuBufferUnmap(readback);
            return lit;
        };
        Frame frame;
        drawOnce();
        frame.firstCompiles = renderer.frameStats().volumeProgramCompiles;
        frame.firstCertificateBuilds = renderer.frameStats().volumeSupportCertificateBuilds;
        sync();
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < timedFrames; ++i) {
            drawOnce();
            frame.warmCompiles += renderer.frameStats().volumeProgramCompiles;
            frame.warmParameterUploads += renderer.frameStats().volumeParameterBytesUploaded;
        }
        sync();
        frame.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() /
                   std::max(timedFrames, 1);
        drawOnce();
        frame.proven = renderer.frameStats().volumeZeroProofCellsProven;
        frame.killers = renderer.frameStats().volumeSupportCertificateKillers;
        frame.eligible = renderer.frameStats().volumeSupportCertificateEligibleMedia;
        frame.warmCompiles += renderer.frameStats().volumeProgramCompiles;
        frame.lit = sync();
        frame.rgba = lastRgba;
        return frame;
    };

    // The shader really carries the lane only when asked: the off lane is the
    // pre-existing marcher text.
    {
        std::vector<sdfwgsl::VolumeProgramInput> inputs;
        for (const auto& f : media) inputs.push_back({f->volumeDensity.get()});
        const auto off = sdfwgsl::compileVolumeSet(inputs, true);
        const auto on = sdfwgsl::composeVolumeSet(inputs, {}, true, std::vector<bool>(inputs.size(), true));
        check(off.ok && off.wgsl.find("volumeSupportReach") == std::string::npos,
              "certificate lane off: no certificate code in the fused marcher");
        check(on.ok && on.wgsl.find("volumeSupportReach_0") != std::string::npos &&
                  on.wgsl.find("volumeSupportExit(") != std::string::npos,
              "certificate lane on: per-member reach functions and the exit join the marcher");
        const auto killers = sdfwgsl::lowerVolumeSupportKillers(media.front()->volumeDensity.get());
        std::printf("       first medium: %zu candidates of %zu switches, %zu operand constants\n",
                    killers.switches.size(), killers.formSwitches, killers.params.size());
        check(killers.ok && !killers.switches.empty(), "the curtain density lowers certificate candidates");
    }

    struct Case { std::string label; std::vector<std::shared_ptr<geom::FieldNode>> set; };
    const std::vector<Case> cases = {
        {"all " + std::to_string(media.size()) + " media (fused set pipeline)", media},
        {"one medium (single pipeline)", {media.front()}},
    };
    for (const auto& c : cases) {
        for (double t : {0.0, 7.25}) {
            const Frame exact = render(c.set, false, false, t, 3);
            const Frame certified = render(c.set, false, true, t, 3);
            const Frame proved = render(c.set, true, false, t, 3);
            const Frame both = render(c.set, true, true, t, 3);
            std::printf("       %s t=%.2f (adapter-relative, not a frame-time claim): exact %.2f ms, "
                        "certificates %.2f ms, grid %.2f ms, grid+certificates %.2f ms; killers %u, lit %llu\n",
                        c.label.c_str(), t, exact.ms, certified.ms, proved.ms, both.ms, certified.killers,
                        (unsigned long long)exact.lit);
            check(exact.lit > 0, c.label + ": the media are visible");
            check(exact.killers == 0 && certified.killers > 0 && both.killers > 0,
                  c.label + ": certificates are off, then actually supplied");
            check(certified.warmCompiles == 0 && both.warmCompiles == 0 && certified.warmParameterUploads == 0,
                  c.label + ": the certificate lane compiles once and uploads nothing warm");
            check(exact.rgba == certified.rgba,
                  c.label + " t=" + std::to_string(t) + ": certificates alone keep every RGBA byte");
            check(proved.rgba == both.rgba,
                  c.label + " t=" + std::to_string(t) + ": certificates over the grid proof keep every RGBA byte");
        }
    }

    // Opt-in, report-only ABBA timing (EARTHCALL_CERTIFICATE_TIMING=1): the
    // adapter's relative cost of each lane. Never gated; on lavapipe it is a
    // CPU rasterizer, so it ranks nothing about Metal.
    if (const char* timing = std::getenv("EARTHCALL_CERTIFICATE_TIMING"); timing && *timing == '1') {
        struct Lane { const char* label; bool proof, certificates; };
        const Lane lanes[] = {{"exact", false, false}, {"certificates", false, true},
                              {"grid", true, false}, {"grid+certificates", true, true}};
        for (const auto& c : cases) {
            for (const auto& [a, b] : {std::pair<int, int>{0, 1}, std::pair<int, int>{2, 3}}) {
                std::vector<double> ta, tb;
                for (int round = 0; round < 4; ++round) {
                    const bool aFirst = round % 2 == 0;
                    for (int k = 0; k < 2; ++k) {
                        const auto& lane = lanes[(k == 0) == aFirst ? a : b];
                        const double ms = render(c.set, lane.proof, lane.certificates, 7.25, 8).ms;
                        ((k == 0) == aFirst ? ta : tb).push_back(ms);
                    }
                }
                std::sort(ta.begin(), ta.end()); std::sort(tb.begin(), tb.end());
                std::printf("TIMING %s: %s median %.2f ms vs %s median %.2f ms (ABBA x4, 8 frames each)\n",
                            c.label.c_str(), lanes[a].label, 0.5 * (ta[1] + ta[2]),
                            lanes[b].label, 0.5 * (tb[1] + tb[2]));
            }
        }
    }

    struct View { const char* label; glm::vec3 eye; glm::vec3 target; };
    const View views[] = {
        {"inside", {0, 50, 75}, {40, 62, 110}},
        {"inside Emerald/Violet support", {-74.9f, 63, 62.72732f}, {0, 55, 90}},
        {"reverse", {0, 60, 160}, {0, 55, 75}},
        {"axis-parallel", {0, 50, 0}, {0, 50, 100}},
        {"side", {90, 55, 90}, {-30, 55, 90}},
        {"fold crossing forward", {-72.5f, 50, 70}, {65, 60, 95}},
    };
    for (const auto& v : views) {
        renderer.setCamera(glm::lookAt(v.eye, v.target, glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), v.eye);
        for (const auto& c : cases) {
            const Frame exact = render(c.set, false, false, 7.25, 0);
            const Frame certified = render(c.set, false, true, 7.25, 0);
            const Frame both = render(c.set, true, true, 7.25, 0);
            check(exact.rgba == certified.rgba && exact.rgba == both.rgba,
                  c.label + " " + v.label + ": certificates preserve every RGBA byte");
        }
    }

    // Translated media: certificates are local to the box, the pad scales with
    // world coordinates. Far from the origin the pad must still hold.
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 10000.0f), eye);
    for (const auto shift : {glm::vec3(100, -50, 200), glm::vec3(-1024, 32, -512)}) {
        auto translated = geom::FieldNode::fromJson(zone.at("spatialFields").front());
        translated->origin += shift;
        renderer.setCamera(glm::lookAt(shift + glm::vec3(0, 50, 0), shift + glm::vec3(0, 50, 100), glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 10000.0f),
                           shift + glm::vec3(0, 50, 0));
        const auto exact = render({translated}, false, false, 7.25, 0);
        const auto certified = render({translated}, false, true, 7.25, 0);
        check(exact.lit > 0 && certified.killers > 0 && exact.rgba == certified.rgba,
              "translated medium keeps every byte under certificates");
    }

    // Invalidation: the payload lives for density revision × extent. Render the
    // certificate lane FIRST after each edit so no other lane evicts the memo.
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
    auto edited = geom::FieldNode::fromJson(zone.at("spatialFields").front());
    render({edited}, false, true, 7.25, 0);
    {
        const auto& other = media.size() > 1 ? media[1] : media.front();
        Property* emission = edited->findProperty("volume.emission.ast");
        Property* density = edited->findProperty("volume.density.ast");
        check(emission && emission->setValue(PropertyValue(other->volumeEmission->toJson().dump())),
              "emission accepts another medium's emission tree");
        const Frame afterEmission = render({edited}, false, true, 7.25, 0);
        check(afterEmission.firstCertificateBuilds == 0,
              "an emission edit rebuilds no certificate payload (per-channel revision)");
        check(density && density->setValue(PropertyValue(other->volumeDensity->toJson().dump())),
              "density accepts another medium's density tree");
        const Frame afterDensity = render({edited}, false, true, 7.25, 0);
        const Frame afterDensityExact = render({edited}, false, false, 7.25, 0);
        check(afterDensity.firstCertificateBuilds == 1 && afterDensity.killers > 0 &&
                  afterDensity.rgba == afterDensityExact.rgba,
              "a density edit rebuilds exactly one payload and stays byte-identical to exact");
        edited->scale *= glm::vec3(1.25f, .85f, .75f);
        edited->noteAuthoredMathWritten();
        const Frame resized = render({edited}, false, true, 7.25, 0);
        const Frame resizedExact = render({edited}, false, false, 7.25, 0);
        check(resized.firstCertificateBuilds == 1 && resized.rgba == resizedExact.rgba,
              "a changed extent re-derives certificates; no stale slack, L or offsets");
    }
    // A density no switch can kill refuses: count 0, exact path, same pixels.
    {
        *edited->volumeDensity = OntoMath::Piecewise::fromJson(nlohmann::json::parse(
            R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.25,"factors":{}}]}}}]})"));
        edited->noteAuthoredMathWritten();
        const Frame refused = render({edited}, false, true, 7.25, 0);
        const Frame refusedExact = render({edited}, false, false, 7.25, 0);
        check(refused.killers == 0 && refused.lit > 0 && refused.rgba == refusedExact.rgba,
              "a density without a killer attaches nothing and keeps the exact path");
    }

    // Positive-budget mode samples the entire lattice: certificates must stay exact there too.
    renderer.setVolumeOutputErrorBudget(1.0 / 255.0);
    {
        const Frame full = render(media, true, true, 7.25, 0);
        const Frame fullExact = render(media, false, false, 7.25, 0);
        check(full.killers > 0 && full.rgba == fullExact.rgba,
              "certificates preserve the positive-budget full lattice");
    }
    renderer.setVolumeOutputErrorBudget(0.0);

    std::vector<std::shared_ptr<geom::FieldNode>> prismWing;   // reused by the eligibility section
    // Prism Cathedral wing B: radial clouds, shells, tori, a CSG crescent and a
    // noise cloud whose zero-proof grid proves 0% of cells while the CPU census
    // certifies 86-99.7% of samples (volume_ray_radial_segmentation_test). This
    // is where certificates are a substitute rather than a complement.
    {
        const std::string prismPath = TestSupport::resolveRealWorldPath("saves/zones/Prism Cathedral/zone.ecform");
        const std::string prismJson = TestSupport::resolveRealWorldPath("saves/zones/Prism Cathedral/zone.json");
        nlohmann::json prism;
        if (std::filesystem::exists(prismJson)) {
            std::ifstream pin(prismJson);
            prism = nlohmann::json::parse(pin);
        } else if (std::filesystem::exists(prismPath)) {
            std::ifstream pin(prismPath, std::ios::binary);
            const std::vector<uint8_t> pbytes((std::istreambuf_iterator<char>(pin)), std::istreambuf_iterator<char>());
            prism = nlohmann::json::parse(nlohmann::json::from_msgpack(pbytes).at("MigrationRoot").get<std::string>());
        }
        std::vector<std::shared_ptr<geom::FieldNode>> wing;
        if (prism.is_object() && prism.contains("spatialFields"))
            for (const auto& f : prism.at("spatialFields")) {
                auto node = geom::FieldNode::fromJson(f);
                if (node && node->volumeDensity && !node->volumeDensity->pieces.empty() &&
                    node->getIdentifier().rfind("prism.wingB.", 0) == 0)
                    wing.push_back(node);
            }
        prismWing = wing;
        if (wing.empty()) {
            std::printf("  SKIP: Prism Cathedral wing B media not present (not a pass)\n");
        } else {
            const bool timing = std::getenv("EARTHCALL_CERTIFICATE_TIMING") &&
                                *std::getenv("EARTHCALL_CERTIFICATE_TIMING") == '1';
            glm::vec3 lo(3e38f), hi(-3e38f);
            for (const auto& m : wing) {
                const glm::vec3 h = glm::abs(m->scale);
                lo = glm::min(lo, m->origin - h); hi = glm::max(hi, m->origin + h);
                const float r = std::max(h.x, std::max(h.y, h.z));
                const glm::vec3 at = m->origin + glm::vec3(0.35f * r, 0.25f * r, 2.6f * r);
                renderer.setCamera(glm::lookAt(at, m->origin, glm::vec3(0, 1, 0)),
                                   glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.05f, 2000.0f), at);
                const Frame exact = render({m}, false, false, 0.75, 0);
                const Frame grid = render({m}, true, false, 0.75, 0);
                const Frame certified = render({m}, false, true, 0.75, 0);
                const Frame both = render({m}, true, true, 0.75, 0);
                std::printf("       %s: killers %u, grid cells proven %u, lit %llu\n", m->getIdentifier().c_str(),
                            certified.killers, grid.proven, (unsigned long long)exact.lit);
                if (exact.rgba != certified.rgba || grid.rgba != both.rgba)
                    std::printf("       %s: alone %s; over grid %s\n", m->getIdentifier().c_str(),
                                byteDiff(exact.rgba, certified.rgba).c_str(), byteDiff(grid.rgba, both.rgba).c_str());
                for (double probeT : {0.0, 0.75 * 3.0}) {
                    if (exact.rgba == certified.rgba) break;
                    const Frame e2 = render({m}, false, false, probeT, 0);
                    const Frame c2 = render({m}, false, true, probeT, 0);
                    std::printf("       %s probe t=%.2f: %s\n", m->getIdentifier().c_str(), probeT,
                                byteDiff(e2.rgba, c2.rgba).c_str());
                }
                check(exact.lit > 0 && exact.rgba == certified.rgba && grid.rgba == both.rgba,
                      m->getIdentifier() + ": certificates keep every RGBA byte");
                if (timing) {
                    std::vector<double> tg, tb;
                    for (int round = 0; round < 4; ++round) {
                        const bool gFirst = round % 2 == 0;
                        for (int k = 0; k < 2; ++k) {
                            const bool g = (k == 0) == gFirst;
                            (g ? tg : tb).push_back(render({m}, true, !g, 0.75, 8).ms);
                        }
                    }
                    std::sort(tg.begin(), tg.end()); std::sort(tb.begin(), tb.end());
                    std::printf("TIMING %s: grid median %.2f ms vs grid+certificates median %.2f ms\n",
                                m->getIdentifier().c_str(), 0.5 * (tg[1] + tg[2]), 0.5 * (tb[1] + tb[2]));
                }
            }
            // Emitter precedence witness (found by this lane, 2026-10-17). A
            // multi-term ScalarLeaf under Sub/Scale was emitted as
            // `(a - (t1) + (t2))`, i.e. (a - t1) + t2. Volume lowering hid it for
            // time-only leaves (prepared slots), not for spatial ones. Two
            // algebraically identical densities, certificates OFF, must agree.
            {
                const auto shell = [](const std::string& radius) {
                    return OntoMath::Piecewise::fromJson(nlohmann::json::parse(
                        R"({"input":"x","pieces":[{"mathNode":{"op":6,"children":[)"
                        R"({"op":0,"scalarForm":{"terms":[{"c":1.8,"factors":{}}]}},)"
                        R"({"op":26,"children":[{"op":5,"children":[{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},)"
                        R"({"op":23,"children":[{"op":25,"children":[{"op":5,"children":[{"op":11,"children":[{"op":1,"var":"p"}]},)" +
                        radius +
                        R"(]}]},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}]}]},)"
                        R"({"op":0,"scalarForm":{"terms":[{"c":0.0,"factors":{}}]}},{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}]}]}}]})"));
                };
                auto leafShell = geom::FieldNode::fromJson(prism.at("spatialFields").front());
                for (const auto& f : prism.at("spatialFields"))
                    if (f.dump().find("prism.wingB.time-breathing-nebula") != std::string::npos)
                        leafShell = geom::FieldNode::fromJson(f);
                auto splitShell = geom::FieldNode::fromJson(nlohmann::json::parse(leafShell->toJson().dump()));
                *leafShell->volumeDensity = shell(
                    R"({"op":0,"scalarForm":{"terms":[{"c":2.4,"factors":{}},{"c":0.3,"factors":{"y":1.0}}]}})");
                leafShell->noteAuthoredMathWritten();
                *splitShell->volumeDensity = shell(
                    R"({"op":4,"children":[{"op":0,"scalarForm":{"terms":[{"c":2.4,"factors":{}}]}},)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":0.3,"factors":{"y":1.0}}]}}]})");
                splitShell->noteAuthoredMathWritten();
                const glm::vec3 at = leafShell->origin + glm::vec3(1.5f, 1.2f, 11.0f);
                renderer.setCamera(glm::lookAt(at, leafShell->origin, glm::vec3(0, 1, 0)),
                                   glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.05f, 2000.0f), at);
                const Frame leaf = render({leafShell}, false, false, 0.75, 0);
                const Frame split = render({splitShell}, false, false, 0.75, 0);
                int worst = 0;
                for (size_t i = 0; i < leaf.rgba.size(); ++i)
                    worst = std::max(worst, std::abs(int(leaf.rgba[i]) - int(split.rgba[i])));
                std::printf("       precedence witness: leaf vs split %s (lit %llu)\n",
                            byteDiff(leaf.rgba, split.rgba).c_str(), (unsigned long long)leaf.lit);
                check(leaf.lit > 0 && worst <= 1,
                      "a multi-term leaf under Sub renders as its parenthesized sum (no worse than one level of rounding)");
                const Frame leafCertified = render({leafShell}, false, true, 0.75, 0);
                check(leafCertified.killers > 0 && leafCertified.rgba == leaf.rgba,
                      "the spatial multi-term shell keeps every byte under certificates");

            // Codex P1 (2026-10-17): D = −min(F, 0), F = ((A + x) − A) − x with
            // A = 2^25. The algebra says D ≡ 0 (and once certified it with an
            // infinite reach); f32 execution makes D ∈ [0, 2]. Certificates must
            // keep every byte: their δ now comes from the authored tree.
            {
                const auto cancelled = OntoMath::Piecewise::fromJson(nlohmann::json::parse(
                    R"({"input":"x","pieces":[{"mathNode":{"op":6,"children":[{"op":0,"scalarForm":{"terms":[{"c":-1.0,"factors":{}}]}},)"
                    R"({"op":20,"children":[{"op":5,"children":[{"op":5,"children":[{"op":4,"children":[)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":33554432.0,"factors":{}}]}},{"op":1,"var":"x"}]},)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":33554432.0,"factors":{}}]}}]},{"op":1,"var":"x"}]},)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":0.0,"factors":{}}]}}]}]}}]})"));
                auto cancelMedium = geom::FieldNode::fromJson(nlohmann::json::parse(leafShell->toJson().dump()));
                *cancelMedium->volumeDensity = cancelled;
                cancelMedium->noteAuthoredMathWritten();
                const Frame exactCancel = render({cancelMedium}, false, false, 0.75, 0);
                const Frame certCancel = render({cancelMedium}, false, true, 0.75, 0);
                std::printf("       cancellation witness: lit %llu, killers %u, %s\n", (unsigned long long)exactCancel.lit,
                            certCancel.killers, byteDiff(exactCancel.rgba, certCancel.rgba).c_str());
                check(exactCancel.rgba == certCancel.rgba,
                      "an f32 cancellation the algebra erased keeps every byte under certificates");

                // Fast-math compilers may reassociate ((A + x) − A) − x into the
                // exact 0 (the Sun's M5 frames were black). No compiler expands a
                // polynomial identity: F = (x + A)·(x + A) − A·A − 2A·x − x·x is
                // 0 in the algebra, but at A = 2^12 its f32 terms reach 2^24 and
                // execution keeps errors of a few units, so D = −min(F, 0) stays
                // lit on every backend. Certificates must keep every byte.
                const auto polyCancelled = OntoMath::Piecewise::fromJson(nlohmann::json::parse(
                    R"({"input":"x","pieces":[{"mathNode":{"op":6,"children":[{"op":0,"scalarForm":{"terms":[{"c":-1.0,"factors":{}}]}},)"
                    R"({"op":20,"children":[{"op":5,"children":[{"op":5,"children":[{"op":5,"children":[)"
                    R"({"op":6,"children":[{"op":4,"children":[{"op":1,"var":"x"},{"op":0,"scalarForm":{"terms":[{"c":4096.0,"factors":{}}]}}]},)"
                    R"({"op":4,"children":[{"op":1,"var":"x"},{"op":0,"scalarForm":{"terms":[{"c":4096.0,"factors":{}}]}}]}]},)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":16777216.0,"factors":{}}]}}]},)"
                    R"({"op":6,"children":[{"op":0,"scalarForm":{"terms":[{"c":8192.0,"factors":{}}]}},{"op":1,"var":"x"}]}]},)"
                    R"({"op":6,"children":[{"op":1,"var":"x"},{"op":1,"var":"x"}]}]},)"
                    R"({"op":0,"scalarForm":{"terms":[{"c":0.0,"factors":{}}]}}]}]}}]})"));
                auto polyMedium = geom::FieldNode::fromJson(nlohmann::json::parse(leafShell->toJson().dump()));
                *polyMedium->volumeDensity = polyCancelled;
                polyMedium->noteAuthoredMathWritten();
                const Frame exactPoly = render({polyMedium}, false, false, 0.75, 0);
                const Frame certPoly = render({polyMedium}, false, true, 0.75, 0);
                std::printf("       polynomial cancellation witness: lit %llu, killers %u, %s\n",
                            (unsigned long long)exactPoly.lit, certPoly.killers,
                            byteDiff(exactPoly.rgba, certPoly.rgba).c_str());
                check(exactPoly.lit > 0 && exactPoly.rgba == certPoly.rgba,
                      "a polynomial identity the algebra cancels (lit on every backend) keeps every byte under certificates");
            }
            }

            const glm::vec3 centre = 0.5f * (lo + hi);
            const float r = glm::length(hi - lo);
            const glm::vec3 at = centre + glm::vec3(0.2f * r, 0.15f * r, 0.9f * r);
            renderer.setCamera(glm::lookAt(at, centre, glm::vec3(0, 1, 0)),
                               glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.05f, 4000.0f), at);
            const Frame exact = render(wing, false, false, 0.75, 0);
            const Frame certified = render(wing, false, true, 0.75, 0);
            const Frame both = render(wing, true, true, 0.75, 0);
            const Frame grid = render(wing, true, false, 0.75, 0);
            check(exact.lit > 0 && certified.killers > 0 && exact.rgba == certified.rgba && grid.rgba == both.rgba,
                  "Prism Cathedral wing B fused set: certificates keep every RGBA byte");
            if (timing) {
                std::vector<double> tg, tb;
                for (int round = 0; round < 4; ++round) {
                    const bool gFirst = round % 2 == 0;
                    for (int k = 0; k < 2; ++k) {
                        const bool g = (k == 0) == gFirst;
                        (g ? tg : tb).push_back(render(wing, true, !g, 0.75, 8).ms);
                    }
                }
                std::sort(tg.begin(), tg.end()); std::sort(tb.begin(), tb.end());
                std::printf("TIMING Prism wing B fused (%zu media): grid median %.2f ms vs grid+certificates median %.2f ms\n",
                            wing.size(), 0.5 * (tg[1] + tg[2]), 0.5 * (tb[1] + tb[2]));
            }
        }
    }

    // Rung 2f-native eligibility (auto mode, the default within an enabled
    // lane). The Kernel's cost estimate should keep Northern Veil on the grid
    // lane (measured +16–49% with certificates) and admit Prism wing B (up to
    // 4.8× cheaper). Pixels stay byte-identical either way; timings report only.
    autoEligibility = true;
    {
        const bool timing = std::getenv("EARTHCALL_CERTIFICATE_TIMING") &&
                            *std::getenv("EARTHCALL_CERTIFICATE_TIMING") == '1';
        auto abba = [&](const std::vector<std::shared_ptr<geom::FieldNode>>& set, const char* label) {
            std::vector<double> tg, ta;
            for (int round = 0; round < 4; ++round) {
                const bool gFirst = round % 2 == 0;
                for (int k = 0; k < 2; ++k) {
                    const bool g = (k == 0) == gFirst;
                    (g ? tg : ta).push_back(render(set, true, !g, 0.75, 8).ms);
                }
            }
            std::sort(tg.begin(), tg.end()); std::sort(ta.begin(), ta.end());
            std::printf("TIMING %s: grid median %.2f ms vs grid+auto-certificates median %.2f ms\n",
                        label, 0.5 * (tg[1] + tg[2]), 0.5 * (ta[1] + ta[2]));
        };
        renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                           glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
        const Frame nvExact = render(media, true, false, 7.25, 0);
        const Frame nvAuto = render(media, true, true, 7.25, 0);
        std::printf("       auto eligibility, Northern Veil: %u of %zu media eligible, %u killers enabled\n",
                    nvAuto.eligible, media.size(), nvAuto.killers);
        check(nvAuto.eligible == 0 && nvAuto.rgba == nvExact.rgba,
              "auto eligibility keeps every Northern Veil curtain on the grid lane, every byte intact");
        // Per curtain (2026-10-18, eleventh pass). Ranking killer subsets for
        // eligibility promoted emerald and cyan; the M5 ran emerald 16–20% slower
        // and cyan even, so eligibility stays with every killer enabled. Rendered
        // alone after the forced sections above, each curtain also witnesses that
        // a changed killer set recompiles with its payload (it once did not).
        bool idleSeen = false;
        uint32_t idleCompiles = 0;
        for (const auto& m : media) {
            const Frame grid = render({m}, true, false, 7.25, 0);
            const Frame autoFrame = render({m}, true, true, 7.25, 0);
            std::printf("       auto eligibility, %s: %s (%u killers enabled)\n", m->getIdentifier().c_str(),
                        autoFrame.eligible ? "certificates attached" : "grid lane only", autoFrame.killers);
            check(grid.rgba == autoFrame.rgba, m->getIdentifier() + ": auto eligibility keeps every byte");
            if (!autoFrame.eligible && !idleSeen) {
                idleSeen = true;
                idleCompiles = autoFrame.firstCompiles + autoFrame.warmCompiles;
            }
            if (timing && autoFrame.eligible) abba({m}, m->getIdentifier().c_str());
        }
        // The Sun's M5 measured the idle certificate variant 18–23% slower: with
        // nothing eligible the lane must select the lane-off program itself.
        if (idleSeen) {
            std::printf("       auto eligibility, an ineligible curtain: %u program compiles after enabling the lane\n",
                        idleCompiles);
            check(idleCompiles == 0,
                  "with nothing eligible, enabling the lane reuses the lane-off program (no certificate shader variant)");
        }
        if (timing) abba(media, "Northern Veil fused (auto)");
        if (!prismWing.empty()) {
            uint32_t eligible = 0;
            for (const auto& m : prismWing) {
                const glm::vec3 h = glm::abs(m->scale);
                const float r = std::max(h.x, std::max(h.y, h.z));
                const glm::vec3 at = m->origin + glm::vec3(0.35f * r, 0.25f * r, 2.6f * r);
                renderer.setCamera(glm::lookAt(at, m->origin, glm::vec3(0, 1, 0)),
                                   glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.05f, 2000.0f), at);
                const Frame grid = render({m}, true, false, 0.75, 0);
                const Frame autoFrame = render({m}, true, true, 0.75, 0);
                eligible += autoFrame.eligible;
                std::printf("       auto eligibility, %s: %s\n", m->getIdentifier().c_str(),
                            autoFrame.eligible ? "certificates attached" : "grid lane only");
                check(grid.rgba == autoFrame.rgba, m->getIdentifier() + ": auto eligibility keeps every byte");
                if (timing && autoFrame.eligible) abba({m}, m->getIdentifier().c_str());
            }
            check(eligible * 2 > prismWing.size(), "auto eligibility admits most of Prism wing B (the grid proves none of it)");
        }
        // Every other medium the census ranks eligible gets a native witness
        // (the Firmament's direction 3, 2026-10-18): Borealis v1 ×2 and Sky-Island
        // sea-of-clouds were promoted by the tenth pass, unmeasured until now.
        for (const auto& [zoneName, prefix] : std::vector<std::pair<std::string, std::string>>{
                 {"Borealis Sanctuary", "borealis."}, {"Sky-Island Sanctuary", "sky.sanctuary."}}) {
            const std::string json = TestSupport::resolveRealWorldPath("saves/zones/" + zoneName + "/zone.json");
            const std::string ecform = TestSupport::resolveRealWorldPath("saves/zones/" + zoneName + "/zone.ecform");
            nlohmann::json z;
            if (std::filesystem::exists(json)) {
                std::ifstream zin(json);
                z = nlohmann::json::parse(zin);
            } else if (std::filesystem::exists(ecform)) {
                std::ifstream zin(ecform, std::ios::binary);
                const std::vector<uint8_t> zb((std::istreambuf_iterator<char>(zin)), std::istreambuf_iterator<char>());
                z = nlohmann::json::parse(nlohmann::json::from_msgpack(zb).at("MigrationRoot").get<std::string>());
            }
            uint32_t found = 0;
            if (z.is_object() && z.contains("spatialFields"))
                for (const auto& f : z.at("spatialFields")) {
                    auto m = geom::FieldNode::fromJson(f);
                    if (!m || !m->volumeDensity || m->volumeDensity->pieces.empty() ||
                        m->getIdentifier().rfind(prefix, 0) != 0)
                        continue;
                    ++found;
                    // From above at an angle: Sky-Island's sea is a thin slab a level
                    // camera grazes, and its halo mist is small and faint.
                    const glm::vec3 h = glm::abs(m->scale);
                    const float r = glm::length(h);
                    const glm::vec3 at = m->origin + glm::normalize(glm::vec3(0.35f, 0.6f, 1.0f)) * (1.6f * r);
                    renderer.setCamera(glm::lookAt(at, m->origin, glm::vec3(0, 1, 0)),
                                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.05f, 8.0f * r + 10.0f), at);
                    const Frame grid = render({m}, true, false, 0.75, 0);
                    const Frame autoFrame = render({m}, true, true, 0.75, 0);
                    std::size_t visible = 0;
                    for (std::size_t px = 0; px < grid.rgba.size(); px += 4)
                        visible += (grid.rgba[px] | grid.rgba[px + 1] | grid.rgba[px + 2]) != 0;
                    std::printf("       auto eligibility, %s: %s (%u killers enabled), %zu visible pixels\n",
                                m->getIdentifier().c_str(), autoFrame.eligible ? "certificates attached" : "grid lane only",
                                autoFrame.killers, visible);
                    if (grid.rgba != autoFrame.rgba)
                        std::printf("       %s: %s\n", m->getIdentifier().c_str(), byteDiff(grid.rgba, autoFrame.rgba).c_str());
                    check(visible > 0 && grid.rgba == autoFrame.rgba, m->getIdentifier() + ": auto eligibility keeps every byte");
                    if (timing && autoFrame.eligible) abba({m}, m->getIdentifier().c_str());
                }
            if (found == 0) std::printf("  SKIP: %s media not present (not a pass)\n", zoneName.c_str());
        }
        // Auto mode also reads the other channels, through the shared-noise
        // prelude a density may fill first (Rendering::volumeDensitySampleCost):
        // an edit re-ranks exactly when it moves that per-sample cost.
        {
            renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                               glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
            auto curtain = geom::FieldNode::fromJson(zone.at("spatialFields").front());
            auto sampleCost = [&] {
                Rendering::VolumeDensityBinding b;
                if (!Rendering::readVolumeDensity(*curtain, 7.25, 1.0 / 60.0, b) || !b.densityExpr) return -1.0;
                const auto prelude = sdfwgsl::inspectVolumeSharedPrelude(b.densityExpr, b.extinctionExpr,
                    b.scatteringExpr, b.volumeChromaExpr, b.phaseExpr, b.emissionExpr);
                return Rendering::volumeDensitySampleCost(*b.densityExpr, prelude.densityUses, prelude.groups,
                                                          prelude.densitySites);
            };
            auto editAndRender = [&](const std::vector<std::pair<const char*, std::string>>& edits, const char* label) {
                const double before = sampleCost();
                bool accepted = true;
                for (const auto& [path, json] : edits) {
                    Property* p = curtain->findProperty(path);
                    accepted = accepted && p && p->setValue(PropertyValue(json));
                }
                const double after = sampleCost();
                const Frame f = render({curtain}, true, true, 7.25, 0);
                std::printf("       auto eligibility, %s: per-sample cost %.0f -> %.0f, %u certificate builds\n",
                            label, before, after, f.firstCertificateBuilds);
                check(accepted && f.firstCertificateBuilds == (after != before ? 1u : 0u),
                      std::string("auto mode, ") + label + ": re-ranks exactly when the per-sample cost moves");
                return after != before;
            };
            render({curtain}, true, true, 7.25, 0);
            const auto& other = media.size() > 1 ? media[1] : media.front();
            editAndRender({{"volume.emission.ast", other->volumeEmission->toJson().dump()}},
                          "another curtain's emission");
            const std::string scalar = R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":0.1,"factors":{}}]}}}]})";
            const std::string c = R"({"op":0,"scalarForm":{"terms":[{"c":0.1,"factors":{}}]}})";
            const std::string vec = R"({"input":"x","pieces":[{"mathNode":{"op":2,"children":[)" + c + "," + c + "," + c + "]}}]}";
            const bool moved = editAndRender({{"volume.extinction.ast", scalar}, {"volume.scattering.ast", scalar},
                                              {"volume.emission.ast", vec}}, "noise-free companion channels");
            check(moved, "removing the companions' noise changes the curtain's prelude (the witness is not vacuous)");
        }
    }
    autoEligibility = false;

    // Native stress size: one spawn frame at 2560x1440, certificates alone.
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    width = 2560; height = 1440; stride = (width * 4 + 255) & ~255u;
    td.size = {width, height, 1}; bd.size = uint64_t(stride) * height;
    target = wgpuDeviceCreateTexture(gpu.device, &td);
    view = wgpuTextureCreateView(target, nullptr);
    readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    lastRgba.resize(size_t(width) * height * 4);
    renderer.setCamera(glm::lookAt(eye, glm::vec3(0, 60, 90), glm::vec3(0, 1, 0)),
                       glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f), eye);
    {
        const Frame stressCertified = render(media, false, true, 0.0, 1);
        const Frame stressExact = render(media, false, false, 0.0, 1);
        std::printf("       2560x1440 spawn (adapter-relative): exact %.1f ms, certificates %.1f ms\n",
                    stressExact.ms, stressCertified.ms);
        check(stressCertified.killers > 0 && stressCertified.rgba == stressExact.rgba,
              "full 2560x1440 spawn preserves every RGBA byte under certificates alone");
    }

    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("webgpu_volume_support_certificates_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
