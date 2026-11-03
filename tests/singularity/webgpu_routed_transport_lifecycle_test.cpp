// Codex / GPT-6 / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-02.
// Actual resource implementation under renamed symbols, with test-only API
// fault injection. Production has no fault switches or instrumentation branch.
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>
static unsigned checks=0,failures=0,pipelines=0,buffers=0,groups=0,layouts=0,begins=0,blocking=0;
static unsigned failPipeline=0,invalidBuffer=0,invalidGroup=0,invalidLayout=0;static bool nullLayout=false,nullPipelineLayout=false,nullGroup=false;static bool forceOOM=false;
static std::map<void*,int> owned;
#define CHECK(c) do{++checks;if(!(c)){++failures;std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);}}while(0)
template<class T>static T acquired(T x){if(x)++owned[reinterpret_cast<void*>(x)];return x;}
template<class T>static void dropped(T x){if(x){auto i=owned.find(reinterpret_cast<void*>(x));CHECK(i!=owned.end()&&i->second>0);if(i!=owned.end()&&--i->second==0)owned.erase(i);}}
static WGPUComputePipeline createPipeline(WGPUDevice d,const WGPUComputePipelineDescriptor*p){if(++pipelines==failPipeline)return nullptr;return acquired(wgpuDeviceCreateComputePipeline(d,p));}
static WGPUBuffer createBuffer(WGPUDevice d,const WGPUBufferDescriptor*p){auto q=*p;if(++buffers==invalidBuffer)q.usage=0;return acquired(wgpuDeviceCreateBuffer(d,&q));}
static WGPUBindGroup createGroup(WGPUDevice d,const WGPUBindGroupDescriptor*p){if(nullGroup){nullGroup=false;return nullptr;}auto q=*p;if(++groups==invalidGroup)q.entryCount=0;return acquired(wgpuDeviceCreateBindGroup(d,&q));}
static WGPUBindGroupLayout createLayout(WGPUDevice d,const WGPUBindGroupLayoutDescriptor*p){if(nullLayout){nullLayout=false;return nullptr;}auto q=*p;std::vector<WGPUBindGroupLayoutEntry> es;if(++layouts==invalidLayout){es.assign(q.entries,q.entries+q.entryCount);if(es.size()>1)es[1].binding=es[0].binding;q.entries=es.data();}return acquired(wgpuDeviceCreateBindGroupLayout(d,&q));}
static WGPUPipelineLayout createPipelineLayout(WGPUDevice d,const WGPUPipelineLayoutDescriptor*p){if(nullPipelineLayout){nullPipelineLayout=false;return nullptr;}return acquired(wgpuDeviceCreatePipelineLayout(d,p));}
static WGPUBindGroupLayout getLayout(WGPUComputePipeline p,uint32_t i){return acquired(wgpuComputePipelineGetBindGroupLayout(p,i));}
static void releasePipeline(WGPUComputePipeline p){dropped(p);wgpuComputePipelineRelease(p);}static void releaseBuffer(WGPUBuffer p){dropped(p);wgpuBufferRelease(p);}static void releaseGroup(WGPUBindGroup p){dropped(p);wgpuBindGroupRelease(p);}static void releaseLayout(WGPUBindGroupLayout p){dropped(p);wgpuBindGroupLayoutRelease(p);}static void releasePipelineLayout(WGPUPipelineLayout p){dropped(p);wgpuPipelineLayoutRelease(p);}
static WGPUBool poll(WGPUDevice d,WGPUBool wait,const WGPUSubmissionIndex*s){blocking+=wait!=0;return wgpuDevicePoll(d,wait,s);}
static WGPUComputePassEncoder auditedBegin(WGPUCommandEncoder e,const WGPUComputePassDescriptor*d){++begins;return wgpuCommandEncoderBeginComputePass(e,d);}
static WGPUFuture pop(WGPUDevice d,WGPUPopErrorScopeCallbackInfo cb){
 if(!forceOOM)return wgpuDevicePopErrorScope(d,cb);forceOOM=false;
 struct Pending{WGPUPopErrorScopeCallbackInfo original;};auto*p=new Pending{cb};cb.userdata1=p;cb.userdata2=nullptr;
 cb.callback=[](WGPUPopErrorScopeStatus s,WGPUErrorType,WGPUStringView,void*u,void*){auto*p=static_cast<Pending*>(u);auto c=p->original;delete p;c.callback(s,WGPUErrorType_OutOfMemory,wgpu::Device::str("injected OOM scope result; no large allocation"),c.userdata1,c.userdata2);};return wgpuDevicePopErrorScope(d,cb);
}
#define KernelRoutedTransport AuditedRoutedTransport
#define kernel_routed_detail audited_routed_detail
#define wgpuDeviceCreateComputePipeline createPipeline
#define wgpuDeviceCreateBuffer createBuffer
#define wgpuDeviceCreateBindGroup createGroup
#define wgpuDeviceCreateBindGroupLayout createLayout
#define wgpuDeviceCreatePipelineLayout createPipelineLayout
#define wgpuComputePipelineGetBindGroupLayout getLayout
#define wgpuComputePipelineRelease releasePipeline
#define wgpuBufferRelease releaseBuffer
#define wgpuBindGroupRelease releaseGroup
#define wgpuBindGroupLayoutRelease releaseLayout
#define wgpuPipelineLayoutRelease releasePipelineLayout
#define wgpuDevicePoll poll
#define wgpuCommandEncoderBeginComputePass auditedBegin
#define wgpuDevicePopErrorScope pop
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.inc"
#undef KernelRoutedTransport
#undef kernel_routed_detail
#undef wgpuDeviceCreateComputePipeline
#undef wgpuDeviceCreateBuffer
#undef wgpuDeviceCreateBindGroup
#undef wgpuDeviceCreateBindGroupLayout
#undef wgpuDeviceCreatePipelineLayout
#undef wgpuComputePipelineGetBindGroupLayout
#undef wgpuComputePipelineRelease
#undef wgpuBufferRelease
#undef wgpuBindGroupRelease
#undef wgpuBindGroupLayoutRelease
#undef wgpuPipelineLayoutRelease
#undef wgpuDevicePoll
#undef wgpuCommandEncoderBeginComputePass
#undef wgpuDevicePopErrorScope
static std::string source(){
 std::string s=R"(struct G {eyePos:vec4f,viewport:vec4f,invViewProj:mat4x4f};
 @group(0) @binding(0) var<uniform> u:G;
 @group(0) @binding(1) var<storage,read> P:array<f32>;
 @group(0) @binding(2) var sceneDepthTex:texture_depth_2d;
 struct Inst {origin:vec4f};@group(1) @binding(0) var<storage,read> instances:array<Inst>;
 var<private> g_instIdx:u32;
 struct Out {@builtin(position) position:vec4f};
 )";
 for(int i=0;i<4;++i){auto n=std::to_string(i);s+="const VOLUME_EMISSION_READS_OMEGA_"+n+"=false;\nfn volumeDensityEval_"+n+"(p:vec3f)->f32{return .5;}\nfn volumeExtinctionEval_"+n+"(p:vec3f,D:f32)->f32{return D;}\nfn volumeEmissionEval_"+n+"(p:vec3f,wo:vec3f)->vec3f{return vec3f(.1);}\n";}
 return s+R"(@fragment
