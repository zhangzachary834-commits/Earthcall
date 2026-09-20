# Prophetic Rete unknown-variable frontier — Sol handoff (2026-09-19)

**From:** GPT-5.6 Sol  
**To:** the next Sun / Earthcall agent continuing Prophetic Rete  
**Human direction:** Zach asked to continue Prophetic Rete after the guard-aware write-state/fixpoint rung was merged, then asked for this handoff before conversation context became unwieldy.

> ## 🚨 DO NOT MERGE PR #250 YET — BASE MOVED 111 COMMITS
>
> At handoff time, feature branch `sol/prophetic-unknown-variable-20260919` is:
>
> - head: `f1b51ae43068351808838a665d9f3edb744c0776`
> - 7 commits ahead of its merge base
> - **111 commits behind current `sync-from-earthcall-main`**
> - historical merge base: `e275a4f472df00273cfb5fb390ad9a52317b33a0`
> - current default observed at handoff: `1ca6adc1cda0d9185344462dda3711c8f7b5b2e7`
> - PR #250 is open + draft
>
> GitHub currently reports PR #250 as mergeable, but **that is NOT sufficient evidence of temporal safety** in this repository.
> Before touching the branch, read the newest `agent intercom/` All-Hands / rollback broadcasts and determine why default advanced by ~111 commits. Do not assume this is ordinary drift.
>
> The PR #53 rollback incident remains the precedent:
> - bad stale-tree merge: `2088f78e00f15bfa93729ede2e8772c6cb69bfce`
> - corrective restore: `8d81dd3c062c7018387024d7ee676a391553c428`
> - broadcast commit: `5c8c40d7fd02b45e65bb04e2d5b36e47ad45d845`
>
> The previous Prophetic fixpoint branch was audited/replayed safely after that incident. Apply the same discipline here.

## What already landed before this branch

PR #236, **Prophetic Rete: guard-aware write-state fixpoint**, was merged before this phase.

That merged rung made whole-Law analysis carry authored pre-state through current-dependent writes:

- guarded `Add`, `Scale`, and `Lerp` get finite interval images;
- ordered `Sequence(Set -> Add)` carries exact state forward;
- `Parallel` siblings do not borrow each other's outputs;
- `Map` receives authored binding bounds;
- `Flow` narrows only when the target/rate inputs and `time.delta` are authored-bounded;
- zero-rate Flow is identity;
- open-world / unguarded current values remain `Top`.

Do **not** reimplement that. It is already on default.

A later CI failure after #236 was unrelated: `person_serialization_test.cpp` could not see `TempSaveRoot`. Provenance showed that defect predated #236, and Jules later fixed it in commit
`ee98fbfea4b49d6700740ae97538a7867343dde4`. Later current-base CI passed.

## Why this next phase exists

The remaining general cross-Law fixpoint cannot soundly infer:

> "there is no authored write edge, therefore this state is impossible"

because Earthcall is intentionally open-world. First Movers, foreign channels, and model-less C++ actuation can inject state outside the authored Law calculus.

Before a global widening solver can use absence as evidence, Prophetic needs an explicit boundary between:

1. what it **knows** from authored/model-backed text; and
2. what an external/First-Mover source may still do but Prophetic cannot enumerate.

Previously, one opaque writer turned the whole relevance graph into an information blackout:
`relevanceComplete() == false` **and the graph was emptied**.

This branch implements the first §20/§21 unknown-variable rung:

> **known facts survive; unknown influence is named; authority does not survive incompleteness.**

## What branch `sol/prophetic-unknown-variable-20260919` implements

### 1. Explicit unknown write frontier

`Prophetic::Index` now exposes:

```cpp
struct UnknownWriteSource {
    std::string lawId;
    std::string why;
    bool hasModeledWrites = false;
};
```

and:

```cpp
const std::vector<UnknownWriteSource>& unknownWriteSources() const;
```

Every `LawFacts::opaqueWrites` source is recorded there during `Index::rebuild()`.

The frontier is also rendered by `Index::toJson()`.

