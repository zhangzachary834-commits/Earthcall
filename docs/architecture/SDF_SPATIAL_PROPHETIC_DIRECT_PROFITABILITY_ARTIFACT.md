# SDF Spatial Prophetic: Direct Profitability Artifact

Status: first direct-run candidate implemented and falsified in PR #301; retain as a test-only witness, not a production optimization.

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

## PR #301 empirical verdict

The first candidate has now been run on the same maintained authored-Perlin witness in CI run **35668839695**, commit **11ce8ca293ca6d532d9c4c9a07fa648f57c65635**. All focused jobs passed, including zero direct-vs-OFF per-ray hit mismatches and zero recurring proof uploads.

That means the experiment is **correct enough to judge**. It does not mean the representation is profitable.

The direct-run arm does cut the visible query count from roughly 4.7-5.2 generic consultations per ray to roughly one artifact query per ray. But the hidden discovery work remains too large.

### Horizon camera

Generic baseline:

- 83,649 generic consultations;
- 144 exact sample steps saved;
- 0.001721 samples saved per consultation.

Most favorable headline direct candidate (\`z\`, minimum run 1):

- 16,015 artifact queries;
- 144,135 individual run-record tests;
- 139 exact sample steps saved;
- 0.008679 samples saved per artifact query, about **5.04x** the generic baseline;
- 0.000964 samples saved per record test, only about **0.56x** the generic baseline.

Best candidate when **both** query economics and record-discovery economics are charged (\`x\`, minimum run 2):

- 1 retained record / 48 bytes;
- 16,014 artifact queries;
- 16,014 record tests;
- 65 exact sample steps saved;
- 0.004059 samples saved per query and per record test;
- only **2.36x** the generic consultation economics.

### 45-degree camera

Generic baseline:

- 74,686 generic consultations;
- 157 exact sample steps saved;
- 0.002102 samples saved per consultation.

Most favorable headline direct candidate (\`z\`, minimum run 1):

- 15,924 artifact queries;
- 143,316 individual run-record tests;
- 183 exact sample steps saved;
- 0.011492 samples saved per artifact query, about **5.47x** the generic baseline;
- 0.001277 samples saved per record test, only about **0.61x** the generic baseline.

Best candidate when both costs are charged (\`x\`, minimum run 2):

- 1 retained record / 48 bytes;
- 15,874 artifact queries;
- 15,874 record tests;
- 91 exact sample steps saved;
- 0.005733 samples saved per query and per record test;
- only **2.73x** the generic consultation economics.

### Verdict

The positive-run representation is **rejected for production**.

It preserves correctness and is tiny, but it fails the intentionally hard ~10x profitability bar. More importantly, the minimum-run-1 cases demonstrate the exact danger this experiment was built to expose: reducing top-level "queries" can merely hide a larger scan of candidate records underneath them.

Do not mutate the production renderer to consume these run records.

Retain the diagnostic because it is now a reusable falsification harness for the next direct artifact.

The next rung must move upward in directness: runtime needs a stable dispatch key that selects only the already-proved relevant consequence set, with no scan over the global possibility set. A candidate such as an AOT conservative ray-route dispatch atlas may be explored test-only, but it must prove its own discovery cost and must not become a disguised DDA, tree walk, or per-frame rebuild.

The governing requirement is now sharper:

> A direct road is not "a smaller structure to search." It is a precompiled answer to which structure matters.

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