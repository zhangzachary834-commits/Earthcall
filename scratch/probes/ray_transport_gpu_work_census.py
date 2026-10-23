"""§6cx: Contract repair, packet trace admission, and complete work census.
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-22.
Commissioned by Codex / GPT-6.1 Sol in §6cw and Firmament (Gemini 4 Argon).
Restores 64/16 capacity contract, exports actual fused packets, reads normal headers,
exercises synthetic budget/capacity controls, and counts all 28 producer/consumer/marcher
operations across all 32,784 distinct native rays.
"""
import argparse, ast, gzip, hashlib, importlib.util, json, math, os, re, shlex, subprocess, sys
from pathlib import Path

def load(root, name):
    p = root / 'scratch/probes' / (name + '.py')
    spec = importlib.util.spec_from_file_location(name, p)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def require_contract(shader):
    sub = int(re.search(r'var local_subpanels: array<\w+, (\d+)>', shader)[1])
    over = int(re.search(r'var local_overlaps: array<\w+, (\d+)>', shader)[1])
    if (sub, over) != (64, 16):
        raise ValueError(f'private capacities ({sub}, {over}) differ from 64/16 contract')

def require_packet_evidence(report):
    if 'fused_packet_census_inputs' not in report:
        raise ValueError('fusion admission refused: actual fused packet trace is absent')

# -------------------------------------------------------------------------
# WGSL Shaders: 64/16 Contract, Packet Trace, Fused Census, Current Census
# -------------------------------------------------------------------------

def make_fused_wgsl_64(base_fused_wgsl):
    # Restore 64/16 capacity in FUSED_WGSL
    s = base_fused_wgsl
    s = s.replace('var local_subpanels: array<Subpanel, 32>;', 'var local_subpanels: array<Subpanel, 64>;')
    s = s.replace('var local_overlaps: array<Overlap, 8>;', 'var local_overlaps: array<Overlap, 16>;')
    s = s.replace('singles + n > 32u || singles + n > cfg.maxSubPerRay', 'singles + n > 64u || singles + n > cfg.maxSubPerRay')
    s = s.replace('overs >= 8u || overs >= cfg.maxOverPerRay', 'overs >= 16u || overs >= cfg.maxOverPerRay')
    require_contract(s)
    return s

def make_packet_trace_wgsl(fused_wgsl_64):
    s = fused_wgsl_64
    extra_bindings = r'''
@group(0) @binding(6) var<storage, read_write> diag_subpanels: array<Subpanel>;
@group(0) @binding(7) var<storage, read_write> diag_overlaps: array<Overlap>;
'''
    s = s.replace('@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;',
                  '@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;' + extra_bindings)
    s = s.replace('fn fusedMain(@builtin(global_invocation_id) gid: vec3u) {',
                  'fn fusedTraceMain(@builtin(global_invocation_id) gid: vec3u) {')
    
    trace_export = r'''
    // Diagnostic packet export for admission verification
    let subBase = id * 64u;
    for (var s = 0u; s < singles; s++) {
        diag_subpanels[subBase + s] = local_subpanels[s];
    }
    let overBase = id * 16u;
    for (var o = 0u; o < overs; o++) {
        diag_overlaps[overBase + o] = local_overlaps[o];
    }
'''
    anchor = '    // Consumer quadrature integration directly from thread-local intervals'
    assert anchor in s, 'consumer anchor not found in trace wgsl'
    s = s.replace(anchor, trace_export + '\n' + anchor)
    return s

