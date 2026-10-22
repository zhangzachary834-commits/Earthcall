"""Broader camera-ray qualification of the retained numerical GPU experiment.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
External manifests pin captured native rays and independent references. No generic
source admission, changed reference threshold, production/default or save mutation.
"""
import argparse,ast,hashlib,importlib.util,json,math,os,shlex,subprocess
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def once(s,a,b):
    if s.count(a)!=1:raise ValueError(('source hook',a,s.count(a)))
    return s.replace(a,b)

def source(base,root):
    s=load(root,'ray_transport_gpu_producer_uncertain_bands').source(base,root)
    s=once(s,'double clock = 7.25;','double clock = std::stod(std::getenv("EC_POSE_CLOCK"));')
    s=once(s,'std::ifstream rin("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json")',
        'std::ifstream rin(std::getenv("EC_POSE_RAYS"))')
    s=once(s,'std::ifstream rref("docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json")',
        'std::ifstream rref(std::getenv("EC_POSE_REFERENCE"))')
    s=once(s,'    const std::string marchingBody=currentProgram.wgsl.substr(begin,end-begin);','    const std::string originalMarchingBody=currentProgram.wgsl.substr(begin,end-begin);\n    std::string marchingBody=originalMarchingBody;unsigned emptySupportDiscardAdaptations=0;\n    const std::string emptySupportDiscard="if (supportEnd <= supportStart) { discard; }";\n    auto emptyExit=marchingBody.find(emptySupportDiscard);\n    if(emptyExit!=std::string::npos){\n        if(marchingBody.find(emptySupportDiscard,emptyExit+1)!=std::string::npos)throw std::runtime_error("duplicate empty support hook");\n        marchingBody.replace(emptyExit,emptySupportDiscard.size(),\n            "if (supportEnd <= supportStart) { capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0);return; }");\n        emptySupportDiscardAdaptations=1;\n    }\n    if(marchingBody.find("discard;")!=std::string::npos)throw std::runtime_error("unadmitted fragment-only operation in compute adapter");')
    s=once(s,'std::vector<int>{1,8,64}','std::vector<int>{1,64,512}')
    s=s.replace('accuracy_vs_65536_step_reference','accuracy_vs_independent_reference')
    # Parent normalizes the final batch. Keep that condition aligned with512.
    s=once(s,'if(n==(repeatFactor==1?64:4))','if(n==(repeatFactor==1?512:4))')
    s=once(s,'        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);',
        '        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);\n        if(!mc.ok)throw std::runtime_error("result mapping failed");')
    meta=r'''
    report["scope"]="Additional native camera-ray numerical qualification; pinned four-curtain source; no generic/native omission/production/full-frame admission";
    report["current_baseline_witness"]["integration_body_verbatim"]=emptySupportDiscardAdaptations==0;
    report["current_baseline_witness"]["empty_support_discard_to_identity_adaptations"]=emptySupportDiscardAdaptations;
    report["current_baseline_witness"]["adaptation_scope"]="Only generated empty-support fragment discard becomes raw transport identity (L=0,T=1); all other operations retained";
    report["reference_resolution"]={{"finest_intervals",ref.at("finest_intervals")},{"half_intervals",ref.at("half_intervals")}};
    report["pose_case"]={{"view",rays.at("view")},{"clock",clock},{"captured_native_ray_count",rays.at("native_volume_rays")},
        {"selected_reference_ray_count",count},{"selection",rays.at("selection")},{"actual_full_viewport",false}};
    report["timing_protocol"]="Matched captured-ray invocations, 1/64/512-dispatch completion-fenced host batches; final consumer-only normalization512; no GPU timestamps or timed readback";
'''
    return once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def verify_case(root,path,zone_hash):
    m=json.loads(path.read_text());r=Path(m['rays']);ref=Path(m['reference'])
    if not r.is_absolute() or not ref.is_absolute():raise ValueError('case paths must be absolute')
    if m['zone_sha256']!=zone_hash or sha(r)!=m['rays_sha256'] or sha(ref)!=m['reference_sha256']:raise ValueError('case/zone hash mismatch')
    rays=json.loads(r.read_text());rows=json.loads(ref.read_text())['rows'];a=rays['rays']
    if not 5<=len(a)<=2304 or len(a)!=len(rows) or rays['viewport']!=[2560,1440]:raise ValueError('case count/viewport')
    if not math.isfinite(rays['time']) or not all(math.isfinite(x) for x in rays['origin']):raise ValueError('case clock/origin')
    if len(set(tuple(x[:2]) for x in a))!=len(a):raise ValueError('duplicate pixels')
    for v,w in zip(a,rows):
        if len(v)!=23 or len(w)!=10 or v[:2]!=w[:2] or not all(math.isfinite(x) for x in v+w) or v[5]>v[6]:raise ValueError('invalid native/reference row')
    return m,rays,rows

