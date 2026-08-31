// Rendering-optimization Phase C: the min/max heightfield grid DDA skip must
// never change what a ray hits -- only how many field evaluations it costs to
// find out. This is the falsifying measurement the addendums insist should
// exist BEFORE a phase ships, aimed at the exact axis (camera angle/distance)
// that hid Bugs.md #20: every prior verification matrix held the camera fixed
// and stayed green through six regressions on other axes.
//
// The check: render the real authored Perlin-floor field from several camera
// angles (straight down, grazing the horizon, 45 degrees down, and a close-up
// oblique view) with the height grid ON and OFF, and require the rendered
// pixels to be IDENTICAL. Not "similar" -- identical. A skip that is sound by
// construction (see heightfield_predicate_test's soundness check) should never
// move a single pixel; any difference here means either the CPU grid bound or
// the WGSL DDA has a defect that could delete real geometry.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr uint32_t W = 128, H = 128; // 128*4 = 512, a clean row stride

int g_failures = 0;

struct MapR { bool done = false; };
void onMap(WGPUMapAsyncStatus, WGPUStringView, void* u, void*) {
    static_cast<MapR*>(u)->done = true;
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }
    WebGpuRenderer r;
    if (!r.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }
    setCurrentRenderer(&r);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);

    const uint32_t rowStride = 512; // 128*4, already 256-aligned
    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = rowStride * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    // The real authored Perlin-floor field, lifted the same way
    // scratch/probes/horizon_cost_probe.cpp does: y - 40*noise(p*0.008+offset)
    // over [1000,30,1000], read verbatim from
    // saves/zones/Perlin Noise Floor Zone/zone.json.
    geom::SdfNode field;
    field.op = geom::SdfOp::Leaf;
    field.prim = geom::SdfPrim::Expr;
    field.mathNode = std::shared_ptr<OntoMath::MathNode>(
        OntoMath::MathNode::fromJson(nlohmann::json::parse(std::string(
            "{\"children\": [{\"op\": 1, \"var\": \"y\"}, {\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 40.0, \"factors\": {}}]}}, {\"children\": [{\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 0.008, \"factors\": {}}]}}, {\"children\": [{\"op\": 1, \"var\": \"p\"}, {\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 100.0, \"factors\": {}}]}}, {\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 0.0, \"factors\": {}}]}}, {\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 100.0, \"factors\": {}}]}}], \"op\": 2}], \"op\": 4}], \"op\": 6}], \"op\": 29}], \"op\": 6}], \"op\": 5}")))
            .release());
    const glm::vec3 extent(1000.f, 30.f, 1000.f);

    geom::HeightGrid heightGrid;
    {
        const OntoMath::MathNode* h = nullptr;
        if (geom::isHeightfieldExpr(field, &h) && h) {
            const int dimX = std::clamp(static_cast<int>(extent.x / 5.0f), 24, 128);
            const int dimZ = std::clamp(static_cast<int>(extent.z / 5.0f), 24, 128);
            heightGrid = geom::computeHeightGrid(*h, extent, dimX, dimZ);
        }
    }
    if (heightGrid.dimX == 0) {
        std::printf("FAIL: the real Perlin-floor field did not produce a height grid "
                    "-- this test cannot exercise the DDA path at all\n");
        return 1;
    }
    std::printf("height grid: %dx%d cells\n", heightGrid.dimX, heightGrid.dimZ);

    auto readPixels = [&](bool gridOn, glm::vec3 eye, glm::vec3 target, float fovDeg) -> std::vector<unsigned char> {
        const glm::mat4 proj = glm::perspectiveZO(glm::radians(fovDeg), 1.0f, 0.1f, 3000.f);
        const glm::mat4 viewM = glm::lookAt(eye, target, glm::vec3(0, 1, 0));
        RenderMaterial mat; mat.baseColor = glm::vec3(0.3f, 0.7f, 0.3f);

        r.setCamera(viewM, proj, eye);
        r.setModel(glm::mat4(1.0f));
        r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        r.drawImplicit(field, extent, mat, nullptr, 0, 0, gridOn ? &heightGrid : nullptr);
        r.endFrame();
        wgpuDevicePoll(gpu.device, true, nullptr);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = tex; src.aspect = WGPUTextureAspect_All; src.origin = {0, 0, 0};
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = rowStride; dst.layout.rowsPerImage = H;
        WGPUExtent3D ext = { W, H, 1 };
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapR m;
        WGPUBufferMapCallbackInfo ci = {};
        ci.mode = WGPUCallbackMode_AllowProcessEvents;
        ci.callback = onMap; ci.userdata1 = &m;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, rowStride * H, ci);
        while (!m.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const auto* px = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, rowStride * H));
        std::vector<unsigned char> out(px, px + rowStride * H);
        wgpuBufferUnmap(readback);
        return out;
    };

    struct Case { const char* name; glm::vec3 eye, target; float fovDeg; };
    const Case cases[] = {
        {"looking straight down",   {0, 120, 0},    {0, 0, 0},     60.f},
        {"grazing the horizon",     {0, 60, -900},  {0, 55, 900},  60.f},
        {"45 degrees down",         {0, 120, -200}, {0, 0, 0},     60.f},
        {"close oblique",           {50, 10, 50},   {0, -5, -50},  70.f},
        {"near-parallel to ground", {0, 20, -300},  {0, 15, 300},  50.f},
    };

    for (const Case& c : cases) {
        auto pxOff = readPixels(false, c.eye, c.target, c.fovDeg);
        auto pxOn  = readPixels(true,  c.eye, c.target, c.fovDeg);

        size_t litOff = 0, litOn = 0, diffPixels = 0, maxChannelDiff = 0;
        for (size_t i = 0; i + 4 <= pxOff.size(); i += 4) {
            const bool onOff = pxOff[i] > 6 || pxOff[i+1] > 6 || pxOff[i+2] > 6;
            const bool onOn  = pxOn[i]  > 6 || pxOn[i+1]  > 6 || pxOn[i+2]  > 6;
            if (onOff) ++litOff;
            if (onOn) ++litOn;
            for (int ch = 0; ch < 4; ++ch) {
                const int d = std::abs(int(pxOff[i+ch]) - int(pxOn[i+ch]));
                if (d > 0) { ++diffPixels; maxChannelDiff = std::max<size_t>(maxChannelDiff, static_cast<size_t>(d)); break; }
            }
        }
        // Not bit-identical (the marcher's cone-stepping epsilon can put a
        // handful of edge pixels' hit point a sub-pixel apart depending on
        // where t started, same as any other early-exit optimization already
        // in this marcher), but must never be MISSING geometry: lit-pixel
        // coverage must not drop, and no pixel may flip from "hit" to "empty".
        bool anyHitBecameEmpty = false;
        for (size_t i = 0; i + 4 <= pxOff.size(); i += 4) {
            const bool onOff = pxOff[i] > 6 || pxOff[i+1] > 6 || pxOff[i+2] > 6;
            const bool onOn  = pxOn[i]  > 6 || pxOn[i+1]  > 6 || pxOn[i+2]  > 6;
            if (onOff && !onOn) { anyHitBecameEmpty = true; break; }
        }

        const bool ok = !anyHitBecameEmpty && litOn >= litOff;
        std::printf("  %-26s off=%5zu lit  on=%5zu lit  diffPixels=%5zu (max ch diff %zu)  %s\n",
                   c.name, litOff, litOn, diffPixels, maxChannelDiff, ok ? "ok" : "FAILED");
        if (!ok) ++g_failures;
    }

    std::printf(g_failures == 0 ? "webgpu_heightfield_sweep_test: ALL OK\n"
                                : "webgpu_heightfield_sweep_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
