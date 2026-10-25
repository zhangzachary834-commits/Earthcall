"""Replay §6di facts and raw controls, optionally restore the exact Metal traces.
No GPU, Xcode, or temporary case required. Counter-backed profiling is NOT replayed.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
"""
import argparse,copy,hashlib,importlib.util,json,os
from pathlib import Path

def load(root,name):
 s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def routing(r):
 cs=r['controls'];expected=[(p,n) for p in [0,1,0] for n in [1,2,4,9]]
 if [(x['pose'],x['repeats']) for x in cs]!=expected:raise ValueError('routing case coverage')
 for c in cs:
  ns=c['counts'];args=c['args'];writes=c['writes']
  if len(ns)!=4 or sum(ns[:3])!=2304 or ns[3]!=0 or any(n<0 or n>2304 for n in ns):raise ValueError('counter reset')
  if ns[:3]!=([0,0,2304] if c['pose'] else [906,707,691]):raise ValueError('fresh classification')
  if args!=sum(([(n+3)//4,1,1] for n in ns[:3]),[]):raise ValueError('current indirect arguments')
  if len(writes)!=32784 or any(x!=1 for x in writes):raise ValueError('unique ray ownership')
  if not 0<=c['max_delta_sibling']<=1e-4:raise ValueError('sibling-only numerical check')
 if r['host_counts_used_for_dispatch'] is not False or r['GPU_generated_indirect_arguments'] is not True or r['shipping_gates_open']!=3:raise ValueError('routing scope')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--restore-traces',type=Path);a=p.parse_args()
 if a.output.exists() or a.restore_traces and a.restore_traces.exists():raise ValueError('fresh output required')
 root=a.root.resolve();record=json.loads(a.record.read_text())
 for rel,digest in record['dependencies'].items():
  if hashlib.sha256((root/rel).read_bytes()).hexdigest()!=digest:raise ValueError('dependency drift '+rel)
 if record['resource_counter_profile_collected'] or record['qualified_speedup'] or record['production_changed'] or record['shipping_gates_open']!=3:raise ValueError('unestablished claim')
 bundle=load(root,'ray_transport_root_review').Bundle(root/record['bundle'],record['bundle_sha256'])
 for name in bundle.index:bundle.read(name)
 sixdd=load(root,'ray_transport_gpu_6dd_continuation');rays=bundle.read('rays');reference=bundle.read('reference');checks={}
 for name in ['disjoint','cert']:
  identity=bundle.read(name+'_identity')
  if hashlib.sha256(bundle.read(name+'_source')).hexdigest()!=identity['observed_cpp_sha256'] or hashlib.sha256(bundle.read('executed_observer')).hexdigest()!=identity['observer_sha256']:raise ValueError('actual capture source identity')
  checks[name]=sixdd.validate_6dd(root,bundle.read(name+'_report'),bundle.read(name+'_rows'),bundle.read(name+'_baseline'),rays,reference)
  checks[name]['clocks_are_instrumented_not_clean_economics']=True
 roles=bundle.read('pipeline_roles')
 for name in ['current','guarded_producer','disjoint','cert']:
  fact=roles[name];source=bundle.read(name+'_wgsl');digest=hashlib.sha256(source).hexdigest()
  if fact['function']!=fact['wgslEntry'] or fact['wgslSHA256']!=digest or fact['threadExecutionWidth']!=32 or fact['maxTotalThreadsPerThreadgroup']!=1024 or fact['staticThreadgroupMemoryLength']!=0:raise ValueError('actual pipeline facts')
 r=bundle.read('routing_report');routing(r);negative=bundle.read('routing_negative_results')
 if [(x['mutation'],x['refused_for_expected_reason']) for x in negative]!=[('no-reset',True),('no-tail',True)]:raise ValueError('executable negative coverage')
 for c in negative:
  if c['returncode']==0 or c['expected_refusal'].encode() not in bundle.read('routing_'+c['mutation']+'_log'):raise ValueError('negative control did not execute expected refusal')
 for name in ['fixed','no-reset','no-tail']:
  source=bundle.read('routing_'+name+'_source');identity=bundle.read('routing_'+name+'_identity')
  if hashlib.sha256(source).hexdigest()!=identity['cpp_sha256']:raise ValueError('routing executable source')
 generic=bundle.read('generic_report');items=generic['results']
 if not generic['pass'] or not generic['shared_analyzer_consumer_used_by_both_families'] or generic['native_skip_admission'] or generic['generic_arbitrary_AST_admission'] or len(items)!=8:raise ValueError('generic scope')
 if [x['label'] for x in items]!=['authored-field']*4+['authored-second-family']*4:raise ValueError('both expression families')
 if any(x['queries']<=0 or x['certified_zero_boxes']<=0 or x['negative_controls']!=4 or x['independent_point_samples']!=x['certified_zero_boxes']*9 for x in items):raise ValueError('host witness coverage')
 if generic['certified_zero_boxes']!=sum(x['certified_zero_boxes'] for x in items) or generic['point_samples']!=sum(x['independent_point_samples'] for x in items):raise ValueError('host totals')
 if bundle.read('generic_source')!=bundle.read('deliverable_ray_transport_generic_family_contract.cpp'):raise ValueError('host executed source identity')
 if hashlib.sha256(bundle.read('generic_source')).hexdigest()!=bundle.read('generic_identity')['cpp_sha256']:raise ValueError('host actual compiler source')
 manifest=bundle.read('trace_manifest');traceStats={}
 for role,files in manifest.items():
  if not files or 'capture' not in files:raise ValueError('incomplete trace')
  total=0
  for rel,document in files.items():
   path=Path(rel)
   if path.is_absolute() or '..' in path.parts:raise ValueError('trace path')
   if isinstance(document,dict):
    target=document['symlink'];tp=Path(target)
    if tp.is_absolute() or '..' in tp.parts or str(path.parent/tp) not in files or isinstance(files[str(path.parent/tp)],dict):raise ValueError('unsafe trace link')
    continue
   data=bundle.read(document);total+=len(data)
   if a.restore_traces:
    dest=a.restore_traces/(role+'.gputrace')/path;dest.parent.mkdir(parents=True,exist_ok=True)
    with dest.open('xb') as f:f.write(data)
  if a.restore_traces:
   for rel,document in files.items():
    if isinstance(document,dict):os.symlink(document['symlink'],a.restore_traces/(role+'.gputrace')/rel)
  traceStats[role]={'files':len(files),'uncompressed_bytes':total}
 if set(traceStats)!={'current','disjoint','cert'}:raise ValueError('trace comparison coverage')
 refusals=[]
 for name,change in [('duplicate write',lambda x:x['controls'][0]['writes'].__setitem__(0,2)),('missing routing case',lambda x:x['controls'].pop()),('accumulated counts',lambda x:x['controls'][0]['counts'].__setitem__(0,1812)),('stale indirect args',lambda x:x['controls'][0]['args'].__setitem__(0,1))]:
  bad=copy.deepcopy(r);change(bad)
  try:routing(bad)
  except ValueError:refusals.append(name)
  else:raise ValueError('corruption accepted '+name)
 out={'pass':True,'verified_documents':len(bundle.index),'actual_pipeline_roles':roles,'qualification':checks,'routing_cases':len(r['controls']),'native_negative_controls':negative,'host_generic_zero_boxes':generic['certified_zero_boxes'],'host_generic_point_samples':generic['point_samples'],'host_refusal_controls':sum(x['negative_controls'] for x in items),'trace_documents':traceStats,'offline_corruption_refusals':refusals,'resource_counters_inspected':False,'shipping_gates_open':3,'limits':'No clean cost, full-frame or Person claim. Native AST and host point assertions recorded, not independently reevaluated offline. Exact trace bytes restored optionally; Xcode profiler unavailable for counter inspection.'}
 a.output.write_text(json.dumps(out,indent=2)+'\n');print('PASS §6di replay',len(bundle.index),'documents;12 native routing cases;2 native mutation refusals;32 host controls;',len(refusals),'offline corruption refusals')
if __name__=='__main__':main()
