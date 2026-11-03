"""Check the later Firmament review's explicit header and normalized-ray cone.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Does not rule out compact schedules or correctly enclosed screen tiles.
"""
import argparse,json,math
from pathlib import Path

def witness():
 # His explicit fields cannot overlap without a different documented encoding.
 mask_bits=15*4;minimal_explicit_bits=mask_bits+4+1
 assert minimal_explicit_bits==65 and minimal_explicit_bits>64
 # At fixed Euclidean ray distance, normalized pinhole directions do not
 # interpolate affinely. All eight corner endpoints can miss an interior max.
 directions=[[x/math.sqrt(3),y/math.sqrt(3),1/math.sqrt(3)] for x in [-1,1] for y in [-1,1]]
 corner_z=[s*d[2] for d in directions for s in [1.,2.]]
 interior_far_z=2.
 assert all(abs(sum(v*v for v in d)-1)<1e-15 for d in directions)
 assert interior_far_z>max(corner_z)
 return {'pass':True,'explicit_header':{'mask_bits':mask_bits,'count_bits':4,'fallback_bits':1,'required_bits':minimal_explicit_bits,'proposed_header_bits':64,'scope':'The stated separate-count/flag/nibble layout does not fit; sentinel or other encoding may fit128 bytes'},'normalized_ray_cone':{'corner_directions':directions,'ray_distance_range':[1.,2.],'corner_z_max':max(corner_z),'interior_direction':[0.,0.,1.],'interior_far_z':interior_far_z,'scope':'Eight normalized corner endpoints at fixed ray distances fail to enclose all interior pixels; use a proved enclosure or all-ray reduction'},'no_native_performance_or_rounding_claim':True}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);a=p.parse_args();result=witness()
 if a.output:
  if a.output.exists():raise ValueError('output exists')
  a.output.write_text(json.dumps(result,indent=2)+'\n')
 print('PASS explicit header capacity and normalized-ray cone counterexample')
if __name__=='__main__':main()
