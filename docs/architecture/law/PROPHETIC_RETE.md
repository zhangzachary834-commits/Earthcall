# Prophetic Rete (B-Time Rete)

*The ahead-of-time abstract interpreter over the authored law set.*

**Origin.** The realization is Zach's, written 2026-09-01 in `docs/architecture/law/B-time Rete.md`
and structured into a specification by ChatGPT 5.6 Luna as
`docs/Agenda/Tasks/Specific Tasks/Rete_Truth_Seeking_Focus.md`. Zach named both
**Prophetic Rete** (what it does) and **B-Time Rete** (how its temporal structure is
conceived). This document records what has been **built** as of 2026-09-01, what the
implementation deliberately does **not** yet do, and which decisions are marked ⚑ AUTHOR
because they are Zach's to make.

*Authored by Claude Opus 5, session `01FCzFYJGGqm2VKd3LLfVoxj`, 2026-09-01.*

---

## 1. The claim, exactly

Zach's sentence is the whole thing:

> In Earthcall, changes are not arbitrary. **Changes are caused by Laws and First Movers.**

An ordinary Rete network optimizes by asking, after a change, *"which facts are dirty, and
which rules care?"* Earthcall can ask an earlier question, because the Laws are **structured
data that exist before they fire**. A Law's text already names the properties it reads, the
properties it writes, its Beta constraints, and the OntoMath transformation between them.
Composing those constraints is an ordinary abstract interpretation, and its result is a set
of filters that change only when the Laws do.

> **Ordinary Rete tracks what changed. Prophetic Rete knows what could matter before it changes.**

---

## 2. THE ONE RULE

Everything else is detail.

> **The analysis is an OVER-approximation, and only ever concludes IMPOSSIBLE.**

An answer that is too generous costs a little performance. An answer that is too narrow makes
a Law **go deaf** — it stops firing, silently, while remaining registered, enabled, and
compiled. Nothing in the engine reports that, and no test that does not happen to exercise
that exact Law will notice.

So the discipline, enforced throughout `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp`:

- Anything the analysis cannot read answers **Top** (anything).
- A Law carrying something the analysis cannot read marks the **whole index incomplete**.
- Every filter derived from an incomplete index **fails open**.
- `met()` (narrowing, for `All`) widens to Top on any case it cannot compute — never to Bottom.
- `mayIntersect()` returns `true` for every pair of lattice kinds it does not know how to relate.

`tests/law/prophetic_rete_test.cpp` **Section F** is the section that matters. Every other
section checks that the analysis is clever; F checks that it is safe, by firing real Laws
through a real `LawManager` and asserting they still hear.

---

## 3. What is built

### 3a. The interval algebra — Pass 3's engine
`src/Singularity/OntoMath/ScalarForm.{hpp,cpp}`

`ScalarForm::evalRange`, `Term::evalRange`, `TransFactor::evalRange` — sound interval
arithmetic over the signomial-plus-transcendental algebra. Before this, `MathNode::evalRange`
answered `[-inf, +inf]` for *any* non-constant formula, which made every bound downstream of
authored mathematics worthless. Now:

| authored | over | bounds to |
|---|---|---|
| `2x + 5` | `x ∈ [0, 10]` | `[5, 25]` — Luna's §7 worked example, literally |
| `x²` | `x ∈ [-3, 2]` | `[0, 9]` — the turn at zero is honored |
| `0.5·sin(2πt) + 0.5` | `t` unbound | `[0, 1]` |
| `sin(t)` | `t ∈ [0, 1]` | `[0, 0.841]` — the arc contains no peak, so the endpoints bound it |

Two details are load-bearing beyond this feature:

- **Sin/Cos are bounded exactly**, by locating the peaks and troughs inside the arc, not by
  assuming `[-1, 1]`. That is what makes "this glow law can never exceed 1" a *usable proof*
  rather than a truism.
