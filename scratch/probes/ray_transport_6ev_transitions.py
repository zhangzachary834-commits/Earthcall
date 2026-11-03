"""Native state-validity/poison matrix for the connected §6eu renderer.
Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
A reused app renderer is compared to a fresh state-specific instance. This is
not an independent mathematical reference, a performance test or Person acceptance.
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
sp=importlib.util.spec_from_file_location('live_review',ROOT/'scratch/probes/ray_transport_live_wiring_review.py')
R=importlib.util.module_from_spec(sp);sp.loader.exec_module(R)
TOOLS=['scratch/probes/ray_transport_6ev_transitions.py','scratch/probes/ray_transport_6ev_transition_main.inc','scratch/probes/ray_transport_6ev_transition_body.inc']

def cpp():
    base=(ROOT/'scratch/probes/volume_demand_probe.cpp').read_text()
    base=base[:base.index('    wgpu::Device gpu;')]
    helper=(ROOT/TOOLS[1]).read_text();body=(ROOT/TOOLS[2]).read_text()
    return '#include <stdexcept>\n#include <array>\n'+base.replace('int main(int argc, char** argv) {',helper+'\nint main(int argc, char** argv) {try{')+body

def build(work,negative):
    R.source=cpp;R.build(work)
    metadata=R.read(work/'build.json');metadata['tool_sha256']={p:R.sha(ROOT/p) for p in TOOLS};metadata['negative_derivation']=False
    if negative:
        original=(ROOT/'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc').read_text()
        derived='#include <cstdlib>\nstatic bool ecOmit(const char* stage){auto* p=std::getenv("EC_6EV_STAGE");return p&&std::string(p)==stage;}\n'+original
        mutations=[('withContext(p.setup);dispatch(common[0]','if(!ecOmit("missing-setup"))withContext(p.setup);dispatch(common[0]'),('for(unsigned i=0;i<3;++i){auto entries=','for(unsigned i=0;i<3;++i){if(ecOmit("missing-producer"))continue;auto entries='),('withContext(p.consumer);wgpuBindGroupRelease(localGroup);','if(!ecOmit("missing-consumer"))withContext(p.consumer);wgpuBindGroupRelease(localGroup);'),('std::string tail=consumer;replace(tail,"EC_PROJECTED",sampling);','std::string tail=consumer;if(ecOmit("missing-publication"))replace(tail,"ecPublication[ecPixel(id)]=vec4<f32>(state.xyz,1.0-state.w);","");replace(tail,"EC_PROJECTED",sampling);')]
        for old,new in mutations:R.need(derived.count(old)==1,'closed native mutation seam');derived=derived.replace(old,new)
        (work/'derived.inc').write_text(derived)
        renderer=(ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text();seam='#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.inc"';R.need(renderer.count(seam)==1,'renderer include seam')
        (work/'derived-renderer.cpp').write_text(renderer.replace(seam,'#include "'+str(work/'derived.inc')+'"'))
        db=R.read(ROOT/'build/compile_commands.json');row=next(r for r in db if r['file'].endswith('/WebGpuRenderer.cpp') and 'earthcall_webgpu.dir' in r['command']);command=shlex.split(row['command']);command[command.index('-c')+1]=str(work/'derived-renderer.cpp');command[command.index('-o')+1]=str(work/'derived-renderer.o');subprocess.run(command,cwd=ROOT/'build',check=True)
        link=metadata['link_argv'];i=next(i for i,v in enumerate(link) if v.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'));link[i]=str(work/'derived-renderer.o');subprocess.run(link,cwd=ROOT/'build',check=True)
        metadata.update(binary_sha256=R.sha(work/'probe'),negative_derivation=dict(parent_renderer_sha256=R.sha(ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp'),parent_inc_sha256=hashlib.sha256(original.encode()).hexdigest(),derived_renderer_sha256=R.sha(work/'derived-renderer.cpp'),derived_inc_sha256=R.sha(work/'derived.inc'),derived_object_sha256=R.sha(work/'derived-renderer.o'),compile_argv=command,mutations=mutations))
    R.need(metadata['after']==R.snapshot(),'source remained fixed');R.write(work/'build.json',metadata)

def run(work,name,mutation):
    b=R.read(work/'build.json');R.need(R.snapshot()==b['after'] and R.sha(work/'probe')==b['binary_sha256'],'native source/binary binding')
    R.need({p:R.sha(ROOT/p) for p in TOOLS}==b['tool_sha256'],'generator binding')
    directory=work/name;directory.mkdir();log=directory/'native.log';argv=[str(work/'probe'),str(R.ZONE),'0123','334','190','7.25',str(directory),'1'];env={k:v for k,v in os.environ.items() if not k.startswith('EC_')};env['EC_VOLUME_SOURCE_MODE']='authored'
    if mutation:env['EC_6EV_MUTATION']=mutation
    start=dt.datetime.now(dt.timezone.utc).isoformat();t=time.monotonic()
    with log.open('w') as f:
        try:code=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=240).returncode
        except subprocess.TimeoutExpired:code='timeout'
    receipt=dict(argv=argv,mutation=mutation,started_utc=start,elapsed_seconds=time.monotonic()-t,exit_code=code,before=b['after'],after=R.snapshot(),binary_sha256=R.sha(work/'probe'),files_sha256={p.name:R.sha(p) for p in directory.iterdir() if p.name!='receipt.json'})
    R.write(directory/'receipt.json',receipt);R.need(receipt['before']==receipt['after'],'stable execution source')
    text=log.read_text();R.need(not any(s in text.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'GPU/compiler failure')
    if mutation:R.need(code==42 and 'TRANSITION FAIL' in text,'intended native rejection '+mutation)
    else:R.need(code==0 and 'TRANSITION PASS states=31' in text,'native transition matrix')
    print('PASS',name,mutation or 'normal',code,flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);q=p.add_subparsers(dest='command',required=True)
    a=q.add_parser('build');a.add_argument('--work',type=Path,required=True);a.add_argument('--negative',action='store_true')
    a=q.add_parser('run');a.add_argument('--work',type=Path,required=True);a.add_argument('--name',required=True);a.add_argument('--mutation',choices=['missing-setup','missing-producer','missing-consumer','missing-publication'],default='')
    a=p.parse_args()
    if a.command=='build':build(a.work.resolve(),a.negative)
    else:run(a.work.resolve(),a.name,a.mutation)

if __name__=='__main__':main()
