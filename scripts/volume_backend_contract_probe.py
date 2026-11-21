"""Observe retained native Metal shader source/options; no optimization admission.
Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-12 PDT.
Zach's minimum-work direction, Astra's backend contract and unknown fallback.
Build earthcall_webgpu first. Uses its actual objects and a read-only saved fixture.
Observation is process-local; generated files go to a NEW directory outside repo.
No shader edits, option overrides, ordinary timestamps, save writes or FPS claims.
"""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import argparse, hashlib, json, os, shlex, subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
SAVE = ROOT / 'saves/zones/Northern Veil/zone.ecform'
CAMERAS = {'spawn': (0,2,0,0,60,90), 'inside': (0,50,75,40,62,110),
           'reverse': (0,60,160,0,55,75), 'side': (90,55,90,-30,55,90)}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--width', type=int, default=320)
    p.add_argument('--height', type=int, default=180)
    p.add_argument('--build-only', action='store_true')
    a=p.parse_args(); out=a.output.resolve()
    if out == ROOT or ROOT in out.parents: p.error('output must be outside the repository')
    if a.width<64 or a.height<36: p.error('minimum fixture dimensions: 64 by 36')
    out.mkdir(parents=True,exist_ok=False)
    db=json.loads((BUILD/'compile_commands.json').read_text())
    def compile_one(suffix,source,obj):
        c=next(c for c in db if c['file'].endswith(suffix) and 'earthcall_webgpu.dir' in c['command'])
        args=shlex.split(c['command']); args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj)
        subprocess.run(args,cwd=BUILD,check=True)
    fixture=ROOT/'scratch/northern_veil_fragment_demand_probe_2026_10_11.cpp'
    text=fixture.read_text()
    entry='int main(int argc, char** argv) {'
    assert text.count(entry)==1
    text=text.replace(entry,'extern "C" void installVolumeMetalCompileObserver(const char*);\n'+entry+
                      '\n    if (const char* dir=std::getenv("EC_METAL_CONTRACT")) installVolumeMetalCompileObserver(dir);')
    copy=out/'fixture.cpp';copy.write_text(text)
    compile_one('entry.cpp',copy,out/'fixture.o')
    observer=ROOT/'scratch/probes/volume_metal_compile_observer.mm'
    subprocess.run(['/usr/bin/clang++','-std=c++17','-arch','arm64','-c',str(observer),'-o',str(out/'observer.o')],check=True)
    def link(binary,observe):
        args=shlex.split((BUILD/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
        idx=next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))
        args[idx]=str(out/'fixture.o');args[args.index('-o')+1]=str(binary)
        if observe:args.append(str(out/'observer.o'))
        else:
            stub=out/'stub.cpp';stub.write_text('extern "C" void installVolumeMetalCompileObserver(const char*) {}\n')
            subprocess.run(['/usr/bin/clang++','-c',str(stub),'-o',str(out/'stub.o')],check=True)
            args.append(str(out/'stub.o'))
        subprocess.run(args,cwd=BUILD,check=True)
    link(out/'observed',True);link(out/'plain',False)
    sources=[ROOT/'src/Singularity/Screen/WebGPU/SdfWgsl.cpp',ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp',SAVE]
    prior={str(x.relative_to(ROOT)):sha(x) for x in sources}
    provenance={'signature':'Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca',
                'at':datetime.now(ZoneInfo('America/Los_Angeles')).isoformat(),
                'head':subprocess.check_output(['git','-c','core.fsmonitor=false','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                'input_sha256':prior,'backend_tag':(ROOT/'third_party/wgpu/wgpu-native-meta/wgpu-native-git-tag').read_text().strip(),
                'backend_library_sha256':sha(ROOT/'third_party/wgpu/lib/libwgpu_native.a'),
                'binary_sha256':{lane:sha(out/lane) for lane in ('plain','observed')},
                'source_sha256':{str(x.relative_to(ROOT)):sha(x) for x in [Path(__file__).resolve(),observer,fixture]},
                'dimensions':[a.width,a.height],'timestamp_request':False,'production_admitted_skips':0,'performance_run':False}
    (out/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    if a.build_only:return
    images=[];profiles=[]
    for camera,coords in CAMERAS.items():
        for time in (0,7.25):
            label=f'{camera}-{time:g}'; records=out/(label+'-modules');records.mkdir()
            for lane in ('plain','observed'):
                env=dict(os.environ);env.pop('EC_METAL_CONTRACT',None);env.pop('EC_CENSUS',None)
                env['EC_CAM']=','.join(map(str,coords))
                if lane=='observed':env['EC_METAL_CONTRACT']=str(records)
                args=[str(out/lane),str(SAVE),'0123',str(a.width),str(a.height),str(time),str(out/(label+'-'+lane+'.rgba')),'1']
                with (out/(label+'-'+lane+'.log')).open('x') as log:
                    subprocess.run(args,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
            plain=(out/(label+'-plain.rgba')).read_bytes();observed=(out/(label+'-observed.rgba')).read_bytes()
            assert len(plain)==a.width*a.height*4 and plain==observed,'compiler observation changed pixels: '+label
            images.append({'camera':camera,'time':time,'sha256':hashlib.sha256(plain).hexdigest(),'changed_bytes':0})
            draw_profiles=[]
            for meta in sorted(records.glob('*.json')):
                record=json.loads(meta.read_text());msl=meta.with_suffix('.metal');source=msl.read_text()
                record.update(path=str(msl),sha256=sha(msl),volume_module=('volumeDensityEval' in source or 'volumeShared' in source))
                profiles.append(record);draw_profiles.append(record)
            assert draw_profiles and all(x.get('libraryCreated') for x in draw_profiles),'native library compilation failed'
            assert any(x['volume_module'] for x in draw_profiles),'no retained volume compilation observed in '+label
            print('Observed '+label+'; pixels identical',flush=True)
    assert prior=={str(x.relative_to(ROOT)):sha(x) for x in sources},'source/save changed during observation'
    volumes=[x for x in profiles if x['volume_module']]
    unsafe=[x for x in volumes if x.get('mathMode')!=0 or x.get('mathFloatingPointFunctions')!=1]
    report={'scope':'native compilation observation, not numerical conformance or performance',
            'images':images,'modules':profiles,'volume_module_count':len(volumes),
            'outside_closed_profile_count':len(unsafe),'production_admitted_skips':0,
            'admission_reason':'production arithmetic/coordinate enclosure remains required; observation cannot grant admission'}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('images','modules')},indent=2))

if __name__=='__main__':main()
