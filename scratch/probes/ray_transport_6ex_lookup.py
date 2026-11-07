"""Source-bound original vs direct-lookup CPU reference experiment.
No app evaluator is overwritten. No GPU execution or new accuracy coverage.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import copy
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import statistics
import struct
import subprocess
import time
import zipfile
import ray_transport_6ex_lookup_derivation as D
import ray_transport_6ep_reference_derivation as W
import ray_transport_6ew_native as N
import ray_transport_6ew_replay as Q
R=N.R
ROOT=Path(__file__).resolve().parents[2]
ARCHIVE=ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EW_MULTIVIEW_NUMERICAL_PILOT_2026-10-28.zip'
ARCHIVE_SHA='977cf69fb71691f8d70daa62681b7a05fda0ca46221aa46519806dddec71a905'
MATH=ROOT/'src/Singularity/OntoMath/ScalarForm.cpp'
GEN=ROOT/'scratch/probes/veil_projected_reference.cpp'
SEM=Path(__file__).with_name('ray_transport_6ex_semantics.cpp')
COLS=['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half']

def need(b,msg):
 if not b:raise ValueError(msg)

def payload(z,m,name):
 s=m['files'][name];raw=z.read('blobs/'+s);need(hashlib.sha256(raw).hexdigest()==s,'retained payload hash');return raw

def prepare(work):
 need(not work.exists(),'fresh experiment');work.mkdir();out=work/'controls';out.mkdir();need(R.sha(ARCHIVE)==ARCHIVE_SHA,'bound independent parent archive')
 with zipfile.ZipFile(ARCHIVE) as z:
  m=json.loads(z.read('manifest.json'));sel=json.loads(payload(z,m,'pilot/selection.json'));files=['pilot/selection.json','reference/build.json','continuity/rays.json','continuity/historical.json']
  for j in sel['jobs']:files += ['pilot/'+j['input'],'pilot/'+j['name']+'-reference.json','pilot/'+j['name']+'-reference.receipt.json']
  for name in files:
   p=out/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(payload(z,m,name))
 for j in sel['jobs']:
  v=R.read(out/'pilot'/(j['name']+'-reference.json'));rec=R.read(out/'pilot'/(j['name']+'-reference.receipt.json'));need(R.sha(out/'pilot'/(j['name']+'-reference.json'))==rec['reference_sha256'],'historical result binding');Q.reference_binding(rec,j,v,out/'pilot'/j['input'],out/'reference')
 # A fixed ABBA cohort is selected without candidate timing/results.
 original=R.read(out/'pilot'/'inside-ideal-rays.json');ranks=[i*79//31 for i in range(32)];cohort=copy.deepcopy(original);cohort['rays']=[original['rays'][i] for i in ranks];R.write(out/'timing-rays.json',cohort)
 old=R.read(out/'pilot'/'inside-ideal-reference.json');expected=copy.deepcopy(old);expected['rows']=[old['rows'][i] for i in ranks];R.write(out/'timing-expected.json',expected)
 R.write(work/'plan.json',dict(source=R.snapshot(),parent_archive_sha256=ARCHIVE_SHA,reference_rows_cap=2048,reference_seconds_cap=600,threads=8,log2N=16,pilot_rows=880,ABBA_blocks=4,timing_rays=32,timing_ranks=ranks,engine_edits=False,native_GPU=False))

def build(work):
 before=R.snapshot();need(before==R.read(work/'plan.json')['source'],'frozen source');db=R.read(ROOT/'build/compile_commands.json');row=next(v for v in db if v['file'].endswith('/ScalarForm.cpp') and 'earthcall_webgpu.dir' in v['command']);entry=next(v for v in db if v['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in v['command']);linkbase=shlex.split((ROOT/'build/CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());old=next(v for v in linkbase if v.endswith('/src/Singularity/OntoMath/ScalarForm.cpp.o'));linkbase.remove(old)
 source=MATH.read_text();derived=D.derive(source);(work/'ScalarForm-lookup.cpp').write_text(derived);(work/'worker.cpp').write_text(W.derive_source(GEN.read_text()));(work/'canonical.cpp').write_text(GEN.read_text());(work/'semantics.cpp').write_bytes(SEM.read_bytes());(work/'ontomath.cpp').write_bytes((ROOT/'tests/singularity/ontomath_test.cpp').read_bytes())
 commands=[]
 def execute(cmd):
  commands.append(cmd)
  with (work/'build.log').open('a') as f:subprocess.run(cmd,cwd=ROOT/'build',stdout=f,stderr=subprocess.STDOUT,check=True)
 def compile_source(base,src,obj):
  cmd=shlex.split(base);cmd[cmd.index('-c')+1]=str(src);cmd[cmd.index('-o')+1]=str(obj);cmd += ['-std=c++20','-O3','-UNDEBUG','-I'+str(ROOT/'scratch/probes')];need(not any(s.startswith(('-ffast-math','-Ofast','-funsafe-math')) for s in cmd),'ordinary arithmetic');execute(cmd)
 for name,src in [('original',MATH),('lookup',work/'ScalarForm-lookup.cpp')]:compile_source(row['command'],src,work/('math-'+name+'.o'))
 for name in ['worker','canonical','semantics','ontomath']:compile_source(entry['command'],work/(name+'.cpp'),work/(name+'.o'))
 for kind in ['worker','semantics','ontomath','canonical']:
  for mode in ['original','lookup']:
   if kind=='canonical' and mode=='lookup':continue
   link=list(linkbase);link.insert(1,str(work/('math-'+mode+'.o')));link[next(i for i,v in enumerate(link) if v.endswith('/src/entry.cpp.o'))]=str(work/(kind+'.o'));link[link.index('-o')+1]=str(work/(kind+'-'+mode));execute(link)
 files={p.name:R.sha(p) for p in work.iterdir() if p.is_file() and p.name not in ['build.json','build.log','plan.json']};meta=dict(before=before,after=R.snapshot(),parent_math_sha256=R.sha(MATH),derived_math_sha256=R.sha(work/'ScalarForm-lookup.cpp'),reference_generator_sha256=R.sha(GEN),worker_derivation=W.derivation_identity(GEN.read_text()),commands=commands,files_sha256=files,tools_sha256={p.name:R.sha(p) for p in [Path(__file__),SEM,Path(D.__file__)]});need(meta['before']==meta['after'],'source changed during build');R.write(work/'build.json',meta);print('PASS original/derived object and7 CPU binaries built',flush=True)

def values(v):
 need(v['columns']==COLS,'reference columns');need(all(len(r)==8 and all(math.isfinite(x) for x in r) for r in v['rows']),'finite complete reference')
 return b''.join(struct.pack('<d',x) for r in v['rows'] for x in r)

def compare(actual,expected):
 need(len(actual['rows'])==len(expected['rows']),'complete reference row count');a=values(actual);b=values(expected);need(a==b,'binary64 reference drift')
 return len(a)//8

def run_reference(work,exe,inp,expected,name):
 meta=R.read(work/'build.json');plan=R.read(work/'plan.json');need(R.snapshot()==meta['after']==plan['source'],'reference engine binding');binary=work/exe;need(R.sha(binary)==meta['files_sha256'][exe],'actual executable identity');need(R.sha(MATH)==meta['parent_math_sha256'],'unchanged production evaluator');n=len(R.read(inp)['rays']);budget_path=work/'budget.json';budget=R.read(budget_path) if budget_path.exists() else dict(rows=0,seconds=0);need(budget['rows']+n<=2048 and budget['seconds']<600,'declared cost cap');out=work/'runs'/name;out.mkdir(parents=True);args=[str(binary),str(inp),'16',str(n),'8',str(out/'reference.json')];env={k:v for k,v in os.environ.items() if not k.startswith('EC_')};t=time.monotonic();started=dt.datetime.now(dt.timezone.utc).isoformat();before=R.snapshot();input_sha=R.sha(inp)
 with (out/'run.log').open('w') as log:
  try:code=subprocess.run(args,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=min(120,600-budget['seconds'])).returncode
  except subprocess.TimeoutExpired:code='timeout'
 elapsed=time.monotonic()-t;R.write(budget_path,dict(rows=budget['rows']+n,seconds=budget['seconds']+elapsed));receipt=dict(argv=args,exit_code=code,elapsed_seconds=elapsed,started_utc=started,binary_sha256=R.sha(binary),input_sha256=input_sha,input_sha256_after=R.sha(inp),before=before,after=R.snapshot(),cwd=str(work),runner_sha256=R.sha(Path(__file__)),build_sha256=R.sha(work/'build.json'),rows=n,files_sha256={p.name:R.sha(p) for p in out.iterdir()});R.write(out/'receipt.json',receipt);need(code==0 and receipt['before']==receipt['after'] and receipt['input_sha256']==receipt['input_sha256_after'],'complete fixed reference execution');actual=R.read(out/'reference.json');want=R.read(expected);count=compare(actual,want);spread=max(abs(r[i]-r[i+4]) for r in actual['rows'] for i in range(4));need(spread<=1e-6,'unchanged convergence gate');print('PASS',name,'rows',n,'seconds',round(elapsed,4),'values',count,flush=True);return receipt

def checks(work):
 out=work/'checks';out.mkdir()
 for mode in ['original','lookup']:
  for kind in ['semantics','ontomath']:
   binary=work/(kind+'-'+mode);log=out/(kind+'-'+mode+'.log');args=[str(binary)]+([str(out/(mode+'.bin'))] if kind=='semantics' else []);t=time.monotonic()
   with log.open('w') as f:code=subprocess.run(args,cwd=out,stdout=f,stderr=subprocess.STDOUT,timeout=60).returncode
   R.write(out/(kind+'-'+mode+'.receipt.json'),dict(argv=args,exit_code=code,elapsed_seconds=time.monotonic()-t,binary_sha256=R.sha(binary),cwd=str(out),runner_sha256=R.sha(Path(__file__)),build_sha256=R.sha(work/'build.json'),log_sha256=R.sha(log)));need(code==0,'general semantics / existing suite '+mode+' '+kind)
 need((out/'original.bin').read_bytes()==(out/'lookup.bin').read_bytes(),'general semantic exactness');print('PASS general scalar/coercion/domain/rebinding/8worker bits and original ontomath suite',flush=True)

def run(work):
 fixture=work/'saves/zones/Northern Veil/zone.ecform';fixture.parent.mkdir(parents=True);fixture.write_bytes(R.ZONE.read_bytes());need(R.sha(fixture)==R.sha(R.ZONE),'isolated frozen fixture');checks(work);control=work/'controls';rays=control/'continuity/rays.json';hist=control/'continuity/historical.json'
 for exe in ['canonical-original','worker-original','worker-lookup']:run_reference(work,exe,rays,hist,'continuity-'+exe)
 jobs=R.read(control/'pilot/selection.json')['jobs'];first=jobs[0];run_reference(work,'worker-original',control/'pilot'/first['input'],control/'pilot'/(first['name']+'-reference.json'),'fresh-control-'+first['name'])
 for j in jobs:run_reference(work,'worker-lookup',control/'pilot'/j['input'],control/'pilot'/(j['name']+'-reference.json'),'retained-'+j['name'])
 for b in range(4):
  for i,mode in enumerate(['original','lookup','lookup','original']):run_reference(work,'worker-'+mode,control/'timing-rays.json',control/'timing-expected.json',f'timing-{b}-{i}-{mode}')
 print('PASS bounded experiment complete',flush=True)

def summarize(work):
 b=R.read(work/'build.json');need(b['before']==b['after'],'stable engine build');need(R.sha(MATH)==b['parent_math_sha256']==D.PARENT_SHA,'original math source retained');need((work/'ScalarForm-lookup.cpp').read_text()==D.derive(MATH.read_text()),'closed source derivation');need((work/'worker.cpp').read_text()==W.derive_source(GEN.read_text()),'closed reference adapter');need(R.sha(work/'canonical.cpp')==W.CANONICAL_SHA,'original canonical generator');receipts=[];total_values=0
 for path in sorted((work/'runs').iterdir()):
  r=R.read(path/'receipt.json');need(r['exit_code']==0 and r['before']==r['after']==b['after'],'stored execution/source');need(r['build_sha256']==R.sha(work/'build.json'),'execution build');exe=Path(r['argv'][0]).name;need(r['binary_sha256']==b['files_sha256'][exe],'receipt executable');need(r['runner_sha256']==R.sha(Path(__file__)),'executed runner identity')
  # On archive replay, resolve original absolute input through frozen filename.
  candidates=list((work/'controls').rglob(Path(r['argv'][1]).name));need(len(candidates)==1,'unambiguous retained input');inp=candidates[0];need(R.sha(inp)==r['input_sha256']==r['input_sha256_after'],'stored input pairing');n=len(R.read(inp)['rays']);need(r['rows']==n and r['argv'][2:5]==['16',str(n),'8'] and 0<r['elapsed_seconds']<=120,'stored execution scope')
  for name,h in r['files_sha256'].items():need(R.sha(path/name)==h,'stored output/log bytes')
  name=path.name
  if name.startswith('continuity-'):expected=work/'controls/continuity/historical.json'
  elif name.startswith('timing-'):expected=work/'controls/timing-expected.json'
  else:
   jobname=name.removeprefix('fresh-control-').removeprefix('retained-');expected=work/'controls/pilot'/(jobname+'-reference.json')
  total_values+=compare(R.read(path/'reference.json'),R.read(expected));receipts.append(r)
 budget=R.read(work/'budget.json');need(budget['rows']==sum(r['rows'] for r in receipts)<=2048 and budget['seconds']==sum(r['elapsed_seconds'] for r in receipts)<=600,'total budget receipt accounting')
 original=json.loads((work/'checks/semantics-original.log').read_text().splitlines()[-1]);lookup=json.loads((work/'checks/semantics-lookup.log').read_text().splitlines()[-1]);need((work/'checks/original.bin').read_bytes()==(work/'checks/lookup.bin').read_bytes(),'semantic records');need(original['allocation_calls_10000_scalar_leaves']==20000 and lookup['allocation_calls_10000_scalar_leaves']==0,'measured leaf allocation controls')
 blocks=[]
 for block in range(4):
  rs=[R.read(next((work/'runs').glob(f'timing-{block}-{i}-*'))/'receipt.json') for i in range(4)];a=(rs[0]['elapsed_seconds']+rs[3]['elapsed_seconds'])/2;c=(rs[1]['elapsed_seconds']+rs[2]['elapsed_seconds'])/2;blocks.append(dict(block=block,original_seconds=a,lookup_seconds=c,ratio=c/a))
 return dict(schema='6ex-external-lookup-reference-experiment-1',reference_rows=budget['rows'],reference_seconds=budget['seconds'],binary64_values=total_values,all_exact=True,allocation_witness=dict(original=original,lookup=lookup),timing_blocks=blocks,median_paired_ratio=statistics.median(x['ratio'] for x in blocks),blocks_faster=sum(x['ratio']<1 for x in blocks),engine_source_unchanged=True,new_reference_population_coverage=0,native_GPU=False,full_population_qualified=False,Person_acceptance=False)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['prepare','build','run','summarize']);p.add_argument('--work',type=Path,required=True);a=p.parse_args();work=a.work.resolve()
 if a.command=='prepare':prepare(work)
 elif a.command=='build':build(work)
 elif a.command=='run':run(work)
 else:R.write(work/'result.json',summarize(work));print(json.dumps(R.read(work/'result.json'),indent=2))
if __name__=='__main__':main()
