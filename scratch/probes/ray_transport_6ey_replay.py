"""Validate/replay actual Engine Release movement receipts and paired costs.
No timing, authored accuracy or Person acceptance is inferred from CPU replay.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse,copy,hashlib,json,math,statistics,struct,tempfile,zipfile,zlib
from pathlib import Path,PurePosixPath
import ray_transport_6ey_engine as E
R=E.R

def need(v,msg):
 if not v:raise ValueError(msg)

def png(path):
 b=path.read_bytes();need(b[:8]==b'\x89PNG\r\n\x1a\n','PNG signature');p=8;data=b'';w=h=None
 while p<len(b):
  n=struct.unpack_from('>I',b,p)[0];kind=b[p+4:p+8];raw=b[p+8:p+8+n];crc=struct.unpack_from('>I',b,p+8+n)[0];need(zlib.crc32(kind+raw)&0xffffffff==crc,'PNG CRC');p+=12+n
  if kind==b'IHDR':w,h=struct.unpack_from('>II',raw);need(raw[8:]==bytes([8,6,0,0,0]),'RGBA8 no-interlace PNG')
  if kind==b'IDAT':data+=raw
 need((w,h)==(2560,1440),'native image extent');raw=zlib.decompress(data);stride=w*4;need(len(raw)==h*(stride+1),'complete image payload');need(all(raw[i*(stride+1)]==0 for i in range(h)),'existing ScreenRecorder filter contract');return b''.join(raw[i*(stride+1)+1:(i+1)*(stride+1)] for i in range(h))

def preflight(work,directory='preflight'):
 j=R.read(work/directory/'result.json');need(j['framebuffer']==[2560,1440] and j['objects']==24 and j['media']==4 and j['foreign_network_service'] is False,'actual Engine fixture scope');need(len(j['captures'])==6,'six native scene captures');comparisons=[]
 for i in [0,32]:
  rows=[next(v for v in j['captures'] if v['index']==i and v['variant']==k) for k in range(3)];cur,on,forced=[png(work/directory/v['snapshot']) for v in rows];need(cur==forced,'forced Current exact scene fallback');need(cur!=on,'normal transport must affect actual Screen');normal=rows[1]['publication'];refusal=rows[2]['publication'];need(normal['nonfinite']==0 and normal['nonempty']>100 and normal['accepted']+normal['refused']==2560*1440,'normal publication');need(refusal['nonempty']==0 and refusal['accepted']==0 and refusal['refused']==2560*1440,'whole-ray forced refusal');comparisons.append(dict(index=i,exact_fallback_bytes=len(cur),normal_changed_bytes=sum(a!=b for a,b in zip(cur,on)),normal_publication=normal,forced_publication=refusal))
 return comparisons

def stats(v):
 need(v and all(math.isfinite(x) and x>=0 for x in v),'finite complete timings');q=sorted(v);return dict(mean=sum(q)/len(q),p50=q[len(q)//2],p95=q[len(q)*95//100],p99=q[len(q)*99//100],max=q[-1])

def path(rows,on,compile_allowed=False):
 need(len(rows)==64 and [r['index'] for r in rows]==list(range(64)),'complete ordered moving path');eyes=[]
 for i,r in enumerate(rows):
  a=2*math.pi*i/64;expected=[8*math.sin(a),50+2*math.sin(a*.5),75+8*(1-math.cos(a))];need(max(abs(r['eye'][k]-expected[k]) for k in range(3))<1e-4,'independent scripted eye');need(abs(sum(x*x for x in r['front'])-1)<1e-5,'unit camera direction');need(abs(r['world_time']-(7.25+i/60+struct.unpack('<f',struct.pack('<f',1/60))[0]))<1e-10,'actual changing world clock');need((r['routed_sets']>0)==on and (on or r['routed_pixels']==0),'lane actually executes');need(compile_allowed or r['volume_compiles']==0,'paid warmup excludes compile');eyes.append(tuple(r['eye']))
 need(len(set(eyes))==64,'movement cannot be frozen');need(all(r['focused'] is True for r in rows),'foreground throughout actual moving path')
 return dict(poses=64,distinct_eyes=64,first_time=rows[0]['world_time'],last_time=rows[-1]['world_time'])

def timing_result(j):
 samples=j['samples'];need(len(samples)==32,'four ABBA blocks per two styles');results={};allframes=0
 for serial in [False,True]:
  blocks=[];aggregates={False:[],True:[]};parts={False:[],True:[]}
  for block in range(4):
   rows=[next(s for s in samples if s['serial']==serial and s['block']==block and s['order']==order) for order in range(4)];need([s['on'] for s in rows]==[False,True,True,False],'paired ABBA mode order')
   for s in rows:
    path(s['frames'],s['on']);allframes+=len(s['frames']);activation=s['mode_activation_frames'];need(len(activation)==2 and all(v['index']==0 and ((v['routed_sets']>0)==s['on']) for v in activation),'explicit paid mode activation');need(s['mode_activation_ms']>=0,'mode activation cost');need(s['total_ms']>=s['terminal_drain_ms']>=0,'complete queue-drained scope');aggregates[s['on']].extend(r['tick_ms'] for r in s['frames']);parts[s['on']].append(s['total_ms']/64)
   a=(rows[0]['total_ms']+rows[3]['total_ms'])/128;b=(rows[1]['total_ms']+rows[2]['total_ms'])/128;blocks.append(dict(block=block,Current_ms_per_frame=a,routed_ms_per_frame=b,ratio=b/a))
  metrics={}
  for on in [False,True]:
   ss=[s for s in samples if s['serial']==serial and s['on']==on];allrows=[r for s in ss for r in s['frames']];metrics['routed' if on else 'Current']=dict(host_tick_ms=stats(aggregates[on]),mean_complete_sample_ms_per_frame=sum(s['total_ms'] for s in ss)/(8*64),completion_inclusive_frame_ms=stats([r['completion_inclusive_ms'] for r in allrows]),host_ticks_above_33_333ms=sum(r['tick_ms']>1000/30 for r in allrows),host_ticks_above_50ms=sum(r['tick_ms']>50 for r in allrows),phase_means_ms={name:sum(r[name] for r in allrows)/len(allrows) for name in ['render3d_ms','surface_ms','submit_ms','gui_ms','zone_ms','laws_ms']},terminal_queue_drain_ms=[s['terminal_drain_ms'] for s in ss],mode_activation_ms=[s['mode_activation_ms'] for s in ss],mode_activation_program_compiles=[sum(v['volume_compiles'] for v in s['mode_activation_frames']) for s in ss])
  results['serial_completion' if serial else 'ordinary_queued_ticks']=dict(blocks=blocks,median_paired_ratio=statistics.median(b['ratio'] for b in blocks),blocks_favor_routed=sum(b['ratio']<1 for b in blocks),metrics=metrics)
 need(allframes==2048,'measured frame population')
 return results

def validate(work):
 meta=R.read(work/'build.json');need(meta['before']==meta['after'],'fixed Release source');need(meta['release_build']['exit_code']==0 and meta['release_configure']['exit_code']==0 and meta['build_type']=='Release','actual app build');need(R.sha(E.CPP)==meta['source_sha256'] and R.sha(Path(E.__file__))==meta['tools_sha256'][Path(E.__file__).name],'matching executed tools');need('-O3' in meta['compile_argv'] and '-DNDEBUG' in meta['compile_argv'],'Release harness flags');need(len(meta['object_sha256'])==238,'complete Release app object set')
 for source,sha in meta['after']['source_files'].items():need(R.sha(E.ROOT/source)==sha,'matching historical engine source')
 j=R.read(work/'session/result.json');receipt=R.read(work/'session/receipt.json');need(receipt['exit_code']==0 and receipt['mode']=='session' and receipt['before']==receipt['after']==meta['after'],'successful actual native session');need(receipt['binary_sha256']==meta['binary_sha256'] and receipt['build_sha256']==R.sha(work/'build.json'),'session executable/build identity');need(receipt['final_zone_sha256']==receipt['initial_fixture_sha256']['zones/Northern Veil/zone.ecform']==meta['after']['zone_sha256'],'frozen authored scene');need(0<receipt['elapsed_seconds']<=600,'bounded final process')
 for name,sha in receipt['files_sha256'].items():need(R.sha(work/'session'/name)==sha,'native final output/log bytes')
 need(j['foreign_network_service'] is False and j['foreground_required'] is True,'declared foreground/network scope');need(j['same_process_movement_negative']=='rejected at scripted camera pose assertion','executed targeted negative');need(len(j['startup_attempts'])<=16 and j['startup_attempts'][-1]['routed_sets']>0,'paid valid-frame readiness');need(all(v['focused'] for v in j['startup_attempts']),'foreground startup')
 
 for name,sha in receipt['initial_fixture_sha256'].items():need(R.sha(work/'initial-fixture'/name)==sha,'exact initial fixture payload')
 captures=preflight(work,'session');need(len(j['warmup'])==128,'two complete warm paths');path(j['warmup'][:64],False,True);path(j['warmup'][64:],True,True);results=timing_result(j);total=receipt['elapsed_seconds'];history=[]
 for entry in R.read(work/'history.json')['entries']:
  directory=work/entry['directory'];old=R.read(directory/'receipt.json');build=R.read(directory/'build.json');need(R.sha(directory/'build.json')==old['build_sha256'],'prior build identity');need(old['binary_sha256']==build['binary_sha256'] and old['before']==old['after']==meta['after'],'prior source/executable identity')
  for name,sha in old['files_sha256'].items():need(R.sha(directory/name)==sha,'prior attempt output identity')
  if (directory/'engine.cpp').exists():need(R.sha(directory/'engine.cpp')==build['source_sha256'],'available prior source bytes')
  total+=old['elapsed_seconds'];history.append(dict(name=entry['name'],mode=old['mode'],exit_code=old['exit_code'],seconds=old['elapsed_seconds'],source_bytes_retained=(directory/'engine.cpp').exists()))
 budget=R.read(work/'native-budget.json');need(abs(budget['seconds']-total)<1e-8 and total<=900,'all-attempt cumulative process cap');need(len(history)==16,'all16 earlier native attempts retained')
 log=(work/'session/native.log').read_text();need('PASS actual Engine session' in log and not any(s in log.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'normal native completion without GPU failure');need(len([s for s in log.splitlines() if s.startswith('SAMPLE ')])==32,'complete timing log')
 negative=[]
 def reject(name,fn):
  try:fn()
  except (ValueError,KeyError):negative.append(name)
  else:raise ValueError('counterexample admitted '+name)
 q=copy.deepcopy(j);q['samples'][0]['frames']=q['samples'][0]['frames'][:-1];reject('missing-timed-frame',lambda:timing_result(q));q=copy.deepcopy(j);q['samples'][0]['on']=True;reject('forged-ABBA-mode',lambda:timing_result(q));q=copy.deepcopy(j);q['samples'][0]['frames'][1]['eye']=q['samples'][0]['frames'][0]['eye'];reject('frozen-camera',lambda:timing_result(q));q=copy.deepcopy(j);q['samples'][0]['frames'][1]['tick_ms']=float('nan');reject('nonfinite-timing',lambda:timing_result(q));q=copy.deepcopy(j);q['samples'][0]['frames'][1]['focused']=False;reject('unfocused-frame',lambda:timing_result(q));q=copy.deepcopy(j);q['samples'][0]['frames'][1]['volume_compiles']=1;reject('unpaid-recomposition',lambda:timing_result(q))
 return dict(schema='6ey-real-Engine-Release-foreground-scripted-window-1',frames=2048,path_warmup_frames=128,mode_activation_frames=64,viewport=[2560,1440],scene_objects=24,volume_media=4,successful_process_seconds=receipt['elapsed_seconds'],all_native_process_seconds=total,all_native_process_cap_seconds=900,prior_attempts=history,preflight=captures,actual_native_negative=j['same_process_movement_negative'],checker_negatives=negative,timings=results,actual_new_authored_reference_rows=0,authored_accuracy_qualified=False,Person_movement_acceptance=False,physical_scanout_or_display_latency_measured=False,production_source_changed=False,scope='Actual fresh-Release Engine, production scene/UI/window/FIFO, isolated frozen data, scripted fixed-step camera/time and detailed host records. Foreground witnessed on every timed frame. Mode activation and warmup recorded outside steady intervals; terminal queue drain included. Network service stopped; no timestamp queries, ordinary human WASD or full-population mathematical admission.')

def package(work,archive):
 result=validate(work);R.write(work/'result.json',result);files={}
 for name in ['build.json','native-budget.json','history.json','result.json']:files['experiment/'+name]=(work/name).read_bytes()
 for directory in [work/'session',work/'prior',work/'initial-fixture']:
  for p in directory.rglob('*'):
   if p.is_file() and not p.is_symlink():
    if directory.name=='session' and p.parent!=directory:continue
    files['experiment/'+p.relative_to(work).as_posix()]=p.read_bytes()
 for name in ['configure-receipt.json','build-receipt.json','CMakeCache.txt','compile_commands.json']:
  files['Release/'+name]=(E.BUILD/name).read_bytes()
 for p in [Path(__file__),Path(E.__file__),E.CPP]:files['tools/'+p.name]=p.read_bytes()
 manifest=dict(schema='6ey-content-addressed-native-session-1',files={name:hashlib.sha256(raw).hexdigest() for name,raw in files.items()})
 with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  z.writestr('manifest.json',json.dumps(manifest,sort_keys=True));seen=set()
  for name,raw in files.items():
   sha=manifest['files'][name]
   if sha not in seen:z.writestr('blobs/'+sha,raw);seen.add(sha)
 print('PACKAGED',archive.stat().st_size,R.sha(archive));return result

def replay(archive):
 with tempfile.TemporaryDirectory(prefix='6ey-replay-') as tmp,zipfile.ZipFile(archive) as z:
  root=Path(tmp);m=json.loads(z.read('manifest.json'));need(set(z.namelist())=={'manifest.json'}|{'blobs/'+h for h in m['files'].values()},'archive inventory')
  for name,sha in m['files'].items():
   p=PurePosixPath(name);need(not p.is_absolute() and '..' not in p.parts,'safe archive path');raw=z.read('blobs/'+sha);need(hashlib.sha256(raw).hexdigest()==sha,'archive payload identity');out=root/name;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(raw)
  for p in [Path(__file__),Path(E.__file__),E.CPP]:need(p.read_bytes()==(root/'tools'/p.name).read_bytes(),'matching replay tools')
  result=validate(root/'experiment');need(result==R.read(root/'experiment/result.json'),'complete stored result replay');print('PASS stored native session:2048 frames,6 actual scene captures,6 checker refusals; not fresh GPU execution');return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['preflight','validate','package','replay']);p.add_argument('path',type=Path);p.add_argument('--archive',type=Path);a=p.parse_args()
 if a.command=='preflight':result=preflight(a.path)
 elif a.command=='package':result=package(a.path,a.archive)
 elif a.command=='replay':result=replay(a.path)
 else:result=validate(a.path)
 print(json.dumps(result,indent=2))
if __name__=='__main__':main()
