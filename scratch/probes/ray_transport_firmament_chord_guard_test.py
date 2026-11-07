"""Counterexample to the counsel's chord-change >2eps monotonicity implication.
A cosine plus a normalized ray's linear z term, within the producer's wave class.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No native execution/rounding or quadrature certificate follows from this witness.
"""
import argparse,json,math
from pathlib import Path

def witness():
 eps=.001;amplitude=.00799;dz=.006;dx=math.sqrt(1-dz*dz);h=1.
 g=lambda s:dz*s+amplitude*(math.cos(dx*s)-1)
 derivative=lambda s:dz-amplitude*dx*math.sin(dx*s)
 curvature=amplitude*dx*dx;bow=curvature*h*h/8;change=abs(g(h)-g(0))
 min_slope=change/h-(.5*curvature*h+2*eps/h)
 peak=math.asin(dz/(amplitude*dx))/dx
 assert abs(dx*dx+dz*dz-1)<1e-15
 assert bow<=eps and change>2*eps
 assert derivative(0)>0 and derivative(h)<0
 assert 0<peak<h and g(peak)>g(h)
 assert g(.9999)<g(.9998) # independent finite-value path sees the reversal
 assert min_slope<0 # existing derivative enclosure correctly declines monotonicity
 return {'pass':True,'epsilon':eps,'ray_direction':[dx,0,dz],'amplitude':amplitude,'width':h,
         'bow':bow,'endpoint_change':change,'derivatives':[derivative(0),derivative(h)],
         'interior_peak_s':peak,'interior_peak_value':g(peak),'endpoint_value':g(h),
         'existing_min_slope':min_slope,'counsel_implication_refuted':True,
         'scope':'Real-arithmetic counterexample in the cosine+linear level-set class; no GPU/native/error-budget admission'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);a=p.parse_args();result=witness()
 if a.output:
  if a.output.exists():raise ValueError('output exists')
  a.output.write_text(json.dumps(result,indent=2)+'\n')
 print('PASS cosine+normalized-ray chord monotonicity counterexample')
if __name__=='__main__':main()
