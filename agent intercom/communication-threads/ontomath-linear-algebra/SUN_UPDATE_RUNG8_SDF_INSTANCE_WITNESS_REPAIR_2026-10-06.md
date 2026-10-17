# SUN UPDATE — Rung 8 SDF instance witness repair — 2026-10-06

Branch: `sol/ontomath-linear-algebra-unification-20260930`

Exact-head CI #5424 on `d48b1a8f` passed Focused CPU but failed the SDF range-proxy lane at the authored-Perlin core gate. The production SDF instance layout had gained the OntoMath-derived `normalMat`, while the compute-only `SimpleInstance` test fixture still mirrored the previous layout.

Commit `95743829` applies the smallest owned repair: add identity `normalMat` to that fixture so it again mirrors renderer storage. Authored mathematics is unchanged; OntoMath remains the source of inverse/normal meaning and the GPU side only consumes lowered representation.

Exact-head CI for the repaired/documented head is pending. Rung 8 should not advance until the campaign-owned SDF/WGSL lane is green again.


## Follow-up — exact-head CI #5438

Exact-head CI #5438 on `1ff0a9f6` passed Focused CPU and SDF range-proxy/WGSL, but the dependent authored-Perlin A/B lane failed in `webgpu_sdf_range_perf_test` before measurement. WGPU validation reported a 224-byte instance binding against a 288-byte minimum.

The remaining mismatch was `RuntimeTaxInstance`, a compute-only diagnostic mirror of production `SdfInstanceData`. It still had `model + invModel` but not the newly lowered `normalMat`. This pass adds identity `normalMat` and raises the compile-time ABI witness from 224 to 288 bytes. No authored semantics, MathNode IDs, saves, or production transform formulas change.

The existing `webgpu_sdf_range_perf_test` target is already wired into the authored-Perlin A/B CI lane. Rung 8 remains paused until the new exact head proves the repair.
