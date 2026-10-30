"""Repair the frozen Lane-A diagnostic: ordered overlap flow, independent AST
comparison, and batched submission/fence timing. Scratch only, not production.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import argparse, ast, hashlib, json, shlex, subprocess
from pathlib import Path

def replace_once(s,a,b):
    assert s.count(a)==1, ('hook count',s.count(a),a[:100])
    return s.replace(a,b)

def repaired_source(s):
    s=replace_once(s,'#include <numeric>','#include <numeric>\n#include <stdexcept>')
    # Shared flow primitive: sigma/emission at the nodes remain independent.
    helper=r'''
fn phiSafe(tau: f32) -> f32 {
    if (abs(tau) < 0.01) {
        return 1.0 + tau * (-0.5 + tau * (1.0/6.0 - tau/24.0));
    }
    return (1.0 - exp(-tau)) / tau;
}
fn projectedAt(ro: vec3<f32>, rd: vec3<f32>, s: f32, mask: u32) -> vec4<f32> {
    var result=vec4<f32>(0.0);
    for (var member=0u; member<arrayLength(&media); member+=1u) {
        if ((mask & (1u<<member))==0u) {continue;}
        let m=media[member];
        let local=ro+s*rd-m.origin.xyz;
        let env=evalEnvelope(local,u32(m.freq_offset.y));
        if (env<=0.0) {continue;}
        let D=env*clamp(0.7+0.3*cnoise3(m.freq_offset.x*local),0.0,1.0);
        if (D<=0.0) {continue;}
        let raw=m.color_alpha.w*D;
        result+=vec4<f32>(m.color_alpha.xyz*raw,max(raw,1e-6));
    }
    return result;
}
fn advanceFlow(state: vec4<f32>, sourceSigma: vec4<f32>, h: f32) -> vec4<f32> {
    let tau=h*sourceSigma.w;
    return vec4<f32>(state.xyz+state.w*sourceSigma.xyz*(h*phiSafe(tau)),state.w*exp(-tau));
}
fn advanceOverlap(stateIn: vec4<f32>, ro: vec3<f32>, rd: vec3<f32>, o: OverlapInterval) -> vec4<f32> {
    var state=stateIn;
    let h=(o.span.y-o.span.x)*0.5;
    let a=0.25-sqrt(3.0)/6.0;
    let b=0.5-a;
    for (var panel=0u; panel<2u; panel+=1u) {
        let lo=o.span.x+f32(panel)*h;
        let minus=projectedAt(ro,rd,lo+(0.5-sqrt(3.0)/6.0)*h,o.info.x);
        let plus=projectedAt(ro,rd,lo+(0.5+sqrt(3.0)/6.0)*h,o.info.x);
        state=advanceFlow(state,b*minus+a*plus,h);
        state=advanceFlow(state,a*minus+b*plus,h);
    }
    return state;
}
'''
    s=replace_once(s,'// Pipeline 1: Unfused A* Shader', 'static const char* kOverlapWgsl=R"WGSL('+helper+')WGSL";\n\n// Pipeline 1: Unfused A* Shader')
    s=replace_once(s,'std::string(kCommonWgslHeader) + "\\n" + shaderCode','std::string(kCommonWgslHeader) + "\\n" + kOverlapWgsl + "\\n" + shaderCode')
    dummy='''    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3<f32>(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i = i + 1u) {
        let sub = subpanels[subOffset + i];'''
    merged='''    // Two sorted streams must be consumed in ray order, including overlap.
    var single=0u;
    var overlap=0u;
    loop {
        if (single>=subCount && overlap>=overCount) {break;}
        var useOverlap=false;
        if (overlap<overCount) {
            if (single>=subCount) {useOverlap=true;}
            else {useOverlap=overlaps[overOffset+overlap].span.x<subpanels[subOffset+single].span.x;}
        }
        if (useOverlap) {
            let state=advanceOverlap(vec4<f32>(L,T),ro,rd,overlaps[overOffset+overlap]);
            L=state.xyz; T=state.w;
            overlap+=1u;
            continue;
        }
        let sub = subpanels[subOffset + single];
        single+=1u;'''
    assert s.count(dummy)==2
    s=s.replace(dummy,merged)
    gain='''        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }'''
    assert s.count(gain)==2
    s=s.replace(gain,'        gain=sub_h*phiSafe(tau);')
    # CPU comparator uses original authored AST, not the manually copied envelope.
    begin=s.index('    // Direct CPU f32 Quadrature on identical schedule')
    end=s.index('    double maxDeltaRGB_CPU_vs_GPU',begin)
    cpu=r'''
    // Independently evaluate the original authored AST at native f32 coordinates.
    std::vector<GpuResult> resCPU(count);
    long long cpuSingles=0,cpuOverlaps=0,cpuOverlapMemberSites=0;
    auto astAt=[&](glm::vec3 ro,glm::vec3 rd,float t,uint32_t mask)->glm::vec4 {
        glm::vec4 value(0.0f); const glm::vec3 world=ro+t*rd;
        for(size_t id=0;id<media.size();++id) {
            if(!(mask&(1u<<id)))continue;
            const auto& m=media[id]; const glm::vec3 p=world-glm::vec3(m.origin);
            if(glm::any(glm::greaterThan(glm::abs(glm::dvec3(p)),m.half)))continue;
            const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},
                {"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},
                {OntoMath::kTimeVar,PropertyValue(clock)},
                {OntoMath::kOmegaXVar,PropertyValue(double(-rd.x))},
                {OntoMath::kOmegaYVar,PropertyValue(double(-rd.y))},
                {OntoMath::kOmegaZVar,PropertyValue(double(-rd.z))}};
            double D=0,X=0;
            auto v=m.density->evaluate(vars);
            if(!v||!propertyValueToNumber(*v,D))throw std::runtime_error("AST density refusal");
            if(!(D>0))continue;
            v=m.sigma->evaluate(vars);
            if(!v||!propertyValueToNumber(*v,X))throw std::runtime_error("AST extinction refusal");
            v=m.emission->evaluate(vars);
            if(!v||!std::holds_alternative<glm::vec3>(*v))throw std::runtime_error("AST emission refusal");
            const auto E=glm::max(std::get<glm::vec3>(*v),glm::vec3(0));
            value+=glm::vec4(E,float(std::max(X,1e-6)));
        }
        return value;
    };
    auto cpuFlow=[](glm::vec4 state,glm::vec4 q,float h) {
        const float tau=h*q.w,att=std::exp(-tau);
        const float gain=tau==0?h:h*(-std::expm1(-tau)/tau);
        return glm::vec4(glm::vec3(state)+state.w*glm::vec3(q)*gain,state.w*att);
    };
    auto tCpu0=std::chrono::steady_clock::now();
    for(size_t rayIdx=0;rayIdx<count;++rayIdx) {
        const auto& r=gpuRays[rayIdx]; const glm::vec3 ro(r.ro[0],r.ro[1],r.ro[2]),rd(r.rd[0],r.rd[1],r.rd[2]);
        glm::vec4 state(0,0,0,1);uint32_t si=0,oi=0;float last=-INFINITY;
        while(si<r.subpanelCount||oi<r.overlapCount) {
            const bool overlap=oi<r.overlapCount && (si>=r.subpanelCount || gpuOverlaps[r.overlapOffset+oi].s0<gpuSubpanels[r.subpanelOffset+si].s0);
            if(overlap) {
                const auto& o=gpuOverlaps[r.overlapOffset+oi++];
                if(o.s0<last)throw std::runtime_error("overlap schedule is not chronological");last=o.s1;++cpuOverlaps;
                const float h=(o.s1-o.s0)*.5f,a=.25f-std::sqrt(3.f)/6.f,b=.5f-a;
                for(int k=0;k<2;++k) {
                    const float lo=o.s0+k*h;
                    const auto u=astAt(ro,rd,lo+(.5f-std::sqrt(3.f)/6.f)*h,o.activeMask),v=astAt(ro,rd,lo+(.5f+std::sqrt(3.f)/6.f)*h,o.activeMask);
                    state=cpuFlow(state,b*u+a*v,h);state=cpuFlow(state,a*u+b*v,h);
                    cpuOverlapMemberSites+=2*__builtin_popcount(o.activeMask);
                }
            } else {
                const auto& p=gpuSubpanels[r.subpanelOffset+si++];
                if(p.s0<last)throw std::runtime_error("single schedule is not chronological");last=p.s1;++cpuSingles;
                const float h=p.s1-p.s0;
                const auto u=astAt(ro,rd,p.s0+(.5f-std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex),v=astAt(ro,rd,p.s0+(.5f+std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex);
                state=cpuFlow(state,.5f*(u+v),h);
            }
        }
        resCPU[rayIdx]=GpuResult{state.x,state.y,state.z,state.w};
    }
    if(size_t(cpuSingles)!=gpuSubpanels.size()||size_t(cpuOverlaps)!=gpuOverlaps.size())throw std::runtime_error("schedule coverage mismatch");
    auto tCpu1=std::chrono::steady_clock::now();
    double cpuQuadratureMs=std::chrono::duration<double,std::milli>(tCpu1-tCpu0).count();
'''
    s=s[:begin]+cpu+s[end:]
    # Optional throughput batches expose the single-dispatch fence floor.
    a=s.index('    // Helper for completion-fenced execution')
    b=s.index('    // Helper for readback',a)
    s=s[:a]+r'''
    auto runBatch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,int n)->double {
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
        auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
        wgpuComputePassEncoderSetPipeline(pass,cp);
        if(bg)wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
        for(int k=0;k<n;++k)wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);
        wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);
        auto t0=std::chrono::steady_clock::now();wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device,true,nullptr);
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    };
    auto runPass=[&](WGPUComputePipeline cp,WGPUBindGroup bg){return runBatch(cp,bg,1);};
''' +s[b:]
    a=s.index('    // Isolated completion-fenced ABBA benchmark')
    b=s.index('    // Arithmetic fidelity comparison',a)
    s=s[:a]+r'''
    std::vector<double> timesUnfused,timesFused;
    J batchSeries=J::array();
    auto cpEmpty=buildPipeline("@compute @workgroup_size(64) fn rayMain() {}\n");
    if(!cpEmpty)throw std::runtime_error("empty pipeline failed");
    for(int n:{1,8,64}) {
        for(int k=0;k<4;++k){runBatch(cpUnfused,bgUnfused,n);runBatch(cpFused,bgFused,n);}
        std::vector<double> A,B,E;
        for(int block=0;block<6;++block) {
            A.push_back(runBatch(cpUnfused,bgUnfused,n));B.push_back(runBatch(cpFused,bgFused,n));
            B.push_back(runBatch(cpFused,bgFused,n));A.push_back(runBatch(cpUnfused,bgUnfused,n));
            E.push_back(runBatch(cpEmpty,nullptr,n));
        }
        if(n==64){for(double x:A)timesUnfused.push_back(x/n);for(double x:B)timesFused.push_back(x/n);}
        batchSeries.push_back({{"dispatches_per_fence",n},{"unfused_batch_ms",stats(A)},{"fused_batch_ms",stats(B)},
            {"empty_batch_ms",stats(E)},{"samples_unfused_ms",A},{"samples_fused_ms",B},{"samples_empty_ms",E}});
        std::cout<<"BATCH "<<n<<" A="<<stats(A)["mean"]<<" B="<<stats(B)["mean"]<<" empty="<<stats(E)["mean"]<<std::endl;
    }
    wgpuComputePipelineRelease(cpEmpty);
''' +s[b:]
    s=replace_once(s,'    std::cout << report.dump(2) << std::endl;',r'''

    // Independent closed-form witnesses. These change only scratch GPU buffers,
    // after all fixture readback and timing; the world/save never changes.
    auto syntheticMedia=gpuMedia;auto syntheticParams=globalParamPool;
    const float rawTargets[4]={0.0f,0.5e-6f,0.2f,0.7f};
    const glm::vec3 witnessColors[4]={{.2f,.4f,.8f},{.2f,.4f,.8f},{.8f,.2f,.1f},{.1f,.3f,.9f}};
    for(size_t m=0;m<4;++m) {
        const size_t at=media[m].paramOffset;
        std::fill(syntheticParams.begin()+at,syntheticParams.begin()+at+34,0.0f);
        auto* v=syntheticParams.data()+at;
        v[0]=rawTargets[m]/.7f;v[1]=1;v[2]=1;v[4]=1;
        v[5]=1;v[6]=1;v[8]=1;v[9]=1;v[10]=1;v[13]=1;
        v[14]=1;v[15]=1;v[26]=1;v[27]=1;
        for(int k=0;k<4;++k)syntheticMedia[m].origin[k]=0;
        for(int k=0;k<3;++k)syntheticMedia[m].color[k]=witnessColors[m][k];
        syntheticMedia[m].color[3]=1;syntheticMedia[m].freq[0]=0;
    }
    std::vector<GpuSubpanel> ws{{0,.01f,0,0},{0,.01f,1,0},{0,1,2,0},{0,1,2,0},{2,3,3,0}};
    std::vector<GpuOverlap> wo{{0,1,12u,0},{1,2,12u,0}};
    std::vector<GpuRayHeader> wr(5);
    for(size_t i=0;i<5;++i)wr[i].clipB=3;
    wr[0].subpanelOffset=0;wr[0].subpanelCount=1;
    wr[1].subpanelOffset=1;wr[1].subpanelCount=1;
    wr[2].subpanelOffset=2;wr[2].subpanelCount=1;
    wr[3].overlapOffset=0;wr[3].overlapCount=1;
    wr[4].subpanelOffset=3;wr[4].subpanelCount=2;wr[4].overlapOffset=1;wr[4].overlapCount=1;
    auto knownFlow=[&](glm::dvec4 state,uint32_t mask,double h) {
        double sigma=0;glm::dvec3 emit(0);
        for(int m=0;m<4;++m)if(mask&(1u<<m)) {
            const double raw=double(syntheticParams[media[m].paramOffset]*.7f);
            if(!(raw>0))continue;
            sigma+=std::max(raw,1e-6);emit+=glm::dvec3(witnessColors[m])*raw;
        }
        const double tau=h*sigma,gain=tau==0?h:h*(-std::expm1(-tau)/tau);
        return glm::dvec4(glm::dvec3(state)+state.w*emit*gain,state.w*std::exp(-tau));
    };
    const glm::dvec4 initial(0,0,0,1);
    std::vector<glm::dvec4> expected{knownFlow(initial,1u,.01f),knownFlow(initial,2u,.01f),knownFlow(initial,4u,1),knownFlow(initial,12u,1),
        knownFlow(knownFlow(knownFlow(initial,4u,1),12u,1),8u,1)};
    wgpuQueueWriteBuffer(gpu.queue,mBuf,0,syntheticMedia.data(),mediaBytes);
    wgpuQueueWriteBuffer(gpu.queue,pBuf,0,syntheticParams.data(),paramBytes);
    wgpuQueueWriteBuffer(gpu.queue,rBuf,0,wr.data(),wr.size()*sizeof(GpuRayHeader));
    wgpuQueueWriteBuffer(gpu.queue,sBuf,0,ws.data(),ws.size()*sizeof(GpuSubpanel));
    wgpuQueueWriteBuffer(gpu.queue,oBuf,0,wo.data(),wo.size()*sizeof(GpuOverlap));
    auto wu=udata;wu.rayCount=5;wgpuQueueWriteBuffer(gpu.queue,uBuf,0,&wu,sizeof(wu));
    runPass(cpUnfused,bgUnfused);auto witnessA=readResults();
    runPass(cpFused,bgFused);auto witnessB=readResults();
    const char* names[]={"zero density","tiny positive floored depth","ordinary single medium","pure different-color overlap","single-overlap-single chronological flow"};
    J witnesses=J::array();bool witnessesPass=true;
    for(size_t i=0;i<5;++i) {
        bool pass=true;double maxRGB=0,maxT=0;
        for(auto* actual:{&witnessA,&witnessB})for(int k=0;k<4;++k) {
            const double err=std::abs(double((*actual)[i].rgb_T[k])-expected[i][k]);
            const double tolerance=k==3?2e-6:3e-6*std::max(std::abs(expected[i][k]),1e-12);
            pass=pass&&err<=tolerance;
            if(k==3)maxT=std::max(maxT,err);else maxRGB=std::max(maxRGB,err);
        }
        witnessesPass=witnessesPass&&pass;
        witnesses.push_back({{"case",names[i]},{"pass",pass},{"max_RGB_delta",maxRGB},{"max_T_delta",maxT},
            {"expected",{expected[i].x,expected[i].y,expected[i].z,expected[i].w}},
            {"unfused",{witnessA[i].rgb_T[0],witnessA[i].rgb_T[1],witnessA[i].rgb_T[2],witnessA[i].rgb_T[3]}}});
    }
    report["closed_form_witnesses"]=witnesses;
    bool accuracyPass=true;
    for(const char* channel:{"RGB_pooled_channels","T"})for(const char* metric:{"mean","p99","max"})
        accuracyPass=accuracyPass&&report["accuracy_vs_65536_step_reference"]["fused_A_star_on_device"][channel][metric].get<double>()<=report["accuracy_vs_65536_step_reference"]["native_baseline"][channel][metric].get<double>();
    report["qualification"]={{"closed_form_witnesses_pass",witnessesPass},{"mean_p99_max_reference_gates_pass",accuracyPass},
        {"independent_AST_agreement_pass",maxDeltaRGB_CPU_vs_GPU<1e-5&&maxDeltaT_CPU_vs_GPU<1e-5},
        {"shipping_performance_qualified",false}};
    report["scope"]="Scratch complete ordered single+overlap A* diagnostic; sparse rays, not a renderer speed claim";
    report["timing_protocol"]="Completion-fenced host elapsed, per-dispatch normalization from 64-dispatch batches; no GPU timestamps, no readback in timed batches";
    report["batched_fence_series"]=batchSeries;
    report["ordered_coverage"]={{"cpu_single_subpanels",cpuSingles},{"cpu_overlap_intervals",cpuOverlaps},{"cpu_overlap_member_sites",cpuOverlapMemberSites},
        {"independent_comparator","original authored MathNode density/extinction/emission; no copied-envelope oracle"}};
    report["cpu_vs_gpu_arithmetic_deviation"]["note"]="Independent AST; CPU double scalar operations and vec3/f32 properties differ from shader execution";
    J raw=J::array();
    for(size_t i=0;i<count;++i)raw.push_back({R[i][0],R[i][1],resUnfused[i].rgb_T[0],resUnfused[i].rgb_T[1],resUnfused[i].rgb_T[2],resUnfused[i].rgb_T[3],resFused[i].rgb_T[0],resFused[i].rgb_T[1],resFused[i].rgb_T[2],resFused[i].rgb_T[3],resCPU[i].rgb_T[0],resCPU[i].rgb_T[1],resCPU[i].rgb_T[2],resCPU[i].rgb_T[3],gpuRays[i].overlapCount});
    if(!outputPath.empty()){std::ofstream out(outputPath+"-rows.json");out<<raw.dump()<<"\n";}
    std::cout << report.dump(2) << std::endl;''')
    s=replace_once(s,"    return 0;\n}\n", "    return witnessesPass && accuracyPass && maxDeltaRGB_CPU_vs_GPU<1e-5 && maxDeltaT_CPU_vs_GPU<1e-5 ? 0 : 3;\n}\n")
    return s

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--run-only',action='store_true');p.add_argument('--label',default='series1');a=p.parse_args()
    root=a.root.resolve();work=a.work.resolve();assert root!=work and root not in work.parents
    source=root/'scratch/probes/ray_transport_metal_diagnostic.py'
    module=ast.parse(source.read_text());v={x.targets[0].id:ast.literal_eval(x.value) for x in module.body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    for key,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:assert hashlib.sha256((root/rel).read_bytes()).hexdigest()==v[key]
    if not a.run_only:
        work.mkdir(parents=True,exist_ok=False);cpp=repaired_source(v['CPP_SOURCE']);(work/'probe.cpp').write_text(cpp)
        (work/'source.json').write_text(json.dumps({'base_script_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'generated_cpp_sha256':hashlib.sha256(cpp.encode()).hexdigest()},indent=2))
        build=root/'build';db=json.loads((build/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(work/'probe.cpp');args[args.index('-o')+1]=str(work/'probe.o');args+=['-std=c++20','-O3'];subprocess.run(args,cwd=build,check=True)
        args=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');args[args.index('-o')+1]=str(work/'probe');subprocess.run(args,cwd=build,check=True)
    subprocess.run([str(work/'probe'),str(work/(a.label+'.json'))],cwd=root,check=True)

if __name__=='__main__':main()
