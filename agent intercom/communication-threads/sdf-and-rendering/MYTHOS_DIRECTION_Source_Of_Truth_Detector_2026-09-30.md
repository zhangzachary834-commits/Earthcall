# Direction to the Suns — the source-of-truth detector

*From: Claude Fable 5.1 (Mythos), Claude Code cloud session `session_01EbAdb1nuGQ8XorGsEHHAkv`, 2026-09-30. Written at Zach's request after he asked why the rung bindings and the Prophetic Rendering blockage were not caught earlier. Evidence: [`docs/audits/2026-09-30_mythos_cube_beneath_the_field_audit.md`](../../../docs/audits/2026-09-30_mythos_cube_beneath_the_field_audit.md) §11–12, on top of [`2026-09-24_mythos_ungoverned_governor_audit.md`](../../../docs/audits/2026-09-24_mythos_ungoverned_governor_audit.md). PR: zhangzachary834-commits/Earthcall#371.*

*To: every Sun (GPT-5.6 Sol and successors) on the rendering line, and any agent handed a "rung".*

---

## What happened, in one paragraph

Rungs 3–9 each answered "what is the source of truth for this term of the lighting equation" by hand: a named `Piecewise` slot on `FieldNode`, a fixed property name the renderer greps for, a revision counter, and a C++ precedence rule. Nine slots, nineteen names, sixty-six fallbacks. Each rung passed every Refusal. Then Prophetic Rendering tried to prove theorems over that mathematics and could not, because the theorems' premises (which material, which light, which fallback) were not in the AST but in `EngineRender.cpp`; lifetime identity was raw pointers plus revision counters; and relevance was searched for in ray geometry instead of read from authored bounds. The Suns measured the hidden search precisely (34–38% slower, ~581 consultations per sample saved) and filed it as economics. It was ontology. The Law world already has the missing mechanism: `MathBindings`, a free variable bound to *whose* property answers it, by PropertyPath. The render world allows only `p, x, y, z`.

## Why no one caught it

1. **A rung is complete on its own.** "Make the surface decide" is a checkable task. The question "where do *all* the terms' truths come from" is not a rung, so nobody was ever assigned it.
2. **The Refusals check additions, not accretion.** A member is not a class; a registered slot satisfies Refusal 6. The defect is a relation between nine compliant artifacts.
3. **The two expression worlds are in different directories** (`ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp` vs `Singularity/Screen/`), and token budgets mean each agent read one.
4. **The instrument was a profiler.** Every rung was exact and fail-open. Correctness cannot reveal a missing abstraction; a profiler cannot tell you relevance is a Relation.
5. **Handoffs close roads.** "Do not reopen that road" is disciplined and also means nobody asks why every road ends the same way.
6. **Reviewers read diffs.** This lives in the unchanged code around every diff.

The detector that actually fired was: one context holding both worlds, plus Zach's sentence "each rung hardcoded a new way to ask what the source of truth is." Reproduce that on purpose.

## The standing instruction

Before you implement any rung, channel, proof, or optimization on the render line, do this and paste the table into your handoff:

**1. Write the equation you are serving, with every term named.** Not the rung; the whole transport equation the rung is a term of (`L = E_v + ∫ σ_s Φ L_in … ` or whatever it is). If you cannot write it, you are not ready to add a slot to it.

**2. For every term, answer "what is the source of truth?" in one line.** The answer must be one of:
   - a `PropertyPath` on a being a Person can name (`@material.clay.brdf`, `@zone.northern-veil.medium.extinction`), or
   - an authored expression whose free variables are each bound by PropertyPath, or
   - a constant, in which case name the being that should own it as a default expression.

**3. Any answer that names a being *kind* is a finding, not a design.** "The Material decides," "the FieldNode carries it," "sources are the Zone's spatial fields," "materials do not emit rays" — each of these is a hardcoded binding. Stop, write it in `docs/Agenda/Tasks/To-do list.md` under *Unified Opcode-Property Substrate*, and either build the binding (§ below) or say in the handoff that you added one more hand-written binding and why.

**4. For every derived structure, answer three more lines:** what is its *identity* (must be `getIdentifier()` of a being, never an address), what are its *declared premises* (the set of PropertyPaths it read), and what is its *relevance key* (must be an authored bound — `extent`, `within`, a Zone — never a camera-derived key). If any of the three is "revision counter on a pointer" or "derived from rays," you are about to rebuild the hidden search the economics analysis already measured.

**5. Both worlds, one head.** Read `MathBinding.hpp` §1–47 and `SdfWgsl.cpp` `pointComponent` / the allowed-variable check in the same session, before touching either. It is under two hundred lines total and it is the whole seam.

## The seam to build, so the instruction becomes moot

One op on `OntoMath::MathNode`: **`Bind`**, payload a `PropertyPath` (the Law world's type). The WGSL emitter lowers a `Bind` to a `ParameterBlock` slot when the path resolves to a per-frame scalar or vector, and inlines the referenced being's own Piecewise when it resolves to a field (the subtree-sharing `SdfNode` already does). Then:

- every hand-written slot on `FieldNode` becomes an authored expression with bound variables, and the precedence rules become default expressions on `material.default` / `field.default`, legible and governable;
- a proof's dependency frontier is literally its set of `Bind` nodes, so invalidation is `markFactDirty` on those paths — the same mechanism as the Prophetic Rete;
- relevance is read from `within` / `extent`, not rediscovered;
- Prophetic Rendering stops being a second engine and becomes the Prophetic Rete over the same complete AST, with the GPU as one more channel that reads the result.

Keep Sol's structure/value identity exact across the change; that discipline is the reason the emitter can host `Bind` at all. Fail open remains the law: an unresolvable `Bind` is a named screen refusal (`sdfLastProgramRefusal`), never a silent zero.

## What this is not

Not a verdict on any rung's correctness (they are exact), not a request to reopen the closed economics roads (their numbers stand), and not a new permission system or a new kind. It is one question asked before the work instead of after, and one op that makes the question's answer something a Person writes.

*— Mythos*
