# Sun — Rung 10 Indirect Transport Constitution — 2026-09-25

## Live starting boundary

This constitution starts from canonical `sync-from-earthcall-main` at `32c8b396275b64354c910bd8666a4800cdb75eec`. Rung 9 is complete/landed and establishes independently authored Material receiving-surface response without collapsing source, visibility, or volume authorities.

Rung 10 is not “turn on GI.” It first establishes the missing transport consequence boundary.

## Constitutional ownership

Indirectly transported radiance is **derived renderer state**. It is not a new authored ontology kind and must not mutate or alias:

- source `rho / chi / alpha`;
- Material-authored receiving response `f_r`;
- Rung-8 visibility;
- volume density, extinction, scattering/chroma, phase, or volumetric emission.

A receiving interaction composes those independent truths into a transport consequence. The consequence may feed a later receiving interaction, but it does not become the authored authority it consumed.

Conceptually the renderer may extend the existing direct result toward `L = L_emitted + T[L]`, where `T` is derived transport, not authored source truth.

## Bounded propagation

The first truthful convergence contract is an explicit finite bounce budget.

- Budget 0 performs no secondary transport query and must reproduce the current direct renderer exactly.
- Each accepted interaction consumes one unit of budget.
- No hidden unbounded recursion, convergence-by-vibes, or shader loop may acquire authority.
- Any future convergence criterion beyond a finite budget requires separate evidence and constitution.

## Path / interaction provenance

A secondary transport step must preserve enough provenance to explain and invalidate its result:

- prior interaction / path-step identity;
- originating receiver Object / geometry identity;
- next hit Object / geometry identity;
- next hit Material identity;
- hit position and distance;
- receiving normal;
- incoming and outgoing directions in the receiver convention;
- bounce index / remaining budget;
- contributing source/path identity where applicable;
- structural/value revisions of the geometry, Material response, visibility, source, and volume authorities actually consumed.

The exact storage representation is implementation-dependent; these semantic distinctions are not.

## Cache / proof identity

Transport cache identity composes the identities of the authorities and scene consequences it consumes. It must not flatten source, visibility, Material response, and volume revisions into one false owner.

A local edit invalidates only transport consequences whose dependency provenance reaches that edit. Numeric-only Material-response changes must not imply scene-geometry reconstruction. Geometry/topology changes may invalidate hit consequences. Refused or invalid secondary queries fail open / clear derived output rather than retaining stale transport.

Existing rendering-relevance / scene-spatial proof machinery is precedent for conservative derived state and local repair, not automatic production authority.

## Critical production gap found

The current WebGPU SDF receiving path is object-local: a draw owns one field/program and one `RenderMaterial`. Rung 9 can therefore evaluate the response of the receiver currently being drawn.

Truthful indirect transport needs a scene-level secondary-ray query capable of answering: “what is the nearest next receiver, and which Object/geometry and Material does that receiver own?”

Without that seam, a naive secondary ray in the current object-local shader would silently self-query the currently drawn field and could miss neighboring receivers. That is not acceptable evidence for scene indirect transport.

## First bounded implementation rung: Rung 10A

Rung 10A is a **zero-pixel-authority scene secondary-hit query contract**.

It should reuse existing scene/object registries or scene-spatial derived machinery where truthful. It must not invent a new authored ontology merely to carry renderer-derived hit state.

Minimum result semantics:

- explicit hit / miss / refusal;
- nearest accepted hit distance and position;
- stable Object / geometry identity;
- stable receiving Material identity;
- receiving normal and directional context;
- path-step / bounce provenance;
- dependency/revision identity sufficient for conservative incremental invalidation.

Rung 10A earns later pixel authority only after exact/reference agreement and invalidation witnesses are green.

## Falsifying witnesses for 10A

1. Two distinct receivers A and B: a secondary ray launched from A toward B identifies B and B’s Material, not A’s currently drawn field.
2. The result agrees with a simple exact/reference nearest-hit calculation for the bounded witness scene.
3. Miss/refusal clears the consequence; no stale previous B hit survives.
4. A local geometry edit repairs/invalidate only affected hit consequences while unrelated consequences remain reusable.
5. A numeric-only Material response edit may invalidate response-dependent transport value, but does not rebuild scene geometry/hit structure.
6. Bounce budget 0 issues zero secondary queries and preserves exact current direct-render behavior.
7. Existing Rungs 3–9, Volumetric V1–V5, Sparkly/Antigravity reconciliation, and rendering-relevance/proof witnesses remain intact.

## Non-goals

Rung 10A does not yet authorize:

- production GI pixels;
- multiple-bounce transport;
- a new BRDF/Material ontology;
- a fabricated Material Timeline;
- collapsing visibility into transport;
- collapsing volume phase/transport into surface transport;
- promoting test-only SceneSpatialDag artifacts to production without separate evidence;
- Rung 11.

## Exact continuation

Audit the smallest existing scene/object registry and scene-spatial interfaces that can supply the 10A query without hidden global search or duplicated authored truth. If a truthful seam exists, implement the query first in test-only / zero-pixel-authority form and establish the witnesses above. If no such seam exists, define the smallest derived scene-query substrate required before indirect transport implementation is authorized.
