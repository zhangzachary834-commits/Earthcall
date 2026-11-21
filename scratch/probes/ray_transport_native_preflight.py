"""Read-only native scalar-depth opportunity census; never a speed benchmark.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Uses the corrected authored-source fixture and sparse actual full-resolution rays.
Only scratch renderer/probe copies are instrumented; production stays unchanged.
"""
import argparse, ast, hashlib, json, os, shlex, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--times',default='0,7.25')
p.add_argument('--views',default='inside,spawn,reverse,side')
p.add_argument('--build-only',action='store_true')
a=p.parse_args();r=Path(__file__).resolve().parents[2];b=r/'build';s=a.output.resolve()
assert s!=r and r not in s.parents,'scratch output must be outside repository'
s.mkdir(parents=True,exist_ok=False)
views={'inside':'0,50,75,40,62,110','spawn':'0,2,0,0,60,90','reverse':'0,60,160,0,55,75','side':'90,55,90,-30,55,90'}
times=[float(x) for x in a.times.split(',')];selected=[x for x in a.views.split(',')];assert all(x in views for x in selected)
old=(r/'scripts/northern_veil_fragment_demand.py').read_text()
helper=next(ast.literal_eval(n.value) for n in ast.parse(old).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='helper' for t in n.targets))
helper=helper[:helper.index('static std::string censusShader')]
helper+=r'''
static std::string censusShader(std::string wgsl) {
 if(wgsl.find("fn volumeShared_3_G0(")==std::string::npos) return wgsl;
 auto replace=[&](const std::string& a,const std::string& b){size_t at=0;unsigned n=0;while((at=wgsl.find(a,at))!=std::string::npos){wgsl.replace(at,a.size(),b);at+=b.size();++n;}if(!n){std::fprintf(stderr,"missing census hook: %s\n",a.c_str());std::abort();}};
 wgsl=R"WGSL(
@group(0) @binding(3) var<storage, read_write> census: array<u32>;
var<private> censusSelected: bool;
var<private> censusSlot: u32;
var<private> censusCounts: array<u32,64>;
var<private> censusLast: u32;
var<private> censusSegmentLast: u32;
fn censusTick(i:u32){if(censusSelected){censusCounts[i]+=1u;}}
fn censusFlush(){if(censusSelected){for(var i=0u;i<64u;i+=1u){census[censusSlot*64u+i]=censusCounts[i];}}}
)WGSL"+wgsl;
 replace("fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {",R"WGSL(fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {
 let dc=vec2<u32>(floor(in.position.xy*vec2<f32>(64.0,36.0)/u.viewport.xy));
 let centre=vec2<u32>(floor((vec2<f32>(dc)+0.5)*u.viewport.xy/vec2<f32>(64.0,36.0)));
 censusSelected=all(vec2<u32>(floor(in.position.xy))==centre)&&all(dc<vec2<u32>(64u,36u));
 censusSlot=dc.x+64u*dc.y;
 if(!censusSelected){discard;}
 censusTick(0u);
)WGSL");
 replace("discard;","censusFlush(); discard;");
 replace("var segmentSpacing = 3.0e38;","var segmentSpacing = 3.0e38; var censusBoxMask=0u; censusSegmentLast=0u;");
 replace("if (!segmentOccupied) { continue; }","if (!segmentOccupied) { continue; } censusTick(59u);");
 replace("let worldP = ro + rd * sampleT;","let worldP = ro + rd * sampleT; censusTick(1u); var censusMask=0u; var censusFloor=false;");
 replace("if (walkAllProven) {","if (walkAllProven) { censusTick(2u);");
 replace("        let oldT = transmittance;",R"WGSL(
        censusTick(3u);censusTick(16u+censusMask);
        if(totalExtinction>0.0){
          censusTick(62u);
          if(countOneBits(censusMask)==1u && !censusFloor){
            censusTick(5u);
            if(countOneBits(censusBoxMask)==1u){censusTick(10u);}else{censusTick(12u);}
            if(censusLast!=censusMask){censusTick(8u);censusTick(44u+u32(firstTrailingBit(censusMask)));}
            if(censusSegmentLast!=censusMask){censusTick(63u);}
            censusLast=censusMask;censusSegmentLast=censusMask;
          }else{
            if(countOneBits(censusMask)>1u){censusTick(6u);}else{censusTick(9u);}
            censusLast=0u;censusSegmentLast=0u;
          }
        }else{censusTick(4u);}
        let oldT = transmittance;
)WGSL");
 replace("transmittance < 0.01) { break; }","transmittance < 0.01) { if(censusSelected){censusCounts[11u]=1u;} break; }");
 replace("    return vec4<f32>(integratedRadiance, alpha);","    censusFlush(); return vec4<f32>(integratedRadiance, alpha);");
 for(unsigned i=0;i<4;++i){const auto n=std::to_string(i);const auto bit=std::to_string(1u<<i)+"u";
   replace("segmentSpacing = min(segmentSpacing, mediumChord["+n+"]", "censusBoxMask |= "+bit+"; segmentSpacing = min(segmentSpacing, mediumChord["+n+"]");
   replace("density"+n+" = max(volumeDensityEval_"+n+"(","censusTick("+std::to_string(48u+i)+"u); density"+n+" = max(volumeDensityEval_"+n+"(");
   replace("if (density"+n+" > 0.0) {","if (density"+n+" > 0.0) { censusMask |= "+bit+"; censusTick("+std::to_string(32u+i)+"u);");
   replace("totalExtinction += extinction"+n+";","totalExtinction += extinction"+n+"; censusTick("+std::to_string(52u+i)+"u); if(!(extinction"+n+">1e-6 && extinction"+n+"<1e30)){censusFloor=true;censusTick("+std::to_string(40u+i)+"u);}");
   replace("if (any(incidentLi"+n+" != vec3<f32>(0.0))) {","if (any(incidentLi"+n+" != vec3<f32>(0.0))) { censusTick(56u);");
 }
 return wgsl;
}
'''
renderer=(r/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text();renderer=renderer.replace('namespace {\n',helper+'\nnamespace {\n',1)
start=renderer.index('WebGpuRenderer::volumePipeline(');end=renderer.index('WebGpuRenderer::',start+30);part=renderer[start:end]
part=part.replace('const auto& wgsl = program.wgsl;','const auto wgsl = censusShader(program.wgsl);').replace('globalEntries[3]','globalEntries[4]').replace('globalDesc.entryCount = 3;','''globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_Storage;globalEntries[3].buffer.minBindingSize=censusBytes;globalDesc.entryCount=4;''')
renderer=renderer[:start]+part+renderer[end:];start=renderer.index('void WebGpuRenderer::flushVolumeComposite(');part=renderer[start:]
part=part.replace('WGPUBindGroupEntry globalEntries[3] = {};','WGPUBindGroupEntry globalEntries[4] = {};').replace('globalBgDesc.entryCount = 3;','censusReset(_device,_queue);globalEntries[3].binding=3;globalEntries[3].buffer=censusBuffer;globalEntries[3].size=censusBytes;globalBgDesc.entryCount=4;',1);renderer=renderer[:start]+part
(s/'census-renderer.cpp').write_text(renderer)
db=json.loads((b/'compile_commands.json').read_text())
def compile_one(suffix,source,obj,defines=()):
 c=next(x for x in db if x['file'].endswith(suffix) and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj);args.extend('-D'+x for x in defines);subprocess.run(args,cwd=b,check=True)
def link(output,subs):
 args=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
 for suffix,obj in subs.items():args[next(i for i,x in enumerate(args) if x.endswith(suffix))]=str(obj)
 args[args.index('-o')+1]=str(output);subprocess.run(args,cwd=b,check=True)
compile_one('/WebGpuRenderer.cpp',s/'census-renderer.cpp',s/'census-renderer.o')
compile_one('/entry.cpp',r/'scratch/probes/volume_demand_probe.cpp',s/'probe.o',['EC_FRAGMENT_CENSUS'])
link(s/'census',{'/src/entry.cpp.o':s/'probe.o','/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o':s/'census-renderer.o'})
(s/'provenance.json').write_text(json.dumps({'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip(),'save_sha256':hashlib.sha256((r/'saves/zones/Northern Veil/zone.ecform').read_bytes()).hexdigest(),'source_mode':'authored','width':2560,'height':1440,'rays':'64x36 stratified exact native pixels; instrumentation is diagnostic, not timing or uninstrumented proof','views':views,'times':times},indent=2))
if a.build_only:raise SystemExit()
rows=[]
for view in selected:
 for t in times:
  stem=f'{view}-{t}';env=dict(os.environ,EC_CAM=views[view],EC_VOLUME_SOURCE_MODE='authored',EC_CENSUS=str(s/f'{stem}.bin'))
  run=subprocess.run([str(s/'census'),str(r/'saves/zones/Northern Veil/zone.ecform'),'0123','2560','1440',str(t),str(s/f'{stem}.rgba'),'1'],cwd=r,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,check=True);(s/f'{stem}.log').write_text(run.stdout)
  words=struct.unpack('<'+'I'*(64*36*64),(s/f'{stem}.bin').read_bytes());tot=[sum(words[k::64]) for k in range(64)]
  row={'view':view,'time':t,'counters':tot,'selected_rays':tot[0],'density_calls':sum(tot[48:52]),'positive_medium_calls':sum(tot[32:36]),'current_exps':tot[62],'single_nonfloor_samples':tot[5],'whole_box_single_samples':tot[10],'overlap_samples':tot[6],'single_floor_samples':tot[9],'runs_across_segments':tot[8],'runs_segment_local':tot[63],'optimistic_exps_across_segments':tot[62]-tot[5]+tot[8],'optimistic_exps_segment_local':tot[62]-tot[5]+tot[63],'nonzero_incident_medium_samples':tot[56],'cutoff_rays':tot[11]}
  rows.append(row);(s/'results.json').write_text(json.dumps(rows,indent=2));print(json.dumps({k:v for k,v in row.items() if k!='counters'}),flush=True)
print('DIAGNOSTIC ONLY: mathematical ratio/source/floor eligibility and unchanged native pixels are separate obligations; no speed result.')
