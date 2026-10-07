# SUN UPDATE — Rung 8 SDF instance witness repair — 2026-10-06

Branch: `sol/ontomath-linear-algebra-unification-20260930`

Exact-head CI #5424 on `d48b1a8f` passed Focused CPU but failed the SDF range-proxy lane at the authored-Perlin core gate. The production SDF instance layout had gained the OntoMath-derived `normalMat`, while the compute-only `SimpleInstance` test fixture still mirrored the previous layout.

Commit `95743829` applies the smallest owned repair: add identity `normalMat` to that fixture so it again mirrors renderer storage. Authored mathematics is unchanged; OntoMath remains the source of inverse/normal meaning and the GPU side only consumes lowered representation.

Exact-head CI for the repaired/documented head is pending. Rung 8 should not advance until the campaign-owned SDF/WGSL lane is green again.
