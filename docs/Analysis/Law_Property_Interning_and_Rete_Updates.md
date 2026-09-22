# Deep Dive: String Interning and Dynamic Rete Updates

**Date:** 2026-09-10
**Author:** Antigravity (Agent)

This document captures the architectural mechanics of how Earthcall executes property lookups under the hood, and how the Rete network stays perfectly synchronized when laws are authored or edited at runtime. 

## 1. Do property lookups use string interning, or something else?

Yes. Earthcall uses string interning via `Earthcall::StringInterner` and `Earthcall::StringId`. This is a critical piece of the engine's performance architecture.

### The Hot Path (Zero Hash Maps, Pure Cache Hits)
When the engine is actually ticking every frame, it doesn't evaluate properties using strings at all. When a Law is loaded or authored, its `PropertyPath` (e.g., `"shape.color.r"`) is pre-parsed. The strings are permanently mapped into 4-byte integers (`StringId`) using the global intern pool.

From that point on, whenever the law evaluates that property on 500 objects, it calls `Singular::findProperty(StringId)`. 

Inside `Singular`, the properties are NOT stored in a hash map. They are stored as two parallel contiguous arrays:
* `std::vector<Earthcall::StringId> _propertyNames`
* `std::vector<std::unique_ptr<Property>> _propertyRegistry`

Because `_propertyNames` is a contiguous array of 4-byte integers, a 64-byte L1 cache line fits 16 properties at once. Finding `"position"` is a pure integer array scan that almost always completes without leaving the CPU cache. It performs zero string allocations, zero string comparisons, and zero hash misses.

### The Cold Path (`beingCarriesProperty`)
Fallback helper functions like `beingCarriesProperty` (used by the engine to build vocabulary indexes) receive raw `std::string` arguments like `"position.y"`. 

These functions extract the base name (`"position"`) and hand it to `Singular::findProperty(const std::string& name)`. This cold path:
1. Hits the `std::unordered_map` inside `StringInterner::intern` **once** to turn the string into its 4-byte integer `StringId`. (This incurs a cache miss).
2. Drops directly back into the contiguous array scan mentioned above.

This hash-miss is acceptable because the cold path is no longer used in the per-frame tick loop. It is only called by `LawManager::seedStateFacts` when the engine is booting up, or when a brand new law is authored and the Rete network must build its vocabulary index. 

## 2. What happens to the Rete network when an existing Law's property paths are edited?

When a Law is already created and registered, but a Person authors an edit that changes its property paths (e.g., from `"position.y"` to `"shape.color.r"`), the engine's Rete network **recompiles and back-fills** in real-time.

Here is the exact sequence of events that happens on the very next `LawManager::tick` after the Law is edited:

### Step A: Revision Syncing
When a Person authors an edit to the Law's text, `Law::bumpTextRevision()` increments the law's version number. At the top of `LawManager::tick`, the engine runs `syncReteCompilation` which checks the revision ID. Seeing that it's changed, it kicks off a recompilation.

### Step B: Dropping the Old Network
`LawManager::compileConditionsToRete` immediately unbinds the law from its old Rete terminal nodes. If no other laws are using those old nodes, they fall out of use and are eventually garbage collected.

### Step C: Interning the New Alpha Nodes
The new condition AST is compiled. When the new property path is encountered, it is parsed and interned into `StringId` values, and the engine calls `ReteNetwork::internAuthoredAlpha` to create a new `AlphaNode` for the condition. 

### Step D: The "Backfill" (Seeding existing objects)
Because this is a brand new `AlphaNode`, its memory is empty. How does it learn about objects that *already exist* and have that property?

Inside `bindLawToAlpha`, the engine checks if the node is brand new (`!alphaIsRead`). If it is, it calls `ReteNetwork::refillAlphaMemory`. This function loops over the master `_facts` registry (which contains a `property-state` fact for every property on every existing object). 

If a fact's property name matches the new path's base name (e.g., `"shape"`), it executes the Law's new condition against the object's **live** memory state. If the object satisfies the new property condition, that object's fact is pushed into the AlphaNode's memory, and the Law is immediately queued on the Agenda.

**Result:** Changing a Law's property paths dynamically updates the Rete network in real-time. The new property paths are interned on the spot, the law is dynamically rebound, and any existing objects that match the new condition will immediately fire on the exact same tick.
