"""Same-device changing-clock uploads and native fallback numerical witness.
Pinned diagnostic, not a full viewport/frame or production lifecycle admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
"""
import argparse,importlib.util,json,os,sys
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    s=load(root,'ray_transport_gpu_prepared_coefficients').source(base,root);q=load(root,'ray_transport_gpu_pose_qualification')
    code=r'''
    // All source/reference/timing observations above are complete. Exercise
    // changing bindings on these SAME recipes, pipelines, buffers and device.
    J clockFrames=J::array();std::vector<GpuResult> firstClockResult;
    const double pairedClock=std::stod(std::getenv("EC_FACTOR_PAIRED_CLOCK"));
    for(int frame=0;frame<5;++frame){
        const bool forced=frame>=3;const double t=(frame==1||frame==3)?pairedClock:clock;
        std::vector<GpuResult> poisoned(executionRays);for(auto& p:poisoned)for(float& v:p.rgb_T)v=NAN;
        wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poisoned.data(),outBytes);
        auto begin=std::chrono::steady_clock::now();
        for(size_t i=0;i<media.size();++i){
            prepareOriginalCoefficients(media[i],coefficientRecipes[i],t);
            for(int k=0;k<3;++k)gpuMedia[i].color[k]=media[i].color[k];gpuMedia[i].color[3]=media[i].alpha;
            currentInstances[i+1].time.x=float(t);
        }
        auto dynamicU=udata;dynamicU.clock=float(t);auto dynamicCurrentU=currentU;dynamicCurrentU.sourceTime.x=float(t);
        auto dynamicCfg=cfg;dynamicCfg.pad=forced?0:cfg.pad;
        wgpuQueueWriteBuffer(gpu.queue,uBuf,0,&dynamicU,sizeof(dynamicU));
        wgpuQueueWriteBuffer(gpu.queue,mBuf,0,gpuMedia.data(),mediaBytes);
        wgpuQueueWriteBuffer(gpu.queue,cuBuf,0,&dynamicCurrentU,sizeof(dynamicCurrentU));
        wgpuQueueWriteBuffer(gpu.queue,ciBuf,0,currentInstances.data(),currentInstances.size()*sizeof(CurrentInstance));
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&dynamicCfg,sizeof(dynamicCfg));
        // Native fallback has its OWN time-prepared values: refresh them too.
        {
            auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
            auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,prepCP);wgpuComputePassEncoderSetBindGroup(pass,0,prepBG,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,media.size(),1,1);
            wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        }
        runBatchConnected(1);
        double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        auto out=readResults();for(const auto& p:out)for(float v:p.rgb_T)if(!std::isfinite(v))throw std::runtime_error("clock replay incomplete output");
        runPass(currentCP,currentBG);auto current=readResults();
        if(frame==0)firstClockResult=out;
        if(frame==2)for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
            if(out[i].rgb_T[k]!=firstClockResult[i].rgb_T[k])throw std::runtime_error("returning clock did not reproduce original output");
        if(forced)for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
            if(out[i].rgb_T[k]!=current[i].rgb_T[k])throw std::runtime_error("changing-clock fallback is stale");
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("clock replay schedule map failed");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned fallbackCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++fallbackCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);
        if(forced&&fallbackCount!=count)throw std::runtime_error("clock replay did not force every native fallback");
        J rows=J::array(),baselineRows=J::array();for(size_t i=0;i<count;++i){
            rows.push_back({R[i][0],R[i][1],out[i].rgb_T[0],out[i].rgb_T[1],out[i].rgb_T[2],out[i].rgb_T[3]});
            baselineRows.push_back({R[i][0],R[i][1],current[i].rgb_T[0],current[i].rgb_T[1],current[i].rgb_T[2],current[i].rgb_T[3]});
        }
        clockFrames.push_back({{"clock",t},{"forced",forced},{"fallback_rays",fallbackCount},{"rows",rows},{"native_current_rows",baselineRows},
            {"host_prepare_upload_preparation_dispatch_and_connected_completion_ms",elapsed}});
    }
    wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
    report["same_device_clock_replay"]={{"frames",clockFrames},{"recipe_source_readmission_in_loop",false},
        {"pipeline_or_device_recreation_in_loop",false},{"output_poisoned_before_each_frame",true},
        {"frame_cost_qualified",false},{"production_revision_device_lifetime_qualified",false}};
'''
    return q.once(s,'    // Independent closed-form witnesses.',code+'\n    // Independent closed-form witnesses.')

