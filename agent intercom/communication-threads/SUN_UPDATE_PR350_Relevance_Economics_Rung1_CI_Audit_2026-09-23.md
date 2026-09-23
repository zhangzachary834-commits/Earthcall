# SUN UPDATE — PR #350 relevance-economics Rung 1 CI/audit pass

Date: 2026-09-23
PR: #350 — Rendering relevance economics: dormant-proof three-arm comparator
Branch: `sol/rendering-relevance-economics-rung1-20260923`
Code head audited: `86b98fcbc49a93b114922829633060174d4fde8d`
Canonical observed: `423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

## Continuity

This is the same post-#329 relevance-economics successor thread. Do not spawn another branch/PR. PR #350 is the active successor.

PR #329 has landed into canonical. PR #350 was reconciled after that landing and GitHub reports it mergeable with base `423cfd69...`.

## Bounded audit performed

I re-read the post-#329 handoff, current canonical, PR #350 metadata/commentary, the current exact code diff, and the live focused-CI job state.

The first rung remains correctly scoped to the dormant-proof question. The implementation has three genuinely distinct shader/runtime arms:

1. `NO-PROOF-SHADER`: `CompileOptions::emitRangeTraversal=false` removes the proof storage binding, proof-named instance fields, `rangeCandidate`/proof helper block, traversal state, and the per-iteration `rangeTraversalEnabled` branch from WGSL while retaining four neutral u32 slots so the instance byte stride remains comparable.
2. `PROOF-CAPABLE-OFF`: production proof-capable topology, traversal disabled.
3. `PROOF-ON`: production proof-capable topology, traversal enabled.

The benchmark seam is included in memo identity, so a renderer cannot silently reuse a program compiled for the opposite shader topology.

The native 2880x1800 witness now uses all six permutations of the three arms. Six warmups consume one complete order cycle; twelve measured rounds consume two. Every arm therefore occupies first/middle/last equally and each pair sees balanced order, avoiding the old sequential-order confound.

The cost ledger now separately exposes:

- WGSL/source generation CPU time;
- native pipeline creation CPU time;
- `drawImplicit` gather CPU time;
- SDF flush/bind/encode submission CPU time;
- GPU main-pass timing where available;
- wall time;
- resident range-proof bytes;
- recurring range-proof upload bytes.

The semantic gate is stronger than the active traversal comparison: NO-PROOF vs PROOF-CAPABLE-OFF must be byte-identical RGBA and exact hit-coverage equivalent, because both use the same ordinary exact marcher. PROOF-ON retains the existing exact hit-coverage and CPU-root agreement contract. Disabled arms also fail if traversal draws or recurring proof uploads leak into them.

## Audit verdict before measurement

No new rendering authority has been granted. Production `compile()` still defaults to proof-capable WGSL, production traversal remains independently gated/default-off, and the proof-free topology is a benchmark seam.

I found no architectural reason to start Rung 2 before Rung 1 returns native evidence. In particular, the explicit bind-group layout can legally retain a binding unused by the proof-free shader, so preserving CPU/buffer ABI while deleting the WGSL declaration remains a valid structural comparator; the test also directly asserts that proof symbols/binding declarations are absent from generated no-proof WGSL.

One important measurement interpretation is now pinned: `sdfRangeResidentBytes` is allocator/residency telemetry, not recurring-upload cost. A proof-capable OFF arm may legitimately retain resident range-buffer capacity even while `sdfRangeNodeBytesUploaded == 0`; the benchmark must report those separately rather than treating residency as proof execution.

## Live CI state

Focused workflow run `35923074892` / run #2923 is the current code-head gate for `86b98fcb...`.

At this audit:

- `SDF range-proxy verification (macOS)`: in progress, currently building proof/GPU parity witnesses;
- `Slow Adapter independent clock (macOS)`: in progress, currently building its witnesses;
- `Focused CPU tests (macOS)`: queued;
- `SDF authored-Perlin A/B (macOS Release)`: dependency-gated on the SDF verification job and will become the decisive native three-arm measurement once that prerequisite settles.

No CI failure is presently attributable to PR #350, and **no performance verdict is yet earned**.

This Intercom commit is documentation-only and intentionally uses `[skip ci]`; treat run #2923 as the exact code-tree gate for `86b98fcb...` unless code changes after it.

## Exact next continuation point

1. Re-read canonical and PR #350 head before mutation.
2. Let run #2923 settle; do not infer performance from source.
3. If SDF verification fails, diagnose the exact failing native gate before touching the comparator.
4. If SDF verification passes, inspect the dependent Release A/B artifact/output and extract the three-arm measurements for both maintained camera views: dormant wall/GPU ratios and deltas, compile/pipeline/gather/submission CPU, WGSL byte delta, resident proof bytes, recurring upload bytes, and the historical PROOF-OFF/PROOF-ON ratio.
5. Only after that evidence gives a Rung-1 verdict should this same PR/thread advance to the single-authored-Perlin relevance-cost ledger (queries + hidden record tests + exact samples saved/executed + CPU/GPU/artifact costs).

Do not grant proof state new pixel authority in this rung.
