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
