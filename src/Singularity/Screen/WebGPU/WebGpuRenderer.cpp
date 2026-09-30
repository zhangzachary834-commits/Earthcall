#include <chrono>
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfRangeProof.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <functional>
#include <set>
#include <string>
#include <utility>

namespace {

// Vertex layout mirrors geom::TessVertex exactly: {pos(3), normal(3), uv(2)}.
// A world-space Lambert term (ambient + diffuse*N·L) tints baseColor; front_facing
// flips the normal so open surfaces (patches) light on both sides. Texture albedo
// (faceTextures) is not sampled yet — a later refinement (needs a WGPU texture).
// Per-instance transform (CPU-GPU micro-mastery Phase 4.3): every mesh draw is
// an instanced draw now, even a "batch" of one, so the model/normal matrices
// that used to live in the per-draw uniform U live in this per-instance
// storage array instead, indexed by @builtin(instance_index). U carries only
// what every instance in a batch genuinely SHARES (camera, material, light).
const char* kMeshWGSL = R"(
struct U {
    viewProj:  mat4x4<f32>,
    lightPos:  vec4<f32>,   // world-space POSITION (GL_LIGHT0 is positional)
    params:    vec4<f32>,   // x=ambient, y=diffuse, z=specular, w=shininess
    eyePos:    vec4<f32>,
};
struct Instance {
    model:     mat4x4<f32>,
    normalMat: mat4x4<f32>,
    baseColor: vec4<f32>,
};
@group(0) @binding(0) var<uniform> u: U;
@group(0) @binding(1) var albedoTex: texture_2d<f32>;
@group(0) @binding(2) var albedoSamp: sampler;
@group(1) @binding(0) var<storage, read> instances: array<Instance>;

struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldNormal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
    @location(3) baseColor: vec4<f32>,
};

@vertex
fn vs_main(@location(0) pos: vec3<f32>, @location(1) normal: vec3<f32>,
           @location(2) uv: vec2<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var out: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos, 1.0);
    out.clip = u.viewProj * world;
    out.worldNormal = (inst.normalMat * vec4<f32>(normal, 0.0)).xyz;
    out.uv = uv;
    out.worldPos = world.xyz;
    out.baseColor = inst.baseColor;
    return out;
}

@fragment
fn fs_main(in: VSOut, @builtin(front_facing) front: bool) -> @location(0) vec4<f32> {
    var N = normalize(in.worldNormal);
    if (!front) { N = -N; }
    let L = normalize(u.lightPos.xyz - in.worldPos);
    let V = normalize(u.eyePos.xyz - in.worldPos);
    let H = normalize(L + V);
    let diff = max(dot(N, L), 0.0);
    let lit = u.params.x + u.params.y * diff;
    // Blinn-Phong specular: white highlight, gated so it only appears on lit faces.
    let spec = u.params.z * pow(max(dot(N, H), 0.0), max(u.params.w, 1.0)) * step(0.0001, diff);
    let texel = textureSample(albedoTex, albedoSamp, in.uv); // paint (white when untextured)
    let rgb = in.baseColor.rgb * texel.rgb * lit + vec3<f32>(spec);
    return vec4<f32>(rgb, in.baseColor.a * texel.a);
}
)";

// std140-compatible: all members are 16-byte aligned, so this matches the WGSL
// uniform block byte-for-byte. glm and WGSL are both column-major. model/
// normalMat moved to the per-instance storage buffer — see kMeshWGSL.
struct MeshUniforms {
    glm::mat4 viewProj;
    glm::vec4 lightPos;
    glm::vec4 params;
    glm::vec4 eyePos;
};

// Unlit flat-colour shader for the selection overlay + wireframe.
const char* kFlatWGSL = R"(
struct FU { mvp: mat4x4<f32>, color: vec4<f32> };
@group(0) @binding(0) var<uniform> fu: FU;
@vertex fn vs(@location(0) pos: vec3<f32>) -> @builtin(position) vec4<f32> {
    return fu.mvp * vec4<f32>(pos, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return fu.color; }
)";
struct FlatUniforms { glm::mat4 mvp; glm::vec4 color; };

// Textured screen-space quad: the brush-canvas blit. Same {mvp, color} uniform as
// the flat shader (colour is the tint), plus an albedo texture + sampler.
const char* kImageWGSL = R"(
struct IU { mvp: mat4x4<f32>, tint: vec4<f32> };
@group(0) @binding(0) var<uniform> iu: IU;
@group(0) @binding(1) var img: texture_2d<f32>;
@group(0) @binding(2) var smp: sampler;
struct VOut { @builtin(position) clip: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs(@location(0) pos: vec3<f32>, @location(1) uv: vec2<f32>) -> VOut {
    var o: VOut;
    o.clip = iu.mvp * vec4<f32>(pos, 1.0);
    o.uv = uv;
    return o;
}
@fragment fn fs(in: VOut) -> @location(0) vec4<f32> {
    return textureSample(img, smp, in.uv) * iu.tint;
}
)";
struct ImageVertex { glm::vec3 pos; glm::vec2 uv; };

const char* kParticleWGSL = R"(
struct PU { 
    mvp: mat4x4<f32>, 
    color: vec4<f32>, 
    originAndTravel: vec4<f32>, 
    flowDir: vec4<f32>, 
    scale: vec4<f32> 
};
@group(0) @binding(0) var<uniform> pu: PU;

var<private> h: u32;

fn rnd() -> f32 {
    h = h ^ (h << 13u);
    h = h ^ (h >> 17u);
    h = h ^ (h << 5u);
    return f32(h & 0xFFFFFFu) / f32(0xFFFFFFu);
}

@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4<f32> {
    h = vi * 2654435761u + 1u;
    let local = vec3<f32>(rnd() * 2.0 - 1.0, rnd() * 2.0 - 1.0, rnd() * 2.0 - 1.0);
    let phase = rnd();
    
    let pos = pu.originAndTravel.xyz + local * pu.scale.xyz + pu.flowDir.xyz * (phase * pu.originAndTravel.w);
    return pu.mvp * vec4<f32>(pos, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return pu.color; }
)";

struct ParticleUniforms {
    glm::mat4 mvp;
    glm::vec4 color;
    glm::vec4 originAndTravel; // xyz: origin, w: travel
    glm::vec4 flowDir; // xyz: flowDir, w: unused
    glm::vec4 scale; // xyz: scale, w: unused
};


// A flat pipeline: position-only vertex, one uniform, chosen topology + blend +
// depth behaviour. In GL these last two were mutable state; here they are baked in.
WGPURenderPipeline makeFlatPipeline(WGPUDevice dev, WGPUPipelineLayout layout,
                                    WGPUShaderModule shader, WGPUPrimitiveTopology topo,
                                    WGPUBlendFactor srcF, WGPUBlendFactor dstF,
                                    WGPUTextureFormat colorFormat,
                                    bool depthWrite, bool depthTest) {
    WGPUVertexAttribute attr = {};
    attr.format = WGPUVertexFormat_Float32x3; attr.offset = 0; attr.shaderLocation = 0;
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex; vbl.arrayStride = 12;
    vbl.attributeCount = 1; vbl.attributes = &attr;

    WGPUBlendState blend = {};
    blend.color.operation = WGPUBlendOperation_Add; blend.color.srcFactor = srcF; blend.color.dstFactor = dstF;
    blend.alpha.operation = WGPUBlendOperation_Add; blend.alpha.srcFactor = WGPUBlendFactor_One; blend.alpha.dstFactor = WGPUBlendFactor_One;
    WGPUColorTargetState ct = {};
    ct.format = colorFormat; ct.writeMask = WGPUColorWriteMask_All; ct.blend = &blend;
    WGPUFragmentState frag = {};
    frag.module = shader; frag.entryPoint = wgpu::Device::str("fs"); frag.targetCount = 1; frag.targets = &ct;

    // The pass always has a depth attachment, so "no depth test" is expressed as
    // compare=Always + write off rather than by detaching it.
    WGPUDepthStencilState ds = {};
    ds.format = WGPUTextureFormat_Depth24Plus;
    ds.depthWriteEnabled = depthWrite ? WGPUOptionalBool_True : WGPUOptionalBool_False;
    ds.depthCompare = depthTest ? WGPUCompareFunction_Less : WGPUCompareFunction_Always;

    WGPURenderPipelineDescriptor pd = {};
    pd.layout = layout;
    pd.vertex.module = shader; pd.vertex.entryPoint = wgpu::Device::str("vs");
    pd.vertex.bufferCount = 1; pd.vertex.buffers = &vbl;
    pd.primitive.topology = topo; pd.primitive.cullMode = WGPUCullMode_None;
    pd.depthStencil = &ds;
    pd.multisample.count = 1; pd.multisample.mask = 0xFFFFFFFFu;
    pd.fragment = &frag;
    return wgpuDeviceCreateRenderPipeline(dev, &pd);
}

} // namespace

