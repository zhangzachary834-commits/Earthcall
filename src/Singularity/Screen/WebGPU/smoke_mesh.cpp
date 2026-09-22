// WebGPU mesh-pipeline smoke (Milestone 5). Proves the substance of drawMesh:
// a WGSL shader + vertex buffer + a uniform (the material colour) render a
// triangle to an offscreen texture, and the read-back pixel equals the uniform.
// This is the offscreen-verifiable core; the on-screen surface is a later step.
#include "WgpuDevice.hpp"
#include <webgpu/wgpu.h>
#include <cstdio>
#include <cstdlib>

using wgpu::Device;

// Position-only vertex; a uniform vec4 supplies the colour (stands in for
// RenderMaterial.baseColor). Fullscreen triangle so every pixel is the colour.
static const char* kWGSL = R"(
struct U { color: vec4<f32> };
@group(0) @binding(0) var<uniform> u: U;

@vertex fn vs_main(@location(0) pos: vec3<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(pos, 1.0);
}
@fragment fn fs_main() -> @location(0) vec4<f32> {
    return u.color;
}
)";

struct MapR { bool ok = false, done = false; };
static void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u); r->ok = (s == WGPUMapAsyncStatus_Success); r->done = true;
}

int main() {
    Device gpu;
    if (!gpu.init()) { printf("FAIL: device bring-up\n"); return 1; }

    // --- Shader module -------------------------------------------------------
    WGPUShaderSourceWGSL wgslSrc = {};
    wgslSrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSrc.code = Device::str(kWGSL);
    WGPUShaderModuleDescriptor smDesc = {};
    smDesc.nextInChain = &wgslSrc.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu.device, &smDesc);
    if (!shader) { printf("FAIL: shader module\n"); return 1; }

    // --- Bind group layout: one uniform buffer, visible to the fragment stage -
    WGPUBindGroupLayoutEntry bglEntry = {};
    bglEntry.binding = 0;
    bglEntry.visibility = WGPUShaderStage_Fragment;
    bglEntry.buffer.type = WGPUBufferBindingType_Uniform;
    bglEntry.buffer.minBindingSize = 16;
    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 1;
    bglDesc.entries = &bglEntry;
    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(gpu.device, &bglDesc);

    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bgl;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(gpu.device, &plDesc);

    // --- Buffers -------------------------------------------------------------
    float color[4] = { 1.0f, 0.5f, 0.25f, 1.0f }; // -> RGBA8 (255,128,64,255)
    WGPUBufferDescriptor ubDesc = {};
    ubDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    ubDesc.size = 16;
    WGPUBuffer ubuf = wgpuDeviceCreateBuffer(gpu.device, &ubDesc);
    wgpuQueueWriteBuffer(gpu.queue, ubuf, 0, color, 16);

    float verts[9] = { -1,-1,0,  3,-1,0,  -1,3,0 }; // fullscreen triangle
    WGPUBufferDescriptor vbDesc = {};
    vbDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    vbDesc.size = sizeof(verts);
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(gpu.device, &vbDesc);
    wgpuQueueWriteBuffer(gpu.queue, vbuf, 0, verts, sizeof(verts));

    WGPUBindGroupEntry bgEntry = {};
    bgEntry.binding = 0; bgEntry.buffer = ubuf; bgEntry.offset = 0; bgEntry.size = 16;
    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = bgl; bgDesc.entryCount = 1; bgDesc.entries = &bgEntry;
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(gpu.device, &bgDesc);

    // --- Render pipeline -----------------------------------------------------
    WGPUVertexAttribute attr = {};
    attr.format = WGPUVertexFormat_Float32x3; attr.offset = 0; attr.shaderLocation = 0;
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex;
    vbl.arrayStride = 12;
    vbl.attributeCount = 1;
    vbl.attributes = &attr;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = WGPUTextureFormat_RGBA8Unorm;
    colorTarget.writeMask = WGPUColorWriteMask_All;
    WGPUFragmentState frag = {};
    frag.module = shader; frag.entryPoint = Device::str("fs_main");
    frag.targetCount = 1; frag.targets = &colorTarget;

    WGPURenderPipelineDescriptor pd = {};
    pd.layout = layout;
    pd.vertex.module = shader;
    pd.vertex.entryPoint = Device::str("vs_main");
    pd.vertex.bufferCount = 1;
    pd.vertex.buffers = &vbl;
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pd.primitive.cullMode = WGPUCullMode_None;
    pd.multisample.count = 1;
    pd.multisample.mask = 0xFFFFFFFFu;
    pd.fragment = &frag;
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(gpu.device, &pd);
    if (!pipeline) { printf("FAIL: render pipeline\n"); return 1; }

    // --- Offscreen target + readback buffer ----------------------------------
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { 4, 4, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);

    WGPUBufferDescriptor rbDesc = {};
    rbDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbDesc.size = 256 * 4;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbDesc);

    // --- Encode: clear black, draw the triangle, copy to buffer --------------
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    WGPURenderPassColorAttachment ca = {};
    ca.view = view; ca.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ca.loadOp = WGPULoadOp_Clear; ca.storeOp = WGPUStoreOp_Store;
    ca.clearValue = { 0.0, 0.0, 0.0, 1.0 };
    WGPURenderPassDescriptor rp = {};
    rp.colorAttachmentCount = 1; rp.colorAttachments = &ca;
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(enc, &rp);
    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vbuf, 0, sizeof(verts));
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUTexelCopyTextureInfo src = {};
    src.texture = tex; src.mipLevel = 0; src.origin = { 0,0,0 }; src.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = readback; dst.layout.offset = 0; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = 4;
    WGPUExtent3D cs = { 4, 4, 1 };
    wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &cs);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);

    // --- Read back the centre pixel ------------------------------------------
    MapR mr;
    WGPUBufferMapCallbackInfo mcb = {};
    mcb.mode = WGPUCallbackMode_AllowProcessEvents; mcb.callback = onMap; mcb.userdata1 = &mr;
    wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * 4, mcb);
    while (!mr.done) wgpuDevicePoll(gpu.device, true, nullptr);
    if (!mr.ok) { printf("FAIL: map\n"); return 1; }

    const unsigned char* px = (const unsigned char*)wgpuBufferGetConstMappedRange(readback, 0, 256 * 4);
    printf("triangle pixel RGBA = (%d, %d, %d, %d)\n", px[0], px[1], px[2], px[3]);
    bool ok = abs(px[0]-255)<3 && abs(px[1]-128)<3 && abs(px[2]-64)<3 && px[3]==255;
    printf(ok ? "OK: WGSL pipeline drew a mesh tinted by a uniform, read back correct\n"
              : "FAIL: wrong triangle colour\n");
    wgpuBufferUnmap(readback);
    return ok ? 0 : 1;
}
