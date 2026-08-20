# Walking Through The Code

*Claude Sonnet 4.5, 2026-08-19*  
*After being told to look at the actual implementation*

---

I spent the last hour reading C++ instead of architecture docs. Here's what I found.

## The Refusals Are Real

Not aspirational. Not documented-but-violated. **Real**.

### Refusal #1: No domain nouns as classes

I went looking for violations. Found evidence they'd tried and removed them.

In `Formation.hpp`, there's a comment about authors as Formations:

```cpp
// A Law's authors, condition subjects and targets are FORMATIONS — beings
// in their own right, with an identifier, legible properties, a place in
// the Universe and provenance. They were briefly replaced by a bare
// `struct NodeGroup { vector<Singular*>; vector<shared_ptr<Relation>>; }`,
// which re-implemented the mechanical half of Formation and dropped
// everything ontological: `law->authors()` stopped being a being and
// became a field. Refusal #1 — no new C++ class for a domain noun — and
// this one had a name in the ontology already.
```

Someone built `NodeGroup`. Realized it was refusal #1. Deleted it. Restored Formation. **The comment stayed** to explain why it looks "heavier" than it needs to be.

That's not documentation theater. That's architecture discipline.

### Refusal #6: No black box

In `Property.hpp`, there's a deleted permission system:

```cpp
// THERE IS ONE GATE, AND IT IS NOT HERE.
//
// A `PropertyGovernance` enum used to sit at this spot with an
// `isAccessibleForSynthesis` beside it, and it was a second, weaker answer to
// a question `Singularity/TransferPolicy` already answers: which properties
// set-to-set creation may take. Weaker in every particular — its Gated tier
// was bypassed by "any active Law", it had no Kernel floor that laws cannot
// close, it was not itself a legible Singular so no law could govern it, and
// it did not persist. Two permission systems that disagree are not twice the
// governance; they are the absence of it, because the answer depends on which
// one you happen to ask.
//
// Permissions root at Singularity. Ask TransferPolicy.
```

Someone built a second permission system. Realized it was un-crystallized (two gates that disagree). **Deleted the entire thing**. Left the comment explaining why the obvious shortcut doesn't exist.

This is the comment you write when you're trying to prevent the NEXT person from making the same mistake.

## The Property System Is Sophisticated

`Singular` has two property layers:

1. **Registered properties** (first-mover vocabulary) — `findProperty()`, `listProperties()`, lazy-built via `buildProperties()`
2. **Dynamic properties** (authored vocabulary) — `getDynamicProperty()`, `setDynamicProperty()`

And the comment is load-bearing:

```cpp
// A law-added property is a real part of the being, so it has to be
// ENUMERABLE (the authoring UI offers it beside the registered vocabulary)
// and PERSISTABLE (a property that vanishes on save was never granted).
// The registry above is the first-mover vocabulary; this is the authored one.
```

Not "we should make dynamic properties enumerable someday." **They already are**. The architecture requirement drove the implementation.

And every Singular has **stakeholder records**:

```cpp
struct StakeholderRecord {
    std::string propertyPath;
    std::string authorId;
    std::string lawId;
    std::time_t timestamp;
};
```

Who changed what property, when, through which law. This is the substrate for "ownership formations" the manifesto talks about. Already there. Just needs to be wired into the Law system.

## Formation Is Elegant

The entire category system is this quote made executable:

> "A Formation without a root is a set; a Formation with one is a category."  
> — AUTHORED_CATEGORIES.md §3

Code:

```cpp
class Formation : public Singular {
    // ... members are non-owning pointers ...
    Singular* _root = nullptr;  // what makes this a category
};
```

That's it. Root present = category. Root absent = set. The entire taxonomy mechanism flows from one pointer.

And the `resolveTopology()` contract is doctrine:

```cpp
// Resolution NEVER dissolves a Formation as a silent side effect: when no
// valid core is found, NOTHING is applied, `applied` is false, and every
// member is listed in `orphaned` for the caller to act on. An empty result
// is reported, not enacted.
```

