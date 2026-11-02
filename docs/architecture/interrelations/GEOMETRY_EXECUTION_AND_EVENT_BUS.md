# Geometry Execution and Event Bus

## Source documents
- [Geometry Execution Substrate Manifesto](../mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md)
- [Physics EventBus Integration](../events/PHYSICS_EVENTBUS_INTEGRATION.md)
- [Physics and Collision](../events/PHYSICS_AND_COLLISION.md)

## The interrelation

The geometry manifesto distinguishes **authored ontology**, **authored mathematical form**, **geometry execution IR**, and **execution backends**. Collision algorithms (including GJK/EPA where applicable) compute physical contacts; their representation tags do not define a being's authored category or meaning.

Physics can publish `PhysicsCollisionEvent` notifications through the EventBus, and the collision architecture also describes `contact-began` / `contact-ended` edges and continuous `isTouching(other)` queries. These are related interfaces, **not interchangeable event names or a promise that every geometric overlap creates an authored Relation**. Contact detection and event publication belong to the sensing/actuation substrate; interpretation belongs to explicit consumers and Laws.

## What the EventBus does — and does not do

The EventBus distributes notifications to subscribed listeners. It is **not itself an ontological author, a permission check, or a universal translation layer**. An event may be logged, used for a sound or effect, consulted by an authored Law, or explicitly recorded in a Zone/Formation relation graph. The source documents show such relation recording as a listener/integration operation; the mere existence of a collision event does not prove that every listener, relation write, or authored Law ran.

The constitutional direction is one-way: authored meaning may lower into mathematical execution, while execution tags and incidental collision results must not silently be promoted into authored kinds or jurisdictional authority. When an event is to become durable world meaning, specify the responsible author/Law, the actual relation/property write, and its provenance. Test the concrete path from collision witness to event delivery to any claimed world mutation rather than inferring it from EventBus presence.
