"""128-byte full-f32 chronological event stream; no renderer admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Firmament's unchopped-event proposal, preserving original root and panel rules.
Count occupies the last four mask bits; count0/mask0=1 is fallback sentinel.
"""
import argparse,importlib.util,re
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_wide_stream').source(base,root)
 def raw(name):
  m=re.search(r'static const char\* '+name+r'\s*=\s*R"WGSL\((.*?)\)WGSL";',s,re.S)
  if not m:raise ValueError('missing raw WGSL '+name)
  return m.group(1)
 producer=raw('kGpuProducerWgsl')
 producer=q.once(producer,'struct Subpanel {span:vec2f,info:vec2u};','struct KnotPacket {spans:array<vec2f,15>,control:vec2u};')
 producer=q.once(producer,'@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;','@group(0) @binding(3) var<storage,read_write> knots:array<KnotPacket>;')
 producer=q.once(producer,'@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;','')
 producer=q.once(producer,'if(cfg.budget==0u){rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|1u);return;}', 'if(cfg.budget==0u){knots[id].control=vec2u(1u,0u);rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|1u);return;}')
 start=producer.index(' var singles=0u;var overs=0u;')
 producer=producer[:start]+r'''
 var singles=0u;var overs=0u;var spanCount=0u;var packed=vec2u(0u);
 for(var i=1u;i<ne&&reason==0u;i++){
  let a=events[i-1u];let b=events[i];if(!(b>a)){continue;}
  let mid=a+0.5*(b-a);var mask=0u;
  for(var k=0u;k<count;k++){if(mid>=pieces[k].x&&mid<=pieces[k].y){mask|=1u<<u32(pieces[k].z);}}
  if(mask==0u){continue;}
  // Retain original capacity refusals as well as the new packet refusal.
  if(countOneBits(mask)==1u){let n=max(1u,u32(ceil((b-a)/4.0)));if(singles+n>cfg.maxSubPerRay){reason|=16u;break;}singles+=n;}
  else {if(overs>=cfg.maxOverPerRay){reason|=32u;break;}overs++;}
  if(spanCount>=15u){reason|=64u;break;}
  knots[id].spans[spanCount]=vec2f(a,b);
  let word=spanCount/8u;let shift=(spanCount%8u)*4u;packed[word]|=mask<<shift;spanCount++;
 }
 if(reason!=0u){knots[id].control=vec2u(1u,0u);rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|reason);}
 else {packed.y|=spanCount<<28u;knots[id].control=packed;rays[id].counts=vec4u(subBase,singles,overBase,overs);}
}
'''
 header=raw('kCommonWgslHeader')
 header=q.once(header,'@group(0) @binding(4) var<storage, read> subpanels: array<Subpanel>;', 'struct KnotPacket {spans:array<vec2f,15>,control:vec2u};\n@group(0) @binding(4) var<storage,read> knots:array<KnotPacket>;')
 header=q.once(header,'@group(0) @binding(5) var<storage, read> overlaps: array<OverlapInterval>;','')
 consumer=raw('kUnfusedShader');body=consumer[consumer.index('        let sub_h = s1 - s0;'):consumer.index('\n    results[rayIdx]')]
 # Last closing brace in this slice belongs to the old merged-stream loop.
 body=body.rsplit('    }',1)[0]
 body=q.once(body,'        let mid = sub.info.x;','        let mid = firstTrailingBit(mask);')
 start=consumer.index('    // Two sorted streams');consumer=consumer[:start]+r'''
    let packed=knots[rayIdx].control;let spanCount=packed.y>>28u;
    if(spanCount==0u && packed.x!=0u){return;}
    for(var spanIndex=0u;spanIndex<spanCount;spanIndex++){
        let mask=(packed[spanIndex/8u]>>((spanIndex%8u)*4u))&15u;
        let a=knots[rayIdx].spans[spanIndex].x;let b=knots[rayIdx].spans[spanIndex].y;
        if(countOneBits(mask)>1u){
            let state=advanceOverlap(vec4f(L,T),ro,rd,OverlapInterval(vec2f(a,b),vec2u(mask,0u)));
            L=state.xyz;T=state.w;continue;
        }
        let n=max(1u,u32(ceil((b-a)/4.0)));
        for(var k=0u;k<n;k++){
            let s0=select(a+f32(k)*(b-a)/f32(n),a,k==0u);
            let s1=select(a+f32(k+1u)*(b-a)/f32(n),b,k+1u==n);
'''+body+r'''
        }
    }
    results[rayIdx] = vec4f(L,T);
}
'''
 cpp='\nstatic const char* kKnotProducer=R"KNOT('+producer+')KNOT";\nstatic const char* kKnotConsumer=R"KNOT('+header+'\n'+raw('kOverlapWgsl')+'\n'+consumer+')KNOT";\n'
 s=q.once(s,'using M = OntoMath::MathNode;',cpp+'\nusing M = OntoMath::MathNode;')
 # Alter ONLY the bounded experimental lane; original bulk/native controls stay.
 begin=s.index('    J streamingCases=J::array();');end=s.index('    // Independent closed-form witnesses.',begin)
 block=s[begin:end]
 block=q.once(block,'    J streamingCases=J::array();',r'''
    auto knotProducer=buildCompute(kKnotProducer,"producerMain");auto knotConsumer=buildCompute(kKnotConsumer,"rayMain");
    struct KnotPacket {float spans[15][2];uint32_t control[2];};static_assert(sizeof(KnotPacket)==128);
    auto packetMirror=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,count*sizeof(KnotPacket),nullptr);
    auto readBytes=[&](WGPUBuffer source,uint64_t bytes){
        auto dest=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,bytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,source,0,dest,0,bytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(dest,WGPUMapMode_Read,0,bytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("knot readback refused");
        std::vector<uint8_t> result(bytes);std::memcpy(result.data(),wgpuBufferGetConstMappedRange(dest,0,bytes),bytes);wgpuBufferUnmap(dest);wgpuBufferRelease(dest);return result;
    };
    J streamingCases=J::array();''')
 block=q.once(block,'for(uint32_t capacity:{8192u,16384u,32768u})','for(uint32_t capacity:{8192u,32768u})')
 block=q.once(block,'const uint64_t tileSingleBytes=uint64_t(capacity)*maxSubPerRay*sizeof(GpuSubpanel);','const uint64_t tileSingleBytes=uint64_t(capacity)*sizeof(KnotPacket);')
 block=q.once(block,'const uint64_t tileOverlapBytes=uint64_t(capacity)*maxOverPerRay*sizeof(GpuOverlap);','const uint64_t tileOverlapBytes=0;')
 block=q.once(block,'auto tileSingles=makeBuffer(WGPUBufferUsage_Storage,tileSingleBytes,nullptr);','auto tileSingles=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,tileSingleBytes,nullptr);')
 block=q.once(block,'auto tileOverlaps=makeBuffer(WGPUBufferUsage_Storage,tileOverlapBytes,nullptr);','')
 block=q.once(block,'return group(cpProducer,{','return group(knotProducer,{')
 block=q.once(block,'entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes)','entry(3,tileSingles,0,tileSingleBytes)')
 block=q.once(block,'auto consumer=group(cpUnfused,','auto consumer=group(knotConsumer,')
 block=q.once(block,'entry(5,tileOverlaps,0,tileOverlapBytes),entry(6,outBuf','entry(6,outBuf')
 block=q.once(block,'auto execute=[&](int mode,int repeats,bool forced){','auto execute=[&](int mode,int repeats,bool forced,bool capturePackets=false){')
 block=q.once(block,'dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);\n                    dispatch(cpUnfused,tile.consumer,tile.n);', '''dispatch(knotProducer,forced?tile.forced:tile.producer,tile.n);
                    if(capturePackets)wgpuCommandEncoderCopyBufferToBuffer(enc,tileSingles,0,packetMirror,uint64_t(tile.start)*sizeof(KnotPacket),uint64_t(tile.n)*sizeof(KnotPacket));
                    dispatch(knotConsumer,tile.consumer,tile.n);''')
 start=block.index('        auto correctness=[&](bool forced){');stop=block.index('        auto rb=makeBuffer',start)
 block=block[:start]+r'''
        auto correctness=[&](bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(2,1,forced,true);auto result=readResults();
            auto headerBytes=readBytes(rBuf,rayBytes);std::vector<GpuRayHeader> headers(count);std::memcpy(headers.data(),headerBytes.data(),count*sizeof(GpuRayHeader));
            MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
            wgpuBufferMapAsync(packetMirror,WGPUMapMode_Read,0,count*sizeof(KnotPacket),cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
            if(!map.ok)throw std::runtime_error("knot mirror map refused");
            std::vector<KnotPacket> packets(count);std::memcpy(packets.data(),wgpuBufferGetConstMappedRange(packetMirror,0,count*sizeof(KnotPacket)),count*sizeof(KnotPacket));wgpuBufferUnmap(packetMirror);
            J rows=J::array(),packetRows=J::array();unsigned fallbacks=0,capacityFallbacks=0,expanded=0;
            auto same=[](float a,float b){return std::bit_cast<uint32_t>(a)==std::bit_cast<uint32_t>(b);};
            for(size_t i=0;i<count;++i){
                const auto flag=headers[i].overlapCount;const bool refused=(flag&0x80000000u)!=0;const auto& packet=packets[i];const unsigned n=packet.control[1]>>28u;
                if(refused){fallbacks++;capacityFallbacks+=bool(flag&64u);if(packet.control[0]!=1||packet.control[1]!=0)throw std::runtime_error("invalid knot fallback sentinel");}
                else if(forced)throw std::runtime_error("knot forced fallback missing");
                const auto& expected=refused?resCurrent:resUnfused;
                for(int k=0;k<4;++k)if(!std::isfinite(result[i].rgb_T[k])||!same(result[i].rgb_T[k],expected[i].rgb_T[k]))throw std::runtime_error("knot output changed or missed");
                unsigned single=0,overlap=0;J spans=J::array();
                for(unsigned j=0;j<n;++j){
                    const unsigned mask=(packet.control[j/8]>>((j%8)*4))&15u;const float a=packet.spans[j][0],b=packet.spans[j][1];
                    if(!mask||!std::isfinite(a)||!std::isfinite(b)||!(b>a))throw std::runtime_error("invalid knot span");
                    spans.push_back({a,b,mask});
                    if(std::popcount(mask)==1){
                        const unsigned panels=std::max(1u,unsigned(std::ceil((b-a)/4.f)));const unsigned member=std::countr_zero(mask);
                        for(unsigned k=0;k<panels;++k){
                            if(single>=gpuRays[i].subpanelCount)throw std::runtime_error("extra knot panel");
                            const float l=k==0?a:a+float(k)*(b-a)/float(panels),r=k+1==panels?b:a+float(k+1)*(b-a)/float(panels);
                            const auto& want=gpuSubpanels[gpuRays[i].subpanelOffset+single++];
                            if(!same(l,want.s0)||!same(r,want.s1)||member!=want.mediumIndex)throw std::runtime_error("knot changed panel endpoint bits");expanded++;
                        }
                    }else{
                        if(overlap>=gpuRays[i].overlapCount)throw std::runtime_error("extra knot overlap");const auto& want=gpuOverlaps[gpuRays[i].overlapOffset+overlap++];
                        if(!same(a,want.s0)||!same(b,want.s1)||mask!=want.activeMask)throw std::runtime_error("knot changed overlap");
                    }
                }
                if(!refused&&(single!=gpuRays[i].subpanelCount||overlap!=gpuRays[i].overlapCount))throw std::runtime_error("knot omitted panels");
                rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
                packetRows.push_back({R[i][0],R[i][1],flag,spans,packet.control[0],packet.control[1]});
            }
            return J{{"rows",rows},{"packets",packetRows},{"fallback_rays",fallbacks},{"capacity_fallback_rays",capacityFallbacks},{"expanded_single_panels_bit_checked",expanded}};
        };
        auto normal=correctness(false);auto forced=correctness(true);auto normalRows=normal["rows"];auto forcedRows=forced["rows"];
'''+block[stop:]
 block=q.once(block,'{"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true}', '{"forced_fallback_rays",forcedCount},{"normal_fallback_rays",normal["fallback_rays"]},{"capacity_fallback_rays",normal["capacity_fallback_rays"]},{"normal_packets",normal["packets"]},{"forced_packets",forced["packets"]},{"expanded_single_panels_bit_checked",normal["expanded_single_panels_bit_checked"]},{"eligible_bit_identical_to_bulk_candidate",true}')
 block=q.once(block,'wgpuBufferRelease(tileOverlaps);','')
 block=q.once(block,'{"original_interval_packets_retained",true}', '{"original_interval_packets_retained",false},{"full_precision_intervals_and_original_panel_nodes",true},{"packet_bytes_per_ray",128},{"max_events_per_ray",15},{"fallback_encoding","count0/control.x1; normal empty is both controls0; native ray flag remains explicit"}')
 block+='\n    wgpuBufferRelease(packetMirror);wgpuComputePipelineRelease(knotProducer);wgpuComputePipelineRelease(knotConsumer);\n'
 s=s[:begin]+block+s[end:];s=q.once(s,'#include <bit>','#include <bit>\n#include <cstring>')
 decompressor=r"""
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct KnotPacket {spans:array<vec2f,15>,control:vec2u};
struct Packet {span:vec2f,info:vec2u};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> knots:array<KnotPacket>;
@group(0) @binding(2) var<storage,read> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> singles:array<Packet>;
@group(0) @binding(4) var<storage,read_write> overlaps:array<Packet>;
@compute @workgroup_size(64)
fn decompressMain(@builtin(global_invocation_id) gid:vec3u){
 let id=gid.x;if(id>=cfg.rayCount||(rays[id].counts.w&0x80000000u)!=0u){return;}
 let packed=knots[id].control;let count=packed.y>>28u;var single=0u;var over=0u;
 for(var j=0u;j<count;j++){
  let a=knots[id].spans[j].x;let b=knots[id].spans[j].y;let mask=(packed[j/8u]>>((j%8u)*4u))&15u;
  if(countOneBits(mask)==1u){
   let n=max(1u,u32(ceil((b-a)/4.0)));let member=firstTrailingBit(mask);
   for(var k=0u;k<n;k++){
 let l=select(a+f32(k)*(b-a)/f32(n),a,k==0u);let r=select(a+f32(k+1u)*(b-a)/f32(n),b,k+1u==n);
 singles[id*cfg.maxSubPerRay+single]=Packet(vec2f(l,r),vec2u(member,0u));single++;
   }
  }else {overlaps[id*cfg.maxOverPerRay+over]=Packet(vec2f(a,b),vec2u(mask,0u));over++;}
 }
}
"""
 s=q.once(s,'using M = OntoMath::MathNode;', 'static const char* kKnotDecompress=R"ROUND('+decompressor+')ROUND";\nusing M = OntoMath::MathNode;')
 s=q.once(s,'auto knotProducer=buildCompute(kKnotProducer,"producerMain");', 'auto knotDecompress=buildCompute(kKnotDecompress,"decompressMain");\n    auto expandedSingles=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,subpanelBytes,nullptr);\n    auto expandedOverlaps=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,overlapBytes,nullptr);\n    auto knotProducer=buildCompute(kKnotProducer,"producerMain");')
 s=q.once(s,'WGPUBindGroup producer,forced,consumer,fallback;', 'WGPUBindGroup producer,forced,consumer,fallback,decompress;')
 s=q.once(s,'tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP)});',r"""auto decomp=group(knotDecompress,{entry(0,cb,0,sizeof(pcfg)),entry(1,tileSingles,0,tileSingleBytes),
             entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),
             entry(3,expandedSingles,uint64_t(start)*maxSubPerRay*sizeof(GpuSubpanel),uint64_t(n)*maxSubPerRay*sizeof(GpuSubpanel)),
             entry(4,expandedOverlaps,uint64_t(start)*maxOverPerRay*sizeof(GpuOverlap),uint64_t(n)*maxOverPerRay*sizeof(GpuOverlap))});
         tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),decomp});""")
 s=q.once(s,'if(capturePackets)wgpuCommandEncoderCopyBufferToBuffer(enc,tileSingles,0,packetMirror,uint64_t(tile.start)*sizeof(KnotPacket),uint64_t(tile.n)*sizeof(KnotPacket));',r"""if(capturePackets){
                     wgpuCommandEncoderCopyBufferToBuffer(enc,tileSingles,0,packetMirror,uint64_t(tile.start)*sizeof(KnotPacket),uint64_t(tile.n)*sizeof(KnotPacket));
                     dispatch(knotDecompress,tile.decompress,tile.n);
                 }""")
 s=q.once(s,'J rows=J::array(),packetRows=J::array();unsigned fallbacks=0,capacityFallbacks=0,expanded=0;',r"""auto sb=readBytes(expandedSingles,subpanelBytes);auto ob=readBytes(expandedOverlaps,overlapBytes);
         std::vector<GpuSubpanel> decompressedSingles(gpuSubpanels.size());std::memcpy(decompressedSingles.data(),sb.data(),sb.size());
         std::vector<GpuOverlap> decompressedOverlaps(gpuOverlaps.size());std::memcpy(decompressedOverlaps.data(),ob.data(),ob.size());
         J rows=J::array(),packetRows=J::array(),hostExamples=J::array();unsigned fallbacks=0,capacityFallbacks=0,expanded=0,hostEndpointDifferences=0;""")
 s=q.once(s,'if(!same(l,want.s0)||!same(r,want.s1)||member!=want.mediumIndex)throw std::runtime_error("knot changed panel endpoint bits");expanded++;',r"""if(!same(l,want.s0)||!same(r,want.s1)){
                             hostEndpointDifferences++;
                             if(hostExamples.size()<4)hostExamples.push_back({{"ray",i},{"panel",single-1},{"CPU",{l,r}},{"original_GPU",{want.s0,want.s1}}});
                         }
                         const auto& native=decompressedSingles[i*maxSubPerRay+single-1];
                         if(!same(native.s0,want.s0)||!same(native.s1,want.s1)||native.mediumIndex!=want.mediumIndex||member!=want.mediumIndex)throw std::runtime_error("GPU knot changed panel endpoint bits");expanded++;""")
 s=q.once(s,'if(!same(a,want.s0)||!same(b,want.s1)||mask!=want.activeMask)throw std::runtime_error("knot changed overlap");',r"""const auto& native=decompressedOverlaps[i*maxOverPerRay+overlap-1];
                     if(!same(a,want.s0)||!same(b,want.s1)||mask!=want.activeMask||!same(native.s0,want.s0)||!same(native.s1,want.s1)||native.activeMask!=want.activeMask)throw std::runtime_error("GPU knot changed overlap");""")
 s=q.once(s,'{"expanded_single_panels_bit_checked",expanded}};', '{"expanded_single_panels_bit_checked",expanded},{"host_endpoint_differences",hostEndpointDifferences},{"host_endpoint_examples",hostExamples}};')
 s=q.once(s,'{"expanded_single_panels_bit_checked",normal["expanded_single_panels_bit_checked"]}', '{"expanded_single_panels_bit_checked",normal["expanded_single_panels_bit_checked"]},{"host_endpoint_differences",normal["host_endpoint_differences"]},{"host_endpoint_examples",normal["host_endpoint_examples"]},{"same_device_decompression_bit_checked",true}')
 s=q.once(s,'tile.producer,tile.forced,tile.consumer,tile.fallback}', 'tile.producer,tile.forced,tile.consumer,tile.fallback,tile.decompress}')
 s=q.once(s,'wgpuBufferRelease(packetMirror);','wgpuBufferRelease(expandedSingles);wgpuBufferRelease(expandedOverlaps);wgpuComputePipelineRelease(knotDecompress);wgpuBufferRelease(packetMirror);')
 s=q.once(s,'std::vector<GpuSubpanel> decompressedSingles(gpuSubpanels.size());', 'if(sb.size()%sizeof(GpuSubpanel)||ob.size()%sizeof(GpuOverlap))throw std::runtime_error("unaligned knot diagnostic bytes");\n            std::vector<GpuSubpanel> decompressedSingles(sb.size()/sizeof(GpuSubpanel));')
 s=q.once(s,'std::vector<GpuOverlap> decompressedOverlaps(gpuOverlaps.size());', 'std::vector<GpuOverlap> decompressedOverlaps(ob.size()/sizeof(GpuOverlap));')
 s=q.once(s,'auto knotDecompress=buildCompute(kKnotDecompress,"decompressMain");', 'std::cout<<"KNOT compiling native decompression/producer/consumer"<<std::endl;\n    auto knotDecompress=buildCompute(kKnotDecompress,"decompressMain");')
 s=q.once(s,'auto correctness=[&](bool forced){',r"""auto refuse=[&](const char* reason,J detail){std::ofstream file(outputPath+"-knot-refusal.json");file<<J{{"reason",reason},{"detail",detail},{"scope","Exact native check refused before candidate timing"}}.dump(2)<<"\n";throw std::runtime_error(reason);};
        auto correctness=[&](bool forced){""")
 s=q.once(s,'throw std::runtime_error("knot output changed or missed");',r"""refuse("knot output changed or missed",{{"ray",i},{"channel",k},{"actual",result[i].rgb_T[k]},{"expected",expected[i].rgb_T[k]},{"flags",flag}});""")
 s=q.once(s,'throw std::runtime_error("GPU knot changed panel endpoint bits");',r"""refuse("GPU knot changed panel endpoint bits",{{"ray",i},{"panel",single-1},{"packet_span",{a,b}},{"panel_count",panels},{"GPU_decompressed",{native.s0,native.s1}},{"original_GPU",{want.s0,want.s1}}});""")
 # Save accepted native expansion outside timings, independently replayable.
 s=q.once(s,'J rows=J::array(),packetRows=J::array(),hostExamples=J::array();','J rows=J::array(),packetRows=J::array(),expandedRows=J::array(),hostExamples=J::array();')
 s=q.once(s,'unsigned single=0,overlap=0;J spans=J::array();','unsigned single=0,overlap=0;J spans=J::array(),nativeSingles=J::array(),nativeOverlaps=J::array();')
 s=q.once(s,'expanded++;\n', 'nativeSingles.push_back({native.s0,native.s1,native.mediumIndex});expanded++;\n')
 s=q.once(s,'throw std::runtime_error("GPU knot changed overlap");', 'throw std::runtime_error("GPU knot changed overlap");\n                        nativeOverlaps.push_back({native.s0,native.s1,native.activeMask});')
 s=q.once(s,'packetRows.push_back({R[i][0],R[i][1],flag,spans,packet.control[0],packet.control[1]});','packetRows.push_back({R[i][0],R[i][1],flag,spans,packet.control[0],packet.control[1]});\n                if(!forced)expandedRows.push_back({R[i][0],R[i][1],flag,nativeSingles,nativeOverlaps});')
 s=q.once(s,'{"rows",rows},{"packets",packetRows}', '{"rows",rows},{"packets",packetRows},{"expanded_packets",expandedRows}')
 s=q.once(s,'{"normal_packets",normal["packets"]}', '{"normal_packets",normal["packets"]},{"GPU_expanded_packets",normal["expanded_packets"]}')
 s=q.once(s,'throw std::runtime_error(reason);};','file.flush();file.close();std::cerr<<detail.dump()<<std::endl;throw std::runtime_error(reason);};')
 s=q.once(s,'{"packet_span",{a,b}},{"panel_count",panels}', '{"packet_span",{a,b}},{"span_index",j},{"panel_in_span",k},{"panel_count",panels}')
 return s

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream');q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
if __name__=='__main__':main()
