"""Bounded next-pass experiment: halve private scratch, preserving refusal.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No quality/error change. A smaller capacity must fall back, never truncate.
"""
import argparse,importlib.util
from pathlib import Path

def load(root):
    p=root/'scratch/probes/ray_transport_gpu_producer_repaired.py'
    spec=importlib.util.spec_from_file_location('small_state_base',p)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    m=load(root);s=m.source(base,root)
    a=s.index('static const char* kGpuProducerWgsl=');b=s.index(')WGSL";',a)+len(')WGSL";')
    part=s[a:b]
    for before,after,n in [('array<f32,96>','array<f32,48>',4),('array<vec2f,32>','array<vec2f,16>',3),
        ('*nb+2u>92u','*nb+2u>44u',1),('*nr>=32u','*nr>=16u',1),
        ('array<vec4f,48>','array<vec4f,24>',1),('count>=48u','count>=24u',1)]:
        if part.count(before)!=n:raise ValueError((before,part.count(before),n))
        part=part.replace(before,after)
    s=s[:a]+part+s[b:]
    anchor='    std::cout << report.dump(2) << std::endl;'
    if s.count(anchor)!=1:raise ValueError('report hook')
    s=s.replace(anchor,'    report["private_state_experiment"]={{"bounds",48},{"root_bands",16},{"pieces",24},{"events",48},{"overflow_uses_native_fallback",true}};\n'+anchor)
    return s

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    a,_=p.parse_known_args();root=a.root.resolve();m=load(root);m.source=lambda base,r:source(base,r);m.main()

if __name__=='__main__':main()
