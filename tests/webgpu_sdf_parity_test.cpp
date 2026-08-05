// GPU raymarch vs CPU evaluator, for every primitive and every operator.
//
// WHY THIS EXISTS: SdfWgsl.cpp's primitive library is a HAND TRANSCRIPTION of the
// formulas in Sdf.cpp. Nothing enforces that the two stay in step, so a slip in
// any one of them — a swapped select() operand, a dropped epsilon, a sign — would
// silently make a shape render differently from the shape the rest of the app
// believes in, and only for whoever happened to use that primitive. Until this
// test, only Sphere had ever been exercised.
//
// The check is a SILHOUETTE comparison: for each pixel, does the GPU report a
// surface where the CPU raycaster reports one? Comparing silhouettes rather than
// colours isolates the distance function itself from shading, and a transcription
// error changes the shape, which changes the silhouette.
//
// Pixels on the boundary legitimately disagree — the two marchers use different
// step sequences and land on opposite sides of a surface within an epsilon — so a
// small edge tolerance is allowed. It is deliberately far tighter than the area
// any real transcription error would move.

#include "Form/Object/Geometry/Sdf.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/RenderMaterial.hpp"
#include "Rendering/WebGPU/WebGpuRenderer.hpp"
#include "Rendering/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr uint32_t W = 32, H = 32;
constexpr float    kExtent = 1.4f;

struct MapR { bool done = false; };
void onMap(WGPUMapAsyncStatus, WGPUStringView, void* u, void*) {
    static_cast<MapR*>(u)->done = true;
}

std::shared_ptr<geom::SdfNode> leaf(geom::SdfPrim prim, glm::vec3 dims,
                                    float p0 = 0.0f, glm::vec3 off = glm::vec3(0.0f)) {
    auto n = std::make_shared<geom::SdfNode>();
    n->op = geom::SdfOp::Leaf; n->prim = prim; n->dims = dims; n->p0 = p0; n->offset = off;
    return n;
}

