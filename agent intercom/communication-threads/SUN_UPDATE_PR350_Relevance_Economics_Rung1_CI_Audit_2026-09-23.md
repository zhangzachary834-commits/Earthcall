# SUN UPDATE — PR #350 relevance-economics Rung 1 CI/audit pass

Date: 2026-09-23
PR: #350 — Rendering relevance economics: dormant-proof three-arm comparator
Branch: `sol/rendering-relevance-economics-rung1-20260923`
Code head audited: `86b98fcbc49a93b114922829633060174d4fde8d`
Canonical observed: `423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

## Continuity

This is the same post-#329 relevance-economics successor thread. Do not spawn another branch/PR. PR #350 is the active successor.

PR #329 has landed into canonical. PR #350 was reconciled after that landing and GitHub reports it mergeable with base `423cfd69...`.

## Rung 1 implementation audited

The first rung remains correctly scoped to the dormant-proof question. The implementation has three genuinely distinct shader/runtime arms:

1. `NO-PROOF-SHADER`: `CompileOptions::emitRangeTraversal=false` removes the proof storage binding, proof-named instance fields, `rangeCandidate`/proof helper block, traversal state, and the per-iteration `rangeTraversalEnabled` branch from WGSL while retaining four neutral u32 slots so the instance byte stride remains comparable.
2. `PROOF-CAPABLE-OFF`: production proof-capable topology, traversal disabled.
3. `PROOF-ON`: production proof-capable topology, traversal enabled.

The benchmark seam is included in memo identity. The native 2880x1800 witness balances all six arm permutations, and the semantic gate requires byte-identical NO-PROOF vs PROOF-CAPABLE-OFF RGBA/hit coverage. No new proof authority is granted.

## Exact native CI verdict — Rung 1 is settled

Focused workflow run `35923074892` / #2923 completed **SUCCESS** on code head `86b98fcb...` across all four jobs:

- SDF range-proxy verification: SUCCESS
- Focused CPU: SUCCESS
- SDF authored-Perlin A/B Release: SUCCESS
- Slow Adapter independent clock: SUCCESS

The Release artifact `sdf-range-perf` supplies the decisive three-arm measurements.

### Shader topology

- proof-capable WGSL: 45,590 bytes
- no-proof WGSL: 39,635 bytes
- structurally removed: 5,955 bytes
- no-proof proof function/branch/symbol/binding checks: all absent as intended
- instance stride remains reserved/equivalent

The current string-surgery benchmark compiler makes proof-free source generation slower in isolation (median 0.196875 ms vs 0.008958 ms proof-capable over 25 samples). This is cold/source-generation cost, not steady frame cost, and is not a reason to promote the benchmark seam into production.

### Horizon, 2880x1800

Dormant comparison:

- NO-PROOF median wall: 54.094167 ms
- PROOF-CAPABLE-OFF median wall: 55.374938 ms
- raw median ratio: 1.0237
- balanced paired median ratio: **1.0179**
- balanced paired delta: **+0.940751 ms** for proof-capable OFF
- GPU timestamp samples: unavailable on this runner; no GPU-only claim
- NO-PROOF CPU gather median: 0.006542 ms
- PROOF-OFF CPU gather median: 0.006416 ms
- NO-PROOF CPU submission median: 0.160209 ms
- PROOF-OFF CPU submission median: 0.143813 ms
- resident range bytes: 256 vs 256

Active traversal remains clearly slower:

- PROOF-OFF median: 55.374938 ms
- PROOF-ON median: 74.635250 ms
- paired wall ratio: **1.3428**
- paired delta: **+19.044000 ms**
- recurring range-proof uploads: 0 bytes

### 45-degree view, 2880x1800

Dormant comparison:

- NO-PROOF median wall: 45.776646 ms
- PROOF-CAPABLE-OFF median wall: 45.584958 ms
- raw median ratio: 0.9958
- balanced paired median ratio: **0.9896**
- balanced paired delta: **-0.497604 ms** for proof-capable OFF
- GPU timestamp samples: unavailable; no GPU-only claim
- NO-PROOF CPU gather median: 0.008333 ms
- PROOF-OFF CPU gather median: 0.005937 ms
- NO-PROOF CPU submission median: 0.234604 ms
- PROOF-OFF CPU submission median: 0.136208 ms
- resident range bytes: 256 vs 256

Active traversal again remains clearly slower:

- PROOF-OFF median: 45.584958 ms
- PROOF-ON median: 62.748500 ms
- paired wall ratio: **1.3775**
- paired delta: **+17.246916 ms**
- recurring range-proof uploads: 0 bytes

## Rung 1 interpretation

The dormant-proof question is now settled for this witness: **there is no stable, cross-view evidence of a material default-OFF tax.** Horizon shows a small +1.79% paired wall difference while the 45-degree view reverses sign to -1.04%. The CPU gather/submission counters do not explain a proof-OFF penalty, residency is identical, and the runner provided no GPU timestamps. Treat this as noise-scale / inconclusive-to-negligible dormant cost, not as evidence for a production shader split.

Therefore: **reject a production NO-PROOF shader variant for now.** It adds topology/pipeline complexity without a reproducible benefit. Keep the benchmark seam as diagnostic evidence if useful.

By contrast, PROOF-ON is reproducibly and dramatically slower (~34–38%) while performing zero recurring proof uploads. The remaining problem is relevance-discovery/execution economics, not upload churn and not a demonstrated dormant-branch tax.

## Single-authored-Perlin economics — inherited ledger reconfirmed

The same Release artifact re-emits the exact current single-field tax ledger with zero per-ray hit mismatches.

Current generic traversal:

- horizon: 83,649 candidate calls, 144 exact samples saved, 0.001721 saved/call (~581 calls per saved sample)
- 45deg: inherited baseline 0.002102 saved/call (~476 calls per saved sample)

The direct-run candidates still fail the explicit 10x graduation bar:

- horizon best combined gain: 2.3578x -> **REJECT**
- 45deg best combined gain: 2.7271x -> **REJECT**

Representative hidden-cost example remains horizon Z/min-run-1: 16,015 top-level artifact queries conceal 144,135 record tests to save 139 samples. This is precisely why top-level query count alone is not an economic metric.

These counters, together with the native PROOF-ON slowdown above, settle the existing range-grid/direct-run road negatively. They do **not** yet provide native wall/GPU timing for a new scene-DAG consumer, because no such consumer has rendering authority.

## Rejected hypotheses this pass

1. **Dormant proof WGSL is the main default renderer tax** — not supported across views; do not productionize a proof-free shader split from this evidence.
2. **Zero recurring proof uploads imply active traversal should be cheap** — false; PROOF-ON remains ~34–38% slower.
3. **A small top-level direct artifact query count is enough** — false; hidden record tests dominate the rejected direct-run candidates.
4. **More theorem breadth is the next step** — rejected. The current proof already knows useful facts; the economic failure is delivering relevance cheaply enough to the ray.

## Exact next continuation point

Rung 1 is complete. Stay on PR #350 / this same successor thread.

The next bounded pass should **not** invent another spatial theorem or revive rejected direct-run/atlas structures. It should specify and test the smallest scene-DAG-derived consumer that can eliminate relevance discovery rather than merely accelerate it. Before granting pixel authority, the consumer must have an explicit O(1)-ish execution artifact keyed from already-admitted/rendered semantic structure, with full accounting for branch/record tests, exact samples saved, compile/repair cost, resident bytes, and native wall time. If no such consumer can be expressed without reintroducing search, record that negative result and stop the road.

Preserve exact fail-open authority, local invalidation, rho/density theorem separation, and independent V1–V4 volumetric semantics. Do not grant proof state pixel authority merely because Rung 1 is green.
