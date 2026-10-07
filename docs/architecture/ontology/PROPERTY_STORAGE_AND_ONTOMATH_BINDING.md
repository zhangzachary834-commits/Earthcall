# Property Storage and OntoMath Binding

**Status:** Architectural proposal; no runtime capability is claimed by this document. The distinctions in §§2–4 and the safety obligations in §§5–8 are the design target, not a description of completed implementation.

**Origin:** Zach's “OntoMath, Opcodes, and Properties” note in `docs/Agenda/Tasks/To-do list.md`, discussed with Codex on 2026-09-22. Zach defined Property as machine-level substrate ordered as predicates of a Singular; asked for live PropertyPath-bound mathematics, authorable data structures and ownership direction, safe shared/unique/weak access, and a measured choice of cache layout. Zach then specified that **permissions for memory and Property access must be governed by Laws**, and identified the bootstrap problem: **Laws themselves occupy memory**. The binding relationships, checked-handle proposal, invalidation contract, and staged proof below are Codex's extension of that direction, offered for Zach's correction.

**Recorded by:** Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:41:24-07:00.

**Companions:** [`PROPERTY_AS_PREDICATION_NOT_BEING.md`](PROPERTY_AS_PREDICATION_NOT_BEING.md), [`NO_BLACK_BOX.md`](NO_BLACK_BOX.md), [`ONTOMATH_FRAMEWORK.md`](../mathematics/ONTOMATH_FRAMEWORK.md), [`PROPERTY_ADDRESSING_IN_FORMATION_RETE.md`](../law/PROPERTY_ADDRESSING_IN_FORMATION_RETE.md), [`DERIVED_STATE_LEDGER.md`](../law/DERIVED_STATE_LEDGER.md), and [`FIRST_MOVER_AUTHORING.md`](../law/FIRST_MOVER_AUTHORING.md) §7.

---

## 0. The question Zach is asking

An Object held in a Person's hand has an authored color Property. A building's mathematical form should be able to derive from **that PropertyPath**, and continue to change when the source changes. The author must also be able to say whether two Properties name one path, share one value with distinct paths, borrow a value without owning it, or hold independent copies. This is a decision about **what is connected to what**, not a request to expose raw addresses, destructors, or GPU buffers to Laws.

The thesis is:

> **A Person authors the meaning-bearing relationships among PropertyPaths, value cells, and expressions, including the Laws that govern access to them. The substrate enforces safe execution and the irreducible bootstrap boundaries.**

This extends the existing doctrine, “a Property is a legible predication of a Singular.” Neither a Property, its value cell, nor an ordinary list or map becomes another Singular solely because it is addressable. A Relation still joins Singulars; a PropertyPath qualifies the aspect of a Singular in question. A graph of Property values is data about bearers, not a second ontology of Property-beings.

## 1. What exists today, and the gap

| Existing seam | Current capacity | Gap this design addresses |
|---|---|---|
| `PropertyValue.hpp` | Typed variant with numeric values, strings, vectors, matrices, non-owning being pointers, shared lists/dicts, and shared OntoMath fields | No authored identity, ownership, or lifetime contract for a general value cell; variant alternatives are not themselves a general operation system |
| `DataStructure.hpp` / `Singular` | A named `DataStructure` with `PropertyValue` data and a `writeBounds` member, stored on a Singular | The structure is a scaffold; its bounds are not an enforced, law-addressable, persisted storage relationship |
| `PropertyPath` | Nested registered/dynamic resolution, including dict/list elements and vector components | A resolved `PropertyValue*` is an execution detail, not a durable or safe authored reference; nested in-place writes and replacement need a complete change feed |
| `MathBindings` | Person-authored variable name → `PropertyPath`; strict failure when a bound path cannot be read | Each evaluation reads a value; arithmetic inputs are widened to `double`; the binding does not carry authored reference/copy/ownership semantics |
| `MathNode` / `ScalarForm` | Typed AST over supported scalar, vector, and field operations; symbolic scalar calculus | `PropertyValue` is broader than the math type system; containers and arbitrary values cannot simply be declared arithmetic |
| `StringId` / `Singular` | Interned path names, parallel arrays of name IDs and Property pointers, dynamic-property map | These accelerate lookup, but do not solve value ownership, dependency invalidation, or shared-storage layout |
| `TransferPolicy` / `Law` | Law-writable gates over **set-to-set transfer**; a Law is a Singular with registered state and may be targeted by a Metalaw | This does not yet mediate every Property read, write, alias, or Law-text edit; general memory-access governance and its bootstrap are open |

