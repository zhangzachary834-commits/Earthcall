# SUN HANDOFF — Make the Earth Inhabitable: Universal Singular Property Persistence

Date: 2026-09-21  
Repository: zhangzachary834-commits/Earthcall  
Source branch: `sol/singular-semantic-persistence-20260919`  
Source branch head before this handoff: `c0057440d154fd1e98d7e040d5552e57883efda5`  
Current default head at handoff time: `4ab5ebe25aaac99915d25d63770960a2e085a34b`

## READ THIS FIRST

Do **not** restart the investigation.

Also read:

`agent intercom/communication-threads/PR_53_Temporal_Rollback_Incident_2026-09-19.md`

before modifying or transplanting anything.

The implementation on the source branch is now **454 commits behind current default**. The source branch was clean when created, and the intervening default commits are later concurrent Earthcall work, but the ancestry gap is now too large to merge this branch directly.

**DO NOT MERGE `sol/singular-semantic-persistence-20260919` AS-IS.**

Create a fresh branch from the current `sync-from-earthcall-main` head and transplant/reconcile the implementation semantically, file by file. Do not overwrite current-default files wholesale with stale branch copies. Use targeted diffs/ranges and preserve newer default work.

The two commits that first moved default past our old base were a Go migration and task-doc updates; since then default has advanced dramatically through many later renderer/visibility/etc. merges. Treat current default as authoritative.

## User requirement that governs this work

The save -> fresh hydration loop must work for **every persistable Singular and its meaningful Properties**, not merely for ordinary Objects or one demo path.

The intended invariant is:

> A Property does not survive because someone remembered to hand-add that concrete C++ subclass to a serializer. It survives because it is meaningful semantic state of a real Singular, unless its concrete root already owns a stronger canonical persistence representation.

This is part of `Make_the_Earth_Inhabitable` P0.

Do **not** touch Person current-Zone / active-Zones / join/leave semantics yet. Zach is still deciding that ontology. This branch intentionally did not redesign those semantics.

## What existed before

Object serialization already persisted authored/dynamic Properties.

The wider audit showed that this was not a universal Singular guarantee. Person, Relation, Material, Zone/Formation and other roots could carry Law-authored state at runtime while their serializers had no shared obligation to preserve it.

The old `PropertyValue` JSON codec also had major continuity holes:

- identity-valued alternatives (Singular*, Object*, Relation*, Formation*) wrote IDs but hydrated as monostate;
- OntoMath ScalarField / VectorField values wrote only type tags rather than their actual payload;
- hydration order could therefore erase identity-valued semantic state before referenced beings existed.

## Architecture implemented on the source branch

### 1. Semantic writability is explicit

`Property` now exposes:

`isSemanticallyWritable()`

The shared persistence layer only treats a registered Property as generic semantic state when it can actually be restored.

Implemented behavior includes:

- PropertyRef<T>: writable when T is representable by PropertyValue;
- setter-backed ComputedProperty<T>: writable;
- read-only/derived computed Properties: not generic persistence inputs;
- authored DynamicPropertyBridge: writable;
- custom bridges were audited and marked appropriately.

Object-specific custom bridges were made context-sensitive where necessary so a dormant Field/Patch surface does not emit impossible fallback state.

### 2. Universal Singular semantic envelope

New shared files:

- `src/Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp`
- `src/Singularity/Storage/Serialization/Common/SingularPropertySerialization.cpp`

The envelope handles:

- `authoredProperties`
- `registeredProperties`
- Zone designations

Authored/dynamic Properties are first-order semantic state and are always preserved.

Registered Properties use the envelope as a **fallback persistence home**, not automatically as a second authority.

### 3. Canonical root truth outranks fallback truth

This became a critical architectural rule after focused tests caught stale duplicate state.

If a concrete root already serializes a Property canonically, the universal envelope must not compete with it.

Generic rule now implemented:

- if a concrete root JSON already contains a top-level field with the same registered Property name, the root field is authoritative;
- the fallback writer skips the duplicate;
- the fallback reader ignores an old/stale duplicate.

Custom mapped ownership exists where PropertyPath names differ from the canonical JSON structure.

#### Object

Object has an explicit `objectRegisteredPropertyNeedsEnvelope(...)` policy.

Canonical Object/Material/Matter-owned state is excluded from fallback authority, including:

- position / rotation / transform
- center
- authoritativeAxis
- targetRotation
- rotationResponsiveness
- material
- x2D / y2D / zOrder2D
- renderMode
- textString
- color
- textureResolution
- `shape.*`
- `field.*`
- `patch.*`
- `face.*`

Uncovered registered state such as telos / physical / rigid-form-style state can still use the envelope.

This same Object authority policy is reused by:

- ordinary Object
- BodyPart primary visual Object
- ObjectConcept

Why this matters: before the filter, stale fallback `shape.*` values replayed after canonical hydration and could resurrect an obsolete shape. Existing `shape_hydration_integrity_test` caught this.

