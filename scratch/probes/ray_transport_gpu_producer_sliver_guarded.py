"""§6cq corrective numerical experiment; no production or general math admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Only short uncertainty pieces may join retained non-uncertain neighbors. Preserve explicit
 affine knots and both endpoints of any uncertainty band containing a zero-level search.
The original width-only §6cp remains reproducible, with its counters/claims superseded.
"""
import argparse, importlib.util, json
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / (name + '.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


POLICY_WGSL = r'''
fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
 previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
 if (protectedBoundary || previous.y != current.x) { return false; }
 let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
 return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}
'''

# Independent intent cases; these invoke the EXACT helper consumed by producerMain
# on the native GPU. Width alone must not erase a short authored positive piece.
POLICY_CASES = [
    ('leading support band', [0, .01], [.01, 2], True, False, False, False, True),
    ('trailing support band', [0, 2], [2, 2.01], False, True, True, False, True),
    ('short positive affine piece', [0, .01], [.01, 2], False, True, False, True, False),
    ('positive pieces without band provenance', [0, .01], [.01, 2], False, True, False, False, False),
    ('apex band start', [0, 2], [2, 2.01], False, True, True, True, False),
    ('apex band end', [0, .01], [.01, 2], True, False, False, True, False),
    ('disjoint short band', [0, .01], [.02, 2], True, False, False, False, False),
    ('two uncertainty pieces', [0, .01], [.01, .02], True, False, True, False, False),
    ('wide uncertainty piece', [0, 2], [2, 2.1], False, True, True, False, False),
]


def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    old = load(root, 'ray_transport_gpu_producer_sliver_merge')
    s = old.source(base, root)
    block = old.NEW_WGSL_BLOCK
    # A single bit per band tracks zero-level provenance through putBand's union.
    # All writes are through putBandTagged; producerMain reads the bits to protect
    # BOTH endpoints, including a band subsequently unioned with another level.
    helper = r'''
fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 putBand(lo, hi, bounds, nb, bands, nr, reason);
 if (*reason == 0u && isZeroLevel && *nr > 0u) { *zeroBands |= 1u << (*nr - 1u); }
}
'''
    block = block.replace('phi_knots: ptr<function, array<f32, 9>>) {',
                          'phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool) {')
    block = block.replace('putBand(', 'putBandTagged(').replace('bounds, nb, bands, nr, reason);', 'bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);')
    block = q.once(block, 'var bands: array<vec2f, 32>; var nr = 0u;',
                   'var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;')
    for level, flag in [('-m.limits.w', 'false'), ('0.0', 'true'), ('m.limits.w', 'false')]:
        before = f'roots(m, o, d, a, b, {level}, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots);'
        block = q.once(block, before, before[:-2] + f', &zeroBands, {flag});')
    block = q.once(block, '  let memberStart = count;',
        '  let memberStart = count; var previousShortBand = false; var previousPositive = false;')
    before = '''   let mid = l + 0.5 * (r - l); var uncertain = false;
   for (var k = 0u; k < nr; k++) { uncertain = uncertain || (l < bands[k].y && r > bands[k].x); }'''
    after = '''   let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
   // Reuse exactly the affine-knot arithmetic used when inserting knots above.
   if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
   if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
   for (var k = 0u; k < nr; k++) {
    uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
    if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
   }'''
    block = q.once(block, before, after)
    block = q.once(block, '''    if (count > memberStart && pieces[count - 1u].y == l &&
        ((l - pieces[count - 1u].x) <= 0.02 || (r - l) <= 0.02)) {''',
        '''    if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary)) {''')
    block = q.once(block, '     pieces[count - 1u].y = r;',
        '     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;')
    block = q.once(block, '     count++;',
        '     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;')
    s = q.once(s, old.NEW_WGSL_BLOCK, POLICY_WGSL + helper + block)
    # Native helper controls are outside timing, on isolated scratch output.
    values = []
    for i, (name, a, b, ps, pp, cu, protected, expected) in enumerate(POLICY_CASES):
        bo = lambda x: 'true' if x else 'false'
        vec = lambda v: 'vec2f(' + ','.join(str(float(x)) for x in v) + ')'
        values.append(f'if (id == {i}u) {{ accepted = mayAbsorb({vec(a)}, {vec(b)}, {bo(ps)}, {bo(pp)}, {bo(cu)}, {bo(protected)}); }}')
    shader = POLICY_WGSL + '\n@group(0) @binding(0) var<storage,read_write> output: array<vec4f>;\n@compute @workgroup_size(64) fn policyMain(@builtin(global_invocation_id) gid: vec3u) { let id = gid.x; if (id >= ' + str(len(POLICY_CASES)) + 'u) { return; } var accepted = false;\n' + '\n'.join(values) + '\noutput[id] = vec4f(select(0.0, 1.0, accepted)); }'
    expected = ','.join('1.0f' if c[-1] else '0.0f' for c in POLICY_CASES)
    cpp = r'''
    // Native semantic controls invoke the producer's shared mayAbsorb helper.
    J policyControls=J::array();
    {
        const char* code=POLICY_SHADER;
        wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL src={};src.chain.sType=WGPUSType_ShaderSourceWGSL;src.code=wgpu::Device::str(code);
        WGPUShaderModuleDescriptor smd={};smd.nextInChain=&src.chain;
        auto sm=wgpuDeviceCreateShaderModule(gpu.device,&smd);
        WGPUComputePipelineDescriptor pd={};pd.compute.module=sm;pd.compute.entryPoint=wgpu::Device::str("policyMain");
        auto cp=wgpuDeviceCreateComputePipeline(gpu.device,&pd);
        PopContext check;WGPUPopErrorScopeCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onPopError;cb.userdata1=&check;
        wgpuDevicePopErrorScope(gpu.device,cb);while(!check.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(check.type!=WGPUErrorType_NoError||!cp)throw std::runtime_error("guarded policy shader refused");
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);
        WGPUBindGroupEntry entry={};entry.binding=0;entry.buffer=outBuf;entry.size=outBytes;
        WGPUBindGroupDescriptor bd={};bd.layout=layout;bd.entryCount=1;bd.entries=&entry;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&bd);
        std::vector<GpuResult> poison(executionRays);for(auto& r:poison)for(float& v:r.rgb_T)v=NAN;
        wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);
        runPass(cp,bg);auto got=readResults();const float expected[]={EXPECTED};
        for(size_t i=0;i<std::size(expected);++i){
            for(float v:got[i].rgb_T)if(v!=expected[i])throw std::runtime_error("native protected-boundary policy failed");
            policyControls.push_back({{"case",i},{"expected",expected[i]},{"observed",got[i].rgb_T[0]},{"pass",true}});
        }
        wgpuBindGroupRelease(bg);wgpuBindGroupLayoutRelease(layout);wgpuComputePipelineRelease(cp);wgpuShaderModuleRelease(sm);
    }
'''.replace('POLICY_SHADER', 'R"POLICY(' + shader + ')POLICY"').replace('EXPECTED', expected)
    s = q.once(s, '    // Independent closed-form witnesses.', cpp + '\n    // Independent closed-form witnesses.')
    meta = '''
    report["guarded_sliver_policy"]={{"numerical_candidate",true},{"requires_short_uncertainty_provenance",true},
        {"affine_knot_boundaries_protected",true},{"zero_level_band_endpoints_protected",true},
        {"policy_native_controls",policyControls},{"mode_names",{{"0","current"},{"1","bulk connected"},{"2","bounded streamed connected"}}},
        {"shipping_performance_qualified",false}};
    report["guarded_sliver_policy"]["curtain_bindings"]=J::array();
    for(const auto& spec:curtainSpecs)report["guarded_sliver_policy"]["curtain_bindings"].push_back({
        {"origin",{spec.origin.x,spec.origin.y,spec.origin.z}},
        {"limits",{spec.limits.x,spec.limits.y,spec.limits.z,spec.limits.w}}});
    report["producer_sliver_merge"]["scope"]="Historical width-only §6cp replaced by guarded numerical policy; unchanged threshold/accuracy/fallback";
    report["producer_sliver_merge"].erase("private_memory_bytes_per_thread");
'''
    return q.once(s, '    std::cout << report.dump(2) << std::endl;', meta + '\n    std::cout << report.dump(2) << std::endl;')


def main():
    p = argparse.ArgumentParser(add_help=False);p.add_argument('--root', type=Path, required=True)
    args, _ = p.parse_known_args();root = args.root.resolve()
    q = load(root, 'ray_transport_gpu_pose_qualification');wide = load(root, 'ray_transport_gpu_wide_stream')
    q.source = lambda base, r: source(base, r)
    q.verify_case = lambda r, p, z: wide.verify_case(r, p, z)
    q.independent_check = wide.independent_check
    q.main()


if __name__ == '__main__':
    main()
