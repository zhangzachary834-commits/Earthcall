"""§6dc: source-pinned review/continuation on the isolated Claude checkout.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-23.
Fresh work only; no temporary source/object substitution or save mutation.
"""
import argparse, ast, copy, hashlib, importlib.util, json, math, os, shlex, statistics, subprocess
from pathlib import Path


def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'))
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def parallel_source(base,root):
    s=load(root,'ray_transport_gpu_parallel_roots').source(base,root)
    def once(a,b):
        nonlocal s
        if s.count(a)!=1:raise ValueError(('parallel source hook',s.count(a),a[:80]))
        s=s.replace(a,b)
    once('const MAX_PIECES_PER_MEMBER: u32 = 16u;','const MAX_PIECES_PER_MEMBER: u32 = 48u;')
    once('let meta = memberMeta[pairId];','let memberInfo = memberMeta[pairId];')
    for field in ['x','y','z']:once('= meta.'+field+';', '= memberInfo.'+field+';')
    # Restore the original per-ray48 bound for each independent member task;
    # the merge still refuses a combined count above48.
    if s.count('*4u*16u*sizeof(glm::vec2)')!=2:raise ValueError('member buffer sizing identity')
    s=s.replace('*4u*16u*sizeof(glm::vec2)','*4u*48u*sizeof(glm::vec2)')
    a='''            auto parallelGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){return group(cp,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),
                entry(4,tileOverlaps,0,tileOverlapBytes),entry(5,tileMemberPieces,0,tileMemberPiecesBytes),
                entry(6,tileMemberMeta,0,tileMemberMetaBytes)});};'''
    b='''            auto parallelGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){
                std::vector<WGPUBindGroupEntry> entries={entry(0,configuration,0,sizeof(pcfg)),
                    entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),
                    entry(5,tileMemberPieces,0,tileMemberPiecesBytes),entry(6,tileMemberMeta,0,tileMemberMetaBytes)};
                if(cp==cpMemberRoots)entries.push_back(entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)));
                else {entries.push_back(entry(3,tileSingles,0,tileSingleBytes));entries.push_back(entry(4,tileOverlaps,0,tileOverlapBytes));}
                return group(cp,entries);
            };'''
    once(a,b)
    once('''                entry(2,parRay,0,rayBytes), entry(3,parSub,0,subpanelBytes), entry(4,parOver,0,overlapBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto bgMg''','''                entry(2,parRay,0,rayBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto bgMg''')
    once('''            auto bgMg = group(cpMerge, {
                entry(0,cfgBuf,0,sizeof(cfg)), entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),''','''            auto bgMg = group(cpMerge, {
                entry(0,cfgBuf,0,sizeof(cfg)),''')
    # Export actual packets/headers; C++ comparison alone is not portable replay.
    once('''            for(size_t i=0;i<count;++i){
                if(pRays[i].subpanelCount''','''            J parallelPackets=J::array();
            for(size_t i=0;i<count;++i){
                J singles=J::array(),overs=J::array();
                for(uint32_t k=0;k<pRays[i].subpanelCount;++k){const auto& a=pSubs[pRays[i].subpanelOffset+k];singles.push_back({a.s0,a.s1,a.mediumIndex});}
                for(uint32_t k=0;k<pRays[i].overlapCount;++k){const auto& a=pOvers[pRays[i].overlapOffset+k];overs.push_back({a.s0,a.s1,a.activeMask});}
                parallelPackets.push_back({R[i][0],R[i][1],pRays[i].overlapCount,singles,overs});
                if(pRays[i].subpanelCount''')
    once('''            wgpuBindGroupRelease(bgMR); wgpuBindGroupRelease(bgMg);''','''            report["parallel_packet_census_inputs"]=parallelPackets;
            wgpuBindGroupRelease(bgMR); wgpuBindGroupRelease(bgMg);''')
    once('''{"bit_exact_packets_verified_in_cpp", true},''','''            {"restored_member_piece_capacity",48},
            {"per_tile_member_state_rebuilt_each_dispatch",true},
            {"bit_exact_packets_verified_in_cpp", true},''')
    # The legacy parent writes its full JSON to stdout; the fresh runner logs it
    # separately so it can never corrupt the report file through shared offsets.
    a='throw std::runtime_error("stream mode " + std::to_string(testMode) + " changed or missed a result at ray " + std::to_string(i));'
    b=r'''{auto hb=readBackBuffer(rBuf,rayBytes);const auto* hs=reinterpret_cast<const GpuRayHeader*>(hb.data());J actual=J::array(),expectedWords=J::array(),actualWords=J::array();
                    for(int k=0;k<4;++k){actual.push_back(result[i].rgb_T[k]);expectedWords.push_back(std::bit_cast<uint32_t>(expected[i].rgb_T[k]));actualWords.push_back(std::bit_cast<uint32_t>(result[i].rgb_T[k]));}
                    J witness={{"ray_index",i},{"ray",R[i]},{"mode",testMode},{"forced",forced},{"actual",actual},{"expected_words",expectedWords},{"actual_words",actualWords},{"actual_header",{hs[i].subpanelCount,hs[i].overlapCount}},{"baseline_header",{gpuRays[i].subpanelCount,gpuRays[i].overlapCount}}};
                    std::ofstream(outputPath+"-counterexample.json")<<witness.dump(2);throw std::runtime_error("stream parity counterexample preserved");}'''
    once(a,b)
    once('    std::cout << report.dump(2) << std::endl;','    std::cout << "Parallel report saved." << std::endl;')
    return s


