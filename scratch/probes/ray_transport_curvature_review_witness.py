"""Bounded mathematical/source witnesses for the Firmament's post-6ce counsel.
No production rendering, native timing or universal certificate.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import argparse,hashlib,json,math
from pathlib import Path
import msgpack
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
fixture=a.root/'saves/zones/Northern Veil/zone.ecform'
assert hashlib.sha256(fixture.read_bytes()).hexdigest()=='6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496'
zone=json.loads(msgpack.unpackb(fixture.read_bytes(),raw=False)['MigrationRoot'])
fields=zone['spatialFields'];assert [x['origin'][0] for x in fields]==[0.,15.,-5.,22.]
assert fields[0]['scale']==[160.,36.,46.]
D=lambda x:max(.01-x*x,0.)
node_values=[];gl2=0.
for lo,hi in [(-1.,0.),(0.,1.)]:
 for q in [-1/math.sqrt(3),1/math.sqrt(3)]:
  s=(lo+hi)*.5+(hi-lo)*.5*q;node_values.append([s,D(s)]);gl2+=(hi-lo)*.5*D(s)
exact=4*.01**1.5/3;baseline=sum(D(-1+(k+.5)*2/96) for k in range(96))*2/96
assert gl2==0 and abs(baseline-exact)<abs(gl2-exact)
# A secant bow with M=2 rejects a false all-negative endpoint omission.
argument_endpoint=.01-1
bow_upper=argument_endpoint+2*2**2/8
assert bow_upper>0
# The extrema of z-f(x) depend on dz/dx, not just on f's extrema.
slope=.5;curve_extremum=math.pi/2;pullback_extremum=math.acos(slope)
assert abs(slope-math.cos(curve_extremum))>.4
assert abs(slope-math.cos(pullback_extremum))<1e-15
report={
 'scope':'Finite analytic counterexample plus frozen source facts; no C++ build, native rendering or GPU timing.',
 'source_box_and_origins':{'emerald_box_half_extent_as_used_by_probe':fields[0]['scale'],'emerald_density_support_planes_local':{'x':[-75,75],'y':[-16,16]},'member_world_x_origins':[x['origin'][0] for x in fields],'shared_reciprocal_is_not_shared_root':True},
 'retained_midpoint_GL2_counterexample':{'density':'max(0.01-s*s,0)','interval':[-1,1],'retained_panels':[[-1,0],[0,1]],'GL2_nodes_and_D':node_values,'GL2_integral':gl2,'exact_integral':exact,'96_midpoint_integral':baseline,'96_midpoint_abs_error':abs(baseline-exact),'GL2_abs_error':exact},
 'bow_example':{'smooth_argument':'0.01-s*s','endpoints':argument_endpoint,'M':2,'h':2,'bow_upper':bow_upper,'possible_positive_region_retained':True},
 'ray_relative_extrema_example':{'f':'sin(x)','phi':'z0+s*dz-sin(x0+s*dx)','dz_over_dx':slope,'f_extremum_x':curve_extremum,'phi_extremum_x':pullback_extremum},
 'emerald_real_curvature_coefficient':sum(v*k*k for v,k in zip([15,5.5,2],[.026,.068,.16]))}
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
