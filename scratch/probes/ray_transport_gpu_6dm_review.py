"""Replay §6dn review of §6dm. CPU-only; no new native timing or correctness capture.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
"""
import argparse,ast,hashlib,importlib.util,json,re,statistics,zipfile
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();root=a.root.resolve();record=json.loads(a.record.read_text());out=a.output
 if out.exists():raise ValueError('fresh output required')
 bundle=a.record.parent/record['bundle']
 if hashlib.sha256(bundle.read_bytes()).hexdigest()!=record['bundle_sha256']:raise ValueError('bundle identity')
 def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()
 spec=importlib.util.spec_from_file_location('sixdd',root/'scratch/probes/ray_transport_gpu_6dd_continuation.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 result={'pass':True,'new_native_execution':False,'qualified_speedup':False,'production_changed':False,'variants':{}}
 with zipfile.ZipFile(bundle) as z:
  expected={d['file']:d for d in record['documents']};index=json.loads(z.read('index.json'))
  if set(expected)!=set(index):raise ValueError('inventory')
  def unpack(v):
   if isinstance(v,dict) and set(v)=={'$shared_array'}:
    h=v['$shared_array'];data=z.read('arrays/'+h+'.json')
    if hashlib.sha256(data).hexdigest()!=h:raise ValueError('shared array identity')
    return json.loads(data)
   if isinstance(v,dict):return {k:unpack(x) for k,x in v.items()}
   if isinstance(v,list):return [unpack(x) for x in v]
   return v
  def raw(name):
   node=index[name];item=expected[name]
   if node['kind']=='json':
    data=canonical(unpack(node['value']))
    if hashlib.sha256(data).hexdigest()!=item['canonical_sha256']:raise ValueError('JSON values: '+name)
   else:
    data=z.read(node['blob'])
    if len(data)!=item['original_bytes'] or hashlib.sha256(data).hexdigest()!=item['original_sha256']:raise ValueError('bytes: '+name)
   return data
  for name in expected:raw(name)
  def js(name):return json.loads(raw(name))
  manifest=js('fixture/manifest.json');rays=js('fixture/rays.json');ref=js('fixture/reference.json')
  for name in ['rays','reference']:
   if expected['fixture/'+name+'.json']['original_sha256']!=manifest[name+'_sha256']:raise ValueError('fixture hash')
  if hashlib.sha256((root/'saves/zones/Northern Veil/zone.ecform').read_bytes()).hexdigest()!=manifest['zone_sha256']:raise ValueError('authored fixture drift')
  v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
  for variant,fun in [('disjoint',m.disjoint_fastpath_source),('cert',m.ux_tile_cert_disjoint_source)]:
   generated=fun(v['CPP_SOURCE'],root,wide=True,scratch_trimmed=True,stream_merge=True)
   producer=re.search(r'kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";',generated,re.S)[1]
   if re.search(r'array\s*<\s*(?:f32|vec\df)\s*,\s*96\s*>',producer):raise ValueError('96-slot array remains')
   if 'var prevKnot = 0.0;' not in producer or 'stepCount >= totalKnots || reason != 0u' not in producer:raise ValueError('streamed merge scope')
   batch=generated.split('auto runBatchConnected =',1)[1].split('auto cmd =',1)[0]
   mode3=generated.split('else if(mode==3)',1)[1].split('auto cmd=',1)[0]
   if 'SetPipeline(pass1, cpProducer)' not in batch or 'cpFastCompact' in batch or 'dispatch(cpFastCompact,' not in mode3:raise ValueError('dispatch scope')
   shader_sha=hashlib.sha256(producer.encode()).hexdigest()
   if raw(variant+'/capture-unobserved.cpp').decode()!=generated:raise ValueError('capture source drift')
   facts=[json.loads(x) for x in raw(variant+'/pipeline-facts.jsonl').decode().splitlines()]
   if not any(x.get('wgslSHA256')==shader_sha and x.get('wgslEntry')=='producerFastMain' for x in facts):raise ValueError('captured pipeline association')
   series=[];controls=[]
   for i in [1,2]:
    prefix=variant+'/series'+str(i)+'/'
    cpp=raw(prefix+'probe.cpp')
    if cpp.decode()!=generated:raise ValueError('clean source reconstruction')
    provenance=js(prefix+'source.json')
    if provenance['cpp_sha256']!=hashlib.sha256(cpp).hexdigest() or provenance['zone_sha256']!=manifest['zone_sha256']:raise ValueError('clean provenance')
    if 'MTLCapture' in cpp.decode() or 'startCaptureWithDescriptor' in cpp.decode():raise ValueError('capture instrumentation in timing source')
    report=js(prefix+'result.json');rows=js(prefix+'rows.json');base=js(prefix+'baseline.json')
    checked=m.validate_6dd(root,report,rows,base,rays,ref)
    scope=checked['stream_merge_scope']
    if scope['shipping_performance_qualified'] or scope['physical_bytes_saved_established'] or scope['host_certificate_creation_and_initial_upload_in_mode3_timing']:raise ValueError('unearned scope')
    medians=[]
    original=js('reviewed/GPU_RAY_TRANSPORT_6DM_STREAMING_MERGE_2026-10-25.json')['mode3_paired_timings']['disjoint' if variant=='disjoint' else 'tile_cert']['series_'+str(i)]
    for batch in report['streamed_schedule']['cases'][0]['series']:
     n=batch['repeats'];values={str(mode):statistics.median(x['total_ms'] for x in batch['samples'] if x['mode']==mode) for mode in range(4)}
     delta=100*(values['3']/values['0']-1)
     if abs(values['0']-original['repeats_'+str(n)+'_current_ms'])>0.000006 or abs(values['3']-original['repeats_'+str(n)+'_mode3_ms'])>0.000006 or abs(delta-original['repeats_'+str(n)+'_delta_pct'])>0.001:raise ValueError('original median table drift')
     medians.append({'repeats':n,'median_total_submission_ms':values,'median_per_complete_query_ms':{k:x/n for k,x in values.items()},'candidate_slower_percent_vs_current':delta})
    checked['resident_medians']=medians
    series.append(checked)
    if i==1:
     def reject(name):
      try:m.validate_6dd(root,report,rows,base,rays,ref)
      except (ValueError,AssertionError) as e:controls.append({'case':name,'rejected':True,'reason':str(e)});return
      raise ValueError('corruption accepted: '+name)
     packet=next(x for x in report['coarse_packet_census_inputs'] if x[3]);old=packet[3][0][0];packet[3][0][0]=old+1;reject('candidate endpoint');packet[3][0][0]=old
     cap=report['coarse_capacity_controls'][0];old=cap['flags'];cap['flags']=old[:-1];reject('missing capacity coverage');cap['flags']=old
     sample=report['streamed_schedule']['cases'][0]['series'][0]['samples'][0];old=sample['total_ms'];sample['total_ms']=-1;reject('negative clock');sample['total_ms']=old
   result['variants'][variant]={'producer_sha256':shader_sha,'series':series,'corruption_refusals':controls}
 result['documents_verified']=len(expected);result['corruption_refusals']=sum(len(x['corruption_refusals']) for x in result['variants'].values())
 result['mean_candidate_slower_cases']=sum(c['candidate_slower_percent_vs_current']>0 for v in result['variants'].values() for s in v['series'] for c in s['clocks'])
 result['median_candidate_slower_cases']=sum(c['candidate_slower_percent_vs_current']>0 for v in result['variants'].values() for s in v['series'] for c in s['resident_medians'])
 if result['median_candidate_slower_cases']!=12 or result['mean_candidate_slower_cases']!=8:raise ValueError('fixed evidence summary drift')
 out.write_text(json.dumps(result,indent=2)+'\n')
 print('PASS §6dn:',len(expected),'documents;',result['corruption_refusals'],'corruption refusals; 12 slower medians, 8 slower means; no new native execution')
if __name__=='__main__':main()
