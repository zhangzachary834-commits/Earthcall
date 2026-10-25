"""Process the historical sibling prototype into a routing-only correctness witness.
No current-renderer accuracy or performance claim. Exact transport rebase remains separate.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
"""
import re,json,hashlib,subprocess,shlex,sys,argparse
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=Path,required=True);parser.add_argument('--build-root',type=Path,required=True);parser.add_argument('--work',type=Path,required=True);parser.add_argument('--mutation',choices=['fixed','no-reset','no-tail'],default='fixed');a=parser.parse_args();root=a.root.resolve();work=a.work.resolve();mutation=a.mutation
if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
work.mkdir(parents=True,exist_ok=False)
s=(root/'scratch/probes/ray_transport_gpu_6dg_specialization.cpp').read_text()
s=s.replace('std::vector<uint32_t> initList(2304, 0xFFFFFFFFu);','std::vector<uint32_t> initList(2304, 0u);')
def once(a,b):
 global s
 if s.count(a)!=1:raise ValueError(('routing hook',s.count(a),a[:90]))
 s=s.replace(a,b)
# Each routed kernel reads its actual GPU-written family length, not a fixed
# maximum. Diagnostics add a per-query write counter, never a new math path.
for name,queue in [('Alpha',0),('Beta',1),('Fallback',2)]:
 pat=r'(static const char\* kPipeline'+name+r'Wgsl = R"WGSL\()(.*?)(\)WGSL";)'
 hit=re.search(pat,s,re.S);assert hit
 shader=hit[2]
 needle='@group(0) @binding(6) var<storage, read_write> results: array<vec4f>;'
 assert shader.count(needle)==1
 shader=shader.replace(needle,needle+'\n@group(0) @binding(7) var<storage, read> routeCounts: array<u32>;\n@group(0) @binding(8) var<storage,read_write> writes: array<atomic<u32>>;')
 shader=shader.replace('if (tileSlot >= 2304u) { return; }',f'if (tileSlot >= min(routeCounts[{queue}u],2304u)) {{ return; }}')
 assert shader.count('atomicAdd(&writes')==0
 shader=shader.replace('results[rayIdx] = state;','results[rayIdx] = state; atomicAdd(&writes[rayIdx],1u);')
 s=s[:hit.start(2)]+shader+s[hit.end(2):]
# GPU builds three indirect argument triples from the current classifier counts.
args='''\nstatic const char* kRouteArgs = R"WGSL(
@group(0) @binding(0) var<storage,read> counts:array<u32>;
@group(0) @binding(1) var<storage,read_write> args:array<u32>;
@compute @workgroup_size(1) fn routeArgs(@builtin(global_invocation_id) id:vec3u){
 if(id.x>=3u){return;} let n=min(counts[id.x],2304u);
 args[3u*id.x]=(n+3u)/4u;args[3u*id.x+1u]=1u;args[3u*id.x+2u]=1u;
}
)WGSL";\n'''
once('int main(int argc, char** argv) {',args+'\nint main(int argc, char** argv) {')
once('    // Buffers','    auto cpArgs=buildPipeline(kRouteArgs,"routeArgs"); if(!cpArgs)throw std::runtime_error("routeArgs compile");\n    // Buffers')
s=s.replace('WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, count * sizeof(GpuResult)', 'WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, count * sizeof(GpuResult)')
once('    // Bind Groups', '''    auto argsBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_Indirect|WGPUBufferUsage_CopySrc,36,nullptr);
    auto writesBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst,count*4,nullptr);
    auto argLayout=wgpuComputePipelineGetBindGroupLayout(cpArgs,0);WGPUBindGroupEntry argEntries[2]={};
    argEntries[0].binding=0;argEntries[0].buffer=cntBuf;argEntries[0].size=16;
    argEntries[1].binding=1;argEntries[1].buffer=argsBuf;argEntries[1].size=36;
    WGPUBindGroupDescriptor argDesc={};argDesc.layout=argLayout;argDesc.entryCount=2;argDesc.entries=argEntries;
    auto bgArgs=wgpuDeviceCreateBindGroup(gpu.device,&argDesc);
    // Bind Groups''')
