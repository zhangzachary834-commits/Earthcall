"""Bounded new-source CPU references for the actual live-consumer inputs.
Original OntoMath AST evaluator and Simpson generator; worker map reuse is the
previously closed §6ep derivation. No new rendering equations or tolerance.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import time
import zipfile

ROOT=Path(__file__).resolve().parents[2]
import ray_transport_6ew_native as N
import ray_transport_6ep_reference_derivation as D
R=N.R
GENERATOR=ROOT/'scratch/probes/veil_projected_reference.cpp'
TEST=ROOT/'tests/singularity/webgpu_line_quadrature_test.cpp'

def build(work):
 R.need(not work.exists(),'fresh CPU build');work.mkdir();before=R.snapshot();source=ROOT/'src/Singularity/OntoMath/ScalarForm.cpp';db=R.read(ROOT/'build/compile_commands.json')
 row=next(v for v in db if v['file'].endswith('/ScalarForm.cpp') and 'earthcall_webgpu.dir' in v['command']);math=shlex.split(row['command']);math[math.index('-o')+1]=str(work/'ScalarForm.o');math+=['-O3'];R.need(not any(s.startswith(('-ffast-math','-Ofast','-funsafe-math')) for s in math),'ordinary FP contract');subprocess.run(math,cwd=ROOT/'build',check=True)
 entry=next(v for v in db if v['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in v['command']);base_link=shlex.split((ROOT/'build/CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());old=next(v for v in base_link if v.endswith('/src/Singularity/OntoMath/ScalarForm.cpp.o'));base_link.remove(old);base_link.insert(1,str(work/'ScalarForm.o'))
 metadata=dict(before=before,math_source_sha256=R.sha(source),math_object_sha256=R.sha(work/'ScalarForm.o'),math_compile_argv=math,canonical_sha256=R.sha(GENERATOR),imported_test_sha256=R.sha(TEST),derivation=D.derivation_identity(GENERATOR.read_text()),executables={})
 for name,text in [('canonical',GENERATOR.read_text()),('worker',D.derive_source(GENERATOR.read_text()))]:
  cpp=work/(name+'.cpp');cpp.write_text(text);cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(cpp);cmd[cmd.index('-o')+1]=str(work/(name+'.o'));cmd+=['-std=c++20','-O3','-I'+str(ROOT/'scratch/probes')];subprocess.run(cmd,cwd=ROOT/'build',check=True)
  link=list(base_link);link[next(i for i,v in enumerate(link) if v.endswith('/src/entry.cpp.o'))]=str(work/(name+'.o'));link[link.index('-o')+1]=str(work/name);subprocess.run(link,cwd=ROOT/'build',check=True)
  metadata['executables'][name]=dict(source_sha256=R.sha(cpp),object_sha256=R.sha(work/(name+'.o')),binary_sha256=R.sha(work/name),compile_argv=cmd,link_argv=link)
 metadata['after']=R.snapshot();R.need(before==metadata['after'],'fixed CPU source');R.write(work/'build.json',metadata)

def fingerprint(work,name):
 b=R.read(work/'build.json');R.need(b['after']==R.snapshot(),'reference engine binding');R.need(R.sha(GENERATOR)==b['canonical_sha256']==D.CANONICAL_SHA,'original reference generator');R.need(R.sha(TEST)==b['imported_test_sha256'],'imported test source');R.need(R.sha(work/'ScalarForm.o')==b['math_object_sha256'] and R.sha(ROOT/'src/Singularity/OntoMath/ScalarForm.cpp')==b['math_source_sha256'],'reference evaluator binding')
 exe=b['executables'][name];R.need(R.sha(work/name)==exe['binary_sha256'] and R.sha(work/(name+'.cpp'))==exe['source_sha256'],'CPU executable binding')
 return dict(engine=b['after'],build_sha256=R.sha(work/'build.json'),binary_sha256=exe['binary_sha256'],source_sha256=exe['source_sha256'])

def run_job(work,name,path,out,timeout):
 before=fingerprint(work,name);budget_path=work/'budget.json';budget=R.read(budget_path) if budget_path.exists() else dict(seconds=0,rows=0);count=len(R.read(path)['rays']);R.need(budget['rows']+count<=960 and budget['seconds']<600,'total frozen reference cap');timeout=min(timeout,600-budget['seconds']);R.need(not out.exists(),'fresh reference output');log=out.with_suffix('.log');argv=[str(work/name),str(path),'16',str(len(R.read(path)['rays'])),'8',str(out)];start=dt.datetime.now(dt.timezone.utc).isoformat();t=time.monotonic();input_sha=R.sha(path);env={k:v for k,v in os.environ.items() if not k.startswith('EC_')}
 with log.open('w') as f:
  try:code=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout).returncode
  except subprocess.TimeoutExpired:code='timeout'
 receipt=dict(argv=argv,started_utc=start,elapsed_seconds=time.monotonic()-t,exit_code=code,before=before,after=fingerprint(work,name),input_sha256=input_sha,input_sha256_after=R.sha(path),log_sha256=R.sha(log),reference_sha256=R.sha(out) if out.exists() else None)
 R.write(budget_path,dict(seconds=budget['seconds']+receipt['elapsed_seconds'],rows=budget['rows']+count));R.write(out.with_suffix('.receipt.json'),receipt);R.need(code==0 and receipt['before']==receipt['after'] and receipt['input_sha256']==receipt['input_sha256_after'],'reference execution binding')
 return R.read(out),receipt

def continuity(work,out):
 out.mkdir();arc=ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.zip'
 with zipfile.ZipFile(arc) as z:
  (out/'rays.json').write_bytes(z.read('rays/continuity.json'));(out/'historical.json').write_bytes(z.read('optimized-first/continuity-reference.json'))
 expected=R.read(out/'historical.json');a,_=run_job(work,'canonical',out/'rays.json',out/'canonical.json',120);b,_=run_job(work,'worker',out/'rays.json',out/'worker.json',120)
 def values(v):return b''.join(struct.pack('<d',x) for row in v['rows'] for x in row)
 R.need(a['columns']==b['columns']==expected['columns'] and values(a)==values(b)==values(expected),'exact historical/canonical/worker continuity');print('PASS continuity',len(a['rows']),flush=True)

def run_selection(work,selection):
 s=R.read(selection);start=time.monotonic();total=len(s['jobs'])*80+36;R.need(total<=960 and s['reference_seconds_cap']==600,'fixed pilot cap')
 for job in s['jobs']:
  remaining=600-(time.monotonic()-start);R.need(remaining>0,'total CPU reference cap');path=selection.parent/job['input'];R.need(R.sha(path)==job['input_sha256'],'frozen selected input');out=selection.parent/(job['name']+'-reference.json');values,rec=run_job(work,'worker',path,out,min(180,remaining));rows=values['rows'];conv=max(abs(row[k]-row[k+4]) for row in rows for k in range(4));R.need(conv<=1e-6,'convergence >1e-6 '+job['name']);print('PASS reference',job['name'],len(rows),rec['elapsed_seconds'],conv,flush=True)


def main():
 p=argparse.ArgumentParser(description=__doc__);q=p.add_subparsers(dest='command',required=True);a=q.add_parser('build');a.add_argument('--work',type=Path,required=True);a=q.add_parser('continuity');a.add_argument('--work',type=Path,required=True);a.add_argument('--out',type=Path,required=True);a=q.add_parser('run');a.add_argument('--work',type=Path,required=True);a.add_argument('--selection',type=Path,required=True);a=p.parse_args()
 if a.command=='build':build(a.work.resolve())
 elif a.command=='continuity':continuity(a.work.resolve(),a.out.resolve())
 else:run_selection(a.work.resolve(),a.selection.resolve())

if __name__=='__main__':main()
