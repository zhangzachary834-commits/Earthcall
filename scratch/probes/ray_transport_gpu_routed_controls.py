"""Instrument the actual canonical routed bridge for correctness, never clean cost.
CPU/source generation and optional isolated native controls; no production changes.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
"""
import argparse,ast,hashlib,importlib.util,json,os,re,subprocess,sys
from pathlib import Path
BRANCH = r"""        J routeChecks=J::array(); auto originalInputs=gpuRays;
        auto bitsEqual=[](const GpuResult& a,const GpuResult& b){for(int k=0;k<4;++k)if(!std::isfinite(a.rgb_T[k])||std::bit_cast<uint32_t>(a.rgb_T[k])!=std::bit_cast<uint32_t>(b.rgb_T[k]))return false;return true;};
        auto poisonOutput=[&](){std::vector<GpuResult> x(executionRays);for(auto& a:x)for(float& v:a.rgb_T)v=NAN;wgpuQueueWriteBuffer(gpu.queue,outBuf,0,x.data(),outBytes);};
        std::vector<GpuResult> currentReview,bulkReview; std::vector<uint8_t> refH,refS,refO;
        auto baselineReview=[&](){execute(0,1,false);currentReview=readResults();execute(1,1,false);bulkReview=readResults();refH=readBackBuffer(rBuf,rayBytes);refS=readBackBuffer(sBuf,subpanelBytes);refO=readBackBuffer(oBuf,overlapBytes);};
        auto checkReview=[&](const std::string& label,int rep,bool normal){
            poisonOutput();execute(3,rep,false);auto output=readResults();auto hb=readBackBuffer(rBuf,rayBytes);auto sb=readBackBuffer(sBuf,subpanelBytes);auto ob=readBackBuffer(oBuf,overlapBytes);
            auto cb=readBackBuffer(cntBuf,sizeof(initCnt));auto ab=readBackBuffer(argsBuf,36);auto wb=readBackBuffer(reviewWritesBuf,count*4);
            DispatchCounters c{};std::memcpy(&c,cb.data(),sizeof(c));std::array<uint32_t,9> args{};std::memcpy(args.data(),ab.data(),36);
            if(c.s1+c.s2+c.fb!=2304u)throw std::runtime_error("routed counter accumulation mismatch");
            const uint32_t ns[3]={c.s1,c.s2,c.fb};for(int k=0;k<3;++k)if(args[3*k]!=(ns[k]+3)/4||args[3*k+1]!=1||args[3*k+2]!=1)throw std::runtime_error("routed indirect args mismatch");
            auto writes=reinterpret_cast<const uint32_t*>(wb.data());auto hs=reinterpret_cast<const GpuRayHeader*>(hb.data());auto rh=reinterpret_cast<const GpuRayHeader*>(refH.data());
            auto subs=reinterpret_cast<const GpuSubpanel*>(sb.data());auto rs=reinterpret_cast<const GpuSubpanel*>(refS.data());auto overs=reinterpret_cast<const GpuOverlap*>(ob.data());auto rr=reinterpret_cast<const GpuOverlap*>(refO.data());
            uint32_t refused=0,packets=0,wrong=0,owners=0;J flags=J::array(),rows=J::array();
            for(size_t i=0;i<count;++i){owners+=(writes[i]!=1u);bool fallback=(hs[i].overlapCount&0x80000000u)!=0;refused+=fallback;
                const auto& expected=normal?bulkReview[i]:(fallback?currentReview[i]:bulkReview[i]);wrong+=!bitsEqual(output[i],expected);
                bool same=hs[i].subpanelCount==rh[i].subpanelCount&&hs[i].overlapCount==rh[i].overlapCount;
                if(normal&&same&&!fallback){
                    for(uint32_t k=0;k<hs[i].subpanelCount;++k){auto a=subs[hs[i].subpanelOffset+k],b=rs[rh[i].subpanelOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.mediumIndex==b.mediumIndex;}
                    for(uint32_t k=0;k<hs[i].overlapCount;++k){auto a=overs[hs[i].overlapOffset+k],b=rr[rh[i].overlapOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.activeMask==b.activeMask;}
                }
                if(normal&&!same)packets++;flags.push_back(hs[i].overlapCount);
                if(rep==1){J a=J::array(),e=J::array();for(int k=0;k<4;++k){a.push_back(std::bit_cast<uint32_t>(output[i].rgb_T[k]));e.push_back(std::bit_cast<uint32_t>(expected.rgb_T[k]));}rows.push_back({i,a,e});}
            }
            if(owners)throw std::runtime_error("routed exactly-once producer publication failure");
            if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");
            routeChecks.push_back({{"case",label},{"repeats",rep},{"rays",count},{"family_counts",{c.s1,c.s2,c.fb}},{"indirect_args",args},{"producer_entry_errors",owners},{"output_differences",wrong},{"packet_differences",normal?J(packets):J(nullptr)},{"refused",refused},{"flags",flags},{"outputs",rows}});
            std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" refused="<<refused<<std::endl;
        };
        for(int pose:{0,1,0,2,3,0}){
            gpuRays=originalInputs;
            for(auto& a:gpuRays){if(pose==1)a.ro[0]+=10000.f;if(pose==2||pose==3){a.ro[0]=0;a.ro[1]=50;a.ro[2]=240;a.rd[2]=-std::abs(a.rd[2]);a.clipA=0;a.clipB=1000;}if(pose==3){a.rd[0]=0;a.rd[1]=0;a.rd[2]=-1;}}
            wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);baselineReview();
            for(int rep:{1,2,4,9})checkReview("pose-"+std::to_string(pose),rep,true);
        }
        for(uint32_t budget:{0u,23u,24u,25u}){auto limited=cfg;limited.pad=budget;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("budget-"+std::to_string(budget),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        for(int kind:{0,1}){auto limited=cfg;if(kind==0)limited.maxSubPerRay=4;else limited.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("capacity-"+std::to_string(kind),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        std::ofstream reviewOut(outputPath);reviewOut<<J({{"scope","Instrumented canonical routed correctness only; no timings or physical profile"},{"cases",routeChecks},{"clean_timing",false},{"shipping_qualified",false}}).dump(2)<<std::endl;return 0;
"""