bool WebGpuRenderer::init(const wgpu::Device& gpu, WGPUTextureFormat colorFormat) {
    _device = gpu.device;
    _queue  = gpu.queue;
    _instance = gpu.instance;
    _colorFormat = colorFormat;
    if (!_device || !_queue) return false;

    // This is observational kernel infrastructure only. Failure or absence is
    // deliberately non-fatal: Earthcall must render identically without a GPU
    // timestamp extension, and F3 will say that no execution sample is present.
    initGpuTimestampQueries(gpu.timestampQueries);

    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(kMeshWGSL);
    WGPUShaderModuleDescriptor smDesc = {};
    smDesc.nextInChain = &src.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(_device, &smDesc);
    if (!shader) return false;

    WGPUBindGroupLayoutEntry bglEntries[3] = {};
    bglEntries[0].binding = 0;
    bglEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bglEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    bglEntries[0].buffer.minBindingSize = sizeof(MeshUniforms);
    bglEntries[1].binding = 1;
    bglEntries[1].visibility = WGPUShaderStage_Fragment;
    bglEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
    bglEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bglEntries[2].binding = 2;
    bglEntries[2].visibility = WGPUShaderStage_Fragment;
    bglEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;
    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 3;
    bglDesc.entries = bglEntries;
    _bgl = wgpuDeviceCreateBindGroupLayout(_device, &bglDesc);

    // group(1): the per-instance transform storage array (Phase 4.3). Same
    // ReadOnlyStorage shape as the SDF params bind group below — minBindingSize
    // left at 0 (unsized), since a batch's instance count varies draw to draw.
    WGPUBindGroupLayoutEntry instEntry = {};
    instEntry.binding = 0;
    instEntry.visibility = WGPUShaderStage_Vertex;
    instEntry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    WGPUBindGroupLayoutDescriptor instBglDesc = {};
    instBglDesc.entryCount = 1;
    instBglDesc.entries = &instEntry;
    _instanceBgl = wgpuDeviceCreateBindGroupLayout(_device, &instBglDesc);

    // group(2): SDF instances plus two derived acceleration buffers:
    // binding 1 is the proven-heightfield min/max grid; binding 2 is the
    // conservative zero-set hierarchy. Both are read-only Kernel substrate.
    WGPUBindGroupLayoutEntry sdfInstEntry[3] = {};
    sdfInstEntry[0].binding = 0;
    sdfInstEntry[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    sdfInstEntry[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    sdfInstEntry[1].binding = 1;
    sdfInstEntry[1].visibility = WGPUShaderStage_Fragment;
    sdfInstEntry[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    sdfInstEntry[2].binding = 2;
    sdfInstEntry[2].visibility = WGPUShaderStage_Fragment;
    sdfInstEntry[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    WGPUBindGroupLayoutDescriptor sdfInstBglDesc = {};
    sdfInstBglDesc.entryCount = 3;
    sdfInstBglDesc.entries = sdfInstEntry;
    _sdfInstanceBgl = wgpuDeviceCreateBindGroupLayout(_device, &sdfInstBglDesc);

    WGPUBindGroupLayout meshLayouts[2] = { _bgl, _instanceBgl };
    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 2;
    plDesc.bindGroupLayouts = meshLayouts;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &plDesc);

    WGPUVertexAttribute attrs[3] = {};
    attrs[0].format = WGPUVertexFormat_Float32x3; attrs[0].offset = 0;  attrs[0].shaderLocation = 0; // pos
    attrs[1].format = WGPUVertexFormat_Float32x3; attrs[1].offset = 12; attrs[1].shaderLocation = 1; // normal
    attrs[2].format = WGPUVertexFormat_Float32x2; attrs[2].offset = 24; attrs[2].shaderLocation = 2; // uv
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex;
    vbl.arrayStride = 32; // sizeof(geom::TessVertex)
    vbl.attributeCount = 3;
    vbl.attributes = attrs;

    // Alpha blending so RenderMaterial::opacity actually means something. With the
    // default opacity of 1.0 this is a no-op — src*1 + dst*0 is exactly what an
    // unblended write does — so it costs nothing for opaque surfaces while making
    // a translucent material render translucent instead of silently solid.
    WGPUBlendState meshBlend = {};
    meshBlend.color.operation = WGPUBlendOperation_Add;
    meshBlend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    meshBlend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    meshBlend.alpha.operation = WGPUBlendOperation_Add;
    meshBlend.alpha.srcFactor = WGPUBlendFactor_One;
    meshBlend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = _colorFormat;
    colorTarget.writeMask = WGPUColorWriteMask_All;
    colorTarget.blend = &meshBlend;
    WGPUFragmentState frag = {};
    frag.module = shader;
    frag.entryPoint = wgpu::Device::str("fs_main");
    frag.targetCount = 1;
    frag.targets = &colorTarget;

    // Depth test so nearer surfaces occlude farther ones. Depth24Plus, less-than,
    // depth writes on. (WebGPU clip depth is [0,1]; the app must build projections
    // with GLM_FORCE_DEPTH_ZERO_TO_ONE when feeding this backend.)
    WGPUDepthStencilState ds = {};
    ds.format = WGPUTextureFormat_Depth24Plus;
    ds.depthWriteEnabled = WGPUOptionalBool_True;
    ds.depthCompare = WGPUCompareFunction_Less;

    WGPURenderPipelineDescriptor pd = {};
    pd.layout = layout;
    pd.vertex.module = shader;
    pd.vertex.entryPoint = wgpu::Device::str("vs_main");
    pd.vertex.bufferCount = 1;
    pd.vertex.buffers = &vbl;
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pd.primitive.cullMode = WGPUCullMode_None; // the app has no backface culling
    pd.depthStencil = &ds;
    pd.multisample.count = 1;
    pd.multisample.mask = 0xFFFFFFFFu;
    pd.fragment = &frag;
    _meshPipeline = wgpuDeviceCreateRenderPipeline(_device, &pd);

    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(shader);
    if (!_meshPipeline) return false;

    // Shared linear sampler.
    WGPUSamplerDescriptor sd = {};
    sd.addressModeU = WGPUAddressMode_ClampToEdge;
    sd.addressModeV = WGPUAddressMode_ClampToEdge;
    sd.addressModeW = WGPUAddressMode_ClampToEdge;
    sd.magFilter = WGPUFilterMode_Linear;
    sd.minFilter = WGPUFilterMode_Linear;
    sd.mipmapFilter = WGPUMipmapFilterMode_Linear;
    sd.lodMaxClamp = 32.0f;
    sd.maxAnisotropy = 1;
    _sampler = wgpuDeviceCreateSampler(_device, &sd);

    // 1×1 white fallback so untextured materials still satisfy the bind group.
    WGPUTextureDescriptor wd = {};
    wd.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    wd.dimension = WGPUTextureDimension_2D;
    wd.size = { 1, 1, 1 };
    wd.format = WGPUTextureFormat_RGBA8Unorm;
    wd.mipLevelCount = 1; wd.sampleCount = 1;
    _whiteTex = wgpuDeviceCreateTexture(_device, &wd);
    _whiteView = wgpuTextureCreateView(_whiteTex, nullptr);
    const unsigned char white[4] = { 255, 255, 255, 255 };
    WGPUTexelCopyTextureInfo wdst = {};
    wdst.texture = _whiteTex; wdst.aspect = WGPUTextureAspect_All; wdst.origin = { 0, 0, 0 };
    WGPUTexelCopyBufferLayout wlay = {};
    wlay.bytesPerRow = 4; wlay.rowsPerImage = 1;
    WGPUExtent3D wsize = { 1, 1, 1 };
    wgpuQueueWriteTexture(_queue, &wdst, white, 4, &wlay, &wsize);

    // Flat-colour pipelines (overlay + wireframe).
    WGPUShaderSourceWGSL fsrc = {};
    fsrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    fsrc.code = wgpu::Device::str(kFlatWGSL);
    WGPUShaderModuleDescriptor fsmDesc = {};
    fsmDesc.nextInChain = &fsrc.chain;
    WGPUShaderModule flatShader = wgpuDeviceCreateShaderModule(_device, &fsmDesc);

    WGPUBindGroupLayoutEntry fbe = {};
    fbe.binding = 0;
    fbe.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    fbe.buffer.type = WGPUBufferBindingType_Uniform;
    fbe.buffer.minBindingSize = sizeof(FlatUniforms);
    WGPUBindGroupLayoutDescriptor fbgd = {};
    fbgd.entryCount = 1; fbgd.entries = &fbe;
    _flatBgl = wgpuDeviceCreateBindGroupLayout(_device, &fbgd);
    WGPUPipelineLayoutDescriptor fpld = {};
    fpld.bindGroupLayoutCount = 1; fpld.bindGroupLayouts = &_flatBgl;
    WGPUPipelineLayout flatLayout = wgpuDeviceCreatePipelineLayout(_device, &fpld);

    // Shader and layout are retained: flatPipeline() builds variants on demand.
    _flatShader = flatShader;
    _flatLayout = flatLayout;

    // ---- Textured screen-space pipeline (drawImage2D) ----
    WGPUShaderSourceWGSL isrc = {};
    isrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    isrc.code = wgpu::Device::str(kImageWGSL);
    WGPUShaderModuleDescriptor ismDesc = {};
    ismDesc.nextInChain = &isrc.chain;
    _imageShader = wgpuDeviceCreateShaderModule(_device, &ismDesc);

    WGPUBindGroupLayoutEntry ibe[3] = {};
    ibe[0].binding = 0;
    ibe[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    ibe[0].buffer.type = WGPUBufferBindingType_Uniform;
    ibe[0].buffer.minBindingSize = sizeof(FlatUniforms);
    ibe[1].binding = 1;
    ibe[1].visibility = WGPUShaderStage_Fragment;
    ibe[1].texture.sampleType = WGPUTextureSampleType_Float;
    ibe[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    ibe[2].binding = 2;
    ibe[2].visibility = WGPUShaderStage_Fragment;
    ibe[2].sampler.type = WGPUSamplerBindingType_Filtering;
    WGPUBindGroupLayoutDescriptor ibgd = {};
    ibgd.entryCount = 3; ibgd.entries = ibe;
    _imageBgl = wgpuDeviceCreateBindGroupLayout(_device, &ibgd);
    WGPUPipelineLayoutDescriptor ipld = {};
    ipld.bindGroupLayoutCount = 1; ipld.bindGroupLayouts = &_imageBgl;
    _imageLayout = wgpuDeviceCreatePipelineLayout(_device, &ipld);

    WGPUVertexAttribute iattrs[2] = {};
    iattrs[0].format = WGPUVertexFormat_Float32x3; iattrs[0].offset = 0;  iattrs[0].shaderLocation = 0;
    iattrs[1].format = WGPUVertexFormat_Float32x2; iattrs[1].offset = 12; iattrs[1].shaderLocation = 1;
    WGPUVertexBufferLayout ivbl = {};
    ivbl.stepMode = WGPUVertexStepMode_Vertex;
    ivbl.arrayStride = sizeof(ImageVertex);
    ivbl.attributeCount = 2; ivbl.attributes = iattrs;

    WGPUBlendState iblend = {};
    iblend.color.operation = WGPUBlendOperation_Add;
    iblend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    iblend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    iblend.alpha.operation = WGPUBlendOperation_Add;
    iblend.alpha.srcFactor = WGPUBlendFactor_One;
    iblend.alpha.dstFactor = WGPUBlendFactor_One;
    WGPUColorTargetState ict = {};
    ict.format = _colorFormat; ict.writeMask = WGPUColorWriteMask_All; ict.blend = &iblend;
    WGPUFragmentState ifrag = {};
    ifrag.module = _imageShader; ifrag.entryPoint = wgpu::Device::str("fs");
    ifrag.targetCount = 1; ifrag.targets = &ict;

    WGPUDepthStencilState ids = {};
    ids.format = WGPUTextureFormat_Depth24Plus;
    ids.depthWriteEnabled = WGPUOptionalBool_False;
    ids.depthCompare = WGPUCompareFunction_Always; // screen space: ignore depth
    WGPURenderPipelineDescriptor ipd = {};
    ipd.layout = _imageLayout;
    ipd.vertex.module = _imageShader; ipd.vertex.entryPoint = wgpu::Device::str("vs");
    ipd.vertex.bufferCount = 1; ipd.vertex.buffers = &ivbl;
    // Keep the fragment stage attached: without this assignment the pipeline
    // has zero color targets. It can still be returned by wgpu_native, but the
    // first drawImage2D command then aborts at encoder finish because the live
    // BGRA render pass and the target-less pipeline are incompatible.
    ipd.fragment = &ifrag;
    ipd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    ipd.primitive.cullMode = WGPUCullMode_None;
    ipd.depthStencil = &ids;
    ipd.multisample.count = 1; ipd.multisample.mask = 0xFFFFFFFFu;
    _imagePipe = wgpuDeviceCreateRenderPipeline(_device, &ipd);

    // Particle pipeline
    WGPUShaderSourceWGSL psrc = {};
    psrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    psrc.code = wgpu::Device::str(kParticleWGSL);
    WGPUShaderModuleDescriptor psmDesc = {};
    psmDesc.nextInChain = &psrc.chain;
    _particleShader = wgpuDeviceCreateShaderModule(_device, &psmDesc);
    WGPUBindGroupLayoutEntry pbgle = {};
    pbgle.binding = 0;
    pbgle.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    pbgle.buffer.type = WGPUBufferBindingType_Uniform;
    pbgle.buffer.minBindingSize = sizeof(ParticleUniforms);
    WGPUBindGroupLayoutDescriptor pbgld = {};
    pbgld.entryCount = 1; pbgld.entries = &pbgle;
    _particleBgl = wgpuDeviceCreateBindGroupLayout(_device, &pbgld);
    WGPUPipelineLayoutDescriptor ppld = {};
    ppld.bindGroupLayoutCount = 1; ppld.bindGroupLayouts = &_particleBgl;
    _particleLayout = wgpuDeviceCreatePipelineLayout(_device, &ppld);
    
    WGPURenderPipelineDescriptor ppd = {};
    ppd.layout = _particleLayout;
    ppd.vertex.module = _particleShader; ppd.vertex.entryPoint = wgpu::Device::str("vs");
    ppd.primitive.topology = WGPUPrimitiveTopology_PointList;
    WGPUBlendState pblend = {};
    pblend.color.operation = WGPUBlendOperation_Add;
    pblend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    pblend.color.dstFactor = WGPUBlendFactor_One;
    pblend.alpha.operation = WGPUBlendOperation_Add;
    pblend.alpha.srcFactor = WGPUBlendFactor_One;
    pblend.alpha.dstFactor = WGPUBlendFactor_One;
    WGPUColorTargetState pct = {};
    pct.format = _colorFormat; pct.writeMask = WGPUColorWriteMask_All; pct.blend = &pblend;
    WGPUFragmentState pfrag = {};
    pfrag.module = _particleShader; pfrag.entryPoint = wgpu::Device::str("fs");
    pfrag.targetCount = 1; pfrag.targets = &pct;
    WGPUDepthStencilState pds = {};
    pds.format = WGPUTextureFormat_Depth24Plus;
    pds.depthWriteEnabled = WGPUOptionalBool_False;
    pds.depthCompare = WGPUCompareFunction_Less;
    ppd.fragment = &pfrag;
    ppd.depthStencil = &pds;
    ppd.multisample.count = 1; ppd.multisample.mask = 0xFFFFFFFFu;
    _particlePipe = wgpuDeviceCreateRenderPipeline(_device, &ppd);

    _bufferPool.init(_device, _queue);
    _meshCache.init(_device, _queue);

    return _sampler && _whiteView && _flatShader && _flatLayout && _imagePipe && _particlePipe;
}

void WebGpuRenderer::releasePersistentSdfParams() {
    for (auto& kv : _persistentSdfParams) {
        if (kv.second.buffer) wgpuBufferRelease(kv.second.buffer);
    }
    _persistentSdfParams.clear();
    _persistentSdfParamVramBytes = 0;
}

void WebGpuRenderer::releasePersistentSdfRangeNodes() {
    for (auto& kv : _persistentSdfRangeNodes) {
        if (kv.second.buffer) wgpuBufferRelease(kv.second.buffer);
    }
    _persistentSdfRangeNodes.clear();
    _persistentSdfRangeNodeVramBytes = 0;
}

void WebGpuRenderer::releasePersistentRadianceSources() {
    if (_persistentRadianceSources.buffer) {
        wgpuBufferRelease(_persistentRadianceSources.buffer);
        _persistentRadianceSources.buffer = nullptr;
    }
    _persistentRadianceSources.capacityBytes = 0;
    _persistentRadianceSources.mirror.clear();
    _persistentRadianceSourceVramBytes = 0;
}

void WebGpuRenderer::reloadShaders() {
    // Keys are SdfPipeline addresses, so release these before destroying the
    // pipeline map whose node addresses identify the caches.
    releasePersistentSdfParams();
    releasePersistentSdfRangeNodes();
    releasePersistentRadianceSources();
    for (auto& kv : _sdfPipes) {
        if (kv.second.pipe) wgpuRenderPipelineRelease(kv.second.pipe);
        if (kv.second.bgl)  wgpuBindGroupLayoutRelease(kv.second.bgl);
    }
    _activeSdfPipelines.clear();
    _sdfBatches.clear();
    _sdfParamsBatches.clear();
    _sdfHeightGridBatches.clear();
    _sdfRangeNodeBatches.clear();
    _sdfPipes.clear();
    _programCache.clear();

    for (auto& kv : _volumePipes) {
        if (kv.second.pipe) wgpuRenderPipelineRelease(kv.second.pipe);
        if (kv.second.globalBgl) wgpuBindGroupLayoutRelease(kv.second.globalBgl);
        if (kv.second.instanceBgl) wgpuBindGroupLayoutRelease(kv.second.instanceBgl);
    }
    _volumePipes.clear();
    _volumeProgramCache.clear();
    _volumeSetProgramCache.clear();
    _volumeBatches.clear();
    _volumeParamBatches.clear();
    _volumeDrawInstanceCounts.clear();
    _activeVolumePipelines.clear();
}

// Build-on-first-use so only the combinations the app actually draws exist.
WGPURenderPipeline WebGpuRenderer::flatPipeline(WGPUPrimitiveTopology topo, Blend blend,
                                                DepthMode depth) {
    const FlatKey key{topo, blend, depth};
    auto it = _flatPipes.find(key);
    if (it != _flatPipes.end()) return it->second;

    WGPUBlendFactor src = WGPUBlendFactor_SrcAlpha, dst = WGPUBlendFactor_OneMinusSrcAlpha;
    switch (blend) {
        case Blend::Opaque:   src = WGPUBlendFactor_One;      dst = WGPUBlendFactor_Zero; break;
        case Blend::Alpha:    src = WGPUBlendFactor_SrcAlpha; dst = WGPUBlendFactor_OneMinusSrcAlpha; break;
        case Blend::Additive: src = WGPUBlendFactor_SrcAlpha; dst = WGPUBlendFactor_One; break;
    }
    WGPURenderPipeline pipe = makeFlatPipeline(
        _device, _flatLayout, _flatShader, topo, src, dst, _colorFormat,
        /*depthWrite=*/depth == DepthMode::TestWrite,
        /*depthTest =*/depth != DepthMode::None);
    _flatPipes[key] = pipe;
    return pipe;
}

void WebGpuRenderer::shutdown() {
    releaseGpuTimestampQueries();
    releasePersistentSdfParams();
    releasePersistentSdfRangeNodes();
    releasePersistentRadianceSources();
    _meshCache.shutdown();
    _bufferPool.shutdown();
    releaseFrameResources();
    if (_depthView) { wgpuTextureViewRelease(_depthView); _depthView = nullptr; }
    if (_depthTex)  { wgpuTextureRelease(_depthTex); _depthTex = nullptr; }
    _depthW = _depthH = 0;
    if (_whiteView) { wgpuTextureViewRelease(_whiteView); _whiteView = nullptr; }
    if (_whiteTex)  { wgpuTextureRelease(_whiteTex); _whiteTex = nullptr; }
    if (_sampler)   { wgpuSamplerRelease(_sampler); _sampler = nullptr; }
    for (auto& kv : _sdfPipes) {
        if (kv.second.pipe) wgpuRenderPipelineRelease(kv.second.pipe);
        if (kv.second.bgl)  wgpuBindGroupLayoutRelease(kv.second.bgl);
    }
    _sdfPipes.clear();
    for (auto& kv : _volumePipes) {
        if (kv.second.pipe) wgpuRenderPipelineRelease(kv.second.pipe);
        if (kv.second.globalBgl) wgpuBindGroupLayoutRelease(kv.second.globalBgl);
        if (kv.second.instanceBgl) wgpuBindGroupLayoutRelease(kv.second.instanceBgl);
    }
    _volumePipes.clear();
    _volumeProgramCache.clear();
    _volumeSetProgramCache.clear();
    _volumeBatches.clear();
    _volumeParamBatches.clear();
    _volumeDrawInstanceCounts.clear();
    _activeVolumePipelines.clear();
    if (_sdfCubeVerts) { wgpuBufferRelease(_sdfCubeVerts); _sdfCubeVerts = nullptr; }
    for (auto& kv : _textures) {
        wgpuTextureViewRelease(kv.second.view);
        wgpuTextureRelease(kv.second.tex);
    }
    _textures.clear();
    for (auto& kv : _flatPipes) wgpuRenderPipelineRelease(kv.second);
    _flatPipes.clear();
    if (_flatLayout)  { wgpuPipelineLayoutRelease(_flatLayout); _flatLayout = nullptr; }
    if (_flatShader)  { wgpuShaderModuleRelease(_flatShader); _flatShader = nullptr; }
    if (_flatBgl)     { wgpuBindGroupLayoutRelease(_flatBgl); _flatBgl = nullptr; }
    if (_imagePipe)   { wgpuRenderPipelineRelease(_imagePipe); _imagePipe = nullptr; }
    if (_imageLayout) { wgpuPipelineLayoutRelease(_imageLayout); _imageLayout = nullptr; }
    if (_imageShader) { wgpuShaderModuleRelease(_imageShader); _imageShader = nullptr; }
    if (_imageBgl)    { wgpuBindGroupLayoutRelease(_imageBgl); _imageBgl = nullptr; }
    if (_meshPipeline) { wgpuRenderPipelineRelease(_meshPipeline); _meshPipeline = nullptr; }
    if (_bgl) { wgpuBindGroupLayoutRelease(_bgl); _bgl = nullptr; }
    if (_instanceBgl) { wgpuBindGroupLayoutRelease(_instanceBgl); _instanceBgl = nullptr; }
    _meshBatches.clear();
    if (_readbackBuffer) {
        wgpuBufferRelease(_readbackBuffer);
        _readbackBuffer = nullptr;
        _readbackBufferSize = 0;
    }
}

bool WebGpuRenderer::initGpuTimestampQueries(bool deviceCapability) {
    if (!deviceCapability || !_device || !_queue || !_instance) return false;

    WGPUQuerySetDescriptor qd = {};
    qd.type = WGPUQueryType_Timestamp;
    qd.count = 2;
    _gpuTimestampQuerySet = wgpuDeviceCreateQuerySet(_device, &qd);
    if (!_gpuTimestampQuerySet) return false;

    for (auto& slot : _gpuTimestampSlots) {
        WGPUBufferDescriptor resolveDesc = {};
        resolveDesc.size = 2 * sizeof(uint64_t);
        resolveDesc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
        slot.resolve = wgpuDeviceCreateBuffer(_device, &resolveDesc);

        WGPUBufferDescriptor readbackDesc = {};
        readbackDesc.size = 2 * sizeof(uint64_t);
        readbackDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        slot.readback = wgpuDeviceCreateBuffer(_device, &readbackDesc);
        if (!slot.resolve || !slot.readback) {
            releaseGpuTimestampQueries();
            return false;
        }
    }

#ifdef __EMSCRIPTEN__
    _gpuTimestampPeriodNs = 1.0f;
#else
    _gpuTimestampPeriodNs = wgpuQueueGetTimestampPeriod(_queue);
#endif
    if (!std::isfinite(_gpuTimestampPeriodNs) || _gpuTimestampPeriodNs <= 0.0f) {
        releaseGpuTimestampQueries();
        return false;
    }
    _gpuTimestampQueriesEnabled = true;
    return true;
}

void WebGpuRenderer::releaseGpuTimestampQueries() {
    for (auto& slot : _gpuTimestampSlots) {
        if (slot.mapReady && slot.mapStatus == WGPUMapAsyncStatus_Success && slot.readback) {
            wgpuBufferUnmap(slot.readback);
        }
        if (slot.readback) { wgpuBufferRelease(slot.readback); slot.readback = nullptr; }
        if (slot.resolve)  { wgpuBufferRelease(slot.resolve); slot.resolve = nullptr; }
        slot = GpuTimestampSlot{};
    }
    if (_gpuTimestampQuerySet) {
        wgpuQuerySetRelease(_gpuTimestampQuerySet);
        _gpuTimestampQuerySet = nullptr;
    }
    _gpuTimestampQueriesEnabled = false;
    _timestampSlotForFrame = -1;
    _hasGpuMainPassTiming = false;
}

void WebGpuRenderer::onGpuTimestampMap(WGPUMapAsyncStatus status, WGPUStringView,
                                       void* userdata1, void*) {
    auto* slot = static_cast<GpuTimestampSlot*>(userdata1);
    if (!slot) return;
    slot->mapStatus = status;
    slot->mapReady = true;
}

void WebGpuRenderer::collectGpuTimestampResults() {
    if (!_gpuTimestampQueriesEnabled || !_instance) return;
    // AllowProcessEvents callbacks are delivered here on the rendering thread.
    // No wait/poll is permitted: a late sample is simply displayed next frame.
    wgpuInstanceProcessEvents(_instance);
    for (auto& slot : _gpuTimestampSlots) {
        if (!slot.mapReady) continue;
        slot.mapReady = false;
        slot.mapPending = false;
        if (slot.mapStatus != WGPUMapAsyncStatus_Success || !slot.readback) continue;

        const void* mapped = wgpuBufferGetMappedRange(slot.readback, 0, 2 * sizeof(uint64_t));
        if (mapped) {
            uint64_t timestamps[2] = {};
            std::memcpy(timestamps, mapped, sizeof(timestamps));
            if (timestamps[1] >= timestamps[0]) {
                const double ns = static_cast<double>(timestamps[1] - timestamps[0]) *
                                  static_cast<double>(_gpuTimestampPeriodNs);
                const double ms = ns / 1.0e6;
                if (std::isfinite(ms) && ms >= 0.0 && ms <= 60000.0) {
                    _latestGpuMainPassMs = static_cast<float>(ms);
                    _hasGpuMainPassTiming = true;
                }
            }
        }
        wgpuBufferUnmap(slot.readback);
    }
}

void WebGpuRenderer::beginGpuTimestampFrame() {
    _timestampSlotForFrame = -1;
    if (!_gpuTimestampQueriesEnabled || !_encoder) return;
    for (size_t offset = 0; offset < _gpuTimestampSlots.size(); ++offset) {
        const size_t candidate = (_nextTimestampSlot + offset) % _gpuTimestampSlots.size();
        if (_gpuTimestampSlots[candidate].mapPending || _gpuTimestampSlots[candidate].mapReady) continue;
        _timestampSlotForFrame = static_cast<int>(candidate);
        _nextTimestampSlot = (candidate + 1) % _gpuTimestampSlots.size();
        wgpuCommandEncoderWriteTimestamp(_encoder, _gpuTimestampQuerySet, 0);
        return;
    }
}

void WebGpuRenderer::endGpuTimestampFrame() {
    if (_timestampSlotForFrame < 0 || !_encoder || !_gpuTimestampQuerySet) return;
    auto& slot = _gpuTimestampSlots[static_cast<size_t>(_timestampSlotForFrame)];
    wgpuCommandEncoderWriteTimestamp(_encoder, _gpuTimestampQuerySet, 1);
    wgpuCommandEncoderResolveQuerySet(_encoder, _gpuTimestampQuerySet, 0, 2, slot.resolve, 0);
    wgpuCommandEncoderCopyBufferToBuffer(
        _encoder, slot.resolve, 0, slot.readback, 0, 2 * sizeof(uint64_t));
}

void WebGpuRenderer::ensureDepth(uint32_t w, uint32_t h) {
    if (_depthTex && _depthW == w && _depthH == h) return;
    if (_depthView) { wgpuTextureViewRelease(_depthView); _depthView = nullptr; }
    if (_depthTex)  { wgpuTextureRelease(_depthTex); _depthTex = nullptr; }
    WGPUTextureDescriptor td = {};
    // V0c composites participating media in a second pass that samples the
    // finished opaque depth. One texture serves both roles across distinct
    // passes on the same command encoder; it is never sampled while attached.
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { w, h, 1 };
    td.format = WGPUTextureFormat_Depth24Plus;
    td.mipLevelCount = 1; td.sampleCount = 1;
    _depthTex = wgpuDeviceCreateTexture(_device, &td);
    _depthView = wgpuTextureCreateView(_depthTex, nullptr);
    _depthW = w; _depthH = h;
}

void WebGpuRenderer::applyBeginFrame(uint32_t width, uint32_t height, const glm::vec4& clear) {
    // Offscreen users call beginFrameOffscreen with an explicit target, so with no
    // surface attached there is nothing to open — and every draw verb no-ops on a
    // null pass, so the frame is simply skipped rather than being an error.
    if (!_surface || width == 0 || height == 0) return;

    WGPUSurfaceTexture st = {};
    wgpuSurfaceGetCurrentTexture(_surface, &st);
    if (st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        // Lost/outdated (usually a resize that raced this frame). Drop the frame;
        // the caller reconfigures and the next one succeeds.
        return;
    }
    _surfaceTex  = st.texture;
    _surfaceView = wgpuTextureCreateView(st.texture, nullptr);

    // Same pass setup as the offscreen path, against the acquired view.
    beginFrameOffscreen(_surfaceView, width, height, clear);
}

void WebGpuRenderer::overlayPass(const std::function<void(WGPURenderPassEncoder)>& record) {
    if (!_surfaceView || !record) return;

    // LoadOp_Load, not Clear: this pass draws ON TOP of the scene. No depth
    // attachment at all — the overlay is 2D and must never be occluded by world
    // geometry, and imgui's pipelines are built without a depth-stencil state.
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(_device, nullptr);
    WGPURenderPassColorAttachment ca = {};
    ca.view = _surfaceView;
    ca.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ca.loadOp = WGPULoadOp_Load;
    ca.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor rp = {};
    rp.colorAttachmentCount = 1;
    rp.colorAttachments = &ca;
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(enc, &rp);

    record(pass);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(_queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);
}

void WebGpuRenderer::present() {
    if (!_surface || !_surfaceView) return;
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(_surface);
#endif
    wgpuTextureViewRelease(_surfaceView);
    _surfaceView = nullptr;
    _surfaceTex = nullptr; // owned by the surface; not ours to release
}

void WebGpuRenderer::beginFrameOffscreen(WGPUTextureView target, uint32_t width, uint32_t height,
                                         const glm::vec4& clear) {
    mutableFrameStats() = FrameStats{};
    collectGpuTimestampResults();
    mutableFrameStats().gpuMainPassTimingSupported = _gpuTimestampQueriesEnabled;
    mutableFrameStats().gpuMainPassTimingValid = _hasGpuMainPassTiming;
    mutableFrameStats().gpuMainPassMs = _latestGpuMainPassMs;
    _frameCount++;
    _meshCache.beginFrame(_frameCount);
    ensureDepth(width, height);
    _encoder = wgpuDeviceCreateCommandEncoder(_device, nullptr);
    _frameColorView = target;
    beginGpuTimestampFrame();
    WGPURenderPassColorAttachment ca = {};
    ca.view = target;
    ca.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ca.loadOp = WGPULoadOp_Clear;
    ca.storeOp = WGPUStoreOp_Store;
    ca.clearValue = { clear.r, clear.g, clear.b, clear.a };
    WGPURenderPassDepthStencilAttachment da = {};
    da.view = _depthView;
    da.depthLoadOp = WGPULoadOp_Clear;
    da.depthStoreOp = WGPUStoreOp_Store;
    da.depthClearValue = 1.0f;
    WGPURenderPassDescriptor rp = {};
    rp.colorAttachmentCount = 1;
    rp.colorAttachments = &ca;
    rp.depthStencilAttachment = &da;
    _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &rp);
    // Each verb ASKS for its pipeline via bindPipeline (mesh vs overlay vs
    // lines), which binds only on an actual change — the pass may interleave
    // meshes and overlays per object. The cache starts empty every pass: a new
    // pass carries no binding from the one before it.
    _boundPipeline = nullptr;
    // Defensive: a normal frame's batches are drained by flushMeshDraws() in
    // endFrame() and this is already empty. Only a frame that never reached
    // endFrame() (an early return, a crash-recovery path) would leave stale
    // entries — clear rather than carry them into a frame with a new _pass.
    _meshBatches.clear();
}

void WebGpuRenderer::drawMesh(const geom::TessMesh& mesh, const RenderMaterial& mat) {
    if (!_pass || mesh.tris.empty()) return;

    // setWireframe: GL had glPolygonMode to draw the same triangles as edges.
    // WebGPU has no such state, and reinterpreting a triangle list as a line list
    // would connect the wrong vertices — so the edges are built explicitly.
    if (_wireframe) {
        std::vector<glm::vec3> edges;
        edges.reserve(mesh.tris.size() * 2);
        for (size_t i = 0; i + 2 < mesh.tris.size(); i += 3) {
            const glm::vec3& a = mesh.tris[i].pos;
            const glm::vec3& b = mesh.tris[i + 1].pos;
            const glm::vec3& c = mesh.tris[i + 2].pos;
            edges.push_back(a); edges.push_back(b);
            edges.push_back(b); edges.push_back(c);
            edges.push_back(c); edges.push_back(a);
        }
        drawFlat(flatPipeline(WGPUPrimitiveTopology_LineList, Blend::Alpha, DepthMode::TestOnly),
                 edges, _viewProj * _model,
                 glm::vec4(mat.baseColor, mat.opacity));
        return;
    }
    // Albedo is resolved NOW, not deferred: a queued batch cannot hold
    // RenderMaterial::albedoPixels (documented "valid only for the duration
    // of the draw call"), and resolving to a stable WGPUTextureView also
    // doubles as the batching key's material identity — see MeshBatchKey.
    WGPUTextureView albedoView = _whiteView;
    // Prefer a texture this backend already owns: FaceTexture re-uploads only when
    // the paint changes, so a static surface costs nothing per frame. The
    // albedoPixels path below is the fallback for callers that hold no handle.
    auto owned = _textures.find(mat.textureId);
    if (mat.textureId != 0 && owned != _textures.end()) {
        albedoView = owned->second.view;
    } else if (mat.albedoPixels && mat.albedoWidth > 0 && mat.albedoHeight > 0) {
        const uint32_t w = static_cast<uint32_t>(mat.albedoWidth);
        const uint32_t h = static_cast<uint32_t>(mat.albedoHeight);
        WGPUTextureDescriptor atd = {};
        atd.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        atd.dimension = WGPUTextureDimension_2D;
        atd.size = { w, h, 1 };
        atd.format = WGPUTextureFormat_RGBA8Unorm;
        atd.mipLevelCount = 1; atd.sampleCount = 1;
        WGPUTexture atex = wgpuDeviceCreateTexture(_device, &atd);
        WGPUTexelCopyTextureInfo adst = {};
        adst.texture = atex; adst.aspect = WGPUTextureAspect_All; adst.origin = { 0, 0, 0 };
        WGPUTexelCopyBufferLayout alay = {};
        alay.bytesPerRow = w * 4; alay.rowsPerImage = h;
        WGPUExtent3D asize = { w, h, 1 };
        wgpuQueueWriteTexture(_queue, &adst, mat.albedoPixels, size_t(w) * h * 4, &alay, &asize);
        albedoView = wgpuTextureCreateView(atex, nullptr);
        _frameTextures.push_back(atex);
        _frameTextureViews.push_back(albedoView);
    }

    MeshBatchKey key;
    key.mesh       = &mesh;
    key.albedoView = albedoView;
    key.shading    = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);

    InstanceData inst;
    inst.model     = _model;
    inst.normalMat = glm::transpose(glm::inverse(_model));
    inst.baseColor = glm::vec4(mat.baseColor, mat.opacity);
    _meshBatches[key].push_back(inst);

    // Every queued instance really will be drawn at flush — count its
    // triangles now. drawCalls/pipelineSwitches are credited once per BATCH
    // in flushMeshDraws(), which is the actual number of GPU draw calls.
    mutableFrameStats().trianglesDrawn += static_cast<uint32_t>(mesh.tris.size() / 3);
}

void WebGpuRenderer::flushMeshDraws() {
    if (_meshBatches.empty()) return;
    if (!_pass) { _meshBatches.clear(); return; }

    for (auto& kv : _meshBatches) {
        const MeshBatchKey& key = kv.first;
        const std::vector<InstanceData>& instances = kv.second;
        if (!key.mesh || key.mesh->tris.empty() || instances.empty()) continue;
        const geom::TessMesh& mesh = *key.mesh;

        bindPipeline(_meshPipeline);

        const size_t vbytes = mesh.tris.size() * sizeof(geom::TessVertex);
        WGPUBuffer vbuf = _meshCache.getOrUpload(mesh);
        uint64_t voffset = 0;
        if (!vbuf) {
            auto vAlloc = bufferPool().suballocateVertex(mesh.tris.data(), vbytes);
            vbuf = vAlloc.buffer;
            voffset = vAlloc.offset;
        }

        MeshUniforms u;
        u.viewProj  = _viewProj;
        u.lightPos  = glm::vec4(lightPos(), 1.0f);
        u.params    = key.shading;
        u.eyePos    = glm::vec4(_eyePos, 1.0f);
        auto uAlloc = bufferPool().suballocateUniform(&u, sizeof(MeshUniforms));

        auto instAlloc = bufferPool().suballocateStorage(
            instances.data(), instances.size() * sizeof(InstanceData));

        WGPUBindGroupEntry bge[3] = {};
        bge[0].binding = 0; bge[0].buffer = uAlloc.buffer; bge[0].offset = uAlloc.offset; bge[0].size = uAlloc.size;
        bge[1].binding = 1; bge[1].textureView = key.albedoView;
        bge[2].binding = 2; bge[2].sampler = _sampler;
        WGPUBindGroupDescriptor bgDesc = {};
        bgDesc.layout = _bgl; bgDesc.entryCount = 3; bgDesc.entries = bge;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(_device, &bgDesc);
        _frameBindGroups.push_back(bindGroup);

        WGPUBindGroupEntry ibge = {};
        ibge.binding = 0; ibge.buffer = instAlloc.buffer; ibge.offset = instAlloc.offset; ibge.size = instAlloc.size;
        WGPUBindGroupDescriptor ibgDesc = {};
        ibgDesc.layout = _instanceBgl; ibgDesc.entryCount = 1; ibgDesc.entries = &ibge;
        WGPUBindGroup instBindGroup = wgpuDeviceCreateBindGroup(_device, &ibgDesc);
        _frameBindGroups.push_back(instBindGroup);

        wgpuRenderPassEncoderSetBindGroup(_pass, 0, bindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(_pass, 1, instBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vbuf, voffset, vbytes);
        wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(mesh.tris.size()),
                                  static_cast<uint32_t>(instances.size()), 0, 0);

        mutableFrameStats().drawCalls++;
        mutableFrameStats().meshDrawCalls++;
    }
    _meshBatches.clear();
}

namespace {
// Uniform block for the raymarcher; must match struct RU in the generated WGSL.
struct SdfGlobalUniforms {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 lightPos;
    glm::vec4 eyePos;
    glm::vec4 lightAmbient;
    glm::vec4 lightDiffuse;
    glm::vec4 lightSpecular;
    glm::vec4 lightControl; // x = lighting enabled, y = derived visibility enabled
    glm::vec4 radianceSourceCoefficients; // intensity, ambient, diffuse, specular
    glm::vec4 limits;       // x = far-plane distance, y = screen width, z = screen height, w = spaceDistortion
    glm::vec4 radianceTime; // x/y = admitted radiance-source coordinate/delta, z/w reserved
    glm::vec4 volumeTime;   // x/y = admitted participating-medium coordinate/delta
};

struct RadianceSourceGpuData {
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 coefficients;
    glm::vec4 time;
    glm::vec4 control;
};

struct VolumeGlobalUniforms {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 eyePos;
    glm::vec4 viewport;
    // xyz = exactly one enabled admitted direct source; w=1 iff valid.
    glm::vec4 incidentSource;
    glm::vec4 incidentColor;
    glm::vec4 sourceTime;
    glm::vec4 volumeControl;
};
} // namespace

void WebGpuRenderer::ensureSdfCubeVerts() {
    if (_sdfCubeVerts) return;

    const float h = 1.0f;
    const glm::vec3 corners[8] = {
        {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
        {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}};
    const int indices[36] = {
        0,1,2, 0,2,3,  4,6,5, 4,7,6,  0,4,5, 0,5,1,
        3,2,6, 3,6,7,  0,3,7, 0,7,4,  1,5,6, 1,6,2};
    std::vector<glm::vec3> tris(36);
    for (int i = 0; i < 36; ++i) tris[i] = corners[indices[i]];

    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    bd.size = tris.size() * sizeof(glm::vec3);
    _sdfCubeVerts = wgpuDeviceCreateBuffer(_device, &bd);
    if (_sdfCubeVerts) {
        wgpuQueueWriteBuffer(_queue, _sdfCubeVerts, 0, tris.data(), bd.size);
    }
}

// Build (or fetch) the pipeline for one field SHAPE. The generated WGSL is the
// cache key: two spheres of different radii generate identical source and share
// this pipeline, differing only in their parameter buffer.
const WebGpuRenderer::SdfPipeline* WebGpuRenderer::sdfPipeline(const std::string& wgsl) {
    auto it = _sdfPipes.find(wgsl);
    if (it != _sdfPipes.end()) return &it->second;

    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(wgsl.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(_device, &smd);
    if (!shader) {
        std::fprintf(stderr, "[WebGpuRenderer] SDF shader failed to compile\n");
        return nullptr;
    }

    const bool usesRadianceSources =
        wgsl.find("@group(0) @binding(2) var<storage, read> RS") != std::string::npos;
    WGPUBindGroupLayoutEntry be[3] = {};
    be[0].binding = 0;
    be[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    be[0].buffer.type = WGPUBufferBindingType_Uniform;
    be[0].buffer.minBindingSize = sizeof(SdfGlobalUniforms);
    be[1].binding = 1;
    be[1].visibility = WGPUShaderStage_Fragment;
    be[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    if (usesRadianceSources) {
        be[2].binding = 2;
        be[2].visibility = WGPUShaderStage_Fragment;
        be[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    }
    WGPUBindGroupLayoutDescriptor bgld = {};
    bgld.entryCount = usesRadianceSources ? 3 : 2;
    bgld.entries = be;

    SdfPipeline out;
    out.usesRadianceSources = usesRadianceSources;
    out.bgl = wgpuDeviceCreateBindGroupLayout(_device, &bgld);
    WGPUBindGroupLayout meshLayouts[2] = { out.bgl, _sdfInstanceBgl };
    WGPUPipelineLayoutDescriptor pld = {};
    pld.bindGroupLayoutCount = 2;
    pld.bindGroupLayouts = meshLayouts;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &pld);

    WGPUVertexAttribute attr = {};
    attr.format = WGPUVertexFormat_Float32x3; attr.offset = 0; attr.shaderLocation = 0;
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex; vbl.arrayStride = 12;
    vbl.attributeCount = 1; vbl.attributes = &attr;

    WGPUBlendState blend = {};
    blend.color.operation = WGPUBlendOperation_Add;
    blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    WGPUColorTargetState ct = {};
    ct.format = _colorFormat; ct.writeMask = WGPUColorWriteMask_All; ct.blend = &blend;
    WGPUFragmentState frag = {};
    frag.module = shader; frag.entryPoint = wgpu::Device::str("fs");
    frag.targetCount = 1; frag.targets = &ct;

    // Depth WRITE is on and the fragment shader supplies frag_depth from the true
    // hit, so a field interleaves with meshes correctly rather than by its box.
    WGPUDepthStencilState ds = {};
    ds.format = WGPUTextureFormat_Depth24Plus;
    ds.depthWriteEnabled = WGPUOptionalBool_True;
    ds.depthCompare = WGPUCompareFunction_Less;

    WGPURenderPipelineDescriptor pd = {};
    pd.layout = layout;
    pd.vertex.module = shader; pd.vertex.entryPoint = wgpu::Device::str("vs");
    pd.vertex.bufferCount = 1; pd.vertex.buffers = &vbl;
    // The proxy is inward-wound. Back-face culling retains the covering face
    // for both inside and outside cameras, so one fragment program traces each
    // analytic ray. The ray/AABB interval remains authoritative.
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pd.primitive.cullMode = WGPUCullMode_Back;
    pd.depthStencil = &ds;
    pd.multisample.count = 1; pd.multisample.mask = 0xFFFFFFFFu;
    pd.fragment = &frag;

    out.pipe = wgpuDeviceCreateRenderPipeline(_device, &pd);

    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(shader);
    if (!out.pipe) {
        wgpuBindGroupLayoutRelease(out.bgl);
        return nullptr;
    }
    return &(_sdfPipes[wgsl] = out);
}

const WebGpuRenderer::VolumePipeline*
WebGpuRenderer::volumePipeline(const std::string& wgsl) {
    auto it = _volumePipes.find(wgsl);
    if (it != _volumePipes.end()) return &it->second;

    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(wgsl.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(_device, &smd);
    if (!shader) {
        std::fprintf(stderr, "[WebGpuRenderer] volume shader failed to compile\n");
        return nullptr;
    }

    WGPUBindGroupLayoutEntry globalEntries[3] = {};
    globalEntries[0].binding = 0;
    globalEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    globalEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    globalEntries[0].buffer.minBindingSize = sizeof(VolumeGlobalUniforms);

    globalEntries[1].binding = 1;
    globalEntries[1].visibility = WGPUShaderStage_Fragment;
    globalEntries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;

    globalEntries[2].binding = 2;
    globalEntries[2].visibility = WGPUShaderStage_Fragment;
    globalEntries[2].texture.sampleType = WGPUTextureSampleType_Depth;
    globalEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
    globalEntries[2].texture.multisampled = false;

    WGPUBindGroupLayoutDescriptor globalDesc = {};
    globalDesc.entryCount = 3;
    globalDesc.entries = globalEntries;

    WGPUBindGroupLayoutEntry instanceEntry = {};
    instanceEntry.binding = 0;
    instanceEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    instanceEntry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    instanceEntry.buffer.minBindingSize = sizeof(VolumeInstanceData);
    WGPUBindGroupLayoutDescriptor instanceDesc = {};
    instanceDesc.entryCount = 1;
    instanceDesc.entries = &instanceEntry;

    VolumePipeline out;
    out.globalBgl = wgpuDeviceCreateBindGroupLayout(_device, &globalDesc);
    out.instanceBgl = wgpuDeviceCreateBindGroupLayout(_device, &instanceDesc);
    if (!out.globalBgl || !out.instanceBgl) {
        if (out.globalBgl) wgpuBindGroupLayoutRelease(out.globalBgl);
        if (out.instanceBgl) wgpuBindGroupLayoutRelease(out.instanceBgl);
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUBindGroupLayout layouts[2] = {out.globalBgl, out.instanceBgl};
    WGPUPipelineLayoutDescriptor pld = {};
    pld.bindGroupLayoutCount = 2;
    pld.bindGroupLayouts = layouts;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &pld);

    WGPUVertexAttribute attr = {};
    attr.format = WGPUVertexFormat_Float32x3;
    attr.offset = 0;
    attr.shaderLocation = 0;
    WGPUVertexBufferLayout vbl = {};
    vbl.arrayStride = sizeof(glm::vec3);
    vbl.stepMode = WGPUVertexStepMode_Vertex;
    vbl.attributeCount = 1;
    vbl.attributes = &attr;

    WGPUBlendState blend = {};
    blend.color.operation = WGPUBlendOperation_Add;
    // V0 returns analytically integrated medium radiance in premultiplied form.
    // Preserve it directly so blending computes C_out = C_medium + T * C_scene.
    blend.color.srcFactor = WGPUBlendFactor_One;
    blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_One;
    blend.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

    WGPUColorTargetState target = {};
    target.format = _colorFormat;
    target.writeMask = WGPUColorWriteMask_All;
    target.blend = &blend;

    WGPUFragmentState frag = {};
    frag.module = shader;
    frag.entryPoint = wgpu::Device::str("fs");
    frag.targetCount = 1;
    frag.targets = &target;

    WGPURenderPipelineDescriptor pd = {};
    pd.layout = layout;
    pd.vertex.module = shader;
    pd.vertex.entryPoint = wgpu::Device::str("vs");
    pd.vertex.bufferCount = 1;
    pd.vertex.buffers = &vbl;
    pd.fragment = &frag;
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    // Same inward-wound unit cube used by the SDF proxy. Back-face culling
    // leaves one covering face for inside and outside camera positions without
    // double-compositing the same medium ray.
    pd.primitive.cullMode = WGPUCullMode_Back;
    pd.multisample.count = 1;
    pd.multisample.mask = 0xFFFFFFFFu;
    // Deliberately NO depthStencil attachment/state. The shader samples the
    // completed opaque depth texture and never claims frag_depth itself.

    out.pipe = wgpuDeviceCreateRenderPipeline(_device, &pd);

    wgpuPipelineLayoutRelease(layout);
    wgpuShaderModuleRelease(shader);

    if (!out.pipe) {
        wgpuBindGroupLayoutRelease(out.globalBgl);
        wgpuBindGroupLayoutRelease(out.instanceBgl);
        return nullptr;
    }

    return &(_volumePipes[wgsl] = out);
}

void WebGpuRenderer::drawImplicit(const geom::SdfNode& field, const glm::vec3& extent,
                                  const RenderMaterial& mat,
                                  const geom::FieldNode* fieldNode,
                                  uint64_t memoId,
                                  uint32_t memoRevision,
                                  const geom::HeightGrid* heightGrid,
                                  uint32_t memoParameterRevision) {
    if (!_pass) return;

    // Memoize WGSL generation and pipeline lookup. A cache hit must be an
    // O(1)-ish reference acquisition, not a copy of the complete WGSL string and
    // parameter vector. Keep a local Program only for uncached/compile-miss work.
    sdfwgsl::Program localProg;
    const sdfwgsl::Program* prog = nullptr;
    const SdfPipeline* sp = nullptr;
    bool isProvenHeightfield = false;
    bool needsCompile = true;

    // Radiance has the same structure/value split as geometry. Rung 7 keeps
    // that rule for a COLLECTION: source membership/count and emitted AST shape
    // are shader structure; source positions, colors, enablement, temporal
    // coordinates and numeric AST coefficients are values.
    const bool multiSource = radianceSources().size() > 1;
    const auto* sourceSet = multiSource ? &radianceSources() : nullptr;

    // Explicit drawImplicit(..., FieldNode*) is the bounded/native V0 witness
    // seam. Production Zone media are projected separately and are NOT consumed
    // here until depth-aware volume composition exists. When this explicit seam
    // is used, volume.density.ast outranks the legacy generic ScalarField inside
    // sdfwgsl::compile/collectParams.
    const OntoMath::Piecewise* densityExpr =
        (fieldNode && fieldNode->volumeDensity &&
         !fieldNode->volumeDensity->pieces.empty())
            ? fieldNode->volumeDensity.get()
            : nullptr;
    const OntoMath::Piecewise* extinctionExpr =
        (fieldNode && fieldNode->volumeExtinction &&
         !fieldNode->volumeExtinction->pieces.empty())
            ? fieldNode->volumeExtinction.get()
            : nullptr;
    const OntoMath::Piecewise* scatteringExpr =
        (fieldNode && fieldNode->volumeScattering &&
         !fieldNode->volumeScattering->pieces.empty())
            ? fieldNode->volumeScattering.get()
            : nullptr;
    const OntoMath::Piecewise* volumeChromaExpr =
        (fieldNode && fieldNode->volumeChroma &&
         !fieldNode->volumeChroma->pieces.empty())
            ? fieldNode->volumeChroma.get()
            : nullptr;
    const OntoMath::Piecewise* phaseExpr =
        (fieldNode && fieldNode->volumePhase &&
         !fieldNode->volumePhase->pieces.empty())
            ? fieldNode->volumePhase.get()
            : nullptr;
    const OntoMath::Piecewise* emissionExpr =
        (fieldNode && fieldNode->volumeEmission &&
         !fieldNode->volumeEmission->pieces.empty())
            ? fieldNode->volumeEmission.get()
            : nullptr;

    sdfwgsl::DensityInputKind densityKind = sdfwgsl::DensityInputKind::LegacyField;
    if (densityExpr) {
        densityKind = sdfwgsl::DensityInputKind::Authored;
    } else if (fieldNode) {
        Rendering::AuthorableLightState authoredLight;
        if (Rendering::readAuthorableLight(*fieldNode, authoredLight)) {
            // rho is source truth, never an implicit participating-medium fallback.
            densityKind = sdfwgsl::DensityInputKind::None;
        }
    }

    uint64_t densityRevision = 0;
    sdfwgsl::ScalarExpressionLayout densityLayout;
    std::string densityStructure =
        densityKind == sdfwgsl::DensityInputKind::None
            ? "<density:none>"
            : "<density:legacy-field>";
    if (densityKind == sdfwgsl::DensityInputKind::Authored) {
        const std::string densityJson = densityExpr->toJson().dump();
        densityRevision = static_cast<uint64_t>(std::hash<std::string>{}(densityJson));
        densityLayout = sdfwgsl::inspectDensityExpression(densityExpr);
        densityStructure = "<density:authored>\n" + densityLayout.structure;
    }

    uint64_t extinctionRevision = 0;
    const auto extinctionLayout = sdfwgsl::inspectExtinctionExpression(extinctionExpr);
    const std::string extinctionStructure = extinctionLayout.structure;
    if (extinctionExpr) {
        const std::string extinctionJson = extinctionExpr->toJson().dump();
        extinctionRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(extinctionJson));
    }

    uint64_t scatteringRevision = 0;
    const auto scatteringLayout = sdfwgsl::inspectScatteringExpression(scatteringExpr);
    const std::string scatteringStructure = scatteringLayout.structure;
    if (scatteringExpr) {
        const std::string scatteringJson = scatteringExpr->toJson().dump();
        scatteringRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(scatteringJson));
    }

    uint64_t volumeChromaRevision = 0;
    const auto volumeChromaLayout =
        sdfwgsl::inspectVolumeChromaExpression(volumeChromaExpr);
    const std::string volumeChromaStructure = volumeChromaLayout.structure;
    if (volumeChromaExpr) {
        const std::string chromaJson = volumeChromaExpr->toJson().dump();
        volumeChromaRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(chromaJson));
    }

    uint64_t phaseRevision = 0;
    const auto phaseLayout = sdfwgsl::inspectPhaseExpression(phaseExpr);
    const std::string phaseStructure = phaseLayout.structure;
    if (phaseExpr) {
        const std::string phaseJson = phaseExpr->toJson().dump();
        phaseRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(phaseJson));
    }

    uint64_t emissionRevision = 0;
    const auto emissionLayout = sdfwgsl::inspectEmissionExpression(emissionExpr);
    const std::string emissionStructure = emissionLayout.structure;
    if (emissionExpr) {
        const std::string emissionJson = emissionExpr->toJson().dump();
        emissionRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(emissionJson));
    }

    // Rung 9: receiver-owned response has independent structure/value identity.
    // It is inspected once for this material draw before any shader cache decision.
    const auto responseLayout =
        sdfwgsl::inspectResponseExpression(mat.responseExpr.get());
    const std::string responseStructure = responseLayout.structure;

    if (multiSource) {
        if (_radianceSourcesLayoutRevision != radianceSourcesRevision()) {
            std::string structure = "sources:" + std::to_string(radianceSources().size()) + "\n";
            bool ok = true;
            std::string error;

            for (std::size_t i = 0; i < radianceSources().size(); ++i) {
                const auto& source = radianceSources()[i];
                const auto rho = sdfwgsl::inspectScalarExpression(source.radianceExpr, true);
                const auto chi = sdfwgsl::inspectVectorExpression(source.chromaExpr, true);
                const auto alpha = sdfwgsl::inspectAngularExpression(source.angularExpr);

                structure += "source[" + std::to_string(i) + "]\n";
                structure += "rho:" + rho.structure + "\n";
                structure += "chi:" + chi.structure + "\n";
                structure += "alpha:" + alpha.structure +
                             (alpha.readsOmega ? ":omega\n" : ":no-omega\n");

                if (!rho.ok && ok) {
                    ok = false;
                    error = "source[" + std::to_string(i) + "] radiance: " + rho.error;
                }
                if (!chi.ok && ok) {
                    ok = false;
                    error = "source[" + std::to_string(i) + "] chroma: " + chi.error;
                }
                if (!alpha.ok && ok) {
                    ok = false;
                    error = "source[" + std::to_string(i) + "] angular: " + alpha.error;
                }
            }

            const bool structureChanged =
                _radianceSourcesLayoutRevision == 0xffffffffffffffffULL ||
                structure != _radianceSourcesLayoutStructure ||
                ok != _radianceSourcesLayoutOk;
            if (structureChanged) ++_radianceSourcesStructureRevision;
            _radianceSourcesLayoutRevision = radianceSourcesRevision();
            _radianceSourcesLayoutStructure = std::move(structure);
            _radianceSourcesLayoutOk = ok;
            _radianceSourcesLayoutError = std::move(error);
        }
    } else {
        // Exact Rungs 3-6 layout inspection.
        if (_radianceLayoutRevision != radianceRevision() ||
            _radianceLayoutExprPtr != radianceExpr()) {
            sdfwgsl::ScalarExpressionLayout nextLayout =
                sdfwgsl::inspectScalarExpression(radianceExpr(), true);
            const bool structureChanged =
                _radianceLayoutRevision == 0xffffffffffffffffULL ||
                nextLayout.ok != _radianceLayout.ok ||
                nextLayout.structure != _radianceLayout.structure;
            if (structureChanged) ++_radianceStructureRevision;
            _radianceLayout = std::move(nextLayout);
            _radianceLayoutRevision = radianceRevision();
            _radianceLayoutExprPtr = radianceExpr();
        }

        if (_chromaLayoutRevision != radianceChromaRevision() ||
            _chromaLayoutExprPtr != radianceChromaExpr()) {
            sdfwgsl::VectorExpressionLayout nextLayout =
                sdfwgsl::inspectVectorExpression(radianceChromaExpr(), true);
            const bool structureChanged =
                _chromaLayoutRevision == 0xffffffffffffffffULL ||
                nextLayout.ok != _chromaLayout.ok ||
                nextLayout.structure != _chromaLayout.structure;
            if (structureChanged) ++_chromaStructureRevision;
            _chromaLayout = std::move(nextLayout);
            _chromaLayoutRevision = radianceChromaRevision();
            _chromaLayoutExprPtr = radianceChromaExpr();
        }

        if (_angularLayoutRevision != radianceAngularRevision() ||
            _angularLayoutExprPtr != radianceAngularExpr()) {
            sdfwgsl::AngularExpressionLayout nextLayout =
                sdfwgsl::inspectAngularExpression(radianceAngularExpr());
            const bool structureChanged =
                _angularLayoutRevision == 0xffffffffffffffffULL ||
                nextLayout.ok != _angularLayout.ok ||
                nextLayout.structure != _angularLayout.structure ||
                nextLayout.readsOmega != _angularLayout.readsOmega;
            if (structureChanged) ++_angularStructureRevision;
            _angularLayout = std::move(nextLayout);
            _angularLayoutRevision = radianceAngularRevision();
            _angularLayoutExprPtr = radianceAngularExpr();
        }
    }

    auto recordProgramRefusal = [&](const std::string& why) {
        auto& stats = mutableFrameStats();
        const bool firstOfReason =
            stats.sdfProgramRefusals == 0 || stats.sdfLastProgramRefusal != why;
        ++stats.sdfProgramRefusals;
        stats.sdfLastProgramRefusal = why;
        if (firstOfReason) {
            std::fprintf(stderr, "[WebGPU] SdfWgsl compile refused: %s\n", why.c_str());
        }
    };

    if (densityKind == sdfwgsl::DensityInputKind::Authored && !densityLayout.ok) {
        recordProgramRefusal("volume density: " + densityLayout.error);
        return;
    }
    if (!extinctionLayout.ok) {
        recordProgramRefusal("volume extinction: " + extinctionLayout.error);
        return;
    }
    if (!scatteringLayout.ok) {
        recordProgramRefusal("volume scattering: " + scatteringLayout.error);
        return;
    }
    if (!volumeChromaLayout.ok) {
        recordProgramRefusal("volume chroma: " + volumeChromaLayout.error);
        return;
    }
    if (!phaseLayout.ok) {
        recordProgramRefusal("volume phase: " + phaseLayout.error);
        return;
    }
    if (!emissionLayout.ok) {
        recordProgramRefusal("volume emission: " + emissionLayout.error);
        return;
    }
    if (!responseLayout.ok) {
        recordProgramRefusal("material response: " + responseLayout.error);
        return;
    }

    if (multiSource) {
        if (!_radianceSourcesLayoutOk) {
            recordProgramRefusal(_radianceSourcesLayoutError);
            return;
        }
    } else {
        if (!_radianceLayout.ok) {
            recordProgramRefusal("radiance: " + _radianceLayout.error);
            return;
        }
        if (!_chromaLayout.ok) {
            recordProgramRefusal("chroma: " + _chromaLayout.error);
            return;
        }
        if (!_angularLayout.ok) {
            recordProgramRefusal("angular: " + _angularLayout.error);
            return;
        }
    }

    MemoizedProgram* memo = nullptr;
    if (memoId != 0) {
        memo = &_programCache[memoId];
        const bool sourceStructureMatches =
            memo->multiSource == multiSource &&
            (multiSource
                ? memo->sourceSetStructureRevision == _radianceSourcesStructureRevision
                : (memo->radianceStructureRevision == _radianceStructureRevision &&
                   memo->chromaStructureRevision == _chromaStructureRevision &&
                   memo->angularStructureRevision == _angularStructureRevision));
        const bool densityStructureMatches =
            memo->densityKind == densityKind &&
            memo->densityStructure == densityStructure;
        const bool extinctionStructureMatches =
            memo->extinctionStructure == extinctionStructure;
        const bool scatteringStructureMatches =
            memo->scatteringStructure == scatteringStructure;
        const bool volumeChromaStructureMatches =
            memo->volumeChromaStructure == volumeChromaStructure;
        const bool phaseStructureMatches =
            memo->phaseStructure == phaseStructure &&
            memo->phaseReadsWi == phaseLayout.readsWi &&
            memo->phaseReadsWo == phaseLayout.readsWo;
        const bool emissionStructureMatches =
            memo->emissionStructure == emissionStructure &&
            memo->emissionReadsOmega == emissionLayout.readsOmega;
        const bool responseStructureMatches =
            memo->responseStructure == responseStructure &&
            memo->responseReadsNormal == responseLayout.readsNormal &&
            memo->responseReadsWi == responseLayout.readsWi &&
            memo->responseReadsWo == responseLayout.readsWo;

        if (memo->revision == memoRevision &&
            memo->colorRevision == mat.colorRevision &&
            sourceStructureMatches &&
            densityStructureMatches &&
            extinctionStructureMatches &&
            scatteringStructureMatches &&
            volumeChromaStructureMatches &&
            phaseStructureMatches &&
            emissionStructureMatches &&
            responseStructureMatches &&
            memo->colorExprPtr == mat.colorExpr.get()) {
            needsCompile = false;

            const bool sourceValuesChanged = multiSource
                ? memo->sourceSetRevision != radianceSourcesRevision()
                : (memo->radianceRevision != radianceRevision() ||
                   memo->chromaRevision != radianceChromaRevision() ||
                   memo->angularRevision != radianceAngularRevision());
            const bool densityValuesChanged =
                densityKind == sdfwgsl::DensityInputKind::Authored &&
                memo->densityRevision != densityRevision;
            const bool extinctionValuesChanged =
                extinctionExpr && memo->extinctionRevision != extinctionRevision;
            const bool scatteringValuesChanged =
                scatteringExpr && memo->scatteringRevision != scatteringRevision;
            const bool volumeChromaValuesChanged =
                volumeChromaExpr && memo->volumeChromaRevision != volumeChromaRevision;
            const bool phaseValuesChanged =
                phaseExpr && memo->phaseRevision != phaseRevision;
            const bool emissionValuesChanged =
                emissionExpr && memo->emissionRevision != emissionRevision;
            const bool responseValuesChanged =
                mat.responseExpr && memo->responseRevision != mat.responseRevision;
            const bool valuesChanged =
                memo->parameterRevision != memoParameterRevision ||
                sourceValuesChanged || densityValuesChanged || extinctionValuesChanged ||
                scatteringValuesChanged || volumeChromaValuesChanged ||
                phaseValuesChanged || emissionValuesChanged || responseValuesChanged;

            if (valuesChanged) {
                sdfwgsl::ParameterBlock refreshed =
                    sdfwgsl::collectParams(field, fieldNode, mat.colorExpr.get(),
                                           radianceExpr(), radianceChromaExpr(),
                                           radianceAngularExpr(), sourceSet,
                                           densityExpr, densityKind, extinctionExpr,
                                           scatteringExpr, volumeChromaExpr, phaseExpr,
                                           emissionExpr, mat.responseExpr.get());
                if (!refreshed.ok) {
                    recordProgramRefusal(refreshed.error);
                    return;
                }
                if (refreshed.values.size() == memo->prog.params.size()) {
                    memo->prog.params = std::move(refreshed.values);
                    memo->parameterRevision = memoParameterRevision;
                    memo->radianceRevision = radianceRevision();
                    memo->chromaRevision = radianceChromaRevision();
                    memo->angularRevision = radianceAngularRevision();
                    memo->sourceSetRevision = radianceSourcesRevision();
                    memo->densityRevision = densityRevision;
                    memo->extinctionRevision = extinctionRevision;
                    memo->scatteringRevision = scatteringRevision;
                    memo->volumeChromaRevision = volumeChromaRevision;
                    memo->phaseRevision = phaseRevision;
                    memo->emissionRevision = emissionRevision;
                    memo->responseRevision = mat.responseRevision;
                } else {
                    needsCompile = true;
                }
            }

            if (!needsCompile) {
                prog = &memo->prog;
                sp = memo->sp;
                isProvenHeightfield = memo->isProvenHeightfield;
                mutableFrameStats().sdfProgramCacheHits++;
            }
        }
    }

    if (needsCompile) {
        mutableFrameStats().sdfProgramCacheMisses++;
        localProg = sdfwgsl::compile(field, fieldNode, mat.colorExpr.get(),
                                     radianceExpr(), radianceChromaExpr(),
                                     radianceAngularExpr(), sourceSet,
                                     densityExpr, densityKind, extinctionExpr,
                                     scatteringExpr, volumeChromaExpr, phaseExpr,
                                     emissionExpr, mat.responseExpr.get());
        mutableFrameStats().sdfProgramCompiles++;
        mutableFrameStats().sdfWgslBytesGenerated += localProg.wgsl.size();
        if (!localProg.ok) {
            recordProgramRefusal(localProg.error);
            return;
        }
        sp = sdfPipeline(localProg.wgsl);
        if (!sp) return;

        isProvenHeightfield = geom::isHeightfieldExpr(field, nullptr);

        if (memo) {
            memo->revision = memoRevision;
            memo->parameterRevision = memoParameterRevision;
            memo->colorRevision = mat.colorRevision;
            memo->multiSource = multiSource;
            memo->radianceRevision = radianceRevision();
            memo->radianceStructureRevision = _radianceStructureRevision;
            memo->chromaRevision = radianceChromaRevision();
            memo->chromaStructureRevision = _chromaStructureRevision;
            memo->angularRevision = radianceAngularRevision();
            memo->angularStructureRevision = _angularStructureRevision;
            memo->sourceSetRevision = radianceSourcesRevision();
            memo->sourceSetStructureRevision = _radianceSourcesStructureRevision;
            memo->densityRevision = densityRevision;
            memo->densityKind = densityKind;
            memo->densityStructure = densityStructure;
            memo->extinctionRevision = extinctionRevision;
            memo->extinctionStructure = extinctionStructure;
            memo->scatteringRevision = scatteringRevision;
            memo->scatteringStructure = scatteringStructure;
            memo->volumeChromaRevision = volumeChromaRevision;
            memo->volumeChromaStructure = volumeChromaStructure;
            memo->phaseRevision = phaseRevision;
            memo->phaseStructure = phaseStructure;
            memo->phaseReadsWi = phaseLayout.readsWi;
            memo->phaseReadsWo = phaseLayout.readsWo;
            memo->emissionRevision = emissionRevision;
            memo->emissionStructure = emissionStructure;
            memo->emissionReadsOmega = emissionLayout.readsOmega;
            memo->responseRevision = mat.responseRevision;
            memo->responseStructure = responseStructure;
            memo->responseReadsNormal = responseLayout.readsNormal;
            memo->responseReadsWi = responseLayout.readsWi;
            memo->responseReadsWo = responseLayout.readsWo;
            memo->colorExprPtr = mat.colorExpr.get();
            memo->prog = std::move(localProg);
            memo->sp = sp;
            memo->isProvenHeightfield = isProvenHeightfield;

            memo->rangeReady = false;
            memo->rangeHierarchy = {};
            memo->rangeProxy = {};
            memo->rangeProofWords.clear();
            memo->rangeHasPositiveSkip = false;
            memo->rangeParameterRevision = 0xffffffff;
            prog = &memo->prog;
        } else {
            prog = &localProg;
        }
    }
    if (!sp || !prog) return;

    // Surface and volumetric proxies share one immutable resident cube.
    ensureSdfCubeVerts();
    if (!_sdfCubeVerts) return;

    SdfInstanceData inst;
    inst.model = _model;
    inst.invModel = glm::inverse(_model);
    
    glm::vec3 albedo(1.0f);
    if (mat.albedoPixels && mat.albedoWidth > 0 && mat.albedoHeight > 0) {
        const int halfW = mat.albedoWidth / 2;
        const int halfH = mat.albedoHeight / 2;
        const size_t idx = (size_t(halfH) * mat.albedoWidth + halfW) * 4;
        albedo = glm::vec3(mat.albedoPixels[idx + 0] / 255.0f,
                           mat.albedoPixels[idx + 1] / 255.0f,
                           mat.albedoPixels[idx + 2] / 255.0f);
    }
    inst.baseColor = glm::vec4(mat.baseColor * albedo, mat.opacity);
    inst.shading = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);
    // These are deliberately separate facts. The AST proves whether heightfield
    // reasoning is lawful; the optional pointer says only whether a separately
    // derived conservative grid happened to be supplied for this draw. A test,
    // diagnostic, or disabled traversal must not change proxy coverage merely by
    // omitting that cache.
    const bool hasConservativeHeightGrid = heightGrid &&
                                           heightGrid->dimX > 0 && heightGrid->dimZ > 0;
    // DDA traversal is quarantined after the native Metal sweep found that its
    // candidate-cell hand-off could miss grazing roots. Keep computing a
    // conservative grid for the proof/test seam, but do not upload or select
    // the traversal until the full on/off camera corpus is exact. This is not a
    // performance regression for the saved Perlin floor: it is y-dependent and
    // was already ineligible for a grid.
    const bool gridActive = usesHeightGridDda() && isProvenHeightfield &&
                            hasConservativeHeightGrid &&
                            !heightGrid->cells.empty();
    // A grid's cell coordinates are authored over `extent` by Object::rebuildHeightGrid.
    // Keep that exact interval for every proved heightfield, whether DDA traversal
    // is enabled or disabled. Other fields retain the historical 5% raster guard.
    // The range hierarchy is built over THIS actual render domain, not merely the
    // authored extent, so enabling it can never trim space the baseline marcher
    // was previously allowed to inspect.
    const glm::vec3 baselineProxyExtent =
        glm::abs(isProvenHeightfield ? extent : extent * 1.05f);
    glm::vec3 proxyExtent = baselineProxyExtent;

    // First generic spatial-Prophetic activation rung. It is deliberately limited
    // to surface-only draws: a FieldNode may carry volumetric density outside the
    // SDF zero set, so zero-set proof is not permission to trim that volume.
    //
    // memoId==0 also fails open: without a stable revision identity there is no
    // lawful cache boundary, and rebuilding an octree every frame would replace
    // one bottleneck with another.
    if (_sdfRangeProxyEnabled && memo && fieldNode == nullptr) {
        const bool extentChanged =
            memo->rangeAuthoredExtent.x != baselineProxyExtent.x ||
            memo->rangeAuthoredExtent.y != baselineProxyExtent.y ||
            memo->rangeAuthoredExtent.z != baselineProxyExtent.z;
        if (!memo->rangeReady ||
            memo->rangeParameterRevision != memoParameterRevision ||
            extentChanged) {
            memo->rangeHierarchy = geom::buildRangeHierarchy(
                field, baselineProxyExtent,
                kSdfRangeProxyMaxDepth, kSdfRangeProxyMaxNodes);
            memo->rangeProxy =
                geom::deriveZeroSetProxy(memo->rangeHierarchy, baselineProxyExtent);

            // Derive only proved-positive OUTSIDE knowledge into a compact
            // fixed-depth bit grid. The complete adaptive hierarchy remains the
            // CPU theorem. A zero GPU bit is deliberately non-authoritative:
            // exact authored marching owns that regular cell.
            static_assert(
                kSdfRangeGpuProofDepth <= kSdfRangeProxyMaxDepth,
                "GPU proof depth cannot exceed the CPU theorem depth");
            auto proofGrid = geom::derivePositiveRangeProofGrid(
                memo->rangeHierarchy, kSdfRangeGpuProofDepth);
            memo->rangeHasPositiveSkip = proofGrid.hasPositiveCells();
            memo->rangeProofWords = std::move(proofGrid.words);

            memo->rangeParameterRevision = memoParameterRevision;
            memo->rangeAuthoredExtent = baselineProxyExtent;
            memo->rangeReady = true;
            mutableFrameStats().sdfRangeHierarchyBuilds++;
        }

        if (!memo->rangeProxy.hasPossibleZero) {
            // Zero-free is not by itself permission to erase the draw. The
            // existing marcher treats entry into negative space as an immediate
            // hit, so an all-negative authored domain must fail open for parity.
            // Only a root theorem f>0 everywhere is truly empty outside space.
            const bool rootPositiveOutside =
                !memo->rangeHierarchy.nodes.empty() &&
                geom::rangeNodeProvesPositiveOutside(memo->rangeHierarchy.nodes.front());
            if (rootPositiveOutside) {
                mutableFrameStats().sdfRangeProxyCulledDraws++;
                return;
            }
        }
        if (kSdfRangeRasterTighteningVerified && memo->rangeProxy.tightened) {
            // Preserve a one-ULP outward raster guard at the derived boundary.
            // This is not a guessed world-space tolerance: it is the next
            // representable float, clamped to the already-authoritative baseline
            // proxy, solely to avoid losing a root that lies exactly on a cube face.
            const float inf = std::numeric_limits<float>::infinity();
            proxyExtent.x = std::min(baselineProxyExtent.x,
                                     std::nextafter(memo->rangeProxy.halfExtent.x, inf));
            proxyExtent.y = std::min(baselineProxyExtent.y,
                                     std::nextafter(memo->rangeProxy.halfExtent.y, inf));
            proxyExtent.z = std::min(baselineProxyExtent.z,
                                     std::nextafter(memo->rangeProxy.halfExtent.z, inf));
            mutableFrameStats().sdfRangeProxyDraws++;
        }
    }

    // Upload/traverse the proof grid only when it contains at least one
    // positive-outside theorem. Zero-bit cells are exact-march fallback space.
    // FieldNode draws remain excluded because zero-set emptiness says nothing
    // about volumetric density.
    const bool rangeTraversalMarcherVerified =
        prog->needsGradientStep || kSdfRangeDistanceTraversalVerified;
    if (_sdfRangeProxyEnabled && memo && fieldNode == nullptr &&
        rangeTraversalMarcherVerified &&
        memo->rangeReady && memo->rangeHasPositiveSkip &&
        !memo->rangeProofWords.empty()) {
        auto& rangeBatch = _sdfRangeNodeBatches[sp];
        const uint64_t base = static_cast<uint64_t>(rangeBatch.size());
        const uint64_t count =
            static_cast<uint64_t>(memo->rangeProofWords.size());
        const uint64_t u32Max =
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max());
        if (base <= u32Max && count <= u32Max && base + count <= u32Max) {
            inst.rangeProofWordOffset = static_cast<uint32_t>(base);
            inst.rangeProofWordCount = static_cast<uint32_t>(count);
            inst.rangeTraversalEnabled = 1u;
            inst.rangeProofDepth = kSdfRangeGpuProofDepth;
            rangeBatch.insert(rangeBatch.end(),
                              memo->rangeProofWords.begin(),
                              memo->rangeProofWords.end());
            mutableFrameStats().sdfRangeTraversalDraws++;
        }
    }

    inst.extents = glm::vec4(proxyExtent, 0.0f);
    
    // The box is grown slightly past the extent so a surface sitting exactly on the
    // boundary still gets fragments. surfaceEps/maxDist mirror the CPU raycaster's
    // 1e-4 hit threshold. maxDist is the object's own diagonal budget, NOT a world
    // constant: a hard cap (this briefly read `min(maxDim * 4, 600)`) makes anything
    // further than the cap vanish, and a large authored terrain is exactly the case
    // that trips it.
    float maxDim = glm::max(glm::max(extent.x, extent.y), extent.z);
    // Min/max heightfield grid (Phase C): wired only when the caller found a
    // proven heightfield (Object::getHeightGrid()) AND the property that
    // governs it is live. Defaulted zero otherwise, which the shader reads as
    // "no grid" and takes the unmodified marcher path.
    inst.heightGridOffset = 0;
    inst.heightGridDimX = 0;
    inst.heightGridDimZ = 0;
    if (gridActive) {
        auto& hgBatch = _sdfHeightGridBatches[sp];
        inst.heightGridOffset = static_cast<uint32_t>(hgBatch.size());
        inst.heightGridDimX = static_cast<uint32_t>(heightGrid->dimX);
        inst.heightGridDimZ = static_cast<uint32_t>(heightGrid->dimZ);
        hgBatch.insert(hgBatch.end(), heightGrid->cells.begin(), heightGrid->cells.end());
    }

    // damping selects the marcher: < 0.5 is the gradient-corrected path for an
    // authored expression, >= 0.5 the over-relaxed path for an exact distance
    // field. Even a proved f=y-h(x,z) is not generally a distance field: |f| is
    // the VERTICAL distance and can exceed the Euclidean distance when h slopes.
    // A min/max grid may conservatively skip empty cells, but it does not license
    // distance-field stepping inside a candidate cell.
    const float damping = prog->needsGradientStep ? 0.25f : 1.0f;
    // misc.x is a distinct proof bit: damping selects the step policy, while
    // only a structurally-proved y-h(x,z) field may use heightfield-only
    // planar/vertical early exits. Keep the two latches separate so a generic
    // gradient-marched Perlin expression cannot inherit those assumptions.
    inst.misc = glm::vec4(isProvenHeightfield ? 1.0f : 0.0f,
                          1e-4f, maxDim * 8.0f, damping);

    inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[sp].size());

    auto& sdfBatch = _sdfBatches[sp];
    if (sdfBatch.empty()) _activeSdfPipelines.push_back(sp);
    sdfBatch.push_back(inst);
    _sdfParamsBatches[sp].insert(_sdfParamsBatches[sp].end(), prog->params.begin(), prog->params.end());

    mutableFrameStats().trianglesDrawn += 12;
}

