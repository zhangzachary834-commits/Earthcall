"""CPU replay of §6dq native correctness evidence; no new GPU, timing, or promotion.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
"""
import argparse,hashlib,importlib.util,json,math,re,struct,zipfile
from pathlib import Path

def check_case(v):
 n=v['rays'];p=v['packets'];o=v['outputs']
 if n!=32784 or len(p)!=n or len(o)!=n:raise ValueError('ray coverage')
 if [x[0] for x in p]!=list(range(n)) or [x[0] for x in o]!=list(range(n)):raise ValueError('ray ownership')
 if sum(not x[3] for x in p)!=v['packet_differing_rays']:raise ValueError('native packet comparison coverage')
 if sum(x[1][3]>0 for x in p)!=v['overlap_rays']:raise ValueError('overlap coverage')
 if sum(x[1]!=x[2] for x in o)!=v['missing_overlap_differing_rays']:raise ValueError('noncanonical consumer differences')
 if sum(x[2]!=x[3] for x in o)!=v['restored_consumer_routed_vs_bulk_differing_rays']:raise ValueError('canonical consumer routed/bulk differences')
 for x in o:
  for words in x[1:]:
   if len(words)!=4 or any(not math.isfinite(struct.unpack('<f',struct.pack('<I',k))[0]) for k in words):raise ValueError('output bit coverage/finiteness')
 counts=v['route_counts'];args=v['indirect_args']
 if sum(counts[:3])!=2304 or len(args)!=9:raise ValueError('route coverage')
 for i,c in enumerate(counts[:3]):
  if args[3*i:3*i+3]!=[(c+3)//4,1,1]:raise ValueError('indirect args')
 if v['new_timing'] or v['shipping_qualified']:raise ValueError('unearned qualification')
 return {'rays':n,'packet_differing_rays':v['packet_differing_rays'],'noncanonical_consumer_differing_rays':v['missing_overlap_differing_rays'],'canonical_consumer_routed_bulk_differing_rays':v['restored_consumer_routed_vs_bulk_differing_rays'],'normal_overlap_rays':v['overlap_rays'],'route_counts':counts,'native_budget_zero_unwritten_assertion':v['budget_zero_unwritten_rays']}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():raise ValueError('fresh output required')
 record=json.loads(a.record.read_text());bundle=a.record.parent/record['bundle']
 if hashlib.sha256(bundle.read_bytes()).hexdigest()!=record['bundle_sha256']:raise ValueError('bundle identity')
 def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()
 with zipfile.ZipFile(bundle) as z:
  expected={x['file']:x for x in record['documents']};index=json.loads(z.read('index.json'))
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
    if hashlib.sha256(data).hexdigest()!=item['canonical_sha256']:raise ValueError('JSON identity')
   else:
    data=z.read(node['blob'])
    if len(data)!=item['original_bytes'] or hashlib.sha256(data).hexdigest()!=item['original_sha256']:raise ValueError('byte identity')
   return data
  for name in expected:raw(name)
  def js(name):return json.loads(raw(name))
  cases={};refusals=[]
  for name in ['before-wide','before-reverse','after-wide','after-reverse','after-parallel']:
   v=js(name+'/result.json');cases[name]=check_case(v)
   if name.startswith('after-'):
    if v['packet_differing_rays'] or v['restored_consumer_routed_vs_bulk_differing_rays'] or v['bad_writes']:raise ValueError('corrected native case')
    def reject(label):
     try:check_case(v)
     except ValueError:refusals.append(name+': '+label);return
     raise ValueError('accepted corruption: '+label)
    old=v['packets'][0][3];v['packets'][0][3]=not old;reject('packet comparison');v['packets'][0][3]=old
    old=v['outputs'][0][2][0];v['outputs'][0][2][0]=old^1;reject('output word');v['outputs'][0][2][0]=old
    old=v['indirect_args'][0];v['indirect_args'][0]=old+1;reject('indirect length');v['indirect_args'][0]=old
  if cases['before-wide']['packet_differing_rays']!=0 or cases['before-reverse']['packet_differing_rays']!=914:raise ValueError('counterexample changed')
  root=a.root.resolve();spec=importlib.util.spec_from_file_location('faithful',root/'scratch/probes/ray_transport_gpu_faithful_residual.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
  for mode in ['fixed','no-reset','no-tail']:
   if m.generate_cpp_source(root,mutation=mode)!=raw(mode+'/probe.cpp').decode():raise ValueError('generator identity')
  witness=raw('after-witness.cpp').decode();facts=[json.loads(x) for x in raw('after-wide/facts.jsonl').decode().splitlines()]
  for name in ['kClassifyWgsl','kRouteArgsWgsl','kS1ProducerWgsl','kS2ProducerWgsl','kFallbackProducerWgsl','kConsumerWgsl','kReferenceProducerWgsl','kCheckedConsumerWgsl']:
   shader=re.search(r'static const char\* '+name+r'\s*=\s*R"WGSL\((.*?)\)WGSL";',witness,re.S)[1]
   if not any(x.get('wgslSHA256')==hashlib.sha256(shader.encode()).hexdigest() for x in facts):raise ValueError('actual pipeline source association: '+name)
  controls=js('native-controls.json')
  if len(controls)!=5 or any(not x['pass'] for x in controls):raise ValueError('control coverage')
  for name,needle in [('forced-budget','Current fallback is not connected'),('no-reset','counter accumulation mismatch'),('no-tail','exactly-once publication failure'),('layout-refusal','pinned tile layout')]:
   if needle not in raw(name+'.log').decode():raise ValueError('expected refusal: '+name)
  for name in ['fixed-wide','fixed-reverse']:
   value=js(name+'.json')
   if value['series_1_abba'] or value['series_2_baab'] or value['qualification']['current_speedup_qualified']:raise ValueError('diagnostic scope')
 result={'pass':True,'documents_verified':len(expected),'cases':cases,'corruption_refusals':refusals,'new_native_execution_by_replay':False,'qualified_speedup':False,'full_transport_connected':False,'production_changed':False,'raw_output_words_rechecked':True,'subpanel_byte_comparisons_native_assertions_not_archived_packets':True}
 a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS §6dq:',len(expected),'documents,',len(refusals),'corruption refusals; canonical transport and shipping remain OPEN')
if __name__=='__main__':main()