Sources: `src/ConstructedBeing/Singular/Property/{PropertyValue.hpp,DataStructure.hpp,PropertyPath.hpp,PropertyPath.cpp}`, `src/ConstructedBeing/Singular/Singular.hpp`, `src/ZonesOfEarth/AuthorsOfLaw/{MathBinding.hpp,Law.cpp}`, `src/Singularity/{Core/StringId.hpp,TransferPolicy.cpp}`, and `src/Singularity/OntoMath/ScalarForm.hpp` (inspected 2026-09-22). This table is source inspection, not an executed witness.

Two identifiers must remain distinct: `Identity::SingularId` is durable identity for a **being**; `Earthcall::StringId` is a runtime-interned **name** and is not serialized. Neither is a value-cell identity. Calling the existing lookup optimization “SingularId interning” would conflate them.

## 2. Three identities and five authored relationships

The design must distinguish:

1. **Bearer identity:** which `Singular` has the predicate.
2. **Path identity:** which authored/registered aspect is addressed on that bearer.
3. **Value-cell identity:** which storage location currently supplies the value.

These can coincide in ordinary use but must not be assumed to coincide. The authored relationships are:

| Relationship | Meaning after the source changes | Meaning after the source path is rebound | Ownership |
|---|---|---|---|
| **Follow path** | Read the new value through the path | Follow the new target | Path keeps no source value alive |
| **Share cell** | See mutations to the same cell | Continue sharing the old cell until explicitly rebound | Two or more strong references to one cell |
| **Unique cell** | Only its one owning Property names that cell | Rebinding transfers or retires that ownership under checked rules | Exactly one strong owner; other observers may be weak |
| **Observe weakly** | See a live cell while it exists | Follow the specifically named cell, not a replacement path | Does not extend cell lifetime |
| **Copy value** | Remain independent | Remain independent | New cell(s), with explicit deep-copy rules for nested values |

**Derive** is a sixth relationship of a different sort: a destination is the result of an authored expression over one or more source bindings. It has dependency semantics rather than shared mutable storage. The building's color may derive from the held object's red PropertyPath; that does not make the two colors the same cell. If the author wants a second Property that directly changes when either path writes, that is **share cell** instead.

These are semantic operations, not proposed names for new `ActionNode::Kind` or `MathNode::Op` values. Any eventual serialization must preserve old integer opcode values, never reuse burned values, and prefer composition of existing Law/Math structure where it can express the intended operation.

### The red object, made precise

Suppose an authored binding resolves the Object currently held by a Person and then its `color` Property. A building expression derives a visible feature from that binding.

```text
Person --holds--> Object A
Object A.color = red
Building.form <- derive(expression, source = Object A.color)
```

If `A.color` becomes blue, the building recomputes from blue. If the held object changes to `B`, **follow path** may resolve `B.color`; **share cell** continues to name A's old cell. If the source disappears, the derived expression is undefined and names the failed binding; it must not silently substitute zero, black, or the last cached value. Whether the authored relationship intentionally follows the *current holder Relation* or fixes Object A is part of its authored referent, not a choice the renderer makes.

## 3. Typed mathematics without turning every value into a number

OntoMath variables need an authored binding that may resolve on demand, preserve the source `PropertyValue` type, and expose a checked typed view to each operation. `ScalarForm` may continue to use its present `double` coefficients and scalar calculus while other operation families admit values they can actually handle. This avoids promising integration, GPU lowering, or arithmetic for a map or a being reference merely because the value is reachable through a PropertyPath.

