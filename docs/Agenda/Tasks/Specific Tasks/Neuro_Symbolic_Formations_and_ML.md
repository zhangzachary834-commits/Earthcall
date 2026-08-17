# Neuro-Symbolic Formations and ML Integration

**Status**: In Progress / Partial (2026-08-13)  
**Related**: `Formation`, `Relation`, `LanguageSystem.cpp`, `Law`

## Current Implementation
- Native neuro-symbolic substrate implemented using `Formation` as neural structure.
- Unsupervised Hebbian learning executes via dynamic weight reinforcement/decay (`Relation.weight` as a `ComputedProperty` in `LanguageSystem::tick`).
- Graph remains inspectable and governable by ECA Laws without opaque tensors. Semantic inference laws derive logical consequences over weighted graphs.

## Design Directions (2026-08-14)
- **(a) Edge transfer functions**: Relations carry a `Piecewise` model with `weight` as a bound variable (ReLU via interval bounds, gating via `whereLEZero`).
- **(b) Exact backpropagation**: `∂(edge)/∂(weight)` computed via `ScalarForm::derivative` with explicit `nullopt` handling.
- **(c) Mixture-of-Experts**: `ConditionNode` world guards for ontological routing.
- **(d) Bayesian edges**: Weight represented as `Distribution` with `sample()` and `expectedValue()`.
- **(e) Field-valued activations**: Structurally checked equivariance via MathNode type calculus.
- **(f) Spike-Timing-Dependent Plasticity (STDP)**: Driven by `Drive`/`CurveModel` on event timing (`RelationEvent`).
- **(g) Aggregation**: `Fold` operations with explicit empty identities.
- **(h) Forward pass**: Kind-III Rete cascade (event-driven, recurrence across ticks).
- **(i) Weight sharing**: Rooted categories with lazy untying.
- **(j) Structure learning**: Dynamic creation/pruning of Relations constrained by Formation cycle rules.
- **(k) Gradient flow training**: `Flow` `dw/dt = −η · ∂L/∂w` with `CurveModel` learning rate schedules.
- **(l) Checkpoint-free history**: Irreversibility mapping via `ONTOMATH_FRAMEWORK §6`.

## Immediate Next Steps
- Migrate hardcoded Hebbian constants (+0.2 reinforcement, -0.02/s decay in `LanguageSystem.cpp`) into authorable Law `Flow` and `Set`/`Add` rules.

## Scale Addendum — massive models on the substrate (2026-08-16)
- The iteration bounds are NOT the scale blocker: the 8-round cascade paces across ticks (`Law.cpp:1990` re-marks `_dirty` when the agenda is non-empty — nothing is dropped, depth converts to latency), and bound changes are safe as raise-only versioned spec changes (raising keeps everything defined; lowering breaks authored worlds).
- The real blockers for frontier-scale parameter counts: per-Relation representation cost (~10²–10³ bytes/weight vs 2 for bf16), CPU production-system compute vs dense GPU matmul, and symbolic expression swell under deep composition (exact derivatives do not survive 100-layer chains; autograd's point-evaluation is the correct tool there).
- The in-doctrine answer: change the ontological granularity. A single weight among 10¹² fails the "could a Person mean something by changing this?" test, so the weight blob belongs beneath the Kernel as a NAMED exemption; the tensor engine is a modality channel under `Singularity/Foreign/` on the SdfWgsl precedent — it compiles authored specs (loss as `ScalarForm`, LR schedule as `CurveModel`, architecture as Formation of categories, dataset admission as law) to a tensor program, executes on foreign hardware, and reports back through registered properties. The channel never decides what the model is.
- The beings at that scale are what carries meaning: datasets, provenance relations, checkpoints with authorship, evals, the law-governed training run itself. Refusal 5 already names the in-world shape: an AI is an Object, never a Person.
- Native capability scaling stays in-ontology: rooted-category weight sharing (parameter count ≠ being count) and guard-routed mixture-of-experts — large through structure, not count.
- Thesis: Earthcall is not the training engine for a GPT; it is the governance/provenance layer a GPT trains under.
