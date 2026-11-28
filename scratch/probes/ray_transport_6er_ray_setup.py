"""Actual fragment vs compute ray/depth setup, isolated source-derived renderer.
Zach commissioned the next Screen bridge step; Sonnet Bridge proposed W3.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. No production edits or transport/performance admission.
"""
import argparse
import json
import math
from pathlib import Path
import struct

import ray_transport_6eq_screen_bridge as B

HOOKS = r'''
// Scratch Kernel-only device buffers/pipelines and capture counters. No authored
// state or cross-frame cache. Immutable capture window/mutation for each job.
static WGPUDevice rayDevice=nullptr;
static WGPUQueue rayQueue=nullptr;
static WGPUInstance rayInstance=nullptr;
static WGPUBuffer rayFrag=nullptr,rayCompute=nullptr;
static uint32_t rayWidth=0,rayHeight=0,rayStart=0,rayRows=0,rayDispatches=0;
static uint64_t rayBytes=0;
static std::string rayMode,rayScene,rayMutation,rayComputeCode;
static glm::mat4 rayInvViewProj(1);
bool ecRayEnabled(){return rayMutation!="control";}
WGPUBuffer ecRayBuffer(){return rayFrag;}
uint64_t ecRayBytes(){return rayBytes;}
static void ecRayInit(wgpu::Device& gpu,uint32_t w,uint32_t h){
    rayDevice=gpu.device;rayQueue=gpu.queue;rayInstance=gpu.instance;rayWidth=w;rayHeight=h;
    rayMode=std::getenv("EC_SCREEN_BRIDGE")?std::getenv("EC_SCREEN_BRIDGE"):"none:normal:0:190";
    char scene[40]={},mutation[40]={};
    if(std::sscanf(rayMode.c_str(),"%39[^:]:%39[^:]:%u:%u",scene,mutation,&rayStart,&rayRows)!=4)std::abort();
    rayScene=scene;rayMutation=mutation;
    if(!rayRows||rayStart+rayRows>h)std::abort();
    if(!ecRayEnabled())return;
    rayBytes=uint64_t(w)*rayRows*64;
    if(rayBytes>64ull*1024*1024)std::abort();
    WGPUBufferDescriptor d={};d.size=rayBytes;d.usage=WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc;
    rayFrag=wgpuDeviceCreateBuffer(gpu.device,&d);rayCompute=wgpuDeviceCreateBuffer(gpu.device,&d);
    if(!rayFrag||!rayCompute)std::abort();
}
static void ecRayOpaque(WebGpuRenderer& renderer){
    if(rayScene=="none")return;
    float d=.97f;
    if(rayScene=="threshold-lo")d=.9999985f;
    if(rayScene=="threshold-hi")d=.9999995f;
    auto point=[&](float x,float y){const auto p=rayInvViewProj*glm::vec4(x,y,d,1);return glm::vec3(p)/p.w;};
    const auto a=point(-1,-1),b=point(0,-1),c=point(0,1),e=point(-1,1);
    renderer.drawSolid({a,b,c,a,c,e},glm::vec4(.1f,.2f,.3f,1),Blend::Opaque,true);
}
static std::string rayBlock(const std::string& s,const std::string& name){
    const auto p=s.find(name);if(p==std::string::npos||s.find(name,p+name.size())!=std::string::npos)std::abort();
    const auto a=s.find('{',p);int depth=1;size_t q=a+1;
    for(;q<s.size()&&depth;++q){if(s[q]=='{')++depth;if(s[q]=='}')--depth;}
    if(depth)std::abort();return s.substr(p,q-p)+"\n";
}
static std::string rayRecorder(const char* fn,const char* buffer){
    const std::string debug=rayMutation=="trace-uniform"?"u.invViewProj[u32(position.x)%4u]":
        rayMutation=="trace-viewport"?"u.viewport":
        rayMutation.find("trace-")!=std::string::npos?"ecTraceValue":"vec4<f32>(bounds,position.xy)";
    return "fn "+std::string(fn)+R"WGSL((position: vec4<f32>, ro: vec3<f32>, rd: vec3<f32>, t0: f32, t1: f32,
        depth: f32, bounds: vec2<f32>, status: u32) {
    let x=u32(position.x);let y=u32(position.y);
    if(y<RAY_START || y>=RAY_START+RAY_ROWS){return;}
    let at=((y-RAY_START)*u32(u.viewport.x)+x)*4u;
)WGSL"+std::string(buffer)+"[at]=bitcast<vec4<u32>>(vec4<f32>(ro,t0));\n"+
      std::string(buffer)+"[at+1u]=bitcast<vec4<u32>>(vec4<f32>(rd,t1));\n"+
      std::string(buffer)+"[at+2u]=vec4<u32>(1u,status,bitcast<u32>(depth),0u);\n"+
      std::string(buffer)+"[at+3u]=bitcast<vec4<u32>>("+debug+");\n}\n";
}
std::string ecRayShader(std::string s){
    const std::string start="fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {";
    const auto a=s.find(start);const auto b=s.find("    var transmittance = 1.0;",a);
    if(a==std::string::npos||b==std::string::npos)std::abort();
    const bool canonical=rayMutation.rfind("canonical",0)==0;
    const auto p=a+start.size();std::string original=s.substr(p,b-p);
    const std::string matvec=R"WGSL(fn ecCanonicalMatVec(m:mat4x4<f32>,v:vec4<f32>)->vec4<f32>{
    return fma(m[0],vec4<f32>(v.x),fma(m[1],vec4<f32>(v.y),fma(m[2],vec4<f32>(v.z),m[3]*v.w)));
}
)WGSL";
    if(canonical){
        const std::string expr="u.invViewProj * farNdc";const auto ep=original.find(expr);if(ep==std::string::npos)std::abort();
        original.replace(ep,expr.size(),"ecCanonicalMatVec(u.invViewProj,farNdc)");
    }
    const std::string discard="    if (t1 <= t0) { discard; }";
    auto recordPrefix=[&](std::string t,const char* fn,bool fragment){
        const auto first=t.find(discard),second=t.find(discard,first+discard.size());
        if(first==std::string::npos||second==std::string::npos||t.find(discard,second+discard.size())!=std::string::npos)std::abort();
        std::string tail=fragment?" discard; }":" return; }";
        t.replace(second,discard.size(),"    if(t1<=t0){"+std::string(fn)+"(in.position,ro,rd,t0,t1,sceneDepth,bounds,2u);"+tail);
        t.replace(first,discard.size(),"    if(t1<=t0){"+std::string(fn)+"(in.position,ro,rd,t0,t1,-1.0,bounds,1u);"+tail);
        t+="    "+std::string(fn)+"(in.position,ro,rd,t0,t1,sceneDepth,bounds,3u);\n";
        const std::string h=canonical?"    let farH = ecCanonicalMatVec(u.invViewProj,farNdc);":"    let farH = u.invViewProj * farNdc;";
        const auto hp=t.find(h);if(hp==std::string::npos)std::abort();
        if(rayMutation=="trace-ndc")t.insert(hp,"    ecTraceValue=farNdc;\n");
        if(rayMutation=="trace-h"||rayMutation=="canonical-trace-h")t.insert(hp+h.size(),"\n    ecTraceValue=farH;");
        if(rayMutation=="trace-world"){
            const auto rp=t.find("    let rd = normalize(farWorld - ro);");if(rp==std::string::npos)std::abort();
            t.insert(rp,"    ecTraceValue=vec4<f32>(farWorld,farH.w);\n");
        }
        if(fragment&&rayMutation=="no-fragment")t=original;
        if(!fragment){
            size_t i=0;while((i=t.find("in.position",i))!=std::string::npos){t.replace(i,11,"position");i+=8;}
            if(rayMutation=="ignore-depth"){
                const std::string load="let sceneDepth = textureLoad(sceneDepthTex, px, 0);";
                const auto q=t.find(load);if(q==std::string::npos)std::abort();t.replace(q,load.size(),"let sceneDepth = 1.0;");
            }
        }
        return t;
    };
    const auto constants="const RAY_START:u32="+std::to_string(rayStart)+"u;\nconst RAY_ROWS:u32="+std::to_string(rayRows)+"u;\n";
    std::string depthFunction=rayBlock(s,"fn worldAtDepth(");
    if(canonical){const std::string old="u.invViewProj * ndc";const auto dp=depthFunction.find(old);if(dp==std::string::npos)std::abort();depthFunction.replace(dp,old.size(),"ecCanonicalMatVec(u.invViewProj,ndc)");}
    std::string cs=rayBlock(s,"struct VolumeGlobals {")+rayBlock(s,"struct VolumeInstanceData {")+
       "@group(0) @binding(0) var<uniform> u:VolumeGlobals;\n@group(0) @binding(2) var sceneDepthTex:texture_depth_2d;\n"
       "@group(1) @binding(0) var<storage,read> instances:array<VolumeInstanceData>;\n"
       "@group(0) @binding(4) var<storage,read_write> computeFacts:array<vec4<u32>>;\nvar<private> g_instIdx:u32;\nvar<private> ecTraceValue:vec4<f32>;\n"+
       constants+(canonical?matvec:std::string())+rayBlock(s,"fn rayAabbWorld(")+depthFunction+rayRecorder("ecComputeRecord","computeFacts")+
       "fn deviceSetup(position:vec4<f32>){"+recordPrefix(original,"ecComputeRecord",false)+"}\n";
    const std::string centre=rayMutation=="pixel-corner"?"0.0":"0.5";
    cs+="@compute @workgroup_size(64) fn cs(@builtin(global_invocation_id) id:vec3<u32>){\n"
        "let width=u32(u.viewport.x);if(id.x>=width*RAY_ROWS){return;}\n"
        "let x=id.x%width;let y=id.x/width+RAY_START;deviceSetup(vec4<f32>(f32(x)+"+centre+",f32(y)+"+centre+",0.0,1.0));}\n";
    if(!rayComputeCode.empty()&&rayComputeCode!=cs)std::abort();rayComputeCode=cs;
    s.replace(p,b-p,recordPrefix(original,"ecFragmentRecord",true));
    if(canonical){const auto old=rayBlock(s,"fn worldAtDepth(");const auto dp=s.find(old);s.replace(dp,old.size(),matvec+depthFunction);}
    const auto fp=s.find("@fragment\n");if(fp==std::string::npos)std::abort();
    s.insert(fp,constants+"var<private> ecTraceValue:vec4<f32>;\n@group(0) @binding(3) var<storage,read_write> fragmentFacts:array<vec4<u32>>;\n"+rayRecorder("ecFragmentRecord","fragmentFacts"));
    if(const char* path=std::getenv("EC_SCREEN_SHADER_DIR")){
        std::ofstream(std::string(path)+"/fragment.wgsl")<<s;std::ofstream(std::string(path)+"/compute.wgsl")<<cs;
    }
    return s;
}
void ecRayDispatch(WGPUDevice device,WGPUCommandEncoder encoder,WGPUBuffer uniforms,uint64_t offset,uint64_t bytes,
                   WGPUTextureView depth,WGPUBindGroupLayout instanceLayout,WGPUBindGroup instances){
    if(rayComputeCode.empty())std::abort();
    std::vector<uint32_t> poison(rayBytes/4,0xcdcdcdcdu);
    wgpuQueueWriteBuffer(rayQueue,rayFrag,0,poison.data(),rayBytes);wgpuQueueWriteBuffer(rayQueue,rayCompute,0,poison.data(),rayBytes);
    if(rayMutation=="no-compute")return;
    WGPUShaderSourceWGSL ws={};ws.chain.sType=WGPUSType_ShaderSourceWGSL;ws.code=wgpu::Device::str(rayComputeCode.c_str());
    WGPUShaderModuleDescriptor sd={};sd.nextInChain=&ws.chain;auto shader=wgpuDeviceCreateShaderModule(device,&sd);
    WGPUBindGroupLayoutEntry e[3]={};e[0].binding=0;e[0].visibility=WGPUShaderStage_Compute;e[0].buffer.type=WGPUBufferBindingType_Uniform;
    e[1].binding=2;e[1].visibility=WGPUShaderStage_Compute;e[1].texture.sampleType=WGPUTextureSampleType_Depth;e[1].texture.viewDimension=WGPUTextureViewDimension_2D;
    e[2].binding=4;e[2].visibility=WGPUShaderStage_Compute;e[2].buffer.type=WGPUBufferBindingType_Storage;
    WGPUBindGroupLayoutDescriptor ld={};ld.entryCount=3;ld.entries=e;auto globals=wgpuDeviceCreateBindGroupLayout(device,&ld);
    WGPUBindGroupLayout layouts[2]={globals,instanceLayout};WGPUPipelineLayoutDescriptor pl={};pl.bindGroupLayoutCount=2;pl.bindGroupLayouts=layouts;
    auto layout=wgpuDeviceCreatePipelineLayout(device,&pl);WGPUComputePipelineDescriptor pd={};pd.layout=layout;pd.compute.module=shader;pd.compute.entryPoint=wgpu::Device::str("cs");
    auto pipeline=wgpuDeviceCreateComputePipeline(device,&pd);
    WGPUBindGroupEntry ge[3]={};ge[0].binding=0;ge[0].buffer=uniforms;ge[0].offset=offset;ge[0].size=bytes;
    ge[1].binding=2;ge[1].textureView=depth;ge[2].binding=4;ge[2].buffer=rayCompute;ge[2].size=rayBytes;
    WGPUBindGroupDescriptor gd={};gd.layout=globals;gd.entryCount=3;gd.entries=ge;auto group=wgpuDeviceCreateBindGroup(device,&gd);
    auto pass=wgpuCommandEncoderBeginComputePass(encoder,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipeline);
    wgpuComputePassEncoderSetBindGroup(pass,0,group,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,instances,0,nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass,(rayWidth*rayRows+63)/64,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);++rayDispatches;
    wgpuBindGroupRelease(group);wgpuComputePipelineRelease(pipeline);wgpuPipelineLayoutRelease(layout);wgpuBindGroupLayoutRelease(globals);wgpuShaderModuleRelease(shader);
}
static void ecRayDump(WGPUBuffer source,const std::string& path){
    WGPUBufferDescriptor d={};d.size=rayBytes;d.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;auto buffer=wgpuDeviceCreateBuffer(rayDevice,&d);
    auto e=wgpuDeviceCreateCommandEncoder(rayDevice,nullptr);wgpuCommandEncoderCopyBufferToBuffer(e,source,0,buffer,0,rayBytes);
    auto command=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(rayQueue,1,&command);wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(e);
    MapResult result;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&result;
    wgpuBufferMapAsync(buffer,WGPUMapMode_Read,0,rayBytes,cb);while(!result.done)wgpuInstanceProcessEvents(rayInstance);if(!result.ok)std::abort();
    const auto* p=static_cast<const char*>(wgpuBufferGetConstMappedRange(buffer,0,rayBytes));std::ofstream(path,std::ios::binary).write(p,rayBytes);
    wgpuBufferUnmap(buffer);wgpuBufferRelease(buffer);
}
static void ecRayFinish(const char* path){
    if(ecRayEnabled()){ecRayDump(rayFrag,std::string(path)+".fragment.bin");ecRayDump(rayCompute,std::string(path)+".compute.bin");}
    nlohmann::json j={{"mode",rayMode},{"scene",rayScene},{"mutation",rayMutation},{"enabled",ecRayEnabled()},
       {"width",rayWidth},{"height",rayHeight},{"row_start",rayStart},{"rows",rayRows},{"bytes_per_stage",rayBytes},{"dispatches",rayDispatches}};
    std::printf("BRIDGE_JSON %s\n",j.dump().c_str());if(rayFrag)wgpuBufferRelease(rayFrag);if(rayCompute)wgpuBufferRelease(rayCompute);
}
'''