for name in ['Alpha','Beta','Fallback']:
 s=s.replace('entries'+name+'[7]','entries'+name+'[9]').replace('bgd'+name+'.entryCount = 7;','bgd'+name+'.entryCount = 9;')
 needle='    bgd'+name+'.entries = entries'+name+';'
 once(needle, f'    entries{name}[7].binding=7;entries{name}[7].buffer=cntBuf;entries{name}[7].size=16;\n    entries{name}[8].binding=8;entries{name}[8].buffer=writesBuf;entries{name}[8].size=count*4;\n'+needle)
# Add argument-generation to BOTH classification sites; same pass, next dispatch.
for passname in ['pass','pass0']:
 needle=f'        wgpuComputePassEncoderDispatchWorkgroups({passname}, 36, 1, 1);'
 assert s.count(needle)==1
 s=s.replace(needle,needle+f'\n        wgpuComputePassEncoderSetPipeline({passname},cpArgs);wgpuComputePassEncoderSetBindGroup({passname},0,bgArgs,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups({passname},3,1,1);')
for x in ['s1','s2','fb']:
 index={'s1':0,'s2':1,'fb':2}[x]
 s=s.replace(f'wgpuComputePassEncoderDispatchWorkgroups(pass, ({x}Tiles + 3u) / 4u, 1, 1);',f'wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,argsBuf,{index*12});')
 s=s.replace(f'wgpuComputePassEncoderDispatchWorkgroups(pass1, ({x}Tiles + 3u) / 4u, 1, 1);',f'wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass1,argsBuf,{index*12});')
for kind in ['Warm','Cold']:
 s=s.replace(f'run{kind}RoutedBatch = [&](uint32_t s1Tiles, uint32_t s2Tiles, uint32_t fbTiles, int rep)',f'run{kind}RoutedBatch = [&](int rep)')
 s=s.replace(f'run{kind}RoutedBatch(counts.s1Count, counts.s2Count, counts.fallbackCount, ',f'run{kind}RoutedBatch(')
# Both warm/cold clear diagnostic writes each iteration. Cold clears append
# counters in command order before EVERY classifier, not via a host count.
needle='''    auto runColdRoutedBatch = [&](int rep) -> double {
        wgpuQueueWriteBuffer(gpu.queue, cntBuf, 0, &initCounters, sizeof(DispatchCounters));'''
once(needle,'''    auto runColdRoutedBatch = [&](int rep) -> double {''')
for passname in ['pass','pass0']:
 # Scope exact lambda so reference repeats do not affect routed instrumentation.
 start=s.index('    auto run'+('Warm' if passname=='pass' else 'Cold')+'RoutedBatch')
 end=s.index('\n    };',start)+7;part=s[start:end]
 needle='        for (int r = 0; r < rep; ++r) {';assert part.count(needle)==1
 clear='\n            wgpuCommandEncoderClearBuffer(enc,writesBuf,0,count*4);'
 if passname=='pass0':clear+='\n            wgpuCommandEncoderClearBuffer(enc,cntBuf,0,16);'
 part=part.replace(needle,needle+clear);s=s[:start]+part+s[end:]
