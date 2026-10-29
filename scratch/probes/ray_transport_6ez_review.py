"""CPU-only review of Opus's §6ey continuation; no GPU cost inferred.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise ValueError(message)


def section(text, start, end=None):
    need(text.count(start) == 1, 'unique source start: ' + start)
    part = text[text.index(start):]
    if end is not None:
        need(part.count(end) == 1, 'unique source end: ' + end)
        part = part[:part.index(end)]
    return part


MOCK = r'''
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <json.hpp>
using J=nlohmann::json;
static uint64_t nextId=100, bindingLimit=128ull*1024*1024;
static J groups=J::array(), passes=J::array(), allocations=J::array();
static uint64_t bytesAllocated=0;
template<class T> T handle(){return reinterpret_cast<T>(++nextId);}
template<class T> uint64_t id(T p){return reinterpret_cast<uintptr_t>(p);}
extern "C" {
WGPUStatus wgpuDeviceGetLimits(WGPUDevice,WGPULimits* out){*out={};out->maxStorageBufferBindingSize=bindingLimit;return WGPUStatus_Success;}
void wgpuBufferRelease(WGPUBuffer){}
WGPUBindGroup wgpuDeviceCreateBindGroup(WGPUDevice,const WGPUBindGroupDescriptor* d){
 auto g=handle<WGPUBindGroup>();J entries=J::array();
 for(size_t i=0;i<d->entryCount;++i){auto& e=d->entries[i];entries.push_back({{"binding",e.binding},{"buffer",id(e.buffer)},{"offset",e.offset},{"size",e.size}});}
 groups.push_back({{"id",id(g)},{"layout",id(d->layout)},{"entries",entries}});return g;
}
WGPUBindGroupLayout wgpuComputePipelineGetBindGroupLayout(WGPUComputePipeline p,uint32_t){return reinterpret_cast<WGPUBindGroupLayout>(p);}
void wgpuBindGroupLayoutRelease(WGPUBindGroupLayout){}
void wgpuBindGroupRelease(WGPUBindGroup){}
WGPUComputePassEncoder wgpuCommandEncoderBeginComputePass(WGPUCommandEncoder,const WGPUComputePassDescriptor*){passes.push_back({{"groups",J::array()}});return handle<WGPUComputePassEncoder>();}
void wgpuComputePassEncoderSetPipeline(WGPUComputePassEncoder,WGPUComputePipeline p){passes.back()["pipeline"]=id(p);}
void wgpuComputePassEncoderSetBindGroup(WGPUComputePassEncoder,uint32_t index,WGPUBindGroup g,size_t,const uint32_t*){passes.back()["groups"].push_back({{"index",index},{"id",id(g)}});}
void wgpuComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder,uint32_t x,uint32_t y,uint32_t z){passes.back()["direct"]={x,y,z};}
void wgpuComputePassEncoderDispatchWorkgroupsIndirect(WGPUComputePassEncoder,WGPUBuffer b,uint64_t offset){passes.back()["indirect"]={{"buffer",id(b)},{"offset",offset}};}
void wgpuComputePassEncoderEnd(WGPUComputePassEncoder){}
void wgpuComputePassEncoderRelease(WGPUComputePassEncoder){}
}
namespace kernel_routed_detail {
inline WGPUBuffer buffer(WGPUDevice,WGPUQueue,uint64_t size,WGPUBufferUsage=0,const void* =nullptr){auto b=handle<WGPUBuffer>();bytesAllocated+=size;allocations.push_back({{"buffer",id(b)},{"bytes",size}});return b;}
inline WGPUComputePipeline pipeline(WGPUDevice,const std::string&,const char*,WGPUPipelineLayout=nullptr){throw std::runtime_error("mock must not compile a shader");}
}
namespace routed_wgsl {
const char* kResetCountersWgsl="";const char* kClassifyWgsl="";const char* kRouteArgsWgsl="";
const char* kS1UnfusedProducerWgsl="";const char* kS2ProducerWgsl="";const char* kFallbackProducerWgsl="";
}
KernelRoutedTransport::~KernelRoutedTransport(){}
'''

MAIN = r'''
int main(int argc,char** argv){
 try{
  if(argc!=4)throw std::runtime_error("width height binding-limit");
  uint32_t w=std::stoul(argv[1]),h=std::stoul(argv[2]);bindingLimit=std::stoull(argv[3]);
  KernelRoutedTransport k;for(auto& p:k.common)p=handle<WGPUComputePipeline>();
  bool ok=k.ensure(handle<WGPUDevice>(),handle<WGPUQueue>(),w,h);
  J output={{"eligible",ok},{"width",w},{"height",h},{"binding_limit",bindingLimit},{"rows",k.stripRows},{"allocations",allocations},{"bytes",bytesAllocated}};
  if(ok){
   KernelRoutedTransport::Program p;p.setup=handle<WGPUComputePipeline>();p.consumer=handle<WGPUComputePipeline>();p.globals=handle<WGPUBindGroupLayout>();p.instances=handle<WGPUBindGroupLayout>();p.local=handle<WGPUBindGroupLayout>();
   auto range=[](){return KernelRoutedRange{handle<WGPUBuffer>(),0,112*4};};
   auto globals=range(),params=range(),instances=range(),specs=range(),uniformSpecs=range();J windows=J::array();uint64_t offset=0;auto ubo=handle<WGPUBuffer>();
   bool encoded=k.encode(p,handle<WGPUCommandEncoder>(),globals,params,instances,handle<WGPUTextureView>(),specs,uniformSpecs,[&](const void* raw,size_t size){
    if(size!=32)throw std::runtime_error("window size");auto* v=static_cast<const uint32_t*>(raw);windows.push_back(J::array({v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]}));auto out=KernelRoutedRange{ubo,offset,size};offset+=256;return out;
   });
   output["encoded"]=encoded;output["windows"]=windows;output["groups"]=groups;output["passes"]=passes;
   output["pipelines"]={{"setup",id(p.setup)},{"consumer",id(p.consumer)}};
   output["common"]=J::array();for(auto c:k.common)output["common"].push_back(id(c));
   output["shared"]={{"rays",id(k.rays)},{"singles",id(k.singles)},{"overlaps",id(k.overlaps)},{"counts",id(k.counts)},{"args",id(k.args)}};
  }
  std::cout<<output.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
'''


def check(row):
    if not row['eligible']:
        need(not row['allocations'], 'refused without allocation')
        return dict(eligible=False)
    count = (row['height'] + row['rows'] - 1) // row['rows']
    need(row['encoded'] and len(row['windows']) == count, 'strip coverage')
    need(len(row['passes']) == count * 8, 'eight actual encode passes per strip')
    need(len(row['groups']) == 5 + 4 * count, 'five frame plus four strip groups')
    expected = [row['pipelines']['setup'], *row['common'], row['pipelines']['consumer']]
    # The expected order is independent of the extracted lambda bodies.
    need(len(expected) == 8, 'eight pipeline roles')
    byid = {g['id']: g for g in row['groups']}
    for i, window in enumerate(row['windows']):
        need(window == [row['width'] * row['rows'], 64, 16, 1024,
                        row['width'], row['height'], i * row['rows'], row['rows']],
             'unreduced capacity, interval limits and contiguous window')
        pp = row['passes'][i * 8:(i + 1) * 8]
        need([p['pipeline'] for p in pp] == expected, 'actual stage order')
        need([p['indirect']['offset'] for p in pp if 'indirect' in p] == [0, 12, 24],
             'three separate indirect producer arguments')
        need(sum('direct' in p for p in pp) == 5, 'five direct dispatches')
        setup_local = byid[pp[0]['groups'][2]['id']]['entries']
        need([e['buffer'] for e in setup_local[1:]] ==
             [row['shared'][s] for s in ['rays', 'singles', 'overlaps']],
             'same scratch bound at every strip setup')
        need(pp[-1]['groups'][2]['id'] == pp[0]['groups'][2]['id'],
             'consumer shares setup local bindings')
    return dict(eligible=True, strips=count, passes=len(row['passes']),
                bind_groups=len(row['groups']), rows=row['rows'],
                allocated_bytes=row['bytes'], direct_dispatches=count * 5,
                indirect_dispatches=count * 3)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    root, work = args.root.resolve(), args.work.resolve()
    need(not work.exists(), 'fresh review work directory')
    work.mkdir()
    inc = root / 'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc'
    header = root / 'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp'
    text = inc.read_text()
    helpers = section(text, 'inline WGPUBindGroupEntry entry(', 'inline WGPUComputePipeline pipeline(')
    bodies = section(text, 'void KernelRoutedTransport::releaseBuffers()', 'KernelRoutedTransport::Program* KernelRoutedTransport::program(')
    encode = section(text, 'bool KernelRoutedTransport::encode(')
    cpp = MOCK + '\nnamespace kernel_routed_detail {\n' + helpers + '\n}\n' + bodies + encode + MAIN
    source = work / 'mock.cpp'
    source.write_text(cpp)
    build = Path('/private/tmp/earthcall-6ey-release-build')
    db = json.loads((build / 'compile_commands.json').read_text())
    row = next(r for r in db if r['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in r['command'])
    flags = shlex.split(row['command'])
    cmd = [flags[0], '-std=c++17', '-O3', '-DNDEBUG'] + [f for f in flags if f.startswith('-I')] + [str(source), '-o', str(work / 'mock')]
    with (work / 'build.log').open('w') as log:
        subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, check=True)
    observations, summaries = [], []
    for w, h, limit in [(2560, 1440, 128 << 20), (2560, 1440, 1 << 30),
                        (2560, 1440, 64 << 20), (320, 181, 128 << 20),
                        (7680, 4320, 1 << 30), (0, 1440, 128 << 20)]:
        args = [str(work / 'mock'), str(w), str(h), str(limit)]
        run = subprocess.run(args, capture_output=True, text=True, check=True)
        value = json.loads(run.stdout)
        observations.append(dict(argv=args, value=value))
        summaries.append(dict(width=w, height=h, binding_limit=limit, **check(value)))
    need(summaries[0]['rows'] == summaries[1]['rows'] == 48, 'higher limit alone cannot defeat hard cap')
    need(summaries[2]['rows'] == 24, 'smaller binding limit lowers strip height')
    negatives = []
    import copy
    for label, mutate in [
        ('missing-pass', lambda r: r['passes'].pop()),
        ('wrong-producer-order', lambda r: r['passes'][4].update(pipeline=r['pipelines']['consumer'])),
        ('lost-window', lambda r: r['windows'][1].__setitem__(6, 0)),
        ('diminished-interval-budget', lambda r: r['windows'][0].__setitem__(1, 32)),
        ('rebound-scratch', lambda r: r['shared'].__setitem__('singles', 1)),
    ]:
        r = copy.deepcopy(observations[0]['value'])
        mutate(r)
        try:
            check(r)
        except ValueError as error:
            negatives.append(dict(mutation=label, refusal=str(error)))
        else:
            raise ValueError('failed negative control: ' + label)
    archive = root / 'docs/audits/GPU_RAY_TRANSPORT_6EY_ENGINE_RELEASE_WINDOW_2026-10-29.zip'
    replay = [ 'python3', '-B', str(root / 'scratch/probes/ray_transport_6ey_replay.py'), 'replay', str(archive)]
    with (work / 'replay.log').open('w') as log:
        subprocess.run(replay, cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
    retained = json.loads((root / 'docs/audits/GPU_RAY_TRANSPORT_6EY_ENGINE_RELEASE_WINDOW_2026-10-29.json').read_text())
    timings = retained['timings']['serial_completion']['metrics']
    c, r = timings['Current'], timings['routed']
    host_delta = r['host_tick_ms']['mean'] - c['host_tick_ms']['mean']
    complete_delta = r['mean_complete_sample_ms_per_frame'] - c['mean_complete_sample_ms_per_frame']
    arithmetic = dict(complete_delta_ms=complete_delta, host_tick_delta_ms=host_delta,
                      remainder_delta_ms=complete_delta - host_delta,
                      interpretation='Arithmetic on retained means only; remainder is not GPU busy time.')
    link_file = build / 'CMakeFiles/earthcall_webgpu.dir/link.txt'
    libraries = [s for s in shlex.split(link_file.read_text()) if 'libwgpu' in s or 'dawn' in s.lower()]
    need(len(libraries) == 1 and libraries[0].endswith('/libwgpu_native.a'), 'actual native backend library')
    result = dict(schema='6ez-opus-review-host-encode-only-1',
                  head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
                  signature='Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29',
                  source_sha256={str(p.relative_to(root)): sha(p) for p in [inc, header]},
                  generated_cpp_sha256=sha(source), executable_sha256=sha(work / 'mock'),
                  compiler_argv=cmd, summaries=summaries, negatives=negatives,
                  observations=observations, retained_archive_sha256=sha(archive),
                  retained_means_arithmetic=arithmetic, native_backend_libraries=libraries,
                  native_link_file_sha256=sha(link_file),
                  replay_argv=replay, replay_exit_code=0,
                  scope='CPU interception of verbatim production ensure/releaseBuffers/encode bodies. No GPU, driver, shader execution, contention, time or speedup measurement.')
    (work / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(summaries=summaries, negatives=negatives, work=str(work)), indent=2))


if __name__ == '__main__':
    main()
