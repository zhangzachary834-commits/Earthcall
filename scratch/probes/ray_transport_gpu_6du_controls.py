"""§6du Strengthened Controls, fallbackCP Atomic Output Witness, sBuf Poisoning & Negative Mutations.

Builds on §6dt with Firmament's 4 forensic repairs:
1. fallbackCP Final-Output Atomic: Removes atomicAdd from producer refusal paths and injects it into fallbackShader (fallbackCP) at actual results write site.
2. sBuf Poisoning: Clears sBuf and oBuf with 0xcdcdcdcd before execute(3); on 0x40000000u (S1 terminal) rays, asserts that sBuf at hs[i].subpanelOffset remains unwritten (proving 0 global sBuf writes directly via memory check!).
3. Negative mutations: Supports 'no-reset', 'no-tail', 'double-publish', 'missing-publish', 'fallback-missing-publish', and 'fallback-double-publish'.
"""

import argparse, ast, hashlib, importlib.util, json, os, re, shlex, subprocess, sys
from pathlib import Path

BRANCH = r"""        J routeChecks=J::array(); auto originalInputs=gpuRays;
        auto bitsEqual=[](const GpuResult& a,const GpuResult& b){for(int k=0;k<4;++k)if(!std::isfinite(a.rgb_T[k])||std::bit_cast<uint32_t>(a.rgb_T[k])!=std::bit_cast<uint32_t>(b.rgb_T[k]))return false;return true;};
        auto poisonOutput=[&](){std::vector<GpuResult> x(executionRays);for(auto& a:x)for(float& v:a.rgb_T)v=NAN;wgpuQueueWriteBuffer(gpu.queue,outBuf,0,x.data(),outBytes);};
        auto poisonSchedule=[&](){
            std::vector<uint32_t> pSub(subpanelBytes / 4, 0xcdcdcdcd);
            wgpuQueueWriteBuffer(gpu.queue, sBuf, 0, pSub.data(), subpanelBytes);
            std::vector<uint32_t> pOver(overlapBytes / 4, 0xcdcdcdcd);
            wgpuQueueWriteBuffer(gpu.queue, oBuf, 0, pOver.data(), overlapBytes);
        };
        std::vector<GpuResult> currentReview,bulkReview; std::vector<uint8_t> refH,refS,refO;
        auto baselineReview=[&](){execute(0,1,false);currentReview=readResults();execute(1,1,false);bulkReview=readResults();refH=readBackBuffer(rBuf,rayBytes);refS=readBackBuffer(sBuf,subpanelBytes);refO=readBackBuffer(oBuf,overlapBytes);};
        auto checkReview=[&](const std::string& label,int rep,bool normal){
            poisonOutput();
            poisonSchedule();
            execute(3,rep,false);
            auto output=readResults();
            auto hb=readBackBuffer(rBuf,rayBytes);
            auto sb=readBackBuffer(sBuf,subpanelBytes);
            auto ob=readBackBuffer(oBuf,overlapBytes);
            auto cb=readBackBuffer(cntBuf,sizeof(initCnt));
            auto ab=readBackBuffer(argsBuf,36);
            auto wb=readBackBuffer(auditWritesBuf,count*8);
            DispatchCounters c{};std::memcpy(&c,cb.data(),sizeof(c));std::array<uint32_t,9> args{};std::memcpy(args.data(),ab.data(),36);
            if(c.s1+c.s2+c.fb!=2304u)throw std::runtime_error("routed counter accumulation mismatch");
            const uint32_t ns[3]={c.s1,c.s2,c.fb};for(int k=0;k<3;++k)if(args[3*k]!=(ns[k]+3)/4||args[3*k+1]!=1||args[3*k+2]!=1)throw std::runtime_error("routed indirect args mismatch");
            auto writes=reinterpret_cast<const uint32_t*>(wb.data());
            auto hs=reinterpret_cast<const GpuRayHeader*>(hb.data());auto rh=reinterpret_cast<const GpuRayHeader*>(refH.data());
            auto subs=reinterpret_cast<const GpuSubpanel*>(sb.data());auto rs=reinterpret_cast<const GpuSubpanel*>(refS.data());auto overs=reinterpret_cast<const GpuOverlap*>(ob.data());auto rr=reinterpret_cast<const GpuOverlap*>(refO.data());
            const auto* sWords=reinterpret_cast<const uint32_t*>(sb.data());
            uint32_t refused=0,packets=0,wrong=0,owners=0,finalOwners=0,completed_s1=0;J flags=J::array(),rows=J::array();
            for(size_t i=0;i<count;++i){
                uint32_t producerEntries = writes[i * 2];
                uint32_t finalPublishes = writes[i * 2 + 1];
                owners += (producerEntries != 1u);
                finalOwners += (finalPublishes != 1u);
                uint32_t flagBits = hs[i].overlapCount & 0xC0000000u;
                if(flagBits == 0xC0000000u) throw std::runtime_error("corrupt mutually exclusive ownership flags: ray=" + std::to_string(i));
                bool fallback=(flagBits & 0x80000000u)!=0;
                bool completed=(flagBits & 0x40000000u)!=0;
                refused+=fallback;
                completed_s1+=completed;
                const auto& expected=normal?bulkReview[i]:(fallback?currentReview[i]:bulkReview[i]);
                bool eq = bitsEqual(output[i],expected);
                if (!eq) {
                    if (wrong < 5) {
                        std::cerr << "MISMATCH [" << label << "] ray=" << i << " out=(" << output[i].rgb_T[0] << "," << output[i].rgb_T[1] << "," << output[i].rgb_T[2] << "," << output[i].rgb_T[3]
                                  << ") exp=(" << expected.rgb_T[0] << "," << expected.rgb_T[1] << "," << expected.rgb_T[2] << "," << expected.rgb_T[3]
                                  << ") fallback=" << fallback << " completed=" << completed << " flagBits=0x" << std::hex << flagBits << std::dec
                                  << " finalWrites=" << finalPublishes << " revWrites=" << producerEntries
                                  << " hexOut=[" << std::hex << std::bit_cast<uint32_t>(output[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[3]) << "]"
                                  << " hexExp=[" << std::bit_cast<uint32_t>(expected.rgb_T[0]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[1]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[2]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[3]) << "]" << std::dec << std::endl;
                    }
                    wrong++;
                }

                if(completed){
                    // Terminal S1 ray: verify subpanelCount matches reference, but global sBuf remained COMPLETELY UNWRITTEN!
                    if(hs[i].subpanelCount != rh[i].subpanelCount) ++packets;
                    uint32_t offsetWords = hs[i].subpanelOffset * (sizeof(GpuSubpanel) / 4);
                    uint32_t countWords = hs[i].subpanelCount * (sizeof(GpuSubpanel) / 4);
                    for(uint32_t w = 0; w < countWords; ++w){
                        if(sWords[offsetWords + w] != 0xcdcdcdcd){
                            throw std::runtime_error("violation: global sBuf written for terminal S1 ray " + std::to_string(i));
                        }
                    }
                } else if(!fallback){
                    bool same=hs[i].subpanelCount==rh[i].subpanelCount&&(hs[i].overlapCount&0x00ffffffu)==rh[i].overlapCount;
                    if(same){
                        for(uint32_t k=0;k<hs[i].subpanelCount;++k){auto a=subs[hs[i].subpanelOffset+k],b=rs[rh[i].subpanelOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.mediumIndex==b.mediumIndex;}
                        for(uint32_t k=0;k<(hs[i].overlapCount&0x00ffffffu);++k){auto a=overs[hs[i].overlapOffset+k],b=rr[rh[i].overlapOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.activeMask==b.activeMask;}
                    }
                    if(!same)packets++;
                }
                flags.push_back(hs[i].overlapCount);
                if(rep==1){J a=J::array(),e=J::array();for(int k=0;k<4;++k){a.push_back(std::bit_cast<uint32_t>(output[i].rgb_T[k]));e.push_back(std::bit_cast<uint32_t>(expected.rgb_T[k]));}rows.push_back({i,a,e});}
            }
            if(owners)throw std::runtime_error("routed exactly-once producer publication failure: " + std::to_string(owners) + " errors");
            if(finalOwners)throw std::runtime_error("routed exactly-once final output publication failure: " + std::to_string(finalOwners) + " errors");
            if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");
            routeChecks.push_back({{"case",label},{"repeats",rep},{"rays",count},{"family_counts",{c.s1,c.s2,c.fb}},{"indirect_args",args},{"producer_entry_errors",owners},{"final_output_errors",finalOwners},{"output_differences",wrong},{"packet_differences",normal?J(packets):J(nullptr)},{"refused",refused},{"completed_s1",completed_s1},{"flags",flags},{"outputs",rows}});
            std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified)\n";
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
        std::ofstream reviewOut(outputPath);reviewOut<<J({{"scope","§6du True Terminal S1 routed correctness and ownership controls"},{"cases",routeChecks},{"clean_timing",false},{"shipping_qualified",false}}).dump(2)<<std::endl;return 0;
"""

