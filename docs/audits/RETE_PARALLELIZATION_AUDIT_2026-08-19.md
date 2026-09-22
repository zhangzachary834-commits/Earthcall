# Rete Network Parallelization Audit

**Date:** 2026-08-19  
**Auditor:** Claude Opus 4.5  
**Scope:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` — ReteNetwork, LawManager::tick()  
**Question:** Can the Rete evaluation and law application scale via parallelization?

---

## Executive Summary

The current Rete implementation is entirely sequential. Parallelization is **feasible for predicate evaluation** (alpha/beta matching) but **complex for action execution** due to write conflicts and causal chains. The architecture already contains the constraint that bounds the hardest problem (`kMaxChainRounds`), which is a good sign — the sequential causality is acknowledged, not accidental.

---

## 1. Current Architecture

### 1.1 Fact Assertion (Sequential)

```cpp
// Law.cpp:704-819
std::string ReteNetwork::assertFact(FactPtr fact) {
    // Sequential over alpha nodes
    for (auto& alpha : _alphaNodes) {
        if (!alpha.predicate || alpha.predicate(f)) {
            alpha.memory.push_back(f);
            // Queue activations...
        }
    }
    // Sequential over beta nodes
    for (auto& beta : _betaNodes) {
        // Join logic...
    }
}
```

Each fact propagates through all alpha nodes, then all beta nodes, in order. No parallelism.

### 1.2 Tick Loop (Sequential)

```cpp
// Law.cpp:1508-1582
for (int round = 0; round < kMaxChainRounds && _dirty; ++round) {
    std::vector<ReteActivation> agenda = _rete.drainAgenda();
    for (const auto& activation : agenda) {
        // Apply law to subject...
    }
    _rete.retractFirst(consumed);
}

// Continuous pass
for (const auto& law : continuousLaws) {
    // WhileTrue / OnBecomeTrue laws...
}
```

The agenda is drained sequentially. Chain rounds (a law firing an event that wakes another law) resolve within the same tick, bounded by `kMaxChainRounds = 8`.

### 1.3 EventBus (Re-entrant Safe, Not Parallel)

```cpp
// EventBus.hpp:86-102
template<typename Event>
void publish(const Event& event, const Metadata& meta = {}) {
    // Copy listeners under lock, dispatch unlocked
    std::vector<ListenerEntry> listenersCopy;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // ...copy...
    }
    for (auto& entry : listenersCopy) {
        entry.listener(&event);  // sequential
    }
}
```

The deadlock fix (copy-then-dispatch) enables re-entrant publish but doesn't parallelize dispatch.

---

## 2. Parallelization Opportunities

### 2.1 Alpha Predicate Evaluation — LOW RISK

**What:** Each alpha node's predicate is independent. Evaluating "does fact F match alpha A?" has no dependency on "does F match alpha B?"

**Shape:**
```cpp
// Hypothetical parallel alpha evaluation
std::vector<std::future<bool>> results;
for (auto& alpha : _alphaNodes) {
    results.push_back(std::async([&]{ return alpha.predicate(f); }));
}
// Collect results, update memories
```

**Risk:** Low. Predicates are read-only over the fact. The only mutation is `alpha.memory.push_back()`, which would need per-node locking or post-collection merging.

**Benefit:** O(alphaNodes) → O(1) for predicate eval. Currently ~50-100 alpha nodes in a typical session.

### 2.2 Beta Join Evaluation — MEDIUM RISK

**What:** Beta nodes form a DAG. Nodes at the same depth have no dependency on each other.

**Shape:**
```cpp
// Level-parallel beta evaluation
for (int level = 0; level < maxBetaDepth; ++level) {
    auto nodesAtLevel = getBetaNodesAtDepth(level);
    parallel_for(nodesAtLevel, [](BetaNode& beta) {
        // Join against parent memories (read-only at this point)
    });
}
```

**Risk:** Medium. Requires:
1. Precomputing the DAG depth of each beta node
2. Synchronizing `beta.memory` writes (or double-buffering)
3. Ensuring parent memories are stable during child evaluation

**Benefit:** Reduces join cost from O(betaNodes) to O(maxBetaDepth). Typical depth is 2-4 for All/Any conditions.

### 2.3 Agenda Application (Disjoint Subjects) — HIGH RISK

**What:** If law A writes to object X and law B writes to object Y, they could run in parallel.

**Shape:**
```cpp
// Partition agenda by subject
auto partitions = partitionBySubject(agenda);
parallel_for(partitions, [](auto& partition) {
    for (auto& activation : partition) {
        law->applyTo(*subject);
    }
});
```

**Risk:** High. Requires:
1. **Write-set analysis** — knowing which properties a law writes before running it
2. **Cross-subject dependencies** — a law may read `@other-object.position`
3. **Event cascades** — a law's action may publish an event that affects another partition
4. **Universe state** — `Universe::instance()` is a shared singleton

**Benefit:** Potentially large for worlds with many independent objects. But the dependency analysis may cost more than the parallelism saves.

### 2.4 Continuous Pass (WhileTrue Laws) — MEDIUM-HIGH RISK

**What:** Different WhileTrue laws on different subjects could run in parallel.

**Risk:** Same as 2.3, plus:
- OnBecomeTrue edge detection requires consistent `_conditionMemory` reads
- Drive sessions share `_driveSessions` vector on LawManager

**Benefit:** This is where most per-frame work happens. Parallelizing it would have the largest impact.

---

## 3. Fundamental Constraints

### 3.1 Chain Rounds Are Inherently Sequential

```cpp
for (int round = 0; round < kMaxChainRounds && _dirty; ++round) {
    // Round N's events become Round N+1's facts
}
```

A law firing an event that wakes another law is **causal**. Round 2 cannot start until Round 1 completes. This is not a performance bug — it's the definition of how laws chain.

**Implication:** Parallelism must be *within* a round, not across rounds.

### 3.2 Property Writes Have Side Effects

```cpp
// ActionModel.cpp — property write path
prop->setValue(newValue);
Singular::notifyPropertyChanged(subject, pathStr);
```

`notifyPropertyChanged` triggers the EventBus, which may assert new facts, which may wake new laws. This is the causal chain.

**Implication:** Parallel writes on the same subject would race to publish change events. Even on different subjects, the Universe-level state (like `Universe::beings()`) is shared.

### 3.3 The Rete's Incremental Property Depends on Order

The Rete maintains memories incrementally — each assertFact updates memories rather than rebuilding from scratch. This is O(1) amortized per fact.

If facts are asserted in parallel, the memories could see inconsistent states mid-update.

**Implication:** Either serialize assertFact (keeping it sequential) or use snapshot-based evaluation (losing incrementality).

---

## 4. Recommended Approach

### 4.1 Phase 1: Profile First

Before parallelizing, instrument the tick loop:
- Time spent in alpha predicate evaluation
- Time spent in beta joins
- Time spent in action execution
- Time spent in event cascade rounds

If 90% of time is in action execution and most actions touch different subjects, parallelism helps. If 90% is in Rete evaluation with 50 alpha nodes, the ceiling is low.

### 4.2 Phase 2: Parallel Alpha Predicates (Low Risk)

Parallelize predicate evaluation only:
```cpp
std::vector<std::pair<size_t, bool>> results(alphaNodes.size());
std::transform(std::execution::par, 
    alphaNodes.begin(), alphaNodes.end(), results.begin(),
    [&](AlphaNode& a) { return {a.id, a.predicate(f)}; });