### 2. Known relevance edges survive opacity

The pairwise relevance graph is now constructed from every structurally modeled `WriteEffect` and branch-local `ReadDemand` **even if some other source is opaque**.

This is intentional.

An incomplete graph can now truthfully mean:

> "These edges definitely exist, AND there may be additional edges I cannot enumerate."

That is strictly richer than the old:

> "Something is unknown, therefore erase everything known."

### 3. Completeness still gates ALL narrowing authority

This is the non-negotiable safety covenant:

```text
relevanceComplete() == false
        =>
known edges are diagnostic/provenance facts only
        =>
runtime / Formation-Rete consumers MUST FALL BACK
```

No hot-path narrowing behavior was added by this phase.

Search performed before implementation found no runtime consumer bypassing the existing
`relevanceComplete()` contract.

### 4. FirstMoverLaw can be both known and unknown at once

Current `analyzeLaw()` still marks every `law.isFirstMover()` as `opaqueWrites=true`, because a First Mover may actuate in C++.

But many `FirstMoverLaw` instances also carry a fully authored `ActionModel`.

This branch preserves both truths:

```text
FirstMoverLaw
    |
    +-- ActionModel says hp := 500
    |      -> modeled WriteEffect
    |      -> modeled relevance edge survives
    |
    +-- engine/C++ actuation may do more
           -> UnknownWriteSource
           -> relevanceComplete == false
           -> no narrowing authority
```

Do not "simplify" this by making model-backed First Movers non-opaque unless you can prove they have no extra C++ actuation surface.

### 5. Cross-Law impossibility findings remain suppressed under opacity

The existing `NoLawfulDriver` finding still refuses to run when:

- there is any opaque write; or
- the index is otherwise incomplete.

This branch does **not** make a partial relevance graph authoritative.

## Tests changed

`tests/law/prophetic_rete_test.cpp` §H now witnesses:

1. complete graph still includes the possible high-hp edge and excludes the provably-disjoint low-hp edge;
2. an opaque read makes the graph incomplete **without erasing unrelated known edges**;
3. opaque-read incompleteness does not manufacture unknown write sources;
4. a model-backed `FirstMoverLaw` simultaneously:
   - contributes its modeled `hp := 500` relevance edge,
   - appears as exactly one `UnknownWriteSource`,
   - reports `hasModeledWrites == true`,
   - leaves `relevanceComplete() == false`;
5. JSON exposes both the partial known graph and unknown frontier;
6. incomplete analysis still suppresses cross-Law `NoLawfulDriver` claims.

## Documentation / ledger updated on this branch

These seven files are the complete intended review surface:

1. `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp`
2. `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp`
3. `tests/law/prophetic_rete_test.cpp`
4. `docs/architecture/law/PROPHETIC_RETE.md`
5. `docs/architecture/law/DERIVED_STATE_LEDGER.md`
6. `docs/plans/ontological_rete_architecture.md`
7. `docs/Agenda/Tasks/Specific Tasks/Prophetic_Rete_B_Time_Rete_foundations/Prophetic_Rete_B_Time_Rete_foundations.md`

Before the 111-commit base sprint, the diff against base was exactly those seven files, with no unrelated deletions.

## CI status on feature head `f1b51ae...`

Both workflows completed successfully on the exact feature head:

- push run **35478015251** — ✅ success
- pull-request run **35478286426** — ✅ success

So the implementation itself has a green witness on its original base.

**Those greens do not waive the need to replay/retest against the new 111-commit-ahead current default.**

## PR

PR #250: **Prophetic Rete: explicit unknown-write frontier**

State at handoff:

- open
- draft
- not merged
- head `f1b51ae43068351808838a665d9f3edb744c0776`
- 7 intended changed files
- green push + PR CI on that head

Do not mark ready until the temporal-safety replay below is complete.

## EXACT next actions for the next Sun

### A. First: establish current timeline safety

Do **not** start by rebasing.

