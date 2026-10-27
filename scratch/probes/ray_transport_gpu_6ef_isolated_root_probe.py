"""§6ef source-isolated outward-safe root candidate diagnostic; no premature shipping claim.
Antigravity / Gemini 2.5 Pro (Sparkly Guy) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27.
Modes0/1/2/3 retain Current/bulk/routed/terminal bisection. Mode4 refuses;
Mode5 is the isolated terminal SBI candidate with outward-safe directed ULP band bounds.
Changed-method differences are recorded; unchanged controls retain strict parity.
"""
import argparse, ast, hashlib, importlib.util, json, os, re, subprocess, sys
from pathlib import Path

def need(ok, message):
    if not ok: raise ValueError(message)
def load(root, name):
    s = importlib.util.spec_from_file_location(name, root/'scratch/probes'/f'{name}.py')
    m = importlib.util.module_from_spec(s); s.loader.exec_module(m); return m
def sha(b): return hashlib.sha256(b).hexdigest()

OUTWARD_SAFE_SBI_LOOP = r'''
  var l = a; var r = b; var fl = va; var fr = vb;
  var stuck_count = 0u; var last_side = 0u;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   let h = r - l;
   let s_bisect = l + 0.5 * h;
   if (s_bisect <= l || s_bisect >= r || h <= 0.0001) { break; }
   var s = s_bisect;
   let D = fr - fl;
   if (abs(D) > 1e-30 && stuck_count < 2u) {
    let w_sec = -fl / D;
    if (w_sec > 0.0 && w_sec < 1.0) {
     let w_clamped = clamp(w_sec, 0.125, 0.875);
     let s_cand = l + w_clamped * h;
     if (s_cand > l && s_cand < r) {
      s = s_cand;
     }
    }
   }
   if (s <= l || s >= r) {
    s = s_bisect;
    if (s <= l || s >= r) { break; }
   }
   let v = evalPhi(m, o, d, s) - c;
   if (abs(v) <= error) {
    let w = 2.0 * error / minSlope;
    let sub_val = s - w;
    var ls = sub_val;
    if (w > 0.0) {
     if (sub_val == 0.0) { ls = bitcast<f32>(0x80000001u); }
     else {
      let u = bitcast<u32>(sub_val);
      if (sub_val > 0.0) { ls = bitcast<f32>(u - 1u); }
      else { ls = bitcast<f32>(u + 1u); }
     }
    }
    let add_val = s + w;
    var rs = add_val;
    if (w > 0.0) {
     if (add_val == 0.0) { rs = bitcast<f32>(1u); }
     else {
      let u = bitcast<u32>(add_val);
      if (add_val > 0.0) { rs = bitcast<f32>(u + 1u); }
      else { rs = bitcast<f32>(u - 1u); }
     }
    }
    l = max(l, ls); r = min(r, rs); break;
   }
   if ((fl < 0.0 && v > 0.0) || (fl > 0.0 && v < 0.0)) {
    r = s; fr = v;
    if (last_side == 1u) { stuck_count += 1u; } else { stuck_count = 0u; }
    last_side = 1u;
   } else {
    l = s; fl = v;
    if (last_side == 2u) { stuck_count += 1u; } else { stuck_count = 0u; }
    last_side = 2u;
   }
  }
'''

def symbols(text, mapping):
    for old, new in sorted(mapping.items(), key=lambda pair: -len(pair[0])):
        text = re.sub(r'\b' + re.escape(old) + r'\b', new, text)
    return text