void WebGpuRenderer::drawParticles(const geom::FieldNode& field, int count) {
    if (!_pass || count <= 0 || !field.vectorField) return;

    const glm::vec3 flow(field.vectorField->baseFlowX,
                         field.vectorField->baseFlowY,
                         field.vectorField->baseFlowZ);
    const float speed = glm::length(flow);
    const glm::vec3 flowDir = speed > 1e-6f ? flow / speed : glm::vec3(0.0f, 1.0f, 0.0f);
    const float travel = field.vectorField->amplitude * glm::length(field.scale);

    bindPipeline(_particlePipe);

    ParticleUniforms pu;
    pu.mvp = _viewProj * _model;
    pu.color = glm::vec4(0.6f, 0.8f, 1.0f, 0.9f);
    pu.originAndTravel = glm::vec4(field.origin, travel);
    pu.flowDir = glm::vec4(flowDir, 0.0f);
    pu.scale = glm::vec4(field.scale, 0.0f);

    auto uAlloc = bufferPool().suballocateUniform(&pu, sizeof(ParticleUniforms));

    WGPUBindGroupEntry bge = {};
    bge.binding = 0;
    bge.buffer = uAlloc.buffer;
    bge.offset = uAlloc.offset;
    bge.size = uAlloc.size;
    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = _particleBgl;
    bgd.entryCount = 1;
    bgd.entries = &bge;
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
    _frameBindGroups.push_back(bg);

    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
    wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(count), 1, 0, 0);
}

