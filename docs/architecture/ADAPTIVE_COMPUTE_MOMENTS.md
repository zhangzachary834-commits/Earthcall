# Adaptive Compute Moments

*Authorable temporal budgeting for resumable world maintenance*

**Date:** 2026-09-17  
**Status:** Architecture proposal  
**Companion analysis:** `docs/Analysis/SLOW_ADAPTER_CLOCK_COUPLING_AND_ADAPTIVE_COMPUTE_ANALYSIS_2026-09-17.md`  
**Related:** `docs/architecture/law/FORMATION_RETE.md`, `docs/AGENTS.md`

---

## 1. Purpose

Earthcall contains processes whose value increases when they are allowed to improve slowly over time: Formation Rete relevance discovery, route revisitation, relation/index maintenance, geometry preparation, cache building, and potentially other derived structures.

These processes must not become accidental frame taxes.

This document defines a general architecture for giving them:

- an independent temporal domain;
- resumable work;
- adaptive compute budgets;
- explicit foreground safety;
- authorable compute policy;
- Zone-level preparation and charge-up;
- first-class `Moment` integration.

The design begins with the Slow Adapter but is intentionally broader. The adapter is the first clear consumer of a more general principle:

> **Earthcall should be able to decide not only what happens, but how finite computational attention is distributed among kinds of happening.**

---

## 2. Core distinction: clock, scheduler, thread

Earthcall must keep three concepts separate.

### Clock / temporal domain

A clock answers:

> **When is this kind of work due or permitted to progress?**

A relevance-maintenance clock is independent from the render clock when a render frame does not inherently imply one unit of relevance work.

### Scheduler

A scheduler answers:

> **Which due work receives CPU time now, and how much?**

It may use frame slack, deadlines, priorities, hardware capacity, or authored policy.

### Thread

A thread answers:

> **Where may this work physically execute?**

Independent clocks can share one thread. Multiple threads can also share one logical clock. Threading is an implementation choice layered below the temporal model.

Earthcall's first implementation SHOULD support independent temporal domains on one owner thread before introducing graph-concurrent execution.

---

## 3. First-class Moment model

A `Moment` is not merely a timestamp. In this architecture, different kinds of work may be represented by different Moment streams.

Illustrative kinds:

```text
RenderMoment
InputMoment
PhysicsMoment
LawEventMoment

MaintenanceOpportunityMoment
RelevanceImproveMoment
RelevanceRevisitMoment
ZonePreparationMoment
```

No universal rule says that every RenderMoment creates any of the maintenance Moments.

Relations among them are explicit.

For example:

```text
RenderMoment
    -> reports deadline/slack telemetry

Relation change
    -> marks relevance structure stale

MaintenanceOpportunityMoment
    -> permits bounded maintenance

RelevanceImproveMoment
    -> advances an unfinished search

ZonePreparationMoment
    -> temporarily requests a different compute allocation
```

The kernel may implement some of these as derived/scheduler-internal Moments initially. They should nevertheless obey the same conceptual boundary: foreground passage is not identical to maintenance passage.

---

## 4. Maintenance work is a resumable Formation of acts

A maintenance operation SHOULD NOT assume it can complete its whole search in one admission.

Instead, it exposes resumable progress.

Conceptually:

```cpp
struct MaintenanceSliceResult {
    bool finished;
    bool invalidated;
    Duration consumed;
    ProgressToken progress;
};

MaintenanceSliceResult advance(Duration budget);
```

The exact API is not doctrine. The contract is.

A maintenance task must be able to:

1. begin;
2. consume a bounded amount of compute;
3. yield;
4. preserve enough state to resume;
5. detect invalidation;
6. restart or repair safely;
7. publish a completed result only when its own soundness rules permit.

For Slow Adapter traversal, resumable state may include the frontier, relation cursor, partial path, partial member collection, visited structure, and generation snapshot.

---

## 5. MaintenanceOpportunity: the bridge between clocks

The frame loop may remain the physical place where the one-thread scheduler runs. That does not make the maintenance clock frame-bound if the frame only supplies an **opportunity**, not a mandatory step count.

The scheduler derives:

```text
availableMaintenanceBudget
```

from current foreground conditions.

Then:

```text
if maintenance is pending
and admitted budget > 0
    advance pending maintenance until:
        budget expires
        work yields voluntarily
        foreground deadline approaches
        world invalidates assumptions
```

Critically, there is no rule:

```text
one frame = one adapter road
```

