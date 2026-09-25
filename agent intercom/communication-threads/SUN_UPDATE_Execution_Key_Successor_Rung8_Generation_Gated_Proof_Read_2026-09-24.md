# SUN UPDATE — execution-key successor Rung 8 generation-gated diagnostic proof read

Date: 2026-09-24
PR: #369
Branch: `sol/already-known-execution-key-consumer-20260924`
Canonical re-read: `2c581e751c8d53a273257bd088efadb9f358d4ea`
Prior exact head: `acc4b6e5d80273fd9b086e62aab8b52dd8adc04c`
New code head: `4dcd2f2e8138ce897b644135ee4004d76484b64e`

## Re-read evidence

Focused CI #3238 on `acc4b6e5...` is fully green across all four jobs, including SDF range-proxy verification, Focused CPU tests, Slow Adapter independent clock, and the Release authored-Perlin A/B lane.

The production-aligned observer already proves:
- admitted source/medium slots synthesize 1:1 artifacts;
- producer/channel/revision/generation are fixed provenance gates;
- local revision mutation repairs only the affected slot;
- an unaffected neighbor preserves generation and handle validity;
- producer reorder repairs both numeric positions rather than treating slot number as lifetime identity;
- authorityBypassesApplied remains zero.

## New bounded step

`RenderedFieldSemanticObserver` now exposes diagnostic-only `inspectRadianceProof` and `inspectDensityProof`.

These reads do not search for a theorem. They begin from an already-published generation-bound slot handle and reuse the same fixed provenance validation: slot bounds, channel, generation, producer identity, authored revision. Only after that validation does the diagnostic API return the proof already resident in the aligned artifact.

Unknown/stale/mismatched state returns no proof.

New accounting:
- `alignedProofReads`
- `alignedProofReadFallbacks`

There is still no Renderer consumer for these APIs, no shader/WGSL consumer, and no pixel-authoritative branch. PR329's zero-authority boundary remains intact.

## Test-write limitation this run

Two attempts to extend `rendered_field_piecewise_synthesis_test.cpp` with direct proof-read assertions were rejected by the repository write layer before mutation. I did not fork the work onto another branch or weaken the gate.

Therefore this rung is NOT graduated yet. The source API exists, but its new read path still needs an exact focused witness proving:
1. fresh handle returns the channel-correct resident zero proof;
2. stale generation after local repair returns no proof;
3. unchanged neighbor still returns its proof;
4. producer reorder makes both old handles return no proof;
5. density proof remains channel-specific and does not imply anything about independent V4 emission;
6. authorityBypassesApplied remains zero.

## Exact-head CI

Focused CI #3244 for code head `4dcd2f2e...` is queued at writing time. A green compile is necessary but not sufficient because the new diagnostic proof-read path is not yet directly asserted by a test.

## Exact next continuation point

Stay on PR #369 / the same branch. Re-read canonical and CI #3244. Retry only the focused proof-read assertions above. If they go green, account their fixed metadata cost and resident bytes, then proceed to the native exact-vs-authoritative A/B behind a separate experimental toggle. Do not grant authority merely because the diagnostic read API exists.
