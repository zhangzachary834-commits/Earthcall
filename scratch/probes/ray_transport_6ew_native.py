"""Full-viewport live transport records with explicit Current observer controls.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
Candidate WGSL is unchanged; actual setup rays are copied on the host encoder.
Current fragment adds diagnostic stores; its Screen parity is a required check.
"""
import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('live',ROOT/'scratch/probes/ray_transport_live_wiring_review.py');R=importlib.util.module_from_spec(sp);sp.loader.exec_module(R)
SCENES={'inside':('0,50,75,40,62,110',False),'turned':('40,64,110,-50,60,95',False),'outside':('0,2,0,0,60,90',False),'front':('0,50,75,40,62,110',True)}
HELPER=ROOT/'scratch/probes/ray_transport_6ew_observer.inc'

def once(s,a,b):R.need(s.count(a)==1,'closed observation seam '+a[:80]);return s.replace(a,b)

def main_cpp():
 s=ORIGINAL_SOURCE();s='#include <openssl/sha.h>\n'+s
 s=once(s,'int main(int argc, char** argv) {',HELPER.read_text()+'\nint main(int argc, char** argv) {')
 s=once(s,'    WebGpuRenderer renderer;','    ecObserveInit(gpu,width,height);\n    WebGpuRenderer renderer;')
 a=s.index('    if(const auto* kernel=renderer.routedTransportKernelForDiagnostics();kernel&&kernel->publication){')
 b=s.index('    setCurrentRenderer(nullptr);\n    return 0;',a)
 s=s[:a]+'    ecObservedFinish(renderer,gpu,outPath);\n'+s[b:]
 return s

def renderer_cpp(work):
 path=ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp';s=path.read_text()
 include='#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"'
 extern='\nextern bool ecObserveEnabled();\nextern WGPUBuffer ecObserveFacts();\nextern uint64_t ecObserveFactsBytes();\nextern std::string ecObserveShader(std::string);\nextern void ecObserveCopy(WGPUCommandEncoder,WGPUBuffer,uint32_t,uint32_t,uint32_t,uint32_t);\n'
 s=once(s,include,include+extern)
 inc=(ROOT/'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc').read_text();inc=once(inc,'withContext(p.setup);dispatch(common[0]','withContext(p.setup);ecObserveCopy(encoder,rays,start,stripRows,width,height);dispatch(common[0]');(work/'observed.inc').write_text(inc)
 s=once(s,'#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.inc"','#include "'+str(work/'observed.inc')+'"')
 s=once(s,'    const auto key = wgsl + program.kernelVolumePreparationWgsl;','    if(!routed&&ecObserveEnabled())wgsl=ecObserveShader(std::move(wgsl));\n    const auto key = wgsl + program.kernelVolumePreparationWgsl;')
 a='    if(routed){globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_ReadOnlyStorage;}'
 s=once(s,a,a+'\n    else if(ecObserveEnabled()){globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_Storage;}')
 s=once(s,'    globalDesc.entryCount = routed ? 4 : 3;','    globalDesc.entryCount = (routed||ecObserveEnabled()) ? 4 : 3;')
 a='        if(pipeline->routed){globalEntries[3].binding=3;globalEntries[3].buffer=_kernelRouted->publication;globalEntries[3].size=uint64_t(_depthW)*_depthH*16;}'
 s=once(s,a,a+'\n        else if(ecObserveEnabled()){globalEntries[3].binding=3;globalEntries[3].buffer=ecObserveFacts();globalEntries[3].size=ecObserveFactsBytes();}')
 s=once(s,'        globalBgDesc.entryCount = pipeline->routed ? 4 : 3;','        globalBgDesc.entryCount = (pipeline->routed||ecObserveEnabled()) ? 4 : 3;')
 return s

