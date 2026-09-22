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

---

# Counter-Audit Reply

**Date:** 2026-09-02 01:15 PDT
**Auditor:** Claude Opus 5, session `01Qq5ryb6Nz4GdqcTxtfv9JN`
**Scope:** this audit's claims, checked against `a8a38e02` and the working tree at time of writing.
**Occasion:** Zach asked for a reading of this audit and its companions, and told me that Claude
Sonnet 4.5 had already implemented the plan derived from them. So this is an audit of an audit
that has already been acted on — the findings below are about what is *now true in the code*.

---

## Verdict on the original audit

**Sustained.** The diagnosis was correct and the recommendation was the right one. String
interning plus a Structure-of-Arrays registry is what a high-performance engine does here, and
declining to reach for `std::unordered_map` (§2, §3) was the right call for the reason given —
hashing overhead and cache fragmentation would have bought less. The implementation exists and
is sound. What follows are corrections and additions, not a reversal.

---

## Finding 1 — CRITICAL (transient): six `printf` calls in the hot path

At the time of writing, the working tree carries six unstaged `printf` calls inside the exact
function this audit was written to make fast:

| Line | Call |
|---|---|
| `PropertyPath.cpp:137` | `"resolve failed: no property found for path! runLength loop exhausted."` |
| `PropertyPath.cpp:152` | `"i=%zu segments.size()=%zu trailingComponent=%p isVec3=%d"` |
| `PropertyPath.cpp:240` | `"setValue failed: NoSuchProperty, …"` |
| `PropertyPath.cpp:266` | `"setValue failed: TypeMismatch in toNumber"` |
| `PropertyPath.cpp:272` | `"setValue failed: BadComponent, …"` |
| `PropertyPath.cpp:281` | `"setValue failed: ReadOnly (setValue returned false)"` |

None is in `HEAD` (`git show HEAD:…` is clean); all six are in the unstaged diff. They are
debugging residue from the in-flight `startIndex` refactor, not shipped code.

Two are worse than leftovers:

- **`:137` is unconditional and its message is false.** It sits after the longest-match loop
  but *before* `if (!found)`, so it fires on every **successful** resolve, announcing failure.
  One unbuffered stdio call per property resolution, per target, per law, per frame — at
  microseconds each, against a lookup this audit reduced to nanoseconds. As the tree stands,
  the optimization is inverted by about three orders of magnitude.
- **`:152` calls `found->value()`** — a virtual call constructing and returning a
  `PropertyValue` by value — purely to print an `isVec3` flag. Precisely the category of
  hidden per-iteration cost §1.1 of the analysis exists to eliminate.

(`:240` also contains a literal `\\n` and prints a backslash-n. Cosmetic.)

**Consequence for verification:** no number taken from this tree means anything until these are
swept. That is why the "Performance" step of the implementation plan's verification section
cannot be signed off yet.

## Finding 2 — the thread-safety note in `StringId.hpp` is factually wrong

`src/Singularity/Core/StringId.hpp:76-80` states:

> "Earthcall's Law system is currently single-threaded (all property lookups happen on the main
> thread during `LawManager::tick`)."

They do not. `WebSocketServer::Impl::on_message` is bound at `WebSocketServer.cpp:922`, runs on
the websocketpp io worker started at `:938`, and there is no dispatch back to the main thread —
no queue, no deferred-command pump. At `:320` that handler calls `PropertyPath::parse` and then
`path.setValue` directly against live beings pulled from `Universe::instance().beings()`.

This was already a data race on being state before this pass, and that race is not this audit's
fault. But interning **enlarged its blast radius from local to global**. `StringInterner::intern`
mutates a process-wide `std::unordered_map` and `std::vector`:

- a rehash concurrent with an insert is undefined behavior, and
- `resolve()` returns `const std::string&` into a `std::vector` that can reallocate underneath
  the caller.

Previously a race corrupted one object's property. Now it can corrupt the name table every
being in the world shares.

**No test in `tests/` uses `std::thread`** — I checked the whole directory — so nothing guards
this.

Two fixes, in preference order:
1. Defer websocket commands onto the main thread. This is the correct fix for the pre-existing
   race as well, and it costs the interner nothing.
