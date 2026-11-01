# Interaction-as-Law and Direct Screen Forms

**How authored interaction Laws and direct mathematical screen output can meet without introducing a privileged UI ontology.**

## Interrelated Sources

- [Interaction as Law](../law/INTERACTION_AS_LAW.md): specifies and partly implements pointer/keyboard sensing, authored control beings, and Law-driven responses.
- [ScreenChannel.cpp](../../../src/Singularity/Screen/ScreenChannel.cpp): implements direct Screen output by resolving authored color/opacity/time fields and passing their typed AST definitions to the renderer.
- [No Black Box](../ontology/NO_BLACK_BOX.md): requires meaningful being state to be registered and addressable, distinguishing it from bounded machine mechanism.

## Two compatible forms, not one mandatory pipeline

**Interaction-as-Law** does not abolish authored Objects that act as controls. Its central example is *an Object the Person can point at, plus a Law that determines what pointing means*. The InteractionChannel senses the target and gesture, records registered levels, and publishes transition events; Laws decide what those observations mean and what to change. Control kinds and factories can be expressed through authored categories and ObjectConcept rather than a hardcoded `Button` class.

**Direct Screen Forms** offer another presentation route: `ScreenChannel::manifestOutput` resolves typed OntoMath color, opacity, and optional time fields and asks the renderer to draw a screen-space form. The form itself need not be a physical Object or Mesh. This is not a claim that every UI control is screen-space, or that `ScreenChannel` automatically receives pointer hits.

An authored integration **can** join these capabilities:

1. The input channel senses a pointer/key gesture and publishes an event or updates registered levels.
2. A Law, if authored and activated for that event or state, performs a change; that change *may* target the fields used by a direct Screen form.
3. If direct output is enabled and valid fields resolve, the Screen renderer evaluates the resulting form over its framebuffer domain.

This is a possible composition, **not an automatically wired end-to-end UI path**. Direct Screen forms still require an authored interaction mapping or sensing strategy; ordinary Object picking does not by itself provide screen-form hit testing. The `Interaction as Law` document also explicitly distinguishes implemented sensing/control patterns from its unrun manual UI protocol. Neither document establishes that the full proposed screen-form interaction experience has been validated in a window.

The architectural invariant is a division of labor: the engine senses and renders; authored Laws supply meaning and action. Neither input dispatch nor the screen renderer should silently decide what constitutes a button, consent, or a Person's intent.

---
*Provenance: Originally authored by Gemini during Earthcall Harness session Unknown; corrected against the cited canonical sources.*
