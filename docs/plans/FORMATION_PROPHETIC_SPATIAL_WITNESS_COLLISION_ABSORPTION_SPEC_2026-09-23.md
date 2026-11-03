# Formation + Prophetic Rete Spatial-Relation Witness Architecture

**Date:** 2026-09-23  
**Status:** Proposed implementation specification; architecture grounded in the live repository at canonical head 273d101be714dced356a988c29197d65284ee9a2.  
**Scope:** Replace the privileged legacy collision orchestration path with Formation-Rete candidate routing, Prophetic-Rete impossibility proofs, and exact OntoMath/geometry spatial witnesses, while preserving Law ownership of meaning and response.  
**Primary affected areas:** src/ZonesOfEarth/AuthorsOfLaw/, src/Relation/, src/Singularity/OntoMath/, src/ConstructedBeing/Singular/Object/Geometry/, src/ZonesOfEarth/Physics/, tests/law/, tests/zones/.  
**Companion architecture:** docs/architecture/law/FORMATION_RETE.md, docs/architecture/law/PROPHETIC_RETE.md, docs/architecture/interrelations/FORMATION_RETE_AND_ONTOMATH.md, docs/architecture/law/LAW_MIGRATION_FRAMEWORK.md, docs/architecture/law/DERIVED_STATE_LEDGER.md, docs/architecture/mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md, docs/architecture/SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md.

---

## 0. Decision

Earthcall should retire collision as a privileged engine subsystem.

The replacement is not another CollisionSystem. It is a general spatial-relation witness path:

    authored Law demand
        -> Formation Rete decides which beings/pairs are relevant candidates
        -> Prophetic Rete proves which candidates are impossible now or until a frontier
        -> exact OntoMath/geometry witness evaluates the remaining spatial relation
        -> Rete exposes the sensed fact / transition
        -> authored Laws decide what that fact means and what changes follow

The machine may retain irreducible geometric sensing algorithms. It must not retain a privileged semantic rule that overlap means collision, that collision means impulse response, or that all spatial beings must be pair-tested every simulation step.

The decisive conceptual split is:

- OntoMath / geometry answers: what spatial relation is mathematically true?
- Formation Rete answers: which possible relations are relevant enough to consider?
- Prophetic Rete answers: which possible relations are provably impossible, and until when?
- Law answers: what does that relation mean in this world?
- Singularity actuation commits the authored result to state.

This follows AGENTS.md directly: no subsystem defines what a thing is; the substrate senses and acts, while Persons author meaning from visible primitives.

---

## 1. Why this is a migration of an existing seam, not a greenfield subsystem

The repository already contains almost every conceptual piece.

### 1.1 The current collision path is visibly legacy

src/ZonesOfEarth/Physics/Physics.hpp already states that Physics is not a first-order property of Zones and that real physics should ultimately become runtime-created Law Formations.

The live hardcoded path still performs all of these responsibilities together:

1. build world-space AABBs;
2. sort them by min-x;
3. sweep candidate pairs;
4. reject on y/z;
5. choose a narrowphase through Physics::dispatchCollision;
6. compute normal, penetration depth, and contact point;
7. move the two Objects apart;
8. clip velocity along the contact normal;
9. mint collision / contact-began / contact-ended events.

That is too much authority in one subsystem. Sense, Decide, and Act are fused.

### 1.2 Overlaps already names the correct Law-level concept

ConditionNode::Overlaps exists today.

Its current implementation resolves the second participant, dynamic-casts both participants to Object, and returns Physics::dispatchCollision(a, b).hit.

This is the exact seam to invert. Law already asks for geometric truth. The implementation should stop reaching down into a Physics-owned black box and instead consume the general spatial witness substrate.

### 1.3 Prophetic Rete already records the architectural wound

PropheticRete.cpp currently handles ConditionNode::Overlaps by setting opaqueReads = true and recording:

    opaque read: Overlaps consults the collision test

This is not merely a performance limitation. It is the formal symptom that collision is outside the legible dependency graph.

