# SDF Spatial Prophetic — Ray-Entry Direct Dispatch Atlas

Status: test-only next-rung architecture after PR #301 falsified maximal positive-run scanning. No production renderer activation is authorized by this document.

## 0. Battlefield result that forces this rung

PR #298 established that the existing depth-4 Spatial Prophetic theorem is already ahead-of-time and GPU-resident, yet the generic consumer pays roughly 75k–84k `rangeCandidate()` consultations across 16k rays to remove only ~144–157 exact authored SDF samples.

PR #301 then compiled the positive proof into maximal axis runs. That reduced the visible query cadence toward one query per ray, but the best min-run-1 cases still performed roughly 8–10 hidden run-record tests per ray. Thresholding the run set reduced hidden search, but also discarded enough useful authority that complete consultation economics improved only about 2–3x rather than the required ~10x.

The new constraint is therefore stronger:

> A direct road is not a smaller structure to search. It is a precompiled answer to which consequence set matters.

The next experiment must remove the global run search itself.

## 1. Proposed artifact

Compile the richer positive-proof theorem into a **camera-independent ray-entry dispatch atlas**.

A runtime ray computes a small deterministic key from field-local geometry:

1. dominant ray axis;
2. sign of travel on that axis;
3. quantized transverse coordinates at field-bound entry;
4. quantized transverse slopes relative to the dominant axis.

That key performs one O(1) table lookup.

The table value is not another hierarchy node and not a pointer to a bucket that must be searched. It is a packed route containing at most a very small fixed number of already-selected positive-proof run records.

Conceptually:

```text
ray enters field bounds
        ↓
compute stable ray-entry key
        ↓
one atlas lookup
        ↓
0..K already-selected proof roads
        ↓
exact AABB authority on only those roads
        ↓
skip proved-positive interval(s)
        ↓
exact authored marcher everywhere else
```

The richer proof grid/hierarchy remains authoritative.

The atlas is disposable derived execution state.

## 2. Why this is materially different from #301

#301 executed:

```text
ray
  ↓
scan/search global derived run set
  ↓
discover which record may matter
  ↓
exact run test
```

The atlas executes:

```text
ray
  ↓
direct key
  ↓
already-selected tiny consequence set
  ↓
exact run test(s)
```

The expensive search is moved entirely into ahead-of-time compilation.

The hot path does work bounded by the fixed route width, not by total proof-cell count, total run count, tree depth, or the number of globally possible consequences.

## 3. Camera independence is mandatory

The atlas must not be built from the current screen, current camera, current frame's rays, or historical observed ray traffic.

The test artifact is generated only from:

- field-local proof geometry;
- authored proof-derived positive runs;
- a fixed geometric quantization of possible field-entry positions;
- a fixed quantization of ray direction relative to the dominant axis.

A camera merely produces runtime keys into that already-existing field-local atlas.

Changing the camera does not rebuild the theorem or the atlas.

This is what separates the experiment from a view-dependent visibility cache.

## 4. First executable key

Let:

- `A` be the dominant direction axis;
- `sign` be the sign of `rd[A]`;
- `U,V` be the other two axes;
- `pEntry` be the exact ray/field-AABB entry point;
- `entryBins` quantize `pEntry[U]` and `pEntry[V]`;
- `slopeBins` quantize
  `rd[U] / abs(rd[A])` and
  `rd[V] / abs(rd[A])`.

Because `A` is dominant, both normalized transverse slopes are within approximately [-1, 1].

The direct key is therefore:

```text
(A, sign, entryU, entryV, slopeU, slopeV)
```

with a flat-table index computed in O(1).

No runtime traversal of proof cells is permitted in order to compute this key.

## 5. AOT route compilation

For each atlas key, construct a representative field-local ray from the center of the corresponding entry/slope bin.

Ahead of time only:

1. enumerate the positive proof runs associated with that dominant axis;
2. exact-intersect those runs with the representative ray;
3. order the intersections by entry distance;
4. retain at most `routeWidth` earliest profitable consequences;
5. pack their run indices directly into the atlas value.

The first implementation may use a whole-atlas bootstrap build because this is a falsification experiment.

That bootstrap is explicitly not the completed maintenance architecture.

## 6. Packed route form

The first test should cap route width at four consequences and pack up to four 8-bit one-based run indices into one 32-bit atlas word:

```text
bits  0.. 7 -> route slot 0
bits  8..15 -> route slot 1
bits 16..23 -> route slot 2
bits 24..31 -> route slot 3
0           -> no consequence
```

This intentionally caps the first experiment to <=254 run records.

If the proof produces more than the representable run count, the candidate must fail open rather than silently aliasing indices.

