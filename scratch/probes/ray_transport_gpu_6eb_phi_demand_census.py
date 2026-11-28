"""§6eb: Source-Bound Diagnostic Census of Actual evalPhi Demands Across Root Levels.

Antigravity / Gemini 2.5 Pro (Sparkly Guy) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a).
Scratch-only diagnostic research on Apple M5 in Earthcall-ray-transport.

Measures actual duplicated evalPhi demand across the three root levels ({-w_z, 0, +w_z}),
separated by S1/S2/general family and evaluation site:
  Site 0: 9 coarse knots (already shared across levels)
  Site 1: first adaptive bin midpoints (depth 1)
  Site 2: deeper adaptive midpoints (depth >= 2)
  Site 3: monotone-isolator iterations
  Site 4: post-root interval selection
  Site 5: consumer work (0 calls)

§6ec correction by Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a.
Captures exact f32 argument bits and explicitly typed observed values (raw materialized Phi, residual, or unavailable),
site identity, root level, curtain member, and per-ray monotonic execution sequence.
Distinguishes online available reuse from offline duplicates.
Enforces bounded event storage with loud overflow refusal.
Qualifies instrumented outputs/packets against reference before trusting counts.
"""

import argparse, ast, hashlib, importlib.util, json, math, os, re, shlex, struct, subprocess, sys
from pathlib import Path
from collections import defaultdict

