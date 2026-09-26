# Sun — Rung 10 Indirect Transport Constitution — 2026-09-25

## Live boundary
Originally established against canonical `32c8b396275b64354c910bd8666a4800cdb75eec`; revalidated on live canonical `862bf01cb086e3177b23b7d802e3f33d83fd10fe`. Rung 9 / PR #375 remains merged. Later canonical changes inspected through this revalidation do not alter the scene-hit/material-response boundary below.

## Ownership
Indirectly transported radiance is **derived renderer state**, not a new authored ontology and not an alias/mutation of source `rho/chi/alpha`, Material-authored receiving response `f_r`, Rung-8 visibility, or volume density/extinction/scattering/phase/emission. A receiving interaction composes those truths into a transport consequence; it does not become them.

Conceptually: `L = L_emitted + T[L]`, where `T` is derived transport.

## Bounded propagation
The first truthful convergence contract is an explicit finite bounce budget. Budget 0 performs no secondary query and preserves current direct rendering exactly. Every accepted interaction consumes one unit. No hidden unbounded recursion/convergence heuristic gains authority without a later constitution.

## Provenance and cache/proof identity
A secondary step preserves prior interaction/path identity, originating receiver identity, next Object/geometry identity, next Material identity, hit position/distance/normal, incoming/outgoing directions, bounce index/budget, contributing source/path identity where applicable, and the structural/value revisions actually consumed.

Transport cache/proof identity composes these independent authorities rather than flattening them. Geometry/topology edits may invalidate hit consequences; numeric-only Material response edits must not imply geometry reconstruction. Refusal/miss/invalidation clears derived output rather than retaining stale transport.

## Production gap
The WebGPU SDF receiving path remains object-local: a draw owns one field/program and one `RenderMaterial`. A truthful secondary bounce needs a scene-level nearest-next-receiver query. Self-querying only the currently drawn field would be false scene transport.

## Rung 10A
Rung 10A is a **zero-pixel-authority scene secondary-hit query contract**. It must reuse existing scene/object truth where possible and return explicit hit/miss/refusal, Object/geometry identity, Material identity, hit distance/position/normal, path-step provenance, and sufficient dependency identity for conservative invalidation.

Falsifying witnesses: distinct A→B identifies B/B.Material; agrees with exact bounded reference; miss/refusal clears state; local geometry edit changes only affected consequence; numeric Material response edit does not rebuild geometry; budget 0 issues zero secondary queries; prior radiance/volumetric gates remain intact.

## Non-goals
No production GI pixels, multiple-bounce transport, new BRDF/Material ontology, fabricated Material Timeline, visibility/volume collapse, unproved promotion of test-only SceneSpatialDag, or Rung 11.

## Promotion gate
Only after the zero-authority 10A adapter and witnesses are green may Rung 10 investigate an economical GPU/scene-spatial representation for the same semantics.
