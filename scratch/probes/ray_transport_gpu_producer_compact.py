"""Next pass: GPU compact demand list and indirect dispatch for refused rays only.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Uses the same repaired numerical producer; no new omission or source admission.
"""
import argparse,ast,hashlib,importlib.util,json,os,shlex,subprocess
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def once(s,a,b):
    if s.count(a)!=1:raise ValueError(('hook count',s.count(a),a[:100]))
    return s.replace(a,b)

def source(base,root):
    repaired=load(root,'ray_transport_gpu_producer_repaired');s=repaired.source(base,root)
    shader=repaired.WGSL
    queue=r'''
struct FallbackQueue {count:atomic<u32>,groups:atomic<u32>,oneY:u32,oneZ:u32,ids:array<u32>};
@group(0) @binding(5) var<storage,read_write> fallbackQueue:FallbackQueue;
fn enqueueFallback(id:u32,subBase:u32,overBase:u32,reason:u32){
 rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|reason);
 let slot=atomicAdd(&fallbackQueue.count,1u);
 if(slot<cfg.rayCount){fallbackQueue.ids[slot]=id;atomicMax(&fallbackQueue.groups,(slot+64u)/64u);}
}
'''
    shader=once(shader,'fn evalWave(',queue+'\nfn evalWave(')
    shader=once(shader,'rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|1u);return;',
                       'enqueueFallback(id,subBase,overBase,1u);return;')
    # The helper itself contains that assignment: replace only final producer branch.
    shader=shader.replace('if(reason!=0u){rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|reason);}',
                          'if(reason!=0u){enqueueFallback(id,subBase,overBase,reason);}')
    s=once(s,repaired.WGSL,shader)
    queue_setup=r'''
    std::vector<uint32_t> fallbackInitial(count+4,0);fallbackInitial[2]=fallbackInitial[3]=1;
    const bool persistentFallback=std::getenv("EC_GPU_FALLBACK_MODE")&&std::string(std::getenv("EC_GPU_FALLBACK_MODE"))=="persistent";
    const size_t fallbackQueueBytes=fallbackInitial.size()*sizeof(uint32_t);
    auto fallbackQueueBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_Indirect|WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc,
        fallbackQueueBytes,fallbackInitial.data());
'''
    s=once(s,'    auto cpProducer = buildCompute(',queue_setup+'\n    auto cpProducer = buildCompute(')
    s=once(s,'WGPUBindGroupEntry bgpEntries[5] = {};','WGPUBindGroupEntry bgpEntries[6] = {};')
    s=once(s,'    WGPUBindGroupDescriptor bgpd = {};',
               '    bgpEntries[5].binding=5;bgpEntries[5].buffer=fallbackQueueBuf;bgpEntries[5].size=fallbackQueueBytes;\n    WGPUBindGroupDescriptor bgpd = {};')
    s=once(s,'bgpd.entryCount = 5;','bgpd.entryCount = 6;')
    # Reset only count/groupX on the device; the stable indirect Y/Z stay one.
    # Every producer dispatch owns exactly one reset, including producer-only controls.
    reset='wgpuCommandEncoderClearBuffer(enc,fallbackQueueBuf,0,8);\n            '
    assert s.count('auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);')==3
    s=s.replace('auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);',reset+'auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);')
    s=once(s,'wgpuComputePassEncoderSetBindGroup(pass,0,fallbackBG,0,nullptr);\n                wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);',
               'wgpuComputePassEncoderSetBindGroup(pass,0,fallbackBG,0,nullptr);\n                if(persistentFallback)wgpuComputePassEncoderDispatchWorkgroups(pass,1,1,1);else wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,fallbackQueueBuf,4);')
    assert s.count('wgpuComputePassEncoderDispatchWorkgroups(pass3,(count+63)/64,1,1);')==2
    s=s.replace('wgpuComputePassEncoderDispatchWorkgroups(pass3,(count+63)/64,1,1);',
                'if(persistentFallback)wgpuComputePassEncoderDispatchWorkgroups(pass3,1,1,1);else wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass3,fallbackQueueBuf,4);')
    queue_read_wgsl='struct CompactFallback {count:u32,groups:u32,oneY:u32,oneZ:u32,ids:array<u32>};\n@group(0) @binding(5) var<storage,read> fallbackQueue:CompactFallback;\n'
    s=once(s,'std::string fallbackShader=currentShader;',
               'std::string fallbackShader=R"WGSL('+queue_read_wgsl+')WGSL"+currentShader;')
    entry=' let idx=gid.x;if(idx>=u32(u.viewport.z)){return;}'
    newentry=' if(gid.x>=fallbackQueue.count){return;} let idx=fallbackQueue.ids[gid.x];if(idx>=u32(u.viewport.z)){return;}'
    # Replace inside the fallback shader only; the baseline remains verbatim.
    bind=r'''
    const std::string oldEntry=R"WGSL(ENTRY)WGSL";
    auto entryAt=fallbackShader.find(oldEntry);if(entryAt==std::string::npos)throw std::runtime_error("compact fallback entry hook");
    fallbackShader.replace(entryAt,oldEntry.size(),R"WGSL(NEWENTRY)WGSL");
    if(persistentFallback){
        const std::string entryHeader="@compute @workgroup_size(64)\nfn rayMain(@builtin(global_invocation_id) gid:vec3<u32>) {";
        auto at=fallbackShader.find(entryHeader);if(at==std::string::npos)throw std::runtime_error("persistent header hook");
        fallbackShader.replace(at,entryHeader.size(),"fn executeFallback(idx:u32) {");
        const std::string indexedEntry=R"WGSL(NEWENTRY)WGSL";
        at=fallbackShader.find(indexedEntry);if(at==std::string::npos)throw std::runtime_error("persistent index hook");
        fallbackShader.replace(at,indexedEntry.size()," if(idx>=u32(u.viewport.z)){return;}");
        fallbackShader+=R"WGSL(
@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid:vec3<u32>){
 var slot=gid.x;
 loop{if(slot>=fallbackQueue.count){break;}executeFallback(fallbackQueue.ids[slot]);slot+=64u;}
}
)WGSL";
    }
    fallbackCP=buildRaw(fallbackShader,"rayMain");
    {
        WGPUBindGroupEntry e[6]={};WGPUBuffer b[6]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf,fallbackQueueBuf};
        uint64_t bytes[6]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes,fallbackQueueBytes};
        for(unsigned i=0;i<6;++i){e[i].binding=i;e[i].buffer=b[i];e[i].size=bytes[i];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(fallbackCP,0);WGPUBindGroupDescriptor desc={};desc.layout=layout;desc.entryCount=6;desc.entries=e;
        fallbackBG=wgpuDeviceCreateBindGroup(gpu.device,&desc);wgpuBindGroupLayoutRelease(layout);
    }
'''.replace('NEWENTRY',newentry).replace('ENTRY',entry)
    s=once(s,'fallbackCP=buildRaw(fallbackShader,"rayMain");fallbackBG=bindCurrent(fallbackCP);',bind)
    validate=r'''
    J compactQueue;
    {
        auto read=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,fallbackQueueBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc,fallbackQueueBuf,0,read,0,fallbackQueueBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext mc;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&mc;
        wgpuBufferMapAsync(read,WGPUMapMode_Read,0,fallbackQueueBytes,cb);while(!mc.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!mc.ok)throw std::runtime_error("compact queue map failed");
        auto data=static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(read,0,fallbackQueueBytes));
        unsigned flagged=0;for(auto f:producerFlags)flagged+=(f&0x80000000u)!=0;
        if(data[0]!=flagged||data[1]!=(flagged+63)/64||data[2]!=1||data[3]!=1)throw std::runtime_error("indirect queue counters mismatch");
        std::vector<bool> seen(count,false);J ids=J::array();
        for(unsigned i=0;i<data[0];++i){auto id=data[4+i];if(id>=count||seen[id]||!(producerFlags[id]&0x80000000u))throw std::runtime_error("invalid compact fallback id");seen[id]=true;ids.push_back(id);}
        compactQueue={{"fallback_rays",data[0]},{"indirect_workgroups",data[1]},{"bytes",fallbackQueueBytes},{"ids",ids},{"validated",true}};
        wgpuBufferUnmap(read);wgpuBufferRelease(read);
    }
'''
    s=once(s,'    if(!geometryCase.empty()){\n        unsigned positiveChecks=0,fallbacks=0;',validate+'\n    if(!geometryCase.empty()){\n        unsigned positiveChecks=0,fallbacks=0;')
    meta=r'''
    report["scope"]="Corrected pinned GPU producer with compact demand-driven native fallback; no source/numeric admission expansion";
    report["compact_fallback_queue"]=compactQueue;
    report["compact_fallback_queue"]["dispatch_mode"]=persistentFallback?"one direct persistent workgroup":"indirect workgroups";
    report["gpu_canonical_producer"]["compact_indirect_fallback"]=true;
    wgpuBufferRelease(fallbackQueueBuf);
'''
    s=once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')
    return s

def main():
    # Keep exactly the repaired runner's validation/qualification protocol.
    import sys
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    args,_=p.parse_known_args();root=args.root.resolve()
    repaired=load(root,'ray_transport_gpu_producer_repaired')
    repaired.source=lambda base,r:source(base,r)
    # source() above loads a fresh repaired module, avoiding recursive patching.
    repaired.main()

if __name__=='__main__':main()
