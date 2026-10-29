// Alternate Current observation through float render attachments: no storage writes
// or per-pixel diagnostic branches inside the integration loop. Scratch only.
static std::array<WGPUTexture,3> ecFloatTextures{};
static std::array<WGPUTextureView,3> ecFloatViews{};
static WGPUBuffer ecMrtModeBuffer=nullptr;static uint32_t ecMrtMode=0;
WGPUBuffer ecObserveMrtMode(){return ecMrtModeBuffer;}
WGPUTextureView ecObserveMrtView(uint32_t i){return ecFloatViews[i];}
static void ecMrtInit(wgpu::Device& gpu){
 if(!ecObserveEnabled())return;WGPULimits limits={};wgpuDeviceGetLimits(gpu.device,&limits);std::printf("MRT capacity attachments=%u bytes=%u\n",limits.maxColorAttachments,limits.maxColorAttachmentBytesPerSample);
 if(limits.maxColorAttachments<2||limits.maxColorAttachmentBytesPerSample<20)std::abort();
 ecMrtMode=std::getenv("EC_6EW_MRT_MODE")?std::atoi(std::getenv("EC_6EW_MRT_MODE")):0;std::array<uint32_t,4> mode{ecMrtMode,0,0,0};WGPUBufferDescriptor md={};md.size=16;md.usage=WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;ecMrtModeBuffer=wgpuDeviceCreateBuffer(gpu.device,&md);wgpuQueueWriteBuffer(gpu.queue,ecMrtModeBuffer,0,mode.data(),16);
 for(unsigned i=0;i<1;++i){WGPUTextureDescriptor d={};d.dimension=WGPUTextureDimension_2D;d.size={ecWidth,ecHeight,1};d.format=WGPUTextureFormat_RGBA32Float;d.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;d.mipLevelCount=1;d.sampleCount=1;ecFloatTextures[i]=wgpuDeviceCreateTexture(gpu.device,&d);ecFloatViews[i]=wgpuTextureCreateView(ecFloatTextures[i],nullptr);}
}
std::string ecMrtShader(std::string s){
 auto once=[&](const std::string& a,const std::string& b){auto p=s.find(a);if(p==std::string::npos||s.find(a,p+a.size())!=std::string::npos)std::abort();s.replace(p,a.size(),b);};
 const std::string old="@fragment\nfn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {";
 once(old,"@group(0) @binding(3) var<uniform> ecMrtControl:vec4<u32>;\nstruct ECObservedOut {@location(0) color:vec4<f32>,@location(1) record:vec4<f32>};\nfn ecObservedValue(ro:vec3<f32>,rd:vec3<f32>,t0:f32,t1:f32,T:f32,L:vec3<f32>)->vec4<f32>{if(ecMrtControl.x==0u){return vec4<f32>(ro,t0);}if(ecMrtControl.x==1u){return vec4<f32>(rd,t1);}return vec4<f32>(T,L); }\n@fragment\nfn fs(in: VolumeVSOut) -> ECObservedOut {");
 once("    if (alpha <= 1e-5 && integratedMagnitude <= 1e-6) { discard; }","    if(alpha<=1e-5 && integratedMagnitude<=1e-6){return ECObservedOut(vec4<f32>(0),ecObservedValue(ro,rd,t0,t1,transmittance,integratedRadiance));}");
 once("    return vec4<f32>(integratedRadiance, alpha);","    return ECObservedOut(vec4<f32>(integratedRadiance,alpha),ecObservedValue(ro,rd,t0,t1,transmittance,integratedRadiance));");
 return s;
}
static std::vector<uint8_t> ecFloatTexture(wgpu::Device& gpu,WGPUTexture texture){
 uint32_t stride=(ecWidth*16+255)&~255u;WGPUBufferDescriptor d={};d.size=uint64_t(stride)*ecHeight;d.usage=WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst;auto b=wgpuDeviceCreateBuffer(gpu.device,&d);auto e=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
 WGPUTexelCopyTextureInfo src={};src.texture=texture;src.aspect=WGPUTextureAspect_All;WGPUTexelCopyBufferInfo dst={};dst.buffer=b;dst.layout.bytesPerRow=stride;dst.layout.rowsPerImage=ecHeight;WGPUExtent3D extent={ecWidth,ecHeight,1};wgpuCommandEncoderCopyTextureToBuffer(e,&src,&dst,&extent);auto c=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(gpu.queue,1,&c);wgpuCommandBufferRelease(c);wgpuCommandEncoderRelease(e);
 auto raw=ecReadObserved(gpu,b,d.size);wgpuBufferRelease(b);return raw;
}
static void ecMrtFinish(WebGpuRenderer& renderer,wgpu::Device& gpu,const char* path){
 ecObservedFinish(renderer,gpu,path);
 if(ecObserveEnabled()&&!renderer.routedTransportKernelForDiagnostics()){
  auto raw=ecFloatTexture(gpu,ecFloatTextures[0]);uint32_t stride=(ecWidth*16+255)&~255u;std::vector<uint8_t> sampled;
  for(uint32_t y=0;y<ecHeight;y+=4)for(uint32_t x=0;x<ecWidth;x+=4){auto p=raw.begin()+uint64_t(y)*stride+x*16;sampled.insert(sampled.end(),p,p+16);}
  std::ofstream(std::string(path)+".mrt.bin",std::ios::binary).write(reinterpret_cast<const char*>(sampled.data()),sampled.size());
  std::remove((std::string(path)+".current.bin").c_str());
  auto info=nlohmann::json::parse(std::ifstream(std::string(path)+".observer.json"));info["mrt_mode"]=ecMrtMode;std::ofstream(std::string(path)+".observer.json")<<info.dump(2)<<"\n";
 }
 if(ecMrtModeBuffer)wgpuBufferRelease(ecMrtModeBuffer);
 for(unsigned i=0;i<3;++i){if(ecFloatViews[i])wgpuTextureViewRelease(ecFloatViews[i]);if(ecFloatTextures[i])wgpuTextureRelease(ecFloatTextures[i]);}
}
