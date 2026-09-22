#include "WebGpuContext.hpp"
#include <iostream>
#include <webgpu/webgpu.h>
#include <emscripten.h>

namespace wgpu {

WindowContext createWindowContext(GLFWwindow* win) {
    WindowContext ctx;
    ctx.instance = wgpuCreateInstance(nullptr);
    if (!ctx.instance) {
        std::cerr << "Failed to create WGPUInstance in Emscripten\n";
        return ctx;
    }

    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
    canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvasDesc.selector = WGPUStringView{"#earthcall-canvas", 17};

    WGPUSurfaceDescriptor surfDesc = {};
    surfDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvasDesc);
    ctx.surface = wgpuInstanceCreateSurface(ctx.instance, &surfDesc);
    if (!ctx.surface) {
        std::cerr << "Failed to create WGPUSurface from #canvas\n";
        return ctx;
    }

    WGPURequestAdapterOptions options = {};
    options.compatibleSurface = ctx.surface;
    options.powerPreference = WGPUPowerPreference_HighPerformance;

    struct AdapterResult { WGPUAdapter adapter = nullptr; bool done = false; };
    auto onAdapter = [](WGPURequestAdapterStatus status, WGPUAdapter a, WGPUStringView, void* ud, void*) {
        auto* r = static_cast<AdapterResult*>(ud);
        if (status == WGPURequestAdapterStatus_Success) r->adapter = a;
        r->done = true;
    };

    AdapterResult ar;
    WGPURequestAdapterCallbackInfo acb = {};
    acb.mode = WGPUCallbackMode_AllowSpontaneous;
    acb.callback = onAdapter;
    acb.userdata1 = &ar;
    wgpuInstanceRequestAdapter(ctx.instance, &options, acb);
    
    // Wait for the async adapter request using ASYNCIFY
    while (!ar.done) {
        emscripten_sleep(10);
    }
    if (!ar.adapter) {
        std::cerr << "Failed to get WGPUAdapter\n";
        return ctx;
    }
    ctx.adapter = ar.adapter;

    struct DeviceResult { WGPUDevice device = nullptr; bool done = false; };
    auto onDevice = [](WGPURequestDeviceStatus status, WGPUDevice d, WGPUStringView, void* ud, void*) {
        auto* r = static_cast<DeviceResult*>(ud);
        if (status == WGPURequestDeviceStatus_Success) r->device = d;
        r->done = true;
    };

    DeviceResult dr;
    WGPURequestDeviceCallbackInfo dcb = {};
    dcb.mode = WGPUCallbackMode_AllowSpontaneous;
    dcb.callback = onDevice;
    dcb.userdata1 = &dr;
    wgpuAdapterRequestDevice(ctx.adapter, nullptr, dcb);
    
    // Wait for the async device request using ASYNCIFY
    while (!dr.done) {
        emscripten_sleep(10);
    }
    if (!dr.device) {
        std::cerr << "Failed to get WGPUDevice\n";
        return ctx;
    }
    ctx.device = dr.device;
    ctx.queue = wgpuDeviceGetQueue(ctx.device);

    return ctx;
}

void configureSurface(WindowContext& ctx, uint32_t width, uint32_t height) {
    if (!ctx.device || !ctx.surface) return;
    WGPUSurfaceConfiguration config = {};
    config.device = ctx.device;
    config.format = kSurfaceFormat;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = width;
    config.height = height;
    config.presentMode = WGPUPresentMode_Fifo;
    wgpuSurfaceConfigure(ctx.surface, &config);
}

void destroyWindowContext(WindowContext& ctx) {
    if (ctx.surface) wgpuSurfaceRelease(ctx.surface);
    if (ctx.queue) wgpuQueueRelease(ctx.queue);
    if (ctx.device) wgpuDeviceRelease(ctx.device);
    if (ctx.adapter) wgpuAdapterRelease(ctx.adapter);
    if (ctx.instance) wgpuInstanceRelease(ctx.instance);
    ctx = WindowContext{};
}

WGPUTextureFormat surfaceFormat() {
    return WGPUTextureFormat_BGRA8Unorm; // Standard WebGPU canvas format
}

} // namespace wgpu
