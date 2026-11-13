"""Compact event reconstruction with explicit native fused multiply-add.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
A separate strict-bit experiment after the plain reconstruction was refused.
Original producer/bulk/native control shaders are untouched.
"""
import argparse,importlib.util,re
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_knot_stream').source(base,root)
 for name,tag in [('kKnotConsumer','KNOT'),('kKnotDecompress','ROUND')]:
  pattern=r'(static const char\* '+name+r'=R"'+tag+r'\()(.*?)(\)'+tag+r'";)';m=re.search(pattern,s,re.S)
  if not m:raise ValueError('missing compact shader')
  text=m.group(2);text=q.once(text,'a+f32(k)*(b-a)/f32(n)','fma(f32(k),(b-a)/f32(n),a)');text=q.once(text,'a+f32(k+1u)*(b-a)/f32(n)','fma(f32(k+1u),(b-a)/f32(n),a)');s=s[:m.start(2)]+text+s[m.end(2):]
 s=q.once(s,'{"packet_bytes_per_ray",128}', '{"packet_bytes_per_ray",128},{"endpoint_reconstruction","Explicit fma(k,(b-a)/n,a); full same-device endpoint/output bits remain mandatory"}')
 return s

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream');q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
if __name__=='__main__':main()