# No benchmark: a correctness-only witness of queue reset, actual length tails,
# repeated classification, poisoned publication, and changed input queues.
begin=s.index('    // Benchmark Matrix (repeats = 1, 2, 4)');end=s.index('    std::ofstream ofs(outJson);',begin)
s=s[:begin]+r'''
    auto readWords=[&](WGPUBuffer source,uint64_t bytes){
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,bytes,nullptr);
        auto e=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(e,source,0,rb,0,bytes);
        auto c=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(gpu.queue,1,&c);wgpuCommandBufferRelease(c);wgpuCommandEncoderRelease(e);
        MapContext m;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&m;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,bytes,cb);while(!m.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!m.ok)throw std::runtime_error("routing control readback");
        auto p=static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(rb,0,bytes));std::vector<uint32_t> v(p,p+bytes/4);
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);return v;
    };
    J controls=J::array();auto originalRays=gpuRays;
    for(int pose:{0,1,0}){
        gpuRays=originalRays;if(pose)for(auto& r:gpuRays)r.ro_clipA.x+=10000.0f;
        wgpuQueueWriteBuffer(gpu.queue,rayBuf,0,gpuRays.data(),gpuRays.size()*sizeof(GpuRay));
        runReferenceBatch(1);auto reference=readBackResults(resRefBuf);
        for(int repeats:{1,2,4,9}){
            std::vector<GpuResult> poison(count);for(auto& p:poison)for(float& f:p.rgb_T)f=std::numeric_limits<float>::quiet_NaN();
            wgpuQueueWriteBuffer(gpu.queue,resRoutedBuf,0,poison.data(),poison.size()*sizeof(GpuResult));
            runColdRoutedBatch(repeats);auto actual=readBackResults(resRoutedBuf);auto writes=readWords(writesBuf,count*4);auto ns=readWords(cntBuf,16);auto args=readWords(argsBuf,36);
            if(ns[0]+ns[1]+ns[2]!=2304u)throw std::runtime_error("counter accumulation");
            if(pose&&(ns[0]!=0||ns[1]!=0||ns[2]!=2304))throw std::runtime_error("changed input not rerouted");
            uint32_t bad=0;double maxDelta=0;
            for(size_t i=0;i<count;++i){if(writes[i]!=1)throw std::runtime_error("ray written other than exactly once");
                for(int k=0;k<4;++k){if(!std::isfinite(actual[i].rgb_T[k]))throw std::runtime_error("poison not overwritten");auto delta=std::abs(actual[i].rgb_T[k]-reference[i].rgb_T[k]);maxDelta=std::max(maxDelta,double(delta));if(delta>1e-4)++bad;}}
            if(bad)throw std::runtime_error("sibling tolerance mismatch");
            for(int k=0;k<3;++k)if(args[k*3]!=(ns[k]+3)/4||args[k*3+1]!=1||args[k*3+2]!=1)throw std::runtime_error("indirect arguments stale");
            controls.push_back({{"pose",pose},{"repeats",repeats},{"counts",ns},{"args",args},{"writes",writes},{"max_delta_sibling",maxDelta},{"all_queries_written_exactly_once",true}});
        }
    }
    J outRecord={{"audit","6di routing correctness repair"},{"scope","Historical sibling semantics retained; no Current accuracy/performance/generic admission"},
       {"controls",controls},{"GPU_generated_indirect_arguments",true},{"host_counts_used_for_dispatch",false},{"new_shader_math",false},{"shipping_gates_open",3}};
''' +s[end:]
s=s.replace('std::cerr << "Wrote master §6dg benchmark record to "','std::cerr << "Wrote routing-only corrected contract to "')
# Keep corruption variants executable: remove one safety rule and require native
# witness failure rather than merely searching for implementation strings.
import sys
if mutation=='no-reset':
 s=s.replace('            wgpuCommandEncoderClearBuffer(enc,cntBuf,0,16);','')
elif mutation=='no-tail':
 s=re.sub(r'min\(routeCounts\[([012])u\],2304u\)',r'min(routeCounts[\1u]+4u,2304u)',s)
# Source pin and actual compilation. The diagnostic has no benchmark table.
(work/'probe.cpp').write_text(s);(work/'source.json').write_text(json.dumps({'parent_sha256':hashlib.sha256((root/'scratch/probes/ray_transport_gpu_6dg_specialization.cpp').read_bytes()).hexdigest(),'cpp_sha256':hashlib.sha256(s.encode()).hexdigest(),'mutation':mutation},indent=2)+'\n')
for engineRoot in [root,a.build_root.resolve()]:
 if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=engineRoot,text=True).strip():raise ValueError('dirty engine source')
 if subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=engineRoot,text=True).strip()!='3603add4018961529a91bc9592d76f9a553500b1':raise ValueError('engine source revision changed')
b=a.build_root.resolve()/'build';db=json.loads((b/'compile_commands.json').read_text());entry=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(work/'probe.cpp');cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3'];subprocess.run(cmd,cwd=b,check=True)
cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');subprocess.run(cmd,cwd=b,check=True)
print('BUILT routing contract',work)
