"""Isolated actual WebGpuRenderer Screen publication witness (not Mode2 math).
Proposal: Sonnet Bridge and Firmament; commissioned by Zach's next-pass request.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Fresh external source/object copies only, no engine/build-tree writes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

from ray_transport_6eo_reference_pilot import ROOT, digest, git, need, read, write

RENDERER = ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp'
PROBE = ROOT/'scratch/probes/volume_demand_probe.cpp'
ZONE = ROOT/'saves/zones/Northern Veil/zone.ecform'
HOOKS = r'''
// Scratch-only Kernel state: hardware buffer handle and diagnostic counters,
// not an authored being/property/cache. Immutable mode for this process; no
// cross-frame validity claim. Init completes before any pipeline is selected.
static WGPUBuffer bridgePublication=nullptr;
static bool bridgeEnabled=false;
static uint64_t bridgeBytes=0;
static uint32_t bridgeShaders=0,bridgeBindings=0;
static std::string bridgeMode;
bool ecBridgeEnabled(){return bridgeEnabled;}
WGPUBuffer ecBridgeBuffer(){++bridgeBindings;return bridgePublication;}
uint64_t ecBridgeBytes(){return bridgeBytes;}
static void ecBridgeInit(WGPUDevice device,WGPUQueue queue,uint32_t w,uint32_t h){
    bridgeMode=std::getenv("EC_SCREEN_BRIDGE")?std::getenv("EC_SCREEN_BRIDGE"):"off";
    const std::vector<std::string> modes{"off","refused","poison-refused","pattern","clear","emission","blend","budget-refuse","mut-order","mut-bypass"};
    if(std::find(modes.begin(),modes.end(),bridgeMode)==modes.end())std::abort();
    const uint64_t bytes=uint64_t(w)*h*16;
    const uint64_t cap=bridgeMode=="budget-refuse"?bytes-16:64ull*1024*1024;
    if(bridgeMode=="off"||bytes>cap)return; // same lane-off shader/layout
    std::vector<glm::vec4> values(size_t(w)*h,glm::vec4(0,0,0,-1));
    for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){
        auto& v=values[size_t(y)*w+x];
        if(bridgeMode=="poison-refused")v=glm::vec4(32768,-32768,8192,-1);
        if(bridgeMode=="pattern"||bridgeMode=="mut-bypass"){const uint32_t i=y*w+x;v=glm::vec4(float(i&255u)/255.f,float((i>>8)&255u)/255.f,float((i>>16)&255u)/255.f,1);}
        if(bridgeMode=="clear")v=glm::vec4(0);
        if(bridgeMode=="emission")v=glm::vec4(1,0,1,0);
        if(bridgeMode=="blend")v=glm::vec4(.125f,.25f,0,.5f);
    }
    WGPUBufferDescriptor d={};d.size=bytes;d.usage=WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst;
    bridgePublication=wgpuDeviceCreateBuffer(device,&d);
    if(!bridgePublication)return; // admission precedes program/layout selection
    wgpuQueueWriteBuffer(queue,bridgePublication,0,values.data(),bytes);
    bridgeBytes=bytes;bridgeEnabled=true;
}
std::string ecBridgeShader(std::string s){
    auto replace=[&](const std::string& a,const std::string& b){
        const auto p=s.find(a);if(p==std::string::npos||s.find(a,p+a.size())!=std::string::npos)std::abort();
        s.replace(p,a.size(),b);
    };
    if(bridgeEnabled){
        const std::string binding="@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;";
        replace(binding,binding+"\n@group(0) @binding(3) var<storage, read> routedPublication: array<vec4<f32>>;");
        const std::string anchor="    var transmittance = 1.0;\n    var integratedRadiance = vec3<f32>(0.0);\n";
        std::string lane=R"WGSL(    {
        let routed = routedPublication[u32(px.y) * u32(u.viewport.x) + u32(px.x)];
        if (routed.w >= 0.0) {
            if (routed.w <= 1e-5 && max(max(abs(routed.x), abs(routed.y)), abs(routed.z)) <= 1e-6) { discard; }
            return routed;
        }
    }
)WGSL";
        if(bridgeMode=="mut-order")lane=R"WGSL(    {
        let routed = routedPublication[u32(px.y) * u32(u.viewport.x) + u32(px.x)];
        if (routed.w <= 1e-5 && max(max(abs(routed.x), abs(routed.y)), abs(routed.z)) <= 1e-6) { discard; }
        if (routed.w >= 0.0) { return routed; }
    }
)WGSL";
        if(bridgeMode=="mut-bypass")lane="    // mutation: ignore all published values\n";
        replace(anchor,lane+anchor);
    }
    if(const char* dir=std::getenv("EC_SCREEN_SHADER_DIR")){
        const auto path=std::string(dir)+"/program-"+std::to_string(bridgeShaders)+".wgsl";
        std::ofstream(path,std::ios::binary)<<s;
    }
    ++bridgeShaders;return s;
}
static void ecBridgeFinish(){
    nlohmann::json j={{"mode",bridgeMode},{"enabled",bridgeEnabled},{"buffer_bytes",bridgeBytes},
        {"shader_calls",bridgeShaders},{"buffer_bindings",bridgeBindings},{"initial_uploads",bridgeEnabled?1:0}};
    std::printf("BRIDGE_JSON %s\n",j.dump().c_str());
    if(bridgePublication)wgpuBufferRelease(bridgePublication);
    bridgePublication=nullptr;
}
'''


def once(s, a, b):
    need(s.count(a)==1,'source anchor not unique: '+a[:90])
    return s.replace(a,b)


def renderer_source(original):
    s=once(original,'#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"',
        '#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"\n'
        '// Scratch diagnostic Kernel buffer hooks; never a production property office.\n'
        'extern bool ecBridgeEnabled();\nextern WGPUBuffer ecBridgeBuffer();\n'
        'extern uint64_t ecBridgeBytes();\nextern std::string ecBridgeShader(std::string);')
    anchor='WebGpuRenderer::volumePipeline(const sdfwgsl::Program& program) {\n    const auto& wgsl = program.wgsl;'
    s=once(s,anchor,'WebGpuRenderer::volumePipeline(const sdfwgsl::Program& program) {\n    const auto wgsl = ecBridgeShader(program.wgsl);')
    s=once(s,'WGPUBindGroupLayoutEntry globalEntries[3] = {};','WGPUBindGroupLayoutEntry globalEntries[4] = {};')
    a='    globalEntries[2].texture.multisampled = false;'
    s=once(s,a,a+'\n    if(ecBridgeEnabled()){globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;\n'
         '        globalEntries[3].buffer.type=WGPUBufferBindingType_ReadOnlyStorage;globalEntries[3].buffer.minBindingSize=16;}')
    s=once(s,'    globalDesc.entryCount = 3;','    globalDesc.entryCount = ecBridgeEnabled()?4:3;')
    s=once(s,'WGPUBindGroupEntry globalEntries[3] = {};','WGPUBindGroupEntry globalEntries[4] = {};')
    a='        globalEntries[2].textureView = _depthView;'
    s=once(s,a,a+'\n        if(ecBridgeEnabled()){globalEntries[3].binding=3;globalEntries[3].buffer=ecBridgeBuffer();globalEntries[3].size=ecBridgeBytes();}')
    s=once(s,'        globalBgDesc.entryCount = 3;','        globalBgDesc.entryCount = ecBridgeEnabled()?4:3;')
    return s


def probe_source(original):
    s=once(original,'namespace {',HOOKS+'\nnamespace {')
    a='    WebGpuRenderer renderer;'
    s=once(s,a,'    ecBridgeInit(gpu.device,gpu.queue,width,height);\n'+a)
    s=once(s,'renderer.beginFrameOffscreen(view, width, height, glm::vec4(0, 0, 0, 1));',
             'renderer.beginFrameOffscreen(view, width, height, glm::vec4(.125f, .25f, .5f, 1));')
    s=once(s,'    setCurrentRenderer(nullptr);\n    return 0;','    setCurrentRenderer(nullptr);\n    ecBridgeFinish();\n    return 0;')
    return s


def build(build_root,work):
    need(not work.exists() and ROOT not in work.parents,'fresh external work required')
    src=git(ROOT,'rev-parse','HEAD:src');need(src==git(build_root,'rev-parse','HEAD:src'),'engine tree mismatch')
    for r in (ROOT,build_root):need(not git(r,'status','--porcelain','--','src'),'dirty engine source')
    work.mkdir();b=build_root/'build';db=read(b/'compile_commands.json')
    p=work/'screen-probe.cpp';p.write_text(probe_source(PROBE.read_text()))
    r=work/'WebGpuRenderer.cpp';r.write_text(renderer_source(RENDERER.read_text()))
    commands=[]
    for needle,source,obj in [('entry.cpp',p,work/'probe.o'),('WebGpuRenderer.cpp',r,work/'renderer.o')]:
        row=next(x for x in db if x['file'].endswith('/'+needle) and 'earthcall_webgpu.dir' in x['command'])
        cmd=shlex.split(row['command']);cmd[cmd.index('-c')+1]=str(source);cmd[cmd.index('-o')+1]=str(obj)
        # Keep the renderer's ORIGINAL optimization/FP/language flags, matching
        # its unmodified object in the control. Only the common probe main gets
        # O3/C++20; changing renderer flags would confound this pixel witness.
        if needle=='entry.cpp':cmd+=['-std=c++20','-O3']
        cmd+=['-I'+str(RENDERER.parent),'-I'+str(ROOT/'scratch/probes')]
        subprocess.run(cmd,cwd=b,check=True);commands.append(cmd)
    original_link=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    links={}
    for name,patched in [('control',False),('bridge',True)]:
        cmd=list(original_link);cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o')
        if patched:cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))]=str(work/'renderer.o')
        cmd[cmd.index('-o')+1]=str(work/name);subprocess.run(cmd,cwd=b,check=True);links[name]=cmd
    metadata=dict(engine_src_tree=src,build_root=str(build_root),original_renderer_sha256=digest(RENDERER),
        original_probe_sha256=digest(PROBE),generated_renderer_sha256=digest(r),generated_probe_sha256=digest(p),
        compile_argv=commands,link_argv=links,binaries={n:digest(work/n) for n in links})
    write(work/'build.json',metadata);print('BUILT',work)


def invoke(work,case,exe,mode,w,h,camera):
    metadata=read(exe.parent/'build.json')
    def fingerprint():
        need(digest(exe)==metadata['binaries'][exe.name],'native binary changed')
        need(digest(RENDERER)==metadata['original_renderer_sha256'] and
             digest(PROBE)==metadata['original_probe_sha256'],'native source changed')
        need(digest(exe.parent/'WebGpuRenderer.cpp')==metadata['generated_renderer_sha256'] and
             digest(exe.parent/'screen-probe.cpp')==metadata['generated_probe_sha256'],'generated source changed')
        tree=git(ROOT,'rev-parse','HEAD:src')
        need(tree==metadata['engine_src_tree']==git(metadata['build_root'],'rev-parse','HEAD:src'),'native engine mismatch')
        need(not git(ROOT,'status','--porcelain','--','src'),'dirty engine source')
        return dict(binary_sha256=digest(exe),build_receipt_sha256=digest(exe.parent/'build.json'),
             engine_src_tree=tree,renderer_sha256=digest(RENDERER),probe_sha256=digest(PROBE),zone_sha256=digest(ZONE))
    before=fingerprint()
    directory=work/case;directory.mkdir()
    shaders=directory/'shaders';shaders.mkdir()
    env={k:v for k,v in os.environ.items() if not k.startswith('EC_')}
    env.update(EC_SCREEN_BRIDGE=mode,EC_SCREEN_SHADER_DIR=str(shaders),EC_CAM=camera,EC_VOLUME_SOURCE_MODE='authored')
    argv=[str(exe),str(ZONE),'0123',str(w),str(h),'7.25',str(directory/'capture.rgba'),'1']
    with (directory/'run.log').open('w') as log:
        code=subprocess.run(argv,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180).returncode
    need(code==0,'native process failed '+case)
    log=(directory/'run.log').read_text()
    need('validation error' not in log.lower() and 'uncaptured error' not in log.lower(),'device validation '+case)
    lines=[x[len('BRIDGE_JSON '):] for x in log.splitlines() if x.startswith('BRIDGE_JSON ')]
    need(len(lines)==1,'bridge diagnostic receipt '+case)
    pixels=(directory/'capture.rgba').read_bytes();need(len(pixels)==w*h*4,'image extent '+case)
    after=fingerprint();need(before==after,'native execution identity changed')
    return dict(case=case,mode=mode,width=w,height=h,camera=camera,returncode=code,
        rgba_sha256=hashlib.sha256(pixels).hexdigest(),diagnostic=json.loads(lines[0]),argv=argv,
        before=before,after=after,
        shaders={p.name:digest(p) for p in shaders.glob('*.wgsl')})


def verify_image(directory,expect,control=None):
    rec=read(directory/'receipt.json');actual=(directory/'capture.rgba').read_bytes()
    w,h=rec['width'],rec['height'];need(len(actual)==w*h*4,'captured image size')
    need(hashlib.sha256(actual).hexdigest()==rec['rgba_sha256'],'capture receipt identity')
    if expect=='current':
        need(control is not None,'independent Current image required')
        expected=control.read_bytes();need(len(expected)==len(actual),'matched viewport required')
        need(any(expected[i:i+4]!=bytes([32,64,128,255]) for i in range(0,len(expected),4)),
             'Current control must show medium contribution')
        message='Screen refusal pixel mismatch'
    elif expect=='pattern':
        expected=bytes(v for y in range(h) for x in range(w) for v in
                       ((y*w+x)&255,((y*w+x)>>8)&255,((y*w+x)>>16)&255,255))
        message='Screen publication pixel mismatch'
    else:
        colors={'clear':(32,64,128,255),'emission':(255,64,255,255),'blend':(48,96,64,255)}
        need(expect in colors,'known synthetic publication expectation')
        expected=bytes(colors[expect])*(w*h);message='Screen publication pixel mismatch'
    changed=sum(a!=b for a,b in zip(actual,expected))
    result=dict(expectation=expect,bytes_compared=len(actual),changed_bytes=changed,
                max_byte_delta=max(abs(a-b) for a,b in zip(actual,expected)))
    write(directory/'verification.json',result)
    need(changed==0,message+': '+str(changed)+' bytes')
    print('PASS',rec['case'],expect,len(actual),'bytes')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    q=sub.add_parser('build');q.add_argument('--build-root',type=Path,required=True);q.add_argument('--work',type=Path,required=True)
    q=sub.add_parser('capture');q.add_argument('--work',type=Path,required=True);q.add_argument('--case',required=True)
    q.add_argument('--binary',type=Path,required=True);q.add_argument('--mode',required=True)
    q.add_argument('--width',type=int,required=True);q.add_argument('--height',type=int,required=True)
    q.add_argument('--camera',default='0,50,75,40,62,110')
    q=sub.add_parser('verify');q.add_argument('--directory',type=Path,required=True)
    q.add_argument('--expect',choices=['current','pattern','clear','emission','blend'],required=True)
    q.add_argument('--control',type=Path)
    a=p.parse_args()
    if a.command=='build':build(a.build_root.resolve(),a.work.resolve())
    elif a.command=='verify':
        try:verify_image(a.directory,a.expect,a.control)
        except ValueError as error:print('REJECT',error);raise SystemExit(2)
    else:
        need(a.case.replace('-','').isalnum() and 64<=a.width<=2560 and 36<=a.height<=1440,'bounded capture')
        result=invoke(a.work.resolve(),a.case,a.binary.resolve(),a.mode,a.width,a.height,a.camera)
        write(a.work/a.case/'receipt.json',result);print(json.dumps(result['diagnostic']))


if __name__=='__main__':main()