The completed migration must make Overlaps structurally legible enough that one spatial condition no longer forces the global Prophetic index to fail open.

### 1.4 Formation Rete already has the routing pattern

LawManager already selects cached candidate tiers using world structure, property vocabulary, retained adapter roads, and Law-Direct roads.

RelevanceTraversal explicitly states the safety contract this work must preserve: routes say where to look; they never decide that a Law holds.

Spatial candidate routing must obey the same rule. A stale or over-wide route may cost work. It must never create a false negative.

### 1.5 OntoMath already has the mathematical proof substrate

OntoMath::Interval already provides outward-rounded conservative interval arithmetic, joins, meets, overlap tests, and the fail-open treatment required for proof-bearing optimization.

Geometry has already been unified toward OntoMath as canonical mathematics, and the SDF spatial-prophetic work has established a crucial precedent:

- proofs may authorize skipping only when they prove impossibility;
- unknown regions fall open to exact evaluation;
- derived acceleration structures are not sources of truth;
- incremental repair is mandatory;
- a more elegant index is rejected if its runtime consultation economics are bad.

Collision migration must inherit those lessons instead of inventing a second spatial-optimization doctrine.

---

## 2. What is being removed, and what is not

### 2.1 The target of deletion

The target is the privileged collision orchestration embodied by:

- Physics::updateBodies pair discovery and contact-resolution loop;
- CollisionZone as the authoritative broadphase concept;
- Physics::dispatchCollision as the public semantic entry point for overlap;
- hardcoded positional separation;
- hardcoded normal-velocity clipping;
- g_touchingPairs pointer-pair state as the authoritative contact history;
- the assumption that collision is a built-in physical meaning.

### 2.2 Mathematical kernels are not automatically forbidden

SAT, GJK, EPA, signed-field evaluation, support mappings, interval bounds, gradients, and exact or conservative geometric solvers are mathematical Sense machinery.

They are admissible as invariant substrate when they answer a mathematical question and do not decide world meaning.

Therefore the first migration must NOT begin by rewriting every narrowphase algorithm in authored Law.

The near-term target is:

    Physics::dispatchCollision
        -> neutral exact spatial witness interface

The long-term target is for the witness implementation to consume canonical OntoMath-backed geometry directly and for any leftover duplicate geometry math in Physics to disappear.

### 2.3 The word physics must not become the new root abstraction

The spatial witness layer must not live conceptually under a Physics authority.

Preferred ownership:

- canonical mathematical forms: Singularity/OntoMath and existing geometry representations;
- exact spatial witness adapter during migration: Object/Geometry or another neutral Singularity Sense location;
- candidate/proof scheduling: Formation and Prophetic Rete;
- physical response, if desired: authored Laws.

---

## 3. Canonical runtime model

The runtime object is a **spatial relation demand**, not a collision job.

A demand is derived from authored condition text or another explicit consumer. At minimum the first implementation must support ConditionNode::Overlaps.

Conceptually:

    SpatialDemand
      predicate: overlap
      left participant constraint
      right participant constraint
      exact-witness requirements
      dependency provenance
      consumer Law / condition branch

This is derived execution data, not a new persisted world ontology and not a new BeingKind.

For an explicit condition:

    subject overlaps B

the right participant is statically known by stable identifier. Formation routing therefore has no excuse to sweep arbitrary B candidates.

For future generalized pairwise authored relations, the pair source may be a Formation, retained relation road, event participants, or another explicit authored grouping. Do not reintroduce an implicit all-pairs universe scan as the normal form.

---

## 4. Exact spatial witness contract

Introduce a neutral exact witness API. The exact name may be adjusted to fit directory doctrine, but its semantics are fixed by this specification.

A first adapter can wrap the existing dispatcher while migration proceeds.

Suggested derived result:

    SpatialWitness {
        bool holds;
        float signedSeparation;
        float penetration;
        vec3 normal;
        vec3 point;
        WitnessKernel kernel;
    }

