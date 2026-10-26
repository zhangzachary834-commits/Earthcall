"""Replay §6dl review evidence; stdlib plus the source-pinned existing validators.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
No GPU, build, timestamp, save mutation or performance promotion.
"""
import argparse, ast, csv, hashlib, importlib.util, io, json, re, zipfile
from pathlib import Path


def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--root',type=Path,required=True)
 p.add_argument('--record',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();root=a.root.resolve();out=a.output.resolve()
 if out.exists():raise ValueError('fresh output required')
 record=json.loads(a.record.read_text());bundle=a.record.parent/record['bundle']
 if hashlib.sha256(bundle.read_bytes()).hexdigest()!=record['bundle_sha256']:raise ValueError('bundle hash')
 spec=importlib.util.spec_from_file_location('sixdd',root/'scratch/probes/ray_transport_gpu_6dd_continuation.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 result={'pass':True,'qualified_speedup':False,'production_changed':False,'variants':{}}
 with zipfile.ZipFile(bundle) as z:
  expected={d['file']:d for d in record['documents']}
  index=json.loads(z.read('index.json'))
  if set(index)!=set(expected):raise ValueError('document inventory')
  def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()
  def unpack(v):
   if isinstance(v,dict) and set(v)=={'$shared_array'}:
    h=v['$shared_array'];data=z.read('arrays/'+h+'.json')
    if hashlib.sha256(data).hexdigest()!=h:raise ValueError('shared array hash')
    return json.loads(data)
   if isinstance(v,dict):return {k:unpack(x) for k,x in v.items()}
   if isinstance(v,list):return [unpack(x) for x in v]
   return v
  def raw(name):
   item=expected[name];node=index[name]
   if node['kind']=='json':
    data=canonical(unpack(node['value']))
    if hashlib.sha256(data).hexdigest()!=item['canonical_sha256']:raise ValueError('JSON value identity: '+name)
   else:
    data=z.read(node['blob'])
    if len(data)!=item['original_bytes'] or hashlib.sha256(data).hexdigest()!=item['original_sha256']:raise ValueError('byte identity: '+name)
   return data
  for name in expected:raw(name)
  def js(name):return json.loads(raw(name))
  rays=js('fixture/rays.json');ref=js('fixture/reference.json')
  v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
  for variant,fun in [('disjoint',m.disjoint_fastpath_source),('cert',m.ux_tile_cert_disjoint_source)]:
   cpp=raw(variant+'/probe.cpp').decode()
   if cpp!=fun(v['CPP_SOURCE'],root,wide=True,scratch_trimmed=True):raise ValueError('generator source identity')
   batch=cpp.split('auto runBatchConnected =',1)[1].split('auto cmd =',1)[0]
   if 'SetPipeline(pass1, cpProducer)' not in batch or 'cpFastCompact' in batch:raise ValueError('legacy batch pipeline scope')
   mode3=cpp.split('else if(mode==3)',1)[1].split('auto cmd=',1)[0]
   if 'dispatch(cpFastCompact,' not in mode3:raise ValueError('candidate mode3 pipeline scope')
   report=js(variant+'/result.json');rows=js(variant+'/rows.json');base=js('fixture/baseline.json')
   checked=m.validate_6dd(root,report,rows,base,rays,ref)
   if checked['scratch_trimmed_scope']['shipping_performance_qualified']:raise ValueError('unearned promotion')
   producer=re.search(r'kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";',cpp,re.S)[1]
   producer_sha=hashlib.sha256(producer.encode()).hexdigest()
   facts=[json.loads(line) for line in raw(variant+'/pipeline-facts.jsonl').decode().splitlines()]
   if not any(x.get('wgslSHA256')==producer_sha and x.get('wgslEntry')=='producerFastMain' for x in facts):raise ValueError('native producer source association')
   profile=list(csv.reader(io.StringIO(raw(variant+'/counters.csv').decode())))
   if len(profile)!=4 or len(profile[0])!=272 or any(len(row)!=272 for row in profile[1:]):raise ValueError('profile counter coverage')
   controls=[]
   def reject(name):
    try:m.validate_6dd(root,report,rows,base,rays,ref)
    except (ValueError,AssertionError) as e:controls.append({'case':name,'rejected':True,'reason':str(e)});return
    raise ValueError('corruption accepted: '+name)
   packet=next(x for x in report['coarse_packet_census_inputs'] if x[3]);old=packet[3][0][0];packet[3][0][0]=old+1;reject('candidate endpoint');packet[3][0][0]=old
   cap=report['coarse_capacity_controls'][0];old=cap['flags'];cap['flags']=old[:-1];reject('missing capacity coverage');cap['flags']=old
   sample=report['streamed_schedule']['cases'][0]['series'][0]['samples'][0];old=sample['total_ms'];sample['total_ms']=-1;reject('negative clock');sample['total_ms']=old
   result['variants'][variant]={'qualification':checked,'producer_sha256':producer_sha,'corruption_refusals':controls}
 result['documents_verified']=len(record['documents']);result['corruption_refusals']=sum(len(v['corruption_refusals']) for v in result['variants'].values())
 out.write_text(json.dumps(result,indent=2)+'\n');print('PASS §6dl:',result['documents_verified'],'documents;',result['corruption_refusals'],'refusals; no qualified speedup')


if __name__=='__main__':main()
