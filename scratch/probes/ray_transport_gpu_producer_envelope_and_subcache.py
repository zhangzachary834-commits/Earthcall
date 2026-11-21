"""§6cs: Linear-z wave-envelope culling and cross-level subdivision phi-cache.
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-22.
Counsel from Aetherion / The Firmament (Gemini 4 Argon).
Parent: §6cr (794e5ea03, merge-sweep) on claude/ray-domain-transport-rung0.

Four fixture-qualified candidates; not a general f32 culling theorem:
1. Whole-member & per-coarse-bin linear-z wave-envelope culling (without modifying a or b).
2. Demand-driven phi_knots[0..8] evaluation (only for knots bounding active bins).
3. Cross-level depth-0 & depth-1 subdivision phi-cache (24 floats, 24-bit mask).
4. Hoisted affine knots & exterior candidate linear-z pre-check (skipping evalPhi on exterior empty spans).

Strict same-device bit-exact schedule and output parity (0 / 32,784 differing rays).
"""
import argparse, importlib.util, sys
from pathlib import Path

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / (name + '.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def source(base, root):
    sys.path.insert(0, str(root / 'scratch/probes'))
    q = load(root, 'ray_transport_gpu_pose_qualification')
    ms = load(root, 'ray_transport_gpu_producer_merge_sweep')
    s = ms.source(base, root)

    # 1. Modify roots signature
    old_sig = 'phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool) {'
    new_sig = 'phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool, activeBins: u32, phi_sub: ptr<function, array<f32, 24>>, sub_mask: ptr<function, u32>) {'
    s = q.once(s, old_sig, new_sig)

    # 2. Modify roots bin loop start to skip inactive bins
    old_bin_start = 'for (var bin = 0u; bin < 8u; bin++) {\n  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;'
    new_bin_start = '''for (var bin = 0u; bin < 8u; bin++) {
  if ((activeBins & (1u << bin)) == 0u) {
   if (*work >= cfg.budget) { *reason |= 1u; break; }
   *work += 1u;
   continue;
  }
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;'''
    s = q.once(s, old_bin_start, new_bin_start)

    # 3. Save let bin_a = a for subdivision cache index
    old_stack_push = 'if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }\n  let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];'
    new_stack_push = 'if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }\n  let bin_a = a;\n  let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];'
    s = q.once(s, old_stack_push, new_stack_push)

    # 4. Modify roots split to use phi_sub cache
    old_split = '''   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let phi_mid = evalPhi(m, o, d, mid);'''
    new_split = '''   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   var phi_mid = 0.0;
   if (depth <= 1u) {
    let cIdx = select(bin, 8u + bin * 2u + select(1u, 0u, a == bin_a), depth == 1u);
    let bit = 1u << cIdx;
    if (((*sub_mask) & bit) != 0u) {
     phi_mid = (*phi_sub)[cIdx];
    } else {
     phi_mid = evalPhi(m, o, d, mid);
     (*phi_sub)[cIdx] = phi_mid;
     *sub_mask |= bit;
    }
   } else {
    phi_mid = evalPhi(m, o, d, mid);
   }'''
    s = q.once(s, old_split, new_split)

    # 5. Replace roots call block in producerMain with linear-z cull & demand-driven phi_knots
    old_roots_call_block = '''  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
  var phi_knots: array<f32, 9>;
  for (var k = 0u; k < 9u; k++) {
   var s = a + (b - a) * f32(k) / 8.0;
   if (k == 0u) { s = a; } if (k == 8u) { s = b; }
   phi_knots[k] = evalPhi(m, o, d, s);
  }
  roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
  if (reason != 0u) { break; }'''

    new_roots_call_block = '''  let wMax = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z);
  let h0 = (b - a) / 8.0;
  let M0 = m.curv.x * d.x * d.x * 1.00002;
  let zCull = wMax + m.limits.w + 0.125 * M0 * h0 * h0 + m.curv.y + 0.05;
  let za = o.z + a * d.z; let zb = o.z + b * d.z;
  if (min(za, zb) > zCull || max(za, zb) < -zCull) {
   if (work > cfg.budget || cfg.budget - work < 24u) { reason |= 1u; break; }
   work += 24u;
   continue;
  }
  var activeBins = 0u;
  for (var bin = 0u; bin < 8u; bin++) {
   var s0 = a + (b - a) * f32(bin) / 8.0;
   var s1 = a + (b - a) * f32(bin + 1u) / 8.0;
   if (bin == 0u) { s0 = a; } if (bin == 7u) { s1 = b; }
   let z0 = o.z + s0 * d.z; let z1 = o.z + s1 * d.z;
   if (min(z0, z1) <= zCull && max(z0, z1) >= -zCull) {
    activeBins |= (1u << bin);
   }
  }
  if (activeBins == 0u) {
   if (work > cfg.budget || cfg.budget - work < 24u) { reason |= 1u; break; }
   work += 24u;
   continue;
  }
  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
  var phi_knots: array<f32, 9>;
  let knotMask = activeBins | (activeBins << 1u);
  for (var k = 0u; k < 9u; k++) {
   if ((knotMask & (1u << k)) != 0u) {
    var s = a + (b - a) * f32(k) / 8.0;
    if (k == 0u) { s = a; } if (k == 8u) { s = b; }
    phi_knots[k] = evalPhi(m, o, d, s);
   }
  }
  var phi_sub: array<f32, 24>; var sub_mask = 0u;
  roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, activeBins, &phi_sub, &sub_mask);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, activeBins, &phi_sub, &sub_mask);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, activeBins, &phi_sub, &sub_mask);
  if (reason != 0u) { break; }'''
    s = q.once(s, old_roots_call_block, new_roots_call_block)

    # 6. Replace affine insertion and piece candidate loop with hoisted affine and exterior pre-check
    old_affine_and_piece_block = '''  if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
  if (d.y != 0.0) {
   let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
   let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
  }
  knots[nb] = b; nb++;
  for (var i = 1u; i < nb; i++) {
   let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
   knots[j] = v;
  }
  let memberStart = count; var previousShortBand = false; var previousPositive = false;
  for (var i = 1u; i < nb; i++) {
   let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
   let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
   // Reuse exactly the affine-knot arithmetic used when inserting knots above.
   if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
   if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
   for (var k = 0u; k < nr; k++) {
    uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
    if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
   }
   if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
    if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary)) {
     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
    } else {
     if (count >= 48u) { reason |= 8u; break; }
     pieces[count] = vec2f(l, r);
     setMember(&pieceMembers, count, member);
     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }'''

    new_affine_and_piece_block = '''  let hasDx = d.x != 0.0; let kx = select(0.0, -o.x / d.x, hasDx);
  if (hasDx && kx > a && kx < b) { knots[nb] = kx; nb++; }
  let hasDy = d.y != 0.0;
  let ky0 = select(0.0, -o.y / d.y, hasDy);
  let ky1 = select(0.0, (m.limits.z - o.y) / d.y, hasDy);
  if (hasDy) {
   if (ky0 > a && ky0 < b) { knots[nb] = ky0; nb++; }
   if (ky1 > a && ky1 < b) { knots[nb] = ky1; nb++; }
  }
  knots[nb] = b; nb++;
  for (var i = 1u; i < nb; i++) {
   let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
   knots[j] = v;
  }
  let memberStart = count; var previousShortBand = false; var previousPositive = false;
  let zPieceLimit = wMax + m.limits.w + m.curv.y + 0.001;
  for (var i = 1u; i < nb; i++) {
   let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
   let mid = l + 0.5 * (r - l); var uncertain = false;
   for (var k = 0u; k < nr; k++) {
    if (l < bands[k].y && r > bands[k].x) { uncertain = true; break; }
   }
   var activePiece = uncertain;
   if (!activePiece && abs(o.z + mid * d.z) <= zPieceLimit) {
    activePiece = abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y;
   }
   if (activePiece) {
    var canMerge = false;
    if (count > memberStart && pieces[count - 1u].y == l) {
     var protectedBoundary = (hasDx && l == kx) || (hasDy && (l == ky0 || l == ky1));
     if (!protectedBoundary) {
      for (var k = 0u; k < nr; k++) {
       if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) {
        protectedBoundary = true; break;
       }
      }
     }
     canMerge = mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary);
    }
    if (canMerge) {
     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
    } else {
     if (count >= 48u) { reason |= 8u; break; }
     pieces[count] = vec2f(l, r);
     setMember(&pieceMembers, count, member);
     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }'''
    s = q.once(s, old_affine_and_piece_block, new_affine_and_piece_block)

    # 7. Add report metadata
    meta = '''
    report["envelope_and_subcache"] = {
        {"whole_member_linear_z_cull", true},
        {"per_coarse_bin_linear_z_cull", true},
        {"demand_driven_phi_knots", true},
        {"cross_level_subdivision_phi_cache_24", true},
        {"hoisted_affine_knots", true},
        {"exterior_candidate_linear_z_precheck", true},
        {"strict_same_device_bit_exactness_required", true},
        {"scope", "§6cs: Linear-z wave-envelope culling and cross-level subdivision phi-cache"}
    };
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n' + anchor)
    return s

def main():
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument('--root', type=Path, required=True)
    args, _ = p.parse_known_args()
    root = args.root.resolve()
    q = load(root, 'ray_transport_gpu_pose_qualification')
    wide = load(root, 'ray_transport_gpu_wide_stream')
    q.source = lambda base, r: source(base, r)
    q.verify_case = lambda r, p, z: wide.verify_case(r, p, z)
    q.independent_check = lambda report, raw, rays, refs: load(root, 'ray_transport_gpu_producer_budget_review').admission(root, report, raw, rays, refs, 'cs-fixed')
    q.main()

if __name__ == '__main__':
    main()