def independent_check(report,raw,rays,refs):
    if len(raw)!=len(refs):raise ValueError('raw count')
    def stats(v):
        v=sorted(v)
        if not v or not all(math.isfinite(x) for x in v):raise ValueError('nonfinite errors')
        return {'mean':sum(v)/len(v),'p99':v[len(v)*99//100],'max':v[-1]}
    result={}
    for name,start in [('finest',2),('half',6)]:
        metrics={}
        for lane in ['current','candidate']:
            rgb=[];trans=[]
            for native,captured,ref in zip(raw,rays['rays'],refs):
                if native[:2]!=captured[:2] or native[:2]!=ref[:2]:raise ValueError('pixel order')
                if not all(math.isfinite(x) for x in native):raise ValueError('nonfinite native output')
                color=captured[8:11] if lane=='current' else native[2:5]
                t=captured[7] if lane=='current' else native[5]
                rgb.extend(abs(color[k]-ref[start+1+k]) for k in range(3));trans.append(abs(t-ref[start]))
            metrics[lane]={'RGB_pooled_channels':stats(rgb),'T':stats(trans)}
        gates={ch+'.'+metric:metrics['candidate'][ch][metric]<=metrics['current'][ch][metric]
            for ch in ['RGB_pooled_channels','T'] for metric in ['mean','p99','max']}
        result[name]={'metrics':metrics,'gates':gates,'all_six_pass':all(gates.values())}
    for lane,key in [('current','native_baseline'),('candidate','unfused_A_star_on_device')]:
        for ch,values in result['finest']['metrics'][lane].items():
            for key2,value in values.items():
                if not math.isclose(value,report['accuracy_vs_independent_reference'][key][ch][key2],rel_tol=1e-10,abs_tol=1e-15):raise ValueError('saved metric mismatch')
    result['reference_convergence']={'T_max_delta':max(abs(r[2]-r[6]) for r in refs),
        'RGB_max_delta':max(abs(r[3+k]-r[7+k]) for r in refs for k in range(3))}
    result['both_resolutions_pass']=result['finest']['all_six_pass'] and result['half']['all_six_pass']
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
    p.add_argument('--case',type=Path);p.add_argument('--build-only',action='store_true');p.add_argument('--run-only',action='store_true');p.add_argument('--label',default='normal')
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if root==work or root in work.parents or Path(a.label).name!=a.label:raise SystemExit('unsafe work/label')
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    zone=root/'saves/zones/Northern Veil/zone.ecform'
    if sha(zone)!=v['ZONE_HASH']:raise SystemExit('pinned authored source changed')
    case_data=None
    if not a.build_only:
        if not a.case:raise SystemExit('--case required for execution')
        case_data=verify_case(root,a.case.resolve(),v['ZONE_HASH']);mhash=sha(a.case.resolve())
    cpp=source(v['CPP_SOURCE'],root);digest=hashlib.sha256(cpp.encode()).hexdigest()
    if not a.run_only:
        work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp)
        (work/'source.json').write_text(json.dumps({'cpp_sha256':digest,'zone_sha256':v['ZONE_HASH']},indent=2)+'\n')
        b=root/'build';db=json.loads((b/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
        cmd=shlex.split(c['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3'];subprocess.run(cmd,cwd=b,check=True)
        cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');subprocess.run(cmd,cwd=b,check=True)
    elif sha(work/'probe.cpp')!=digest:raise SystemExit('replay source mismatch')
    if a.build_only:return
    m,rays,refs=case_data
    verify_case(root,a.case.resolve(),v['ZONE_HASH'])
    if sha(a.case.resolve())!=mhash or sha(zone)!=v['ZONE_HASH']:raise ValueError('case/source changed before execution')
    output=work/(a.label+'.json')
    if output.exists():raise SystemExit('refusing output overwrite')
    env=dict(os.environ,EC_POSE_RAYS=m['rays'],EC_POSE_REFERENCE=m['reference'],EC_POSE_CLOCK=str(rays['time']),
        EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE',None)
    result=subprocess.run([str(work/'probe'),str(output)],cwd=root,env=env)
    verify_case(root,a.case.resolve(),v['ZONE_HASH'])
    if sha(a.case.resolve())!=mhash or sha(zone)!=v['ZONE_HASH']:raise ValueError('case/source changed during execution')
    if not output.exists():raise SystemExit('native run failed without report: '+str(result.returncode))
    report=json.loads(output.read_text());raw=json.loads(Path(str(output)+'-rows.json').read_text())
    independent=independent_check(report,raw,rays,refs)
    evidence={'case_manifest_sha256':mhash,'case':m,'child_exit_code':result.returncode,'independent_checks':independent}
    Path(str(output)+'-independent.json').write_text(json.dumps(evidence,indent=2)+'\n')
    if result.returncode!=0:raise SystemExit('native numerical gate failed: '+str(result.returncode))
    load(root,'ray_transport_gpu_producer_repaired').validate_report(report)
    if not independent['both_resolutions_pass']:raise SystemExit('reference-resolution gate failed')
    print('PASS pinned camera/clock numerical scope:',m['case'],output)

if __name__=='__main__':main()