def generate(root, mutation='fixed', only_mode=None, strict_candidate=False):
    base = load(root, 'ray_transport_gpu_6dz_producer_coalescing'); cpp = base.controls_cpp(root, 'bound-stack-15')
    original = cpp; loop = OUTWARD_SAFE_SBI_LOOP
    shader_map = {'kS1ProducerWgsl': 'kSbiS1ProducerWgsl', 'kS2ProducerWgsl': 'kSbiS2ProducerWgsl', 'kFallbackProducerWgsl': 'kSbiGeneralProducerWgsl'}
    cp_map = {'cpS1Terminal': 'cpSbiS1', 'cpS2': 'cpSbiS2', 'cpFallback': 'cpSbiGeneral'}
    bg_map = {name: name + 'Sbi' for name in ['bgS1Terminal', 'bgS1TerminalForced', 'bgS2', 'bgS2Forced', 'bgFallback', 'bgFallbackForced']}
    mapping = dict(shader_map, **cp_map, **bg_map); clones = []
    for old, new in shader_map.items():
        m = re.search(r'static const char\* ' + old + r' = R"WGSL\((.*?)\)WGSL";', cpp, re.S); need(m is not None, 'missing original shader')
        sh = m[1]; pattern = r'var l = a; var r = b; var left = va;.*?(?=\n  putBandTagged\(l, r,)'
        hits = list(re.finditer(pattern, sh, re.S)); need(len(hits) == 1, 'root replacement must match once')
        start, end = hits[0].span(); changed = sh[:start] + loop.lstrip() + sh[end:]
        if mutation == 'terminal-missing' and old == 'kS1ProducerWgsl':
            hook = '    results[id] = vec4<f32>(L, T);\n    atomicAdd(&auditWrites[id * 2u + 1u], 1u);'
            need(changed.count(hook) == 1, 'missing terminal output mutation hook')
            changed = changed.replace(hook, '    if(id>1000000u){\n' + hook + '\n    }')
        clones.append('static const char* ' + new + ' = R"WGSL(' + changed + ')WGSL";\n')
    hook = '    auto cpS1Unfused = buildCompute(kS1UnfusedProducerWgsl, "s1ProducerMain");'
    need(cpp.count(hook) == 1, 'pipeline insertion hook'); cpp = cpp.replace(hook, '\n'.join(clones) + '\n' + hook)
    for old, new in cp_map.items():
        pattern = r'    auto ' + old + r' = buildCompute\([^;]+;'; m = re.search(pattern, cpp); need(m is not None, 'missing pipeline')
        cpp = cpp[:m.end()] + '\n' + symbols(m[0], mapping) + cpp[m.end():]
    for old, new in bg_map.items():
        m = re.search(r'    auto ' + old + r' = makeGroup\(.*?\n    \}\);', cpp, re.S); need(m is not None, 'missing bind group')
        cpp = cpp[:m.end()] + '\n' + symbols(m[0], mapping) + cpp[m.end():]
    # Candidate dispatch copies the exact non-coalesced control dispatch topology.
    start = cpp.index('        auto execute='); end = cpp.index('        J routeChecks=', start)
    execute = cpp[start:end]
    m = re.search(r'                else if\(mode==3\)\{.*?(?=                else if\(mode==4\))', execute, re.S); need(m is not None, 'missing frozen terminal branch')
    candidate = symbols(m[0].replace('mode==3', 'mode==5'), mapping)
    if mutation == 'no-reset':
        hook = 'wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);'; need(candidate.count(hook) == 1, 'candidate reset hook'); candidate = candidate.replace(hook, '// reset intentionally omitted')
    if mutation == 'fallback-missing':
        hook = 'dispatch(fallbackCP, fallbackBG, count);'; need(candidate.count(hook) == 1, 'candidate Current fallback hook'); candidate = candidate.replace(hook, '// actual Current fallback publication intentionally omitted')
    old_start = execute.index('                else if(mode==4)'); old_end = execute.index('\n            }\n            auto cmd=', old_start)
    execute = execute[:old_start] + '                else if(mode==4){throw std::runtime_error("Mode4 unused in6ef");}\n' + candidate.rstrip() + execute[old_end:]
    cpp = cpp[:start] + execute + cpp[end:]
    modes = [only_mode] if only_mode is not None else [2, 3, 5]
    need(all(m in [2, 3, 5] for m in modes), 'invalid6ef mode')
    cpp = cpp.replace('for(int reviewMode:{2,3,4,5}){', 'for(int reviewMode:{' + ','.join(map(str, modes)) + '}){ bool savedPackets=false;')
    # Record changed-method observations separately; strict admission remains on controls.
    gate = 'if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");'
    need(cpp.count(gate) == 1, 'missing strict original gate')
    new_gate = gate if strict_candidate else 'if(reviewMode!=5 && (wrong||packets))throw std::runtime_error("unchanged control differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");'
    cpp = cpp.replace(gate, new_gate)
    cpp = cpp.replace('std::cerr<<"ROUTED REVIEW mode="', 'std::cerr<<(reviewMode==5?"CANDIDATE OBSERVATION mode=":"STRICT CONTROL mode=")')
    cpp = cpp.replace('auto expected=rs[rh[i].subpanelOffset+k];\n                        if(s0!=std::bit_cast<uint32_t>(expected.s0)||s1!=std::bit_cast<uint32_t>(expected.s1))++packets;',
        'if(k < rh[i].subpanelCount){auto expected=rs[rh[i].subpanelOffset+k];\n                        if(s0!=std::bit_cast<uint32_t>(expected.s0)||s1!=std::bit_cast<uint32_t>(expected.s1))++packets;}else{++packets;}')
    # Capture actual candidate packets, including terminal endpoints, on first baseline.
    hook = '            routeChecks.push_back({{"mode",reviewMode}'
    need(cpp.count(hook) == 1, 'missing diagnostic result hook')
    extra = r'''            J packetRows=J::array();
            if(!savedPackets && normal && label=="pose-0" && rep==1){
                savedPackets=true;
                for(size_t i=0;i<count;++i){
                    J ss=J::array(),oo=J::array();const auto& h=hs[i];bool terminal=(h.overlapCount&0x40000000u)!=0;
                    if(!(h.overlapCount&0x80000000u)){
                        for(uint32_t k=0;k<h.subpanelCount;++k){
                            uint32_t s0,s1,member;
                            if(terminal){uint32_t at=2u*count+i*128u+2u*k;s0=writes[at];s1=writes[at+1];member=0u;}
                            else{const auto& s=subs[h.subpanelOffset+k];s0=std::bit_cast<uint32_t>(s.s0);s1=std::bit_cast<uint32_t>(s.s1);member=s.mediumIndex;}
                            float lo=std::bit_cast<float>(s0),hi=std::bit_cast<float>(s1);
                            if(!std::isfinite(lo)||!std::isfinite(hi)||hi<lo)throw std::runtime_error("invalid actual subpanel endpoints");
                            ss.push_back({s0,s1,member});
                        }
                        for(uint32_t k=0;k<(h.overlapCount&0x00ffffffu);++k){const auto& o=overs[h.overlapOffset+k];oo.push_back({std::bit_cast<uint32_t>(o.s0),std::bit_cast<uint32_t>(o.s1),o.activeMask});}
                    }
                    packetRows.push_back({i,h.overlapCount,ss,oo});
                }
            }
            double currentMaxRGB=0,currentMaxT=0;
            if(currentReview.size()!=count||bulkReview.size()!=count)throw std::runtime_error("comparison vectors not populated");
            for(size_t i=0;i<count;++i){currentMaxT=std::max(currentMaxT,std::abs(double(output[i].rgb_T[3])-double(currentReview[i].rgb_T[3])));for(int k=0;k<3;++k)currentMaxRGB=std::max(currentMaxRGB,std::abs(double(output[i].rgb_T[k])-double(currentReview[i].rgb_T[k])));}
'''.replace('\\"', '"')
    cpp = cpp.replace(hook, extra + hook)
    cpp = cpp.replace('routeChecks.push_back({{"mode",reviewMode}', 'routeChecks.push_back({{"math_qualified",false},{"strict_reference_parity",wrong==0&&packets==0},{"current_max_rgb_delta",currentMaxRGB},{"current_max_t_delta",currentMaxT},{"actual_packets",packetRows},{"mode",reviewMode}')
    cpp = cpp.replace('§6dz coalesced & uncoalesced routed correctness and ownership controls', '§6ef outward-safe SBI observation; modes0-3 frozen bisection, mode5 candidate; no premature shipping claim')
    # Prove reference shaders were not overwritten.
    original_blocks = dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";', original, re.S))
    blocks = dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";', cpp, re.S))
    need(all(blocks.get(n) == v for n, v in original_blocks.items()), 'an unchanged shader was modified')
    need('double maxDeltaRGB = 0.0' not in cpp, 'legacy empty-vector logger was inherited')
    return cpp

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--mutation', choices=['fixed', 'terminal-missing', 'fallback-missing', 'no-reset'], default='fixed')
    p.add_argument('--mode', type=int, choices=[2, 3, 5])
    p.add_argument('--strict-candidate', action='store_true')
    p.add_argument('--run', action='store_true')
    a = p.parse_args()
    root = a.root.resolve(); work = a.work.resolve()
    need(not work.exists() and root not in work.parents and work != root, 'fresh external work required')
    work.mkdir(parents=True, exist_ok=False)
    cpp = generate(root, a.mutation, a.mode, a.strict_candidate)
    source = work/'probe.cpp'; source.write_text(cpp)
    (work/'source.json').write_text(json.dumps({
      'experiment': '6ef-outward-safe-SBI',
      'cpp_sha256': sha(cpp.encode()),
      'mutation': a.mutation,
      'only_mode': a.mode,
      'strict_candidate': a.strict_candidate,
      'modes': {'0': 'Current', '1': 'unchanged bulk bisection', '2': 'unchanged routed bisection', '3': 'unchanged terminal bisection', '4': 'unused/refuses', '5': 'isolated terminal outward-safe SBI'}
    }, indent=2) + '\n')
    subprocess.run([sys.executable, '-B', str(root/'scratch/probes/ray_transport_probe_build.py'), '--root', str(root), '--build-root', str(a.build_root.resolve()), '--source', str(source), '--work', str(work/'build')], check=True)
    if not a.run: return
    wide = load(root, 'ray_transport_gpu_wide_stream'); zone = root/'saves/zones/Northern Veil/zone.ecform'; before = sha(zone.read_bytes())
    meta, rays, _ = wide.verify_case(root, Path('/private/tmp/earthcall-wide-case/manifest.json'), before)
    env = dict(os.environ, EC_POSE_RAYS=meta['rays'], EC_POSE_REFERENCE=meta['reference'], EC_POSE_CLOCK=str(rays['time']), EC_MATCH_REPEAT='1', EC_GPU_BUDGET='1024', EC_GPU_SUB_CAP='64', EC_GPU_OVER_CAP='16', EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None); env.pop('METAL_CAPTURE_ENABLED', None)
    with (work/'native.log').open('xb') as f:
        res = subprocess.run([str(work/'build/probe'), str(work/'review.json')], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT)
    need(sha(zone.read_bytes()) == before, 'fixture drift'); wide.verify_case(root, Path('/private/tmp/earthcall-wide-case/manifest.json'), before)
    if res.returncode: raise SystemExit(res.returncode)
    print('DIAGNOSTIC COMPLETE: unchanged controls strict; candidate differences recorded, not mathematical or performance qualification')

if __name__ == '__main__': main()
