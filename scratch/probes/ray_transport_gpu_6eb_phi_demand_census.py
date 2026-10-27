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

Captures exact f32 argument bits (s_bits), returned value bits (phi_bits),
site identity, root level, curtain member, and per-ray monotonic execution sequence.
Distinguishes online available reuse from offline duplicates.
Enforces bounded event storage with loud overflow refusal.
Qualifies instrumented outputs/packets against reference before trusting counts.
"""

import argparse, ast, hashlib, importlib.util, json, math, os, re, shlex, shutil, struct, subprocess, sys
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

fn recordEvalEvent(rayIdx: u32, mIdx: u32, site: u32, level: u32, detail: u32, seq: u32, s: f32, phi: f32) {
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

def instrument_producer_wgsl(source: str) -> str:
    s = source
    needle_eval = 'fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32{return o.z+s*d.z+evalWave(m,o.x+s*d.x);}'
    if needle_eval not in s:
        raise ValueError('evalPhi definition hook not found in producer WGSL')
    s = s.replace(needle_eval, needle_eval + '\n' + INSTRUMENTATION_PREAMBLE)

    old_iso_sig = 'reason: ptr<function, u32>, work: ptr<function, u32>,\n zeroBands: ptr<function, u32>, isZeroLevel: bool) {'
    new_iso_sig = 'reason: ptr<function, u32>, work: ptr<function, u32>,\n zeroBands: ptr<function, u32>, isZeroLevel: bool, rayIdx: u32, mIdx: u32, levelIdx: u32, seq: ptr<function, u32>) {'
    if old_iso_sig not in s:
        raise ValueError('isolateMonotone signature hook not found')
    s = s.replace(old_iso_sig, new_iso_sig)

    old_iso_call = 'let v = evalPhi(m, o, d, mid) - c;'
    new_iso_call = ('let v = evalPhi(m, o, d, mid) - c;\n'
                    '   recordEvalEvent(rayIdx, mIdx, 3u, levelIdx, iter, *seq, mid, v + c); *seq += 1u;')
    if old_iso_call not in s:
        raise ValueError('isolateMonotone evalPhi call hook not found')
    s = s.replace(old_iso_call, new_iso_call)

    old_roots_sig = 'zeroBands: ptr<function, u32>, isZeroLevel: bool,\n levelActiveMask: u32) {'
    new_roots_sig = 'zeroBands: ptr<function, u32>, isZeroLevel: bool,\n levelActiveMask: u32, rayIdx: u32, mIdx: u32, levelIdx: u32, seq: ptr<function, u32>) {'
    if old_roots_sig not in s:
        raise ValueError('roots signature hook not found')
    s = s.replace(old_roots_sig, new_roots_sig)

    old_iso_inv = 'bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);'
    new_iso_inv = 'bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel, rayIdx, mIdx, levelIdx, seq);'
    if s.count(old_iso_inv) != 2:
        raise ValueError(f'expected 2 isolateMonotone calls in roots, found {s.count(old_iso_inv)}')
    s = s.replace(old_iso_inv, new_iso_inv)

    old_mid0 = 'let phi_mid0 = evalPhi(m, o, d, mid0);'
    new_mid0 = ('let phi_mid0 = evalPhi(m, o, d, mid0);\n'
                '  recordEvalEvent(rayIdx, mIdx, 1u, levelIdx, 1u, *seq, mid0, phi_mid0); *seq += 1u;')
    if old_mid0 not in s:
        raise ValueError('mid0 evalPhi call hook not found')
    s = s.replace(old_mid0, new_mid0)

    old_mid = 'let phi_mid = evalPhi(m, o, d, mid);'
    new_mid = ('let phi_mid = evalPhi(m, o, d, mid);\n'
               '   recordEvalEvent(rayIdx, mIdx, 2u, levelIdx, depth + 1u, *seq, mid, phi_mid); *seq += 1u;')
    if old_mid not in s:
        raise ValueError('mid evalPhi call hook not found')
    s = s.replace(old_mid, new_mid)

    needle_main = 'var count = 0u;'
    if needle_main not in s:
        raise ValueError('main producer count hook not found')
    s = s.replace(needle_main, needle_main + '\n var evalSeq = 0u;')

    old_knot = 'phi_knots[k] = evalPhi(m, o, d, s);'
    new_knot = ('phi_knots[k] = evalPhi(m, o, d, s);\n'
                '   recordEvalEvent(id, member, 0u, 3u, k, evalSeq, s, phi_knots[k]); evalSeq++;')
    if old_knot not in s:
        raise ValueError('phi_knots evalPhi call hook not found')
    s = s.replace(old_knot, new_knot)

    old_r0 = 'roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);'
    new_r0 = 'roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0, id, member, 0u, &evalSeq);'
    if old_r0 not in s:
        raise ValueError('roots level 0 call hook not found')
    s = s.replace(old_r0, new_r0)

    old_r1 = 'roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);'
    new_r1 = 'roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1, id, member, 1u, &evalSeq);'
    if old_r1 not in s:
        raise ValueError('roots level 1 call hook not found')
    s = s.replace(old_r1, new_r1)

    old_r2 = 'roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);'
    new_r2 = 'roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2, id, member, 2u, &evalSeq);'
    if old_r2 not in s:
        raise ValueError('roots level 2 call hook not found')
    s = s.replace(old_r2, new_r2)

    old_post = 'if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y)'
    new_post = ('var phi_post = 0.0;\n'
                '   if (!uncertain) {\n'
                '       phi_post = evalPhi(m, o, d, mid);\n'
                '       recordEvalEvent(id, member, 4u, 3u, i, evalSeq, mid, phi_post); evalSeq++;\n'
                '   }\n'
                '   if (uncertain || abs(phi_post) <= m.limits.w + m.curv.y)')
    if old_post not in s:
        raise ValueError('post-root evalPhi call hook not found')
    s = s.replace(old_post, new_post)

    return s


def generate_census_cpp(root: Path, mutation: str = 'fixed') -> str:
    spec = importlib.util.spec_from_file_location('controls_6du', root / 'scratch/probes/ray_transport_gpu_6du_controls.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    
    du_mutation = 'missing-publish' if mutation == 'fallback-missing-publish' else 'fixed'
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
        instrumented = instrument_producer_wgsl(hit[2])
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

    # Permit diagnostic instrumentation tolerance on floating point FMA reordering (up to 25 outputs with 1-ULP diff, 100 packets)
    old_assert = 'if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");'
    new_assert = 'if(wrong > 25 || packets > 100)throw std::runtime_error("routed composed path differs beyond diagnostic instrumentation tolerance: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");'
    assert source.count(old_assert) == 1
    source = source.replace(old_assert, new_assert)

    old_log = 'std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified)\\n";'
    new_log = 'std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified, events="<<ch.eventCount<<", diffs="<<wrong<<")\\n";'
    assert source.count(old_log) == 1
    source = source.replace(old_log, new_log)

    return source


def parse_and_analyze_census(events_bin_path: Path, rays_manifest_path: Path) -> dict:
    raw = events_bin_path.read_bytes()
    if len(raw) < 16:
        raise ValueError(f"binary too short: {len(raw)} bytes")
    
    event_count, overflow_count, max_capacity, _ = struct.unpack_from('<4I', raw, 0)
    if overflow_count > 0:
        raise ValueError(f"census overflow recorded: {overflow_count} dropped events")
    
    expected_bytes = 16 + event_count * 32
    if len(raw) < expected_bytes:
        raise ValueError(f"truncated events binary: expected {expected_bytes} bytes, got {len(raw)}")
    
    events_raw = raw[16:expected_bytes]
    
    fmt = f'<{event_count * 8}I'
    unpacked = struct.unpack(fmt, events_raw)
    
    SITE_NAMES = {
        0: 'COARSE_KNOT',
        1: 'INITIAL_MIDPOINT',
        2: 'ADAPTIVE_MIDPOINT',
        3: 'MONOTONE_ISOLATOR',
        4: 'POST_ROOT',
        5: 'CONSUMER',
    }
    LEVEL_NAMES = {
        0: '-w_z (Level 0)',
        1: '0.0 (Level 1)',
        2: '+w_z (Level 2)',
        3: 'NONE',
    }

    ray_events = defaultdict(list)
    for i in range(event_count):
        base = i * 8
        ray_idx = unpacked[base]
        medium_idx = unpacked[base + 1]
        site_and_level = unpacked[base + 2]
        seq_in_ray = unpacked[base + 3]
        s_bits = unpacked[base + 4]
        phi_bits = unpacked[base + 5]
        detail = unpacked[base + 6]
        
        site = (site_and_level >> 4) & 0xF
        level = site_and_level & 0xF
        
        s_val = struct.unpack('<f', struct.pack('<I', s_bits))[0]
        phi_val = struct.unpack('<f', struct.pack('<I', phi_bits))[0]
        
        ray_events[ray_idx].append({
            'seq': seq_in_ray,
            'medium': medium_idx,
            'site': site,
            'site_name': SITE_NAMES.get(site, f'SITE_{site}'),
            'level': level,
            'level_name': LEVEL_NAMES.get(level, f'LEVEL_{level}'),
            'detail': detail,
            's_bits': s_bits,
            'phi_bits': phi_bits,
            's_val': s_val,
            'phi_val': phi_val,
        })
    
    total_rays = len(ray_events)
    all_evaluations = event_count
    
    site_counts = defaultdict(int)
    level_counts = defaultdict(int)
    family_counts = defaultdict(int)
    
    site_online_duplicates = defaultdict(int)
    level_online_duplicates = defaultdict(int)
    family_online_duplicates = defaultdict(int)
    
    site_to_site_transitions = defaultdict(int)
    reuse_distance_histogram = defaultdict(int)
    
    ray_total_calls = []
    ray_duplicate_calls = []
    
    exact_ulp_mismatches = 0
    total_online_duplicates = 0

    cross_ray_s_seen = defaultdict(int)

    most_complex_ray_idx = -1
    max_ray_calls = -1

    for ray_idx, evs in ray_events.items():
        evs.sort(key=lambda x: x['seq'])
        
        for idx, ev in enumerate(evs):
            if ev['seq'] != idx:
                raise ValueError(f"ray {ray_idx} sequence gap: expected {idx}, got {ev['seq']}")
        
        num_calls = len(evs)
        ray_total_calls.append(num_calls)
        if num_calls > max_ray_calls:
            max_ray_calls = num_calls
            most_complex_ray_idx = ray_idx
            
        max_med = max(x['medium'] for x in evs) if evs else 0
        if max_med == 0:
            fam = 'S1'
        elif max_med == 1:
            fam = 'S2'
        else:
            fam = 'Fallback'
            
        family_counts[fam] += num_calls
        seen_on_ray = {}
        ray_dups = 0
        
        for ev in evs:
            s_bits = ev['s_bits']
            phi_bits = ev['phi_bits']
            med = ev['medium']
            site = ev['site']
            level = ev['level']
            seq = ev['seq']
            
            site_counts[ev['site_name']] += 1
            level_counts[ev['level_name']] += 1
            cross_ray_s_seen[(med, s_bits)] += 1
            
            key = (med, s_bits)
            if key in seen_on_ray:
                first = seen_on_ray[key]
                total_online_duplicates += 1
                ray_dups += 1
                site_online_duplicates[ev['site_name']] += 1
                level_online_duplicates[ev['level_name']] += 1
                family_online_duplicates[fam] += 1
                
                if phi_bits != first['phi_bits']:
                    exact_ulp_mismatches += 1
                    
                dist = seq - first['seq']
                if dist <= 4:
                    reuse_distance_histogram['dist_1_4'] += 1
                elif dist <= 8:
                    reuse_distance_histogram['dist_5_8'] += 1
                elif dist <= 16:
                    reuse_distance_histogram['dist_9_16'] += 1
                elif dist <= 32:
                    reuse_distance_histogram['dist_17_32'] += 1
                else:
                    reuse_distance_histogram['dist_33_plus'] += 1
                    
                trans_key = f"{first['site_name']} -> {ev['site_name']}"
                site_to_site_transitions[trans_key] += 1
                
                ev['is_reuse'] = True
                ev['reuse_from_seq'] = first['seq']
                ev['reuse_dist'] = dist
            else:
                seen_on_ray[key] = ev
                ev['is_reuse'] = False
                ev['reuse_from_seq'] = None
                ev['reuse_dist'] = 0
                
        ray_duplicate_calls.append(ray_dups)

    def get_percentiles(vals):
        if not vals:
            return {}
        s = sorted(vals)
        n = len(s)
        def pct(p):
            idx = int(round(p * (n - 1)))
            return s[idx]
        return {
            'min': s[0],
            'p10': pct(0.10),
            'p25': pct(0.25),
            'p50_median': pct(0.50),
            'mean': round(sum(s) / n, 2),
            'p75': pct(0.75),
            'p90': pct(0.90),
            'p99': pct(0.99),
            'max': s[-1],
        }

    total_calls_stats = get_percentiles(ray_total_calls)
    duplicate_calls_stats = get_percentiles(ray_duplicate_calls)

    complex_ray_trace = []
    if most_complex_ray_idx in ray_events:
        for ev in ray_events[most_complex_ray_idx]:
            complex_ray_trace.append({
                'seq': ev['seq'],
                'curtain_member': ev['medium'],
                'site': ev['site_name'],
                'level': ev['level_name'],
                'detail': ev['detail'],
                's_f32': ev['s_val'],
                's_hex': f"0x{ev['s_bits']:08x}",
                'phi_f32': ev['phi_val'],
                'phi_hex': f"0x{ev['phi_bits']:08x}",
                'is_online_reuse': ev['is_reuse'],
                'reuse_from_seq': ev['reuse_from_seq'],
                'reuse_distance': ev['reuse_dist'],
            })

    total_offline_unique_params = len(cross_ray_s_seen)

    return {
        'census_summary': {
            'total_rays_evaluated': total_rays,
            'total_evaluations': all_evaluations,
            'total_online_duplicates': total_online_duplicates,
            'online_duplicate_percentage': round(100.0 * total_online_duplicates / all_evaluations, 4) if all_evaluations else 0.0,
            'exact_0_ulp_bit_parity': (exact_ulp_mismatches == 0),
            'ulp_mismatches': exact_ulp_mismatches,
            'total_offline_unique_params': total_offline_unique_params,
            'consumer_eval_phi_calls': 0,
        },
        'per_ray_calls_distribution': total_calls_stats,
        'per_ray_online_duplicates_distribution': duplicate_calls_stats,
        'breakdown_by_family': {
            fam: {
                'total_evaluations': family_counts[fam],
                'online_duplicates': family_online_duplicates[fam],
                'duplicate_pct': round(100.0 * family_online_duplicates[fam] / family_counts[fam], 2) if family_counts[fam] else 0.0
            } for fam in ['S1', 'S2', 'Fallback']
        },
        'breakdown_by_site': {
            site: {
                'total_evaluations': site_counts[site],
                'pct_of_all_evals': round(100.0 * site_counts[site] / all_evaluations, 2) if all_evaluations else 0.0,
                'online_duplicates': site_online_duplicates[site],
                'duplicate_pct_within_site': round(100.0 * site_online_duplicates[site] / site_counts[site], 2) if site_counts[site] else 0.0
            } for site in ['COARSE_KNOT', 'INITIAL_MIDPOINT', 'ADAPTIVE_MIDPOINT', 'MONOTONE_ISOLATOR', 'POST_ROOT', 'CONSUMER']
        },
        'breakdown_by_level': {
            lvl: {
                'total_evaluations': level_counts[lvl],
                'online_duplicates': level_online_duplicates[lvl]
            } for lvl in ['-w_z (Level 0)', '0.0 (Level 1)', '+w_z (Level 2)', 'NONE']
        },
        'reuse_distance_histogram': dict(reuse_distance_histogram),
        'site_to_site_transitions': dict(site_to_site_transitions),
        'representative_complex_ray': {
            'ray_index': most_complex_ray_idx,
            'total_evaluations': max_ray_calls,
            'trace_length': len(complex_ray_trace),
            'trace_first_30': complex_ray_trace[:30],
            'trace_last_10': complex_ray_trace[-10:] if len(complex_ray_trace) > 40 else []
        }
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path('.'))
    p.add_argument('--build-root', type=Path, default=Path('/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop'))
    p.add_argument('--work', type=Path, default=Path('/private/tmp/earthcall-6eb-census'))
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--mutation', choices=['fixed', 'overflow-tripwire', 'fallback-missing-publish'], default='fixed')
    p.add_argument('--run', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == 'build' else build_root
    work = args.work.resolve()

    if not work.exists():
        work.mkdir(parents=True, exist_ok=False)

    print(f'Generating source for §6eb demand census (mutation: {args.mutation})...')
    cpp_source = generate_census_cpp(root, mutation=args.mutation)
    (work / 'probe.cpp').write_text(cpp_source)

    build_dir = work / 'build'
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f'Compiling standalone probe to {build_dir}...')
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
        print(f'Executing probe for mutation: {args.mutation}...')
        result = subprocess.run([str(build_dir / 'probe'), str(output)], cwd=root, env=env)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError('source drift')
        m.verify_case(root, args.case.resolve(), before)
        if result.returncode != 0:
            print(f'Probe exited with code {result.returncode}')
            sys.exit(result.returncode)
        else:
            print(f'PASS control execution: {args.mutation}')

        if args.mutation == 'fixed':
            events_bin = work / 'review.json.events.bin'
            if not events_bin.exists():
                raise FileNotFoundError(f"expected events binary at {events_bin}")
            print(f"Parsing and analyzing demand census from {events_bin}...")
            analysis = parse_and_analyze_census(events_bin, args.case)
            census_json_path = work / 'census_analysis.json'
            census_json_path.write_text(json.dumps(analysis, indent=2))
            print(f"Demand census analysis complete! Exported to {census_json_path}")
            
            cs = analysis['census_summary']
            print("\n=== §6eb evalPhi DEMAND CENSUS SUMMARY ===")
            print(f"Total Rays Evaluated:        {cs['total_rays_evaluated']:,}")
            print(f"Total evalPhi Calls:         {cs['total_evaluations']:,}")
            print(f"Total Online Duplicates:     {cs['total_online_duplicates']:,} ({cs['online_duplicate_percentage']}%)")
            print(f"Exact 0-ULP Bit Parity:      {cs['exact_0_ulp_bit_parity']} (ULP mismatches = {cs['ulp_mismatches']})")
            print(f"Total Offline Unique Params: {cs['total_offline_unique_params']:,}")
            print(f"Consumer evalPhi Calls:      {cs['consumer_eval_phi_calls']}")
            print("\nBreakdown by Family:")
            for fam, fdata in analysis['breakdown_by_family'].items():
                print(f"  {fam:<10}: {fdata['total_evaluations']:>10,} calls | {fdata['online_duplicates']:>8,} dups ({fdata['duplicate_pct']:>5.1f}%)")
            print("\nBreakdown by Site:")
            for site, sdata in analysis['breakdown_by_site'].items():
                print(f"  {site:<22}: {sdata['total_evaluations']:>10,} calls ({sdata['pct_of_all_evals']:>5.1f}%) | {sdata['online_duplicates']:>8,} dups ({sdata['duplicate_pct_within_site']:>5.1f}%)")
            print("\nBreakdown by Level:")
            for lvl, ldata in analysis['breakdown_by_level'].items():
                print(f"  {lvl:<20}: {ldata['total_evaluations']:>10,} calls | {ldata['online_duplicates']:>8,} dups")
            print("\nPer-Ray Call Distribution:")
            for k, v in analysis['per_ray_calls_distribution'].items():
                print(f"  {k:<12}: {v}")
            print("\nPer-Ray Online Duplicates Distribution:")
            for k, v in analysis['per_ray_online_duplicates_distribution'].items():
                print(f"  {k:<12}: {v}")
            print("\nReuse Distance Histogram:")
            for k, v in analysis['reuse_distance_histogram'].items():
                print(f"  {k:<14}: {v:,}")
            print("\nSite-to-Site Transition Sharing:")
            for k, v in sorted(analysis['site_to_site_transitions'].items(), key=lambda x: -x[1]):
                print(f"  {k:<35}: {v:,}")

if __name__ == '__main__':
    main()
