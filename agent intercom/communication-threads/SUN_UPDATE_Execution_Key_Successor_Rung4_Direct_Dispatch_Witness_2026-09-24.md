# SUN UPDATE — execution-key successor Rung 4 direct-dispatch witness

Date: 2026-09-24
PR: #369 — Rendering: already-known execution-key direct-dispatch witness
Branch: `sol/already-known-execution-key-consumer-20260924`
Canonical reconciled: `55974af259ebb9f27a29341ff77fa780b23baeb4`
Canonical reconciliation merge: `08428ae5272d0ba7e10caf2fa914510e7850cd39`
Authoritative code head for this pass: `34b067ac8b6dd39e7cf951e5069cec4cf9a16465`
Focused CI: run `36066729769` / #3119 — queued at handoff-writing time

## Continuity

This is the same already-known-execution-key successor. No replacement branch was created.

The earlier connector block disappeared when the user retried interactively. The existing successor branch was first reconciled onto current canonical, preserving the three prior Intercom updates, then the bounded code witness was written on that same branch. Draft PR #369 now exists.

## What changed

Two implementation changes landed:

1. `tests/singularity/rendered_field_direct_dispatch_test.cpp`
   - new test-only aligned-slot direct-dispatch witness;
   - no renderer/WebGPU authority path;
   - no spatial classification or Piecewise piece scan on the hot path.

2. `.github/workflows/earthcall-ci.yml`
   - the new witness is built and executed inside the existing SDF range-proxy verification CPU witness lane.

No production renderer transport behavior changed.

## Direct-key shape under test

The witness models the already-selected renderer binding slot as the hot-path key:

`known binding slot -> artifact[slot] -> fixed provenance comparisons -> conservative action OR exact fallback`

Each artifact entry carries:

- stable producer identity;
- semantic channel;
- exact authored channel revision;
- artifact generation;
- one deliberately tiny conservative bit: exact everywhere-literal-zero.

The dispatch path performs exactly four provenance comparisons for a valid slot:

1. producer identity;
2. semantic channel;
3. authored revision;
4. artifact generation.

It does not inspect Piecewise, classify a sample position, walk a hierarchy, scan theorem records, hash semantic structures, or query a variable-length candidate set.

Explicit accounting counters for record scans, hierarchy walks, hash probes, and spatial searches therefore remain zero by construction and are regression-asserted.

## Hostile identity/lifetime matrix

The witness rejects to exact execution for:

- same numeric slot with a different producer;
- authored revision mutation before repair;
- deliberately stale artifact generation;
- semantic channel mismatch;
- removal/re-addition represented by a new producer identity reusing the same slot.

This proves again that numeric slot alone is not lifetime identity.

## Incremental repair

The table supports a single-slot repair path.

The test mutates/repairs the source slot and asserts an unrelated neighbor slot retains exactly the same artifact generation. This is the required local-repair shape: local authored change does not globally rebuild or mint new identity for unaffected entries.

## Channel and V1–V4 sovereignty

Two separately-authored literal-zero Piecewise values with byte-identical mathematics are admitted as:

- `SourceRho`;
- `MediumDensity`.

The same zero theorem produces different channel-scoped conservative actions. Canonical math equality does not merge authority.

The density-zero action is also exercised while the actual `VolumeDensityBinding` still carries:

- an independent V4 `emissionExpr/emissionRevision`;
- the current independent `occluderSdf/occluderRevision` lane.

The witness asserts both remain unchanged. Density-zero means only density-zero support; it does not erase emission or occluder semantics.

## Economic accounting currently pinned

Before execution-backed CI, source assertions expect this bounded scenario to produce:

- 11 direct dispatch lookups;
- 44 fixed provenance tests;
- 4 conservative exact evaluations avoided;
- 7 exact fallbacks;
- 0 record scans;
- 0 hierarchy walks;
- 0 hash probes;
- 0 spatial searches;
- 1 full artifact build;
- 1 local repair;
- nonzero artifact-byte accounting;
- measured build and repair nanoseconds printed by the witness.

These are test-witness work units, not native frame/FPS claims.

## Rejected hypotheses this pass

1. **The connector still cannot write this successor.** Rejected: code writes and draft PR creation both succeeded interactively.
2. **Reuse old Piecewise query helpers.** Still rejected: those helpers classify pieces before proof use and therefore belong to the old relevance-search road.
3. **Slot alone is enough authority.** Rejected by producer-replacement and remove/re-add hostile cases.
4. **Density-zero can suppress other medium lanes.** Rejected; emission and occluder remain independent.
5. **Source inspection is enough to graduate.** Rejected. The witness is now wired into exact-head CI, and no authority promotion occurs before execution-backed evidence.

## Authority boundary

PR329's zero-authority boundary remains intact.

This pass adds only a test artifact and CI coverage. No theorem result is consumed by production renderer control flow. No pixel, ray-march, visibility, source admission, or volumetric transport path uses the witness.

## Exact next continuation point

Stay on PR #369 and this branch.

First re-read canonical and exact-head CI. Treat `34b067ac...` as the authoritative code head for run #3119 even if a later Intercom-only commit becomes branch head.

If the direct-dispatch witness compiles and executes green:

1. record its actual printed accounting;
2. compare the measured build/repair/artifact costs against the structural gate;
3. only then propose the narrow production provenance projection that preserves the already-known FieldNode stable identity across EngineRender -> renderer-facing binding admission;
4. still grant no pixel authority yet.

If CI fails, repair the witness on this same branch/PR and rerun exact-head CI. Do not open a replacement PR.

A later production projection should be narrowly scoped to **preserving identity already known upstream**, not adding a new ontology noun or a search structure.
