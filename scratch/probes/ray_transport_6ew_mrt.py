"""Alternative float-attachment Current observation; must pass exact Screen controls.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
from pathlib import Path
import shlex
import subprocess
import ray_transport_6ew_native as N
R=N.R;ROOT=N.ROOT;MRT=ROOT/'scratch/probes/ray_transport_6ew_mrt.inc'

def renderer(work):
 s=(ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text();one=N.once
 include='#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"';s=one(s,include,include+'\nextern bool ecObserveEnabled();\nextern std::string ecMrtShader(std::string);\nextern WGPUTextureView ecObserveMrtView(uint32_t);\nextern WGPUBuffer ecObserveMrtMode();\nextern void ecObserveCopy(WGPUCommandEncoder,WGPUBuffer,uint32_t,uint32_t,uint32_t,uint32_t);\n')
 inc=(ROOT/'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc').read_text();inc=one(inc,'withContext(p.setup);dispatch(common[0]','withContext(p.setup);ecObserveCopy(encoder,rays,start,stripRows,width,height);dispatch(common[0]');(work/'observed.inc').write_text(inc);s=one(s,'#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.inc"','#include "'+str(work/'observed.inc')+'"')
 s=one(s,'    const auto key = wgsl + program.kernelVolumePreparationWgsl;','    if(!routed&&ecObserveEnabled())wgsl=ecMrtShader(std::move(wgsl));\n    const auto key = wgsl + program.kernelVolumePreparationWgsl;')
 a='    frag.entryPoint = wgpu::Device::str("fs");\n    frag.targetCount = 1;\n    frag.targets = &target;'
 b='    std::array<WGPUColorTargetState,2> mrt{};mrt[0]=target;for(unsigned i=1;i<2;++i){mrt[i].format=WGPUTextureFormat_RGBA32Float;mrt[i].writeMask=WGPUColorWriteMask_All;}\n    frag.entryPoint = wgpu::Device::str("fs");\n    frag.targetCount = (!routed&&ecObserveEnabled())?2:1;\n    frag.targets = (!routed&&ecObserveEnabled())?mrt.data():&target;'
 s=one(s,a,b)
 a='    volumePassDesc.colorAttachments = &color;';s=one(s,a,a+'\n    std::array<WGPURenderPassColorAttachment,2> ecColors{};ecColors[0]=color;for(unsigned i=1;i<2;++i){ecColors[i].view=ecObserveMrtView(i-1);ecColors[i].depthSlice=WGPU_DEPTH_SLICE_UNDEFINED;ecColors[i].loadOp=WGPULoadOp_Clear;ecColors[i].storeOp=WGPUStoreOp_Store;}')
 a='        if (!_pass) {\n            _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &volumePassDesc);'
 s=one(s,a,'        if (!_pass) {\n            volumePassDesc.colorAttachmentCount=(!pipeline->routed&&ecObserveEnabled())?2:1;volumePassDesc.colorAttachments=(!pipeline->routed&&ecObserveEnabled())?ecColors.data():&color;\n            _pass = wgpuCommandEncoderBeginRenderPass(_encoder, &volumePassDesc);')
 a='    if(routed){globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_ReadOnlyStorage;}'
 s=one(s,a,a+'\n    else if(ecObserveEnabled()){globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_Uniform;}')
 s=one(s,'    globalDesc.entryCount = routed ? 4 : 3;','    globalDesc.entryCount = (routed||ecObserveEnabled()) ? 4 : 3;')
 a='        if(pipeline->routed){globalEntries[3].binding=3;globalEntries[3].buffer=_kernelRouted->publication;globalEntries[3].size=uint64_t(_depthW)*_depthH*16;}'
 s=one(s,a,a+'\n        else if(ecObserveEnabled()){globalEntries[3].binding=3;globalEntries[3].buffer=ecObserveMrtMode();globalEntries[3].size=16;}')
 s=one(s,'        globalBgDesc.entryCount = pipeline->routed ? 4 : 3;','        globalBgDesc.entryCount = (pipeline->routed||ecObserveEnabled()) ? 4 : 3;')
 return s

def build(work):
 original=N.ORIGINAL_SOURCE
 def main():
  s=N.main_cpp();s=N.once(s,'int main(int argc, char** argv) {',MRT.read_text()+'\nint main(int argc, char** argv) {');s=N.once(s,'    ecObserveInit(gpu,width,height);','    ecObserveInit(gpu,width,height);ecMrtInit(gpu);');return N.once(s,'    ecObservedFinish(renderer,gpu,outPath);','    ecMrtFinish(renderer,gpu,outPath);')
 R.source=main;R.build(work);meta=R.read(work/'build.json');path=work/'observed-renderer.cpp';path.write_text(renderer(work));row=next(v for v in R.read(ROOT/'build/compile_commands.json') if v['file'].endswith('/WebGpuRenderer.cpp') and 'earthcall_webgpu.dir' in v['command']);cmd=shlex.split(row['command']);cmd[cmd.index('-c')+1]=str(path);cmd[cmd.index('-o')+1]=str(work/'observed-renderer.o');subprocess.run(cmd,cwd=ROOT/'build',check=True)
 link=list(meta['link_argv']);link[next(i for i,v in enumerate(link) if v.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))]=str(work/'observed-renderer.o');link[link.index('-o')+1]=str(work/'observed');subprocess.run(link,cwd=ROOT/'build',check=True)
 meta.update(observed_binary_sha256=R.sha(work/'observed'),observed_renderer_sha256=R.sha(path),observed_inc_sha256=R.sha(work/'observed.inc'),observed_object_sha256=R.sha(work/'observed-renderer.o'),observed_compile_argv=cmd,observed_link_argv=link,observer_kind='float-attachments',tool_sha256={str(p.relative_to(ROOT)):R.sha(p) for p in [Path(__file__),MRT,Path(N.__file__),N.HELPER]});R.need(R.snapshot()==meta['after'],'fixed engine');R.write(work/'build.json',meta)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,required=True);a=p.parse_args();build(a.work.resolve())
if __name__=='__main__':main()