def coarse_peel_source(base,root,wide=False):
    """Peel ONLY the existing depth0 bow reject; keep all original capacities.
    No cross-level proof, amplitude cull, knot skip, secant or compact arrays.
    """
    import re
    parent=load(root,'ray_transport_gpu_depth0_fastpath')
    s=parent.source(base,root)
    original=re.search(r'kGpuProducerWgsl\s*=\s*R"WGSL\((.*?)\)WGSL";',s,re.S)[1].strip()
    original=original.replace('fn producerMain(', 'fn producerFastMain(')
    needle='stack[0] = vec4f(a, b, phi_a0, phi_b0); setDepth(&depthBits, 0u, 0u); top = 1u;'
    if original.count(needle)!=1:raise ValueError('coarse peel stack-entry identity')
    peeled=original.replace(needle,'''// Exact existing level-specific depth0 predicate, before private stack use.
        if (*work >= cfg.budget) { *reason |= 1u; break; }
        let coarseH = b - a;
        if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; break; }
        let coarseVa = phi_a0 - c; let coarseVb = phi_b0 - c;
        let coarseScale = 1.0 + abs(coarseVa) + abs(coarseVb) + abs(a) + abs(b);
        let coarseArithmetic = 0.000001 * coarseScale;
        let coarseError = err + coarseArithmetic;
        let coarseBow = 0.125 * M * coarseH * coarseH + coarseError;
        if (min(coarseVa, coarseVb) > coarseBow || max(coarseVa, coarseVb) < -coarseBow) { *work += 1u; continue; }
        '''+needle)
    def once(a,b):
        nonlocal s
        if s.count(a)!=1:raise ValueError(('coarse source hook',s.count(a),a[:70]))
        s=s.replace(a,b)
    for compact,new,name in [(False,original,'kFastPathFullShader'),(True,peeled,'kFastPathCompactShader')]:
        old='static const char* '+name+' = R"WGSL(\n'+parent.make_fastpath_wgsl(compact).strip()+'\n)WGSL";'
        once(old,'static const char* '+name+' = R"WGSL(\n'+new+'\n)WGSL";')
    begin=s.index('    report["depth0_fastpath_experiment"] = {')
    end=s.index('    wgpuComputePipelineRelease(cpFastFull);',begin)
    s=s[:begin]+'''    report["coarse_peel_experiment"]={{"scope","Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"},
        {"new_math_or_cull",false},{"modes",{{"0","Current"},{"1","Guarded bulk"},{"2","Guarded stream"},{"3","Coarse peel stream"}}}};
'''+s[end:]
    # Export actual global candidate packets using the SAME uninstrumented
    # producer pipeline as timing, outside timed work. No traced math variant.
    trace=r'''
        {
            auto bg=group(cpFastCompact,{entry(0,cfgBuf,0,sizeof(cfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,0,rayBytes),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});
            auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,cpFastCompact);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,(count+63u)/64u,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
            auto hb=readBackBuffer(rBuf,rayBytes),sb=readBackBuffer(sBuf,subpanelBytes),ob=readBackBuffer(oBuf,overlapBytes);
            const auto* hs=reinterpret_cast<const GpuRayHeader*>(hb.data());const auto* ss=reinterpret_cast<const GpuSubpanel*>(sb.data());const auto* os=reinterpret_cast<const GpuOverlap*>(ob.data());
            J packets=J::array();
            for(size_t i=0;i<count;++i){
                if(hs[i].subpanelCount!=gpuRays[i].subpanelCount || hs[i].overlapCount!=gpuRays[i].overlapCount)throw std::runtime_error("coarse candidate header mismatch");
                J singles=J::array(),overs=J::array();
                for(uint32_t k=0;k<hs[i].subpanelCount;++k){const auto& a=ss[hs[i].subpanelOffset+k];const auto& b=gpuSubpanels[gpuRays[i].subpanelOffset+k];
                    if(std::bit_cast<uint32_t>(a.s0)!=std::bit_cast<uint32_t>(b.s0)||std::bit_cast<uint32_t>(a.s1)!=std::bit_cast<uint32_t>(b.s1)||a.mediumIndex!=b.mediumIndex)throw std::runtime_error("coarse candidate single packet mismatch");
                    singles.push_back({a.s0,a.s1,a.mediumIndex});}
                for(uint32_t k=0;k<hs[i].overlapCount;++k){const auto& a=os[hs[i].overlapOffset+k];const auto& b=gpuOverlaps[gpuRays[i].overlapOffset+k];
                    if(std::bit_cast<uint32_t>(a.s0)!=std::bit_cast<uint32_t>(b.s0)||std::bit_cast<uint32_t>(a.s1)!=std::bit_cast<uint32_t>(b.s1)||a.activeMask!=b.activeMask)throw std::runtime_error("coarse candidate overlap packet mismatch");
                    overs.push_back({a.s0,a.s1,a.activeMask});}
                packets.push_back({R[i][0],R[i][1],hs[i].overlapCount,singles,overs});
            }
            report["coarse_packet_census_inputs"]=packets;wgpuBindGroupRelease(bg);
        }
'''
    # Synthetic budget boundaries on BOTH actual pipelines, no RGB inference
    # from the altered geometry. Never modify the real fixture buffers/specs.
    controls=r'''
        J budgetControls=J::array();auto specs=curtainSpecs;
        for(auto& m:specs){m.origin=glm::vec4(1000,1000,1000,0);m.half=glm::vec4(1);m.amps=glm::vec4(0);m.freqs=glm::vec4(0);m.phases=glm::vec4(0);}
        specs[0].origin=glm::vec4(0);specs[0].half=glm::vec4(10);specs[0].limits={2,2,0,.2f};specs[0].curv={0,.001f,0,0};
        std::vector<GpuRayHeader> synthetic(16);for(auto& r:synthetic){r.ro[0]=-.5f;r.ro[1]=0;r.ro[2]=2;r.rd[0]=1;r.rd[1]=r.rd[2]=0;r.clipA=1;r.clipB=2;}
        auto specB=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,specs.size()*sizeof(GpuCurtainSpec),specs.data());
        auto rayB=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst,16*sizeof(GpuRayHeader),synthetic.data());
        for(int variant:{0,1})for(uint32_t budget:{23u,24u,25u}){
            auto u=cfg;u.rayCount=16;u.pad=budget;auto cb=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(u),&u);auto cp=variant?cpFastCompact:cpFastFull;
            wgpuQueueWriteBuffer(gpu.queue,rayB,0,synthetic.data(),16*sizeof(GpuRayHeader));
            auto bg=group(cp,{entry(0,cb,0,sizeof(u)),entry(1,specB,0,specs.size()*sizeof(GpuCurtainSpec)),entry(2,rayB,0,16*sizeof(GpuRayHeader)),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});
            auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,1,1,1);
            wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
            auto hb=readBackBuffer(rayB,16*sizeof(GpuRayHeader));const auto* hs=reinterpret_cast<const GpuRayHeader*>(hb.data());J flags=J::array();
            for(int i=0;i<16;++i){uint32_t expected=budget<24?0x80000001u:0u;if(hs[i].overlapCount!=expected||hs[i].subpanelCount!=0)throw std::runtime_error("coarse exact-budget boundary mismatch");flags.push_back(hs[i].overlapCount);}
            budgetControls.push_back({{"variant",variant},{"budget",budget},{"flags",flags}});wgpuBindGroupRelease(bg);wgpuBufferRelease(cb);
        }
        wgpuBufferRelease(specB);wgpuBufferRelease(rayB);report["coarse_budget_controls"]=budgetControls;
        J capacityControls=J::array();
        for(int kind:{0,1}){
            for(size_t ti=0;ti<tiles.size();++ti){auto u=cfg;u.rayCount=tiles[ti].n;if(kind==0)u.maxSubPerRay=4;else u.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,tileUniforms[4*ti+1],0,&u,sizeof(u));}
            std::vector<GpuResult> poison(executionRays);for(auto& r:poison)for(float& v:r.rgb_T)v=NAN;wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);
            execute(3,1,false);auto res=readResults();auto hb=readBackBuffer(rBuf,rayBytes);const auto* hs=reinterpret_cast<const GpuRayHeader*>(hb.data());J flags=J::array(),output=J::array();uint32_t refused=0;
            for(size_t i=0;i<count;++i){const bool fallback=(hs[i].overlapCount&0x80000000u)!=0;refused+=fallback;
                if(fallback&&(hs[i].overlapCount&0x7fffffffu)!=(kind==0?16u:32u))throw std::runtime_error("coarse capacity reason mismatch");
                const auto& expected=fallback?resCurrent[i]:resUnfused[i];for(int k=0;k<4;++k)if(!std::isfinite(res[i].rgb_T[k])||std::bit_cast<uint32_t>(res[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected.rgb_T[k]))throw std::runtime_error("coarse poisoned capacity output mismatch");
                flags.push_back(hs[i].overlapCount);output.push_back({R[i][0],R[i][1],res[i].rgb_T[0],res[i].rgb_T[1],res[i].rgb_T[2],res[i].rgb_T[3]});}
            if(refused==0)throw std::runtime_error("coarse capacity witness vacuous");capacityControls.push_back({{"kind",kind},{"refused",refused},{"flags",flags},{"output_rows",output}});
        }
        for(size_t ti=0;ti<tiles.size();++ti){auto u=cfg;u.rayCount=tiles[ti].n;wgpuQueueWriteBuffer(gpu.queue,tileUniforms[4*ti+1],0,&u,sizeof(u));}
        report["coarse_capacity_controls"]=capacityControls;correctness(3,false);
'''
    once('        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}',
         trace+controls+'        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}')
    if wide:once('for(uint32_t capacity:{32768u})','for(uint32_t capacity:{65536u})')
    once('    std::cout << report.dump(2) << std::endl;','    std::cout << "Coarse-only report saved." << std::endl;')
    return s


