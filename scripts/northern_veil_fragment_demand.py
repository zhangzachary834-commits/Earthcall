"""Read-only Northern Veil fragment-demand experiment, NOT a performance benchmark.
Codex / GPT-6.1 Sol / session 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-11 PDT.
Inspired by Zach's minimum necessary mathematics and Astra's demanded-work counsel.
Build the app first. This links its configured objects into scratch binaries,
adds bounded counters to a COPY of the renderer, and compares real GPU pixels.
Original sources/save stay read-only. Requires the current four-curtain fixture.
Instrumentation is known to perturb f32 output; results are explicitly diagnostic.
See docs/audits/NORTHERN_VEIL_FRAGMENT_DEMAND_2026-10-11.md.
"""
from pathlib import Path
import sys,json,hashlib,subprocess,math
import argparse,shlex,os,struct
P=argparse.ArgumentParser(description=__doc__)
P.add_argument('--output',type=Path,required=True,help='New scratch directory outside the repository; never overwritten')
P.add_argument('--width',type=int,default=2560); P.add_argument('--height',type=int,default=1440)
P.add_argument('--camera',help='Exact native eye/look ex,ey,ez,lx,ly,lz; otherwise the four fixture views')
P.add_argument('--times',default='0,7.25'); P.add_argument('--build-only',action='store_true')
a=P.parse_args()
R=Path(__file__).resolve().parents[1]; S=a.output.resolve(); B=R/'build'
if S == R or R in S.parents: P.error('output must be outside the repository')
if a.width<64 or a.height<36: P.error('framebuffer must accommodate the 64 by 36 diagnostic selection')
if a.camera:
 try:
  camera=[float(x) for x in a.camera.split(',')]
  assert len(camera)==6 and all(math.isfinite(x) for x in camera)
 except (ValueError,AssertionError): P.error('camera must contain six finite native coordinates')
try: times=[float(x) for x in a.times.split(',')]; assert times and all(math.isfinite(x) for x in times)
except (ValueError,AssertionError): P.error('times must be finite comma-separated numbers')
S.mkdir(parents=True,exist_ok=False)
DB=json.loads((B/'compile_commands.json').read_text())
def compile_one(suffix,source,output,defines=()):
 c=next(c for c in DB if c['file'].endswith(suffix) and 'earthcall_webgpu.dir' in c['command'])
 args=shlex.split(c['command']); args[args.index('-c')+1]=str(source); args[args.index('-o')+1]=str(output)
 args.extend('-D'+x for x in defines); subprocess.run(args,cwd=B,check=True)
