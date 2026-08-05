#pragma once

// Minimal synchronous wgpu-native device bring-up, shared by the WebGPU smokes
// and (later) WebGpuRenderer. The webgpu.h adapter/device requests are async;
// wgpu-native resolves them through processEvents, which we pump until done.
// Header-only so a single-TU smoke can just #include it.

#include <webgpu/webgpu.h>

namespace wgpu {
namespace detail {

struct AdapterR { WGPUAdapter a = nullptr; bool done = false; };
inline void onAdapter(WGPURequestAdapterStatus s, WGPUAdapter a, WGPUStringView, void* u, void*) {
    auto* r = static_cast<AdapterR*>(u);
    if (s == WGPURequestAdapterStatus_Success) r->a = a;
    r->done = true;
}
struct DeviceR { WGPUDevice d = nullptr; bool done = false; };
inline void onDevice(WGPURequestDeviceStatus s, WGPUDevice d, WGPUStringView, void* u, void*) {
    auto* r = static_cast<DeviceR*>(u);
    if (s == WGPURequestDeviceStatus_Success) r->d = d;
    r->done = true;
}

} // namespace detail

// The device and its dependencies. Not RAII yet (the smokes are short-lived);
// WebGpuRenderer will own release. init() returns false on any bring-up failure.
struct Device {
    WGPUInstance instance = nullptr;
    WGPUAdapter  adapter  = nullptr;
    WGPUDevice   device   = nullptr;
    WGPUQueue    queue    = nullptr;

    bool init() {
        instance = wgpuCreateInstance(nullptr);
        if (!instance) return false;

        detail::AdapterR ar;
        WGPURequestAdapterCallbackInfo acb = {};
        acb.mode = WGPUCallbackMode_AllowProcessEvents;
        acb.callback = detail::onAdapter;
        acb.userdata1 = &ar;
        wgpuInstanceRequestAdapter(instance, nullptr, acb);
        while (!ar.done) wgpuInstanceProcessEvents(instance);
        if (!ar.a) return false;
        adapter = ar.a;

        detail::DeviceR dr;
        WGPURequestDeviceCallbackInfo dcb = {};
        dcb.mode = WGPUCallbackMode_AllowProcessEvents;
        dcb.callback = detail::onDevice;
        dcb.userdata1 = &dr;
        wgpuAdapterRequestDevice(adapter, nullptr, dcb);
        while (!dr.done) wgpuInstanceProcessEvents(instance);
        if (!dr.d) return false;
        device = dr.d;

        queue = wgpuDeviceGetQueue(device);
        return queue != nullptr;
    }

    // Helper to make a WGPUStringView from a C string literal.
    static WGPUStringView str(const char* s) { return WGPUStringView{ s, s ? __builtin_strlen(s) : 0 }; }
};

} // namespace wgpu
