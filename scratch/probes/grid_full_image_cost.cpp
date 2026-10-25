// Scratch full-image cost probe (scratch/probes; never in CMake): Claude Code / Opus 5.5 /
// f3af84cf / 2026-10-20. Times the grid depth march over every pixel of the
// aurora/inside view (eye (0,50,75) → (40,62,110), 60° fovy) per curtain.
// Completion-fenced compute batches; NOT native frame time.
#define main unused_ray_witness_main
#include "../../tests/singularity/webgpu_line_quadrature_test.cpp"
#undef main
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
struct Stats{double ms=0;long long hit=0,admitted=0,occupied=0;double calls=0,switches=0,meanBound=0;};
static Stats runImage(wgpu::Device& gpu,const sdfwgsl::Program& prog,const glm::vec3& eyeMedium,const glm::vec3 basis[3],const glm::vec3& half,
                      uint32_t W,uint32_t H,int repeats,bool work){
    Stats st;std::vector<float> params=prog.params;const uint32_t cam=uint32_t(params.size());
    for(const auto& v:{eyeMedium,basis[0],basis[1],basis[2],half*0.98f})for(int i=0;i<3;++i)params.push_back(v[i]);
    params.push_back(float(W));params.push_back(float(H));params.push_back(0.0f);
    const uint64_t outBytes=uint64_t(W)*H*16;
    const float screenData[8]={float(W),float(H),0,0,0,0,0,0};
    auto buffer=[&](WGPUBufferUsage usage,uint64_t size,const void* data){WGPUBufferDescriptor d={};d.usage=usage;d.size=size;
        WGPUBuffer b=wgpuDeviceCreateBuffer(gpu.device,&d);if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return b;};
    WGPUBuffer screenBuf=buffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(screenData),screenData);
    WGPUBuffer paramBuf=buffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,params.size()*4,params.data());
    WGPUBuffer outBuf=buffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,outBytes,nullptr);
    WGPUBuffer readBuf=buffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,outBytes,nullptr);
    const std::string c=std::to_string(cam);
    std::string call=work?"lineGridWorkAt(o,d,lo,hi,0.006,0.0,vec3<f32>(0.0,0.0,-1.0))":
        "vec4<f32>(r.transmittance,r.status,r.panels,r.transmittanceError)";
    const std::string code=prog.wgsl+
        "\n@group(0) @binding(2) var<storage, read_write> results: array<vec4<f32>>;\n"
        "@compute @workgroup_size(8,8)\nfn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {\n"
        " let W=u32(P.v["+c+"u+15u]);let H=u32(P.v["+c+"u+16u]);if(gid.x>=W||gid.y>=H){return;}\n"
        " let i=gid.y*W+gid.x;screenPoint=vec3<f32>(0.0);\n"
        " let o=vec3<f32>(P.v["+c+"u],P.v["+c+"u+1u],P.v["+c+"u+2u]);\n"
        " let right=vec3<f32>(P.v["+c+"u+3u],P.v["+c+"u+4u],P.v["+c+"u+5u]);let up=vec3<f32>(P.v["+c+"u+6u],P.v["+c+"u+7u],P.v["+c+"u+8u]);\n"
        " let fwd=vec3<f32>(P.v["+c+"u+9u],P.v["+c+"u+10u],P.v["+c+"u+11u]);let half=vec3<f32>(P.v["+c+"u+12u],P.v["+c+"u+13u],P.v["+c+"u+14u]);\n"
        " let x=(2.0*(f32(gid.x)+0.5)/f32(W)-1.0);let y=(1.0-2.0*(f32(gid.y)+0.5)/f32(H));\n"
        " let d=normalize(fwd+x*right+y*up);\n"
        " let inv=1.0/d;let ta=(-half-o)*inv;let tb=(half-o)*inv;\n"
        " let lo=max(max(max(min(ta.x,tb.x),min(ta.y,tb.y)),min(ta.z,tb.z)),0.0);let hi=min(min(max(ta.x,tb.x),max(ta.y,tb.y)),max(ta.z,tb.z));\n"
        " if(!(hi>lo)){results[i]=vec4<f32>(1.0,-1.0,0.0,0.0);return;}\n"
        +(work?" results[i]="+call+";\n":" let r=lineGridDepthAt(o,d,lo,hi,0.006,0.0,vec3<f32>(0.0,0.0,-1.0));\n results[i]="+call+";\n")+
        " results[i]=results[i]+vec4<f32>(0.0*screen.size.x);\n}\n";
    wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);
    WGPUShaderSourceWGSL src={};src.chain.sType=WGPUSType_ShaderSourceWGSL;src.code=wgpu::Device::str(code.c_str());
    WGPUShaderModuleDescriptor smd={};smd.nextInChain=&src.chain;WGPUShaderModule sm=wgpuDeviceCreateShaderModule(gpu.device,&smd);
    WGPUComputePipelineDescriptor cpd={};cpd.compute.module=sm;cpd.compute.entryPoint=wgpu::Device::str("rayMain");
    WGPUComputePipeline cp=wgpuDeviceCreateComputePipeline(gpu.device,&cpd);
    PopResult compile;{WGPUPopErrorScopeCallbackInfo pcb={};pcb.mode=WGPUCallbackMode_AllowProcessEvents;pcb.callback=onPopError;pcb.userdata1=&compile;
        wgpuDevicePopErrorScope(gpu.device,pcb);while(!compile.done)wgpuDevicePoll(gpu.device,true,nullptr);}
    if(compile.type!=WGPUErrorType_NoError){std::printf("compile failed\n");st.ms=-1;return st;}
    WGPUBindGroupLayout bgl=wgpuComputePipelineGetBindGroupLayout(cp,0);
    WGPUBindGroupEntry e[3]={};e[0].binding=0;e[0].buffer=screenBuf;e[0].size=sizeof(screenData);
    e[1].binding=1;e[1].buffer=paramBuf;e[1].size=params.size()*4;e[2].binding=2;e[2].buffer=outBuf;e[2].size=outBytes;
    WGPUBindGroupDescriptor bgd={};bgd.layout=bgl;bgd.entryCount=3;bgd.entries=e;WGPUBindGroup bg=wgpuDeviceCreateBindGroup(gpu.device,&bgd);
    auto submit=[&](int n,bool read){
        WGPUCommandEncoder enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
        for(int k=0;k<n;++k){WGPUComputePassEncoder pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,(W+7)/8,(H+7)/8,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);}
        if(read)wgpuCommandEncoderCopyBufferToBuffer(enc,outBuf,0,readBuf,0,outBytes);
        WGPUCommandBuffer cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
        MapR mr;WGPUBufferMapCallbackInfo mci={};mci.mode=WGPUCallbackMode_AllowProcessEvents;mci.callback=onMap;mci.userdata1=&mr;
        if(!read){// fence via a tiny map of the read buffer after a copy-free submit
            WGPUCommandEncoder e2=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(e2,outBuf,0,readBuf,0,16);
            WGPUCommandBuffer c2=wgpuCommandEncoderFinish(e2,nullptr);wgpuQueueSubmit(gpu.queue,1,&c2);wgpuCommandBufferRelease(c2);wgpuCommandEncoderRelease(e2);
            wgpuBufferMapAsync(readBuf,WGPUMapMode_Read,0,16,mci);}
        else wgpuBufferMapAsync(readBuf,WGPUMapMode_Read,0,outBytes,mci);
        while(!mr.done)wgpuDevicePoll(gpu.device,true,nullptr);};
    submit(1,false);wgpuBufferUnmap(readBuf);   // warm
    const auto t0=std::chrono::steady_clock::now();submit(repeats,false);
    st.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count()/repeats;wgpuBufferUnmap(readBuf);
    submit(1,true);
    const auto* v=static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf,0,outBytes));
    for(uint64_t i=0;i<uint64_t(W)*H;++i){const float* q=v+4*i;
        if(work){if(q[1]==-1.0f && q[0]==1.0f)continue;++st.hit;st.calls+=q[0];st.switches+=q[1];}
        else{if(q[1]==-1.0f)continue;++st.hit;st.admitted+=q[1]==1.0f;st.occupied+=q[0]<0.999f;st.meanBound+=q[3];}}
    wgpuBufferUnmap(readBuf);
    if(st.hit){st.calls/=st.hit;st.switches/=st.hit;st.meanBound/=st.hit;}
    wgpuBindGroupRelease(bg);wgpuBindGroupLayoutRelease(bgl);wgpuComputePipelineRelease(cp);wgpuShaderModuleRelease(sm);
    for(WGPUBuffer b:{screenBuf,paramBuf,outBuf,readBuf})wgpuBufferRelease(b);
    return st;
}
int main(int argc,char**argv){
    const uint32_t W=argc>1?std::atoi(argv[1]):2560,H=argc>2?std::atoi(argv[2]):1440;const std::size_t kinks=argc>3?std::atoi(argv[3]):6;
    wgpu::Device gpu;if(!gpu.init()){std::printf("no device\n");return 77;}
    namespace fs=std::filesystem;fs::path dir="saves/zones/Northern Veil";nlohmann::json zone;
    if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
    else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});zone=nlohmann::json::parse(nlohmann::json::from_msgpack(b).at("MigrationRoot").get<std::string>());}
    const glm::vec3 eye(0,50,75),target(40,62,110);
    const glm::vec3 fwd=glm::normalize(target-eye),right=glm::normalize(glm::cross(fwd,glm::vec3(0,1,0))),up=glm::cross(right,fwd);
    const float ty=std::tan(glm::radians(30.0f)),tx=ty*float(W)/H;
    const glm::vec3 basis[3]={right*tx,up*ty,fwd};
    double total=0;
    for(const auto& json:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
        if(!field || !Rendering::readVolumeDensity(*field,0,0,b) || !b.extinctionExpr)continue;
        const auto* sigma=b.extinctionExpr->pieces[0].mathNode.get();
        const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}},{OntoMath::kOmegaXVar,{-1,1}},{OntoMath::kOmegaYVar,{-1,1}},{OntoMath::kOmegaZVar,{-1,1}}};
        const glm::vec3 half=glm::abs(b.scale);const double hd[3]={half.x,half.y,half.z},lo[3]={-hd[0],-hd[1],-hd[2]};
        OntoMath::RegionCertificate rc(*sigma,Rendering::kVolumeExecutionModel,4,symbols);
        const auto p0=std::chrono::steady_clock::now();
        const auto g=rc.proveGrid(lo,hd,1u<<24,nullptr,1.0/256,kinks);
        const double proveMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-p0).count();
        const auto prog=g?sdfwgsl::compileGridDepthTransport(*sigma,rc,*g,4096):sdfwgsl::Program{};
        if(!prog.ok){std::printf("%s: %s\n",b.producerId.c_str(),prog.error.c_str());continue;}
        const glm::vec3 eyeMedium=eye-b.origin;
        const auto s=runImage(gpu,prog,eyeMedium,basis,half,W,H,4,false);
        const auto w=runImage(gpu,prog,eyeMedium,basis,half,W,H,1,true);
        total+=s.ms;
        std::printf("%s: prove %.0f ms; %ux%u: %.1f ms per full-image dispatch; %lld pixels hit the box, %lld occupied, %lld admitted (%.1f%%); mean bound %.4g; per hit pixel %.1f σ calls, %.1f switch evals\n",
            b.producerId.c_str(),proveMs,W,H,s.ms,s.hit,s.occupied,s.admitted,s.hit?100.0*s.admitted/s.hit:0.0,s.meanBound,w.calls,w.switches);
    }
    std::printf("TOTAL depth march, four curtains: %.1f ms per frame-equivalent (compute only; no RGB, compositing or native gates)\n",total);
}
