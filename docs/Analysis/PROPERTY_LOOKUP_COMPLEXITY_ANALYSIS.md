# Property Lookup Complexity Analysis

**Related Documents:**
* [Audit: Property Lookup Complexity Audit](../audits/PROPERTY_LOOKUP_COMPLEXITY_AUDIT_2026-09-01.md)
* [Implementation Plan: String Interning](../plans/STRING_INTERNING_IMPLEMENTATION_PLAN.md)

---

## 1. Current Architecture Complexity

Earthcall currently splits properties into C++ defined (Registered) and Author defined (Dynamic).

### 1.1 Registered Properties (C++ Defined)
* **Storage:** `std::vector<std::unique_ptr<Property>> _propertyRegistry`
* **Lookup (Time Complexity):** **$O(N)$** 
  `Singular::findProperty` iterates through the vector. 
* **The Hidden Constants (Cache & Allocations):** 
  While $O(N)$ over contiguous memory is theoretically fast, the current implementation incurs massive hidden costs:
  1. **Cache Misses:** Iterating the vector dereferences a `unique_ptr` per item, causing a random heap access (cache miss) on every iteration.
  2. **String Allocations:** `Property::name()` returns a `std::string` by value, meaning every iteration allocates/copies a string.
  3. **String Comparisons:** Each match check performs a character-by-character comparison ($O(L)$ where $L$ is string length).

### 1.2 Dynamic Properties (Author Defined)
* **Storage:** `std::map<std::string, PropertyValue> _dynamicProperties`
* **Lookup (Time Complexity):** **$O(\log D)$**
  Binary tree traversal via `std::map`.
* **The Hidden Constants:** Every lookup allocates tree nodes and traverses heap memory.

### 1.3 Property Path Resolution (e.g., `shape.color.r`)
* **Time Complexity:** **$O(S \times N)$** or **$O(S \times \log D)$** (where $S$ is path segments).
* **The Hidden Constants:** `PropertyPath::resolve` concatenates strings inside the hot loop to check for longest-match properties, meaning multiple string allocations per property check.

### 1.4 The Multiplier Effect: ActionNode Execution
* **The Scope:** Laws do not apply to a single object. An `ActionNode` is compiled once into a target-agnostic lambda and then applied sequentially to *every* object matched by the Rete network in a given frame.
* **The Penalty:** Because `Property` memory locations differ per instance, the lambda cannot cache property pointers. It must pass its `std::string` path into the target object and trigger the full $O(N)$ lookup described above.
* **The Total Cost:** $(O(N) + O(L)) \times \text{Matched Objects}$. If a Law matches 1,000 objects, the engine executes 1,000 separate string allocations and cache-missing pointer traversals per frame just for one Action.

---

## 2. Proposed Architecture Complexity (String Interning / Structure of Arrays)

By adopting a String Interning system and a Structure of Arrays (SoA) layout (e.g., splitting Names from Pointers into two vectors), the complexities shift dramatically toward upfront costs and away from the hot path.

### 2.1 String Interning (The Caching Step)
* **Interning a String (Cache Miss/Insert):** **$O(L)$**
  Hashing a string requires reading every character ($O(L)$) and a hash map lookup ($O(1)$). This is paid exactly once at load/parse time (e.g., when an `ActionNode` compiles).
* **Comparing Strings:** **$O(1)$**
  Once interned into a 4-byte `StringId`, comparing two strings is a single integer comparison instruction.
* **Retrieving Original String:** **$O(1)$**
  The interner maps `StringId` to an array index holding the original string.

### 2.2 Hot Path Lookup (The Rete Tick)
* **Lookup:** **$O(N)$**, but with a fraction of the constant time.
  By iterating over a contiguous `std::vector<StringId>` (or array of ids), a 64-byte CPU cache line holds 16 properties at once. The CPU performs pure integer comparisons with zero cache misses and zero string allocations.
* **Property Path Resolution:** **$O(S \times N)$**
  Because sub-paths (e.g. `shape`, `shape.color`) are pre-interned at parse time, the hot loop simply compares `StringId` combinations with zero allocations.
* **ActionNode Execution:** 
  The multiplier effect is neutralized. The Action applies its pre-interned `StringId` against 1,000 objects. Each object does a blisteringly fast L1-cached integer scan. The total cost becomes $O(N_{int}) \times \text{Matched Objects}$ with zero string overhead.