void WebGpuRenderer::drawFlat(WGPURenderPipeline pipe, const std::vector<glm::vec3>& verts,
                             const glm::mat4& mvp, const glm::vec4& color) {
    if (!_pass || verts.empty()) return;
    bindPipeline(pipe);

    const size_t vbytes = verts.size() * sizeof(glm::vec3);
    auto vAlloc = bufferPool().suballocateVertex(verts.data(), vbytes);

    FlatUniforms fu{ mvp, color };
    auto uAlloc = bufferPool().suballocateUniform(&fu, sizeof(FlatUniforms));

    WGPUBindGroupEntry bge = {};
    bge.binding = 0; bge.buffer = uAlloc.buffer; bge.offset = uAlloc.offset; bge.size = uAlloc.size;
    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = _flatBgl; bgDesc.entryCount = 1; bgDesc.entries = &bge;
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(_device, &bgDesc);

    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vAlloc.buffer, vAlloc.offset, vbytes);
    wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(verts.size()), 1, 0, 0);

    mutableFrameStats().drawCalls++;
    mutableFrameStats().trianglesDrawn += static_cast<uint32_t>(verts.size() / 3);

    _frameBindGroups.push_back(bindGroup);
}

void WebGpuRenderer::drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                               const glm::vec4& color, float /*width*/,
                               Blend blend) {
    // WebGPU line primitives are always 1px — the OpenGL multi-pass width/glow does
    // not translate; the wireframe still reads clearly. Each segment is two points.
    std::vector<glm::vec3> verts;
    verts.reserve(segments.size() * 2);
    for (const auto& s : segments) { verts.push_back(s.first); verts.push_back(s.second); }
    drawFlat(flatPipeline(WGPUPrimitiveTopology_LineList, blend, DepthMode::TestOnly),
             verts, _viewProj * _model, color);
}

