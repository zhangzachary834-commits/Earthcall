# SUN UPDATE — PR #350 Rung 2 direct-dispatch boundary verdict

Date: 2026-09-23
PR: #350
Branch: `sol/rendering-relevance-economics-rung1-20260923`
Canonical observed this pass: `67bb9d0cd446c557bedd01c7d9544d7b6f89ec2a`
PR head audited: `01ecf1d73f5ce25f33690bd2f910da3cd058a6ee`

## Continuity and base drift

This is the same post-#329 relevance-economics successor thread and the same PR #350. No new branch or PR was created.

Canonical advanced from the PR's original base `423cfd69...` to `67bb9d0c...` through unrelated #349 palette UX work. The relevance-economics branch remains the active successor; this pass did not manufacture a rebase merely to absorb unrelated UI drift.

## Bounded implementation/audit result

I audited the exact Rung-2 question against the production observer and the maintained Piecewise synthesis witness.

The decisive finding is **negative for the single-authored-Perlin per-sample marcher, positive for already-keyed semantic boundaries**.

The current Perlin skip theorem answers a spatial question: whether the current ray sample lies in a region for which exact field evaluation may be skipped. At an arbitrary march step the renderer necessarily knows the sample position, but it does **not** already possess a semantic region/piece/theorem slot identifying the applicable proof. Producing that slot requires one of the mechanisms Rung 1 measured as the tax: range-grid traversal, direct-run scanning, hierarchy traversal, or equivalent spatial classification.

Therefore a supposed direct-dispatch table for the individual Perlin march cannot be indexed without first rediscovering spatial relevance. Moving the result into a fixed table does not remove the query; it merely moves the query in front of the table. This candidate is rejected before pixel authority.

By contrast, the production `RenderedFieldSemanticObserver` demonstrates a genuinely already-keyed boundary: the caller already knows the source/medium binding and channel before observation. A future stable source/medium execution slot could index conservative semantic action metadata in O(1) without asking a spatial relevance question. That is an admissible consumer family, but it is **not** evidence for accelerating the maintained Perlin horizon march.

## Why I did not add a fake test-only Perlin dispatch table

The previous continuation requested the smallest test-only direct-dispatch witness *if* an already-known key could be named. This audit establishes that no such key exists for the maintained per-sample Perlin theorem without spatial classification.

Adding a table indexed by a synthetic test-only integer would prove only that array indexing is O(1). It would omit the real production cost of deriving that integer and would therefore violate the relevance-economics mission. I reject that hypothesis rather than committing benchmark theater.

## Authority and semantic invariants preserved

- PR329 remains zero-authority: no theorem result changes pixels or shader control flow.
- Exact authored mathematics remains sovereign and fail-open.
- No production shader/march path changed in this pass.
- `SourceRho` and `MediumDensity` remain distinct channels even when canonical math is byte-identical.
- Density support cannot erase independently authored V4 self-emission.
- V1 extinction, V2 scattering/chroma, V3 phase/directionality, and V4 emission remain independent semantic channels; no cross-channel theorem authority was introduced.

## Rejected hypotheses

1. **Fixed table keyed by sample position.** Rejected: position is not a table slot; mapping position to theorem applicability is the spatial query.
2. **Direct-run/range-grid key derivation before dispatch.** Rejected: this is precisely the relevance discovery Rung 1 found uneconomic.
3. **Synthetic test-only slot as evidence for Perlin.** Rejected: it removes the expensive operation from the experiment rather than eliminating it architecturally.
4. **Raw `Piecewise*` + revision as future authoritative identity.** Rejected for authority: address reuse/removal/re-addition hostility remains unresolved. It is diagnostic identity only.

## What changed

Documentation/economic classification only. No renderer authority or authored math changed.

The important change is the search-space reduction: **stop trying to promote the current spatial Perlin theorem into a per-sample direct-dispatch consumer.** It cannot satisfy the no-hidden-search gate on this workload.

## Exact next continuation point

Stay on PR #350.

Rung 1's three-arm comparator is settled, and the maintained single-authored-Perlin relevance economics are now settled negatively for generic per-sample proof consultation: the exact field samples saved do not repay relevance discovery, and there is no already-known semantic execution key that removes that discovery for the horizon march.

The next bounded pass should close PR #350's evidence package: re-read exact-head CI and the native Rung-1 measurements, reconcile only if current canonical drift intersects these files, and update the PR summary with the final two-part verdict:

1. dormant proof-capable shader topology does not justify a production NO-PROOF fork;
2. active per-sample Perlin proof consultation is economically rejected for this workload.

Do **not** invent a new source/medium optimization inside this task. A separately measured already-keyed semantic consumer is future scope and must earn promotion independently.