- **`Interval::operator*` is now NaN-safe.** `0 · inf` is NaN, and `std::min/max` over a NaN
  is not a bound — it silently poisons every interval downstream. `geom::evalRange` feeds
  `MathNode` intervals to the tessellator's straddle test, which *discards* cells whose
  interval does not cross zero, so a NaN bound there deletes cells that really do contain
  surface. Same class of hazard the `Op::Noise` comment already warns about.

### 3b. The abstract value lattice
`Prophetic::Range` — Bottom / Number (interval) / Boolean / Text (a finite set, plus a
"maybe something else" flag) / Top. Not serialized: like `OntoMath::ValueKind`, it is a
judgement *about* authored text, never written into a save, so it may be extended freely.

Booleans relate to numbers, because `bool` is arithmetic everywhere in this codebase
(`propertyValueToNumber` accepts it) and refusing to relate the two lattices would make an
ordinary comparison unprovable in either direction.

### 3c. Write effects — what a Law can put where
`Prophetic::analyzeAction`. Per `ActionNode::Kind`:

| kind | write range |
|---|---|
| `Set` | the literal's exact singleton |
| `Drive` | the `CurveModel`'s range — exact for Constant, `bias ± \|amp\|` for Sinusoid |
| `Map` | **the authored OntoMath, bounded** (§3a) |
| `Add` / `Scale` / `Lerp` | Top — they compose with a value the analysis never saw |
| `Flow` | Top — a bounded *rate* is not a bounded *value*; Flow integrates |
| `AddProperty` | the opening value |
| `Create` / `Spawn` / `Synthesize` / `Destroy` / `AddElement` / `RemoveElement` / `RemoveProperty` / `Publish` / `AuthorZone` / `AddRelation` | **opaque** — these change the *fact base*, not a value in it |
| a `FirstMoverLaw` | **opaque** — it actuates in C++; that is what makes it a first mover |

### 3d. Read demands — what would satisfy a condition
`Prophetic::analyzeCondition`. The combination rules are the part that is easy to get
backwards, and getting them backwards is the difference between a filter and a bug:

- `All` **meets** its children's demands per path (both must hold → the demand narrows).
- `Any` **joins** them — and a path only *one* arm constrains is **unconstrained overall**,
  because the other arm can satisfy the Law without it.
- `Not` and `Ne` widen to Top: the complement of a satisfying set is a hole, and this lattice
  holds intervals and sets, not holes.
- `ForAny` / `ForAll` inner demands are filed as **instance** reads (`aboutInstances`) and
  deliberately do **not** propagate to the parent — meeting "every Object's height > 3"
  against a demand on the subject's own height would be a claim about two different beings.
- `Overlaps` and `Related` consult the collision test and the relation graph. Those reads are
  **not enumerable**, so the whole Law's reads go opaque rather than half-enumerated.

### 3e. The relevance filter — Passes 1 and 2, wired
`LawManager::propheticHears`, consulted from the property-change callback in
`LawManager::connectToEventBus`.

`ReteNetwork::markFactDirty` scans the entire fact list — one `property-state` fact per
property per being, which on a real save is thousands — on **every property write in the
engine**. A property no authored condition can read cannot matter, whoever wrote it, so the
scan is skipped entirely. That is the difference between O(world) and O(1) on the single
hottest callback in the engine.

It fails open three ways, in the order they are cheapest to check:

1. **Stale** — one integer compare against `Law::textRevision()`, a counter bumped by every
   edit to any Law's condition or action model and by every Law entering or leaving the
   register. This is what makes the gate safe to consult from a callback that runs *between*
   ticks.
2. **Incomplete** — some Law reads through a closure, a collision test, or a condition kind
   this build does not know.
