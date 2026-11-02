# Hover Events and Interaction as Law

## Interrelated Documents
- `docs/architecture/events/OBJECT_HOVER_EVENTS_SYSTEM.md`
- `docs/architecture/law/INTERACTION_AS_LAW.md`

## The Interrelation

The `OBJECT_HOVER_EVENTS_SYSTEM.md` details how the engine monitors the pointer's location and triggers state changes (or dispatches events) when the user's cursor intersects an Object. Crucially, the latest architectural update shifted this system away from merely publishing internal C++ events and instead exposed `hovered` and `hoverPoint` as **registered, read-only property paths** on `Object`.

`INTERACTION_AS_LAW.md` describes Earthcall's paradigm for UI and user engagement, which rejects traditional event-listener-based widgets in favor of pointers directly interacting with the continuous evaluation of Laws via `ScreenChannel`.

These two documents interrelate by illustrating how lower-level pointer logic is elevated into the ontological graph. Because hover states are now legible properties (`@world.pointerOver` or `object.hovered`), they can be consumed directly by the Interaction-as-Law system.

## Thoughts on Integration and Interrelation

The exposure of hover events as queryable properties is what bridges standard interactive input into the Law engine's domain. Rather than a developer writing a `onHoverEnter` C++ callback to change a button's color (creating a black box), an Author writes a Law: "When object.hovered is true, set object.material.emission to 1.0".

This integration means that UI interaction and in-world physical manipulation use the exact same evaluative substrate. The continuous space rasterized to the screen and the discrete bounds of a hovering pointer seamlessly feed facts into the Rete network. The engine's job is simply to keep `object.hovered` updated every frame; it is up to the authored Laws to decide what that spatial intersection actually *means* for the interactive experience.
