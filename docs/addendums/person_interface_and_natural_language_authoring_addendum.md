# Integrating Person Interface and Natural Language Law Authoring

**How the terminal-first approach to natural language authoring realizes the vision of the Person Interface as a physical, in-world surface rather than a product UI overlay.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../core/Person Interface and Experience.md` (The philosophy of UI as authored beings, rejecting the private UI overlay)
*   `../architecture/law/Natural Language Law Authoring.md` (The natural language mechanism for speaking Laws into existence)

---

## The Interrelation

The `Person Interface and Experience.md` document outlines a strict philosophical fork for Earthcall's UI: the surface should not be an artificial "ImGui chrome around the world," but rather "in-world... surface is itself beings — Objects, Formations, Lexemes — authored the same way everything else is." Earthcall refuses to hide the world behind a secondary namespace of UI menus.

The `Natural Language Law Authoring.md` system is the direct mechanical realization of this philosophy. By making the primary authoring surface a terminal where Persons type sentences (e.g., `called "Blend Glow" when clicked then Lerp <path: "@law-line-cube.glow", operand: 1, factor: 0.25>`), the system entirely bypasses the need for complex, hardcoded GUI overlays (like `CreatorSection`, `Mode3D`, or `Tool::Type` panels).

1. **Words as Physical Artifacts:** In the Natural Language authoring paradigm, a word is a `Lexeme` that denotes a `Law`. When a Person types into the terminal, they are not interacting with an abstract configuration file; they are physically manifesting `Lexeme`s into the Zone. The UI *is* the instantiation of physical beings.
2. **The Terminal as a Shared Reality:** Because the terminal output and the resulting Laws are registered in the world (such as on the "Law Line"), they obey the rules of multiplayer presence (`Second Person Framework`). If Person A speaks a Law into existence, the resulting `Lexeme`s and `ActionModel` modifications are visible and addressable parts of the shared environment, not hidden in Person A's local HUD.
3. **No Hidden State:** The creation notation (`Create <SingularKind, properties:...>`) uses exactly the same `PropertyPath` structures that govern the rest of the engine. There is no "Black Box" UI logic performing magic behind the scenes; the terminal command compiles into explicit `Set`, `AddElement`, or `Map` actions that manipulate the exposed, addressable state of the world.

Therefore, the Natural Language authoring system is not just a convenience feature; it is the ontological mechanism that allows Earthcall to finally discard the "ImGui chrome" and transition to a purely in-world, shared interface.

## Thoughts on Integration

*Model: Jules*
*Harness: default*
*Session ID: 11468276636518108894*

When looking at the Person Interface document, Zach notes that the ImGui "HUD" was always meant to be a temporary anchor—a "Hardcoded First Mover"—while the true, dynamic, in-world UI was being developed. The Natural Language Law Authoring document is the blueprint for that true UI.

The integration here is deeply satisfying because it solves the "UI problem" by refusing to build a UI. Instead of writing complex widget systems, Earthcall builds a compiler. You type a sentence, the sentence becomes a `Lexeme`, the `Lexeme` denotes a `Law`, and the `Law` alters the `Object`. This means the interface is completely bound by the physics and the `TransferPolicy` of the world. It completely dismantles the traditional paradigm where the UI is a god-like layer sitting outside the simulation. Here, to change the simulation, you have to stand inside it and speak its language. It makes authoring an environmental act, perfectly aligning with the mandate that "UI is inherently a shared, multiplayer physical reality."
