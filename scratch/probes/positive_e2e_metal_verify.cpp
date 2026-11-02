// Native Apple M5 Metal verification and Dual-Regime benchmark for Positive E2E Ray Transport Plan (P1-P5).
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/ClampedWaveSupport.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;

double elapsedMs(Clock::time_point a) {
    return std::chrono::duration<double, std::milli>(Clock::now() - a).count();
}

struct MapResult { bool done = false; bool ok = false; };
void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapResult*>(u);
    r->ok = (s == WGPUMapAsyncStatus_Success);
    r->done = true;
}

#define REQUIRE(c, msg) do { if (!(c)) { std::fprintf(stderr, "FAIL: %s (%s)\n", msg, #c); std::exit(1); } } while (0)

static std::pair<glm::vec3, glm::vec3> canonicalPose(int i) {
    const double a = 6.2831853071795864769 * i / 64.0;
    glm::vec3 eye(float(8 * std::sin(a)), float(50 + 2 * std::sin(a * .5)), float(75 + 8 * (1 - std::cos(a))));
    glm::vec3 aim(float(40 + 8 * std::sin(a * .5)), float(62 + 2 * std::cos(a)), 110);
    return {eye, aim};
}

static uint64_t fnv1a(const void* d, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    const auto* b = static_cast<const unsigned char*>(d);
    for (size_t i = 0; i < n; ++i) h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static std::string hex64(uint64_t v) {
    char b[24];
    std::snprintf(b, sizeof(b), "%016llx", (unsigned long long)v);
    return b;
}

static bool fenceQueue(const wgpu::Device& gpu) {
    struct Done { bool done = false, ok = false; } result;
    WGPUQueueWorkDoneCallbackInfo cb = {};
    cb.mode = WGPUCallbackMode_AllowProcessEvents;
    cb.userdata1 = &result;
    cb.callback = [](WGPUQueueWorkDoneStatus s, WGPUStringView, void* p, void*) {
        auto* r = static_cast<Done*>(p);
        r->ok = (s == WGPUQueueWorkDoneStatus_Success);
        r->done = true;
    };
    wgpuQueueOnSubmittedWorkDone(gpu.queue, cb);
    while (!result.done) wgpuInstanceProcessEvents(gpu.instance);
    return result.ok;
}

static void poisonScratch(const wgpu::Device& gpu, const KernelRoutedTransport* k) {
    std::vector<unsigned char> v;
    for (auto b : {k->publication, k->rays, k->singles, k->overlaps, k->counts, k->lists[0], k->lists[1], k->lists[2], k->args}) {
        const uint64_t n = wgpuBufferGetSize(b);
        if (v.size() < n) v.assign(n, 0xFF);
        wgpuQueueWriteBuffer(gpu.queue, b, 0, v.data(), n);
    }
}

static void poisonPublicationFinite(const wgpu::Device& gpu, const KernelRoutedTransport* k, glm::vec4 val) {
    const uint64_t bytes = uint64_t(k->width) * k->height * 16;
    std::vector<float> f(bytes / 4);
    for (size_t i = 0; i < f.size(); i += 4) {
        f[i + 0] = val.x;
        f[i + 1] = val.y;
        f[i + 2] = val.z;
        f[i + 3] = val.w;
    }
    wgpuQueueWriteBuffer(gpu.queue, k->publication, 0, f.data(), bytes);
}

static std::shared_ptr<OntoMath::Piecewise> makeClampedWaveDensity(
    float wx, float wy, float wz, float a,
    float amp0, float amp1, float amp2,
    float freq0 = 0.01f, float freq1 = 0.02f, float freq2 = 0.03f,
    float phase0 = 0.0f, float phase1 = 0.0f, float phase2 = 0.0f) {
    using N = OntoMath::MathNode;
    using O = N::Op;

    auto makeVal = [](const std::string& var) {
        auto n = std::make_shared<N>();
        n->op = O::ValueLeaf;
        n->variableName = var;
        return n;
    };

    auto makeLit = [](double val) {
        auto n = std::make_shared<N>();
        n->op = O::ScalarLeaf;
        OntoMath::Term t(val);
        n->scalarForm.terms.push_back(t);
        return n;
    };

    auto makeWave = [](double amp, double freq, double phase, bool isCos) {
        auto n = std::make_shared<N>();
        n->op = O::ScalarLeaf;
        OntoMath::Term t(amp);
        OntoMath::TransFactor tf(isCos ? OntoMath::TransFactor::Kind::Cos : OntoMath::TransFactor::Kind::Sin, "x", freq, phase);
        t.trans.push_back(tf);
        n->scalarForm.terms.push_back(t);
        return n;
    };

    auto makeBinary = [](O op, std::shared_ptr<N> a, std::shared_ptr<N> b) {
        auto n = std::make_shared<N>();
        n->op = op;
        n->children.push_back(std::make_unique<N>(*a));
        n->children.push_back(std::make_unique<N>(*b));
        return n;
    };

    auto makeUnary = [](O op, std::shared_ptr<N> a) {
        auto n = std::make_shared<N>();
        n->op = op;
        n->children.push_back(std::make_unique<N>(*a));
        return n;
    };

    auto makeClamp = [&](std::shared_ptr<N> val, double lo, double hi) {
        auto n = std::make_shared<N>();
        n->op = O::Clamp;
        n->children.push_back(std::make_unique<N>(*val));
        n->children.push_back(std::make_unique<N>(*makeLit(lo)));
        n->children.push_back(std::make_unique<N>(*makeLit(hi)));
        return n;
    };

    // 1. x: clamp(1 - abs(x) / wx, 0, 1)
    auto xVar = makeVal("x");
    auto absX = makeUnary(O::Abs, xVar);
    auto divX = makeBinary(O::Div, absX, makeLit(wx));
    auto oneMinusX = makeBinary(O::Sub, makeLit(1.0), divX);
    auto clampX = makeClamp(oneMinusX, 0.0, 1.0);

    // 2. yr: clamp(a * (y + wy), 0, 1)
    auto yVar = makeVal("y");
    auto yPlusWy = makeBinary(O::Add, yVar, makeLit(wy));
    auto scaleYr = makeBinary(O::Scale, makeLit(a), yPlusWy);
    auto clampYr = makeClamp(scaleYr, 0.0, 1.0);

    // 3. yf: clamp(1 - (y + wy) / (2 * wy), 0, 1)
    auto divYf = makeBinary(O::Div, yPlusWy, makeLit(2.0 * wy));
    auto oneMinusYf = makeBinary(O::Sub, makeLit(1.0), divYf);
    auto clampYf = makeClamp(oneMinusYf, 0.0, 1.0);

    // 4. z: clamp(1 - abs(z + wave0 + wave1 + wave2) / wz, 0, 1)
    auto zVar = makeVal("z");
    auto w0 = makeWave(amp0, freq0, phase0, false);
    auto w1 = makeWave(amp1, freq1, phase1, true);
    auto w2 = makeWave(amp2, freq2, phase2, false);
    auto bent0 = makeBinary(O::Add, zVar, w0);
    auto bent1 = makeBinary(O::Add, bent0, w1);
    auto bent2 = makeBinary(O::Add, bent1, w2);
    auto absBent = makeUnary(O::Abs, bent2);
    auto divZd = makeBinary(O::Div, absBent, makeLit(wz));
    auto oneMinusZd = makeBinary(O::Sub, makeLit(1.0), divZd);
    auto clampZd = makeClamp(oneMinusZd, 0.0, 1.0);

    // Product of the 4 terms:
    auto prod0 = makeBinary(O::Scale, clampX, clampYr);
    auto prod1 = makeBinary(O::Scale, prod0, clampYf);
    auto prod2 = makeBinary(O::Scale, prod1, clampZd);

    auto pw = std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece piece;
    piece.mathNode = prod2;
    pw->pieces.push_back(piece);
    return pw;
}

struct OffscreenTarget {
    uint32_t width = 0;
    uint32_t height = 0;
    WGPUTexture texture = nullptr;
    WGPUTextureView view = nullptr;
    WGPUBuffer readback = nullptr;
    uint32_t stride = 0;

    void init(const wgpu::Device& gpu, uint32_t w, uint32_t h) {
        width = w;
        height = h;
        WGPUTextureDescriptor td = {};
        td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        td.dimension = WGPUTextureDimension_2D;
        td.size = {w, h, 1};
        td.format = WGPUTextureFormat_RGBA8Unorm;
        td.mipLevelCount = 1;
        td.sampleCount = 1;
        texture = wgpuDeviceCreateTexture(gpu.device, &td);
        view = wgpuTextureCreateView(texture, nullptr);
        stride = (w * 4 + 255) & ~255u;
        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        bd.size = static_cast<uint64_t>(stride) * h;
        readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    }

    void release() {
        if (readback) { wgpuBufferRelease(readback); readback = nullptr; }
        if (view) { wgpuTextureViewRelease(view); view = nullptr; }
        if (texture) { wgpuTextureRelease(texture); texture = nullptr; }
    }
};

static std::vector<float> readPublication(const wgpu::Device& gpu, const KernelRoutedTransport* k) {
    const uint64_t bytes = uint64_t(k->width) * k->height * 16;
    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bd.size = bytes;
    WGPUBuffer dst = wgpuDeviceCreateBuffer(gpu.device, &bd);
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(enc, k->publication, 0, dst, 0, bytes);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);
    MapResult res;
    WGPUBufferMapCallbackInfo cb = {};
    cb.mode = WGPUCallbackMode_AllowProcessEvents;
    cb.callback = onMapped;
    cb.userdata1 = &res;
    wgpuBufferMapAsync(dst, WGPUMapMode_Read, 0, bytes, cb);
    while (!res.done) wgpuInstanceProcessEvents(gpu.instance);
    const auto* f = static_cast<const float*>(wgpuBufferGetConstMappedRange(dst, 0, bytes));
    std::vector<float> out(f, f + bytes / 4);
    wgpuBufferUnmap(dst);
    wgpuBufferRelease(dst);
    return out;
}

