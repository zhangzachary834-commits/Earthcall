# Physics Events as Algorithmic Drivers in Continuous Space

*Origin: Jules task 3260200770766759045; reviewed by Sun Squid Intake II, 2026-10-26.*
**Status: conceptual interrelation, not a claim that all physics is event-sourced.**

[Physics as an Algorithmic Driver](../architecture/interrelations/PHYSICS_AS_ALGORITHMIC_DRIVER.md) describes how collision events can trigger Person-authored Laws and thereby advance algorithms in discrete steps. [Discrete Entities vs Continuous Space Scaling](../architecture/interrelations/DISCRETE_ENTITIES_VS_CONTINUOUS_SPACE_SCALING.md) distinguishes discrete beings from continuous mathematical/geometric domains and their different scaling costs.

## The boundary
A physics substrate may evolve continuous positions and detect collisions; the Event Bus can expose *selected* collision facts to authored Law. This does not mean every physical state transition is a Law-authored event, that every physics tick is a semantic Moment, or that the engine itself is a human `Person` or autonomous First Mover with Person authority.

A Person could author a collision-driven process whose Laws respond to those events. The engineering questions are which events are actually emitted, their ordering and identity, whether repeated contacts are coalesced, how physics and Law clocks interact, and which state changes are governed by TransferPolicy.

The synthesis is a **candidate design discipline**: preserve the distinction between continuous computation, discrete observations, and Person-authored semantic actions. Verify concrete behavior with EventBus/physics integration tests before asserting stronger guarantees.