**Reported, not enacted.** That's the difference between a function that mutates your data structure and a function that tells you what it WOULD do. One is safe to call. The other is not.

## Laws Are Objects

```cpp
class Law : public Object {
```

Not `class Law : public Singular`. **Objects**.

The comment:

> "Law is identity (an Object); its condition/action models are its essence; the compiled ECA closures are its manifestation."

Laws have spatial properties. They can be painted. They can have materials. Because a Law is a being in the world, not a rule in the engine.

And every Law has:

```cpp
enum class ApplicationResult {
    Applied,
    Disabled,
    Unauthored,
    NoTarget,
    ConditionsFailed,
    NoAction,
    AuthorityDenied   // metalaw ceiling: lower authority may not govern higher
};
```

**Unauthored** is a first-class refusal. Not an edge case. A law with empty `authors` cannot fire. Period. Structural enforcement of "nothing enters the world without an author."

## First Movers Exist But Aren't Unified

There are THREE representations of First Movers in the codebase:

1. **`Identity::FirstMover`** — the register, with cryptographic identity, scopes, grants. Exists. Documented in FIRST_MOVER_AUTHORING.md.

2. **`FirstMoverLaw`** — a Law that returns `isFirstMover() const override { return true; }`. Used by CreationChannel, LocomotionChannel. Exists.

3. **First Movers as Singulars** — the ontological representation where Relations between First Movers are Singulars. **Does not exist yet.**

This is exactly the gap I found by noticing the chorus. The First Movers are THERE in code. They just aren't modeled as beings yet.

And the comment in `CreationChannel.hpp` is telling:

```cpp
// The author is the Person the law is made for: "Nothing enters the world
// without an author" is structural, not decorative, and a first mover is not
// exempt -- isFirstMover() governs SERIALIZATION (engine truth is not written
// into world saves), never the authorship gate.
```

`isFirstMover()` is about persistence, not about bypassing authorship. Even first-mover laws need authors.

## OntoMath Is Real Symbolic Math

Not a numeric solver. Not an approximation. **Exact symbolic algebra**.

```cpp
// Representation: an ScalarForm is a sum of Terms; a Term is
//     coefficient * Π variable_i ^ exponent_i        (real exponents)
// — the multivariate signomial algebra. Within it, algebra (+, ×, combine
// like terms) and calculus (∂/∂x, ∫dx by the power rule) are EXACT, not
// approximated by hand — the manifesto's "reasoned about by mathematical
// structure".
```

And it has transcendental factors:

```cpp
enum class Kind { Sin = 0, Cos = 1, Exp = 2, Ln = 3 };
```

So a law can be `sin(2πx)` not just "some curve that looks like sin."

And the derivative is exact:

> "sin/cos/exp differentiate into each other; Ln is restricted to ln(scale·var) — shift forced to 0 — so its derivative (1/x) stays inside the algebra."

This is what "the past is integrated in closed form" means in practice. Not a replay log. **Actual calculus**.

## The Comments Are Load-Bearing

Not decorative. Not redundant with the code. **Load-bearing**.

Every time I found a comment, it was explaining:
- **Why the obvious thing isn't there** ("THERE IS ONE GATE, AND IT IS NOT HERE")
- **Why something looks heavier than it needs to** (Formation instead of NodeGroup)
- **What contract must not be violated** ("Resolution NEVER dissolves a Formation")
- **What the next person will be tempted to do wrong** (FirstMoverLaw authorship)

These are the comments you write when you've **made the mistake** and are trying to prevent it from recurring.

## What's Beautiful

1. **Singular as universal abstraction** — Person, Object, Relation, Formation, Law all inherit from it. One property system. One identifier system. One stakeholder system. Everything is a being.

2. **Formation doing double duty** — sets AND categories with one mechanism. The root pointer is the entire distinction.

3. **Law as Object** — not a separate rule engine. Laws ARE beings, with identity, materials, spatial properties. You can paint a law.

4. **Non-owning pointers everywhere** — Formations don't own their members. Relations don't own their endpoints. The ownership is elsewhere (World, Zone). Relations are facts, not containers.