or:

```text
one frame = one improve + one revisit
```

The amount of maintenance achieved in a frame may be zero, tiny, or large.

---

## 6. Adaptive budget

Let:

```text
T = target foreground frame period
F = measured foreground work
S = required safety margin
```

Then a scheduler can estimate:

```text
slack = T - F - S
```

and admit some fraction of positive slack to background maintenance.

The implementation SHOULD NOT use only a moving average. It SHOULD consider instability so a system with periodic spikes does not donate aggressively merely because its mean is low.

Useful inputs:

- target FPS / target frame period;
- recent actual frame durations;
- recent high percentile or worst frame;
- variance;
- input latency pressure;
- GPU/CPU synchronization stalls if available;
- maintenance backlog;
- task priority;
- task staleness;
- author-requested convergence;
- thermal/power policy where exposed safely by the platform.

Useful behavior:

```text
more stable slack -> budget grows gradually
less slack / more spikes -> budget shrinks quickly
deadline miss -> noncritical maintenance yields
stable recovery -> budget returns slowly
```

This is a control system, not a fixed per-frame quota.

---

## 7. Hardware adaptation

The same authored world should scale naturally across hardware.

On slow hardware:

```text
small maintenance slices
slow convergence
responsive foreground
```

On fast hardware:

```text
large maintenance slices
fast convergence
responsive foreground
```

At high refresh targets, the per-frame foreground budget is small, so the scheduler may donate less per opportunity.

At lower targets, or when the engine is substantially exceeding its useful FPS target, more compute may be redirected to world maintenance instead of producing frames whose additional rate carries little value.

Thus Earthcall may eventually distinguish:

```text
maximum achievable FPS
```

from:

```text
desired perceptual FPS
```

Compute above the desired foreground requirement can become deeper world preparation.

---

## 8. Authorable compute policy

Earthcall's bound-authoring doctrine applies here:

> Having bounds is doctrine. Their exact operating values should be authorable rather than frozen as invisible C++ truth.

The kernel therefore owns **safety invariants**.

Persons and Laws may author **policy requests within them**.

Possible properties:

```text
Scheduler:
    targetFPS
    maintenanceShare
    frameSafetyMargin
    maxMaintenanceBurst
    spikeTolerance

Slow Adapter:
    desiredBudget
    maxFrameTax
    improvePriority
    revisitPriority
    targetConvergence
    maxStaleness

Zone:
    preparationPolicy
    preparationReadiness
    preferredEntryFPS
    desiredPrewarmBudget
```

These names are illustrative. They SHOULD be modeled using Earthcall's existing authorable property and Law mechanisms rather than becoming a growing enum of engine modes.

A Law may alter the desired policy. The kernel clamps and arbitrates it.

---

## 9. Kernel invariants

Authorability must not mean "a Law can accidentally freeze Earthcall."

The scheduler MUST preserve hard invariants such as:

1. **Yieldability.** Noncritical maintenance cannot monopolize the owner thread indefinitely.
2. **Foreground liveness.** Input/render/essential simulation cannot be starved forever.
3. **Hard ceiling.** A platform/kernel maximum maintenance burst remains enforceable.
4. **Soundness.** Derived relevance structures remain proposals/indexes; they do not silently replace authoritative truth when their contract requires fallback verification.
5. **Generation awareness.** Resumable work detects when its assumptions have become obsolete.
6. **Safe publication.** Partial/incomplete roads are not exposed as complete candidate sets.
7. **No hidden fixed doctrine.** Adjustable policy values should not be buried in C++ merely because the first implementation needs defaults.
8. **Observability.** The world and debugging tools can inspect why compute was allocated or denied.

The authored layer requests temporal emphasis.

The kernel guarantees survival.

---

## 10. Compute allocation as lawful world state

Once compute policy is represented by authorable properties, Earthcall can make finite computation part of the world's lawful structure.

This creates a new class of authored effects:

```text
sacrifice visual temporal fidelity
    -> gain relevance convergence

sacrifice background convergence
    -> gain combat responsiveness

enter contemplative Zone
    -> world thinks more deeply than it moves

enter reflex-critical Zone
    -> world moves more quickly than it reorganizes relevance
```

These are not hardcoded game presets. They are authored relations between the world's circumstances and its computational policy.

---

## 11. Mega Evolution example

An artifact can author a temporary compute reallocation.

Example:

