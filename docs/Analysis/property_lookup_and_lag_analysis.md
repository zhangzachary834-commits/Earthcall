# Property Lookups and Lag Analysis

This document addresses several architectural questions about property lookups, string interning, data structures, and the root cause of the "eval + sweep" lag in complex worlds like `synthesis_studio_living`.

## 1. Property Lookups vs String Interning
> *"aren't property lookups supposed to use string interning or is that a diffrent thing"*

They do! Our previous optimization to `PropertyPath::resolve` wired up standard property lookups (e.g., `subject.shape.kind` or `target.color`) to use `Earthcall::StringId` (string interning). This eliminated all string allocations and turned those lookups into fast integer comparisons.

However, the lag you experienced was caused by a **different** type of lookup: **Qualified Roots** (e.g., `@state.studio.mode`). 

While regular properties are looked up locally on the `subject`, qualified roots are global absolute paths that address a completely different object in the universe. The `MathBinding.hpp` layer resolves these by calling `Universe::instance().beings()` and searching through them. This did *not* use string interning; it used raw `std::string` comparisons and string concatenation (`candidate += "." + ...`).

## 2. Hash Maps vs Contiguous Arrays
> *"does this use hash maps that have cache miss or does it use contiguous array of structures"*

Prior to our fix, the global object lookup for qualified roots used neither optimally. It iterated over a `std::vector<Singular*>` (a contiguous array of pointers). Because it's an array of *pointers*, every iteration required dereferencing the pointer to fetch the object's `getIdentifier()` string, causing massive CPU cache misses (pointer chasing), followed by slow string comparisons. 

Our fix introduces a cached `std::map` (Hash Map / Tree) bound to the `Universe::structuralRevision()`. 
- **Why this is faster:** Instead of looping through all 1592 objects and doing 3 string comparisons for each (4776 comparisons total per lookup), the map finds the exact object in `O(log N)` or `O(1)` time using a single key lookup.
- **Cache Misses:** While hash maps can have cache misses on the nodes, a single `O(1)` lookup with a cache miss is infinitely faster than an `O(N)` loop of 1592 cache misses.

## 3. Dynamic Property Path Changes
> *"you named seed state facts and brand new laws. What about for Laws that are already created and registered but had changed property paths after that?"*

When a property is changed via `applyTo` or direct assignment, `PropertyPath::setValue` calls `Singular::notifyPropertyChanged`. This publishes an event to the `Core::EventBus`.

The `LawManager` listens to these events via `LawManager::onPropertyChanged`. When an object's property changes, the `LawManager` dynamically retracts the object's old facts from the Rete network and asserts the new facts. This allows already-registered laws to instantly react to property changes without needing to be re-compiled or re-registered.

## 4. The Anatomy of the `synthesis_studio_living` Lag

**The Symptom:** `eval + sweep` taking ~229 ms per frame in headless and ~6ms in UI.
**The Cause:** 19 `WhileTrue` laws in `synthesis_studio_living` have a mixture of bare conditions (e.g., `isStudioSurface`) and qualified roots (e.g., `@state.studio.themeNight`).

Here is exactly what the engine was doing:
1. The Rete network efficiently finds all 1592 objects matching the bare conditions.
2. `LawManager::tick()` loops through all 1592 objects to verify the full condition (`conditionsSatisfied`).
3. For **each** of the 1592 objects, it evaluates the qualified root (`@state.studio.mode == 5`).
4. To evaluate the qualified root, `resolveLawRoot` allocates a new `std::vector` of all 1592 beings, and loops through them doing string concatenation and comparisons.
5. **Math:** 19 laws * 1592 objects * 1592 beings * 3 path segments = **~144 million string comparisons per frame.**

**The Fix:** By caching the global identifier lookup in `resolveLawRoot` using `Universe::structuralRevision()`, we skip the `std::vector` allocation and reduce the 1592 string comparisons to a single map lookup. This immediately resolves the O(N^2) bottleneck.
