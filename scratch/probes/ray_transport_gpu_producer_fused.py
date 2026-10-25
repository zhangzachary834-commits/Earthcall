"""Bounded next-pass screen: integrate each derived event in the producer invocation.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Reuses pointwise transport helpers; preserves explicit native fallback. Scratch only.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def once(s,a,b):
    if s.count(a)!=1:raise ValueError(('hook count',s.count(a),a[:100]))
    return s.replace(a,b)

def source(base,root):
    repaired=load(root,'ray_transport_gpu_producer_repaired');s=repaired.source(base,root)
    setup=r'''
    // Reuse the established point evaluator and overlap flow; only resource
    // binding and the entrypoint change. No zone formula is added here.
    auto change=[](std::string& text,const std::string& a,const std::string& b){
        auto pos=text.find(a);if(pos==std::string::npos||text.find(a,pos+a.size())!=std::string::npos)throw std::runtime_error("fused hook: "+a);
        text.replace(pos,a.size(),b);
    };
    auto eraseLine=[&](std::string& text,const std::string& needle){
        auto pos=text.find(needle);if(pos==std::string::npos)throw std::runtime_error("fused binding hook");
        auto begin=text.rfind('\n',pos);auto end=text.find('\n',pos);text.erase(begin+1,end-begin);
    };
    std::string fusedCommon=kCommonWgslHeader;
    for(const char* binding:{"var<storage, read> rays:","var<storage, read> subpanels:","var<storage, read> overlaps:"})eraseLine(fusedCommon,binding);
    change(fusedCommon,"struct Subpanel {","struct HistoricalSubpanel {");
    eraseLine(fusedCommon,"var<uniform> u:");
    change(fusedCommon,"@binding(1) var<storage, read> media:","@binding(3) var<storage, read> media:");
    change(fusedCommon,"@binding(2) var<storage, read> params:","@binding(4) var<storage, read> params:");
    change(fusedCommon,"@binding(6) var<storage, read_write> results:","@binding(5) var<storage, read_write> results:");
    std::string fusedBody=kGpuProducerWgsl;
    eraseLine(fusedBody,"var<storage,read_write> subpanels:");eraseLine(fusedBody,"var<storage,read_write> overlaps:");
    change(fusedBody,"fn producerMain(","fn producerFusedMain(");
    change(fusedBody,"var singles=0u;var overs=0u;","var state=vec4f(0,0,0,1);var singles=0u;var overs=0u;");
    change(fusedBody,"subpanels[subBase+singles]=Subpanel(vec2f(l,r),vec2u(member,0));singles++;",R"WGSL(
    let h=r-l;
    let a=projectedAt(ro,d,l+(0.5-sqrt(3.0)/6.0)*h,1u<<member);
    let b=projectedAt(ro,d,l+(0.5+sqrt(3.0)/6.0)*h,1u<<member);
    state=advanceFlow(state,0.5*(a+b),h);singles++;
)WGSL");
    change(fusedBody,"overlaps[overBase+overs]=Overlap(vec2f(a,b),vec2u(mask,0));overs++;",R"WGSL(
    state=advanceOverlap(state,ro,d,OverlapInterval(vec2f(a,b),vec2u(mask,0)));overs++;
)WGSL");
    change(fusedBody,"else {rays[id].counts=vec4u(subBase,singles,overBase,overs);}",
        "else {rays[id].counts=vec4u(subBase,singles,overBase,overs);results[id]=state;}");
    auto fusedShader=fusedCommon+std::string(kOverlapWgsl)+fusedBody;
    auto cpFusedProducer=buildCompute(fusedShader,"producerFusedMain");
    WGPUBindGroup bgFusedProducer;
    {
        WGPUBindGroupEntry e[6]={};WGPUBuffer b[6]={cfgBuf,specBuf,rBuf,mBuf,pBuf,outBuf};
        uint64_t bytes[6]={sizeof(cfg),curtainSpecs.size()*sizeof(GpuCurtainSpec),rayBytes,mediaBytes,paramBytes,outBytes};
        for(unsigned k=0;k<6;++k){e[k].binding=k;e[k].buffer=b[k];e[k].size=bytes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cpFusedProducer,0);WGPUBindGroupDescriptor desc={};desc.layout=layout;desc.entryCount=6;desc.entries=e;
        bgFusedProducer=wgpuDeviceCreateBindGroup(gpu.device,&desc);wgpuBindGroupLayoutRelease(layout);
    }
    if(!outputPath.empty()){std::ofstream out(outputPath+"-fused.wgsl");out<<fusedShader;}
'''
    s=once(s,'    // Helper for completion-fenced execution',setup+'\n    // Helper for completion-fenced execution') if '    // Helper for completion-fenced execution' in s else once(s,'    WGPUComputePipeline fallbackCP=nullptr;',setup+'\n    WGPUComputePipeline fallbackCP=nullptr;')
    # Only connected runs use fusion. Cold producer readback remains available
    # as an independent schedule for AST comparison; ordinary consumer controls
    # and closed-form cases remain unchanged.
    for begin,end in [('    auto runBatchConnected =','    // Helper for readback'),('        auto connectedPass =','        // Standalone Producer Pass')]:
        a=s.index(begin);b=s.index(end,a);part=s[a:b]
        part=once(part,'wgpuComputePassEncoderSetPipeline(pass1, cpProducer);','wgpuComputePassEncoderSetPipeline(pass1, cpFusedProducer);')
        part=once(part,'wgpuComputePassEncoderSetBindGroup(pass1, 0, bgProducer, 0, nullptr);','wgpuComputePassEncoderSetBindGroup(pass1, 0, bgFusedProducer, 0, nullptr);')
        start=part.index('            auto pass2 =')
        stop=part.index('wgpuComputePassEncoderRelease(pass2);',start)+len('wgpuComputePassEncoderRelease(pass2);')
        part=part[:start]+part[stop:]
        s=s[:a]+part+s[b:]
    # Keep gradient-fusion parity on its original identical schedule. Full
    # producer/transport fusion is a separate floating-point program, scored
    # against original AST and independent reference without changing tolerances.
    s=once(s,'    J connected;','    const auto gradientParityUnfused=resUnfused;\n    J connected;')
    a=s.index('    // Arithmetic fidelity comparison');b=s.index('    // Independently evaluate the original authored AST',a)
    part=s[a:b].replace('resUnfused[i]','gradientParityUnfused[i]');s=s[:a]+part+s[b:]
    meta=r'''
    report["scope"]="Pinned fused GPU root-production and transport experiment, explicit native fallback; no native omission/generic admission expansion";
    report["fused_ray_execution"]={{"producer_and_transport_one_invocation",true},{"per_run_intermediate_schedule_writes",0},
        {"cold_reference_schedule_retained_for_validation",true},{"peak_diagnostic_allocation_reduced",false}};
    report["device_arithmetic_fidelity"]["scope"]="Gradient fusion parity on the same cold schedule, not identity of full fused execution";
    wgpuBindGroupRelease(bgFusedProducer);wgpuComputePipelineRelease(cpFusedProducer);
'''
    s=once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')
    return s

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    args,_=p.parse_known_args();root=args.root.resolve();m=load(root,'ray_transport_gpu_producer_repaired')
    m.source=lambda base,r:source(base,r);m.main()

if __name__=='__main__':main()