```text
Being: MegaStone

Law:
    WHEN MegaStone activated
    FOR 1 second:
        request renderer targetFPS = lower value
        request adapter budget = much higher
        request improvePriority = high
        request revisitPriority = high
        request targetConvergence = ultra
    AFTERWARD:
        restore previous policy
```

The intended semantics are:

> **Use the stone to mega evolve the adapter.**

The engine does not special-case Mega Stones. The object is powerful because authored Law changes scheduler policy.

This example is intentionally playful, but it demonstrates the architectural separation correctly: a being in the world can lawfully change how much computational attention the world devotes to understanding its relations.

---

## 12. Heavy Zone charge-up

A heavy Zone may declare a preparation policy before entry.

### 12.1 Motivation

Without preparation:

```text
approach
    -> enter
    -> sudden relevance/geometry/Law/cache work
    -> frame spike
    -> recovery
```

With authored preparation:

```text
approach
    -> detect or declare heavy entry
    -> begin preparation policy
    -> temporarily reallocate compute
    -> converge required structures
    -> readiness reaches threshold
    -> restore normal policy
    -> enter prepared
```

The world may deliberately lower target FPS for a short charge interval to gain a large temporary compute budget.

This transforms accidental lag into intentional preparation.

### 12.2 Readiness

A Zone MAY expose a derived readiness property:

```text
zone.preparationReadiness : [0, 1]
```

which can depend on multiple subordinate readiness measures:

```text
relevance readiness
geometry readiness
asset readiness
Law candidate readiness
relation/index readiness
shader/pipeline readiness
other Zone-specific authored requirements
```

No single universal formula is required.

A Zone's Law can define which readiness dimensions matter.

### 12.3 Entry Law

Conceptually:

```text
WHEN
    Person approaches HeavyZone
AND
    HeavyZone.preparationReadiness < threshold

THEN
    request temporary compute policy:
        foreground target slightly reduced
        background preparation increased
    advance preparation

WHEN
    HeavyZone.preparationReadiness >= threshold

THEN
    restore ordinary policy
    permit / prefer full entry
```

Whether entry is actually blocked, merely advised, or allowed with degraded readiness is an authorial policy question, not a kernel constant.

---

## 13. The world can "think harder than it moves"

This architecture creates temporal character.

A combat arena may prioritize:

```text
high target FPS
low noncritical maintenance share
defer deep relevance search
```

A library, dream, oracle, or prophecy Zone may prioritize:

```text
lower visual temporal demand
high relevance maintenance
high revisit depth
aggressive conceptual preparation
```

The ontology is unchanged. Only the budget of attention changes.

This is a direct consequence of treating compute policy as authorable world state rather than an opaque engine preference.

---

## 14. Slow Adapter specialization

For the Slow Adapter specifically, the old fixed shape:

```cpp
step(Budget{
    improve = 1,
    revisit = 1
});
```

SHOULD evolve toward budget-aware resumable maintenance.

The adapter still needs two conceptual rates:

- **improve:** discover routes not yet known;
- **revisit:** challenge old routes so "found a good route once" does not become permanent dogma.

But those rates SHOULD become priorities/quotas within an admitted time budget rather than unconditional per-frame counts.

Example:

```text
maintenance slice = 1.2 ms

scheduler / adapter allocation:
    65% improve backlog
    35% overdue revisit

adapter advances until deadline
then yields with cursors preserved
```

If only 0.1 ms is available, it may do almost nothing.

If 8 ms is deliberately donated during a Zone charge-up, it may make substantial progress.

---

## 15. Priority and convergence

Budget answers "how much compute?"

Priority answers "which pending maintenance deserves it?"

Possible inputs include:

- authored task priority;
- age / staleness;
- number of Laws depending on a road;
- expected reuse;
- imminent Zone entry;
- whether the relevant Person is near the affected structure;
- cost already invested in a partial search;
- safety/correctness urgency;
- preparation contract.

Priority itself should remain legible.

If Earthcall later uses a cost model, it SHOULD distinguish computational cost from ontological/telic strength rather than overloading one vague `Relation::weight`.

---

## 16. Publication and truth

Adaptive scheduling must never blur the line between a partial search and a complete result.

For relevance maintenance:

```text
partial route:
    private maintenance state

complete, current route:
    candidate-producing structure

stale route:
    refused or explicitly marked stale

authoritative condition truth:
    Law evaluates it
```

A compute boost may make the world reach good routes faster.

It does not grant permission to call incomplete routes true.

This keeps performance policy orthogonal to semantic truth.

---

