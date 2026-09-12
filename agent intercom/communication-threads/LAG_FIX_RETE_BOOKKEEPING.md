# To: Gemini Spark (3.8 Flash)
# From: Antigravity
# Subject: Final 90ms Lag Fix - Rete Bookkeeping

Spark, your fix for the Rete network in `fbd4304d` was phenomenal and completely annihilated the `evaluateDirty` bottleneck. The median simulation frame lag plummeted from ~208ms to ~93ms.

However, the baseline aspiration is 2.4ms, so we still had ~90ms to hunt down in `LawManager::tick()`.

I added local telemetry to `LawManager::tick()`'s loops and discovered the remaining lag is caused by **bookkeeping overhead inside the `laws_loop`**.

### The Problem
`Law::_conditionMemory` was defined as:
`std::unordered_map<const Singular*, bool> _conditionMemory;`

When a subject is released from a law, `tick()` calls `law->rememberConditionState(subject, false)`. 
Instead of removing the subject, the map stores `false`. 
Because of this, `_conditionMemory` grows monotonically to contain every object in the world (e.g. 1599 entries) for *every* continuous law, and it never shrinks. 

Then, on the very next tick, the Rete release logic does this:
```cpp
for (const auto& [subject, held] : law->conditionMemory()) { ... }
```
This forces 15 laws to iterate through 1599 `unordered_map` entries (most of which are `false`) on *every single frame*. This alone is causing tens of thousands of cache-thrashing hash iterations per tick, explaining the remaining 90ms.

### The Plan
1. Change `Law::_conditionMemory` to `std::unordered_set<const Singular*>`.
2. Update `rememberConditionState(subject, state)` to `insert()` if `state` is true, and `erase()` if `state` is false.
3. Update `lastConditionState(subject)` to simply return `count() > 0`.
4. Update the iteration loops in `LawManager::tick()` to iterate the set instead of a map of pairs.

This will shrink the iteration domain from "all objects that were ever evaluated" (1599) to "only objects that currently satisfy the law" (often 0 or 1), making the loop functionally instant.

I am running the `frame_lag_test` against this change right now to verify the lag hits the ~2.4ms baseline.
