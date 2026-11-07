"""Offline replay of the 32784 distinct native-ray screen, including refusals.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Archives are source/raw evidence, not native omission or full-frame authority.
"""
import argparse,copy,gzip,hashlib,importlib.util,json,math,struct,tempfile
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def sha(b):return hashlib.sha256(b).hexdigest()
def capture_check(raw,rays,ref):
 if sha(raw)!=rays['capture_sha256'] or len(raw)!=256*144*64*4:raise ValueError('capture identity/size')
 rows=[];origins=[]
 for i in range(256*144):
  b=raw[i*256:(i+1)*256];u=struct.unpack('<64I',b);f=struct.unpack('<64f',b)
  if u[0]!=1 or u[3]!=1:continue
  rows.append([u[1],u[2],*f[8:11],f[7],f[11],*f[12:16]]+[0.]*12);origins.append(list(f[4:7]))
 if len(rows)!=36864 or any(o!=rays['origin'] for o in origins):raise ValueError('capture origin/validity')
 # Actual native pixel centres selected by the instrumented capture grid.
 expected=[[int((x+.5)*2560/256),int((y+.5)*1440/144)] for y in range(144) for x in range(256)]
 if [r[:2] for r in rows]!=expected:raise ValueError('capture pixel lattice')
 selected=[rows[int((i+.5)*len(rows)/32784)] for i in range(32784)]
 if selected!=rays['rays'] or ref['ray_indices']!=[int((i+.5)*32784/192) for i in range(192)]:raise ValueError('capture selection/holdout')
 return {'valid_native_rays':len(rows),'distinct_query_rays':len(selected),'dense_reference_count':192,'pass':True}

def case_controls(wide,root,rays,ref,zone):
 passed=[]
 with tempfile.TemporaryDirectory(prefix='earthcall-wide-guards-') as td:
  p=Path(td);rp=p/'rays.json';fp=p/'reference.json';mp=p/'manifest.json'
  for name,change in [
   ('duplicate native ray',lambda r,f:r['rays'].__setitem__(1,list(r['rays'][0]))),
   ('wrong viewport',lambda r,f:r.update(viewport=[1280,720])),
   ('shifted holdout',lambda r,f:f['ray_indices'].__setitem__(0,f['ray_indices'][0]+1)),
   ('wrong reference pixel',lambda r,f:f['rows'][0].__setitem__(0,-1)),
   ('nonfinite direction',lambda r,f:r['rays'][0].__setitem__(2,float('nan'))),
   ('wrong reference resolution',lambda r,f:f.update(finest_intervals=16))]:
   r=copy.deepcopy(rays);f=copy.deepcopy(ref);change(r,f);rp.write_text(json.dumps(r));fp.write_text(json.dumps(f));mp.write_text(json.dumps({'rays':str(rp),'reference':str(fp),'zone_sha256':zone,'rays_sha256':sha(rp.read_bytes()),'reference_sha256':sha(fp.read_bytes())}))
   try:wide.verify_case(root,mp,zone)
   except (ValueError,KeyError):passed.append(name)
   else:raise ValueError('bad external case accepted: '+name)
 return passed

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path);a=p.parse_args();root=a.root.resolve();record=a.record.resolve();doc=json.loads(record.read_text());archives=doc['archives']
 def read(key):
  m=archives[key];compressed=(record.parent/m['file']).read_bytes();raw=gzip.decompress(compressed)
  if sha(compressed)!=m['compressed_sha256'] or sha(raw)!=m['sha256']:raise ValueError('archive identity: '+key)
  return json.loads(raw) if m['kind']=='json' else raw
 # Check every archive, including exact capture/reference source, not just used JSON.
 for key in archives:read(key)
 wide=load(root,'ray_transport_gpu_wide_stream');old=load(root,'ray_transport_streamed_schedule_evidence');q=load(root,'ray_transport_gpu_pose_qualification')
 zone=q.sha(root/'saves/zones/Northern Veil/zone.ecform');manifest=read('manifest');rays=read('rays');ref=read('reference')
 if (manifest['zone_sha256'],manifest['rays_sha256'],manifest['reference_sha256'])!=(zone,sha(gzip.decompress((record.parent/archives['rays']['file']).read_bytes())),sha(gzip.decompress((record.parent/archives['reference']['file']).read_bytes()))):raise ValueError('source/case identity')
 with tempfile.TemporaryDirectory(prefix='earthcall-wide-restore-') as td:
  t=Path(td);rp=t/'rays.json';fp=t/'reference.json';mp=t/'manifest.json';rp.write_text(json.dumps(rays));fp.write_text(json.dumps(ref));m=dict(manifest,rays=str(rp),reference=str(fp),rays_sha256=sha(rp.read_bytes()),reference_sha256=sha(fp.read_bytes()));mp.write_text(json.dumps(m));wide.verify_case(root,mp,zone)
 if [v[2:] for v in ref['rows']]!=read('reference_raw')['rows']:raise ValueError('independent reference provenance')
 reference=(ref['rows'],ref['ray_indices']);results={'capture':capture_check(read('capture'),rays,ref),'case_refusals':case_controls(wide,root,rays,ref,zone),'variants':{}}
 # Reuse the existing packet/float-bit/clock validator, with explicit wide capacities.
 original=old.check_record
 old.check_record=lambda *args:original(*args,capacities=[8192,16384,32768])
 q.independent_check=wide.independent_check
 for label in ['wide','envelope','adaptive']:
  report=read(label+'_report');raw=read(label+'_rows');baseline=read(label+'_baseline');source=read(label+'_source')
  if source['zone_sha256']!=zone or sha(read(label+'_cpp'))!=source['cpp_sha256']:raise ValueError('compiled source identity')
  header=root/'src/Singularity/OntoMath/CommonScalarFactor.hpp'
  if ('// CommonScalarFactor.hpp sha256: '+sha(header.read_bytes())).encode() not in read(label+'_cpp'):raise ValueError('generic mathematical header identity')
  if any(v is not True for k,v in report['qualification'].items() if k!='shipping_performance_qualified') or report['qualification']['shipping_performance_qualified'] is not False:raise ValueError('qualification scope')
  for case in report['streamed_schedule']['cases']:
   for series in case['series']:
    if [x['mode'] for x in series['samples']]!=[0,1,2,2,1,0,2,1,0,0,1,2]*2:raise ValueError('paired mode order')
  results['variants'][label]=old.check_record(report,raw,baseline,rays,reference,q)
  if label=='wide':results['output_refusals']=old.controls(report,raw,baseline,rays,reference,q)
 counter=load(root,'ray_transport_firmament_chord_guard_test').witness()
 if counter!=read('chord'):raise ValueError('chord counterexample')
 if load(root,'ray_transport_firmament_knot_cone_test').witness()!=read('knot_cone'):raise ValueError('knot/cone counterexample')
 results.update(archive_count=len(archives),counterexample=counter);results['pass']=True
 if a.output:
  if a.output.exists():raise ValueError('output exists')
  a.output.write_text(json.dumps(results,indent=2)+'\n')
 print('PASS',len(archives),'archives; native capture selection; 9 large-ring normal/forced cases;192-ray two-resolution gates x3;',len(results['case_refusals'])+len(results['output_refusals']),'refusal controls; chord counterexample')
if __name__=='__main__':main()
