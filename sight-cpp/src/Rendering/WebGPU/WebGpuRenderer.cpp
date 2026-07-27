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
    lightDir:  vec4<f32>,
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
    let L = normalize(u.lightDir.xyz);
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
    glm::vec4 lightDir;
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

// A flat pipeline: position-only vertex, one uniform, chosen topology + blend,
// depth tested but NOT written (overlays must not occlude each other).
WGPURenderPipeline makeFlatPipeline(WGPUDevice dev, WGPUPipelineLayout layout,
                                    WGPUShaderModule shader, WGPUPrimitiveTopology topo,
                                    WGPUBlendFactor srcF, WGPUBlendFactor dstF,
                                    WGPUTextureFormat colorFormat) {
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

    WGPUDepthStencilState ds = {};
    ds.format = WGPUTextureFormat_Depth24Plus;
    ds.depthWriteEnabled = WGPUOptionalBool_False; // read depth, don't write
    ds.depthCompare = WGPUCompareFunction_Less;

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

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = _colorFormat;
    colorTarget.writeMask = WGPUColorWriteMask_All;
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

    _overlayAddPipe = makeFlatPipeline(_device, flatLayout, flatShader,
        WGPUPrimitiveTopology_TriangleList, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_One, _colorFormat);
    _overlayAlphaPipe = makeFlatPipeline(_device, flatLayout, flatShader,
        WGPUPrimitiveTopology_TriangleList, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha, _colorFormat);
    _linesPipe = makeFlatPipeline(_device, flatLayout, flatShader,
        WGPUPrimitiveTopology_LineList, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha, _colorFormat);
    wgpuPipelineLayoutRelease(flatLayout);
    wgpuShaderModuleRelease(flatShader);

    return _sampler && _whiteView && _overlayAddPipe && _overlayAlphaPipe && _linesPipe;
}

void WebGpuRenderer::shutdown() {
    releaseFrameResources();
    if (_depthView) { wgpuTextureViewRelease(_depthView); _depthView = nullptr; }
    if (_depthTex)  { wgpuTextureRelease(_depthTex); _depthTex = nullptr; }
    _depthW = _depthH = 0;
    if (_whiteView) { wgpuTextureViewRelease(_whiteView); _whiteView = nullptr; }
    if (_whiteTex)  { wgpuTextureRelease(_whiteTex); _whiteTex = nullptr; }
    if (_sampler)   { wgpuSamplerRelease(_sampler); _sampler = nullptr; }
    if (_overlayAddPipe)   { wgpuRenderPipelineRelease(_overlayAddPipe); _overlayAddPipe = nullptr; }
    if (_overlayAlphaPipe) { wgpuRenderPipelineRelease(_overlayAlphaPipe); _overlayAlphaPipe = nullptr; }
    if (_linesPipe)        { wgpuRenderPipelineRelease(_linesPipe); _linesPipe = nullptr; }
    if (_flatBgl) { wgpuBindGroupLayoutRelease(_flatBgl); _flatBgl = nullptr; }
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

void WebGpuRenderer::applyBeginFrame(uint32_t /*width*/, uint32_t /*height*/, const glm::vec4& /*clear*/) {
    // Live/on-screen frames need the window surface texture, configured in the
    // CAMetalLayer phase. Until then this is a no-op so the interface is complete
    // and GameRender can call it harmlessly; offscreen rendering uses
    // beginFrameOffscreen with an explicit target.
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
    u.lightDir  = glm::vec4(glm::normalize(glm::vec3(2.0f, 5.0f, 2.0f)), 0.0f); // matches ShadingSystem offset
    u.params    = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);
    u.eyePos    = glm::vec4(_eyePos, 1.0f);

    WGPUBufferDescriptor ubDesc = {};
    ubDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    ubDesc.size = sizeof(MeshUniforms);
    WGPUBuffer ubuf = wgpuDeviceCreateBuffer(_device, &ubDesc);
    wgpuQueueWriteBuffer(_queue, ubuf, 0, &u, sizeof(MeshUniforms));

    // Albedo: upload the material's pixels to a WGPU texture, or fall back to white.
    WGPUTextureView albedoView = _whiteView;
    if (mat.albedoPixels && mat.albedoSize > 0) {
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
                               Blend /*blend*/) {
    // WebGPU line primitives are always 1px — the OpenGL multi-pass width/glow does
    // not translate; the wireframe still reads clearly. Each segment is two points.
    // TODO(M5): `blend` is ignored — _linesPipe is alpha-only, so additive line
    // draws (the gravity-field arrows) render alpha until a second pipeline exists.
    std::vector<glm::vec3> verts;
    verts.reserve(segments.size() * 2);
    for (const auto& s : segments) { verts.push_back(s.first); verts.push_back(s.second); }
    drawFlat(_linesPipe, verts, _viewProj * _model, color);
}

void WebGpuRenderer::drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                                 float scale, bool additive) {
    std::vector<glm::vec3> verts;
    verts.reserve(mesh.tris.size());
    for (const auto& v : mesh.tris) verts.push_back(v.pos);
    glm::mat4 mvp = _viewProj * _model * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    drawFlat(additive ? _overlayAddPipe : _overlayAlphaPipe, verts, mvp, color);
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
// Flat-colour primitives — NOT YET IMPLEMENTED (Milestone 5, 2D pipeline).
//
// They exist so the OpenGL call sites can migrate onto the boundary ahead of this
// backend without breaking the webgpu-* targets. Each warns once instead of
// silently drawing nothing, so a half-finished backend is loud rather than
// mysterious. Replacing these with a real ortho pipeline is the next step.
// ---------------------------------------------------------------------------

namespace {
void warnOnce(const char* verb) {
    static std::set<std::string> seen;
    if (seen.insert(verb).second)
        std::fprintf(stderr, "[WebGpuRenderer] %s is not implemented yet — nothing drawn.\n", verb);
}
} // namespace

void WebGpuRenderer::drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) {
    warnOnce("drawSolid");
}
void WebGpuRenderer::begin2D(uint32_t, uint32_t) { warnOnce("begin2D"); }
void WebGpuRenderer::end2D() {}
void WebGpuRenderer::drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) {
    warnOnce("drawTris2D");
}
void WebGpuRenderer::drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) {
    warnOnce("drawLines2D");
}
void WebGpuRenderer::drawImage2D(const uint8_t*, uint32_t, uint32_t,
                                 const glm::vec4&, const glm::vec4&) {
    warnOnce("drawImage2D");
}
