"""Offline compact-packet, hybrid output, fallback and paired-clock replay.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No GPU. Full-f32 endpoints, original panel/overlap rule, recorded numerical changes; no new math authority.
"""
import argparse,copy,gzip,hashlib,importlib.util,json,math,statistics,struct
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(values):return b''.join(struct.pack('<f',v) for v in values)
def encode(spans,fallback=False):
 if len(spans)>15 or (fallback and spans):raise ValueError('packet capacity/fallback')
 control=[1,0] if fallback else [0,len(spans)<<28]
 data=[]
 for i,(lo,hi,mask) in enumerate(spans):
  if not 1<=mask<=15 or not all(math.isfinite(x) for x in [lo,hi]) or hi<=lo:raise ValueError('span input')
  control[i//8]|=mask<<((i%8)*4);data.extend([lo,hi])
 return struct.pack('<30f2I',*(data+[0.]*(30-len(data))),*control)
def decode(raw):
 if len(raw)!=128:raise ValueError('packet byte count')
 fields=struct.unpack('<30f2I',raw);control=fields[-2:];n=control[1]>>28
 if n==0 and control[0]==1 and control[1]==0:return [],True
 spans=[[fields[2*i],fields[2*i+1],(control[i//8]>>((i%8)*4))&15] for i in range(n)]
 # Canonical words disallow hidden masks, malformed empties and zero members.
 expected=struct.unpack('<2I',encode(spans)[-8:])
 if expected!=control:raise ValueError('noncanonical mask/count encoding')
 return spans,False

def codec_controls():
 count=0
 for n in range(16):
  for mask in range(1,16):
   spans=[[float(i),float(i)+.5,((mask+i-1)%15)+1] for i in range(n)];raw=encode(spans);decoded,refused=decode(raw)
   assert len(raw)==128 and decoded==spans and not refused;count+=1
 assert decode(encode([],True))==([],True)
 rejected=[]
 for name,fn in [('sixteenth span',lambda:encode([[0.,1.,1]]*16)),('partial fallback',lambda:encode([[0.,1.,1]],True)),('zero active mask',lambda:encode([[0.,1.,0]])),('nonfinite endpoint',lambda:encode([[0.,float('inf'),1]])),('short packet',lambda:decode(bytes(127))),('hidden empty mask',lambda:decode(struct.pack('<30f2I',*([0.]*30),2,0)))]:
  try:fn()
  except ValueError:rejected.append(name)
  else:raise ValueError('codec corruption accepted: '+name)
 return {'round_trips':count+1,'refusals':rejected,'pass':True}

def expand(spans):
 singles=[];overlaps=[]
 for a,b,mask in spans:
  if mask.bit_count()>1:overlaps.append([a,b,mask]);continue
  delta=f32(b-a);n=max(1,math.ceil(f32(delta/4.)));member=mask.bit_length()-1
  for k in range(n):
   lo=a if k==0 else f32(a+f32(f32(f32(k)*delta)/f32(n)))
   hi=b if k+1==n else f32(a+f32(f32(f32(k+1)*delta)/f32(n)))
   singles.append([lo,hi,member])
 return singles,overlaps

def check(report,bulk,baseline,rays,reference,root):
 n=len(rays['rays']);cases=report['streamed_schedule']['cases'];census=report['schedule_census_inputs'];wide=load(root,'ray_transport_gpu_wide_stream');wide.independent_check(report,bulk,rays,reference)
 phase=report['streamed_schedule'].get('phase_only',False)
 expected_capacities=[65536] if phase else [8192,32768]
 if phase and (report['streamed_schedule'].get('phase_consumer_output_checks') is not True or report['streamed_schedule'].get('component_clocks_not_additive') is not True):raise ValueError('phase scope')
 if [c['capacity'] for c in cases]!=expected_capacities or len(census)!=n or len(baseline)!=n:raise ValueError('query/capacity coverage')
 summaries=[]
 for case in cases:
  numeric=case.get('numerical_candidate',False)
  if case.get('same_device_decompression_bit_checked') is not (not numeric):raise ValueError('exact/numerical scope mismatch')
  stats=dict(changed_output_rays=0,changed_endpoint_packets=0,max_RGB_change_vs_bulk=0.,max_T_change_vs_bulk=0.,max_endpoint_change_vs_bulk=0.,original_AST_max_RGB=0.,original_AST_max_T=0.)
  ast_rows=case.get('original_AST_rows',[])
  if numeric and len(ast_rows)!=n:raise ValueError('original AST coverage')
  cap=case['capacity'];tiles=(n+cap-1)//cap
  if (case['tiles'],case['tail_rays'],case['schedule_storage_bytes'],case['uniform_buffer_bytes'])!=(tiles,n-(tiles-1)*cap,cap*128,tiles*288):raise ValueError('ring bytes/tail')
  normal=case['normal_rows'];forced=case['forced_rows'];packets=case['normal_packets'];fp=case['forced_packets'];gpu_expanded=case['GPU_expanded_packets']
  if any(len(v)!=n for v in [normal,forced,packets,fp,gpu_expanded]):raise ValueError('missing output/packet')
  fallbacks=capacity_fallbacks=expanded=0;histogram={}
  for i,(r,b,c,p,force,pf) in enumerate(zip(normal,bulk,census,packets,forced,fp)):
   pixel=rays['rays'][i][:2]
   if any(v[:2]!=pixel for v in [r,b,c,p,force,pf,baseline[i]]):raise ValueError('pixel alignment')
   _,_,flags,spans,word0,word1=p;refused=bool(flags&0x80000000)
   control=struct.unpack('<2I',encode(spans,refused)[-8:])
   if control!=(word0,word1):raise ValueError('packet controls')
   if refused:fallbacks+=1;capacity_fallbacks+=bool(flags&64)
   else:
    if c[2]&0x80000000:raise ValueError('accepted originally refused source')
    predicted_singles,predicted_overlaps=expand(spans);native=gpu_expanded[i]
    if native[:2]!=pixel or native[2]!=flags:raise ValueError('native expansion order/flags')
    singles,overlaps=native[3:];expanded+=len(singles)
    if flags!=len(overlaps):raise ValueError('normal header overlap count')
    if len(singles)!=len(predicted_singles) or len(overlaps)!=len(predicted_overlaps):raise ValueError('native expansion panel count')
    # Actual same-device endpoints, not CPU interpolation, decide bit fidelity.
    at=0;over=0
    for a,z,mask in spans:
     if mask.bit_count()>1:
      if bits(overlaps[over][:2])!=bits([a,z]) or overlaps[over][2]!=mask:raise ValueError('packet overlap round trip')
      over+=1;continue
     panels=max(1,math.ceil(f32(f32(z-a)/4.)));segment=singles[at:at+panels];at+=panels
     if bits([segment[0][0],segment[-1][1]])!=bits([a,z]):raise ValueError('packet outer endpoint round trip')
     if any(v[2]!=mask.bit_length()-1 or not(v[1]>v[0]) for v in segment):raise ValueError('native panel member/order')
     if any(bits([x[1]])!=bits([y[0]]) for x,y in zip(segment,segment[1:])):raise ValueError('panel gap')
    if len(singles)!=len(c[3]) or len(overlaps)!=len(c[4]):raise ValueError('event/panel omission')
    for got,want in zip(singles+overlaps,c[3]+c[4]):
     if got[2]!=want[2] or not all(math.isfinite(v) for v in got) or not(got[1]>got[0]):raise ValueError('expanded endpoint/member structure')
     changed=bits(got[:2])!=bits(want[:2])
     if changed and not numeric:raise ValueError('expanded endpoint/member bits')
     stats['changed_endpoint_packets']+=changed
     stats['max_endpoint_change_vs_bulk']=max(stats['max_endpoint_change_vs_bulk'],*(abs(x-y) for x,y in zip(got[:2],want[:2])))
   histogram[str(len(spans))]=histogram.get(str(len(spans)),0)+1
   want=baseline[i] if refused else b[:6]
   if len(r)!=6 or not all(math.isfinite(v) for v in r):raise ValueError('hybrid output finiteness')
   changed=bits(r[2:])!=bits(want[2:6])
   if changed and (refused or not numeric):raise ValueError('hybrid output bit mismatch')
   stats['changed_output_rays']+=changed
   stats['max_RGB_change_vs_bulk']=max(stats['max_RGB_change_vs_bulk'],*(abs(x-y) for x,y in zip(r[2:5],want[2:5])))
   stats['max_T_change_vs_bulk']=max(stats['max_T_change_vs_bulk'],abs(r[5]-want[5]))
   if numeric:
    ast=ast_rows[i]
    if len(ast)!=6 or ast[:2]!=pixel or not all(math.isfinite(v) for v in ast):raise ValueError('AST order/finiteness')
    stats['original_AST_max_RGB']=max(stats['original_AST_max_RGB'],*(abs(x-y) for x,y in zip(r[2:5],ast[2:5])))
    stats['original_AST_max_T']=max(stats['original_AST_max_T'],abs(r[5]-ast[5]))
   if len(force)!=6 or bits(force[2:])!=bits(baseline[i][2:]) or not all(math.isfinite(v) for v in force):raise ValueError('native forced output')
   if not(pf[2]&0x80000000) or pf[3:]!=[[],1,0]:raise ValueError('forced packet sentinel')
  if (fallbacks,capacity_fallbacks,expanded)!=(case['normal_fallback_rays'],case['capacity_fallback_rays'],case['expanded_single_panels_bit_checked']) or case['forced_fallback_rays']!=n:raise ValueError('saved packet counts')
  if numeric:
   for key,value in stats.items():
    if not math.isclose(value,case['numeric_checks'][key],rel_tol=1e-12,abs_tol=1e-15):raise ValueError('saved numerical delta mismatch: '+key)
   if stats['original_AST_max_RGB']>=1e-5 or stats['original_AST_max_T']>=1e-5:raise ValueError('original AST1e-5 gate')
  refs,indices=reference;selected=dict(rays,rays=[rays['rays'][i] for i in indices]);q=load(root,'ray_transport_gpu_pose_qualification');accuracy=q.reference_metrics([normal[i] for i in indices],selected,refs)
  if not accuracy['both_resolutions_pass']:raise ValueError('compact independent reference gate')
  if [s['repeats'] for s in case['series']]!=[1,2,4]:raise ValueError('command repetition')
  clocks=[]
  for series in case['series']:
   samples=series['samples']
   expected_order=([0,1,2,3,4,5,6,6,5,4,3,2,1,0,6,5,4,3,2,1,0,0,1,2,3,4,5,6] if phase else [0,1,2,2,1,0,2,1,0,0,1,2])*2
   if [x['mode'] for x in samples]!=expected_order:raise ValueError('paired observations/order')
   for x in samples:
    if any(not math.isfinite(x[k]) or x[k]<0 for k in ['encode_ms','completion_ms','total_ms']) or not math.isclose(x['total_ms'],x['encode_ms']+x['completion_ms'],rel_tol=1e-12,abs_tol=1e-10):raise ValueError('complete clock')
   means=[statistics.mean(x['total_ms'] for x in samples if x['mode']==m)/series['repeats'] for m in range(7 if phase else 3)];clocks.append({'repeats':series['repeats'],'current_ms':means[0],'bulk_ms':means[1],'compact_ms':means[2],'current_over_compact':means[0]/means[2]})
   if phase:clocks[-1]['component_ms']=dict(zip(['bulk_producer','bulk_consumer_plus_fallback','compact_producer','compact_consumer_plus_fallback'],means[3:]))
  summaries.append({'capacity':cap,'normal_fallbacks':fallbacks,'capacity_fallbacks':capacity_fallbacks,'expanded_panels':expanded,'event_histogram':histogram,'accuracy':accuracy,'clocks':clocks,'numerical_candidate':numeric,'numeric_checks':stats})
 return {'pass':True,'cases':summaries,'codec':codec_controls()}

def controls(report,bulk,baseline,rays,reference,root):
 passed=[]
 # Each mutation must independently be refused; preserve the physical run.
 def refuse(name,change):
  candidate=copy.deepcopy(report);change(candidate)
  try:check(candidate,bulk,baseline,rays,reference,root)
  except (ValueError,KeyError,IndexError):passed.append(name);return
  raise ValueError('corruption accepted: '+name)
 C=lambda r:r['streamed_schedule']['cases'][0]
 first=next(i for i,p in enumerate(C(report)['normal_packets']) if p[3])
 refuse('missing tail output',lambda r:C(r)['normal_rows'].pop())
 refuse('changed pixel order',lambda r:C(r)['normal_rows'][0].__setitem__(0,-1))
 refuse('nonfinite output',lambda r:C(r)['normal_rows'][0].__setitem__(2,float('nan')))
 refuse('inexact native fallback',lambda r:C(r)['forced_rows'][0].__setitem__(2,C(r)['forced_rows'][0][2]+.1))
 refuse('wrong packed control',lambda r:C(r)['normal_packets'][first].__setitem__(4,0))
 refuse('wrong native member',lambda r:C(r)['GPU_expanded_packets'][first][3][0].__setitem__(2,7))
 refuse('false exactness claim',lambda r:C(r).update(numerical_candidate=False,same_device_decompression_bit_checked=True))
 refuse('changed original AST answer',lambda r:C(r)['original_AST_rows'][first].__setitem__(2,C(r)['original_AST_rows'][first][2]+.1))
 refuse('wrong allocation',lambda r:C(r).update(schedule_storage_bytes=1))
 refuse('missing timing pair',lambda r:C(r)['series'][0]['samples'].pop())
 return passed

def rounding_witness():
 # This reproduces the archived actual native endpoint discrepancy, without
 # asserting which instruction the Metal compiler emitted.
 from fractions import Fraction
 a=25.873123168945313;b=43.049068450927734;k=3;n=5
 step=f32(f32(b-a)/f32(n));sequential=f32(a+f32(f32(k)*step))
 exact=Fraction(a)+k*Fraction(step)
 near=struct.unpack('<I',struct.pack('<f',float(exact)))[0]
 candidates=[struct.unpack('<f',struct.pack('<I',i))[0] for i in range(near-2,near+3)]
 fused=min(candidates,key=lambda x:(abs(Fraction(x)-exact),struct.unpack('<I',struct.pack('<f',x))[0]&1))
 assert sequential==36.178688049316406 and fused==36.17869186401367
 assert struct.unpack('<I',struct.pack('<f',fused))[0]-struct.unpack('<I',struct.pack('<f',sequential))[0]==1
 return {'sequential_f32':sequential,'single_rounding_FMA':fused,'difference':fused-sequential,'ULP_difference':1,'pass':True,'scope':'Exact rational rounding reproduces observed endpoint pair; not Metal disassembly or a universal f32 rule'}

def finish(root):
 # The shared runner validates the ORIGINAL bulk lane; this extra gate validates
 # the numerical compact lane itself before any success claim by its entrypoint.
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--work',type=Path,required=True);parser.add_argument('--case',type=Path,required=True);parser.add_argument('--label',default='normal');a,_=parser.parse_known_args()
 wide=load(root,'ray_transport_gpu_wide_stream');q=load(root,'ray_transport_gpu_pose_qualification');_,rays,reference=wide.verify_case(root,a.case.resolve(),q.sha(root/'saves/zones/Northern Veil/zone.ecform'))
 report_path=a.work/(a.label+'.json');report=json.loads(report_path.read_text());bulk=json.loads(Path(str(report_path)+'-rows.json').read_text());baseline=json.loads(Path(str(report_path)+'-baseline-rows.json').read_text());result=check(report,bulk,baseline,rays,reference,root)
 output=Path(str(report_path)+'-compact-independent.json')
 if output.exists():raise ValueError('compact check output exists')
 output.write_text(json.dumps(result,indent=2)+'\n');print('PASS numerical compact lane: all-query original-AST, exact native fallback, unchanged two-resolution reference gates; no bit-lossless or shipping claim')

def replay_record(root,record):
 import tempfile
 document=json.loads(record.read_text());archives=document['archives']
 def read(key,parse=True):
  meta=archives[key];compressed=(record.parent/meta['file']).read_bytes();raw=gzip.decompress(compressed)
  if hashlib.sha256(compressed).hexdigest()!=meta['compressed_sha256'] or hashlib.sha256(raw).hexdigest()!=meta['sha256']:raise ValueError('archive identity: '+key)
  return json.loads(raw) if parse and meta['kind']=='json' else raw
 for key in archives:read(key,False)
 wide=load(root,'ray_transport_gpu_wide_stream');q=load(root,'ray_transport_gpu_pose_qualification');rays=read('rays');reference=read('reference');manifest=read('manifest');zone=q.sha(root/'saves/zones/Northern Veil/zone.ecform')
 if zone!=manifest['zone_sha256'] or hashlib.sha256(read('rays',False)).hexdigest()!=manifest['rays_sha256'] or hashlib.sha256(read('reference',False)).hexdigest()!=manifest['reference_sha256']:raise ValueError('case source identity')
 with tempfile.TemporaryDirectory(prefix='earthcall-knot-replay-') as td:
  temp=Path(td);rp=temp/'rays.json';fp=temp/'reference.json';mp=temp/'manifest.json';rp.write_bytes(read('rays',False));fp.write_bytes(read('reference',False));m=dict(manifest,rays=str(rp),reference=str(fp));mp.write_text(json.dumps(m));wide.verify_case(root,mp,zone)
 capture=load(root,'ray_transport_wide_evidence').capture_check(read('capture',False),rays,reference)
 results={'normal_query_hashes':[],'capture':capture,'records':{},'codec':codec_controls(),'rounding':rounding_witness(),'archive_count':len(archives)}
 for key in ['numerical','phases','host_refusal','readback_refusal','plain_gpu_refusal','plain','fma']:
  if hashlib.sha256(read(key+'_cpp',False)).hexdigest()!=read(key+'_source')['cpp_sha256'] or read(key+'_source')['zone_sha256']!=zone:raise ValueError('compiled source identity')
 for label in ['numerical','phases']:
  report=read(label+'_report');bulk=read(label+'_rows');baseline=read(label+'_baseline');refs=(reference['rows'],reference['ray_indices']);results['records'][label]=check(report,bulk,baseline,rays,refs,root)
  if report['execution_workload']['distinct_rays']!=32784 or report['execution_workload']['invocations_per_dispatch']!=32784:raise ValueError('legacy label versus actual count')
  for case in report['streamed_schedule']['cases']:
   results['normal_query_hashes'].append(hashlib.sha256(b''.join(bits(row) for row in case['normal_rows'])).hexdigest())
  if label=='numerical':results['negative_controls']=controls(report,bulk,baseline,rays,refs,root)
 plain=read('plain_refusal');fma=read('fma_refusal');w=results['rounding']
 if plain['reason']!='GPU knot changed panel endpoint bits' or plain['detail']['GPU_decompressed'][0]!=w['sequential_f32'] or plain['detail']['original_GPU'][0]!=w['single_rounding_FMA']:raise ValueError('native endpoint refusal')
 if fma['reason']!='knot output changed or missed' or fma['detail']['actual']==fma['detail']['expected']:raise ValueError('native output refusal')
 if b'native run failed without report: -11' not in read('readback_refusal_log',False):raise ValueError('missing readback failure')
 if len(set(results['normal_query_hashes']))!=1:raise ValueError('cross-configuration numerical output mismatch')
 results['pass']=True;return results

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--report',type=Path);p.add_argument('--record',type=Path);p.add_argument('--case',type=Path);p.add_argument('--output',type=Path);p.add_argument('--controls',action='store_true');a=p.parse_args();root=a.root.resolve()
 if a.record:
  result=replay_record(root,a.record.resolve())
  if a.output:
   if a.output.exists():raise ValueError('output exists')
   a.output.write_text(json.dumps(result,indent=2)+'\n')
  print('PASS',result['archive_count'],'archives;3 configurations, exact fallback, numerical references, component clocks,10 corruptions,241 codec round trips and strict native refusal witnesses');return
 if not a.report:print(json.dumps({'codec':codec_controls(),'rounding':rounding_witness()},indent=2));return
 wide=load(root,'ray_transport_gpu_wide_stream');q=load(root,'ray_transport_gpu_pose_qualification');_,rays,reference=wide.verify_case(root,a.case.resolve(),q.sha(root/'saves/zones/Northern Veil/zone.ecform'))
 report=json.loads(a.report.read_text());bulk=json.loads(Path(str(a.report)+'-rows.json').read_text());baseline=json.loads(Path(str(a.report)+'-baseline-rows.json').read_text());result=check(report,bulk,baseline,rays,reference,root);result['rounding_witness']=rounding_witness()
 if a.controls:result['negative_controls']=controls(report,bulk,baseline,rays,reference,root)
 if a.output:
  if a.output.exists():raise ValueError('output exists')
  a.output.write_text(json.dumps(result,indent=2)+'\n')
 print('PASS compact packet expansion/output/fallback, two-resolution reference and complete clocks;',result['codec']['round_trips'],'codec round trips')
if __name__=='__main__':main()
