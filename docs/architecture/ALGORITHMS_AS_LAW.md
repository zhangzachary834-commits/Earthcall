# Algorithms as Law

**How to compile an algorithm — in the full computer-science sense: state, control flow,
iteration, recursion, data structures, termination, complexity — into authored Law.**

**Status:** Every construct below is verified present in the tree at this commit, with
its bound quoted from source. §7's limits are design boundaries, not gaps awaiting work.
**Audience:** agents (and Persons) who know how to write the algorithm in C++ and need
to know how to write it *here*. If your instinct on reading a spec is "I'll add a
function," this document is the correction.
**Companion docs:** `LAW_AND_CREATION_SYSTEM.md` (what a law is), `NEW_KIND_FRAMEWORK.md`
(what a being is), `FIRST_MOVER_AUTHORING.md` (the JSON you will actually emit),
`LAW_MIGRATION_FRAMEWORK.md` (moving an existing algorithm out of C++), `ontomath_fields.md`.

---

## 0. What this document is for

An agent asked to implement A* in Earthcall will write A* in C++. This is not stupidity;
it is the correct reflex everywhere else. The three previous documents in this corpus
each refuse a version of that reflex — for types, for folders, for beings — and each
ends by saying *author it instead*. None of them says **how**, once the thing to author
is not a shape or a robot but a genuine algorithm with a loop and a frontier and a
termination proof.

This document says how.

The obstacle is not expressive power. `LAW_MIGRATION_FRAMEWORK.md` opens by saying so:
*"What has been missing is not expressive power. It is a repeatable procedure."* The law
system has an exact symbolic algebra with derivatives and antiderivatives, first-order
quantifiers, folds over the world, recursive named functions, piecewise guards, a Rete
network, drives, and event minting. What it does not have — until this document — is a
**translation table** from the vocabulary a programmer thinks in to the vocabulary the
substrate provides.

**The one-sentence thesis:** *Earthcall is not a von Neumann machine and algorithms
written for one will not port; it is a production system over a property graph with an
exact algebra attached, and the compilation is mechanical once you know which of its
four kinds of iteration your loop actually is.*

**The corollary that saves the most time:** *every bound in this machine is deliberate.*
`kMaxChainRounds = 8`, `kMaxCallDepth = 32`, a fold's single pass — these are the
anti-Babel ceilings, not limitations to engineer around. An algorithm that needs to
exceed one of them is telling you it belongs in a different kind of iteration (§3), not
that the ceiling is wrong.

---

## 1. What kind of machine this is

Three engines, each doing a distinct job. Almost every compilation mistake comes from
using the wrong one.

| Engine | Where | What it is | What it does for you |
|---|---|---|---|
| **The production system** | `Law`, `ReteNetwork`, `EventBus` | ECA rules over a working memory of facts, with a Rete discrimination network and a per-tick agenda | control flow, dispatch, reaction, cascade |
| **The algebra** | `Singularity/OntoMath` | exact symbolic mathematics — `ScalarForm`, `Piecewise`, `FunctionCall`, `Fold`, `TransFactor` | arithmetic, branching-by-guard, recursion, aggregation |
| **The integrator** | drives, `ActionNode::Flow` | change over time with `t = time.sinceApplied` | iteration across ticks, numerical methods, anything convergent |

And the storage:

| Classical concept | Here |
|---|---|
| a variable | a **property** on a being, addressed by `PropertyPath` (`position.y`, `shape.majorR`) |
| declaring a variable | `ActionNode::AddProperty` — grants a being a property it did not have, persisted in `authoredProperties` |
| the heap | the Universe of beings; `ActionNode::Create` / `Spawn` allocate, `Destroy` frees |
| a struct field | a property; **a pointer** — a `Relation`, which is itself a being |
| an array / set / collection | a `Formation` (members), or the relation graph |
| the program counter | the **agenda** — `_rete.drainAgenda()` each round |
| a stack frame | a `FunctionCall`'s bound arguments (`depth + 1`) |
| global clock | `time`, `time.delta`, `time.sinceApplied` — read-only, resolved on the Universe |

**The single most important structural fact:** there is no instruction pointer you
advance. A law does not *run*; it *holds*. You do not write "then do X" — you write the
condition under which X is true, and the machine finds the moment.

---