def verify_frames(report,first,pair,q):
    frames=report['same_device_clock_replay']['frames'];results=[]
    expected=[(first[1]['time'],False),(pair[1]['time'],False),(first[1]['time'],False),(pair[1]['time'],True),(first[1]['time'],True)]
    if [(f['clock'],f['forced']) for f in frames]!=expected:raise ValueError('clock sequence/coverage')
    for frame in frames:
        data=first if frame['clock']==first[1]['time'] else pair
        rays,refs=data[1:];rows=frame['rows'];current=frame['native_current_rows']
        if len(rows)!=len(refs) or len(current)!=len(refs):raise ValueError('clock replay coverage')
        for v,w,captured in zip(rows,current,rays['rays']):
            if v[:2]!=captured[:2] or w[:2]!=captured[:2]:raise ValueError('clock replay alignment')
            expected=[captured[8],captured[9],captured[10],captured[7]]
            if max(abs(x-y) for x,y in zip(w[2:],expected))>1e-5:raise ValueError('changing-clock native baseline disagrees with capture')
            if frame['forced'] and v!=w:raise ValueError('fallback not exact')
        raw=[v[:2]+v[2:]+v[2:]+v[2:]+[0] for v in rows]
        actual=dict(rays);actual['rays']=[v[:7]+[w[5],w[2],w[3],w[4]]+v[11:] for v,w in zip(rays['rays'],current)]
        result=q.reference_metrics(raw,actual,refs)
        if not result['both_resolutions_pass']:raise ValueError('changing-clock numerical gate')
        results.append({'clock':frame['clock'],'forced':frame['forced'],'checks':result})
    if frames[0]['rows']!=frames[2]['rows'] or frames[0]['rows']==frames[1]['rows']:raise ValueError('clock change/return failed')
    for frame in frames:
        if frame['forced'] and frame['fallback_rays']!=len(first[1]['rays']):raise ValueError('missing forced fallback')
    return results

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);p.add_argument('--paired-case',type=Path)
    a,rest=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification')
    pair=None
    if '--build-only' not in rest:
        if a.paired_case is None:raise ValueError('--paired-case required')
        zone_hash=q.sha(root/'saves/zones/Northern Veil/zone.ecform');pair=q.verify_case(root,a.paired_case.resolve(),zone_hash)
        pair_hash=q.sha(a.paired_case.resolve());primary=Path(rest[rest.index('--case')+1]).resolve();first=q.verify_case(root,primary,zone_hash)
        if first[1]['time']==pair[1]['time'] or first[1]['origin']!=pair[1]['origin'] or [r[:7] for r in first[1]['rays']]!=[r[:7] for r in pair[1]['rays']]:raise ValueError('paired clocks require identical native rays and distinct times')
        os.environ['EC_FACTOR_PAIRED_CLOCK']=str(pair[1]['time'])
    old=sys.argv
    try:
        sys.argv=[old[0],'--root',str(root)]+rest;q.source=lambda base,r:source(base,r);q.main()
    finally:sys.argv=old
    if pair is None:return
    q.verify_case(root,a.paired_case.resolve(),zone_hash)
    if pair_hash!=q.sha(a.paired_case.resolve()):raise ValueError('paired case changed during execution')
    work=Path(rest[rest.index('--work')+1]);label=rest[rest.index('--label')+1] if '--label' in rest else 'normal'
    path=work/(label+'.json');report=json.loads(path.read_text());frames=report['same_device_clock_replay']['frames'];results=[]
    results=verify_frames(report,first,pair,q)
    evidence={'both_manifests':[str(primary),str(a.paired_case.resolve())],'paired_manifest_sha256':pair_hash,'frames':results,'pass':True}
    Path(str(path)+'-clock-independent.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print('PASS same-device changing clock, exact return, and actual native fallback at both clocks')
if __name__=='__main__':main()
