# Pre-Computation at Law and Prophetic Rete Boundaries

Earthcall moves repeated structural work out of selected inner loops where the implementation has an explicit precomputation boundary. This is a **local** optimization pattern, not a guarantee that the entire Law tick performs no allocations or scans.

## Law compilation

In `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `Law::recompile()` rebuilds compiled condition gates and action closures. When an action model exists, it collects write paths and sets `_writesQualifiedRoots` if a write begins with an `@`-qualified root. It then calls `rebuildRequiredProperties()`, which collects distinct unqualified property roots used by condition and action models.

The current implementation **does not** compute the formerly documented `_writesMotionProperties`, `_writesZoneCanExit`, or `_writesZoneOwner` flags in `recompile()`. Do not infer that every movement or consent decision is O(1), or that `applyToImpl()` never collects paths.

## Prophetic relevance rebuild

In `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp`, `Prophetic::Index::rebuild()` constructs local `writeNorms` and `readNorms` arrays before the pairwise writer/reader relevance loop. The loop then calls `pathsMayAlias` on prepared normalized paths instead of reparsing the same strings for each candidate pair.

These arrays are built **during each rebuild**; this is not a persistent cross-rebuild cache. The implementation also tracks incomplete/opaque relevance, requiring runtime consumers to fall back when the graph is not complete. Optimization must never turn uncertainty into an omitted possible Law edge.

## Measurement and correctness

Any claim of fewer allocations, O(1) end-to-end gating, or wall-frame speedup requires an executable witness or measured A/B comparison on the relevant path. The source establishes where repeated work is reduced; it does not by itself establish a quantified performance gain.
