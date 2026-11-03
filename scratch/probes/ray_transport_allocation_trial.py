"""Scratch full-box anisotropic proof allocation; unchanged production WGSL.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Distinct from cropped-support/parent-bank trials: same grid domain, bit packing,
metadata, proof theorem and shader. Only CPU-chosen cell counts are varied.
"""
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--dims',nargs=3,type=int,default=[64,8,64]);p.add_argument('--census',required=True,type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[2];b=r/'build';s=a.output.resolve();assert r not in s.parents and r!=s;s.mkdir(parents=True,exist_ok=False);assert all(0<x<=1023 for x in a.dims);cells=a.dims[0]*a.dims[1]*a.dims[2];assert cells<=32768
src=(r/'src/Singularity/Screen/VolumeZeroProof.cpp').read_text();needle='    proof.totalCells = proof.dims.x * proof.dims.y * proof.dims.z;';assert src.count(needle)==1
insert='''    // SCRATCH: explicit same-domain allocation, fixed logical cell budget.
    if(targetCells >= '''+str(cells)+'''u) {
        proof.dims=glm::uvec3('''+','.join(str(x)+'u' for x in a.dims)+''');
        std::fprintf(stderr,"ALLOCATION dims='''+','.join(map(str,a.dims))+''' cells='''+str(cells)+'''\\n");
    }
''';src='#include <cstdio>\n'+src.replace(needle,insert+needle);(s/'allocation-VolumeZeroProof.cpp').write_text(src)
db=json.loads((b/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/VolumeZeroProof.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(s/'allocation-VolumeZeroProof.cpp');args[args.index('-o')+1]=str(s/'allocation-proof.o');subprocess.run(args,cwd=b,check=True)
def link(out,subs):
 args=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
 subs['/src/Singularity/Screen/VolumeZeroProof.cpp.o']=s/'allocation-proof.o'
 for suffix,obj in subs.items():args[next(i for i,x in enumerate(args) if x.endswith(suffix))]=str(obj)
 args[args.index('-o')+1]=str(out);subprocess.run(args,cwd=b,check=True)
c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(r/'scratch/probes/volume_demand_probe.cpp');args[args.index('-o')+1]=str(s/'probe.o');subprocess.run(args,cwd=b,check=True)
link(s/'allocation',{'/src/entry.cpp.o':s/'probe.o'})
link(s/'allocation-census',{'/src/entry.cpp.o':a.census/'probe.o','/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o':a.census/'census-renderer.o'})
(s/'provenance.json').write_text(json.dumps({'dims':a.dims,'logical_cells':cells,'physical_bitmap_bytes':4*((cells+23)//24),'source_sha256':hashlib.sha256(src.encode()).hexdigest(),'kind':'fixed frozen-scene allocation experiment; not production policy','wgsl':'unchanged production shader'},indent=2))
rows=[]
for view,cam in {'inside':'0,50,75,40,62,110','spawn':'0,2,0,0,60,90','reverse':'0,60,160,0,55,75','side':'90,55,90,-30,55,90'}.items():
 env=dict(os.environ,EC_CAM=cam,EC_VOLUME_SOURCE_MODE='authored',EC_CENSUS=str(s/f'{view}.bin'));p=subprocess.run([str(s/'allocation-census'),'saves/zones/Northern Veil/zone.ecform','0123','2560','1440','7.25',str(s/f'{view}-census.rgba'),'1'],cwd=r,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,check=True);(s/f'{view}.log').write_text(p.stdout);assert 'ALLOCATION dims=' in p.stdout
 words=struct.unpack('<'+'I'*(64*36*64),(s/f'{view}.bin').read_bytes());tot=[sum(words[k::64]) for k in range(64)];row={'view':view,'counters':tot,'density_calls':sum(tot[48:52]),'positive_calls':sum(tot[32:36]),'exps':tot[62],'visited':tot[1]};rows.append(row);(s/'census.json').write_text(json.dumps(rows,indent=2));print({k:v for k,v in row.items() if k!='counters'},flush=True)
print('Allocation census only, not performance. Native uninstrumented parity/cost still required.')
