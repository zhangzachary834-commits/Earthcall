"""Reproduce the bounded CPU lookup experiment from retained independent controls.
This is CPU replay of stored observations, not a fresh timing or GPU execution.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path,PurePosixPath
import struct
import tempfile
import zipfile
import ray_transport_6ex_lookup as X
D=X.D
R=X.R

def need(b,msg):
 if not b:raise ValueError(msg)

def refused(name,fn,out):
 try:fn()
 except (ValueError,KeyError):out.append(name)
 else:raise ValueError('negative admitted: '+name)

def validate(work):
 meta=R.read(work/'build.json');need(meta['before']==meta['after']==R.read(work/'plan.json')['source'],'fixed source build');need(R.sha(work/'executed-build-runner.py')==meta['tools_sha256']['ray_transport_6ex_lookup.py'],'original build-stage tool retained');need(R.sha(Path(D.__file__))==meta['tools_sha256']['ray_transport_6ex_lookup_derivation.py'],'same math derivation');need(R.sha(X.SEM)==meta['tools_sha256']['ray_transport_6ex_semantics.cpp'],'same semantics harness');need(R.sha(work/'ScalarForm-lookup.cpp')==meta['derived_math_sha256'],'derived evaluator binding')
 # Object/binary hashes describe actual build products; replay stores source,
 # commands and execution receipts, not hundreds of MB of duplicate binaries.
 for name in ['worker','canonical','semantics','ontomath']:
  need(R.sha(work/(name+'.cpp'))==meta['files_sha256'][name+'.cpp'],'compiled main source')
 need(R.sha(work/'saves/zones/Northern Veil/zone.ecform')==R.sha(R.ZONE),'isolated fixture bytes')
 for command in meta['commands']:
  need(not any(v.startswith(('-ffast-math','-Ofast','-funsafe-math')) for v in command),'ordinary FP flags')
  if '-c' in command:need('-O3' in command and '-UNDEBUG' in command,'optimized assertion-enabled build')
  else:
   math_objects=[v for v in command if Path(v).name in ['math-original.o','math-lookup.o']];need(len(math_objects)==1 and command[1]==math_objects[0],'evaluator object linked first');need(not any(v.endswith('/src/Singularity/OntoMath/ScalarForm.cpp.o') for v in command),'old evaluator object excluded')
 # Bind retained expected rows to the prior independent archive, rather than
 # allowing candidate and expectation to be altered together.
 need(R.sha(X.ARCHIVE)==X.ARCHIVE_SHA,'parent evidence archive')
 with zipfile.ZipFile(X.ARCHIVE) as z:
  manifest=json.loads(z.read('manifest.json'))
  for p in (work/'controls').rglob('*'):
   if not p.is_file() or p.name.startswith('timing-'):continue
   relative=p.relative_to(work/'controls').as_posix();need(p.read_bytes()==X.payload(z,manifest,relative),'independent retained control '+relative)
  need((work/'saves/zones/Northern Veil/zone.ecform').read_bytes()==R.ZONE.read_bytes(),'same reference scene')
 # Timing subset is derived from the frozen input / independently held results.
 plan=R.read(work/'plan.json');ranks=[i*79//31 for i in range(32)];need(plan['timing_ranks']==ranks and plan['ABBA_blocks']==4 and plan['reference_rows_cap']==2048 and plan['reference_seconds_cap']==600,'predeclared scope')
 a=R.read(work/'controls/pilot/inside-ideal-rays.json');b=R.read(work/'controls/timing-rays.json');need(b['rays']==[a['rays'][i] for i in ranks],'frozen timing input subset');a=R.read(work/'controls/pilot/inside-ideal-reference.json');b=R.read(work/'controls/timing-expected.json');need(b['rows']==[a['rows'][i] for i in ranks],'independent timing reference subset')
 for mode in ['original','lookup']:
  for kind in ['semantics','ontomath']:
   r=R.read(work/'checks'/(kind+'-'+mode+'.receipt.json'));need(r['exit_code']==0 and r['binary_sha256']==meta['files_sha256'][kind+'-'+mode] and r['build_sha256']==R.sha(work/'build.json'),'suite binary/build');need(r['runner_sha256']==R.sha(Path(X.__file__)) and r['log_sha256']==R.sha(work/'checks'/(kind+'-'+mode+'.log')),'suite runner/log identity')
 # A real failed environment attempt is preserved without counting it as an
 # assertion failure or a candidate success.
 failure=R.read(work/'rejected-environment-checks/ontomath-original.receipt.json');need(failure['exit_code']!=0 and failure['binary_sha256']==meta['files_sha256']['ontomath-original'],'retained baseline environment failure');need('Operation not permitted' in (work/'rejected-environment-checks/ontomath-original.log').read_text(),'environment failure cause')
 result=X.summarize(work)
 need(result['reference_rows']==1526 and result['binary64_values']==12208,'declared full observed population')
 # Exact values must be checked even when a mutated result could look plausible.
 expected=R.read(work/'controls/timing-expected.json');neg=[]
 q=copy.deepcopy(expected);q['rows'][0][0]=math.nextafter(q['rows'][0][0],math.inf);refused('one-ULP-output-drift',lambda:X.compare(q,expected),neg)
 q=copy.deepcopy(expected);q['rows'][0][0]=float('nan');refused('nonfinite-output',lambda:X.compare(q,expected),neg)
 q=copy.deepcopy(expected);q['rows']=q['rows'][:-1];refused('missing-result-row',lambda:X.compare(q,expected),neg)
 q=copy.deepcopy(expected);i=next(i for i,row in enumerate(q['rows']) if row!=q['rows'][0]);q['rows'][0],q['rows'][i]=q['rows'][i],q['rows'][0];refused('wrong-input-output-order',lambda:X.compare(q,expected),neg)
 source=X.MATH.read_text();refused('changed-parent-source',lambda:D.derive(source+'\n'),neg)
 # An executed derivation is identified as derived, not impersonated canonical.
 derived=(work/'ScalarForm-lookup.cpp').read_text();refused('derived-labelled-canonical',lambda:need(D.sha(derived)==D.PARENT_SHA,'canonical identity'),neg)
 need(len(neg)==6,'all independent checker refusals')
 result['negative_controls']=neg
 return result

def package(work,archive):
 result=validate(work);R.write(work/'result.json',result);files={}
 for p in work.rglob('*'):
  if not p.is_file() or p.name.endswith('.o') or (p.parent==work and p.name in ['canonical-original','worker-original','worker-lookup','semantics-original','semantics-lookup','ontomath-original','ontomath-lookup']):continue
  files['experiment/'+p.relative_to(work).as_posix()]=p.read_bytes()
 for p in [Path(__file__),Path(X.__file__),Path(D.__file__),X.SEM]:files['tools/'+p.name]=p.read_bytes()
 manifest=dict(schema='6ex-source-bound-reference-experiment-archive-1',files={name:hashlib.sha256(raw).hexdigest() for name,raw in files.items()})
 with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  z.writestr('manifest.json',json.dumps(manifest,sort_keys=True));seen=set()
  for name,raw in files.items():
   sha=manifest['files'][name]
   if sha not in seen:z.writestr('blobs/'+sha,raw);seen.add(sha)
 print('PACKAGED',archive.stat().st_size,R.sha(archive),flush=True)
 return result

def replay(archive):
 with tempfile.TemporaryDirectory(prefix='6ex-replay-') as tmp,zipfile.ZipFile(archive) as z:
  root=Path(tmp);m=json.loads(z.read('manifest.json'));need(set(z.namelist())=={'manifest.json'}|{'blobs/'+s for s in m['files'].values()},'complete archive inventory')
  for name,sha in m['files'].items():
   p=PurePosixPath(name);need(not p.is_absolute() and '..' not in p.parts,'safe archive path');raw=z.read('blobs/'+sha);need(hashlib.sha256(raw).hexdigest()==sha,'archive payload hash');dest=root/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
  for p in [Path(__file__),Path(X.__file__),Path(D.__file__),X.SEM]:need(p.read_bytes()==(root/'tools'/p.name).read_bytes(),'matching executed/replay tool')
  result=validate(root/'experiment');need(result==R.read(root/'experiment/result.json'),'replayed exact result')
  print('PASS retained reference experiment:',result['reference_rows'],'rows /',result['binary64_values'],'exact values;6 checker refusals; no new GPU execution')
  return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['validate','package','replay']);p.add_argument('path',type=Path);p.add_argument('--archive',type=Path);a=p.parse_args()
 if a.command=='package':result=package(a.path.resolve(),a.archive.resolve())
 elif a.command=='replay':result=replay(a.path.resolve())
 else:result=validate(a.path.resolve())
 print(json.dumps(result,indent=2))
if __name__=='__main__':main()
