# Addendum: Integrating Event Bus and First Mover Substrate Reversal

*(Original synthesis: Claude 3.5 Sonnet / Jules, session 13284209740648546535. Correctness and status clarification: Sun Squid Intake III, 2026-10-27.)*

**Status: architectural direction, not a claim that the Event Bus already executes arbitrary algorithms as Laws.**

## Control flow as observable authored steps

[Algorithms as Law and the Event Bus](../architecture/interrelations/EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md) proposes decomposing an iterative algorithm into bounded Law steps. An event delivers the fact that a step occurred; a handler or Law evaluator, **not the bus itself**, determines the next state transition. The bus is a delivery boundary, not a hidden interpreter, scheduler, or substitute for authority checks.

[First Mover substrate reversal](../architecture/interrelations/SUBSTRATE_ISOMORPHISM_AND_FIRST_MOVER_REVERSAL.md) names a longer-term goal: progressively replacing hardcoded execution decisions with authored semantics and eventually compiling those semantics to suitable machine substrates. The Event Bus can make causal steps inspectable, but event routing alone does not prove that the C++ engine has been replaced or that an authored program is equivalent to its former native implementation.

## What must be proved before calling it a call stack

A loop split across events can still run forever, recursively enqueue unbounded work, or observe partially committed state. The proposed architecture therefore needs explicit execution budgets or yielding, deterministic ordering where required, reentrancy rules, causal provenance, authority and Law applicability checks, and defined failure/rollback boundaries. It must also account for First-Mover and foreign events rather than assuming every state mutation originates in a Law.

The desired invariant is not *every iteration emits an event, therefore it is safe*. It is *every published transition is attributable, governed, and bounded by an executable contract*. A future compiled substrate must preserve those contracts, not merely reproduce the same visible output.

**Engineering gate:** demonstrate a real authored multi-step algorithm, bounded progress, cancellation/failure handling, deterministic replay where promised, and parity with the reference semantics before treating this conceptual bridge as implemented.
