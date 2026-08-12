// On-screen WebGPU demo (Milestone 5 — the piece that needs a real display).
// Opens a GLFW window with a CAMetalLayer surface and renders a spinning, lit cube
// through the real WebGpuRenderer + swapchain. Run it: you should see a window with
// a rotating shaded cube on a dark blue background. Close the window to exit.
//
//   make webgpu-window   (then ./webgpu_window)
//
// This proves the surface/swapchain/present path on your hardware. Once confirmed,
// the same surface code drops into the app's Engine to make WebGPU the live backend.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "WebGpuRenderer.hpp"
#include "WgpuDevice.hpp"
#include "ConstructedBeing/Object/Geometry/SmoothSurface.hpp"
#include "Rendering/RenderMaterial.hpp"

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

// --- async adapter/device request with a surface-compatible adapter ------------
struct AdapterR { WGPUAdapter a = nullptr; bool done = false; };
static void onAdapter(WGPURequestAdapterStatus s, WGPUAdapter a, WGPUStringView, void* u, void*) {
    auto* r = (AdapterR*)u; if (s == WGPURequestAdapterStatus_Success) r->a = a; r->done = true;
}
struct DeviceR { WGPUDevice d = nullptr; bool done = false; };
static void onDevice(WGPURequestDeviceStatus s, WGPUDevice d, WGPUStringView, void* u, void*) {
    auto* r = (DeviceR*)u; if (s == WGPURequestDeviceStatus_Success) r->d = d; r->done = true;
}

// A unit cube: 6 faces, outward normals, 2 triangles each.
static geom::TessMesh makeCube() {
    struct Face { glm::vec3 n, a, b, c, d; };
    const float h = 0.5f;
    const Face faces[6] = {
        {{ 1,0,0}, { h,-h,-h},{ h, h,-h},{ h, h, h},{ h,-h, h}},
        {{-1,0,0}, {-h,-h,-h},{-h,-h, h},{-h, h, h},{-h, h,-h}},
        {{0, 1,0}, {-h, h,-h},{-h, h, h},{ h, h, h},{ h, h,-h}},
        {{0,-1,0}, {-h,-h,-h},{ h,-h,-h},{ h,-h, h},{-h,-h, h}},
        {{0,0, 1}, {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}},
        {{0,0,-1}, {-h,-h,-h},{-h, h,-h},{ h, h,-h},{ h,-h,-h}},
    };
    auto vert = [](glm::vec3 p, glm::vec3 n) {
        geom::TessVertex v; v.pos = p; v.normal = n; v.uv = glm::vec2(0.5f); return v;
    };
    geom::TessMesh m;
    for (const Face& f : faces) {
        m.tris.push_back(vert(f.a, f.n)); m.tris.push_back(vert(f.b, f.n)); m.tris.push_back(vert(f.c, f.n));
        m.tris.push_back(vert(f.a, f.n)); m.tris.push_back(vert(f.c, f.n)); m.tris.push_back(vert(f.d, f.n));
    }
    return m;
}

int main() {
    if (!glfwInit()) { printf("FAIL: glfwInit\n"); return 1; }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // WebGPU manages presentation, no GL context
    GLFWwindow* win = glfwCreateWindow(800, 600, "Earthcall — WebGPU", nullptr, nullptr);
    if (!win) { printf("FAIL: window\n"); glfwTerminate(); return 1; }

    // Back the window's view with a CAMetalLayer.
    NSWindow* nswin = glfwGetCocoaWindow(win);
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.contentsScale = nswin.backingScaleFactor;
    nswin.contentView.wantsLayer = YES;
    nswin.contentView.layer = layer;

    WGPUInstance instance = wgpuCreateInstance(nullptr);
    WGPUSurfaceSourceMetalLayer metalSrc = {};
    metalSrc.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    metalSrc.layer = (void*)layer;
    WGPUSurfaceDescriptor sdesc = {};
    sdesc.nextInChain = &metalSrc.chain;
    WGPUSurface surface = wgpuInstanceCreateSurface(instance, &sdesc);

    AdapterR ar; WGPURequestAdapterOptions opts = {}; opts.compatibleSurface = surface;
    WGPURequestAdapterCallbackInfo acb = {};
    acb.mode = WGPUCallbackMode_AllowProcessEvents; acb.callback = onAdapter; acb.userdata1 = &ar;
    wgpuInstanceRequestAdapter(instance, &opts, acb);
    while (!ar.done) wgpuInstanceProcessEvents(instance);
    if (!ar.a) { printf("FAIL: adapter\n"); return 1; }

    DeviceR dr;
    WGPURequestDeviceCallbackInfo dcb = {};
    dcb.mode = WGPUCallbackMode_AllowProcessEvents; dcb.callback = onDevice; dcb.userdata1 = &dr;
    wgpuAdapterRequestDevice(ar.a, nullptr, dcb);
    while (!dr.done) wgpuInstanceProcessEvents(instance);
    if (!dr.d) { printf("FAIL: device\n"); return 1; }

    wgpu::Device gpu;
    gpu.instance = instance; gpu.adapter = ar.a; gpu.device = dr.d;
    gpu.queue = wgpuDeviceGetQueue(dr.d);

    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
    layer.drawableSize = CGSizeMake(fbw, fbh);
    WGPUSurfaceConfiguration cfg = {};
    cfg.device = gpu.device;
    cfg.format = WGPUTextureFormat_BGRA8Unorm;
    cfg.usage = WGPUTextureUsage_RenderAttachment;
    cfg.width = (uint32_t)fbw; cfg.height = (uint32_t)fbh;
    cfg.presentMode = WGPUPresentMode_Fifo;
    cfg.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface, &cfg);

    WebGpuRenderer r;
    if (!r.init(gpu, WGPUTextureFormat_BGRA8Unorm)) { printf("FAIL: renderer init\n"); return 1; }

    geom::TessMesh cube = makeCube();
    RenderMaterial mat; mat.baseColor = glm::vec3(0.85f, 0.35f, 0.2f); mat.specular = 0.6f;

    glm::vec3 eye(2.2f, 1.6f, 2.6f);
    printf("WebGPU window open — spinning cube. Close the window to exit.\n");
    float t = 0.0f;
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        t += 0.016f;

        int nw, nh; glfwGetFramebufferSize(win, &nw, &nh);
        if (nw != fbw || nh != fbh) {
            fbw = nw; fbh = nh;
            layer.drawableSize = CGSizeMake(fbw, fbh);
            cfg.width = (uint32_t)fbw; cfg.height = (uint32_t)fbh;
            wgpuSurfaceConfigure(surface, &cfg);
        }
        if (fbw == 0 || fbh == 0) continue;

        WGPUSurfaceTexture st = {};
        wgpuSurfaceGetCurrentTexture(surface, &st);
        if (st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) continue;
        WGPUTextureView target = wgpuTextureCreateView(st.texture, nullptr);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(fbw) / fbh, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
        r.setCamera(proj * view, eye);
        r.setModel(glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f))));
        r.beginFrameOffscreen(target, (uint32_t)fbw, (uint32_t)fbh, glm::vec4(0.08f, 0.09f, 0.13f, 1.0f));
        r.drawMesh(cube, mat);
        r.endFrame();
        wgpuSurfacePresent(surface);
        wgpuTextureViewRelease(target);
    }

    r.shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    printf("window closed — clean exit\n");
    return 0;
}
