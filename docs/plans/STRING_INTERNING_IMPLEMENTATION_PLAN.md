# String Interning Implementation Plan

**Related Documents:**
* [Audit: Property Lookup Complexity Audit](../audits/PROPERTY_LOOKUP_COMPLEXITY_AUDIT_2026-09-01.md)
* [Analysis: Property Lookup Complexity Analysis](../analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md)

---

## The Goal
Replace the current pointer-chasing, string-allocating property lookup system with an $O(1)$-equivalent, cache-friendly integer comparison architecture using String Interning and a Structure of Arrays (SoA) layout. This ensures that `ActionNode` property writes do not bleed performance across hundreds of matched entities.

---

## 1. Core Data Structure: `StringId`
Introduce a central string interning registry to convert strings to lightweight integers.

**Files:** `src/Singularity/Core/StringId.hpp` / `.cpp`
* `struct StringId { uint32_t value; }`
* `class StringInterner`:
  * `static StringId intern(const std::string& str)`
  * `static const std::string& resolve(StringId id)`
* Backed by `std::unordered_map<std::string, uint32_t>` and `std::vector<std::string>`.

---

## 2. Property Interface Updates
Properties must cache their interned names at creation so they don't allocate strings.

**Files:** `src/ConstructedBeing/Singular/Property/Property.hpp`, `PropertyRef.hpp`, `ComputedProperty.hpp`
* Add `virtual StringId nameId() const = 0;`.
* `PropertyRef` and `ComputedProperty` constructors will call `StringInterner::intern(_name)` and store the result.

---

## 3. The Lookup Layer (Structure of Arrays)
Optimize the `Singular` property registry to eliminate cache misses and pointer dereferencing during the linear scan.

**Files:** `src/ConstructedBeing/Singular/Singular.hpp`, `Singular.cpp`
* **Registered Properties:** Replace `std::vector<std::unique_ptr<Property>> _propertyRegistry` with a two-vector SoA layout:
  ```cpp
  std::vector<StringId> _propertyNames;
  std::vector<std::unique_ptr<Property>> _propertyPointers;
  ```
  `findProperty(StringId)` scans `_propertyNames` (which perfectly utilizes CPU cache) and uses the found index to access `_propertyPointers`.
* **Dynamic Properties:** Change `_dynamicProperties` to `std::unordered_map<StringId, PropertyValue>` or a similar flat vector scan for authored properties.

---

## 4. PropertyPath and ActionNode Optimization
Remove all runtime string concatenation from Law evaluations, and ensure `ActionNode` passes integers instead of strings.

**Files:** `src/ConstructedBeing/Singular/Property/PropertyPath.hpp`, `PropertyPath.cpp`, `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp`
* `PropertyPath::parse` will pre-calculate and intern all joined sub-path combinations (e.g., `shape`, `shape.color`, `shape.color.r`) at parse time.
* `PropertyPath::resolve` will query `Singular` using only these pre-calculated `StringId` arrays.
* `ActionNode::compile` (in `ActionModel.cpp`) will capture the fully parsed/interned `PropertyPath`. The returned lambdas will pass these integer-backed paths to `lawSetValue()`, ensuring that when the lambda runs against 500 target objects, it is passing zero-allocation IDs.

---

## Verification Plan
1. **Automated Tests:** Verify `no_black_box_test`, `channel_paths_test`, and `object_roundtrip_test` pass to ensure property semantics and serialization hold.
2. **Performance:** Run `frame_lag_test` and `webgpu_micro_mastery_lag_test` to confirm identical or improved frame timing.
3. **Manual Validation:** Ensure authored laws involving deeply nested properties (`@shape.material.reflectivity`) resolve cleanly in the engine.

---

# Reply — Status Against the Implemented Pass

*Claude Opus 5, session `01Qq5ryb6Nz4GdqcTxtfv9JN`, 2026-09-02 01:15 PDT.*

**Occasion.** Zach asked me to read this plan and its companions, then told me that Claude
Sonnet 4.5 had already implemented it. This plan therefore reads as a proposal but describes
work that is largely done — `a8a38e02` ("Properties write with String Interning"), plus
uncommitted work in the tree. What follows is the plan marked up against reality, so the next
reader is not misled into implementing it a second time.

---

## Phase status

| § | Phase | Status |
|---|---|---|
| 1 | `StringId` / `StringInterner` | **Done.** `src/Singularity/Core/StringId.{hpp,cpp}`, as specified. |
| 2 | `Property::nameId()` | **Done.** Pure virtual at `Property.hpp:47`; `PropertyRef` and `ComputedProperty` intern at construction. |
| 3 | SoA registry | **Done.** `_propertyNames` / `_propertyRegistry` parallel arrays, `Singular.hpp:190-191`. `_dynamicProperties` is now `unordered_map<StringId, PropertyValue>` (`:195`). |
| 4 | `PropertyPath` / `ActionNode` | **Partly done, in flight, uncommitted.** See below. |
| — | Verification | **Not run.** See below. |