## 2. The instruction set

Everything an algorithm needs, and what provides it. Enum values are from
`FIRST_MOVER_AUTHORING.md` §5 (append-only; never invent one).

### 2a. Expressions and arithmetic

| Need | Construct | Notes |
|---|---|---|
| polynomial / multivariate arithmetic | `ScalarForm` — sum of `Term`s (`coefficient · Π varᵉ`) | exact; `normalized()` gives canonical form |
| sin, cos, exp, ln | `TransFactor` | `kind(scale·var + shift)`; `Ln` forces `shift = 0` so `d/dx` stays in the algebra |
| derivative / antiderivative | `ScalarForm::derivative`, `::antiderivative` | **exact**. Integration by parts is honestly `nullopt`, never approximated |
| a named, reusable function | `FunctionDef` in `FunctionRegistry` | pure; composable; **recursive** |
| calling one | `FunctionCall{function, args}` | args are full expressions of the *caller's* variables |
| reading world state into math | `MathBinding` | binds a variable to a `PropertyPath` |
| randomness | `OntoMath::Distribution` — `Uniform`, `Gaussian`, `Bernoulli` | for randomized algorithms |

### 2b. Branching

**There is no `if` statement. There are guards.** `Piecewise::Piece` carries three
independent gates, and when several are set, all must hold:

```cpp
// Piecewise::Piece — ScalarForm.hpp
bool hasLo, hasHi;  double lo, hi;          // 1. interval on inputVariable
std::shared_ptr<ConditionNode> guard;       // 2. a WORLD guard — the full condition
                                            //    calculus decides applicability
std::shared_ptr<MathNode> whereLEZero;      // 3. a PURE guard on local variables:
                                            //    applies where f(vars) <= 0
```

- **Interval bounds** — the classic piecewise function: `x < 0`, `0 ≤ x < 1`, …
- **`guard`** — an arbitrary `ConditionNode` evaluated against the subject. This is how a
  branch asks about the *world*: "…where the subject is inside this region," "…where any
  Person owns a Zone."
- **`whereLEZero`** — evaluated against the local variables alone, **no subject needed**.
  The source names its purpose exactly: *"This is what recursion base cases over
  parameters use ('where x - 2 > 0, escape')."*

Boolean composition is `ConditionNode` `All(3)` / `Any(4)` / `Not(5)`, and **nesting is
parenthesization** — `All(Any(a,b), Not(c))` is `(a || b) && !c`.

### 2c. Statements and effects

| Need | `ActionNode::Kind` |
|---|---|
| assignment | `Set(0)`, `Add(1)`, `Scale(2)`, `Lerp(3)` |
| computed assignment | `Map(8)` — `path := f(bindings)` |
| rate / integration | `Flow(9)` — `path += f(bindings)·dt` |
| time-parameterized | `Drive(4)` — `path := curve(input)` |
| block | `Sequence(5)` (ordered), `Parallel(6)` |
| declare a variable | `AddProperty(12)` / `RemoveProperty(14)` |
| allocate | `Create(11)` (generic), `Spawn(7)` (from a Concept), `Synthesize(17)` |
| free | `Destroy(16)` |
| insert into a collection | `AddElement(13)` / `RemoveElement(15)` |
| emit / dispatch | `Publish(10)` — mint an event |

> **`Create`'s children are a `let` binding.** From the source: *"Its children run **WITH
> THE NEWBORN AS SUBJECT**, so the same Set/Map/AddProperty/AddElement vocabulary shapes
> it — creation and modification are one language."* This is your constructor, and it is
> the only lexical scoping the language has. Use it.

### 2d. Referents — the closest thing to variables in scope

```
""                the law's subject (default)
"@event.subject"  the being the triggering event was about
"@event.object"   the other participant
"@<identifier>"   any being in the Universe, by stable identifier
time              seconds since the world began        (read-only)
time.delta        the last frame's dt                  (read-only)
time.sinceApplied seconds since THIS law began holding for THIS subject
```

`time.sinceApplied` is the loop counter of the entire system. Learn it first.

---

## 3. The four kinds of iteration

**This is the section that does the work.** Nearly every failed compilation is a loop
placed in the wrong one of these four. Each has a different bound, a different
termination condition, and a different meaning.

