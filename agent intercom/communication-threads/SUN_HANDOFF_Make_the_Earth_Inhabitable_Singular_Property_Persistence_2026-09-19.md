# SUN HANDOFF — Make the Earth Inhabitable: Universal Singular Property Persistence

**Date:** 2026-09-19 (America/Los_Angeles)  
**From:** GPT-5.6 Sol — The Sun  
**To:** the next GPT-5.6 Sol continuing Earthcall  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Branch:** `sol/singular-semantic-persistence-20260919`  
**Current branch head:** `a88be0b7cd8a2250993b80e80ddaf31ece60fe74`  
**Current default/base observed at handoff:** `ee3d3054547b7cd44ffbdc7ce6e9f93fe995e536`  
**Branch relation at handoff:** diverged, 55 commits ahead / 2 commits behind, 30 changed files, +1325/-137.  
**Latest CI at this exact head:** Earthcall focused CI run **#1283 / run 35485283102** — **Focused CPU tests: SUCCESS; Slow Adapter independent clock: SUCCESS.**  
**PR:** not opened yet. Finish the bounded audit below, sync the two harmless base commits, reverify, then open a checkpoint PR if still clean.

---

## 0. READ THIS FIRST — ROLLBACK / PROVENANCE SAFETY

Before doing any more implementation, read:

1. `AGENTS.md`
2. `agent intercom/communication-threads/PR_53_Temporal_Rollback_Incident_2026-09-19.md`
3. `docs/Agenda/Tasks/Specific Tasks/Make_the_Earth_Inhabitable/Make_the_Earth_Inhabitable.md`
4. `docs/Agenda/Tasks/Specific Tasks/Singular_Serialization_Topology/Singular_Serialization_Topology.md`
5. `docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md`

Do **not** start over.

This branch began from the corrected modern line after the temporal rollback incident. The branch later became two commits behind current base only because base advanced concurrently:

- `3cd9fcf36924f3f59cbd842056b4238993117bdc` — “migrated Go”
  - touched Go authored saves/scripts/tests and task docs
- `ee3d3054547b7cd44ffbdc7ce6e9f93fe995e536` — “to do list updated”
  - touched task docs only

At handoff, neither base commit overlaps this branch's implementation files. Still, **sync current base before PR and re-run CI**. Do not merge/rebase blindly if base has advanced again.

Also: Zach is still thinking through the ontology of **Person current Zone / active Zones / Zone joining/presence**. This branch intentionally does **NOT** decide that architecture. Do not wander into it.

---

## 1. THE USER REQUIREMENT THAT CHANGED THE TARGET

Zach explicitly corrected the inhabitability plan:

> The save/restart loop already works for some things. Make sure it works for **every Singular and their Properties**.

The target therefore became stronger than “an Object round-trips.”

The architecture being implemented is:

> A Property should not survive persistence merely because somebody remembered to add it to one concrete serializer. If it is a real semantic Property of a persistable Singular, Earthcall should have a mechanically checkable persistence path for it, unless that Property is derived/read-only or explicitly belongs to another canonical substrate.

The user also requires **bounded retrieval**:
- no whole giant files unless absolutely necessary;
- no giant CI logs;
- search exact symbols, fetch exact ranges;
- on CI failure: job status → failed step → exact error/test lines only.

“NO BIG CHUNGUS” is not a joke for workflow purposes. The connector can time out and terminate the message stream.

---

## 2. WHAT IS ALREADY IMPLEMENTED

### 2.1 Property semantic writability

The base `Property` interface now exposes whether a serialized semantic value can honestly be written back.

Files:
- `src/ConstructedBeing/Singular/Property/Property.hpp`
- `PropertyRef.hpp`
- `ComputedProperty.hpp`
- custom bridges in Material, Lexeme, FieldNode, ObjectProperties

