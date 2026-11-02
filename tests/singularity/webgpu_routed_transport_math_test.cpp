// Final checkpoint audit: independent exact-root and physical-box oracles.
// Exercises actual WGSL from all three production producers, not the CPU mirror.
// Codex / GPT-6 / session 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-02.
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/OntoMath/ClampedWaveSupport.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s\n",#x);std::exit(2);}}while(0)
static std::string function(const std::string& code,const char* name){auto s=code.find(std::string("fn ")+name+"(");CHECK(s!=std::string::npos);auto p=code.find('{',s);int d=0;for(auto i=p;i<code.size();++i){if(code[i]=='{')++d;if(code[i]=='}'&&!--d)return code.substr(s,i-s+1);}std::exit(3);}
int main(int argc,char**argv){
 // Independent IEEE cancellation witness: equal real expressions differ in f32.
 volatile float A=33554432.0f,z=1.0f,first=z+A,waves=A-A;
 CHECK(first-A==0.0f && z+waves==1.0f);
 OntoMath::ClampedWaveSupport large{},ordinary{};
 large.amplitudes={A,-A,0,0};large.curvature[1]=.001f;
 ordinary.amplitudes={6,3,1,0};ordinary.curvature[1]=.001f;
 CHECK(!OntoMath::clampedWaveGroupingSlackFits(large,2));
 CHECK(OntoMath::clampedWaveGroupingSlackFits(ordinary,1024));
 CHECK(!OntoMath::clampedWaveGroupingSlackFits(ordinary,std::numeric_limits<float>::quiet_NaN()));
 wgpu::Device gpu;CHECK(gpu.init());
 WGPUAdapterInfo adapter={};CHECK(wgpuAdapterGetInfo(gpu.adapter,&adapter)==WGPUStatus_Success);
#ifdef __APPLE__
 CHECK(adapter.backendType==WGPUBackendType_Metal);
#endif
 wgpuAdapterInfoFreeMembers(adapter);
 WGPUBufferDescriptor bd={};bd.size=16;bd.usage=WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;auto cfg=wgpuDeviceCreateBuffer(gpu.device,&bd);uint32_t conf[4]={16,64,16,1024};wgpuQueueWriteBuffer(gpu.queue,cfg,0,conf,16);
 bd.size=512;bd.usage=WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc;auto result=wgpuDeviceCreateBuffer(gpu.device,&bd);
 bd.usage=WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst;auto readback=wgpuDeviceCreateBuffer(gpu.device,&bd);
 unsigned totalBad=0;
 std::string report="{\"backend\":\"native WebGPU\",\"scope\":\"actual producer isolateMonotone functions; independent exact linear roots\",\"kernels\":[";
 const char* kernels[]={routed_wgsl::kS1UnfusedProducerWgsl,routed_wgsl::kS2ProducerWgsl,routed_wgsl::kFallbackProducerWgsl};
 for(int k=0;k<3;++k){
 std::string code=R"(struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
 struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
 @group(0) @binding(0) var<uniform> cfg:Config;
 @group(0) @binding(1) var<storage,read_write> out:array<vec4f>;
 )";
 for(const char* name:{"evalWave","evalPhi","putBand","putBandTagged","isolateMonotone"})code+=function(kernels[k],name)+"\n";
 code+=R"(@compute @workgroup_size(16) fn main(@builtin(local_invocation_index) id:u32){
 let slopes=array<f32,8>(0.0009765625,0.0078125,0.03125,0.0625,0.125,0.25,0.5,1.0);
 let slope=slopes[id%8u]*select(1.0,-1.0,id>=8u);
 var m:CurtainSpec;m.curv=vec4f(0,.001,0,0);
 var bounds:array<f32,48>;var bands:array<vec2f,24>;
 var nb=1u;var nr=0u;var reason=0u;var work=0u;var zero=0u;
 isolateMonotone(m,vec3f(0,0,-8.0*slope),vec3f(sqrt(max(0.0,1.0-slope*slope)),0,slope),0,16,0,-8.0*slope,8.0*slope,.001,abs(slope),0,&bounds,&nb,&bands,&nr,&reason,&work,&zero,true);
 out[id*2u]=vec4f(bands[0],f32(reason),f32(nr));
 out[id*2u+1u]=vec4f(8,select(0.0,1.0,bands[0].x<=8.0&&bands[0].y>=8.0),f32(work),slope);
 })";
 WGPUShaderSourceWGSL ws={};ws.chain.sType=WGPUSType_ShaderSourceWGSL;ws.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&ws.chain;auto module=wgpuDeviceCreateShaderModule(gpu.device,&md);CHECK(module);
 WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint=wgpu::Device::str("main");auto pipe=wgpuDeviceCreateComputePipeline(gpu.device,&pd);CHECK(pipe);
 auto layout=wgpuComputePipelineGetBindGroupLayout(pipe,0);WGPUBindGroupEntry es[2]={};es[0].binding=0;es[0].buffer=cfg;es[0].size=16;es[1].binding=1;es[1].buffer=result;es[1].size=512;WGPUBindGroupDescriptor gd={};gd.layout=layout;gd.entryCount=2;gd.entries=es;auto group=wgpuDeviceCreateBindGroup(gpu.device,&gd);CHECK(group);
 auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipe);wgpuComputePassEncoderSetBindGroup(pass,0,group,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,1,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);wgpuCommandEncoderCopyBufferToBuffer(enc,result,0,readback,0,512);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);
 struct Map {bool done=false,ok=false;} mapped;
 WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&mapped;cb.callback=[](WGPUMapAsyncStatus s,WGPUStringView,void* p,void*){auto& m=*static_cast<Map*>(p);m.ok=s==WGPUMapAsyncStatus_Success;m.done=true;};wgpuBufferMapAsync(readback,WGPUMapMode_Read,0,512,cb);while(!mapped.done)wgpuInstanceProcessEvents(gpu.instance);CHECK(mapped.ok);
 auto* f=static_cast<const float*>(wgpuBufferGetConstMappedRange(readback,0,512));CHECK(f);unsigned bad=0;for(unsigned i=0;i<16;++i){if(f[i*8+5]!=1||f[i*8+2]!=0)++bad;}totalBad+=bad;
 char row[256];std::snprintf(row,sizeof(row),"%s{\"kernel\":%d,\"cases\":16,\"missed_roots\":%u,\"shallow_first_band\":[%.9g,%.9g]}",k?",":"",k,bad,f[0],f[1]);report+=row;
 wgpuBufferUnmap(readback);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);wgpuBindGroupRelease(group);wgpuBindGroupLayoutRelease(layout);wgpuComputePipelineRelease(pipe);wgpuShaderModuleRelease(module);
 }
 report+="],\"physical_box\":[";
 auto makeBuffer=[&](uint64_t size,WGPUBufferUsage usage,const void* data)->WGPUBuffer{WGPUBufferDescriptor desc={};desc.size=size;desc.usage=usage|WGPUBufferUsage_CopyDst;auto b=wgpuDeviceCreateBuffer(gpu.device,&desc);CHECK(b);if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return b;};
 for(int k=0;k<3;++k){
  float specs[4][28]={};for(int m=0;m<4;++m){specs[m][0]=m?1000:0;specs[m][4]=.01f;specs[m][5]=.01f;specs[m][6]=.01f;specs[m][8]=100;specs[m][9]=10;specs[m][11]=100;specs[m][13]=.001f;}
  float rayData[16][12]={};for(unsigned i=0;i<16;++i){auto& r=rayData[i];unsigned axis=i%3;float sign=(i/3)%2?-1.f:1.f;r[axis]=-sign;r[axis+4]=sign;r[7]=2;}
  uint32_t offsets[2]={0,16},indices[16],counts[4]={1,1,1,0};for(int i=0;i<16;++i)indices[i]=i;
  uint32_t list=k==0?0u:(k==1?0x01000000u:0x10000000u);
  WGPUBuffer bufs[9]={cfg,makeBuffer(sizeof(specs),WGPUBufferUsage_Uniform,specs),makeBuffer(sizeof(rayData),WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,rayData),makeBuffer(16384,WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,nullptr),nullptr,makeBuffer(sizeof(offsets),WGPUBufferUsage_Storage,offsets),makeBuffer(sizeof(indices),WGPUBufferUsage_Storage,indices),makeBuffer(4,WGPUBufferUsage_Storage,&list),makeBuffer(sizeof(counts),WGPUBufferUsage_Storage,counts)};
  if(k==2)bufs[4]=makeBuffer(4096,WGPUBufferUsage_Storage,nullptr);
  WGPUShaderSourceWGSL ws={};ws.chain.sType=WGPUSType_ShaderSourceWGSL;ws.code=wgpu::Device::str(kernels[k]);WGPUShaderModuleDescriptor md={};md.nextInChain=&ws.chain;auto module=wgpuDeviceCreateShaderModule(gpu.device,&md);CHECK(module);
  const char* entries[]={"s1ProducerMain","s2ProducerMain","fallbackProducerMain"};WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint=wgpu::Device::str(entries[k]);auto pipe=wgpuDeviceCreateComputePipeline(gpu.device,&pd);CHECK(pipe);auto layout=wgpuComputePipelineGetBindGroupLayout(pipe,0);
  std::vector<WGPUBindGroupEntry> es;for(unsigned i=0;i<9;++i)if(bufs[i]){WGPUBindGroupEntry e={};e.binding=i;e.buffer=bufs[i];e.size=wgpuBufferGetSize(bufs[i]);es.push_back(e);}
  WGPUBindGroupDescriptor gd={};gd.layout=layout;gd.entryCount=es.size();gd.entries=es.data();auto group=wgpuDeviceCreateBindGroup(gpu.device,&gd);CHECK(group);
  auto rb=makeBuffer(17152,WGPUBufferUsage_MapRead,nullptr);auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipe);wgpuComputePassEncoderSetBindGroup(pass,0,group,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,1,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);wgpuCommandEncoderCopyBufferToBuffer(enc,bufs[2],0,rb,0,768);wgpuCommandEncoderCopyBufferToBuffer(enc,bufs[3],0,rb,768,16384);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);
  struct Map{bool done=false,ok=false;} mapped;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&mapped;cb.callback=[](WGPUMapAsyncStatus s,WGPUStringView,void*p,void*){auto&m=*static_cast<Map*>(p);m.ok=s==WGPUMapAsyncStatus_Success;m.done=true;};wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,17152,cb);while(!mapped.done)wgpuInstanceProcessEvents(gpu.instance);CHECK(mapped.ok);auto*f=static_cast<const float*>(wgpuBufferGetConstMappedRange(rb,0,17152));CHECK(f);
  unsigned outside=0,refused=0,empty=0;float firstLo=0,firstHi=0;
  for(unsigned ray=0;ray<16;++ray){uint32_t num=0,flags=0;std::memcpy(&num,f+ray*12+9,4);std::memcpy(&flags,f+ray*12+11,4);refused+=(flags&0x80000000u)!=0;empty+=num==0;bool bad=false;for(unsigned s=0;s<num&&s<64;++s){auto base=192+(s*16+ray)*4;float a=f[base],b=f[base+1];if(ray==0&&s==0){firstLo=a;firstHi=b;}bad|=a<.99f-2e-6f||b>1.01f+2e-6f;}outside+=bad;}
  totalBad+=outside+refused+empty;char row[256];std::snprintf(row,sizeof(row),"%s{\"kernel\":%d,\"rays\":16,\"outside_physical_box\":%u,\"refused\":%u,\"empty\":%u,\"first_span\":[%.9g,%.9g]}",k?",":"",k,outside,refused,empty,firstLo,firstHi);report+=row;
  wgpuBufferUnmap(rb);wgpuBufferRelease(rb);for(int i=1;i<9;++i)if(bufs[i])wgpuBufferRelease(bufs[i]);wgpuBindGroupRelease(group);wgpuBindGroupLayoutRelease(layout);wgpuComputePipelineRelease(pipe);wgpuShaderModuleRelease(module);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
 }

 report+="]}\n";std::printf("%s",report.c_str());if(argc>1)std::ofstream(argv[1])<<report;
 return totalBad?42:0;
}