def generate(root, mutation='fixed'):
    spec = importlib.util.spec_from_file_location('terminal_s1', root / 'scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    source = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)

    # Flush stderr on WebGPU error
    source = source.replace('std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data);',
                            'std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data); std::fflush(stderr);')
    needle_out_buf = '    WGPUBuffer readBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, outBytes, nullptr);'
    assert source.count(needle_out_buf) == 1
    new_buffers = (needle_out_buf + '\n' +
                   '    size_t auditBytes = executionRays * 2 * sizeof(uint32_t);\n' +
                   '    WGPUBuffer auditWritesBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, auditBytes, nullptr);')
    source = source.replace(needle_out_buf, new_buffers)

    # 1. In S1 producer: add auditWrites at binding 3 (replacing old subpanels binding!)
    hit_s1 = re.search(r'(static const char\* kS1ProducerWgsl = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_s1
    s1_shader = hit_s1[2]
    old_s1_rays = '@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;'
    new_s1_rays = (old_s1_rays + '\n' +
                   '@group(0) @binding(3) var<storage, read_write> auditWrites: array<atomic<u32>>;')
    assert s1_shader.count(old_s1_rays) == 1
    s1_shader = s1_shader.replace(old_s1_rays, new_s1_rays)
    needle_s1_input = '    let input = rays[id];'
    assert s1_shader.count(needle_s1_input) == 1
    s1_shader = s1_shader.replace(needle_s1_input, '    atomicAdd(&auditWrites[id * 2u], 1u);\n' + needle_s1_input)

    # In S1 producer: instrument results write with atomicAdd
    needle_s1_results = '    results[id] = vec4<f32>(L, T);'
    assert s1_shader.count(needle_s1_results) == 1
    if mutation == 'missing-publish':
        s1_shader = s1_shader.replace(needle_s1_results, '    if (id > 1000000u) { results[id] = vec4<f32>(L, T); }')
    else:
        s1_shader = s1_shader.replace(needle_s1_results, needle_s1_results + '\n    atomicAdd(&auditWrites[id * 2u + 1u], 1u);')

    source = source[:hit_s1.start(2)] + s1_shader + source[hit_s1.end(2):]

    # 2. In S2 and Fallback producers: add auditWrites at binding 9 (producer entry only; NO refusal output atomic!)
    for name in ['kS2ProducerWgsl', 'kFallbackProducerWgsl']:
        hit = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
        assert hit
        shader = hit[2]
        needle = '@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;'
        assert shader.count(needle) == 1
        shader = shader.replace(needle, needle + '\n@group(0) @binding(9) var<storage, read_write> auditWrites: array<atomic<u32>>;')
        needle_input = '    let input = rays[id];' if name == 'kS2ProducerWgsl' else ' let input = rays[id];'
        assert shader.count(needle_input) == 1
        shader = shader.replace(needle_input, ' atomicAdd(&auditWrites[id * 2u], 1u);\n' + needle_input)
        source = source[:hit.start(2)] + shader + source[hit.end(2):]

    # 3. Add binding 7 to kCommonWgslHeader (so cpUnfused and cpFused both declare auditWrites)
    old_common_results = '@group(0) @binding(6) var<storage, read_write> results: array<vec4<f32>>;'
    new_common_results = (old_common_results + '\n' +
                          '@group(0) @binding(7) var<storage, read_write> auditWrites: array<atomic<u32>>;')
    assert source.count(old_common_results) == 1
    source = source.replace(old_common_results, new_common_results)

    # Decode counts.w low 24 bits for overlap count in consumers
    assert source.count('let overCount = r.counts.w;') == 2
    source = source.replace('let overCount = r.counts.w;', 'let overCount = r.counts.w & 0x00ffffffu;')

    # In cpUnfused (kUnfusedShader): instrument results write with atomicAdd
    hit_unfused = re.search(r'(static const char\* kUnfusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_unfused
    unfused_shader = hit_unfused[2]
    needle_unfused_ray = '    results[rayIdx] = vec4<f32>(L, T);'
    assert unfused_shader.count(needle_unfused_ray) == 1
    unfused_shader = unfused_shader.replace(needle_unfused_ray, needle_unfused_ray + '\n    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);')

    if mutation == 'double-publish':
        needle_completed_check = '    if((r.counts.w & 0x40000000u) != 0u){ return; }'
        assert unfused_shader.count(needle_completed_check) == 1
        unfused_shader = unfused_shader.replace(needle_completed_check, '    // S1 completed guard omitted for double-publish mutation')

    source = source[:hit_unfused.start(2)] + unfused_shader + source[hit_unfused.end(2):]

    # Also instrument results write in kFusedShader
    hit_fused = re.search(r'(static const char\* kFusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_fused
    fused_shader = hit_fused[2]
    assert fused_shader.count(needle_unfused_ray) == 1
    fused_shader = fused_shader.replace(needle_unfused_ray, needle_unfused_ray + '\n    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);')
    source = source[:hit_fused.start(2)] + fused_shader + source[hit_fused.end(2):]

    # Update makeBindGroup for cpUnfused and cpFused: add entry 7 for auditWritesBuf
    old_make_bg = '''        WGPUBindGroupEntry entries[7] = {};
        entries[0].binding = 0; entries[0].buffer = uBuf; entries[0].size = sizeof(udata);
        entries[1].binding = 1; entries[1].buffer = mBuf; entries[1].size = mediaBytes;
        entries[2].binding = 2; entries[2].buffer = pBuf; entries[2].size = paramBytes;
        entries[3].binding = 3; entries[3].buffer = rBuf; entries[3].size = rayBytes;
        entries[4].binding = 4; entries[4].buffer = sBuf; entries[4].size = subpanelBytes;
        entries[5].binding = 5; entries[5].buffer = oBuf; entries[5].size = overlapBytes > 0 ? overlapBytes : 16;
        entries[6].binding = 6; entries[6].buffer = outBuf; entries[6].size = outBytes;

        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = bgl;
        bgd.entryCount = 7;
        bgd.entries = entries;'''

    new_make_bg = '''        WGPUBindGroupEntry entries[8] = {};
        entries[0].binding = 0; entries[0].buffer = uBuf; entries[0].size = sizeof(udata);
        entries[1].binding = 1; entries[1].buffer = mBuf; entries[1].size = mediaBytes;
        entries[2].binding = 2; entries[2].buffer = pBuf; entries[2].size = paramBytes;
        entries[3].binding = 3; entries[3].buffer = rBuf; entries[3].size = rayBytes;
        entries[4].binding = 4; entries[4].buffer = sBuf; entries[4].size = subpanelBytes;
        entries[5].binding = 5; entries[5].buffer = oBuf; entries[5].size = overlapBytes > 0 ? overlapBytes : 16;
        entries[6].binding = 6; entries[6].buffer = outBuf; entries[6].size = outBytes;
        entries[7].binding = 7; entries[7].buffer = auditWritesBuf; entries[7].size = auditBytes;

        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = bgl;
        bgd.entryCount = 8;
        bgd.entries = entries;'''
    assert source.count(old_make_bg) == 1
    source = source.replace(old_make_bg, new_make_bg)

    # In tile streaming loop: update consumer bind group
    old_tile_consumer = 'entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'
    new_tile_consumer = 'entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult)), entry(7,auditWritesBuf,outputOffset/2,uint64_t(n)*2*sizeof(uint32_t))});'
    assert source.count(old_tile_consumer) == 1
    source = source.replace(old_tile_consumer, new_tile_consumer)

    # 4. Update extra and bindCurrent so currentShader and fallbackShader share identical 6-buffer layout and code
    old_extra = (
        '    const std::string extra=R"WGSL(\n'
        'struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };\n'
        '@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;\n'
        '@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;\n'
        ')WGSL";'
    )
    new_extra = (
        '    const std::string extra=R"WGSL(\n'
        'struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };\n'
        '@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;\n'
        '@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;\n'
        '@group(0) @binding(5) var<storage,read_write> auditWrites:array<atomic<u32>>;\n'
        ')WGSL";'
    )
    assert source.count(old_extra) == 1
    source = source.replace(old_extra, new_extra)

    # Instrument main write site and discard site in currentShader
    needle_curr_out = ' capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);'
    assert source.count(needle_curr_out) == 1
    source = source.replace(needle_curr_out, needle_curr_out + '\n atomicAdd(&auditWrites[idx * 2u + 1u], 1u);')

    needle_curr_discard = 'capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0);return;'
    assert source.count(needle_curr_discard) == 1
    source = source.replace(needle_curr_discard, 'capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0); atomicAdd(&auditWrites[idx * 2u + 1u], 1u); return;')

    # Update bindCurrent to bind auditWritesBuf at entry 5, keeping bindCurrent5 for prepCP
    old_bind_current = '''    auto bindCurrent=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[5]={};
        WGPUBuffer buffers[5]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf};
        uint64_t sizes[5]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes};
        for(uint32_t k=0;k<5;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=5;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto currentBG=bindCurrent(currentCP),prepBG=bindCurrent(prepCP);'''
    new_bind_current = '''    auto bindCurrent5=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[5]={};
        WGPUBuffer buffers[5]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf};
        uint64_t sizes[5]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes};
        for(uint32_t k=0;k<5;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=5;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto bindCurrent=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[6]={};
        WGPUBuffer buffers[6]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf,auditWritesBuf};
        uint64_t sizes[6]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes,auditBytes};
        for(uint32_t k=0;k<6;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=6;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto prepBG=bindCurrent5(prepCP),currentBG=bindCurrent(currentCP);'''
    assert source.count(old_bind_current) == 1
    source = source.replace(old_bind_current, new_bind_current)

    # In fallback setup, handle fallback mutations
    old_fallback_hook = '    fallbackShader.insert(where+rayHook.size(),"\\n if((ray.counts.w & 0x80000000u)==0u){return;}\\n");'
    assert source.count(old_fallback_hook) == 1
    if mutation == 'fallback-double-publish':
        source = source.replace(old_fallback_hook, '    // bypassed 0x80000000u fallback guard for double-publish mutation')
    elif mutation == 'fallback-missing-publish':
        mutation_patch = (
            old_fallback_hook + '\n' +
            '    {\n' +
            '        const std::string target = "capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);\\n atomicAdd(&auditWrites[idx * 2u + 1u], 1u);";\n' +
            '        auto p = fallbackShader.find(target);\n' +
            '        if(p != std::string::npos) fallbackShader.replace(p, target.size(), "if(idx > 1000000u) { capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance); atomicAdd(&auditWrites[idx * 2u + 1u], 1u); }");\n' +
            '    }'
        )
        source = source.replace(old_fallback_hook, mutation_patch)

    # In tile current/fallback lambda:
    old_tile_current = 'entry(4,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'
    assert source.count(old_tile_current) == 1
    new_tile_current = 'entry(4,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult)), entry(5,auditWritesBuf,outputOffset/2,uint64_t(n)*2*sizeof(uint32_t))});'
    source = source.replace(old_tile_current, new_tile_current)

    # 5. Bind groups for S1: add auditWrites at binding 3 (in both normal and forced bgS1!)
    needle_bg_s1 = ('        makeEntry(2, rBuf, 0, rayBytes),\n' +
                    '        makeEntry(4, outBuf, 0, outBytes),')
    assert source.count(needle_bg_s1) == 2
    source = source.replace(needle_bg_s1,
                            '        makeEntry(2, rBuf, 0, rayBytes),\n' +
                            '        makeEntry(3, auditWritesBuf, 0, count * 8),\n' +
                            '        makeEntry(4, outBuf, 0, outBytes),')

    # Bind groups for S2 and fallback: add auditWrites at binding 9
    needle_bg_s2_fb = 'makeEntry(8, cntBuf, 0, sizeof(initCnt))\n    });'
    assert source.count(needle_bg_s2_fb) == 4
    source = source.replace(needle_bg_s2_fb, 'makeEntry(8, cntBuf, 0, sizeof(initCnt)),\n        makeEntry(9, auditWritesBuf, 0, count * 8)\n    });')

    # 6. Clear auditWritesBuf in mode 3
    a = source.index('        auto execute=')
    b = source.index('        auto correctness=', a)
    part = source[a:b]
    needle_repeats = '            for(int k=0;k<repeats;++k){'
    assert part.count(needle_repeats) == 1
    part = part.replace(needle_repeats, needle_repeats + '\n                if(mode==3) wgpuCommandEncoderClearBuffer(enc,auditWritesBuf,0,count*8);')
    source = source[:a] + part + source[b:]
    source = source[:source.index('        auto correctness=', a)] + BRANCH + source[source.index('        auto correctness=', a):]

    s = source
    if mutation == 'no-reset':
        start = s.index('        auto execute=')
        end = s.index('        J routeChecks=', start)
        old_reset = 'auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);\n                        wgpuComputePassEncoderSetPipeline(p, cpReset);\n                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);\n                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);\n                        wgpuComputePassEncoderEnd(p);\n                        wgpuComputePassEncoderRelease(p);'
        assert s.count(old_reset, start, end) == 1
        s = s[:start] + s[start:end].replace(old_reset, '// reset omitted for no-reset mutation') + s[end:]

    if mutation == 'no-tail':
        start = s.index('static const char* kS1ProducerWgsl =')
        end = s.index('static const char* kS2ProducerWgsl =', start)
        old_tail = 'if (tileSlot >= min(routeCounts.x, 2304u)) { return; }'
        assert s.count(old_tail, start, end) == 1
        s = s[:start] + s[start:end].replace(old_tail, 'if (tileSlot >= min(routeCounts.x + 4u, 2304u)) { return; }') + s[end:]

    return s


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'no-tail', 'double-publish', 'missing-publish', 'fallback-missing-publish', 'fallback-double-publish'], default='fixed')
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--run', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    # If build_root is the build folder directly, its parent is the tree root
    worktree_root = build_root.parent if build_root.name == 'build' else build_root
    work = args.work.resolve()

    if work.exists():
        import shutil
        shutil.rmtree(work)
    work.mkdir(parents=True)

    print(f'Generating source for mutation: {args.mutation}...')
    cpp_source = generate(root, mutation=args.mutation)
    (work / 'probe.cpp').write_text(cpp_source)

    build_dir = work / 'build'
    subprocess.run([
        sys.executable, '-B', str(root / 'scratch/probes/ray_transport_probe_build.py'),
        '--root', str(root),
        '--build-root', str(worktree_root),
        '--source', str(work / 'probe.cpp'),
        '--work', str(build_dir)
    ], check=True)
    print('Compilation successful!')

    if args.run:
        spec = importlib.util.spec_from_file_location('wide', root / 'scratch/probes/ray_transport_gpu_wide_stream.py')
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / 'saves/zones/Northern Veil/zone.ecform'
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        meta, rays, _ = m.verify_case(root, args.case.resolve(), before)
        env = dict(
            os.environ,
            EC_POSE_RAYS=meta['rays'],
            EC_POSE_REFERENCE=meta['reference'],
            EC_POSE_CLOCK=str(rays['time']),
            EC_MATCH_REPEAT='1',
            EC_GPU_BUDGET='1024',
            EC_GPU_SUB_CAP='64',
            EC_GPU_OVER_CAP='16',
            EC_GPU_MARGIN='.001',
        )
        env.pop('EC_GPU_GEOMETRY_CASE', None)
        env.pop('METAL_CAPTURE_ENABLED', None)

        output = work / 'review.json'
        print(f'Executing control probe for mutation: {args.mutation}...')
        result = subprocess.run([str(build_dir / 'probe'), str(output)], cwd=root, env=env)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError('source drift')
        m.verify_case(root, args.case.resolve(), before)
        if result.returncode != 0:
            print(f'Control probe exited with code {result.returncode}')
            sys.exit(result.returncode)
        else:
            print(f'PASS control probe: {args.mutation}')



if __name__ == '__main__':
    main()