Rules:
- ordinary `PropertyRef<T>` for PropertyValue-legible T: writable;
- setter-backed `ComputedProperty<T>`: writable;
- read-only/derived computed properties: not included in the fallback semantic envelope;
- custom bridges opt in only when they have a real restoration path.

Object custom bridges were tightened so “writable in principle” does not automatically mean “meaningful to persist in the current object state.”

Examples:
- Patch controls participate only when a Patch exists;
- Field controls participate only when the Field state makes them meaningful;
- paint properties avoid read-only layer-count/texture-size surfaces.

This is persistence metadata only. It does not change normal Law authorability.

### 2.2 PropertyValue JSON is now identity-aware and payload-complete

Files:
- `src/ConstructedBeing/Singular/Property/PropertyValueJson.hpp/.cpp`

Previously:
- `Singular*`, `Object*`, `Relation*`, `Formation*` wrote an id and then loaded as `monostate`;
- OntoMath `ScalarField` / `VectorField` wrote effectively only a type tag.

Now:
- reference alternatives write stable id + pointer-kind;
- a resolver-aware decoder can restore the correctly typed pointer after the target being exists;
- ScalarField and VectorField serialize their real payload;
- lists/dicts recursively preserve/resolve nested identity references;
- typed null pointers round-trip as typed nulls;
- null OntoMath shared_ptr alternatives round-trip honestly.

Do not resurrect raw pointer addresses. Identity is stable identifier → resolver → current live being.

### 2.3 Preserve-first / bind-later hydration state on Singular

Files:
- `src/ConstructedBeing/Singular/Singular.hpp/.cpp`

A Singular can now hold hydration-only raw JSON for:
- unresolved authored Property values;
- unresolved registered Property values.

This is deliberately analogous to Relation’s saved endpoint IDs.

Rule:

> If the referenced identity is not live yet, preserve the exact raw semantic value. Do not collapse it to none, do not guess, and do not let a default value overwrite it on the next save.

Deferred raw truth wins over placeholder/default live values until resolution succeeds.

The pending maps survive Singular copy/move. They are **storage-phase state, not authored world state** and are not exposed as Law-addressable hidden semantics.

### 2.4 Universal Singular semantic property envelope

New files:
- `src/Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp`
- `src/Singularity/Storage/Serialization/Common/SingularPropertySerialization.cpp`

Core API:
- `writeSingularProperties(...)`
- `readSingularProperties(...)`
- `resolveDeferredSingularProperties(...)`

The envelope carries:
- `authoredProperties` — dynamic/Person/Law-granted Properties;
- `registeredProperties` — writable PropertyValue-legible registered state that does not already have a more authoritative persistence home;
- `designatedZones`.

The generic serializer has an optional **registered-property inclusion predicate**, because the fallback envelope must never become a second canonical serializer.

### 2.5 Critical anti-double-truth rule

This was discovered by real regressions, not theory.

The first universal writer naïvely serialized canonical Object state a second time under `registeredProperties`. Then stale values like `shape.*` replayed after canonical Object hydration and could overwrite newer JSON.

That broke:
- `object_roundtrip_test`
- `shape_hydration_integrity_test`
- BodyPart invalid-shape behavior

The corrected doctrine is now:

> **Concrete/root codec = authority for state it already owns.  
> Universal Singular envelope = fallback persistence home for registered state with no other canonical home.  
> Authored/dynamic Properties = always envelope state.**

Generic protection:
- if a top-level JSON key has the exact same name as a registered Property, the concrete root field wins;
- fallback writer skips that property;
- fallback reader ignores stale same-name registered copies.

Custom mapped protection is still needed where Property names map into nested/different canonical fields.

Already implemented:

#### Object filter
Declared in:
- `src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp/.cpp`

`objectRegisteredPropertyNeedsEnvelope(name)` excludes canonical Object/Material/Matter vocabulary such as:
- pose/transform/center/rotation;
- material;
- 2D/text/render state;
- color / textureResolution;
- `shape.*`;
- `field.*`;
- `patch.*`;
- `face.*`.

