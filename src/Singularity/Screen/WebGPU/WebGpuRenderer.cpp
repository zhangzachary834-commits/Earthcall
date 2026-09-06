#include <chrono>
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

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

    // group(2): the SDF per-instance storage array, plus (binding 1) the
    // shared min/max heightfield-grid cells buffer (Phase C) -- fragment-only,
    // since only the marcher's DDA skip (fs) ever reads it.
    WGPUBindGroupLayoutEntry sdfInstEntry[2] = {};
    sdfInstEntry[0].binding = 0;
    sdfInstEntry[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    sdfInstEntry[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    sdfInstEntry[1].binding = 1;
    sdfInstEntry[1].visibility = WGPUShaderStage_Fragment;
    sdfInstEntry[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    WGPUBindGroupLayoutDescriptor sdfInstBglDesc = {};
    sdfInstBglDesc.entryCount = 2;
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
    if (_sdfInstanceBgl) { wgpuBindGroupLayoutRelease(_sdfInstanceBgl); _sdfInstanceBgl = nullptr; }
    _meshBatches.clear();
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

    _gpuTimestampPeriodNs = wgpuQueueGetTimestampPeriod(_queue);
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
    td.usage = WGPUTextureUsage_RenderAttachment;
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
    } else if (mat.albedoPixels && mat.albedoSize > 0) {
        const uint32_t s = static_cast<uint32_t>(mat.albedoSize);
        WGPUTextureDescriptor atd = {};
        atd.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        atd.dimension = WGPUTextureDimension_2D;
        atd.size = { s, s, 1 };
        atd.format = WGPUTextureFormat_RGBA8Unorm;
        atd.mipLevelCount = 1; atd.sampleCount = 1;
        WGPUTexture atex = wgpuDeviceCreateTexture(_device, &atd);
        WGPUTexelCopyTextureInfo adst = {};
        adst.texture = atex; adst.aspect = WGPUTextureAspect_All; adst.origin = { 0, 0, 0 };
        WGPUTexelCopyBufferLayout alay = {};
        alay.bytesPerRow = s * 4; alay.rowsPerImage = s;
        WGPUExtent3D asize = { s, s, 1 };
        wgpuQueueWriteTexture(_queue, &adst, mat.albedoPixels, size_t(s) * s * 4, &alay, &asize);
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
    glm::vec4 lightPos;
    glm::vec4 eyePos;
    glm::vec4 limits;   // x = far-plane distance in world units; see struct RU
};
} // namespace

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

    WGPUBindGroupLayoutEntry be[2] = {};
    be[0].binding = 0;
    be[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    be[0].buffer.type = WGPUBufferBindingType_Uniform;
    be[0].buffer.minBindingSize = sizeof(SdfGlobalUniforms);
    be[1].binding = 1;
    be[1].visibility = WGPUShaderStage_Fragment;
    be[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    WGPUBindGroupLayoutDescriptor bgld = {};
    bgld.entryCount = 2; bgld.entries = be;

    SdfPipeline out;
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

void WebGpuRenderer::drawImplicit(const geom::SdfNode& field, const glm::vec3& extent,
                                  const RenderMaterial& mat,
                                  const geom::FieldNode* fieldNode,
                                  uint64_t memoId,
                                  uint32_t memoRevision,
                                  const geom::HeightGrid* heightGrid) {
    if (!_pass) return;

    // Memoize the WGSL string generation and pipeline lookup.
    sdfwgsl::Program prog;
    const SdfPipeline* sp = nullptr;
    bool needsCompile = true;
    if (memoId != 0) {
        auto& entry = _programCache[memoId];
        if (entry.revision == memoRevision) {
            prog = entry.prog;
            sp = entry.sp;
            needsCompile = false;
        }
    }
    if (needsCompile) {
        prog = sdfwgsl::compile(field, fieldNode);
        if (!prog.ok) {
            std::fprintf(stderr, "[WebGPU] SdfWgsl compile refused: %s\n", prog.error.c_str());
            return;
        }
        sp = sdfPipeline(prog.wgsl);
        if (memoId != 0) {
            auto& entry = _programCache[memoId];
            entry.revision = memoRevision;
            entry.prog = prog;
            entry.sp = sp;
        }
    }
    if (!sp) return;

    // The bounding cube, shared by every field: the vertex shader scales it by the
    // field extent, so one buffer serves all of them.
    if (!_sdfCubeVerts) {
        const float h = 1.0f;
        const glm::vec3 c[8] = {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}};
        const int idx[36] = {
            0,1,2, 0,2,3,  4,6,5, 4,7,6,  0,4,5, 0,5,1,
            3,2,6, 3,6,7,  0,3,7, 0,7,4,  1,5,6, 1,6,2};
        std::vector<glm::vec3> tris(36);
        for (int i = 0; i < 36; ++i) tris[i] = c[idx[i]];

        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bd.size = tris.size() * sizeof(glm::vec3);
        _sdfCubeVerts = wgpuDeviceCreateBuffer(_device, &bd);
        wgpuQueueWriteBuffer(_queue, _sdfCubeVerts, 0, tris.data(), bd.size);
    }

    SdfInstanceData inst;
    inst.model = _model;
    inst.invModel = glm::inverse(_model);
    
    glm::vec3 albedo(1.0f);
    if (mat.albedoPixels && mat.albedoSize > 0) {
        const int   half = mat.albedoSize / 2;
        const size_t idx = (size_t(half) * mat.albedoSize + half) * 4;
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
    const bool isProvenHeightfield = geom::isHeightfieldExpr(field, nullptr);
    const bool hasConservativeHeightGrid = heightGrid &&
                                           heightGrid->dimX > 0 && heightGrid->dimZ > 0;
    // DDA traversal is quarantined after the native Metal sweep found that its
    // candidate-cell hand-off could miss grazing roots. Keep computing a
    // conservative grid for the proof/test seam, but do not upload or select
    // the traversal until the full on/off camera corpus is exact. This is not a
    // performance regression for the saved Perlin floor: it is y-dependent and
    // was already ineligible for a grid.
    constexpr bool kHeightGridDdaTraversalVerified = false;
    const bool gridActive = kHeightGridDdaTraversalVerified &&
                            _heightGridDdaEnabled && isProvenHeightfield &&
                            hasConservativeHeightGrid &&
                            !heightGrid->cells.empty();
    // A grid's cell coordinates are authored over `extent` by Object::rebuildHeightGrid.
    // Keep that exact interval for every proved heightfield, whether DDA traversal
    // is enabled or disabled: toggling the skip may not quietly change the proxy
    // coverage that the comparison is meant to judge. Other fields retain the
    // small rasterization guard band for roots on an authored boundary.
    const glm::vec3 proxyExtent = isProvenHeightfield ? extent : extent * 1.05f;
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
    const float damping = prog.needsGradientStep ? 0.25f : 1.0f;
    // misc.x is a distinct proof bit: damping selects the step policy, while
    // only a structurally-proved y-h(x,z) field may use heightfield-only
    // planar/vertical early exits. Keep the two latches separate so a generic
    // gradient-marched Perlin expression cannot inherit those assumptions.
    inst.misc = glm::vec4(isProvenHeightfield ? 1.0f : 0.0f,
                          1e-4f, maxDim * 8.0f, damping);

    inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[sp].size());

    _sdfBatches[sp].push_back(inst);
    _sdfParamsBatches[sp].insert(_sdfParamsBatches[sp].end(), prog.params.begin(), prog.params.end());

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
    if (_sdfBatches.empty()) return;
    if (!_pass) { _sdfBatches.clear(); _sdfParamsBatches.clear(); return; }
    
    // Global uniforms for SDFs
    SdfGlobalUniforms u;
    u.viewProj = _viewProj;
    u.lightPos = glm::vec4(lightPos(), 1.0f);
    u.eyePos = glm::vec4(_eyePos, 1.0f);
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
    u.limits = glm::vec4(farDist, 0.0f, 0.0f, 0.0f);

    auto uAlloc = bufferPool().suballocateUniform(&u, sizeof(SdfGlobalUniforms));

    for (auto& kv : _sdfBatches) {
        const SdfPipeline* sp = kv.first;
        const auto& instances = kv.second;
        if (instances.empty()) continue;
        
        const auto& params = _sdfParamsBatches[sp];
        
        auto pAlloc = bufferPool().suballocateStorage(params.data(), params.size() * sizeof(float));
        auto instAlloc = bufferPool().suballocateStorage(instances.data(), instances.size() * sizeof(SdfInstanceData));

        // Group 0: Globals and Parameters
        WGPUBindGroupEntry bge[2] = {};
        bge[0].binding = 0; bge[0].buffer = uAlloc.buffer; bge[0].offset = uAlloc.offset; bge[0].size = uAlloc.size;
        bge[1].binding = 1; bge[1].buffer = pAlloc.buffer; bge[1].offset = pAlloc.offset; bge[1].size = pAlloc.size;
        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = sp->bgl; bgd.entryCount = 2; bgd.entries = bge;
        WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
        _frameBindGroups.push_back(bg);

        // Group 1: Instances (binding 0) + min/max heightfield-grid cells
        // (binding 1, Phase C). A storage array of length zero is invalid, same
        // as the params buffer above -- when nothing in this pipeline's batch
        // built a grid, one unused cell keeps the binding legal without the
        // shader having to know (every instance's heightGridDimX/Z stay 0, so
        // it is never indexed).
        auto& hgCells = _sdfHeightGridBatches[sp];
        if (hgCells.empty()) hgCells.push_back(glm::vec2(0.0f));
        auto hgAlloc = bufferPool().suballocateStorage(hgCells.data(), hgCells.size() * sizeof(glm::vec2));

        WGPUBindGroupEntry ibge[2] = {};
        ibge[0].binding = 0; ibge[0].buffer = instAlloc.buffer; ibge[0].offset = instAlloc.offset; ibge[0].size = instAlloc.size;
        ibge[1].binding = 1; ibge[1].buffer = hgAlloc.buffer; ibge[1].offset = hgAlloc.offset; ibge[1].size = hgAlloc.size;
        WGPUBindGroupDescriptor ibgDesc = {};
        ibgDesc.layout = _sdfInstanceBgl; ibgDesc.entryCount = 2; ibgDesc.entries = ibge;
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
    _sdfBatches.clear();
    _sdfParamsBatches.clear();
    _sdfHeightGridBatches.clear();
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

    auto& fs = mutableFrameStats();
    fs.vramAllocatedBytes = bufferPool().totalVramBytes() + _meshCache.totalCachedBytes();
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

void WebGpuRenderer::releaseTexture(TextureHandle handle) {
    auto it = _textures.find(handle);
    if (it == _textures.end()) return;
    wgpuTextureViewRelease(it->second.view);
    wgpuTextureRelease(it->second.tex);
    _textures.erase(it);
}
