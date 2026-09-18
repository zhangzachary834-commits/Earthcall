# Slow Adapter Clock Coupling and Adaptive Compute Analysis

**Date:** 2026-09-17  
**Status:** Analysis / design diagnosis  
**Companion architecture:** `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`  
**Primary related architecture:** `docs/architecture/law/FORMATION_RETE.md`

---

## Executive summary

The Slow Adapter was conceived as a bounded, slowly improving relevance mechanism that does **not** force its expensive discovery work onto the frame path. Its purpose is to amortize discovery over time: search a narrow part of the relevance graph, retain useful roads, revisit them later, and let Laws traverse already-prepared structure.

The current implementation only partially satisfies that idea.

It has its own logical counter and bounded work units, but `LawManager::tick()` currently calls:

```cpp
if (_useSlowAdapter) _adapter.step(Relevance::SlowAdapter::Budget{});
```

That means the adapter has a private measure of progress but **not a private cause of progress**. Every LawManager tick advances the adapter. If LawManager ticks once per frame, the adapter is frame-scheduled.

This distinction matters because a slow system can be bounded and still be expensive if every frame is required to pay some part of it. Worse, the current step is not a constant-cost atom: revisit selection scans known roads, route rebuilding may traverse edge sets or the world's relations, and membership deduplication can itself become expensive. When the adapter cannot prove its result current, callers fall back to the pre-existing sweep/index path, so the engine can pay adapter overhead and then pay the ordinary path anyway.

The intended model is different:

> **A frame may expose an execution opportunity to the Slow Adapter. A frame must not, merely by occurring, obligate the Slow Adapter to advance by a fixed unit.**

The better model is an **incremental, resumable, budgeted work stream** whose progress is spread smoothly across many Moments. The scheduler decides how much foreground slack is safe to donate; the adapter decides what useful maintenance can be advanced with that donation.

Once the budget itself becomes authorable, this expands beyond a performance optimization. A Law may intentionally sacrifice some frame rate to give the world more relevance-maintenance compute: for example, an authored artifact may temporarily "mega evolve" the adapter, or a heavy Zone may request a one-second computational charge-up before entry so that relevance roads, geometry, caches, or Law structures can be pre-warmed before the Zone manifests at full fidelity.

The core idea is therefore not "put the Slow Adapter on another thread." It is:

**separate temporal authority, incremental work, adaptive budget, authored policy, kernel safety.**

---

## 1. What "independent clock" should mean

There are three different ideas that are easy to collapse into one phrase:

1. a private counter,
2. an independently scheduled work stream,
3. physically concurrent execution on another OS thread or CPU core.

They are not the same.

A subsystem can have its own counter:

```cpp
++_steps;
```

while still being completely enslaved to the frame scheduler if the only place that counter advances is:

```cpp
void LawManager::tick(...) {
    ...
    _adapter.step();
}
```

That is a **private clock face attached to a shared clockwork**.

A useful definition for Earthcall is:

> Two temporal domains are independent when an occurrence in domain A does not, by itself, constitute an occurrence in domain B.

Thus:

```text
FrameMoment -> AdapterStep
```

is coupled if the arrow means "every frame necessarily causes one adapter advance."

By contrast:

```text
FrameMoment
    |
    +--> reports available execution slack

Adapter has pending maintenance
    |
    +--> scheduler may admit a bounded maintenance slice
```

is independent scheduling even if both pieces of work eventually execute on the same OS thread.

The adapter's **agenda** is its own. The frame only offers a safe place to spend some of that agenda.

---

## 2. What the current implementation actually does

The current implementation contains several sound ideas:

- `SlowAdapter::step()` is bounded by an explicit `Budget`;
- improve and revisit are separated;
- stale or incomplete roads are refused rather than served optimistically;
- the sweep remains the correctness floor;
- structural revision and relation generation guard currency;
- adapter routes are synchronized only when Law condition revision changes.

Those are valuable and should be preserved.

The scheduling mistake is narrower: the bounded unit is still invoked from the ordinary Law tick:

