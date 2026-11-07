"""Actual app-renderer native witness for default-on transport and Current refusal.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Review requested by Zach after Claude preserved the interrupted pass.
Working-tree builds are explicitly bound to a patch and exact source hashes;
never impersonate a clean historical engine tree or a reference integral.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import time
import datetime as dt

RECIPE_WITNESS = r'''
static bool ecRecipeWitness(const std::vector<geom::FieldNode*>& active,double t){
 using N=OntoMath::MathNode;using O=N::Op;unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok)std::printf("RECIPE FAIL %u\n",checks);return ok;};
 auto binary=[](O op,const N& a,const N& b){auto n=std::make_unique<N>();n->op=op;n->children.push_back(std::make_unique<N>(a));n->children.push_back(std::make_unique<N>(b));return n;};
 for(auto* f:active){
  Rendering::VolumeDensityBinding b;if(!Rendering::readVolumeDensity(*f,t,1.0/60.0,b))return false;
  const auto* original=b.densityExpr;auto recipe=OntoMath::clampedWaveSupport(original);if(!check(bool(recipe)))return false;
  auto p=*original;const auto tree=p.pieces[0].mathNode;N zero;
  p.pieces[0].mathNode=std::shared_ptr<N>(binary(O::Add,*tree,zero));if(!check(!OntoMath::clampedWaveSupport(&p)))return false;
  std::vector<const N*> factors;OntoMath::clamped_wave_detail::factors(tree.get(),0,factors);
  auto regroup=std::make_unique<N>(*factors.back());for(int i=int(factors.size())-2;i>=0;--i)regroup=binary(O::Scale,*regroup,*factors[i]);
  p.pieces[0].mathNode=std::shared_ptr<N>(std::move(regroup));auto r=OntoMath::clampedWaveSupport(&p);
  if(!check(r&&r->limits==recipe->limits&&r->amplitudes==recipe->amplitudes&&r->frequencies==recipe->frequencies&&r->phases==recipe->phases))return false;
  const N* x=nullptr;for(auto* n:factors){auto d=OntoMath::clamped_wave_detail::oneMinus(OntoMath::clamped_wave_detail::clamp(n));if(OntoMath::clamped_wave_detail::child(d,O::Div,0,2)&&OntoMath::clamped_wave_detail::symbol(OntoMath::clamped_wave_detail::child(d->children[0].get(),O::Abs,0,1),"x"))x=n;}
  if(!x)return false;p.pieces[0].mathNode=std::shared_ptr<N>(binary(O::Scale,*tree,*x));if(!check(!OntoMath::clampedWaveSupport(&p)))return false;
  p.pieces[0].mathNode=std::make_shared<N>(*tree);
  std::function<bool(N&)> computedDivisor=[&](N& n){if(n.op==O::Div&&n.children.size()==2){n.children[1]=binary(O::Add,*n.children[1],zero);return true;}for(auto& c:n.children)if(c&&computedDivisor(*c))return true;return false;};
  if(!computedDivisor(*p.pieces[0].mathNode)||!check(!OntoMath::clampedWaveSupport(&p)))return false;
  p=*original;p.pieces[0].hasLo=true;if(!check(!OntoMath::clampedWaveSupport(&p)))return false;
  p=*original;p.pieces[0].whereLEZero=std::make_shared<N>();if(!check(!OntoMath::clampedWaveSupport(&p)))return false;
  p=*original;p.pieces[0].mathNode.reset();if(!check(!OntoMath::clampedWaveSupport(&p)))return false;
 }
 if(!check(!OntoMath::clampedWaveSupport(nullptr)))return false;
 std::printf("RECIPE PASS checks=%u media=%zu; mathematical support only, no GPU accuracy claim\n",checks,active.size());return true;
}
'''

ROOT=Path(__file__).resolve().parents[2]
FILES=['src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp','src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp','src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp','src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc','src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp','src/Singularity/OntoMath/ClampedWaveSupport.hpp','src/Singularity/Screen/Camera.hpp','src/Singularity/Screen/Camera.cpp','src/Singularity/Screen/ScreenChannel.hpp','src/Singularity/Screen/ScreenChannel.cpp','src/Singularity/Screen/Renderer.hpp','src/Singularity/Core/EngineRender.cpp']
ZONE=ROOT/'saves/zones/Northern Veil/zone.ecform'
def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def write(p,v):Path(p).write_text(json.dumps(v,indent=2,allow_nan=False)+'\n')
def git(*args):return subprocess.check_output(['git','-c','core.fsmonitor=false',*args],cwd=ROOT,text=True)

def source():
    s=(ROOT/'scratch/probes/volume_demand_probe.cpp').read_text()
    s='#include "Singularity/Screen/Camera.hpp"\n#include "Singularity/OntoMath/ClampedWaveSupport.hpp"\n'+s
    s=s.replace('int main(int argc, char** argv) {',RECIPE_WITNESS+'\nint main(int argc, char** argv) {')
    s=s.replace('    wgpu::Device gpu;', '    if(std::getenv("EC_REVIEW_RECIPE"))return ecRecipeWitness(active,fixedT)?0:1;\n    wgpu::Device gpu;')
    a='    if (!renderer.init(gpu)) { std::puts("FAIL: renderer init"); return 1; }'
    need(s.count(a)==1,'renderer init seam')
    s=s.replace(a,a+'\n    Core::Camera movementWitness;std::printf("MOVEMENT default=%g\\n",movementWitness.speed);\n    REQUIRE(movementWitness.speed==12.0f);\n    std::printf("ROUTE_DEFAULT %d\\n",renderer.volumeRoutedTransportEnabled());\n    if(std::getenv("EC_REVIEW_OFF"))renderer.setVolumeRoutedTransportEnabled(false);')
    a='    renderer.setRadianceSources({source}, 1);';need(s.count(a)==1,'source seam')
    s=s.replace(a,a+'\n    std::printf("ADMISSION rootZero=%d\\n",OntoMath::literalZero(source.radianceExpr));\n    for(auto* f:active){Rendering::VolumeDensityBinding b;Rendering::readVolumeDensity(*f,fixedT,1.0/60.0,b);std::printf("ADMISSION %s recipe=%d\\n",b.producerId.c_str(),bool(OntoMath::clampedWaveSupport(b.densityExpr)));}')
    s=s.replace('    renderer.setLightingEnabled(light.enabled);','    renderer.setLightingEnabled(light.enabled);\n    if(std::getenv("EC_REVIEW_NO_SOURCE"))renderer.setRadianceSources({},2);')
    a='    auto syncImage = [&] {';need(s.count(a)==1,'opaque fixture seam')
    s=s.replace(a,'''    // GLM inverse constructs diagnostic triangle vertices only. Renderer owns its camera inverse.
    const auto opaqueInverse=glm::inverse(glm::perspectiveZO(glm::radians(60.0f),float(width)/height,.1f,1000.f)*glm::lookAt(eye,look,glm::vec3(0,1,0)));
    auto reviewOpaque=[&]{if(!std::getenv("EC_REVIEW_DEPTH"))return;auto point=[&](float x,float y){auto v=opaqueInverse*glm::vec4(x,y,.97f,1);return glm::vec3(v)/v.w;};const auto a=point(-1,-1),b=point(0,-1),c=point(0,1),d=point(-1,1);renderer.drawSolid({a,b,c,a,c,d},glm::vec4(.1f,.2f,.3f,1),Blend::Opaque,true);};
'''+a)
    s=s.replace('        renderer.composeVolumes();','        reviewOpaque();\n        renderer.composeVolumes();')
    a='        renderer.endFrame();';need(s.count(a)==1,'frame seam')
    s=s.replace(a,a+'\n        const auto review=renderer.frameStats();\n        std::printf("REVIEW routedSets=%u pixels=%u bytes=%llu refusals=%u message=%s\\n",review.volumeRoutedSets,review.volumeRoutedPixels,(unsigned long long)review.volumeRoutedWorkingSetBytes,review.volumeProgramRefusals,review.volumeLastProgramRefusal.c_str());\n        if(std::getenv("EC_REVIEW_FORCE"))renderer.setRoutedWorkLimitForDiagnostics(0);')
    a='    setCurrentRenderer(nullptr);\n    return 0;';need(s.count(a)==1,'final diagnostic seam')
    dump=r'''    if(const auto* kernel=renderer.routedTransportKernelForDiagnostics();kernel&&kernel->publication){
        const uint64_t bytes=uint64_t(width)*height*16;
        WGPUBufferDescriptor d={};d.size=bytes;d.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;auto buffer=wgpuDeviceCreateBuffer(gpu.device,&d);
        auto encoder=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(encoder,kernel->publication,0,buffer,0,bytes);
        auto command=wgpuCommandEncoderFinish(encoder,nullptr);wgpuQueueSubmit(gpu.queue,1,&command);wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(encoder);
        MapResult result;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&result;
        wgpuBufferMapAsync(buffer,WGPUMapMode_Read,0,bytes,cb);while(!result.done)wgpuInstanceProcessEvents(gpu.instance);REQUIRE(result.ok);
        const auto* data=static_cast<const float*>(wgpuBufferGetConstMappedRange(buffer,0,bytes));
        std::ofstream(std::string(outPath)+".publication.bin",std::ios::binary).write(reinterpret_cast<const char*>(data),bytes);
        uint32_t accepted=0,refused=0,lit=0,nonfinite=0;
        for(uint64_t i=0;i<uint64_t(width)*height;++i){const auto* v=data+4*i;for(int k=0;k<4;++k)nonfinite+=!std::isfinite(v[k]);if(v[3]<0)++refused;else{++accepted;if(v[3]>1e-5f||std::max({std::abs(v[0]),std::abs(v[1]),std::abs(v[2])})>1e-6f)++lit;}}
        std::printf("PUBLICATION accepted=%u refused=%u nonempty=%u nonfinite=%u strips=%u workLimit=%u\n",accepted,refused,lit,nonfinite,(height+kernel->stripRows-1)/kernel->stripRows,kernel->workLimit);
        wgpuBufferUnmap(buffer);wgpuBufferRelease(buffer);
    }
'''
    return s.replace(a,dump+a)

def snapshot():
    return dict(base_src_tree=git('rev-parse','HEAD:src').strip(),working_patch_sha256=hashlib.sha256(git('diff','--binary','--','src').encode()).hexdigest(),source_files={p:sha(ROOT/p) for p in FILES},zone_sha256=sha(ZONE))

def build(work):
    need(not work.exists(),'fresh external build');work.mkdir(parents=True);before=snapshot();(work/'probe.cpp').write_text(source());(work/'source.patch').write_text(git('diff','--binary','--','src'))
    db=read(ROOT/'build/compile_commands.json');row=next(r for r in db if r['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in r['command']);cmd=shlex.split(row['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3','-I'+str(ROOT/'scratch/probes')]
    subprocess.run(cmd,cwd=ROOT/'build',check=True)
    link=shlex.split((ROOT/'build/CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());link[next(i for i,v in enumerate(link) if v.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');link[link.index('-o')+1]=str(work/'probe');subprocess.run(link,cwd=ROOT/'build',check=True)
    need(before==snapshot(),'source changed during native build')
    write(work/'build.json',dict(before=before,after=snapshot(),compile_argv=cmd,link_argv=link,probe_cpp_sha256=sha(work/'probe.cpp'),binary_sha256=sha(work/'probe'),renderer_object_sha256=sha(ROOT/'build/CMakeFiles/earthcall_webgpu.dir/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o')))

def capture(work,name,mode,width,height,mask,camera,depth=False,no_source=False):
    b=read(work/'build.json');need(snapshot()==b['after'] and sha(work/'probe')==b['binary_sha256'],'native source/binary identity')
    directory=work/name;directory.mkdir();log=directory/'native.log';image=directory/'image.rgba';env={k:v for k,v in os.environ.items() if not k.startswith('EC_')};env.update(EC_CAM=camera,EC_VOLUME_SOURCE_MODE='authored')
    if no_source:env['EC_REVIEW_NO_SOURCE']='1'
    if depth:env['EC_REVIEW_DEPTH']='1'
    if mode=='off':env['EC_REVIEW_OFF']='1'
    if mode=='forced':env['EC_REVIEW_FORCE']='1'
    argv=[str(work/'probe'),str(ZONE),mask,str(width),str(height),'7.25',str(image),'3'];start=dt.datetime.now(dt.timezone.utc).isoformat();t=time.monotonic()
    with log.open('w') as f:
        try:code=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180).returncode
        except subprocess.TimeoutExpired:code='timeout'
    receipt=dict(name=name,mode=mode,depth=depth,no_source=no_source,width=width,height=height,mask=mask,camera=camera,argv=argv,started_utc=start,elapsed_seconds=time.monotonic()-t,exit_code=code,before=b['after'],after=snapshot(),binary_sha256=sha(work/'probe'),log_sha256=sha(log),files_sha256={p.name:sha(p) for p in directory.iterdir() if p.name!='receipt.json'})
    write(directory/'receipt.json',receipt);need(receipt['before']==receipt['after'] and code==0,'native failure '+name)
    text=log.read_text();need(not any(t in text.lower() for t in ['validation error','uncaptured error','device lost','[routed compiler]']),'native GPU/compiler refusal '+name)
    need('MOVEMENT default=12' in text and 'ROUTE_DEFAULT 1' in text,'runtime defaults')
    need(len(image.read_bytes())==width*height*4,'native image coverage');print('PASS',name,flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    q=sub.add_parser('build');q.add_argument('--work',type=Path,required=True)
    q=sub.add_parser('capture');q.add_argument('--work',type=Path,required=True);q.add_argument('--name',required=True);q.add_argument('--mode',choices=['on','off','forced'],required=True);q.add_argument('--width',type=int,default=334);q.add_argument('--height',type=int,default=190);q.add_argument('--mask',default='0123');q.add_argument('--camera',default='0,50,75,40,62,110');q.add_argument('--depth',action='store_true');q.add_argument('--no-source',action='store_true')
    a=p.parse_args()
    if a.command=='build':build(a.work.resolve())
    else:capture(a.work.resolve(),a.name,a.mode,a.width,a.height,a.mask,a.camera,a.depth,a.no_source)

if __name__=='__main__':main()
