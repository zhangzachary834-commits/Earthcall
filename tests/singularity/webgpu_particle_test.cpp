#include <iostream>
#include <cassert>
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

int main() {
    std::cout << "Starting webgpu_particle_test..." << std::endl;
    
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "Failed to init WebGPU device" << std::endl;
        return 1;
    }
    auto renderer = std::make_unique<WebGpuRenderer>();
    renderer->init(gpu);
    
    // Create a field node
    auto fieldNode = std::make_unique<geom::FieldNode>("test_field");
    
    // Set some field parameters
    fieldNode->vectorField->baseFlowY = 2.0f;
    fieldNode->vectorField->baseFlowX = 0.5f;
    
    // Try to draw particles
    std::cout << "Dispatching drawParticles..." << std::endl;
    
    WGPUTextureDescriptor texDesc = {};
    texDesc.usage = WGPUTextureUsage_RenderAttachment;
    texDesc.dimension = WGPUTextureDimension_2D;
    texDesc.size = {800, 600, 1};
    texDesc.format = WGPUTextureFormat_RGBA8Unorm;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = 1;
    WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &texDesc);
    WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);
    
    renderer->beginFrameOffscreen(view, 800, 600, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    renderer->drawParticles(*fieldNode, 1000);
    renderer->endFrame();
    
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(tex);
    
    // We don't have a full window or swapchain in this headless test, 
    // but we can verify it doesn't crash during pipeline creation and dispatch setup.
    std::cout << "drawParticles completed without crashing." << std::endl;
    
    return 0;
}