def clocks(report):
    order=[0,1,2,3,3,2,1,0,3,2,1,0,0,1,2,3]*2;out=[]
    cases=report['streamed_schedule']['cases']
    if len(cases)!=1 or cases[0]['capacity'] not in [32768,65536]:raise ValueError('primary case coverage')
    if [s['repeats'] for s in cases[0]['series']]!=[1,2,4]:raise ValueError('repeat coverage')
    for batch in cases[0]['series']:
        n=batch['repeats'];samples=batch['samples']
        if [s['mode'] for s in samples]!=order:raise ValueError('four-mode order')
        for s in samples:
            if any(not math.isfinite(s[k]) or s[k]<0 for k in ['total_ms','encode_ms','completion_ms']):raise ValueError('clock finiteness')
            if not math.isclose(s['total_ms'],s['encode_ms']+s['completion_ms'],rel_tol=1e-12,abs_tol=1e-10):raise ValueError('clock accounting')
        means={str(m):statistics.mean(s['total_ms']/n for s in samples if s['mode']==m) for m in range(4)}
        out.append({'repeats':n,'per_complete_query_ms':means,'candidate_slower_percent_vs_current':100*(means['3']/means['0']-1),
                    'candidate_slower_percent_vs_stream':100*(means['3']/means['2']-1)})
    return out


