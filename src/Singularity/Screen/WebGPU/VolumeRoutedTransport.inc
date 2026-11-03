#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
// Implementation included by WebGpuRenderer.cpp so all existing renderer
// targets share the same backend (no scratch-only alternate executable).
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp"
#include <cstring>
#include <limits>
#include <cctype>
#include <algorithm>
namespace kernel_routed_detail {
inline bool replace(std::string& s,const std::string& a,const std::string& b){auto p=s.find(a);if(p==std::string::npos||s.find(a,p+a.size())!=std::string::npos)return false;s.replace(p,a.size(),b);return true;}
inline std::string extractFunctionBody(const std::string& src, const std::string& name) {
    const std::string tag = "fn " + name + "(";
    auto p = src.find(tag);
    if (p == std::string::npos) return "";
    auto brace = src.find('{', p);
    if (brace == std::string::npos) return "";
    int depth = 0;
    for (size_t i = brace; i < src.size(); ++i) {
        if (src[i] == '{') ++depth;
        else if (src[i] == '}') {
            --depth;
            if (depth == 0) return src.substr(brace, i - brace + 1);
        }
    }
    return "";
}
inline bool checkEvaluatorsDeduplicatable(const std::string& source, std::vector<uint32_t>& sharedGroups) {
    for (int i = 0; i < 4; ++i) {
        const auto n = std::to_string(i);
        if (source.find("fn volumeDensityEval_" + n + "(") == std::string::npos) return false;
        if (source.find("fn volumeExtinctionEval_" + n + "(") == std::string::npos) return false;
    }
    auto norm = [](std::string s, int idx) {
        if (idx == 0) return s;
        const std::string idxStr = std::to_string(idx);
        const std::string sFrom = "volumeShared_" + idxStr + "_";
        size_t p = 0;
        while ((p = s.find(sFrom, p)) != std::string::npos) {
            s.replace(p, sFrom.size(), "volumeShared_0_");
            p += 15;
        }
        const std::string gFrom = "g_volumeShared_" + idxStr + "_";
        p = 0;
        while ((p = s.find(gFrom, p)) != std::string::npos) {
            s.replace(p, gFrom.size(), "g_volumeShared_0_");
            p += 17;
        }
        const std::string dFrom = "volumeDensityEval_" + idxStr;
        p = 0;
        while ((p = s.find(dFrom, p)) != std::string::npos) {
            s.replace(p, dFrom.size(), "volumeDensityEval_0");
            p += 19;
        }
        const std::string eFrom = "volumeExtinctionEval_" + idxStr;
        p = 0;
        while ((p = s.find(eFrom, p)) != std::string::npos) {
            s.replace(p, eFrom.size(), "volumeExtinctionEval_0");
            p += 22;
        }
        return s;
    };
    const std::string baseDensity = extractFunctionBody(source, "volumeDensityEval_0");
    const std::string baseExtinction = extractFunctionBody(source, "volumeExtinctionEval_0");
    const std::string baseSharedEval = extractFunctionBody(source, "volumeShared_0_Eval");
    if (baseDensity.empty() || baseExtinction.empty()) return false;
    for (int i = 1; i < 4; ++i) {
        const auto n = std::to_string(i);
        if (norm(extractFunctionBody(source, "volumeDensityEval_" + n), i) != baseDensity) return false;
        if (norm(extractFunctionBody(source, "volumeExtinctionEval_" + n), i) != baseExtinction) return false;
        if (norm(extractFunctionBody(source, "volumeShared_" + n + "_Eval"), i) != baseSharedEval) return false;
    }
    auto groups = [&](int member) {
        std::vector<uint32_t> ids;
        const std::string prefix = "g_volumeShared_" + std::to_string(member) + "_G";
        size_t pos = 0;
        while ((pos = source.find(prefix, pos)) != std::string::npos) {
            pos += prefix.size(); size_t end = pos;
            while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) ++end;
            if (end > pos) {
                try { const auto id=std::stoul(source.substr(pos, end - pos));
                    if(id>=UINT32_MAX)return std::vector<uint32_t>{UINT32_MAX};
                    ids.push_back(static_cast<uint32_t>(id)); }
                catch (const std::exception&) { return std::vector<uint32_t>{UINT32_MAX}; }
            }
            pos = end;
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        return ids;
    };
    sharedGroups = groups(0);
    if (std::find(sharedGroups.begin(), sharedGroups.end(), UINT32_MAX) != sharedGroups.end()) return false;
    for (int i = 1; i < 4; ++i) if (groups(i) != sharedGroups) return false;
    for (uint32_t gid : sharedGroups) {
        const std::string baseG = extractFunctionBody(source, "volumeShared_0_G" + std::to_string(gid));
        if (baseG.empty()) return false;
        for (int i = 1; i < 4; ++i) {
            if (norm(extractFunctionBody(source, "volumeShared_" + std::to_string(i) + "_G" + std::to_string(gid)), i) != baseG) {
                return false;
            }
        }
    }
    return true;
}
// Kernel resource transaction: consume validation/error objects before dispatch.
// Polling is non-blocking; validation completion must not fence submitted GPU work.
struct CreationScope {
 WGPUDevice device; bool closed=false,ok=true;
 explicit CreationScope(WGPUDevice d):device(d){wgpuDevicePushErrorScope(d,WGPUErrorFilter_OutOfMemory);wgpuDevicePushErrorScope(d,WGPUErrorFilter_Validation);}
 bool finish(){
  if(closed)return ok;closed=true;
  for(int i=0;i<2;++i){
   struct Result{bool done=false,ok=false;} result;
   WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.userdata1=&result;
   cb.callback=[](WGPUPopErrorScopeStatus status,WGPUErrorType type,WGPUStringView message,void* ptr,void*){
    auto& r=*static_cast<Result*>(ptr);r.ok=status==WGPUPopErrorScopeStatus_Success&&type==WGPUErrorType_NoError;r.done=true;
    if(!r.ok)std::fprintf(stderr,"[routed resource] %.*s\n",int(std::min<size_t>(message.length,1024)),message.data);
   };
   wgpuDevicePopErrorScope(device,cb);while(!result.done)wgpuDevicePoll(device,false,nullptr);
   ok=ok&&result.ok;
  }
  return ok;
 }
 ~CreationScope(){finish();}
};
inline void releaseProgram(KernelRoutedTransport::Program& p){
 for(auto bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);
 if(p.setup)wgpuComputePipelineRelease(p.setup);if(p.consumer)wgpuComputePipelineRelease(p.consumer);
 if(p.globals)wgpuBindGroupLayoutRelease(p.globals);if(p.instances)wgpuBindGroupLayoutRelease(p.instances);if(p.local)wgpuBindGroupLayoutRelease(p.local);
 p={};
}
inline void releaseCommon(KernelRoutedTransport& k){
 for(auto& p:k.common)if(p){wgpuComputePipelineRelease(p);p=nullptr;}
 if(k.classifySkip){wgpuComputePipelineRelease(k.classifySkip);k.classifySkip=nullptr;}
}
inline WGPUBuffer buffer(WGPUDevice d,WGPUQueue q,uint64_t bytes,WGPUBufferUsage extra=0,const void* data=nullptr){
 WGPUBufferDescriptor b={};b.label=wgpu::Device::str("Kernel bounded routed transport");b.size=bytes;b.usage=WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc|extra;
 auto out=wgpuDeviceCreateBuffer(d,&b);if(out&&data)wgpuQueueWriteBuffer(q,out,0,data,bytes);return out;
}
inline WGPUBindGroupEntry entry(uint32_t i,WGPUBuffer b,uint64_t size,uint64_t offset=0){WGPUBindGroupEntry e={};e.binding=i;e.buffer=b;e.offset=offset;e.size=size;return e;}
inline WGPUBindGroup group(WGPUDevice d,WGPUBindGroupLayout layout,std::vector<WGPUBindGroupEntry> entries){if(!layout)return nullptr;WGPUBindGroupDescriptor x={};x.layout=layout;x.entryCount=entries.size();x.entries=entries.data();return wgpuDeviceCreateBindGroup(d,&x);}
inline WGPUBindGroup group(WGPUDevice d,WGPUComputePipeline p,std::vector<WGPUBindGroupEntry> entries){if(!p)return nullptr;auto l=wgpuComputePipelineGetBindGroupLayout(p,0);if(!l)return nullptr;auto out=group(d,l,std::move(entries));wgpuBindGroupLayoutRelease(l);return out;}
inline WGPUComputePipeline pipeline(WGPUDevice d,const std::string& code,const char* name,WGPUPipelineLayout layout=nullptr){
 CreationScope scope(d);
 WGPUShaderSourceWGSL s={};s.chain.sType=WGPUSType_ShaderSourceWGSL;s.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&s.chain;
 auto m=wgpuDeviceCreateShaderModule(d,&md);if(!m){scope.finish();return nullptr;}WGPUComputePipelineDescriptor p={};p.layout=layout;p.compute.module=m;p.compute.entryPoint=wgpu::Device::str(name);auto out=wgpuDeviceCreateComputePipeline(d,&p);if(m)wgpuShaderModuleRelease(m);
 if(!scope.finish()||!m||!out){if(out)wgpuComputePipelineRelease(out);return nullptr;}return out;
}
const char* auxiliary=R"WGSL(
struct ECWindow {control:vec4<u32>,window:vec4<u32>,flags:vec4<u32>};
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
fn ecFlow(state:vec4<f32>,v:vec4<f32>,h:f32)->vec4<f32>{
 let tau=h*v.w;let e=exp(-tau);
 let phi=select((1.0-e)/tau,1.0+tau*(-.5+tau*(1.0/6.0-tau/24.0)),abs(tau)<.01);
 return vec4<f32>(state.xyz+state.w*v.xyz*(h*phi),state.w*e);
}
EC_CONSUMER_HELPERS
fn ecProjected(ro:vec3<f32>,rd:vec3<f32>,wo:vec3<f32>,s:f32,mask:u32)->vec4<f32>{
 let world=ro+s*rd;
 var result=vec4<f32>(0);
 EC_PROJECTED
 return result;
}
@compute @workgroup_size(64) fn ecConsume(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x||id/ecWindow.window.x+ecWindow.window.z>=ecWindow.window.y){return;}
 let ray=ecRays[id];
 if((ray.counts.w&0x40000000u)!=0u){return;}
 if((ray.counts.w&0x80000000u)!=0u||ray.counts.y>64u||ray.counts.w>16u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
 if((ray.counts.y|ray.counts.w)==0u){return;}
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;let wo=-rd;
 let stride=ecWindow.control.x;
 var state=vec4<f32>(0,0,0,1);
 let gx0=.5*(1.0-.5773502691896257);let gx1=.5*(1.0+.5773502691896257);
 let oa=.25-sqrt(3.0)/6.0;let ob=.5-oa;
 if(ray.counts.w==0u){
  for(var s=0u;s<ray.counts.y;s++){
   let sub=ecSingles[s*stride+id];
   if(sub.info.x>=4u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
   let lo=sub.span.x;let h=sub.span.y-lo;let subMember=sub.info.x;let mask=1u<<subMember;
   EC_EVAL_SINGLE_PAIR
   let v=.5*(minus+plus);
   if(v.w>0.0){state=ecFlow(state,v,h);}
  }
 }else{
  var single=0u;var overlap=0u;var overPanel=0u;
  loop{
   if(single>=ray.counts.y&&overlap>=ray.counts.w){break;}
   var useOverlap=false;
   if(overlap<ray.counts.w){if(single>=ray.counts.y){useOverlap=true;}else{useOverlap=ecOverlaps[overlap*stride+id].span.x<ecSingles[single*stride+id].span.x;}}
   var lo=0.0;var h=0.0;var mask=0u;var subMember=0u;
   if(useOverlap){
    let sp=ecOverlaps[overlap*stride+id];
    h=(sp.span.y-sp.span.x)*.5;lo=sp.span.x+f32(overPanel)*h;mask=sp.info.x;
   }else{
    let sub=ecSingles[single*stride+id];single++;
    if(sub.info.x>=4u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
    lo=sub.span.x;h=sub.span.y-sub.span.x;mask=1u<<sub.info.x;subMember=sub.info.x;
   }
   var minus=vec4<f32>(0);var plus=vec4<f32>(0);
   for(var q=0u;q<2u;q++){
    let node=select(select(gx0,.5-sqrt(3.0)/6.0,useOverlap),select(gx1,.5+sqrt(3.0)/6.0,useOverlap),q==1u);
    EC_EVAL_NODE
    if(q==0u){minus=val;}else{plus=val;}
   }
   if(useOverlap){
    let v0=ob*minus+oa*plus;if(v0.w>0.0){state=ecFlow(state,v0,h);}
    let v1=oa*minus+ob*plus;if(v1.w>0.0){state=ecFlow(state,v1,h);}
    if(overPanel==0u){overPanel=1u;}else{overPanel=0u;overlap++;}
   }else{
    let v=.5*(minus+plus);if(v.w>0.0){state=ecFlow(state,v,h);}
   }
  }
 }
 if(any((bitcast<vec4<u32>>(state)&vec4<u32>(0x7f800000u))==vec4<u32>(0x7f800000u))||state.w<0||state.w>1){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
 ecPublication[ecPixel(id)]=vec4<f32>(state.xyz,1.0-state.w);
}
)WGSL";
}
KernelRoutedTransport::~KernelRoutedTransport(){
 enableRouteObserver(false);releaseBuffers();for(auto p:common)if(p)wgpuComputePipelineRelease(p);if(classifySkip)wgpuComputePipelineRelease(classifySkip);
 for(auto& [key,p]:programs)kernel_routed_detail::releaseProgram(p);
}
void KernelRoutedTransport::releaseBuffers(){
 for(auto* p:{&publication,&rays,&singles,&overlaps,&offsets,&indices,&counts,&args,&stripUniform,&specsUniform})if(*p){wgpuBufferRelease(*p);*p=nullptr;}
 for(auto& p:lists)if(p){wgpuBufferRelease(p);p=nullptr;}
 for(auto& cohort:prodGroups){for(auto& bg:cohort)if(bg)wgpuBindGroupRelease(bg);cohort.clear();}
 if(resetGroup){wgpuBindGroupRelease(resetGroup);resetGroup=nullptr;}
 if(argsGroup){wgpuBindGroupRelease(argsGroup);argsGroup=nullptr;}
 if(cachedClassifyGroup){wgpuBindGroupRelease(cachedClassifyGroup);cachedClassifyGroup=nullptr;}
 cachedSpecsBuffer=nullptr;cachedClassifyPipe=nullptr;
 cachedWorkLimit=0xFFFFFFFFu;cachedSkipEmpty=false;
 for(auto& [key,p]:programs){for(auto& bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);p.localGroups.clear();p.localGroupsBuilt=false;}
 width=height=capacity=tiles=stripRows=maxStrips=0;
}
bool KernelRoutedTransport::ensure(WGPUDevice d,WGPUQueue q,uint32_t w,uint32_t h){
 using namespace kernel_routed_detail;device=d;queue=q;WGPULimits limits={};
 if(wgpuDeviceGetLimits(d,&limits)!=WGPUStatus_Success||!w||!h||uint64_t(w)*h*16>std::min<uint64_t>(64ull*1024*1024,limits.maxStorageBufferBindingSize))return false;
 const auto groupsReady=[&](){return maxStrips&&std::all_of(prodGroups.begin(),prodGroups.end(),[&](const auto& v){return v.size()==maxStrips&&std::all_of(v.begin(),v.end(),[](auto g){return g!=nullptr;});});};
 if(width==w&&height==h&&publication&&rays&&singles&&overlaps&&offsets&&indices&&counts&&args&&stripUniform&&specsUniform&&
    std::all_of(lists.begin(),lists.end(),[](auto b){return b!=nullptr;})&&std::all_of(common.begin(),common.end(),[](auto p){return p!=nullptr;})&&
    classifySkip&&resetGroup&&argsGroup&&cachedClassifyGroup&&groupsReady())return true;
 releaseBuffers();CreationScope creation(d);
 const auto failed=[&](){creation.finish();releaseBuffers();enableRouteObserver(false);releaseCommon(*this);return false;};
 // 64 intervals/ray retained; partition work, never lower accuracy to fit.
 const auto maxRows=std::min<uint64_t>(48,limits.maxStorageBufferBindingSize/(uint64_t(w)*64*16));
 const uint32_t rows=static_cast<uint32_t>(maxRows/4*4);if(rows<4)return false;
 width=w;height=h;stripRows=rows;capacity=w*rows;tiles=((w+3)/4)*(rows/4);
 std::vector<uint32_t> off,idx;off.reserve(tiles+1);idx.reserve(capacity);
 for(uint32_t y=0;y<rows;y+=4)for(uint32_t x=0;x<w;x+=4){off.push_back(idx.size());for(uint32_t dy=0;dy<4;++dy)for(uint32_t dx=0;dx<4&&x+dx<w;++dx)idx.push_back((y+dy)*w+x+dx);}
 off.push_back(idx.size());publication=buffer(d,q,uint64_t(w)*h*16);rays=buffer(d,q,uint64_t(capacity)*48);singles=buffer(d,q,uint64_t(capacity)*64*16);overlaps=buffer(d,q,uint64_t(capacity)*16*16);
 offsets=buffer(d,q,off.size()*4,0,off.data());indices=buffer(d,q,idx.size()*4,0,idx.data());counts=buffer(d,q,16);args=buffer(d,q,48,WGPUBufferUsage_Indirect);
 for(auto& b:lists)b=buffer(d,q,uint64_t(tiles)*4);
 maxStrips=(h+rows-1)/rows;if(!maxStrips)return false;
 {WGPUBufferDescriptor bd={};bd.label=wgpu::Device::str("strip uniforms");bd.size=uint64_t(maxStrips)*kUniformStride;bd.usage=WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;stripUniform=wgpuDeviceCreateBuffer(d,&bd);}
 {WGPUBufferDescriptor bd={};bd.label=wgpu::Device::str("curtain specs uniform");bd.size=sizeof(KernelRoutedSpec)*4;bd.usage=WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;specsUniform=wgpuDeviceCreateBuffer(d,&bd);}
 if(!publication||!rays||!singles||!overlaps||!offsets||!indices||!counts||!args||!lists[0]||!lists[1]||!lists[2]||!stripUniform||!specsUniform)return failed();
 // Static pipelines may have survived a successful resize. Missing pipelines
 // are built independently; a failed transaction releases the partial set.
 if(common[2]){wgpuComputePipelineRelease(common[2]);common[2]=nullptr;}
 std::string routeArgsCode=routed_wgsl::kRouteArgsWgsl;
 replace(routeArgsCode,"CAPACITY_WORKGROUPS",std::to_string((capacity+63u)/64u)+"u");
 const char* codes[]={routed_wgsl::kResetCountersWgsl,routed_wgsl::kClassifyWgsl,routeArgsCode.c_str(),routed_wgsl::kS1UnfusedProducerWgsl,routed_wgsl::kS2ProducerWgsl,routed_wgsl::kFallbackProducerWgsl};
 const char* names[]={"resetCountersMain","classifyTilesMain","routeArgs","s1ProducerMain","s2ProducerMain","fallbackProducerMain"};
 for(unsigned i=0;i<6;++i)if(!common[i]){common[i]=pipeline(d,codes[i],names[i]);if(!common[i])return failed();}
 if(!classifySkip)classifySkip=pipeline(d,routed_wgsl::kClassifyWgsl,"classifyTilesMain");
 if(!classifySkip)return failed();
 if(!resetGroup)resetGroup=group(device,common[0],{entry(0,counts,16)});
 if(!argsGroup)argsGroup=group(device,common[2],{entry(0,counts,16),entry(1,args,48)});
 if(!cachedClassifyGroup)cachedClassifyGroup=group(device,common[1],{entry(0,specsUniform,sizeof(KernelRoutedSpec)*4,0),entry(1,rays,uint64_t(capacity)*48),entry(2,offsets,uint64_t(tiles+1)*4),entry(3,indices,uint64_t(capacity)*4),entry(4,lists[0],uint64_t(tiles)*4),entry(5,lists[1],uint64_t(tiles)*4),entry(6,lists[2],uint64_t(tiles)*4),entry(7,counts,16)});
 cachedClassifyPipe=common[1];
 if(!resetGroup||!argsGroup||!cachedClassifyGroup)return failed();
 for(unsigned i=0;i<3;++i)prodGroups[i].assign(maxStrips,nullptr);
 for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
  for(unsigned i=0;i<3;++i){
   auto entries=std::vector<WGPUBindGroupEntry>{entry(0,stripUniform,48,uint64_t(stripIdx)*kUniformStride),entry(1,specsUniform,sizeof(KernelRoutedSpec)*4,0),entry(2,rays,uint64_t(capacity)*48),entry(3,singles,uint64_t(capacity)*64*16)};
   if(i==2)entries.push_back(entry(4,overlaps,uint64_t(capacity)*16*16));
   entries.push_back(entry(5,offsets,uint64_t(tiles+1)*4));entries.push_back(entry(6,indices,uint64_t(capacity)*4));entries.push_back(entry(7,lists[i],uint64_t(tiles)*4));entries.push_back(entry(8,counts,16));
   prodGroups[i][stripIdx]=group(device,common[i+3],entries);
   if(!prodGroups[i][stripIdx])return failed();
  }
 }
 if(!creation.finish())return failed();
 return true;
}
bool KernelRoutedTransport::enableRouteObserver(bool on){
 using namespace kernel_routed_detail;
 if(!on){observeRoutes=false;if(observerPipe){wgpuComputePipelineRelease(observerPipe);observerPipe=nullptr;}if(accumPipe){wgpuComputePipelineRelease(accumPipe);accumPipe=nullptr;}if(routeHistogram){wgpuBufferRelease(routeHistogram);routeHistogram=nullptr;}return true;}
 if(!device||!common[1])return false;if(observerPipe&&accumPipe&&routeHistogram){observeRoutes=true;return true;}
 std::string src=routed_wgsl::kClassifyWgsl;
 const std::string tailAnchor="    if (activeCount == 0u) {\n";const auto tail=src.find(tailAnchor);if(tail==std::string::npos)return false;
 src.erase(tail);src+="    var ordered = (activeCount >= 2u && !tileCanOverlap);\n    let key = activeMemberMask | (aabbMask << 4u) | (select(0u, 1u, tileCanOverlap) << 8u) | (select(0u, 1u, ordered) << 9u);\n    atomicAdd(&routeHist[key], 1u);\n}\n";
 if(!replace(src,"var activeMemberMask = 0u;","var activeMemberMask = 0u;\n    var aabbMask = 0u;")||!replace(src,"        if (anyRayHit && uxMax > uxMin) {","        if (anyRayHit) { aabbMask |= (1u << m); }\n        if (anyRayHit && uxMax > uxMin) {")||
    !replace(src,"@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;","@group(0) @binding(8) var<storage, read_write> routeHist: array<atomic<u32>>;"))return false;
 static const char* accum=R"WGSL(
@group(0) @binding(0) var<storage, read> counts: array<u32>;
@group(0) @binding(1) var<storage, read_write> hist: array<atomic<u32>>;
@compute @workgroup_size(1) fn accumCounts() { for (var i = 0u; i < 3u; i++) { atomicAdd(&hist[1024u + i], counts[i]); } atomicAdd(&hist[1027u], 1u); }
)WGSL";
 CreationScope creation(device);
 observerPipe=pipeline(device,src,"classifyTilesMain");accumPipe=pipeline(device,accum,"accumCounts");routeHistogram=buffer(device,queue,1032*4);
 if(!creation.finish()||!observerPipe||!accumPipe||!routeHistogram){enableRouteObserver(false);return false;}resetRouteHistogram();observeRoutes=true;return true;
}
void KernelRoutedTransport::resetRouteHistogram(){if(!routeHistogram)return;std::vector<uint32_t> z(1032,0);wgpuQueueWriteBuffer(queue,routeHistogram,0,z.data(),z.size()*4);}
KernelRoutedTransport::Program* KernelRoutedTransport::program(const std::string& source){
 using namespace kernel_routed_detail;auto it=programs.find(source);if(it!=programs.end())return it->second.consumer&&it->second.setup?&it->second:nullptr;
 if(programs.size()>=kMaxPrograms)return nullptr;
 Program p;CreationScope creation(device);
 const auto failed=[&]()->Program*{creation.finish();releaseProgram(p);return nullptr;};
 const auto fs=source.find("@fragment\nfn fs(");const auto brace=source.find('{',fs);const auto end=source.find("    var transmittance = 1.0;",brace);
 if(fs==std::string::npos||brace==std::string::npos||end==std::string::npos)return nullptr;
 auto prefix=source.substr(0,fs);auto setupBody=source.substr(brace+1,end-brace-1);
 size_t pos=0;while((pos=setupBody.find("in.position",pos))!=std::string::npos){setupBody.replace(pos,11,"position");pos+=8;}
 // Candidate setup arithmetic is isolated from the unchanged fragment fallback.
 replace(setupBody,"u.invViewProj * farNdc","ecMat(u.invViewProj,farNdc)");
 pos=0;while((pos=setupBody.find("discard;",pos))!=std::string::npos){setupBody.replace(pos,8,"ecPublication[ecPixel(id)]=vec4<f32>(0);ecRays[id].counts.w=0x40000000u;return;");pos+=79;}
 auto setupPrefix=prefix;replace(setupPrefix,"u.invViewProj * ndc","ecMat(u.invViewProj,ndc)");
 const std::string commonText=auxiliary;
 std::string setup=setupPrefix+commonText+R"WGSL(
 @compute @workgroup_size(64) fn ecSetup(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x){return;}
 let x=id%ecWindow.window.x;let y=id/ecWindow.window.x+ecWindow.window.z;
 if(y>=ecWindow.window.y){ecRays[id].counts.w=0x40000000u;return;}
 if(any(abs(u.eyePos.xyz)>vec3<f32>(2048))||ecWindow.control.w==0u){
  ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);
  ecRays[id]=ECRay(vec4<f32>(0),vec4<f32>(0),vec4<u32>(0u,0u,0u,select(0x40000000u,0x80000001u,ecWindow.control.w==0u)));
  return;
 }
 let position=vec4<f32>(f32(x)+.5,f32(y)+.5,0,1);
)WGSL"+setupBody+R"WGSL(
 if(t1>2048.0){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);ecRays[id].counts.w=0x40000000u;return;}
 ecPublication[ecPixel(id)] = vec4<f32>(0.0);
 ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(0u, 0u, 0u, 0u));
 }
)WGSL";
 std::vector<uint32_t> sharedGroups;
 const bool deduplicate = checkEvaluatorsDeduplicatable(source, sharedGroups);
 p.evaluatorsDeduplicated=deduplicate;
 auto consumePrefix = prefix;
 if (deduplicate) {
  for (int i = 1; i < 4; ++i) {
   const std::string n = std::to_string(i);
   for (uint32_t gid : sharedGroups) {
    const std::string decl = "var<private> g_volumeShared_" + n + "_G" + std::to_string(gid) + ": f32;\n";
    size_t dp = 0;
    while ((dp = consumePrefix.find(decl, dp)) != std::string::npos) {
     consumePrefix.erase(dp, decl.size());
    }
   }
   const std::string gFrom = "g_volumeShared_" + n + "_";
   size_t gp = 0;
   while ((gp = consumePrefix.find(gFrom, gp)) != std::string::npos) {
    consumePrefix.replace(gp, gFrom.size(), "g_volumeShared_0_");
    gp += 17;
   }
  }
 }
 std::string tail = consumer;
 if (deduplicate) {
  std::string helper = "fn ecSingleMember(world: vec3<f32>, wo: vec3<f32>, s: f32, m: u32) -> vec4<f32> {\n"
                       " g_instIdx = m + 1u;\n"
                       " let p = world - instances[g_instIdx].origin.xyz;\n";
  if (source.find("fn volumeShared_0_Eval(") != std::string::npos) {
   helper += " volumeShared_0_Eval(p);\n";
  }
  helper += " let D = volumeDensityEval_0(p);\n"
            " if (D > 0.0) {\n"
            "  let X = max(volumeExtinctionEval_0(p, D), 1e-6);\n"
            "  var E = vec3<f32>(0.0);\n";
  for (unsigned i = 0; i < 4; ++i) {
   const auto n = std::to_string(i);
   const std::string ifClause = (i == 0 ? "  if (m == 0u) {\n" : (i < 3 ? "  } else if (m == " + n + "u) {\n" : "  } else {\n"));
   helper += ifClause;
   helper += "   if (!VOLUME_EMISSION_READS_OMEGA_" + n + " || s > 1e-8) { E = max(volumeEmissionEval_" + n + "(p, wo), vec3<f32>(0.0)); }\n";
  }
  helper += "  }\n"
            "  return vec4<f32>(E, X);\n"
            " }\n"
            " return vec4<f32>(0.0);\n"
            "}\n";
  replace(tail, "EC_CONSUMER_HELPERS", helper);
  replace(tail, "EC_PROJECTED", "_ = wo; _ = mask; _ = world; return result;");
  replace(tail, "EC_EVAL_SINGLE_PAIR", "let s0 = lo + gx0 * h;\n"
                                       "   let s1 = lo + gx1 * h;\n"
                                       "   let minus = ecSingleMember(ro + s0 * rd, wo, s0, subMember);\n"
                                       "   let plus = ecSingleMember(ro + s1 * rd, wo, s1, subMember);");
  replace(tail, "EC_EVAL_NODE", "var val = vec4<f32>(0.0);\n"
                                "   let sNode = lo + node * h;\n"
                                "   let worldNode = ro + sNode * rd;\n"
                                "   if (!useOverlap) {\n"
                                "    val = ecSingleMember(worldNode, wo, sNode, subMember);\n"
                                "   } else {\n"
                                "    var mMask = mask;\n"
                                "    loop {\n"
                                "     if (mMask == 0u) { break; }\n"
                                "     let m = firstTrailingBit(mMask);\n"
                                "     mMask &= mMask - 1u;\n"
                                "     val += ecSingleMember(worldNode, wo, sNode, m);\n"
                                "    }\n"
                                "   }");
 } else {
  replace(tail, "EC_CONSUMER_HELPERS", "");
  std::string sampling;
  for (unsigned i = 0; i < 4; ++i) {
   const auto n = std::to_string(i);
   sampling += "if((mask&" + std::to_string(1u << i) + "u)!=0u){g_instIdx=" + std::to_string(i + 1) + "u;let p=world-instances[" + std::to_string(i + 1) + "u].origin.xyz;" +
               (source.find("fn volumeShared_" + n + "_Eval(") != std::string::npos ? "volumeShared_" + n + "_Eval(p);" : "") +
               "let D=volumeDensityEval_" + n + "(p);if(D>0.0){let X=max(volumeExtinctionEval_" + n + "(p,D),1e-6);var E=vec3<f32>(0);if(!VOLUME_EMISSION_READS_OMEGA_" + n + "||s>1e-8){E=max(volumeEmissionEval_" + n + "(p,wo),vec3<f32>(0));}result+=vec4<f32>(E,X);}}\n";
  }
  replace(tail, "EC_PROJECTED", sampling);
  replace(tail, "EC_EVAL_SINGLE_PAIR", "let minus = ecProjected(ro, rd, wo, lo + gx0 * h, mask);\n"
                                       "   let plus = ecProjected(ro, rd, wo, lo + gx1 * h, mask);");
  replace(tail, "EC_EVAL_NODE", "let val = ecProjected(ro, rd, wo, lo + node * h, mask);");
 }
 std::string consume = consumePrefix + commonText + tail;
 WGPUBindGroupLayoutEntry g[4]={};g[0].binding=0;g[0].visibility=WGPUShaderStage_Compute;g[0].buffer.type=WGPUBufferBindingType_Uniform;
 g[1].binding=1;g[1].visibility=WGPUShaderStage_Compute;g[1].buffer.type=WGPUBufferBindingType_ReadOnlyStorage;
 g[2].binding=2;g[2].visibility=WGPUShaderStage_Compute;g[2].texture.sampleType=WGPUTextureSampleType_Depth;g[2].texture.viewDimension=WGPUTextureViewDimension_2D;
 g[3].binding=3;g[3].visibility=WGPUShaderStage_Compute;g[3].buffer.type=WGPUBufferBindingType_Storage;
 WGPUBindGroupLayoutDescriptor gd={};gd.entryCount=4;gd.entries=g;p.globals=wgpuDeviceCreateBindGroupLayout(device,&gd);
 WGPUBindGroupLayoutEntry inst={};inst.binding=0;inst.visibility=WGPUShaderStage_Compute;inst.buffer.type=WGPUBufferBindingType_ReadOnlyStorage;gd.entryCount=1;gd.entries=&inst;p.instances=wgpuDeviceCreateBindGroupLayout(device,&gd);
 WGPUBindGroupLayoutEntry local[4]={};for(int i=0;i<4;++i){local[i].binding=i;local[i].visibility=WGPUShaderStage_Compute;local[i].buffer.type=i==0?WGPUBufferBindingType_Uniform:WGPUBufferBindingType_Storage;}gd.entryCount=4;gd.entries=local;p.local=wgpuDeviceCreateBindGroupLayout(device,&gd);
 if(!p.globals||!p.instances||!p.local)return failed();
 WGPUBindGroupLayout layouts[]={p.globals,p.instances,p.local};WGPUPipelineLayoutDescriptor ld={};ld.bindGroupLayoutCount=3;ld.bindGroupLayouts=layouts;auto layout=wgpuDeviceCreatePipelineLayout(device,&ld);if(!layout)return failed();
 p.setup=pipeline(device,setup,"ecSetup",layout);p.consumer=pipeline(device,consume,"ecConsume",layout);wgpuPipelineLayoutRelease(layout);
 if(!creation.finish()||!p.setup||!p.consumer)return failed();std::fprintf(stderr,"[WebGpuRenderer] routed transport production-evaluator pipeline active\n");return &programs.emplace(source,std::move(p)).first->second;
}
bool KernelRoutedTransport::encode(Program& p,WGPUCommandEncoder encoder,const KernelRoutedRange& globals,const KernelRoutedRange& params,const KernelRoutedRange& instances,WGPUTextureView depth){
 using namespace kernel_routed_detail;
 encodedPasses=0;encodedDispatches=0;dispatchTrace=collectDispatchTrace?0xcbf29ce484222325ull:0;
 if(!stripUniform||!specsUniform||!maxStrips)return false;
 if((height+stripRows-1)/stripRows>maxStrips)return false;
 CreationScope creation(device);
 if(cachedWorkLimit!=workLimit||cachedSkipEmpty!=skipEmptyTiles){
  for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
   std::array<uint32_t,12> config{capacity,64,16,workLimit,width,height,start,stripRows,skipEmptyTiles?1u:0u,0,0,0};
   wgpuQueueWriteBuffer(queue,stripUniform,uint64_t(stripIdx)*kUniformStride,config.data(),sizeof(config));
  }
  cachedWorkLimit=workLimit;cachedSkipEmpty=skipEmptyTiles;
 }
 // Lazily build cached local bind groups for this Program.
 if(!p.localGroupsBuilt||p.localGroups.size()!=maxStrips){
  for(auto& bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);
  p.localGroups.assign(maxStrips,nullptr);
  for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
   p.localGroups[stripIdx]=group(device,p.local,{
     entry(0,stripUniform,48,uint64_t(stripIdx)*kUniformStride),
     entry(1,rays,uint64_t(capacity)*48),
     entry(2,singles,uint64_t(capacity)*64*16),
     entry(3,overlaps,uint64_t(capacity)*16*16)
   });
   if(!p.localGroups[stripIdx]){
    creation.finish();
    for(auto bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);
    p.localGroups.clear();p.localGroupsBuilt=false;cachedWorkLimit=0xFFFFFFFFu;
    return false;
   }
  }
  p.localGroupsBuilt=true;
 }
 auto g=std::vector<WGPUBindGroupEntry>{entry(0,globals.buffer,globals.size,globals.offset),entry(1,params.buffer,params.size,params.offset),{},entry(3,publication,uint64_t(width)*height*16)};g[2].binding=2;g[2].textureView=depth;
 auto globalGroup=group(device,p.globals,g),instanceGroup=group(device,p.instances,{entry(0,instances.buffer,instances.size,instances.offset)});
 if(!this->resetGroup)this->resetGroup=group(device,common[0],{entry(0,counts,16)});
 auto resetGroup=this->resetGroup;
 auto classifyPipe=skipEmptyTiles&&classifySkip?classifySkip:common[1];
 if(!cachedClassifyGroup||cachedClassifyPipe!=classifyPipe){
  if(cachedClassifyGroup)wgpuBindGroupRelease(cachedClassifyGroup);
  cachedClassifyGroup=group(device,classifyPipe,{entry(0,specsUniform,sizeof(KernelRoutedSpec)*4,0),entry(1,rays,uint64_t(capacity)*48),entry(2,offsets,uint64_t(tiles+1)*4),entry(3,indices,uint64_t(capacity)*4),entry(4,lists[0],uint64_t(tiles)*4),entry(5,lists[1],uint64_t(tiles)*4),entry(6,lists[2],uint64_t(tiles)*4),entry(7,counts,16)});
  cachedClassifyPipe=classifyPipe;
 }
 auto classifyGroup=cachedClassifyGroup;
 if(!this->argsGroup)this->argsGroup=group(device,common[2],{entry(0,counts,16),entry(1,args,48)});
 auto argsGroup=this->argsGroup;
 WGPUBindGroup observerGroup=nullptr,accumulatorGroup=nullptr;
 if(observeRoutes&&observerPipe&&accumPipe&&routeHistogram){
  observerGroup=group(device,observerPipe,{entry(0,specsUniform,sizeof(KernelRoutedSpec)*4,0),entry(1,rays,uint64_t(capacity)*48),entry(2,offsets,uint64_t(tiles+1)*4),entry(3,indices,uint64_t(capacity)*4),entry(8,routeHistogram,1032*4)});
  accumulatorGroup=group(device,accumPipe,{entry(0,counts,16),entry(1,routeHistogram,1032*4)});
 }
 const auto releaseTemporary=[&](){for(auto bg:{globalGroup,instanceGroup,observerGroup,accumulatorGroup})if(bg)wgpuBindGroupRelease(bg);};
 if(!creation.finish()||!globalGroup||!instanceGroup||!resetGroup||!classifyGroup||!argsGroup||
    (observeRoutes&&(!observerGroup||!accumulatorGroup))){
  releaseTemporary();
  for(auto bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);p.localGroups.clear();p.localGroupsBuilt=false;
  if(cachedClassifyGroup){wgpuBindGroupRelease(cachedClassifyGroup);cachedClassifyGroup=nullptr;}cachedClassifyPipe=nullptr;
  if(this->resetGroup){wgpuBindGroupRelease(this->resetGroup);this->resetGroup=nullptr;}
  if(this->argsGroup){wgpuBindGroupRelease(this->argsGroup);this->argsGroup=nullptr;}
  cachedWorkLimit=0xFFFFFFFFu;return false;
 }
 auto begin=[&](){++encodedPasses;return wgpuCommandEncoderBeginComputePass(encoder,nullptr);};
 auto note=[&](const void* pipe,uint32_t n,uint64_t tag){++encodedDispatches;if(!collectDispatchTrace)return;for(uint64_t v:{uint64_t(reinterpret_cast<uintptr_t>(pipe)),uint64_t(n),tag})for(int b=0;b<8;++b)dispatchTrace=(dispatchTrace^((v>>(8*b))&255))*0x100000001b3ull;};
 WGPUComputePassEncoder shared=coalescePasses?begin():nullptr;
 auto open=[&](){return shared?shared:begin();};
 auto close=[&](WGPUComputePassEncoder pass){if(pass==shared)return;wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);};
 for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
  auto dispatch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,uint32_t n){auto pass=open();wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);note(cp,n,1);wgpuComputePassEncoderDispatchWorkgroups(pass,n,1,1);close(pass);};
  auto withContext=[&](WGPUComputePipeline cp){auto pass=open();wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,globalGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,instanceGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,2,p.localGroups[stripIdx],0,nullptr);note(cp,(capacity+63)/64,2);wgpuComputePassEncoderDispatchWorkgroups(pass,(capacity+63)/64,1,1);close(pass);};
  withContext(p.setup);dispatch(common[0],resetGroup,1);dispatch(classifyPipe,classifyGroup,(tiles+63)/64);dispatch(common[2],argsGroup,4);
  if(observerGroup){dispatch(observerPipe,observerGroup,(tiles+63)/64);dispatch(accumPipe,accumulatorGroup,1);}
  for(unsigned i=0;i<3;++i){auto pass=open();wgpuComputePassEncoderSetPipeline(pass,common[i+3]);wgpuComputePassEncoderSetBindGroup(pass,0,prodGroups[i][stripIdx],0,nullptr);note(common[i+3],i*12,3);wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,args,i*12);close(pass);}
  {auto pass=open();wgpuComputePassEncoderSetPipeline(pass,p.consumer);wgpuComputePassEncoderSetBindGroup(pass,0,globalGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,instanceGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,2,p.localGroups[stripIdx],0,nullptr);note(p.consumer,36,2);wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,args,36);close(pass);}
 }
 if(shared){wgpuComputePassEncoderEnd(shared);wgpuComputePassEncoderRelease(shared);}
 releaseTemporary();return true;
}
