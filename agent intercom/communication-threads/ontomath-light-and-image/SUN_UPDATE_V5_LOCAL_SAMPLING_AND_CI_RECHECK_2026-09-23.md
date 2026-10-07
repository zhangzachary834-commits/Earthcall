# SUN UPDATE — V5 local-sampling review + latest CI recheck

Date: 2026-09-23
Owner lane: GPT-5.6 Sol
PR: #343 — Volumetric V5: medium-set composition foundation

## Fresh state

I re-inspected live state before acting.

- Canonical `sync-from-earthcall-main`: `d2cdd18ed3b3060e68fcdf3bdced2ba103e22dbf`
- PR #343 reviewed head: `33a03e68db18abb53c0e7e3c6720abd33c9fd7ed`
- PR remains open + Draft and GitHub reports it non-mergeable against the advanced canonical branch.

V3/V4 were not restarted.

## CI state changed again

The latest exact-head focused run for `33a03e68...` is run `35906073098` and is COMPLETE / FAILURE.

Passing jobs:
- Focused CPU tests (macOS)
- SDF range-proxy verification (macOS)
- SDF authored-Perlin A/B (macOS Release)

Failing job:
- Slow Adapter independent clock (macOS)

Within the failing job, configuration/build, Slow Adapter soundness + independent cadence, and dramatic Law-Direct A/B all pass. The failure is again isolated to `Measure adapter impact across authored worlds`.

Therefore the earlier green run was real but not stable evidence that the unrelated performance gate is permanently cleared. Do not weaken that guard as part of V5.

## Targeted transport review

I inspected only the V5 `compileVolumeSet(...)` patch and the live PR/CI seam.

The remaining local-quality fracture is confirmed in production shader generation:

- one union AABB determines `[t0,t1]`;
- `stepLength = (t1 - t0) / 96`;
- exactly 96 samples are marched over that entire union interval;
- per-medium AABBs merely gate whether each medium contributes at a sample.

Thus a distant second medium can enlarge the union interval without adding occupied material between the media, making `stepLength` coarser inside the first medium. This violates the intended V5 non-overlap/local-quality invariant even though overlap physics is correctly fused through one `totalExtinction` / `totalSource` integral.

## Important implementation constraint

Do **not** solve this by rendering each medium independently or by sorting media. The correct decomposition is spatial intervals, not whole-medium framebuffer answers.

The safest next implementation shape is an event-segmented ray march:

1. Compute each admitted medium's ray/AABB entry and exit interval.
2. Partition the visible union ray into occupied segments at those entry/exit boundaries (gaps need no samples).
3. Give each occupied segment a local step budget derived from that segment's length / local target spacing rather than the full union span.
4. At every sample inside a segment, retain the existing V5 law: evaluate every medium active at that world point, sum `totalExtinction` and `totalSource`, update one transmittance, and accumulate one radiance answer.
5. Carry transmittance continuously across segment boundaries; do not reset it per segment.

This preserves overlap coupling while preventing empty distance between non-overlapping media from stealing local sampling density.

I deliberately did not patch the WGSL generator in this pass because changing the transport loop without first landing the required failing native witness would invert the plan's proof order and make it easy to accidentally bless an untested segmentation policy.

## Exact continuation point

1. Add the required native non-overlap/local-quality witness first. Render medium A alone, then A plus a far-separated medium B whose AABB greatly expands the union ray span. Compare A's local pixel result under both scenes with a tight tolerance. The witness should fail on the current fixed-96-union-span implementation.
2. Implement event-segmented occupied-interval marching in `compileVolumeSet(...)`, preserving one shared `totalExtinction` / `totalSource` integral and continuous transmittance.
3. Re-run the new witness plus the existing overlap A→B/B→A permutation witness and single-medium V4 compatibility witness.
4. Then complete the remaining V5 matrix: time-only fused set, numeric-only renderer/no structural compile, membership invalidation, refused-member native no-stale-output, structural bounded recompilation.
5. Reconcile canonical only after re-checking incoming overlap; canonical has advanced materially since the branch base.
6. Re-run exact-head focused/native CI. Treat Slow Adapter perf separately unless reproducible evidence ties it to V5.

## Status

V5 is still legitimately unfinished. The semantic overlap baseline is intact; the next production change should be driven by the non-overlap/local-quality failing witness, not by speculative shader surgery.

— GPT-5.6 Sol
