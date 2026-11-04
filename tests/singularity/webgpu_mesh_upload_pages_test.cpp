// Codex / GPT-6 / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-03.
// Independent pixel oracle for aligned upload pages, multiple flushes, an
// oversized instance batch, and changed transforms/materials between frames.
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SmoothSurface.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    constexpr unsigned columns = 48, rows = 24, count = columns * rows;
    constexpr unsigned width = 512, height = 256, stride = width * 4;
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::puts("SKIP: no WebGPU device");
        return 0;
    }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) return 1;
    renderer.setCamera(glm::mat4(1), glm::vec3(0, 0, 2));
    renderer.setLight(glm::vec3(0, 0, 2), glm::vec3(1), glm::vec3(0), glm::vec3(0));
    geom::TessMesh mesh;
    const auto vertex = [](float x, float y) {
        return geom::TessVertex{{x, y, 0}, {0, 0, 1}, {0, 0}};
    };
    mesh.tris = {vertex(-1, -1), vertex(1, -1), vertex(1, 1),
                 vertex(-1, -1), vertex(1, 1), vertex(-1, 1)};
    std::vector<TextureHandle> textures;
    std::vector<std::array<uint8_t, 4>> colors;
    for (unsigned i = 0; i < count; ++i) {
        colors.push_back({uint8_t(40 + i % 180), uint8_t(60 + i * 7 % 160), uint8_t(80 + i * 11 % 140), 255});
        textures.push_back(renderer.uploadTexture(0, colors.back().data(), 1, 1));
    }
    WGPUTextureDescriptor descriptor{};
    descriptor.size = {width, height, 1};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    descriptor.mipLevelCount = descriptor.sampleCount = 1;
    auto texture = wgpuDeviceCreateTexture(gpu.device, &descriptor);
    auto view = wgpuTextureCreateView(texture, nullptr);
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = stride * height;
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    auto readback = wgpuDeviceCreateBuffer(gpu.device, &bufferDescriptor);
    unsigned failures = 0, checks = 0;
    for (unsigned frame = 0; frame < 3; ++frame) {
        if (frame == 1) {
            colors[17] = {231, 19, 141, 255};
            renderer.uploadTexture(textures[17], colors[17].data(), 1, 1);
        }
        renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));
        RenderMaterial material;
        material.ambient = 1; material.diffuse = material.specular = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto cell = (i + frame * 37) % count;
            const float x = (float(cell % columns) + .5f) * 2 / columns - 1;
            const float y = 1 - (float(cell / columns) + .5f) * (240.f / height) * 2 / rows;
            renderer.setModel(glm::scale(glm::translate(glm::mat4(1), {x, y, .5f}),
                glm::vec3(.85f / columns, .85f * (240.f / height) / rows, 1)));
            material.textureId = textures[i];
            material.ambient = i % 2 ? .5f : 1.f;
            renderer.drawMesh(mesh, material);
            if (frame == 1 && i == 600) {
                renderer.begin2D(width, height);
                renderer.end2D();
            }
        }
        // This one batch exceeds a 256 KiB page. Its last instance is visible.
        material.textureId = 0; material.ambient = 1;
        material.baseColor = glm::vec3(1, 0, 1);
        for (unsigned i = 0; i < 2050; ++i) {
            renderer.setModel(glm::scale(glm::translate(glm::mat4(1),
                glm::vec3(i == 2049 ? -.95f : 3.f, i == 2049 ? -.96f : 3.f, .5f)),
                glm::vec3(.02f, .02f, 1)));
            renderer.drawMesh(mesh, material);
        }
        renderer.endFrame();
        auto encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo source{}; source.texture = texture; source.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo destination{}; destination.buffer = readback;
        destination.layout.bytesPerRow = stride; destination.layout.rowsPerImage = height;
        WGPUExtent3D extent{width, height, 1};
        wgpuCommandEncoderCopyTextureToBuffer(encoder, &source, &destination, &extent);
        auto command = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        struct Result { bool done = false, ok = false; } result;
        WGPUBufferMapCallbackInfo callback{};
        callback.mode = WGPUCallbackMode_AllowProcessEvents; callback.userdata1 = &result;
        callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* data, void*) {
            auto& result = *static_cast<Result*>(data);
            result.done = true; result.ok = status == WGPUMapAsyncStatus_Success;
        };
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, stride * height, callback);
        while (!result.done) wgpuInstanceProcessEvents(gpu.instance);
        if (!result.ok) return 1;
        const auto* pixels = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(readback, 0, stride * height));
        for (unsigned i = 0; i < count; ++i) {
            const unsigned cell = (i + frame * 37) % count;
            const unsigned x = unsigned((float(cell % columns) + .5f) * width / columns);
            const unsigned y = (cell / columns) * 10 + 5;
            for (unsigned channel = 0; channel < 3; ++channel) {
                const float expected = colors[i][channel] * (i % 2 ? .5f : 1.f);
                ++checks;
                if (std::abs(float(pixels[(y * width + x) * 4 + channel]) - expected) > 1) ++failures;
            }
        }
        const unsigned last = (251 * width + 13) * 4;
        ++checks;
        if (pixels[last] != 255 || pixels[last + 1] != 0 || pixels[last + 2] != 255) ++failures;
        wgpuBufferUnmap(readback);
        wgpuCommandBufferRelease(command); wgpuCommandEncoderRelease(encoder);
    }
    for (auto handle : textures) renderer.releaseTexture(handle);
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(texture);
    std::printf("mesh upload pages: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