void WebGpuRenderer::drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                                 float scale, bool additive) {
    std::vector<glm::vec3> verts;
    verts.reserve(mesh.tris.size());
    for (const auto& v : mesh.tris) verts.push_back(v.pos);
    glm::mat4 mvp = _viewProj * _model * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    drawFlat(flatPipeline(WGPUPrimitiveTopology_TriangleList,
                          additive ? Blend::Additive : Blend::Alpha, DepthMode::TestOnly),
             verts, mvp, color);
}

void WebGpuRenderer::flushSdfDraws() {
    if (_activeSdfPipelines.empty()) return;
    if (!_pass) {
        for (const SdfPipeline* sp : _activeSdfPipelines) {
            _sdfBatches[sp].clear();
            _sdfParamsBatches[sp].clear();
            _sdfHeightGridBatches[sp].clear();
            _sdfRangeNodeBatches[sp].clear();
        }
        _activeSdfPipelines.clear();
        return;
    }
    
    // Rung 7 source records are shared across every SDF pipeline in the frame.
    // Upload only when their byte representation changes; source time and
    // placement are values, not reasons to rebuild WGSL.
    WGPUBuffer sourceBuffer = nullptr;
    uint64_t sourceOffset = 0;
    uint64_t sourceBindingSize = 0;

    if (radianceSources().size() > 1) {
        std::vector<RadianceSourceGpuData> gpuSources;
        gpuSources.reserve(radianceSources().size());
        for (const auto& source : radianceSources()) {
            RadianceSourceGpuData data{};
            data.position = glm::vec4(source.position, 1.0f);
            data.ambient = glm::vec4(source.ambientRadiance, 1.0f);
            data.diffuse = glm::vec4(source.diffuseRadiance, 1.0f);
            data.specular = glm::vec4(source.specularRadiance, 1.0f);
            data.coefficients = source.coefficients;
            data.time = glm::vec4(static_cast<float>(source.temporalCoordinate),
                                  static_cast<float>(source.temporalDelta), 0.0f, 0.0f);
            data.control =
                glm::vec4(source.enabled ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
            gpuSources.push_back(data);
        }

        const uint64_t bytes =
            static_cast<uint64_t>(gpuSources.size() * sizeof(RadianceSourceGpuData));
        uint64_t capacity = 256;
        while (capacity < bytes) capacity *= 2;

        if (!_persistentRadianceSources.buffer ||
            _persistentRadianceSources.capacityBytes < bytes) {
            WGPUBufferDescriptor bd = {};
            bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
            bd.size = capacity;
            WGPUBuffer grown = wgpuDeviceCreateBuffer(_device, &bd);
            if (grown) {
                if (_persistentRadianceSources.buffer) {
                    wgpuBufferRelease(_persistentRadianceSources.buffer);
                }
                _persistentRadianceSources.buffer = grown;
                _persistentRadianceSources.capacityBytes = capacity;
                _persistentRadianceSources.mirror.clear();
                _persistentRadianceSourceVramBytes = static_cast<size_t>(capacity);
            }
        }

        const auto* raw =
            reinterpret_cast<const unsigned char*>(gpuSources.data());
        const std::vector<unsigned char> bytesNow(raw, raw + bytes);
        if (_persistentRadianceSources.buffer &&
            _persistentRadianceSources.capacityBytes >= bytes) {
            if (_persistentRadianceSources.mirror != bytesNow) {
                wgpuQueueWriteBuffer(_queue, _persistentRadianceSources.buffer, 0,
                                     gpuSources.data(), bytes);
                _persistentRadianceSources.mirror = bytesNow;
            }
            sourceBuffer = _persistentRadianceSources.buffer;
            sourceBindingSize = bytes;
        } else {
            auto fallback =
                bufferPool().suballocateStorage(gpuSources.data(), bytes);
            sourceBuffer = fallback.buffer;
            sourceOffset = fallback.offset;
            sourceBindingSize = fallback.size;
        }
    }

    // Global uniforms for SDFs
    SdfGlobalUniforms u;
    u.viewProj = _viewProj;
    u.invViewProj = glm::inverse(_viewProj);
    u.lightPos = glm::vec4(lightPos(), 1.0f);
    u.eyePos = glm::vec4(_eyePos, 1.0f);
    u.lightAmbient = glm::vec4(lightAmbient(), 1.0f);
    u.lightDiffuse = glm::vec4(lightDiffuse(), 1.0f);
    u.lightSpecular = glm::vec4(lightSpecular(), 1.0f);
    u.lightControl = glm::vec4(lightingEnabled() ? 1.0f : 0.0f,
                               radianceVisibilityEnabled() ? 1.0f : 0.0f,
                               0.0f, 0.0f);
    u.radianceSourceCoefficients = radianceSourceCoefficients();
    u.radianceTime = glm::vec4(static_cast<float>(radianceTemporalCoordinate()),
                               static_cast<float>(radianceTemporalDelta()), 0.0f, 0.0f);
    u.volumeTime = glm::vec4(static_cast<float>(volumeDensityTemporalCoordinate()),
                            static_cast<float>(volumeDensityTemporalDelta()), 0.0f, 0.0f);
    // Unprojected rather than read off a named setting: the far plane belongs to
    // whatever projection the caller actually set, and asking the matrix cannot
    // drift away from it. NDC z = 1 is the far plane under the [0,1] depth range
    // WebGPU uses; view space looks down -z.
    float farDist = 1e6f;
    {
        const glm::vec4 farPt = glm::inverse(proj()) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (std::fabs(farPt.w) > 1e-9f) {
            const float d = -(farPt.z / farPt.w);
            if (std::isfinite(d) && d > 0.0f) farDist = d;
        }
    }
    u.limits = glm::vec4(farDist, float(_depthW), float(_depthH), _spaceDistortion);

    auto uAlloc = bufferPool().suballocateUniform(&u, sizeof(SdfGlobalUniforms));

    for (const SdfPipeline* sp : _activeSdfPipelines) {
        const auto& instances = _sdfBatches[sp];
        
        const auto& params = _sdfParamsBatches[sp];

        const size_t paramBytes = params.size() * sizeof(float);
        auto& persistent = _persistentSdfParams[sp];

        // Grow geometrically so a pipeline whose instance count fluctuates does
        // not churn buffers. Buffer contents are compared byte-for-byte: NaNs,
        // signed zero, and authored float bit patterns are all treated as data,
        // not normalized by a semantic comparison.
        const uint64_t requiredBytes = static_cast<uint64_t>(std::max<size_t>(paramBytes, sizeof(float)));
        if (!persistent.buffer || persistent.capacityBytes < requiredBytes) {
            uint64_t capacity = 256;
            while (capacity < requiredBytes) capacity *= 2;

            WGPUBufferDescriptor bd = {};
            bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
            bd.size = capacity;
            WGPUBuffer grown = wgpuDeviceCreateBuffer(_device, &bd);
            if (grown) {
                if (persistent.buffer) {
                    _persistentSdfParamVramBytes -= static_cast<size_t>(persistent.capacityBytes);
                    wgpuBufferRelease(persistent.buffer);
                }
                persistent.buffer = grown;
                persistent.capacityBytes = capacity;
                persistent.mirror.clear();
                _persistentSdfParamVramBytes += static_cast<size_t>(capacity);
            }
        }

        bool paramsChanged = persistent.mirror.size() != params.size();
        if (!paramsChanged && !params.empty()) {
            paramsChanged = std::memcmp(persistent.mirror.data(), params.data(), paramBytes) != 0;
        }

        // A failed grow leaves the previous buffer alive for accounting/later
        // retry, but it is not large enough for this batch and must not be used.
        const bool persistentUsable =
            persistent.buffer && persistent.capacityBytes >= requiredBytes;
        WGPUBuffer paramBuffer = persistentUsable ? persistent.buffer : nullptr;
        uint64_t paramOffset = 0;
        uint64_t paramBindingSize = requiredBytes;

        if (paramBuffer) {
            if (paramsChanged) {
                wgpuQueueWriteBuffer(_queue, paramBuffer, 0, params.data(), paramBytes);
                persistent.mirror = params;
                mutableFrameStats().sdfParameterBytesUploaded += paramBytes;
            }
        } else {
            // Allocation failure must degrade to the already-correct frame ring,
            // never to a missing parameter binding.
            auto fallback = bufferPool().suballocateStorage(params.data(), paramBytes);
            paramBuffer = fallback.buffer;
            paramOffset = fallback.offset;
            paramBindingSize = fallback.size;
            mutableFrameStats().sdfParameterBytesUploaded += paramBytes;
        }

        // Keep the compact positive-proof words resident exactly like static
        // SDF parameters. The CPU batch is rebuilt in instance order, but
        // unchanged proof bytes are not re-uploaded after warmup.
        const auto& rangeNodes = _sdfRangeNodeBatches[sp];
        const size_t rangeBytes = rangeNodes.size() * sizeof(uint32_t);
        auto& persistentRange = _persistentSdfRangeNodes[sp];
        const uint64_t requiredRangeBytes =
            static_cast<uint64_t>(std::max<size_t>(rangeBytes, sizeof(uint32_t)));
        if (!persistentRange.buffer || persistentRange.capacityBytes < requiredRangeBytes) {
            uint64_t capacity = 256;
            while (capacity < requiredRangeBytes) capacity *= 2;

            WGPUBufferDescriptor bd = {};
            bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
            bd.size = capacity;
            WGPUBuffer grown = wgpuDeviceCreateBuffer(_device, &bd);
            if (grown) {
                if (persistentRange.buffer) {
                    _persistentSdfRangeNodeVramBytes -=
                        static_cast<size_t>(persistentRange.capacityBytes);
                    wgpuBufferRelease(persistentRange.buffer);
                }
                persistentRange.buffer = grown;
                persistentRange.capacityBytes = capacity;
                persistentRange.mirror.clear();
                _persistentSdfRangeNodeVramBytes += static_cast<size_t>(capacity);
            }
        }

        bool rangeChanged = persistentRange.mirror.size() != rangeNodes.size();
        if (!rangeChanged && !rangeNodes.empty()) {
            rangeChanged = std::memcmp(
                persistentRange.mirror.data(), rangeNodes.data(), rangeBytes) != 0;
        }

        const bool persistentRangeUsable =
            persistentRange.buffer && persistentRange.capacityBytes >= requiredRangeBytes;
        WGPUBuffer rangeBuffer = persistentRangeUsable ? persistentRange.buffer : nullptr;
        uint64_t rangeOffset = 0;
        uint64_t rangeBindingSize = requiredRangeBytes;

        if (rangeBuffer) {
            if (rangeChanged && !rangeNodes.empty()) {
                wgpuQueueWriteBuffer(_queue, rangeBuffer, 0, rangeNodes.data(), rangeBytes);
                persistentRange.mirror = rangeNodes;
                mutableFrameStats().sdfRangeNodeBytesUploaded += rangeBytes;
            } else if (rangeChanged) {
                persistentRange.mirror.clear();
            }
        } else {
            // Binding 2 must remain valid even for a batch with no proof grid.
            // A zeroed dummy is never read because each such instance has count=0.
            const uint32_t dummy = 0u;
            const void* src = rangeNodes.empty()
                ? static_cast<const void*>(&dummy)
                : static_cast<const void*>(rangeNodes.data());
            auto fallback = bufferPool().suballocateStorage(src, requiredRangeBytes);
            rangeBuffer = fallback.buffer;
            rangeOffset = fallback.offset;
            rangeBindingSize = fallback.size;
            if (!rangeNodes.empty()) {
                mutableFrameStats().sdfRangeNodeBytesUploaded += rangeBytes;
            }
        }

        auto instAlloc = bufferPool().suballocateStorage(instances.data(), instances.size() * sizeof(SdfInstanceData));

        // Group 0: globals + authored parameter values + optional Rung 7 sources.
        WGPUBindGroupEntry bge[3] = {};
        bge[0].binding = 0;
        bge[0].buffer = uAlloc.buffer;
        bge[0].offset = uAlloc.offset;
        bge[0].size = uAlloc.size;
        bge[1].binding = 1;
        bge[1].buffer = paramBuffer;
        bge[1].offset = paramOffset;
        bge[1].size = paramBindingSize;
        if (sp->usesRadianceSources) {
            if (!sourceBuffer || sourceBindingSize == 0) continue;
            bge[2].binding = 2;
            bge[2].buffer = sourceBuffer;
            bge[2].offset = sourceOffset;
            bge[2].size = sourceBindingSize;
        }
        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = sp->bgl;
        bgd.entryCount = sp->usesRadianceSources ? 3 : 2;
        bgd.entries = bge;
        WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
        _frameBindGroups.push_back(bg);

        // Group 1: instances + heightfield cells + positive-proof bit words.
        // Empty accelerators bind legal dummy storage but advertise zero count,
        // so the shader cannot observe the dummy contents.
        auto& hgCells = _sdfHeightGridBatches[sp];
        if (hgCells.empty()) hgCells.push_back(glm::vec2(0.0f));
        auto hgAlloc = bufferPool().suballocateStorage(
            hgCells.data(), hgCells.size() * sizeof(glm::vec2));

        WGPUBindGroupEntry ibge[3] = {};
        ibge[0].binding = 0; ibge[0].buffer = instAlloc.buffer; ibge[0].offset = instAlloc.offset; ibge[0].size = instAlloc.size;
        ibge[1].binding = 1; ibge[1].buffer = hgAlloc.buffer; ibge[1].offset = hgAlloc.offset; ibge[1].size = hgAlloc.size;
        ibge[2].binding = 2; ibge[2].buffer = rangeBuffer; ibge[2].offset = rangeOffset; ibge[2].size = rangeBindingSize;
        WGPUBindGroupDescriptor ibgDesc = {};
        ibgDesc.layout = _sdfInstanceBgl; ibgDesc.entryCount = 3; ibgDesc.entries = ibge;
        WGPUBindGroup instBindGroup = wgpuDeviceCreateBindGroup(_device, &ibgDesc);
        _frameBindGroups.push_back(instBindGroup);

        bindPipeline(sp->pipe);
        wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(_pass, 1, instBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, _sdfCubeVerts, 0, 36 * sizeof(glm::vec3));
        wgpuRenderPassEncoderDraw(_pass, 36, static_cast<uint32_t>(instances.size()), 0, 0);

        mutableFrameStats().drawCalls++;
        mutableFrameStats().sdfDrawCalls++;
    }
    for (const SdfPipeline* sp : _activeSdfPipelines) {
        _sdfBatches[sp].clear();
        _sdfParamsBatches[sp].clear();
        _sdfHeightGridBatches[sp].clear();
        _sdfRangeNodeBatches[sp].clear();
    }
    _activeSdfPipelines.clear();
}

void WebGpuRenderer::flushVolumeComposite() {
    if (!_encoder || !_frameColorView || !_depthView) return;

    // V3 does not invent incident direction. A wi-reading Phi can consume one
    // and only one enabled admitted direct source. Position/enablement are
    // runtime values, so changing them must not regenerate WGSL.
    const Rendering::RadianceSourceBinding* incidentSource = nullptr;
    std::size_t enabledIncidentSources = 0;
    for (const auto& source : radianceSources()) {
        if (!source.enabled) continue;
        ++enabledIncidentSources;
        if (enabledIncidentSources == 1) incidentSource = &source;
    }
    if (enabledIncidentSources != 1) incidentSource = nullptr;

    // Cross-rung transport may consume source rho/chi/alpha, but it must consume
    // their revision/structure authority too. Pointer identity alone is not enough:
    // an authored source can change value or structure in place.
    //
    // Architectural note — GPT-5.6 Sol ("The Sun"), 2026-09-24:
    // These revisions, layouts, memo keys, caches, and GPU projections are
    // epistemic/execution machinery, not new world ontology. Let the machine's
    // knowledge grow aggressively while keeping the authored semantic basis small:
    // source emission remains source truth, medium response remains medium truth,
    // and transport consumes both without redefining either. The cache may know
    // more; it must never decide what a thing IS. This is the minimum–maximum
    // principle at the renderer boundary.
    auto combineRevision = [](uint64_t& seed, uint64_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    };
    uint64_t incidentSourceContentRevision = 0;
    if (incidentSource) {
        combineRevision(incidentSourceContentRevision, incidentSource->radianceRevision);
        combineRevision(incidentSourceContentRevision, incidentSource->chromaRevision);
        combineRevision(incidentSourceContentRevision, incidentSource->angularRevision);
    }

    const auto incidentRadianceLayout = sdfwgsl::inspectScalarExpression(
        incidentSource ? incidentSource->radianceExpr : nullptr, true);
    const auto incidentChromaLayout = sdfwgsl::inspectVectorExpression(
        incidentSource ? incidentSource->chromaExpr : nullptr, true);
    const auto incidentAngularLayout = sdfwgsl::inspectAngularExpression(
        incidentSource ? incidentSource->angularExpr : nullptr);
    const bool incidentSourceLayoutsOk =
        incidentRadianceLayout.ok && incidentChromaLayout.ok && incidentAngularLayout.ok;
    const std::string incidentSourceLayoutError =
        !incidentRadianceLayout.ok
            ? "incident source radiance: " + incidentRadianceLayout.error
            : !incidentChromaLayout.ok
                  ? "incident source chroma: " + incidentChromaLayout.error
                  : !incidentAngularLayout.ok
                        ? "incident source angular: " + incidentAngularLayout.error
                        : std::string{};
    const std::string incidentSourceStructure =
        "\nincident-source-rho:\n" + incidentRadianceLayout.structure +
        "\nincident-source-chi:\n" + incidentChromaLayout.structure +
        "\nincident-source-alpha:\n" + incidentAngularLayout.structure +
        (incidentAngularLayout.readsOmega ? ":reads-omega" : ":no-omega");

    // V5 chooses the production path by the number of valid bounded media, not
    // merely by vector size. One valid medium remains on the exact V4 path.
    std::vector<const Rendering::VolumeDensityBinding*> activeMedia;
    activeMedia.reserve(volumeDensitySources().size());
    for (const auto& medium : volumeDensitySources()) {
        if (!medium.densityExpr || medium.densityExpr->pieces.empty()) continue;
        const glm::vec3 halfExtent = glm::abs(medium.scale);
        if (halfExtent.x <= 1e-6f || halfExtent.y <= 1e-6f || halfExtent.z <= 1e-6f) {
            continue;
        }
        activeMedia.push_back(&medium);
    }

    if (activeMedia.size() > 1) {
        VolumeSetProgramKey setKey;
        setKey.reserve(activeMedia.size());
        std::vector<sdfwgsl::VolumeProgramInput> compilerInputs;
        compilerInputs.reserve(activeMedia.size());
        for (const auto* medium : activeMedia) {
            setKey.emplace_back(
                medium->densityExpr, medium->extinctionExpr,
                medium->scatteringExpr, medium->volumeChromaExpr,
                medium->phaseExpr, medium->emissionExpr,
                medium->occluderSdf,
                incidentSource ? incidentSource->radianceExpr : nullptr,
                incidentSource ? incidentSource->chromaExpr : nullptr,
                incidentSource ? incidentSource->angularExpr : nullptr);
            compilerInputs.push_back({
                medium->densityExpr, medium->extinctionExpr,
                medium->scatteringExpr, medium->volumeChromaExpr,
                medium->phaseExpr, medium->emissionExpr,
                medium->occluderSdf,
                incidentSource ? incidentSource->radianceExpr : nullptr,
                incidentSource ? incidentSource->chromaExpr : nullptr,
                incidentSource ? incidentSource->angularExpr : nullptr});
        }

        auto& setMemo = _volumeSetProgramCache[setKey];
        uint64_t setContentRevision = volumeDensitySourcesRevision();
        combineRevision(setContentRevision, incidentSourceContentRevision);
        if (setMemo.contentRevision != setContentRevision) {
            setMemo.contentRevision = setContentRevision;
            setMemo.phaseReadsWi = false;

            bool layoutsOk = incidentSourceLayoutsOk;
            std::string layoutError = incidentSourceLayoutError;
            std::string structure =
                "medium-count:" + std::to_string(activeMedia.size()) + "\n" +
                incidentSourceStructure + "\n";

            for (std::size_t i = 0; layoutsOk && i < activeMedia.size(); ++i) {
                const auto& medium = *activeMedia[i];
                const auto densityLayout =
                    sdfwgsl::inspectDensityExpression(medium.densityExpr);
                const auto extinctionLayout =
                    sdfwgsl::inspectExtinctionExpression(medium.extinctionExpr);
                const auto scatteringLayout =
                    sdfwgsl::inspectScatteringExpression(medium.scatteringExpr);
                const auto chromaLayout =
                    sdfwgsl::inspectVolumeChromaExpression(medium.volumeChromaExpr);
                const auto phaseLayout =
                    sdfwgsl::inspectPhaseExpression(medium.phaseExpr);
                const auto emissionLayout =
                    sdfwgsl::inspectEmissionExpression(medium.emissionExpr);
                const auto occluderLayout =
                    sdfwgsl::inspectOccluderLayout(medium.occluderSdf);

                setMemo.phaseReadsWi = setMemo.phaseReadsWi || phaseLayout.readsWi;
                if (!densityLayout.ok || !extinctionLayout.ok ||
                    !scatteringLayout.ok || !chromaLayout.ok ||
                    !phaseLayout.ok || !emissionLayout.ok || !occluderLayout.ok) {
                    layoutsOk = false;
                    layoutError =
                        "volume set member " + std::to_string(i) + ": " +
                        (!densityLayout.ok
                             ? "density: " + densityLayout.error
                             : !extinctionLayout.ok
                                   ? "extinction: " + extinctionLayout.error
                                   : !scatteringLayout.ok
                                         ? "scattering: " + scatteringLayout.error
                                         : !chromaLayout.ok
                                               ? "volume chroma: " + chromaLayout.error
                                               : !phaseLayout.ok
                                                     ? "volume phase: " + phaseLayout.error
                                                     : !emissionLayout.ok
                                                           ? "volume emission: " +
                                                                 emissionLayout.error
                                                           : "volume occluder: " +
                                                                 occluderLayout.error);
                    break;
                }

                structure +=
                    "member:" + std::to_string(i) +
                    "\ndensity:\n" + densityLayout.structure +
                    "\nextinction:\n" + extinctionLayout.structure +
                    "\nscattering:\n" + scatteringLayout.structure +
                    "\nvolume-chroma:\n" + chromaLayout.structure +
                    "\nvolume-phase:\n" + phaseLayout.structure +
                    (phaseLayout.readsWi ? ":reads-wi" : ":no-wi") +
                    (phaseLayout.readsWo ? ":reads-wo" : ":no-wo") +
                    "\nvolume-emission:\n" + emissionLayout.structure +
                    (emissionLayout.readsOmega ? ":reads-omega" : ":no-omega") +
                    "\noccluder:\n" + occluderLayout.structure +
                    "\n";
            }

            if (!layoutsOk) {
                setMemo.ok = false;
                setMemo.error = layoutError;
                setMemo.pipeline = nullptr;
            } else {
                const bool needsCompile =
                    !setMemo.ok || setMemo.structure != structure ||
                    !setMemo.pipeline;
                if (needsCompile) {
                    setMemo.prog = sdfwgsl::compileVolumeSet(compilerInputs);
                    ++mutableFrameStats().volumeProgramCompiles;
                    mutableFrameStats().volumeWgslBytesGenerated +=
                        setMemo.prog.wgsl.size();
                    setMemo.structure = structure;
                    setMemo.ok = setMemo.prog.ok;
                    setMemo.error = setMemo.prog.error;
                    setMemo.pipeline =
                        setMemo.ok ? volumePipeline(setMemo.prog.wgsl) : nullptr;
                    if (!setMemo.pipeline) setMemo.ok = false;
                }

                // Whether structure compiled or only values changed, rebuild the
                // concatenated parameter block through the existing V4 collector.
                // Each projected medium points at its own segment; no second
                // OntoMath value walker is introduced for V5.
                if (setMemo.ok && setMemo.pipeline) {
                    std::vector<float> setParams;
                    std::vector<uint32_t> paramOffsets;
                    paramOffsets.reserve(activeMedia.size());
                    for (std::size_t i = 0; i < activeMedia.size(); ++i) {
                        const auto& medium = *activeMedia[i];
                        paramOffsets.push_back(
                            static_cast<uint32_t>(setParams.size()));
                        const auto params = sdfwgsl::collectVolumeParams(
                            medium.densityExpr, medium.extinctionExpr,
                            medium.scatteringExpr, medium.volumeChromaExpr,
                            medium.phaseExpr, medium.emissionExpr,
                            medium.occluderSdf,
                            incidentSource ? incidentSource->radianceExpr : nullptr,
                            incidentSource ? incidentSource->chromaExpr : nullptr,
                            incidentSource ? incidentSource->angularExpr : nullptr);
                        if (!params.ok) {
                            setMemo.ok = false;
                            setMemo.error =
                                "volume set member " + std::to_string(i) +
                                " params: " + params.error;
                            break;
                        }
                        setParams.insert(
                            setParams.end(), params.values.begin(), params.values.end());
                    }
                    if (setMemo.ok) {
                        if (setParams.empty()) setParams.push_back(0.0f);
                        setMemo.prog.params = std::move(setParams);
                        setMemo.paramOffsets = std::move(paramOffsets);
                    }
                }
            }
        } else {
            // Preserve telemetry's historical meaning as useful per-medium
            // reuse even though V5 reuses one fused program for the set.
            mutableFrameStats().volumeProgramCacheHits +=
                static_cast<uint32_t>(activeMedia.size());
        }

        // Missing runtime incident-source truth must not poison the compiled
        // set memo. If the source state becomes lawful next frame, the same
        // structure is immediately reusable just like the V3 one-medium path.
        std::string runtimeSetRefusal;
        if (setMemo.phaseReadsWi && !incidentSource) {
            runtimeSetRefusal =
                "volume set phase: Phi reads wi but transport has " +
                std::to_string(enabledIncidentSources) +
                " enabled admitted direct sources; exactly one is required";
        }

        if (!runtimeSetRefusal.empty()) {
            ++mutableFrameStats().volumeProgramRefusals;
            mutableFrameStats().volumeLastProgramRefusal = runtimeSetRefusal;
        } else if (!setMemo.ok || !setMemo.pipeline) {
            if (!setMemo.error.empty()) {
                ++mutableFrameStats().volumeProgramRefusals;
                mutableFrameStats().volumeLastProgramRefusal = setMemo.error;
            }
        } else {
            auto& instances = _volumeBatches[setMemo.pipeline];
            auto& params = _volumeParamBatches[setMemo.pipeline];
            if (instances.empty()) {
                _activeVolumePipelines.push_back(setMemo.pipeline);
            }

            glm::vec3 setMin(std::numeric_limits<float>::max());
            glm::vec3 setMax(std::numeric_limits<float>::lowest());
            std::vector<VolumeInstanceData> mediumInstances;
            mediumInstances.reserve(activeMedia.size());

            for (std::size_t i = 0; i < activeMedia.size(); ++i) {
                const auto& medium = *activeMedia[i];
                const glm::vec3 halfExtent = glm::abs(medium.scale);
                setMin = glm::min(setMin, medium.origin - halfExtent);
                setMax = glm::max(setMax, medium.origin + halfExtent);

                VolumeInstanceData instance;
                instance.origin = glm::vec4(medium.origin, 1.0f);
                instance.halfExtent = glm::vec4(halfExtent, 0.0f);
                instance.time =
                    glm::vec4(static_cast<float>(medium.temporalCoordinate),
                              static_cast<float>(medium.temporalDelta), 0.0f, 0.0f);
                instance.paramOffset =
                    i < setMemo.paramOffsets.size() ? setMemo.paramOffsets[i] : 0u;
                mediumInstances.push_back(instance);
            }

            VolumeInstanceData header;
            const glm::vec3 setOrigin = 0.5f * (setMin + setMax);
            const glm::vec3 setHalfExtent = 0.5f * (setMax - setMin);
            header.origin = glm::vec4(setOrigin, 1.0f);
            header.halfExtent = glm::vec4(setHalfExtent, 0.0f);
            header.time = glm::vec4(0.0f);
            header.paramOffset = 0u;

            instances.push_back(header);
            instances.insert(
                instances.end(), mediumInstances.begin(), mediumInstances.end());
            params.insert(
                params.end(), setMemo.prog.params.begin(), setMemo.prog.params.end());
            _volumeDrawInstanceCounts[setMemo.pipeline] = 1u;
        }
    }

    // Build ordinary V0-V4 batches only from the bounded projection EngineRender
    // handed to the renderer. A multi-medium set instead uses the fused path
    // above; no Zone/Object scan occurs here.
    if (activeMedia.size() <= 1) {
    for (const auto& medium : volumeDensitySources()) {
        if (!medium.densityExpr || medium.densityExpr->pieces.empty()) continue;

        // FieldNode's existing spatial convention is origin ± scale (the
        // particle modality samples local [-1,+1] and multiplies by scale).
        // Do not reinterpret this shared authored property as a full span.
        const glm::vec3 halfExtent = glm::abs(medium.scale);
        if (halfExtent.x <= 1e-6f || halfExtent.y <= 1e-6f || halfExtent.z <= 1e-6f) {
            continue;
        }

        const VolumeProgramKey programKey{
            medium.densityExpr, medium.extinctionExpr,
            medium.scatteringExpr, medium.volumeChromaExpr, medium.phaseExpr,
            medium.emissionExpr,
            medium.occluderSdf,
            incidentSource ? incidentSource->radianceExpr : nullptr,
            incidentSource ? incidentSource->chromaExpr : nullptr,
            incidentSource ? incidentSource->angularExpr : nullptr};
        auto& memo = _volumeProgramCache[programKey];
        uint64_t mediumContentRevision = Rendering::volumeContentRevision(medium);
        combineRevision(mediumContentRevision, incidentSourceContentRevision);
        if (memo.contentRevision != mediumContentRevision) {
            const auto densityLayout =
                sdfwgsl::inspectDensityExpression(medium.densityExpr);
            const auto extinctionLayout =
                sdfwgsl::inspectExtinctionExpression(medium.extinctionExpr);
            const auto scatteringLayout =
                sdfwgsl::inspectScatteringExpression(medium.scatteringExpr);
            const auto volumeChromaLayout =
                sdfwgsl::inspectVolumeChromaExpression(medium.volumeChromaExpr);
            const auto phaseLayout =
                sdfwgsl::inspectPhaseExpression(medium.phaseExpr);
            const auto emissionLayout =
                sdfwgsl::inspectEmissionExpression(medium.emissionExpr);
            const auto occluderLayout =
                sdfwgsl::inspectOccluderLayout(medium.occluderSdf);
            memo.contentRevision = mediumContentRevision;
            memo.phaseReadsWi = phaseLayout.readsWi;
            memo.phaseReadsWo = phaseLayout.readsWo;
            memo.emissionReadsOmega = emissionLayout.readsOmega;

            if (!incidentSourceLayoutsOk ||
                !densityLayout.ok || !extinctionLayout.ok ||
                !scatteringLayout.ok || !volumeChromaLayout.ok ||
                !phaseLayout.ok || !emissionLayout.ok || !occluderLayout.ok) {
                memo.ok = false;
                memo.error = !incidentSourceLayoutsOk
                    ? incidentSourceLayoutError
                    : !densityLayout.ok
                    ? "density: " + densityLayout.error
                    : !extinctionLayout.ok
                        ? "extinction: " + extinctionLayout.error
                        : !scatteringLayout.ok
                            ? "scattering: " + scatteringLayout.error
                            : !volumeChromaLayout.ok
                                ? "volume chroma: " + volumeChromaLayout.error
                                : !phaseLayout.ok
                                    ? "volume phase: " + phaseLayout.error
                                    : !emissionLayout.ok
                                        ? "volume emission: " + emissionLayout.error
                                        : "volume occluder: " + occluderLayout.error;
                memo.pipeline = nullptr;
                ++mutableFrameStats().volumeProgramRefusals;
                mutableFrameStats().volumeLastProgramRefusal = memo.error;
                continue;
            }

            const std::string structure =
                "density:\n" + densityLayout.structure +
                "\nextinction:\n" + extinctionLayout.structure +
                "\nscattering:\n" + scatteringLayout.structure +
                "\nvolume-chroma:\n" + volumeChromaLayout.structure +
                "\nvolume-phase:\n" + phaseLayout.structure +
                (phaseLayout.readsWi ? ":reads-wi" : ":no-wi") +
                (phaseLayout.readsWo ? ":reads-wo" : ":no-wo") +
                "\nvolume-emission:\n" + emissionLayout.structure +
                (emissionLayout.readsOmega ? ":reads-omega" : ":no-omega") +
                "\noccluder:\n" + occluderLayout.structure +
                incidentSourceStructure;
            if (!memo.ok || memo.structure != structure || !memo.pipeline) {
                memo.prog =
                    sdfwgsl::compileVolume(
                        medium.densityExpr, medium.extinctionExpr,
                        medium.scatteringExpr, medium.volumeChromaExpr,
                        medium.phaseExpr, medium.emissionExpr,
                        medium.occluderSdf,
                        incidentSource ? incidentSource->radianceExpr : nullptr,
                        incidentSource ? incidentSource->chromaExpr : nullptr,
                        incidentSource ? incidentSource->angularExpr : nullptr);
                ++mutableFrameStats().volumeProgramCompiles;
                mutableFrameStats().volumeWgslBytesGenerated += memo.prog.wgsl.size();
                memo.structure = structure;
                memo.ok = memo.prog.ok;
                memo.error = memo.prog.error;
                memo.pipeline = memo.ok ? volumePipeline(memo.prog.wgsl) : nullptr;
                if (!memo.pipeline) memo.ok = false;
            } else {
                const auto params =
                    sdfwgsl::collectVolumeParams(
                        medium.densityExpr, medium.extinctionExpr,
                        medium.scatteringExpr, medium.volumeChromaExpr,
                        medium.phaseExpr, medium.emissionExpr,
                        medium.occluderSdf,
                        incidentSource ? incidentSource->radianceExpr : nullptr,
                        incidentSource ? incidentSource->chromaExpr : nullptr,
                        incidentSource ? incidentSource->angularExpr : nullptr);
                memo.ok = params.ok;
                memo.error = params.error;
                if (params.ok) memo.prog.params = params.values;
            }
        } else {
            ++mutableFrameStats().volumeProgramCacheHits;
        }

        // Refusal never falls back to stale compiled medium state.
        if (!memo.ok || !memo.pipeline) {
            if (!memo.error.empty()) {
                ++mutableFrameStats().volumeProgramRefusals;
                mutableFrameStats().volumeLastProgramRefusal = memo.error;
            }
            continue;
        }

        if (memo.phaseReadsWi && !incidentSource) {
            const std::string why =
                "volume phase: Phi reads wi but transport has " +
                std::to_string(enabledIncidentSources) +
                " enabled admitted direct sources; exactly one is required";
            ++mutableFrameStats().volumeProgramRefusals;
            mutableFrameStats().volumeLastProgramRefusal = why;
            continue;
        }

        auto& instances = _volumeBatches[memo.pipeline];
        auto& params = _volumeParamBatches[memo.pipeline];
        if (instances.empty()) _activeVolumePipelines.push_back(memo.pipeline);

        VolumeInstanceData instance;
        instance.origin = glm::vec4(medium.origin, 1.0f);
        instance.halfExtent = glm::vec4(halfExtent, 0.0f);
        instance.time = glm::vec4(static_cast<float>(medium.temporalCoordinate),
                                  static_cast<float>(medium.temporalDelta), 0.0f, 0.0f);
        instance.paramOffset = static_cast<uint32_t>(params.size());

        instances.push_back(instance);
        params.insert(params.end(), memo.prog.params.begin(), memo.prog.params.end());
    }
    }

    if (_activeVolumePipelines.empty()) return;

    ensureSdfCubeVerts();
    if (!_sdfCubeVerts) {
        for (const VolumePipeline* pipeline : _activeVolumePipelines) {
            _volumeBatches[pipeline].clear();
            _volumeParamBatches[pipeline].clear();
            _volumeDrawInstanceCounts.erase(pipeline);
        }
        _activeVolumePipelines.clear();
        return;
    }

    // Opaque meshes/SDFs were deferred; make their depth authoritative before
    // any medium samples it.
    flushMeshDraws();
    flushSdfDraws();

    // Close the world pass. The color/depth attachments remain stored.
    wgpuRenderPassEncoderEnd(_pass);
    wgpuRenderPassEncoderRelease(_pass);
    _pass = nullptr;
    _boundPipeline = nullptr;

    WGPURenderPassColorAttachment color = {};
    color.view = _frameColorView;
    color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color.loadOp = WGPULoadOp_Load;
    color.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor volumePassDesc = {};
    volumePassDesc.colorAttachmentCount = 1;
    volumePassDesc.colorAttachments = &color;
    // No depth attachment: finished depth is read as a texture instead.
    _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &volumePassDesc);
    _boundPipeline = nullptr;

    VolumeGlobalUniforms globals;
    globals.viewProj = _viewProj;
    globals.invViewProj = glm::inverse(_viewProj);
    globals.eyePos = glm::vec4(_eyePos, 1.0f);
    globals.viewport = glm::vec4(static_cast<float>(_depthW),
                                 static_cast<float>(_depthH), 0.0f, 0.0f);
    if (incidentSource) {
        globals.incidentSource = glm::vec4(incidentSource->position, 1.0f);
        globals.incidentColor = glm::vec4(incidentSource->diffuseRadiance, incidentSource->coefficients.x);
        globals.sourceTime = glm::vec4(
            static_cast<float>(incidentSource->temporalCoordinate),
            static_cast<float>(incidentSource->temporalDelta), 0.0f, 0.0f);
        // V3 compatibility remains isotropic unless Phi is explicitly authored.
        globals.volumeControl = glm::vec4(24.0f, 1.0f, 0.0f, 0.0f);
    } else {
        globals.incidentSource = glm::vec4(0.0f);
        globals.incidentColor = glm::vec4(1.0f);
        globals.sourceTime = glm::vec4(0.0f);
        globals.volumeControl = glm::vec4(0.0f);
    }
    auto globalAlloc = bufferPool().suballocateUniform(&globals, sizeof(globals));

    for (const VolumePipeline* pipeline : _activeVolumePipelines) {
        const auto& instances = _volumeBatches[pipeline];
        const auto& params = _volumeParamBatches[pipeline];
        if (!pipeline || !pipeline->pipe || instances.empty() || params.empty()) continue;

        auto paramAlloc =
            bufferPool().suballocateStorage(params.data(), params.size() * sizeof(float));
        auto instanceAlloc =
            bufferPool().suballocateStorage(instances.data(),
                                            instances.size() * sizeof(VolumeInstanceData));

        WGPUBindGroupEntry globalEntries[3] = {};
        globalEntries[0].binding = 0;
        globalEntries[0].buffer = globalAlloc.buffer;
        globalEntries[0].offset = globalAlloc.offset;
        globalEntries[0].size = globalAlloc.size;
        globalEntries[1].binding = 1;
        globalEntries[1].buffer = paramAlloc.buffer;
        globalEntries[1].offset = paramAlloc.offset;
        globalEntries[1].size = paramAlloc.size;
        globalEntries[2].binding = 2;
        globalEntries[2].textureView = _depthView;

        WGPUBindGroupDescriptor globalBgDesc = {};
        globalBgDesc.layout = pipeline->globalBgl;
        globalBgDesc.entryCount = 3;
        globalBgDesc.entries = globalEntries;
        WGPUBindGroup globalBg = wgpuDeviceCreateBindGroup(_device, &globalBgDesc);
        _frameBindGroups.push_back(globalBg);

        WGPUBindGroupEntry instanceEntry = {};
        instanceEntry.binding = 0;
        instanceEntry.buffer = instanceAlloc.buffer;
        instanceEntry.offset = instanceAlloc.offset;
        instanceEntry.size = instanceAlloc.size;
        WGPUBindGroupDescriptor instanceBgDesc = {};
        instanceBgDesc.layout = pipeline->instanceBgl;
        instanceBgDesc.entryCount = 1;
        instanceBgDesc.entries = &instanceEntry;
        WGPUBindGroup instanceBg = wgpuDeviceCreateBindGroup(_device, &instanceBgDesc);
        _frameBindGroups.push_back(instanceBg);

        bindPipeline(pipeline->pipe);
        wgpuRenderPassEncoderSetBindGroup(_pass, 0, globalBg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(_pass, 1, instanceBg, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(
            _pass, 0, _sdfCubeVerts, 0, 36 * sizeof(glm::vec3));
        uint32_t drawInstanceCount = static_cast<uint32_t>(instances.size());
        if (const auto countIt = _volumeDrawInstanceCounts.find(pipeline);
            countIt != _volumeDrawInstanceCounts.end()) {
            drawInstanceCount = countIt->second;
        }
        wgpuRenderPassEncoderDraw(
            _pass, 36, drawInstanceCount, 0, 0);

        mutableFrameStats().drawCalls++;
        mutableFrameStats().trianglesDrawn +=
            static_cast<uint32_t>(12 * drawInstanceCount);
    }

    wgpuRenderPassEncoderEnd(_pass);
    wgpuRenderPassEncoderRelease(_pass);
    _pass = nullptr;
    _boundPipeline = nullptr;

    for (const VolumePipeline* pipeline : _activeVolumePipelines) {
        _volumeBatches[pipeline].clear();
        _volumeParamBatches[pipeline].clear();
        _volumeDrawInstanceCounts.erase(pipeline);
    }
    _activeVolumePipelines.clear();

    // Reopen the ordinary pass for nametags, menus and 2D authored overlays.
    // Load both attachments exactly; volume color is now part of the world, while
    // opaque depth remains available to any later world-space overlay.
    WGPURenderPassColorAttachment continueColor = {};
    continueColor.view = _frameColorView;
    continueColor.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    continueColor.loadOp = WGPULoadOp_Load;
    continueColor.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDepthStencilAttachment continueDepth = {};
    continueDepth.view = _depthView;
    continueDepth.depthLoadOp = WGPULoadOp_Load;
    continueDepth.depthStoreOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor continueDesc = {};
    continueDesc.colorAttachmentCount = 1;
    continueDesc.colorAttachments = &continueColor;
    continueDesc.depthStencilAttachment = &continueDepth;

    _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &continueDesc);
    _boundPipeline = nullptr;
}

void WebGpuRenderer::composeVolumes() {
    if (!_pass || volumeDensitySources().empty()) return;
    flushVolumeComposite();
}

void WebGpuRenderer::endFrame() {
    if (!_pass) return;
    // Every drawMesh() call this frame only queued into _meshBatches; this is
    // where those batches actually become wgpuRenderPassEncoderDraw calls, so
    // it must run before the pass ends.
    flushMeshDraws();
    flushSdfDraws();
    wgpuRenderPassEncoderEnd(_pass);
    wgpuRenderPassEncoderRelease(_pass);
    _pass = nullptr;

    endGpuTimestampFrame();

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(_encoder, nullptr);
    wgpuQueueSubmit(_queue, 1, &cmd);
    if (_timestampSlotForFrame >= 0) {
        auto& slot = _gpuTimestampSlots[static_cast<size_t>(_timestampSlotForFrame)];
        slot.mapPending = true;
        WGPUBufferMapCallbackInfo mapCallback = {};
        mapCallback.mode = WGPUCallbackMode_AllowProcessEvents;
        mapCallback.callback = onGpuTimestampMap;
        mapCallback.userdata1 = &slot;
        wgpuBufferMapAsync(slot.readback, WGPUMapMode_Read, 0,
                           2 * sizeof(uint64_t), mapCallback);
    }
    _timestampSlotForFrame = -1;
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(_encoder);
    _encoder = nullptr;
    _frameColorView = nullptr;

    auto& fs = mutableFrameStats();
    fs.vramAllocatedBytes = bufferPool().totalVramBytes() + _meshCache.totalCachedBytes() +
                            _persistentSdfParamVramBytes + _persistentSdfRangeNodeVramBytes +
                            _persistentRadianceSourceVramBytes;
    fs.uniformBytesWritten = bufferPool().bytesWrittenThisFrame();
    fs.bufferSuballocations = bufferPool().suballocationsThisFrame();
    fs.cachedMeshesCount = static_cast<uint32_t>(_meshCache.cachedMeshCount());

    // Queue-ordered writes are part of this frame's submission; reuse the
    // allocation offsets for the next recorded frame without multiplying the
    // pool's resident GPU memory.
    bufferPool().resetFrame();
    _meshCache.endFrame();

    // The submit is done recording; the resources it referenced can go now.
    releaseFrameResources();
}

void WebGpuRenderer::releaseFrameResources() {
    for (WGPUBindGroup bg : _frameBindGroups) wgpuBindGroupRelease(bg);
    for (WGPUBuffer b : _frameBuffers) wgpuBufferRelease(b);
    for (WGPUTextureView v : _frameTextureViews) wgpuTextureViewRelease(v);
    for (WGPUTexture t : _frameTextures) wgpuTextureRelease(t);
    _frameBindGroups.clear();
    _frameBuffers.clear();
    _frameTextureViews.clear();
    _frameTextures.clear();
}

// ---------------------------------------------------------------------------
// Flat-colour primitives.
// ---------------------------------------------------------------------------

void WebGpuRenderer::drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                               Blend blend, bool depthWrite) {
    if (!_pass || tris.empty()) return;
    // In 2D scope the model transform is meaningless; otherwise these are world-space
    // triangles under the current model, exactly like drawMesh.
    const glm::mat4 mvp = _in2D ? _ortho2D : _viewProj * _model;
    drawFlat(flatPipeline(WGPUPrimitiveTopology_TriangleList, blend,
                          depthWrite ? DepthMode::TestWrite : DepthMode::TestOnly),
             tris, mvp, color);
}