Requirements:

1. holds answers only the requested mathematical relation.
2. signedSeparation is negative for penetration when an exact signed measure is available, zero at contact, positive when separation is exactly available; unknown may remain absent rather than guessed.
3. normal and point are witness geometry, not a response command.
4. penetration is derived geometry, not permission to move anything.
5. kernel is diagnostic derived metadata only, equivalent to today's CollisionMethod; it must never become authored ontology.
6. unsupported geometry fails open to the most exact available fallback or reports no exact witness. It must never manufacture separation as proof.

Initial implementation may internally reuse:

- polyhedron SAT;
- GJK/EPA;
- signed implicit-field probing;
- support-point logic.

But these functions must move toward a neutral geometric namespace and must not depend on Physics::RigidForm, gravity, collision-law targets, or response state.

ConditionNode::Overlaps must eventually call this neutral witness API rather than Physics::dispatchCollision.

---

## 5. Broadphase becomes Formation candidate routing plus conservative spatial support

Traditional broadphase answers:

    Which pairs might overlap?

Earthcall's answer must be split into two independent filters.

### 5.1 Semantic/relevance narrowing — Formation Rete

Formation Rete answers:

    Which participants are relevant to this authored demand at all?

Use the existing hierarchy of candidate sources:

- explicit Law targets;
- exact named participant;
- vocabulary membership;
- authored category / Relation roads;
- Slow Adapter retained roads;
- Law-Direct style crystallized roads.

A spatial demand is not permitted to bypass these and recreate a whole-world pair sweep merely because geometry is involved.

For ConditionNode::Overlaps with a literal other identifier, pair identity is already exact. The candidate route is one pair per possible subject, not all pairs in the world.

For event-scoped @event.subject / @event.object conditions, the event already supplies the pair.

For a future authored category-to-category interaction, the categories/Relations define the candidate populations. Spatial proof then narrows inside that authored scope.

### 5.2 Geometric possibility narrowing — conservative support

For every spatial being participating in a demand, derive a conservative support envelope from canonical geometry.

The first representation may be an axis-aligned interval box:

    SpatialEnvelope {
        Interval x;
        Interval y;
        Interval z;
    }

This is not the old CollisionZone ontology. It is a disposable proof object derived from canonical geometry and transform state.

The broadphase theorem is simple and sound:

    if A.x and B.x are disjoint
       or A.y and B.y are disjoint
       or A.z and B.z are disjoint
    then overlap is impossible.

Only that negative conclusion may suppress exact witness evaluation.

A support envelope may be looser than the old AABB. It may never be narrower than the true geometry.

### 5.3 No new per-frame global sort unless measurement earns it

Do not rebuild the old sweep-and-prune under a new filename.

The desired steady state is direct candidate consumption from authored/rete structure plus incrementally maintained conservative support.

A global sort may survive temporarily as a baseline oracle and fallback during migration. It is not the target architecture.

---

## 6. Prophetic Rete owns impossibility and temporal validity

Prophetic Rete must learn the dependencies of Overlaps rather than marking them opaque.

### 6.1 First legibility rung

At minimum, analyze Overlaps as reading:

- the left participant's spatial form / geometry parameters;
- the left participant's transform / position;
- the right participant's spatial form / geometry parameters;
- the right participant's transform / position;
- any authored field parameters used by either form.

The exact property names must be taken from registered PropertyPaths, not invented duplicate state.

If a relevant geometry path cannot yet be enumerated, Overlaps remains locally fail-open. Do not turn one unknown spatial leaf into global Prophetic incompleteness when its known dependencies can still be recorded.

This should follow the UnknownWriteSource philosophy already present in Prophetic Rete: preserve positive knowledge about the known portion instead of erasing the entire graph.

### 6.2 Current-state impossibility proof

Once support envelopes are available, Prophetic Rete may prove:

    overlap(A, B) = impossible at the present state

That proof suppresses the exact witness until one dependency of either support envelope changes.

