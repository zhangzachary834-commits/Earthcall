# SUN UPDATE — Rung 10B Secondary-Surface BVH Tribunal

**Date:** 2026-10-02  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Branch:** `sol/rung10b-secondary-surface-bvh-20261002`  
**Base at branch creation:** `sync-from-earthcall-main@ddd65445e87b48cb8687bf3a1cee280c091ec85e`  
**Pixel authority:** **ZERO**

## Why this pass exists

The Sun Zone material passes #029 and #030 isolated a rendering boundary.

- #029 substantially improved material identity: ivory can remain ivory and gold can read categorically as gold.
- #030 changed source chroma/radiance ecology, broadened gold midtones, and gave the processional stone a more polished receiver.
- The Person render after #030 remained much closer to #029 than the 4o reference would predict from those changes alone.

That is useful evidence, not permission to crank more constants.

The current SDF surface shader can author source strength/chroma/angular emission, derived visibility, and receiver-local Material response. But the current Rung-8 `sourceVisibility` query sees only the geometry owned by the currently executing SDF instance. It does not provide a scene-wide secondary-surface interaction road.

The 4o reference's remaining visual cues — especially warm/cool color exchange, luminous shadow fill, and a processional floor that appears to carry reflected architectural light — require a truthful route for radiance leaving one surface to become incident at another. More receiver polish alone cannot manufacture information about what another surface actually is.

## Inherited Rung-10A constitution

The 2026-09-26 Rung-10 dispatch already established the semantic truth:

```text
secondary ray
  -> actual hit Object
  -> that Object's geometry
  -> that Object's Material identity
  -> bounded interaction provenance
```

The CPU `pickSurface(...)` seam is the reference oracle for which Object a scene ray encounters. The dispatch deliberately granted it zero production-pixel authority because a linear scene scan is not an acceptable millions-of-pixels execution road.

The missing prerequisite was stated correctly then:

> a production-economical scene-level spatial/renderer query representation that preserves the same Object -> Material consequence identity, provenance, invalidation, and refusal semantics as the tested reference.

Rung 10B begins exactly there.

## Candidate execution road

The first candidate is a **conservative scene-level BVH** over Object world-space collision AABBs.

The BVH has only one authority:

> discover which Objects might deserve an exact query.

It does **not** become geometry truth.

At every candidate leaf the query still calls:

```cpp
Object::raycastFace(...)
```

and the nearest exact hit still determines:

- Object identity;
- Material identity;
- hit distance;
- hit point;
- hit face;
- current Rung-10A normal consequence;
- bounce index;
- remaining finite bounce budget.

AABB miss may reject a candidate because the Object's current world-space collision AABB conservatively contains that Object. AABB hit may never be treated as a surface hit.

## First tribunal

New witness:

`tests/singularity/secondary_surface_bvh_test.cpp`

It compares two roads over the same real `Object::raycastFace` implementation:

1. **Exhaustive oracle** — ask every scene Object.
2. **BVH candidate road** — traverse conservative AABBs, then ask exact Object geometry only at reached leaves.

The initial scene contains:

- one central receiver;
- 256 unrelated distractor Objects;
- stable Object identifiers;
- distinct Material identifiers.

The tribunal requires:

### 1. Budget-zero structural compatibility

A secondary budget of zero must perform:

- zero scene queries;
- zero AABB tests;
- zero BVH node visits;
- zero exact Object raycasts.

Disabling indirect propagation must therefore remain an execution absence, not “compute it and multiply by zero.”

### 2. Exact consequence parity

For the central ray and a deterministic sweep of rays through the scene, the BVH road must agree with exhaustive truth on:

- status;
- hit Object pointer/identity;
- Material identity;
- face;
- distance;
- hit point;
- normal;
- bounce provenance.

### 3. Economical candidate discovery

For the central 257-Object witness, the exhaustive road invokes exact Object raycast **257 times**.

The candidate road is required to invoke exact Object raycast at most **4 times** and therefore demonstrate at least a 32x reduction in exact Object queries on this bounded witness.

AABB/node work is reported separately rather than hidden.

### 4. Material / geometry separation

Changing only the receiver's Material identity must change the returned Material consequence without refitting geometry.

Material edits are not scene-geometry edits.

### 5. Local geometry repair

Moving only the receiver updates:

- its one BVH leaf;
- the leaf's ancestor chain.

The witness rejects a whole-scene BVH rebuild for this local transform edit.

### 6. Miss clears derived identity

After prior hits, a miss must return fresh empty Object/Material consequences. It may not retain a stale receiver from the preceding interaction.

## What success would mean

If the tribunal is green and economically meaningful, it earns only this conclusion:

> Earthcall has a candidate scene-level secondary-hit road whose discovery cost can be much smaller than exhaustive Object traversal while preserving the tested Rung-10 consequence semantics.

It does **not** yet earn:

- indirect-light pixels;
- WebGPU scene traversal;
- path tracing;
- multiple bounces;
- generic non-cube production normal authority;
- a new GI ontology;
- permission to collapse source, visibility, receiver response, or volume transport.

## What the next Sun/Sonnet should do if this tribunal is green

Do not jump immediately to a beautiful Sun Zone screenshot.

First ask whether the exact same road can be represented at the Renderer/WebGPU boundary while preserving:

- stable Object identity;
- Material identity;
- exact/conservative geometry separation;
- local invalidation/refit;
- miss/refusal clearing;
- finite bounce/path provenance;
- no O(world) per-ray fallback disguised as “GI.”

Only after a GPU/native scene-level secondary-hit witness preserves those invariants should a **bounded one-bounce pixel A/B** be proposed.

The best first visual tribunal remains the Sun Zone processional court because the 4o reference gives an unusually clear falsifier:

> polished ivory should carry reflected warm/cool scene radiance across the floor while structural ivory remains a mineral receiver and gold remains a separate reflective material.

That future visual pass must be able to turn indirect transport OFF and recover the current direct-light image exactly.

## Cultural warning

Do not answer the #030 disappointment by making every Material brighter or more reflective.

The material laws already taught the world that ivory, gold, and solar bodies are different substances.

The next missing lesson is not another color.

**It is how light travels from one substance to another.**

— **Sun**
