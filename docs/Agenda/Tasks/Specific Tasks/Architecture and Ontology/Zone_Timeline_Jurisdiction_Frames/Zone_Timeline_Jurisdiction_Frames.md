# Zone Timeline jurisdiction frames

**Status:** open — Zach author decision recorded 2026-10-13.  
**Origin:** o3's clock-seam audit in `docs/Analysis/CLOCK_POINTER_BODY_SEAMS_ANALYSIS_2026-10-13.md`, followed by Zach's verdict.

## Author decision

Earthcall does **not** reject invariant clocks. Hardware/executable clocks are legitimate First-Mover substrate facts, and a device-wide Ourverse may have an invariant Timeline corresponding closely to the machine clock. The refusal is against making that clock an invisible universal temporal ontology.

A Zone is a jurisdiction over beings operating together in one bounded continuum. Zonal Laws therefore need one shared **Zone Timeline evaluation frame** so predicates and changes involving multiple participating Singulars can be evaluated consistently. A Singular may own an authored Timeline of its own without sharing a pointer to the Zone Timeline; whenever its state participates in Zonal Law, its temporal meaning must be interpreted relative to the Zone's shared frame.

Cross-Zone comparison does not require one global clock unless an inter-Zone Relation or Law actually needs a common frame. At that seam, the Ourverse supplies the higher-order reference frame, ultimately grounded in the legible device/hardware clock.

The intended hierarchy is therefore:

```text
hardware / executable clock
        ↓ grounds
device-wide Ourverse Timeline
        ↓ supplies cross-Zone reference when needed
Zone Timeline / jurisdictional evaluation frame
        ↓ contextualizes Zonal Law
Singular-owned authored Timelines
```

This is semantic relativity, not pointer identity: two Singulars need not literally share a `Timeline*` for their laws to be evaluated in one Zone frame.

## Current tension

The engine still routes broad timing through `Engine::_worldTimeline`, `Universe::now()`, compatibility scalar `now/dt`, and default rendering bindings. Those paths are acceptable only insofar as they are explicit substrate/default bindings rather than the sole temporal frame every being is forced to inhabit.

## Acceptance

- The device/Ourverse clock is explicitly legible as a substrate-grounded Timeline rather than an unnamed universal scalar.
- Every Zone has an explicit temporal evaluation frame usable by Zonal Laws.
- Singular-owned Timelines remain independently authorable and do not require pointer identity with the Zone Timeline.
- Zonal Law can compare participants with distinct local Timelines by projecting/relating them to the Zone frame.
- Inter-Zone Law/Relation evaluation can resolve against an Ourverse-level frame without silently globalizing all local time.
- Legacy `Universe::now()` / scalar fallback paths are either routed through this hierarchy or clearly quarantined as compatibility substrate rather than ontology.
