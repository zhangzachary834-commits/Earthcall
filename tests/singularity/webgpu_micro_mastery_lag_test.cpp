// Heavy duty CPU-GPU micro-mastery lag test.
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <chrono>
#include <memory>

extern MaterialManager materials;

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }

    setCurrentRenderer(&renderer);

    const uint32_t W = 512, H = 512;
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    const glm::vec3 eye(0.0f, 0.0f, 150.0f);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), float(W) / H, 0.1f, 1000.0f);
    const glm::mat4 view3d = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

    std::printf("Creating massive object population...\n");
    std::vector<std::unique_ptr<Object>> meshes;
    for (int i = 0; i < 3000; ++i) {
        auto obj = std::make_unique<Object>("mesh_" + std::to_string(i));
        obj->setShapeKind(Object::ShapeKind::Cube);
        obj->setPosition(glm::vec3( (i % 100) * 1.5f - 75.0f, (i / 100) * 1.5f - 75.0f, 0.0f ));
        meshes.push_back(std::move(obj));
    }

    std::vector<std::unique_ptr<Object>> fields;
    geom::SdfNode sphere;
    sphere.op = geom::SdfOp::Leaf;
    sphere.prim = geom::SdfPrim::Sphere;
    sphere.dims = glm::vec3(0.5f);
    for (int i = 0; i < 1500; ++i) {
        auto obj = std::make_unique<Object>("field_" + std::to_string(i));
        obj->setFieldShape(sphere, 1.0f);
        obj->setPosition(glm::vec3( (i % 50) * 2.0f - 50.0f, (i / 50) * 2.0f - 50.0f, 10.0f ));
        fields.push_back(std::move(obj));
    }
    
    std::printf("Running frames...\n");
    const int kFrames = 60; 
    
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    
    for (int frame = 0; frame < kFrames; ++frame) {
        renderer.setCamera(view3d, proj, eye);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        
        for (size_t i = 0; i < meshes.size(); ++i) {
            meshes[i]->setRotationEulerDegrees(glm::vec3(frame * 0.5f, frame * 0.5f, 0.0f));
            meshes[i]->drawObject();
        }
        
        for (size_t i = 0; i < fields.size(); ++i) {
            fields[i]->setRotationEulerDegrees(glm::vec3(frame * -0.5f, frame * -0.5f, 0.0f));
            fields[i]->drawObject();
        }
        
        renderer.endFrame();
        
        auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
        if (elapsed > 20000.0) {
             const auto& stats = renderer.frameStats();
             std::printf("FAIL: Frame rendering exceeded time limit (20s threshold hit at frame %d).\n", frame);
             std::printf("Failure frame stats:\n");
             std::printf("  drawCalls: %d\n", stats.drawCalls);
             std::printf("  vramAllocatedBytes: %.2f MB\n", stats.vramAllocatedBytes / 1048576.0);
             std::printf("  bufferSuballocations: %d\n", stats.bufferSuballocations);
             setCurrentRenderer(nullptr);
             renderer.shutdown();
             std::fflush(stdout);
             std::_Exit(1);
        }
    }
    
    auto t1 = Clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double averageMs = totalMs / kFrames;
    
    std::printf("Finished %d frames in %.2f ms. Average frame time: %.2f ms\n", kFrames, totalMs, averageMs);
    
    const auto& stats = renderer.frameStats();
    std::printf("Last frame stats:\n");
    std::printf("  drawCalls: %d\n", stats.drawCalls);
    std::printf("  vramAllocatedBytes: %.2f MB\n", stats.vramAllocatedBytes / 1048576.0);
    std::printf("  bufferSuballocations: %d\n", stats.bufferSuballocations);
    
    if (averageMs > 300.0) {
        std::printf("LAG: Average frame time %.2f ms exceeded 300ms budget.\n", averageMs);
        setCurrentRenderer(nullptr);
        renderer.shutdown();
        std::fflush(stdout);
        std::_Exit(1);
    }
    
    std::printf("CPU-GPU Micro-Mastery Heavy-Duty Test: PASSED\n");

    setCurrentRenderer(nullptr);
    renderer.shutdown();
    std::fflush(stdout);
    std::_Exit(0);
}
