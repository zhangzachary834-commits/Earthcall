// Rung-8 native-resolution image-parity witness for OntoMath transform sovereignty.
//
// Arm A drives the production Renderer boundary: view + projection remain separate,
// WebGpuRenderer asks OntoMath to compose P*V, then asks OntoMath again to compose
// (P*V)*M when the model changes.
//
// Arm B is an independent GLM reference oracle. It composes P*V*M outside production
// and supplies that final matrix through WebGpuRenderer's representation-level
// two-argument setCamera overload with identity model.
//
// The resulting 1280x720 RGBA frames must be byte-for-byte identical. A nonblank /
// non-full-frame guard prevents two equally empty or broken frames from passing.

#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr uint32_t W = 1280;
constexpr uint32_t H = 720;
constexpr uint32_t kRowBytes = W * 4u;
constexpr uint64_t kFrameBytes = static_cast<uint64_t>(kRowBytes) * H;
static_assert((kRowBytes % 256u) == 0u,
              "native-resolution readback row must satisfy WebGPU alignment");

struct MapResult {
    bool done = false;
    bool ok = false;
};

void onMap(WGPUMapAsyncStatus status, WGPUStringView,
           void* userdata1, void*) {
    auto* result = static_cast<MapResult*>(userdata1);
    result->ok = status == WGPUMapAsyncStatus_Success;
    result->done = true;
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) {
        std::printf("FAIL: no WebGPU device\n");
        return 1;
    }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) {
        std::printf("FAIL: renderer init\n");
        return 1;
    }

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {W, H, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView targetView = wgpuTextureCreateView(target, nullptr);

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = kFrameBytes;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    auto capture = [&]() {
        WGPUCommandEncoder encoder =
            wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);

        WGPUTexelCopyTextureInfo src = {};
        src.texture = target;
        src.aspect = WGPUTextureAspect_All;
        src.origin = {0, 0, 0};

        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback;
        dst.layout.bytesPerRow = kRowBytes;
        dst.layout.rowsPerImage = H;

        WGPUExtent3D extent = {W, H, 1};
        wgpuCommandEncoderCopyTextureToBuffer(
            encoder, &src, &dst, &extent);
        WGPUCommandBuffer command =
            wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);

        MapResult mapped;
        WGPUBufferMapCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowProcessEvents;
        callback.callback = onMap;
        callback.userdata1 = &mapped;
        wgpuBufferMapAsync(
            readback, WGPUMapMode_Read, 0, kFrameBytes, callback);
        while (!mapped.done) {
            wgpuDevicePoll(gpu.device, true, nullptr);
        }
        assert(mapped.ok);

        const auto* bytes = static_cast<const uint8_t*>(
            wgpuBufferGetConstMappedRange(readback, 0, kFrameBytes));
        assert(bytes);
        std::vector<uint8_t> frame(bytes, bytes + kFrameBytes);
        wgpuBufferUnmap(readback);
        return frame;
    };

    const glm::vec3 eye(0.0f, 0.0f, 4.0f);
    const glm::mat4 view =
        glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    const glm::mat4 projection =
        glm::perspectiveRH_ZO(
            glm::radians(52.0f), static_cast<float>(W) / H,
            0.125f, 32.0f);

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(0.37f, -0.19f, -0.35f));
    model = glm::rotate(
        model, glm::radians(23.0f),
        glm::normalize(glm::vec3(0.25f, 1.0f, 0.15f)));
    model = glm::scale(model, glm::vec3(1.35f, 0.82f, 1.0f));

    const std::vector<glm::vec3> triangles = {
        {-0.90f, -0.58f, 0.0f},
        { 0.78f, -0.58f, 0.0f},
        { 0.78f,  0.66f, 0.0f},
        {-0.90f, -0.58f, 0.0f},
        { 0.78f,  0.66f, 0.0f},
        {-0.90f,  0.66f, 0.0f},
    };

    const glm::vec4 clear(0.03125f, 0.0625f, 0.125f, 1.0f);
    const glm::vec4 color(0.8125f, 0.3125f, 0.625f, 1.0f);

    // Arm A: production camera/model semantics are authored by OntoMath.
    renderer.setCamera(view, projection, eye);
    renderer.setModel(model);
    renderer.beginFrameOffscreen(targetView, W, H, clear);
    renderer.drawSolid(
        triangles, color, Blend::Opaque, /*depthWrite=*/true);
    renderer.endFrame();
    const std::vector<uint8_t> production = capture();

    // Arm B: independent test oracle. GLM originates the final transform here,
    // outside production. WebGPU only carries that frozen representation.
    const glm::mat4 oracleMvp = projection * view * model;
    renderer.setCamera(oracleMvp, eye);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(targetView, W, H, clear);
    renderer.drawSolid(
        triangles, color, Blend::Opaque, /*depthWrite=*/true);
    renderer.endFrame();
    const std::vector<uint8_t> oracle = capture();

    assert(production.size() == oracle.size());
    const uint8_t background[4] = {
        production[0], production[1], production[2], production[3]
    };

    uint64_t mismatchPixels = 0;
    uint64_t foregroundPixels = 0;
    for (uint64_t pixel = 0; pixel < static_cast<uint64_t>(W) * H; ++pixel) {
        const uint64_t at = pixel * 4u;
        if (std::memcmp(production.data() + at, oracle.data() + at, 4) != 0) {
            ++mismatchPixels;
        }
        if (std::memcmp(production.data() + at, background, 4) != 0) {
            ++foregroundPixels;
        }
    }

    const uint64_t totalPixels = static_cast<uint64_t>(W) * H;
    std::printf(
        "ONTOMATH_NATIVE_RENDER_PARITY width=%u height=%u "
        "foreground_pixels=%llu mismatch_pixels=%llu\n",
        W, H,
        static_cast<unsigned long long>(foregroundPixels),
        static_cast<unsigned long long>(mismatchPixels));

    assert(foregroundPixels > 10000 &&
           "native parity witness rendered an effectively blank frame");
    assert(foregroundPixels + 10000 < totalPixels &&
           "native parity witness accidentally filled the entire frame");
    assert(mismatchPixels == 0 &&
           "OntoMath-authored native frame differs from frozen GLM oracle");

    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(targetView);
    wgpuTextureRelease(target);
    renderer.shutdown();

    std::printf("webgpu_ontomath_native_resolution_parity_test: PASS\n");
    std::fflush(stdout);
    std::_Exit(0);
}
