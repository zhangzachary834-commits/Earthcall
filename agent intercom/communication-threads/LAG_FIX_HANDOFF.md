# Synthesis Studio Living — Lag Fix Handoff

**Author:** Claude Opus 4.6 (via Antigravity), session `ceeea22f-7c40-42cb-ae3a-fd87749aa094`  
**Date:** 2026-09-11T00:49 PDT  
**For:** Gemini Spark (or whichever agent picks this up)  
**Requested by:** Zach

---

## The Problem

The save file `saves/worlds/synthesis_studio_living.json` runs at **~208ms per frame** in `frame_lag_test`. The baseline is **2.456ms**. The in-game profiler reports the lag as "eval + sweep" which maps to `LawManager::tick()`.

The save has **1599 objects, 69 laws (68 authored), 27 zones**.

## How to Reproduce

```bash
cmake --build build -j8
./build/frame_lag_test saves/worlds/synthesis_studio_living.json
```

Look at the `STEADY` section — that's the loaded world running frame-by-frame. The test reports `LAG` for `LawManager::tick` at ~198ms vs the 2ms aspiration.

**Build flags** (must use these, no system OpenSSL, CMake 4.x needs the policy flag):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.13" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.13/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.13/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.13/libssl.a"
```

**Important:** `frame_lag_test` is a headless test. It calls `glfwInit()` (we patched that in) so `glfwGetTime()` works for internal profiling, but there's no window.

---

## What's Been Done (Committed Changes Worth Keeping)

### 1. Zero-Allocation String Interning in MathBinding (Phase 1 fix, KEEP)
**File:** `src/Singularity/Core/MathBinding.hpp` (~L36 area, diff shows changes)  
**File:** `src/ConstructedBeing/Singular/Property/PropertyPath.hpp` (+1 line)

Previously, every property lookup in law evaluation was doing string construction for the `Singular.` prefix. We wired the property path's prefix resolution into the preexisting string interning system. Dropped lag from ~232ms to ~94-220ms depending on load.

### 2. `gatesHold` Fix (Phase 2 fix, KEEP)
**File:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` (the `gatesHold` function, ~L2179 in current file)

The old code checked for qualified-root writes BEFORE evaluating the gate. If the gate was false, the law can't run, so it can't write anything — checking writes first was wasted work and also wrong (it would refuse to hoist a false gate if the law wrote to a qualified root). The fix evaluates the gate first: if false, return false immediately; only check for qualified-root writes when the gate is true.

### 3. Compiled Gates Cache (Phase 2.5 fix, KEEP)
**Files:** `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp` (+2 lines), `Law.cpp` (`recompile()` method)

`gatesHold()` was calling `collectHoistableGates()` + `gate->compile()` **every tick for every law**. `compile()` builds a new `std::function` lambda closure each time. Now `recompile()` pre-compiles gates into `_compiledGates` (a `std::vector<ConditionPredicate>`) and `gatesHold` just evaluates the cached predicates.

### 4. `glfwInit()` in frame_lag_test (infrastructure, KEEP)
**File:** `tests/singularity/frame_lag_test.cpp` (+3 lines)

Added `glfwInit()` call so `glfwGetTime()` works inside `LawManager::tick` timing probes.

---

## What's Been Done (Temporary Instrumentation — REMOVE Before Merging)

### Timing Probes in `LawManager::tick()`
**File:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`

I added extensive timing instrumentation throughout `tick()`. These are the `static int tickCount`, `total_seed_ms`, `total_vocab_ms`, `total_eval_dirty_ms`, `total_rest_of_tick`, `total_agenda_size`, `total_agenda_loop_ms`, `total_laws_loop_ms`, `reteCount`, `totalReteTime`, `totalApplyTime`, `sweepCount`, `totalSweepTime`, `global_prop_hears_count`, `global_prop_hears_true_count` variables, plus the big `printf("TICK: ...")` every 24 frames.

**All of this must be removed before merging.** It's diagnostic scaffolding.

The output from the last successful run (before the compiled-gates cache, so you should see improvement):
```
TICK: agenda_size=1512, agenda_loop=1.844, laws_loop=2801.138,
     reteCount=15 (0.046 ms rete, 106.275 ms apply),
     sweepCount=2 (1.015 ms sweep), seed=7.618, vocab=0.667,
     evaluateDirty=1149.637, restOfTick=3980.678