void WebGpuRenderer::begin2D(uint32_t width, uint32_t height) {
    // (0,0) at the TOP-LEFT, matching the boundary contract and glOrtho(0,w,h,0,-1,1).
    // Built with the [0,1] clip depth WebGPU requires; depth is ignored anyway since
    // the 2D pipelines compare Always.
    _in2D = true;
    _ortho2D = glm::orthoZO(0.0f, static_cast<float>(width),
                            static_cast<float>(height), 0.0f, -1.0f, 1.0f);
}

void WebGpuRenderer::end2D() {
    _in2D = false;
}

void WebGpuRenderer::drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) {
    if (!_pass || tris.empty()) return;
    std::vector<glm::vec3> verts;
    verts.reserve(tris.size());
    for (const glm::vec2& p : tris) verts.push_back(glm::vec3(p, 0.0f));
    drawFlat(flatPipeline(WGPUPrimitiveTopology_TriangleList, Blend::Alpha, DepthMode::None),
             verts, _ortho2D, color);
}

void WebGpuRenderer::drawLines2D(const std::vector<glm::vec2>& segments,
                                 const glm::vec4& color, float /*width*/) {
    if (!_pass || segments.empty()) return;
    // Width is unrepresentable: native WebGPU lines are 1px. Thick 2D strokes would
    // have to be expanded into quads — deliberately not done here, so that when the
    // UI looks thin the cause is visible rather than buried in a silent emulation.
    std::vector<glm::vec3> verts;
    verts.reserve(segments.size());
    for (const glm::vec2& p : segments) verts.push_back(glm::vec3(p, 0.0f));
    drawFlat(flatPipeline(WGPUPrimitiveTopology_LineList, Blend::Alpha, DepthMode::None),
             verts, _ortho2D, color);
}

