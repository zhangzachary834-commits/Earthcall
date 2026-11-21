// Scratch-only observation of existing execution; never an authored property.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
static WGPUBuffer ecCurrentFacts=nullptr,ecSetupRows=nullptr;
static uint32_t ecWidth=0,ecHeight=0;static uint64_t ecCurrentBytes=0,ecSetupBytes=0,ecRowsCopied=0;
static bool ecObserve=false;
bool ecObserveEnabled(){return ecObserve;}
WGPUBuffer ecObserveFacts(){return ecCurrentFacts;}
uint64_t ecObserveFactsBytes(){return ecCurrentBytes;}
static std::string ecHash(const void* data,size_t bytes){unsigned char h[32];SHA256(static_cast<const unsigned char*>(data),bytes,h);char text[65];for(unsigned i=0;i<32;++i)std::sprintf(text+2*i,"%02x",h[i]);return text;}
static void ecObserveInit(wgpu::Device& gpu,uint32_t w,uint32_t h){
 ecWidth=w;ecHeight=h;ecObserve=std::getenv("EC_6EW_OBSERVE")!=nullptr;if(!ecObserve)return;
 ecCurrentBytes=uint64_t((w+3)/4)*((h+3)/4)*64;ecSetupBytes=uint64_t(w)*((h+3)/4)*48;
 auto create=[&](uint64_t bytes,WGPUBufferUsage usage){WGPUBufferDescriptor d={};d.size=bytes;d.usage=usage|WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc;auto b=wgpuDeviceCreateBuffer(gpu.device,&d);std::vector<uint32_t> poison(bytes/4,0x7fc000a5u);wgpuQueueWriteBuffer(gpu.queue,b,0,poison.data(),bytes);return b;};
 ecCurrentFacts=create(ecCurrentBytes,WGPUBufferUsage_Storage);ecSetupRows=create(ecSetupBytes,0);
}
void ecObserveCopy(WGPUCommandEncoder encoder,WGPUBuffer rays,uint32_t start,uint32_t rows,uint32_t w,uint32_t h){
 if(!ecObserve)return;
 for(uint32_t y=start;y<std::min(start+rows,h);y+=4){wgpuCommandEncoderCopyBufferToBuffer(encoder,rays,uint64_t(y-start)*w*48,ecSetupRows,uint64_t(y/4)*w*48,uint64_t(w)*48);++ecRowsCopied;}
}
std::string ecObserveShader(std::string s){
 auto once=[&](const std::string& a,const std::string& b){auto p=s.find(a);if(p==std::string::npos||s.find(a,p+a.size())!=std::string::npos)std::abort();s.replace(p,a.size(),b);};
 const std::string depth="@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;";
 once(depth,depth+"\n@group(0) @binding(3) var<storage,read_write> ecFacts:array<vec4<u32>>;\n");
 const std::string fs="@fragment\nfn fs(";
 const std::string record=R"WGSL(
fn ecRecord(position:vec4<f32>,ro:vec3<f32>,rd:vec3<f32>,t0:f32,t1:f32,status:u32,depth:f32){
 let x=u32(position.x);let y=u32(position.y);if(x%4u!=0u||y%4u!=0u){return;}
 let i=((y/4u)*((u32(u.viewport.x)+3u)/4u)+x/4u)*4u;
 ecFacts[i]=bitcast<vec4<u32>>(vec4<f32>(ro,t0));ecFacts[i+1u]=bitcast<vec4<u32>>(vec4<f32>(rd,t1));
 ecFacts[i+2u]=vec4<u32>(1u,status,bitcast<u32>(depth),0u);ecFacts[i+3u]=bitcast<vec4<u32>>(vec4<f32>(1,0,0,0));
}
)WGSL";
 once(fs,record+fs);
 const std::string discard="    if (t1 <= t0) { discard; }";auto a=s.find(discard),b=s.find(discard,a+discard.size());if(a==std::string::npos||b==std::string::npos||s.find(discard,b+discard.size())!=std::string::npos)std::abort();
 s.replace(b,discard.size(),"    if(t1<=t0){ecRecord(in.position,ro,rd,t0,t1,2u,sceneDepth);discard;}");s.replace(a,discard.size(),"    if(t1<=t0){ecRecord(in.position,ro,rd,t0,t1,1u,-1.0);discard;}");
 const std::string anchor="    var transmittance = 1.0;";once(anchor,"    ecRecord(in.position,ro,rd,t0,t1,3u,sceneDepth);\n"+anchor);
 const std::string alpha="    let alpha = 1.0 - transmittance;";
 once(alpha,R"WGSL(    {
 let x=u32(in.position.x);let y=u32(in.position.y);
 if(x%4u==0u&&y%4u==0u){let i=((y/4u)*((u32(u.viewport.x)+3u)/4u)+x/4u)*4u;ecFacts[i+3u]=bitcast<vec4<u32>>(vec4<f32>(transmittance,integratedRadiance));}
    }
)WGSL"+alpha);
 return s;
}
static std::vector<uint8_t> ecReadObserved(wgpu::Device& gpu,WGPUBuffer source,uint64_t bytes){
 WGPUBufferDescriptor d={};d.size=bytes;d.usage=WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst;auto out=wgpuDeviceCreateBuffer(gpu.device,&d);
 auto e=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(e,source,0,out,0,bytes);auto c=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(gpu.queue,1,&c);wgpuCommandBufferRelease(c);wgpuCommandEncoderRelease(e);
 MapResult result;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&result;wgpuBufferMapAsync(out,WGPUMapMode_Read,0,bytes,cb);while(!result.done)wgpuInstanceProcessEvents(gpu.instance);if(!result.ok)std::abort();
 const auto* p=static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(out,0,bytes));std::vector<uint8_t> data(p,p+bytes);wgpuBufferUnmap(out);wgpuBufferRelease(out);return data;
}
static void ecObservedFinish(WebGpuRenderer& renderer,wgpu::Device& gpu,const char* path){
 nlohmann::json info={{"width",ecWidth},{"height",ecHeight},{"stride",4},{"observe",ecObserve},{"copied_rows",ecRowsCopied}};
 auto save=[&](const char* suffix,const std::vector<uint8_t>& bytes){std::ofstream(std::string(path)+suffix,std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()),bytes.size());};
 const auto* k=renderer.routedTransportKernelForDiagnostics();
 if(k&&k->publication){
  auto pub=ecReadObserved(gpu,k->publication,uint64_t(ecWidth)*ecHeight*16);info["full_publication_sha256"]=ecHash(pub.data(),pub.size());info["full_publication_bytes"]=pub.size();
  std::vector<uint8_t> sampled;for(uint32_t y=0;y<ecHeight;y+=4)for(uint32_t x=0;x<ecWidth;x+=4){auto p=pub.begin()+(uint64_t(y)*ecWidth+x)*16;sampled.insert(sampled.end(),p,p+16);}save(".publication.bin",sampled);
  if(ecObserve){auto all=ecReadObserved(gpu,ecSetupRows,ecSetupBytes);std::vector<uint8_t> rays;for(uint32_t y=0;y<ecHeight;y+=4)for(uint32_t x=0;x<ecWidth;x+=4){auto p=all.begin()+(uint64_t(y/4)*ecWidth+x)*48;rays.insert(rays.end(),p,p+48);}save(".rays.bin",rays);}
 }else if(ecObserve){save(".current.bin",ecReadObserved(gpu,ecCurrentFacts,ecCurrentBytes));}
 std::ofstream(std::string(path)+".observer.json")<<info.dump(2)<<"\n";
 if(ecCurrentFacts)wgpuBufferRelease(ecCurrentFacts);if(ecSetupRows)wgpuBufferRelease(ecSetupRows);
}