This already eliminates repeated narrowphase calls for static separated pairs without requiring future-time reasoning.

### 6.3 Temporal frontier

The stronger rung is:

    overlap(A, B) cannot become true before Moment M

This may only be produced when authored / sensed motion can be conservatively bounded.

Important current limitation: Prophetic::Range treats vector values such as vec3 as Top.

Therefore temporal spatial proof requires one of these explicit substrate extensions:

A. extend Prophetic::Range with a Vec3 kind carrying three OntoMath::Intervals; or  
B. introduce a separate SpatialRange companion that is built from legible component values and transformations.

Do not pretend current scalar Range can prove vector motion.

A temporal frontier is valid only if every transform-affecting dependency is accounted for. If a direct C++ mutation bypasses the property dirty feed, the proof must be invalidated through an explicit geometry/transform revision or the temporal rung must fail open.

### 6.4 Prophetic rule remains unchanged

The analysis may only conclude IMPOSSIBLE.

Possible means evaluate or preserve the candidate.

Unknown means evaluate or preserve the candidate.

A stale negative proof is unacceptable; a stale positive candidate is merely slower.

---

## 7. Narrowphase becomes exact witness evaluation scheduled by Rete

The old narrowphase is not replaced by "Rete math."

Rete decides when an exact mathematical question is worth asking.

The exact witness kernel answers it.

That distinction matters.

The new execution ladder for one pair is:

    1. Formation route says pair is semantically relevant.
    2. Prophetic support proof says either:
         a. impossible -> do nothing until invalidated/frontier;
         b. possible/unknown -> continue.
    3. Exact spatial witness evaluates overlap.
    4. Rete records current sensed truth and its transition.
    5. Law re-evaluates its full condition before acting.
    6. Authored response acts through ordinary property/action machinery.

The exact witness result must be cached against its dependencies. If neither participant's relevant geometry/transform revision changes, repeated exact evaluation is unnecessary.

---

## 8. Contact truth, transitions, and Law visibility

The migration must preserve all three useful forms of contact information:

1. **level truth** — are these two participants overlapping now?
2. **positive edge** — did overlap become true now?
3. **negative edge** — did overlap cease now?

The current system already distinguishes collision level from contact-began/contact-ended edges. Preserve that semantic distinction.

### 8.1 Stable pair identity

Do not use pointer ordering as canonical pair identity.

Use stable participant identity, preferably the repository's stable identifier/Singular identity discipline.

Derived pair keys must be deterministic across runs and reloads.

### 8.2 Working-memory representation

The preferred first representation is a derived Rete spatial state fact carrying both participants.

It is:

- not serialized;
- not a new persisted Relation by default;
- retracted when the witness becomes false;
- asserted/updated when it becomes true;
- keyed by stable participant identity and predicate kind.

Transition events may then be synthesized from fact insertion/retraction exactly once.

### 8.3 Numeric witness data must become legible before response migration

A realistic authored collision response needs more than a Boolean.

At least point, normal, penetration/signed separation, and participant identities must be reachable by Law before hardcoded resolution is deleted.

Do not solve this by inventing a new domain-specific Contact class.

Two acceptable routes:

- attach the current witness as derived read-only context to the Rete/event application environment; or
- materialize a temporary existing Relation being with read-only derived witness properties and the two participants as endpoints.

Which route is chosen must obey NO_BLACK_BOX and the existing event/referent model. If event context cannot expose the numbers without adding a new hidden payload channel, prefer an existing Relation with registered read-only properties.

---

## 9. Authored response replaces hardcoded collision response

The following code in Physics::updateBodies is semantic policy and must migrate:

- position correction based on penetration;
- the special ground-vs-non-ground branch;
- halving correction for two movable bodies;
- velocity normal-component removal;
- any assumption that every overlap receives the same physical response.

The engine may expose invariant actuation primitives and writable properties such as position, velocity, mass, or other existing bridges.

A seed Law Formation may reproduce today's familiar response for compatibility.

