"""§6cr: Fixed 4-way merge of ordered member streams and linear mask sweep.
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-22.
Commissioned by Sixth-one Sun (Codex / GPT-6.1 Sol) in §6cq review.
Replaces global O(ne^2) insertion sort and O(ne * count) repeated piece scans
with a fixed 4-way merge of the 4 monotonic disjoint member piece streams,
and a linear-advance active mask sweep.
Preserves bit-identical subpanel and overlap intervals, counts, and masks.
"""
import argparse, importlib.util, sys
from pathlib import Path

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / (name + '.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

NEW_SWEEP_WGSL = r''' var ne = 0u;
 var head = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 loop {
  if (ne >= totalKnots) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let pIdx = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  knots[ne] = bestVal;
  ne++;
  head[bestM]++;
 }
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }
  let mid = a + 0.5 * (b - a);
  var mask = 0u;
  for (var m = 0u; m < 4u; m++) {
   let start = memStart[m];
   let numPieces = memCount[m];
   var cur = pIdx[m];
   loop {
    if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
    cur++;
   }
   pIdx[m] = cur;
   if (cur < numPieces) {
    let p = pieces[start + cur];
    if (mid >= p.x && mid <= p.y) {
     mask |= (1u << m);
    }
   }
  }
  if (mask == 0u) { continue; }
  if (countOneBits(mask) == 1u) {
   let n = max(1u, u32(ceil((b - a) / 4.0)));
   if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
   let member = firstTrailingBit(mask);
   for (var k = 0u; k < n; k++) {
    let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
    let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
    subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
   }
  } else {
   if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
   overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
  }
 }
'''

def source(base, root):
    sys.path.insert(0, str(root / 'scratch/probes'))
    q = load(root, 'ray_transport_gpu_pose_qualification')
    guarded = load(root, 'ray_transport_gpu_producer_sliver_guarded')
    s = guarded.source(base, root)

    # 1. Add memStart and memCount tracking around the member loop
    old_mem_setup = ' var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);\n var knots: array<f32, 96>;'
    new_mem_setup = ' var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);\n var knots: array<f32, 96>;\n var memStart = vec4u(0u); var memCount = vec4u(0u);'
    s = q.once(s, old_mem_setup, new_mem_setup)

    old_mem_start = 'for (var member = 0u; member < 4u; member++) {\n  let m = curtains[member]; let o = ro - m.origin.xyz;'
    new_mem_start = 'for (var member = 0u; member < 4u; member++) {\n  memStart[member] = count;\n  let m = curtains[member]; let o = ro - m.origin.xyz;'
    s = q.once(s, old_mem_start, new_mem_start)

    old_mem_end = '  if (reason != 0u) { break; }\n }'
    new_mem_end = '  memCount[member] = count - memStart[member];\n  if (reason != 0u) { break; }\n }'
    s = q.once(s, old_mem_end, new_mem_end)

    # 2. Replace the sort and sweep block
    pos = s.find('var ne = 0u;\n for (var k = 0u; k < count; k++) { knots[ne] = pieces[k].x;')
    end = s.find('if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }', pos)
    assert pos != -1 and end != -1, 'sort and sweep block not found'
    old_sweep_block = s[pos:end]
    s = q.once(s, old_sweep_block, NEW_SWEEP_WGSL + ' ')

    # 3. Add report metadata
    meta = '''
    report["four_way_merge_and_sweep"] = {
        {"algorithm", "4-way merge of ordered member streams with linear-advance mask sweep"},
        {"global_insertion_sort_eliminated", true},
        {"repeated_full_piece_scan_eliminated", true},
        {"bit_identical_to_guarded_baseline_required", true},
        {"scope", "§6cr: Isolated event construction/sweep optimization under guarded policy"}
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
    q.independent_check = wide.independent_check
    q.main()

if __name__ == '__main__':
    main()
