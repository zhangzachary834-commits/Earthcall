# `ForAny` / `ForAll` compile to an unfiltered Universe scan

**Status:** open, **read but not yet measured**. Found 2026-09-08 by Claude Opus 5, session
`01Jf1mZyMWX69HHkG43qMv3F`. Architecture context:
[FORMATION_RETE.md](../../../../architecture/law/FORMATION_RETE.md) §1.2(b), §8 step 1.

## The finding

`ForAny`/`ForAll` compile to a closure that loops `Universe::instance().beings()`
(`ConditionModel.cpp:424`) — a vector the provider **rebuilds on every call**.

In `compileToRete`, quantifiers are neither `All` nor `Any`, so they fall through to the
leaf path, where `targetAttr` is set only for `Compare` and `Related`. A quantified
condition therefore becomes **an alpha node with no attribute filter whose predicate scans
the entire Universe** — woken by every property-state fact of every being. The network
believes it is holding a cheap filter; it is holding a full scan.

On the sweep path the cost is cleaner to state: `collectPaths` correctly returns early for
quantifiers, so a law whose only condition is a `ForAny` has **empty `requiredProperties`**
→ `sweepSubjects` returns *every* being ("no stated requirements = no filter") →
**O(N) subjects × O(N) inner scan = O(N²) per tick.**

## What must NOT be "fixed"

Two things the tree gets right, and which look like bugs if skimmed:

- `collectPaths` returning early for quantifiers — the inner condition is about the
  instances, not the subject.
- `PropheticRete::walkCondition` filing a quantifier's inner demands as instance reads that
  do **not** propagate to the parent, while still recording their `readNames`
  (`PropheticRete.cpp:496–506`). Demands narrow; read-names widen. Each goes the sound
  direction, and the reactive path stays reachable *because* read-names propagate.

## Next step

**Measure before fixing** — this finding is from reading the source, not from running
anything. `cmake --build build --target lag` with a world holding a purely quantified
`WhileTrue` law would witness it.

Then give quantifiers a real index. Formation Rete's Layer 0 (Categories as authored
Formations replacing the implicit `couldApplyTo` vocabulary filter) is the intended home:
a quantifier ranging over a `BeingKind` is a Category query wearing a loop.