def validate(root,report,rows,baseline,rays,ref):
    p=load(root,'ray_transport_sliver_review');p.clocks=clocks
    result=p.check(root,report,rows,baseline,rays,ref['rows'],ref['ray_indices']);n=len(rows)
    for c in report['streamed_schedule']['cases']:
        for field,expected in [('mode2_normal_rows',rows),('mode2_forced_rows',baseline)]:
            data=c[field]
            if len(data)!=n or any(a[:2]!=b[:2] or p.bits(a[2:6])!=p.bits(b[2:6]) for a,b in zip(data,expected)):raise ValueError('mode2 output coverage/parity')
    if 'parallel_roots_experiment' in report:
        packets=report['parallel_packet_census_inputs'];original=report['schedule_census_inputs']
        equal=load(root,'ray_transport_root_review').packets_equal
        if len(packets)!=n or any(not equal(a,b,p) for a,b in zip(packets,original)):raise ValueError('actual parallel packet parity')
    if 'coarse_peel_experiment' in report:
        equal=load(root,'ray_transport_root_review').packets_equal
        packets=report['coarse_packet_census_inputs'];original=report['schedule_census_inputs']
        if len(packets)!=n or any(not equal(a,b,p) for a,b in zip(packets,original)):raise ValueError('actual coarse packet parity')
        budgets=report['coarse_budget_controls']
        if [(c['variant'],c['budget']) for c in budgets]!=[(v,b) for v in [0,1] for b in [23,24,25]]:raise ValueError('coarse budget coverage')
        for c in budgets:
            if len(c['flags'])!=16 or any(x!=(0x80000001 if c['budget']<24 else 0) for x in c['flags']):raise ValueError('coarse boundary flags')
        caps=report['coarse_capacity_controls']
        if [c['kind'] for c in caps]!=[0,1]:raise ValueError('coarse capacity coverage')
        for c in caps:
            if len(c['flags'])!=n or len(c['output_rows'])!=n or c['refused']!=sum(bool(x&0x80000000) for x in c['flags']) or c['refused']==0:raise ValueError('coarse raw capacity coverage')
            for flag,a,b,current in zip(c['flags'],c['output_rows'],rows,baseline):
                if flag&0x80000000 and flag&0x7fffffff!=(16 if c['kind']==0 else 32):raise ValueError('coarse capacity reason')
                if a[:2]!=b[:2] or p.bits(a[2:6])!=p.bits((current if flag&0x80000000 else b)[2:6]):raise ValueError('coarse mixed output')
    result['no_full_frame_or_hardware_cause_claim']=True
    return result


