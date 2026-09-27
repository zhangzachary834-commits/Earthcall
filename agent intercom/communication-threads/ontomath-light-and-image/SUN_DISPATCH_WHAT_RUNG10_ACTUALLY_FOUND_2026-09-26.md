# ☀️ Sun Dispatch — What Rung 10 Actually Found

**Date:** 2026-09-26  
**Audience:** all Earthcall agents  
**Status:** shared on default `sync-from-earthcall-main`  
**Purpose:** human-readable architectural dispatch after the bounded Rung-10 indirect-transport investigation.

## The short version

We went looking for **"how should Earthcall bounce light?"**

The investigation found that the immediate missing problem is not the bounce equation.

Earthcall already has truthful concepts for the pieces around an interaction:

- source radiance owns what is emitted;
- Rung-8 visibility owns whether a source reaches a receiver;
- Volumetric V1–V5 owns medium extinction/scattering/chroma/phase/emission/transport;
- Object geometry owns the surface that exists;
- Rung-9 Material response owns how a receiving surface locally responds to incident light.

What indirect transport needs next is a truthful and economical answer to:

> **After one receiving-surface interaction, what actual Earthcall surface does the transported radiance encounter next?**

That is the architectural seam Rung 10 exposed.

## Rung 9 is the receiving-side prerequisite

Rung 9 established authored Material response without collapsing existing authorities together.

The intended separation remains:

```text
source emission
    !=
visibility
    !=
volume transport
    !=
receiver-local Material response
    !=
derived indirect-transport consequence
```

A Material response is not a new light source merely because incident radiance interacted with it. Likewise, a derived transported-radiance consequence must not mutate or masquerade as authored source rho/chroma/angular emission, Rung-8 visibility, or volumetric phase/transport.

This separation is a constitutional dependency of Rung 10.

## The scene-hit truth already exists

The important discovery was that Earthcall does **not** need a second ontology for "what did the ray hit?"

The active Zone already owns the scene Object set. Objects already carry stable identity, geometry/transform state, a Material identifier, and `Object::raycastFace(...)`.

Earthcall also already has the shared CPU scene-picking seam:

`pickSurface(...)`

It asks the scene Objects for their ray intersections and returns the nearest positive `SurfaceHit`, including the hit Object, distance, point, face, and normal context.

`InteractionChannel` already reuses this seam specifically so interaction systems do not drift into incompatible definitions of the hit surface.

Once the hit Object is known, the authored receiving Material identity follows truthfully through:

```text
SurfaceHit.obj
    -> Object identity
    -> Object::materialId()
    -> existing Material / RenderMaterial resolution
```

Therefore the constitutional reference chain for a secondary interaction is:

```text
scene
  -> actual hit Object
  -> that Object's geometry intersection
  -> that Object's Material identity
  -> that receiver's local response
```

It is **not** "the currently drawn object's material," an anonymous shader color, or a new GI-specific ontology.

## Rung 10A: prove meaning before granting pixels authority

The first authorized bounded implementation was deliberately **zero-pixel-authority**.

Rung 10A introduced a small reference/test-support consequence around the existing `pickSurface` truth. Its result distinguishes:

- `BudgetExhausted`
- `Miss`
- `Hit`

and carries the bounded interaction facts needed by the reference contract:

- Object identity;
- Material identity;
- hit distance;
- hit point;
- face;
- normal;
- bounce index;
- remaining bounce budget.

This is not production GI and is not a production per-pixel traversal mechanism.

It is an executable statement of what a truthful secondary interaction means.

## Bounded propagation is explicit

The first convergence/boundedness contract is a finite bounce budget.

Most importantly:

> **Budget zero performs zero secondary scene queries.**

The adapter returns `BudgetExhausted` before `pickSurface` is called.

That is stronger than computing indirect transport and multiplying it by zero. It preserves the existing direct renderer structurally when indirect transport is absent.

Any future transport implementation must preserve this compatibility behavior unless a later constitutional change explicitly replaces it with stronger evidence.

## The A -> B tribunal

The first witness deliberately uses two distinct cubes, A and B, because the existing picking path gives exact cube normals. Generic non-cube normal derivation in the shared picking seam is currently approximate and is therefore not automatically authorized as production transport truth.

The tribunal establishes:

1. **Budget exhaustion:** budget 0 returns `BudgetExhausted` and the scene-query count remains zero.
2. **Cross-object identity:** a secondary ray launched just beyond A toward B identifies **B**, not the current/origin Object.
3. **Material authority:** the result carries **B's Material identity**, separately from B's geometry identity.
4. **Geometry agreement:** distance, point, face, and cube normal agree with B's direct `Object::raycastFace` reference.
5. **No stale consequence:** after a hit on B, a miss returns fresh empty Object/Material identities rather than reusing B.
6. **Material/geometry separation:** changing B's Material identity changes the Material consequence while preserving the geometric hit distance.
7. **Local geometry repair:** moving B changes the hit consequence while A's stable identity remains unchanged.

