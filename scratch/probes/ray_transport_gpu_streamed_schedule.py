"""Fixed-capacity two-pass schedule reuse, exact output and native cost screen.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Firmament's streaming proposal, with original interval packets and fallback.
Sparse unique inputs only; not a full viewport/depth/composition witness.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    s=load(root,'ray_transport_gpu_prepared_coefficients').source(base,root);q=load(root,'ray_transport_gpu_pose_qualification')
    code=r'''
    // Bounded schedule storage only. Frozen input/output buffers still span the
    // finite query; full viewport ray generation/depth/composition is future work.

    // Inspect the ORIGINAL complete GPU schedule, including all overlap members.
    // Do not assume a periodic authored fold intersects a ray in one span.
    J spanExamples=J::array(),spanHistogram=J::object(),scheduleCensusInputs=J::array();unsigned multiSpanPairs=0,maxSpans=0;
    for(size_t i=0;i<count;++i){
        std::array<std::vector<glm::vec2>,4> spans;const auto& ray=gpuRays[i];
        J rawSingles=J::array(),rawOverlaps=J::array();
        if(producerFlags[i]&0x80000000u){scheduleCensusInputs.push_back({R[i][0],R[i][1],producerFlags[i],rawSingles,rawOverlaps});continue;}
        for(unsigned k=0;k<ray.subpanelCount;++k){const auto& p=gpuSubpanels[ray.subpanelOffset+k];spans[p.mediumIndex].push_back({p.s0,p.s1});rawSingles.push_back({p.s0,p.s1,p.mediumIndex});}
        for(unsigned k=0;k<ray.overlapCount;++k){const auto& p=gpuOverlaps[ray.overlapOffset+k];for(unsigned m=0;m<4;++m)if(p.activeMask&(1u<<m))spans[m].push_back({p.s0,p.s1});rawOverlaps.push_back({p.s0,p.s1,p.activeMask});}
        scheduleCensusInputs.push_back({R[i][0],R[i][1],producerFlags[i],rawSingles,rawOverlaps});
        for(unsigned m=0;m<4;++m){
            auto& v=spans[m];std::sort(v.begin(),v.end(),[](auto a,auto b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});
            std::vector<glm::vec2> merged;
            for(auto p:v){if(!merged.empty()&&p.x<=merged.back().y)merged.back().y=std::max(merged.back().y,p.y);else merged.push_back(p);}
            const unsigned n=merged.size();maxSpans=std::max(maxSpans,n);
            auto key=std::to_string(n);spanHistogram[key]=spanHistogram.value(key,0u)+1u;
            if(n>1){++multiSpanPairs;if(spanExamples.size()<8){J intervals=J::array();for(auto p:merged)intervals.push_back({p.x,p.y});
                spanExamples.push_back({{"pixel",{R[i][0],R[i][1]}},{"member",m},{"retained_disjoint_intervals",intervals}});}}
        }
    }
    report["schedule_census_inputs"]=scheduleCensusInputs;
    report["single_span_compression_preflight"]={{"ray_member_pairs",count*4},{"pairs_with_multiple_retained_spans",multiSpanPairs},
        {"maximum_retained_spans_per_member",maxSpans},{"span_count_histogram",spanHistogram},{"examples",spanExamples},
        {"merged_only_touching_or_overlapping",true},{"source_or_omission_authority",false}};
    J streamingCases=J::array();
    for(uint32_t capacity:{176u,512u,1024u,8192u}){
        auto setupBegin=std::chrono::steady_clock::now();
        const uint64_t tileSingleBytes=uint64_t(capacity)*maxSubPerRay*sizeof(GpuSubpanel);
        const uint64_t tileOverlapBytes=uint64_t(capacity)*maxOverPerRay*sizeof(GpuOverlap);
        auto tileSingles=makeBuffer(WGPUBufferUsage_Storage,tileSingleBytes,nullptr);
        auto tileOverlaps=makeBuffer(WGPUBufferUsage_Storage,tileOverlapBytes,nullptr);
        struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback;};
        std::vector<Tile> tiles;std::vector<WGPUBuffer> tileUniforms;
        auto group=[&](WGPUComputePipeline cp,std::vector<WGPUBindGroupEntry> entries){
            auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);
            WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=entries.size();d.entries=entries.data();
            auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);
            if(!bg)throw std::runtime_error("stream bind group refused");return bg;
        };
        auto entry=[](uint32_t binding,WGPUBuffer buffer,uint64_t offset,uint64_t size){
            WGPUBindGroupEntry e={};e.binding=binding;e.buffer=buffer;e.offset=offset;e.size=size;return e;
        };
        for(uint32_t start=0;start<count;start+=capacity){
            const uint32_t n=std::min(capacity,uint32_t(count)-start);
            const uint64_t rayOffset=uint64_t(start)*sizeof(GpuRayHeader),outputOffset=uint64_t(start)*sizeof(GpuResult);
            // Dawn's storage alignment is 256 here; fail rather than misbind.
            if(rayOffset%256||outputOffset%256)throw std::runtime_error("unaligned stream storage offset");
            auto u=udata;u.rayCount=n;auto pcfg=cfg;pcfg.rayCount=n;auto fcfg=pcfg;fcfg.pad=0;
            auto cu=currentU;cu.viewport.z=float(n);
            auto ub=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(u),&u);
            auto cb=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(pcfg),&pcfg);
            auto fb=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(fcfg),&fcfg);
            auto cub=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(cu),&cu);
            tileUniforms.insert(tileUniforms.end(),{ub,cb,fb,cub});
            auto producer=[&](WGPUBuffer configuration){return group(cpProducer,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes)});};
            auto consumer=group(cpUnfused,{entry(0,ub,0,sizeof(u)),entry(1,mBuf,0,mediaBytes),entry(2,pBuf,0,paramBytes),
                entry(3,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(4,tileSingles,0,tileSingleBytes),
                entry(5,tileOverlaps,0,tileOverlapBytes),entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});
            auto current=[&](WGPUComputePipeline cp){return group(cp,{entry(0,cub,0,sizeof(cu)),entry(1,cparamsBuf,0,currentParams.size()*4),
                entry(2,ciBuf,0,currentInstances.size()*sizeof(CurrentInstance)),entry(3,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),
                entry(4,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP)});
        }
        const double setupMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-setupBegin).count();
        // mode 0 current marcher, 1 retained bulk connected, 2 streamed connected.
        // Both command encoding and submit-to-completion are separately observed.
        auto execute=[&](int mode,int repeats,bool forced){
            auto begin=std::chrono::steady_clock::now();auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
            auto dispatch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,uint32_t n){
                auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
                wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(pass,(n+63u)/64u,1,1);
                wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
            };
            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }
            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);
            auto submitted=std::chrono::steady_clock::now();wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
            wgpuDevicePoll(gpu.device,true,nullptr);auto end=std::chrono::steady_clock::now();
            return std::array<double,3>{std::chrono::duration<double,std::milli>(submitted-begin).count(),
                std::chrono::duration<double,std::milli>(end-submitted).count(),std::chrono::duration<double,std::milli>(end-begin).count()};
        };
        // Poison checks and raw output are outside timed batches.
        auto correctness=[&](bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(2,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))throw std::runtime_error("stream changed or missed a result");
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        auto normalRows=correctness(false);auto forcedRows=correctness(true);
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("stream header readback refused");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned forcedCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++forcedCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);
        if(forcedCount!=count)throw std::runtime_error("stream did not force every fallback");
        // Separate warm correctness from measured ABBA/BAAB complete costs.
        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }
        streamingCases.push_back({{"capacity",capacity},{"tiles",tiles.size()},{"tail_rays",tiles.back().n},
            {"schedule_storage_bytes",tileSingleBytes+tileOverlapBytes},{"uniform_buffer_bytes",tiles.size()*(sizeof(udata)+2*sizeof(cfg)+sizeof(currentU))},
            {"binding_buffer_setup_ms",setupMs},{"normal_rows",normalRows},{"forced_rows",forcedRows},
            {"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true},{"forced_bit_identical_to_native",true},{"series",series}});
        for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback})wgpuBindGroupRelease(bg);
        for(auto b:tileUniforms)wgpuBufferRelease(b);wgpuBufferRelease(tileSingles);wgpuBufferRelease(tileOverlaps);
    }
    // Restore global producer offsets/schedules before other diagnostic callers.
    runBatchConnected(1);
    report["streamed_schedule"]={{"cases",streamingCases},{"unique_input_rays",count},{"full_viewport",false},
        {"original_interval_packets_retained",true},{"schedule_buffer_reused_only_after_consumer_and_fallback",true},
        {"world_or_source_admission_changed",false},{"production_lifetime_qualified",false},
        {"timing_scope","Host encoding + submission/completion; matching inputs/dispatch repetition; cold buffer/bind-group setup and correctness readbacks separate"}};
'''
    return q.once(s,'    // Independent closed-form witnesses.',code+'\n    // Independent closed-form witnesses.')

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve()
    q=load(root,'ray_transport_gpu_pose_qualification');q.source=lambda base,r:source(base,r);q.main()
if __name__=='__main__':main()