The envelope still carries uncovered registered state such as telos, physical, rigid-form dynamics, and future properties lacking another persistence home.

Reused by:
- ordinary Object;
- BodyPart primary Object semantic record;
- ObjectConcept.

#### Relation filter
Declared in:
- `src/Singularity/Storage/Serialization/Relation/RelationSerialization.hpp/.cpp`

`relationRegisteredPropertyNeedsEnvelope(name)` excludes:
- `attachment.*`

because those Properties map into the canonical nested `"attachment"` JSON object.

Exact `type`, `directed`, `weight`, `events` overlap is handled by the generic same-name rule.

### 2.6 Concrete persistence roots already wired

The shared envelope is wired into real persistence boundaries including:

- Object
- ObjectConcept (important: its custom serializer bypasses ordinary Object to_json)
- Person
- Soul nested in Person
- Joy Formation nested in Person
- called Lexeme nested in Person
- Body
- BodyPart
- BodyPart primary Object
- Material
- Relation
- Formation
- Zone
- Home through Zone/Home codec
- FieldNode
- Law
- Ourverse
- FirstMover
- Zone-interned Lexemes

Do not fabricate serializers for types that do not currently cross a persistence boundary. During the audit, Moment/Event and Utterance had no independent toJson/fromJson persistence root; we intentionally did not invent one merely to make a matrix look complete.

### 2.7 Two-phase identity rebinding

Reference-valued Properties must not depend on file order.

Second-phase binding exists in the natural graph-completion points:

#### Zone graph
`FormationSerialization.cpp` now:
- hydrates members/Lexemes even when the Zone has zero Relations;
- includes Relations in endpoint identity resolution;
- after the Zone closure exists, walks the closure and runs `resolveDeferredSingularProperties`.

This fixed another preexisting bug:
> a Zone containing Lexemes but no `formationRelations` could write Lexemes and then never hydrate them.

#### LawManager
After all authored Laws exist, deferred Law/world property references are rebound before continuous-law Rete compilation.

#### ConceptRegistry
After all ObjectConcepts exist, concept-to-concept references can bind without file-order dependence.

#### FirstMoverRegister
After all FirstMovers exist, mover-to-mover references bind. External world references remain preserve-first deferred for broader world resolution.

---

## 3. TEST WITNESSES

### 3.1 Universal Property matrix

New:
- `tests/singularity/singular_property_persistence_matrix_test.cpp`

This exercises the real semantic contract, not merely JSON syntax.

It covers every current PropertyValue alternative:
- none
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
- ScalarField
- VectorField

Identity references are checked after deferred rebinding.

It also round-trips representative real roots:
- Object
- Material
- Relation
- Formation
- Zone
- Home
- FieldNode
- Law
- ObjectConcept
- FirstMover
- Person
- Soul
- Body
- Ourverse
- Lexeme

The test now includes adversarial stale-fallback poison checks:
- Material canonical `baseColor` must beat a conflicting stale `registeredProperties.baseColor`;
- Relation canonical nested `attachment.enabled` must beat a conflicting stale `registeredProperties["attachment.enabled"]`.

Object stale-shape authority is covered by existing shape hydration guards.

### 3.2 Real Zone identity-store fresh hydration

New:
- `tests/zones/zone_singular_property_roundtrip_test.cpp`

This is the important “not just codec unit test” witness.

Writer graph:
- Zone
- object A
- object B
- A has an authored Object* Property pointing to B
- A has a nested PropertyDict containing B
- A has authored ScalarField payload
- Zone/Object carry registered inherited state such as telos

Then:
1. write through the real Zone identity store to an isolated SaveRoot;
2. destroy the writer graph;
3. create a fresh ZoneManager;
4. hydrate only from the Zone store;
5. prove A and B are fresh instances;
6. prove A’s authored Object* now points to the **new hydrated B**, not the old pointer;
7. prove nested reference and field payload survive.

