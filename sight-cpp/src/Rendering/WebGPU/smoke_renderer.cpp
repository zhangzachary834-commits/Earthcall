// WebGpuRenderer verification (Milestone 5). Drives the real renderer class the
// way the app will: setCamera → beginFrame → setModel → drawMesh → endFrame, then
// reads the offscreen result back. Three things proven together:
//   1. Perspective camera + lighting: colour = (ambient + diffuse·N·L)·baseColor.
//   2. DEPTH: a near quad drawn BEFORE a far quad still wins at the centre.
//   3. Texture albedo: a green albedo tints a white-baseColor surface green.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // WebGPU clip depth is [0,1], not GL's [-1,1]
#include "WebGpuRenderer.hpp"
#include "WgpuDevice.hpp"
#include "Form/Object/Geometry/SmoothSurface.hpp" // geom::TessMesh / TessVertex
#include "Rendering/RenderMaterial.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

static const uint32_t W = 8, H = 8;

struct MapR { bool ok = false, done = false; };
static void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u); r->ok = (s == WGPUMapAsyncStatus_Success); r->done = true;
}

static geom::TessVertex v(glm::vec3 p, glm::vec3 n) {
    geom::TessVertex tv; tv.pos = p; tv.normal = n; tv.uv = glm::vec2(0.5f); return tv;
}
static geom::TessMesh quadNorm(float z, glm::vec3 n) {
    glm::vec3 a(-0.8f,-0.8f,z), b(0.8f,-0.8f,z), c(0.8f,0.8f,z), d(-0.8f,0.8f,z);
    geom::TessMesh m; m.tris = { v(a,n), v(b,n), v(c,n), v(a,n), v(c,n), v(d,n) };
    return m;
}
static geom::TessMesh quadAt(float z) { return quadNorm(z, glm::vec3(0, 0, 1)); }

