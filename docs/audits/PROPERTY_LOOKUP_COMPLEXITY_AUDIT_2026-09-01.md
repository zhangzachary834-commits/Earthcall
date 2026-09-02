# Property Lookup and Write Complexity Audit

**Date:** 2026-09-01  
**Auditor:** Antigravity (Gemini 3.1 Pro)  
**Scope:** `src/ConstructedBeing/Singular/Singular.hpp`, `Singular.cpp`, `PropertyPath.cpp`, `ActionModel.cpp`

**Related Documents:**
* [Analysis: Property Lookup Complexity Analysis](../analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md)
* [Implementation Plan: String Interning](../plans/STRING_INTERNING_IMPLEMENTATION_PLAN.md)

---

## Executive Summary

The current architecture guarantees **$O(N)$ lookup/write time for C++ registered properties** and **$O(\log D)$ lookup/write time for authored dynamic properties**. 

While $O(N)$ via a `std::vector` linear scan is generally optimal for small datasets due to CPU cache locality, Earthcall's `_propertyRegistry` penalizes its own cache by storing `std::unique_ptr<Property>`. This requires a heap jump (cache miss) and a string allocation for every comparison during iteration. 

**The Multiplier Effect:** This baseline penalty becomes catastrophic during Law evaluation. Because an `ActionNode` does not know which object it is writing to until it executes, it *cannot cache property pointers*. It must perform the full string-matching, pointer-chasing linear scan against every target object, every single frame.

Moving to a **String Interning (StringId)** / Structure of Arrays architecture is the recommended frontier approach to eliminate this exact bottleneck.

---

## 1. Current Complexity Breakdown

### 1.1 Registered Properties (C++ Defined)
* **Storage:** `std::vector<std::unique_ptr<Property>> _propertyRegistry`
* **Lookup:** $O(N)$ — `Singular::findProperty` iterates through the vector.
* **Write:** $O(N)$ — The operation is bound by the time it takes to find the property. The actual assignment is $O(1)$.

### 1.2 Dynamic Properties (Author Defined)
* **Storage:** `std::map<std::string, PropertyValue> _dynamicProperties`
* **Lookup:** $O(\log D)$ — Binary search tree traversal.
* **Write:** $O(\log D)$ — Finding the node and updating the map.

### 1.3 Property Path Resolution
* **Path Parsing:** `PropertyPath::resolve` evaluates dotted addresses like `@shape.color.r`.
* **Complexity:** $O(S \times N)$ or $O(S \times \log D)$ where $S$ is the number of segments in the path.
* **Hot-Path Cost:** `resolve()` splits and re-joins segments at runtime (`joined += '.' + segments[j]`), incurring dynamic string allocations inside the hot loop.

### 1.4 The Multiplier Effect (ActionNode Execution)
* **The Design:** An `ActionNode` (like a `Set` or `Add` operation) compiles into a lambda that is agnostic to the target object. It takes the `target` as a parameter when executing.
* **The Penalty:** Because the Rete network might apply the same action to 500 different target objects in a single frame, and each object possesses its own unique `Property` memory locations, **the Action cannot cache pointers**. It is forced to pass the `std::string` and trigger the full $O(N)$ lookup (with all its cache misses, string copies, and character comparisons) 500 times per frame.

---

## 2. The Hash Map Threshold

When should Earthcall abandon $O(N)$ linear scans for an $O(1)$ `std::unordered_map`?

1. **The Vector Limit (~20-30 Properties):** Contiguous memory iteration typically beats hashing for under 20 items. However, because `findProperty` dereferences a `unique_ptr` per iteration, the CPU cache is broken. If beings consistently exceed 20 properties, the cache misses will outweigh the vector's benefits.
2. **Authored Heavyweights:** An $O(1)$ `unordered_map` is fundamentally better for large dynamic state than the current $O(\log D)$ `std::map`.
3. **The Rete Hot-Path:** As detailed in 1.4, laws evaluate paths continuously. When the action execution multiplier hits, a structural change is required to maintain 200FPS.

---

## 3. The Frontier Recommendation: String Interning

High-performance engines do not usually fix this by swapping to `std::unordered_map` (which still suffers from hashing overhead and cache fragmentation). They use **String Interning and Structure of Arrays (SoA)**.

By mapping every `std::string` to a lightweight `uint32_t StringId` at load/parse time, Earthcall can achieve $O(1)$-equivalent lookups while perfectly utilizing the CPU cache.

### The Shape of the Fix:
1. **The Registry Structure (Structure of Arrays):** 
   Change `_propertyRegistry` to separate the names from the heavy pointers:
   ```cpp
   std::vector<StringId> _propertyNames;
   std::vector<std::unique_ptr<Property>> _propertyPointers;
   ```
2. **The Lookup:**
   `findProperty(StringId id)` performs a linear scan over the `_propertyNames` vector. A 64-byte cache line holds 16 `StringId`s, meaning the CPU scans through them in a single clock cycle with zero cache misses. Only when the correct index is found does it dereference `_propertyPointers`.
3. **ActionNode Synergy:**
   The `ActionNode` compiles the string into a `StringId` exactly once. When it runs against 500 targets, it passes the integer ID instead of a string. The lookup penalty vanishes.

---

*Written in accordance with Refusal 6 (No Black Box) and the Engine's performance mandates.*