The packed word makes the actual runtime dispatch artifact tiny enough to judge honestly.

## 7. Runtime authority and safety

The atlas itself does **not** authorize skipping.

Each atlas-selected run is still an AOT derivative of positive proof cells, and the runtime shader must exact-intersect the current ray with that run's field-local AABB before using it.

Therefore:

- a false atlas dispatch costs a record test but cannot create false proof;
- a missed useful run only loses an optimization opportunity;
- an empty key falls open;
- a run miss falls open;
- any unrepresented proof remains available to the exact marcher;
- no clear/unknown region is promoted into skip authority.

Correctness remains guarded by direct-vs-OFF per-ray hit parity.

## 8. This is not DDA

The runtime must not:

- advance cell-by-cell through the proof grid;
- perform a voxel walk;
- branch on neighboring proof cells;
- descend a tree;
- search a mip pyramid;
- repeatedly ask the atlas for the next spatial cell.

One ray-entry key is computed once.

The first experiment may execute only the fixed route encoded by that key.

After those route consequences are exhausted, runtime falls open to exact marching.

If later evidence justifies a chained direct road, that chain must itself be precompiled and bounded by actual retained consequences rather than rediscovering grid adjacency at runtime.

## 9. Parameter sweep

The test-only rung should sweep a deliberately small family, for example:

- minimum positive-run length: 1 and 2 cells;
- entry bins per transverse axis: 4 and 8;
- slope bins per transverse slope: 2 and 4;
- route width: 1 and 2.

The goal is not to maximize benchmark score by exhaustive tuning.

The goal is to answer whether direct ray-space dispatch has qualitatively better economics than generic proof interpretation and global run search.

## 10. Required metrics

For every camera and candidate report:

- rays;
- atlas keys / table entries;
- populated keys;
- atlas bytes;
- retained run-record count and bytes;
- total artifact bytes;
- atlas lookups;
- non-empty dispatches;
- selected route slots;
- exact selected-run AABB tests;
- useful skip calls;
- exact direct `sdfSampleStep()` calls;
- exact OFF `sdfSampleStep()` calls;
- exact samples saved;
- samples saved per atlas lookup;
- samples saved per selected-run test;
- skipped distance;
- per-ray hit mismatches;
- OFF-baseline drift against the #298 exact diagnostic.

A representation does not get credit for hiding work inside one logical lookup.

All selected-run tests count.

## 11. Falsification bar

The generic #298 baseline was approximately:

- 4.7–5.2 proof consultations per ray;
- 0.0017–0.0021 exact samples saved per consultation.

The first direct-run #301 candidate improved complete economics only about 2–3x.

The dispatch atlas should be rejected before production if it cannot achieve roughly an order-of-magnitude improvement over the generic baseline on complete lookup/test economics while preserving zero per-ray hit mismatches.

A candidate may also be rejected if:

- artifact size becomes disproportionate to the proof it accelerates;
- most keys are populated but almost never useful;
- coarse bins cause selected-run tests to recreate a broad search tax;
- fine bins improve precision only by exploding resident memory;
- camera movement would require rebuilding;
- runtime execution begins to resemble DDA/tree traversal.

## 12. Incremental repair invariant

The final architecture must obey the same doctrine as Prophetic/Formation Rete:

> Time passing does not invalidate proof. Only changes to proof premises do.

When authored semantics change:

```text
semantic delta
    ↓
Prophetic dependency frontier
    ↓
affected theorem regions only
    ↓
repair affected positive runs only
    ↓
repair atlas keys whose dependency frontier overlaps those runs/regions
    ↓
preserve every unaffected theorem node, run, packed route, and GPU-resident region
```

The production-quality atlas therefore needs reverse provenance sufficient to answer:

```text
changed proof region -> which direct routes could have changed?
```

A route that explicitly references a changed run is obviously dirty.

A newly created/deleted/moved run can also change keys that previously selected some other run or no run at all, so the mature dependency frontier must include the geometric ray-key region whose representative/conservative route selection could be affected by that proof region.

The test-only whole-atlas builder is allowed as bootstrap evidence only.

## 13. Graduation boundary

Even a successful diagnostic does not authorize immediate production replacement.

The sequence remains:

```text
test-only atlas
    ↓
falsify economics + correctness
    ↓
if large-margin success
    ↓
separate production A/B PR
    ↓
persistent GPU residency
    ↓
incremental dependency-frontier maintenance
```

Production must retain exact fallback and richer-theorem authority.

## 14. Governing sentence

Spatial Prophetic should compile not merely what is true, but the cheapest stable road by which runtime can exploit that truth.

And the companion maintenance rule remains:

> Do not rediscover what stayed true. Repair what stopped being proved.
