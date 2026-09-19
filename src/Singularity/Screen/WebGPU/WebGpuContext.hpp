#pragma once

// Window-attached WebGPU bring-up: instance, CAMetalLayer-backed surface, a
// surface-COMPATIBLE adapter, device and queue.
//
// This exists separately from WgpuDevice.hpp (which is headless: no surface, so
// any adapter will do) and separately from Engine.cpp because attaching a
// CAMetalLayer to a Cocoa view is Objective-C++. Keeping that in WebGpuContext.mm
// lets Engine.cpp stay ordinary C++ — the alternative was making the whole engine
// an .mm file for the sake of six lines of Cocoa.
//
// The recipe here is the one proven on hardware by smoke_window.mm.

#include <webgpu/webgpu.h>
#include <cstdint>

struct GLFWwindow;

namespace wgpu {

struct WindowContext {
    WGPUInstance instance = nullptr;
    WGPUAdapter  adapter  = nullptr;
    WGPUDevice   device   = nullptr;
    WGPUQueue    queue    = nullptr;
    WGPUSurface  surface  = nullptr;
    void*        metalLayer = nullptr; // CAMetalLayer*, retained by the window's view
    // Kernel diagnostic capability negotiated when the device was created. It
    // is deliberately separate from any authored screen-channel property.
    bool timestampQueries = false;

    bool valid() const { return device && queue && surface; }
};

// Attaches a CAMetalLayer to `win`'s content view and builds a WebGPU device able
// to present to it. The window MUST have been created with GLFW_NO_API — WebGPU
// owns presentation, and a GL context on the same window would fight it.
// Returns an invalid context (valid() == false) on any failure.
WindowContext createWindowContext(GLFWwindow* win);

// (Re)configure the swapchain for a framebuffer size. Call once after creation and
// again on every resize; presenting against a stale size gives a suboptimal or
// failed surface texture.
void configureSurface(WindowContext& ctx, uint32_t width, uint32_t height);

void destroyWindowContext(WindowContext& ctx);

// The format configureSurface uses, and therefore the format any pipeline drawing
// to this surface must declare.
constexpr WGPUTextureFormat kSurfaceFormat = WGPUTextureFormat_BGRA8Unorm;

} // namespace wgpu
