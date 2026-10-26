"""Replay §6dr routed-bridge evidence on CPU only; no new GPU/timing/profile.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
"""
import argparse,ast,hashlib,importlib.util,json,math,statistics,struct,zipfile
from pathlib import Path

def native_check(v):
 cases=v['cases'];poses=[0,1,0,2,3,0]
 if [(x['case'],x['repeats']) for x in cases[:24]]!=[('pose-'+str(p),n) for p in poses for n in [1,2,4,9]]:raise ValueError('native temporal coverage')
 if [x['case'] for x in cases[24:]]!=['budget-0','budget-23','budget-24','budget-25','capacity-0','capacity-1']:raise ValueError('routed refusal coverage')
 for x in cases:
  if x['rays']!=32784 or len(x['flags'])!=32784 or x['producer_entry_errors'] or x['output_differences']:raise ValueError('native ray/output/producer coverage')
  if x['packet_differences']!=(0 if x['case'].startswith('pose-') else None):raise ValueError('native packet assertion')
  counts=x['family_counts'];args=x['indirect_args']
  if sum(counts)!=2304 or args!=[v for n in counts for v in [(n+3)//4,1,1]]:raise ValueError('native indirect args')
  if sum(bool(f&0x80000000) for f in x['flags'])!=x['refused']:raise ValueError('native refusal flags')
  if x['repeats']==1:
   if len(x['outputs'])!=32784:raise ValueError('native output coverage')
   for i,a,e in x['outputs']:
    if a!=e or len(a)!=4 or any(not math.isfinite(struct.unpack('<f',struct.pack('<I',w))[0]) for w in a):raise ValueError('native output bits')
  elif x['outputs']:raise ValueError('native output scope')
 if cases[24]['refused']!=32784 or any(not x['refused'] for x in cases[24:]):raise ValueError('vacuous routed refusal')
 if v['clean_timing'] or v['shipping_qualified']:raise ValueError('native diagnostic scope')
 return [{'case':x['case'],'repeats':x['repeats'],'counts':x['family_counts'],'refused':x['refused']} for x in cases]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=a.root.resolve()
 if a.output.exists():raise ValueError('fresh output required')
 record=json.loads(a.record.read_text());bundle=a.record.parent/record['bundle']
 if hashlib.sha256(bundle.read_bytes()).hexdigest()!=record['bundle_sha256']:raise ValueError('bundle identity')
 def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()
 with zipfile.ZipFile(bundle) as z:
  expected={d['file']:d for d in record['documents']};index=json.loads(z.read('index.json'))
  if set(index)!=set(expected):raise ValueError('inventory')
  def unpack(v):
   if isinstance(v,dict) and set(v)=={'$shared_array'}:
    h=v['$shared_array'];data=z.read('arrays/'+h+'.json')
    if hashlib.sha256(data).hexdigest()!=h:raise ValueError('array identity')
    return json.loads(data)
   if isinstance(v,dict):return {k:unpack(x) for k,x in v.items()}
   if isinstance(v,list):return [unpack(x) for x in v]
   return v
  def raw(name):
   node=index[name];item=expected[name]
   if node['kind']=='json':
    data=canonical(unpack(node['value']))
    if hashlib.sha256(data).hexdigest()!=item['canonical_sha256']:raise ValueError('JSON values identity')
   else:
    data=z.read(node['blob'])
    if len(data)!=item['original_bytes'] or hashlib.sha256(data).hexdigest()!=item['original_sha256']:raise ValueError('byte identity')
   return data
  for name in expected:raw(name)
  def js(name):return json.loads(raw(name))
  def module(name,file):
   spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
  c6dd=module('sixdd','ray_transport_gpu_6dd_continuation.py');rq=module('rq','ray_transport_gpu_6dq_routed_residual.py');controls=module('controls','ray_transport_gpu_routed_controls.py')
  v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
  if rq.make_routed_residual_cpp_source(v['CPP_SOURCE'],root,True)!=raw('original/probe.cpp').decode():raise ValueError('clean executed source identity')
  for mode in ['fixed','no-reset','no-tail']:
   if controls.generate(root,mode)!=raw('native/'+mode+'.cpp').decode():raise ValueError('native control source identity')
  report=js('original/result.json');rows=js('original/rows.json');base=js('original/baseline.json');rays=js('fixture/rays.json');ref=js('fixture/reference.json')
  checked=c6dd.validate_6dd(root,report,rows,base,rays,ref);refusals=[]
  def reject(fn,label):
   try:fn()
   except (ValueError,AssertionError):refusals.append(label);return
   raise ValueError('accepted corruption: '+label)
  validate=lambda:c6dd.validate_6dd(root,report,rows,base,rays,ref)
  packet=next(x for x in report['coarse_packet_census_inputs'] if x[3]);old=packet[3][0][0];packet[3][0][0]=old+1;reject(validate,'routed packet endpoint');packet[3][0][0]=old
  cap=report['coarse_capacity_controls'][0];old=cap['flags'];cap['flags']=old[:-1];reject(validate,'missing capacity coverage');cap['flags']=old
  sample=report['streamed_schedule']['cases'][0]['series'][0]['samples'][0];old=sample['total_ms'];sample['total_ms']=-1;reject(validate,'negative clock');sample['total_ms']=old
  row=report['streamed_schedule']['cases'][0]['forced_rows'][0];old=row[2];row[2]=old+1;reject(validate,'forced Current output');row[2]=old
  old=report['routed_residual']['s1_tiles'];report['routed_residual']['s1_tiles']=old+1;reject(validate,'routed census');report['routed_residual']['s1_tiles']=old
  native=js('native/result.json');native_summary=native_check(native)
  old=native['cases'][0]['outputs'][0][1][0];native['cases'][0]['outputs'][0][1][0]=old^1;reject(lambda:native_check(native),'native final output word');native['cases'][0]['outputs'][0][1][0]=old
  old=native['cases'][0]['indirect_args'][0];native['cases'][0]['indirect_args'][0]=old+1;reject(lambda:native_check(native),'native indirect args');native['cases'][0]['indirect_args'][0]=old
  old=native['cases'][24]['flags'];native['cases'][24]['flags']=old[:-1];reject(lambda:native_check(native),'native forced coverage');native['cases'][24]['flags']=old
  for mode,needle in [('no-reset','routed counter accumulation mismatch'),('no-tail','routed exactly-once producer publication failure')]:
   if needle not in raw('native/'+mode+'.log').decode():raise ValueError('negative mutation evidence')
 result={'pass':True,'documents_verified':len(expected),'raw_qualification':checked,'native_summary':native_summary,'corruption_refusals':refusals,'new_native_execution_by_replay':False,'shipping_performance_qualified':False,'new_physical_profile':False,'native_packet_word_comparisons_are_native_assertions':True}
 a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS §6dr:',len(expected),'documents;',len(native_summary),'native cases;',len(refusals),'corruption refusals; no shipping gain')
if __name__=='__main__':main()