Four test files landed with the pass and are real coverage of the contract:
`string_id_test.cpp`, `property_string_id_test.cpp`, `singular_soa_lookup_test.cpp`,
`property_path_precalc_test.cpp`. Note they are picked up by the `tests/*.cpp` glob at
`CMakeLists.txt:299`, so **a reconfigure is required** before they appear in `ctest` — without
it they are silently absent, not failing.

---

## What Phase 4 actually became — and it is better than the plan

The plan's §4 says `ActionNode::compile` should "capture the fully parsed/interned
`PropertyPath`." The implementation found something the plan did not anticipate, and it is the
most valuable change in the entire pass.

`lawGetValue` / `lawSetValue` (`MathBinding.hpp`) were constructing a **`PropertyPath remainder`
per call** — a `std::vector<std::string>` suffix copy, so one heap allocation per remaining
segment plus the vector's own buffer, *per target, per law, per frame*, before any of the
lookup this plan optimizes had begun. For `@being.shape.color.r` that is four allocations paid
unconditionally, which is strictly more than the per-iteration `name()` copy the plan set out
to remove.

The in-flight fix replaces the suffix copy with a `std::size_t startIndex` threaded through
`resolveLawRoot`, `PropertyPath::resolve/getValue/setValue`. This is why the $O(S^2)$ sub-path
precalculation in §4 earns its cost: `_joinedIds[i][k]` is exactly the "join of segments
$i..i+k$" that resolving from an arbitrary offset requires. The two halves fit together
properly.

**This belongs in the plan as a named phase**, because it is the part a future reader would
otherwise assume was incidental. Credit where it is due: it came out of doing the work, not out
of planning it, and out of a scope line — `Singular`, `PropertyPath`, `ActionModel` — that
`MathBinding.hpp` sat just outside.

---

## Remaining work the plan does not list

1. **`notifyPropertyChanged` still takes `const std::string&`.** `Singular.cpp:297` resolves
   the `StringId` *back* to a string on every dynamic-property write, so authored-property
   writes still allocate on the hot path. The code comment there already admits it ("changing
   that signature is Phase 4 work"). It is not in this plan. It should be Phase 5.
2. **Sweep the six `printf` calls** in `PropertyPath.cpp` (`:137, :152, :240, :266, :272, :281`)
   before anything is measured or committed. `:137` is unconditional and its message is false —
   it fires on successful resolves. `:152` calls `found->value()`, a virtual call constructing a
   `PropertyValue`, purely to print a flag. Detail in the audit's reply.
3. **Interner thread-safety, or main-thread deferral of websocket commands.**
   `StringId.hpp:76-80` asserts single-threaded property access; `WebSocketServer::on_message`
   (bound `:922`, worker `:938`, no dispatch) calls `PropertyPath::parse` and `setValue` at
   `:320` off-thread. Interning turned a local race into a global one. Detail in the audit's
   reply.
4. **State the `parse`-is-cold-path invariant.** I checked all 87 `PropertyPath::parse` call
   sites; none is per-frame. That is now load-bearing — a `parse` inside a tick would be
   *worse* than the code this plan replaced, because it interns $O(S^2)$ sub-paths. Currently
   true by accident; should be true by construction, with a comment on `parse` saying so.
5. **Document that the interner never evicts.** Every sub-path of every path parsed in every
   world loaded this session is retained for the process lifetime. That is the right trade —
   it is what makes `resolve()`'s returned reference stable — but it is an undocumented choice,
   and it interacts with `clear()` (below).
6. **`StringInterner::clear()` is a live footgun.** It is correctly marked "for tests only," but
   it resets the id counter, so ids are *reused for different strings*. Any `Property` or
   `PropertyPath` constructed before the call keeps a cached id that now names something else.
   Safe in the standalone test binaries that use it; catastrophic if it is ever called with a
   world loaded. Worth a stronger guard than a comment.

---

## On the Verification Plan (§ Verification)

None of the three steps has been run.

- **Automated tests** — the build was in progress and error-free at the time of writing;
  `ctest` results are owed. Reconfigure first, per the glob note above.
- **Performance** — `frame_lag_test` cannot produce a meaningful number from the current tree
  while the `printf`s are in it. The cost of one unbuffered stdio call per resolve exceeds the
  entire lookup by roughly three orders of magnitude, which would swamp the very signal the
  test exists to read. Sweep first, then measure.
- **Manual validation** — not done.

One addition to §Verification: **`testSetValueZeroAllocation`
(`property_path_precalc_test.cpp:163`) does not measure allocations.** It asserts that
`setValue` returns `Ok` and that the value landed. The name promises a performance guarantee
that nothing in the body checks, so a regression reintroducing string allocation on the hot
path passes it green. Either give it teeth — count via an overridden `operator new` and assert
zero — or rename it. A test whose name lies is worse than a missing test, because it stops
anyone from writing the real one.

Per `docs/ENGINEERING_DISCIPLINE.md`: *don't claim a doc is verified because you read the
source; run things.* The architecture here is right and the implementation looks sound on
reading. Neither of those is verification.
