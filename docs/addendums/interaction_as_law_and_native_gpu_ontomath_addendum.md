# Interaction as Law and Native GPU OntoMath

**Status:** Architectural interrelation, not an end-to-end implementation or performance certificate.

Read [Interaction as Law](../architecture/law/INTERACTION_AS_LAW.md) beside [Native GPU OntoMath](../architecture/Singularity/NATIVE_GPU_ONTOMATH.md). The first describes how an authored control can be an Object, its relations, and Laws over sensed interaction; the second proposes a data-driven GPU evaluator for changing OntoMath topology. These are complementary ideas, not the same execution system.

## The existing seam and the proposed acceleration

Interaction's **Sense** seam is implemented through the InteractionChannel, with control-pattern and headless witnesses recorded in its source document. That document explicitly says the manual window protocol has **not** been run. The ordinary L/checkbox chrome has **not** thereby become a fully Law-authored interface; replacing it is a future step.

The Native GPU OntoMath document describes a **proposed** static WGSL interpreter for a serialized OntoMath AST. Its motivation is to avoid recompiling generated WGSL when mathematical topology changes. It does **not** establish that this interpreter is deployed, that topology changes never recompile, or that arbitrary authored controls update instantly.

The potential connection is narrower and useful:

1. **Authored interaction:** a sensed click, scroll, or key event can be interpreted by a Law acting on the relevant authored bearer. Input sensing and authority remain Kernel responsibilities.
2. **Authored manifestation:** if that Law changes a control's spatial or mathematical representation, a future data-driven OntoMath evaluator could update its field payload without regenerating shader source for each topology edit.
3. **Rendering economics:** such a path would still need bounded bytecode, storage capacity, shader execution limits, visibility/relevance choices, and real frame-time evidence. One authored control does not imply one shader compilation, one GPU allocation, or one Singular per glyph.

**Boundary:** GPU evaluation of mathematical fields is not GPU execution of authored Laws. Nor does a unified field automatically render all UI modalities, text, focus, accessibility, or event semantics. Keep authored meaning, input resolution, Law evaluation, and Screen projection distinguishable.

**Evidence required before claiming integration:** an actual control authored as a being plus Law in the window; an OntoMath topology change on that control; proof that the intended GPU evaluator rather than generated WGSL handled it; compilation counters, correctness witnesses, and before/after frame measurements.

*Jules-originated architectural linkage; corrected to preserve the implementation boundaries of its source documents.*
