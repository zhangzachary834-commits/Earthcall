# Addendum: Zone Timeline Jurisdiction and the Moment Execution Weave — Proposed Integration

*Origin: Jules architectural synthesis; Sun Squid Intake accuracy reconciliation, 2026-10-26.*

**Status: architectural direction, not an implemented cross-Timeline scheduler.** [Zone Timeline Jurisdiction Frames](../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Zone_Timeline_Jurisdiction_Frames/Zone_Timeline_Jurisdiction_Frames.md) records an **open** author decision. [Timeline and Moment Execution Weave](../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Timeline_Moment_Execution_Weave/Timeline_Moment_Execution_Weave.md) explicitly states **execution unimplemented**.

## Two complementary responsibilities

**Jurisdictional frame:** Zonal Laws need a shared temporal evaluation frame when comparing participants with independently authored Timelines. The Singulars need not share a `Timeline*`. Inter-Zone comparison may instead require an Ourverse reference frame. Hardware and executable clocks remain legitimate substrate facts; the refusal is to elevate one implicit global tick into the only possible temporal ontology.

**Moment execution weave:** Independently grounded occurrences need authored precedence, explicit output dependencies, contention rules, and continuation semantics. Ordering A before B is not equivalent to B consuming A's output. The proposed ready frontier must admit eligible work while retaining unresolved work without duplicating effects or clock advancement.

## Integration obligations before implementation claims

1. Define how each participating Timeline coordinate projects into the applicable Zone or Ourverse frame, including ambiguous or unavailable translations.
2. Represent three independently testable occurrence grounds: declared Timeline start, qualifying state transition, and clock-only boundary. Unchanged holding conditions must not repeatedly publish transitions.
3. Compile authored precedence and data dependencies, detect cycles, and preserve Law authority/consent guards and durable occurrence identity.
4. Witness two independent Timelines with authored `A₁ → B₁ → A₂ → C₁`; prove that B consumes the selected A output, unrelated D progresses when B waits, and changing authored order requires no C++ scheduling rewrite.
5. Separately verify observable native effects, persistence, cross-Zone reference selection, and safe continuation before declaring the runtime architecture complete.

## Boundary of the claim

Current `Engine::_worldTimeline`, `Universe::now()`, and compatibility `now/dt` paths remain in the engine. The jurisdiction proposal describes how they should become explicit substrate/default bindings, **not** that they have already been displaced. No current causal-trace, native-pixel, or production cross-Timeline execution witness is claimed by this addendum.