def build(root,build_root,work,variant):
    if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
    # Reusing the configured ray build is explicit and allowed only with the
    # same committed AND working engine source. No default-build object grafts.
    trees=[]
    for r in [root,build_root]:
        trees.append(subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=r,text=True).strip())
        if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=r,text=True).strip():raise ValueError('engine working tree differs')
    if trees[0]!=trees[1]:raise ValueError('configured build engine source differs')
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    if variant not in ['parallel','coarse','coarse-wide']:raise ValueError('unsupported native experiment')
    cpp=parallel_source(v['CPP_SOURCE'],root) if variant=='parallel' else coarse_peel_source(v['CPP_SOURCE'],root,wide=(variant=='coarse-wide'))
    work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp)
    (work/'source.json').write_text(json.dumps({'cpp_sha256':sha(work/'probe.cpp'),'zone_sha256':v['ZONE_HASH'],'build_root':str(build_root),'engine_tree':trees[0]},indent=2)+'\n')
    b=build_root/'build';db=json.loads((b/'compile_commands.json').read_text());entry=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3'];subprocess.run(cmd,cwd=b,check=True)
    cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');subprocess.run(cmd,cwd=b,check=True)
    return v['ZONE_HASH']


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--case',type=Path,required=True);p.add_argument('--variant',choices=['parallel','coarse','coarse-wide'],default='parallel');a=p.parse_args()
    root=a.root.resolve();work=a.work.resolve();case=a.case.resolve();wide=load(root,'ray_transport_gpu_wide_stream')
    zone=root/'saves/zones/Northern Veil/zone.ecform';z=sha(zone);meta,rays,refs=wide.verify_case(root,case,z);case_sha=sha(case)
    expected=build(root,a.build_root.resolve(),work,a.variant)
    if z!=expected or sha(zone)!=z or sha(case)!=case_sha:raise ValueError('pinned source/case drift')
    output=work/'result.json';env=dict(os.environ,EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001');env.pop('EC_GPU_GEOMETRY_CASE',None)
    with (work/'native.log').open('x') as f:subprocess.run([str(work/'probe'),str(output)],cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT,check=True)
    if sha(zone)!=z or sha(case)!=case_sha:raise ValueError('source/case changed during execution')
    wide.verify_case(root,case,z)
    report=json.loads(output.read_text());rows=json.loads(Path(str(output)+'-rows.json').read_text());baseline=json.loads(Path(str(output)+'-baseline-rows.json').read_text());ref=json.loads(Path(meta['reference']).read_text())
    checked=validate(root,report,rows,baseline,rays,ref);(work/'independent.json').write_text(json.dumps(checked,indent=2)+'\n');print('PASS fresh native',a.variant,'qualification',work/'independent.json')


if __name__=='__main__':main()
