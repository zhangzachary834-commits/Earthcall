# Foreign State as Native Ontology

**How external applications are fully integrated into Earthcall's physics and laws, refusing black-box embedding in favor of total legibility and interaction.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../Integration/INTEGRATION_FRAMEWORK.md` (Bringing external software into Earthcall natively)
*   `../ontology/NO_BLACK_BOX.md` (Total legibility of state and refusal of hidden mechanisms)
*   `../law/INTERACTION_AS_LAW.md` (UI and interaction driven purely by in-world Law)

---

## The Interrelation

When translating an external application (like a calendar or CAD tool) into Earthcall, conventional architectures would rely on an `iframe`, a webview, or a black-box container. Earthcall's architecture violently refuses this.

According to the `INTEGRATION_FRAMEWORK.md` and `NO_BLACK_BOX.md` (The Sixth Refusal), external state must be ingested and reconstructed entirely as native Earthcall primitives: `Singular`, `Object`, `Relation`, and `Law`. If an external app has a "button" or a "calendar event", it does not render as an opaque rectangle of pixels; it is instantiated as a native `Object` with registered, governable properties (like spatial bounds and text).

Because the foreign state is mapped into this fully legible ontology, it inherently participates in **Interaction as Law**. The same `shape-generator-3d-law` and pointer interaction paradigms that allow a Person to click an Earthcall button apply identically to the foreign "calendar event" object.

There is no "Foreign Interaction Subsystem." Because the external app's state obeys the No Black Box rule, it sits naked before the `EventBus` and the `Zone`'s Rete network. When a Person clicks the foreign object, an in-world interaction Law evaluates it, mutates its properties, and only then does the `ForeignChannel` (via `Sync`) serialize that change back out to the external world. The external app becomes native physics.
