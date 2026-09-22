# First Mover Trust and Provenance Analysis

**Date:** 2026-08-20  
**Analyst:** OpenCode (GPT-5.6 Sol)  
**Status:** Source-confirmed findings; the absent-grantor exploit path has not
yet been promoted to an executable regression test.  
**Scope:** `Identity::FirstMoverRegister`, its save-path integration, and the
adjacent Law/Rete provenance claims.

## Origination

Zach originated the governing requirements: a model's recognition is delegated
and traceable to a Person; no First Mover attests itself; recognition never
raises Law authority; First Movers and their Relations must eventually be
first-class and property-legible. These are stated in
`FIRST_MOVER_AUTHORING.md`, the manifesto, and Agenda item 31.

Sonnet identified the missing framework. Opus 4.5 identified the missing causal
source in the Rete fact schema. Opus 4.7 connected session-scoped intercom
identity to that source problem. This analysis extends those observations by
tracing the currently built authorization path and separating confirmed defects
from proposed ontology.

---

## Executive finding

The First Mover Register contains careful cryptographic and path-scope checks,
but its two central end-to-end claims do not currently hold:

1. A valid grant does not prove that its grantor is a Person when the grantor is
   absent from the same register.
2. Production save writes do not establish an active First Mover session or
   serialize/load the register, so the save gate normally takes its documented
   fail-open branch.

This means `FIRST_MOVER_AUTHORING.md`'s status statement that the framework is
“built and enforced at the save path” overstates what is integrated. The gate is
built and directly tested. Its trust root and production activation are not.

## Finding 1: An absent grantor is treated as a Person

**Severity:** High. This defeats the stated delegated-recognition floor for
loaded register data.

`FirstMoverRegister::evaluate` correctly refuses self-attestation and checks a
valid signature. It also attempts to refuse model grantors:

```cpp
for (const auto& candidate : all) {
    if (candidate.id == m->grantedBy && candidate.kind != FirstMover::Kind::Person) {
        return Gate::GrantorNotPerson;
    }
}
```

The check only refuses when a matching grantor entry exists and says `Model`.
When no matching entry exists, evaluation continues through signature,
subject/issuer, scope, and path checks and can return `Gate::Ok`.

Relevant code:

- `src/Identity/FirstMoverRegister.cpp:176-206`
- `src/Identity/Claim.hpp:24-27`
- `src/Identity/Claim.cpp:106-113`
- `src/Identity/SingularId.hpp:21-30`

`Claim` explicitly documents the missing distinction: a signature proves that
the issuer said something; it does not prove the issuer was entitled to say it.
`SingularId::canAuthenticate()` proves only that the identifier is a key. Tests
generate the same kind of key for both Persons and models. Therefore a key is
not, by itself, evidence of Personhood.

### Exploit shape

An untrusted writer can construct:

1. a model entry `M` with a broad scope;
2. an independent attacker key `G`, omitted from `movers[]`;
3. `M.grantedBy = G`;
4. a valid claim signed by `G`, covering `M`, its model kind, and its scopes.

`G` is neither self nor a known model. Its signature verifies. The current code
never establishes that `G` belongs to a Person, so the grant can pass.

The existing test `testModelSignedGrantRefusedOnLoad` covers only the easier case
where the model grantor is present in the register and labeled as a model
(`tests/person/first_mover_test.cpp:234-279`). No test covers an absent grantor.

### Required decision

Do not repair this by trusting a serialized `kind: "person"` label; the attacker
can write that too. Evaluation needs a trust root outside the claim being judged,
for example a Person identity already established by the world's identity
ledger/Person registry or a separately rooted recognition chain. The exact root
is an Identity architecture decision for Zach, not something this analysis
silently chooses.

## Finding 2: The save gate has no production activation path

**Severity:** High for the documented guarantee; currently low behavioral impact
because no production agent-writing path claims to activate it.

`SaveSystem::permitted` consults the singleton register on every engine-mediated
write (`src/Singularity/Storage/SaveSystem.cpp:243-257`). However,
`FirstMoverRegister::permitsWrite` deliberately allows every write when no active
mover is set (`FirstMoverRegister.hpp:117-135`).

Repository-wide callers show:

- `FirstMoverSession` is instantiated only in
  `tests/person/first_mover_test.cpp`.
- `FirstMoverRegister::toJson()` and `loadFromJson()` have no production caller.
- no production save composition writes a `firstMovers` section.

Therefore the end-to-end production path is:

```text
SaveSystem write -> singleton register -> no active mover -> allowed
```

The test proves that enforcement works after the test itself constructs a grant
and opens a `FirstMoverSession`. It does not prove that an actual fixture builder,
agent bridge, or direct-injection workflow opens that session. This is the same
self-agreement class documented in `ENGINEERING_DISCIPLINE.md`: the test supplies
the condition whose real caller is the matter under test.

The architecture document does acknowledge that a process with filesystem access
can bypass engine enforcement. That is not this finding. This finding is that even
the engine-mediated path has not connected a real First Mover entry point to the
gate.

## Finding 3: The register is described as a being but implemented as an opaque record

**Severity:** Architectural debt, already partly acknowledged by Agenda item 31.

`FIRST_MOVER_AUTHORING.md` §8a specifies “a legible Singular whose gates are bool
properties.” The implementation is instead:

- `Identity::FirstMover`, a plain C++ class with public fields;
- `Identity::FirstMoverRegister`, a plain C++ container/singleton;
- no `Singular` inheritance;
- no property registration;
- no production world serialization.

This is not automatically wrong. Identity verification may include Kernel state
that should remain beneath authored ontology. But the current comments and docs
call the register a serialized world-owned being while the code makes it
process-global opaque state. One of those descriptions must yield before the
framework can claim refusal #6 compliance.

The empty `Singularity/FirstMoverOntology/InteriorFirstMover/` and
`SrcFirstMovers/` directories show that a recut is in progress, but directory
names are not implementation and should not be cited as one.

## Finding 4: “Source” is underspecified across events, facts, and provenance

**Severity:** Design risk; do not patch by adding one field yet.

The Rete observation is factually correct: `ReteFact` carries `subject`, `object`,
and state data, but no causal source (`Law.hpp:429-439`). `EventBus::Metadata`
does have an optional `source` pointer, but both synchronous and asynchronous
publish currently discard metadata (`EventBus.hpp:44-55, 85-112`).

Meanwhile durable paths already carry several source-like records:

- `Law::ApplicationRecord` records application;
- Law and ObjectConcept provenance records `authored-by`, `generated-from`, and
  `abstracted-from`;
- the developer creation bypass records the `CreationChannel` as the newborn's
  `authored-by` endpoint (`Tool.cpp:402-410, 512-514`).

These do not all mean the same thing. A click-created Object may involve:

- a Person who intended and performed the gesture;
- an Interaction/Creation channel that sensed it;
- a Law or bypass that applied it;
- a Concept from which it was generated;
- an agent session that injected relevant text earlier.

Calling any one of these simply `source` loses information. Before growing the
Rete fact schema, define which queries need to be answered during matching and
which belong only in durable provenance. Otherwise every fact pays for a field
whose semantics vary by publisher, while the ontology gains another black box
hidden behind an overloaded name.

## Minimal build order

1. Add a failing regression case for a valid grant signed by an absent grantor.
2. Decide and document the trusted proof that a grantor is a Person.
3. Make `evaluate` fail closed when that proof is absent; keep signature validity
   and authority entitlement as separate checks.
4. Identify one real substrate-writing entry point and wrap it in
   `FirstMoverSession`; test through that entry point rather than constructing the
   session only inside the test.
5. Serialize/load the register only after its ownership is settled: process,
   world, or Identity kernel. Do not let a save establish its own trust root.
6. Specify whether the first-class framework represents actors, sessions,
   engine-backed Laws, or a Formation relating them. Do not make one class stand
   for all four by default.
7. Add causal source to Rete facts only when an authored condition actually needs
   to match on it and its endpoint semantics are stable.

## Verification boundary

This pass traced all production references to `FirstMoverRegister`,
`FirstMoverSession`, `firstMovers`, and the relevant provenance paths. The
existing `first_mover_test` should still pass because it does not exercise the
absent-grantor case. A dedicated executable probe or regression test is still
required before calling Finding 1 dynamically reproduced.
