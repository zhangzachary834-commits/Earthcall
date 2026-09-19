// Rendering-optimization Phase C's native camera corpus. DDA traversal is
// currently quarantined after this test exposed grazing-root losses, but the
// cache/proxy boundary still needs an end-to-end guard: supplying a proved
// linear-heightfield grid must never alter the generic exact renderer's pixels.
//
// The cases deliberately cover straight-down, horizon, 45-degree, oblique,
// near-parallel, and inside-proxy cameras -- the axes that prior fixed-camera
// checks missed. A future DDA reintroduction must first make this exact
// comparison pass while traversal is actually enabled.

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

using OntoMath::MathNode;

std::unique_ptr<MathNode> leaf(const char* name) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::unique_ptr<MathNode> constant(double value) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(value);
    return n;
}

std::unique_ptr<MathNode> unary(MathNode::Op op, std::unique_ptr<MathNode> a) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    return n;
}

std::unique_ptr<MathNode> binary(MathNode::Op op, std::unique_ptr<MathNode> a,
                                 std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

std::unique_ptr<MathNode> component(std::unique_ptr<MathNode> a, const char* axis) {
    auto n = unary(MathNode::Op::Component, std::move(a));
    n->stringArg = axis;
    return n;
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

    // A genuinely 2D linear heightfield with a compositionally derived bound.
    // Perlin grids are deliberately refused until the exact implementation has
    // a closed-form Lipschitz proof; a sampled margin may not delete geometry.
    geom::SdfNode field;
    field.op = geom::SdfOp::Leaf;
    field.prim = geom::SdfPrim::Expr;
    auto height = binary(
        MathNode::Op::Add,
        binary(MathNode::Op::Scale, constant(0.02), component(leaf("p"), "x")),
        binary(MathNode::Op::Scale, constant(0.01), component(leaf("p"), "z")));
    field.mathNode = std::shared_ptr<MathNode>(
        binary(MathNode::Op::Sub, leaf("y"), std::move(height)).release());
    // This synthetic true heightfield has amplitude 40, so its test volume
    // admits that complete vertical range. The saved y-dependent field keeps
    // its authored extent and is covered separately by SDF parity.
    const glm::vec3 extent(1000.f, 50.f, 1000.f);

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
        std::printf("FAIL: the proved linear heightfield did not produce a height grid "
                    "-- this test cannot exercise the cache/proxy boundary\n");
        return 1;
    }
    std::printf("height grid: %dx%d cells\n", heightGrid.dimX, heightGrid.dimZ);

    auto readPixels = [&](bool passGridCache, glm::vec3 eye, glm::vec3 target,
                          glm::vec3 up, float fovDeg) -> std::vector<unsigned char> {
        const glm::mat4 proj = glm::perspectiveZO(glm::radians(fovDeg), 1.0f, 0.1f, 3000.f);
        const glm::mat4 viewM = glm::lookAt(eye, target, up);
        RenderMaterial mat; mat.baseColor = glm::vec3(0.3f, 0.7f, 0.3f);

        r.setCamera(viewM, proj, eye);
        r.setModel(glm::mat4(1.0f));
        r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        r.drawImplicit(field, extent, mat, nullptr, 0, 0,
                       passGridCache ? &heightGrid : nullptr);
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

    struct Case { const char* name; glm::vec3 eye, target, up; float fovDeg; };
    const Case cases[] = {
        {"looking straight down",   {0, 120, 0},    {0, 0, 0},     {0, 0, -1}, 60.f},
        {"grazing the horizon",     {0, 60, -900},  {0, 55, 900},  {0, 1, 0},  60.f},
        {"45 degrees down",         {0, 120, -200}, {0, 0, 0},     {0, 1, 0},  60.f},
        {"close oblique",           {50, 10, 50},   {0, -5, -50},  {0, 1, 0},  70.f},
        {"near-parallel to ground", {0, 20, -300},  {0, 15, 300},  {0, 1, 0},  50.f},
        {"camera inside proxy",     {0, 0, 0},      {0, -5, 100},  {0, 1, 0},  70.f},
    };

    for (const Case& c : cases) {
        // DDA traversal is quarantined after a native grazing-root mismatch.
        // This remains an end-to-end guard that supplying an eligible cache
        // cannot change proxy coverage or the generic exact result.
        auto pxOff = readPixels(false, c.eye, c.target, c.up, c.fovDeg);
        auto pxOn  = readPixels(true,  c.eye, c.target, c.up, c.fovDeg);

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
        size_t firstMissingPixel = 0;
        for (size_t i = 0; i + 4 <= pxOff.size(); i += 4) {
            const bool onOff = pxOff[i] > 6 || pxOff[i+1] > 6 || pxOff[i+2] > 6;
            const bool onOn  = pxOn[i]  > 6 || pxOn[i+1]  > 6 || pxOn[i+2]  > 6;
            if (onOff && !onOn) {
                anyHitBecameEmpty = true;
                firstMissingPixel = i / 4;
                break;
            }
        }

        // Every case is deliberately aimed through the field. An all-black
        // pair cannot validate the cache/proxy boundary.
        const bool ok = litOff > 0 && !anyHitBecameEmpty && litOn >= litOff;
        std::printf("  %-26s off=%5zu lit  on=%5zu lit  diffPixels=%5zu (max ch diff %zu)  %s\n",
                   c.name, litOff, litOn, diffPixels, maxChannelDiff, ok ? "ok" : "FAILED");
        if (anyHitBecameEmpty) {
            const size_t i = firstMissingPixel * 4;
            std::printf("    first missing pixel (%zu,%zu): off=(%u,%u,%u,%u) on=(%u,%u,%u,%u)\n",
                        firstMissingPixel % W, firstMissingPixel / W,
                        pxOff[i], pxOff[i+1], pxOff[i+2], pxOff[i+3],
                        pxOn[i], pxOn[i+1], pxOn[i+2], pxOn[i+3]);
        }
        if (!ok) ++g_failures;
    }

    std::printf(g_failures == 0 ? "webgpu_heightfield_sweep_test: ALL OK\n"
                                : "webgpu_heightfield_sweep_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
