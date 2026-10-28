#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
// Implementation included by WebGpuRenderer.cpp so all existing renderer
// targets share the same backend (no scratch-only alternate executable).
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp"
#include <cstring>
#include <limits>
namespace kernel_routed_detail {
inline bool replace(std::string& s,const std::string& a,const std::string& b){auto p=s.find(a);if(p==std::string::npos||s.find(a,p+a.size())!=std::string::npos)return false;s.replace(p,a.size(),b);return true;}
inline WGPUBuffer buffer(WGPUDevice d,WGPUQueue q,uint64_t bytes,WGPUBufferUsage extra=0,const void* data=nullptr){
 WGPUBufferDescriptor b={};b.label=wgpu::Device::str("Kernel bounded routed transport");b.size=bytes;b.usage=WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc|extra;
 auto out=wgpuDeviceCreateBuffer(d,&b);if(out&&data)wgpuQueueWriteBuffer(q,out,0,data,bytes);return out;
}
inline WGPUBindGroupEntry entry(uint32_t i,WGPUBuffer b,uint64_t size,uint64_t offset=0){WGPUBindGroupEntry e={};e.binding=i;e.buffer=b;e.offset=offset;e.size=size;return e;}
inline WGPUBindGroup group(WGPUDevice d,WGPUBindGroupLayout layout,std::vector<WGPUBindGroupEntry> entries){WGPUBindGroupDescriptor x={};x.layout=layout;x.entryCount=entries.size();x.entries=entries.data();return wgpuDeviceCreateBindGroup(d,&x);}
inline WGPUBindGroup group(WGPUDevice d,WGPUComputePipeline p,std::vector<WGPUBindGroupEntry> entries){auto l=wgpuComputePipelineGetBindGroupLayout(p,0);auto out=group(d,l,std::move(entries));wgpuBindGroupLayoutRelease(l);return out;}
inline WGPUComputePipeline pipeline(WGPUDevice d,const std::string& code,const char* name,WGPUPipelineLayout layout=nullptr){
 wgpuDevicePushErrorScope(d,WGPUErrorFilter_Validation);
 WGPUShaderSourceWGSL s={};s.chain.sType=WGPUSType_ShaderSourceWGSL;s.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&s.chain;
 auto m=wgpuDeviceCreateShaderModule(d,&md);WGPUComputePipelineDescriptor p={};p.layout=layout;p.compute.module=m;p.compute.entryPoint=wgpu::Device::str(name);auto out=wgpuDeviceCreateComputePipeline(d,&p);wgpuShaderModuleRelease(m);
 struct Error {bool done=false;WGPUErrorType type=WGPUErrorType_NoError;} error;
 WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&error;
 cb.callback=[](WGPUPopErrorScopeStatus,WGPUErrorType type,WGPUStringView message,void* user,void*){auto& e=*static_cast<Error*>(user);e.type=type;e.done=true;if(type!=WGPUErrorType_NoError)std::fprintf(stderr,"[routed compiler] %.*s\n",int(message.length),message.data);};
 wgpuDevicePopErrorScope(d,cb);while(!error.done)wgpuDevicePoll(d,true,nullptr);
 if(error.type!=WGPUErrorType_NoError){if(out)wgpuComputePipelineRelease(out);return nullptr;}return out;
}
const char* auxiliary=R"WGSL(
struct ECWindow {control:vec4<u32>,window:vec4<u32>};
struct ECRay {ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32>};
struct ECSpan {span:vec2<f32>,info:vec2<u32>};
@group(0) @binding(3) var<storage,read_write> ecPublication:array<vec4<f32>>;
@group(2) @binding(0) var<uniform> ecWindow:ECWindow;
@group(2) @binding(1) var<storage,read_write> ecRays:array<ECRay>;
@group(2) @binding(2) var<storage,read_write> ecSingles:array<ECSpan>;
@group(2) @binding(3) var<storage,read_write> ecOverlaps:array<ECSpan>;
fn ecPixel(id:u32)->u32 {return (id/ecWindow.window.x+ecWindow.window.z)*ecWindow.window.x+id%ecWindow.window.x;}
fn ecMat(m:mat4x4<f32>,v:vec4<f32>)->vec4<f32>{return fma(m[0],vec4<f32>(v.x),fma(m[1],vec4<f32>(v.y),fma(m[2],vec4<f32>(v.z),m[3]*v.w)));}
)WGSL";
const char* consumer=R"WGSL(
fn ecPhi(tau:f32)->f32 {if(abs(tau)<.01){return 1.0+tau*(-.5+tau*(1.0/6.0-tau/24.0));}return (1.0-exp(-tau))/tau;}
fn ecFlow(state:vec4<f32>,v:vec4<f32>,h:f32)->vec4<f32>{let tau=h*v.w;return vec4<f32>(state.xyz+state.w*v.xyz*(h*ecPhi(tau)),state.w*exp(-tau));}
fn ecProjected(ro:vec3<f32>,rd:vec3<f32>,s:f32,mask:u32)->vec4<f32>{
 let world=ro+s*rd;let delta=ro-world;let len=length(delta);let wo=select(vec3<f32>(0),delta/max(len,1e-8),len>1e-8);
 var result=vec4<f32>(0);
 EC_PROJECTED
 return result;
}
fn ecAdvanceOverlap(stateIn:vec4<f32>,ro:vec3<f32>,rd:vec3<f32>,span:ECSpan)->vec4<f32>{
 var state=stateIn;let h=(span.span.y-span.span.x)*.5;let a=.25-sqrt(3.0)/6.0;let b=.5-a;
 for(var panel=0u;panel<2u;panel++){
  let lo=span.span.x+f32(panel)*h;
  let minus=ecProjected(ro,rd,lo+(.5-sqrt(3.0)/6.0)*h,span.info.x);
  let plus=ecProjected(ro,rd,lo+(.5+sqrt(3.0)/6.0)*h,span.info.x);
  state=ecFlow(state,b*minus+a*plus,h);state=ecFlow(state,a*minus+b*plus,h);
 }
 return state;
}
@compute @workgroup_size(64) fn ecConsume(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x||id/ecWindow.window.x+ecWindow.window.z>=ecWindow.window.y){return;}
 let ray=ecRays[id];if((ray.counts.w&0xc0000000u)!=0u){return;}
 if(ray.counts.y>64u||ray.counts.w>16u){return;}
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;var state=vec4<f32>(0,0,0,1);var single=0u;var overlap=0u;
 let gx0=.5*(1.0-.5773502691896257);let gx1=.5*(1.0+.5773502691896257);
 loop{
  if(single>=ray.counts.y&&overlap>=ray.counts.w){break;}
  var useOverlap=false;
  if(overlap<ray.counts.w){if(single>=ray.counts.y){useOverlap=true;}else{useOverlap=ecOverlaps[ray.counts.z+overlap].span.x<ecSingles[ray.counts.x+single].span.x;}}
  if(useOverlap){state=ecAdvanceOverlap(state,ro,rd,ecOverlaps[ray.counts.z+overlap]);overlap++;continue;}
  let sub=ecSingles[ray.counts.x+single];single++;if(sub.info.x>=4u){return;}
  let h=sub.span.y-sub.span.x;let mask=1u<<sub.info.x;
  let minus=ecProjected(ro,rd,sub.span.x+gx0*h,mask);let plus=ecProjected(ro,rd,sub.span.x+gx1*h,mask);
  state=ecFlow(state,.5*(minus+plus),h);
 }
 if(any((bitcast<vec4<u32>>(state)&vec4<u32>(0x7f800000u))==vec4<u32>(0x7f800000u))||state.w<0||state.w>1){return;}
 ecPublication[ecPixel(id)]=vec4<f32>(state.xyz,1.0-state.w);
}
)WGSL";
}
KernelRoutedTransport::~KernelRoutedTransport(){
 releaseBuffers();for(auto p:common)if(p)wgpuComputePipelineRelease(p);
 for(auto& [key,p]:programs){if(p.setup)wgpuComputePipelineRelease(p.setup);if(p.consumer)wgpuComputePipelineRelease(p.consumer);if(p.globals)wgpuBindGroupLayoutRelease(p.globals);if(p.instances)wgpuBindGroupLayoutRelease(p.instances);if(p.local)wgpuBindGroupLayoutRelease(p.local);}
}
void KernelRoutedTransport::releaseBuffers(){
 for(auto* p:{&publication,&rays,&singles,&overlaps,&offsets,&indices,&counts,&args})if(*p){wgpuBufferRelease(*p);*p=nullptr;}
 for(auto& p:lists)if(p){wgpuBufferRelease(p);p=nullptr;}width=height=capacity=tiles=stripRows=0;
}
bool KernelRoutedTransport::ensure(WGPUDevice d,WGPUQueue q,uint32_t w,uint32_t h){
 using namespace kernel_routed_detail;device=d;queue=q;WGPULimits limits={};
 if(wgpuDeviceGetLimits(d,&limits)!=WGPUStatus_Success||!w||!h||uint64_t(w)*h*16>std::min<uint64_t>(64ull*1024*1024,limits.maxStorageBufferBindingSize))return false;
 if(width==w&&height==h&&publication)return std::all_of(common.begin(),common.end(),[](auto p){return p!=nullptr;});releaseBuffers();
 // 64 intervals/ray retained; partition work, never lower accuracy to fit.
 const auto maxRows=std::min<uint64_t>(48,limits.maxStorageBufferBindingSize/(uint64_t(w)*64*16));
 const uint32_t rows=static_cast<uint32_t>(maxRows/4*4);if(rows<4)return false;
 width=w;height=h;stripRows=rows;capacity=w*rows;tiles=((w+3)/4)*(rows/4);
 std::vector<uint32_t> off,idx;off.reserve(tiles+1);idx.reserve(capacity);
 for(uint32_t y=0;y<rows;y+=4)for(uint32_t x=0;x<w;x+=4){off.push_back(idx.size());for(uint32_t dy=0;dy<4;++dy)for(uint32_t dx=0;dx<4&&x+dx<w;++dx)idx.push_back((y+dy)*w+x+dx);}
 off.push_back(idx.size());publication=buffer(d,q,uint64_t(w)*h*16);rays=buffer(d,q,uint64_t(capacity)*48);singles=buffer(d,q,uint64_t(capacity)*64*16);overlaps=buffer(d,q,uint64_t(capacity)*16*16);
 offsets=buffer(d,q,off.size()*4,0,off.data());indices=buffer(d,q,idx.size()*4,0,idx.data());counts=buffer(d,q,16);args=buffer(d,q,36,WGPUBufferUsage_Indirect);
 for(auto& b:lists)b=buffer(d,q,uint64_t(tiles)*4);
 if(!publication||!rays||!singles||!overlaps||!offsets||!indices||!counts||!args||!lists[0]||!lists[1]||!lists[2]){releaseBuffers();return false;}
 if(!common[0]){
  const char* codes[]={routed_wgsl::kResetCountersWgsl,routed_wgsl::kClassifyWgsl,routed_wgsl::kRouteArgsWgsl,routed_wgsl::kS1UnfusedProducerWgsl,routed_wgsl::kS2ProducerWgsl,routed_wgsl::kFallbackProducerWgsl};
  const char* entries[]={"resetCountersMain","classifyTilesMain","routeArgs","s1ProducerMain","s2ProducerMain","fallbackProducerMain"};
  for(int i=0;i<6;++i){common[i]=pipeline(d,codes[i],entries[i]);if(!common[i])return false;}
 }
 return true;
}
KernelRoutedTransport::Program* KernelRoutedTransport::program(const std::string& source){
 using namespace kernel_routed_detail;auto it=programs.find(source);if(it!=programs.end())return it->second.consumer&&it->second.setup?&it->second:nullptr;
 auto& p=programs[source];const auto fs=source.find("@fragment\nfn fs(");const auto brace=source.find('{',fs);const auto end=source.find("    var transmittance = 1.0;",brace);
 if(fs==std::string::npos||brace==std::string::npos||end==std::string::npos)return nullptr;
 auto prefix=source.substr(0,fs);auto setupBody=source.substr(brace+1,end-brace-1);
 size_t pos=0;while((pos=setupBody.find("in.position",pos))!=std::string::npos){setupBody.replace(pos,11,"position");pos+=8;}
 // Candidate setup arithmetic is isolated from the unchanged fragment fallback.
 replace(setupBody,"u.invViewProj * farNdc","ecMat(u.invViewProj,farNdc)");
 pos=0;while((pos=setupBody.find("discard;",pos))!=std::string::npos){setupBody.replace(pos,8,"ecPublication[ecPixel(id)]=vec4<f32>(0);return;");pos+=60;}
 auto setupPrefix=prefix;replace(setupPrefix,"u.invViewProj * ndc","ecMat(u.invViewProj,ndc)");
 const std::string commonText=auxiliary;
 std::string setup=setupPrefix+commonText+R"WGSL(
 @compute @workgroup_size(64) fn ecSetup(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x){return;}
 ecRays[id]=ECRay(vec4<f32>(0),vec4<f32>(0),vec4<u32>(0,0,0,0x40000000u));
 let x=id%ecWindow.window.x;let y=id/ecWindow.window.x+ecWindow.window.z;if(y>=ecWindow.window.y){return;}
 ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);
 if(any(abs(u.eyePos.xyz)>vec3<f32>(2048))){return;}
 let position=vec4<f32>(f32(x)+.5,f32(y)+.5,0,1);
)WGSL"+setupBody+R"WGSL(
 if(t1>2048.0){return;}
 ecRays[id]=ECRay(vec4<f32>(ro,t0),vec4<f32>(rd,t1),vec4<u32>(0));
 }
)WGSL";
 std::string sampling;
 for(unsigned i=0;i<4;++i){const auto n=std::to_string(i);sampling+="if((mask&"+std::to_string(1u<<i)+"u)!=0u){let inst=instances["+std::to_string(i+1)+"u];if(all(world>=inst.origin.xyz-inst.halfExtent.xyz)&&all(world<=inst.origin.xyz+inst.halfExtent.xyz)){g_instIdx="+std::to_string(i+1)+"u;let p=world-inst.origin.xyz;let D=volumeDensityEval_"+n+"(p);if(D>0.0){let X=max(volumeExtinctionEval_"+n+"(p,D),1e-6);var E=vec3<f32>(0);if(!VOLUME_EMISSION_READS_OMEGA_"+n+"||len>1e-8){E=max(volumeEmissionEval_"+n+"(p,wo),vec3<f32>(0));}result+=vec4<f32>(E,X);}}}\n";}
 std::string tail=consumer;replace(tail,"EC_PROJECTED",sampling);std::string consume=prefix+commonText+tail;
 WGPUBindGroupLayoutEntry g[4]={};g[0].binding=0;g[0].visibility=WGPUShaderStage_Compute;g[0].buffer.type=WGPUBufferBindingType_Uniform;
 g[1].binding=1;g[1].visibility=WGPUShaderStage_Compute;g[1].buffer.type=WGPUBufferBindingType_ReadOnlyStorage;
 g[2].binding=2;g[2].visibility=WGPUShaderStage_Compute;g[2].texture.sampleType=WGPUTextureSampleType_Depth;g[2].texture.viewDimension=WGPUTextureViewDimension_2D;
 g[3].binding=3;g[3].visibility=WGPUShaderStage_Compute;g[3].buffer.type=WGPUBufferBindingType_Storage;
 WGPUBindGroupLayoutDescriptor gd={};gd.entryCount=4;gd.entries=g;p.globals=wgpuDeviceCreateBindGroupLayout(device,&gd);
 WGPUBindGroupLayoutEntry inst={};inst.binding=0;inst.visibility=WGPUShaderStage_Compute;inst.buffer.type=WGPUBufferBindingType_ReadOnlyStorage;gd.entryCount=1;gd.entries=&inst;p.instances=wgpuDeviceCreateBindGroupLayout(device,&gd);
 WGPUBindGroupLayoutEntry local[4]={};for(int i=0;i<4;++i){local[i].binding=i;local[i].visibility=WGPUShaderStage_Compute;local[i].buffer.type=i==0?WGPUBufferBindingType_Uniform:WGPUBufferBindingType_Storage;}gd.entryCount=4;gd.entries=local;p.local=wgpuDeviceCreateBindGroupLayout(device,&gd);
 WGPUBindGroupLayout layouts[]={p.globals,p.instances,p.local};WGPUPipelineLayoutDescriptor ld={};ld.bindGroupLayoutCount=3;ld.bindGroupLayouts=layouts;auto layout=wgpuDeviceCreatePipelineLayout(device,&ld);
 p.setup=pipeline(device,setup,"ecSetup",layout);p.consumer=pipeline(device,consume,"ecConsume",layout);wgpuPipelineLayoutRelease(layout);
 if(!p.setup||!p.consumer)return nullptr;std::fprintf(stderr,"[WebGpuRenderer] routed transport production-evaluator pipeline active\n");return &p;
}
bool KernelRoutedTransport::encode(Program& p,WGPUCommandEncoder encoder,const KernelRoutedRange& globals,const KernelRoutedRange& params,const KernelRoutedRange& instances,WGPUTextureView depth,const KernelRoutedRange& specs,const std::function<KernelRoutedRange(const void*,size_t)>& uniform){
 using namespace kernel_routed_detail;
 auto g=std::vector<WGPUBindGroupEntry>{entry(0,globals.buffer,globals.size,globals.offset),entry(1,params.buffer,params.size,params.offset),{},entry(3,publication,uint64_t(width)*height*16)};g[2].binding=2;g[2].textureView=depth;
 auto globalGroup=group(device,p.globals,g),instanceGroup=group(device,p.instances,{entry(0,instances.buffer,instances.size,instances.offset)});
 auto resetGroup=group(device,common[0],{entry(0,counts,16)});
 auto classifyGroup=group(device,common[1],{entry(0,specs.buffer,specs.size,specs.offset),entry(1,rays,uint64_t(capacity)*48),entry(2,offsets,uint64_t(tiles+1)*4),entry(3,indices,uint64_t(capacity)*4),entry(4,lists[0],uint64_t(tiles)*4),entry(5,lists[1],uint64_t(tiles)*4),entry(6,lists[2],uint64_t(tiles)*4),entry(7,counts,16)});
 auto argsGroup=group(device,common[2],{entry(0,counts,16),entry(1,args,36)});
 for(uint32_t start=0;start<height;start+=stripRows){
  std::array<uint32_t,8> config{capacity,64,16,1024,width,height,start,stripRows};const auto u=uniform(config.data(),sizeof(config));
  auto localGroup=group(device,p.local,{entry(0,u.buffer,u.size,u.offset),entry(1,rays,uint64_t(capacity)*48),entry(2,singles,uint64_t(capacity)*64*16),entry(3,overlaps,uint64_t(capacity)*16*16)});
  auto dispatch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,uint32_t n){auto pass=wgpuCommandEncoderBeginComputePass(encoder,nullptr);wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,n,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);};
  auto withContext=[&](WGPUComputePipeline cp){auto pass=wgpuCommandEncoderBeginComputePass(encoder,nullptr);wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,globalGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,instanceGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,2,localGroup,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,(capacity+63)/64,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);};
  withContext(p.setup);dispatch(common[0],resetGroup,1);dispatch(common[1],classifyGroup,(tiles+63)/64);dispatch(common[2],argsGroup,3);
  for(unsigned i=0;i<3;++i){auto entries=std::vector<WGPUBindGroupEntry>{entry(0,u.buffer,u.size,u.offset),entry(1,specs.buffer,specs.size,specs.offset),entry(2,rays,uint64_t(capacity)*48),entry(3,singles,uint64_t(capacity)*64*16)};if(i==2)entries.push_back(entry(4,overlaps,uint64_t(capacity)*16*16));entries.push_back(entry(5,offsets,uint64_t(tiles+1)*4));entries.push_back(entry(6,indices,uint64_t(capacity)*4));entries.push_back(entry(7,lists[i],uint64_t(tiles)*4));entries.push_back(entry(8,counts,16));
   auto bg=group(device,common[i+3],entries);auto pass=wgpuCommandEncoderBeginComputePass(encoder,nullptr);wgpuComputePassEncoderSetPipeline(pass,common[i+3]);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,args,i*12);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);wgpuBindGroupRelease(bg);
  }
  withContext(p.consumer);wgpuBindGroupRelease(localGroup);
 }
 for(auto g:{globalGroup,instanceGroup,resetGroup,classifyGroup,argsGroup})wgpuBindGroupRelease(g);return true;
}
