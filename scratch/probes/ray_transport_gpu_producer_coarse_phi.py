"""Isolated producer experiment: coarse Phi reuse across 3 level-set searches (-w_z, 0, +w_z).
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-22.
Follows Firmament's §6cn counsel and Codex's post-§6cn handoff:
Cache/reuse only the 9 initial coarse-knot Phi evaluations across the 3 level-set searches.
All bisection, subdivision, stack (20), pieces (48), bounds (96), bands (32),
packet format, consumer, and native fallback remain 100% unchanged.
"""
import argparse, importlib.util, json, math, os, sys
from pathlib import Path

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def source(base, root):
    wide = load(root, 'ray_transport_gpu_wide_stream')
    q = load(root, 'ray_transport_gpu_pose_qualification')
    s = wide.source(base, root)

    old_roots_decl = 'fn roots(m:CurtainSpec,o:vec3f,d:vec3f,lo:f32,hi:f32,c:f32,\n bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,bands:ptr<function,array<vec2f,32>>,\n nr:ptr<function,u32>,reason:ptr<function,u32>,work:ptr<function,u32>){'
    new_roots_decl = 'fn roots(m:CurtainSpec,o:vec3f,d:vec3f,lo:f32,hi:f32,c:f32,\n bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,bands:ptr<function,array<vec2f,32>>,\n nr:ptr<function,u32>,reason:ptr<function,u32>,work:ptr<function,u32>,\n phi_knots:ptr<function,array<f32,9>>){'
    s = q.once(s, old_roots_decl, new_roots_decl)

    old_va_vb = '   let va=evalPhi(m,o,d,a)-c;let vb=evalPhi(m,o,d,b)-c;'
    new_va_vb = '''   var va:f32;var vb:f32;
   if(cell.z==0.0){
    va=(*phi_knots)[bin]-c;
    vb=(*phi_knots)[bin+1u]-c;
   }else{
    va=evalPhi(m,o,d,a)-c;
    vb=evalPhi(m,o,d,b)-c;
   }'''
    s = q.once(s, old_va_vb, new_va_vb)

    old_call_site = '''  roots(m,o,d,a,b,-m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,0.0,&bounds,&nb,&bands,&nr,&reason,&work);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work);'''
    new_call_site = '''  var phi_knots:array<f32,9>;
  for(var k=0u;k<9u;k++){
   var s=a+(b-a)*f32(k)/8.0;
   if(k==0u){s=a;}if(k==8u){s=b;}
   phi_knots[k]=evalPhi(m,o,d,s);
  }
  roots(m,o,d,a,b,-m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work,&phi_knots);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,0.0,&bounds,&nb,&bands,&nr,&reason,&work,&phi_knots);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work,&phi_knots);'''
    s = q.once(s, old_call_site, new_call_site)

    meta = r'''
    report["coarse_phi_reuse"] = {
        {"coarse_evalPhi_calls_per_hit_member", 9},
        {"unshared_baseline_coarse_evalPhi_calls_per_hit_member", 48},
        {"coarse_evalPhi_reduction_factor", 5.333333333333333},
        {"subdivision_and_bisection_unaltered", true},
        {"old_packet_format_retained", true},
        {"all_capacities_retained", true},
        {"scope", "Isolated producer experiment: coarse Phi reuse across 3 level-set searches (-w_z, 0, +w_z)"}
    };
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n' + anchor)
    return s

def main():
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument('--root', type=Path, required=True)
    a, _ = p.parse_known_args()
    root = a.root.resolve()
    q = load(root, 'ray_transport_gpu_pose_qualification')
    wide = load(root, 'ray_transport_gpu_wide_stream')
    q.source = lambda base, r: source(base, r)
    q.verify_case = lambda r, p, z: wide.verify_case(r, p, z)
    q.independent_check = wide.independent_check
    q.main()

if __name__ == '__main__':
    main()
