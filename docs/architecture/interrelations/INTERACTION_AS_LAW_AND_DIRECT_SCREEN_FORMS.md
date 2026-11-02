# Interaction as Law and Direct Screen Forms

**Status:** Conceptual interrelation between two distinct modality seams.

## Interrelated Documents
- [Interaction as Law](../law/INTERACTION_AS_LAW.md)
- [Direct Screen Forms](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md)

## Boundary and connection
Interaction-as-Law treats pointer, wheel, keyboard, and focus **sensing** as engine-owned First Mover work. The InteractionChannel publishes observations that authored control Laws can condition on or react to. This is not a promise that all legacy UI chrome has been replaced; the interaction specification explicitly retains work to migrate existing controls and calls out an unrun manual interaction protocol.

Direct Screen Forms supply a separate **manifestation** path: `ScreenChannel::manifestOutput` samples authored OntoMath color/opacity expressions at physical framebuffer pixel centers. The authored form need not be an Object, Mesh, or Material; the fullscreen dispatch triangle is implementation machinery, not a new visual being. Compatibility HUD/tools still draw afterward.

Together these seams could support Law-governed interaction with direct screen output, but **drawing a pixel does not automatically create a pointer target, control Relation, or event-to-Law binding**. Any such integration must explicitly author the state, sensing, and Law connections and verify them. The direct screen channel currently admits one composed output expression, not a general-purpose multi-widget ownership system.
