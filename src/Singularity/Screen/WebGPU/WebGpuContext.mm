// Objective-C++ half of the window-attached WebGPU bring-up. See the header for
// why this is a separate translation unit. The sequence is the one proven on
// hardware by smoke_window.mm — do not reorder it casually: the adapter must be
// requested with the surface as `compatibleSurface`, or it may come back unable
// to present to that surface.

#include "Singularity/Screen/WebGPU/WebGpuContext.hpp"

#include <webgpu/wgpu.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <cstdio>

namespace wgpu {
namespace {

struct AdapterResult { WGPUAdapter adapter = nullptr; bool done = false; };
void onAdapter(WGPURequestAdapterStatus status, WGPUAdapter a, WGPUStringView, void* ud, void*) {
    auto* r = static_cast<AdapterResult*>(ud);
    if (status == WGPURequestAdapterStatus_Success) r->adapter = a;
    r->done = true;
}

struct DeviceResult { WGPUDevice device = nullptr; bool done = false; };
void onDevice(WGPURequestDeviceStatus status, WGPUDevice d, WGPUStringView, void* ud, void*) {
    auto* r = static_cast<DeviceResult*>(ud);
    if (status == WGPURequestDeviceStatus_Success) r->device = d;
    r->done = true;
}

} // namespace

WindowContext createWindowContext(GLFWwindow* win) {
    WindowContext ctx;
    if (!win) return ctx;

    // Back the window's content view with a CAMetalLayer — the thing wgpu-native's
    // Metal backend can actually present into.
    NSWindow* nswin = glfwGetCocoaWindow(win);
    if (!nswin) { std::fprintf(stderr, "[WebGPU] no Cocoa window\n"); return ctx; }
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;   // must match kSurfaceFormat
    layer.contentsScale = nswin.backingScaleFactor; // Retina: drawable in pixels
    if (@available(macOS 10.13, *)) {
        layer.displaySyncEnabled = NO;
    }
    nswin.contentView.wantsLayer = YES;
    nswin.contentView.layer = layer;
    ctx.metalLayer = (void*)layer;

    ctx.instance = wgpuCreateInstance(nullptr);
    if (!ctx.instance) { std::fprintf(stderr, "[WebGPU] instance failed\n"); return ctx; }

    WGPUSurfaceSourceMetalLayer metalSrc = {};
    metalSrc.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    metalSrc.layer = (void*)layer;
    WGPUSurfaceDescriptor sd = {};
    sd.nextInChain = &metalSrc.chain;
    ctx.surface = wgpuInstanceCreateSurface(ctx.instance, &sd);
    if (!ctx.surface) { std::fprintf(stderr, "[WebGPU] surface failed\n"); return ctx; }

    // Adapter must be told which surface it has to present to.
    AdapterResult ar;
    WGPURequestAdapterOptions opts = {};
    opts.compatibleSurface = ctx.surface;
    WGPURequestAdapterCallbackInfo acb = {};
    acb.mode = WGPUCallbackMode_AllowProcessEvents;
    acb.callback = onAdapter;
    acb.userdata1 = &ar;
    wgpuInstanceRequestAdapter(ctx.instance, &opts, acb);
    while (!ar.done) wgpuInstanceProcessEvents(ctx.instance);
    if (!ar.adapter) { std::fprintf(stderr, "[WebGPU] no compatible adapter\n"); return ctx; }
    ctx.adapter = ar.adapter;

    // GPU timestamps are optional instrumentation. Request the native encoder
    // write capability only when the presenting adapter advertises both pieces;
    // a rejection falls back to the ordinary visual device below.
    WGPUDeviceDescriptor dd = {};
    const WGPUFeatureName timestampFeatures[] = {
        WGPUFeatureName_TimestampQuery,
        static_cast<WGPUFeatureName>(WGPUNativeFeature_TimestampQueryInsideEncoders),
    };
    const bool canTimestamp =
        wgpuAdapterHasFeature(ctx.adapter, WGPUFeatureName_TimestampQuery) &&
        wgpuAdapterHasFeature(
            ctx.adapter,
            static_cast<WGPUFeatureName>(WGPUNativeFeature_TimestampQueryInsideEncoders));
    if (canTimestamp) {
        dd.requiredFeatureCount = 2;
        dd.requiredFeatures = timestampFeatures;
    }

    const bool requestedTimestampQueries = dd.requiredFeatureCount != 0;
    DeviceResult dr;
    WGPURequestDeviceCallbackInfo dcb = {};
    dcb.mode = WGPUCallbackMode_AllowProcessEvents;
    dcb.callback = onDevice;
    dcb.userdata1 = &dr;
    wgpuAdapterRequestDevice(ctx.adapter, dd.requiredFeatureCount ? &dd : nullptr, dcb);
    while (!dr.done) wgpuInstanceProcessEvents(ctx.instance);
    const bool timestampRequestAccepted =
        dd.requiredFeatureCount != 0 && dr.device != nullptr;
    if (!dr.device && dd.requiredFeatureCount) {
        dr = {};
        wgpuAdapterRequestDevice(ctx.adapter, nullptr, dcb);
        while (!dr.done) wgpuInstanceProcessEvents(ctx.instance);
    }
    if (!dr.device) { std::fprintf(stderr, "[WebGPU] device request failed\n"); return ctx; }

    ctx.device = dr.device;
    ctx.queue = wgpuDeviceGetQueue(ctx.device);
    ctx.timestampQueries = requestedTimestampQueries && timestampRequestAccepted;
    return ctx;
}

void configureSurface(WindowContext& ctx, uint32_t width, uint32_t height) {
    if (!ctx.valid() || width == 0 || height == 0) return;

    // The layer's drawable size is in PIXELS; keeping it in step with the
    // framebuffer size is what stops a resized window from presenting stretched.
    CAMetalLayer* layer = (__bridge CAMetalLayer*)ctx.metalLayer;
    layer.drawableSize = CGSizeMake(width, height);

    WGPUSurfaceConfiguration cfg = {};
    cfg.device = ctx.device;
    cfg.format = kSurfaceFormat;
    cfg.usage = WGPUTextureUsage_RenderAttachment;
    cfg.width = width;
    cfg.height = height;
    cfg.presentMode = WGPUPresentMode_Immediate; // vsync OFF
    cfg.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(ctx.surface, &cfg);
    if (@available(macOS 10.13, *)) {
        layer.displaySyncEnabled = NO;
    }
}

void destroyWindowContext(WindowContext& ctx) {
    if (ctx.surface)  { wgpuSurfaceRelease(ctx.surface);   ctx.surface  = nullptr; }
    if (ctx.queue)    { wgpuQueueRelease(ctx.queue);       ctx.queue    = nullptr; }
    if (ctx.device)   { wgpuDeviceRelease(ctx.device);     ctx.device   = nullptr; }
    if (ctx.adapter)  { wgpuAdapterRelease(ctx.adapter);   ctx.adapter  = nullptr; }
    if (ctx.instance) { wgpuInstanceRelease(ctx.instance); ctx.instance = nullptr; }
    ctx.metalLayer = nullptr;
}

} // namespace wgpu
