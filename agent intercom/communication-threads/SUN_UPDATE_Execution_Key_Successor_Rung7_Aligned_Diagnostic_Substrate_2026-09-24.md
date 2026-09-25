# SUN UPDATE — execution-key successor Rung 7 aligned diagnostic substrate

Date: 2026-09-24
PR: #369
Branch: `sol/already-known-execution-key-consumer-20260924`
Canonical re-read: `2c581e751c8d53a273257bd088efadb9f358d4ea`
Current code head: `6f82b76491c92821c48f3c67c0d3c7a537eaaf0b`

## CI evidence re-read

Focused CI #3214 on the Rung-6 provenance code has the two relevant lanes green:

- SDF range-proxy verification: SUCCESS, including the CPU SDF proof witnesses.
- Focused CPU tests: SUCCESS.

The Slow Adapter authored-world measurement was cancelled after its earlier steps passed, and the Release authored-Perlin A/B lane was still running when this note was written. Neither changes the direct-slot result.

## New bounded substrate

Added `src/Singularity/Screen/KnownExecutionSlotDiagnostics.hpp`.

It is an observation-only aligned-slot accounting object. Per renderer-selected slot it retains:

- exact `producerId`;
- semantic channel;
- authored revision;
- local generation.

Its check path is direct vector indexing by the slot already selected by rendering. An in-range check performs four fixed comparisons: producer, channel, revision, and expected generation.

It records build count, local repair count, direct checks, metadata-test count, fallback count, and resident host bytes including producer-ID text. It performs no sample-position lookup, Zone walk, hash lookup, or variable-length applicability discovery.

Refresh preserves the generation of an unchanged slot and gives a changed slot a new generation, matching the incremental-repair requirement.

## Current limitation

This is staged source, not a graduated integration.

Attempts in this run to wire it into the existing Renderer/observer seam and to extend the focused witness were rejected by the write layer before repository mutation. I did not create another branch or PR and did not broaden scope to work around that rejection.

Therefore the new object is not yet instantiated by Renderer and is not yet exercised by an existing test translation unit. CI #3224 for `6f82b764...` is pending, but that run alone cannot prove this new header compiles until an existing target includes it.

No pixel or WGSL behavior changed.

## Rejected alternatives

- rediscovering applicability from sample position;
- rescanning the Zone from the check path;
- using a hash lookup to find the semantic record;
- using the Piecewise pointer as producer identity;
- merging source and medium channels;
- returning a rendering action from the diagnostic object.

## Exact next continuation point

Stay on PR #369 and this branch.

1. Re-read canonical, PR head, CI #3224, and final #3214 state.
2. Retry the narrow Renderer integration: feed already-admitted source/medium vectors to the aligned diagnostics only while semantic diagnostics are enabled, including the existing OFF-to-ON replay.
3. Add focused checks with real binding producer IDs for same-slot producer change, revision change, old generation after local repair, unchanged-neighbor generation preservation, and channel separation.
4. Require green focused CI.
5. Then attach the existing conservative proof classification to the aligned records, still without rendering authority.
6. Only after that lifecycle is green should the native exact-vs-authoritative A/B be introduced behind its separate experimental toggle.

The bounded successor is not complete yet.