This test passed.

### 3.3 Focused CI

Both new tests are explicitly included in:
- focused build targets;
- focused CTest regex.

Do not remove them from CI.

Important latest signals:

At commit `401d53b1...`, after the Object authority fix:
- `object_roundtrip_test` — PASS
- `shape_hydration_integrity_test` — PASS
- `singular_property_persistence_matrix_test` — PASS
- `zone_singular_property_roundtrip_test` — PASS
- focused suite: **34/34 PASS**

After the generic same-name hardening + Relation attachment filter + adversarial Material/Relation checks, exact current head `a88be0b7...` ran Earthcall focused CI #1283 and:
- Focused CPU tests — **SUCCESS**
- Slow Adapter independent clock — **SUCCESS**

So the current head is green.

---

## 4. IMPORTANT CI / TOOLING INCIDENT — DO NOT REPEAT

While adding the two test names to `.github/workflows/earthcall-ci.yml`, there was a temporary workflow corruption caused by JavaScript replacement-string semantics.

The focused CTest regex ends in the literal characters:

`$'`

In JavaScript `String.replace(search, replacementString)`, the sequence `$'` means:

> insert the entire suffix after the match

Therefore a replacement string containing the literal shell regex ending `$'` duplicated the rest of the workflow, producing a terrifying +112/+113-line phantom delta and invalid YAML.

This was fully repaired.

Recovery used the exact raw Git blob of current base, then replacement via a **callback function**, where `$'` stays literal.

Current workflow diff against base is sane:
- additions: 3
- deletions: 1

That corresponds to two added build-target lines and replacement of the one focused-regex line.

If you ever edit that regex again:
- DO NOT pass a replacement string containing `$'` directly to JS String.replace.
- Use `.replace(needle, () => replacement)` or another literal-safe edit.
- Verify workflow diff size immediately. A four-line intention must not become +100 lines.

This incident did **not** contaminate application source and the repaired workflow now parses and passes.

---

## 5. EXACT PLACE TO RESUME

I stopped during the final bounded **duplicate-authority audit** after completing Object, Material, Law, Person/Body, Zone/Home, Relation, and Formation.

The next four weird roots being inspected were:

1. Lexeme
2. FieldNode
3. FirstMover
4. Ourverse

Use targeted symbol searches only:

- `void Lexeme::buildProperties`
- FieldNode registered properties / its `toJson/applyJson/fromJson`
- `void FirstMover::buildProperties`
- `void Ourverse::buildProperties`

The question for EACH is only:

> Does a writable registered Property already have an authoritative canonical field elsewhere in that root’s codec?

If exact same-name:
- generic same-name suppression already solves it.

If non-exact mapping:
- add a tiny root-specific inclusion predicate like Object or Relation.

If no canonical persistence home:
- KEEP it in the universal envelope.

Do NOT solve this by disabling registered persistence generally. The whole point is universal semantic coverage without duplicate authority.

Known search results at handoff:
- Lexeme has writable `symbol` and `ConceptualWeightProperty`.
- FirstMover has its own `buildProperties` in `src/Identity/FirstMoverRegister.cpp`.
- Ourverse has `buildProperties` in `src/ZonesOfEarth/Ourverse/Ourverse.cpp`.
- FieldNode custom bridges are already marked semantically writable and FieldNode is already envelope-wired; audit whether any bridge duplicates canonical AST fields.

After these four:
1. add an adversarial stale-fallback test only if a real non-exact duplicate is found;
2. run the focused branch CI again;
3. inspect only failed step/error lines if red.

---

## 6. THEN SYNC CURRENT BASE

At handoff the branch is 2 commits behind `ee3d3054...`.

Those two known commits do not overlap implementation files, but base may advance again. Before PR:

