"""Pinned-fixture canonical producer correction; matched native diagnostics only.

Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Retains Sparkly's fixture experiment, not generic OntoMath/GPU producer admission.
Reuses the qualified ordered-overlap and production-marcher comparison harnesses.
"""
import argparse
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess
from ray_transport_canonical_root_contract import CPP_ROOT_CONTRACT, CPP_FIXTURE


def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def once(s,a,b):
    if s.count(a)!=1:raise ValueError(('hook count',s.count(a),a[:80]))
    return s.replace(a,b)


def build_canonical_probe_source(base,root):
    repaired=load(root,'ray_transport_metal_overlap_repair').repaired_source(base)
    # Preserve the previously qualified generic host schedule as a whole-ray
    # diagnostic fallback. No partially constructed canonical schedule survives.
    start=repaired.index('    for (size_t i = 0; i < count; ++i) {')
    end=repaired.index('    std::vector<GpuMediumParams> gpuMedia(media.size());',start)
    legacy=repaired[start:end]
    body=legacy[legacy.index('{')+1:legacy.rfind('}')]
    body=body.replace('if (!m.hasEnv) continue;','if (!m.hasEnv) throw std::runtime_error("fallback has no admitted envelope");')
    body=body.replace('if (!ev.ok()) continue;','if (!ev.ok()) throw std::runtime_error("fallback evaluator refused");')
    # The original box loop also used a near-zero threshold. Only exact zero
    # is constant, including in the fallback path.
    body=body.replace('std::abs(d[k]) < 1e-15','d[k] == 0.0')
    producer=r'''
    auto legacyRay=[&](size_t i){
LEGACY_BODY
    };
    unsigned canonicalFallbackRays=0,canonicalUnresolved=0,canonicalRootBrackets=0;
    auto prepareSchedules=[&](){
        gpuSubpanels.clear();gpuOverlaps.clear();maxSubpanelsPerRay=0;
        canonicalFallbackRays=0;canonicalUnresolved=0;canonicalRootBrackets=0;
        canonical_fixture::Limits limits;
        if(const char* x=std::getenv("EC_CANONICAL_NODE_BUDGET"))limits.maxNodes=std::stoul(x);
        for(size_t i=0;i<count;++i){
            const auto& r=R[i];
            const std::array<double,3> origin{ro.x,ro.y,ro.z},d{r[2],r[3],r[4]};
            auto prepared=canonical_fixture::prepare(origin,d,r[5],r[6],limits);
            canonicalUnresolved+=prepared.unresolved;canonicalRootBrackets+=prepared.roots;
            if(prepared.fallback){++canonicalFallbackRays;legacyRay(i);continue;}
            // Keep all bracket slivers. Outward conversion prevents a narrow
            // retained interval disappearing merely because both ends round
            // to the same f32. Exact event deduplication only.
            auto floatLow=[](double x){float f=float(x);return double(f)>x?std::nextafter(f,-INFINITY):f;};
            auto floatHigh=[](double x){float f=float(x);return double(f)<x?std::nextafter(f,INFINITY):f;};
            auto pieces=prepared.pieces;
            std::vector<double> events;
            for(auto& p:pieces){p.lo=floatLow(p.lo);p.hi=floatHigh(p.hi);events.push_back(p.lo);events.push_back(p.hi);}
            std::sort(events.begin(),events.end());events.erase(std::unique(events.begin(),events.end()),events.end());
            const uint32_t ss=gpuSubpanels.size(),os=gpuOverlaps.size();
            for(size_t e=1;e<events.size();++e){
                const double a=events[e-1],b=events[e];if(!(b>a))continue;
                const double mid=a+(b-a)*.5;uint32_t mask=0;
                for(const auto& p:pieces)if(p.lo<=mid&&mid<=p.hi)mask|=1u<<p.medium;
                if(!mask)continue;
                if(__builtin_popcount(mask)==1){
                    const unsigned id=__builtin_ctz(mask),n=std::max(1,int(std::ceil((b-a)/4.)));
                    for(unsigned k=0;k<n;++k){
                        float lo=float(a+(b-a)*double(k)/n),hi=float(a+(b-a)*double(k+1)/n);
                        if(!(hi>lo))throw std::runtime_error("nonrepresentable packet interval");
                        gpuSubpanels.push_back(GpuSubpanel{lo,hi,id,0});
                    }
                }else gpuOverlaps.push_back(GpuOverlap{float(a),float(b),mask,0});
            }
            auto& gr=gpuRays[i];
            for(int k=0;k<3;++k){gr.ro[k]=float(origin[k]);gr.rd[k]=float(d[k]);}
            gr.clipA=r[5];gr.clipB=r[6];gr.subpanelOffset=ss;gr.subpanelCount=gpuSubpanels.size()-ss;
            gr.overlapOffset=os;gr.overlapCount=gpuOverlaps.size()-os;
            maxSubpanelsPerRay=std::max(maxSubpanelsPerRay,gr.subpanelCount);
        }
    };
    auto producerStart=std::chrono::steady_clock::now();prepareSchedules();
    const double producerTotalMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-producerStart).count();
'''.replace('LEGACY_BODY',body)
    repaired=repaired[:start]+producer+repaired[end:]
    repaired=once(repaired,'using J = nlohmann::json;',CPP_ROOT_CONTRACT+CPP_FIXTURE+'\nusing J = nlohmann::json;')
    result=load(root,'ray_transport_native_march_comparison').source(repaired)
    # Actual measured cold-to-warm scope: stable scene/evaluators/buffers are
    # resident, each candidate sample rebuilds concrete host schedules and
    # uploads them before the dispatch/fence. Current uses identical base rays.
    connected=r'''
    J connected;
    if(repeatFactor==1&&!std::getenv("EC_CANONICAL_SKIP_CONNECTED")){
        auto connectedPass=[&](){
            const auto begin=std::chrono::steady_clock::now();prepareSchedules();
            if(gpuSubpanels.size()*sizeof(GpuSubpanel)!=subpanelBytes||gpuOverlaps.size()*sizeof(GpuOverlap)!=overlapBytes)
                throw std::runtime_error("fixed-fixture payload changed capacity");
            wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);
            if(subpanelBytes)wgpuQueueWriteBuffer(gpu.queue,sBuf,0,gpuSubpanels.data(),subpanelBytes);
            if(overlapBytes)wgpuQueueWriteBuffer(gpu.queue,oBuf,0,gpuOverlaps.data(),overlapBytes);
            runPass(cpUnfused,bgUnfused);
            return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        };
        connectedPass();runPass(currentCP,currentBG);
        std::vector<double> c,a;
        for(int block=0;block<4;++block){
            c.push_back(runPass(currentCP,currentBG));a.push_back(connectedPass());
            a.push_back(connectedPass());c.push_back(runPass(currentCP,currentBG));
        }
        connected={{"scope","Same 2304 frozen rays; warmed stable scene/evaluators/buffers; host producer+schedule upload+consumer+fence versus resident production consumer"},
            {"current_host_ms",stats(c)},{"candidate_host_ms",stats(a)},{"samples_current_ms",c},{"samples_candidate_ms",a},
            {"ratio_current_over_candidate",stats(c)["mean"].get<double>()/stats(a)["mean"].get<double>()},
            {"actual_full_viewport",false},{"cold_compilation_and_scene_preparation_included",false}};
        auto again=readResults();
        for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
            if(again[i].rgb_T[k]!=resUnfused[i].rgb_T[k])throw std::runtime_error("connected output changed");
    }else connected={{"measured",false},{"reason","explicitly skipped or repeated throughput work reuses schedules; no connected claim"}};
'''
    # The last operation above is current; explicitly rerun candidate before
    # checking connected output, outside the timed samples.
    connected=connected.replace('auto again=readResults();','runPass(cpUnfused,bgUnfused);auto again=readResults();')
    result=once(result,'    // Arithmetic fidelity comparison (Fused vs Unfused on device)',connected+'\n    // Arithmetic fidelity comparison (Fused vs Unfused on device)')
    report=r'''
    report["scope"]="Pinned fixture host producer correction; mathematical root contract, not native omission admission or generic extraction";
    report["canonical_producer"]={{"producer_total_cpu_ms",producerTotalMs},{"distinct_rays",count},
        {"root_brackets",canonicalRootBrackets},{"unresolved_leaves",canonicalUnresolved},{"fallback_rays",canonicalFallbackRays},
        {"fallback","previously qualified branch-aware host schedule for the complete affected ray"},
        {"native_omission_admitted",false},{"generic_OntoMath_extraction",false},{"GPU_producer",false}};
    report["connected_measured_scope"]=connected;
    report["qualification"]["device_fused_unfused_parity_pass"]=maxDeltaRGB_dev==0&&maxDeltaT_dev==0;
'''
    result=once(result,'    std::cout << report.dump(2) << std::endl;',report+'\n    std::cout << report.dump(2) << std::endl;')
    result=once(result,'return witnessesPass && accuracyPass && maxDeltaRGB_CPU_vs_GPU<1e-5 && maxDeltaT_CPU_vs_GPU<1e-5 ? 0 : 3;',
                'return witnessesPass && accuracyPass && maxDeltaRGB_CPU_vs_GPU<1e-5 && maxDeltaT_CPU_vs_GPU<1e-5 && maxDeltaRGB_dev==0 && maxDeltaT_dev==0 ? 0 : 3;')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
    p.add_argument('--run-only',action='store_true');p.add_argument('--repeat',type=int,default=1)
    p.add_argument('--label',default='series1');p.add_argument('--node-budget',type=int,default=8192)
    p.add_argument('--skip-connected',action='store_true',help='Correctness/forced-fallback control; omit producer-inclusive timing')
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if work==root or root in work.parents:raise SystemExit('external work directory required')
    if not 1<=a.repeat<=128 or not 0<=a.node_budget<=1048576:raise SystemExit('invalid workload or node budget')
    if Path(a.label).name!=a.label:raise SystemExit('label must be a filename')
    base=root/'scratch/probes/ray_transport_metal_diagnostic.py'
    tree=ast.parse(base.read_text())
    v={x.targets[0].id:ast.literal_eval(x.value) for x in tree.body
       if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    fixture_hashes={}
    for key,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),
                    ('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),
                    ('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:
        digest=hashlib.sha256((root/rel).read_bytes()).hexdigest()
        if digest!=v[key]:raise SystemExit('Pinned fixture changed; canonical model refuses: '+rel)
        fixture_hashes[rel]=digest
    cpp=build_canonical_probe_source(v['CPP_SOURCE'],root)
    source_hash=hashlib.sha256(cpp.encode()).hexdigest()
    output=work/(a.label+'.json')
    if output.exists():raise SystemExit('refusing report overwrite')
    if not a.run_only:
        work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp)
        (work/'source.json').write_text(json.dumps({'generated_cpp_sha256':source_hash,'fixtures':fixture_hashes},indent=2)+'\n')
        build=root/'build';db=json.loads((build/'compile_commands.json').read_text())
        c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
        args=shlex.split(c['command']);args[args.index('-c')+1]=str(work/'probe.cpp');args[args.index('-o')+1]=str(work/'probe.o');args+=['-std=c++20','-O3']
        subprocess.run(args,cwd=build,check=True)
        args=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
        args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o')
        args[args.index('-o')+1]=str(work/'probe');subprocess.run(args,cwd=build,check=True)
    elif hashlib.sha256((work/'probe.cpp').read_bytes()).hexdigest()!=source_hash:
        raise SystemExit('replay source differs; rebuild in a fresh directory')
    # Compilation can take time; a concurrent author edit cannot inherit the
    # admission decision made before it. The fixed model must still match.
    for rel,expected in fixture_hashes.items():
        if hashlib.sha256((root/rel).read_bytes()).hexdigest()!=expected:
            raise SystemExit('Pinned fixture changed before execution: '+rel)
    env=dict(os.environ,EC_MATCH_REPEAT=str(a.repeat),EC_CANONICAL_NODE_BUDGET=str(a.node_budget))
    env.pop('EC_CANONICAL_SKIP_CONNECTED',None)
    if a.skip_connected:env['EC_CANONICAL_SKIP_CONNECTED']='1'
    subprocess.run([str(work/'probe'),str(output)],cwd=root,check=True,env=env)
    report=json.loads(output.read_text())
    if not all(report['qualification'][key] for key in ['closed_form_witnesses_pass','mean_p99_max_reference_gates_pass','independent_AST_agreement_pass','device_fused_unfused_parity_pass']):
        raise SystemExit('qualification failed')
    print('Qualified only for the stated frozen diagnostic scope:',output)


if __name__=='__main__':main()
