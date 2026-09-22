// An analytic shape must still be there when you step back from it.
//
// WHY THIS EXISTS: Zach loaded chess_app and found "the chess pieces are not
// rendering except the 4 rooks, except when I am very close to them. They still
// seem to move and respond to clicks fine" (Bugs.md #20). The rooks are the only
// pieces that are ShapeKind::Cube, which draws as a mesh; every other piece is a
// Sphere, Cone, Cylinder, Ellipsoid or Ovoid, which the WebGPU backend
// raymarches. So the whole SDF path was disappearing at a few units' range while
// the mesh path was fine -- and picking still worked, because picking is CPU
// geometry and never asks the shader anything.
//
// The cause was a march budget compared against the wrong origin. `misc.z` is a
// LENGTH (`maxDim * 8`), and the marcher bounded itself with
// `maxDist = min(box.y, misc.z)` while `t` is measured from the EYE. For a chess
// piece `maxDim` is about 0.6, so the budget is ~4.8 -- and any piece further
// than that from the camera entered the loop with `t > maxDist` already true,
// broke on the first iteration, and discarded. Bigger things were unaffected
// (the noise floor's budget is 8000), which is exactly why this survived: every
// shape anyone tested a raymarcher with is a shape at arm's length.
//
// No existing test could see it. webgpu_sdf_parity_test renders every case from
// one fixed camera 3 units away, inside any plausible budget. Distance was a
// free variable nothing varied, so this file varies it.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cmath>
#include <cstdio>

namespace {

constexpr uint32_t W = 64, H = 64;   // 64 * 4 bytes = the 256-byte row stride exactly

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

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    // A chess pawn's shape and scale: a small sphere, drawn the way
    // Object::drawSmoothModel draws one.
    auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.35f));
    const glm::vec3 extent(0.6f);

    // Count lit pixels with the camera pulled back along +z, framing held constant
    // by narrowing the field of view as distance grows — so the shape occupies
    // roughly the same screen area at every distance and coverage is comparable.
    auto coverageAt = [&](float dist) {
        const float halfAngle = std::atan2(extent.x * 1.2f, dist);
        const glm::mat4 proj = glm::perspectiveZO(2.0f * halfAngle, 1.0f,
                                                  std::max(0.01f, dist * 0.5f), dist * 4.0f);
        const glm::vec3 eye(0.0f, 0.0f, dist);
        const glm::mat4 viewM = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

        RenderMaterial mat;
        mat.baseColor = glm::vec3(1.0f);
        r.setCamera(viewM, proj, eye);
        r.setModel(glm::mat4(1.0f));
        r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        r.drawImplicit(sphere, extent, mat);
        r.endFrame();

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = tex; src.aspect = WGPUTextureAspect_All; src.origin = {0,0,0};
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
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
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, ci);
        while (!m.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const auto* px = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, 256 * H));
        size_t lit = 0;
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x) {
                const unsigned char* p = px + y * 256 + x * 4;
                if (p[0] > 6 || p[1] > 6 || p[2] > 6) ++lit;
            }
        wgpuBufferUnmap(readback);
        return lit;
    };

    // 2.0 is inside any plausible budget and establishes what "visible" means.
    // maxDim * 8 for this shape is 4.8, so 8 and beyond is where the defect lived.
    const float distances[] = { 2.0f, 4.0f, 8.0f, 20.0f, 60.0f, 200.0f };
    const size_t near = coverageAt(distances[0]);

    std::printf("=== an analytic shape stays visible as the camera pulls back ===\n");
    std::printf("  (sphere r=0.35, extent 0.6 — a chess pawn; maxDim*8 = %.1f)\n", 0.6f * 8.0f);
    std::printf("  %8.1f units: %4zu lit pixels  (reference)\n", distances[0], near);

    if (near < 100) {
        std::printf("  FAILED: the shape is not visible even up close — %zu pixels\n", near);
        ++g_failures;
    }

    for (size_t i = 1; i < sizeof(distances) / sizeof(distances[0]); ++i) {
        const size_t lit = coverageAt(distances[i]);
        // Framing is held constant, so coverage should barely move. Half the
        // near-field count is a generous floor that still catches "gone".
        //
        // It drifts UP at the far distances (about 2.3x at 200 units) and that
        // is this test's own doing, not a defect. The marcher relaxes its hit
        // threshold with distance -- `current_eps = max(eps, t * 0.001)` -- so
        // the surface reads as slightly fatter the further off it is. That
        // constant is about one pixel of footprint for a 60-degree field of view
        // at 1080p, which is what it is for; this test deliberately narrows the
        // field of view to keep the shape framed, so the same epsilon covers far
        // more of the screen than it ever would in the app. Asserting a ceiling
        // here would be asserting the test's framing, not the renderer's.
        const bool ok = lit >= near / 2;
        std::printf("  %8.1f units: %4zu lit pixels  %s\n", distances[i], lit,
                    ok ? "ok" : "FAILED — the shape vanished");
        if (!ok) ++g_failures;
    }

    std::printf(g_failures == 0 ? "webgpu_sdf_distance_test: ALL OK\n"
                                : "webgpu_sdf_distance_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