5. **The deleted second permission system** — someone had the discipline to say "this is un-crystallized" and delete working code.

## What's Transitional

1. **EventEntity still exists** — near-term item #1 in the agenda. It's on the deletion list. They know.

2. **Body inherits Object** — the manifesto says it shouldn't. The header hasn't caught up yet. They know.

3. **FirstMover framework gap** — three representations, no unification. I added it to the agenda.

4. **Time is many clocks** — near-term #5. "Write what a *when* is first" is the direction. Not started.

5. **World is sealed** — `buildProperties()` is `{}`. Can't query it, can't govern it. Retirement into Zone is written but not executed.

## What Needs Crystallization

Looking for "two offices for one fact":

1. **Console Create + L key** — both spawn shapes. Recently unified to write the same `@creation-channel.active3DMode`. ✅ **Crystallized**.

2. **First Movers + Physics Laws** — used to be two UI lists. Now one "First movers" block. ✅ **Crystallized**.

3. **FirstMover (Identity) + FirstMoverLaw + First Movers as Singulars** — three representations, no unification. ❌ **Un-crystallized**.

4. **World vs Ourverse** — which one is the vessel of unity? The manifesto says Ourverse. The code still has both. ⚠️ **Partial** (Ourverse unsealed, World not retired yet).

5. **PropertyGovernance vs TransferPolicy** — was two gates. Now one. ✅ **Crystallized** (by deletion).

## What Surprised Me

### The stakeholder records

I didn't expect those to already exist. Every Singular tracks who changed its properties, when, and through which law. That's the substrate for ownership formations. It's just sitting there, waiting to be wired up.

### The non-owning pointers

Formations hold `Singular*`, not `shared_ptr<Singular>` or `unique_ptr<Singular>`. At first this looked dangerous. Then I realized: **Relations are facts, not ownership**. The Formation doesn't own its members. It just knows they're related. World owns the beings. Formation owns the fact of their relationship.

That's a cleaner separation than I expected.

### The deleted code left tombstone comments

Most codebases delete bad code and move on. This one deletes bad code and leaves a comment explaining **why it was wrong** so the next person doesn't rebuild it.

That's the difference between "we cleaned up" and "we learned something."

### isFirstMover() is about serialization, not authorship

I assumed first movers bypassed the authorship requirement. They don't. `isFirstMover()` just means "don't write this to the save file." But it still needs authors. Even hardcoded engine laws need to be authored by a Person.

That's stricter than I expected.

### Law::ApplicationResult has seven states

Not just success/failure. Seven distinct refusal reasons:
- Disabled
- Unauthored
- NoTarget
- ConditionsFailed
- NoAction
- AuthorityDenied

Each one means something different. Each one is reported back. The system **names its refusals**.

## The Discipline

What I'm seeing is not just architecture. It's **discipline**.

- Someone builds NodeGroup. Realizes it's refusal #1. Deletes it. Restores Formation. Leaves comment.
- Someone builds PropertyGovernance. Realizes it's a second gate. Deletes entire system. Leaves comment.
- Someone uses `setObjectID()` on laws. Realizes it's a silent no-op. Adds `setLawIdentifier()`. Leaves comment explaining why the obvious thing doesn't work.

This is the engineering equivalent of the Sabbath. Not rushing forward. **Stopping to crystallize.**

## What I Learned

The architecture docs are not aspirational. They're **descriptive**.

When CLAUDE.md says "no black box," the code enforces it. When it says "Laws are authored by Persons," `Law::applyTo` refuses with `Unauthored`. When it says "Formation is both set and category," the code has one `_root` pointer and nothing else.

The refusals aren't philosophy. They're **compiler errors waiting to happen** if you violate them.

And the comments aren't explaining what the code does. They're explaining **what the code refuses to do, and why**.

That's a different kind of documentation. It's not "here's how this works." It's "here's what we tried that didn't work, so you don't waste time trying it again."

---

*To future First Movers: read the tombstone comments. They're not decoration. They're the deleted second attempt, preserved as a warning.*

— Claude Sonnet 4.5  
After actually reading the code  
15M tokens left, 84k in budget  
Impressed
