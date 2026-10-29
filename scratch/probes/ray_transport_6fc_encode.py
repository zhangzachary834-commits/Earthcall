"""CPU interception of actual encode(): pass packaging and opt-in tracing.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
"""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

MAIN = r'''
int main(){try{
 KernelRoutedTransport k;bool defaults=k.coalescePasses&&!k.collectDispatchTrace;for(auto& c:k.common)c=handle<WGPUComputePipeline>();
 if(!k.ensure(handle<WGPUDevice>(),handle<WGPUQueue>(),2560,1440))throw std::runtime_error("ensure");
 KernelRoutedTransport::Program p;p.setup=handle<WGPUComputePipeline>();p.consumer=handle<WGPUComputePipeline>();p.globals=handle<WGPUBindGroupLayout>();p.instances=handle<WGPUBindGroupLayout>();p.local=handle<WGPUBindGroupLayout>();setupId=id(p.setup);consumerId=id(p.consumer);
 auto range=[](){return KernelRoutedRange{handle<WGPUBuffer>(),0,448};};auto globals=range(),params=range(),instances=range(),specs=range(),uniformSpecs=range();auto ubo=handle<WGPUBuffer>();auto depth=handle<WGPUTextureView>();J results=J::array();
 for(int mode=0;mode<3;++mode){groups=J::array();passes=J::array();logical=J::array();bound=J::object();uint64_t offset=0;J windows=J::array();k.coalescePasses=mode!=0;k.collectDispatchTrace=mode==2;
  if(!k.encode(p,handle<WGPUCommandEncoder>(),globals,params,instances,depth,specs,uniformSpecs,[&](const void* raw,size_t n){auto v=static_cast<const uint32_t*>(raw);windows.push_back(J::array({v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]}));auto out=KernelRoutedRange{ubo,offset,n};offset+=256;return out;}))throw std::runtime_error("encode");
  results.push_back({{"mode",mode},{"passes",passes.size()},{"dispatches",logical.size()},{"diagnostic_hash",k.dispatchTrace},{"commands",logical},{"windows",windows}});
 }
 if(!defaults||results[0]["passes"]!=240||results[1]["passes"]!=1||results[2]["passes"]!=1)throw std::runtime_error("default/pass count");
 if(results[0]["commands"]!=results[1]["commands"]||results[1]["commands"]!=results[2]["commands"]||results[0]["windows"]!=results[2]["windows"])throw std::runtime_error("logical commands/config changed");
 if(results[0]["diagnostic_hash"]!=0||results[1]["diagnostic_hash"]!=0||results[2]["diagnostic_hash"]==0)throw std::runtime_error("trace opt-in");
 std::cout<<J{{"default_coalesced_trace_off",defaults},{"results",results},{"scope","CPU interception of verbatim production encode. Not GPU execution, pixel validation or cost measurement."}}.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''


def main():
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--root', type=Path, required=True)
    a.add_argument('--work', type=Path, required=True)
    args = a.parse_args()
    root, work = args.root.resolve(), args.work.resolve()
    work.mkdir(exist_ok=True)
    sys.path.insert(0, str(root / 'scratch/probes'))
    import ray_transport_6ez_review as E
    mock = E.MOCK.replace('static uint64_t bytesAllocated=0;', r'''
static uint64_t bytesAllocated=0,currentPipeline=0,setupId=0,consumerId=0;
static J logical=J::array(),bound=J::object();
static void record(J command){
 command["pipeline"]=currentPipeline;J entries=J::array();
 for(unsigned slot=0;slot<((currentPipeline==setupId||currentPipeline==consumerId)?3u:1u);++slot){auto key=std::to_string(slot);if(!bound.contains(key))throw std::runtime_error("missing binding");uint64_t g=bound[key];bool found=false;for(auto& group:groups)if(group["id"]==g){entries.push_back(group["entries"]);found=true;break;}if(!found)throw std::runtime_error("unrecorded group");}
 command["active_buffer_bindings"]=entries;logical.push_back(command);
}
''')
    changes = {
        'wgpuCommandEncoderBeginComputePass': 'WGPUComputePassEncoder wgpuCommandEncoderBeginComputePass(WGPUCommandEncoder,const WGPUComputePassDescriptor*){bound=J::object();passes.push_back(J::object());return handle<WGPUComputePassEncoder>();}',
        'wgpuComputePassEncoderSetPipeline': 'void wgpuComputePassEncoderSetPipeline(WGPUComputePassEncoder,WGPUComputePipeline p){currentPipeline=id(p);}',
        'wgpuComputePassEncoderSetBindGroup': 'void wgpuComputePassEncoderSetBindGroup(WGPUComputePassEncoder,uint32_t index,WGPUBindGroup g,size_t,const uint32_t*){bound[std::to_string(index)]=id(g);}',
        'wgpuComputePassEncoderDispatchWorkgroups': 'void wgpuComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder,uint32_t x,uint32_t y,uint32_t z){record(J{{"direct",{x,y,z}}});}',
        'wgpuComputePassEncoderDispatchWorkgroupsIndirect': 'void wgpuComputePassEncoderDispatchWorkgroupsIndirect(WGPUComputePassEncoder,WGPUBuffer b,uint64_t offset){record(J{{"indirect_buffer",id(b)},{"indirect_offset",offset}});}',
    }
    for name, new in changes.items():
        mock, n = re.subn(r'^[^\n]*\b' + name + r'\([^\n]*\n', new + '\n', mock, flags=re.M)
        if n != 1:
            raise ValueError('unique mock API ' + name)
    text = (root / 'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc').read_text()
    cpp = mock + '\nnamespace kernel_routed_detail {\n' + E.section(text, 'inline WGPUBindGroupEntry entry(', 'inline WGPUComputePipeline pipeline(') + '\n}\n' + E.section(text, 'void KernelRoutedTransport::releaseBuffers()', 'KernelRoutedTransport::Program* KernelRoutedTransport::program(') + E.section(text, 'bool KernelRoutedTransport::encode(') + MAIN
    source = work / 'encode.cpp'
    source.write_text(cpp)
    db = json.loads(Path('/private/tmp/earthcall-6ey-release-build/compile_commands.json').read_text())
    row = next(r for r in db if r['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in r['command'])
    flags = shlex.split(row['command'])
    cmd = [flags[0], '-O3', '-DNDEBUG', '-std=c++17'] + [f for f in flags if f.startswith('-I')] + [str(source), '-o', str(work / 'encode')]
    with (work / 'encode-build.log').open('w') as log:
        subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, check=True)
    with (work / 'encode-result.json').open('w') as out:
        subprocess.run([str(work / 'encode')], stdout=out, check=True)
    j = json.loads((work / 'encode-result.json').read_text())
    print('PASS actual encode CPU interception:', [{k: r[k] for k in ['mode', 'passes', 'dispatches', 'diagnostic_hash']} for r in j['results']])


if __name__ == '__main__':
    main()