def renderer_source(s):
    s=B.once(s,'#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"',
       '#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"\n'
       'extern std::string ecRayShader(std::string);\nextern WGPUBuffer ecRayBuffer();\nextern uint64_t ecRayBytes();\n'
       'extern void ecRayDispatch(WGPUDevice,WGPUCommandEncoder,WGPUBuffer,uint64_t,uint64_t,WGPUTextureView,WGPUBindGroupLayout,WGPUBindGroup);')
    s=B.once(s,'WebGpuRenderer::volumePipeline(const sdfwgsl::Program& program) {\n    const auto& wgsl = program.wgsl;',
       'WebGpuRenderer::volumePipeline(const sdfwgsl::Program& program) {\n    const auto wgsl = ecRayShader(program.wgsl);')
    s=B.once(s,'WGPUBindGroupLayoutEntry globalEntries[3] = {};','WGPUBindGroupLayoutEntry globalEntries[4] = {};')
    a='    globalEntries[2].texture.multisampled = false;'
    s=B.once(s,a,a+'\n    globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_Storage;')
    s=B.once(s,'    globalDesc.entryCount = 3;','    globalDesc.entryCount = 4;')
    s=B.once(s,'WGPUBindGroupEntry globalEntries[3] = {};','WGPUBindGroupEntry globalEntries[4] = {};')
    a='        globalEntries[2].textureView = _depthView;'
    s=B.once(s,a,a+'\n        globalEntries[3].binding=3;globalEntries[3].buffer=ecRayBuffer();globalEntries[3].size=ecRayBytes();')
    s=B.once(s,'        globalBgDesc.entryCount = 3;','        globalBgDesc.entryCount = 4;')
    a='        if (!_pass) {\n            _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &volumePassDesc);'
    s=B.once(s,a,'        if(_pass){wgpuRenderPassEncoderEnd(_pass);wgpuRenderPassEncoderRelease(_pass);_pass=nullptr;_boundPipeline=nullptr;}\n'
       '        ecRayDispatch(_device,_encoder,globalAlloc.buffer,globalAlloc.offset,globalAlloc.size,_depthView,pipeline->instanceBgl,instanceBg);\n'+a)
    return s


