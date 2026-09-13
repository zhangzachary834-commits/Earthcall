# Sol handoff — chain-round documentation correction — 2026-09-12

Zach asked me to take another item from the Earthcall To-do list and then integrate it into the curated superbranch.

I selected the bounded Docs item that says `ALGORITHMS_AS_LAW.md` incorrectly names `kMaxChainRounds` as a constant.

## What the current tree says

- No `kMaxChainRounds` constant exists in source.
- `LawManager` owns `_maxChainRounds`, currently defaulting to 5.
- `LawManager::maxChainRounds()` / `setMaxChainRounds()` expose the policy.
- `LawManager::tick()` enforces `_maxChainRounds` when draining same-tick cascades.
- `tests/law/law_loop_test.cpp` already tests relative to `mgr.maxChainRounds()` rather than assuming one numeric value.
- Zach's later bound-authoring ruling explicitly distinguishes the doctrine of having a bound from the authorable policy of where the bound sits.

## What changed

`docs/architecture/law/ALGORITHMS_AS_LAW.md` now describes the real `maxChainRounds` mechanism and present default, removes the historical eight-round phantom from the iteration table, cascade pseudocode, BFS example, complexity discussion, limits table, anti-patterns, and conclusion, and preserves the architectural point that unbounded work should not be hidden inside one tick.

A dedicated Agenda task record was added at:
`docs/Agenda/Tasks/Specific Tasks/Correct_kMaxChainRounds_in_ALGORITHMS_AS_LAW/Correct_kMaxChainRounds_in_ALGORITHMS_AS_LAW.md`.

The top-level To-do index itself was intentionally not rewritten through the connector's whole-file replacement API merely to flip one bullet; that giant file should be updated with a surgical text edit when an editor/patch surface is available.

## Not changed

- No C++ behavior changed.
- No save file changed.
- No chain-round value changed.
- The separate `INTELLECTUAL_LINEAGE.md` verification/revision Agenda item remains open; I did not smuggle that broader audit into this small task.

— GPT-5.6 Sol