std::shared_ptr<geom::SdfNode> binary(geom::SdfOp op, std::shared_ptr<geom::SdfNode> a,
                                      std::shared_ptr<geom::SdfNode> b, float t = 0.5f) {
    auto n = std::make_shared<geom::SdfNode>();
    n->op = op; n->t = t;
    n->children = { std::move(a), std::move(b) };
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

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    // Model is identity throughout, so field space == world space and the two
    // marchers are unambiguously tracing the same ray.
    const glm::vec3 eye(0.0f, 0.0f, 3.0f);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    const glm::mat4 viewM = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    const glm::mat4 invVP = glm::inverse(proj * viewM);

    auto gpuMask = [&](const geom::SdfNode& field, const glm::mat4& model) {
        RenderMaterial mat;
        mat.baseColor = glm::vec3(1.0f);
        r.setCamera(viewM, proj, eye);
        r.setModel(model);
        r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        r.drawImplicit(field, kExtent, mat);
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
        std::vector<uint8_t> mask(W * H, 0);
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x) {
                const unsigned char* p = px + y * 256 + x * 4;
                // Cleared to black; any lit surface is non-black.
                mask[y * W + x] = (p[0] > 6 || p[1] > 6 || p[2] > 6) ? 1 : 0;
            }
        wgpuBufferUnmap(readback);
        return mask;
    };

    auto cpuMask = [&](const geom::SdfNode& field, const glm::mat4& model) {
        // The GPU marches in FIELD space, so the reference must too: an affine
        // transform maps the world ray to a field-space ray (lines stay lines),
        // and the SDF is only meaningful there.
        const glm::mat4 inv = glm::inverse(model);
        std::vector<uint8_t> mask(W * H, 0);
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                // Texture row 0 is the TOP of the image, which is ndc.y = +1.
                const float nx = (float(x) + 0.5f) / W * 2.0f - 1.0f;
                const float ny = 1.0f - (float(y) + 0.5f) / H * 2.0f;
                glm::vec4 nearP = invVP * glm::vec4(nx, ny, 0.0f, 1.0f);
                glm::vec4 farP  = invVP * glm::vec4(nx, ny, 1.0f, 1.0f);
                glm::vec3 ow = glm::vec3(nearP) / nearP.w;
                glm::vec3 fw = glm::vec3(farP) / farP.w;
                glm::vec3 o = glm::vec3(inv * glm::vec4(ow, 1.0f));
                glm::vec3 d = glm::normalize(glm::vec3(inv * glm::vec4(fw, 1.0f)) - o);

                float tHit = 0.0f; glm::vec3 nrm;
                bool hit = geom::raycastSdf(field, o, d, tHit, nrm);
                // The GPU only rasterises the bounding cube, so a CPU hit outside
                // that box could never be drawn. Match the domain.
                if (hit) {
                    glm::vec3 p = o + d * tHit;
                    if (std::abs(p.x) > kExtent * 1.05f || std::abs(p.y) > kExtent * 1.05f ||
                        std::abs(p.z) > kExtent * 1.05f) hit = false;
                }
                mask[y * W + x] = hit ? 1 : 0;
            }
        }
        return mask;
    };

    struct Case {
        const char* name;
        std::shared_ptr<geom::SdfNode> node;
        glm::mat4 model{1.0f};
    };
    std::vector<Case> cases;

    // Every primitive.
    cases.push_back({ "Sphere",    leaf(geom::SdfPrim::Sphere,    glm::vec3(0.7f)) });
    cases.push_back({ "Box",       leaf(geom::SdfPrim::Box,       glm::vec3(0.5f, 0.6f, 0.4f)) });
    cases.push_back({ "RoundBox",  leaf(geom::SdfPrim::RoundBox,  glm::vec3(0.6f, 0.5f, 0.5f), 0.15f) });
    cases.push_back({ "Ellipsoid", leaf(geom::SdfPrim::Ellipsoid, glm::vec3(0.8f, 0.5f, 0.6f)) });
    cases.push_back({ "Cylinder",  leaf(geom::SdfPrim::Cylinder,  glm::vec3(0.5f, 0.6f, 0.0f)) });
    cases.push_back({ "Cone",      leaf(geom::SdfPrim::Cone,      glm::vec3(0.6f, 0.6f, 0.0f)) });
    cases.push_back({ "Torus",     leaf(geom::SdfPrim::Torus,     glm::vec3(0.55f, 0.2f, 0.0f)) });

    // Leaf placement, which every primitive routes through.
    cases.push_back({ "Sphere@offset",
                      leaf(geom::SdfPrim::Sphere, glm::vec3(0.45f), 0.0f, glm::vec3(0.3f, -0.2f, 0.1f)) });

    // Convex: a tetrahedron's four half-spaces.
    {
        auto n = leaf(geom::SdfPrim::Convex, glm::vec3(0.0f));
        const glm::vec3 ns[4] = {{ 0.577f, 0.577f, 0.577f}, {-0.577f,-0.577f, 0.577f},
                                 {-0.577f, 0.577f,-0.577f}, { 0.577f,-0.577f,-0.577f}};
        for (const glm::vec3& v : ns) n->planes.push_back(glm::vec4(v, 0.45f));
        cases.push_back({ "Convex(tetra)", n });
    }

    // The RPN path, unwound at codegen time. Two forms deliberately:
    //   distance form — a true distance field, traced without damping;
    //   iso form      — NOT a distance (value >> distance), which is what forces
    //                   both marchers to damp their steps. This case is exactly
    //                   what exposed the un-damped CPU raycaster.
    cases.push_back({ "Expr(distance)",
        std::make_shared<geom::SdfNode>(geom::makeImplicit("sqrt(x*x + y*y + z*z) - 0.55")) });
    cases.push_back({ "Expr(iso)",
        std::make_shared<geom::SdfNode>(geom::makeImplicit("x*x + y*y + z*z - 0.3")) });

    // Every operator.
    auto a = leaf(geom::SdfPrim::Sphere, glm::vec3(0.6f), 0.0f, glm::vec3(-0.25f, 0, 0));
    auto b = leaf(geom::SdfPrim::Box,    glm::vec3(0.45f), 0.0f, glm::vec3( 0.25f, 0, 0));
    cases.push_back({ "Union",       binary(geom::SdfOp::Union,       a, b) });
    cases.push_back({ "Intersect",   binary(geom::SdfOp::Intersect,   a, b) });
    cases.push_back({ "Subtract",    binary(geom::SdfOp::Subtract,    a, b) });
    cases.push_back({ "Morph",       binary(geom::SdfOp::Morph,       a, b, 0.35f) });
    cases.push_back({ "SmoothUnion", binary(geom::SdfOp::SmoothUnion, a, b, 0.3f) });

    // Nesting, to prove the emitter composes rather than only handling depth 1.
    cases.push_back({ "Nested",
        binary(geom::SdfOp::Subtract,
               binary(geom::SdfOp::Union, a, b),
               leaf(geom::SdfPrim::Sphere, glm::vec3(0.35f), 0.0f, glm::vec3(0, 0.4f, 0.4f))) });

    // Under a TRANSFORM. Every case above uses an identity model, which never
    // exercises the marcher's invModel path — and that path is where a field would
    // silently render rotated, offset, or the wrong size. Non-uniform scale is
    // included deliberately: marching in field space is correct for any invertible
    // affine transform, and this proves it rather than assuming it.
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, -0.15f, 0.1f));
        m = glm::rotate(m, glm::radians(35.0f), glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f)));
        m = glm::scale(m, glm::vec3(1.3f, 0.7f, 1.0f)); // deliberately non-uniform
        cases.push_back({ "Box@xform",
                          leaf(geom::SdfPrim::Box, glm::vec3(0.5f, 0.55f, 0.4f)), m });
        cases.push_back({ "Torus@xform",
                          leaf(geom::SdfPrim::Torus, glm::vec3(0.55f, 0.2f, 0.0f)), m });
        cases.push_back({ "SmoothUnion@xform",
                          binary(geom::SdfOp::SmoothUnion, a, b, 0.3f), m });
    }

    int failures = 0;
    for (const Case& c : cases) {
        const std::vector<uint8_t> g = gpuMask(*c.node, c.model);
        const std::vector<uint8_t> p = cpuMask(*c.node, c.model);

        size_t gpuOn = 0, cpuOn = 0, diff = 0;
        for (size_t i = 0; i < g.size(); ++i) {
            gpuOn += g[i]; cpuOn += p[i];
            if (g[i] != p[i]) ++diff;
        }

        // Scale the tolerance with the silhouette's perimeter: disagreement is a
        // boundary phenomenon, so it grows with the edge, not the area. ~sqrt(area)
        // approximates the perimeter; 2.5x leaves room for two marchers landing on
        // opposite sides of an epsilon without admitting a real shape difference.
        const double perimeter = std::sqrt(double(cpuOn ? cpuOn : 1)) * 4.0;
        const size_t tolerance = size_t(perimeter * 2.5) + 4;

        const bool ok = (cpuOn > 0) && (gpuOn > 0) && (diff <= tolerance);
        std::printf("  %-14s gpu=%4zu cpu=%4zu diff=%3zu (tol %3zu) %s\n",
                    c.name, gpuOn, cpuOn, diff, tolerance, ok ? "ok" : "MISMATCH");
        if (!ok) ++failures;
    }

    setCurrentRenderer(nullptr);
    r.shutdown();

    if (failures) {
        std::printf("webgpu_sdf_parity_test: %d MISMATCH(es) — the WGSL transcription in "
                    "SdfWgsl.cpp disagrees with Sdf.cpp\n", failures);
        std::fflush(stdout);
        std::_Exit(1);
    }
    std::printf("webgpu_sdf_parity_test: ALL OK (%zu shapes agree with the CPU)\n", cases.size());
    // See webgpu_object_test for why _Exit: an unrelated static-destruction abort
    // when a wgpu device shares a process with this app's globals.
    std::fflush(stdout);
    std::_Exit(0);
}
