"""Final bounded counsel screen: tight harmonic envelope plus1..16 initial bins.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Existing derivative enclosure, work budget, numerical margin and GL2/overlap
rules remain. No endpoint-change monotonicity shortcut or native authority.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_wide_envelope').source(base,root)
 before=''' for(var bin=0u;bin<8u;bin++){
  var a=lo+(hi-lo)*f32(bin)/8.0;var b=lo+(hi-lo)*f32(bin+1u)/8.0;
  if(bin==0u){a=lo;}if(bin==7u){b=hi;}'''
 after=''' let bins=clamp(u32(ceil((hi-lo)/4.0)),1u,16u);let inverseBins=1.0/f32(bins);
 for(var bin=0u;bin<bins;bin++){
  var a=lo+(hi-lo)*f32(bin)*inverseBins;var b=lo+(hi-lo)*f32(bin+1u)*inverseBins;
  if(bin==0u){a=lo;}if(bin+1u==bins){b=hi;}'''
 s=q.once(s,before,after)
 meta='\n    report["harmonic_envelope_trial"]["initial_bins"]="clamp(ceil(chord/4),1,16)";\n    report["harmonic_envelope_trial"]["other_algorithm_changes"]=true;\n    report["harmonic_envelope_trial"]["monotonicity_enclosure_unchanged"]=true;\n'
 return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream')
 q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
if __name__=='__main__':main()
