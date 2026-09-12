# Audit: Directional Blindness in Rete Cross-Subject Joins

**Origin.** The necessity of this audit was prompted by Zach on 2026-09-03, who questioned whether the engine's practice of dropping cross-subject condition checks from the Rete index was actually viable for complex multi-subject evaluation. In response to his prompt to evaluate the architecture, I (Antigravity) analyzed the execution path and derived the resulting logical flaw—Directional Blindness (where laws fail to wake up if the remote target moves). This document formalizes that finding, exposing how Earthcall's current engine functions merely as a single-subject index rather than a true relational Rete.


> **⚠ CORRECTION (2026-09-08, Claude Opus 5, session `01Jf1mZyMWX69HHkG43qMv3F`).**
> **The central finding below does not reproduce.** Scenario B in §2 fails in no activation
> mode: `WhileTrue` terminal memories hold *state* facts, which survive `retractFirst` and are
> **polled every tick**, and `applyTo` re-evaluates the whole condition tree — qualified-root
> conjunct included — before firing (`Law.cpp:382`); `OnBecomeTrue` never takes the reactive
> path at all (`Law.cpp:1778`) and sweeps every tick; `OnEvent` firing on its event is what an
> event law *means*. So the drop at `ConditionModel.cpp:500` is a widening, as its comment
> claims. §3's premise is also inexact: a qualified root resolves to *one named being*
> (`ActionModel.cpp:206`), so the many-to-many join a Beta network exists for is not
> expressible in the condition language, and §4's "dodged the hard relational problem" is
> backwards — kinds 12 and 13 were `ForAnyPair`/`ForAllPair`, **built and deliberately burned**
> in favour of modelling pairs as Relations (`ConditionModel.hpp:37`).
>
> This audit nonetheless asked the right question — Zach's question, *can the Rete honestly and
> swiftly evaluate multi-subject conditions* — and checking its claim is what surfaced the two
> real defects. Those, the corrected analysis, and the architecture that answers them are in
> [`docs/architecture/law/FORMATION_RETE.md`](../architecture/law/FORMATION_RETE.md) §1.
> Antigravity's original text is left unedited below.

## 1. Executive Summary
Earthcall's Rete network currently drops cross-subject conditions (e.g., `@target.position.y > 10`) during compilation to avoid the combinatorial explosion of Beta memory joins. While this succeeds in preventing $O(N^2)$ performance degradation for highly continuous data, it introduces a fatal logical flaw: **Directional Blindness**. Laws cease to be true declarative statements and secretly become directional event listeners. The engine currently masks this limitation by falling back to an $O(N)$ brute-force sweep for continuous laws, which establishes a hard architectural ceiling on world scale.

## 2. The Mechanism of Failure
When a Law's condition AST contains a reference to another being's state (identified by a qualified root, such as `@target`), `ConditionNode::compileToRete` deliberately drops that clause from the index:

```cpp
// ConditionModel.cpp : ConditionNode::compileToRete
// A conjunct that reads someone ELSE'S state is dropped from the index...
if (child.readsQualifiedRoot()) continue;
```

This ensures the Rete network only ever acts as a candidate filter for the primary subject's local state.

### The Breakdown Scenario
Consider the Law: `All( Compare(position.x == 24), Compare(@target.position.y == 10) )`

* **Scenario A (Works):** The target is already at `y=10`. The primary subject moves to `x=24`. The Alpha node monitoring the primary subject's `x` triggers, pushing the Law onto the Agenda. The dynamic evaluation (`Law::applyTo`) checks both conditions, passes, and fires.
* **Scenario B (Fails):** The primary subject is already resting at `x=24`. The target moves to `y=10`. Because the target's condition was dropped during compilation, **there is no Alpha node monitoring the target's Y coordinate**. The Rete network remains entirely silent. The Law never wakes up.

The logic is no longer declarative ("When A and B are true"). It has degraded into procedural listening ("When A happens, check B").

## 3. The Identity of the Network: A "Dishonest" Rete
This limitation exposes a fundamental architectural truth: **Earthcall does not actually possess a Rete network.** 

The entire purpose of the classical Rete algorithm (Forgy, 1979) is to efficiently solve the many-to-many, multi-subject join problem. Rete's defining superpower is its Beta network, which caches the exact cross-product of different entities matching complex relational rules. 

Earthcall adopts the structural shape of Rete (Alpha nodes, Beta nodes, memories, and tokens) but deliberately neuters the Beta nodes. Because Earthcall forces every Beta join to check `leftSubject == right->subjectId` and drops cross-subject clauses entirely, it strictly forbids multi-subject tokens from forming. Functionally, it is not a Rete network; it is merely a **Single-Subject Forward-Chaining Index** wearing a Rete network's clothes.

## 4. The Motivation: Prophetic Bounds vs. Continuous Data
Why was it built this way? The architecture was laser-focused on the *Prophetic* part of "Prophetic Rete"—achieving ahead-of-time mathematical bounds checking (e.g., proving $2x+5$ can never exceed 100). The engine possesses a brilliant abstract interpreter for math, but when it came to the runtime event-triggering, maintaining cross-subject joins for continuous properties (like 60fps positions) would cause severe $O(N^2)$ memory thrashing. 

By dropping remote state from the index, Earthcall dodged the hard relational problem entirely, sacrificing the defining feature of Rete to maintain an $O(1)$ fast-path per subject.

## 5. Current Mitigation: The Universe Sweep
Earthcall survives this flaw because continuous (`WhileTrue`) laws largely bypass the network's limitations. As noted in `rete_compile_test.cpp`, continuous laws fall back to the "Sweep Path." 

Every tick, the engine loops over every entity via the `Universe` provider and brute-forces the AST evaluation (`conditionsSatisfied(target)`). The so-called Rete network is only serving as an optimization for discrete (`OnBecomeTrue` / `Event`) state changes on single subjects.

## 6. Architectural Verdict
The current design is a pragmatic compromise, but it is fundamentally incomplete for a true relational reasoning system. As the simulation scales to thousands of concurrent beings, the $O(N)$ continuous sweep fallback will become an insurmountable bottleneck.

If Earthcall is to support rich relational law authoring, the "drop and sweep" compromise—and the single-subject index masquerading as a Rete network—must be replaced. The engine requires a true relational solution capable of resolving cross-subject state without combinatorial explosion (e.g., via spatial partitioning adapters for Beta nodes, or targeted dependency graphs).

---
*Authored by Antigravity, session b7b980a8-6cb5-452f-a382-0c554ebd1d69, 2026-09-03T18:46:02-07:00.*
