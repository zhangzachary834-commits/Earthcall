"""Bounded Sixth-One Sun takeover of 6el: actual poisoned preflight and late refusal.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Preserves GPU arithmetic; no production, save or full-frame/FPS claim.
"""
import argparse, gzip, hashlib, importlib.util, json, os, re, subprocess, sys
from pathlib import Path


def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'))
    m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def clean_source(root):
    s=load(root,'ray_transport_gpu_6el_clean_multi_scale').generate(root)
    before=s
    needle='        // Preflight Parity Assertion outside timed loops at scale K:'
    helpers='''        J preflightRows=J::object(), preflightChecks=J::array();
        auto poisonPreflight=[&](){
            std::vector<GpuResult> poison(executionRays);
            for(auto& x:poison)for(float& v:x.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);
        };
        auto finitePreflight=[&](const std::vector<GpuResult>& rows,int mode){
            for(size_t i=0;i<count;++i)for(float v:rows[i].rgb_T)
                if(!std::isfinite(v))throw std::runtime_error("nonfinite/unpublished clean preflight: mode="+std::to_string(mode)+" ray="+std::to_string(i));
        };
        auto capturePreflight=[&](const std::vector<GpuResult>& rows,int mode){
            if(scaleK!=1)return;
            J values=J::array();
            for(size_t i=0;i<count;++i)values.push_back({i,rows[i].rgb_T[0],rows[i].rgb_T[1],rows[i].rgb_T[2],rows[i].rgb_T[3]});
            preflightRows[std::to_string(mode)]=values;
        };
'''
    assert s.count(needle)==1;s=s.replace(needle,helpers+needle)
    start=s.index(needle);end=s.index('        // 4 Warmup cycles',start);part=s[start:end]
    part=part.replace('        execute(0, 1, false);','        poisonPreflight(); execute(0, 1, false);').replace('        execute(1, 1, false);','        poisonPreflight(); execute(1, 1, false);')
    part=part.replace('auto resCurrent = readResults();','auto resCurrent = readResults(); finitePreflight(resCurrent,0); capturePreflight(resCurrent,0);').replace('auto resUnfused = readResults();','auto resUnfused = readResults(); finitePreflight(resUnfused,1); capturePreflight(resUnfused,1);')
    needle2='            execute(m, 1, false);\n            auto resM = readResults();'
    assert part.count(needle2)==1
    part=part.replace(needle2,'''            poisonPreflight();
            if(!(std::getenv("EC_SUN_MISSING_PUBLICATION") && m==5))execute(m,1,false);
            auto resM = readResults(); finitePreflight(resM,m); capturePreflight(resM,m);''')
    part=part.replace('            if (diffs != 0) {','            preflightChecks.push_back({{"mode",m},{"differing_rays",diffs},{"finite",true}});\n            if (diffs != 0) {')
    s=s[:start]+part+s[end:]
    needle3='        report["preflight_parity_pass"] = true;'
    assert s.count(needle3)==1
    s=s.replace(needle3,needle3+'\n        report["preflight_checks"] = preflightChecks;\n        report["preflight_actual_rows"] = preflightRows;\n        report["preflight_poisoned"] = true;')
    # Shader literals must remain byte-identical: change host validation only.
    wgsl=lambda x:re.findall(r'R"WGSL\((.*?)\)WGSL"',x,re.S)
    assert wgsl(s)==wgsl(before)
    return s