## 17. Suggested implementation objects

The architecture does not require these exact C++ types. The following division is useful conceptually:

### `ComputeScheduler`

Owns:

- foreground deadline model;
- maintenance opportunity calculation;
- kernel clamps;
- arbitration among maintenance clients;
- telemetry.

### `MaintenanceClient`

Exposes:

- pending work;
- priority;
- estimated/sampled cost;
- resumable `advance(budget)`;
- yield state;
- invalidation state.

Slow Adapter becomes one client.

### `ComputePolicy`

Represents effective policy assembled from:

- kernel defaults;
- platform limits;
- Person-authored requests;
- Zone requests;
- active Law effects;
- temporary artifacts/effects.

### `PreparationContract`

Optional Zone-level description of readiness dependencies and desired pre-entry compute behavior.

Again, Earthcall SHOULD prefer reusing existing Singular / Relation / Formation / Property / Law machinery over adding a parallel black-box object model where feasible.

---

## 18. Observability

The scheduler should expose enough truth that a Person or agent can answer:

- Why did adapter maintenance run this Moment?
- Why was it denied?
- What budget was requested?
- What budget was granted?
- What authored Law changed the request?
- Which Zone is charging?
- How ready is the Zone?
- Which maintenance task is consuming the budget?
- How much frame slack exists?
- Did maintenance contribute to a missed frame deadline?
- Is the adapter converging or thrashing?

This is important for Refusal 6 / no-black-box reasoning: scheduler policy that materially affects world behavior must be inspectable.

---

## 19. Example temporal sequence

```text
t0  RenderMoment
    foreground cost low
    scheduler grants 0.7 ms maintenance

t1  MaintenanceOpportunityMoment
    Slow Adapter advances partial route
    yields at budget

t2  RenderMoment
    foreground cost high
    scheduler grants 0 ms

t3  Relation change
    active partial route marked for repair/restart as required

t4  RenderMoment
    foreground stable
    scheduler grants 1.1 ms

t5  MaintenanceOpportunityMoment
    adapter repairs and advances route

t6  Person approaches HeavyZone
    authored Zone preparation Law raises desired maintenance budget
    target FPS request temporarily lowers

t7..tN
    larger maintenance opportunities
    relevance/geometry/Law preparation converge

tN+1
    HeavyZone.preparationReadiness reaches threshold
    ordinary target FPS restored

tN+2
    Person enters HeavyZone
    foreground remains smooth
```

The important property is that the temporal relations are explicit. "A frame happened" is not synonymous with "every subsystem advanced one step."

---

## 20. Architectural laws

The design can be summarized as ten laws:

1. **No universal tick sovereignty.** A FrameMoment is one temporal stream, not time itself.
2. **Pending work owns its agenda.** Foreground passage does not prescribe maintenance progress.
3. **The scheduler owns admission.** It grants bounded compute opportunities.
4. **Maintenance must yield.** Heavy search is decomposed into resumable slices.
5. **Budget adapts to reality.** Hardware and foreground load determine safe grain.
6. **Spikes matter more than averages.** Stability controls donation.
7. **Bounds are doctrine; values are authorable.** Kernel safety surrounds authored policy.
8. **Compute can be lawful world state.** Persons, Laws, Zones, and artifacts may request reallocations.
9. **Preparation may precede manifestation.** Heavy Zones can charge before entry.
10. **Performance never becomes truth.** Derived indexes accelerate discovery; authoritative semantics remain authoritative.

---

## 21. The Heavy Zone principle

The most characteristic expression of the architecture is:

> **Before an expensive world manifests, Earthcall may intentionally gather compute, lower another temporal demand, and prepare the structures that world will require.**

The system does not merely load.

It **musters itself**.

And because the policy is authored, a world can choose *how* it musters: relevance, geometry, simulation, Law preparation, or any future maintenance domain.

---

## 22. Final invariant

A concise implementation test:

> If changing the monitor refresh rate from 60 Hz to 240 Hz causes the Slow Adapter to perform approximately four times as much maintenance per second solely because there are four times as many frames, the temporal domains are still improperly coupled.

The intended result is instead:

> Foreground timing changes the amount of safe compute opportunity. Maintenance progress responds to admitted budget, pending work, and authored policy.

That is an independent clock in Earthcall's sense.

That is also what makes the "Mega Stone" and Heavy Zone charge-up lawful rather than hardcoded: they do not command a special engine mode. They author a temporary claim on the world's finite computational attention.
