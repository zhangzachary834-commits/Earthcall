"""§6ed experiment RETRACTED by source-isolated §6ee review.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
The original generator overwrote its reference and logged empty host vectors.
Its exact source is preserved in the §6ee evidence archive. Only its frozen
candidate root loop remains here so the corrected diagnostic can reproduce it.
No standalone timing or mathematical qualification is authorized by this file.
"""
FROZEN_SBI_LOOP = '  var l = a; var r = b; var fl = va; var fr = vb;\n  var stuck_count = 0u; var last_side = 0u;\n  for (var iter = 0u; iter < 32u; iter++) {\n   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;\n   let h = r - l;\n   let s_bisect = l + 0.5 * h;\n   if (s_bisect <= l || s_bisect >= r || h <= 0.0001) { break; }\n   var s = s_bisect;\n   let D = fr - fl;\n   if (abs(D) > 1e-30 && stuck_count < 2u) {\n    let w_sec = -fl / D;\n    if (w_sec > 0.0 && w_sec < 1.0) {\n     let w_clamped = clamp(w_sec, 0.125, 0.875);\n     let s_cand = l + w_clamped * h;\n     if (s_cand > l && s_cand < r) {\n      s = s_cand;\n     }\n    }\n   }\n   if (s <= l || s >= r) {\n    s = s_bisect;\n    if (s <= l || s >= r) { break; }\n   }\n   let v = evalPhi(m, o, d, s) - c;\n   if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, s - w); r = min(r, s + w); break; }\n   if ((fl < 0.0 && v > 0.0) || (fl > 0.0 && v < 0.0)) {\n    r = s; fr = v;\n    if (last_side == 1u) { stuck_count += 1u; } else { stuck_count = 0u; }\n    last_side = 1u;\n   } else {\n    l = s; fl = v;\n    if (last_side == 2u) { stuck_count += 1u; } else { stuck_count = 0u; }\n    last_side = 2u;\n   }\n  }'

def generate_guarded_root_cpp(*args,**kwargs):
    raise RuntimeError('Retracted §6ed generator: use ray_transport_gpu_6ee_isolated_root_probe.py; candidate remains unqualified')

if __name__=='__main__':
    raise SystemExit('Retracted §6ed: use the source-isolated §6ee diagnostic. See GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.md')