For each operation, the contract must state:

- admitted input and output value types, including shape and units where relevant;
- whether it reads a value, follows a path, or consumes a reference to a cell;
- coercion and precision rules, including overflow and undefined cases;
- CPU evaluation and, when supported, channel lowering to WGSL or another target;
- dependency paths whose changes invalidate a derived result.

An unsupported value/operation pair refuses with a reason. A channel that cannot lower a valid OntoMath expression refuses at that channel; it does not reinterpret the expression. Existing `MathNode::ValueKind` and `MathNode::Op` are the starting constraints, not proof that all `PropertyValue` alternatives already have semantics in OntoMath. The separate To-Do item about a unified opcode-property substrate should examine common *invariants* across OntoMath and Actions before attempting a shared instruction set; it must not collapse their different meanings by renumbering serialized enums.

## 4. Authorable data structures remain Properties

Lists, dictionaries, and later generic substrate containers can be values of Properties without becoming domain classes or independent Singulars. A Person may author their schema, contents, bounds, and Law operations. C++ still owns allocation, traversal safety, and physical layout. The container's meaningful state must be reachable through registered or authored PropertyPaths; raw capacity, allocator state, and temporary buffers remain named machine mechanism below the Kernel.

An authored container operation needs a bounded, typed effect: which path(s) it reads, which path(s) it may write, and what happens on missing keys, out-of-range indices, cycles, or type mismatch. This is compatible with the algorithm-as-Law discipline; “make a map Property” does not authorize an opaque C++ method that quietly decides domain behavior. The existing `DataStructure::writeBounds` field must not become an independent permission system: any authored bounds must participate in the same Law-governed access decision.

## 5. Safe storage and authority

**Proposed substrate representation:** a table of value cells addressed by opaque, generation-checked runtime handles. The table controls allocation and reclamation. A handle can resolve to a typed cell or fail explicitly as expired/stale; it is never an arbitrary address and cannot be forged from a Property value. A stable authored referent and saved alias topology are serialized in human-legible form; runtime slot numbers, pointer values, and `StringId`s are rebuilt on load.

This proposal allows strong shared access, single-owner access, and weak observation as authored directions while keeping actual dereference in C++. It must define how unique ownership transfers, how strong cycles are rejected or collected, and what a weak observation returns after expiry. Copying a container must specify whether nested references are cloned, retained, or refused; the word “deep copy” alone is insufficient for cyclic or being-referencing graphs.

**Authority follows the effective access.** An alternate path to a shared cell must not open a closed source or destination. The access decision must name the actor, operation (read, write, bind, share, copy, observe, or derive), bearer, path, and effective cell; an alias cannot evade a Law by changing the spelling of a path. `TransferPolicy` is the existing Law-governed precedent for set-to-set transfer, **not a general Property read/write gate in today's code**. General access mediation must extend that one authority office and its Law-facing state rather than grow a competing `DataStructure` or pointer permission system. A generic share operation must not bypass `Object::ownMaterial` / `Object::setFaceColor`: painting through a resolved shared Material currently repaints every Object naming it. An authored choice to share paint and an accidental shared-material mutation are different acts.

All failures are explicit: expired handle, absent path, incompatible type, forbidden write, unsupported operation, and unresolved save reference must remain distinguishable. No authorable operation may expose a `PropertyValue*`, `Property*`, `Singular*`, or C++ smart pointer as a durable authority token. Existing raw pointer alternatives in `PropertyValue` are an implementation fact to audit, not a safety precedent for the new interface.

### 5a. Access permissions are Law-governed

Zach's correction is constitutional to this design: **the decision whether a Person, Law, or channel may access memory is itself an authored Law decision**. The engine may carry out that decision and enforce the Kernel floor; it may not bury a second, hardcoded, domain-specific access table in the cell manager. The existence and type of a meaningful Property remain legible under `NO_BLACK_BOX.md`. Whether a particular actor may read its value, write it, follow it, share its cell, or derive from it is a separate question for the applicable authored Laws.