3. **Foreign alpha** — `ReteNetwork::hasForeignBoundAlpha()`. Alpha nodes now carry an
   `AlphaSource` provenance tag: `Interned` (a `type == x` node), `Authored` (compiled from a
   `ConditionNode`, so its *text* is readable even though its closure is not), or `Foreign`
   (a hand-written predicate — the graph editor, a test, a channel). The default is `Foreign`,
   because a caller that has not said where its predicate came from has not earned the
   assumption that it can be reasoned about.
   > This distinction is why the existing `hasOpaqueBoundAlpha()` could not be reused: it
   > counts every non-interned node, authored conditions included, and would have switched
   > the filter off in every world that has laws in it.

### 3f. The prophecy — what the interpretation concludes
`Prophetic::Index::unreachable()`. Two findings, and only these two:

- **`selfImpossible`** — proved from one Law's text alone. An authored `Zone` function whose
  whole range misses its own satisfaction window can never be satisfied by anything, ever.
- **no lawful driver** — some Law writes the path, but the union of *every* authored write to
  it is disjoint from the demand. Reported only when the whole index is complete **and
  nothing in it writes opaquely**.

**Read the second claim exactly. It is NOT "this law can never fire."** Properties are also
moved by First Movers and by foreign channels, whose transforms this analysis cannot see —
Zach's §20/§21 edge case. What it says is: *nothing in this world's law text can carry that
property into that range.* Correspondingly, "nobody writes it" is deliberately **not** a
finding: a property no Law touches is exactly what a tool or a First Mover moves, and most
of them are.

Findings go to the audit log on every rebuild, and `Index::toJson()` renders the whole
possibility space — read names, write ranges per path, per-law reads and writes with their
reasons. Nothing here is a black box.

---

## 4. The bug this uncovered

Building Section F surfaced a pre-existing failure in the substrate the whole design stands
on, and it is worth naming plainly because it is severe.

**`PropertyRef::set` was the only place in the engine that called `notifyPropertyChanged`.**
Every property backed by anything else was invisible to the change feed:

- `Object::position` and `rotation` live in the transform matrix and are `ComputedProperty`;
- shape parameters, shape kind, field shapes, patch controls, rigid forms and **face colours**
  go through seven hand-written `Property` bridges in `ObjectProperties.cpp`;