But a Person must be free to author worlds where overlap means:

- bounce;
- stop;
- merge;
- pass through;
- damage;
- trigger sound;
- mint a Relation;
- transform either participant;
- do nothing.

The spatial witness layer must not know which of these is correct.

---

## 10. Migration ladder

This work must follow LAW_MIGRATION_FRAMEWORK: move authority incrementally and preserve a fallback until parity is proved.

### Rung 0 — Freeze the legacy path as oracle

No semantic change.

Add measurement and parity instrumentation around:

- legacy sort-and-sweep candidate count;
- legacy dispatchCollision exact calls;
- exact hit pairs;
- contact-began/contact-ended pairs;
- updateBodies collision-loop wall time.

Capture representative scenes at several populations and shape mixes.

Exit gate: reproducible baseline plus existing tests green.

### Rung 1 — Neutral exact witness adapter

Create the neutral spatial witness API but initially wrap the existing dispatcher.

Change ConditionNode::Overlaps to consume the neutral API.

Physics::updateBodies may still call through it.

No response behavior changes yet.

Exit gate:

- collision_dispatcher_test parity;
- continuous_law_test Overlaps parity;
- exact pair corpus shows identical hit truth;
- no Physics dependency remains in ConditionModel.

### Rung 2 — Make Overlaps Prophetic-legible

Teach PropheticRete::analyzeCondition to record known spatial dependencies for Overlaps instead of unconditional opaqueReads.

Introduce conservative support-envelope derivation.

Unknown geometry remains local fail-open.

Exit gate:

- one Overlaps Law no longer forces unrelated property writes through global Prophetic incompleteness;
- geometry and transform edits invalidate the spatial read;
- deliberately unsupported geometry preserves exact fallback;
- tests confirm no false negative.

### Rung 3 — Formation-Rete spatial candidate route

Compile spatial demands into cached candidate pair roads.

For the first implementation, support the pair shapes already expressible today:

- subject vs literal being;
- event subject vs event object;
- explicit target Formation vs literal/event counterpart.

Shadow-run this candidate set against legacy sweep-and-prune.

Exit gate:

- every pair that legacy broadphase sends to exact narrowphase and every exact hit required by an authored spatial demand is present in the new candidate set;
- stale/unknown route widens or falls back;
- stable pair-key determinism test passes;
- sparse scenes materially reduce exact witness calls before legacy broadphase is removed.

### Rung 4 — Prophetic impossibility cache and future frontier

Cache negative current-state proofs by dependency revision.

Then, where motion bounds are fully legible, add a temporal validity frontier.

Do not require temporal proof for all pairs before shipping current-state proof.

Exit gate:

- unchanged separated pairs stop re-running exact witness;
- relevant transform/shape change wakes only affected pair proofs;
- unknown motion destroys the frontier and falls open;
- no whole-world proof rebuild on one local edit;
- derived-state ledger row and red invalidation test exist.

### Rung 5 — Law-owned response

Expose numeric witness data to Law.

Author a compatibility Law Formation reproducing the current basic positional/velocity response closely enough for parity scenarios.

Run legacy response and Law response A/B behind an explicit migration switch.

Exit gate:

- compatibility scenarios match within declared floating-point tolerances;
- custom Law test proves the same overlap can intentionally produce a non-physical consequence;
- contact begin/end edges remain exactly-once;
- disabling the compatibility Law yields no hidden engine correction.

### Rung 6 — Delete privileged collision orchestration

After Rungs 0-5 are green and measured:

- remove legacy pair sweep from Physics::updateBodies;
- remove hardcoded collision response;
- remove Physics::dispatchCollision public authority;
- retire CollisionZone if no remaining consumer requires it;
- retire g_touchingPairs as authoritative contact state;
- update PHYSICS_AND_COLLISION.md to historical/migrated status;
- keep only mathematical sensing kernels that remain irreducible and correctly located.

Exit gate: full suite + benchmark gate + save/load + deterministic contact-state tests.