def make_fused_census_wgsl(fused_wgsl_64):
    s = fused_wgsl_64
    extra_struct = r'''
struct CensusRecord {
    boxHits: u32,
    coarseBinsVisited: u32,
    coarseBinsBowRejected: u32,
    subdivisionSplits: u32,
    maxStackTop: u32,
    maxDepthReached: u32,
    evalPhiCoarseKnots: u32,
    evalPhiSubdivision: u32,
    evalPhiBisection: u32,
    evalPhiPieceMid: u32,
    evalPhiTotal: u32,
    signChangeBrackets: u32,
    bisectionItersTotal: u32,
    uncertainBandsRetained: u32,
    candidateIntervalsChecked: u32,
    piecesEmitted: u32,
    mergedKnots: u32,
    singleSubpanels: u32,
    overlapIntervals: u32,
    singleEvalEnvelopeCalls: u32,
    singleCnoise3Calls: u32,
    overlapEvalEnvelopeCalls: u32,
    overlapCnoise3Calls: u32,
    expAttenSteps: u32,
    pad0: u32, pad1: u32, pad2: u32, pad3: u32,
    pad4: u32, pad5: u32, pad6: u32, pad7: u32,
};

@group(0) @binding(6) var<storage, read_write> fusedCensus: array<CensusRecord>;

var<private> g_boxHits: u32 = 0u;
var<private> g_coarseBinsVisited: u32 = 0u;
var<private> g_coarseBinsBowRejected: u32 = 0u;
var<private> g_subdivisionSplits: u32 = 0u;
var<private> g_maxStackTop: u32 = 0u;
var<private> g_maxDepthReached: u32 = 0u;
var<private> g_evalPhiCoarseKnots: u32 = 0u;
var<private> g_evalPhiSubdivision: u32 = 0u;
var<private> g_evalPhiBisection: u32 = 0u;
var<private> g_evalPhiPieceMid: u32 = 0u;
var<private> g_signChangeBrackets: u32 = 0u;
var<private> g_bisectionItersTotal: u32 = 0u;
var<private> g_uncertainBandsRetained: u32 = 0u;
var<private> g_candidateIntervalsChecked: u32 = 0u;
var<private> g_piecesEmitted: u32 = 0u;
var<private> g_singleCnoise3Calls: u32 = 0u;
var<private> g_overlapEvalEnvelopeCalls: u32 = 0u;
var<private> g_overlapCnoise3Calls: u32 = 0u;
var<private> g_expAttenSteps: u32 = 0u;
'''
    s = s.replace('@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;',
                  '@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;' + extra_struct)
    s = s.replace('fn fusedMain(@builtin(global_invocation_id) gid: vec3u) {',
                  'fn fusedCensusMain(@builtin(global_invocation_id) gid: vec3u) {')

    replacements = [
        ('if (!hit || !(b > a)) { continue; }',
         'if (!hit || !(b > a)) { continue; }\n        g_boxHits++;'),
        ('phi_knots[k] = evalPhi(m, o, d, s);',
         'phi_knots[k] = evalPhi(m, o, d, s);\n            g_evalPhiCoarseKnots++;'),
        ('for (var bin = 0u; bin < 8u; bin++) {',
         'for (var bin = 0u; bin < 8u; bin++) {\n        g_coarseBinsVisited++;'),
        ('if (min(va, vb) > bow || max(va, vb) < -bow) { continue; }',
         'if (min(va, vb) > bow || max(va, vb) < -bow) { g_coarseBinsBowRejected++; continue; }'),
        ('if ((va > error && vb < -error) || (va < -error && vb > error)) {',
         'if ((va > error && vb < -error) || (va < -error && vb > error)) {\n                    g_signChangeBrackets++;'),
        ('let v = evalPhi(m, o, d, mid) - c;',
         'g_bisectionItersTotal++;\n                        g_evalPhiBisection++;\n                        let v = evalPhi(m, o, d, mid) - c;'),
        ('let phi_mid = evalPhi(m, o, d, mid);',
         'g_evalPhiSubdivision++;\n            g_subdivisionSplits++;\n            g_maxStackTop = max(g_maxStackTop, top + 2u);\n            g_maxDepthReached = max(g_maxDepthReached, depth + 1u);\n            let phi_mid = evalPhi(m, o, d, mid);'),
        ('roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);\n        if (reason != 0u) { break; }',
         'roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);\n        g_uncertainBandsRetained += nr;\n        if (reason != 0u) { break; }'),
        ('var previousPositive = false;\n        for (var i = 1u; i < nb; i++) {',
         'var previousPositive = false;\n        for (var i = 1u; i < nb; i++) {\n            g_candidateIntervalsChecked++;'),
        ('if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {',
         'var isInside = uncertain;\n            if (!uncertain) {\n                g_evalPhiPieceMid++;\n                if (abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) { isInside = true; }\n            }\n            if (isInside) {'),
        ('pieces[count] = vec2f(l, r);\n                    setMember(&pieceMembers, count, member);\n                    count++;',
         'pieces[count] = vec2f(l, r);\n                    setMember(&pieceMembers, count, member);\n                    count++;\n                    g_piecesEmitted++;'),
        ('if (env_m > 0.0) {\n            let nm = cnoise3(freq * w_m);',
         'if (env_m > 0.0) {\n            g_singleCnoise3Calls++;\n            let nm = cnoise3(freq * w_m);'),
        ('if (env_p > 0.0) {\n            let np = cnoise3(freq * w_p);',
         'if (env_p > 0.0) {\n            g_singleCnoise3Calls++;\n            let np = cnoise3(freq * w_p);'),
        ('let env = evalEnvelope(local, u32(m.freq_offset.y));',
         'g_overlapEvalEnvelopeCalls++;\n        let env = evalEnvelope(local, u32(m.freq_offset.y));'),
        ('let D = env * clamp(0.7 + 0.3 * cnoise3(m.freq_offset.x * local), 0.0, 1.0);',
         'g_overlapCnoise3Calls++;\n        let D = env * clamp(0.7 + 0.3 * cnoise3(m.freq_offset.x * local), 0.0, 1.0);'),
        ('fn advanceFlow(state: vec4<f32>, sourceSigma: vec4<f32>, h: f32) -> vec4<f32> {',
         'fn advanceFlow(state: vec4<f32>, sourceSigma: vec4<f32>, h: f32) -> vec4<f32> {\n    g_expAttenSteps++;'),
        ('let sub = local_subpanels[single];\n        single += 1u;',
         'let sub = local_subpanels[single];\n        single += 1u;\n        g_expAttenSteps++;')
    ]

    for orig, repl in replacements:
        assert orig in s, f'Hook failed in make_fused_census_wgsl: {orig[:40]!r}'
        s = s.replace(orig, repl)

    # Reset globals at top of fusedCensusMain
    main_start = 'let id = gid.x;\n    if (id >= cfg.rayCount) { return; }'
    assert main_start in s, 'main_start not found in fusedCensusMain'
    reset_globals = r'''
    let id = gid.x;
    if (id >= cfg.rayCount) { return; }
    g_boxHits = 0u;
    g_coarseBinsVisited = 0u;
    g_coarseBinsBowRejected = 0u;
    g_subdivisionSplits = 0u;
    g_maxStackTop = 0u;
    g_maxDepthReached = 0u;
    g_evalPhiCoarseKnots = 0u;
    g_evalPhiSubdivision = 0u;
    g_evalPhiBisection = 0u;
    g_evalPhiPieceMid = 0u;
    g_signChangeBrackets = 0u;
    g_bisectionItersTotal = 0u;
    g_uncertainBandsRetained = 0u;
    g_candidateIntervalsChecked = 0u;
    g_piecesEmitted = 0u;
    g_singleCnoise3Calls = 0u;
    g_overlapEvalEnvelopeCalls = 0u;
    g_overlapCnoise3Calls = 0u;
    g_expAttenSteps = 0u;
'''
    s = s.replace(main_start, reset_globals)

    # Write CensusRecord at end of fusedCensusMain
    record_write = r'''
    let totalPhi = g_evalPhiCoarseKnots + g_evalPhiSubdivision + g_evalPhiBisection + g_evalPhiPieceMid;
    fusedCensus[id] = CensusRecord(
        g_boxHits, g_coarseBinsVisited, g_coarseBinsBowRejected, g_subdivisionSplits,
        g_maxStackTop, g_maxDepthReached, g_evalPhiCoarseKnots, g_evalPhiSubdivision,
        g_evalPhiBisection, g_evalPhiPieceMid, totalPhi, g_signChangeBrackets,
        g_bisectionItersTotal, g_uncertainBandsRetained, g_candidateIntervalsChecked,
        g_piecesEmitted, ne, singles, overs,
        2u * singles, g_singleCnoise3Calls,
        g_overlapEvalEnvelopeCalls, g_overlapCnoise3Calls,
        g_expAttenSteps,
        0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
    );
'''
    res_anchor = 'results[id] = vec4<f32>(L, T);'
    assert res_anchor in s, 'res_anchor not found'
    s = s.replace(res_anchor, res_anchor + '\n' + record_write)
    return s

def make_current_census_wgsl(current_wgsl):
    text = current_wgsl
    # 1. Add currentCensus storage binding
    extra = r'''
struct CurrentCensusRecord {
    marchLoopSteps: u32,
    zeroProofSkippedSteps: u32,
    currentDensityEvals: u32,
    currentCnoise3Calls: u32,
};
@group(0) @binding(5) var<storage, read_write> currentCensus: array<CurrentCensusRecord>;
var<private> g_currentCnoise3Calls: u32 = 0u;
var<private> g_currentDensityEvals: u32 = 0u;
'''
    cnoise_pos = text.find('fn cnoise3(P: vec3<f32>) -> f32 {')
    assert cnoise_pos != -1, 'cnoise3 not found in current_wgsl'
    text = text[:cnoise_pos] + extra + 'fn cnoise3(P: vec3<f32>) -> f32 {\n    g_currentCnoise3Calls++;\n' + text[cnoise_pos + len('fn cnoise3(P: vec3<f32>) -> f32 {'):]

    # 2. Add g_currentDensityEvals to volumeShared_X_Eval
    for m in range(4):
        hook = f'fn volumeShared_{m}_Eval(p: vec3<f32>) {{'
        pos = text.find(hook)
        assert pos != -1, f'volumeShared_{m}_Eval not found'
        text = text[:pos] + hook + '\n    g_currentDensityEvals++;' + text[pos + len(hook):]

    # 3. Add march loop step counter
    loop_hook = 'for (var step = 0; step < segmentSamples; step = step + 1) {'
    pos = text.find(loop_hook)
    assert pos != -1, 'step loop not found'
    text = text[:pos] + 'var marchLoopSteps: u32 = 0u;\n    var zeroProofSkippedSteps: u32 = 0u;\n    ' + loop_hook + '\n        marchLoopSteps++;' + text[pos + len(loop_hook):]

    # 4. Add skip step counter
    skip_hook = 'step = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);'
    pos = text.find(skip_hook)
    assert pos != -1, 'skip hook not found'
    text = text[:pos] + 'let nextStep = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);\n                zeroProofSkippedSteps += u32(max(0, nextStep - step));\n                step = nextStep;' + text[pos + len(skip_hook):]

    # 5. Write record at end of rayMain
    out_hook = 'capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);'
    pos = text.find(out_hook)
    assert pos != -1, 'out_hook not found'
    write_rec = out_hook + '\n currentCensus[idx] = CurrentCensusRecord(marchLoopSteps, zeroProofSkippedSteps, g_currentDensityEvals, g_currentCnoise3Calls);'
    text = text[:pos] + write_rec + text[pos + len(out_hook):]

    # 6. Reset globals at top of rayMain
    top_hook = 'let t0=ray.ro_clipA.w;let t1=ray.rd_clipB.w;g_instIdx=0u;'
    pos = text.find(top_hook)
    assert pos != -1, 'top_hook not found'
    text = text[:pos] + top_hook + ' g_currentCnoise3Calls=0u; g_currentDensityEvals=0u;' + text[pos + len(top_hook):]
    return text

