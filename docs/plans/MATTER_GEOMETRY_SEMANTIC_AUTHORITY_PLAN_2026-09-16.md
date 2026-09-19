# Matter Geometry / Semantic Authority Plan — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Timestamp:** 2026-09-16 ~14:20 PDT  
**Status:** PR #188 implementation complete; broader split-substrate debt remains follow-up  
**Parent audit:** `docs/audits/SHAPE_SERIALIZATION_HYDRATION_INTEGRITY_AUDIT_2026-09-16.md`  
**Repair PR:** #188  
**Follow-up issue:** #189

## Purpose

Finish the shape-hydration repair **without changing what `.ecmatter` was designed to be**.

The original split is two-domain, not one-domain-plus-cache:

- semantic Form / Relations / Laws define identity, meaning, authoring intent and the representation that exists;
- `.ecmatter` stores dense physical state used to manifest that Form;
- derived render/physics accelerations may be disposable caches, but Matter itself is not synonymous with “cache.”

The rule is therefore:

> **Form determines what the being is. Matter gives that Form physical density. Matter may hydrate a matching Form; Matter never chooses or rewrites the Form.**

## Domain assignment used by PR #188

### Semantic Form owns

- Object identity and current ShapeKind;
- named author-facing ShapeParams and append-only compatibility fields;
- Laws, Relations, authored properties and provenance surfaces;
- compact mathematical authoring recipe where lossless Matter representation does not yet exist;
- the current Field recipe and evaluation extent as a transitional necessity.

### Matter owns

- custom Polyhedron vertex/face density;
- Bezier Patch control-net density;
- physical transform/pose snapshot already carried by the Matter schema;
- other dense physical arrays as the split-substrate implementation matures (especially texture pixels and compiled/sampled fields).

### Backward compatibility may temporarily duplicate

Readers may accept older semantic `patch` / `polyhedron` payloads, but **new writes do not produce them**. Existing semantic placement fields and Material pixel Base64 are pre-existing transitional exceptions tracked separately; they are not examples to copy.

## Rung 1 — representation-aware Matter admission — DONE

### Polyhedron

- semantic kind must be `Polyhedron`;
- Matter vertices/faces are injected when the semantic Polyhedron shell lacks topology;
- an older semantic record that already embeds topology remains valid and Matter does not overwrite it;
- mismatched Matter cannot turn another semantic shape into Polyhedron.

### Bezier Patch

- semantic kind must be `Patch`;
- Matter control points hydrate a Patch shell that lacks topology;
- an older embedded semantic control net remains valid and is not overwritten;
- stale Patch Matter cannot reclassify another semantic shape.

### SmoothSurface

Current named analytic shapes reconstruct deterministically from semantic ShapeKind + ShapeParams, so their Matter payload is usually redundant today. If physical smooth topology is missing and Matter is consulted:

- the semantic kind must be one of the analytic smooth kinds;
- exact model/form/parametric-kind agreement is required;
- valid Torus Matter cannot hydrate into a Sphere shell, and vice versa.

### Field

Field remains transitional because current Matter stores one root only.

- semantic kind must be `Field`;
- if a complete semantic Field recipe already exists, Matter may not replace tree or extent;
- if only a Field shell exists, validated root-only Matter may hydrate a leaf representation;
- non-leaf operators are refused because children are absent;
- Convex roots are refused because planes are absent;
- Expr roots must contain text that actually compiles.

## Rung 2 — validate physical topology before mutation — DONE

FlatBuffers verification is structural, not semantic. PR #188 adds:

### Polyhedron checks

1. finite vertices;
2. at least one start/end offset pair;
3. first offset exactly zero;
4. final offset exactly `face_data.size()`;
5. monotone, non-negative, in-range face spans before any `end - start` allocation;
6. at least three indices per face;
7. every vertex index within the vertex array.

### SDF checks

- primitive and operation ordinals range-checked before conversion;
- finite dimensions, offset and scalar values;
- finite positive extent;
- lossy root-only structures refused;
- Expr source must compile.

### SmoothSurface checks

- model/form/parametric ordinals range-checked;
- exact semantic-kind agreement;
- exactly 16 finite matrix values;
- finite axes, trim and parameters.

## Rung 3 — regression witness for the actual split — DONE

`shape_hydration_integrity_test` now proves the substrate contract directly:

1. Patch semantic JSON contains Patch identity but **not** its control net.
2. Polyhedron semantic JSON contains Polyhedron identity but **not** vertices/faces.
3. `buildMatterFlatBuffer` carries the dense topology.
4. semantic hydration creates representation shells.
5. `applyMatterFlatBuffer` fleshes matching shells out.
6. stale Matter cannot reclassify current semantic identity.
7. complete Field tree/extent cannot be demoted by stale Matter.
8. hostile topology / enum / Expr payloads are refused.

This is the key correction from the earlier draft of this plan: “semantic JSON is independently complete for all dense topology” is **not** the desired architecture. The two substrates are intentionally complementary.

## Rung 4 — preserve compatibility without fossilizing duplication — DONE FOR PR #188

The reader keeps support for historical/transitional semantic Patch/Polyhedron payloads. This lets saves produced by older builds or by the earlier draft of this branch continue loading.

The writer does not emit those dense payloads, so compatibility does not become the new canonical format.

## Rung 5 — complete the original split beyond this PR — FOLLOW-UP

### 5A. Move dense Material pixels back to Matter

`Material::toJson()` still writes `FaceTexture` pixels as Base64 semantic text. That contradicts the original split and recreates the exact bloat `.ecmatter` was designed to solve.

Do not simply re-enable old Matter paint overwrites: preserve Material identity, copy-on-write paint semantics, shared Material ownership and the Basic Pixel Changer precedence guarantees. Prefer a Matter blob/reference owned by the Material being rather than duplicating the same pixel array per Object.

### 5B. Add per-being Matter handles / form revisions

Generation-level `matterGeneration` already protects the root+Matter file pair. Add an append-only per-being reference/fingerprint layer:

- canonical semantic Form revision/hash;
- Matter handle or record identifier;
- optional physical schema/version;
- loader verifies match before injection;
- mismatch refuses/regenerates physical data instead of changing semantic Form.

### 5C. Decide placement's final representation under the Property graph

Transform/center/axis/target-rotation currently live in both semantic Object state and Matter. The newer serialization architecture permits primitive/mat4 Property variants in semantic state, while Matter can carry physical snapshots. Decide whether these are authoring properties, physical snapshots, or both with explicit coherence—not by deleting one path because it looks duplicated.

### 5D. Complete Field Matter

Current root-only Field Matter cannot carry recursive SDF/OntoMath/Convex structure. Extend the physical substrate with a lossless representation or compiled field bytecode/buffer referenced from semantic Form.

## Other non-goals / follow-ups

This PR does not:

- reinterpret `ShapeParams.r` as RoundedBox size;
- finish all persisted geometry integer boundaries;
- rewrite the historical Law implicit producer;
- regenerate the Cathedral artifact;
- author the Cathedral's standing-wave OntoMath expression.

Those remain separate contracts.

## Final architecture

The end state is not:

`semantic JSON contains everything -> Matter is optional cache`

and not:

`Matter payload exists -> Matter decides what the Object is`

It is:

**semantic Form / Relations / Laws -> verified matching Matter -> manifestation**

with optional derived accelerations layered underneath. This preserves both halves of Earthcall's split-substrate design.