- every `Relation` property is a `ComputedProperty`;
- **authored** properties (`AddProperty` — Refusal 6's "the vocabulary a Person adds") live in
  the dynamic map.

None of them ever marked a fact dirty. So a `WhileTrue` law watching `position.y` matched only
the beings that *already* satisfied it when the network first met them, and went permanently
deaf to anything that moved afterwards — silently, because the law stayed registered, enabled
and compiled while its alpha memory simply stayed empty. This is almost certainly the "we
stopped one implementation step short" that the To-do list's *"Audit whether the Rete is
actually skipping known facts"* item was reaching for.

**The fix** is at the two write seams rather than in each Property subclass, so a new bridge
cannot forget to announce itself:

- `PropertyPath::setValue` — the one seam every path-addressed write passes through, whatever
  backs the slot. `PropertyPath::resolve` gained an optional `owner` out-parameter, because
  the notification needs the Singular the property is registered *on*, which is not the root
  once a path descends through a nested Singular.
- `Singular::setDynamicProperty` — authored properties.

**What this still does not catch, stated plainly:** a direct C++ setter (`obj.setPosition(...)`)
writes the transform without going through the property vocabulary at all. That was always
outside the property layer's reach — it is the boundary, not an oversight — and the per-frame
world seeding is what keeps such writes from being lost entirely. Closing it properly is a
separate piece of work (see §6).

---

## 5. What is deliberately NOT built

Named so nobody mistakes the foundation for the whole design.

**§9, ActionNode → Beta back-pointers.** Zach: *"the relevant action nodes get an innate
pointer to the relevant beta-chain's evaluation criteria... it immediately links up the
finished value to the relevant beta branches."* The index now computes exactly which
(writing action, reading condition) pairs are live, which is the prerequisite. Installing the
pointers and evaluating at write time — collapsing the assert/propagate/drain round trip into
one step — is the next commit, and it is where the measured win lives.

**§7 Pass 4, current-situation filtering.** The four-pass model's last pass ("would this
branch fire in the situation we are *actually in*") is what the existing Rete already does.
Passes 1–3 are the new ones, and they are what is here.

**A fixpoint over the write graph.** `Add` / `Scale` / `Lerp` / `Flow` all answer Top today
because they compose with a value the analysis never saw. A standard widening fixpoint over
the write graph would bound many of them, and would turn "this law can only push health
between 0 and 100" into a proof. This is the single highest-value refinement remaining.

**⚑ AUTHOR — §16–19, Rete evaluation as Singulars, and the First Mover stratification.**
Zach's question — *"what if Rete evaluation itself were Singulars exposed with
propertypaths?"*, and the joystick → Singularity-Kernel → abstract-interpreter-metalaws →
main-Rete strata — is an architectural decision, not an implementation detail, and Zach's
note says *"Leave architectural decisions for me."* Nothing here forecloses it: `Prophetic::Index`
is derived state over the law text with a complete `toJson()`, so if the strata are adopted,
this becomes the C++ floor those metalaws stand on rather than something to unwind.

**⚑ AUTHOR — §12, metalaws that rewrite laws.** `Law::textRevision()` is bumped by every
model edit including one performed *by* a Law, so a metalaw rewriting a law already
invalidates the index correctly. Whether the interpreter should instead *synthesize through*
the metalaw (§13's law-synthesis path) rather than re-deriving is Zach's call.

**§20/§21, the unknown-variable model.** Foreign writers are handled today by the blunt
instrument: any opaque writer anywhere suppresses cross-law findings. Modelling a Singular-
represented external agent as a genuine *unknown variable* — keeping the Beta-side constraints
while treating only its transform as unconstrained — is the finer version Zach describes, and
it is not built.

**Rendering.** Zach: *"when rendering itself is handled by Laws everything I said here also
applies to rendering."* True, and the interval algebra in §3a is already shared with the SDF
tessellator. Nothing further is done.

---

## 6. Where the code is

| what | where |
|---|---|
| the interval algebra | `src/Singularity/OntoMath/ScalarForm.{hpp,cpp}` — `evalRange` on `TransFactor`, `Term`, `ScalarForm`; `Interval::joined/met/overlaps/bounded` |
| the interpreter | `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.{hpp,cpp}` |
| the wiring | `LawManager::syncProphetic` / `propheticHears` / `prophetic()` in `Law.{hpp,cpp}` |
| alpha provenance | `ReteNetwork::AlphaSource`, `hasForeignBoundAlpha()` in `Law.{hpp,cpp}`; tagged in `ConditionModel.cpp` |
| the change-feed fix | `PropertyPath::setValue` + `resolve`'s `owner` out-param; `Singular::setDynamicProperty` |
| the tests | `tests/law/prophetic_rete_test.cpp` — **Section F is the safety section** |

---

## 7. Refusals check

- **#1** no domain noun in C++. `Range`, `WriteEffect`, `LawFacts`, `Index` are substrate
  analysis machinery, the same tier as `ReteNetwork` itself — not things in the world.
- **#2** no new top-level directory. Law analysis lives with Law, in `ZonesOfEarth/AuthorsOfLaw/`.
- **#3** no new serialized enum of kinds. `Range::Kind` and `ReteNetwork::AlphaSource` are
  never written to a save; both carry the `ValueKind` comment saying so.
- **#6** no black box. The index is derived state over law text — the Laws remain the truth —
  and it renders itself completely through `toJson()`. Its findings reach the audit log. The
  change-feed fix in §4 is Refusal 6 in the other direction: state that no law could *see*
  change was exactly a black box.
- **#7** no new methods for variable behavior. The analysis is Kernel-tier sense: it decides
  nothing about what anything *is*, only about what the authored text makes possible.
