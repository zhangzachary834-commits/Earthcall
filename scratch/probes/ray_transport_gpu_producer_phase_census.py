"""Read the corrected diagnostic's native phase costs; no shipping qualification.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Isolated phase times are not additive and never replace the connected clock.
"""
import argparse,importlib.util
from pathlib import Path

def load(root):
    p=root/'scratch/probes/ray_transport_gpu_producer_repaired.py'
    spec=importlib.util.spec_from_file_location('phase_census_base',p)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    m=load(root);s=m.source(base,root)
    anchor='    J reasons=J::object();unsigned fallbackRays=0;'
    if s.count(anchor)!=1:raise ValueError('phase hook')
    insert=r'''
    // All accuracy rows above have already been captured from the complete
    // pipeline. These isolated controls are deliberately not that pipeline.
    // Closed-form controls changed the five-ray uniform and scratch inputs;
    // restore the full captured workload before measuring any phase.
    wgpuQueueWriteBuffer(gpu.queue,uBuf,0,&udata,sizeof(udata));
    wgpuQueueWriteBuffer(gpu.queue,mBuf,0,gpuMedia.data(),mediaBytes);
    wgpuQueueWriteBuffer(gpu.queue,pBuf,0,globalParamPool.data(),paramBytes);
    wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);
    runPass(cpProducer,bgProducer);
    runPass(cpUnfused,bgUnfused);
    auto restored=readResults();
    for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
        if(restored[i].rgb_T[k]!=resUnfused[i].rgb_T[k])throw std::runtime_error("phase census workload restoration mismatch");
    std::vector<double> producerPhase,partialConsumerPhase,fallbackPhase,pairedPhase;
    for(int block=0;block<6;++block){
        producerPhase.push_back(runBatch(cpProducer,bgProducer,64));
        auto savedFallback=fallbackCP;fallbackCP=nullptr;
        partialConsumerPhase.push_back(runBatch(cpUnfused,bgUnfused,64));
        fallbackCP=savedFallback;
        fallbackPhase.push_back(runBatch(fallbackCP,fallbackBG,64));
        pairedPhase.push_back(runBatch(cpUnfused,bgUnfused,64));
    }
    runPass(cpUnfused,bgUnfused);
    report["isolated_phase_census"]={{"dispatches_per_fence",64},{"full_workload_restore_bitwise_verified",true},
        {"GPU_producer_only_batch_ms",stats(producerPhase)},
        {"partial_A_star_without_native_fallback_batch_ms",stats(partialConsumerPhase)},
        {"native_fallback_only_batch_ms",stats(fallbackPhase)},
        {"A_star_plus_native_fallback_same_compute_pass_batch_ms",stats(pairedPhase)},
        {"scope","Isolated repeated phase controls, host completion-fenced, non-additive; partial A* skips refused rays; no accuracy or shipping claim for isolated phases"}};
'''
    return s.replace(anchor,insert+'\n'+anchor)

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    a,_=p.parse_known_args();root=a.root.resolve();m=load(root);m.source=lambda base,r:source(base,r);m.main()

if __name__=='__main__':main()
