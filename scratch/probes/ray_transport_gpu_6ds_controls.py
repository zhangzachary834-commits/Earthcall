"""Instrument the canonical Terminal S1 routed bridge for correctness and ownership.
24 temporal pose cases (original, far shifted, restored, reverse, parallel, restored across repeats 1, 2, 4, 9)
+ 6 refusal limits (budget 0, 23, 24, 25, subpanel capacity 4, overlap capacity 1)
+ negative mutations (no-reset, no-tail).
Verifies 100.0000% exact bit parity against canonical guarded reference (maxDelta = 0.0),
exactly-once producer execution, and Kernel ownership seam flags.
Antigravity / Gemini / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
"""

import argparse, ast, hashlib, importlib.util, json, os, re, subprocess, sys
from pathlib import Path

BRANCH = r"""        J routeChecks=J::array(); auto originalInputs=gpuRays;
        auto bitsEqual=[](const GpuResult& a,const GpuResult& b){for(int k=0;k<4;++k)if(!std::isfinite(a.rgb_T[k])||std::bit_cast<uint32_t>(a.rgb_T[k])!=std::bit_cast<uint32_t>(b.rgb_T[k]))return false;return true;};
        auto poisonOutput=[&](){std::vector<GpuResult> x(executionRays);for(auto& a:x)for(float& v:a.rgb_T)v=NAN;wgpuQueueWriteBuffer(gpu.queue,outBuf,0,x.data(),outBytes);};
        std::vector<GpuResult> currentReview,bulkReview; std::vector<uint8_t> refH,refS,refO;
        auto baselineReview=[&](){execute(0,1,false);currentReview=readResults();execute(1,1,false);bulkReview=readResults();refH=readBackBuffer(rBuf,rayBytes);refS=readBackBuffer(sBuf,subpanelBytes);refO=readBackBuffer(oBuf,overlapBytes);};
        auto checkReview=[&](const std::string& label,int rep,bool normal){
            poisonOutput();execute(3,rep,false);auto output=readResults();auto hb=readBackBuffer(rBuf,rayBytes);auto sb=readBackBuffer(sBuf,subpanelBytes);auto ob=readBackBuffer(oBuf,overlapBytes);
            auto cb=readBackBuffer(cntBuf,sizeof(initCnt));auto ab=readBackBuffer(argsBuf,36);auto wb=readBackBuffer(reviewWritesBuf,count*4);
            DispatchCounters c{};std::memcpy(&c,cb.data(),sizeof(c));std::array<uint32_t,9> args{};std::memcpy(args.data(),ab.data(),36);
            if(c.s1+c.s2+c.fb!=2304u)throw std::runtime_error("routed counter accumulation mismatch");
            const uint32_t ns[3]={c.s1,c.s2,c.fb};for(int k=0;k<3;++k)if(args[3*k]!=(ns[k]+3)/4||args[3*k+1]!=1||args[3*k+2]!=1)throw std::runtime_error("routed indirect args mismatch");
            auto writes=reinterpret_cast<const uint32_t*>(wb.data());auto hs=reinterpret_cast<const GpuRayHeader*>(hb.data());auto rh=reinterpret_cast<const GpuRayHeader*>(refH.data());
            auto subs=reinterpret_cast<const GpuSubpanel*>(sb.data());auto rs=reinterpret_cast<const GpuSubpanel*>(refS.data());auto overs=reinterpret_cast<const GpuOverlap*>(ob.data());auto rr=reinterpret_cast<const GpuOverlap*>(refO.data());
            uint32_t refused=0,packets=0,wrong=0,owners=0,completed_s1=0;J flags=J::array(),rows=J::array();
            for(size_t i=0;i<count;++i){
                owners+=(writes[i]!=1u);
                bool fallback=(hs[i].overlapCount&0x80000000u)!=0;
                bool completed=(hs[i].overlapCount&0x40000000u)!=0;
                refused+=fallback;
                completed_s1+=completed;
                const auto& expected=normal?bulkReview[i]:(fallback?currentReview[i]:bulkReview[i]);
                wrong+=!bitsEqual(output[i],expected);
                bool same=hs[i].subpanelCount==rh[i].subpanelCount&&(hs[i].overlapCount&0x00ffffffu)==rh[i].overlapCount;
                if(normal&&same&&!fallback){
                    for(uint32_t k=0;k<hs[i].subpanelCount;++k){auto a=subs[hs[i].subpanelOffset+k],b=rs[rh[i].subpanelOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.mediumIndex==b.mediumIndex;}
                    for(uint32_t k=0;k<(hs[i].overlapCount&0x00ffffffu);++k){auto a=overs[hs[i].overlapOffset+k],b=rr[rh[i].overlapOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.activeMask==b.activeMask;}
                }
                if(normal&&!same)packets++;flags.push_back(hs[i].overlapCount);
                if(rep==1){J a=J::array(),e=J::array();for(int k=0;k<4;++k){a.push_back(std::bit_cast<uint32_t>(output[i].rgb_T[k]));e.push_back(std::bit_cast<uint32_t>(expected.rgb_T[k]));}rows.push_back({i,a,e});}
            }
            if(owners)throw std::runtime_error("routed exactly-once producer publication failure");
            if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");
            routeChecks.push_back({{"case",label},{"repeats",rep},{"rays",count},{"family_counts",{c.s1,c.s2,c.fb}},{"indirect_args",args},{"producer_entry_errors",owners},{"output_differences",wrong},{"packet_differences",normal?J(packets):J(nullptr)},{"refused",refused},{"completed_s1",completed_s1},{"flags",flags},{"outputs",rows}});
            std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<std::endl;
        };
        for(int pose:{0,1,0,2,3,0}){
            gpuRays=originalInputs;
            for(auto& a:gpuRays){if(pose==1)a.ro[0]+=10000.f;if(pose==2||pose==3){a.ro[0]=0;a.ro[1]=50;a.ro[2]=240;a.rd[2]=-std::abs(a.rd[2]);a.clipA=0;a.clipB=1000;}if(pose==3){a.rd[0]=0;a.rd[1]=0;a.rd[2]=-1;}}
            wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);baselineReview();
            for(int rep:{1,2,4,9})checkReview("pose-"+std::to_string(pose),rep,true);
        }
        for(uint32_t budget:{0u,23u,24u,25u}){auto limited=cfg;limited.pad=budget;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("budget-"+std::to_string(budget),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        for(int kind:{0,1}){auto limited=cfg;if(kind==0)limited.maxSubPerRay=4;else limited.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("capacity-"+std::to_string(kind),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        std::ofstream reviewOut(outputPath);reviewOut<<J({{"scope","§6ds Terminal S1 routed correctness and ownership controls"},{"cases",routeChecks},{"clean_timing",false},{"shipping_qualified",false}}).dump(2)<<std::endl;return 0;
"""

