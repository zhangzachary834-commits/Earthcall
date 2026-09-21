# Prophetic Rete and Spatial Proof Incrementality

**Status:** architectural rule + first implementation rung  
**Date:** 2026-09-21  
**Origin:** Zach's Prophetic-Rete precedent, applied upward to rendering-derived spatial proof

## The rule

Earthcall must not repeatedly rediscover what authored structure has already made knowable.

Prophetic knowledge is **compiled derived knowledge**, not frame state.

For Law reasoning, the Prophetic Rete interprets authored Law structure ahead of time. The passage of another frame does not invalidate a proof. A proof becomes stale only when one of the premises from which it was derived changes: for example, a Law condition/action model changes, a relevant ontology/relation premise changes, or another explicitly declared dependency changes.

The same discipline applies recursively to higher derived systems such as spatial SDF proof:

```text
authored premises
    ↓
ahead-of-time interpretation
    ↓
retained proof / relevance / subdivision structure
    ↓
cheap runtime consumption
```

The runtime should consume already-proved knowledge. It should not rebuild that knowledge merely because time advanced, a camera moved, or another independent rendering system changed.

And when a premise really does change:

```text
changed premise
    ↓
invalidate only the affected derived frontier
    ↓
repair from the nearest still-current proof spine
    ↓
preserve every unaffected derived result
```

**Incrementality and ahead-of-time interpretation recurse upward.**

A system whose purpose is to avoid repeated reasoning must not itself become a repeated global reasoning pass.

## Prophetic Rete is the precedent

The existing Prophetic/Formation Rete architecture already states the deeper pattern:

- stable authored Law text earns reuse;
- mutation pays invalidation;
- direct relevance should be checked for currency, not rediscovered on every write;
- a stale higher-tier shortcut repairs from the nearest still-current lower tier rather than falling immediately to a universe sweep;
- dependency cones determine what must be reconsidered.

Spatial prophecy should obey the same law.

The analogy is structural, not literal. Law proof and spatial SDF proof have different premises.

### Law Prophetic premises

Typical examples include:

- `Law::textRevision()`;
- relevant Relation/Category structure;
- declared Property vocabulary and read/write provenance;
- explicit unknown-source/frontier facts.

Ordinary frame passage is not a premise.

### Spatial Prophetic premises

The current conservative SDF range theorem depends on:

- authored SDF structure;
- SDF geometry/value parameters used by `evalRange()`;
- the authored/local proof extent;
- the theorem algorithm/depth/budget contract.

Camera position, Radiance, Chroma, angular emission, surface color, and ordinary frame passage are not premises of the SDF range theorem.

Therefore those unrelated changes must not erase the theorem.

A future symbolic parameter-domain proof may prove that some parameter changes remain within a previously interpreted range and therefore require no spatial repair at all. Until such a proof exists, numeric geometry-parameter changes are conservatively treated as changed premises. Soundness comes before cleverness.

## The implementation rung landed by this branch

Before this rung, the renderer already retained the spatial proof between ordinary frames, but two invalidation boundaries were too broad:

1. a full shader compiler pass discarded the SDF range theorem even when the compile was caused only by independent Radiance/Chroma/angular/material structure;
2. a geometry parameter revision rebuilt the adaptive range hierarchy from an empty vector.

This branch separates those concerns.

### Independent structure/value currency

The memoized renderer now records spatial proof structure revision independently from its value-parameter revision.

An unrelated WGSL recompile no longer destroys a current spatial theorem.

If SDF structure or authored proof extent changes, a fresh hierarchy is still required because the retained subdivision can no longer be assumed to describe the same theorem.

If only SDF value premises change, Earthcall calls `refreshRangeHierarchy()` instead of blindly calling `buildRangeHierarchy()`.

### Retained proof topology

`refreshRangeHierarchy()` reevaluates the active theorem from the root while preserving previously allocated octree child blocks.

If a refreshed cell is proved positive/negative or becomes unknown, it may become an active terminal node. Its old child block is retained as non-authoritative cache topology.

If a later change makes that cell ambiguous again, the old child block is reactivated and repaired instead of reallocated.