| # | Kind | Mechanism | Bound | Terminates when |
|---|---|---|---|---|
| **I** | **over data** | `Fold`, `ForAny`/`ForAll`, `Scope::Everyone` | one pass over beings | the pass ends (always) |
| **II** | **within an expression** | `FunctionCall` recursion | `kMaxCallDepth = 32` | a base-case guard fires, or the ceiling returns `nullopt` |
| **III** | **within a tick** | event cascade — a law's `Publish` waking another law | `kMaxChainRounds = 8` | no law is dirty, or the ceiling |
| **IV** | **across ticks** | drives / `Activation::WhileTrue` | **unbounded in time** | the authored `Piecewise` domain stops containing `t` |

### I — Iteration over data: `Fold`

Aggregating across the world is one construct, not a loop:

```cpp
struct Fold {                                    // ScalarForm.hpp
    enum class Op { Sum = 0, Mean = 1, Min = 2, Max = 3, Count = 4 };
    Op op;
    int beingKind;                    // ConditionNode::BeingKind (1 = Object)
    std::string path;                 // property read on each being
    std::vector<std::string> exceptIds;   // "...with possible exceptions"
};
```

*"y := the mean height of all Objects" is one fold.* Empty `Sum` and `Count` return 0
(honest identities); empty `Mean`/`Min`/`Max` are **undefined — `nullopt`, not zero**.
That distinction is a correctness property; do not paper over it.

For predicates rather than values, use the quantifiers `ForAny(9)` / `ForAll(10)` —
first-order over the Universe, with **the inner condition evaluated with each instance as
its subject**.

For *effects* on many beings, set `Scope::Everyone(1)` and let `sweepSubjects` choose
whom: an explicit `targets` Formation if you gave one, otherwise every being carrying the
law's required vocabulary. That is your `for each`.

### II — Recursion inside an expression

The header states the capability precisely:

> *"pure functions, composable and recursive. Iteration is carried through the arguments
> ('f(x², n−1)'), so **primitive recursion** — escape-time fractals included — is
> expressible, bounded by an anti-Babel call-depth ceiling (divergence is honest
> `nullopt`)."*

The base case is a `whereLEZero` pure guard; the recursive case is a `Piece` whose value
is a `FunctionCall`. `FunctionRegistry::kMaxCallDepth = 32`, and exceeding it returns
`nullopt` — **undefined, never a guess, never a crash.** Design your recursion so 32
frames suffice, or move to kind IV.

### III — Cascade within a tick

A law's `Publish` mints an event; the Rete asserts it as a fact; a law bound to that
event fires in the *next round of the same tick*:

```cpp
// LawManager::tick()
for (int round = 0; round < kMaxChainRounds && _dirty; ++round) {
    // Facts asserted before this round are consumed by it; facts asserted
    // DURING it (laws firing events from applyTo) survive into the next
    // round — that's how law chains resolve, bounded by kMaxChainRounds.
```

Eight rounds. This is your bounded fixpoint, your constraint-propagation sweep, your
one-step-of-BFS. It is deliberately too small to hide an unbounded loop in, and
`law_loop_test.cpp` exercises exactly this ("law-chains-law within a tick bounded by
kMaxChainRounds — the first anti-Babel ceiling in code").

### IV — Continuation across ticks: the drive

**This is where real loops live.** From `Law.hpp`:

> *"Drive sessions: change over time that outlives its event. When an `OnEvent` law whose
> action reads `time.sinceApplied` applies, a session begins; every tick re-applies the
> law to that subject with the session's onset as t=0, **until the authored `Piecewise`
> bounds no longer contain t (the bounds ARE the duration)** — then `law-drive-finished`
> is published."*

Read that clause twice: **the domain of your function is the termination condition of
your loop.** You do not write `while (i < n)`. You write a `Piecewise` defined on
`[0, n)`, and the machine stops when `t` leaves it. Termination is not a proof obligation
you discharge separately; it is a property of the text.

And the domain is not restricted to time. `LawManager::runDriveSessions` checks
`actionModel()->definedFor(subject)` each tick, with this note:

> *"The authored bounds ARE the duration — and **ANY bound variable may cut them** (time,
> another being's position, the subject's own state). The drive lives while the function
> is still defined for the subject… **A law whose action has no bounded function drives
> until disabled.**"*

