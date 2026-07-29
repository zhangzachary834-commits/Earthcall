#include "Rendering/WebGPU/WebGpuRenderer.hpp"
#include "Rendering/WebGPU/WgpuDevice.hpp"

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
const char* kMeshWGSL = R"(
struct U {
    viewProj:  mat4x4<f32>,
    model:     mat4x4<f32>,
    normalMat: mat4x4<f32>,
    baseColor: vec4<f32>,
    lightPos:  vec4<f32>,   // world-space POSITION (GL_LIGHT0 is positional)
    params:    vec4<f32>,   // x=ambient, y=diffuse, z=specular, w=shininess
    eyePos:    vec4<f32>,
};
@group(0) @binding(0) var<uniform> u: U;
@group(0) @binding(1) var albedoTex: texture_2d<f32>;
@group(0) @binding(2) var albedoSamp: sampler;

struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldNormal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
};

@vertex
fn vs_main(@location(0) pos: vec3<f32>, @location(1) normal: vec3<f32>,
           @location(2) uv: vec2<f32>) -> VSOut {
    var out: VSOut;
    let world = u.model * vec4<f32>(pos, 1.0);
    out.clip = u.viewProj * world;
    out.worldNormal = (u.normalMat * vec4<f32>(normal, 0.0)).xyz;
    out.uv = uv;
    out.worldPos = world.xyz;
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
    let rgb = u.baseColor.rgb * texel.rgb * lit + vec3<f32>(spec);
    return vec4<f32>(rgb, u.baseColor.a * texel.a);
}
)";

// std140-compatible: all members are 16-byte aligned, so this matches the WGSL
// uniform block byte-for-byte. glm and WGSL are both column-major.
struct MeshUniforms {
    glm::mat4 viewProj;
    glm::mat4 model;
    glm::mat4 normalMat;
    glm::vec4 baseColor;
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
    _colorFormat = colorFormat;
    if (!_device || !_queue) return false;

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

    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &_bgl;
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
    ipd.fragment = &ifrag;
    _imagePipe = wgpuDeviceCreateRenderPipeline(_device, &ipd);

    return _sampler && _whiteView && _flatShader && _flatLayout && _imagePipe;
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
    releaseFrameResources();
    if (_depthView) { wgpuTextureViewRelease(_depthView); _depthView = nullptr; }
    if (_depthTex)  { wgpuTextureRelease(_depthTex); _depthTex = nullptr; }
    _depthW = _depthH = 0;
    if (_whiteView) { wgpuTextureViewRelease(_whiteView); _whiteView = nullptr; }
    if (_whiteTex)  { wgpuTextureRelease(_whiteTex); _whiteTex = nullptr; }
    if (_sampler)   { wgpuSamplerRelease(_sampler); _sampler = nullptr; }
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
    wgpuSurfacePresent(_surface);
    wgpuTextureViewRelease(_surfaceView);
    _surfaceView = nullptr;
    _surfaceTex = nullptr; // owned by the surface; not ours to release
}

void WebGpuRenderer::beginFrameOffscreen(WGPUTextureView target, uint32_t width, uint32_t height,
                                         const glm::vec4& clear) {
    ensureDepth(width, height);
    _encoder = wgpuDeviceCreateCommandEncoder(_device, nullptr);
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
    // Each verb sets its own pipeline (mesh vs overlay vs lines), so nothing is
    // pre-bound here — the pass may interleave meshes and overlays per object.
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
    wgpuRenderPassEncoderSetPipeline(_pass, _meshPipeline);

    const size_t vbytes = mesh.tris.size() * sizeof(geom::TessVertex);
    WGPUBufferDescriptor vbDesc = {};
    vbDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    vbDesc.size = vbytes;
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(_device, &vbDesc);
    wgpuQueueWriteBuffer(_queue, vbuf, 0, mesh.tris.data(), vbytes);

    MeshUniforms u;
    u.viewProj  = _viewProj;
    u.model     = _model;
    u.normalMat = glm::transpose(glm::inverse(_model));
    u.baseColor = glm::vec4(mat.baseColor, mat.opacity);
    // The scene's light, as recorded by Renderer::setLight. Previously a hardcoded
    // DIRECTIONAL vector, which diverged from OpenGL's GL_LIGHT0 — that light is
    // positional and follows the camera, so lighting drifted apart as you moved.
    // NOTE: params.x/y still carry the MATERIAL's ambient/diffuse coefficients,
    // whose defaults (0.2/0.8) happen to equal the light's. Under GL_COLOR_MATERIAL
    // the light's coefficients are what actually apply, so a material overriding
    // ambient/diffuse will shade differently here than under OpenGL. Unifying that
    // is a shading-model decision, not a migration step.
    u.lightPos  = glm::vec4(lightPos(), 1.0f);
    u.params    = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);
    u.eyePos    = glm::vec4(_eyePos, 1.0f);

    WGPUBufferDescriptor ubDesc = {};
    ubDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    ubDesc.size = sizeof(MeshUniforms);
    WGPUBuffer ubuf = wgpuDeviceCreateBuffer(_device, &ubDesc);
    wgpuQueueWriteBuffer(_queue, ubuf, 0, &u, sizeof(MeshUniforms));

    // Albedo: upload the material's pixels to a WGPU texture, or fall back to white.
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

    WGPUBindGroupEntry bge[3] = {};
    bge[0].binding = 0; bge[0].buffer = ubuf; bge[0].offset = 0; bge[0].size = sizeof(MeshUniforms);
    bge[1].binding = 1; bge[1].textureView = albedoView;
    bge[2].binding = 2; bge[2].sampler = _sampler;
    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = _bgl; bgDesc.entryCount = 3; bgDesc.entries = bge;
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(_device, &bgDesc);

    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vbuf, 0, vbytes);
    wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(mesh.tris.size()), 1, 0, 0);

    _frameBuffers.push_back(vbuf);
    _frameBuffers.push_back(ubuf);
    _frameBindGroups.push_back(bindGroup);
}

