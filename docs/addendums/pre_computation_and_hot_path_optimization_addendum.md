# Integrating Pre-Computation and Hot Path Optimization

In Earthcall, the "Hot Tick"—the continuous cycle of evaluating rules, routing events, and processing spatial queries—demands rigorous performance discipline. The engine avoids redundant O(N) scans, tree traversals, and dynamic memory allocations by shifting work out of the evaluation loop and into a discrete pre-computation (or recompilation) phase.

This integration of up-front structural analysis and hot-path execution is visible across the Law evaluation pipeline and the Prophetic Rete.

## Law Compilation (`Law::recompile`)

When a Law is authored or structurally modified, it must evaluate whether its conditions apply to candidates and execute its actions. Naively parsing properties or analyzing action trees on every target would destroy performance.

Instead, `Law::recompile()` (`src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`) prepares the law for execution:

```cpp
void Law::recompile() {
    // ...
    std::vector<PropertyPath> writes;
    _actionModel->collectPaths(writes);
    for (const PropertyPath& w : writes) {
        if (!w.segments.empty() && !w.segments.front().empty() &&
            w.segments.front() == "motion") {
            _writesMotionProperties = true;
        }
        // ... evaluates _writesZoneCanExit and _writesZoneOwner
    }
}
```

By pre-calculating these boolean flags, `Law::applyToImpl()` can make O(1) gating decisions—such as checking if a movement action requires consent or if zone boundaries are crossed—without traversing the `_actionModel` or allocating property path vectors during the Hot Tick.

## Rete Normalization (`Prophetic::Index::rebuild`)

Similarly, the Prophetic Rete must determine whether fact writes in one rule can trigger the reads of another. Doing this string parsing dynamically would be disastrous.

In `Prophetic::Index::rebuild` (`src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp`), the normalization of paths is performed once during index construction:

```cpp
std::vector<std::vector<std::vector<std::string>>> writeNorms(_facts.size());
// ...
writeNorms[i].push_back(normalizedPaths(write.path));
```

By caching the `writeNorms` and `readNorms`, the subsequent pairwise relevance loop (`pathsMayAlias`) operates on prepared, structured data. The hot path avoids redundant `PropertyPath` string parsing entirely.

## Identifier Caching in LawManager

At a broader level, `LawManager` optimizes hot routines (such as the agenda loop in `tick()`, `sweepSubjects()`, and `restartDriveSession()`) by locally caching the result of `law.getIdentifier()`. Because `getIdentifier()` involves a virtual call and constructs a `std::string` on the heap (by value), calling it repeatedly within a tight loop would incur significant overhead. Local caching avoids these redundant allocations, maintaining the strict performance requirements of the engine's core cycle.

Through these pre-computations, Earthcall honors its commitment to performance as truth, ensuring that the engine remains a swift and scalable vessel for the authored ontology.
