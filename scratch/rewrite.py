import re

with open("scratch/analysis.md", "r") as f:
    content = f.read()

# 1. Update Section 1 / 3 Archetype logic
content = content.replace(
    "**Verdict:** Incompatible with a highly authored, heterogenous ontology.",
    "**Verdict:** Incompatible with a highly authored, heterogenous ontology if implemented as C++ types. However, a purely *runtime* archetype table keyed by authored property sets—which carves nothing into the type system—is viable and legible to law, even if it leads to empirical fragmentation."
)

# 2. Update Cycle counts
content = content.replace(
    "Property Lookup (Structure of Arrays linear integer scan): ~5-15 cycles.",
    "Property Lookup (Structure of Arrays `startIndex` offset lookup): 1 cycle (zero-allocation)."
)

content = content.replace(
    "* **The Current Engine:** ~300-500 cycles (string allocations, virtual lambda captures, cache misses).",
    "* **The Current Engine:** (Baseline heavily dominated by world load/terrain tessellation, but lookup previously suffered from string allocations before Phase 1 Interning)."
)


# 3. Update The Invalidation Path
old_invalidation = """**The Invalidation Path (First Movers):**
If a First Mover (a human author or an external API) suddenly injects a structural change, it bypasses the Laws. However, Earthcall's architecture is already fail-safe: any such intervention bumps `Law::textRevision()`, instantly dirtying the Prophetic Index."""

new_invalidation = """**The Invalidation Path (First Movers):**
If a First Mover (a human author or an external API) suddenly injects a structural change, it bypasses the Laws. However, Earthcall's architecture is now fail-safe: any such intervention (adding/removing properties, or spawning/destroying beings) bumps `Universe::instance().structuralRevision()`, instantly dirtying the Prophetic Index."""

content = content.replace(old_invalidation, new_invalidation)


# 4. Emphasize Refusal 6
old_verdict_2 = "**Verdict:** The optimal, scale-ready solution that protects Earthcall's ontological principles for normal planetary simulation."
new_verdict_2 = """**Verdict:** The optimal, scale-ready solution that protects Earthcall's ontological principles for normal planetary simulation.
*Crucially, this satisfies Refusal 6 (No black box)*: The bytecode remains the authoritative form, fully legible, diffable, and transparent to Persons. Native code is only ever a pure cache of it."""
content = content.replace(old_verdict_2, new_verdict_2)

# 5. Add Zach attribution to Prophetic JIT
content = content.replace(
    "### The Earthcall Breakthrough: Prophetic JIT",
    "### The Earthcall Breakthrough: Prophetic JIT (Originated by Zach)"
)

# 6. Add Opacity analysis
opacity_addition = """

**Opacity, Disjointness, and Fallback Economics:**
For this 1.0x proof to hold, the Prophetic Rete must prove a "No" (disjointness). However, many actions (like `Create`, `Spawn`, `FirstMoverLaw`) are structurally opaque. A single opaque action makes the write set unbounded, returning "I cannot say."
In a real law register, a non-zero fraction of laws can be proven quiescent only if opacity is *path-granular*—bounding *which* properties an opaque action can touch. If the Rete cannot prove disjointness (the likely reality for heavily dynamic zones), the JIT falls back to inserting bailout guards, mirroring V8.
In this fallback scenario, the JIT operates at the standard **1.1x–1.5x C++ speed**, which remains the quoted, empirically defensible baseline for unguarded paths."""

content = content.replace(
    "before JITing the new, unguarded reality.",
    "before JITing the new, unguarded reality." + opacity_addition
)

# Remove the Opus 5 reply section entirely, as we have now integrated the feedback.
content = content.split("--- \n\n# Reply — The Prophetic JIT Is Aimed Correctly")[0]

with open("scratch/analysis_new.md", "w") as f:
    f.write(content)