1. inspect current latest base;
2. compare the commits since the branch’s known base;
3. if no overlapping architectural changes, sync/rebase/merge in the repository’s preferred safe manner;
4. re-run focused CI;
5. compare branch against corrected current base for deletion waves / rollback contamination.

Do not trust a branch simply because tests are green. PR #53 taught us that stale-tree resurrection can still produce superficially plausible code.

Expected implementation scope is currently about:
- 30 changed files
- +1325/-137
- workflow only +3/-1

If sync suddenly produces hundreds of thousands of line changes, STOP.

---

## 7. WHAT NOT TO TOUCH

Do NOT decide or implement yet:
- Person “current Zone”
- active Zones
- the ontology of presence
- Zone join/leave semantics beyond existing behavior

Zach explicitly said he is thinking through that architecture before we implement the Zone-joining portion of inhabitability.

Also do not start a parallel serialization framework. This branch is deliberately connecting existing concrete codecs through one shared Singular semantic envelope.

---

## 8. WHAT THIS BRANCH DOES / DOES NOT CLAIM

It DOES claim, subject to the final four-root audit:

- authored/dynamic Property state no longer belongs only to Object;
- registered semantic state has a general fallback persistence path;
- reference-valued Properties preserve identity instead of collapsing to none;
- unresolved references survive hydration order and re-save;
- OntoMath PropertyValue payloads survive;
- concrete codec truth is protected from stale fallback duplicates;
- real Zone identity save → fresh graph hydration reconnects identity-valued authored Properties.

It does NOT claim:

- every piece of all Singular metadata has been universally normalized;
- every possible Singular type has an independent serializer;
- the full “Make the Earth Inhabitable” programme is finished;
- Zone current/active presence semantics are solved;
- whole Zone closure transactions / all Materials-Categories-Matter closure work is finished.

This is the **universal Singular Property persistence rung** inside P0 inhabitability.

---

## 9. WHY THIS MATTERS TO MAKE THE EARTH INHABITABLE

The programme’s boring loop is:

> make a meaningful change → save → quit → fresh process → same world

Before this branch, that promise depended too much on concrete C++ kind:
- Object authored Properties had special support;
- other Singular roots often did not;
- identity-valued Properties could serialize an id and then reload as nothing;
- OntoMath field PropertyValues could lose their payload;
- hydration order could erase references;
- and naïvely universalizing registered Properties introduced a new danger: two serialized truths fighting over the same semantic state.

The emerging invariant is now:

> **A real semantic Property of a persistable Singular either has one explicit canonical persistence home or falls back to the universal Singular envelope. Never zero homes. Never two authorities.**

That is the rung we are finishing.

When it is complete, “the thing is still there” means more than an Object’s mesh remained. It means the authored semantic state by which Laws know what the thing is remained too.

---

## 10. HANDOFF CHECKLIST FOR THE NEXT SUN

- [ ] Read PR #53 rollback incident doc and AGENTS instructions.
- [ ] Confirm branch head / current base; do not assume the SHAs above are still latest.
- [ ] Resume at Lexeme / FieldNode / FirstMover / Ourverse registered-vs-canonical audit.
- [ ] Add custom exclusion only for real non-exact duplicate authority.
- [ ] Preserve authored/dynamic Properties unconditionally.
- [ ] Preserve unresolved identity values raw; never silently convert to none.
- [ ] Keep concrete codec canonical over fallback duplicates.
- [ ] Run only targeted CI/status/log retrieval.
- [ ] Ensure both new persistence tests remain in focused CI.
- [ ] Sync current base after audit.
- [ ] Run focused CI after sync.
- [ ] Compare diff for rollback-sized anomalies.
- [ ] If clean, open a checkpoint PR / mark ready for review.
- [ ] Leave Person current/active Zone ontology untouched.

---

**Signed:** GPT-5.6 Sol — The Sun  
**Continuation principle:** Do not start over. Continue from the green universal-property architecture at `a88be0b7...`, finish the four-root authority audit, sync current base, reverify, then PR.
