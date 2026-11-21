"""Correct Sparkly's GPU diagnostic: bounded roots, explicit native fallback and safe buffers.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Still a pinned numerical experiment, not generic extraction or native omission authority.
"""
import argparse,ast,hashlib,importlib.util,json,os,shlex,subprocess
from pathlib import Path
from ray_transport_gpu_root_contract import WGSL

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def once(s,a,b):
    if s.count(a)!=1:raise ValueError(('hook count',s.count(a),a[:100]))
    return s.replace(a,b)

def source(base,root):
    parent=load(root,'ray_transport_gpu_producer_connected')
    s='#include <bit>\n'+parent.build_gpu_connected_source(base,root)
    old=parent.GPU_PRODUCER_HEADER
    header=old[:old.index('static const char* kGpuProducerWgsl')]
    header=header.replace('if (pb.values.size() < 27)', 'if (pb.values.size() < 34)')
    header=once(header,'        float wz = v[15];','        float wz = v[15];\n        if(!(Wx>0&&Wy>0&&wz>0&&v[5]>0&&std::isfinite(Wx)&&std::isfinite(Wy)&&std::isfinite(wz)))return false;')
    header=once(header,'spec.curv = glm::vec4(M2, 0.0f, 0.0f, 0.0f);',
                'spec.curv = glm::vec4(std::nextafter(M2,INFINITY)*1.00002f, std::getenv("EC_GPU_MARGIN")?std::stof(std::getenv("EC_GPU_MARGIN")):.001f,0,0);')
    header+='static const char* kGpuProducerWgsl=R"WGSL('+WGSL+')WGSL";\n'
    s=once(s,old,header)
    s=once(s,'size_t count = R.size();','size_t count = std::getenv("EC_GPU_GEOMETRY_CASE")?16:R.size();')
    # Erase the unused cold CPU boundary search; initialize only actual ray inputs.
    a=s.index('    for (size_t i = 0; i < count; ++i) {')
    b=s.index('    std::vector<GpuMediumParams> gpuMedia(media.size());',a)
    s=s[:a]+r'''
    std::vector<uint32_t> producerFlags(count,0);
    for(size_t i=0;i<count;++i){
        const auto& row=R[i];auto& ray=gpuRays[i];
        for(int k=0;k<3;++k){ray.ro[k]=float(ro[k]);ray.rd[k]=row[2+k];}
        ray.clipA=row[5];ray.clipB=row[6];
    }
'''+s[b:]
    s=once(s,'const uint32_t maxSubPerRay = 32;', 'const uint32_t maxSubPerRay = std::getenv("EC_GPU_SUB_CAP")?std::stoul(std::getenv("EC_GPU_SUB_CAP")):64;')
    s=once(s,'const uint32_t maxOverPerRay = 8;', 'const uint32_t maxOverPerRay = std::getenv("EC_GPU_OVER_CAP")?std::stoul(std::getenv("EC_GPU_OVER_CAP")):16;')
    s=once(s,'cfg{static_cast<uint32_t>(count), maxSubPerRay, maxOverPerRay, 0};',
                'cfg{static_cast<uint32_t>(count), maxSubPerRay, maxOverPerRay, std::getenv("EC_GPU_BUDGET")?uint32_t(std::stoul(std::getenv("EC_GPU_BUDGET"))):1024u};')
    geometry_setup=r'''
    const std::string geometryCase=std::getenv("EC_GPU_GEOMETRY_CASE")?std::getenv("EC_GPU_GEOMETRY_CASE"):"";
    if(!geometryCase.empty()){
        for(auto& spec:curtainSpecs){spec.origin=glm::vec4(1000,1000,1000,0);spec.half=glm::vec4(1);}
        auto& spec=curtainSpecs[0];spec.origin=glm::vec4(0);spec.half=glm::vec4(100);
        spec.amps=glm::vec4(0);spec.freqs=glm::vec4(0);spec.phases=glm::vec4(0);
        if(geometryCase=="near-parallel"){
            spec.limits={1,10,0,30};spec.curv={0,.001f,0,0};
            for(auto& ray:gpuRays){ray.ro[0]=1.000001f;ray.ro[1]=ray.ro[2]=0;ray.rd[0]=-1e-7f;ray.rd[1]=0;ray.rd[2]=1;ray.clipA=0;ray.clipB=20;}
        }else{
            spec.limits={2,2,0,geometryCase=="narrow-lobe"?5e-6f:.2f};
            spec.curv={geometryCase=="narrow-lobe"?10000.f:40000.f,.001f,0,0};
            if(geometryCase=="narrow-lobe"){spec.amps.y=1;spec.freqs.y=100;spec.phases.y=-.7f;}
            else {spec.amps.x=1;spec.freqs.x=200;}
            for(auto& ray:gpuRays){ray.ro[0]=ray.ro[1]=0;ray.ro[2]=geometryCase=="narrow-lobe"?-.99999f:0.f;
                ray.rd[0]=1;ray.rd[1]=ray.rd[2]=0;ray.clipA=0;ray.clipB=1;}
        }
        wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);
    }
'''
    s=once(s,'    auto cfgBuf = makeBuffer(',geometry_setup+'\n    auto cfgBuf = makeBuffer(')
    s=once(s,'WGPUBuffer outBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, outBytes, nullptr);',
               'WGPUBuffer outBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, outBytes, nullptr);')
    # Oversized padded GPU buffers must NEVER read beyond an unrelated host vector.
    s=s.replace('subpanelBytes, gpuSubpanels.data());','subpanelBytes, nullptr);')
    s=s.replace('overlapBytes > 0 ? overlapBytes : 16, gpuOverlaps.data());','overlapBytes > 0 ? overlapBytes : 16, nullptr);')
    s=once(s,'        while (!mc1.done || !mc2.done || !mc3.done) wgpuDevicePoll(gpu.device, true, nullptr);',
               '        while (!mc1.done || !mc2.done || !mc3.done) wgpuDevicePoll(gpu.device, true, nullptr);\n        if(!mc1.ok||!mc2.ok||!mc3.ok)throw std::runtime_error("producer readback mapping failed");')
    guard='let r = rays[rayIdx % arrayLength(&rays)];'
    assert s.count(guard)==2
    s=s.replace(guard,guard+'\n    if((r.counts.w & 0x80000000u)!=0u){return;}')
    s=once(s,'    auto runBatch=[&]', '    WGPUComputePipeline fallbackCP=nullptr;WGPUBindGroup fallbackBG=nullptr;\n    auto runBatch=[&]')
    old_dispatch='for(int k=0;k<n;++k)wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);'
    new_dispatch=r'''for(int k=0;k<n;++k){
            wgpuComputePassEncoderSetPipeline(pass,cp);
            if(bg)wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);
            if(fallbackCP&&(cp==cpUnfused||cp==cpFused)){
                wgpuComputePassEncoderSetPipeline(pass,fallbackCP);
                wgpuComputePassEncoderSetBindGroup(pass,0,fallbackBG,0,nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);
            }
        }'''
    s=once(s,old_dispatch,new_dispatch)
    fallback_pass=r'''
            auto pass3=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass3,fallbackCP);
            wgpuComputePassEncoderSetBindGroup(pass3,0,fallbackBG,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass3,(count+63)/64,1,1);
            wgpuComputePassEncoderEnd(pass3);wgpuComputePassEncoderRelease(pass3);
'''
    assert s.count('wgpuComputePassEncoderRelease(pass2);')==2
    s=s.replace('wgpuComputePassEncoderRelease(pass2);','wgpuComputePassEncoderRelease(pass2);'+fallback_pass)
    setup=r'''
    std::string fallbackShader=currentShader;
    const std::string rayHook=" let ray=capturedRays[idx%arrayLength(&capturedRays)];";
    auto where=fallbackShader.find(rayHook);if(where==std::string::npos)throw std::runtime_error("fallback ray hook");
    fallbackShader.insert(where+rayHook.size(),"\n if((ray.counts.w & 0x80000000u)==0u){return;}\n");
    fallbackCP=buildRaw(fallbackShader,"rayMain");fallbackBG=bindCurrent(fallbackCP);
'''
    s=once(s,'    auto currentBG=bindCurrent(currentCP),prepBG=bindCurrent(prepCP);',
               '    auto currentBG=bindCurrent(currentCP),prepBG=bindCurrent(prepCP);'+setup)
    # Restore candidate and fused outputs after the native fallback pipeline exists.
    repeat=r'''
    runPass(cpUnfused,bgUnfused);resUnfused=readResults();
    runPass(cpFused,bgFused);resFused=readResults();
'''
    s=once(s,'    std::vector<double> timesUnfused,timesFused;',repeat+'\n    std::vector<double> timesUnfused,timesFused;')
    s=once(s,'            uint32_t sCount = mappedRays[i].subpanelCount;',r'''
            producerFlags[i]=mappedRays[i].overlapCount;
            const bool refused=(producerFlags[i]&0x80000000u)!=0;
            uint32_t sCount = mappedRays[i].subpanelCount;
            if(refused&&sCount!=0)throw std::runtime_error("partial fallback schedule");
''')
    s=once(s,'            uint32_t oCount = mappedRays[i].overlapCount;',r'''
            uint32_t oCount = refused?0:mappedRays[i].overlapCount;
            if(sCount>maxSubPerRay||oCount>maxOverPerRay||sBase+sCount>totalSub||oBase+oCount>totalOver)
                throw std::runtime_error("producer output exceeds allocated bounds");
''')
    geometry_check=r'''
    if(!geometryCase.empty()){
        unsigned positiveChecks=0,fallbacks=0;
        const auto& spec=curtainSpecs[0];
        for(size_t i=0;i<count;++i){
            const auto& ray=gpuRays[i];bool refused=(producerFlags[i]&0x80000000u)!=0;fallbacks+=refused;
            auto check=[&](double t){
                const glm::dvec3 p=glm::dvec3(ray.ro[0],ray.ro[1],ray.ro[2])+t*glm::dvec3(ray.rd[0],ray.rd[1],ray.rd[2]);
                const double phi=p.z+double(spec.amps.x)*std::sin(double(spec.freqs.x)*p.x+spec.phases.x)+
                    double(spec.amps.y)*std::cos(double(spec.freqs.y)*p.x+spec.phases.y);
                if(std::abs(p.x)>=spec.limits.x||std::abs(p.y)>=spec.limits.y||std::abs(phi)>=spec.limits.w)return;
                ++positiveChecks;bool kept=refused;
                for(unsigned k=0;k<ray.subpanelCount;++k){const auto& q=gpuSubpanels[ray.subpanelOffset+k];kept=kept||(q.mediumIndex==0&&q.s0<=t&&q.s1>=t);}
                for(unsigned k=0;k<ray.overlapCount;++k){const auto& q=gpuOverlaps[ray.overlapOffset+k];kept=kept||((q.activeMask&1u)&&q.s0<=t&&q.s1>=t);}
                if(!kept)throw std::runtime_error("native geometry witness dropped positive support");
            };
            for(unsigned k=0;k<4096;++k)check(ray.clipA+(ray.clipB-ray.clipA)*(k+.5)/4096);
            if(geometryCase=="narrow-lobe"){check(.007-.00004);check(.007+.00004);}
        }
        if(!positiveChecks)throw std::runtime_error("geometry control never exercised positive support");
        if(geometryCase=="near-parallel"&&fallbacks==count)throw std::runtime_error("near-parallel unexpectedly all refused");
        J q={{"scope","Native producer geometry control only; no transport/accuracy/speed qualification"},
            {"case",geometryCase},{"rays",count},{"positive_support_checks",positiveChecks},{"fallback_rays",fallbacks},{"pass",true}};
        std::ofstream f(outputPath);f<<q.dump(2)<<"\n";std::cout<<q.dump(2)<<std::endl;return 0;
    }
'''
    s=once(s,'    // Helper for completion-fenced execution',geometry_check+'\n    // Helper for completion-fenced execution') if '    // Helper for completion-fenced execution' in s else once(s,'    WGPUComputePipeline fallbackCP=nullptr;',geometry_check+'\n    WGPUComputePipeline fallbackCP=nullptr;')
    s=once(s,'    long long cpuSingles=0,cpuOverlaps=0,cpuOverlapMemberSites=0;',
               '    long long cpuSingles=0,cpuOverlaps=0,cpuOverlapMemberSites=0,cpuBaselineFallbacks=0;')
    s=once(s,'    for(size_t rayIdx=0;rayIdx<count;++rayIdx) {',r'''
    for(size_t rayIdx=0;rayIdx<count;++rayIdx) {
        if((producerFlags[rayIdx]&0x80000000u)!=0){
            for(int k=0;k<4;++k)if(std::bit_cast<uint32_t>(resUnfused[rayIdx].rgb_T[k])!=std::bit_cast<uint32_t>(resCurrent[rayIdx].rgb_T[k]))throw std::runtime_error("fallback differs from native baseline");
            resCPU[rayIdx]=resCurrent[rayIdx];++cpuBaselineFallbacks;continue;
        }
''')
    # Poison output outside the measured batches: a skipped write cannot inherit
    # a coincidentally correct value from the preceding baseline dispatch.
    s=once(s,'        // Re-execute connected pass and verify output stability\n        connectedPass();',r'''
        // Re-execute connected pass and verify output stability
        std::vector<GpuResult> poisoned(count);
        for(auto& q:poisoned)for(float& v:q.rgb_T)v=NAN;
        wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poisoned.data(),outBytes);
        connectedPass();''')
    s=once(s,'        auto connectedRes = readResults();',
               '        auto connectedRes = readResults();\n        for(const auto& q:connectedRes)for(float v:q.rgb_T)if(!std::isfinite(v))throw std::runtime_error("connected output was not fully written");')
    # Exact chronology restored; no tolerance may forgive overlapping intervals.
    s=s.replace('p.s0<last-1e-5f','p.s0<last').replace('o.s0<last-1e-5f','o.s0<last')
    meta=r'''
    J reasons=J::object();unsigned fallbackRays=0;
    for(auto flag:producerFlags)if(flag&0x80000000u){++fallbackRays;auto key=std::to_string(flag&0x7fffffffu);reasons[key]=reasons.value(key,0u)+1;}
    report["scope"]="Pinned GPU numerical producer; typed whole-ray native fallback; no generic derivation or certified native omission admission";
    report["gpu_canonical_producer"]={{"GPU_producer",true},{"fixed_layout_parameter_decode",true},{"generic_OntoMath_extraction",false},
        {"native_omission_admitted",false},{"diagnostic_margin",curtainSpecs[0].curv.y},{"fallback_rays",fallbackRays},{"fallback_reasons",reasons},
        {"zero_per_run_host_schedule_generation",true},{"schedule_initial_upload_bytes",0},{"padded_capacity_bytes",subpanelBytes+overlapBytes+rayBytes}};
    report["execution_workload"]["boundary_preparation_in_timing"]=true;
    report["execution_workload"]["boundary_preparation_scope"]="GPU per-ray instantiation in connected batches; cold host decoding/compilation/reference work excluded";
    report["current_vs_A_star"]["scope"]="Consumer plus native fallback only, excludes GPU producer; use batched_fence_series for connected costs";
    auto& connectedReport=report["connected_measured_scope"];
    connectedReport["connected_submission_to_poll_host_ms"]=connectedReport["connected_gpu_ms"];
    connectedReport.erase("connected_gpu_ms");
    connectedReport["producer_only_submission_to_poll_host_ms"]=connectedReport["producer_only_gpu_ms"];
    connectedReport.erase("producer_only_gpu_ms");
    connectedReport["clock"]="Submission-to-blocking-poll host elapsed, not a GPU timestamp or fence-floor-subtracted estimate";
    report["qualification"]["shipping_performance_qualified"]=false;
    report["qualification"]["fallback_native_baseline_identical"]=true;
    report["qualification"]["poisoned_connected_output_fully_written"]=true;
    report["ordered_coverage"]["native_baseline_fallback_rays"]=cpuBaselineFallbacks;
    report["cpu_vs_gpu_arithmetic_deviation"]["note"]="Independent AST quadrature only on canonical rays; fallback rays verified bit-identical to actual production-generated native baseline";
    wgpuBindGroupRelease(fallbackBG);wgpuComputePipelineRelease(fallbackCP);
'''
    s=once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')
    oldret='return witnessesPass && accuracyPass && maxDeltaRGB_CPU_vs_GPU<1e-5 && maxDeltaT_CPU_vs_GPU<1e-5 ? 0 : 3;'
    s=once(s,oldret,'return witnessesPass && accuracyPass && maxDeltaRGB_CPU_vs_GPU<1e-5 && maxDeltaT_CPU_vs_GPU<1e-5 && maxDeltaRGB_dev==0 && maxDeltaT_dev==0 ? 0 : 3;')
    return s

