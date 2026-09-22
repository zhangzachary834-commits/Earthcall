# Interaction and The Second Person

**How Earthcall's refusal of private UI namespaces inevitably subjects every interface to the physical, shared rules of multiplayer presence and conflict.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/INTERACTION_AS_LAW.md` (UI as authored beings in-world)
*   `../ourverse/SECOND_PERSON_FRAMEWORK.md` (Multiplayer representation, visibility, and conflict)

---

## The Interrelation

In traditional software, UI is a private layer. When Player A opens an inventory or clicks a button, a 2D overlay is drawn on Player A's screen, in a private state space fundamentally distinct from the 3D world they inhabit.

Earthcall (`INTERACTION_AS_LAW.md`) explicitly destroys this boundary. Controls, panels, and buttons are not overlays; they are `Object`s, `Formation`s, and `Law`s authored in the world, subject to the exact same ontological physics as a tree or a stone. The interaction happens via the `InteractionChannel`, which fires rays into the world that intersect with these beings.

The profound consequence of this decision crystallizes when a second Person enters the world (`SECOND_PERSON_FRAMEWORK.md`).

Because UI is just a collection of in-world Beings managed by Laws, **UI is inherently a shared, multiplayer physical reality.** There is no such thing as "local UI" by default.

If Person A summons a control panel to adjust the volume of the world, that control panel is a real physical artifact in the Zone. Unless specific Laws of invisibility or non-interaction are authored to hide it, **Person B sees Person A's control panel.**

Furthermore, conflict resolution for UI interactions falls strictly under the Second Person Framework and `TransferPolicy` authority. If Person A and Person B grab the same spatial slider at the same moment, the engine doesn't need a custom UI-conflict-resolution subsystem. It relies entirely on the same physical and authoritative laws that govern what happens when two people grab the same sword.

**Conclusion:** Interface design in Earthcall is environmental design. UI is subjected to shared physics, spatial jurisdiction, and mutual visibility, forcing interfaces to behave as shared artifacts of human intention rather than private screen hallucinations.