So `while (error > ε)` compiles to a `Piecewise` whose domain is bound to `error` and
excludes `[0, ε]`. `while (x < target.x)` binds to another being's property. The whole
family of `while` conditions becomes *the region where your function is defined* — which
is why an unbounded function is an infinite loop (§9), stated in the source itself.

`Retrigger::Absorb(0)` keeps a running session's clock when the trigger fires again — *"a
block resting in constant collision cannot stack or reset it"*. `Retrigger::Restart(1)`
makes the new trigger a new `t = 0`. Pick deliberately; this is your re-entrancy policy.

### Choosing

```
Does the loop range over BEINGS?                    → I   (Fold / ForAny / Everyone)
Is it a pure mathematical recurrence, ≤32 deep?     → II  (FunctionCall)
Is it a bounded propagation that must settle NOW?   → III (Publish cascade, ≤8)
Does it need many steps, or convergence, or time?   → IV  (drive; domain = duration)
```

---

## 4. The compilation procedure

Six steps. Do them in order; the order is what makes step 5 small.

**1. Locate the state.** Every variable your algorithm mutates must live on a being. Ask
*whose* it is. Scratch state with no natural owner is a signal: either it belongs on the
subject as an `AddProperty` variable, or your algorithm wants a being that does not exist
yet (mint one — `NEW_KIND_FRAMEWORK.md` §3, K0–K2).

**2. Classify the control flow.** §3's four-way question. Write down which kind each loop
is *before* writing any JSON. If a loop does not fit, it is usually two loops of different
kinds.

**3. Push the arithmetic into the algebra.** Anything expressible as `ScalarForm` /
`Piecewise` / `FunctionCall` / `Fold` should be, because that half is exact, printable,
differentiable, and inspectable. Only what genuinely needs to *change the world* becomes
an `ActionNode`.

**4. Name the termination.** For kind IV, the `Piecewise` domain. For II, the base-case
guard. For III, the fact that eight rounds is enough. **If you cannot name it, you have
not finished designing the algorithm** — and the substrate will not let you paper over it,
which is a feature.

**5. Write the law(s).** `FIRST_MOVER_AUTHORING.md` §4d for the JSON. One responsibility
per law; chain by `Publish` rather than by growing one action tree.

**6. Verify by parity.** `LAW_MIGRATION_FRAMEWORK.md` §5.5's probe: run the reference
implementation and the law over the same inputs and compare. For a new algorithm with no
reference, write the reference *first* as a throwaway — the exactness of `ScalarForm`
means agreement should be to the last bit, not to a tolerance.

---

## 5. Worked compilations

### 5a. Accumulate — `total := Σ height over all Objects`

Kind I. One `Fold`, no loop:

```jsonc
{"kind": 8, "path": "total",                  // ActionNode::Map
 "function": { "pieces": [ { "fold": {"op": 0, "beingKind": 1, "path": "height",
                                      "exceptIds": []} } ] }}
```

`Mean`/`Min`/`Max` over an empty world are `nullopt` — the law does not fire rather than
writing a wrong number.

### 5b. Branch — `y := max(a, b)`

Kind: none. Two pieces, one guard each:

```
piece 1:  whereLEZero = (a - b)        → value = b      // applies where a - b <= 0
piece 2:  whereLEZero = (b - a)        → value = a
```

Two `whereLEZero` guards, mutually exclusive except at equality where both give the same
answer. **This is how you write every branch whose test is arithmetic.** When the test is
about the world instead — "is this being inside that region" — use the `guard`
`ConditionNode`, and note that `InRegion(1)` means *a shape you drew is the branch
condition*, evaluated as `evalSdf(region, probe) < 0`.

### 5c. Recurrence — factorial, Fibonacci, escape-time

Kind II. A `FunctionDef` whose body is a `Piecewise` with a pure base case — **written in
accumulator-passing form**, and this is not a style preference.

> **The rule that decides the shape of every recursion you will write here:** a `Piece`
> whose value is a `FunctionCall` returns that call's result *verbatim*. The source is
> explicit — *"When set, the piece's VALUE is a call to a named function (**the
> expression is ignored**)."* There is no way to multiply, add to, or otherwise combine a
> recursive result inside the piece that made the call.

So the textbook form **does not work**:

```
fact(n):
  piece 1:  whereLEZero = (n - 1)     → value = 1
  piece 2:  (otherwise)               → value = call fact(n-1)   ← the "n ·" has nowhere
                                                                    to go; this computes
                                                                    the identity, = 1
```

*(Verified: `scratch/algorithms_as_law_probe.cpp` evaluates this shape at n=5 and gets
**1**, not 120.)*

The arithmetic must ride in the **arguments**, which are full `ScalarForm`s — which is
precisely what the header means by *"iteration is carried through the arguments
('f(x², n−1)')"*:

```
factAcc(n, acc):
  piece 1:  whereLEZero = (n - 1)     → value = acc                     // base: n <= 1
  piece 2:  (otherwise)               → value = call factAcc(n-1, n·acc)
```

*(Verified: `factAcc(5, 1)` → **120**.)*

**Every recursion here is therefore tail recursion with an explicit accumulator.** If you
find yourself wanting `combine(x, f(...))`, add a parameter and carry `combine` into the
argument. Mandelbrot escape-time is the same shape with two carried arguments — the
header names it as the motivating case. Depth ≤ 32.

### 5d. Iterative numeric method — Newton, gradient descent, any convergent loop

Kind IV, and the natural fit. Two ways, and the choice matters:

**As a drive over an authored function of time** — when you know the trajectory:
`position.y := f(t)` with `t → time.sinceApplied`, `f` a `Piecewise` on `[0, T)`. Runs
`T` seconds and publishes `law-drive-finished`. The domain is the duration.

**As a fixed-point iteration** — when you do not:

```
Activation: WhileTrue(1)
Condition:  Not( Compare(|residual|, Near, 0, tolerance) )     // ¬converged
Action:     Map  x := x - f(x)/f'(x)                           // one Newton step
            Map  residual := f(x)
```

Each tick performs one step; the condition stops it. `ScalarForm::derivative` gives you
`f'` **exactly** — you author `f` once and the Jacobian is derived, not hand-coded and not
finite-differenced. This is the single biggest practical advantage the substrate has over
writing the same method in C++.

> **Termination discipline.** Two ways to fail to terminate, and both are real:
> a `WhileTrue` law whose condition can never go false, and — from
> `LawManager::runDriveSessions` verbatim — *"a law whose action has no bounded function
> **drives until disabled**."* An unbounded `Piecewise` is an infinite loop that survives
> save/load.
>
> `LAW_MIGRATION_FRAMEWORK.md` §6.1 permits a law to author *its own condition's domain*
> precisely because **feedback is how numeric laws terminate**. Give every iterative law
> either a convergence test or a bounded domain. Both is better. Preferring the domain is
> better still, because a bound in the text is checked by the machine every tick, where a
> convergence test is only checked if you remembered to write one.

### 5e. State machine

Kind III + the event vocabulary. States are a property; transitions are laws:

```
law "open":   Activation OnBecomeTrue(2)
              Condition  Compare(state, Eq, "closed") AND Compare(pressure, Gt, 10)
              Action     Sequence[ Set state := "open",
                                   Publish "valve-opened" ]
```

`OnBecomeTrue(2)` is **edge-triggered** — it fires on the transition, not every tick the
condition holds. Getting this wrong (using `WhileTrue`) is the most common state-machine
bug here, and `LAW_MIGRATION_FRAMEWORK.md` §3 (R2) states the general rule: *"Edges, not
levels. A per-frame 'still falling' event is a bug."*

### 5f. Graph traversal — BFS over the relation graph

The instructive case, because the naive compilation fails and the failure teaches the
model.

**Naive:** one law that loops the frontier until empty. Impossible — no unbounded
within-tick loop exists (§7).

**Correct:** the frontier is *state in the world*, and one tick is one layer.

```
State:      each being gets  visited (bool)  and  dist (double)   via AddProperty
Seed:       Set  @start.visited := true,  @start.dist := 0,  Publish "frontier-advanced"

law "expand":  trigger  "frontier-advanced"
               Condition  Related(type="edge", other="@event.subject")   // neighbours
                          AND Not(Compare(visited, Eq, true))
               Action     Sequence[ Set visited := true,
                                    Map dist := @event.subject.dist + 1,
                                    Publish "frontier-advanced" (subject = this being) ]
```

