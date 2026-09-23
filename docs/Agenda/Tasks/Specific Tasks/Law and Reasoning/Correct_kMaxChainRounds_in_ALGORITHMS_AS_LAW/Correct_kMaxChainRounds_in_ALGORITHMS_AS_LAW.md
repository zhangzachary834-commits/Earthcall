# Correct `kMaxChainRounds` in `ALGORITHMS_AS_LAW.md`

**Status:** Done and source-verified, 2026-09-12.

## Person intent

Zach's Agenda called out a concrete documentation error: `ALGORITHMS_AS_LAW.md` repeatedly described `kMaxChainRounds = 8` as a C++ constant even though no such constant exists. Zach's later bound-authoring direction also clarified that the existence of a bound is doctrine while the exact chain-round value should be authorable policy rather than hidden kernel policy.

## What the current tree actually says

`LawManager` owns `int _maxChainRounds = 5`, exposes `maxChainRounds()` and `setMaxChainRounds(int)`, uses `_maxChainRounds` in the `tick()` cascade loop, and serializes the setting. `tests/law/law_loop_test.cpp` checks against `mgr.maxChainRounds()` rather than a hardcoded number.

## Correction made

`docs/architecture/law/ALGORITHMS_AS_LAW.md` now:

- removes the nonexistent `kMaxChainRounds = 8` symbol and hardcoded eight-round claims;
- names `LawManager::maxChainRounds()` / `_maxChainRounds` as the real mechanism and documents the current default of 5;
- distinguishes the doctrine of *having an explicit bound* from the authored policy of *where that bound sits*;
- updates the within-tick iteration table, cascade pseudocode, BFS example, complexity table, non-expressibility table, Turing-completeness discussion, anti-patterns, and closing summary so they no longer contradict the current implementation or Zach's bound-authoring ruling.

## Verification

Source-verified against:

- `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp` (`maxChainRounds`, `setMaxChainRounds`, `_maxChainRounds = 5`);
- `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` (`tick()` uses `_maxChainRounds`);
- `tests/law/law_loop_test.cpp` (asserts relative to `mgr.maxChainRounds()`);
- `docs/Reflections on Earthcall's Progression/Reflections on the Substrate/When_Bounds_Are_Doctrine_And_When_They_Are_Not.md` (Zach's authorable-bound ruling).

No executable behavior changed, so this task did not require a runtime build/test claim. The separate Agenda item to revise `INTELLECTUAL_LINEAGE.md` against its verification audit remains open and was intentionally not folded into this task.