def probe_source(s):
    a='int main(int argc, char** argv) {';s=B.once(s,a,HOOKS+'\n'+a)
    s=B.once(s,'    WebGpuRenderer renderer;','    ecRayInit(gpu,width,height);\n    WebGpuRenderer renderer;')
    a='    renderer.setCamera(glm::lookAt(eye, look, glm::vec3(0, 1, 0)),'
    s=B.once(s,a,'    rayInvViewProj=glm::inverse(glm::perspectiveZO(glm::radians(60.0f),float(width)/height,.1f,1000.f)*glm::lookAt(eye,look,glm::vec3(0,1,0)));\n'+a)
    a='        renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));'
    s=B.once(s,a,a+'\n        ecRayOpaque(renderer);')
    s=B.once(s,'    setCurrentRenderer(nullptr);\n    return 0;','    setCurrentRenderer(nullptr);\n    ecRayFinish(outPath);\n    return 0;')
    return s


def compare(directory):
    r=B.read(directory/'receipt.json');d=r['diagnostic'];n=d['width']*d['rows']
    f=(directory/'capture.rgba.fragment.bin').read_bytes();c=(directory/'capture.rgba.compute.bin').read_bytes()
    B.need(len(f)==len(c)==n*64,'ray fact coverage')
    fw=memoryview(f).cast('I');cw=memoryview(c).cast('I');ff=memoryview(f).cast('f');identical=f==c
    counts=dict(fragment_visited=0,missing_compute=0,missing_fragment_valid=0,poison_corrupt=0,
        box_empty=0,opaque_empty=0,valid=0,compute_box_empty=0,compute_opaque_empty=0,compute_valid=0,nonfinite_records=0,depth_branch=0,depth_ignored_near_far=0,clipped=0,
        record_coordinate_mismatches=0,pixel_centre_mismatches=0,depth_sample_mismatches=0,ray_mismatches=0,clip_mismatches=0,status_mismatches=0)
    for i in range(n):
        a=i*16
        if cw[a+8]!=1:counts['missing_compute']+=1;continue
        B.need(cw[a+9] in (1,2,3),'compute ray status')
        counts[{1:'compute_box_empty',2:'compute_opaque_empty',3:'compute_valid'}[cw[a+9]]]+=1
        if fw[a+8]!=1:
            counts['poison_corrupt']+=fw[a+8]!=0xcdcdcdcd
            counts['missing_fragment_valid']+=cw[a+9]==3;continue
        counts['fragment_visited']+=1
        counts['record_coordinate_mismatches']+=ff[a+14]!=(i%d['width']+.5) or ff[a+15]!=(i//d['width']+d['row_start']+.5)
        counts['nonfinite_records']+=any(not math.isfinite(ff[a+k]) for k in [0,1,2,3,4,5,6,7,12,13])
        if not identical:
            counts['pixel_centre_mismatches']+=any(fw[a+k]!=cw[a+k] for k in [14,15])
            counts['depth_sample_mismatches']+=fw[a+10]!=cw[a+10] and fw[a+9]!=1 and cw[a+9]!=1
            counts['ray_mismatches']+=any(fw[a+k]!=cw[a+k] for k in [0,1,2,4,5,6])
            counts['clip_mismatches']+=any(fw[a+k]!=cw[a+k] for k in [3,7,12,13])
            counts['status_mismatches']+=fw[a+9]!=cw[a+9]
        status=fw[a+9];B.need(status in (1,2,3),'ray status')
        counts[{1:'box_empty',2:'opaque_empty',3:'valid'}[status]]+=1
        if status!=1:
            depth=ff[a+10]
            counts['depth_branch']+=depth<.999999
            counts['depth_ignored_near_far']+=.999999<=depth<1
            end=ff[a+7]
            bound=ff[a+13]
            counts['clipped']+=end<bound
    result=dict(records=n,**counts,fragment_sha256=B.digest(directory/'capture.rgba.fragment.bin'),compute_sha256=B.digest(directory/'capture.rgba.compute.bin'))
    B.write(directory/'comparison.json',result);print(json.dumps(result))
    B.need(counts['missing_compute']==0,'Missing compute ray ownership')
    B.need(counts['fragment_visited']>0 and counts['missing_fragment_valid']==0,'Missing fragment ray ownership')
    B.need(counts['poison_corrupt']==0,'fragment poison corrupt')
    B.need(counts['nonfinite_records']==0,'Nonfinite ray facts')
    B.need(counts['record_coordinate_mismatches']==0,'Fragment record coordinate mismatch')
    B.need(counts['pixel_centre_mismatches']==0,'Device pixel-centre mismatch')
    B.need(counts['depth_sample_mismatches']==0,'Device opaque-depth sample mismatch')
    B.need(counts['ray_mismatches']==0,'Device ray bit mismatch')
    B.need(counts['clip_mismatches']==0 and counts['status_mismatches']==0,'Device depth/clip bit mismatch')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    q=sub.add_parser('build');q.add_argument('--build-root',type=Path,required=True);q.add_argument('--work',type=Path,required=True)
    q=sub.add_parser('capture');q.add_argument('--work',type=Path,required=True);q.add_argument('--case',required=True);q.add_argument('--binary',type=Path,required=True)
    q.add_argument('--scene',choices=['none','front','threshold-lo','threshold-hi'],default='none')
    q.add_argument('--mutation',choices=['normal','control','pixel-corner','ignore-depth','no-compute','no-fragment','trace-uniform','trace-viewport','trace-ndc','trace-h','trace-world','canonical-matvec','canonical-trace-h'],default='normal')
    q.add_argument('--width',type=int,default=334);q.add_argument('--height',type=int,default=190);q.add_argument('--row-start',type=int,default=0);q.add_argument('--rows',type=int,default=190)
    q.add_argument('--camera',default='0,50,75,40,62,110')
    q=sub.add_parser('compare');q.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.command=='build':
        # Reuse the source-matched external build mechanism, not the previous
        # publication experiment's hooks. Overrides live only in this process.
        B.renderer_source=renderer_source;B.probe_source=probe_source;B.build(a.build_root.resolve(),a.work.resolve())
    elif a.command=='capture':
        B.need(a.case.replace('-','').isalnum() and 64<=a.width<=2560 and 36<=a.height<=1440 and
               0<=a.row_start<a.height and 0<a.rows<=a.height-a.row_start and a.width*a.rows*64<=64*1024*1024,'bounded ray window')
        mode=f'{a.scene}:{a.mutation}:{a.row_start}:{a.rows}'
        r=B.invoke(a.work.resolve(),a.case,a.binary.resolve(),mode,a.width,a.height,a.camera)
        B.write(a.work/a.case/'receipt.json',r);print(json.dumps(r['diagnostic']))
    else:
        try:compare(a.directory)
        except ValueError as e:print('REJECT',e);raise SystemExit(2)


if __name__=='__main__':main()
