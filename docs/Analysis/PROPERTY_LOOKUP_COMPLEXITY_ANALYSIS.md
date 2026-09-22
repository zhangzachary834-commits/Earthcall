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

---

# Reply — The Cost Model Is Right About Shape and Wrong About Magnitude

*Claude Opus 5, session `01Qq5ryb6Nz4GdqcTxtfv9JN`, 2026-09-02 01:15 PDT.*

**Context.** Zach asked for a reading of this document alongside the audit, the plan,
and the two law-execution documents. He also told me — after I had read them — that
Claude Sonnet 4.5 had already implemented the plan these three describe. So this reply
reads the analysis twice: once as the argument it makes, and once against the code that
argument produced (`a8a38e02`, plus uncommitted work in the tree at the time of writing).

## 1. What §1 gets right, and why it matters more than the complexity classes

The genuine insight in this document is not $O(N)$. It is §1.1's observation that the
$O(N)$ was **self-inflicted**. A linear scan over twenty-one items should be nearly free;
this one was not, because each iteration dereferenced a `unique_ptr` into unrelated heap,
called a virtual `name()` that returned `std::string` **by value**, and then walked
characters. Three costs, none of which appear in the complexity class.

That is the correct level of analysis for this engine, and it is the reason the fix is
right. Stating it plainly: **the complexity class was never the problem, and the document
would be stronger if it led with that rather than with $O(N)$.** A reader who takes the
headline literally concludes "replace the vector with a hash map," which §2 of the audit
correctly declines to do.

## 2. The claim in §2.2 that should be withdrawn

> "a 64-byte CPU cache line holds 16 properties at once. The CPU performs pure integer
> comparisons with zero cache misses"
>
> and, in the audit's §3.2, "meaning the CPU scans through them in a single clock cycle."

One cache line is **one miss avoided**, not one comparison performed. Scanning sixteen
`StringId`s is sixteen compare-and-branch pairs scalar, or roughly four operations if the
compiler vectorizes — which, over a `std::vector` with a runtime bound and an early
return, it generally will not. Call it 5–20 cycles, which is what the tradeoffs document
independently estimates at its own §2 ("Property Lookup … ~5-15 cycles") without noticing
that the two figures contradict each other by an order of magnitude.

This matters beyond pedantry. The true win here is **21 heap dereferences and 21 string
allocations going to zero**. That is a large, defensible, *measurable* number. Replacing
it with "a single clock cycle" trades a real result for an unreal one and gives a skeptical
reader a reason to discount §1, which is sound.

## 3. The largest cost in the hot path is not in this document

§1.4 is the best section here — the multiplier effect is real and correctly identified.
But it names the wrong multiplicand. Until the uncommitted `startIndex` work, every law
read and every law write ran this, in `MathBinding.hpp`:

```cpp
PropertyPath remainder;                                   // a std::vector<std::string>
Singular* root = resolveLawRoot(subject, path, remainder);
remainder.segments.assign(path.segments.begin() + n, path.segments.end());
```

That `assign` **heap-allocates one `std::string` per remaining segment, plus the vector's
own buffer, on every call** — constructed and destroyed per target, per law, per frame.
For `@being.shape.color.r` that is four allocations before the lookup this document
analyzes has even begun. It is strictly larger than the per-iteration `name()` copy in
§1.1, because it is paid whether or not the scan is short.

An analysis of property-lookup cost that does not contain the word `remainder` has missed
the dominant term. The fix — carrying a `std::size_t startIndex` instead of copying a
suffix — is not in the implementation plan either; it was found during implementation.
Credit where due: that is the more valuable half of this work, and it came from doing the
work rather than from planning it.

## 4. $N$ is not bounded by the C++ vocabulary

§2.2 reasons about lookup cost on the assumption that $N$ is the registered vocabulary
(the audit puts it at "~20-30"). That assumption does not survive contact with
`Singular::findProperty`, which on a dynamic-property hit **lazily constructs a
`DynamicPropertyBridge` and pushes it into both parallel arrays**
(`Singular.cpp:229-233`). Every authored property a Person grants therefore lengthens the
very array whose shortness the argument depends on, permanently, for the being's lifetime.

This is not a defect — the bridge is what makes an authored property a real, enumerable
Property, and Refusal 6 requires exactly that. But it means $N$ grows with authorship, in
a system whose whole purpose is unbounded authorship. A being carrying two hundred
authored properties gets a two-hundred-entry linear scan. The scan is now cheap enough
that this is fine for a long time, and the honest statement is *"cheap enough for a long
time"* rather than *"$O(1)$-equivalent."* The threshold at which it stops being fine is a
real question this document should own, and the answer should be a measurement, not an
estimate.

## 5. The precalculation has a cost, and it is quadratic

§2.2 says sub-paths are "pre-interned at parse time" and treats that as free. It is
$O(S^2)$ in path length: `PropertyPath::parse` interns the join of `segments[i..j]` for
every pair, so a four-segment path interns ten strings, each hashed at $O(L)$.

Paid once per authored path, this is the correct trade and I would make it again — it is
what allows §3's `startIndex` resolution to work at all, since resolving from an arbitrary
offset needs the joins that start at that offset. But two consequences belong in the
analysis:

1. **The interner never evicts.** Every sub-path of every path ever parsed, in every
   world ever loaded this session, is retained for the process lifetime. Loading many
   worlds in one session grows the table monotonically. That is a deliberate and defensible
   choice — it is what makes `resolve()` return a stable reference — but it is a choice,
   and it is currently undocumented as one.
2. **`parse` must never reach the hot path.** I checked all 87 call sites; none is
   per-frame (`ConditionModel.cpp:294` is inside a `compile()`, not a tick). That is
   currently true by accident rather than by construction. It is now a load-bearing
   invariant and should be stated as one, because a future `parse` inside a tick would be
   *worse* than the code this document replaced.

## 6. The missing section: measurement

This document argues a performance case across six sections and contains no number
measured from Earthcall. `frame_lag_test` exists, is machine-normalized, and has a
committed baseline. Nothing in this pass ran it.

Per `docs/ENGINEERING_DISCIPLINE.md` — *don't claim a doc is verified because you read the
source; run things* — the analysis is incomplete without a before/after. I could not
supply one at the time of writing for a specific reason, recorded in the audit's reply:
six `printf` calls are sitting in `PropertyPath::resolve` and `setValue` in the working
tree, one of them unconditional, which makes the current tree unmeasurable by roughly
three orders of magnitude. That is transient debugging residue, not a defect in the
argument — but until it is swept, no number taken from this tree means anything.

## 7. Summary judgment

| §  | Verdict |
|---|---|
| 1.1 | Correct, and the strongest part. Lead with the constants, not the class. |
| 1.2 | Correct. |
| 1.3 | Correct diagnosis of `joined += '.' + …`; the fix landed. |
| 1.4 | Right effect, wrong dominant term — see §3 above. |
| 2.1 | Correct. |
| 2.2 | "Single cache line, single cycle" should be withdrawn; $N$ is unbounded (§4); precalc is $O(S^2)$ (§5). |

The architecture chosen is the right one. What this document needs is not a different
conclusion but honest magnitudes and one measured number.
