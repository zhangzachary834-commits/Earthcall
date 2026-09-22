# Analysis of Spike 2 Lag Regression in LawManager::tick

**Date:** 2026-09-11
**Context:** `synthesis_studio_living` world load lag spikes.
**Initial Median Frame Time:** ~161 ms
**Current Median Frame Time:** ~89 ms

## 1. Algorithmic Bottlenecks Identified & Resolved

### A. The `OnBecomeTrue` Fast Path Exclusion
In `LawManager::tick()`, law evaluations are routed through two paths:
1. **The Rete Fast Path:** For laws with active Rete terminals, scaling with dirty facts rather than the world population.
2. **The Full Sweep Fallback:** A naive $O(N)$ sweep over `sweepSubjects(*law)`, running `conditionsSatisfied` on all matching entities.

Previously, the fast path explicitly excluded edge-triggered laws:
```cpp
if (hasTerminals && law->activation() == Law::Activation::WhileTrue)
```
This forced all `OnBecomeTrue` laws to evaluate via the $O(N)$ sweep every frame.
**Resolution:** By modifying the fast path loop to maintain a `std::vector<Singular*> newlyTrue`, we can evaluate `OnBecomeTrue` laws in the fast path. If a subject was not previously holding the condition (`!wasHolding`), we add it to `newlyTrue` and dispatch drives only for those subjects. This single change cut the frame lag nearly in half.

### B. `PropheticRete::drainAgenda` Vector Shift Overhead
When draining dirty facts, the Rete network used `_facts.erase(it)`. Erasing from the middle of a `std::vector` causes an $O(N)$ memory shift of all subsequent elements. 
**Resolution:** Replaced with an $O(1)$ `swap_and_pop` pattern:
```cpp
if (it != _facts.end() - 1) *it = std::move(_facts.back());
_facts.pop_back();
```

### C. `_conditionMemory` Data Structure (`unordered_set` vs `unordered_map`)
An earlier attempt to optimize memory by swapping `_conditionMemory` from `std::unordered_map<std::string, bool>` to `std::unordered_set<std::string>` actually caused a performance regression. 
**Why:** During a full sweep of 1,600 objects, calling `_conditionMemory.erase(subjectId)` on objects that do not hold the condition forces a hash bucket lookup for a non-existent key. In contrast, an `unordered_map` with pre-allocated buckets can quickly overwrite a boolean via `operator[] = false`.
**Resolution:** Reverted to `std::unordered_map`.

---

## 2. The Final Boss: The Remaining ~75ms Lag

After the above fixes, the frame time stabilized at ~89ms (against an aspiration of 5.5ms). Profiling `LawManager::tick` revealed that almost all of this remaining lag (~75ms) comes from the full sweep of exactly four `WhileTrue` laws.

### The Phantom Laws
Profiling `sweepSubjects(*law)` durations yielded the following:
- `law-4`: ~33.1 ms
- `law-3`: ~25.9 ms
- `law-2`: ~11.7 ms
- `law-1`: ~5.9 ms

**Observations about these laws:**
1. **They all share the same name:** When querying `law->name()`, all four return `"Studio: Show the selected harmony"`.
2. **They are generated at runtime:** Searching the `synthesis_studio_living.json` save file shows only one law with that name, and none of them possess the `"id": "law-1"` attribute. Because Earthcall assigns `"law-" + id` (via `g_nextLawId`) when a Law is instantiated without a strict identifier, these four copies are being spawned dynamically—likely by `LawSynthesis.cpp` or a duplication bug during the Zone instantiation phase.
3. **They are deaf to the Rete network:** According to `ConditionModel::compileToRete()`, if a condition reads a qualified root (e.g., `@state.studio.harmony`), it returns an empty terminal set `{}`. This is an intentional design choice to prevent the law from going "deaf." Because Rete Alpha nodes index on the subject's properties, a law relying on global state wouldn't wake up when the global state changes. Thus, returning `{}` forces these laws into the full sweep fallback.
4. **They trigger an $O(N \times M)$ explosion:** While they fall back to the full sweep, they report `requiredProperties.size() == 2` and actually only sweep **3 subjects** in the zone. However, evaluating `conditionsSatisfied` for just 3 subjects takes up to 33ms (~11ms per object). This extremely high per-object cost strongly implies the presence of a **Quantifier** inside the condition tree, which triggers a nested loop over the entire 1,601-object world for each of the 3 subjects.

### Next Steps for Handoff
To bridge the final gap to the 5.5ms aspiration:
1. **Trace the instantiator:** Identify why four identical copies of `"Studio: Show the selected harmony"` are being spawned at runtime without proper identifiers.
2. **Optimize global/quantifier evaluations:** If these laws must remain off the Rete fast path, we need a mechanism to memoize pure global conditions (`@state.studio.harmony == "piano"`) per frame so they aren't re-evaluated deep inside quantifier loops.