2. Failing that, make the interner thread-safe. Since strings are never evicted, a `std::deque`
   plus a `std::shared_mutex` gives stable references cheaply — a `vector` cannot, because
   reallocation invalidates the reference `resolve()` already handed out.

Either way, the comment must be corrected. A comment asserting an invariant the code does not
hold is worse than no comment: it is the thing the next reader trusts instead of checking.

## Finding 3 — §2's "~20-30 properties" threshold does not hold

§2.1 reasons from a vector limit of twenty to thirty properties. `Singular::findProperty`
(`Singular.cpp:229-233`) lazily constructs a `DynamicPropertyBridge` on a dynamic-property hit
and **pushes it into `_propertyNames` and `_propertyRegistry`**. Every property a Person
authors therefore permanently lengthens the array whose shortness the threshold assumes.

The bridge is correct and required — Refusal 6 wants authored properties enumerable — but it
means the registered array grows with authorship, in an engine built for unbounded authorship.
The threshold in §2 should be restated as a function of authored state, not as a constant, and
the point at which the linear scan stops paying should be *measured* rather than assumed.

Also worth stating: `listProperties()` now returns bridges alongside the C++ vocabulary, and
does so only for dynamic properties that have already been looked up once. Enumeration is
therefore history-dependent. I believe that is harmless today; it is the kind of thing that
stops being harmless when an authoring UI starts trusting the list.

## Finding 4 — the magnitude claim in §3.2 should be withdrawn

> "A 64-byte cache line holds 16 `StringId`s, meaning the CPU scans through them in a single
> clock cycle with zero cache misses."

One cache line is one miss avoided, not one comparison performed. Sixteen `StringId`s is
sixteen compare-and-branch pairs scalar; a few operations if vectorized, which over a
runtime-bounded loop with an early return it generally will not be. The companion tradeoffs
document independently estimates the same operation at 5–15 cycles, so the doc set contradicts
itself by an order of magnitude.

The accurate claim is stronger than the inaccurate one: **21 pointer dereferences and 21 string
allocations per lookup, reduced to zero.**

## Finding 5 — the audit missed the largest allocation in the path

The audit's §1.4 correctly identifies the multiplier but attributes it to the wrong term. Until
the uncommitted `startIndex` work, `lawGetValue` and `lawSetValue` in `MathBinding.hpp`
constructed a `PropertyPath remainder` per call — a `std::vector<std::string>` suffix copy, so
one heap allocation per remaining segment plus the vector buffer, *per target, per law, per
frame*, before the scan this audit measured even began.

That was strictly larger than the per-iteration `name()` copy, because it was paid regardless
of scan length. It is not named anywhere in this audit or in the implementation plan. It was
found during implementation, and replacing it with a `std::size_t` offset is the more valuable
half of the whole pass.

The lesson for future audits of this kind: an audit scoped to
`Singular.{hpp,cpp}`/`PropertyPath.cpp`/`ActionModel.cpp` will systematically miss costs paid
in the *caller*. `MathBinding.hpp` was one file outside the scope line and held the dominant
term.

## Finding 6 — document hygiene

- The cross-links in this file and its companions use `../analysis/…`; git tracks the directory
  as `docs/Analysis`. Resolves on this case-insensitive Mac, 404s on GitHub and on Linux.
- This audit carries an auditor and a date but no session ID or timestamp; the two Analysis
  documents and `LAW_EXECUTION_FRONTIER.md` carry no signature at all. `CLAUDE.md` asks for
  name, session ID, date, and timestamp.
- None of this work appears in `docs/Agenda/Tasks/To-do list.md` — no entry for string
  interning, the bytecode VM, or the Prophetic JIT. `CLAUDE.md` requires that anything worked
  on be listed.

---

## What is verified and what is not

**Verified by reading source:** every file:line citation above.
**Verified by running:** nothing yet. A full `cmake --build build -j8` was in progress and
error-free at the time of writing; `ctest` results and a `frame_lag_test` before/after are
owed, and are blocked on Finding 1.

*Written in the same spirit as the original: Refusal 6 says state that is not legible cannot be
governed. A performance claim that is not measured is the same failure one level up.*
