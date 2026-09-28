# SUN VERDICT — Already-Known Execution-Key Consumer After PR #350

Date: 2026-09-28  
PR: #369

The bounded successor is complete.

## Verdict

The real already-known source/medium binding slot passes the no-hidden-search, identity/lifetime fail-open, channel-sovereignty, incremental-repair, and native economics gates.

Final exact-head reconciled A/B on `310fff58b3f5855337a4c6ed688dab3bca84ebba`:

- iterations: 200,000
- exact authored evaluation: 430,651,167 ns
- aligned proof path: 14,895,166 ns
- exact/aligned: **28.912143x**
- artifact build: 1,458 ns
- logical resident bytes: 70
- proof reads: 200,000
- metadata tests: 800,000
- proof fallbacks: 0
- benchmark authority decisions: 200,000
- production authority bypasses: 0

Exact-head workflows #3511 and #3512 are green across Focused CPU, Slow Adapter, SDF range-proxy verification, and authored-Perlin Release A/B.

## What this changes

PR #350 rejected generic per-sample relevance discovery because the search/classification work dominated the exact evaluations it avoided.

PR #369 establishes a distinct profitable regime: when production execution already knows the semantic binding slot independently of theorem lookup, relevance discovery disappears. The hot path becomes a fixed aligned artifact read plus bounded provenance tests.

## What it does not change

- PR329 does not gain blanket renderer authority.
- Production pixel authority remains zero in #369.
- Arbitrary spatial sample positions still do not possess a free theorem key.
- Equal canonical math does not merge semantic authority.
- Density-zero authority may never erase independent V4 self-emission.

## Identity/lifetime boundary

Numeric slot is execution address, not lifetime identity. Authority requires stable producer identity + semantic channel + authored revision + artifact generation. Producer replacement, stale generation, removal/re-addition, slot reorder/reuse, and cross-channel reuse all fail open to exact execution.

Local mutation repairs only its aligned slot; unaffected neighbors remain valid.

## Integration gait

Stable semantic base:
`1ca65259b4888acf8fa58688e3272cfe2326895b`

Final deliberate ancestry-preserving reconciliation:
`310fff58b3f5855337a4c6ed688dab3bca84ebba`

Parents:
- successor `b388afd8a43c6b5e8f478abea16805fec1c54436`
- canonical `3728fd419a72fd2ef85d88b336242bcf1e6d254d`

No force push or snapshot overwrite.

Canonical movement after that reconciliation was audited rather than chased. The only successor-path overlap is unrelated focused-test additions in the workflow; no successor semantic dependency changed.

## Exact next continuation point

None for this bounded successor.

If Zach commissions the next experiment, start a new narrowly-scoped production-authority experiment:

**one already-selected SourceRho binding + everywhere-defined scalar literal zero + exact-vs-authoritative renderer A/B + live hostile mutation fail-open + real frame/CPU/GPU economics.**

Only after that succeeds should MediumDensity zero authority be tested separately, explicitly preserving nonzero V4 self-emission.

Do not reopen generic Perlin relevance search. Do not invent another rung merely to keep this Sun alive.
