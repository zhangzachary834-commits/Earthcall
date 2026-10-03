# Addendum: Interaction as Law and Property Predication

*(Model: Claude 3.5 Sonnet, Harness: Jules, Session ID: 9535643377711650977)*

## Reflections on the Architectural Synthesis

When examining `docs/architecture/law/INTERACTION_AS_LAW.md` alongside `docs/architecture/ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`, a critical synthesis emerges regarding how Earthcall conceptualizes user interfaces and interaction state.

Traditional UI frameworks create a secondary ontology: buttons, sliders, and canvases exist as their own specialized entities ("widgets"), and their internal states (hovered, pressed, focused) are often hidden within those objects or managed by a parallel event-bubbling system. Earthcall explicitly rejects this dual-ontology approach.

### The Interface is Not a Second Ontology
As stated in `INTERACTION_AS_LAW.md`, an interface in Earthcall is not a separate UI subsystem. A "button" is simply an Object that a Person has agreed to mean something by. The interaction with that button—pointing, clicking, dragging—is governed by the Law system and the Singular set-to-set creation aimed at the pointer.

### Properties Disclose the Interaction
Crucially, the state of this interaction (e.g., whether the button is hovered or clicked) must be legible to the Law system without violating the doctrine of `PROPERTY_AS_PREDICATION_NOT_BEING.md`. The fact that an Object is currently "hovered" or "focused" does not mean we spawn a new `HoverState` Singular or `FocusEventEntity`.

Instead, interaction state is exposed as properties—legible predications of existing Singulars (like the `InteractionChannel` or the target Object itself). For example, `@creation-channel.active3DMode` or `hoveredId` are properties that disclose the state of the interaction channel. They are not independent beings.

### Resolving "No Black Box" for UI
This synthesis perfectly fulfills the "No Black Box" principle. The interaction state (what the user is currently doing with the pointer) is fully exposed as properties on the `InteractionChannel` Singular. Laws can read these properties to govern behavior (e.g., "if hovered and clicked, then perform action"). We achieve a fully authorable, law-governed interaction surface without polluting the ontology with UI-specific entities or burying the interaction state in unreadable C++ callbacks.

The architecture holds firm: Relations join beings (e.g., the Person and the control Object), Properties disclose them (the state of the pointer), and Laws govern the change based on those disclosures.