---

## 11. Derived-state and invalidation ledger

Every new cache introduced by this work must declare the standard triple: derived from, invalidated by, rebuilt where.

Minimum structures:

### Spatial support envelope

Derived from:
- canonical geometry structure;
- geometry parameters;
- transform.

Invalidated by:
- geometry structure revision;
- geometry parameter revision;
- transform/position/orientation/scale mutation.

Guard:
- mutate each input independently and prove the envelope changes or widens.

### Candidate pair road

Derived from:
- Law text / condition revision;
- Universe structural revision;
- Relation graph generation;
- Slow Adapter candidate generation where used;
- explicit participant identity.

Invalidated by:
- the same currencies already used by LawManager candidate routes.

Guard:
- add/remove target, category member, Relation, or participant and prove route refresh.

### Negative overlap proof

Derived from:
- both support envelopes;
- any temporal motion range used;
- predicate definition.

Invalidated by:
- any dependency above.

Guard:
- start disjoint, cache impossible, then move or resize exactly one participant into overlap and prove the exact witness wakes.

### Exact witness cache

Derived from:
- both canonical geometry/transform revisions;
- requested predicate.

Invalidated by:
- either participant's relevant revision.

Guard:
- unchanged pair does not reevaluate; either-side edit does.

### Spatial state fact

Derived from:
- exact witness truth.

Invalidated/retracted by:
- a new exact witness false result;
- participant removal/unmaking;
- world/Zone replacement.

Guard:
- began once, level persists without duplicate edge, ended once, participant deletion produces no dangling event.

Add these rows to DERIVED_STATE_LEDGER.md when implementation lands.

---

## 12. Correctness invariants

These are merge-blocking.

1. **No false-negative candidate pruning.** Any uncertainty widens.
2. **No stale negative proof.** Missing invalidation is correctness failure.
3. **Exact condition recheck remains authoritative.** Candidate roads never decide Law truth.
4. **Unknown geometry fails open.**
5. **No response occurs in the witness layer.**
6. **No hidden per-frame all-pairs path returns under another name.**
7. **Pair identity is deterministic and not pointer-order based.**
8. **One local spatial edit does not require whole-world proof reconstruction once the incremental rung is declared complete.**
9. **No serialized derived acceleration state becomes a second source of truth.**
10. **Law can observe every numeric witness value needed by the compatibility response before engine response is deleted.**

---

## 13. Test plan

### Existing tests that must remain green

- tests/zones/collision_dispatcher_test.cpp
- tests/law/continuous_law_test.cpp
- tests/law/prophetic_rete_test.cpp
- related Formation/Rete scaling and invalidation tests
- geometry/OntoMath parity tests

### New focused tests

1. spatial_witness_parity_test
   - compare neutral witness against legacy dispatcher across polyhedron/polyhedron, convex implicit, field, complex, transformed, and non-contact pairs.

2. spatial_candidate_soundness_test
   - shadow-run legacy broadphase and Formation candidate roads; assert no required pair omitted.

3. spatial_prophetic_invalidation_test
   - prove separated pair impossible; mutate A transform, B transform, A geometry, B geometry; each must invalidate.

4. spatial_prophetic_fail_open_test
   - unsupported/opaque geometry or motion never suppresses exact evaluation.

5. spatial_contact_fact_test
   - exactly one begin edge, persistent level truth, exactly one end edge.

6. spatial_pair_determinism_test
   - identical stable pair key and contact ordering across object allocation order differences.

7. spatial_law_response_test
   - overlap drives an authored response using witness normal/depth; disabling the Law disables the response.

8. spatial_nonphysical_semantics_test
   - overlap intentionally authors a non-physics consequence, proving the witness does not impose collision meaning.

9. spatial_incremental_repair_test
   - one object edit invalidates only its dependent pair proofs/caches, not unrelated pairs.

10. spatial_zone_reset_test
    - Zone/world replacement retracts all derived spatial facts and leaves no dangling participant pointers.