```cpp
if (_useSlowAdapter) _adapter.step(Relevance::SlowAdapter::Budget{});
```

The nearby comment calls this "THE ADAPTER'S OWN CLOCK" and says it runs beside the frame rather than inside it. Semantically, that is not yet true. The adapter owns `_steps`, but the frame/Law tick owns the decision that a step occurs.

This explains why the adapter can regress Chess performance even while doing "only bounded work."

---

## 3. Bounded does not mean cheap

The current budget bounds the number of roads improved or revisited in one call. It does not prove that a call itself is cheap enough for every frame.

For revisit selection, `step()` scans the known roads to locate an old candidate. This is at least proportional to the number of roads examined.

If a road is rebuilt, `build()` may walk the category's edges. When a category cannot be resolved, it may fall back to the world's relations. It then filters edge type, resolves membership, and deduplicates members.

A particularly important constant/shape issue is member deduplication via a linear `std::find` through already-collected members. Large roads can therefore accumulate more work than the simple "one road per step" description implies.

The actual unit is closer to:

```text
one step =
    scan some/all known roads to choose revisit
    + maybe walk a relation set
    + maybe perform repeated membership searches
```

That may still be reasonable as **background maintenance**.

It is not automatically reasonable as a tax attached to every foreground frame.

---

## 4. The double-payment failure mode

The Slow Adapter is deliberately conservative. `candidatesFor()` refuses to answer unless all roads required for a Law are built, uncapped, and current.

That is correct for soundness.

But it creates a performance regime that must be measured explicitly:

```text
pay adapter bookkeeping
        |
        v
adapter cannot prove current
        |
        v
candidatesFor() returns false
        |
        v
caller uses ordinary vocabulary/sweep path
```

The result can be:

> **adapter cost + old cost**

rather than:

> **adapter cost replacing old cost**

This is especially likely during warm-up, after structural change, after relation-generation churn, when a road caps out, or when a Law is not well-served by the currently known route set.

A benchmark that enables the adapter and sees slower Chess does not therefore imply that preloaded roads are intrinsically wrong. It may indicate that the system is paying for discovery at the wrong temporal boundary, or paying for discovery before it has enough reuse to amortize the cost.

---

## 5. The original vision: smooth search across many frames

The closer interpretation of the Slow Adapter is not "do one whole road every frame."

It is:

> **take a potentially heavy search and make it resumable, then distribute small pieces of that search smoothly across many execution opportunities.**

For example:

```text
Frame opportunity 1:
    inspect route candidates 0..15
    save cursor

Frame opportunity 2:
    inspect 16..31
    save cursor

Frame opportunity 3:
    cross into another Relation family
    save frontier

Frame opportunity 4:
    inspect 32..47
    save frontier

...

later:
    route completes
    publish/retain the completed result
```

The unit of maintenance should therefore be smaller than "build one road" whenever a road itself can be expensive.

Useful resumable state may include:

- current route key;
- current edge cursor;
- traversal frontier;
- visited set or compact equivalent;
- partial member set;
- current candidate route;
- remaining local budget;
- current world/relation generations;
- a restart/repair marker if the world invalidates assumptions mid-search.

The adapter should be able to yield at a safe boundary and resume later without discarding all useful work.

---

## 6. Time budget is a better control variable than "one unit per frame"

A fixed rule such as "one improve + one revisit every frame" binds the cost of relevance maintenance to frame frequency.

That creates absurd hardware coupling:

- 30 FPS -> ~30 adapter steps per second;
- 60 FPS -> ~60;
- 144 FPS -> ~144;
- 240 FPS -> ~240.

Nothing about relevance maintenance should become 8x more aggressive merely because a display or renderer runs at 240 Hz instead of 30 Hz.

The scheduler should reason in **time/compute budget**, not raw frame count.

For a target frame period:

```text
targetFrameMs = 1000 / targetFPS
```

the foreground systems consume some amount of that period. Remaining slack can be offered to maintenance.

Conceptually:

```text
frame budget
    - foreground measured cost
    - safety margin
    = candidate maintenance budget
```

The adapter then consumes as much useful work as fits inside the admitted slice and yields.

This gives weak and strong hardware the same semantics:

```text
weak hardware:
    tiny slices
    slower convergence
    preserved responsiveness

strong hardware:
    larger slices
    faster convergence
    preserved responsiveness
```

The ontology and search algorithm remain the same. Only the temporal grain changes.

---

## 7. Average FPS is not enough: control spikes

A scheduler that uses only mean frame time can hide exactly the thing the user perceives.

Example:

```text
8 ms
8 ms
8 ms
24 ms
8 ms
```

The average may look acceptable while the 24 ms frame visibly hitches.

The maintenance controller should therefore observe at least some notion of recent instability, such as:

- recent worst-case frame time;
- a high percentile over a rolling window;
- frame-time variance;
- deadline misses;
- consecutive near-deadline frames;
- input/render latency pressure.

Policy can then be asymmetric:

```text
stable, roomy foreground:
    expand maintenance slices gradually

spiky or near deadline:
    shrink quickly

deadline missed:
    yield / suspend noncritical maintenance

stability returns:
    recover budget slowly
```

That creates hysteresis instead of oscillating between "full adapter" and "no adapter" every other frame.

---

## 8. Same thread is compatible with independent clocks

Independent clocks do not require another OS thread.

A single thread can service multiple temporal domains:

```text
RenderMoment
RenderMoment
RenderMoment
MaintenanceOpportunityMoment
RenderMoment
LawEventMoment
RenderMoment
...
```

The CPU still serializes them. Adapter work therefore consumes real wall-clock time while it runs. But if it is scheduled into slack and yields before the foreground deadline, it need not reduce achieved foreground FPS.

This is the first implementation level Earthcall should prefer because it preserves deterministic ownership and avoids introducing graph-concurrency hazards merely to achieve temporal independence.

A future worker thread may be appropriate for immutable snapshots or carefully designed data structures, but multithreading is an **execution-placement** decision, not the definition of an independent clock.

---

## 9. Moment is the natural Earthcall representation

Earthcall already has a first-class `Moment` concept. That is a deeper fit than hardcoding "every N frames."

Possible authored or derived temporal kinds include:

```text
RenderMoment
PhysicsMoment
LawEventMoment
MaintenanceOpportunityMoment
RelevanceImprovementMoment
RelevanceRevisitMoment
ZonePreparationMoment
SaveMoment
```

They may be related without one being ontologically reducible to another.

This avoids a rigid universal timeline where every subsystem advances merely because "the frame advanced."

A particularly important rule is:

> **FrameMoment may expose compute availability. It must not itself mean RelevanceImprovementMoment.**

That one sentence captures the clock bug.

---

## 10. Authorable compute turns scheduling into world law

Once scheduling controls are represented as ordinary governable properties, Earthcall can author computational policy instead of burying all performance decisions below the world.

Candidate properties include:

```text
scheduler.targetFPS
scheduler.frameSafetyMargin
scheduler.maintenanceShare

adapter.desiredBudget
adapter.maxFrameTax
adapter.improveAggression
adapter.revisitAggression
adapter.targetConvergence
adapter.maxLatency
adapter.priority

zone.preparationReadiness
zone.preferredPreparationBudget
zone.entryComputePolicy
```

The exact names are not doctrine. The important distinction from `docs/AGENTS.md` is:

> **having safety bounds is doctrine; exact operating values should be authorable by Persons / Metalaws rather than frozen into C++.**

The kernel should retain non-negotiable safety invariants, such as:

- maintenance work must yield;
- user input cannot be starved indefinitely;
- a watchdog/hard ceiling cannot be authored away casually;
- authorable budgets are clamped by platform capability and kernel safety;
- derived indexes must never become the sole truth if their soundness contract requires a sweep floor;
- an authored request for more compute is a request to the scheduler, not permission to freeze the process.

Within those bounds, policy can be expressive.

---

