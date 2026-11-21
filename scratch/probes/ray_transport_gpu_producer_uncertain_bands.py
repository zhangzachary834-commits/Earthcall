"""Retain complete depth-limited bands as numerical integration work.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
NOT a resolved root, accuracy certificate or native omission admission. Work and
capacity exhaustion still discard the whole schedule and use native fallback.
"""
import argparse,importlib.util
from pathlib import Path

def load(root):
    p=root/'scratch/probes/ray_transport_gpu_producer_repaired.py'
    spec=importlib.util.spec_from_file_location('uncertain_bands_base',p)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    m=load(root);s=m.source(base,root)
    changes=[('if(cell.z>=14.0 || top+2u>20u){*reason|=4u;break;}',
        'if(cell.z>=14.0 || top+2u>20u){putBand(a,b,bounds,nb,bands,nr,reason);continue;}'),
        ('let mid=a+0.5*h;if(mid<=a||mid>=b){*reason|=4u;break;}',
        'let mid=a+0.5*h;if(mid<=a||mid>=b){putBand(a,b,bounds,nb,bands,nr,reason);continue;}')]
    changes.append(('   if(cell.z>=14.0 || top+2u>20u)',
        '   if(0.125*M*h*h<=error){putBand(a,b,bounds,nb,bands,nr,reason);continue;}\n   if(cell.z>=14.0 || top+2u>20u)'))
    for before,after in changes:
        if s.count(before)!=1:raise ValueError('uncertainty hook')
        s=s.replace(before,after)
    before=' if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}'
    after=r''' // Bands describe uncertainty sets, not distinct roots. Merge only
 // intersecting/touching bands; never bridge an unrepresented gap.
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}'''
    if s.count(before)!=1:raise ValueError('band union hook')
    s=s.replace(before,after)
    anchor='    std::cout << report.dump(2) << std::endl;'
    if s.count(anchor)!=1:raise ValueError('report hook')
    meta=r'''
    report["uncertain_band_experiment"]={{"depth_limited_intervals_retained_whole",true},{"touching_uncertainty_bands_unioned_without_gap",true},
        {"uncertainty_is_root_resolution",false},{"proof_subdivision_stops_when_curvature_is_below_existing_error_model",true},{"numerical_quadrature_error_certified",false},
        {"work_and_capacity_exhaustion_use_native_fallback",true},
        {"scope","Fixture numerical experiment: no native omission admission or shipping qualification"}};
'''
    return s.replace(anchor,meta+'\n'+anchor)

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    a,_=p.parse_known_args();root=a.root.resolve();m=load(root);m.source=lambda base,r:source(base,r);m.main()

if __name__=='__main__':main()