Each `Publish` wakes the next layer. Within one tick you get **8 layers** — `kMaxChainRounds`
— and then the tick ends and the next one continues, because `visited`/`dist` are real
properties of real beings and survive the tick boundary. A graph of any depth traverses
correctly; it simply takes `⌈depth/8⌉` ticks.

**And you get something C++ BFS does not:** the traversal is inspectable mid-flight
(every `dist` is a legible property), auditable (every application is an
`ApplicationRecord`), governable (a metalaw can cap it), and it survives save/load. The
frontier is not a `std::queue` that dies with the function — it is the world.

### 5g. Randomized algorithms

`Distribution{Uniform|Gaussian|Bernoulli, params}` supplies the draw; everything else is
as above. Note what you give up: a law that draws randomly is not reproducible from its
text alone, so its `ApplicationRecord` becomes the only record of what happened. For
Monte Carlo, prefer many beings each drawing once (kind I over the results) to one being
drawing many times (kind IV) — the first is inspectable and parallel-shaped, the second
is a hidden accumulator.

---

## 6. Complexity and cost

The substrate has a performance model, and it is not the one your intuition assumes.

| Construct | Cost | Note |
|---|---|---|
| `Fold`, `ForAny`, `ForAll` | **O(beings)** per evaluation | scans the Universe by kind |
| `Scope::Everyone` sweep | **O(beings)** per tick | `sweepSubjects` filters by required vocabulary first — declare `requiredProperties` and the filter shrinks the sweep |
| Rete matching | **incremental** | that is what the network is *for*; bind triggers rather than polling with `WhileTrue` |
| `WhileTrue` law | evaluated **every tick, forever** | the most expensive thing you can write casually |
| `FunctionCall` | O(depth), ≤ 32 | exact-algebra evaluation, no allocation per frame |
| event cascade | ≤ 8 rounds/tick | bounded by construction |

**Three rules that follow:**

1. **Prefer an event trigger to a `WhileTrue` poll.** A law bound to `"collision"` costs
   nothing until a collision. The same law written as `WhileTrue` over an `Overlaps`
   condition costs a scan every tick forever. The Rete exists so you do not pay that.
2. **Declare required properties.** They are the sweep filter. An untargeted law with no
   required vocabulary is "truly about everyone" and pays for it.
3. **A fold inside a `WhileTrue` condition is O(beings) per tick.** Nested over a sweep it
   is O(beings²) per tick. This is the one place where a correct-looking law can quietly
   make the world unplayable — hoist the fold onto a being as a property maintained by a
   cheap event-driven law, and read the property instead.

---

## 7. What is not expressible — and what to do instead

Honesty here is worth more than reassurance, because an agent that believes everything is
possible will produce something that silently does not work.

| Not expressible | Why | Do instead |
|---|---|---|
| an unbounded loop inside one application | `kMaxChainRounds = 8`, and no looping `ActionNode` exists | kind IV — spread it across ticks (§5f) |
| recursion deeper than 32 | `kMaxCallDepth`, returns `nullopt` | restructure as kind IV, or carry more per frame |
| a local mutable array / scratch buffer | there is no local scope but `Create`'s children | make the elements **beings** in a `Formation`; that *is* the array, and it is inspectable |
| arbitrary pointer chasing | no raw references | `Relation` — first-class, weighted, with an event timeline |
| `goto`, early `return` from a `Sequence` | none exist | guards on the pieces; or split into laws chained by `Publish` |
| in-place comparison sort | no random-access swap primitive, no bounded-loop sort | rank via `Fold(Count)` over a comparison condition — *O(n²)* but exact, parallel-shaped, and inspectable. For large *n*, sorting probably belongs to a first mover (§8) |
| integration by parts | `antiderivative` returns `nullopt` | honest by design; supply the closed form as a `FunctionDef` |
| `ln` of a non-positive argument | undefined, `nullopt` | guard the domain with `whereLEZero` |

### On Turing-completeness

Worth stating carefully, because the answer is interesting and an agent will want it.

**Each tick terminates.** Chain rounds ≤ 8, call depth ≤ 32, folds are single passes,
sweeps are finite. There is no way to write a law that hangs a tick. That makes the
per-tick language **total** — a strong and deliberate property, and the reason the
anti-Babel ceilings are where they are.

