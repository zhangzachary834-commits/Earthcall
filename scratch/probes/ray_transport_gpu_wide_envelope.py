"""Numerical screen: clip the existing harmonic recipe to its amplitude envelope.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Real |sum harmonic terms|<=sum|amplitudes|, with the same diagnostic margin.
No native enclosure or source-generic derivation granted. No adaptive-bin,
f16, overlap-rule, tolerance, coefficient, clock or production/default change.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 wide=load(root,'ray_transport_gpu_wide_stream');q=load(root,'ray_transport_gpu_pose_qualification');s=wide.source(base,root)
 s=q.once(s,'if(axis==0u){half=min(half,m.limits.x);}if(axis==1u){half=min(half,m.limits.y);}',
  'if(axis==0u){half=min(half,m.limits.x);}if(axis==1u){half=min(half,m.limits.y);}\n   if(axis==2u){half=min(half,abs(m.amps.x)+abs(m.amps.y)+abs(m.amps.z)+m.limits.w);}')
 meta=r'''
    report["harmonic_envelope_trial"]={{"tight_z_support_clip",true},{"bound","sum(abs(amplitudes))+tent_half_width; existing diagnostic margin retained"},
        {"native_execution_enclosure",false},{"generic_source_derivation",false},{"initial_bins",8},{"other_algorithm_changes",false}};
'''
 return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream')
 q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
if __name__=='__main__':main()
