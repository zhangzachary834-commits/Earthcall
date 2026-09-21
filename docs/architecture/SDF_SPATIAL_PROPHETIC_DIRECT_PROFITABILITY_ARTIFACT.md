# SDF Spatial Prophetic: Direct Profitability Artifact

Status: next experimental rung after PR #298 runtime-tax verdict.

## Evidence boundary

PR #298 measured the exact generated-WGSL consumption path without instrumenting the production renderer. On the maintained authored-Perlin witness:

- horizon: 83,649 `rangeCandidate()` consultations saved 144 exact `sdfSampleStep()` calls (~581 consultations per saved sample); paired wall median 1.3127x, +17.51 ms;
- 45 degrees: 74,686 consultations saved 157 exact sample calls (~476 consultations per saved sample); paired wall median 1.4030x, +17.39 ms;
- per-ray ON/OFF hit mismatches: 0;
- recurring proof upload bytes: 0.

The theorem is already ahead-of-time and resident. The generic hot-path proof interpreter is the tax.

This evidence does **not** authorize reviving the rejected one-AABB gate, depth-5 density, DDA, dense/sparse octrees, mip pyramids, negative proof, raster tightening, distance-field traversal, or grid-scale hoist.

## Formation-Rete Direct precedent

The richer theorem is not itself the desired frame-time representation. It is the derivation and repair spine.

The frame-time representation should be a smaller artifact compiled from that theorem whose only purpose is to answer a narrower question cheaply enough to be worth asking:

> Is there a proof-authorized interval on this ray that is likely to eliminate more authored field work than the direct artifact costs to consult?

This is analogous to Law-Direct: derive the expensive relevance relation ahead of time, then execute a crystallized road rather than rediscovering relevance through the generic structure on every event.

## First experimental artifact: profitable proof runs

The narrowest next experiment is **not another spatial hierarchy**. Compile the existing positive-proof bitmap into maximal axis-adjacent positive runs, preserving the exact authority of the underlying theorem.

A run record contains only derived data:

- axis;
- fixed coordinates on the other two axes;
- first positive cell;
- one-past-last positive cell;
- field-space interval bounds implied by those cells;
- source proof-cell range / dependency provenance sufficient for incremental repair.

The regular proof grid remains the authority and repair/explanation spine. Runs are disposable derived execution artifacts.

### Profitability filter

Do not emit every positive run blindly. Ahead of time, reject runs whose maximum possible skipped distance cannot plausibly amortize one direct-artifact consultation on the maintained marcher.

The first experiment must sweep this threshold **test-only**. It must not hard-code a production threshold before measurement.

The sweep should report, per threshold and camera:

- direct-artifact consultations;
- authorized skips;
- exact `sdfSampleStep()` calls ON/OFF;
- exact samples saved per consultation;
- skipped distance;
- per-ray hit mismatches;
- artifact record count and bytes.

The decisive metric remains `exact authored evaluations avoided / runtime artifact consultations`, not proof density.

## Test-only A/B before production

The first implementation belongs in the existing native authored-Perlin measurement witness (or a sibling diagnostic target), not in the production fragment shader.

Use the exact WGSL emitted by `sdfwgsl::compile()` for field evaluation and preserve the current side-by-side ON/OFF marcher fidelity. Add a third diagnostic marcher arm that consumes the compiled run artifact. Do not add atomics to the production shader or contaminate the existing persistent-renderer AB/BA timing lane.

A candidate representation graduates to a production A/B only if all of these hold:

1. zero per-ray hit mismatches against exact OFF;
2. materially fewer consultations than the current 4.7–5.2 `rangeCandidate()` calls/ray;
3. materially better exact samples-saved-per-consultation than 0.0017–0.0021;
4. artifact size remains small enough to be resident and upload-free during ordinary frames;
5. no change to proof authority or fallback semantics.

If the diagnostic cannot improve consultation economics by at least an order of magnitude, reject the representation before production mutation.

## Incremental repair is a hard architectural constraint

The direct artifact is derived state, not a second source of truth.

Ordinary frames do not rebuild it.

When a relevant semantic edit invalidates proof cells, repair must operate on the dependency frontier:

1. determine affected proof regions from Prophetic dependencies;
2. recompute only those theorem regions;
3. identify direct run records whose provenance overlaps the changed regions;
4. delete/split/extend/rebuild only those records;
5. preserve all unaffected proof nodes, proof cells, direct records, and GPU-resident state.

A whole-grid rebuild may remain a temporary bootstrap/fallback implementation, but it is not the target architecture and must not be mistaken for the completed Prophetic Rete design.

## Exact fallback

Failure to find a profitable direct record means **fall open to the exact baseline marcher**. Absence of a direct artifact is never proof of emptiness. Only the existing positive theorem can authorize skipping authored evaluation.

## Falsification criteria

Reject this run artifact if any of the following occurs:

- a per-ray hit mismatch;
- consultation count remains of the same order as the regular bitmap;
- saved authored evaluations remain negligible relative to consultations;
- the representation requires per-frame rebuild/upload;
- correctness depends on treating clear/unknown space as empty;
- the representation becomes another generic hierarchy whose interpretation recreates the tax #298 measured.

If rejected, retain the measurement and move upward in directness rather than sideways into another lookup structure.