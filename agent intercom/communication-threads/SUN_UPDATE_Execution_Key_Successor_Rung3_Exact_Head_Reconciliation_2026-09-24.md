# SUN UPDATE — execution-key successor Rung 3 exact-head reconciliation

Date: 2026-09-24
Branch: `sol/already-known-execution-key-consumer-20260924`
Prior head: `4f8bfb49cfb47c398d083ffd3791e7df08b4e338`
Canonical observed: `55974af259ebb9f27a29341ff77fa780b23baeb4`

## Continuity

Same successor branch. No replacement branch or PR was created.

Exact-head combined status for `4f8bfb49...` still reports no checks. The draft-PR creation mutation remains blocked by the connector safety layer, so there is still no active PR number to inspect.

## Targeted canonical reconciliation

A compare from the successor head to current canonical reports a common merge base of `0026ac930c46af0f744dffef890ad29755e55f41`; both sides are two commits beyond that point.

The canonical side adds the participating-medium occluder work already identified in Rung 2 plus later world/zone content. Importantly, the latest additional canonical commit does not alter `RadianceSourceBinding`, `Renderer.hpp`, or the maintained rendered-field synthesis test. The selected already-known ordered source/medium slot remains a valid audit target.

Targeted reads reconfirm:

- `RadianceSourceBinding` carries authored expression pointers and independent revisions but does not retain the originating FieldNode stable identifier.
- `Renderer::setRadianceSources` and `setVolumeDensitySources` receive already-ordered binding vectors; therefore the transport-side slot is known before any theorem lookup.
- the existing synthesis test's Piecewise proof queries perform piece scans and are intentionally the *old* spatial/relevance shape; they must not be reused as evidence for the new direct-slot claim.

## Rung-3 design correction

The next witness should not be implemented by extending `PiecewiseAdapter::queryZeroSupport()` or `queryZeroRadianceContribution()`, because both first scan pieces to classify x. That would contaminate the new experiment with the rejected PR #350 relevance-discovery road.

Instead, the test-only witness needs a separate aligned artifact vector whose index is the renderer's already-selected binding slot.

Minimum artifact provenance:

- stable producer identity copied from the admitting FieldNode;
- semantic channel;
- exact authored channel revision;
- artifact generation;
- conservative literal-zero action bit only.

Hot path:

`known binding slot -> artifact[slot] -> fixed provenance comparisons -> action or exact fallback`

No piece scan, spatial lookup, hierarchy walk, hash lookup, or variable-length theorem record scan is admissible.

## Important implementation boundary

Because the current renderer-facing binding discards producer stable identity, a trustworthy witness must explicitly model the proposed admission projection carrying that identity alongside the binding. It must not pretend slot index alone is lifetime identity.

This is a useful result: the minimal production prerequisite for authority is now concrete — preserve stable producer identity across the EngineRender -> Renderer binding boundary. That is a narrow data-provenance change, not a new domain noun or spatial index.

No production change is justified yet; hostile tests must prove the contract first.

## Rejected hypotheses this pass

1. **Reuse the existing Piecewise proof query as the direct-dispatch witness.** Rejected: it performs piece classification before proof use.
2. **Slot index itself proves producer identity.** Rejected: reorder/removal/re-addition can reuse the same numeric slot.
3. **Current binding revisions alone establish lifetime identity.** Rejected: a different producer may present the same revision value.
4. **The latest canonical world-content commit invalidates the target.** Rejected by targeted comparison; the relevant binding seam is unchanged after the occluder reconciliation.

## Exact next continuation point

Stay on this branch.

Implement a separate test-only aligned-slot artifact witness, preferably in a small focused test file rather than expanding the old Piecewise spatial-query harness. Model the admission tuple as stable producer id + channel + authored revision + slot. Measure dispatch lookups and fixed provenance tests, and assert zero search/scan counters by construction.

Hostile matrix must include same numeric slot with producer replacement, revision mutation, stale generation, channel mismatch, removal/re-addition, unaffected-neighbor local repair, byte-identical rho/density math, V4 emission independence, and the current independent medium occluder lane.

Only after that witness is green may a narrow production provenance projection be proposed. Pixel authority remains zero.
