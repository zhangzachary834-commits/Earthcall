# Integrating Relation Events and Timeline Moments

**Status:** Proposed bridge, not an automatic durable event-history feature.
**Connected documents:** [Relation Event System](../architecture/events/RELATION_EVENT_SYSTEM.md) and [Time and Moment](../architecture/ontology/TIME_AND_MOMENT.md).

## Two distinct event representations

When `RelationManager::add` accepts a **new** Relation, it publishes a `RelationCreatedEvent` with a Relation pointer and timestamp; it also emits the Law-facing ECA `relation-formed` echo. This is immediate EventBus publication, not a durable historical write. Updates of an existing Relation do not produce another creation event.

The temporal ontology separately defines `Timeline` and `Moment` as Singulars, with ontological `Event : Moment`. A C++ `RelationCreatedEvent` or ECA event is **not automatically** that ontological Event, and the EventBus does not by itself insert anything into a Timeline.

## Proposed authored history bridge

An explicit, authorized consumer could take a newly accepted Relation event, resolve the involved Singulars and the Timeline(s) lawfully in scope, construct an ontological Event/Moment with provenance, and persist it through a witnessed Timeline save path. No such automatic translation or durable insertion is claimed here.

The `RelationCreatedEvent` timestamp is a wall-clock value. A Timeline can have an independent temporal domain, so the timestamp is **not** automatically the correct coordinate on either endpoint's Timeline. Any bridge must declare a clock mapping or refuse an ambiguous conversion; it must not silently assume a global time axis.

Likewise, both endpoints do not necessarily own Timelines, and an event on one Timeline does not automatically occur on another. A real bridge must establish authored ownership/reachability, consent and visibility, event identity and deduplication, ordering, and serialization/reload tests before claiming a permanent navigable history.

This is a possible use of the two architectures, not a feature already implemented by their coexistence.