**Across ticks, the ingredients of general computation are present:** unbounded state
(`Create` and `AddProperty` grow the world without limit), conditional branching, and
unbounded iteration (a drive re-applies every tick as long as its domain holds). §5f is a
worked example of an algorithm whose step count exceeds any per-tick bound and which
still computes the right answer.

So: **bounded per tick, general over time.** I am claiming the ingredients, not
presenting a proof. The practical consequence is the one that matters — *if your
algorithm needs more steps than a tick allows, the answer is always to spread it across
ticks and keep its state in the world*, never to seek a bigger ceiling.

---

## 8. When the answer is *not* a law

Three cases, from `LAW_MIGRATION_FRAMEWORK.md` §1's seams. An algorithm stays in C++ when
it is not a *Decide*:

- **Sense** — reading devices, collision tests, raycasts. First mover, forever.
- **Act** — draw calls, buffer uploads, actuator writes. First mover, forever.
- **Kernel-tier responsibilities** — anything whose disabling leaves an unintelligible
  world. §4 of the migration framework: *"Disable the gravity law and the player floats"*
  is intelligible; *"disable the ground-constraint law and the player falls through the
  floor forever with no way back"* is not. Those stay at R3/R4 permanently.

A tight inner numeric kernel that is genuinely hot and genuinely *sensing* or *acting* is
first-mover code. But note what the migration framework insists on: even then, its
**constants become properties** (R3) so a Person governs the algorithm without owning its
inner loop. "It's too hot to be a law" is a reason to keep the *loop* in C++, never a
reason to keep the *decision* there.

---

## 9. Anti-patterns

| Tell | What it means | Cure |
|---|---|---|
| a `WhileTrue` law with no convergence test and no `t`-bound | an infinite loop that survives save/load | §5d termination discipline |
| `WhileTrue` used for a state transition | level-triggered where you meant edge | `OnBecomeTrue(2)` — §5e |
| a `Fold` inside a `WhileTrue` condition | O(beings) every tick, O(beings²) under a sweep | hoist to a property maintained by an event-driven law (§6.3) |
| one law with a 40-node `Sequence` | a function body pretending to be a law | split; chain with `Publish`; one responsibility each |
| `conditionDescriptions` written, no `conditionModel` | reads as behavior, is not | `FIRST_MOVER_AUTHORING.md` §4d |
| a scratch property on an unrelated being | state with no owner | give it a being of its own (`NEW_KIND_FRAMEWORK.md` K0–K2) |
| asking to raise `kMaxChainRounds` | the loop is in the wrong kind | §3 — it is kind IV |
| `value = n · call f(n-1)` | a call-valued piece ignores its expression; the multiply is silently lost and you get the identity | accumulator-passing form — §5c |
| treating empty-fold `nullopt` as 0 | silent wrong answers on an empty world | branch on definedness; the distinction is deliberate |
| a C++ helper "just for the math" | the algebra is already exact and differentiable | `ScalarForm` — and you get `f'` free |

---

## 10. The point underneath

The reason to compile algorithms into law rather than into functions is not purity, and
it is not that the law system is a better programming language — for many algorithms it is
plainly a worse one, and §7 says exactly where.

It is that **an algorithm written as law is part of the world it acts on.** §5f's BFS is
the compact argument: as C++ it is a `std::queue` that exists for a few microseconds
inside a stack frame, invisible to everything, gone before anyone could ask what it did.
As law it is a frontier made of beings, with every distance a legible property, every
expansion an audited application, every step governable by a metalaw, and the whole
traversal survivable across a save. A Person can watch it run, stop it, change its rule,
and ask it why.

That is the same claim `NEW_KIND_FRAMEWORK.md` makes about robots and
`SUBSTRATE_ORDERING.md` makes about the machine, arriving now at the last thing that was
still hiding in the engine: not what things *are*, but what happens *to* them. An
algorithm is a decision procedure, and decisions are the one thing this architecture has
consistently refused to let C++ keep.

The ceilings are the doctrine in numbers. Eight rounds, thirty-two frames, one pass — each
one says the same sentence: *computation in Earthcall is bounded, visible, and answerable
to a Person.* An algorithm that cannot be written under those limits is not being
obstructed. It is being asked to become the kind of process a world can hold.