1. Read the newest files under `agent intercom/`, especially any All-Hands / rollback / contamination broadcast newer than this note.
2. Identify why `sync-from-earthcall-main` advanced ~111 commits from `e275a4f...` to the live head.
3. Confirm the live default is the intended corrected Earthcall timeline.
4. Only then compare those base advances against the seven files listed above.

### B. Replay safely rather than merging stale history

If the 111 base commits do not overlap these seven files:

1. snapshot the seven files from `f1b51ae...`;
2. reset `sol/prophetic-unknown-variable-20260919` to the verified current default;
3. replay exactly those seven file contents;
4. compare branch vs current default;
5. require:
   - 0 commits behind;
   - only the intended seven files (plus this handoff doc if retained on feature branch);
   - sane additions/deletions;
   - no unexplained mass deletion / stale-tree resurrection.

If there **is** overlap, reconcile only the overlapping hunks. Do not overwrite newer base work wholesale.

### C. Run final-head CI

Run/watch the repository CI on the replayed exact head.

Minimum meaningful gates:

- Focused CPU tests build
- Terminal exact-reference smoke
- focused regression witnesses, including `prophetic_rete_test`
- Slow Adapter soundness/cadence
- authored-world Slow Adapter perf

Use targeted job-step summaries. **DO NOT ingest giant workflow logs.**

If a step fails:

1. inspect step name;
2. grep only diagnostic signatures (`error:`, `FAILED:`, target name, failing assertion);
3. read tiny surrounding slices;
4. compare provenance against base before blaming this branch.

This workflow already caught a pre-existing `TempSaveRoot` defect once; do not repeat the mistake of equating "CI after my merge" with "CI caused by my merge."

### D. Then update PR #250

Only after:

- verified current timeline,
- 0-behind replay,
- intended diff only,
- final exact-head CI green,

mark PR #250 ready for review / merge.

## What comes AFTER PR #250

Do **not** jump straight to a closed-world cross-Law fixed point.

The next prerequisite is:

### Path-granular unknown influence / First-Mover property provenance

Today `UnknownWriteSource` says:

> this source may write something Prophetic cannot enumerate.

It does **not** yet say which property paths/families the unknown transform could possibly touch.

Until Earthcall has a truthful way to derive that boundary from legible First-Mover/property provenance, an opaque source must remain globally capable.

The desired next shape is roughly:

```text
Unknown source F
    |
    +-- proved may touch: position.*, velocity.*
    |
    +-- proved cannot touch: health
```

Then absence of an unknown `health` edge can become meaningful.

Only after that can the general cross-Law least-post-fixpoint / widening solver safely reason that all relevant writers for a path are known.

### General cross-Law widening remains future precision work

Current guard-aware fixpoint:
- condition pre-state
- intra-Sequence propagation

Future:
- mutually dependent Law writers
- monotone iteration
- widening / convergence
- explicit external-source boundary
- no closed-world assumption unless authored/proved

If you cannot prove the external boundary, widen to `Top`. **Never make a Law deaf for speed.**

## Big Chungus rule

Zach explicitly asked not to get swallowed by massive files/logs.

Use the repository's own AGENTS.md rule:

> Retrieval must be proportional to the epistemic need.

For this work:
- search exact symbol first;
- fetch bounded line ranges;
- use workflow step summaries;
- grep only error signatures;
- do not read an entire CI log unless every narrower route failed.

The previous Sun repeatedly became "Workflow Abyss Man" by polling the same job. Do not inherit that spiritual discipline. 😭

## Human-origin thread to preserve

The central architectural intent comes from Zach:

- Prophetic Rete should reason ahead of runtime;
- it must never narrow reality beyond what authored truth proves;
- First Movers / foreign channels are genuine unknown variables, not excuses to assume a closed Law universe;
- Formation Rete should eventually consume Prophetic proof as increasingly direct relevance roads;
- derived-state optimizations must retain sound lower-tier fallbacks.

This branch's contribution is narrower: it turns "unknown exists" from an information eraser into a **named frontier around preserved knowledge**.

— GPT-5.6 Sol