fn fs(in:Out)->@location(0)vec4f{
    let ro=u.eyePos.xyz;
    let rd=vec3f(0,0,1);
    var t0=0.0;var t1=1.0;
    var transmittance = 1.0;
    return vec4f(0,0,0,transmittance);
})";
}
static void outerPop(wgpu::Device& g){struct R{bool done=false;}r;WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&r;cb.callback=[](WGPUPopErrorScopeStatus,WGPUErrorType,WGPUStringView,void*p,void*){static_cast<R*>(p)->done=true;};wgpuDevicePopErrorScope(g.device,cb);while(!r.done)wgpuDevicePoll(g.device,false,nullptr);}
int main(int argc,char**argv){
 wgpu::Device g;CHECK(g.init());if(!g.device)return 1;std::string which=argc>1?argv[1]:"all";
 auto run=[&](const char*name,auto body){if(which!="all"&&which!=name)return;size_t before=owned.size();body();CHECK(owned.size()==before);std::printf("CASE %s: %u failures\n",name,failures);};
 run("partial",[&]{AuditedRoutedTransport k;failPipeline=pipelines+3;CHECK(!k.ensure(g.device,g.queue,64,48));bool clean=!k.publication&&std::all_of(k.common.begin(),k.common.end(),[](auto p){return !p;});CHECK(clean);failPipeline=0;if(!clean)return;CHECK(k.ensure(g.device,g.queue,64,48));CHECK(k.ensure(g.device,g.queue,80,52));CHECK(k.ensure(g.device,g.queue,64,48));});
 run("partial-all",[&]{for(unsigned slot=1;slot<=7;++slot){AuditedRoutedTransport k;failPipeline=pipelines+slot;CHECK(!k.ensure(g.device,g.queue,64,48));CHECK(!k.publication&&std::all_of(k.common.begin(),k.common.end(),[](auto p){return !p;})&&!k.classifySkip);failPipeline=0;CHECK(k.ensure(g.device,g.queue,64,48));CHECK(k.ensure(g.device,g.queue,80,52));}});
 run("buffers",[&]{AuditedRoutedTransport k;wgpuDevicePushErrorScope(g.device,WGPUErrorFilter_Validation);invalidBuffer=buffers+1;bool accepted=k.ensure(g.device,g.queue,64,48);outerPop(g);CHECK(!accepted&&!k.publication);invalidBuffer=0;CHECK(k.ensure(g.device,g.queue,64,48));});
 run("groups",[&]{AuditedRoutedTransport k;wgpuDevicePushErrorScope(g.device,WGPUErrorFilter_Validation);invalidGroup=groups+1;bool accepted=k.ensure(g.device,g.queue,64,48);outerPop(g);CHECK(!accepted&&!k.publication);invalidGroup=0;CHECK(k.ensure(g.device,g.queue,64,48));});
 run("layouts",[&]{AuditedRoutedTransport k;CHECK(k.ensure(g.device,g.queue,64,48));wgpuDevicePushErrorScope(g.device,WGPUErrorFilter_Validation);invalidLayout=layouts+1;CHECK(k.program(source())==nullptr);outerPop(g);invalidLayout=0;CHECK(k.programs.empty());CHECK(k.program(source())!=nullptr);});
 run("null-layouts",[&]{AuditedRoutedTransport k;CHECK(k.ensure(g.device,g.queue,64,48));nullLayout=true;CHECK(!k.program(source()));CHECK(k.programs.empty());nullPipelineLayout=true;CHECK(!k.program(source()));CHECK(k.programs.empty());CHECK(k.program(source()));});
 run("cache",[&]{AuditedRoutedTransport k;CHECK(k.ensure(g.device,g.queue,64,48));auto*p=k.program(source());CHECK(p);for(int i=1;i<16;++i)CHECK(k.program(source()+"\n// variant "+std::to_string(i)));CHECK(k.program(source()+"\n// variant 17")==nullptr);CHECK(k.programs.size()==16);CHECK(p==k.program(source())&&p->consumer&&p->setup);CHECK(k.ensure(g.device,g.queue,80,52));CHECK(p==k.program(source()));});
 run("dedup",[&]{std::vector<uint32_t> ids;auto s=source();CHECK(audited_routed_detail::checkEvaluatorsDeduplicatable(s,ids));s+="\nvar<private> g_volumeShared_1_G7:f32;\nfn volumeShared_1_Eval(p:vec3f){g_volumeShared_1_G7=p.x;}\nfn volumeShared_1_G7(p:vec3f)->f32{return p.x;}\n";CHECK(!audited_routed_detail::checkEvaluatorsDeduplicatable(s,ids));s=source();for(int i=0;i<4;++i)s+="\nvar<private> g_volumeShared_"+std::to_string(i)+"_G0:f32;\nfn volumeShared_"+std::to_string(i)+"_G0(p:vec3f)->f32{return p.x;}\n";CHECK(audited_routed_detail::checkEvaluatorsDeduplicatable(s,ids));s+="\nvar<private> g_volumeShared_2_G9:f32;\nfn volumeShared_2_G9(p:vec3f)->f32{return p.y;}\n";CHECK(!audited_routed_detail::checkEvaluatorsDeduplicatable(s,ids));});
 run("observer",[&]{AuditedRoutedTransport k;CHECK(k.ensure(g.device,g.queue,64,48));invalidBuffer=buffers+1;CHECK(!k.enableRouteObserver(true));CHECK(!k.observeRoutes&&!k.observerPipe&&!k.accumPipe&&!k.routeHistogram);invalidBuffer=0;CHECK(k.enableRouteObserver(true));CHECK(k.observeRoutes);CHECK(k.enableRouteObserver(false));});
 run("encode",[&]{AuditedRoutedTransport k;CHECK(k.ensure(g.device,g.queue,64,48));auto*p=k.program(source());CHECK(p);if(!p)return;
  auto mk=[&](size_t bytes,WGPUBufferUsage usage){WGPUBufferDescriptor d={};d.size=bytes;d.usage=usage|WGPUBufferUsage_CopyDst;return wgpuDeviceCreateBuffer(g.device,&d);};auto globals=mk(256,WGPUBufferUsage_Uniform),params=mk(16,WGPUBufferUsage_Storage),inst=mk(80,WGPUBufferUsage_Storage);WGPUTextureDescriptor td={};td.size={64,48,1};td.dimension=WGPUTextureDimension_2D;td.format=WGPUTextureFormat_Depth32Float;td.mipLevelCount=td.sampleCount=1;td.usage=WGPUTextureUsage_TextureBinding|WGPUTextureUsage_RenderAttachment;auto tex=wgpuDeviceCreateTexture(g.device,&td);auto view=wgpuTextureCreateView(tex,nullptr);
  for(int mode=0;mode<4;++mode){auto enc=wgpuDeviceCreateCommandEncoder(g.device,nullptr);wgpuDevicePushErrorScope(g.device,WGPUErrorFilter_Validation);begins=0;
   if(mode==0)invalidGroup=groups+1;else if(mode==1)forceOOM=true;else if(mode==2)nullGroup=true;
#ifdef AUDIT_OLD_API
   bool ok=k.encode(*p,enc,{globals,0,256},{params,0,16},{inst,0,80},view,{},{},[](const void*,size_t){return KernelRoutedRange{};});
#else
   bool ok=k.encode(*p,enc,{globals,0,256},{params,0,16},{inst,0,80},view);
#endif
   outerPop(g);invalidGroup=0;forceOOM=false;
   if(mode<3){CHECK(!ok&&begins==0&&k.encodedDispatches==0);CHECK(p->localGroups.empty());}else CHECK(ok&&begins>0&&k.encodedDispatches>0);
   wgpuCommandEncoderRelease(enc);
  }
  wgpuTextureViewRelease(view);wgpuTextureRelease(tex);for(auto b:{globals,params,inst})wgpuBufferRelease(b);
 });
 CHECK(blocking==0);CHECK(owned.empty());std::printf("resource lifecycle: %u checks, %u failures, blocking polls=%u\n",checks,failures,blocking);return failures?1:0;
}