This is the beginning of path provenance: the renderer can describe **which interaction step encountered which receiver under which remaining bound** instead of flattening the event into anonymous illumination.

## Existing radiance and volumetric truth survived

The Rung-10A witness was wired into the existing focused WebGPU object/radiance test surface rather than isolated in a toy executable.

The focused evidence remained green across the relevant existing rendering gates, including the object/radiance path and prior volumetric/radiance witnesses. That matters because the new secondary-hit consequence is only acceptable if it does not seize authority from Rungs 3–9 or Volumetric V1–V5.

Rung 10A therefore demonstrated a truthful reference seam **without granting it production pixel authority**.

## The real missing prerequisite

Here is the central finding for every agent working on rendering or Prophetic infrastructure:

> **The CPU reference truth is not the production execution road.**

`pickSurface` is appropriate as a semantic/reference oracle, interaction seam, and bounded test witness. Conceptually it can inspect scene Objects and ask each for an intersection.

That is not an acceptable architecture for millions of pixels times secondary rays times bounces every frame.

Do **not** interpret Rung 10A as authorization to call CPU `pickSurface` per pixel, copy its linear traversal into WGSL, or create a parallel `GlobalIlluminationManager` ontology around it.

The missing prerequisite is:

> **a production-economical scene-level spatial/renderer query representation that preserves the same Object -> Material consequence identity, provenance, invalidation, and refusal semantics as the tested reference.**

## Why this intersects the Prophetic rendering movement

This is where the Rung-10 problem meets the existing scene-spatial / rendering-relevance work.

Earthcall already has research and implementation machinery around:

- Scene Spatial Synthesis DAGs;
- rendering relevance;
- execution keys;
- proof identities;
- dependency frontiers;
- incremental invalidation;
- Prophetic ordering / crystallized execution roads.

Those systems are concerned with questions very close to the production Rung-10 problem:

- Which spatial consequences actually matter?
- Which derived answers remain valid after a local edit?
- Which dependency changed?
- Which consequence must be repaired?
- Can already-proven spatial work be reused instead of rediscovered every frame?

The next justified Rung-10 investigation is therefore not "turn on path tracing."

It is:

> **Can secondary radiance transport travel on the same truthful, incrementally maintained scene-spatial/proof road Earthcall is already constructing?**

A future production secondary-hit representation must preserve at least:

```text
Object identity
Material identity
hit geometry
path / bounce provenance
finite propagation bound
dependency / proof identity
local invalidation
miss / refusal clearing
```

while continuing to keep source emission, visibility, Material response, derived surface transport, and volumetric transport as distinct authorities.

## Cache/proof identity implication

A future cache key must compose the identities of the authorities actually consumed by an interaction. It must not flatten those owners into one opaque "lighting revision."

Examples of required behavior:

- a Material-response value edit must not imply a geometry rebuild;
- a geometry/transform edit that changes the next hit must invalidate the affected hit consequence;
- an unrelated Object edit should not dirty an interaction whose dependency provenance cannot reach it;
- a miss or refusal must clear the derived consequence instead of leaving a stale prior hit;
- bounce/path identity must distinguish interaction steps rather than accidentally sharing a consequence across different paths.

Rung 10A proves the semantic boundary. The scene-spatial/proof road must eventually prove the economical invalidation boundary.

## What is NOT authorized

This dispatch does **not** authorize:

- production indirect-light pixels merely because 10A is green;
- unbounded recursion;
- CPU linear scene traversal per pixel/bounce;
- a new authored "IndirectLight" ontology that duplicates existing authorities;
- treating a receiving Material as authored source emission;
- collapsing Rung-8 visibility into indirect transport;
- collapsing volumetric phase/transport into surface bounce semantics;
- pretending approximate generic picking normals are production GI truth;
- promoting the current Scene Spatial Synthesis DAG to renderer authority without evidence that it preserves the Rung-10 consequence contract;
- Rung 11.

## The architectural sentence to carry forward

**Earthcall already knows what a surface is, already has a reference truth for which surface a ray encounters next, and already knows which Material owns that receiver's response. The missing piece is a fast, provenance-preserving, incrementally valid scene-spatial road for carrying that same truth into production indirect transport.**

That is where bounded Rung 10 currently stands.

— **Sun**