def refusal_source(root,negative=False):
    m=load(root,'ray_transport_gpu_6ek_indirect_consumer')
    s=m.generate(root,mutation='late-member1-refusal' if negative else 'positive-late-refusal')
    start=s.index('        for(int pose:{0,1,0,2,3,0}){')
    end=s.index('        std::ofstream reviewOut(outputPath);',start)
    # One full-population original-pose case, no unrequested 30-case rerun.
    s=s[:start]+'''        wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);
        baselineReview();checkReview("pose-0",1,false);
'''+s[end:]
    hook='            uint32_t refused=0,packets=0,wrong=0,owners=0,finalOwners=0,completed_s1=0,completed_s2=0,terminalEndpointChecks=0;'
    assert s.count(hook)==1
    pre='''            // Intended refusal set comes from classifier S2 membership, independently of refusal flags.
            auto intendedBytes=readBackBuffer(s2TileBuf,2304*sizeof(uint32_t));
            const auto* intendedTiles=reinterpret_cast<const uint32_t*>(intendedBytes.data());
            std::vector<bool> intendedRefusal(count,false);
            for(uint32_t t=0;t<c.s2;++t){
                auto tile=intendedTiles[t];if(tile>=2304)throw std::runtime_error("invalid intended S2 tile");
                for(uint32_t j=tileOffsets[tile];j<tileOffsets[tile+1];++j)intendedRefusal[tileRayIndices[j]]=true;
            }
'''
    s=s.replace(hook,pre+hook)
    hook='                refused+=fallback;'
    assert s.count(hook)==1
    s=s.replace(hook,hook+'''
                if(fallback!=intendedRefusal[i])throw std::runtime_error("late refusal set differs from intended S2 membership");
                if(fallback){
                    if(hs[i].subpanelCount!=0u)throw std::runtime_error("partial refused packet");
                    for(uint32_t k=0;k<cfg.maxSubPerRay*4u;++k)
                        if(sWords[i*cfg.maxSubPerRay*4u+k]!=0xcdcdcdcdu)throw std::runtime_error("refused ray wrote sBuf");
                }
''')
    return s


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True)
    p.add_argument('--work',type=Path,required=True);p.add_argument('--case',type=Path,required=True)
    p.add_argument('--scales',type=int,nargs='+',default=[1,2,4,8,16]);p.add_argument('--run',action='store_true')
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if work.exists() or work==root or root in work.parents:raise ValueError('fresh external work required')
    work.mkdir(parents=True)
    sources={'clean':clean_source(root),'positive':refusal_source(root),'partial_negative':refusal_source(root,True)}
    for name,s in sources.items(): (work/(name+'.cpp')).write_text(s)
    index={'scope':'bounded native scratch validation; no renderer/full-frame/FPS qualification','sources':{k:hashlib.sha256(v.encode()).hexdigest() for k,v in sources.items()}}
    if not a.run:(work/'index.json').write_text(json.dumps(index,indent=2));return
    wide=load(root,'ray_transport_gpu_wide_stream');zone=root/'saves/zones/Northern Veil/zone.ecform';zone_sha=hashlib.sha256(zone.read_bytes()).hexdigest()
    meta,rays,_=wide.verify_case(root,a.case.resolve(),zone_sha)
    env=dict(os.environ,EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001')
    for k in ['EC_GPU_GEOMETRY_CASE','METAL_CAPTURE_ENABLED','EC_SUN_MISSING_PUBLICATION']:env.pop(k,None)
    def build(name):
        log=work/(name+'-build.log')
        with log.open('w') as f:subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(a.build_root.resolve()),'--source',str(work/(name+'.cpp')),'--work',str(work/(name+'-build'))],stdout=f,stderr=subprocess.STDOUT,check=True)
        print('BUILT',name,flush=True)
    def run(name,label,extra=None,negative=None):
        out=work/(label+'.json');log=work/(label+'.log')
        with log.open('w') as f:r=subprocess.run([str(work/(name+'-build/probe')),str(out)],cwd=root,env=dict(env,**(extra or {})),stdout=f,stderr=subprocess.STDOUT)
        if hashlib.sha256(zone.read_bytes()).hexdigest()!=zone_sha:raise ValueError('authored fixture drift')
        wide.verify_case(root,a.case.resolve(),zone_sha)
        if negative:
            if r.returncode==0 or negative not in log.read_text():raise ValueError('negative failed at wrong boundary: '+label)
            print('EXPECTED REJECTION',label,r.returncode,flush=True);return {'returncode':r.returncode,'expected_error':negative,'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()}
        if r.returncode!=0:raise ValueError('native failed: '+label+'; see '+str(log))
        print('PASS',label,flush=True);return json.loads(out.read_text())
    for name in ['positive','partial_negative','clean']:build(name)
    index['positive']=run('positive','positive')
    case=index['positive']['cases'][0]
    assert case['refused']==10068 and case['final_output_errors']==case['producer_entry_errors']==case['output_differences']==0
    # Keep result rows/flags as compact replay evidence, omit endpoint bulk from index.
    index['positive']={'case':{k:v for k,v in case.items() if k not in ['outputs','flags','terminal_endpoints']},'file':'positive.json'}
    index['partial_negative']=run('partial_negative','partial-negative',negative='routed exactly-once final output publication failure: 10068 errors')
    index['missing_publication_negative']=run('clean','missing-publication-negative',{'EC_GPU_SCALE':'1','EC_SUN_MISSING_PUBLICATION':'1'},negative='nonfinite/unpublished clean preflight: mode=5')
    gate=load(root,'ray_transport_full_workload_accuracy');ref=gate.read(root/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')
    index['clean_runs']={};index['accuracy']={}
    for style in [1,0]:
        for k in a.scales:
            result=run('clean',f'clean-w{style}-K{k}',{'EC_GPU_SCALE':str(k),'EC_GPU_WAIT_STYLE':str(style)})
            assert result['preflight_parity_pass'] and result['preflight_poisoned']
            if k==1:
                normalized={m:[[ *rays['rays'][i][:2],*row[1:]] for i,row in enumerate(rows)] for m,rows in result['preflight_actual_rows'].items()}
                for m,rows in normalized.items():
                    metrics=gate.evaluate(rows,normalized['0'],ref,gate.digest(meta['rays']))
                    if not metrics['both_resolutions_pass']:raise ValueError('all-row accuracy failed mode '+m)
                    index['accuracy'][f'w{style}-m{m}']=metrics
            # Original actual rows persist in run JSON; keep compact index separate.
            result.pop('preflight_actual_rows',None);index['clean_runs'][f'w{style}-K{k}']=result
    index.update(fixture_sha256=zone_sha,rays_sha256=gate.digest(meta['rays']),engine_src_tree=subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=root,text=True).strip(),shipping_gates_open=[1,2,3])
    (work/'index.json').write_text(json.dumps(index,indent=2)+'\n');print('COMPLETE; all shipping gates OPEN',flush=True)

if __name__=='__main__':main()
