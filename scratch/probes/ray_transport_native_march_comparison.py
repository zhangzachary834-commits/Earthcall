"""Compare production-generated V5 marching and complete unfused A* on identical
frozen rays in compute. Precomputed A* boundaries are diagnostic-only. Repeated
rays probe throughput, not a full-view or inhabited FPS result.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import argparse,ast,hashlib,importlib.util,json,os,shlex,subprocess
from pathlib import Path

def once(s,a,b):
 assert s.count(a)==1,(s.count(a),a[:80])
 return s.replace(a,b)

def source(s):
 s=once(s,'#include <webgpu/wgpu.h>','#include <webgpu/wgpu.h>\n#include "Singularity/Screen/VolumeZeroProof.hpp"')
 setup=r'''
    struct alignas(16) CurrentUniforms {
        glm::mat4 viewProj{1},invViewProj{1};
        glm::vec4 eyePos,viewport,incidentSource,incidentColor,sourceTime,volumeControl,volumeOutputError;
    };
    struct alignas(16) CurrentInstance {
        glm::vec4 origin,halfExtent,time;
        uint32_t paramOffset=0,zeroProofOffset=0,zeroProofDims=0,zeroProofPresent=0;
        uint32_t supportCertOffset=0,supportCertCount=0,supportClosedOffset=0,supportClosedCount=0;
    };
    static_assert(sizeof(CurrentUniforms)==240&&sizeof(CurrentInstance)==80);
    const int repeatFactor=std::getenv("EC_MATCH_REPEAT")?std::atoi(std::getenv("EC_MATCH_REPEAT")):1;
    if(repeatFactor<1||repeatFactor>128)throw std::runtime_error("repeat outside 1..128");
    const size_t executionRays=count*size_t(repeatFactor);
    CurrentUniforms currentU{};
    currentU.eyePos=glm::vec4(ro,1);currentU.viewport={2560,1440,float(executionRays),1};
    currentU.incidentSource={0,0,0,1};currentU.incidentColor=glm::vec4(1);currentU.sourceTime={float(clock),0,0,0};
    currentU.volumeControl={24,1,0,96};currentU.volumeOutputError=glm::vec4(0);
    auto rootLight=geom::FieldNode::fromJson(zone.at("spatialRoot"));
    if(!rootLight||!rootLight->field)throw std::runtime_error("missing authored source");
    auto light=single(&rootLight->field->astDefinition);
    if(!light||light->op!=M::Op::ScalarLeaf)throw std::runtime_error("source is not admitted zero literal");
    for(const auto& term:light->scalarForm.terms)if(term.coefficient!=0)throw std::runtime_error("not zero source");
    std::vector<sdfwgsl::VolumeProgramInput> currentInputs;
    std::vector<sdfwgsl::KernelVolumeEvaluator> currentEvaluators;
    std::vector<CurrentInstance> currentInstances(media.size()+1);
    uint32_t offset=0;bool trim=true;
    for(size_t id=0;id<media.size();++id) {
        Rendering::VolumeDensityBinding b;
        if(!Rendering::readVolumeDensity(*media[id].owner,0,0,b))throw std::runtime_error("binding refusal");
        sdfwgsl::VolumeProgramInput in;
        in.densityExpr=b.densityExpr;in.extinctionExpr=b.extinctionExpr;in.scatteringExpr=b.scatteringExpr;
        in.volumeChromaExpr=b.volumeChromaExpr;in.phaseExpr=b.phaseExpr;in.emissionExpr=b.emissionExpr;
        in.occluderSdf=b.occluderSdf;in.lightRadianceExpr=&rootLight->field->astDefinition;
        currentInputs.push_back(in);currentEvaluators.push_back(sdfwgsl::lowerVolumeEvaluator(in));
        if(!currentEvaluators.back().program.ok)throw std::runtime_error(currentEvaluators.back().program.error);
        auto& inst=currentInstances[id+1];inst.origin=glm::vec4(media[id].origin,0);inst.halfExtent=glm::vec4(media[id].half,0);
        inst.time={float(clock),0,0,0};inst.paramOffset=offset;offset+=currentEvaluators.back().program.params.size();
        if(glm::all(glm::lessThanEqual(glm::abs(ro-media[id].origin),media[id].half)))trim=false;
    }
    auto currentProgram=sdfwgsl::composeVolumeSet(currentInputs,currentEvaluators,trim);
    if(!currentProgram.ok)throw std::runtime_error(currentProgram.error);
    auto currentParams=currentProgram.params;
    for(size_t id=0;id<media.size();++id) {
        auto proof=Rendering::buildVolumeZeroProof(*currentInputs[id].densityExpr,glm::vec3(media[id].half));
        if(!proof.any())continue;
        auto& inst=currentInstances[id+1];inst.zeroProofOffset=currentParams.size();
        inst.zeroProofDims=proof.dims.x|(proof.dims.y<<10u)|(proof.dims.z<<20u);
        inst.zeroProofPresent=trim?1u|((proof.supportAxis+1u)<<1u):1u;
        currentParams.insert(currentParams.end(),proof.words.begin(),proof.words.end());
        const auto lo=proof.supportMin,hi=proof.supportMax;
        currentParams.insert(currentParams.end(),{lo.x,lo.y,lo.z,hi.x,hi.y,hi.z});
        if(trim)for(const auto& band:proof.supportBands)currentParams.insert(currentParams.end(),{band.x,band.y,band.z,band.w});
    }
    // Retain the production compiler's generated evaluator and marching body.
    // Only entrypoint/ray inputs and raw output are adapted to compute.
    const auto fs=currentProgram.wgsl.find("@fragment\nfn fs(");
    const auto begin=currentProgram.wgsl.find("    var transmittance = 1.0;",fs);
    const auto end=currentProgram.wgsl.find("    let alpha = 1.0 - transmittance;",begin);
    if(fs==std::string::npos||begin==std::string::npos||end==std::string::npos)throw std::runtime_error("current shader hooks changed");
    const std::string marchingBody=currentProgram.wgsl.substr(begin,end-begin);
    auto adaptBindings=[](std::string text) {
        auto replace=[&](const std::string& a,const std::string& b){auto pos=text.find(a);if(pos==std::string::npos)throw std::runtime_error("current binding hook");text.replace(pos,a.size(),b);};
        replace("@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;","");
        replace("@group(1) @binding(0)","@group(0) @binding(2)");
        return text;
    };
    const std::string extra=R"WGSL(
struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };
@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;
@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;
)WGSL";
    std::string currentShader=adaptBindings(currentProgram.wgsl.substr(0,fs))+extra+R"WGSL(
@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid:vec3<u32>) {
 let idx=gid.x;if(idx>=u32(u.viewport.z)){return;}
 let ray=capturedRays[idx%arrayLength(&capturedRays)];
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;
 let t0=ray.ro_clipA.w;let t1=ray.rd_clipB.w;g_instIdx=0u;
)WGSL"+marchingBody+R"WGSL(
 capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);
}
)WGSL";
    auto prepShader=adaptBindings(currentProgram.kernelVolumePreparationWgsl)+extra;
    if(currentProgram.kernelVolumePreparationWgsl.empty())throw std::runtime_error("expected native clock preparation");
    auto brace=prepShader.find('{',prepShader.find("fn volumeTimePrepare"));
    prepShader.insert(brace+1,"\n if(u.viewport.w<0.0){capturedOutput[0]=vec4<f32>(f32(capturedRays[0].counts.x));return;}\n");
    if(!outputPath.empty()) {
        std::ofstream a(outputPath+"-current.wgsl");a<<currentShader;
        std::ofstream b(outputPath+"-current-march-body.wgsl");b<<marchingBody;
    }
'''
 s=once(s,'    // --- WebGPU / Metal Initialization ---',setup+'\n    // --- WebGPU / Metal Initialization ---')
 s=once(s,'size_t outBytes = count * sizeof(GpuResult);','size_t outBytes = executionRays * sizeof(GpuResult);')
 s=once(s,'UniformData udata{float(clock), (uint32_t)count, 0, 0};','UniformData udata{float(clock), (uint32_t)executionRays, 0, 0};')
 # Native A* reads the same base rays at each deterministic repeat, writes unique outputs.
 s=s.replace('let r = rays[rayIdx];','let r = rays[rayIdx % arrayLength(&rays)];')
 s=once(s,'uint32_t workgroups = (uint32_t(count) + 63u) / 64u;','uint32_t workgroups = (uint32_t(executionRays) + 63u) / 64u;')
 s=once(s,'std::vector<GpuResult> res(ptr, ptr + count);','std::vector<GpuResult> res(ptr, ptr + executionRays);')
 # Build native baseline + GPU clock preparation using reflected layouts.
 gpu=r'''
    auto cuBuf=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(currentU),&currentU);
    auto cparamsBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,currentParams.size()*4,currentParams.data());
    auto ciBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,currentInstances.size()*sizeof(CurrentInstance),currentInstances.data());
    auto buildRaw=[&](const std::string& text,const char* entry) {
        wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL code={};code.chain.sType=WGPUSType_ShaderSourceWGSL;code.code=wgpu::Device::str(text.c_str());
        WGPUShaderModuleDescriptor md={};md.nextInChain=&code.chain;auto module=wgpuDeviceCreateShaderModule(gpu.device,&md);
        WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint=wgpu::Device::str(entry);
        auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);wgpuShaderModuleRelease(module);
        PopContext error;WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onPopError;cb.userdata1=&error;
        wgpuDevicePopErrorScope(gpu.device,cb);while(!error.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(error.type!=WGPUErrorType_NoError||!pipeline)throw std::runtime_error("production compute pipeline validation failed");
        return pipeline;
    };
    auto currentCP=buildRaw(currentShader,"rayMain"),prepCP=buildRaw(prepShader,"volumeTimePrepare");
    auto bindCurrent=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[5]={};
        WGPUBuffer buffers[5]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf};
        uint64_t sizes[5]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes};
        for(uint32_t k=0;k<5;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=5;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto currentBG=bindCurrent(currentCP),prepBG=bindCurrent(prepCP);
    {
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
        wgpuComputePassEncoderSetPipeline(pass,prepCP);wgpuComputePassEncoderSetBindGroup(pass,0,prepBG,0,nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass,media.size(),1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device,true,nullptr);
    }
    runPass(currentCP,currentBG);auto resCurrent=readResults();
    double currentDeltaRGB=0,currentDeltaT=0;
    for(size_t i=0;i<count;++i) {
        currentDeltaT=std::max(currentDeltaT,std::abs(double(resCurrent[i].rgb_T[3])-R[i][7].get<double>()));
        for(int k=0;k<3;++k)currentDeltaRGB=std::max(currentDeltaRGB,std::abs(double(resCurrent[i].rgb_T[k])-R[i][8+k].get<double>()));
    }
    std::cout<<"CURRENT stored-native agreement RGB="<<currentDeltaRGB<<" T="<<currentDeltaT<<" trim="<<trim<<std::endl;
    if(!outputPath.empty()) {J a=J::array();for(size_t i=0;i<count;++i)a.push_back({R[i][0],R[i][1],resCurrent[i].rgb_T[0],resCurrent[i].rgb_T[1],resCurrent[i].rgb_T[2],resCurrent[i].rgb_T[3]});std::ofstream f(outputPath+"-baseline-rows.json");f<<a;}
    if(currentDeltaRGB>1e-5||currentDeltaT>1e-5)throw std::runtime_error("baseline does not reproduce recorded native; timing refused");
    for(size_t i=count;i<executionRays;++i)for(int k=0;k<4;++k)
        if(resCurrent[i].rgb_T[k]!=resCurrent[i%count].rgb_T[k]||resUnfused[i].rgb_T[k]!=resUnfused[i%count].rgb_T[k])throw std::runtime_error("replicated output mismatch");
'''
 s=once(s,'    std::vector<double> timesUnfused,timesFused;',gpu+'\n    std::vector<double> timesUnfused,timesFused;')
 # Restrict replacement to the actual benchmark section, retain A* correctness cases.
 a=s.index('    std::vector<double> timesUnfused,timesFused;');b=s.index('    // Arithmetic fidelity comparison',a);part=s[a:b]
 part=part.replace('for(int n:{1,8,64})','for(int n: (repeatFactor==1?std::vector<int>{1,8,64}:std::vector<int>{1,2,4}))')
 part=part.replace('if(n==64)','if(n==(repeatFactor==1?64:4))')
 part=part.replace('runBatch(cpUnfused,bgUnfused,n)','runBatch(currentCP,currentBG,n)')
 part=part.replace('runBatch(cpFused,bgFused,n)','runBatch(cpUnfused,bgUnfused,n)')
 part=part.replace('unfused_batch_ms','current_batch_ms').replace('fused_batch_ms','A_star_batch_ms').replace('samples_unfused_ms','samples_current_ms').replace('samples_fused_ms','samples_A_star_ms')
 s=s[:a]+part+s[b:]
 extra=r'''
    report.erase("fusion_isolation_benchmark");
    report["timing_protocol"]="Completion-fenced host elapsed; normalization by the last configured batch (64 dispatches at repeat=1, 4 at repeat>1); no GPU timestamps, no readback in timed batches";
    report["scope"]="Generated production marcher versus complete unfused A* on identical captured rays, repeated for throughput; precomputed A* schedules";
    report["current_baseline_witness"]={{"stored_native_max_RGB_delta",currentDeltaRGB},{"stored_native_max_T_delta",currentDeltaT},
        {"integration_body_verbatim",true},{"zero_proofs",true},{"trim_support_lane",trim},{"GPU_time_preparation",true}};
    report["execution_workload"]={{"distinct_rays",count},{"repeat_factor",repeatFactor},{"invocations_per_dispatch",executionRays},
        {"replicated_output_verified",true},{"actual_full_viewport",false},{"boundary_preparation_in_timing",false}};
    report["current_vs_A_star"]={{"current_normalized_host_ms",meanUnfusedMs},{"A_star_normalized_host_ms",meanFusedMs},
        {"speedup_factor",meanUnfusedMs/meanFusedMs},{"clock","submission-to-blocking-poll host elapsed; normalized batched dispatches"}};
    WGPUAdapterInfo adapterInfo={};
    if(wgpuAdapterGetInfo(gpu.adapter,&adapterInfo)==WGPUStatus_Success) {
        report["observed_adapter"]={{"device",std::string(adapterInfo.device.data,adapterInfo.device.length)},
            {"backend",uint32_t(adapterInfo.backendType)},{"metal",adapterInfo.backendType==WGPUBackendType_Metal},{"timestamps",gpu.timestampQueries}};
        wgpuAdapterInfoFreeMembers(adapterInfo);
    }
    wgpuBindGroupRelease(currentBG);wgpuBindGroupRelease(prepBG);wgpuComputePipelineRelease(currentCP);wgpuComputePipelineRelease(prepCP);
    for(auto b:{cuBuf,cparamsBuf,ciBuf})wgpuBufferRelease(b);
'''
 s=once(s,'    std::cout << report.dump(2) << std::endl;',extra+'\n    std::cout << report.dump(2) << std::endl;')
 # Score the timed candidate itself, rather than inheriting the retired fused label.
 for channel in ['R','G','B','T']:
  old='double err'+channel+'_dev = std::abs(resFused[i].rgb_T['
  assert s.count(old)==1
  s=s.replace(old,old.replace('resFused','resUnfused'))
 s=s.replace('fused_A_star_on_device','unfused_A_star_on_device')

 return s

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--run-only',action='store_true');p.add_argument('--repeat',type=int,default=1);p.add_argument('--label',default='series1');a=p.parse_args()
 root=a.root.resolve();work=a.work.resolve();assert root!=work and root not in work.parents;assert 1<=a.repeat<=128
 spec=importlib.util.spec_from_file_location('overlap_repair',root/'scratch/probes/ray_transport_metal_overlap_repair.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 base=root/'scratch/probes/ray_transport_metal_diagnostic.py';tree=ast.parse(base.read_text());v={x.targets[0].id:ast.literal_eval(x.value) for x in tree.body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
 for key,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:assert hashlib.sha256((root/rel).read_bytes()).hexdigest()==v[key]
 cpp=source(m.repaired_source(v['CPP_SOURCE']))
 if not a.run_only:
  work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp);(work/'source.json').write_text(json.dumps({'original_script_sha256':hashlib.sha256(base.read_bytes()).hexdigest(),'overlap_driver_sha256':hashlib.sha256((root/'scratch/probes/ray_transport_metal_overlap_repair.py').read_bytes()).hexdigest(),'generated_CPP_sha256':hashlib.sha256(cpp.encode()).hexdigest()},indent=2))
  build=root/'build';db=json.loads((build/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(work/'probe.cpp');args[args.index('-o')+1]=str(work/'probe.o');args+=['-std=c++20','-O3'];subprocess.run(args,cwd=build,check=True)
  args=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');args[args.index('-o')+1]=str(work/'probe');subprocess.run(args,cwd=build,check=True)
 else:assert hashlib.sha256((work/'probe.cpp').read_bytes()).hexdigest()==hashlib.sha256(cpp.encode()).hexdigest()
 subprocess.run([str(work/'probe'),str(work/(a.label+'.json'))],cwd=root,check=True,env=dict(os.environ,EC_MATCH_REPEAT=str(a.repeat)))
if __name__=='__main__':main()
