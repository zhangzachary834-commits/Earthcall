# Property Lookup and Write Complexity Audit

**Date:** 2026-09-01  
**Auditor:** Antigravity (Gemini 3.1 Pro)  
**Scope:** `src/ConstructedBeing/Singular/Singular.hpp`, `Singular.cpp`, `PropertyPath.cpp`  

**Related Documents:**
* [Analysis: Property Lookup Complexity Analysis](../analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md)
* [Implementation Plan: String Interning](../plans/STRING_INTERNING_IMPLEMENTATION_PLAN.md)

---

## Executive Summary

The current architecture guarantees **$O(N)$ lookup/write time for C++ registered properties** and **$O(\log D)$ lookup/write time for authored dynamic properties**. 

While $O(N)$ via a `std::vector` linear scan is generally optimal for small datasets due to CPU cache locality, Earthcall's `_propertyRegistry` currently penalizes its own cache by storing `std::unique_ptr<Property>`. This requires a pointer dereference for every string comparison during iteration. 

If Rete Law evaluation begins struggling to maintain the 200FPS target, moving to a **String Interning (StringId)** architecture is the recommended frontier approach, rather than simply dropping in a `std::unordered_map`.

---

## 1. Current Complexity Breakdown

### 1.1 Registered Properties (C++ Defined)
* **Storage:** `std::vector<std::unique_ptr<Property>> _propertyRegistry`
* **Lookup:** $O(N)$ — `Singular::findProperty` iterates through the vector and performs a `std::string` comparison (`property->name() == name`).
* **Write:** $O(N)$ — The operation is bound by the time it takes to find the property. The actual assignment (e.g., `PropertyRef::setValue`) is $O(1)$.

### 1.2 Dynamic Properties (Author Defined)
* **Storage:** `std::map<std::string, PropertyValue> _dynamicProperties`
* **Lookup:** $O(\log D)$ — Binary search tree traversal.
* **Write:** $O(\log D)$ — Finding the node and updating the map.

### 1.3 Property Path Resolution
* **Path Parsing:** `PropertyPath::resolve` evaluates dotted addresses like `@shape.color.r`.
* **Complexity:** $O(S \times N)$ or $O(S \times \log D)$ where $S$ is the number of segments in the path.
* **Hot-Path Cost:** Crucially, `resolve()` splits and re-joins segments at runtime (`joined += '.' + segments[j]`) to check for longest-match registrations. This involves dynamic string allocations inside the hot loop.

---

## 2. The Hash Map Threshold

When should Earthcall abandon $O(N)$ linear scans for an $O(1)$ `std::unordered_map`?

1. **The Vector Limit (~20-30 Properties):** Contiguous memory iteration typically beats hashing for under 20 items. However, because `findProperty` dereferences a `unique_ptr` per iteration, the CPU cache is broken. If beings like `CreationChannel` (21 properties) or deeply composed Objects consistently exceed 20 properties, the cache misses will outweigh the vector's benefits.
2. **Authored Heavyweights:** If Persons begin authoring complex entities with dozens of dynamic properties, the $O(\log D)$ node-allocating traversal of `std::map` will become a bottleneck. An $O(1)$ `unordered_map` is fundamentally better for authored state (unless the UI picker specifically requires alphabetical iteration).
3. **The Rete Hot-Path:** `WhileTrue` and `OnBecomeTrue` laws evaluate paths continuously. If profiling shows that `PropertyPath::resolve` (specifically its string concatenation and pointer-chasing linear scan) takes a measurable percentage of the frame budget, a structural change is required.

---

## 3. The Frontier Recommendation: String Interning

High-performance engines do not usually fix this by swapping to `std::unordered_map` (which still suffers from hashing overhead and cache fragmentation). They use **String Interning**.

By mapping every `std::string` to a lightweight `uint32_t StringId` at load/parse time, Earthcall can achieve $O(1)$-equivalent lookups while perfectly utilizing the CPU cache.

### The Shape of the Fix:
1. **The Registry Structure:** 
   Change `_propertyRegistry` to hold the ID alongside the pointer:
   ```cpp
   struct RegisteredProperty {
       StringId id;
       std::unique_ptr<Property> prop;
   };
   std::vector<RegisteredProperty> _propertyRegistry;
   ```
2. **The Lookup:**
   `findProperty(StringId id)` performs a linear scan over the vector comparing *only* the 4-byte integers. It completely avoids dereferencing the `unique_ptr` until a match is found. This costs zero cache misses and is significantly faster than querying a hash map.
3. **The Path Resolution:**
   `PropertyPath::parse` pre-calculates and interns the combined dotted strings (`shape`, `shape.color`, `shape.color.r`) *once*. The hot-loop `PropertyPath::resolve` then traverses the path using only `StringId` comparisons, completely eliminating dynamic string allocations during frame ticks.

---

*Written in accordance with Refusal 6 (No Black Box) and the Engine's performance mandates.*