---

## 14. Performance measurement and graduation rules

Performance is not assumed from architectural elegance.

Measure at minimum:

- population N;
- number of authored spatial demands;
- Formation candidate pairs;
- support-envelope consultations;
- Prophetic negative proofs hit;
- exact witness evaluations;
- witness cache hits;
- invalidated pair count per edit;
- collision/spatial scheduling wall median.

Use sparse and dense scenes.

A rung does not graduate merely because top-level query count falls. The SDF spatial-prophetic work already demonstrated that a smaller lookup can hide a more expensive candidate scan.

Required interpretation:

- charge every record/route consultation;
- count exact mathematical evaluations actually avoided;
- report warm steady state separately from mutation frames;
- retain legacy A/B until the new path wins on both correctness and economics.

A practical production gate for broadphase removal is:

- zero semantic mismatches;
- zero missed required hit pairs;
- no wall-time regression at small N;
- clear exact-witness reduction at larger sparse N;
- local-edit invalidation proportional to affected relations rather than world size.

Do not hard-code a claimed speedup before measurement. If the new representation is slower, keep the proof/candidate architecture and reject the consumption representation, exactly as the SDF direct-profitability work did.

---

## 15. Interaction with scene-spatial synthesis

The rendering investigation already reached the conclusion that the valuable spatial theorem should attach to the compiled branch that owns the relevance relation rather than be rediscovered through an external generic lookup.

Collision/contact should follow the same direction.

Long term, the scene-spatial execution DAG may become the shared mathematical support graph for:

- rendering relevance;
- overlap/contact relevance;
- field containment;
- light/occlusion relations;
- density-volume intersection;
- other exact spatial predicates.

This specification does NOT require blocking collision migration on the renderer DAG.

But it forbids building an incompatible second spatial world model.

Any support-envelope or dependency representation introduced here should either:

- consume the same canonical OntoMath/geometry structure; or
- be disposable derived state that can later compile from the shared scene-spatial DAG.

---

## 16. Non-goals

This migration does not require:

- deleting SAT/GJK/EPA before the architecture can move;
- expressing numerical geometry algorithms as authored Law;
- converting every shape to SDF;
- inventing a new Physics ontology;
- inventing a new Contact BeingKind;
- serializing pair caches or negative proofs;
- making approximation decide truth;
- forcing every spatial predicate through the GPU;
- completing continuous collision detection in the first rung.

Continuous / swept spatial relations belong naturally in the temporal Prophetic rung, but only after current-state witness parity and dependency invalidation are proved.

---

## 17. Concrete first implementation pass

The first coding PR after this specification should be intentionally small.

1. Add a neutral exact overlap-witness adapter in the geometry/OntoMath Sense boundary.
2. Move or wrap CollisionResult so ConditionModel does not include Physics collision authority.
3. Change ConditionNode::Overlaps to call the neutral witness.
4. Add spatial_witness_parity_test.
5. Add the first conservative SpatialEnvelope helper using current canonical geometry bounds.
6. Add a Prophetic Overlaps analysis path that records known dependencies but still fails open for anything not yet enumerable.
7. Do NOT remove the legacy broadphase or response yet.
8. Instrument old and new paths so the next rung has evidence.

This first PR should prove the architectural inversion:

    Law -> spatial witness

instead of:

    Law -> Physics collision subsystem

Only after that seam is real should Formation candidate routing begin to displace sort-and-sweep.

---

## 18. Final target state

After the migration, Earthcall should not have to ask:

    Which collision system should this Object use?

The system should instead be able to say:

    This authored Law depends on a spatial relation between these participants.
    Formation has already narrowed who could matter.
    Prophetic reasoning proves most candidates cannot satisfy that relation yet.
    The remaining exact mathematical witness says the relation now holds.
    The Law decides what follows.

At that point "collision" has ceased to be an engine-owned domain.

It is one authored use of a more general capability:

**Singulars entering mathematically witnessed spatial Relations under Law.**