#### Relation

Relation now has `relationRegisteredPropertyNeedsEnvelope(...)`.

Exact canonical names such as type/directed/weight are already handled by the generic same-name rule.

`attachment.*` is explicitly excluded from fallback authority because those PropertyPaths map into the nested canonical `attachment` JSON object.

### 4. Preserve-first, bind-later identity Properties

`Singular` now carries hydration-only pending raw JSON for unresolved authored and registered semantic Properties.

This is analogous to Relation saved endpoint IDs.

If a saved Property names another Singular that does not exist yet:

- preserve the raw JSON;
- do **not** turn it into monostate;
- do **not** guess a pointer;
- re-emit the exact raw payload if the world is saved again before binding;
- once the target exists, bind through a load-scoped resolver and remove the pending copy.

Deferred raw truth intentionally outranks default live values on re-save until successful resolution.

Pending semantic values also survive Singular copy/move.

### 5. PropertyValue round-trip was upgraded

`PropertyValueJson` now supports resolver-aware hydration.

Implemented continuity includes:

- int
- float
- double
- bool
- char
- long
- string
- vec3
- mat4
- PropertyList
- PropertyDict
- Singular*
- Object*
- Relation*
- Formation*
- OntoMath ScalarField
- OntoMath VectorField
- typed null references
- typed null field pointers

Identity references persist stable identifier + reference kind and bind only when a resolver can find the target.

Lists/dicts resolve recursively, so a reference nested inside authored structured state is not exempt from identity continuity.

OntoMath fields now persist their real `toJson()/fromJson()` payload rather than an empty type tag.

### 6. Two-phase graph hydration

Reference-valued Properties cannot depend on JSON order.

Post-load binding was added where the whole relevant graph becomes available.

Important cases:

- Zone graph: after members / Lexemes / Objects / Relations exist, deferred Properties bind through the same Zone endpoint resolver used by Relations;
- LawManager: after all authored Laws are installed, Law-to-Law and world references can bind before Rete compilation;
- ConceptRegistry: concept-to-concept references bind after all concepts exist;
- FirstMover register: mover-to-mover references bind after the whole register exists.

External references that still cannot resolve remain preserved, not erased.

### 7. Lexeme-only Zone bug fixed

The previous formation-hydration function returned early when a Zone had no `formationRelations`.

That meant a Zone could serialize Lexemes but fail to hydrate them if it happened to have zero relations.

Member/Lexeme hydration is now independent of relation-record presence, and deferred Zone Property resolution still runs.

## Persistence roots / nested Singulars wired

The shared envelope is wired through the real persistence boundaries for:

- Object
- ObjectConcept
- Person
- Soul nested in Person
- Joys Formation nested in Person
- Body nested in Person
- BodyPart
- BodyPart primary Object
- Relation
- Formation
- Material
- Zone
- Home
- FieldNode
- Zone-interned Lexeme
- Law
- Ourverse
- FirstMover

Do not infer from this list that every possible future Singular subtype automatically has an independent file format. Moment/Event/Utterance, for example, did not have a standalone persistence codec during the audit; do not invent one merely to satisfy a checklist.

## Tests added

### `tests/singularity/singular_property_persistence_matrix_test.cpp`

This is the codec/root matrix.

It exercises every PropertyValue alternative and multiple real root codecs.

It also includes adversarial stale-fallback witnesses.

Examples:

- canonical Object shape state must outrank stale fallback `shape.*`;
- canonical Material `baseColor` must outrank a deliberately poisoned stale `registeredProperties.baseColor`;
- canonical Relation attachment state must outrank stale `attachment.enabled` fallback state.

### `tests/zones/zone_singular_property_roundtrip_test.cpp`

This is the important real-path witness.

It:

1. creates a Zone;
2. creates object.a and object.b;
3. authors scalar / structured / OntoMath state;
4. gives object.a an authored Object* Property pointing at object.b;
5. places another Object* reference inside a nested PropertyDict;
6. persists through the real Zone identity store;
7. destroys the writer graph;
8. creates a fresh ZoneManager;
9. hydrates from disk;
10. proves the references bind to the **newly hydrated object.b**, not the old pointer.

This test is not merely a JSON unit test.

## CI evidence

Latest branch head at the time this handoff was prepared:

`c0057440d154fd1e98d7e040d5552e57883efda5`

Latest Earthcall focused CI:

Run: `35493303849`

Focused CPU result: **SUCCESS**

Focused set: **34 / 34 tests passed**

Decisive witnesses on the latest green run:

- `object_roundtrip_test` — PASS
- `shape_hydration_integrity_test` — PASS
- `singular_property_persistence_matrix_test` — PASS
- `zone_singular_property_roundtrip_test` — PASS

The same run's Slow Adapter job failed only its unrelated performance threshold:

Chess:

- adapter off / direct off median: 9.957916 ms
- adapter on / direct on median: 12.683333 ms
- ratio: 1.27
- diff: +2.725417 ms
- workflow threshold: ratio <= 1.20 OR diff <= 1.0 ms

This branch did not implement Slow Adapter / Law-Direct performance changes. Do not attribute that perf failure to Singular persistence without new causal evidence.

## Important CI incident during this work

A workflow-editing accident temporarily duplicated a large tail of `.github/workflows/earthcall-ci.yml`.

Root cause was **JavaScript replacement-string semantics**: the literal sequence `$'` in the ctest regex was interpreted by `String.replace(..., replacementString)` as "insert the suffix after the match," duplicating the remainder of the workflow.

This was caught by diff-size auditing, not merged.

The workflow was then restored byte-for-byte from the exact base Git blob and modified safely. The final intended workflow delta was tiny: add the two new persistence tests to focused build/run selection.

Lesson for the next Sun:

- never trust a giant workflow diff when intent was four lines;
- use exact Git blob bytes for workflow restoration;
- use callback replacement functions when replacement text contains JavaScript `$` substitution sequences;
- keep the PR_53 rollback-era provenance discipline.

## Base divergence warning — CRITICAL

At the end of this work, current default had advanced to:

`4ab5ebe25aaac99915d25d63770960a2e085a34b`

The source branch was:

- 57 commits ahead of its old ancestry;
- **454 commits behind current default**;
- about 31 files different when handoff docs are counted.

The implementation itself was green on its own branch, but that does NOT prove it is green after 454 commits of current-default evolution.

### Correct next move

1. Read the PR #53 rollback incident.
2. Create a **fresh branch from current `sync-from-earthcall-main`**.
3. Use the old branch only as a source of semantic changes.
4. Transplant/reconcile implementation file by file against current versions.
5. Do not copy stale whole files over current default.
6. Re-run the focused witnesses on the fresh branch.
7. Re-run the real Zone identity roundtrip witness.
8. Audit current-default equivalents before assuming an old change is still necessary.
9. Only then open/refresh a PR.

Do not merge the old source branch directly.

## Remaining bounded audit before calling the architecture closed

The canonical-vs-fallback authority audit was completed explicitly for:

- Object / ObjectConcept / BodyPart primary Object
- Material
- Law
- Person / Body
- Zone / Home
- Relation
- Formation

The handoff happened while beginning the last weird-root overlap sweep.

The next Sun should finish a **bounded** check of:

- Lexeme
- FieldNode
- FirstMover
- Ourverse

Question for each:

> Does a writable registered Property already have a canonical persistence representation under a different JSON name/nesting?

If exact same-name canonical fields exist, the generic same-name rule already handles them.

If a mapped/nested representation exists (like Object `shape.*` or Relation `attachment.*`), add one root-specific filter rather than letting two authorities coexist.

Do not broaden this into a giant serializer rewrite unless current-default evidence requires it.

## Architectural invariant to preserve

There are now three classes of semantic state:

1. **Canonical concrete-root state**  
   The root codec owns it. This wins.

2. **Universal registered-property fallback state**  
   Used only when no stronger canonical persistence home exists.

3. **Authored/dynamic Property state**  
   First-order authored world state; always preserved by the shared envelope.

And unresolved identity values obey:

> Preserve first. Bind later. Never erase merely because hydration order has not created the target yet.

## Person/Home boundary already established before this branch

The earlier Make-the-Earth-Inhabitable work already established:

- every admitted Person must have at least one primary Home;
- zero-Home admission attempts repair and then refuses if still zero;
- duplicate primary Homes satisfy the existential "at least one" invariant but unique resolution still refuses ambiguity;
- authoritative owned-by Relations outrank stale compatibility owner text;
- primary Home continuity is kernel guarded.

Do not redo that work.

## Explicitly deferred ontology

Do not redesign or normalize:

- Person current Zone
- Person active Zones
- Zone presence
- joinZone / leaveZone semantics

Zach explicitly paused that part to think through the ontology.

## Where this sits in Make the Earth Inhabitable

This branch addresses the universal semantic-memory part of P0:

> a being and its Properties must survive leaving and returning.

After this transplant is green on current default, the larger P0 frontier remains Zone-native whole closure:

- Materials closure
- Categories closure
- Zone-scoped Matter generations
- atomic/preflighted whole-closure activation/save
- migration of remaining authored apps away from legacy World dependencies

The canonical acceptance loop remains:

Home -> meaningful edit -> Save Zone -> terminate process -> fresh boot -> same Home -> same Singular identities -> same Properties -> same Relations/Laws/Materials -> continue authoring.

The point is not merely that one demo survives.

The point is that Earthcall can make the promise generically:

> If a Person truly authored semantic state onto a real Singular, crossing the temporal boundary does not silently make that meaning cease to exist.