def generate(root,mutation='fixed'):
    spec=importlib.util.spec_from_file_location('routed',root/'scratch/probes/ray_transport_gpu_6dq_routed_residual.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    source=m.make_routed_residual_cpp_source(v['CPP_SOURCE'],root,wide=True)
    for name in ['kS1ProducerWgsl','kS2ProducerWgsl','kFallbackProducerWgsl']:
     hit=re.search(r'(static const char\* '+name+r' = R"WGSL\()(.*?)(\)WGSL";)',source,re.S);assert hit
     shader=hit[2];needle='@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;';assert shader.count(needle)==1
     shader=shader.replace(needle,needle+'\n@group(0) @binding(9) var<storage,read_write> reviewWrites:array<atomic<u32>>;')
     needle='    let input = rays[id];' if name=='kS2ProducerWgsl' else ' let input = rays[id];';assert shader.count(needle)==1
     shader=shader.replace(needle,' atomicAdd(&reviewWrites[id],1u);\n'+needle)
     source=source[:hit.start(2)]+shader+source[hit.end(2):]
    needle='    auto argsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 36, nullptr);';assert source.count(needle)==1
    source=source.replace(needle,needle+'\n    auto reviewWritesBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst,count*4,nullptr);')
    needle='makeEntry(8, cntBuf, 0, sizeof(initCnt))';assert source.count(needle)==6
    source=source.replace(needle,needle+',\n        makeEntry(9, reviewWritesBuf, 0, count*4)')
    a=source.index('        auto execute=');b=source.index('        auto correctness=',a);part=source[a:b]
    needle='            for(int k=0;k<repeats;++k){';assert part.count(needle)==1
    part=part.replace(needle,needle+'\n                if(mode==3)wgpuCommandEncoderClearBuffer(enc,reviewWritesBuf,0,count*4);')
    source=source[:a]+part+source[b:];source=source[:source.index('        auto correctness=',a)]+BRANCH+source[source.index('        auto correctness=',a):]
    s=source
    if mutation=='no-reset':
     start=s.index('        auto execute=');end=s.index('        J routeChecks=',start);part=s[start:end];needle='wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);';assert part.count(needle)==1;part=part.replace(needle,'// reset omitted for negative control');s=s[:start]+part+s[end:]
    if mutation=='no-tail':
     assert s.count('tileSlot >= min(routeCounts[')==3
     for i in range(3):s=s.replace(f'tileSlot >= min(routeCounts[{i}u], 2304u)',f'tileSlot >= min(routeCounts[{i}u]+4u, 2304u)')
    return s

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--mutation',choices=['fixed','no-reset','no-tail'],default='fixed');p.add_argument('--case',type=Path,default=Path('/private/tmp/earthcall-wide-case/manifest.json'));p.add_argument('--run',action='store_true');a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
    work.mkdir(parents=True);cpp=generate(root,a.mutation);(work/'probe.cpp').write_text(cpp)
    subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(a.build_root.resolve()),'--source',str(work/'probe.cpp'),'--work',str(work/'build')],check=True)
    if a.run:
        spec=importlib.util.spec_from_file_location('wide',root/'scratch/probes/ray_transport_gpu_wide_stream.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);zone=root/'saves/zones/Northern Veil/zone.ecform';before=hashlib.sha256(zone.read_bytes()).hexdigest();meta,rays,_=m.verify_case(root,a.case.resolve(),before)
        env=dict(os.environ,EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001');env.pop('EC_GPU_GEOMETRY_CASE',None)
        with (work/'native.log').open('x') as f:result=subprocess.run([str(work/'build/probe'),str(work/'result.json')],cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT)
        if hashlib.sha256(zone.read_bytes()).hexdigest()!=before:raise ValueError('source drift')
        m.verify_case(root,a.case.resolve(),before)
        if result.returncode:raise SystemExit(result.returncode)
    print(work)
if __name__=='__main__':main()
