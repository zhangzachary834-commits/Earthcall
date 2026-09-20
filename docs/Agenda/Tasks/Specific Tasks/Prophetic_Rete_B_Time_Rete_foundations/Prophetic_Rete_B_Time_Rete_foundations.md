# Prophetic Rete (B-Time Rete) — foundations

**Status:** ✅ done and verified  
**Section in the To-Do list:** R&D:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Prophetic Rete (B-Time Rete) — foundations** — done and verified (2026-09-01, Claude Opus 5, session `01FCzFYJGGqm2VKd3LLfVoxj`). Built from Zach's realization in [`law/B-time Rete.md`](../../../../architecture/law/B-time%20Rete.md) and Luna's structured spec [`Rete_Truth_Seeking_Focus.md`](../../Specific%20Tasks/Rete_Truth_Seeking_Focus.md). Full write-up, including what was deliberately NOT built and the ⚑ AUTHOR decisions left to Zach: [`law/PROPHETIC_RETE.md`](../../../../architecture/law/PROPHETIC_RETE.md). What landed: (1) **sound interval arithmetic over OntoMath** (`ScalarForm/Term/TransFactor::evalRange`) — `MathNode::evalRange` previously answered `[-inf,+inf]` for *any* non-constant formula; it now bounds `2x+5` over `x∈[0,10]` to `[5,25]` (Luna's §7 worked example), `x²` over `[-3,2]` to `[0,9]`, and `0.5·sin(2πt)+0.5` to `[0,1]` with `t` unbound, finding sin/cos extrema inside the arc rather than assuming `[-1,1]`; `Interval::operator*` is now NaN-safe (`0·inf` was poisoning bounds the SDF tessellator's straddle test culls cells with). (2) **`Prophetic::Index`** (`src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.{hpp,cpp}`) — the abstract value lattice, write-effect extraction per `ActionNode::Kind`, read-demand extraction with the correct `All`-meets/`Any`-joins combination rules, and the possibility-space filters, rebuilt only when `Law::textRevision()` moves. (3) **Passes 1/2 wired** — `LawManager::propheticHears` gates the property-change callback, skipping `markFactDirty`'s full fact-list scan for any property no authored condition can read; fails open three ways (stale index / incomplete index / a `Foreign`-tagged alpha node). (4) **Pass 3 as a finding** — `Index::unreachable()` reports a condition no authored Law can drive into its satisfying range, to the audit log. Guarded by `tests/law/prophetic_rete_test.cpp` (7 sections; **F is the safety section** — it fires real laws through a real `LawManager` and asserts they still hear).


## 2026-09-19 — guarded current-dependent write refinement

The foundation has now gained the first write-state/fixpoint rung that its architecture document
previously named as the highest-value missing refinement.

Whole-Law analysis now seeds an abstract pre-state from authored condition demands and carries that
state through ordered `Sequence` actions. This makes guarded `Add`, `Scale`, and `Lerp`
finite where the Law text actually proves a finite input; lets an earlier `Set` establish the
state for a later current-dependent action; gives `Map` its authored binding bounds; and lets
`Flow` narrow only when the Law also bounds `time.delta` (with zero-rate Flow treated as
identity). `Parallel` siblings remain simultaneous and cannot borrow one another's writes.

The fail-open covenant remains explicit: an unguarded current value is still `Top`, because a
First Mover or foreign channel may have supplied anything. The broader cross-Law widening closure
over mutually dependent writers remains future precision work; this pass does not assume a closed
world.

Witness: `tests/law/prophetic_rete_test.cpp` Section I. Post-recovery CI on corrected default
passed the focused CPU suite after the PR #53 temporal-rollback containment rebase.


## 2026-09-19 — unknown-variable frontier, first rung

Following Zach's §20/§21 requirement that external/First-Mover influence remain explicit rather than
silently guessed away, the relevance graph now preserves structurally known edges even when another
source is opaque. Opaque writers are represented separately as
`UnknownWriteSource{lawId, why, hasModeledWrites}`.

This is deliberately a **knowledge/authority split**: `relevanceComplete()` remains false, so the
partial graph cannot narrow runtime execution and Formation Rete must fall back exactly as before.
The gain is epistemic rather than permissive: a model-backed `FirstMoverLaw` can now say both
"this ActionModel definitely writes here" and "my C++ actuation may also do something Prophetic
cannot enumerate" without erasing every known relation in the register.

The next prerequisite for general cross-Law widening is path-granular unknown influence: First
Mover/property provenance must establish what an opaque source can touch before any solver may use
the absence of an unknown edge as evidence.


## 2026-09-19 — path-granular unknown-domain representation

Continuation from the unknown-variable frontier handoff. `UnknownWriteSource` now separates
positive domain knowledge (`knownMayWritePaths`) from proof that the domain is exhaustive
(`domainComplete`). An incomplete domain remains wildcard for negative reasoning even when it
already names some paths; a complete domain may prove disjoint properties unreachable by that
source. `unknownWriteMayReach(path)` and `unknownWriteDomainCompleteFor(path)` make this
distinction queryable and the JSON report exposes it.

This does **not** hardcode per-channel capability lists. Current C++ First Movers remain
domain-incomplete because Earthcall does not yet have a truthful ontology-native source proving
their full in-world actuation footprint. The next rung is to ground these domains in
First-Mover/property capability provenance and declare its invalidation revision in the Derived
State Ledger; only then should the SCC-based cross-Law widening solver consume absence of unknown
influence as authority.