def link(output,substitutions):
 args=shlex.split((B/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
 for suffix,obj in substitutions.items(): args[next(i for i,x in enumerate(args) if x.endswith(suffix))]=str(obj)
 args[args.index('-o')+1]=str(output); subprocess.run(args,cwd=B,check=True)
renderer=(R/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text()
# Kernel-only diagnostic resources, process-local and destroyed by dump after final frame.
helper=r'''
// SCRATCH ONLY: independent per-fragment private counters, bounded sampled writes.
// Never part of production; no atomics, arithmetic substitutions or per-sample global writes.
static WGPUBuffer censusBuffer = nullptr;
static constexpr uint64_t censusBytes = 64u * 36u * 64u * sizeof(uint32_t);
static void censusReset(WGPUDevice device, WGPUQueue queue) {
    if (!censusBuffer) {
        WGPUBufferDescriptor b = {};
        b.label = wgpu::Device::str("Scratch Kernel fragment demand census");
        b.size = censusBytes; b.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
        censusBuffer = wgpuDeviceCreateBuffer(device, &b);
    }
    std::vector<uint32_t> zero(censusBytes / 4u, 0u);
    wgpuQueueWriteBuffer(queue, censusBuffer, 0, zero.data(), censusBytes);
}
extern "C" bool dumpFragmentCensus(WGPUDevice device, WGPUInstance instance, WGPUQueue queue, const char* path) {
    if (!censusBuffer) return false;
    WGPUBufferDescriptor b = {}; b.size = censusBytes;
    b.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    auto readback = wgpuDeviceCreateBuffer(device, &b);
    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(encoder, censusBuffer, 0, readback, 0, censusBytes);
    auto command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command); wgpuCommandEncoderRelease(encoder);
    struct Result { bool done=false, ok=false; } result;
    WGPUBufferMapCallbackInfo cb = {}; cb.mode = WGPUCallbackMode_AllowProcessEvents;
    cb.userdata1 = &result;
    cb.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* p, void*) {
        auto* r = static_cast<Result*>(p); r->ok = status == WGPUMapAsyncStatus_Success; r->done = true;
    };
    wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, censusBytes, cb);
    while (!result.done) wgpuInstanceProcessEvents(instance);
    bool ok = false;
    if (result.ok) {
        auto data = wgpuBufferGetConstMappedRange(readback, 0, censusBytes);
        if (data) { if (FILE* f = std::fopen(path, "wb")) {
            ok = std::fwrite(data, 1, censusBytes, f) == censusBytes;
            std::fclose(f);
        }}
        wgpuBufferUnmap(readback);
    }
    wgpuBufferRelease(readback); wgpuBufferRelease(censusBuffer); censusBuffer = nullptr;
    return ok;
}
static std::string censusShader(std::string wgsl) {
    if (wgsl.find("fn volumeShared_3_G0(") == std::string::npos) return wgsl;
    auto replace = [&](const std::string& a, const std::string& b) {
        size_t at=0; unsigned n=0;
        while ((at=wgsl.find(a,at)) != std::string::npos) { wgsl.replace(at,a.size(),b); at+=b.size(); ++n; }
        if (!n) { std::fprintf(stderr,"CENSUS missing hook: %s\n", a.c_str()); std::abort(); }
    };
    wgsl = R"WGSL(
@group(0) @binding(3) var<storage, read_write> census: array<u32>;
var<private> censusSelected: bool;
var<private> censusSlot: u32;
var<private> censusCounts: array<u32,64>;
fn censusTick(i: u32) { if (censusSelected) { censusCounts[i] += 1u; } }
fn censusFlush() {
    if (censusSelected) {
        for (var i=0u; i<64u; i+=1u) { census[censusSlot*64u+i] = censusCounts[i]; }
    }
}
)WGSL" + wgsl;
    replace("fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {", R"WGSL(fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {
    let cell = vec2<u32>(floor(in.position.xy * vec2<f32>(64.0,36.0) / u.viewport.xy));
    let centre = vec2<u32>(floor((vec2<f32>(cell)+0.5) * u.viewport.xy / vec2<f32>(64.0,36.0)));
    censusSelected = all(vec2<u32>(floor(in.position.xy)) == centre) && all(cell < vec2<u32>(64u,36u));
    censusSlot = cell.x + 64u*cell.y;
    censusTick(0u);
)WGSL");
    replace("discard;", "censusFlush(); discard;");
    replace("if (sceneDepth < 0.999999) {", "if (sceneDepth < 0.999999) { censusTick(6u);");
    replace("let worldP = ro + rd * sampleT;", "let worldP = ro + rd * sampleT; censusTick(1u); var censusPositive = 0u;");
    replace("if (walkAllProven) {", "if (walkAllProven) { censusTick(2u);");
    replace("        let oldT = transmittance;", "        censusTick(3u); if (censusPositive > 1u) { censusTick(4u); }\n        let oldT = transmittance;");
    replace("transmittance < 0.01) { break; }", "transmittance < 0.01) { censusTick(5u); break; }");
    replace("if (alpha <= 1e-5 && integratedMagnitude <= 1e-6) {", "if (alpha <= 1e-5 && integratedMagnitude <= 1e-6) { censusTick(7u);");
    replace("    return vec4<f32>(integratedRadiance, alpha);", "    censusFlush(); return vec4<f32>(integratedRadiance, alpha);");
    for (unsigned medium=0; medium<4u; ++medium) {
        const auto n = std::to_string(medium);
        auto tick=[&](unsigned offset) { return " censusTick("+std::to_string(8u+14u*medium+offset)+"u); "; };
        replace("all(worldP <= mediumInst"+n+".origin.xyz + mediumInst"+n+".halfExtent.xyz)) {", "all(worldP <= mediumInst"+n+".origin.xyz + mediumInst"+n+".halfExtent.xyz)) {"+tick(0));
        replace("sampleT > raySupport"+n+".y) {", "sampleT > raySupport"+n+".y) {"+tick(1));
        replace("} else if (sampleT < raySupport"+n+".x) {", "} else if (sampleT < raySupport"+n+".x) {"+tick(2));
        replace("let cellExit"+n+" =", tick(3)+"let cellExit"+n+" =");
        replace("if (cellExit"+n+" < 0.0) {", "if (cellExit"+n+" < 0.0) {"+tick(5));
        replace("} else { walkExit = min(walkExit, cellExit"+n+"); }", "} else {"+tick(4)+"walkExit = min(walkExit, cellExit"+n+"); }");
        replace("if (density"+n+" > 0.0) {", "if (density"+n+" > 0.0) {"+tick(6)+" censusPositive+=1u;");
        replace("fn volumeShared_"+n+"_G0(p: vec3<f32>) -> f32 {", "fn volumeShared_"+n+"_G0(p: vec3<f32>) -> f32 {"+tick(7));
        // Restrict factor instrumentation to the actual shared original evaluator.
        auto start=wgsl.find("fn volumeShared_"+n+"_G0("); auto end=wgsl.find("\n}",start);
        auto body=wgsl.substr(start,end-start);
        for (unsigned f=0; f<5u; ++f) {
            const auto a="if (densityFactor"+std::to_string(f)+" == 0.0) { return 0.0; }";
            auto p=body.find(a); if (p==std::string::npos) std::abort();
            body.replace(p,a.size(),"if (densityFactor"+std::to_string(f)+" == 0.0) {"+tick(8u+f)+"return 0.0; }");
        }
        auto p=body.find("    let densityFactor5 ="); if(p==std::string::npos) std::abort();
        body.insert(p,tick(13)); wgsl.replace(start,end-start,body);
    }
    return wgsl;
}
'''
renderer=renderer.replace('namespace {\n',helper+'\nnamespace {\n',1)
# Only volume layouts (not unrelated SDF layouts).
start=renderer.index('WebGpuRenderer::volumePipeline('); end=renderer.index('WebGpuRenderer::',start+30)
part=renderer[start:end]
part=part.replace('const auto& wgsl = program.wgsl;', 'const auto wgsl = censusShader(program.wgsl);')
part=part.replace('globalEntries[3]', 'globalEntries[4]').replace('globalDesc.entryCount = 3;', '''globalEntries[3].binding = 3;
    globalEntries[3].visibility = WGPUShaderStage_Fragment;
    globalEntries[3].buffer.type = WGPUBufferBindingType_Storage;
    globalEntries[3].buffer.minBindingSize = censusBytes;
    globalDesc.entryCount = 4;''')
renderer=renderer[:start]+part+renderer[end:]
start=renderer.index('void WebGpuRenderer::flushVolumeComposite(')
part=renderer[start:]
part=part.replace('WGPUBindGroupEntry globalEntries[3] = {};','WGPUBindGroupEntry globalEntries[4] = {};')
part=part.replace('globalBgDesc.entryCount = 3;', '''censusReset(_device, _queue);
        globalEntries[3].binding = 3; globalEntries[3].buffer = censusBuffer;
        globalEntries[3].size = censusBytes;
        globalBgDesc.entryCount = 4;''',1)
renderer=renderer[:start]+part
(S/'WebGpuRenderer-census.cpp').write_text(renderer)
probe=R/'scratch/northern_veil_fragment_demand_probe_2026_10_11.cpp'
save=R/'saves/zones/Northern Veil/zone.ecform'
provenance={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip(),
 'source_sha256':hashlib.sha256((R/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_bytes()).hexdigest(),
 'compiler_sha256':hashlib.sha256((R/'src/Singularity/Screen/WebGPU/SdfWgsl.cpp').read_bytes()).hexdigest(),
 'save_sha256':hashlib.sha256(save.read_bytes()).hexdigest(),'probe_sha256':hashlib.sha256(probe.read_bytes()).hexdigest(),
 'count_slots':64,'selected_grid':[64,36],'instrumented_frames_are_not_performance_measurements':True,
 'signature':'Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-11 PDT'}
(S/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
compile_one('WebGpuRenderer.cpp',S/'WebGpuRenderer-census.cpp',S/'renderer.o')
compile_one('entry.cpp',probe,S/'probe-census.o',('EC_FRAGMENT_CENSUS',))
compile_one('entry.cpp',probe,S/'probe-baseline.o')
link(S/'census',{'/src/entry.cpp.o':S/'probe-census.o','/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o':S/'renderer.o'})
link(S/'baseline',{'/src/entry.cpp.o':S/'probe-baseline.o'})
if a.build_only:sys.exit(0)
views={'inside':'0,50,75,40,62,110','spawn':'0,2,0,0,60,90','reverse':'0,60,160,0,55,75','side':'90,55,90,-30,55,90'}
if a.camera:views={'supplied-native-view':a.camera}
global_names=['invoked','sample_visits','all_proven_jumps','transport_updates','multiple_positive_media','legacy_break_checks_taken','opaque_clips','final_empty_discards']
medium_names=['membership','support_void_or_tail','support_prefix','proof_query','proven_empty','density_demand','positive_density','shared_shape_demand','factor0_zero','factor1_zero','factor2_zero','factor3_zero','factor4_zero','noise_demand']
rows=[]
for t in times:
 for view,cam in views.items():
  for kind in ['baseline','census']:
   stem=f'{view}-{t}-{kind}';env=dict(os.environ,EC_CAM=cam)
   if kind=='census':env['EC_CENSUS']=str(S/(stem+'.bin'))
   else:env.pop('EC_CENSUS',None)
   result=subprocess.run([str(S/kind),str(save),'0123',str(a.width),str(a.height),str(t),str(S/(stem+'.rgba')),'1'],env=env,text=True,capture_output=True,check=True)
   (S/(stem+'.log')).write_text(result.stdout+result.stderr)
  left=(S/f'{view}-{t}-baseline.rgba').read_bytes();right=(S/f'{view}-{t}-census.rgba').read_bytes()
  assert len(left)==len(right)==a.width*a.height*4
  data=struct.unpack('<'+'I'*(64*36*64),(S/f'{view}-{t}-census.bin').read_bytes())
  totals=[sum(data[i::64]) for i in range(64)]
  if not totals[0]:raise RuntimeError('no selected fragments: invalid census')
  row={'view':view,'time':t,'camera':cam,'framebuffer':[a.width,a.height],
   'globals':dict(zip(global_names,totals[:8])),
   'media':[dict(zip(medium_names,totals[8+14*i:22+14*i])) for i in range(4)],
   'pixels_identical':left==right,'changed_bytes':sum(x!=y for x,y in zip(left,right)),
   'max_error':max(abs(x-y) for x,y in zip(left,right)),
   'baseline_sha256':hashlib.sha256(left).hexdigest(),'diagnostic_sha256':hashlib.sha256(right).hexdigest(),
   'scope':'instrumented shader demand only; neither baseline-exact counts nor hardware timing'}
  for medium in row['media']:
   assert medium['proof_query']==medium['proven_empty']+medium['density_demand']
   assert medium['shared_shape_demand']==medium['density_demand']
   assert medium['shared_shape_demand']==sum(medium[f'factor{i}_zero'] for i in range(5))+medium['noise_demand']
   assert medium['positive_density']<=medium['noise_demand']
  assert totals[1]==totals[2]+totals[3]
  rows.append(row);(S/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(row),flush=True)
  if left!=right:print('INSTRUMENTATION PERTURBS PIXELS: do not promote these counts to exact original-fragment evidence.',flush=True)
if hashlib.sha256(save.read_bytes()).hexdigest()!=provenance['save_sha256']:raise RuntimeError('save changed concurrently; invalidate fixture attribution')