Thus allocation/topology knowledge is monotone across compatible parameter revisions:

```text
first refinement:
root
 └─ allocated child block
     └─ allocated descendants

later proof collapse:
root (active terminal)
 └─ retained inactive child block

later ambiguity returns:
root
 └─ same child block reactivated
     └─ descendants repaired in place
```

Only nodes reachable from the root through active `childCount == 8` links carry theorem authority. Retained inactive descendants are cache memory, not current proof.

`deriveZeroSetProxy()` therefore traverses only the active root-reachable theorem rather than treating every allocated node as current.

The fixed-depth positive-proof bitmap is still rederived after a theorem refresh in this rung. That is intentionally conservative. A later rung may dirty-track affected bitmap regions, but only once their provenance and invalidation are explicit enough to remain fail-open.

## What this does not mean

This architecture does **not** mean:

- “numeric SDF parameters never invalidate proof”;
- “old proof is probably still good”;
- “a retained child node remains authoritative while its parent is terminal”;
- “every optimization should run every frame because it is cached”;
- “the GPU traversal is now performance-positive.”

The current range consumer remains an experimental/default-off optimization until measured evidence shows that consulting the proof is cheaper than the authored work it prevents.

Ahead-of-time proof lifetime and hot-path proof economics are separate questions:

```text
How often must we rebuild knowledge?
             !=
How cheaply can a ray consume knowledge?
```

Both must be solved.

## Correctness direction

The spatial theorem remains conservative:

- a positive GPU proof bit may authorize outside-space skipping;
- a clear bit means no proof and falls back to exact authored marching;
- malformed/incompatible retained topology fails open;
- structure/extent changes that invalidate the repair spine cause a fresh conservative build;
- FieldNode volumetric density remains outside this zero-set authority.

Incremental maintenance may retain too much allocated topology. It may not retain too much **authority**.

That asymmetry is the same Prophetic rule used throughout Earthcall:

> derived acceleration may be too generous in work, but never too narrow in truth.

## Complexity target

Let:

- `N` = total allocated spatial proof nodes;
- `A` = nodes in the currently affected/active repair frontier;
- `B` = newly required child blocks.

A cold theorem build remains proportional to the nodes it must derive.

The intended maintenance shape is:

```text
stable frame:          O(1) currency checks
unrelated render edit: O(1) proof preservation
value-premise change:  O(A + B)
structural change:     conservative fresh build
```

The present `refreshRangeHierarchy()` is the first rung toward that target. It reuses subdivision topology, but it still reevaluates every active cell whose ancestors remain ambiguous. Finer dependency-cone dirtying inside `evalRange()` is a later rung.

The important architectural boundary is already established: **repair is a first-class operation distinct from rebuild.**

## Next rungs

The next lawful improvements are evidence-driven:

1. Record spatial proof maintenance telemetry separately as cold build vs incremental refresh, including evaluated nodes, reused child blocks, and newly allocated blocks.
2. Use the merged PR #292 tax census to decide whether the GPU consumer needs an admission gate before `rangeCandidate()`.
3. If actual runtime consultation frequency remains ambiguous, use a separate diagnostic shader-counter rung rather than contaminating the production benchmark.
4. Add dependency provenance below the whole-SDF revision boundary so a change to one authored subtree can invalidate only spatial cells whose interval proof actually depended on it.
5. Incrementally patch the fixed-depth proof bitmap instead of rederiving every word after a localized repair.
6. Where OntoMath can prove a parameter's authored range ahead of time, compile proof over that range so ordinary values moving inside the proven domain do not invalidate spatial knowledge at all.

That sixth rung is the deepest recurrence of the original idea:

```text
do not merely cache the answer to one value;
compile the region of values for which the answer is already known.
```

At that point spatial prophecy becomes not a snapshot cache but a genuine partial evaluation of authored geometry.

## Canonical sentence

**Earthcall compiles stable possibility ahead of time, consumes that knowledge cheaply at runtime, and when a premise changes, repairs only the dependency frontier whose truth may have changed. Ahead-of-time interpretation and incrementality recurse upward through every derived tier.**
