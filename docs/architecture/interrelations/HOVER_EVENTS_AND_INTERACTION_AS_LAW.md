# Hover Events and Interaction as Law

## Source documents
- [Object Hover Events System](../events/OBJECT_HOVER_EVENTS_SYSTEM.md)
- [Interaction as Law](../law/INTERACTION_AS_LAW.md)

## The interrelation

`InteractionChannel::observe`, stepped from `Engine::update`, performs pointer sensing and updates the hovered Object's state. The hover specification documents both C++ enter/hover/exit events and **registered, read-only** Object paths `hovered` and `hoverPoint`. It also recommends `@world.pointerOver` when the Law only needs to identify the current pointer target, avoiding redundant per-object state.

`INTERACTION_AS_LAW.md` separates **Sense** (engine input, hit testing, edges), **Decide** (authored `ConditionModel`), and **Act** (authored `ActionModel`). An Object is not automatically a button because the pointer hits it; the authored category and Law determine what that contact means. The input observer is `InteractionChannel`, not `ScreenChannel` acting as a universal pointer sensor.

## Two supported ways to respond

- **State/condition:** an authored Law reads `@world.pointerOver` or the relevant Object's `hovered` property and, when its conditions and authority permit, changes a registered, writable property.
- **Event/edge:** an authored `Activation::OnEvent` Law or other explicit subscriber reacts to a published input edge such as `object-clicked`; C++ hover enter/exit notifications remain available to subscribers where needed.

A conceptual hover-highlight rule is: *when this Object is the pointer target, apply an authorized visual-property change*. This is a design example, **not** a claim that an arbitrary `object.material.emission` path exists or is writable.

The engine senses and publishes; it does not decide that a hover must highlight, grant consent, or create a Relation. Nor does merely exposing a property guarantee that the Rete network reevaluates every such fact on every render frame. Verify the registered path, actual Law activation, and resulting mutation for each implementation. Rendering must not become a hidden input/authority loop.
