# PropertyPath and Memory Micromastery — Storage and OntoMath Binding

**Status:** General storage/binding architecture remains a proposal. The narrow typed-binding and container-access rung recorded in §9 is implemented; checked shared/unique/weak cells, custody, and persistent alias topology remain open.

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
| `MathBindings` | Person-authored variable name → `PropertyPath`; strict failure when a bound path cannot be read; reads now preserve the `PropertyValue` alternative | Arithmetic operations choose their numeric representation; the binding does not yet carry authored cell ownership or deep-copy semantics |
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

**Zach's 2026-09-27 correction:** These distinctions do not make a PropertyPath an identity holder. A path does not retain a Singular's ID on its own. A read is resolved under the Zones in which it occurs, including plural Zones in an inter-Zone transmission. A Zone is any domain or set over a mathematical or discrete bound, including a device domain; it need not be a digital 3D world. A path intended for one individual must reach that same individual or refuse; a changing Relation can select another bearer only when that changing referent is what the Person authored. Zone authorization to hold identity is rooted in Person-authorized First Mover action through owned machines and governance/ownership Relations, not inferred from the physical location of data. The exact machinery and conflict rules remain open; no path-owned ID or alternate permission system is prescribed here.

**Zach's 2026-09-28 direction:** The public ID denotes the **same identity** across its physical-machine custody, authorized Zones, and several Ourverses. An Ourverse may be a guardian and participant in shared recognition; it cannot, by itself, rewrite the Person's key or reauthor the ID as someone else. A key change requires the Person's consent. This is consistent with [`OURVERSE.md`](../ourverse/OURVERSE.md), which makes Ourverse a Singular ordering Zones rather than an owned Zone or Home. Zach's [distinctions among authorship, ownership, governance, dependency, and higher Constitution](../ourverse/SECOND_PERSON_FRAMEWORK.md#zachs-account-of-standing-and-constitution-2026-09-28) govern how these roles are interpreted. Device keys are Zone keys, while the Person has one private signing key shared among their own machines. These are authorial constraints, not a specified key-sharing or recovery mechanism. Today's `Identity::SingularId` equates a Person's ID with their Ed25519 public key, so loss, compromise, or replacement of that key raises an unresolved continuity question. No ordinary path owns or stores that ID.

Zach further directed that these grounds are **five distinct Relation kinds, like Category Relations, never hardcoded enum values**. The PropertyPath access design may read those Relations as authority evidence, but it may not invent their endpoints or admission rules; those are still authorial questions. Each Zone governs the things owned within its bounds: its own reads and writes, and its reception of another Zone's transmission. It cannot govern the sender's decision to transmit. Inter-Zone access must preserve that separation of jurisdiction rather than collapse all Zone decisions into one global gate.

**Zach's migration default (2026-09-28):** Ordinary Property reads and writes remain open until an explicit reason closes them. That preserves current ungoverned-world behavior while allowing authored Zone Metalaws to make scoped decisions later. Structural read-only Properties and Kernel/Person guards are already explicit reasons to refuse. Do not reuse `TransferPolicy`'s set-to-set `Gated` defaults as denials of ordinary PropertyPath access; they answer a different operation today. This default does not authorize an unrecognized First Mover's foreign mutation or override a receiving Zone's future authored refusal.

**Zach's constitutional exception (2026-09-28):** An intrinsic Kernel guard, such as refusing another actor's positive assignment of a Person's body or location to a specific place, is hardcoded at the relevant actuation boundary. It exists before bootstrapping, cannot be opened by a Metalaw or `TransferPolicy`, and is not a default-deny Zone policy. Zach further specified that being among a Law's authors is **not** consent to every later firing: movement requires an **authored, revocable consent Relation** between the Person who would move and the Law that would move them. Consent is ongoing until that Person revokes it. Zach confirmed that active consent is a sub-Relation of their enduring primary Relation; revocation changes the active consent to a historical kind rather than erasing that past, and the Person's signing key must authorize both creation and revocation. The existing Law guard now follows the bearer of a `Set`/`Add`/`Scale`/`Lerp`/`Drive`/`Map`/`Flow` motion write (and `RemoveProperty` clearing a motion slot), including qualified `@person` paths and qualified Create children. Since the current Relation graph does not yet implement primary/sub-Relation preservation and the Law application context cannot verify that signed consent, those writes refuse even for a Law that the Person authored. The authored kind identities, signature transcript, storage and revocation path, and enforcement at non-Law actuation channels remain to be designed and implemented; do not substitute an ungrounded `"consent"` string or erasable graph edge. A direct `PropertyPath::setValue` call does not itself know an actor and must not be mistaken for proof of that broader enforcement.

**Zach's positive-write distinction (2026-09-28):** The intrinsic refusal is against positively placing or driving a Person's body/location without their actuation consent. It does not forbid a Law from judging that a Person is in a restricted, dangerous, or private area and authoring a prohibition on being there. In the current engine, a location condition can be read and can lead to a non-motion action; that proves this narrow guard does not silence such Laws. How a normative prohibition affects entry, continued presence, warning, or egress is a separate authored policy question, not an implied power to assign the Person a destination.

The first narrow runtime refusal now lives in `Person::setPersonId` and `personFromJson`: a Person already bearing a key ID cannot be assigned a different one, and profile hydration checks the claimed ID before modifying other fields. This protects live identity continuity at those two seams. It is not the Zone resolution, Metalaw disclosure, proof of an initially unbound Person's claim, or a Person-consented key rotation protocol.

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

**Proposed substrate representation:** a table of value cells addressed by opaque, generation-checked runtime handles. The table controls allocation and reclamation. A handle can resolve to a typed cell or fail explicitly as expired/stale; it is never an arbitrary address and cannot be forged from a Property value. Saved alias topology must preserve the authored relationships; how a referent is individuated and disclosed is subject to the Zone-governed identity direction above. Runtime slot numbers, pointer values, and `StringId`s are rebuilt on load.

This proposal allows strong shared access, single-owner access, and weak observation as authored directions while keeping actual dereference in C++. It must define how unique ownership transfers, how strong cycles are rejected or collected, and what a weak observation returns after expiry. Copying a container must specify whether nested references are cloned, retained, or refused; the word “deep copy” alone is insufficient for cyclic or being-referencing graphs.

**Authority follows the effective access.** An alternate path to a shared cell must not open a closed source or destination. The access decision must name the actor, operation (read, write, bind, share, copy, observe, or derive), bearer, path, and effective cell; an alias cannot evade a Law by changing the spelling of a path. `TransferPolicy` is the existing Law-governed precedent for set-to-set transfer, **not a general Property read/write gate in today's code**. General access mediation must extend that one authority office and its Law-facing state rather than grow a competing `DataStructure` or pointer permission system. A generic share operation must not bypass `Object::ownMaterial` / `Object::setFaceColor`: painting through a resolved shared Material currently repaints every Object naming it. An authored choice to share paint and an accidental shared-material mutation are different acts.

All failures are explicit: expired handle, absent path, incompatible type, forbidden write, unsupported operation, and unresolved save reference must remain distinguishable. No authorable operation may expose a `PropertyValue*`, `Property*`, `Singular*`, or C++ smart pointer as a durable authority token. Existing raw pointer alternatives in `PropertyValue` are an implementation fact to audit, not a safety precedent for the new interface.

### 5a. Access permissions are Law-governed

Zach's correction is constitutional to this design: **the decision whether a Person, Law, or channel may access memory is itself an authored Law decision**. The engine may carry out that decision and enforce the Kernel floor; it may not bury a second, hardcoded, domain-specific access table in the cell manager. The existence and type of a meaningful Property remain legible under `NO_BLACK_BOX.md`. Whether a particular actor may read its value, write it, follow it, share its cell, or derive from it is a separate question for the applicable authored Laws.

**Permission to read without permission to write is an access outcome, not a hidden Property.** It is distinct from a genuinely derived Property with no setter. Zach asks for Law-governed read-only and read-and-write possibilities as part of PropertyPath and memory *micromastery*. A Metalaw may also restrict whether a reader learns the resolved ID; when it does, the authorized disambiguation must preserve the same individual without disclosing the ID. The read and write decisions must be unified with `Singularity/TransferPolicy`'s existing authority office and Kernel/Governable/Gated discipline, not duplicated in a cell manager or Zone-specific permission engine. Today's `TransferPolicy::canTransfer` only guards set-to-set transfer, so its current tiers and gate names are **not yet a general read/write decision procedure**. Zach subsequently specified that bootstrap Metalaws configure TransferPolicy, downstream Laws branch on policy and actor/context, and each Zone governs its own acts and incoming reception. The exact authored access schema and conflict resolution among shared aliases remain open.

The access funnel must be common to direct PropertyPath operations, OntoMath bindings, Law Actions, authored container operations, and channel projections. It checks the **effective referent**, including a shared cell reached through another path, and records which Law authorized or refused the act. A fast path may cache a proved decision only with declared dependencies on Law text, policy-gate state, actor/jurisdiction, path-to-cell bindings, and relevant Property values. A stale or unknown cache has no authority to grant access. What a denied read reveals through error text also needs an explicit Law-governed disclosure rule: the engine can name the refused operation without leaking the protected value.

`Law::applyTo` already checks authorship, target-Law authority, kernel boundaries, jurisdiction, and conditions. `Law::buildProperties` already registers some of a Law's state (`enabled`, `conditionMode`, `name`, `drives`), allowing Metalaws. The Law's condition/action text and its dependencies are also memory with meaning; a future access model must expose their governable representation without exposing `authorityLevel` as an authored setter or inventing a hidden path by omission. The existing `TransferPolicy` gate is specifically used by set-to-set capture and replay. Its law-writable `gate.*` Properties demonstrate the direction, but they are not evidence that arbitrary `PropertyPath::getValue` or `setValue` is already permission-checked.

### 5b. Bootstrap: Laws govern Law memory only after a first act

No access Law can be read or run before its text is present and admitted. Requiring an already-running Law to authorize the first Law is a circular dependency. The bootstrap sequence is therefore explicit:

1. **Kernel substrate admits and verifies.** It parses bounded Law text, resolves recorded authors, enforces the authored authority ceiling and immutable body/Kernel guards, and checks First Mover provenance. Zach's prohibition on positively forcing a Person's body/location into a particular position belongs here before any Metalaw bootstrap. These are irreducible machine acts, not a standing C++ policy deciding which ordinary domain Properties Persons may access.
2. **A recognized First Mover seeds the initial access Laws.** The seed names the Person whose authority it carries and the exact Law records it installs. It follows the existing First Mover register and save-file discipline. An absent author leaves a Law `Unauthored`; an unattested injection is quarantined, visible, and inert. A seed cannot attest itself or raise a Law's authority by writing a file.
3. **Bootstrapping Metalaws configure `TransferPolicy`.** Zach's 2026-09-28 ordering makes the configured policy the state subsequent Laws read. Those Laws use their authored condition trees to branch on the policy together with the actor and context. Ordinary reads and writes remain open where no explicit applicable reason closes them; this is the migration default, not a quiet bootstrap exception. Any emergency or migration repair path remains an explicit, provenance-bearing First Mover act.

**Incremental evaluation is part of Zach's direction:** Prophetic Rete should identify which policy, actor, and context changes could affect a branch so irrelevant changes need not trigger its recheck. The current code already offers `@transfer-policy.gate.*` as a Law-readable Property and `ConditionNode::All/Any/Not` branching, and Prophetic analysis records Property names a condition may read. It does **not** yet maintain a complete cached answer for a Zone-scoped access decision: its current index is an ahead-of-time over-approximation, while ordinary Rete/property change facts carry live changes. Future access decisions must declare dependencies on TransferPolicy state, actor facts, relevant Zone/Relation facts, and any other context their authored conditions read. Unknown or stale dependencies must widen to a live check, never reuse a possibly false cached permission. No new condition enum or actor/Zone payload is prescribed by this note.

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

Zach confirmed on 2026-10-01 that a write through either shared path updates one value, while explicitly rebinding one path leaves the other attached to the old cell. Sharing a uniquely owned cell must refuse until the Person explicitly changes it to shared ownership, and a weak read must refuse after the final owner releases the cell. He requires topology to survive across saved Singulars and Zones. Custody must be configurable: one authoritative Zone, synchronized local copies, a dedicated third Zone, or a super-Zone encompassing both participants are all admissible authored arrangements. Custody connects **Zone ↔ bearer, qualified by PropertyPath**. Do not hardcode a custodian enum, decide an offline fallback, or make a cell a Singular. Whether this qualifies the existing ownership/governance kinds or uses a distinct authored custody kind remains open, as do following a change of bearer and nested copy/cycle semantics; do not infer them from C++ pointer behavior.

**Narrow runtime rung, 2026-10-01:** `readMathBindings` preserves the source type, including exact integers, vectors, and existing list/dict references. A `Map` over an existing `ValueLeaf` can pass that typed value through the existing authored Action/JSON/compiler path; numeric operations still perform their own numeric conversions. `PropertyPath` now traverses registered as well as authored containers, validates the entire list index, rejects unfinished paths and invalid offsets, and pins getter-returned container lifetimes only for the access. A member-backed `PropertyRef` supports in-place element writes; a structurally read-only parent refuses them. A getter/setter bridge that cannot safely expose an in-place edit returns `Unsupported` instead of bypassing its setter; replacing the whole value through the setter remains supported. Storage assignment detects equal-content rebinding and notifies its root, while content equality uses iterative traversal and conservatively treats distinct loops as potentially changed, without deciding authored graph-copy semantics. `property_memory_access_test` is the isolated and authored-Law witness; the final focused build and all four tests passed (`property_memory_access_test`, `property_path_precalc_test`, `law_model_test`, `ontomath_test`; 9.03 seconds total), without a full-suite or live Person verdict.

This rung does not implement a generic scalar cell, unique/weak bindings, graph copies, alias-wide change fan-out, cross-Zone custody, or persistent alias topology. Existing container sharing remains the existing C++ `shared_ptr` behavior, and the current save codec still flattens it; it must not be called the completed authored storage mechanism. The operation-local pins are machine lifetime protection, not a new Singular, a path-owned ID, or an authority grant.

Implementation order: (1) specify the binding, ownership, and Law-governed access algebra with bootstrap and save compatibility; (2) make the current Property change feed complete for nested/shared writes and policy changes; (3) implement checked cells and durable alias topology; (4) add typed live OntoMath bindings and channel refusals; (5) measure and selectively optimize storage. The associated one-sentence tasks live in the To-Do list; the original Zach note is preserved in its Specific Task record.

---

*Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:44:32-07:00. Zach originated the Property, memory-direction, live-binding, cache, and Law-governed-permission questions, including the Law-memory bootstrap problem; Codex formalized the distinctions and proposed the safety and proof obligations. This is an architecture proposal, not a verified runtime feature.*

*Codex · GPT-6 · session `01a0e64f-5853-7d30-8196-995b4fd16b89` · 2026-09-28 01:49 PDT. Zach supplied the Zone, Ourverse, Person-key, TransferPolicy, stakeholding, Constitution, and unique-Relation constraints in dialogue; Codex recorded them here without implementing the proposed access system.*

*Codex · GPT-6 · session `01a0e64f-5853-7d30-8196-995b4fd16b89` · 2026-09-28 19:26 PDT. Zach specified bootstrap Metalaws, downstream policy branches, a pre-bootstrap bodily Kernel boundary, and signed ongoing Person–Law consent as a historical sub-Relation when revoked. Codex recorded the boundary, implemented a fail-closed Law rung, and left the unbuilt Relation/authentication mechanics visibly open.*

*Codex · GPT-6 · session `01a0e64f-5853-7d30-8196-995b4fd16b89` · 2026-09-28 19:29 PDT. Zach distinguished prohibited positive location writes from legitimate authored location prohibitions; Codex recorded the distinction and added a focused Law witness for location conditions without forced movement.*

*Codex · GPT-6 · session `01a0e64f-5853-7d30-8196-995b4fd16b89` · 2026-10-01 13:00 PDT. Zach confirmed shared writes, independent rebinding, explicit unique-to-shared conversion, weak expiry, and cross-Zone topology preservation. Codex implemented type-preserving binding reads and checked container access; ownership custody and nested-copy semantics remain open.*