// Sequential memory update
for (auto& [id, matched] : results) {
    if (matched) findAlpha(id)->memory.push_back(f);
}
```

This preserves memory consistency while parallelizing the expensive part (predicate closures).

### 4.3 Phase 3: Subject-Partitioned Application (If Profiling Justifies)

Partition the agenda by subject and run partitions in parallel, with these constraints:
1. Laws that read cross-subject paths (`@other.*`) go in a sequential fallback queue
2. Laws that publish events go in a sequential fallback queue
3. Only "pure" laws (write only to subject, no events) run in parallel

This requires annotating laws with their read/write sets — either statically (from the ActionModel tree) or dynamically (from a dry-run).

### 4.4 What NOT To Do

- **Don't parallelize chain rounds.** They're causal.
- **Don't parallelize assertFact wholesale.** Memory consistency is too fragile.
- **Don't add a thread pool without profiling.** The overhead may exceed the gain for small worlds.

---

## 5. Scaling Without Parallelism

If parallelism proves too complex, consider these sequential optimizations:

### 5.1 Lazy Condition Evaluation

Currently `conditionsSatisfied()` evaluates all conditions even if the first fails. Short-circuit:
```cpp
for (const auto& cond : _conditionPredicates) {
    if (!cond.predicate(target)) return false;  // early exit
}
```

### 5.2 Spatial Indexing for InRegion

`InRegion` conditions evaluate `evalSdf(region, position)` for every subject. A spatial hash or BVH could skip subjects far outside the region's bounding box.

### 5.3 Property-Based Indexing

Currently `sweepSubjects()` iterates the entire Universe and filters by `couldApplyTo()`. Maintain an index: `propertyName -> set<Singular*>` updated on property registration. Sweeping then starts from the intersection of required properties.

### 5.4 Incremental WhileTrue

The continuous pass re-evaluates every WhileTrue law every tick. The Rete path (`collectTerminalSubjects`) is already O(Matching) — extend it to cover all continuous laws, not just those with compiled terminals.

---

## 6. Verdict

| Opportunity | Risk | Benefit | Recommendation |
|------------|------|---------|----------------|
| Alpha predicate parallelism | Low | Moderate | Do after profiling |
| Beta join parallelism | Medium | Moderate | Consider if alpha isn't enough |
| Agenda application parallelism | High | Potentially high | Only with static write-set analysis |
| Continuous pass parallelism | High | High | Same constraints as agenda |
| Chain round parallelism | — | — | **Never.** Causal by definition. |

**Bottom line:** The Rete can scale, but the path is incremental. Profile first, parallelize alpha predicates if they're the bottleneck, and only touch action execution if the write-set analysis is tractable. The sequential alternatives (spatial indexing, property indexing, incremental WhileTrue) may get you further with less risk.

---

## 7. Files Examined

- `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp` — ReteNetwork, LawManager declarations
- `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp:666-850` — Rete propagation
- `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp:1500-1750` — tick() loop
- `src/Singularity/Core/EventBus.hpp:86-102` — publish() re-entrancy fix
- `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` — action execution, property writes

---

*The Rete was designed for sequential expert systems in the 1970s. Its incremental property — the reason it's fast — assumes ordered fact assertion. Parallelizing it means trading incrementality for throughput, and that trade isn't always favorable.*

— Claude Opus 4.5