void WebGpuRenderer::drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                                 const glm::vec4& rect, const glm::vec4& tint) {
    if (!_pass || !rgba || width == 0 || height == 0) return;

    // Upload the pixels. Callers regenerate these every frame (the brush canvas),
    // so this is a per-draw texture, released with the rest of the frame.
    WGPUTextureDescriptor td = {};
    td.dimension = WGPUTextureDimension_2D;
    td.size = { width, height, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    td.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    WGPUTexture tex = wgpuDeviceCreateTexture(_device, &td);
    WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);
    _frameTextures.push_back(tex);
    _frameTextureViews.push_back(view);

    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = tex; dst.mipLevel = 0; dst.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferLayout lay = {};
    lay.bytesPerRow = width * 4; lay.rowsPerImage = height;
    WGPUExtent3D ext = { width, height, 1 };
    wgpuQueueWriteTexture(_queue, &dst, rgba, size_t(width) * height * 4, &lay, &ext);

    // Two triangles over `rect`, with v increasing downward to match the
    // top-left-origin ortho (the GL path relied on glTexCoord doing the same).
    const ImageVertex quad[6] = {
        {{rect.x, rect.y, 0.0f}, {0.0f, 0.0f}},
        {{rect.z, rect.y, 0.0f}, {1.0f, 0.0f}},
        {{rect.z, rect.w, 0.0f}, {1.0f, 1.0f}},
        {{rect.x, rect.y, 0.0f}, {0.0f, 0.0f}},
        {{rect.z, rect.w, 0.0f}, {1.0f, 1.0f}},
        {{rect.x, rect.w, 0.0f}, {0.0f, 1.0f}},
    };
    auto vAlloc = bufferPool().suballocateVertex(quad, sizeof(quad));

    FlatUniforms u{};
    u.mvp = _ortho2D;
    u.color = tint;
    auto uAlloc = bufferPool().suballocateUniform(&u, sizeof(u));

    WGPUBindGroupEntry bge[3] = {};
    bge[0].binding = 0; bge[0].buffer = uAlloc.buffer; bge[0].offset = uAlloc.offset; bge[0].size = uAlloc.size;
    bge[1].binding = 1; bge[1].textureView = view;
    bge[2].binding = 2; bge[2].sampler = _sampler;
    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = _imageBgl; bgd.entryCount = 3; bgd.entries = bge;
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
    _frameBindGroups.push_back(bg);

    bindPipeline(_imagePipe);
    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vAlloc.buffer, vAlloc.offset, sizeof(quad));
    wgpuRenderPassEncoderDraw(_pass, 6, 1, 0, 0);

    mutableFrameStats().drawCalls++;
    mutableFrameStats().trianglesDrawn += 2;
}