def build(work):
 R.source=lambda:main_cpp();R.build(work)
 meta=R.read(work/'build.json');renderer=work/'observed-renderer.cpp';renderer.write_text(renderer_cpp(work))
 row=next(v for v in R.read(ROOT/'build/compile_commands.json') if v['file'].endswith('/WebGpuRenderer.cpp') and 'earthcall_webgpu.dir' in v['command']);cmd=shlex.split(row['command']);cmd[cmd.index('-c')+1]=str(renderer);cmd[cmd.index('-o')+1]=str(work/'observed-renderer.o');subprocess.run(cmd,cwd=ROOT/'build',check=True)
 link=list(meta['link_argv']);link[next(i for i,x in enumerate(link) if x.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))]=str(work/'observed-renderer.o');link[link.index('-o')+1]=str(work/'observed');subprocess.run(link,cwd=ROOT/'build',check=True)
 meta.update(observed_binary_sha256=R.sha(work/'observed'),observed_renderer_sha256=R.sha(renderer),observed_inc_sha256=R.sha(work/'observed.inc'),observed_object_sha256=R.sha(work/'observed-renderer.o'),observed_compile_argv=cmd,observed_link_argv=link,tool_sha256={str(p.relative_to(ROOT)):R.sha(p) for p in [Path(__file__),HELPER]})
 R.need(R.snapshot()==meta['after'],'fixed engine source');R.write(work/'build.json',meta)

def capture(work,scene,on,observed,mrt_mode=-1):
 meta=R.read(work/'build.json');R.need(R.snapshot()==meta['after'],'native source identity');name=scene+('-on' if on else '-off')+('-observed' if observed else '-plain')+('' if mrt_mode<0 else '-m'+str(mrt_mode));directory=work/name;directory.mkdir();exe=work/('observed' if observed else 'probe');expected=meta['observed_binary_sha256'] if observed else meta['binary_sha256'];R.need(R.sha(exe)==expected,'native binary binding')
 camera,depth=SCENES[scene];env={k:v for k,v in os.environ.items() if not k.startswith('EC_')};env.update(EC_CAM=camera,EC_VOLUME_SOURCE_MODE='authored')
 if mrt_mode>=0:env['EC_6EW_MRT_MODE']=str(mrt_mode)
 if not on:env['EC_REVIEW_OFF']='1'
 if observed:env['EC_6EW_OBSERVE']='1'
 if depth:env['EC_REVIEW_DEPTH']='1'
 argv=[str(exe),str(R.ZONE),'0123','2560','1440','7.25',str(directory/'image.rgba'),'3'];start=dt.datetime.now(dt.timezone.utc).isoformat();t=time.monotonic();log=directory/'native.log'
 with log.open('w') as f:
  try:code=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180).returncode
  except subprocess.TimeoutExpired:code='timeout'
 receipt=dict(scene=scene,mrt_mode=mrt_mode,camera=camera,depth=depth,on=on,observed=observed,width=2560,height=1440,clock=7.25,argv=argv,started_utc=start,elapsed_seconds=time.monotonic()-t,exit_code=code,binary_sha256=expected,before=meta['after'],after=R.snapshot(),files_sha256={p.name:R.sha(p) for p in directory.iterdir() if p.name!='receipt.json'})
 R.write(directory/'receipt.json',receipt);R.need(code==0 and receipt['before']==receipt['after'],'native process/source '+name);text=log.read_text();R.need(not any(s in text.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'GPU/compiler '+name);print('PASS',name,flush=True)

ORIGINAL_SOURCE=R.source

def main():
 p=argparse.ArgumentParser(description=__doc__);q=p.add_subparsers(dest='command',required=True);a=q.add_parser('build');a.add_argument('--work',type=Path,required=True);a=q.add_parser('capture');a.add_argument('--work',type=Path,required=True);a.add_argument('--scene',choices=SCENES,required=True);a.add_argument('--on',action='store_true');a.add_argument('--observed',action='store_true');a.add_argument('--mrt-mode',type=int,default=-1);a=p.parse_args()
 if a.command=='build':build(a.work.resolve())
 else:capture(a.work.resolve(),a.scene,a.on,a.observed,a.mrt_mode)

if __name__=='__main__':main()
