"""Replay the scoped native multi-view pilot; never a full-population gate.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import copy
import hashlib
import json
import math
import os
from pathlib import Path,PurePosixPath
import struct
import tempfile
import zipfile
import ray_transport_6ew_pilot as P
import ray_transport_6ep_reference_derivation as D
R=P.R


def need(v,msg):
 if not v:raise ValueError(msg)


def stats(values):
 v=sorted(values);need(v and all(math.isfinite(x) for x in v),'finite error metrics');return dict(mean=sum(v)/len(v),p99=v[len(v)*99//100],max=v[-1])


def errors(rows,refs,offset):
 t=[];components=[];maxima=[]
 for r,q in zip(rows,refs):
  t.append(abs(r[7]-q[offset]));rgb=[abs(r[8+k]-q[offset+1+k]) for k in range(3)];components+=rgb;maxima.append(max(rgb))
 return dict(T=stats(t),RGB_max_per_ray=stats(maxima),RGB_components=stats(components))


def compare(current,candidate):return {kind:{s:candidate[kind][s]<=current[kind][s] for s in current[kind]} for kind in current}


def reference_binding(receipt,job,values,path,build):
 need(receipt['exit_code']==0 and receipt['before']==receipt['after'],'CPU process/source')
 need(receipt['input_sha256']==receipt['input_sha256_after']==job['input_sha256']==R.sha(path),'own/common reference pairing')
 need(receipt['before']['build_sha256']==R.sha(build/'build.json') and receipt['before']['source_sha256']==R.read(build/'build.json')['executables']['worker']['source_sha256'],'compiled reference identity')
 meta=R.read(build/'build.json');need(receipt['before']['engine']==meta['after'] and receipt['before']['binary_sha256']==meta['executables']['worker']['binary_sha256'],'reference executable/engine identity')
 need(receipt['argv'][2:5]==['16','80','8'] and 0<receipt['elapsed_seconds']<=180,'fixed reference execution scope')
 need(len(values['rows'])==job['count']==80 and values['columns']==['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'],'reference coverage')
 need(all(len(q)==8 and all(math.isfinite(x) for x in q) for q in values['rows']),'finite reference rows')
 conv=max(abs(q[k]-q[k+4]) for q in values['rows'] for k in range(4));need(conv<=1e-6,'reference convergence');return conv


def summarize(native,pilot,build):
 selection=R.read(pilot/'selection.json');native_meta=R.read(native/'build.json');cpu_meta=R.read(build/'build.json');need(native_meta['after']==cpu_meta['after'],'native/CPU engine binding')
 budget=R.read(build/'budget.json');need(budget['rows']==916 and budget['seconds']<=600,'declared total reference cap')
 for lane in ['canonical','worker']:
  text=(build/(lane+'.cpp')).read_text();expected=(R.ROOT/'scratch/probes/veil_projected_reference.cpp').read_text();need(text==(expected if lane=='canonical' else D.derive_source(expected)),'closed CPU math derivation')
 need(cpu_meta['math_source_sha256']==R.sha(R.ROOT/'src/Singularity/OntoMath/ScalarForm.cpp'),'CPU evaluator source')
 continuity=pilot.parent/'continuity';old=R.read(continuity/'historical.json');canonical=R.read(continuity/'canonical.json');worker=R.read(continuity/'worker.json')
 encode=lambda v:b''.join(struct.pack('<d',x) for row in v['rows'] for x in row)
 need(len(old['rows'])==18 and old['columns']==canonical['columns']==worker['columns'] and encode(old)==encode(canonical)==encode(worker),'historical/canonical/worker continuity')
 receipts=[R.read(pilot/(j['name']+'-reference.receipt.json')) for j in selection['jobs']]+[R.read(continuity/(name+'.receipt.json')) for name in ['canonical','worker']]
 need(sum(r['elapsed_seconds'] for r in receipts)==budget['seconds'],'reference total budget receipt accounting')
 for j in selection['jobs']:
  name=j['name']+'-reference';rr=R.read(pilot/(name+'.receipt.json'));need(R.sha(pilot/(name+'.log'))==rr['log_sha256'],'reference log identity')
 rebuilt=pilot.parent/'regenerated-selection';P.prepare(native,rebuilt);need(R.read(rebuilt/'selection.json')==selection,'frozen native selection replay')
 for job in selection['jobs']:need((rebuilt/job['input']).read_bytes()==(pilot/job['input']).read_bytes(),'exact native/reference input reconstruction')
 metrics={};max_convergence=0;pair_checks=0;frame_bytes=0;full_pub_bytes=0;all_ref_inputs=set()
 for scene in P.N.SCENES:
  for on,observed,mode in [(True,False,-1),(True,True,-1),(False,False,-1),(False,True,0),(False,True,1),(False,True,2)]:
   name=scene+('-on' if on else '-off')+('-observed' if observed else '-plain')+('' if mode<0 else '-m'+str(mode));directory=native/name;r=R.read(directory/'receipt.json');need(r['before']==r['after']==native_meta['after'] and r['exit_code']==0,'native source/process')
   need(r['binary_sha256']==native_meta['observed_binary_sha256' if observed else 'binary_sha256'],'native executable');log=(directory/'native.log').read_text();need(not any(s in log.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'GPU/compiler')
   need(len((directory/'image.rgba').read_bytes())==2560*1440*4,'full Screen extent')
   for file,sha in r['files_sha256'].items():need(R.sha(directory/file)==sha,'native artifact binding')
  # prepare/rows enforce all four complete-image observer comparisons.
  frame_bytes+=4*2560*1440*4;full_pub_bytes+=2560*1440*16
  cur=R.read(pilot/(scene+'-current-rays.json'))['rays'];can=R.read(pilot/(scene+'-candidate-rays.json'))['rays'];metrics[scene]={}
  for target in ['own']+(['ideal'] if scene!='front' else []):
   refs={}
   for lane in ['current','candidate']:
    kind=lane if target=='own' else 'ideal';name=scene+'-'+kind;job=next(j for j in selection['jobs'] if j['name']==name);values=R.read(pilot/(name+'-reference.json'));receipt=R.read(pilot/(name+'-reference.receipt.json'));need(receipt['reference_sha256']==R.sha(pilot/(name+'-reference.json')),'reference result bytes');conv=reference_binding(receipt,job,values,pilot/job['input'],build);need(conv<=1e-6,'unconverged reference');max_convergence=max(max_convergence,conv);refs[lane]=values['rows'];pair_checks+=1
   result={}
   for label,offset in [('2^16',0),('2^15',4)]:
    a=errors(cur,refs['current'],offset);b=errors(can,refs['candidate'],offset);gates=compare(a,b)
    individual={}
    for kind in ['T','RGB']:
     ea=[abs(r[7]-q[offset]) if kind=='T' else max(abs(r[8+k]-q[offset+1+k]) for k in range(3)) for r,q in zip(cur,refs['current'])]
     eb=[abs(r[7]-q[offset]) if kind=='T' else max(abs(r[8+k]-q[offset+1+k]) for k in range(3)) for r,q in zip(can,refs['candidate'])]
     individual[kind]=dict(candidate_worse=sum(y>x for x,y in zip(ea,eb)),largest_increase=max([y-x for x,y in zip(ea,eb)]+[0]),candidate_above_Current_cohort_max=sum(y>max(ea) for y in eb))
    result[label]=dict(Current=a,candidate=b,gates=gates,individual=individual)
   metrics[scene][target]=result
  if scene!='front':
   image_cur=(native/(scene+'-off-plain')/'image.rgba').read_bytes();image_can=(native/(scene+'-on-plain')/'image.rgba').read_bytes();reference=R.read(pilot/(scene+'-ideal-reference.json'))['rows'];screen={}
   for label,offset in [('2^16',0),('2^15',4)]:
    ac=[];bc=[];am=[];bm=[]
    for r,q in zip(cur,reference):
     i=(r[1]*2560+r[0])*4;a=[abs(image_cur[i+k]/255-max(0,min(1,q[offset+1+k]))) for k in range(3)];b=[abs(image_can[i+k]/255-max(0,min(1,q[offset+1+k]))) for k in range(3)];ac+=a;bc+=b;am.append(max(a));bm.append(max(b))
    a=dict(RGB_max_per_ray=stats(am),RGB_components=stats(ac));b=dict(RGB_max_per_ray=stats(bm),RGB_components=stats(bc));screen[label]=dict(Current=a,candidate=b,gates=compare(a,b),per_pixel_max_RGB_worse=sum(y>x for x,y in zip(am,bm)))
   metrics[scene]['unmodified_Screen_common_ideal']=screen
 for job in selection['jobs']:
  c=R.read(pilot/job['input']);all_ref_inputs.update(tuple([*c['origin'],c['time'],*row[:7]]) for row in c['rays'])
 # Deliberate metadata failures must reject independently of numerical quality.
 job=selection['jobs'][0];values=R.read(pilot/(job['name']+'-reference.json'));receipt=R.read(pilot/(job['name']+'-reference.receipt.json'));negatives=[]
 cases={}
 q=copy.deepcopy(receipt);q['input_sha256']='0'*64;cases['wrong-input-binding']=(q,values)
 q=copy.deepcopy(receipt);q['after']['binary_sha256']='0'*64;cases['changed-reference-binary']=(q,values)
 q=copy.deepcopy(values);q['rows']=q['rows'][:-1];cases['missing-reference-row']=(receipt,q)
 q=copy.deepcopy(values);q['rows'][0][0]=float('nan');cases['nonfinite-reference']=(receipt,q)
 for name,(r,v) in cases.items():
  try:reference_binding(r,job,v,pilot/job['input'],build)
  except ValueError as e:negatives.append(dict(name=name,rejection=str(e)))
  else:raise ValueError('invalid reference admitted '+name)
 all_gates=[ok for scene in metrics.values() for target in scene.values() for res in target.values() for family in res['gates'].values() for ok in family.values()]
 need(all(all_gates),'a scoped aggregate numerical gate regressed')
 return dict(scoped_aggregate_gate_count=len(all_gates),all_scoped_aggregate_gates_pass=all(all_gates),schema='6ew-bounded-native-multiview-numerical-pilot-1',native_cases=24,full_viewport=[2560,1440],record_grid=[640,360],sample_stride=4,pixels_selected_per_case=80,paired_pixel_cases=320,reference_rows=880,distinct_reference_inputs=len(all_ref_inputs),continuity_execution_rows=36,reference_budget=budget,max_convergence=max_convergence,exact_Screen_observer_comparison_bytes=frame_bytes,full_publication_hash_comparison_bytes=full_pub_bytes,metrics=metrics,reference_binding_negatives=negatives,full_population_qualified=False,inhabited_performance_measured=False,scope='Own-input comparisons bind each lane to its captured f32 rays/clips; common targets use explicit ideal cameras on3clear views. Current float records are diagnostic, with exact original Screen controls. Some individual float errors regress. Full publication equality is retained as native hashes; raw replay covers sampled publication and all Screen bytes. No whole-frame accuracy, generic certification, moving FPS or Person acceptance.')


def replay(archive,out):
 with tempfile.TemporaryDirectory(prefix='6ew-replay-') as tmp,zipfile.ZipFile(archive) as z:
  root=Path(tmp);manifest=json.loads(z.read('manifest.json'));need(set(z.namelist())=={'manifest.json'}|{'blobs/'+s for s in manifest['files'].values()},'archive inventory');blobs=root/'blobs';blobs.mkdir()
  for sha in set(manifest['files'].values()):
   raw=z.read('blobs/'+sha);need(hashlib.sha256(raw).hexdigest()==sha,'archive payload hash');(blobs/sha).write_bytes(raw)
  for name,sha in manifest['files'].items():
   p=PurePosixPath(name);need(not p.is_absolute() and '..' not in p.parts,'safe archive path');path=root/name;path.parent.mkdir(parents=True,exist_ok=True);os.link(blobs/sha,path)
  for f in (R.ROOT/'scratch/probes').glob('ray_transport_6ew*'):need(R.sha(f)==manifest['files']['tools/'+f.name],'use archived tool revision')
  build=R.read(root/'native/build.json');need(build['before']==build['after'],'stable native build');need(R.sha(root/'native/probe.cpp')==build['probe_cpp_sha256'] and R.sha(root/'native/observed-renderer.cpp')==build['observed_renderer_sha256'] and R.sha(root/'native/observed.inc')==build['observed_inc_sha256'],'native generated source bytes')
  for path,sha in build['after']['source_files'].items():need(R.sha(R.ROOT/path)==sha,'matching historical engine source required')
  # Current capture mode is a uniform read solely in the post-integrator observer.
  text=(root/'native/probe.cpp').read_text();need('var<uniform> ecMrtControl' in text and 'ecObservedValue(ro,rd,t0,t1,transmittance,integratedRadiance)' in text,'runtime observation selector')
  expected=summarize(root/'native',root/'pilot',root/'reference');need(expected==R.read(root/'summaries/result.json'),'numerical pilot replay')
  if out:R.write(out,expected)
  print('PASS retained numerical pilot:24 native captures,880 new reference rows,4 pairing refusals; all scoped metrics reported, no full-population admission')
  return expected


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--output',type=Path);a=p.parse_args();replay(a.archive,a.output)
if __name__=='__main__':main()