int main() {
    wgpu::Device gpu;
    if (!gpu.init()) { printf("FAIL: device\n"); return 1; }
    WebGpuRenderer r;
    if (!r.init(gpu)) { printf("FAIL: renderer init\n"); return 1; }

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D; td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm; td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);

    WGPUBufferDescriptor rbDesc = {};
    rbDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead; rbDesc.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbDesc);

    // Copy the target to the readback buffer and return the centre pixel (4 bytes).
    auto readCenter = [&](unsigned char out[4]) {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = tex; src.aspect = WGPUTextureAspect_All; src.origin = {0,0,0};
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
        WGPUExtent3D cs = { W, H, 1 };
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &cs);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        MapR mr; WGPUBufferMapCallbackInfo mcb = {};
        mcb.mode = WGPUCallbackMode_AllowProcessEvents; mcb.callback = onMap; mcb.userdata1 = &mr;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, mcb);
        while (!mr.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const unsigned char* px = (const unsigned char*)wgpuBufferGetConstMappedRange(readback, 0, 256 * H);
        const unsigned char* c4 = px + (4 * 256) + (4 * 4);
        out[0]=c4[0]; out[1]=c4[1]; out[2]=c4[2]; out[3]=c4[3];
        wgpuBufferUnmap(readback);
    };

    glm::vec3 eye(0, 0, 3);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(W) / H, 0.1f, 10.0f);
    glm::mat4 vw = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    r.setCamera(proj * vw, eye);
    r.setModel(glm::mat4(1.0f));

    // --- Scene 1: perspective + depth (near BLUE drawn before far RED) ----------
    // specular 0 keeps these two deterministic (no white highlight added).
    RenderMaterial blue; blue.baseColor = glm::vec3(0, 0, 1); blue.specular = 0;
    RenderMaterial red;  red.baseColor  = glm::vec3(1, 0, 0); red.specular = 0;
    r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    r.drawMesh(quadAt(0.5f), blue);  // near, first
    r.drawMesh(quadAt(-0.5f), red);  // far, second — depth must reject it
    r.endFrame();
    unsigned char p1[4]; readCenter(p1);
    printf("scene 1 (depth) centre = (%d,%d,%d,%d)\n", p1[0],p1[1],p1[2],p1[3]);
    bool depthOK = p1[2] > 80 && p1[0] < 20;

    // --- Scene 2: texture albedo (white material + green albedo -> green) -------
    std::vector<unsigned char> green(4 * 4 * 4);           // 4×4 RGBA8, all green
    for (size_t i = 0; i < green.size(); i += 4) { green[i]=0; green[i+1]=255; green[i+2]=0; green[i+3]=255; }
    RenderMaterial painted; painted.baseColor = glm::vec3(1, 1, 1); painted.specular = 0;
    painted.albedoPixels = green.data(); painted.albedoSize = 4;
    r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    r.drawMesh(quadAt(0.0f), painted);
    r.endFrame();
    unsigned char p2[4]; readCenter(p2);
    printf("scene 2 (albedo) centre = (%d,%d,%d,%d)\n", p2[0],p2[1],p2[2],p2[3]);
    bool texOK = p2[1] > 80 && p2[0] < 20 && p2[2] < 20; // green present, r/b absent

    // --- Scene 3: Blinn-Phong specular ------------------------------------------
    // A RED quad whose normals point along the half-vector H (so N·H = 1) gets a
    // full white specular highlight — the green/blue channels jump from 0, which
    // baseColor red alone can never produce. That is the specular term firing.
    glm::vec3 halfV = glm::normalize(glm::normalize(glm::vec3(2, 5, 2)) + glm::vec3(0, 0, 1));
    RenderMaterial shiny; shiny.baseColor = glm::vec3(1, 0, 0);
    shiny.specular = 0.7f; shiny.shininess = 32.0f;
    r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    r.drawMesh(quadNorm(0.0f, halfV), shiny);
    r.endFrame();
    unsigned char p3[4]; readCenter(p3);
    printf("scene 3 (specular) centre = (%d,%d,%d,%d)\n", p3[0],p3[1],p3[2],p3[3]);
    bool specOK = p3[1] > 80 && p3[2] > 80; // white highlight added to g and b

    // Count non-black pixels in the current target (for the wireframe check).
    auto countNonBlack = [&]() -> int {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {}; src.texture = tex; src.aspect = WGPUTextureAspect_All; src.origin = {0,0,0};
        WGPUTexelCopyBufferInfo dst = {}; dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
        WGPUExtent3D cs = { W, H, 1 };
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &cs);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr); wgpuQueueSubmit(gpu.queue, 1, &cmd);
        MapR mr; WGPUBufferMapCallbackInfo mcb = {};
        mcb.mode = WGPUCallbackMode_AllowProcessEvents; mcb.callback = onMap; mcb.userdata1 = &mr;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, mcb);
        while (!mr.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const unsigned char* px = (const unsigned char*)wgpuBufferGetConstMappedRange(readback, 0, 256 * H);
        int n = 0;
        for (uint32_t row = 0; row < H; ++row)
            for (uint32_t col = 0; col < W; ++col) {
                const unsigned char* p = px + row * 256 + col * 4;
                if (p[0] > 20 || p[1] > 20 || p[2] > 20) ++n;
            }
        wgpuBufferUnmap(readback);
        return n;
    };

    // --- Scene 4: drawOverlay (additive green glow shell) -----------------------
    r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    r.drawOverlay(quadAt(0.0f), glm::vec4(0, 1, 0, 0.5f), 1.0f, /*additive=*/true);
    r.endFrame();
    unsigned char p4[4]; readCenter(p4);
    printf("scene 4 (overlay) centre = (%d,%d,%d,%d)\n", p4[0],p4[1],p4[2],p4[3]);
    bool overlayOK = p4[1] > 80 && p4[0] < 20; // additive green reached the frame

    // --- Scene 5: drawLines (wireframe box) -------------------------------------
    std::vector<std::pair<glm::vec3, glm::vec3>> box = {
        {{-0.6f,-0.6f,0}, { 0.6f,-0.6f,0}}, {{ 0.6f,-0.6f,0}, { 0.6f, 0.6f,0}},
        {{ 0.6f, 0.6f,0}, {-0.6f, 0.6f,0}}, {{-0.6f, 0.6f,0}, {-0.6f,-0.6f,0}},
    };
    r.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    r.drawLines(box, glm::vec4(1, 1, 0, 1), 2.0f, Blend::Alpha);
    r.endFrame();
    int linePixels = countNonBlack();
    printf("scene 5 (lines) drew %d non-black pixels\n", linePixels);
    bool linesOK = linePixels > 0;

    bool ok = depthOK && texOK && specOK && overlayOK && linesOK;
    printf(ok ? "OK: mesh + depth + albedo + specular + overlay + lines through WebGpuRenderer\n"
              : "FAIL: depth=%d tex=%d spec=%d overlay=%d lines=%d\n",
              depthOK, texOK, specOK, overlayOK, linesOK);
    return ok ? 0 : 1;
}
