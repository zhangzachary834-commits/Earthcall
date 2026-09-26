// Diagnostic only: read either saved Zone without writing it. This isolates the
// authored volume path at several drawable sizes; it is NOT whole-app FPS or
// full-scene image acceptance (the saved objects are not drawn here).
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "json.hpp"
#include <chrono>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
        std::fprintf(stderr, "usage: probe <zone.json> <authored> [width] [height] [frames] [blocks] [eyeX] [eyeY] [eyeZ] [lookX] [lookY] [lookZ] [motion:0|1] [sourceTime]\n");
        return 2;
    }
    if (std::string(argv[2]) != "authored") return 2;
    const uint32_t width = argc > 3 ? static_cast<uint32_t>(std::atoi(argv[3])) : 160;
    const uint32_t height = argc > 4 ? static_cast<uint32_t>(std::atoi(argv[4])) : 90;
    const int frames = argc > 5 ? std::atoi(argv[5]) : 30;
    const int blocks = argc > 6 ? std::atoi(argv[6]) : 3;
    const bool northern = std::string(argv[1]).find("Northern Veil") != std::string::npos;
    const glm::vec3 eye(argc > 7 ? std::atof(argv[7]) : 0.0f,
                        argc > 8 ? std::atof(argv[8]) : (northern ? 64.0f : 2.2f),
                        argc > 9 ? std::atof(argv[9]) : (northern ? 230.0f : 8.0f));
    const glm::vec3 look(argc > 10 ? std::atof(argv[10]) : 0.0f,
                         argc > 11 ? std::atof(argv[11]) : (northern ? 64.0f : 2.25f),
                         argc > 12 ? std::atof(argv[12]) : (northern ? 80.0f : -12.0f));
    const bool motion = argc > 13 && std::atoi(argv[13]) != 0;
    const double sourceTime = argc > 14 ? std::atof(argv[14]) : 0.0;
    assert(width >= 16 && height >= 16 && frames > 0 && blocks > 0);

    auto saved = readJson(argv[1]);
    assert(saved.at("identifier") == (northern ? "Northern Veil" : "Sanctuary of Sunlit Mist"));
    auto sun = geom::FieldNode::fromJson(saved.at("spatialRoot"));
    if (northern && sourceTime != 0.0) {
        // Exercise the admitted source Timeline itself; the saved root's rho
        // happens to be a constant, which cannot witness a temporal binding.
        auto timeLeaf = std::make_shared<OntoMath::MathNode>();
        timeLeaf->op = OntoMath::MathNode::Op::ValueLeaf;
        timeLeaf->variableName = "t";
        sun->field->astDefinition.pieces[0].mathNode = timeLeaf;
    }
    std::vector<std::shared_ptr<geom::FieldNode>> fields;
    std::vector<Rendering::VolumeDensityBinding> media;
    for (const auto& json : saved.at("spatialFields")) {
        fields.push_back(geom::FieldNode::fromJson(json));
        Rendering::VolumeDensityBinding medium;
        assert(fields.back() && Rendering::readVolumeDensity(*fields.back(), sourceTime, 0, medium));
        media.push_back(medium);
    }
    assert(sun && media.size() == (northern ? 4u : 1u));
    Rendering::AuthorableLightState light;
    assert(Rendering::readAuthorableLight(*sun, light) && light.enabled);
    if (!northern) assert(media.front().occluderSdf);

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
    source.temporalCoordinate = sourceTime;
    source.enabled = light.enabled;
    if (sun->field &&
        sun->field->mode == OntoMath::ScalarField::EvaluationMode::AST &&
        !sun->field->astDefinition.pieces.empty()) {
        source.radianceExpr = &sun->field->astDefinition;
        source.radianceRevision =
            std::hash<std::string>{}(sun->field->astDefinition.toJson().dump());
    }
    if (sun->lightChroma && !sun->lightChroma->pieces.empty()) {
        source.chromaExpr = sun->lightChroma.get();
        source.chromaRevision = std::hash<std::string>{}(sun->lightChroma->toJson().dump());
    }
    if (sun->lightAngular && !sun->lightAngular->pieces.empty()) {
        source.angularExpr = sun->lightAngular.get();
        source.angularRevision = std::hash<std::string>{}(sun->lightAngular->toJson().dump());
    }
    const bool reuseSourceGeometry = std::getenv("EARTHCALL_EXPERIMENT_VOLUME_REUSE_SOURCE_GEOMETRY") &&
        std::string(std::getenv("EARTHCALL_EXPERIMENT_VOLUME_REUSE_SOURCE_GEOMETRY")) == "1";
    if (northern) {
        std::vector<sdfwgsl::VolumeProgramInput> inputs;
        for (const auto& medium : media) {
            inputs.push_back({medium.densityExpr, medium.extinctionExpr,
                              medium.scatteringExpr, medium.volumeChromaExpr,
                              medium.phaseExpr, medium.emissionExpr,
                              medium.occluderSdf, source.radianceExpr,
                              source.chromaExpr, source.angularExpr});
        }
        const auto generated = sdfwgsl::compileVolumeSet(inputs);
        const auto fs = generated.wgsl.find("\n@fragment");
        size_t sourceEvalCalls = 0;
        size_t otherMemberCalls = 0;
        if (fs != std::string::npos) {
            for (size_t at = fs; (at = generated.wgsl.find("lightRadianceEval_", at)) != std::string::npos; ++at)
                ++sourceEvalCalls;
            for (size_t i=1; i<media.size(); ++i)
                if (generated.wgsl.find("lightRadianceEval_" + std::to_string(i) + "(", fs) != std::string::npos)
                    ++otherMemberCalls;
        }
        std::printf("V5_WGSL ok=%d error=%s bytes=%zu fragment=%d radial_eval_call_sites=%zu other_member_calls=%zu source_time_binding=%d\n",
                    generated.ok, generated.error.c_str(), generated.wgsl.size(),
                    fs != std::string::npos, sourceEvalCalls, otherMemberCalls,
                    generated.wgsl.find("u.sourceTime.x") != std::string::npos);
        if (!generated.ok || fs == std::string::npos ||
            sourceEvalCalls != media.size() ||
            otherMemberCalls != media.size()-1u ||
            (sourceTime != 0.0 && generated.wgsl.find("u.sourceTime.x") == std::string::npos)) return 3;
    } else if (reuseSourceGeometry) {
        const auto& m = media.front();
        auto generated = sdfwgsl::compileVolume(
            m.densityExpr, m.extinctionExpr, m.scatteringExpr,
            m.volumeChromaExpr, m.phaseExpr, m.emissionExpr, m.occluderSdf,
            source.radianceExpr, source.chromaExpr, source.angularExpr);
        const auto original = generated.wgsl;
        std::string error;
        const bool applied = sdfwgsl::reuseVolumeSourceGeometry(generated, error);
        std::printf("SOURCE_GEOMETRY_REUSE applied=%d changed=%d error=%s\n",
                    applied, original != generated.wgsl, error.c_str());
        if (!applied || original == generated.wgsl ||
            generated.wgsl.find("sourceDist, lightDir);") == std::string::npos)
            return 3;
    }
    renderer.setRadianceSources({source}, 1);

    const glm::mat4 view3d = glm::lookAt(eye, look, glm::vec3(0,1,0));
    const glm::mat4 proj = glm::perspectiveZO(
        glm::radians(45.0f), float(width) / height, 0.1f, 100.0f);
    renderer.setCamera(view3d, proj, eye);

    const bool diagnosticWork = std::getenv("EARTHCALL_DIAGNOSTIC_VOLUME_WORK") &&
        std::string(std::getenv("EARTHCALL_DIAGNOSTIC_VOLUME_WORK")) == "1";
    int captureIndex = 0;
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
        uint64_t lum = 0, alphaSum = 0, viewSum = 0, stepSum = 0, saturated = 0;
        uint64_t hash = 1469598103934665603ull;
        for (uint32_t y=0; y<height; ++y) {
            for (uint32_t x=0; x<width; ++x) {
                const size_t at = static_cast<size_t>(y)*stride + x*4;
                lum += pixels[at] + pixels[at+1] + pixels[at+2];
                alphaSum += pixels[at+3];
                if (diagnosticWork && pixels[at+3] == 255) {
                    const unsigned view = pixels[at] | ((pixels[at+1] & 15u) << 8u);
                    const unsigned step = (pixels[at+1] >> 4u) | (pixels[at+2] << 4u);
                    viewSum += view;
                    stepSum += step;
                    saturated += (view == 4095 || step == 4095);
                }
                for (int c=0;c<4;++c) {
                    hash ^= pixels[at+c];
                    hash *= 1099511628211ull;
                }
            }
        }
        // Paired raw captures retain the exact medium radiance and opacity
        // independently. They are diagnostic pixels over transparent black,
        // not a saved scene composite or a final quality acceptance image.
        if (const char* dir = std::getenv("EARTHCALL_VOLUME_CAPTURE_DIR")) {
            std::filesystem::create_directories(dir);
            const std::string stem = std::string(dir) + "/" +
                (northern ? "veil" : "mist") + "-" + std::to_string(width) + "x" +
                std::to_string(height) + (motion ? "-moving-" : "-fixed-") +
                std::to_string(static_cast<int>(eye.x)) + "-" +
                std::to_string(captureIndex++);
            std::ofstream rgb(stem + ".ppm", std::ios::binary);
            std::ofstream alpha(stem + "-alpha.pgm", std::ios::binary);
            rgb << "P6\n" << width << " " << height << "\n255\n";
            alpha << "P5\n" << width << " " << height << "\n255\n";
            for (uint32_t y=0; y<height; ++y) for (uint32_t x=0; x<width; ++x) {
                const char* pixel = reinterpret_cast<const char*>(pixels + size_t(y)*stride + x*4);
                rgb.write(pixel, 3);
                alpha.write(pixel+3, 1);
            }
            assert(rgb.good() && alpha.good());
        }
        wgpuBufferUnmap(readback);
        return std::array<uint64_t,6>{lum,alphaSum,hash,viewSum,stepSum,saturated};
    };

    int frameIndex = 0;
    auto frame = [&](double& projectionMs, double& submitMs) {
        const auto t0=Clock::now();
        if (motion) {
            const float shift = static_cast<float>(frameIndex++ % 32) / 31.0f *
                                (northern ? 16.0f : 2.0f);
            const glm::vec3 delta(shift, 0, 0);
            renderer.setCamera(glm::lookAt(eye + delta, look, glm::vec3(0,1,0)),
                               proj, eye + delta);
        }
        std::string setIdentity;
        for (size_t i=0; i<fields.size(); ++i) {
            assert(Rendering::readVolumeDensity(*fields[i], sourceTime, 0, media[i]));
            Rendering::appendVolumeSetIdentity(setIdentity, fields[i]->getIdentifier(), media[i]);
        }
        renderer.setVolumeDensitySources(media, std::hash<std::string>{}(setIdentity));
        const auto t1=Clock::now();
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(view,width,height,glm::vec4(0,0,0,0));
        renderer.composeVolumes();
        renderer.endFrame();
        const auto t2=Clock::now();
        projectionMs += elapsedMs(t0,t1);
        submitMs += elapsedMs(t1,t2);
    };
    for (int i=0;i<12;++i) { double p=0,s=0;frame(p,s); }
    syncImage();
    std::printf("SCENE=%s media=%zu size=%ux%u eye=(%.2f,%.2f,%.2f) look=(%.2f,%.2f,%.2f) frames=%d blocks=%d motion=%d sourceTime=%.3f volume-only\n",
                saved.at("identifier").get<std::string>().c_str(), media.size(),
                width,height,eye.x,eye.y,eye.z,look.x,look.y,look.z,frames,blocks,motion,sourceTime);
    for (int block=0;block<blocks;++block) {
        double projectionMs=0,submitMs=0;
        const auto wallStart=Clock::now();
        const auto cpuStart=std::clock();
        for(int i=0;i<frames;++i) frame(projectionMs,submitMs);
        const auto cpuEnd=std::clock();
        const auto [lum,alpha,hash,viewSum,stepSum,saturated]=syncImage();
        const auto wallEnd=Clock::now();
        std::printf("BLOCK %d projection_ms=%.4f submit_ms=%.4f process_cpu_ms=%.4f "
                    "wall_sync_ms=%.4f ring_allocs=%u ring_bytes=%zu "
                    "wgsl_compiles=%u gpu_timestamp_supported=%d gpu_timestamp_valid=%d gpu_main_ms=%.4f "
                    "lum_sum=%llu alpha_sum=%llu rgba_hash=%016llx "
                    "work_view_sum=%llu work_step_sum=%llu work_saturated_pixels=%llu\n",
                    block,projectionMs/frames,submitMs/frames,
                    1000.0*double(cpuEnd-cpuStart)/CLOCKS_PER_SEC/frames,
                    elapsedMs(wallStart,wallEnd)/frames,
                    renderer.frameStats().bufferSuballocations,
                    renderer.frameStats().uniformBytesWritten,
                    renderer.frameStats().volumeProgramCompiles,
                    renderer.frameStats().gpuMainPassTimingSupported,
                    renderer.frameStats().gpuMainPassTimingValid,
                    renderer.frameStats().gpuMainPassMs,
                    static_cast<unsigned long long>(lum),
                    static_cast<unsigned long long>(alpha),
                    static_cast<unsigned long long>(hash),
                    static_cast<unsigned long long>(viewSum),
                    static_cast<unsigned long long>(stepSum),
                    static_cast<unsigned long long>(saturated));
        if (diagnosticWork && (viewSum == 0 || saturated != 0)) return 4;
    }
    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    return 0;
}