def generate(root, mutation='fixed'):
    spec = importlib.util.spec_from_file_location('terminal_s1', root / 'scratch/probes/ray_transport_gpu_6ds_terminal_s1.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    source = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)

    # Allow mBuf to be bound as uniform buffer as well
    source = source.replace('WGPUBuffer mBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, mediaBytes',
                            'WGPUBuffer mBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, mediaBytes')

    # In S1 producer: bind media as uniform, add reviewWrites at binding 11
    hit_s1 = re.search(r'(static const char\* kS1ProducerWgsl = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_s1
    s1_shader = hit_s1[2]
    old_s1_media = '@group(0) @binding(9) var<storage, read> media: array<MediumParams>;'
    new_s1_media = '@group(0) @binding(9) var<uniform> media: array<MediumParams, 4>;\n@group(0) @binding(11) var<storage, read_write> reviewWrites: array<atomic<u32>>;'
    assert s1_shader.count(old_s1_media) == 1
    s1_shader = s1_shader.replace(old_s1_media, new_s1_media)
    needle_s1_input = '    let input = rays[id];'
    assert s1_shader.count(needle_s1_input) == 1
    s1_shader = s1_shader.replace(needle_s1_input, '    atomicAdd(&reviewWrites[id], 1u);\n' + needle_s1_input)
    source = source[:hit_s1.start(2)] + s1_shader + source[hit_s1.end(2):]

    # In S2 and Fallback producers: add reviewWrites at binding 9
    for name in ['kS2ProducerWgsl', 'kFallbackProducerWgsl']:
        hit = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
        assert hit
        shader = hit[2]
        needle = '@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;'
        assert shader.count(needle) == 1
        shader = shader.replace(needle, needle + '\n@group(0) @binding(9) var<storage, read_write> reviewWrites: array<atomic<u32>>;')
        needle_input = '    let input = rays[id];' if name == 'kS2ProducerWgsl' else ' let input = rays[id];'
        assert shader.count(needle_input) == 1
        shader = shader.replace(needle_input, ' atomicAdd(&reviewWrites[id], 1u);\n' + needle_input)
        source = source[:hit.start(2)] + shader + source[hit.end(2):]

    # Buffer declaration
    needle_args = '    auto argsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 36, nullptr);'
    assert source.count(needle_args) == 1
    source = source.replace(needle_args, needle_args + '\n    auto reviewWritesBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, count * 4, nullptr);')

    # Bind groups
    needle_bg_s1 = 'makeEntry(10, pBuf, 0, paramBytes)'
    assert source.count(needle_bg_s1) == 2
    source = source.replace(needle_bg_s1, needle_bg_s1 + ',\n        makeEntry(11, reviewWritesBuf, 0, count * 4)')

    needle_bg_s2_fb = 'makeEntry(8, cntBuf, 0, sizeof(initCnt))\n    });'
    assert source.count(needle_bg_s2_fb) == 4
    source = source.replace(needle_bg_s2_fb, 'makeEntry(8, cntBuf, 0, sizeof(initCnt)),\n        makeEntry(9, reviewWritesBuf, 0, count * 4)\n    });')

    # Clear reviewWritesBuf in mode 3
    a = source.index('        auto execute=')
    b = source.index('        auto correctness=', a)
    part = source[a:b]
    needle_repeats = '            for(int k=0;k<repeats;++k){'
    assert part.count(needle_repeats) == 1
    part = part.replace(needle_repeats, needle_repeats + '\n                if(mode==3)wgpuCommandEncoderClearBuffer(enc,reviewWritesBuf,0,count*4);')
    source = source[:a] + part + source[b:]
    source = source[:source.index('        auto correctness=', a)] + BRANCH + source[source.index('        auto correctness=', a):]

    s = source
    if mutation == 'no-reset':
        start = s.index('        auto execute=')
        end = s.index('        J routeChecks=', start)
        part = s[start:end]
        needle_reset = 'wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);'
        assert part.count(needle_reset) == 1
        part = part.replace(needle_reset, '// reset omitted for negative control')
        s = s[:start] + part + s[end:]
    if mutation == 'no-tail':
        assert s.count('tileSlot >= min(routeCounts[') == 2  # S2 and fallback
        assert s.count('tileSlot >= min(routeCounts.x') == 1  # S1 uniform
        s = s.replace('tileSlot >= min(routeCounts.x, 2304u)', 'tileSlot >= min(routeCounts.x + 4u, 2304u)')
        for i in range(1, 3):
            s = s.replace(f'tileSlot >= min(routeCounts[{i}u], 2304u)', f'tileSlot >= min(routeCounts[{i}u]+4u, 2304u)')

    return s

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'no-tail'], default='fixed')
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--run', action='store_true')
    a = p.parse_args()
    root = a.root.resolve()
    work = a.work.resolve()
    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    work.mkdir(parents=True)
    cpp = generate(root, a.mutation)
    (work / 'probe.cpp').write_text(cpp)
    subprocess.run([sys.executable, '-B', str(root / 'scratch/probes/ray_transport_probe_build.py'),
                    '--root', str(root), '--build-root', str(a.build_root.resolve()),
                    '--source', str(work / 'probe.cpp'), '--work', str(work / 'build')], check=True)
    if a.run:
        spec = importlib.util.spec_from_file_location('wide', root / 'scratch/probes/ray_transport_gpu_wide_stream.py')
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / 'saves/zones/Northern Veil/zone.ecform'
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        meta, rays, _ = m.verify_case(root, a.case.resolve(), before)
        env = dict(os.environ, EC_POSE_RAYS=meta['rays'], EC_POSE_REFERENCE=meta['reference'],
                   EC_POSE_CLOCK=str(rays['time']), EC_MATCH_REPEAT='1', EC_GPU_BUDGET='1024',
                   EC_GPU_SUB_CAP='64', EC_GPU_OVER_CAP='16', EC_GPU_MARGIN='.001')
        env.pop('EC_GPU_GEOMETRY_CASE', None)
        with (work / 'native.log').open('x') as f:
            result = subprocess.run([str(work / 'build/probe'), str(work / 'result.json')],
                                    cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError('source drift')
        m.verify_case(root, a.case.resolve(), before)
        if result.returncode:
            raise SystemExit(result.returncode)
    print(work)

if __name__ == '__main__':
    main()