```

Over 24 frames, that's:
- **laws_loop = 2801ms** → ~117ms/frame — the continuous law sweep. THE #1 COST.
- **evaluateDirty = 1150ms** → ~48ms/frame — Rete dirty-fact retract+reassert. THE #2 COST.
- **apply (via rete terminals) = 106ms** → ~4.4ms/frame — moderate
- **agenda_loop = 1.8ms** → negligible
- **sweep (non-rete path) = 1ms** → negligible
- **seed = 7.6ms** → negligible
- **vocab = 0.7ms** → negligible

---

## Current Build State

The code **compiles successfully** as of my last build (exit code 0). The timing probes are in place but the `global_prop_hears_count`/`global_prop_hears_true_count` counters are now file-scope `static int` globals (at ~L1779) visible to both `propheticHears()` and `tick()`.

---

## THE TWO REMAINING BOTTLENECKS (What You Need to Fix)

### Bottleneck 1: `ReteNetwork::retractFact` and `assertFact` — O(N×M) per dirty fact

**File:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`  
**Functions:** `ReteNetwork::retractFact` (~L958), `ReteNetwork::assertFact` (~L760)

`evaluateDirty()` (~L1061) iterates dirty facts and calls `retractFact` then `assertFact` for each one.

**`retractFact(factId)`** does:
1. `std::remove_if` sweep over `_facts` (ALL facts in the network)
2. `std::remove_if` sweep over EVERY `alpha.memory` (every alpha node's memory)
3. `std::remove_if` sweep over EVERY `beta.memory` (every beta node's memory)  
4. `std::remove_if` sweep over `_agenda`

This is **O(Facts × Nodes)** PER RETRACTION. With ~1599 objects × ~10 properties each = ~16000 facts, and dozens of alpha+beta nodes, a single `evaluateDirty()` call on a few hundred dirty facts triggers **millions** of loop iterations.

**`assertFact(fact)`** does:
1. Linear loop over ALL `_alphaNodes`, testing predicate
2. For matching alphas, linear loop over ALL `_betaNodes`, computing joins
3. Pushes matches into `_agenda`

**The fix:** Index facts to nodes. Options:
- **Fact-to-node index:** `std::unordered_map<factId, std::vector<nodeId>>` — when retracting a fact, only visit the nodes it's actually in
- **Indexed alpha memories:** Use `std::unordered_set` or `std::unordered_map` keyed by fact ID instead of `std::vector` with `remove_if`
- **Alpha dispatch by type:** `assertFact` currently tests every alpha predicate. Since most alphas are "type == X" (via `internTypeAlpha`), index alphas by fact type for O(1) dispatch instead of O(alphas)

### Bottleneck 2: The Continuous Law Sweep — O(Laws × Subjects)

**File:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, inside `tick()` starting ~L1958

The `for (const auto& law : continuousLaws)` loop runs every tick. For each law:
1. Check `gatesHold` (now cached — should be fast)
2. If it has Rete terminals and is WhileTrue → collect terminal subjects and apply (the "rete path")
3. Otherwise → `sweepSubjects(law)` + `conditionsSatisfied` for each subject (the "sweep path")

The timing shows `reteCount=15, sweepCount=2` — most laws take the rete path, which is good. But even the rete path's `apply` costs **106ms over 24 frames** (4.4ms/frame). The question is: what inside `applyTo` is expensive?

Likely candidates:
- `Law::applyTo` itself calls `conditionsSatisfied` which re-evaluates conditions via `compile()` closures — string lookups, property resolution
- The law audit logger (`LawAuditLogger::instance().log(...)`) inside condition evaluation (see `ConditionModel.cpp` ~L229) — it logs on EVERY evaluation, even successes. For 1599 objects × 15 laws = 24000 condition evaluations per tick, the string formatting alone could be significant

**Investigation approach:**
- Add timing inside the rete-path loop: how much is `collectTerminalSubjects` vs `applyAndMaybeDrive`?
- Inside `applyAndMaybeDrive` → `law.applyTo(subject)` → how much is condition eval vs action execution?
- Check if `LawAuditLogger` is doing expensive string formatting on the hot path

---

## Files You'll Need to Read

| File | Why |
|---|---|
| `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` | The main file. `LawManager::tick()` (~L1808), `ReteNetwork::retractFact` (~L958), `ReteNetwork::assertFact` (~L760), `ReteNetwork::evaluateDirty` (~L1061), `gatesHold` (~L2179) |
| `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp` | Class definitions for `Law`, `ReteNetwork`, `LawManager` |
| `src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp` | `ConditionNode::compile()` (~L229) — the condition closures |
| `tests/singularity/frame_lag_test.cpp` | The test harness |
| `tests/singularity/frame_lag_baseline.txt` | The baseline numbers the test checks against |
| `docs/architecture/law/PROPHETIC_RETE.md` | Why the Rete analysis may only ever conclude IMPOSSIBLE, never too-narrow |
| `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.orig` | Original file before any changes — useful for diffing |

---

## What NOT to Do (Zach's Rules)

1. **Don't put scripts in the top-level directory** — use `scratch/` for temporary files
2. **Don't state a hypothesis is true unless it's logically necessary or empirically proven** — Zach said this explicitly
3. **Don't add enum values, new C++ classes for domain nouns, or new top-level directories** — read `GEMINI.md`
4. **Run the test, don't just read the code** — `docs/ENGINEERING_DISCIPLINE.md` says "run things"
5. **The `frame_lag_test` SHAPE section's `EXP-FAIL` for LawManager::tick k=1.272** is the growth-rate regression. k should be ≤1.150. This is the algorithmic issue (O(n²) where it should be O(n log n) or O(n))
6. **Don't use subagents** — they burn through Zach's token quotas (120k+ for basic lookups)

---

## Scratch Files

All my temporary patch scripts are in `scratch/`. The relevant ones:
- `scratch/patch_timing*.py` — the various timing instrumentation patches (diagnostic only)
- `scratch/patch_gates*.py` — the gatesHold fix patches
- `scratch/lag_out*.txt` — captured test output from various runs (lag_out13.txt is the most recent successful one)

There's also `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.orig` — the original file before any changes. Useful for diffing.

---

## Git State

The working tree is on a **detached HEAD** (this happened before I started — Zach noticed). There are uncommitted changes to 25 files. The important changes are all in `Law.cpp`, `Law.hpp`, `MathBinding.hpp`, `PropertyPath.hpp`, and `frame_lag_test.cpp`. The rest are from a prior session (Ourverse, BasicPixelChanger, etc.).

Run `git diff HEAD -- src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` to see exactly what changed.

---

## Summary: Priority Order

1. **Fix `retractFact`/`assertFact` indexing** — this alone should cut `evaluateDirty` from 48ms/frame to <1ms/frame
2. **Profile what's expensive inside the rete-path apply loop** — the 4.4ms/frame there might drop naturally once retract/assert are faster (since `evaluateDirty` won't be polluting caches), or it might need its own fix
3. **Profile the continuous law sweep loop** — 117ms/frame, figure out whether the cost is inside `applyTo`, `conditionsSatisfied`, the audit logger, or subject collection
4. **Remove all timing instrumentation** before merging
5. **Run `frame_lag_test`** and verify the STEADY section shows median ≤2ms and k ≤1.15 in SHAPE
6. **Run the full test suite**: `cmake --build build -j8 && ctest --test-dir build --output-on-failure -j4`
