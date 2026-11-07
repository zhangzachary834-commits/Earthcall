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
    auto render = [&](const std::vector<std::shared_ptr<geom::FieldNode>>& set, bool proofOn,
                      bool certificatesOn, double t, int timedFrames) {
        renderer.setVolumeZeroProofEnabled(proofOn);
        renderer.setVolumeSupportCertificatesEnabled(certificatesOn);
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
        frame.warmCompiles += renderer.frameStats().volumeProgramCompiles;
        frame.lit = sync();
        frame.rgba = lastRgba;
        return frame;
    };


    WGPUAdapterInfo adapterInfo={};
    wgpuAdapterGetInfo(gpu.adapter,&adapterInfo);
    std::printf("ADAPTER backend=%u name=%.*s\n",unsigned(adapterInfo.backendType),int(adapterInfo.device.length),adapterInfo.device.data);
    auto leaf=[](double c){return nlohmann::json{{"op",0},{"scalarForm",{{"terms",nlohmann::json::array({{{"c",c},{"factors",nlohmann::json::object()}}})}}}};};
    auto op=[](int n,std::initializer_list<nlohmann::json> c){return nlohmann::json{{"op",n},{"children",c}};};
    auto piece=[](nlohmann::json node){return OntoMath::Piecewise::fromJson({{"input","x"},{"pieces",nlohmann::json::array({{{"mathNode",node}}})}});};
    const nlohmann::json x={{"op",0},{"scalarForm",{{"terms",nlohmann::json::array({{{"c",1.},{"factors",{{"x",1.}}}}})}}}};
    auto F=op(5,{op(5,{op(4,{leaf(18014398509481984.),x}),leaf(18014398509481984.)}),x});
    auto D=op(6,{leaf(-1),F});
    const std::vector<std::pair<std::string,nlohmann::json>> cases={
        {"positive-control",leaf(.5)},
        {"cancelled-min",op(6,{leaf(-1),op(20,{F,leaf(0)})})},
        {"computed-clamp-bound",op(5,{leaf(.5),op(26,{x,op(5,{leaf(33554432.),op(5,{leaf(33554432.),leaf(1)})}),leaf(1)})})}
    };
    renderer.setCamera(glm::lookAt(glm::vec3(0,0,6),glm::vec3(0),glm::vec3(0,1,0)),
        glm::perspectiveZO(glm::radians(60.f),float(width)/height,.05f,100.f),glm::vec3(0,0,6));

    {
        auto probe=geom::FieldNode::fromJson(media.front()->toJson());*probe->volumeDensity=piece(D);probe->noteAuthoredMathWritten();
        auto value=probe->volumeDensity->evaluate({{"x",1.},{"y",0.},{"z",0.}});
        double number=-999; if(value)propertyValueToNumber(*value,number);
        Rendering::VolumeDensityBinding binding;Rendering::readVolumeDensity(*probe,0,0,binding);
        Rendering::VolumeRayFormCache cache;const auto& forms=cache.formsFor(binding);
        Rendering::VolumeRayInput ray;ray.origin={1,0,6};ray.direction={0,0,-1};
        Rendering::VolumeLatticeSpan span;span.firstSample=4;span.step=.1;span.count=40;
        auto plan=Rendering::planVolumeRaySegment({{&forms,glm::dvec3(0),0}},ray,span);
        std::printf("CPU cancellation D(1)=%.17g strategy=%s reason=%s\n",number,Rendering::volumeSegmentStrategyName(plan.strategy),plan.reason.c_str());
    }
    for(const auto& item:cases){
        auto field=geom::FieldNode::fromJson(media.front()->toJson());field->origin=glm::vec3(0);field->scale=glm::vec3(2);
        *field->volumeDensity=piece(item.second);
        *field->volumeExtinction=piece(leaf(.25));
        *field->volumeEmission=piece(op(2,{leaf(1),leaf(.2),leaf(.1)}));
        field->noteAuthoredMathWritten();
        Rendering::VolumeDensityBinding binding;Rendering::readVolumeDensity(*field,0,0,binding);
        Rendering::VolumeRayFormCache cache;
        const auto& forms=cache.formsFor(binding);
        const auto certs=Rendering::deriveVolumeSupportCertificates(forms,glm::dvec3(2));
        std::printf("ADVERSARIAL %s host_at_x1=%s certificates=%zu\n",item.first.c_str(),field->volumeDensity->evaluate({{"x",1.},{"y",0.},{"z",0.}}).has_value()?"evaluated":"refused",certs.killers.size());
        for(const auto& c:certs.killers)std::printf("HEADER role=%u sign=%d L=%.17g magnitude=%.17g offset=%.17g\n",c.role,int(c.sign),c.spatialLipschitz,c.argumentMagnitude,c.offset);
        for(bool grid:{false,true}){
            auto exact=render({field},grid,false,0,0),cert=render({field},grid,true,0,0);
            std::printf("ADVERSARIAL %s single grid=%d killers=%u lit=%llu %s\n",item.first.c_str(),grid,cert.killers,(unsigned long long)exact.lit,byteDiff(exact.rgba,cert.rgba).c_str());
            check(exact.rgba==cert.rgba,item.first+" single parity");
        }
        auto nj=field->toJson();nj["id"]="native-adversarial-null";auto null=geom::FieldNode::fromJson(nj);
        *null->volumeDensity=piece(leaf(0));null->noteAuthoredMathWritten();
        for(bool grid:{false,true}){
            auto exact=render({field,null},grid,false,0,0),cert=render({field,null},grid,true,0,0);
            std::printf("ADVERSARIAL %s fused grid=%d killers=%u lit=%llu %s\n",item.first.c_str(),grid,cert.killers,(unsigned long long)exact.lit,byteDiff(exact.rgba,cert.rgba).c_str());
            check(exact.rgba==cert.rgba,item.first+" fused parity");
        }
    }
    std::printf("Adversarial native checks: %d/%d passed\n",g_checks-g_failures,g_checks);
    return g_failures?1:0;
}