The access funnel must be common to direct PropertyPath operations, OntoMath bindings, Law Actions, authored container operations, and channel projections. It checks the **effective referent**, including a shared cell reached through another path, and records which Law authorized or refused the act. A fast path may cache a proved decision only with declared dependencies on Law text, policy-gate state, actor/jurisdiction, path-to-cell bindings, and relevant Property values. A stale or unknown cache has no authority to grant access. What a denied read reveals through error text also needs an explicit Law-governed disclosure rule: the engine can name the refused operation without leaking the protected value.

`Law::applyTo` already checks authorship, target-Law authority, kernel boundaries, jurisdiction, and conditions. `Law::buildProperties` already registers some of a Law's state (`enabled`, `conditionMode`, `name`, `drives`), allowing Metalaws. The Law's condition/action text and its dependencies are also memory with meaning; a future access model must expose their governable representation without exposing `authorityLevel` as an authored setter or inventing a hidden path by omission. The existing `TransferPolicy` gate is specifically used by set-to-set capture and replay. Its law-writable `gate.*` Properties demonstrate the direction, but they are not evidence that arbitrary `PropertyPath::getValue` or `setValue` is already permission-checked.

### 5b. Bootstrap: Laws govern Law memory only after a first act

No access Law can be read or run before its text is present and admitted. Requiring an already-running Law to authorize the first Law is a circular dependency. The bootstrap sequence is therefore explicit:

1. **Kernel substrate admits and verifies.** It parses bounded Law text, resolves recorded authors, enforces the authored authority ceiling and immutable body/Kernel guards, and checks First Mover provenance. These are irreducible machine acts, not a standing C++ policy deciding which domain Properties Persons may access.
2. **A recognized First Mover seeds the initial access Laws.** The seed names the Person whose authority it carries and the exact Law records it installs. It follows the existing First Mover register and save-file discipline. An absent author leaves a Law `Unauthored`; an unattested injection is quarantined, visible, and inert. A seed cannot attest itself or raise a Law's authority by writing a file.
3. **The initial Law set becomes the world policy.** Once admitted, ordinary access and edits to Law memory are judged through that Law set, including Metalaws and the existing authority ceiling. Any emergency or migration repair path remains an explicit, provenance-bearing First Mover act, never a quiet “if no Laws, allow everything” fallback.

**Self-reference needs transaction order.** A proposed evaluation rule is to decide an attempted edit against the *already committed* policy snapshot. The edit, if authorized, becomes visible to subsequent decisions only after commit. Newly written Law text cannot authorize the write that created it. The Kernel may read the committed policy text to execute the decision; that interpreter read is bounded machine mechanism, not an author-visible grant to inspect every Property. If an access Law needs another protected world value while deciding, dependency evaluation must be bounded and stratified; unresolved authorization cycles refuse with a trace instead of recursing forever or guessing permission. This is a proposal for Zach to approve or revise, not an implemented rule.

**⚑ AUTHOR — open:** What is the initial seed's minimum accessible vocabulary, and may an access Law authorize edits to its own future text when an independent Metalaw would refuse? The committed-snapshot rule prevents a new Law from granting its own creation, but does not alone settle legitimate self-editing, policy deletion, or conflict among several Laws. These require an authored decision before a serialized permission meaning is fixed.

## 6. Change feed, cache, and persistence

The value cell and every path that observes it need a complete dependency record. A successful mutation must:

1. identify the affected cell and all paths/derived expressions whose results may change;
2. apply the existing authority check at the actual write destination;
3. advance a value revision and notify the Law/Rete change feed once per semantic change, including any access-policy decision that depends on the value;
4. invalidate CPU derived values and channel parameter buffers whose declared inputs changed;
5. preserve a complete fallback if a dependency index is stale or incomplete.