void WebGpuRenderer::drawImplicit(const geom::SdfNode&, float, const RenderMaterial&) {
    // Stub: ObjectRender caches _fieldMesh and calls drawMesh directly, so this is
    // unused today. Milestone 6 implements the raymarcher here (exact SDF render).
    // Kept dependency-free (no tessellateSdf link) until then.
}

void WebGpuRenderer::drawFlat(WGPURenderPipeline pipe, const std::vector<glm::vec3>& verts,
                             const glm::mat4& mvp, const glm::vec4& color) {
    if (!_pass || verts.empty()) return;
    wgpuRenderPassEncoderSetPipeline(_pass, pipe);

    const size_t vbytes = verts.size() * sizeof(glm::vec3);
    WGPUBufferDescriptor vbDesc = {};
    vbDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst; vbDesc.size = vbytes;
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(_device, &vbDesc);
    wgpuQueueWriteBuffer(_queue, vbuf, 0, verts.data(), vbytes);

    FlatUniforms fu{ mvp, color };
    WGPUBufferDescriptor ubDesc = {};
    ubDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst; ubDesc.size = sizeof(FlatUniforms);
    WGPUBuffer ubuf = wgpuDeviceCreateBuffer(_device, &ubDesc);
    wgpuQueueWriteBuffer(_queue, ubuf, 0, &fu, sizeof(FlatUniforms));

    WGPUBindGroupEntry bge = {};
    bge.binding = 0; bge.buffer = ubuf; bge.offset = 0; bge.size = sizeof(FlatUniforms);
    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = _flatBgl; bgDesc.entryCount = 1; bgDesc.entries = &bge;
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(_device, &bgDesc);

    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vbuf, 0, vbytes);
    wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(verts.size()), 1, 0, 0);

    _frameBuffers.push_back(vbuf);
    _frameBuffers.push_back(ubuf);
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

void WebGpuRenderer::endFrame() {
    if (!_pass) return;
    wgpuRenderPassEncoderEnd(_pass);
    wgpuRenderPassEncoderRelease(_pass);
    _pass = nullptr;

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(_encoder, nullptr);
    wgpuQueueSubmit(_queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(_encoder);
    _encoder = nullptr;

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
    WGPUBufferDescriptor vbd = {};
    vbd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    vbd.size = sizeof(quad);
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(_device, &vbd);
    wgpuQueueWriteBuffer(_queue, vbuf, 0, quad, sizeof(quad));
    _frameBuffers.push_back(vbuf);

    FlatUniforms u{};
    u.mvp = _ortho2D;
    u.color = tint;
    WGPUBufferDescriptor ubd = {};
    ubd.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    ubd.size = sizeof(u);
    WGPUBuffer ubuf = wgpuDeviceCreateBuffer(_device, &ubd);
    wgpuQueueWriteBuffer(_queue, ubuf, 0, &u, sizeof(u));
    _frameBuffers.push_back(ubuf);

    WGPUBindGroupEntry bge[3] = {};
    bge[0].binding = 0; bge[0].buffer = ubuf; bge[0].size = sizeof(u);
    bge[1].binding = 1; bge[1].textureView = view;
    bge[2].binding = 2; bge[2].sampler = _sampler;
    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = _imageBgl; bgd.entryCount = 3; bgd.entries = bge;
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
    _frameBindGroups.push_back(bg);

    wgpuRenderPassEncoderSetPipeline(_pass, _imagePipe);
    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, vbuf, 0, sizeof(quad));
    wgpuRenderPassEncoderDraw(_pass, 6, 1, 0, 0);
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
