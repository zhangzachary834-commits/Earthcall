import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

particle_init = """    // Particle pipeline
    _particleShader = loadShader(_device, kParticleWGSL, "particle");
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
"""

content = content.replace("    _bufferPool.init(_device, _queue);", particle_init + "\n    _bufferPool.init(_device, _queue);")

content = content.replace("return _sampler && _whiteView && _flatShader && _flatLayout && _imagePipe;", "return _sampler && _whiteView && _flatShader && _flatLayout && _imagePipe && _particlePipe;")

with open(file_path, "w") as f:
    f.write(content)
print("Particle init added")