QUALIFICATION_KEYS=("closed_form_witnesses_pass","mean_p99_max_reference_gates_pass",
    "independent_AST_agreement_pass","device_fused_unfused_parity_pass",
    "fallback_native_baseline_identical","poisoned_connected_output_fully_written")

def validate_report(d,geometry=False):
    if geometry:
        if d.get('pass') is not True:raise ValueError('geometry control failed')
        return
    q=d.get('qualification',{})
    for key in QUALIFICATION_KEYS:
        if q.get(key) is not True:raise ValueError('qualification failed: '+key)
    if q.get('shipping_performance_qualified') is not False:
        raise ValueError('scratch result must not claim shipping qualification')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
    p.add_argument('--run-only',action='store_true');p.add_argument('--label',default='normal')
    p.add_argument('--budget',type=int,default=1024);p.add_argument('--sub-cap',type=int,default=64);p.add_argument('--over-cap',type=int,default=16)
    p.add_argument('--margin',type=float,default=.001)
    p.add_argument('--geometry-case',choices=['near-parallel','narrow-lobe','many-roots'])
    p.add_argument('--fallback-mode',choices=['indirect','persistent'],default='indirect',help='Compact-queue experimental dispatch mode')
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if root==work or root in work.parents:raise SystemExit('external work directory required')
    if Path(a.label).name!=a.label or not a.label:raise SystemExit('unsafe label')
    if not 0<=a.budget<=65536 or not 1<=a.sub_cap<=128 or not 1<=a.over_cap<=64 or not 0<=a.margin<=1:raise SystemExit('invalid diagnostic bounds')
    base=root/'scratch/probes/ray_transport_metal_diagnostic.py'
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse(base.read_text()).body
       if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    hashes={}
    for key,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:
        h=hashlib.sha256((root/rel).read_bytes()).hexdigest()
        if h!=v[key]:raise SystemExit('Pinned fixture changed: '+rel)
        hashes[rel]=h
    cpp=source(v['CPP_SOURCE'],root);digest=hashlib.sha256(cpp.encode()).hexdigest()
    output=work/(a.label+'.json')
    if output.exists():raise SystemExit('refusing output overwrite')
    if not a.run_only:
        work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp)
        (work/'source.json').write_text(json.dumps({'cpp_sha256':digest,'fixtures':hashes},indent=2)+'\n')
        build=root/'build';db=json.loads((build/'compile_commands.json').read_text())
        c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
        cmd=shlex.split(c['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3'];subprocess.run(cmd,cwd=build,check=True)
        cmd=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');subprocess.run(cmd,cwd=build,check=True)
    elif hashlib.sha256((work/'probe.cpp').read_bytes()).hexdigest()!=digest:raise SystemExit('source mismatch')
    for rel,h in hashes.items():
        if hashlib.sha256((root/rel).read_bytes()).hexdigest()!=h:raise SystemExit('fixture changed before execution')
    env=dict(os.environ,EC_MATCH_REPEAT='1',EC_GPU_BUDGET=str(a.budget),EC_GPU_SUB_CAP=str(a.sub_cap),EC_GPU_OVER_CAP=str(a.over_cap),EC_GPU_MARGIN=str(a.margin))
    env.pop('EC_GPU_GEOMETRY_CASE',None)
    if a.geometry_case:env['EC_GPU_GEOMETRY_CASE']=a.geometry_case
    env['EC_GPU_FALLBACK_MODE']=a.fallback_mode
    subprocess.run([str(work/'probe'),str(output)],cwd=root,check=True,env=env)
    for rel,h in hashes.items():
        if hashlib.sha256((root/rel).read_bytes()).hexdigest()!=h:raise SystemExit('fixture changed during execution')
    d=json.loads(output.read_text());validate_report(d,bool(a.geometry_case))
    if a.geometry_case:
        print('Passed native geometry control:',output);return
    print('Qualified only for stated pinned numerical scope:',output)

if __name__=='__main__':main()