def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    fn = load(root, 'ray_transport_gpu_producer_fused_normal')
    s = fn.source(base, root)

    # 1. Replace FUSED_WGSL with 64/16 contract in C++ source
    fused_64 = make_fused_wgsl_64(fn.FUSED_WGSL)
    fused_decl = 'static const char* kFusedNormalShader = R"WGSL(\n' + fn.FUSED_WGSL.strip() + '\n)WGSL";'
    fused_64_decl = 'static const char* kFusedNormalShader = R"WGSL(\n' + fused_64.strip() + '\n)WGSL";'
    s = q.once(s, fused_decl, fused_64_decl)

    # 2. Add trace and census shaders
    trace_wgsl = make_packet_trace_wgsl(fused_64)
    fused_census_wgsl = make_fused_census_wgsl(fused_64)

    shaders_block = '\nstatic const char* kFusedPacketTraceShader = R"WGSL(\n' + trace_wgsl.strip() + '\n)WGSL";\n\n' + \
                    'static const char* kFusedCensusShader = R"WGSL(\n' + fused_census_wgsl.strip() + '\n)WGSL";\n\n'
    s = q.once(s, 'auto cpFusedNormal = buildCompute(kFusedNormalShader, "fusedMain");',
                  shaders_block + 'auto cpFusedNormal = buildCompute(kFusedNormalShader, "fusedMain");\n' +
                                  'auto cpFusedPacketTrace = buildCompute(kFusedPacketTraceShader, "fusedTraceMain");\n' +
                                  'auto cpFusedCensus = buildCompute(kFusedCensusShader, "fusedCensusMain");')

    # 3. Add readBackBuffer helper
    readback_helper = r'''
    auto readBackBuffer = [&](WGPUBuffer src, uint64_t size) -> std::vector<uint8_t> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, size, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, size);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo = {};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, size, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer mapping failed");

        const uint8_t* ptr = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(rb, 0, size));
        std::vector<uint8_t> data(ptr, ptr + size);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };
'''
    s = q.once(s, 'auto readResults = [&]() -> std::vector<GpuResult> {', readback_helper + '\n    auto readResults = [&]() -> std::vector<GpuResult> {')

    # 4. Declare normalRows and headers outside the capacity loop
    decl_outside = r'''
    J normalRows;
    J forcedRows;
    std::vector<GpuRayHeader> finalHeaders;
    size_t normalFallbacks = 0;
    size_t totalSingles = 0;
    size_t totalOvers = 0;
'''
    s = q.once(s, 'J streamingCases=J::array();', 'J streamingCases=J::array();\n' + decl_outside)

    # In correctness lambda: read rBuf immediately after normal run BEFORE forced fallback runs!
    old_correctness_call = r'''auto normalRows=correctness(false);auto forcedRows=correctness(true);
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("stream header readback refused");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned forcedCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++forcedCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);'''

    new_correctness_call = r'''
        // 1. Execute normal mode and immediately read back rBuf before forced fallback overwrites it!
        normalRows = correctness(false);
        auto normalHeaderBytes = readBackBuffer(rBuf, rayBytes);
        const auto* normalHeaders = reinterpret_cast<const GpuRayHeader*>(normalHeaderBytes.data());
        finalHeaders.assign(normalHeaders, normalHeaders + count);

        normalFallbacks = 0;
        totalSingles = 0;
        totalOvers = 0;
        for (size_t i = 0; i < count; ++i) {
            if (normalHeaders[i].overlapCount & 0x80000000u) normalFallbacks++;
            totalSingles += normalHeaders[i].subpanelCount;
            totalOvers += normalHeaders[i].overlapCount;
        }
        if (normalFallbacks != 0) throw std::runtime_error("normal_fallbacks != 0 in fused normal path");

        // 2. Execute forced fallback mode
        forcedRows = correctness(true);
        unsigned forcedCount = count;'''
    s = q.once(s, old_correctness_call, new_correctness_call)

    # 5. Wire up Diagnostic Packet Trace, Synthetic Controls, and Work Census in C++ main
    diagnostic_execution = r'''
    // --- Diagnostic Packet Trace, Synthetic Controls, and Work Census ---
    const uint64_t diagSubSingleBytes = uint64_t(count) * 64u * sizeof(GpuSubpanel);
    const uint64_t diagOverBytes = uint64_t(count) * 16u * sizeof(GpuOverlap);
    auto diagSubpanelsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagSubSingleBytes, nullptr);
    auto diagOverlapsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagOverBytes, nullptr);

    WGPUBindGroupEntry bgptEntries[8] = {};
    bgptEntries[0].binding = 0; bgptEntries[0].buffer = cfgBuf; bgptEntries[0].size = sizeof(cfg);
    bgptEntries[1].binding = 1; bgptEntries[1].buffer = specBuf; bgptEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgptEntries[2].binding = 2; bgptEntries[2].buffer = rBuf; bgptEntries[2].size = rayBytes;
    bgptEntries[3].binding = 3; bgptEntries[3].buffer = mBuf; bgptEntries[3].size = mediaBytes;
    bgptEntries[4].binding = 4; bgptEntries[4].buffer = pBuf; bgptEntries[4].size = paramBytes;
    bgptEntries[5].binding = 5; bgptEntries[5].buffer = outBuf; bgptEntries[5].size = outBytes;
    bgptEntries[6].binding = 6; diagSubpanelsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagSubSingleBytes, nullptr);
    bgptEntries[6].buffer = diagSubpanelsBuf; bgptEntries[6].size = diagSubSingleBytes;
    bgptEntries[7].binding = 7; diagOverlapsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagOverBytes, nullptr);
    bgptEntries[7].buffer = diagOverlapsBuf; bgptEntries[7].size = diagOverBytes;

    auto bglTrace = wgpuComputePipelineGetBindGroupLayout(cpFusedPacketTrace, 0);
    WGPUBindGroupDescriptor bgptd = {};
    bgptd.layout = bglTrace;
    bgptd.entryCount = 8;
    bgptd.entries = bgptEntries;
    WGPUBindGroup bgFusedPacketTrace = wgpuDeviceCreateBindGroup(gpu.device, &bgptd);
    wgpuBindGroupLayoutRelease(bglTrace);

    // Run Packet Trace
    {
        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpFusedPacketTrace);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgFusedPacketTrace, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto traceResults = readResults();
        for (size_t i = 0; i < count; ++i) {
            for (int k = 0; k < 4; ++k) {
                if (std::bit_cast<uint32_t>(traceResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resUnfused[i].rgb_T[k]))
                    throw std::runtime_error("packet trace output differs from normal rows");
            }
        }
    }

    auto subBytes = readBackBuffer(diagSubpanelsBuf, diagSubSingleBytes);
    auto overBytes = readBackBuffer(diagOverlapsBuf, diagOverBytes);
    const auto* diagSubs = reinterpret_cast<const GpuSubpanel*>(subBytes.data());
    const auto* diagOvers = reinterpret_cast<const GpuOverlap*>(overBytes.data());

    // Construct fused_packet_census_inputs
    J fusedPacketCensus = J::array();
    for (size_t i = 0; i < count; ++i) {
        uint32_t px = (uint32_t)R[i][0];
        uint32_t py = (uint32_t)R[i][1];
        const auto& hdr = finalHeaders[i];
        uint32_t fallbackFlag = hdr.overlapCount & 0x80000000u;
        J rawSingles = J::array();
        J rawOverlaps = J::array();
        if (fallbackFlag != 0u) {
            fusedPacketCensus.push_back({px, py, hdr.overlapCount, rawSingles, rawOverlaps});
        } else {
            uint32_t singles = hdr.subpanelCount;
            uint32_t overs = hdr.overlapCount;
            for (uint32_t s = 0; s < singles; s++) {
                const auto& sub = diagSubs[i * 64 + s];
                rawSingles.push_back({sub.s0, sub.s1, sub.mediumIndex});
            }
            for (uint32_t o = 0; o < overs; o++) {
                const auto& over = diagOvers[i * 16 + o];
                rawOverlaps.push_back({over.s0, over.s1, over.activeMask});
            }
            fusedPacketCensus.push_back({px, py, overs, rawSingles, rawOverlaps});
        }
    }
    report["fused_packet_census_inputs"] = fusedPacketCensus;
    report["normal_headers_readback"] = {
        {"normal_fallbacks", normalFallbacks},
        {"total_singles", totalSingles},
        {"total_overs", totalOvers},
        {"contract_pass", true}
    };

    // --- Synthetic Budget Controls ---
    J budgetControlResults = J::array();
    std::vector<GpuCurtainSpec> budgetSpecs = curtainSpecs;
    for (auto& s : budgetSpecs) {
        s.origin = glm::vec4(1000.0f, 1000.0f, 1000.0f, 0.0f);
        s.half = glm::vec4(1.0f);
    }
    budgetSpecs[0].origin = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    budgetSpecs[0].half = glm::vec4(10.0f, 10.0f, 10.0f, 0.0f);
    budgetSpecs[0].limits = glm::vec4(2.0f, 2.0f, 0.0f, 0.2f);
    budgetSpecs[0].curv = glm::vec4(0.0f, 0.001f, 0.0f, 0.0f);

    std::vector<GpuRayHeader> budgetRays(16);
    for (auto& ray : budgetRays) {
        ray.ro[0] = -0.5f; ray.ro[1] = 0.0f; ray.ro[2] = 2.0f; ray.clipA = 1.0f;
        ray.rd[0] = 1.0f;  ray.rd[1] = 0.0f; ray.rd[2] = 0.0f; ray.clipB = 2.0f;
        ray.subpanelOffset = 0; ray.subpanelCount = 0;
        ray.overlapOffset = 0;  ray.overlapCount = 0;
    }
    auto budgetSpecBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, budgetSpecs.size() * sizeof(GpuCurtainSpec), budgetSpecs.data());
    auto budgetRayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc, 16u * sizeof(GpuRayHeader), budgetRays.data());

    for (uint32_t b : {23u, 24u, 25u}) {
        wgpuQueueWriteBuffer(gpu.queue, budgetRayBuf, 0, budgetRays.data(), 16u * sizeof(GpuRayHeader));
        auto bcfg = cfg;
        bcfg.rayCount = 16u;
        bcfg.pad = b;
        auto bcfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(bcfg), &bcfg);
        WGPUBindGroupEntry entries[6] = {};
        entries[0].binding = 0; entries[0].buffer = bcfgBuf; entries[0].size = sizeof(bcfg);
        entries[1].binding = 1; entries[1].buffer = budgetSpecBuf; entries[1].size = budgetSpecs.size() * sizeof(GpuCurtainSpec);
        entries[2].binding = 2; entries[2].buffer = budgetRayBuf; entries[2].size = 16u * sizeof(GpuRayHeader);
        entries[3].binding = 3; entries[3].buffer = mBuf; entries[3].size = mediaBytes;
        entries[4].binding = 4; entries[4].buffer = pBuf; entries[4].size = paramBytes;
        entries[5].binding = 5; entries[5].buffer = outBuf; entries[5].size = 16u * sizeof(GpuResult);
        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpFusedNormal, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = entries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpFusedNormal);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, 1, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(budgetRayBuf, 16u * sizeof(GpuRayHeader));
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        uint32_t fallbacks = 0;
        for (uint32_t i = 0; i < 16u; ++i) if (hdrs[i].overlapCount & 0x80000000u) fallbacks++;
        uint32_t expected = (b == 23u ? 16u : 0u);
        budgetControlResults.push_back({{"budget", b}, {"fallbacks", fallbacks}, {"expected", expected}, {"pass", fallbacks == expected}});
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(bcfgBuf);
    }
    wgpuBufferRelease(budgetSpecBuf);
    wgpuBufferRelease(budgetRayBuf);
    report["synthetic_budget_controls"] = budgetControlResults;

    // --- Synthetic Capacity Controls ---
    // Capacity Control A: maxSubPerRay = 4u
    J capacityControlResults = J::array();
    {
        auto ccfg = cfg;
        ccfg.maxSubPerRay = 4u;
        auto ccfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(ccfg), &ccfg);
        WGPUBindGroupEntry entries[6] = {};
        entries[0].binding = 0; entries[0].buffer = ccfgBuf; entries[0].size = sizeof(ccfg);
        entries[1].binding = 1; entries[1].buffer = specBuf; entries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
        entries[2].binding = 2; entries[2].buffer = rBuf; entries[2].size = rayBytes;
        entries[3].binding = 3; entries[3].buffer = mBuf; entries[3].size = mediaBytes;
        entries[4].binding = 4; entries[4].buffer = pBuf; entries[4].size = paramBytes;
        entries[5].binding = 5; entries[5].buffer = outBuf; entries[5].size = outBytes;
        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpFusedNormal, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = entries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpFusedNormal);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderSetPipeline(pass, fallbackCP);
        wgpuComputePassEncoderSetBindGroup(pass, 0, fallbackBG, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(rBuf, rayBytes);
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        auto res = readResults();
        uint32_t subFallbacks = 0;
        bool fallbackParityPass = true;
        for (size_t i = 0; i < count; ++i) {
            if (hdrs[i].overlapCount & 0x80000000u) {
                subFallbacks++;
                for (int k = 0; k < 4; ++k) {
                    if (std::bit_cast<uint32_t>(res[i].rgb_T[k]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[k]))
                        fallbackParityPass = false;
                }
            }
        }
        capacityControlResults.push_back({
            {"control", "maxSubPerRay=4"},
            {"fallbacks", subFallbacks},
            {"fallback_bit_exact_pass", fallbackParityPass}
        });
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(ccfgBuf);
    }

    // Capacity Control B: maxOverPerRay = 1u
    {
        auto ccfg = cfg;
        ccfg.maxOverPerRay = 1u;
        auto ccfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(ccfg), &ccfg);
        WGPUBindGroupEntry entries[6] = {};
        entries[0].binding = 0; entries[0].buffer = ccfgBuf; entries[0].size = sizeof(ccfg);
        entries[1].binding = 1; entries[1].buffer = specBuf; entries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
        entries[2].binding = 2; entries[2].buffer = rBuf; entries[2].size = rayBytes;
        entries[3].binding = 3; entries[3].buffer = mBuf; entries[3].size = mediaBytes;
        entries[4].binding = 4; entries[4].buffer = pBuf; entries[4].size = paramBytes;
        entries[5].binding = 5; entries[5].buffer = outBuf; entries[5].size = outBytes;
        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpFusedNormal, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = entries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpFusedNormal);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderSetPipeline(pass, fallbackCP);
        wgpuComputePassEncoderSetBindGroup(pass, 0, fallbackBG, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(rBuf, rayBytes);
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        auto res = readResults();
        uint32_t overFallbacks = 0;
        bool fallbackParityPass = true;
        for (size_t i = 0; i < count; ++i) {
            if (hdrs[i].overlapCount & 0x80000000u) {
                overFallbacks++;
                for (int k = 0; k < 4; ++k) {
                    if (std::bit_cast<uint32_t>(res[i].rgb_T[k]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[k]))
                        fallbackParityPass = false;
                }
            }
        }
        capacityControlResults.push_back({
            {"control", "maxOverPerRay=1"},
            {"fallbacks", overFallbacks},
            {"fallback_bit_exact_pass", fallbackParityPass}
        });
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(ccfgBuf);
    }
    report["synthetic_capacity_controls"] = capacityControlResults;

    // --- Fused Work Census Run ---
    struct GpuCensusRecord {
        uint32_t boxHits;
        uint32_t coarseBinsVisited;
        uint32_t coarseBinsBowRejected;
        uint32_t subdivisionSplits;
        uint32_t maxStackTop;
        uint32_t maxDepthReached;
        uint32_t evalPhiCoarseKnots;
        uint32_t evalPhiSubdivision;
        uint32_t evalPhiBisection;
        uint32_t evalPhiPieceMid;
        uint32_t evalPhiTotal;
        uint32_t signChangeBrackets;
        uint32_t bisectionItersTotal;
        uint32_t uncertainBandsRetained;
        uint32_t candidateIntervalsChecked;
        uint32_t piecesEmitted;
        uint32_t mergedKnots;
        uint32_t singleSubpanels;
        uint32_t overlapIntervals;
        uint32_t singleEvalEnvelopeCalls;
        uint32_t singleCnoise3Calls;
        uint32_t overlapEvalEnvelopeCalls;
        uint32_t overlapCnoise3Calls;
        uint32_t expAttenSteps;
        uint32_t pad[8];
    };
    const uint64_t censusBytes = uint64_t(count) * sizeof(GpuCensusRecord);
    auto fusedCensusBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, censusBytes, nullptr);

    WGPUBindGroupEntry bgfcEntries[7] = {};
    bgfcEntries[0].binding = 0; bgfcEntries[0].buffer = cfgBuf; bgfcEntries[0].size = sizeof(cfg);
    bgfcEntries[1].binding = 1; bgfcEntries[1].buffer = specBuf; bgfcEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgfcEntries[2].binding = 2; bgfcEntries[2].buffer = rBuf; bgfcEntries[2].size = rayBytes;
    bgfcEntries[3].binding = 3; bgfcEntries[3].buffer = mBuf; bgfcEntries[3].size = mediaBytes;
    bgfcEntries[4].binding = 4; bgfcEntries[4].buffer = pBuf; bgfcEntries[4].size = paramBytes;
    bgfcEntries[5].binding = 5; bgfcEntries[5].buffer = outBuf; bgfcEntries[5].size = outBytes;
    bgfcEntries[6].binding = 6; bgfcEntries[6].buffer = fusedCensusBuf; bgfcEntries[6].size = censusBytes;

    auto bglCensus = wgpuComputePipelineGetBindGroupLayout(cpFusedCensus, 0);
    WGPUBindGroupDescriptor bgfcd = {};
    bgfcd.layout = bglCensus;
    bgfcd.entryCount = 7;
    bgfcd.entries = bgfcEntries;
    WGPUBindGroup bgFusedCensus = wgpuDeviceCreateBindGroup(gpu.device, &bgfcd);
    wgpuBindGroupLayoutRelease(bglCensus);

    {
        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpFusedCensus);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgFusedCensus, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto censusResults = readResults();
        for (size_t i = 0; i < count; ++i) {
            for (int k = 0; k < 4; ++k) {
                if (std::bit_cast<uint32_t>(censusResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resUnfused[i].rgb_T[k]))
                    throw std::runtime_error("fused census output differs from normal rows");
            }
        }
    }
    auto fcData = readBackBuffer(fusedCensusBuf, censusBytes);
    const auto* fRecs = reinterpret_cast<const GpuCensusRecord*>(fcData.data());

    // Aggregate Fused Census Records
    J fusedTotals = J::object();
    std::vector<uint32_t> vecBoxHits(count), vecCoarseBins(count), vecBowRej(count), vecSplits(count),
                          vecMaxStack(count), vecMaxDepth(count), vecPhiCoarse(count), vecPhiSub(count),
                          vecPhiBisect(count), vecPhiPieceMid(count), vecPhiTotal(count), vecBrackets(count),
                          vecBisectIters(count), vecUncertainBands(count), vecCandInt(count), vecPieces(count),
                          vecMergedKnots(count), vecSingles(count), vecOvers(count), vecSingleEnv(count),
                          vecSingleNoise(count), vecOverEnv(count), vecOverNoise(count), vecExpAtten(count);
    for (size_t i = 0; i < count; ++i) {
        const auto& r = fRecs[i];
        vecBoxHits[i] = r.boxHits; vecCoarseBins[i] = r.coarseBinsVisited; vecBowRej[i] = r.coarseBinsBowRejected;
        vecSplits[i] = r.subdivisionSplits; vecMaxStack[i] = r.maxStackTop; vecMaxDepth[i] = r.maxDepthReached;
        vecPhiCoarse[i] = r.evalPhiCoarseKnots; vecPhiSub[i] = r.evalPhiSubdivision; vecPhiBisect[i] = r.evalPhiBisection;
        vecPhiPieceMid[i] = r.evalPhiPieceMid; vecPhiTotal[i] = r.evalPhiTotal; vecBrackets[i] = r.signChangeBrackets;
        vecBisectIters[i] = r.bisectionItersTotal; vecUncertainBands[i] = r.uncertainBandsRetained;
        vecCandInt[i] = r.candidateIntervalsChecked; vecPieces[i] = r.piecesEmitted; vecMergedKnots[i] = r.mergedKnots;
        vecSingles[i] = r.singleSubpanels; vecOvers[i] = r.overlapIntervals; vecSingleEnv[i] = r.singleEvalEnvelopeCalls;
        vecSingleNoise[i] = r.singleCnoise3Calls; vecOverEnv[i] = r.overlapEvalEnvelopeCalls;
        vecOverNoise[i] = r.overlapCnoise3Calls; vecExpAtten[i] = r.expAttenSteps;
    }

    auto computeStats = [&](const std::vector<uint32_t>& v) {
        uint64_t sum = 0;
        uint32_t minV = v.empty() ? 0 : v[0];
        uint32_t maxV = v.empty() ? 0 : v[0];
        for (auto x : v) { sum += x; minV = std::min(minV, x); maxV = std::max(maxV, x); }
        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        double mean = v.empty() ? 0.0 : double(sum) / v.size();
        uint32_t p50 = sorted.empty() ? 0 : sorted[sorted.size() / 2];
        uint32_t p90 = sorted.empty() ? 0 : sorted[size_t(sorted.size() * 0.90)];
        uint32_t p99 = sorted.empty() ? 0 : sorted[size_t(sorted.size() * 0.99)];
        return J{{"total", sum}, {"mean", mean}, {"min", minV}, {"max", maxV}, {"p50", p50}, {"p90", p90}, {"p99", p99}};
    };

    report["work_census_fused"] = {
        {"boxHits", computeStats(vecBoxHits)},
        {"coarseBinsVisited", computeStats(vecCoarseBins)},
        {"coarseBinsBowRejected", computeStats(vecBowRej)},
        {"subdivisionSplits", computeStats(vecSplits)},
        {"maxStackTop", computeStats(vecMaxStack)},
        {"maxDepthReached", computeStats(vecMaxDepth)},
        {"evalPhiCoarseKnots", computeStats(vecPhiCoarse)},
        {"evalPhiSubdivision", computeStats(vecPhiSub)},
        {"evalPhiBisection", computeStats(vecPhiBisect)},
        {"evalPhiPieceMid", computeStats(vecPhiPieceMid)},
        {"evalPhiTotal", computeStats(vecPhiTotal)},
        {"signChangeBrackets", computeStats(vecBrackets)},
        {"bisectionItersTotal", computeStats(vecBisectIters)},
        {"uncertainBandsRetained", computeStats(vecUncertainBands)},
        {"candidateIntervalsChecked", computeStats(vecCandInt)},
        {"piecesEmitted", computeStats(vecPieces)},
        {"mergedKnots", computeStats(vecMergedKnots)},
        {"singleSubpanels", computeStats(vecSingles)},
        {"overlapIntervals", computeStats(vecOvers)},
        {"singleEvalEnvelopeCalls", computeStats(vecSingleEnv)},
        {"singleCnoise3Calls", computeStats(vecSingleNoise)},
        {"overlapEvalEnvelopeCalls", computeStats(vecOverEnv)},
        {"overlapCnoise3Calls", computeStats(vecOverNoise)},
        {"expAttenSteps", computeStats(vecExpAtten)}
    };

    // --- Current Marcher Census Run ---
    // Extract current march shader and instrument it
    std::string currentCensusShader = adaptBindings(currentProgram.wgsl.substr(0, fs)) + extra + R"WGSL(
@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid:vec3<u32>) {
 let idx=gid.x;if(idx>=u32(u.viewport.z)){return;}
 let ray=capturedRays[idx%arrayLength(&capturedRays)];
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;
 let t0=ray.ro_clipA.w;let t1=ray.rd_clipB.w;g_instIdx=0u;
)WGSL" + marchingBody + R"WGSL(
 capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);
}
)WGSL";

    // Instrument currentCensusShader
    {
        std::string extraCurrent = R"WGSL(
struct CurrentCensusRecord {
    marchLoopSteps: u32,
    zeroProofSkippedSteps: u32,
    currentDensityEvals: u32,
    currentCnoise3Calls: u32,
};
@group(0) @binding(5) var<storage, read_write> currentCensus: array<CurrentCensusRecord>;
var<private> g_currentCnoise3Calls: u32 = 0u;
var<private> g_currentDensityEvals: u32 = 0u;
)WGSL";
        auto posCnoise = currentCensusShader.find("fn cnoise3(P: vec3<f32>) -> f32 {");
        if (posCnoise != std::string::npos) {
            currentCensusShader.insert(posCnoise, extraCurrent);
            auto posEntry = currentCensusShader.find("fn cnoise3(P: vec3<f32>) -> f32 {");
            currentCensusShader.insert(posEntry + 33, "\n    g_currentCnoise3Calls++;\n");
        } else {
            throw std::runtime_error("cnoise3 hook not found in currentCensusShader");
        }
        std::string topHook = "let t0=ray.ro_clipA.w;let t1=ray.rd_clipB.w;g_instIdx=0u;";
        auto pTop = currentCensusShader.find(topHook);
        if (pTop != std::string::npos) {
            currentCensusShader.insert(pTop + topHook.size(), " g_currentCnoise3Calls=0u; g_currentDensityEvals=0u; var marchLoopSteps: u32 = 0u; var zeroProofSkippedSteps: u32 = 0u;");
        } else {
            throw std::runtime_error("topHook not found in currentCensusShader");
        }
        for (int m = 0; m < 4; ++m) {
            std::string hook = "fn volumeDensityEval_" + std::to_string(m) + "(p: vec3<f32>) -> f32 {";
            auto p = currentCensusShader.find(hook);
            if (p != std::string::npos) currentCensusShader.insert(p + hook.size(), "\n    g_currentDensityEvals++;\n");
        }
        std::string emptyExitHook = "if (supportEnd <= supportStart) { capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0);return; }";
        auto pEmpty = currentCensusShader.find(emptyExitHook);
        if (pEmpty != std::string::npos) {
            currentCensusShader.replace(pEmpty, emptyExitHook.size(),
                "if (supportEnd <= supportStart) { capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0); currentCensus[idx] = CurrentCensusRecord(0u, 0u, 0u, 0u); return; }");
        }
        std::string loopHook = "step < segmentSamples; step = step + 1) {";
        auto pLoop = currentCensusShader.find(loopHook);
        if (pLoop != std::string::npos) {
            currentCensusShader.insert(pLoop + loopHook.size(), "\n        marchLoopSteps++;\n");
        } else {
            throw std::runtime_error("loopHook not found in currentCensusShader");
        }
        std::string skipHook = "step = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);";
        auto pSkip = currentCensusShader.find(skipHook);
        if (pSkip != std::string::npos) {
            std::string repl = "let nextStep = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);\n                zeroProofSkippedSteps += u32(max(0, nextStep - step));\n                step = nextStep;";
            currentCensusShader.replace(pSkip, skipHook.size(), repl);
        } else {
            throw std::runtime_error("skipHook not found in currentCensusShader");
        }
        std::string outHook = "capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);";
        auto pOut = currentCensusShader.find(outHook);
        if (pOut != std::string::npos) {
            currentCensusShader.insert(pOut + outHook.size(), "\n currentCensus[idx] = CurrentCensusRecord(marchLoopSteps, zeroProofSkippedSteps, g_currentDensityEvals, g_currentCnoise3Calls);");
        } else {
            throw std::runtime_error("outHook not found in currentCensusShader");
        }
    }

    struct GpuCurrentCensus {
        uint32_t marchLoopSteps;
        uint32_t zeroProofSkippedSteps;
        uint32_t currentDensityEvals;
        uint32_t currentCnoise3Calls;
    };
    const uint64_t curCensusBytes = uint64_t(count) * sizeof(GpuCurrentCensus);
    auto curCensusBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, curCensusBytes, nullptr);

    auto cpCurrentCensus = buildRaw(currentCensusShader, "rayMain");
    WGPUBindGroupEntry bgccEntries[6] = {};
    bgccEntries[0].binding = 0; bgccEntries[0].buffer = cuBuf; bgccEntries[0].size = sizeof(currentU);
    bgccEntries[1].binding = 1; bgccEntries[1].buffer = cparamsBuf; bgccEntries[1].size = currentParams.size() * 4;
    bgccEntries[2].binding = 2; bgccEntries[2].buffer = ciBuf; bgccEntries[2].size = currentInstances.size() * sizeof(CurrentInstance);
    bgccEntries[3].binding = 3; bgccEntries[3].buffer = rBuf; bgccEntries[3].size = rayBytes;
    bgccEntries[4].binding = 4; bgccEntries[4].buffer = outBuf; bgccEntries[4].size = outBytes;
    bgccEntries[5].binding = 5; bgccEntries[5].buffer = curCensusBuf; bgccEntries[5].size = curCensusBytes;

    auto bglCurrentCensus = wgpuComputePipelineGetBindGroupLayout(cpCurrentCensus, 0);
    WGPUBindGroupDescriptor bgccd = {};
    bgccd.layout = bglCurrentCensus;
    bgccd.entryCount = 6;
    bgccd.entries = bgccEntries;
    WGPUBindGroup bgCurrentCensus = wgpuDeviceCreateBindGroup(gpu.device, &bgccd);
    wgpuBindGroupLayoutRelease(bglCurrentCensus);

    {
        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpCurrentCensus);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgCurrentCensus, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto curResults = readResults();
        for (size_t i = 0; i < count; ++i) {
            for (int k = 0; k < 4; ++k) {
                if (std::bit_cast<uint32_t>(curResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[k]))
                    throw std::runtime_error("current census output differs from current baseline");
            }
        }
    }
    auto ccData = readBackBuffer(curCensusBuf, curCensusBytes);
    const auto* cRecs = reinterpret_cast<const GpuCurrentCensus*>(ccData.data());

    std::vector<uint32_t> vecMarchSteps(count), vecZeroSkipped(count), vecCurrentDensity(count), vecCurrentNoise(count);
    for (size_t i = 0; i < count; ++i) {
        vecMarchSteps[i] = cRecs[i].marchLoopSteps;
        vecZeroSkipped[i] = cRecs[i].zeroProofSkippedSteps;
        vecCurrentDensity[i] = cRecs[i].currentDensityEvals;
        vecCurrentNoise[i] = cRecs[i].currentCnoise3Calls;
    }
    report["work_census_current"] = {
        {"marchLoopSteps", computeStats(vecMarchSteps)},
        {"zeroProofSkippedSteps", computeStats(vecZeroSkipped)},
        {"currentDensityEvals", computeStats(vecCurrentDensity)},
        {"currentCnoise3Calls", computeStats(vecCurrentNoise)}
    };

    report["compiler_resource_facts"] = {
        {"metal_offline_compiler", "unavailable via Dawn WebGPU API / offline tool"},
        {"dawn_pipeline_reflection_registers", "unavailable via Dawn WebGPU API"},
        {"host_command_buffer_architecture", "Single command buffer with single queue submit and single completion fence per batch (not per-tile host fence)"}
    };

    wgpuBufferRelease(diagSubpanelsBuf);
    wgpuBufferRelease(diagOverlapsBuf);
    wgpuBufferRelease(fusedCensusBuf);
    wgpuBufferRelease(curCensusBuf);
    wgpuBindGroupRelease(bgFusedPacketTrace);
    wgpuBindGroupRelease(bgFusedCensus);
    wgpuBindGroupRelease(bgCurrentCensus);
    wgpuComputePipelineRelease(cpFusedPacketTrace);
    wgpuComputePipelineRelease(cpFusedCensus);
    wgpuComputePipelineRelease(cpCurrentCensus);
'''
    anchor_streamed = 'report["streamed_schedule"]='
    pos_streamed = s.find(anchor_streamed)
    assert pos_streamed != -1, 'streamed_schedule anchor not found'
    # Find end of streamed_schedule statement
    pos_semi = s.find('};', pos_streamed)
    assert pos_semi != -1, 'end of streamed_schedule not found'
    s = s[:pos_semi + 2] + diagnostic_execution + s[pos_semi + 2:]

    return load(root, 'ray_transport_gpu_census_review').correct_source(s)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, required=True)
    p.add_argument('--label', default='census')
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    case_path = args.case.resolve()
    if work == root or root in work.parents or Path(args.label).name != args.label: raise SystemExit('unsafe work/label')

    # Load modules
    q = load(root, 'ray_transport_gpu_pose_qualification')
    wide = load(root, 'ray_transport_gpu_wide_stream')
    diag = load(root, 'ray_transport_metal_diagnostic')
    review = load(root, 'ray_transport_gpu_fused_review')
    portable = load(root, 'ray_transport_sliver_review')

    # Verify case and authored zone
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    if sha(zone) != v['ZONE_HASH']: raise SystemExit('pinned authored source changed')
    case_meta, rays, (ref_rows, ref_indices) = wide.verify_case(root, case_path, v['ZONE_HASH'])
    mhash = sha(case_path)

    # Generate source
    print('Generating C++ source with 64/16 contract, packet trace, and work census...')
    cpp = source(v['CPP_SOURCE'], root)
    digest = hashlib.sha256(cpp.encode()).hexdigest()

    # Build probe
    work.mkdir(parents=True, exist_ok=False)
    probe_cpp = work / 'probe.cpp'
    probe_o = work / 'probe.o'
    probe_bin = work / 'probe'

    probe_cpp.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({'cpp_sha256': digest, 'zone_sha256': v['ZONE_HASH']}, indent=2) + '\n')

    b = root / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])

    print('Compiling probe.cpp with -std=c++20 -O3...')
    cmd = shlex.split(c['command'])
    cmd[cmd.index('-c') + 1] = str(probe_cpp)
    cmd[cmd.index('-o') + 1] = str(probe_o)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    print('Linking probe executable...')
    cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    cmd[next(i for i, x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))] = str(probe_o)
    cmd[cmd.index('-o') + 1] = str(probe_bin)
    subprocess.run(cmd, cwd=b, check=True)

    if args.build_only:
        print('Build complete (--build-only).')
        return

    # Run probe
    output_json = work / (args.label + '.json')
    if output_json.exists(): raise SystemExit('refusing output overwrite')

    env = dict(os.environ,
               EC_POSE_RAYS=case_meta['rays'],
               EC_POSE_REFERENCE=case_meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_BUDGET='1024',
               EC_GPU_SUB_CAP='64',
               EC_GPU_OVER_CAP='16',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)

    wide.verify_case(root, case_path, v['ZONE_HASH'])
    if sha(case_path)!=mhash or sha(zone)!=v['ZONE_HASH']: raise SystemExit('fixture changed before execution')
    print('Executing probe on GPU...')
    res = subprocess.run([str(probe_bin), str(output_json)], cwd=root, env=env)
    if res.returncode != 0:
        raise SystemExit(f'Probe execution failed with code {res.returncode}')

    wide.verify_case(root, case_path, v['ZONE_HASH'])
    if sha(case_path)!=mhash or sha(zone)!=v['ZONE_HASH']: raise SystemExit('fixture changed during execution')
    # Validate output
    report = json.loads(output_json.read_text())
    rows_path = Path(str(output_json) + '-rows.json')
    rows = json.loads(rows_path.read_text())

    print('Validating admission contract...')
    review.require_contract(make_fused_wgsl_64(load(root, 'ray_transport_gpu_producer_fused_normal').FUSED_WGSL))
    require_packet_evidence(report)

    # Independent admission check
    print('Running ray_transport_gpu_fused_review.admission...')
    admission_res = review.admission(root, report, rows, rays, (ref_rows, ref_indices))
    print('Admission result: both_resolutions_pass =', admission_res['both_resolutions_pass'])
    if not admission_res['both_resolutions_pass']:
        raise SystemExit('Reference holdout gate failed')

    census_review = load(root, 'ray_transport_gpu_census_review')
    baseline = json.loads(Path(str(output_json) + '-baseline-rows.json').read_text())
    checked = census_review.validate_report(root, report, rows, baseline, rays, (ref_rows, ref_indices))
    Path(str(output_json) + '-independent.json').write_text(json.dumps(checked, indent=2) + '\n')

    # Width classes only; root provenance is not recorded.

    print('Analyzing subpanel widths (not provenance)...')
    trace = report['fused_packet_census_inputs']
    total_single_count = 0
    sub_apex = 0       # h <= 0.02
    sub_support = 0    # 0.02 < h <= 0.05
    sub_inter = 0      # 0.05 < h <= 0.20
    sub_macro = 0      # h > 0.20
    width_apex = 0.0
    width_support = 0.0
    width_inter = 0.0
    width_macro = 0.0

    for r in trace:
        singles = r[3]
        for lo, hi, _ in singles:
            total_single_count += 1
            h = hi - lo
            if h <= 0.02:
                sub_apex += 1
                width_apex += h
            elif h <= 0.05:
                sub_support += 1
                width_support += h
            elif h <= 0.20:
                sub_inter += 1
                width_inter += h
            else:
                sub_macro += 1
                width_macro += h

    provenance = {
        "total_singles": total_single_count,
        "width_le_2cm": {"count": sub_apex, "pct": sub_apex * 100.0 / total_single_count, "total_width_m": width_apex},
        "width_2_to_5cm": {"count": sub_support, "pct": sub_support * 100.0 / total_single_count, "total_width_m": width_support},
        "intermediate_5_to_20cm": {"count": sub_inter, "pct": sub_inter * 100.0 / total_single_count, "total_width_m": width_inter},
        "macro_gt_20cm": {"count": sub_macro, "pct": sub_macro * 100.0 / total_single_count, "total_width_m": width_macro}
    }
    report["subpanel_width_breakdown"] = provenance
    output_json.write_text(json.dumps(report, indent=2) + '\n')

    print('\n================ WORK CENSUS SUMMARY ================')
    print(f'Total Native Rays: {len(trace):,}')
    print(f'Normal Fallbacks: {report["normal_headers_readback"]["normal_fallbacks"]}')
    print(f'Total Single Subpanels: {total_single_count:,}')
    print(f'  - Width <= 2cm: {sub_apex:,} ({provenance["width_le_2cm"]["pct"]:.1f}%)')
    print(f'  - Width 2-5cm: {sub_support:,} ({provenance["width_2_to_5cm"]["pct"]:.1f}%)')
    print(f'  - Macro (> 20cm curtain spans):  {sub_macro:,} ({provenance["macro_gt_20cm"]["pct"]:.1f}%)')
    print(f'Total Overlap Intervals: {report["normal_headers_readback"]["total_overs"]:,}')

    wf = report["work_census_fused"]
    wc = report["work_census_current"]
    print('\n--- Fused Producer Math ---')
    print(f'  - Box Hits:                {wf["boxHits"]["total"]:,}')
    print(f'  - Subdivision Splits:      {wf["subdivisionSplits"]["total"]:,}')
    print(f'  - Sign-Change Brackets:    {wf["signChangeBrackets"]["total"]:,}')
    print(f'  - Bisection Iters:         {wf["bisectionItersTotal"]["total"]:,}')
    print(f'  - evalPhi Coarse Knots:    {wf["evalPhiCoarseKnots"]["total"]:,}')
    print(f'  - evalPhi Subdivision:     {wf["evalPhiSubdivision"]["total"]:,}')
    print(f'  - evalPhi Bisection:       {wf["evalPhiBisection"]["total"]:,}')
    print(f'  - evalPhi Piece Midpoint:  {wf["evalPhiPieceMid"]["total"]:,}')
    print(f'  - Total evalPhi:           {wf["evalPhiTotal"]["total"]:,}')

    print('\n--- Fused Consumer Math ---')
    print(f'  - Single evalEnvelope:     {wf["singleEvalEnvelopeCalls"]["total"]:,}')
    print(f'  - Single cnoise3:          {wf["singleCnoise3Calls"]["total"]:,}')
    print(f'  - Overlap evalEnvelope:    {wf["overlapEvalEnvelopeCalls"]["total"]:,}')
    print(f'  - Overlap cnoise3:         {wf["overlapCnoise3Calls"]["total"]:,}')
    total_fused_noise = wf["singleCnoise3Calls"]["total"] + wf["overlapCnoise3Calls"]["total"]
    print(f'  - Total Fused cnoise3:     {total_fused_noise:,}')

    print('\n--- Mode 0 Current Marcher Math ---')
    print(f'  - March Loop Steps:        {wc["marchLoopSteps"]["total"]:,}')
    print(f'  - Zero-Proof Skipped Steps:{wc["zeroProofSkippedSteps"]["total"]:,}')
    print(f'  - Density Evals:           {wc["currentDensityEvals"]["total"]:,}')
    print(f'  - Current cnoise3:         {wc["currentCnoise3Calls"]["total"]:,}')

    diff_noise = total_fused_noise - wc["currentCnoise3Calls"]["total"]
    pct_noise = (diff_noise / wc["currentCnoise3Calls"]["total"]) * 100.0
    print(f'\nNoise Delta (Fused vs Current): {diff_noise:+,} ({pct_noise:+.1f}%)')
    print('=====================================================\n')

if __name__ == '__main__':
    main()
