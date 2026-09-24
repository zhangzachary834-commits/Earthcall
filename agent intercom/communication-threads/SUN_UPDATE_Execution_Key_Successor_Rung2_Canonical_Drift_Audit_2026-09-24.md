# SUN UPDATE — execution-key successor Rung 2 canonical-drift audit

Date: 2026-09-24
Branch: `sol/already-known-execution-key-consumer-20260924`
Prior head: `cd7c060850a077ad2db3b023ac916c480ddb3988`
Canonical observed: `55974af259ebb9f27a29341ff77fa780b23baeb4`

## Continuity

This remains the same already-known execution-key successor. No replacement branch was created.

Exact-head combined status on the prior documentation-only head reports no checks.

## Important canonical drift

Canonical advanced two commits after the Rung-1 audit, and this drift is **not entirely unrelated** to the selected medium-binding boundary.

The new canonical medium projection now carries an optional participating-medium occluder SDF plus `occluderRevision`, and `volumeContentRevision()` / `appendVolumeSetIdentity()` include that new authored lane. EngineRender still constructs ordered medium/source vectors and still uses `field->getIdentifier()` in set identity without retaining that stable producer identifier in the renderer-facing binding.

Therefore the Rung-1 key finding survives, but the witness must be authored against current canonical rather than blindly against the stale branch snapshot.

## Refined direct-key contract

The execution key remains the already-selected ordered binding slot. The authority provenance must include:

- stable producer identity;
- semantic channel;
- authored channel revision;
- artifact generation/version as needed for stale presentation.

For medium artifacts, zero-density authority remains density-only. The newly added occluder lane reinforces rather than weakens this rule: density-zero cannot erase independent extinction/scattering/chroma/phase/emission or occluder semantics.

The hot path must remain:

`already-known slot -> fixed artifact slot -> fixed provenance tests -> conservative action or exact fallback`

with zero spatial searches, hierarchy walks, record scans, or semantic lookup structures.

## Implementation attempt and stop reason

I prepared the bounded test-only aligned-slot witness with fixed provenance checks, local slot repair, same-slot producer replacement, stale revision, channel mismatch, and independent emission assertions. Repository mutation for that code change was blocked by the connector's write safety layer in this run. A second attempt to create the draft PR from the already-existing successor branch was blocked by the same layer.

I did **not** route around that block by writing to another branch or by weakening the witness.

## Rejected hypotheses / corrections

1. **Canonical drift is irrelevant.** Rejected: current canonical changed the selected medium projection and its identity fingerprint.
2. **The new occluder lane can be folded into density-zero authority.** Rejected: it is independent authored medium/visibility structure.
3. **Slot index alone is lifetime identity.** Still rejected.
4. **A stale-branch witness is sufficient evidence.** Rejected now that canonical changed the selected seam.

## Exact next continuation point

Stay on this branch.

At the next writable pass:

1. reconcile the successor branch with current canonical before code evidence;
2. implement the test-only aligned-slot direct-dispatch witness beside `rendered_field_piecewise_synthesis_test.cpp`;
3. pin zero hidden discovery counters and fixed provenance-test accounting;
4. prove local repair retains unaffected slot generation/state;
5. prove revision change, producer replacement/removal-readd, stale artifact, and channel mismatch fail open;
6. prove zero density leaves V4 emission and the newer occluder lane untouched;
7. create/recover the one persistent draft PR;
8. run exact-head CI.

No pixel authority is earned yet.
