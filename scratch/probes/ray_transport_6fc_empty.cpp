// Actual native-kernel counterexample to unconditional empty-tile skipping.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp"
#include <json.hpp>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using J=nlohmann::json;
static wgpu::Device gpu;
static void need(bool x,const char* message){if(!x)throw std::runtime_error(message);}
struct B{WGPUBuffer h;size_t size;};
static B buffer(size_t size,WGPUBufferUsage usage,const void* data=nullptr){WGPUBufferDescriptor d={};d.size=size;d.usage=usage|WGPUBufferUsage_CopyDst|((usage&WGPUBufferUsage_MapRead)?WGPUBufferUsage(0):WGPUBufferUsage_CopySrc);auto b=wgpuDeviceCreateBuffer(gpu.device,&d);need(b,"create buffer");if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return{b,size};}
static WGPUComputePipeline pipeline(const std::string& code,const char* name){
 wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);WGPUShaderSourceWGSL s={};s.chain.sType=WGPUSType_ShaderSourceWGSL;s.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&s.chain;auto m=wgpuDeviceCreateShaderModule(gpu.device,&md);WGPUComputePipelineDescriptor d={};d.compute.module=m;d.compute.entryPoint=wgpu::Device::str(name);auto p=wgpuDeviceCreateComputePipeline(gpu.device,&d);wgpuShaderModuleRelease(m);
 struct E{bool done=false;bool ok=false;} e;WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&e;cb.callback=[](WGPUPopErrorScopeStatus,WGPUErrorType type,WGPUStringView m,void* v,void*){auto& e=*static_cast<E*>(v);e.ok=type==WGPUErrorType_NoError;e.done=true;if(!e.ok)std::cerr.write(m.data,m.length);};wgpuDevicePopErrorScope(gpu.device,cb);while(!e.done)wgpuDevicePoll(gpu.device,true,nullptr);need(e.ok&&p,"compile native pipeline");return p;
}
static WGPUBindGroup group(WGPUComputePipeline p,const std::vector<std::pair<uint32_t,B>>& buffers){std::vector<WGPUBindGroupEntry> entries;for(auto [index,b]:buffers){WGPUBindGroupEntry e={};e.binding=index;e.buffer=b.h;e.size=b.size;entries.push_back(e);}auto layout=wgpuComputePipelineGetBindGroupLayout(p,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=entries.size();d.entries=entries.data();auto g=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return g;}
static std::vector<uint32_t> download(B b){
 auto dst=buffer(b.size,WGPUBufferUsage_MapRead);auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,b.h,0,dst.h,0,b.size);auto c=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&c);wgpuCommandBufferRelease(c);wgpuCommandEncoderRelease(enc);
 struct S{std::atomic<bool> done{false};bool ok=false;} s;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&s;cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void* u,void*){auto& s=*static_cast<S*>(u);s.ok=status==WGPUMapAsyncStatus_Success;s.done.store(true);};wgpuBufferMapAsync(dst.h,WGPUMapMode_Read,0,b.size,cb);while(!s.done.load())wgpuDevicePoll(gpu.device,true,nullptr);need(s.ok,"native readback");auto ptr=static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(dst.h,0,b.size));need(ptr,"mapped readback");std::vector<uint32_t> out(ptr,ptr+b.size/4);wgpuBufferUnmap(dst.h);wgpuBufferRelease(dst.h);return out;
}
struct alignas(16) Ray{std::array<float,4> ro,rd;std::array<uint32_t,4> counts;};
struct alignas(16) Spec{std::array<float,4> origin,half,limits,curv,amps,freqs,phases;};
static_assert(sizeof(Ray)==48&&sizeof(Spec)==112);
int main(){try{
 need(gpu.init(false),"native device acquisition");need(!gpu.timestampQueries,"timestamps disabled");WGPUAdapterInfo info={};need(wgpuAdapterGetInfo(gpu.adapter,&info)==WGPUStatus_Success&&info.backendType==WGPUBackendType_Metal,"actual Metal adapter required");std::string original=routed_wgsl::kClassifyWgsl,patched=original;const std::string site="    let activeCount = countOneBits(activeMemberMask);";auto at=patched.find(site);need(at!=std::string::npos&&patched.find(site,at+1)==std::string::npos,"one external mutation site");patched.insert(at,"    if (activeMemberMask == 0u) { return; }\n");
 auto cp0=pipeline(original,"classifyTilesMain"),cp1=pipeline(patched,"classifyTilesMain"),fb=pipeline(routed_wgsl::kFallbackProducerWgsl,"fallbackProducerMain");J cases=J::array();
 for(uint32_t budget:{0u,95u,96u,1024u})for(bool skip:{false,true}){
  std::array<Ray,16> ray{};for(unsigned i=0;i<16;++i){ray[i].ro={float(i)*.01f,0,50,0};ray[i].rd={.70710677f,0,.70710677f,1};}ray[15].counts[3]=0x40000000u;
  std::array<Spec,4> specs{};for(auto& s:specs){s.half={100,100,100,0};s.limits={100,100,0,.1f};s.curv={.01f,.000001f,0,0};}
  std::array<uint32_t,4> cfg{16,64,16,budget},counts{};std::array<uint32_t,2> offsets{0,16};std::array<uint32_t,16> indices{};for(unsigned i=0;i<16;++i)indices[i]=i;
  auto rays=buffer(sizeof(ray),WGPUBufferUsage_Storage,ray.data()),specsS=buffer(sizeof(specs),WGPUBufferUsage_Storage,specs.data()),specsU=buffer(sizeof(specs),WGPUBufferUsage_Uniform,specs.data()),config=buffer(sizeof(cfg),WGPUBufferUsage_Uniform,cfg.data()),off=buffer(sizeof(offsets),WGPUBufferUsage_Storage,offsets.data()),idx=buffer(sizeof(indices),WGPUBufferUsage_Storage,indices.data()),ct=buffer(sizeof(counts),WGPUBufferUsage_Storage,counts.data()),s1=buffer(4,WGPUBufferUsage_Storage),s2=buffer(4,WGPUBufferUsage_Storage),fl=buffer(4,WGPUBufferUsage_Storage),singles=buffer(16*64*16,WGPUBufferUsage_Storage),overlaps=buffer(16*16*16,WGPUBufferUsage_Storage);
  auto cp=skip?cp1:cp0;auto cg=group(cp,{{0,specsS},{1,rays},{2,off},{3,idx},{4,s1},{5,s2},{6,fl},{7,ct}});auto fg=group(fb,{{0,config},{1,specsU},{2,rays},{3,singles},{4,overlaps},{5,off},{6,idx},{7,fl},{8,ct}});
  auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);for(auto pair:{std::make_pair(cp,cg),std::make_pair(fb,fg)}){auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);wgpuComputePassEncoderSetPipeline(pass,pair.first);wgpuComputePassEncoderSetBindGroup(pass,0,pair.second,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,1,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);}auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
  auto route=download(ct),raw=download(rays);unsigned refused=0,accepted=0,invalid=0;J words=J::array();for(unsigned i=0;i<16;++i){auto w=raw[i*12+11];if(w&0x40000000u)++invalid;else if(w&0x80000000u)++refused;else++accepted;words.push_back(J::array({raw[i*12+8],raw[i*12+9],raw[i*12+10],w}));}
  need(route[0]==0&&route[1]==0&&route[2]==(skip?0u:1u),"actual empty-tile classification");need(invalid==1,"invalid setup marker retained");need(refused==((!skip&&budget<96)?15u:0u),"expected budget refusal difference");need(accepted==((skip||budget>=96)?15u:0u),"expected acceptance counts");cases.push_back({{"budget",budget},{"skip_empty",skip},{"route_counts",route},{"refused",refused},{"accepted",accepted},{"invalid",invalid},{"ray_count_words",words}});
  wgpuBindGroupRelease(cg);wgpuBindGroupRelease(fg);for(auto b:{rays,specsS,specsU,config,off,idx,ct,s1,s2,fl,singles,overlaps})wgpuBufferRelease(b.h);
 }
 std::cout<<J{{"schema","6fc-native-empty-mask-budget-boundary-1"},{"cases",cases},{"scope","Actual classifier and fallback WGSL on native device; one external skip-empty mutation. No consumer/rendered image/accuracy/performance claim."}}.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