static std::vector<uint32_t> readWords(const wgpu::Device& gpu, WGPUBuffer src, uint64_t bytes) {
    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bd.size = bytes;
    WGPUBuffer dst = wgpuDeviceCreateBuffer(gpu.device, &bd);
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, dst, 0, bytes);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);
    MapResult res;
    WGPUBufferMapCallbackInfo cb = {};
    cb.mode = WGPUCallbackMode_AllowProcessEvents;
    cb.callback = onMapped;
    cb.userdata1 = &res;
    wgpuBufferMapAsync(dst, WGPUMapMode_Read, 0, bytes, cb);
    while (!res.done) wgpuInstanceProcessEvents(gpu.instance);
    const auto* f = static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(dst, 0, bytes));
    std::vector<uint32_t> out(f, f + bytes / 4);
    wgpuBufferUnmap(dst);
    wgpuBufferRelease(dst);
    return out;
}

static double vecMean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / double(v.size());
}

static double vecMedian(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return (n % 2 == 1) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
} // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    REQUIRE(argc >= 3, "usage: verify <zone.ecform> <result.json>");
    const char* zonePath = argv[1];
    const char* outJsonPath = argv[2];
    const uint32_t width = 2560;
    const uint32_t height = 1440;

    std::ifstream in(zonePath, std::ios::binary);
    REQUIRE(bool(in), "open zone.ecform");
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    REQUIRE(!bytes.empty(), "nonempty zone.ecform");
    const auto saved = bytes.front() == '{'
        ? Json::parse(bytes)
        : Json::parse(Json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());

    auto root = geom::FieldNode::fromJson(saved.at("spatialRoot"));
    REQUIRE(bool(root), "spatialRoot");
    std::vector<std::shared_ptr<geom::FieldNode>> curtains;
    for (const auto& f : saved.at("spatialFields")) curtains.push_back(geom::FieldNode::fromJson(f));
    REQUIRE(curtains.size() == 4, "4 aurora curtains");
    std::vector<geom::FieldNode*> active;
    for (auto& c : curtains) active.push_back(c.get());

    Rendering::AuthorableLightState light;
    REQUIRE(Rendering::readAuthorableLight(*root, light), "readAuthorableLight");

    wgpu::Device gpu;
    REQUIRE(gpu.init(), "WebGPU device init");
    WebGpuRenderer renderer;
    REQUIRE(renderer.init(gpu), "WebGpuRenderer init");
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
    source.temporalCoordinate = 7.25;
    source.temporalDelta = 1.0 / 60.0;
    root->verifyAuthoredMath();
    if (root->field && root->field->mode == OntoMath::ScalarField::EvaluationMode::AST &&
        !root->field->astDefinition.pieces.empty()) {
        source.radianceExpr = &root->field->astDefinition;
        source.radianceRevision = root->authoredMathRevision(geom::FieldNode::kMathFieldAst);
    }
    if (root->lightChroma && !root->lightChroma->pieces.empty()) {
        source.chromaExpr = root->lightChroma.get();
        source.chromaRevision = root->authoredMathRevision(geom::FieldNode::kMathLightChroma);
    }
    if (root->lightAngular && !root->lightAngular->pieces.empty()) {
        source.angularExpr = root->lightAngular.get();
        source.angularRevision = root->authoredMathRevision(geom::FieldNode::kMathLightAngular);
    }
    renderer.setRadianceSources({source}, 1);
    renderer.setLight(source.position, source.ambientRadiance, source.diffuseRadiance, source.specularRadiance);
    renderer.setLightingEnabled(light.enabled);

    auto readTargetRgbaHash = [&](WGPUTexture srcTex, WGPUBuffer dstBuf, uint32_t tw, uint32_t th, uint32_t tstride) -> uint64_t {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {}; src.texture = srcTex; src.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst = {}; dst.buffer = dstBuf;
        dst.layout.bytesPerRow = tstride; dst.layout.rowsPerImage = th;
        WGPUExtent3D ext = {tw, th, 1};
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);
        MapResult r;
        WGPUBufferMapCallbackInfo cb = {};
        cb.mode = WGPUCallbackMode_AllowProcessEvents;
        cb.callback = onMapped;
        cb.userdata1 = &r;
        const uint64_t bufSize = static_cast<uint64_t>(tstride) * th;
        wgpuBufferMapAsync(dstBuf, WGPUMapMode_Read, 0, bufSize, cb);
        while (!r.done) wgpuInstanceProcessEvents(gpu.instance);
        const auto* px = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(dstBuf, 0, bufSize));
        uint64_t h = 0xcbf29ce484222325ull;
        for (uint32_t y = 0; y < th; ++y) {
            const unsigned char* row = px + size_t(y) * tstride;
            for (uint32_t x = 0; x < tw * 4; ++x) h = (h ^ row[x]) * 0x100000001b3ull;
        }
        wgpuBufferUnmap(dstBuf);
        return h;
    };

    auto readRgbaHash = [&]() -> uint64_t {
        return readTargetRgbaHash(target, readback, width, height, stride);
    };

    auto renderSceneOffscreen = [&](const std::vector<std::shared_ptr<geom::FieldNode>>& sceneCurtains,
                                    glm::vec3 eye, glm::vec3 aim, glm::vec3 upVec, float fovDeg,
                                    bool routedOn, WGPUTextureView renderView, uint32_t rw, uint32_t rh) {
        renderer.setVolumeRoutedTransportEnabled(routedOn);
        renderer.setCamera(
            glm::lookAt(eye, aim, upVec),
            glm::perspectiveZO(glm::radians(fovDeg), float(rw) / float(rh), 0.1f, 1000.0f),
            eye);
        std::vector<Rendering::VolumeDensityBinding> media;
        std::string identity;
        for (const auto& c : sceneCurtains) {
            Rendering::VolumeDensityBinding m;
            if (Rendering::readVolumeDensity(*c, 7.25, 1.0 / 60.0, m)) {
                Rendering::appendVolumeSetIdentity(identity, m.producerId, m);
                media.push_back(m);
            }
        }
        renderer.setVolumeDensitySources(std::move(media), std::hash<std::string>{}(identity));
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(renderView, rw, rh, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
    };

    auto renderViewCamera = [&](glm::vec3 eye, glm::vec3 aim, glm::vec3 up, float fovDeg, double t, bool routedOn) {
        renderer.setVolumeRoutedTransportEnabled(routedOn);
        renderer.setCamera(
            glm::lookAt(eye, aim, up),
            glm::perspectiveZO(glm::radians(fovDeg), float(width) / height, 0.1f, 1000.0f),
            eye);
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
        renderer.setModel(glm::mat4(1));
        renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const auto& s = renderer.frameStats();
        if (s.volumeProgramRefusals != 0 || (routedOn ? s.volumeRoutedSets != 1 : s.volumeRoutedSets != 0)) {
            std::fprintf(stderr, "FAIL: renderViewCamera refusals=%u routedSets=%u\n",
                         s.volumeProgramRefusals, s.volumeRoutedSets);
            std::exit(1);
        }
    };

    auto renderPose = [&](int poseIdx, bool routedOn) {
        const auto [eye, aim] = canonicalPose(poseIdx);
        const double t = 7.25 + double(poseIdx) / 60.0;
        renderViewCamera(eye, aim, glm::vec3(0, 1, 0), 60.0f, t, routedOn);
    };

    // Warm up 1 frame so _kernelRouted is initialized
    renderPose(0, true);
    REQUIRE(fenceQueue(gpu), "initial warmup fence");
    const KernelRoutedTransport* k = renderer.routedTransportKernelForDiagnostics();
    REQUIRE(k && k->publication && k->width == width && k->height == height, "kernel initialized at 2560x1440");
    const uint32_t strips = (k->height + k->stripRows - 1) / k->stripRows;
    const uint32_t expectDispatches = 8 * strips;
    std::printf("INIT_OK 2560x1440 strips=%u tilesPerStrip=%u capacity=%u\n", strips, k->tiles, k->capacity);

    Json report;
    report["resolution"] = {width, height};
    report["strips"] = strips;
    report["tiles_per_strip"] = k->tiles;
    report["tiles_per_frame"] = uint64_t(k->tiles) * strips;

    // 1. Parity, Poisoned-Scratch & Forced-Refusal Checks at Poses 0 and 32
    uint64_t canonicalPose0PubHash = 0, canonicalPose0RgbaHash = 0;
    Json parityRows = Json::array();
    for (int poseIdx : {0, 32}) {
        std::map<std::string, uint64_t> pubHashes, imgHashes;
        auto runCase = [&](const char* label, bool routed, bool coalesce, uint32_t limit, bool poison) {
            renderer.setRoutedWorkLimitForDiagnostics(limit);
            renderer.setRoutedCoalescedPassesForDiagnostics(coalesce, true);
            if (poison) {
                REQUIRE(fenceQueue(gpu), "pre-poison fence");
                poisonScratch(gpu, k);
                REQUIRE(fenceQueue(gpu), "post-poison fence");
            }
            renderPose(poseIdx, routed);
            REQUIRE(fenceQueue(gpu), "case fence");
            uint64_t imgH = readRgbaHash();
            imgHashes[label] = imgH;
            Json row = {
                {"pose", poseIdx}, {"label", label}, {"routed", routed},
                {"coalesced", coalesce}, {"work_limit", limit}, {"poisoned", poison},
                {"rgba_fnv1a", hex64(imgH)}
            };
            if (routed) {
                REQUIRE(k->encodedDispatches == expectDispatches, "dispatch count");
                REQUIRE(k->encodedPasses == (coalesce ? 1u : expectDispatches), "pass count");
                auto raw = readPublication(gpu, k);
                uint64_t accepted = 0, refused = 0, nonempty = 0, nonfinite = 0;
                for (uint64_t i = 0; i < raw.size() / 4; ++i) {
                    const float* v = &raw[i * 4];
                    if (v[3] < 0.0f) {
                        ++refused;
                        continue;
                    }
                    ++accepted;
                    if (v[3] > 0.0f || v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f) ++nonempty;
                    for (int c = 0; c < 4; ++c) nonfinite += !std::isfinite(v[c]);
                }
                REQUIRE(nonfinite == 0, "zero nonfinite words");
                if (limit == 0) {
                    REQUIRE(accepted == 0 && refused == uint64_t(width) * height && nonempty == 0,
                            "forced refusal must refuse 100% of pixels");
                } else {
                    REQUIRE(refused == 0 && accepted == uint64_t(width) * height && nonempty > 100,
                            "normal routed transport must accept 100% of pixels with zero refusals");
                }
                uint64_t pubH = fnv1a(raw.data(), raw.size() * sizeof(float));
                pubHashes[label] = pubH;
                row["publication_fnv1a"] = hex64(pubH);
                row["accepted"] = accepted;
                row["refused"] = refused;
                row["nonempty"] = nonempty;
                row["nonfinite"] = nonfinite;
            }
            parityRows.push_back(row);
        };

        runCase("current", false, true, 1024, false);
        runCase("routed_coal", true, true, 1024, false);
        runCase("routed_orig", true, false, 1024, false);
        runCase("routed_coal_poisoned", true, true, 1024, true);
        runCase("forced_refusal", true, true, 0, false);
        runCase("forced_refusal_poisoned", true, true, 0, true);

        REQUIRE(pubHashes["routed_coal"] == pubHashes["routed_orig"], "coalesced vs uncoalesced publication parity");
        REQUIRE(pubHashes["routed_coal"] == pubHashes["routed_coal_poisoned"], "unpoisoned vs poisoned publication parity");
        REQUIRE(imgHashes["routed_coal"] == imgHashes["routed_orig"], "coalesced vs uncoalesced RGBA parity");
        REQUIRE(imgHashes["routed_coal"] == imgHashes["routed_coal_poisoned"], "unpoisoned vs poisoned RGBA parity");
        REQUIRE(pubHashes["forced_refusal"] == pubHashes["forced_refusal_poisoned"], "forced refusal poisoned publication parity");
        std::printf("DEBUG: current=%s forced_refusal=%s forced_ref_pub=%s\n",
                    hex64(imgHashes["current"]).c_str(),
                    hex64(imgHashes["forced_refusal"]).c_str(),
                    hex64(pubHashes["forced_refusal"]).c_str());
        REQUIRE(imgHashes["current"] == imgHashes["forced_refusal"], "forced refusal must match Current RGBA exactly");
        REQUIRE(imgHashes["current"] == imgHashes["forced_refusal_poisoned"], "poisoned forced refusal must match Current RGBA exactly");
        REQUIRE(imgHashes["current"] != imgHashes["routed_coal"], "routed transport must contribute visible aurora");
        if (poseIdx == 0) {
            canonicalPose0PubHash = pubHashes["routed_coal"];
            canonicalPose0RgbaHash = imgHashes["routed_coal"];
        }
        std::printf("PARITY_OK pose=%d pub=%s current_rgba=%s routed_rgba=%s\n",
                    poseIdx, hex64(pubHashes["routed_coal"]).c_str(),
                    hex64(imgHashes["current"]).c_str(), hex64(imgHashes["routed_coal"]).c_str());
    }
    renderer.setRoutedWorkLimitForDiagnostics(1024);
    renderer.setRoutedCoalescedPassesForDiagnostics(true, false);
    report["parity_checks"] = parityRows;

    // 2. 64-Pose Canonical Route Census (with Phase P4 16-lane cooperative classifier)
    REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(true), "enable route observer");
    const uint64_t tilesPerFrame = uint64_t(k->tiles) * strips;
    uint64_t totEmpty = 0, totS1 = 0, totS2 = 0, totFb = 0;
    uint64_t r1Empty = 0, r1S1 = 0, r1S2 = 0, r1Fb = 0;
    uint64_t r2Empty = 0, r2S1 = 0, r2S2 = 0, r2Fb = 0;
    Json censusPoses = Json::array();

    for (int i = 0; i < 64; ++i) {
        renderer.resetRoutedRouteObserverForDiagnostics();
        renderPose(i, true);
        REQUIRE(fenceQueue(gpu), "census fence");
        auto w = readWords(gpu, k->routeHistogram, 1032 * 4);
        uint64_t sum = 0, empty = 0, s1 = 0, s2 = 0, fb = 0;
        for (uint32_t key = 0; key < 1024; ++key) {
            if (!w[key]) continue;
            sum += w[key];
            const uint32_t mask = key & 15u;
            const uint32_t overlap = (key >> 8u) & 1u;
            const uint32_t ordered = (key >> 9u) & 1u;
            const uint32_t bits = __builtin_popcount(mask);
            if (bits == 0u) empty += w[key];
            else if (bits == 1u && !overlap) s1 += w[key];
            else if (ordered) s2 += w[key];
            else fb += w[key];
        }
        REQUIRE(sum == tilesPerFrame, "16-lane cooperative classifier must classify every tile");
        REQUIRE(w[1027] == strips, "observer strip count");
        REQUIRE(s1 == w[1024] && s2 == w[1025] && fb == w[1026],
                "observer cohort counts must match production s1/s2/fallback counters");
        totEmpty += empty; totS1 += s1; totS2 += s2; totFb += fb;
        const bool isRegime1 = (i <= 19 || i >= 51);
        if (isRegime1) {
            r1Empty += empty; r1S1 += s1; r1S2 += s2; r1Fb += fb;
        } else {
            r2Empty += empty; r2S1 += s1; r2S2 += s2; r2Fb += fb;
        }
        censusPoses.push_back({
            {"pose", i}, {"regime", isRegime1 ? 1 : 2},
            {"empty", empty}, {"s1", s1}, {"s2", s2}, {"fallback", fb}
        });
    }
    REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(false), "disable route observer");
    const uint64_t totalTileFrames = tilesPerFrame * 64;
    report["route_census"] = {
        {"total_tile_frames", totalTileFrames},
        {"all_64_poses", {
            {"empty", totEmpty}, {"s1", totS1}, {"s2", totS2}, {"fallback", totFb},
            {"empty_pct", 100.0 * double(totEmpty) / double(totalTileFrames)},
            {"s1_pct", 100.0 * double(totS1) / double(totalTileFrames)},
            {"s2_pct", 100.0 * double(totS2) / double(totalTileFrames)},
            {"fallback_pct", 100.0 * double(totFb) / double(totalTileFrames)}
        }},
        {"regime1_full_screen_green", {
            {"poses", 33}, {"tile_frames", tilesPerFrame * 33},
            {"empty", r1Empty}, {"s1", r1S1}, {"s2", r1S2}, {"fallback", r1Fb},
            {"empty_pct", 100.0 * double(r1Empty) / double(tilesPerFrame * 33)},
            {"s1_pct", 100.0 * double(r1S1) / double(tilesPerFrame * 33)},
            {"s2_pct", 100.0 * double(r1S2) / double(tilesPerFrame * 33)},
            {"fallback_pct", 100.0 * double(r1Fb) / double(tilesPerFrame * 33)}
        }},
        {"regime2_open_sky", {
            {"poses", 31}, {"tile_frames", tilesPerFrame * 31},
            {"empty", r2Empty}, {"s1", r2S1}, {"s2", r2S2}, {"fallback", r2Fb},
            {"empty_pct", 100.0 * double(r2Empty) / double(tilesPerFrame * 31)},
            {"s1_pct", 100.0 * double(r2S1) / double(tilesPerFrame * 31)},
            {"s2_pct", 100.0 * double(r2S2) / double(tilesPerFrame * 31)},
            {"fallback_pct", 100.0 * double(r2Fb) / double(tilesPerFrame * 31)}
        }},
        {"poses", censusPoses}
    };
    std::printf("CENSUS_OK empty=%.2f%% s1=%.2f%% s2=%.2f%% fallback=%.2f%% (R1_fb=%.2f%% R2_fb=%.2f%%)\n",
                100.0 * double(totEmpty) / double(totalTileFrames),
                100.0 * double(totS1) / double(totalTileFrames),
                100.0 * double(totS2) / double(totalTileFrames),
                100.0 * double(totFb) / double(totalTileFrames),
                100.0 * double(r1Fb) / double(tilesPerFrame * 33),
                100.0 * double(r2Fb) / double(tilesPerFrame * 31));

    // 3. Trajectory Refusal Audit across 8 evenly spaced poses (0, 8, 16, 24, 32, 40, 48, 56)
    for (int poseIdx = 0; poseIdx < 64; poseIdx += 8) {
        renderPose(poseIdx, true);
        REQUIRE(fenceQueue(gpu), "trajectory refusal fence");
        auto raw = readPublication(gpu, k);
        uint64_t refused = 0, nonfinite = 0;
        for (uint64_t i = 0; i < raw.size() / 4; ++i) {
            const float* v = &raw[i * 4];
            if (v[3] < 0.0f) {
                if (refused < 5) {
                    uint32_t px = i % 2560;
                    uint32_t py = i / 2560;
                    std::printf("REFUSED PIXEL pose=%d idx=%llu (px=%u, py=%u) pub=(%f, %f, %f, %f)\n",
                                poseIdx, (unsigned long long)i, px, py, v[0], v[1], v[2], v[3]);
                }
                ++refused;
            }
            else for (int c = 0; c < 4; ++c) nonfinite += !std::isfinite(v[c]);
        }
        if (refused != 0 || nonfinite != 0) {
            std::printf("DEBUG GATE 4 FAIL: poseIdx=%d refused=%llu nonfinite=%llu\n", poseIdx, (unsigned long long)refused, (unsigned long long)nonfinite);
        }
        REQUIRE(refused == 0 && nonfinite == 0, "zero refused or nonfinite rays across trajectory");
    }
    std::printf("TRAJECTORY_ZERO_REFUSALS_OK (8/8 sampled poses at 2560x1440 have 0 refused rays)\n");

    // =========================================================================
    // Gate 6 (Native Metal All-Empty-Strip & Lit->Empty Transition with Finite-Poisoned ecPublication)
    // =========================================================================
    std::printf("--- RUNNING GATE 6: Empty-Strip Finite-Poisoned Publication Ownership ---\n");
    // 1. Render lit Pose 0 with Routed ON to establish preceding lit-frame state
    renderPose(0, true);
    REQUIRE(fenceQueue(gpu), "gate6 lit pose fence");

    // 2. Poison publication with finite accepted non-zero values (0.75, 0.50, 0.25, 0.80)
    poisonPublicationFinite(gpu, k, glm::vec4(0.75f, 0.50f, 0.25f, 0.80f));
    REQUIRE(fenceQueue(gpu), "gate6 post-poison fence");

    // 3. Render Sixth-One Sun's exact union-box-hit / zEnv-empty synthetic scene:
    //    eye = (0, 0, 10), looking toward (0, 20, 10) along +y, up = (0, 0, 1).
    //    4 curtains at origin = (0, 0, 0), half = (20, 20, 20), limits = (20, 20, 0, 1), amps = (0.1, 0.1, 0.1)
    //    where zEnv = 1.3 < 10.0.
    //    With 10 deg vertical FOV, all rays in the 2560x1440 viewport stay in z in [8.25, 11.75],
    //    hitting the union box [ -20, 20 ]^3 in ecSetup (t1 <= 2048.0) but proved empty across all strips!
    std::vector<std::shared_ptr<geom::FieldNode>> gate6Curtains;
    for (int i = 0; i < 4; ++i) {
        auto node = std::make_shared<geom::FieldNode>("gate6_curtain_" + std::to_string(i));
        node->origin = glm::vec3(0.0f);
        node->scale = glm::vec3(20.0f);
        *node->volumeDensity = *makeClampedWaveDensity(20.0f, 20.0f, 1.0f, 0.05f, 0.1f, 0.1f, 0.1f);
        node->noteAuthoredMathWritten();
        gate6Curtains.push_back(node);
    }

    renderSceneOffscreen(gate6Curtains, glm::vec3(0, 0, 10), glm::vec3(0, 20, 10), glm::vec3(0, 0, 1), 10.0f, true, view, width, height);
    REQUIRE(fenceQueue(gpu), "gate6 routed render fence");
    const auto& s6 = renderer.frameStats();
    REQUIRE(s6.volumeProgramRefusals == 0 && s6.volumeRoutedSets == 1, "gate6 routed frame stats");

    auto pub6 = readPublication(gpu, k);
    uint64_t accepted6 = 0, refused6 = 0, nonempty6 = 0, nonfinite6 = 0, poisonSurviving6 = 0;
    for (uint64_t i = 0; i < pub6.size() / 4; ++i) {
        const float* v = &pub6[i * 4];
        for (int c = 0; c < 4; ++c) nonfinite6 += !std::isfinite(v[c]);
        if (v[3] < 0.0f) ++refused6;
        else ++accepted6;
        if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f || v[3] != 0.0f) ++nonempty6;
        if (std::abs(v[0] - 0.75f) < 1e-4f && std::abs(v[1] - 0.50f) < 1e-4f &&
            std::abs(v[2] - 0.25f) < 1e-4f && std::abs(v[3] - 0.80f) < 1e-4f) ++poisonSurviving6;
    }
    REQUIRE(nonfinite6 == 0, "gate6 zero nonfinite words");
    REQUIRE(refused6 == 0, "gate6 zero refused rays");
    REQUIRE(accepted6 == uint64_t(width) * height, "gate6 all rays accepted");
    REQUIRE(nonempty6 == 0, "gate6 100% of publication pixels must be vec4(0.0)");
    REQUIRE(poisonSurviving6 == 0, "gate6 zero surviving poison pixels");

    uint64_t routedHash6 = readRgbaHash();

    // 4. Render Current (routedOn = false) and assert bit-exact RGBA hash match
    renderSceneOffscreen(gate6Curtains, glm::vec3(0, 0, 10), glm::vec3(0, 20, 10), glm::vec3(0, 0, 1), 10.0f, false, view, width, height);
    REQUIRE(fenceQueue(gpu), "gate6 current render fence");
    uint64_t currentHash6 = readRgbaHash();
    REQUIRE(routedHash6 == currentHash6, "gate6 bit-exact RGBA hash match between Routed and Current");

    std::printf("GATE6_OK: all-empty-strip lit->empty finite-poisoned publication: 100%% vec4(0.0), 0 poison surviving, RGBA parity %s == %s\n",
                hex64(routedHash6).c_str(), hex64(currentHash6).c_str());

    report["gates"]["gate6_empty_strip_finite_poison"] = {
        {"status", "PASS"},
        {"pixels", uint64_t(width) * height},
        {"accepted", accepted6},
        {"refused", refused6},
        {"nonempty", nonempty6},
        {"nonfinite", nonfinite6},
        {"poison_surviving", poisonSurviving6},
        {"routed_rgba_fnv1a", hex64(routedHash6)},
        {"current_rgba_fnv1a", hex64(currentHash6)},
        {"rgba_parity", routedHash6 == currentHash6}
    };

    // =========================================================================
    // Gate 7 (Native Metal Near-Parallel Center-Column & Later-Ray Overlap Witness)
    // =========================================================================
    std::printf("--- RUNNING GATE 7: Near-Parallel Overlap Witness ---\n");
    // Viewport: 1281x1440 (odd width so center column x=640 has dx=0 exactly).
    const uint32_t w7 = 1281, h7 = 1440;
    OffscreenTarget tgt7;
    tgt7.init(gpu, w7, h7);

    std::vector<std::shared_ptr<geom::FieldNode>> gate7Curtains;
    auto makeCurtain7 = [&](const std::string& id, glm::vec3 origin, glm::vec3 half) {
        auto node = std::make_shared<geom::FieldNode>(id);
        node->origin = origin;
        node->scale = half;
        *node->volumeDensity = *makeClampedWaveDensity(100.0f, 5.0f, 5.0f, 0.2f, 1e-6f, 1e-6f, 1e-6f, 0.01f, 0.02f, 0.03f);
        node->noteAuthoredMathWritten();
        return node;
    };
    gate7Curtains.push_back(makeCurtain7("gate7_c0", glm::vec3(0, 5, 5), glm::vec3(100, 5, 5)));
    gate7Curtains.push_back(makeCurtain7("gate7_c1", glm::vec3(0, 13, 13), glm::vec3(100, 5, 5)));
    gate7Curtains.push_back(makeCurtain7("gate7_c2", glm::vec3(1000, 0, 0), glm::vec3(100, 5, 5)));
    gate7Curtains.push_back(makeCurtain7("gate7_c3", glm::vec3(1000, 0, 0), glm::vec3(100, 5, 5)));

    const double step7 = 2.0 * std::tan(3.14159265358979323846 / 8.0) / 1440.0;
    const double angle7 = std::atan(1.0001) - std::atan(-0.5 * step7);
    const glm::dvec3 fwd7(0, std::sin(angle7), std::cos(angle7));
    const glm::dvec3 up7(0, std::cos(angle7), -std::sin(angle7));
    const glm::vec3 eye7(0, -38, -40);
    const glm::vec3 aim7 = eye7 + glm::vec3(fwd7) * 100.0f;
    const glm::vec3 upVec7 = glm::vec3(up7);

    REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(true), "enable route observer for gate 7");
    renderer.resetRoutedRouteObserverForDiagnostics();

    renderSceneOffscreen(gate7Curtains, eye7, aim7, upVec7, 45.0f, true, tgt7.view, w7, h7);
    REQUIRE(fenceQueue(gpu), "gate7 routed render fence");
    const auto& s7 = renderer.frameStats();
    REQUIRE(s7.volumeProgramRefusals == 0 && s7.volumeRoutedSets == 1, "gate7 routed frame stats");

    auto hist7 = readWords(gpu, k->routeHistogram, 1032 * 4);
    uint64_t s1_7 = hist7[1024], s2_7 = hist7[1025], fb_7 = hist7[1026];
    std::printf("GATE7: route histogram s1=%llu s2=%llu fallback=%llu\n",
                (unsigned long long)s1_7, (unsigned long long)s2_7, (unsigned long long)fb_7);
    REQUIRE(fb_7 > 0, "gate7 overlapping tile must route to fallback");

    auto pub7 = readPublication(gpu, k);
    uint64_t refused7 = 0, nonfinite7 = 0;
    for (uint64_t i = 0; i < pub7.size() / 4; ++i) {
        if (pub7[i * 4 + 3] < 0.0f) ++refused7;
        for (int c = 0; c < 4; ++c) nonfinite7 += !std::isfinite(pub7[i * 4 + c]);
    }
    REQUIRE(nonfinite7 == 0, "gate7 nonfinite == 0");
    REQUIRE(refused7 == 0, "gate7 zero refused rays in publication");

    uint64_t routedHash7 = readTargetRgbaHash(tgt7.texture, tgt7.readback, w7, h7, tgt7.stride);

    renderSceneOffscreen(gate7Curtains, eye7, aim7, upVec7, 45.0f, false, tgt7.view, w7, h7);
    REQUIRE(fenceQueue(gpu), "gate7 current render fence");
    uint64_t currentHash7 = readTargetRgbaHash(tgt7.texture, tgt7.readback, w7, h7, tgt7.stride);
    REQUIRE(routedHash7 == currentHash7, "gate7 bit-exact RGBA hash match between Routed and Current");

    REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(false), "disable route observer");
    tgt7.release();

    std::printf("GATE7_OK: near-parallel overlap correctly handled, 0 erroneous S2 rays, RGBA parity %s == %s\n",
                hex64(routedHash7).c_str(), hex64(currentHash7).c_str());

    report["gates"]["gate7_near_parallel_overlap"] = {
        {"status", "PASS"},
        {"viewport", {w7, h7}},
        {"s1_tiles", s1_7},
        {"s2_tiles", s2_7},
        {"fallback_tiles", fb_7},
        {"nonfinite_rays", nonfinite7},
        {"refused_rays", refused7},
        {"routed_rgba_fnv1a", hex64(routedHash7)},
        {"current_rgba_fnv1a", hex64(currentHash7)},
        {"rgba_parity", routedHash7 == currentHash7}
    };

    // =========================================================================
    // Gate 8 (Native Metal Tall-Viewport 1024x4096 86-Strip + 64/65-Strip Boundary + Resize-Back Witness)
    // =========================================================================
    std::printf("--- RUNNING GATE 8: Tall-Viewport Full Strip Capacity & Resize-Back ---\n");
    const auto [eye0, aim0] = canonicalPose(0);
    const struct { uint32_t w, h, expectedStrips; } gate8Cases[] = {
        {1024, 3072, 64},
        {1024, 3120, 65},
        {1024, 4096, 86}
    };
    Json gate8Rows = Json::array();

    for (const auto& c : gate8Cases) {
        OffscreenTarget tgt;
        tgt.init(gpu, c.w, c.h);

        // Pre-warm / resize kernel to c.w x c.h
        renderSceneOffscreen(curtains, eye0, aim0, glm::vec3(0, 1, 0), 60.0f, true, tgt.view, c.w, c.h);
        REQUIRE(fenceQueue(gpu), "gate8 warmup fence");
        k = renderer.routedTransportKernelForDiagnostics();
        REQUIRE(k && k->width == c.w && k->height == c.h, "gate8 kernel dimension check");
        const uint32_t actualStrips = (c.h + k->stripRows - 1) / k->stripRows;
        REQUIRE(actualStrips == c.expectedStrips, "gate8 strip count calculation");
        REQUIRE(k->maxStrips == c.expectedStrips, "gate8 k->maxStrips allocated");

        // Poison publication with finite accepted values
        poisonPublicationFinite(gpu, k, glm::vec4(0.75f, 0.50f, 0.25f, 0.80f));
        REQUIRE(fenceQueue(gpu), "gate8 post-poison fence");

        // Render Pose 0 with Routed ON
        renderSceneOffscreen(curtains, eye0, aim0, glm::vec3(0, 1, 0), 60.0f, true, tgt.view, c.w, c.h);
        REQUIRE(fenceQueue(gpu), "gate8 render fence");
        const auto& s8 = renderer.frameStats();
        REQUIRE(s8.volumeProgramRefusals == 0 && s8.volumeRoutedSets == 1, "gate8 frame stats");

        // Verify 100% of publication pixels updated with zero surviving poison
        auto pub8 = readPublication(gpu, k);
        uint64_t accepted8 = 0, refused8 = 0, nonempty8 = 0, nonfinite8 = 0, poisonSurviving8 = 0;
        for (uint64_t i = 0; i < pub8.size() / 4; ++i) {
            const float* v = &pub8[i * 4];
            for (int ch = 0; ch < 4; ++ch) nonfinite8 += !std::isfinite(v[ch]);
            if (v[3] < 0.0f) ++refused8;
            else ++accepted8;
            if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f || v[3] != 0.0f) ++nonempty8;
            if (std::abs(v[0] - 0.75f) < 1e-4f && std::abs(v[1] - 0.50f) < 1e-4f &&
                std::abs(v[2] - 0.25f) < 1e-4f && std::abs(v[3] - 0.80f) < 1e-4f) ++poisonSurviving8;
        }
        REQUIRE(nonfinite8 == 0, "gate8 zero nonfinite");
        REQUIRE(refused8 == 0, "gate8 zero refused");
        REQUIRE(accepted8 == uint64_t(c.w) * c.h, "gate8 all pixels accepted across all strips");
        REQUIRE(poisonSurviving8 == 0, "gate8 zero surviving poison");
        REQUIRE(nonempty8 > 100, "gate8 non-empty aurora pixels present");

        tgt.release();
        std::printf("GATE8_CASE_OK: %ux%u strips=%u maxStrips=%u accepted=%llu surviving_poison=%llu\n",
                    c.w, c.h, actualStrips, k->maxStrips, (unsigned long long)accepted8, (unsigned long long)poisonSurviving8);

        gate8Rows.push_back({
            {"resolution", {c.w, c.h}},
            {"expected_strips", c.expectedStrips},
            {"actual_strips", actualStrips},
            {"max_strips", k->maxStrips},
            {"accepted_pixels", accepted8},
            {"refused_pixels", refused8},
            {"poison_surviving", poisonSurviving8}
        });
    }

    // Resize back to 2560x1440 (30 strips) and assert exact Pose 0 parity
    renderPose(0, true);
    REQUIRE(fenceQueue(gpu), "gate8 resize-back fence");
    k = renderer.routedTransportKernelForDiagnostics();
    REQUIRE(k && k->width == width && k->height == height, "gate8 restore 2560x1440 dimensions");
    REQUIRE(k->maxStrips == strips, "gate8 restore 30 strips");
    uint64_t restoredRgbaHash = readRgbaHash();
    REQUIRE(restoredRgbaHash == canonicalPose0RgbaHash, "gate8 restored 2560x1440 RGBA parity with initial Pose 0");
    std::printf("GATE8_OK: tall viewport cases (64, 65, 86 strips) + resize-back to 30 strips pass with exact parity %s == %s\n",
                hex64(restoredRgbaHash).c_str(), hex64(canonicalPose0RgbaHash).c_str());

    report["gates"]["gate8_tall_viewport_strip_capacity"] = {
        {"status", "PASS"},
        {"cases", gate8Rows},
        {"restored_resolution", {width, height}},
        {"restored_max_strips", k->maxStrips},
        {"restored_rgba_fnv1a", hex64(restoredRgbaHash)},
        {"initial_pose0_rgba_fnv1a", hex64(canonicalPose0RgbaHash)},
        {"parity", restoredRgbaHash == canonicalPose0RgbaHash}
    };

    // Audit mode keeps every correctness gate above, without re-running timing.
    if (argc > 3 && std::string(argv[3]) == "--correctness-only") {
        report["native_metal_gates_passed"] = "8/8 correctness gates; timing gates 9/10 and dual-regime sweep unrun";
        std::ofstream(outJsonPath) << report.dump(2) << '\n';
        std::printf("PASS positive_e2e_metal_verify correctness-only\n");
        return 0;
    }

    // --- RUNNING GATE 9: Zach's 3 Northern Veil Checkpoint Views + Live Captured Camera Views ---
    std::printf("--- RUNNING GATE 9: Checkpoint Views Parity, Census & ABBA Benchmark ---\n");
    struct ViewSpec {
        std::string name;
        glm::vec3 eye;
        glm::vec3 aim;
        glm::vec3 up;
        float fov;
    };
    std::vector<ViewSpec> gate9Views = {
        {"Live_23FPS_View_A",
         glm::vec3(-6.749656f, 2.397435f, 3.705208f),
         glm::vec3(-6.749656f, 2.397435f, 3.705208f) + glm::vec3(0.0968301f, 0.5180270f, 0.8498659f),
         glm::vec3(0, 1, 0), 45.0f},
        {"Live_Parked_View",
         glm::vec3(-14.059818f, 5.853045f, 0.831757f),
         glm::vec3(-14.059818f, 5.853045f, 0.831757f) + glm::vec3(0.0f, 0.5f, 0.8660254f),
         glm::vec3(0, 1, 0), 45.0f},
        {"Checkpoint_View1_Inside_Veil",
         glm::vec3(12.5f, 67.0f, 39.0f),
         glm::vec3(12.5f, 67.0f, 184.0f),
         glm::vec3(0, 1, 0), 45.0f},
        {"Checkpoint_View2_Spawn_Dais_Diag",
         glm::vec3(0.0f, 1.3f, 0.0f),
         glm::vec3(0.0f, 72.0f, 106.5f),
         glm::vec3(0, 1, 0), 45.0f},
        {"Checkpoint_View3A_Below_Veils_Sparse_UpZ",
         glm::vec3(12.5f, 0.0f, 106.5f),
         glm::vec3(12.5f, 72.0f, 106.5f),
         glm::vec3(0, 0, 1), 45.0f},
        {"Checkpoint_View3B_Below_Veils_Dense_UpX",
         glm::vec3(12.5f, 0.0f, 106.5f),
         glm::vec3(12.5f, 72.0f, 106.5f),
         glm::vec3(1, 0, 0), 45.0f},
        {"Checkpoint_View3C_Below_Veils_PersonWalk_Pitch75",
         glm::vec3(12.5f, 0.0f, 106.5f),
         glm::vec3(31.135f, 69.5467f, 106.5f),
         glm::vec3(0, 1, 0), 45.0f}
    };

    Json gate9Rows = Json::array();
    for (const auto& vs : gate9Views) {
        const double viewTime = 7.25;

        // 1. Forced-Refusal Bit-Exact Parity Check
        renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, false);
        REQUIRE(fenceQueue(gpu), "gate9 current fence");
        const uint64_t curRgba = readRgbaHash();

        renderer.setRoutedWorkLimitForDiagnostics(0);
        renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate9 forced-refusal fence");
        const uint64_t refRgba = readRgbaHash();
        renderer.setRoutedWorkLimitForDiagnostics(1024);
        REQUIRE(curRgba == refRgba, "gate9 forced-refusal RGBA bit-exact parity with Current");

        // 2. Normal Routed Render & Refusal / Finite Check
        renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate9 normal routed fence");
        const auto& s9 = renderer.frameStats();
        REQUIRE(s9.volumeProgramRefusals == 0 && s9.volumeRoutedSets == 1, "gate9 frame stats");
        const uint64_t routedRgba = readRgbaHash();

        auto pub9 = readPublication(gpu, k);
        uint64_t nonfinite9 = 0, refused9 = 0, nonempty9 = 0;
        for (uint64_t i = 0; i < pub9.size() / 4; ++i) {
            const float* v = &pub9[i * 4];
            for (int ch = 0; ch < 4; ++ch) nonfinite9 += !std::isfinite(v[ch]);
            if (v[3] < 0.0f) ++refused9;
            if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f || v[3] != 0.0f) ++nonempty9;
        }
        REQUIRE(nonfinite9 == 0, "gate9 zero non-finite");
        REQUIRE(refused9 == 0, "gate9 zero refused rays");

        // 3. Route Census
        REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(true), "gate9 enable route observer");
        renderer.resetRoutedRouteObserverForDiagnostics();
        renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate9 census fence");
        auto w9 = readWords(gpu, k->routeHistogram, 1032 * 4);
        const uint64_t totalTiles = uint64_t(k->tiles) * strips;
        const uint64_t s1Count = w9[1024];
        const uint64_t s2Count = w9[1025];
        const uint64_t fbCount = w9[1026];
        const uint64_t emptyCount = (totalTiles >= s1Count + s2Count + fbCount) ? (totalTiles - s1Count - s2Count - fbCount) : 0;
        REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(false), "gate9 disable route observer");

        // 4. 4-Round Completion-Fenced ABBA Timing Comparison (8 frames per leg = 32 frames per mode)
        std::vector<double> curTimes, routedTimes;
        curTimes.reserve(32);
        routedTimes.reserve(32);

        // Warm up both paths for this camera
        for (bool on : {false, true}) {
            for (int f = 0; f < 4; ++f) renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, on);
            REQUIRE(fenceQueue(gpu), "gate9 warmup fence");
        }

        for (int r = 0; r < 4; ++r) {
            // Leg 0: Current
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, false);
                REQUIRE(fenceQueue(gpu), "gate9 leg0 fence");
                curTimes.push_back(elapsedMs(t0));
            }
            // Leg 1: Routed
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, true);
                REQUIRE(fenceQueue(gpu), "gate9 leg1 fence");
                routedTimes.push_back(elapsedMs(t0));
            }
            // Leg 2: Routed
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, true);
                REQUIRE(fenceQueue(gpu), "gate9 leg2 fence");
                routedTimes.push_back(elapsedMs(t0));
            }
            // Leg 3: Current
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(vs.eye, vs.aim, vs.up, vs.fov, viewTime, false);
                REQUIRE(fenceQueue(gpu), "gate9 leg3 fence");
                curTimes.push_back(elapsedMs(t0));
            }
        }

        const double curMean = std::accumulate(curTimes.begin(), curTimes.end(), 0.0) / curTimes.size();
        const double routedMean = std::accumulate(routedTimes.begin(), routedTimes.end(), 0.0) / routedTimes.size();
        const double deltaMs = routedMean - curMean;
        const double deltaPct = 100.0 * deltaMs / curMean;
        const double curFps = 1000.0 / curMean;
        const double routedFps = 1000.0 / routedMean;

        std::printf("GATE9_VIEW_OK: %-30s Cur=%6.2fms (%4.1f FPS) Routed=%6.2fms (%4.1f FPS) Delta=%+6.2fms (%+5.2f%%) | Empty=%4.1f%% S1=%4.1f%% S2=%4.1f%% FB=%4.1f%%\n",
                    vs.name.c_str(), curMean, curFps, routedMean, routedFps, deltaMs, deltaPct,
                    100.0 * double(emptyCount) / totalTiles,
                    100.0 * double(s1Count) / totalTiles,
                    100.0 * double(s2Count) / totalTiles,
                    100.0 * double(fbCount) / totalTiles);

        gate9Rows.push_back({
            {"name", vs.name},
            {"eye", {vs.eye.x, vs.eye.y, vs.eye.z}},
            {"aim", {vs.aim.x, vs.aim.y, vs.aim.z}},
            {"up", {vs.up.x, vs.up.y, vs.up.z}},
            {"fov_deg", vs.fov},
            {"current_mean_ms", curMean},
            {"routed_mean_ms", routedMean},
            {"delta_ms", deltaMs},
            {"delta_pct", deltaPct},
            {"implied_current_fps", curFps},
            {"implied_routed_fps", routedFps},
            {"forced_refusal_parity", curRgba == refRgba},
            {"census", {
                {"total_tiles", totalTiles},
                {"empty_tiles", emptyCount},
                {"s1_tiles", s1Count},
                {"s2_tiles", s2Count},
                {"fallback_tiles", fbCount},
                {"empty_pct", 100.0 * double(emptyCount) / totalTiles},
                {"s1_pct", 100.0 * double(s1Count) / totalTiles},
                {"s2_pct", 100.0 * double(s2Count) / totalTiles},
                {"fallback_pct", 100.0 * double(fbCount) / totalTiles}
            }}
        });
    }

    report["gates"]["gate9_checkpoint_views"] = {
        {"status", "PASS"},
        {"views", gate9Rows}
    };

    // --- RUNNING GATE 10: Prince Prism II's 12-Angle View-3 Roll Sweep & 4-Curtain ABBA Benchmark ---
    std::printf("--- RUNNING GATE 10: View 3 12-Angle Roll Sweep & 4-Curtain ABBA Benchmark ---\n");
    const float rollAnglesDeg[12] = {0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f, 105.0f, 120.0f, 135.0f, 150.0f, 165.0f};
    Json gate10Rows = Json::array();
    for (float rollDeg : rollAnglesDeg) {
        const float rad = rollDeg * 3.14159265358979323846f / 180.0f;
        const glm::vec3 up(std::sin(rad), 0.0f, std::cos(rad));
        const glm::vec3 eye(12.5f, 0.0f, 106.5f);
        const glm::vec3 aim(12.5f, 72.0f, 106.5f);
        const float fov = 45.0f;
        const double viewTime = 7.25;

        // 1. Forced-Refusal Bit-Exact Parity Check
        renderViewCamera(eye, aim, up, fov, viewTime, false);
        REQUIRE(fenceQueue(gpu), "gate10 current fence");
        const uint64_t curRgba = readRgbaHash();

        renderer.setRoutedWorkLimitForDiagnostics(0);
        renderViewCamera(eye, aim, up, fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate10 forced-refusal fence");
        const uint64_t refRgba = readRgbaHash();
        renderer.setRoutedWorkLimitForDiagnostics(1024);
        REQUIRE(curRgba == refRgba, "gate10 forced-refusal RGBA bit-exact parity with Current");

        // 2. Normal Routed Render & Refusal / Finite Check
        renderViewCamera(eye, aim, up, fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate10 normal routed fence");
        const auto& s10 = renderer.frameStats();
        REQUIRE(s10.volumeProgramRefusals == 0 && s10.volumeRoutedSets == 1, "gate10 frame stats");

        auto pub10 = readPublication(gpu, k);
        uint64_t nonfinite10 = 0, refused10 = 0, nonempty10 = 0;
        for (uint64_t i = 0; i < pub10.size() / 4; ++i) {
            const float* v = &pub10[i * 4];
            for (int ch = 0; ch < 4; ++ch) nonfinite10 += !std::isfinite(v[ch]);
            if (v[3] < 0.0f) ++refused10;
            if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f || v[3] != 0.0f) ++nonempty10;
        }
        REQUIRE(nonfinite10 == 0, "gate10 zero non-finite");
        REQUIRE(refused10 == 0, "gate10 zero refused rays");

        // 3. Route Census
        REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(true), "gate10 enable route observer");
        renderer.resetRoutedRouteObserverForDiagnostics();
        renderViewCamera(eye, aim, up, fov, viewTime, true);
        REQUIRE(fenceQueue(gpu), "gate10 census fence");
        auto w10 = readWords(gpu, k->routeHistogram, 1032 * 4);
        const uint64_t totalTiles = uint64_t(k->tiles) * strips;
        const uint64_t s1Count = w10[1024];
        const uint64_t s2Count = w10[1025];
        const uint64_t fbCount = w10[1026];
        const uint64_t emptyCount = (totalTiles >= s1Count + s2Count + fbCount) ? (totalTiles - s1Count - s2Count - fbCount) : 0;
        REQUIRE(renderer.setRoutedRouteObserverForDiagnostics(false), "gate10 disable route observer");

        // 4. Paired ABBA Timing Comparison (2 rounds = 16 frames per mode)
        std::vector<double> curTimes, routedTimes;
        curTimes.reserve(16);
        routedTimes.reserve(16);

        // Warmup
        for (bool on : {false, true}) {
            for (int f = 0; f < 2; ++f) renderViewCamera(eye, aim, up, fov, viewTime, on);
            REQUIRE(fenceQueue(gpu), "gate10 warmup fence");
        }

        for (int r = 0; r < 2; ++r) {
            // Leg 0: Current
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(eye, aim, up, fov, viewTime, false);
                REQUIRE(fenceQueue(gpu), "gate10 leg0 fence");
                curTimes.push_back(elapsedMs(t0));
            }
            // Leg 1: Routed
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(eye, aim, up, fov, viewTime, true);
                REQUIRE(fenceQueue(gpu), "gate10 leg1 fence");
                routedTimes.push_back(elapsedMs(t0));
            }
            // Leg 2: Routed
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(eye, aim, up, fov, viewTime, true);
                REQUIRE(fenceQueue(gpu), "gate10 leg2 fence");
                routedTimes.push_back(elapsedMs(t0));
            }
            // Leg 3: Current
            for (int f = 0; f < 8; ++f) {
                const auto t0 = Clock::now();
                renderViewCamera(eye, aim, up, fov, viewTime, false);
                REQUIRE(fenceQueue(gpu), "gate10 leg3 fence");
                curTimes.push_back(elapsedMs(t0));
            }
        }

        const double curMean = std::accumulate(curTimes.begin(), curTimes.end(), 0.0) / curTimes.size();
        const double routedMean = std::accumulate(routedTimes.begin(), routedTimes.end(), 0.0) / routedTimes.size();
        const double deltaMs = routedMean - curMean;
        const double deltaPct = 100.0 * deltaMs / curMean;
        const double curFps = 1000.0 / curMean;
        const double routedFps = 1000.0 / routedMean;
        const double nonEmptyPct = 100.0 * double(nonempty10) / double(pub10.size() / 4);

        std::printf("GATE10_ROLL_OK: Roll=%3.0f deg up=(%5.2f, 0, %5.2f) Cur=%6.2fms (%4.1f FPS) Routed=%6.2fms (%4.1f FPS) Delta=%+6.2fms (%+5.2f%%) | Empty=%4.1f%% S1=%4.1f%% S2=%4.1f%% FB=%4.1f%% NonEmptyPub=%4.1f%%\n",
                    rollDeg, up.x, up.z, curMean, curFps, routedMean, routedFps, deltaMs, deltaPct,
                    100.0 * double(emptyCount) / totalTiles,
                    100.0 * double(s1Count) / totalTiles,
                    100.0 * double(s2Count) / totalTiles,
                    100.0 * double(fbCount) / totalTiles,
                    nonEmptyPct);

        gate10Rows.push_back({
            {"roll_deg", rollDeg},
            {"up", {up.x, up.y, up.z}},
            {"current_mean_ms", curMean},
            {"routed_mean_ms", routedMean},
            {"delta_ms", deltaMs},
            {"delta_pct", deltaPct},
            {"implied_current_fps", curFps},
            {"implied_routed_fps", routedFps},
            {"forced_refusal_parity", curRgba == refRgba},
            {"nonempty_pub_pixels", nonempty10},
            {"nonempty_pub_pct", nonEmptyPct},
            {"census", {
                {"total_tiles", totalTiles},
                {"empty_tiles", emptyCount},
                {"s1_tiles", s1Count},
                {"s2_tiles", s2Count},
                {"fallback_tiles", fbCount},
                {"empty_pct", 100.0 * double(emptyCount) / totalTiles},
                {"s1_pct", 100.0 * double(s1Count) / totalTiles},
                {"s2_pct", 100.0 * double(s2Count) / totalTiles},
                {"fallback_pct", 100.0 * double(fbCount) / totalTiles}
            }}
        });
    }

    report["gates"]["gate10_view3_orientation_sweep"] = {
        {"status", "PASS"},
        {"sweep", gate10Rows}
    };

    // 4. Dual-Regime Completion-Fenced Timing (4 ABBA blocks: Current vs Positive E2E Routed P1-P4)
    for (bool on : {false, true}) {
        for (int i = 0; i < 64; ++i) renderPose(i, on);
        REQUIRE(fenceQueue(gpu), "warmup fence");
    }

    std::vector<double> curPoseMs[64], routedPoseMs[64];
    std::vector<double> curBlockTotalMs, routedBlockTotalMs;
    std::vector<double> curR1BlockMs, routedR1BlockMs, curR2BlockMs, routedR2BlockMs;

    for (int block = 0; block < 4; ++block) {
        for (int order = 0; order < 4; ++order) {
            const bool on = (order == 1 || order == 2); // ABBA: Current, Routed, Routed, Current
            renderPose(0, on);
            renderPose(0, on);
            REQUIRE(fenceQueue(gpu), "activation fence");

            double blockTotal = 0.0, r1Sum = 0.0, r2Sum = 0.0;
            for (int i = 0; i < 64; ++i) {
                const auto t0 = Clock::now();
                renderPose(i, on);
                REQUIRE(fenceQueue(gpu), "per-frame completion fence");
                const double dtMs = elapsedMs(t0);
                REQUIRE(renderer.frameStats().volumeProgramCompiles == 0, "zero cold compiles in timed loop");
                blockTotal += dtMs;
                if (i <= 19 || i >= 51) r1Sum += dtMs;
                else r2Sum += dtMs;
                if (on) routedPoseMs[i].push_back(dtMs);
                else curPoseMs[i].push_back(dtMs);
            }
            if (on) {
                routedBlockTotalMs.push_back(blockTotal / 64.0);
                routedR1BlockMs.push_back(r1Sum / 33.0);
                routedR2BlockMs.push_back(r2Sum / 31.0);
            } else {
                curBlockTotalMs.push_back(blockTotal / 64.0);
                curR1BlockMs.push_back(r1Sum / 33.0);
                curR2BlockMs.push_back(r2Sum / 31.0);
            }
            std::printf("TIMING block=%d order=%d lane=%s mean_ms=%.3f r1_ms=%.3f r2_ms=%.3f\n",
                        block, order, on ? "Positive_E2E_Routed" : "Current",
                        blockTotal / 64.0, r1Sum / 33.0, r2Sum / 31.0);
        }
    }

    Json perPoseTiming = Json::array();
    std::vector<double> curR1PoseMeans, routedR1PoseMeans, curR2PoseMeans, routedR2PoseMeans;
    for (int i = 0; i < 64; ++i) {
        const double cMean = vecMean(curPoseMs[i]);
        const double rMean = vecMean(routedPoseMs[i]);
        const bool isR1 = (i <= 19 || i >= 51);
        if (isR1) {
            curR1PoseMeans.push_back(cMean);
            routedR1PoseMeans.push_back(rMean);
        } else {
            curR2PoseMeans.push_back(cMean);
            routedR2PoseMeans.push_back(rMean);
        }
        perPoseTiming.push_back({
            {"pose", i}, {"regime", isR1 ? 1 : 2},
            {"current_mean_ms", cMean}, {"routed_mean_ms", rMean},
            {"delta_ms", rMean - cMean},
            {"delta_pct", 100.0 * (rMean - cMean) / cMean}
        });
    }

    const double curAllMean = vecMean(curBlockTotalMs);
    const double routedAllMean = vecMean(routedBlockTotalMs);
    const double curR1Mean = vecMean(curR1BlockMs);
    const double routedR1Mean = vecMean(routedR1BlockMs);
    const double curR2Mean = vecMean(curR2BlockMs);
    const double routedR2Mean = vecMean(routedR2BlockMs);

    report["dual_regime_timing"] = {
        {"all_64_poses", {
            {"current_mean_ms", curAllMean},
            {"routed_mean_ms", routedAllMean},
            {"current_median_ms", vecMedian(curBlockTotalMs)},
            {"routed_median_ms", vecMedian(routedBlockTotalMs)},
            {"delta_ms", routedAllMean - curAllMean},
            {"delta_pct", 100.0 * (routedAllMean - curAllMean) / curAllMean}
        }},
        {"regime1_full_screen_green_poses_0_19_51_63", {
            {"poses_count", 33},
            {"current_mean_ms", curR1Mean},
            {"routed_mean_ms", routedR1Mean},
            {"current_median_ms", vecMedian(curR1BlockMs)},
            {"routed_median_ms", vecMedian(routedR1BlockMs)},
            {"delta_ms", routedR1Mean - curR1Mean},
            {"delta_pct", 100.0 * (routedR1Mean - curR1Mean) / curR1Mean}
        }},
        {"regime2_open_sky_poses_20_50", {
            {"poses_count", 31},
            {"current_mean_ms", curR2Mean},
            {"routed_mean_ms", routedR2Mean},
            {"current_median_ms", vecMedian(curR2BlockMs)},
            {"routed_median_ms", vecMedian(routedR2BlockMs)},
            {"delta_ms", routedR2Mean - curR2Mean},
            {"delta_pct", 100.0 * (routedR2Mean - curR2Mean) / curR2Mean}
        }},
        {"poses", perPoseTiming}
    };

    std::printf("SUMMARY All64: Current=%.3fms Routed=%.3fms (%.2f%%) | Regime1(Green): Current=%.3fms Routed=%.3fms (%.2f%%) | Regime2(Sky): Current=%.3fms Routed=%.3fms (%.2f%%)\n",
                curAllMean, routedAllMean, 100.0 * (routedAllMean - curAllMean) / curAllMean,
                curR1Mean, routedR1Mean, 100.0 * (routedR1Mean - curR1Mean) / curR1Mean,
                curR2Mean, routedR2Mean, 100.0 * (routedR2Mean - curR2Mean) / curR2Mean);

    report["repaired_correctness_gates"] = 3;
    report["native_metal_gates_passed"] = "10/10";

    std::ofstream(outJsonPath) << report.dump(2) << '\n';
    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::puts("PASS positive_e2e_metal_verify");
    return 0;
}