This is not merely an optimization. Today `propertyValueUnchanged` treats equal list/dict `shared_ptr`s as unchanged, even though their contents may mutate in place; nested writes can touch a `PropertyValue*` directly. A reference design cannot rely on pointer equality as proof that a value is unchanged. The current derived-state ledger distinguishes structural, relation, Law-text, and Property-value signals; the new cell/path index would need its own declared dependencies, invalidation, and guard test. Prophetic or Formation relevance may only discard a candidate on proof of impossibility, never because a stale cache looked empty.

Save/load must preserve **alias topology**: two distinct Properties that share a cell before save must still share one cell after load; a copied Property must remain independent; weak observations must resolve or report expiry. Save changes obey `FIRST_MOVER_AUTHORING.md` §7: patch the existing save, stage and compare, keep the old file, and rename atomically. No implementation may regenerate a Person's world to introduce this representation. Existing saves lacking binding metadata need defined compatibility semantics, with no accidental sharing inferred from equal values.

## 7. Lookup layout is a measured choice

Interning and structure of arrays answer different questions. `StringId` makes a path-name comparison cheap; `Singular` already scans a contiguous array of those IDs beside its Property pointer array. A cell table can provide checked identity and revisions independently of that lookup. A dense, type-specific array may then be worthwhile for a measured hot population of uniform values. Heterogeneous authored dictionaries may remain sparse. There is no reason to replace all Property storage with one SoA on principle.

Before changing layout, measure parse/load cost, path resolution, reads and writes, change fan-out, shader parameter update, cache footprint, and save/load alias restoration on representative authored worlds. Compare against the existing interned-name path. Any cache must be entered in the derived-state ledger with its dependency, invalidation, rebuild site, and a test that catches a missed change. `SingularId` must not be used as an interned Property-name or transient cell index.

## 8. Proof before implementation is called complete

**Isolated path:** construct two Properties sharing one cell, one copied Property, a weak observer, and a derived expression. Mutate, rebind, destroy, save, and reload their sources. Assert values, type refusals, alias topology, expiry, Law-governed read/write/share decisions, and exactly the notifications each semantic change owes. Include nested list/dict mutation, strong-cycle behavior, denial through an alternate alias, and a policy change invalidating a cached decision. Admit a First Mover seed, reject an unauthored or unattested seed, and attempt a Law that grants its own creation; the attempted self-grant must fail under the committed policy.

**Human-facing path:** in an authored world, hold a colored Object, bind a building's visible form to its color, change the color, exchange the held Object, and load the world again. The Person must see the authored relationship behave as specified. Record any visual judgment Zach must make in `docs/Agenda/Tasks/Person Verification List.md` when an implementation leaves that check open.

**Performance path:** exercise the same worlds at growing Property and dependency counts. Compare interning alone, handle lookup, and selective dense storage with the same semantics. Do not promote a fast cache that can leave a Law deaf.

## 9. Decisions reserved for Zach and staged work

The following require an authorial decision before implementation fixes a serialized meaning: whether **follow path** tracks a change of bearer through a Relation by default; whether a strong shared cell may have multiple independently writable Property predicates; what authorable “unique” means when a weak observer exists; and which nested copy/alias forms are admissible for saved cyclic structures. This document proposes explicit choices and failure states; it does not silently decide these for Zach.

Implementation order: (1) specify the binding, ownership, and Law-governed access algebra with bootstrap and save compatibility; (2) make the current Property change feed complete for nested/shared writes and policy changes; (3) implement checked cells and durable alias topology; (4) add typed live OntoMath bindings and channel refusals; (5) measure and selectively optimize storage. The associated one-sentence tasks live in the To-Do list; the original Zach note is preserved in its Specific Task record.

---

*Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:44:32-07:00. Zach originated the Property, memory-direction, live-binding, cache, and Law-governed-permission questions, including the Law-memory bootstrap problem; Codex formalized the distinctions and proposed the safety and proof obligations. This is an architecture proposal, not a verified runtime feature.*
