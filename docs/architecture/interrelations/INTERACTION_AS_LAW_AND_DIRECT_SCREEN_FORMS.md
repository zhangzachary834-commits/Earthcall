# Interaction-as-Law and Direct Screen Forms

**How Earthcall unites pointer inputs with non-physical UI rendering.**

## Interrelated Documents
- `docs/architecture/law/INTERACTION_AS_LAW.md`: Details how Earthcall handles UI inputs by rejecting traditional widgets in favor of pointer events interacting directly with Laws.
- `src/Singularity/Screen/ScreenChannel.cpp`: The implementation of "Direct Screen Forms," evaluating OntoMath expressions at framebuffer sample centers to render interfaces without physical Objects or Meshes.

## Thoughts on Integration and Interrelation

Earthcall's user interface architecture is defined by the rejection of traditional UI paradigms, both in how input is processed and how graphics are rendered. The interrelation between "Interaction-as-Law" and "Direct Screen Forms" is the complete cycle of this rejection.

"Interaction-as-Law" dictates that there are no retained UI elements like buttons or sliders to receive events. Instead, pointer events (like clicks or sweeps) are routed into input channels. Laws listen to these channels and react accordingly. When a user interacts with a UI, they are actually actuating a Law that observes their pointer's state.

"Direct Screen Forms," implemented via `ScreenChannel.cpp`, is the visual counterpart. Just as there are no logical UI objects to receive clicks, there are no physical Objects or Meshes to be drawn. The UI is rendered by evaluating OntoMath expressions directly at the sample centers of the framebuffer. The visual form of the interface is purely derived from mathematical fields.

The synthesis of these two systems creates Earthcall's UI paradigm:
1. **Input:** The user moves the pointer, updating registered pointer state (No Black Box).
2. **Logic:** A Law (Interaction-as-Law) observes this state change and executes its logic, often updating an OntoMath field.
3. **Output:** The `ScreenChannel` (Direct Screen Forms) samples the modified OntoMath field at the framebuffer pixel centers, rendering the resulting visual change to the screen.

Together, they allow for complex, interactive graphical interfaces that exist entirely as mathematical fields mediated by Laws, free from the constraints and assumptions of traditional widget hierarchies and object meshes.

---
*Provenance: Authored by Gemini during Earthcall Harness session Unknown.*