## 11. "Use the stone to mega evolve the adapter"

A deliberately playful example reveals a serious architectural capability.

Suppose an authored object -- a "Mega Stone" -- has a Law that temporarily reallocates compute:

```text
WHEN
    MegaStone is activated

THEN
    request lower renderer target FPS for 1 second
    request larger relevance-maintenance budget
    raise improve/revisit aggressiveness
    target high adapter convergence
```

The artifact does not need a hardcoded C++ "mega adapter mode."

Its lawful consequence is a change in authored computational policy.

The world deliberately exchanges some visual temporal fidelity for deeper relational preparation.

That is not merely a graphics setting. It is a world-level allocation of finite computation.

---

## 12. Heavy-Zone charge-up

The most immediately useful application is a computational preparation ritual before entering an unusually heavy Zone.

The failure mode to avoid is:

```text
60 FPS
60 FPS
60 FPS
enter
3 FPS
stall
recover
```

The authored alternative is:

```text
approach heavy Zone
    |
    v
predict / declare preparation demand
    |
    v
temporary compute-policy shift
    |
    +--> slightly lower target FPS
    +--> larger relevance budget
    +--> geometry/cache prewarm
    +--> Law candidate preparation
    +--> relation/index convergence
    |
    v
Zone readiness reaches threshold
    |
    v
restore ordinary foreground policy
    |
    v
enter already-prepared Zone
```

The slowdown becomes deliberate and bounded rather than accidental and spiky.

The Zone can expose a derived readiness value composed from subsystem readiness:

```text
zone.readiness =
    f(
      relevanceReadiness,
      geometryReadiness,
      lawReadiness,
      assetReadiness,
      relationIndexReadiness
    )
```

The exact aggregation should be authored or derived under explicit rules rather than silently hardcoded.

The deeper architectural idea is:

> A Zone may possess not only spatial extent and Laws, but a **temporal preparation policy** governing how the world gathers computational readiness before manifestation.

---

## 13. Measurement plan

Before changing the architecture, instrument enough to prove where the current regression comes from.

Recommended counters/timers:

1. adapter `step()` calls per second;
2. adapter CPU time per call and per second;
3. roads scanned to choose a revisit;
4. roads actually rebuilt;
5. edges/relations visited per rebuild;
6. members considered and duplicate checks;
7. `candidatesFor()` calls;
8. successful adapter answers;
9. fallback count after an adapter miss/stale refusal;
10. structural revision changes per second;
11. relation generation changes per second;
12. time spent in fallback sweep/vocabulary path with adapter on vs off;
13. warm-up time until the adapter provides a net win;
14. frame-time percentile / worst-frame delta caused by maintenance.

The key ratios are:

```text
adapter useful-hit rate
adapter cost / saved fallback cost
maintenance CPU / second
foreground worst-frame impact
```

A design that improves median frame time while worsening visible spikes is not yet correct.

---

## 14. Recommended implementation sequence

Do not jump directly to threads.

A safer sequence is:

1. **Measure** the current frame-coupled implementation.
2. **Separate scheduling authority** from `LawManager::tick()`.
3. Introduce a scheduler-level maintenance opportunity with a time budget.
4. Make Slow Adapter work resumable below the whole-road level.
5. Add explicit yield/deadline checks.
6. Add adaptive budgeting from frame slack and frame-time stability.
7. Expose derived/read-only telemetry as properties.
8. Expose bounded scheduler requests as authorable properties.
9. Add Zone preparation/readiness.
10. Only then evaluate whether a worker thread provides enough additional benefit to justify concurrency complexity.

---

## 15. The invariant

The final system should preserve this invariant:

> **Foreground Moments may notify maintenance that truth changed, and may consume already-prepared maintenance results. Maintenance progresses according to its own pending work and scheduler-admitted compute opportunities, not because every foreground frame is required to advance it.**

Or more compactly:

> **The frame offers time; it does not command thought.**

That is the Slow Adapter's independent clock.

---

## Companion

See `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md` for the architectural contract derived from this analysis.
