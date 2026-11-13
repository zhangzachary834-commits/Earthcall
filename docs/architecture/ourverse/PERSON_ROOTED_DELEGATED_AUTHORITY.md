# Person-Rooted Delegated Authority and Causal Provenance

**Status:** Authorial direction, specified 2026-10-18 by Zach while reviewing First Mover authority. The constitutional rules in this document are canonical design constraints. The inspected tree implements only part of them: Person-rooted First Mover recognition, authenticated grantors, save/resource scoping, revocation of recognition, authored-law authority clamping, and several Kernel refusals exist; a general Person-specific delegable-authority lattice carried through later Law execution and native effects does not yet exist.

**Origin:** Zach's decree is the source. The formal invariants and separation of authority-kind from dynamic scope below are the Sun's normalization of that decree into testable architecture. This document does not replace TransferPolicy, FirstMoverRegister, or authored Relations with a second permission system. It states what the one authority office must eventually mean.

**Companions:** law/FIRST_MOVER_AUTHORING.md, ourverse/SECOND_PERSON_FRAMEWORK.md, ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md, interrelations/SECURITY_AS_IN_WORLD_PHYSICS.md.

---

## 0. The decree

A First Mover with a valid key acts from authority **derived from the Person or Persons who grant it standing**.

A grant cannot manufacture authority.

If the granting Person may delegably exercise some authority, that authority may be granted within an authored scope. If the granting Person does not possess that authority, or possesses it but may not delegate it, no interface, save file, Law text, model output, or displayed word "granted" can make the grant effective.

Authority is not frozen forever. A grant is itself governed state: it may be narrowed, widened within the grantor's current delegable authority, revoked, replaced, or allowed to expire by the Person or Persons who hold authority to modify it.

All aspects and scopes of a new delegation begin **off / empty by default**. Each admitted authority and each scope is positively authored.

Where future scope cannot be enumerated safely at creation time, Persons may author temporal or semantic scope rules: expiration, conditions, or a subordinate Law that reconstructs a candidate scope from changing Relations, Formations, Categories, Lexemes, Properties, or other legible world state. Reconstruction never bypasses the authority ceiling from which it derives.

---

## 1. Four questions that must remain distinct

Earthcall already distinguishes several ideas that conventional systems often collapse. Delegated authority must preserve that separation.

| Question | Meaning |
|---|---|
| **Who is this?** | Identity / authenticated standing. |
| **Whose word is this?** | Authorship / provenance. |
| **What may this actor cause?** | Effective authority. |
| **What may this actor grant onward?** | Delegable authority. |

Recognition is not capability. Authorship is not capability. Possession of a capability does not imply permission to delegate it.

A useful ordering is:

    effective authority
        may include powers that are non-delegable

    delegable authority
        is the subset that may lawfully be transmitted onward

Therefore:

    DelegableAuthority(P) ⊆ EffectiveAuthority(P)