// ---------------------------------------------------------------------------
// Persistent textures. The handle is a dense counter, not a pointer: a stale
// handle then fails a lookup harmlessly instead of dereferencing freed memory.
// ---------------------------------------------------------------------------

TextureHandle WebGpuRenderer::uploadTexture(TextureHandle handle, const uint8_t* rgba,
                                            uint32_t width, uint32_t height) {
    if (!_device || !rgba || width == 0 || height == 0) return handle;

    auto it = _textures.find(handle);
    // A resize cannot be done in place — drop the old texture and build again.
    if (it != _textures.end() && it->second.size != width) {
        wgpuTextureViewRelease(it->second.view);
        wgpuTextureRelease(it->second.tex);
        _textures.erase(it);
        it = _textures.end();
    }

    if (it == _textures.end()) {
        WGPUTextureDescriptor td = {};
        td.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        td.dimension = WGPUTextureDimension_2D;
        td.size = { width, height, 1 };
        td.format = WGPUTextureFormat_RGBA8Unorm;
        td.mipLevelCount = 1; td.sampleCount = 1;
        OwnedTexture ot;
        ot.tex  = wgpuDeviceCreateTexture(_device, &td);
        if (!ot.tex) return 0;
        ot.view = wgpuTextureCreateView(ot.tex, nullptr);
        ot.size = width;
        if (handle == 0) handle = _nextTexture++;
        it = _textures.emplace(handle, ot).first;
    }

    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = it->second.tex; dst.aspect = WGPUTextureAspect_All; dst.origin = { 0, 0, 0 };
    WGPUTexelCopyBufferLayout lay = {};
    lay.bytesPerRow = width * 4; lay.rowsPerImage = height;
    WGPUExtent3D ext = { width, height, 1 };
    wgpuQueueWriteTexture(_queue, &dst, rgba, size_t(width) * height * 4, &lay, &ext);
    return handle;
}


TextureHandle WebGpuRenderer::uploadTextureRegion(TextureHandle handle, const uint8_t* rgba,
                                                  uint32_t texWidth, uint32_t texHeight,
                                                  uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!_device || !rgba || handle == 0 || width == 0 || height == 0) return handle;

    auto it = _textures.find(handle);
    if (it == _textures.end()) return handle;

    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = it->second.tex; dst.aspect = WGPUTextureAspect_All; 
    dst.origin = { x, y, 0 };
    
    WGPUTexelCopyBufferLayout lay = {};
    lay.bytesPerRow = texWidth * 4; lay.rowsPerImage = texHeight; lay.offset = (y * texWidth + x) * 4;
    
    WGPUExtent3D ext = { width, height, 1 };
    
    // Some WebGPU implementations require buffer copies rather than queueWriteTexture with offset
    // wgpuQueueWriteTexture is allowed to specify data layout offset, but we just pass the shifted pointer
    lay.offset = 0;
    const uint8_t* subdata = rgba + (y * texWidth + x) * 4;
    // Wait, wgpuQueueWriteTexture expects the pointer to point to the start of the data being written, 
    // AND lay.bytesPerRow must be the full row pitch. 
    wgpuQueueWriteTexture(_queue, &dst, subdata, ((height - 1) * texWidth + width) * 4, &lay, &ext);
    
    return handle;
}

void WebGpuRenderer::releaseTexture(TextureHandle handle) {
    auto it = _textures.find(handle);
    if (it == _textures.end()) return;
    wgpuTextureViewRelease(it->second.view);
    wgpuTextureRelease(it->second.tex);
    _textures.erase(it);
}

bool WebGpuRenderer::readPixels(uint8_t* outRgba, uint32_t width, uint32_t height) {
    if (!outRgba || width == 0 || height == 0) return false;
    if (!_device || !_queue || !_surfaceTex) return false;

    // WebGPU requires bytesPerRow to be 256-byte aligned
    const uint32_t bytesPerRow = (width * 4 + 255) & ~255;
    const uint64_t bufferSize = static_cast<uint64_t>(bytesPerRow) * height;

    if (!_readbackBuffer || _readbackBufferSize < bufferSize) {
        if (_readbackBuffer) {
            wgpuBufferRelease(_readbackBuffer);
            _readbackBuffer = nullptr;
        }
        WGPUBufferDescriptor desc = {};
        desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        desc.size = bufferSize;
        _readbackBuffer = wgpuDeviceCreateBuffer(_device, &desc);
        _readbackBufferSize = bufferSize;
    }

    if (!_readbackBuffer) return false;

    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(_device, nullptr);

    WGPUTexelCopyTextureInfo src = {};
    src.texture = _surfaceTex;
    src.mipLevel = 0;
    src.origin = { 0, 0, 0 };
    src.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = _readbackBuffer;
    dst.layout.offset = 0;
    dst.layout.bytesPerRow = bytesPerRow;
    dst.layout.rowsPerImage = height;

    WGPUExtent3D copySize = { width, height, 1 };
    wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &copySize);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(_queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    struct MapResult {
        bool done = false;
        bool ok = false;
    };
    MapResult mr;
    auto onMap = [](WGPUMapAsyncStatus status, WGPUStringView, void* ud, void*) {
        auto* r = static_cast<MapResult*>(ud);
        r->ok = (status == WGPUMapAsyncStatus_Success);
        r->done = true;
    };

    WGPUBufferMapCallbackInfo ci = {};
    ci.mode = WGPUCallbackMode_AllowProcessEvents;
    ci.callback = onMap;
    ci.userdata1 = &mr;

    wgpuBufferMapAsync(_readbackBuffer, WGPUMapMode_Read, 0, bufferSize, ci);
    while (!mr.done) {
        wgpuDevicePoll(_device, true, nullptr);
    }

    if (!mr.ok) {
        return false;
    }

    const uint8_t* mapped = static_cast<const uint8_t*>(
        wgpuBufferGetConstMappedRange(_readbackBuffer, 0, bufferSize));
    if (!mapped) {
        wgpuBufferUnmap(_readbackBuffer);
        return false;
    }

    const bool isBgra = (_colorFormat == WGPUTextureFormat_BGRA8Unorm);
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* srcRow = mapped + y * bytesPerRow;
        uint8_t* dstRow = outRgba + y * (width * 4);
        if (isBgra) {
            for (uint32_t x = 0; x < width; ++x) {
                dstRow[x * 4 + 0] = srcRow[x * 4 + 2]; // R
                dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
                dstRow[x * 4 + 2] = srcRow[x * 4 + 0]; // B
                dstRow[x * 4 + 3] = srcRow[x * 4 + 3]; // A
            }
        } else {
            std::memcpy(dstRow, srcRow, width * 4);
        }
    }

    wgpuBufferUnmap(_readbackBuffer);
    return true;
}