def sha256_file(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

INSTRUMENTATION_PREAMBLE = '''
struct EvalEvent {
    rayIdx: u32,
    mediumIdx: u32,
    siteAndLevel: u32,
    seqInRay: u32,
    sBits: u32,
    phiBits: u32,
    detail: u32,
    reserved: u32,
};

struct EvalCensusBuffer {
    eventCount: atomic<u32>,
    overflowCount: atomic<u32>,
    maxCapacity: u32,
    reserved: u32,
    events: array<EvalEvent>,
};

@group(0) @binding(11) var<storage, read_write> evalCensus: EvalCensusBuffer;

fn recordEvalEvent(rayIdx: u32, mIdx: u32, site: u32, level: u32, detail: u32, seq: u32, s: f32, phi: f32, quantity: u32) {
    let slot = atomicAdd(&evalCensus.eventCount, 1u);
    if (slot < evalCensus.maxCapacity) {
        evalCensus.events[slot].rayIdx = rayIdx;
        evalCensus.events[slot].mediumIdx = mIdx;
        evalCensus.events[slot].siteAndLevel = (site << 4u) | (level & 15u);
        evalCensus.events[slot].seqInRay = seq;
        evalCensus.events[slot].sBits = bitcast<u32>(s);
        evalCensus.events[slot].phiBits = bitcast<u32>(phi);
        evalCensus.events[slot].detail = detail;
        evalCensus.events[slot].reserved = 0u;
    } else {
        atomicAdd(&evalCensus.overflowCount, 1u);
    }
}
'''

def instrument_producer_wgsl(source: str, family: int, capture_values='residual') -> str:
    if family not in [1,2,3] or capture_values not in ['residual','arguments']:
        raise ValueError('invalid diagnostic provenance')
    # Bind the uninstrumented roots/depth helpers to the reviewed proof. The
    # diagnostic retains stack[20]; it does not inherit a smaller allocation.
    import importlib.util
    proof_path = Path(__file__).with_name('ray_transport_gpu_6dx_stack_bound.py')
    spec = importlib.util.spec_from_file_location('census_proof', proof_path)
    proof = importlib.util.module_from_spec(spec); spec.loader.exec_module(proof)
    proof.verify_source_matches_proof(source)
    patches = []
    def patch(text, old, new):
        count = text.count(old)
        if count < 1: raise ValueError('missing certified instrumentation hook')
        patches.append((old,new,count))
        return text.replace(old,new)
    s = source
    needle_eval = 'fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32{return o.z+s*d.z+evalWave(m,o.x+s*d.x);}'
    if needle_eval not in s:
        raise ValueError('evalPhi definition hook not found in producer WGSL')
    s = patch(s,needle_eval, needle_eval + '\n' + INSTRUMENTATION_PREAMBLE.replace('slot].reserved = 0u;', f'slot].reserved = ({family}u << 8u) | quantity;'))

    old_iso_sig = 'reason: ptr<function, u32>, work: ptr<function, u32>,\n zeroBands: ptr<function, u32>, isZeroLevel: bool) {'
    new_iso_sig = 'reason: ptr<function, u32>, work: ptr<function, u32>,\n zeroBands: ptr<function, u32>, isZeroLevel: bool, rayIdx: u32, mIdx: u32, levelIdx: u32, seq: ptr<function, u32>) {'
    if old_iso_sig not in s:
        raise ValueError('isolateMonotone signature hook not found')
    s = patch(s,old_iso_sig, new_iso_sig)

    old_iso_call = 'let v = evalPhi(m, o, d, mid) - c;'
    new_iso_call = ('let v = evalPhi(m, o, d, mid) - c;\n'
                    f'   recordEvalEvent(rayIdx, mIdx, 3u, levelIdx, iter, *seq, mid, {"v, 1u" if capture_values == "residual" else "0.0, 2u"}); *seq += 1u;')
    if old_iso_call not in s:
        raise ValueError('isolateMonotone evalPhi call hook not found')
    s = patch(s,old_iso_call, new_iso_call)

    old_roots_sig = 'zeroBands: ptr<function, u32>, isZeroLevel: bool,\n levelActiveMask: u32) {'
    new_roots_sig = 'zeroBands: ptr<function, u32>, isZeroLevel: bool,\n levelActiveMask: u32, rayIdx: u32, mIdx: u32, levelIdx: u32, seq: ptr<function, u32>) {'
    if old_roots_sig not in s:
        raise ValueError('roots signature hook not found')
    s = patch(s,old_roots_sig, new_roots_sig)

    old_iso_inv = 'bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);'
    new_iso_inv = 'bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel, rayIdx, mIdx, levelIdx, seq);'
    if s.count(old_iso_inv) != 2:
        raise ValueError(f'expected 2 isolateMonotone calls in roots, found {s.count(old_iso_inv)}')
    s = patch(s,old_iso_inv, new_iso_inv)

    old_mid0 = 'let phi_mid0 = evalPhi(m, o, d, mid0);'
    new_mid0 = ('let phi_mid0 = evalPhi(m, o, d, mid0);\n'
                f'  recordEvalEvent(rayIdx, mIdx, 1u, levelIdx, 1u, *seq, mid0, {"phi_mid0, 0u" if capture_values == "residual" else "0.0, 2u"}); *seq += 1u;')
    if old_mid0 not in s:
        raise ValueError('mid0 evalPhi call hook not found')
    s = patch(s,old_mid0, new_mid0)

    old_mid = 'let phi_mid = evalPhi(m, o, d, mid);'
    new_mid = ('let phi_mid = evalPhi(m, o, d, mid);\n'
               f'   recordEvalEvent(rayIdx, mIdx, 2u, levelIdx, depth + 1u, *seq, mid, {"phi_mid, 0u" if capture_values == "residual" else "0.0, 2u"}); *seq += 1u;')
    if old_mid not in s:
        raise ValueError('mid evalPhi call hook not found')
    s = patch(s,old_mid, new_mid)

    needle_main = 'var count = 0u;'
    if needle_main not in s:
        raise ValueError('main producer count hook not found')
    s = patch(s,needle_main, needle_main + '\n var evalSeq = 0u;')

    old_knot = 'phi_knots[k] = evalPhi(m, o, d, s);'
    new_knot = ('phi_knots[k] = evalPhi(m, o, d, s);\n'
                f'   recordEvalEvent(id, member, 0u, 3u, k, evalSeq, s, {"phi_knots[k], 0u" if capture_values == "residual" else "0.0, 2u"}); evalSeq++;')
    if old_knot not in s:
        raise ValueError('phi_knots evalPhi call hook not found')
    s = patch(s,old_knot, new_knot)

    old_r0 = 'roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);'
    new_r0 = 'roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0, id, member, 0u, &evalSeq);'
    if old_r0 not in s:
        raise ValueError('roots level 0 call hook not found')
    s = patch(s,old_r0, new_r0)

    old_r1 = 'roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);'
    new_r1 = 'roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1, id, member, 1u, &evalSeq);'
    if old_r1 not in s:
        raise ValueError('roots level 1 call hook not found')
    s = patch(s,old_r1, new_r1)

    old_r2 = 'roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);'
    new_r2 = 'roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2, id, member, 2u, &evalSeq);'
    if old_r2 not in s:
        raise ValueError('roots level 2 call hook not found')
    s = patch(s,old_r2, new_r2)

    old_post = 'if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y)'
    # Observe demand without rewriting the original short-circuit expression
    # or claiming a raw Phi value that the original native arithmetic may fuse.
    new_post = ('if (!uncertain) {\n'
                '       recordEvalEvent(id, member, 4u, 3u, i, evalSeq, mid, 0.0, 2u); evalSeq++;\n'
                '   }\n   ' + old_post)
    if old_post not in s:
        raise ValueError('post-root evalPhi call hook not found')
    s = patch(s,old_post, new_post)

    restored = s
    for old,new,count in reversed(patches):
        if restored.count(new) != count: raise ValueError('instrumentation inverse is ambiguous')
        restored = restored.replace(new,old)
    if restored != source: raise ValueError('instrumentation does not restore the original shader')
    return s


def generate_census_cpp(root: Path, mutation: str = 'fixed', capture_values='residual') -> str:
    spec = importlib.util.spec_from_file_location('controls_6du', root / 'scratch/probes/ray_transport_gpu_6du_controls.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    
    du_mutation = {'terminal-missing-publish':'missing-publish','fallback-missing-publish':'fallback-missing-publish'}.get(mutation,'fixed')
    source = m.generate(root, mutation=du_mutation)

    # Patch device limits to grant up to 31 storage buffers per stage on Metal
    old_init = '''    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "FAILED to initialize WebGPU device on Metal" << std::endl;
        return 1;
    }'''

    new_init = '''    wgpu::Device gpu;
    gpu.instance = wgpuCreateInstance(nullptr);
    {
        wgpu::detail::AdapterR ar;
        WGPURequestAdapterCallbackInfo acb{};
        acb.mode = WGPUCallbackMode_AllowProcessEvents;
        acb.callback = wgpu::detail::onAdapter;
        acb.userdata1 = &ar;
        wgpuInstanceRequestAdapter(gpu.instance, nullptr, acb);
        while (!ar.done) wgpuInstanceProcessEvents(gpu.instance);
        gpu.adapter = ar.a;

        WGPULimits supportedLimits{};
        wgpuAdapterGetLimits(gpu.adapter, &supportedLimits);

        WGPUDeviceDescriptor dd{};
        dd.requiredLimits = &supportedLimits;

        wgpu::detail::DeviceR dr;
        WGPURequestDeviceCallbackInfo dcb{};
        dcb.mode = WGPUCallbackMode_AllowProcessEvents;
        dcb.callback = wgpu::detail::onDevice;
        dcb.userdata1 = &dr;
        wgpuAdapterRequestDevice(gpu.adapter, &dd, dcb);
        while (!dr.done) wgpuInstanceProcessEvents(gpu.instance);
        gpu.device = dr.d;
        gpu.queue = wgpuDeviceGetQueue(gpu.device);
    }'''
    assert old_init in source
    source = source.replace(old_init, new_init)

    # Add census header struct and buffer allocation
    max_events = 100 if mutation == 'overflow-tripwire' else 8000000
    census_preamble = f'''
    struct alignas(16) EvalCensusHeader {{
        uint32_t eventCount;
        uint32_t overflowCount;
        uint32_t maxCapacity;
        uint32_t reserved;
    }};
    struct alignas(16) EvalEvent {{
        uint32_t rayIdx;
        uint32_t mediumIdx;
        uint32_t siteAndLevel;
        uint32_t seqInRay;
        uint32_t sBits;
        uint32_t phiBits;
        uint32_t detail;
        uint32_t reserved;
    }};
    size_t maxCensusEvents = {max_events}u;
    size_t censusBytes = sizeof(EvalCensusHeader) + maxCensusEvents * sizeof(EvalEvent);
    WGPUBuffer evalCensusBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, censusBytes, nullptr);
'''
    needle_audit_buf = '    WGPUBuffer auditWritesBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, auditBytes, nullptr);'
    assert source.count(needle_audit_buf) == 1
    source = source.replace(needle_audit_buf, needle_audit_buf + '\n' + census_preamble)

    # Instrument all 3 producer WGSL shaders
    for name in ['kS1ProducerWgsl', 'kS2ProducerWgsl', 'kFallbackProducerWgsl']:
        hit = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
        assert hit, f"shader {name} not found"
        family = {'kS1ProducerWgsl':1,'kS2ProducerWgsl':2,'kFallbackProducerWgsl':3}[name]
        instrumented = instrument_producer_wgsl(hit[2], family, capture_values)
        source = source[:hit.start(2)] + instrumented + source[hit.end(2):]

    # Add binding 11 to S1 bind groups (bgS1 and bgS1Forced)
    needle_s1_b10 = 'makeEntry(10, pBuf, 0, paramBytes)\n    });'
    assert source.count(needle_s1_b10) == 2, f"expected 2 occurrences of bgS1 paramBytes, found {source.count(needle_s1_b10)}"
    source = source.replace(needle_s1_b10,
                            'makeEntry(10, pBuf, 0, paramBytes),\n' +
                            '        makeEntry(11, evalCensusBuf, 0, censusBytes)\n    });')

    # Add binding 11 to S2 and Fallback bind groups (bgS2, bgS2Forced, bgFallback, bgFallbackForced)
    needle_s2_fb_b9 = 'makeEntry(9, auditWritesBuf, 0, count * 8)\n    });'
    assert source.count(needle_s2_fb_b9) == 4, f"expected 4 occurrences of bgS2/FB auditWrites, found {source.count(needle_s2_fb_b9)}"
    source = source.replace(needle_s2_fb_b9,
                            'makeEntry(9, auditWritesBuf, 0, count * 8),\n' +
                            '        makeEntry(11, evalCensusBuf, 0, censusBytes)\n    });')

    # Clear evalCensusBuf header (eventCount and overflowCount) in repeat loop
    needle_clear = 'if(mode==3) wgpuCommandEncoderClearBuffer(enc,auditWritesBuf,0,auditBytes);'
    assert source.count(needle_clear) == 1
    source = source.replace(needle_clear, needle_clear + '\n                if(mode==3) wgpuCommandEncoderClearBuffer(enc,evalCensusBuf,0,8);')

    # In checkReview: inject buffer reset before execute(3, rep, false)
    needle_before_exec = 'execute(3,rep,false);'
    assert source.count(needle_before_exec) == 1
    reset_census = '''            // Reset evalCensus buffer header before execution
            EvalCensusHeader initCensusHeader{0, 0, (uint32_t)maxCensusEvents, 0};
            wgpuQueueWriteBuffer(gpu.queue, evalCensusBuf, 0, &initCensusHeader, sizeof(initCensusHeader));
            '''
    source = source.replace(needle_before_exec, reset_census + needle_before_exec)

    # In checkReview: inject readback and overflow check after outputs readback
    needle_after_exec = 'auto wb=readBackBuffer(auditWritesBuf,auditBytes);'
    assert source.count(needle_after_exec) == 1
    census_readback = '''auto wb=readBackBuffer(auditWritesBuf,auditBytes);
            // Read back census header and verify overflow
            auto censusHdrBytes = readBackBuffer(evalCensusBuf, sizeof(EvalCensusHeader));
            EvalCensusHeader ch{};
            std::memcpy(&ch, censusHdrBytes.data(), sizeof(ch));
            if(ch.overflowCount > 0){
                throw std::runtime_error("LOUD REFUSAL: evalCensus event storage overflowed by " +
                                         std::to_string(ch.overflowCount) + " events! (capacity=" +
                                         std::to_string(ch.maxCapacity) + ", attempted=" +
                                         std::to_string(ch.eventCount) + ")");
            }

            // Dump event buffer on baseline pose-0 rep=1
            if(rep == 1 && normal && label == "pose-0"){
                size_t readBytes = sizeof(EvalCensusHeader) + ch.eventCount * sizeof(EvalEvent);
                auto allBytes = readBackBuffer(evalCensusBuf, readBytes);
                std::string binPath = outputPath + ".events.bin";
                std::ofstream binOut(binPath, std::ios::binary);
                binOut.write(reinterpret_cast<const char*>(allBytes.data()), allBytes.size());
                std::cerr << "EVAL PHI CENSUS: captured " << ch.eventCount << " events (" << readBytes << " bytes) -> " << binPath << "\\n";
            }'''
    source = source.replace(needle_after_exec, census_readback)

    # Exact admission is unchanged. Any wrong output OR packet refuses.
    strict = 'if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");'
    if source.count(strict) != 1: raise ValueError('strict output/packet admission missing')
    source = source.replace('J routeChecks=J::array(); auto originalInputs=gpuRays;',
        'J routeChecks=J::array(); auto originalInputs=gpuRays; bool censusSaved=false; std::map<std::string,uint32_t> censusCounts;')
    source = source.replace('if(rep == 1 && normal && label == "pose-0"){',
        'if(!censusSaved && rep == 1 && normal && label == "pose-0"){ censusSaved=true;')
    marker = '            // Dump event buffer on baseline pose-0 rep=1'
    source = source.replace(marker,
        '            if(rep==1)censusCounts[label]=ch.eventCount;\n'
        '            else if(censusCounts[label]!=ch.eventCount)throw std::runtime_error("census repeat accumulation");\n'+marker)
    source = source.replace('routeChecks.push_back({{"case",label}',
        'routeChecks.push_back({{"census_events",ch.eventCount},{"census_overflow",ch.overflowCount},{"case",label}')
    if mutation == 'no-census-reset':
        marker = 'if(mode==3) wgpuCommandEncoderClearBuffer(enc,evalCensusBuf,0,8);'
        if source.count(marker)!=1: raise ValueError('census reset hook missing')
        source = source.replace(marker,'// census repeat reset intentionally omitted')

    old_log = 'std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified)\\n";'
    new_log = 'std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified, events="<<ch.eventCount<<", diffs="<<wrong<<")\\n";'
    assert source.count(old_log) == 1
    source = source.replace(old_log, new_log)

    return source


def parse_and_analyze_census(events_bin_path: Path, rays_manifest_path: Path) -> dict:
    helper = Path(__file__).with_name('ray_transport_gpu_6ec_census_review.py')
    spec = importlib.util.spec_from_file_location('census_admission', helper)
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    return m.analyze_events(events_bin_path, m.manifest_ray_count(rays_manifest_path))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path('.'))
    p.add_argument('--build-root',type=Path,required=True)
    p.add_argument('--work',type=Path,required=True)
    p.add_argument('--case',type=Path,default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--mutation',choices=['fixed','overflow-tripwire','terminal-missing-publish','fallback-missing-publish','no-census-reset'],default='fixed')
    p.add_argument('--capture-values',choices=['residual','arguments'],default='residual')
    p.add_argument('--run',action='store_true')
    a=p.parse_args();root=a.root.resolve();build=a.build_root.resolve();work=a.work.resolve()
    if work.exists() or work==root or root in work.parents: raise ValueError('fresh external work required; existing outputs are preserved')
    work.mkdir(parents=True,exist_ok=False)
    cpp=generate_census_cpp(root,a.mutation,a.capture_values);source=work/'probe.cpp';source.write_text(cpp)
    meta={'cpp_sha256':sha256_file(source),'generator_sha256':sha256_file(Path(__file__)),'capture_values':a.capture_values,'mutation':a.mutation,'diagnostic_pending_stack':20,'engine_src_tree':subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=root,text=True).strip()}
    (work/'source.json').write_text(json.dumps(meta,indent=2)+'\n')
    subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(build),'--source',str(source),'--work',str(work/'build')],check=True)
    if not a.run:return
    spec=importlib.util.spec_from_file_location('wide',root/'scratch/probes/ray_transport_gpu_wide_stream.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    zone=root/'saves/zones/Northern Veil/zone.ecform';before=sha256_file(zone);fixture,rays,_=m.verify_case(root,a.case.resolve(),before)
    env=dict(os.environ,EC_POSE_RAYS=fixture['rays'],EC_POSE_REFERENCE=fixture['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE',None);env.pop('METAL_CAPTURE_ENABLED',None)
    with (work/'native.log').open('xb') as log:result=subprocess.run([str(work/'build/probe'),str(work/'review.json')],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
    if sha256_file(zone)!=before:raise ValueError('fixture drift')
    m.verify_case(root,a.case.resolve(),before)
    if result.returncode:raise SystemExit(result.returncode)
    controls=json.loads((work/'review.json').read_text())
    if len(controls['cases'])!=30 or any(c['output_differences'] or c['packet_differences'] not in [0,None] or c['census_overflow'] for c in controls['cases']):raise ValueError('strict census control qualification failed')
    analysis=parse_and_analyze_census(work/'review.json.events.bin',a.case)
    (work/'census_analysis.json').write_text(json.dumps(analysis,indent=2)+'\n')
    print('PASS: strict30-case output/packet/ownership controls; explicit actual routes and observed value kinds')
    print(json.dumps({k:analysis[k] for k in ['total_evaluations','position_repeats','value_observations','actual_route_counts']}))

if __name__=='__main__':main()
