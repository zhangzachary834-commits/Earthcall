# Proof-grid generality witness

**Mission (Zach, 2026-10-20):** "lets do that in another mission and keep the current plan." He asked whether the ray-transport optimizations generalize to other authored fields of the same mathematical kinds, or whether the renderer knows what a Northern Veil is.

**What is already true.** No production code branches on a field's name or zone. The only two Northern Veil mentions in `LineRestriction.*`/`ScalarForm.cpp` are comments citing evidence. All of the following work on the line-restricted mathematics (`LineAlgebraicNode::Kind`: polynomial, + − ×, sqrt, abs, min/max, clamp, select, transcendental, classic Perlin noise):
- the exact proof reuse of plan §6bc (evaluator reuse, the `constantUnder` and signed-range memos, the six-endpoint Perlin memo);
- the hierarchical `RegionCertificate::proveGrid` of §6bd.

**What is unproved.** Every measurement so far is on the Northern Veil, whose clamps make 96–98% of cells exactly zero.

## Do
1. **A second real authored field with exact-zero support.** Find one in another zone: clamp, max(0, …), piecewise cutoff or bounded support. Read the save read-only. Measure `proveGrid` against the flat grid: cost, occupancy, and a bit-identical leaf / zero-agreement check like the Veil witness in `ontomath_veil_transport_feasibility_test`.
2. **A worst case: a field that is small but never exactly zero** (e.g. exp(−r²), or noise + positive offset). Then every cell is occupied, and the hierarchy proves all leaves *plus* every containing box, up to roughly twice as many proofs, with the containing boxes slower. Measure it.
3. **If the worst case loses, add an exact guard.** For example, stop descending when a subtree's proofs keep failing to prove zero, and certify its leaves directly. The guard must stay bit-identical for occupied leaves. **Never** skip "negligible" cells: that needs an authored error budget, which the exact-zero contract does not grant.
4. **A noise-free field.** It shows the §6bc gains (which were measured Perlin-heavy) apart from the Perlin memo.

## Pitfalls
- Saves are sacred: read only, restore any tracked save a test rewrites.
- Release measurements are the shipping basis; Debug is ~10× slower (§6bc).
- No subagents, no parallel full ctest; run named targets one at a time.

*Opened by Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-20.*