and any grant must satisfy:

    Authority(grantee through grant G)
        ⊆ DelegableAuthority(grantor(s) under G's governing rule)

The exact semantics for several Persons jointly authorizing one grant are authored governance, not hardcoded majority logic.

---

## 2. No authority ex nihilo

**Constitutional invariant:** no effective authority appears merely because a representation says it exists.

These are descriptions or proposals, not proof of authority:

- a JSON field saying "granted": true;
- an interface badge saying "Administrator";
- a Law asserting that another Law has power;
- a model claiming that a Person approved an action;
- a saved Relation whose authorizing chain does not validate;
- a dynamic scope selector whose present membership now includes a more powerful channel.

The substrate must validate the current authority ancestry.

Conceptually:

    proposal to grant
          ↓
    authenticate author(s)
          ↓
    resolve their current delegable authority
          ↓
    apply authored governance / joint-author rules
          ↓
    intersect requested authority with what may actually be delegated
          ↓
    effective Authority Relation or explicit refusal

A sentence about power is not power.

---

## 3. Default-empty delegation

A new delegation starts with no admitted authority.

Conceptually:

    Delegation {
        world.read          = NONE
        world.write         = NONE
        law.author          = NONE
        relation.author     = NONE
        filesystem.read     = NONE
        filesystem.write    = NONE
        network             = NONE
        process.execute     = NONE
        device.control      = NONE
        delegate            = NONE
    }

These names are illustrative capability families, **not a request for new C++ enums**. Refusals 1, 3, 6, and 7 still apply. The meaning should be authorable through existing ontological machinery and one authority office.

The security property is the default, not the spelling:

> **An omitted authority is absent authority.**

This differs from the ordinary PropertyPath migration default, where current legacy reads/writes remain open until governed. Delegated actor authority is a different question: a newly created delegation must not inherit ambient powers merely because some Property path is technically reachable.

---

## 4. Authority is relational and Person-rooted

Earthcall should model meaningful delegation as authored relationship, not as a magic scalar on the grantee.

Conceptually:

    Person A
       -- delegates [authority kind K,
                     scope S,
                     validity V,
                     delegation bound D] -->
    First Mover M

This direction composes with Zach's earlier five Relation grounds in SECOND_PERSON_FRAMEWORK.md. It must not introduce a hardcoded AuthorityKind enum merely to encode domain vocabulary.

The effective grant is rooted in a real Person's authenticated standing and the governance Relations that make the grant admissible. A Model is never promoted into Person to make this work.

For a grant to be effective, the engine must be able to answer:

1. which Person or Persons authored / govern the grant;
2. whether they are presently authenticated where cryptographic proof is required;
3. what authority they currently hold;
4. what subset of that authority they may delegate;
5. what authority kind this grant transmits;
6. what Singulars / Properties / Zones / Relations currently fall inside its scope;
7. whether the grant is presently valid;
8. who may amend or revoke the grant.

---

## 5. Authority kind and scope selector are different things

A changing scope must never silently become a new kind of power.

Example:

    capability:
        write visual.color

    scope:
        all Decorative Objects presently participating in Formation Garden

    validity:
        until T

The Formation may gain and lose members. The selector can therefore change which Objects are in scope.

But if the Formation later acquires a Shell channel, Network channel, robot, or other consequential Singular, membership alone must not transform write visual.color into process.execute, network.write, or device.control.

Therefore:

> **Dynamic scope may change membership. It may not mint a capability class.**

A scope selector answers **where / to which referents** an already-authorized kind of act applies. It does not answer **what kind of act** is authorized.

---

## 6. Temporally authored authority

Authority Relations may be time-dependent because the world they govern changes.

A Person may author:

- an absolute or relative expiration;
- a validity condition over legible Properties or Relations;
- a scope defined by changing Formation membership;
- a scope defined by authored Categories or Lexeme-resolved predicates;
- a fallback Law that proposes reconstruction when the old scope no longer expresses the Person's intent.

The important distinction is between **semantic recomputation** and **authority creation**.

A reconstruction Law may compute a candidate:

    old grant expires or becomes inapplicable
            ↓
    authored reconstruction Law evaluates current world
            ↓
    candidate grant / candidate scope
            ↓
    authority office validates candidate against
    current grantor authority + delegation rights + Kernel floors
            ↓
    new effective relation OR refusal

The reconstruction Law is not sovereign over its own result.

If the founders were wise enough to author the categories, Forms, Property thresholds, Lexeme vessels, or temporal conditions by which scope should evolve, those meanings may be continuously re-evaluated. The engine enforces the resulting bound; it does not invent the human teleology.

---

## 7. Causal authority must survive durable Laws

A First Mover's initial authoring scope is not the end of the authority story.

The dangerous shape is:

    Person grants Model:
        may author Garden/**

    Model authors Garden Law
            ↓
    Law becomes durable world state
            ↓
    future event fires Law
            ↓
    qualified Property path
            ↓
    global File / Shell / Stream / device channel
            ↓
    host effect

If the original grant disappears from evaluation after the Law is authored, authority has fallen off the causal chain.

That is not permitted by this doctrine.

A derived Law, Relation, Formation, action, or delayed event may preserve semantic causation without acquiring new power. Any consequential act must be bounded by the authority lineage under which the acting machinery exists.

The desired invariant is:

    Authority(effect)
        ⊆ Authority(executing Law / process)
        ⊆ Authority(author under the relevant delegation)
        ⊆ Authority granted by the Person(s))

More generally:

> **Meaning may propagate freely. Authority may not.**

A Law may describe, reason about, or request an effect outside its authority. Description is not execution. Crossing into a consequential native effect requires the same authority chain to remain valid at the actuation boundary.

This is the constitutional answer to delayed capability smuggling.

---

## 8. Mutation, revocation, and adoption

A grant is not set in stone.

Those with authority to govern a grant may:

- narrow its scope;
- widen its scope only within authority they may currently delegate;
- add or remove admitted capability families;
- change expiration / temporal conditions;
- revoke it;
- replace it with a successor grant.

Revocation must have a defined effect on durable Laws previously authored under that grant. The precise lifetime modes remain an implementation/design choice, but they must be explicit rather than accidental. Admissible policies include immediate dynamic revocation, explicitly durable signed grants with their own expiry, or Person adoption of an authored Law under a new authority lineage.

What is forbidden is an unexamined ghost capability: a model loses standing while an old Law quietly retains ambient host authority because nobody carried provenance into execution.

---

## 9. Authority to act, delegate, and modify authority are separate

These powers must not imply one another.

For example:

    may operate Robot R

does not imply:

    may delegate operation of Robot R to a Model

and neither implies:

    may change the governance rule deciding who can operate Robot R

The architecture must preserve at least these distinctions:

- authority to perform an act;
- authority to delegate some or all of that act;
- authority to amend / revoke the delegation;
- authority to author the higher-order Laws or Relations governing that delegation.

Higher-order authoring is itself subject to authority. There is no recursive escape hatch where a Law becomes sovereign by authoring a Law about sovereignty.

---

## 10. Multiple Persons

Zach's decree explicitly admits authority derived from a granting **Person or People**.

This document does not invent a universal quorum rule.

A jointly governed grant must instead name or derive its own valid governance relation: unanimity, threshold, scoped offices, ownership shares, constitutional floors, or another Person-authored rule. The engine verifies that the governing rule has standing; it does not silently choose "one signature is enough" or "majority wins."

No participant may use group syntax to launder an authority that the governing relation does not permit the group to exercise or delegate.

---

## 11. Relationship to current First Mover machinery

The current FirstMoverRegister is a strong beginning but answers a narrower question.

It currently establishes, among other things:

- a model cannot self-attest;
- its recognition is rooted in an authenticated Person;
- an empty mover scope means it may write nothing;
- save/resource writes are limited by signed path scopes;
- revocation of mover standing exists;
- foreign mutation is checked against current standing and the durable resource path.

Its current scopes are **resource/save authoring scopes**, not a complete Person-specific authority vocabulary.

The register itself says recognition is about recognition, not general capability. Keep that distinction.

The following are still broader design/implementation work and must not be claimed as landed merely because this document exists:

- a first-class Person-specific effective/delegable authority representation;
- authored authority Relations with temporal/dynamic selectors;
- general actor-aware PropertyPath read/write mediation;
- carrying originating delegation through delayed Law execution;
- native-effect authorization for filesystem / process / network / device channels using that causal authority;
- explicit revocation semantics for already-authored durable Laws;
- joint-Person grant governance.

---

## 12. Required future witnesses

When this doctrine becomes implementation, tests should establish at least:

1. **No authority ex nihilo:** a forged or merely textual grant cannot create authority.
2. **Default empty:** a new delegation with no admitted scopes can cause no delegated effects.
3. **Non-delegable means non-delegable:** possession of an act does not imply power to grant it.
4. **Monotonic derivation:** a grantee and every derived Law remain within the upstream authority ceiling.
5. **Dynamic membership, fixed kind:** Formation membership changes cannot turn a visual/property grant into process/network/device authority.
6. **Temporal expiry:** an expired relation ceases to authorize the covered act.
7. **Reconstruction is bounded:** a fallback Law can propose a successor but cannot exceed current grantor delegable authority.
8. **Causal continuity:** a Law firing hours after authorship still presents the authority lineage of the delegation under which it was authored/adopted.
9. **Revocation:** the chosen revocation semantics are explicit and tested at the actuation boundary.
10. **No interface sovereignty:** UI state, save fields, and authored text cannot override failed authority proof.

---

## 13. The short form

**Authorship answers "whose word?" Authority answers "whose power?"**

**No authority ex nihilo.**

**Everything in a delegation is off until positively granted.**

**A grantee can receive only what its grantor is allowed to delegate.**

**Dynamic scope may change membership; it may never mint a new kind of power.**

**Authority can be temporally authored and modified by those authorized to govern it.**

**Durable semantic causation must carry its authority ancestry all the way to consequential effects.**

**Meaning may propagate freely. Authority may not.**
