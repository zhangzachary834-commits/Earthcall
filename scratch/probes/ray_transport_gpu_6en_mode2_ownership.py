"""Actual routed-unfused Mode2 publication/refusal diagnostic; no performance claim.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
Builds from the qualified clean kernels; instrumentation is separate from cost execution.
"""
import argparse,hashlib,importlib.util,json,os,re,subprocess,sys
from pathlib import Path


def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def literal(s,name):
    m=re.search(r'static const char\* '+name+r' = R"WGSL\((.*?)\)WGSL";',s,re.S)
    if not m:raise ValueError('shader missing '+name)
    return m


def replace_shader(s,name,body):
    m=literal(s,name);return s[:m.start(1)]+body+s[m.end(1):]


def group(s,name):
    start=s.index('auto '+name+' = makeGroup(');end=s.index('});',start)+3;return start,end,s[start:end]


def generate(root):
    clean=load(root,'ray_transport_gpu_6em_corrective_takeover').clean_source(root)
    s=load(root,'ray_transport_gpu_6ek_indirect_consumer').generate(root,'fixed')
    restored={}
    names=[('kS1UnfusedProducerWgsl','kS1ProducerWgsl'),('kS2ProducerWgsl','kS2ProducerWgsl'),('kFallbackProducerWgsl','kFallbackProducerWgsl')]
    for src,dst in names:
        original=literal(clean,src)[1];code=original
        audit='\n@group(0) @binding(9) var<storage,read_write> auditWrites:array<atomic<u32>>;\n'
        hook='    if (id >= cfg.rayCount) { return; }';assert code.count(hook)==1
        code=code.replace(hook,hook+'\n    atomicAdd(&auditWrites[id*2u],1u);')
        if src=='kS2ProducerWgsl':
            control='\n@group(0) @binding(10) var<uniform> witnessControl:vec4u;\n@group(0) @binding(11) var<storage,read_write> witnessResults:array<vec4f>;\n'
            member='    for (var member = 0u; member < 2u; member++) {';assert code.count(member)==1
            late='''
        if(member==1u && witnessControl.x!=0u){
            if(witnessControl.y!=0u){witnessResults[id]=vec4f(1.0,0.0,0.0,0.5);atomicAdd(&auditWrites[id*2u+1u],1u);}
            reason|=64u;break;
        }'''
            code=code.replace(member,member+late)
            restored[src]=code.replace(member+late,member).replace(hook+'\n    atomicAdd(&auditWrites[id*2u],1u);',hook)
            assert restored[src]==original
            code=audit+control+code
        else:
            assert code.replace(hook+'\n    atomicAdd(&auditWrites[id*2u],1u);',hook)==original
            code=audit+code
        s=replace_shader(s,dst,code)
    # Consumer arithmetic is taken directly from the actual clean winner.
    original=literal(clean,'kUnfusedShader')[1];needle='    results[rayIdx] = vec4<f32>(L, T);';assert original.count(needle)==1
    s=replace_shader(s,'kUnfusedShader',original.replace(needle,needle+'\n    atomicAdd(&auditWrites[rayIdx*2u+1u],1u);'))
    anchor='    auto cpS1 = buildCompute(kS1ProducerWgsl, "s1ProducerMain");';assert s.count(anchor)==1
    s=s.replace(anchor,'    std::array<uint32_t,4> witnessControl{};\n    auto witnessControlBuf=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,16,witnessControl.data());\n'+anchor)
    for old,new in [('bgS1','bgS1Unfused'),('bgS1Forced','bgS1UnfusedForced'),('bgS2','bgS2'),('bgS2Forced','bgS2Forced'),('bgFallback','bgFallback'),('bgFallbackForced','bgFallbackForced')]:
        _,_,g=group(clean,new);g=g.replace(new,old).replace('cpS1Unfused','cpS1').replace('numTiles * 4','2304 * 4')
        end=g.rfind('\n    });');extra=',\n        makeEntry(9,auditWritesBuf,0,count*8)'
        if old in ['bgS2','bgS2Forced']:extra+=',\n        makeEntry(10,witnessControlBuf,0,16),\n        makeEntry(11,outBuf,0,outBytes)'
        g=g[:end]+extra+g[end:];a,b,_=group(s,old);s=s[:a]+g+s[b:]
    # Run exactly the qualified Mode2 pass chain through the diagnostic slot3.
    a=clean.index('else if(mode==2){');b=clean.index('else if(mode==3){',a);path=clean[a:b]
    path=path.replace('else if(mode==2){','else if(mode==3){').replace('cpS1Unfused','cpS1').replace('bgS1Unfused','bgS1').replace('numTiles','2304u')
    for label,env in [('Pass 3: S1 Producer','EC_OMIT_S1'),('Pass 4: S2 Producer','EC_OMIT_S2'),('Pass 5: Fallback Producer','EC_OMIT_GENERAL')]:
        # Identify by pipeline, since prose comments differ across ancestor versions.
        cp={'EC_OMIT_S1':'cpS1','EC_OMIT_S2':'cpS2','EC_OMIT_GENERAL':'cpFallback'}[env]
        hit=path.index('wgpuComputePassEncoderSetPipeline(p, '+cp+');');start=path.rfind('{',0,hit);end=path.index('wgpuComputePassEncoderRelease(p);',hit)+len('wgpuComputePassEncoderRelease(p);');end=path.index('}',end)+1
        block=path[start:end];path=path[:start]+f'if(!std::getenv("{env}"))'+block+path[end:]
    path=path.replace('dispatch(cpUnfused, bgUnfused, count);','if(!std::getenv("EC_OMIT_CONSUMER"))dispatch(cpUnfused,bgUnfused,count);')
    path=path.replace('dispatch(fallbackCP, fallbackBG, count);','if(!std::getenv("EC_OMIT_CURRENT"))dispatch(fallbackCP,fallbackBG,count);\n                    if(std::getenv("EC_DOUBLE_CURRENT"))dispatch(fallbackCP,fallbackBG,count);')
    a=s.index('else if(mode==3){');b=s.index('else if(mode==4){',a);s=s[:a]+path+s[b:]
    start=s.index('        J routeChecks=J::array();');cut=s.index('        auto checkReview=',start);helpers=s[start:cut]
    stop=s.index('return 0;',s.index('std::ofstream reviewOut(outputPath);',cut))+len('return 0;')
    s=s[:start]+helpers+HOST+s[stop:]
    return s


