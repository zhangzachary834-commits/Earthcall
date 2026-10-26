"""Prism M5-0: source-pinned actual Metal pipeline facts and short captures.
No clean economics or resource-counter inference from instrumented runs.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
"""
import argparse,ast,json,hashlib,importlib.util,subprocess,shlex,os,re
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('variant',choices=['disjoint','cert','disjoint-trimmed','cert-trimmed','disjoint-stream-merge','cert-stream-merge']);p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--case',type=Path,required=True);p.add_argument('--capture',action='store_true');a=p.parse_args();root=a.root.resolve();buildRoot=a.build_root.resolve();b=buildRoot/'build';work=a.work.resolve();base=root/'scratch/probes'
if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
work.mkdir(parents=True,exist_ok=False)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for r in [root,buildRoot]:
 if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=r,text=True).strip():raise ValueError('dirty engine')
 if subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=r,text=True).strip()!='3603add4018961529a91bc9592d76f9a553500b1':raise ValueError('engine changed')
spec=importlib.util.spec_from_file_location('sixdd',root/'scratch/probes/ray_transport_gpu_6dd_continuation.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
if a.variant=='disjoint':
 cpp=m.disjoint_fastpath_source(v['CPP_SOURCE'],root,True,scratch_trimmed=False)
elif a.variant=='disjoint-trimmed':
 cpp=m.disjoint_fastpath_source(v['CPP_SOURCE'],root,True,scratch_trimmed=True)
elif a.variant=='disjoint-stream-merge':
 cpp=m.disjoint_fastpath_source(v['CPP_SOURCE'],root,True,scratch_trimmed=True,stream_merge=True)
elif a.variant=='cert':
 cpp=m.ux_tile_cert_disjoint_source(v['CPP_SOURCE'],root,True,scratch_trimmed=False)
elif a.variant=='cert-trimmed':
 cpp=m.ux_tile_cert_disjoint_source(v['CPP_SOURCE'],root,True,scratch_trimmed=True)
elif a.variant=='cert-stream-merge':
 cpp=m.ux_tile_cert_disjoint_source(v['CPP_SOURCE'],root,True,scratch_trimmed=True,stream_merge=True)
else:
 raise ValueError('unknown variant: '+a.variant)
original=cpp
needle='int main(int argc, char** argv) {';assert cpp.count(needle)==1;cpp=cpp.replace(needle,'extern "C" void rtInstallMetalObserver();\nextern "C" void rtMetalSetContext(const char*,const char*,size_t);\n'+needle+'\n    rtInstallMetalObserver();')
# Attach exact WGSL source identity at the same synchronous create call. Labels,
# compiler options, shaders, dispatch geometry and resource bindings are untouched.
hooks=[('WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);','rtMetalSetContext("rayMain",fullCode.data(),fullCode.size());\n        '),('auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);','rtMetalSetContext(entry,text.data(),text.size());\n        '),('auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);','rtMetalSetContext(entry,text.data(),text.size());\n        ')]
for needle,pre in hooks:
 assert cpp.count(needle)==1,(needle,cpp.count(needle));cpp=cpp.replace(needle,pre+needle)
if a.capture:
 cpp=cpp.replace('extern "C" void rtInstallMetalObserver();','extern "C" void rtBeginCapture(const char*);\nextern "C" void rtEndCapture();\nextern "C" void rtInstallMetalObserver();')
 hook='        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}'
 assert cpp.count(hook)==1
 captures=''
 for mode,label in ([(0,'current'),(3,a.variant)] if 'disjoint' in a.variant else [(3,a.variant)]):
  captures+='        rtBeginCapture("'+str(work/(label+'.gputrace'))+'"); execute('+str(mode)+',1,false); rtEndCapture();\n'
 cpp=cpp.replace(hook,captures+hook)
(work/'probe.cpp').write_text(cpp);(work/'unobserved.cpp').write_text(original)
observer=base/'ray_transport_metal_pipeline_observer.mm';subprocess.run(['/usr/bin/clang++','-std=c++20','-O2','-fobjc-arc','-c',str(observer),'-o',str(work/'observer.o')],check=True)
db=json.loads((b/'compile_commands.json').read_text());entry=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3'];subprocess.run(cmd,cwd=b,check=True)
cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');cmd += [str(work/'observer.o'),'-framework','Metal','-framework','Foundation'];subprocess.run(cmd,cwd=b,check=True)
zone=root/'saves/zones/Northern Veil/zone.ecform';zone_before=sha(zone);case_before=sha(a.case);wide=m.load(root,'ray_transport_gpu_wide_stream');meta,rays,_=wide.verify_case(root,a.case.resolve(),zone_before);ref=json.loads(Path(meta['reference']).read_text());assert zone_before==v['ZONE_HASH']
report=work/'result.json';env=dict(os.environ,EC_RAY_PIPELINE_FACTS=str(work/'pipeline-facts.jsonl'),EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001');env.pop('EC_GPU_GEOMETRY_CASE',None)
if a.capture:env['METAL_CAPTURE_ENABLED']='1'
with (work/'native.log').open('x') as f:subprocess.run([str(work/'probe'),str(report)],cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT,check=True)
if sha(zone)!=zone_before or sha(a.case)!=case_before:raise ValueError('source/case changed during run')
wide.verify_case(root,a.case.resolve(),zone_before)
r=json.loads(report.read_text());rows=json.loads(Path(str(report)+'-rows.json').read_text());baseline=json.loads(Path(str(report)+'-baseline-rows.json').read_text());checked=m.validate_6dd(root,r,rows,baseline,rays,ref);checked['instrumented_clocks_not_clean_economics']=True;(work/'independent.json').write_text(json.dumps(checked,indent=2)+'\n')
ident={'variant':a.variant,'observed_cpp_sha256':sha(work/'probe.cpp'),'original_cpp_sha256':sha(work/'unobserved.cpp'),'observer_sha256':sha(observer),'zone_sha256':sha(zone),'engine_src_tree':'3603add4018961529a91bc9592d76f9a553500b1','no_engine_headers_or_objects_substituted':True,'counter_sampling_or_timestamps_requested':False};(work/'source.json').write_text(json.dumps(ident,indent=2)+'\n');print('PASS observed real-harness facts/qualification',a.variant)