HOST=r'''
        baselineReview();
        const auto* nominal=reinterpret_cast<const GpuRayHeader*>(refH.data());
        for(size_t i=0;i<count;++i)if(nominal[i].overlapCount&0x80000000u)throw std::runtime_error("nominal bulk refused: independent demand unavailable");
        auto checkCase=[&](std::string label,uint32_t subCap,uint32_t overCap,bool late,bool partial,bool zeroBudget){
            auto limited=cfg;limited.maxSubPerRay=subCap;limited.maxOverPerRay=overCap;if(zeroBudget)limited.pad=0;
            wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));
            witnessControl={uint32_t(late),uint32_t(partial),0,0};wgpuQueueWriteBuffer(gpu.queue,witnessControlBuf,0,witnessControl.data(),16);
            poisonOutput();poisonSchedule();execute(3,1,false);
            auto actual=readResults();auto hb=readBackBuffer(rBuf,rayBytes);auto sb=readBackBuffer(sBuf,subpanelBytes);auto ob=readBackBuffer(oBuf,overlapBytes);
            auto wb=readBackBuffer(auditWritesBuf,auditBytes);auto cb=readBackBuffer(cntBuf,sizeof(initCnt));auto tb=readBackBuffer(s2TileBuf,2304*4);
            DispatchCounters census{};std::memcpy(&census,cb.data(),sizeof(census));if(census.s1+census.s2+census.fb!=2304)throw std::runtime_error("route census mismatch");
            auto tileIds=reinterpret_cast<const uint32_t*>(tb.data());std::vector<bool> member1Route(count,false);
            for(uint32_t t=0;t<census.s2;++t){if(tileIds[t]>=2304)throw std::runtime_error("invalid S2 tile");for(auto j=tileOffsets[tileIds[t]];j<tileOffsets[tileIds[t]+1];++j)member1Route[tileRayIndices[j]]=true;}
            auto hs=reinterpret_cast<const GpuRayHeader*>(hb.data());auto writes=reinterpret_cast<const uint32_t*>(wb.data());auto sw=reinterpret_cast<const uint32_t*>(sb.data());
            auto subs=reinterpret_cast<const GpuSubpanel*>(sb.data());auto overs=reinterpret_cast<const GpuOverlap*>(ob.data());auto refSubs=reinterpret_cast<const GpuSubpanel*>(refS.data());auto refOvers=reinterpret_cast<const GpuOverlap*>(refO.data());
            uint32_t owners=0,publishers=0,flags=0,wrong=0,packets=0,refused=0;uint64_t privateWrites=0;J rows=J::array(),intendedIds=J::array();
            for(size_t i=0;i<count;++i){
                owners+=(writes[2*i]!=1);publishers+=(writes[2*i+1]!=1);
                bool expected=zeroBudget||(late&&member1Route[i])||nominal[i].subpanelCount>subCap||nominal[i].overlapCount>overCap;
                bool fallback=(hs[i].overlapCount&0x80000000u)!=0;
                flags+=(fallback!=expected)||bool(hs[i].overlapCount&0x40000000u);refused+=fallback;
                if(expected)intendedIds.push_back(i);
                const auto& truth=expected?currentReview[i]:bulkReview[i];wrong+=!bitsEqual(actual[i],truth);
                if(fallback){
                    packets+=hs[i].subpanelCount!=0;
                    uint64_t base=uint64_t(hs[i].subpanelOffset)*4;
                    if(base+uint64_t(subCap)*4>sb.size()/4)throw std::runtime_error("refused private packet storage outside bounds");
                    for(uint32_t k=0;k<subCap*4;++k)privateWrites+=(sw[base+k]!=0xcdcdcdcdu);
                }else{
                    packets+=hs[i].subpanelCount!=nominal[i].subpanelCount||hs[i].overlapCount!=nominal[i].overlapCount;
                    if(hs[i].subpanelOffset+hs[i].subpanelCount>sb.size()/sizeof(GpuSubpanel)||hs[i].overlapOffset+hs[i].overlapCount>ob.size()/sizeof(GpuOverlap))throw std::runtime_error("published packet outside bounds");
                    for(uint32_t k=0;k<hs[i].subpanelCount;++k){auto x=subs[hs[i].subpanelOffset+k],y=refSubs[nominal[i].subpanelOffset+k];packets+=std::bit_cast<uint32_t>(x.s0)!=std::bit_cast<uint32_t>(y.s0)||std::bit_cast<uint32_t>(x.s1)!=std::bit_cast<uint32_t>(y.s1)||x.mediumIndex!=y.mediumIndex;}
                    for(uint32_t k=0;k<hs[i].overlapCount;++k){auto x=overs[hs[i].overlapOffset+k],y=refOvers[nominal[i].overlapOffset+k];packets+=std::bit_cast<uint32_t>(x.s0)!=std::bit_cast<uint32_t>(y.s0)||std::bit_cast<uint32_t>(x.s1)!=std::bit_cast<uint32_t>(y.s1)||x.activeMask!=y.activeMask;}
                }
                J a=J::array(),e=J::array();for(int c=0;c<4;++c){a.push_back(std::bit_cast<uint32_t>(actual[i].rgb_T[c]));e.push_back(std::bit_cast<uint32_t>(truth.rgb_T[c]));}rows.push_back({i,a,e,hs[i].overlapCount});
            }
            if(owners)throw std::runtime_error("Mode2 producer ownership failure: "+std::to_string(owners));
            if(publishers)throw std::runtime_error("Mode2 final publication failure: "+std::to_string(publishers));
            if(flags)throw std::runtime_error("Mode2 intended refusal set failure: "+std::to_string(flags));
            if(wrong||packets)throw std::runtime_error("Mode2 output/packet mismatch: "+std::to_string(wrong)+"/"+std::to_string(packets));
            routeChecks.push_back({{"case",label},{"rays",count},{"refused",refused},{"owners",owners},{"publishers",publishers},{"flags",flags},{"wrong",wrong},{"packets",packets},{"private_scratch_words_changed_on_refused",privateWrites},{"intended_refusal_ids",intendedIds},{"outputs",rows}});
            std::cerr<<"MODE2 "<<label<<" PASS refused="<<refused<<" privateScratchWords="<<privateWrites<<std::endl;
        };
        bool negative=std::getenv("EC_NEGATIVE")!=nullptr;
        if(negative)checkCase("negative",64,16,true,std::getenv("EC_PARTIAL_PUBLISH")!=nullptr,false);
        else{
            checkCase("nominal",64,16,false,false,false);
            checkCase("sub-cap4",4,16,false,false,false);
            checkCase("over-cap1",64,1,false,false,false);
            checkCase("forced-late-S2",64,16,true,false,false);
            checkCase("zero-budget",64,16,false,false,true);
        }
        std::ofstream(outputPath)<<J({{"scope","Mode2 diagnostic ownership/refusal; no timing qualification"},{"cases",routeChecks}}).dump()<<std::endl;return 0;
'''


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['root','build-root','work','case']:p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--run',action='store_true');a=p.parse_args();root=a.root.resolve();w=a.work.resolve()
    if w.exists() or root==w or root in w.parents:raise ValueError('fresh external work required')
    w.mkdir(parents=True);s=generate(root);(w/'probe.cpp').write_text(s)
    if not a.run:return
    with (w/'build.log').open('w') as f:subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(a.build_root),'--source',str(w/'probe.cpp'),'--work',str(w/'build')],stdout=f,stderr=subprocess.STDOUT,check=True)
    wide=load(root,'ray_transport_gpu_wide_stream');fixture=root/'saves/zones/Northern Veil/zone.ecform';before=hashlib.sha256(fixture.read_bytes()).hexdigest();meta,rays,_=wide.verify_case(root,a.case,before)
    env=dict(os.environ,EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001')
    for key in list(env):
        if key.startswith(('EC_OMIT_','EC_DOUBLE_')) or key in ['EC_NEGATIVE','EC_PARTIAL_PUBLISH','EC_GPU_GEOMETRY_CASE','METAL_CAPTURE_ENABLED']:env.pop(key)
    controls=[('positive',{},None),('omit-S1',{'EC_NEGATIVE':'1','EC_OMIT_S1':'1'},'Mode2 producer ownership failure'),('omit-S2',{'EC_NEGATIVE':'1','EC_OMIT_S2':'1'},'Mode2 producer ownership failure'),('omit-General',{'EC_NEGATIVE':'1','EC_OMIT_GENERAL':'1'},'Mode2 producer ownership failure'),('omit-consumer',{'EC_NEGATIVE':'1','EC_OMIT_CONSUMER':'1'},'Mode2 final publication failure'),('omit-Current',{'EC_NEGATIVE':'1','EC_OMIT_CURRENT':'1'},'Mode2 final publication failure'),('double-Current',{'EC_NEGATIVE':'1','EC_DOUBLE_CURRENT':'1'},'Mode2 final publication failure'),('partial-publish',{'EC_NEGATIVE':'1','EC_PARTIAL_PUBLISH':'1'},'Mode2 final publication failure')]
    statuses=[]
    for label,extra,error in controls:
        with (w/(label+'.log')).open('w') as f:r=subprocess.run([str(w/'build/probe'),str(w/(label+'.json'))],cwd=root,env=dict(env,**extra),stdout=f,stderr=subprocess.STDOUT)
        if hashlib.sha256(fixture.read_bytes()).hexdigest()!=before:raise ValueError('fixture changed')
        wide.verify_case(root,a.case,before)
        text=(w/(label+'.log')).read_text()
        if (error and (r.returncode==0 or error not in text)) or (not error and r.returncode!=0):raise ValueError('failed at unexpected boundary: '+label+' see '+str(w/(label+'.log')))
        statuses.append({'case':label,'returncode':r.returncode,'expected_error':error});print('PASS',label,r.returncode,flush=True)
    (w/'controls.json').write_text(json.dumps({'scope':'instrumented native Mode2, no cost/FPS claim','controls':statuses,'cpp_sha256':hashlib.sha256(s.encode()).hexdigest(),'fixture_sha256':before,'rays_sha256':wide.sha(meta['rays']) if hasattr(wide,'sha') else hashlib.sha256(Path(meta['rays']).read_bytes()).hexdigest()},indent=2))

if __name__=='__main__':main()
