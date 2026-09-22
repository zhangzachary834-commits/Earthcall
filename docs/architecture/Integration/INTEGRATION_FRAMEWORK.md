# External App Integration: Conceptual Architecture (Updated)

The manifesto outlines a profound capability: bringing external software (like a Calendar app or a CAD tool) into Earthcall, not as an embedded iframe, but by fundamentally translating its state and functionality into Earthcall's native primitives (`Singular`, `Object`, `Relation`, `Law`). Changes made within Earthcall are then serialized back to drive the original application.

Based on Earthcall's `NEW_KIND_FRAMEWORK.md` and `EarthcallOurverse.md`, here is a conceptual breakdown of how we can build this without violating the Five Refusals.

## 0. Integration vs Foreign — two names, two scopes

These are not synonyms and neither is a misnaming of the other:

- **Integration** (this docs directory) is the tentative name of the *whole conceptual
  system*: sensing an external app, projecting it into Zones, reconstructing its behavior,
  and syncing changes back. It spans the channel, the ML First Mover, the metalaws that
  vet what that First Mover proposes, and the authoring surface. It is a framework name,
  and it lives in `docs/` — the workshop, not the ontology.
- **Foreign** (`src/Singularity/Foreign/`) is narrower: the Singularity-level modality
  layer holding the hardwired connectors to external software. `ForeignChannel` stays
  at the modality root (the `PhysicalChannel` pattern). Protocol adapters live in
  `Adapters/`, the engine-facing surface in `API/`, the overlay/webview surface in
  `Web/`, and state-over-`dt` / sync-back / inference in `Sync/`.

So: there is **no `src/Integration/`** and there will not be one — a top-level directory
for a subsystem is refusal #2, and the C++ half of this system is already correctly placed
under `Singularity/`. There is likewise no `Singularity/Integration`; at the Singularity
level the name is Foreign. Integration names the idea, Foreign names the connectors.

## 1. The Channel (The only C++ allowed)

The external application is a foreign device. We must not create `src/CalendarApp/`. Instead, we create a modality channel under `Singularity/Foreign`.

**Path**: `src/Singularity/Foreign/ForeignChannel.hpp`
- **Sense**: Reads the external application's state via OS Accessibility APIs, DOM inspection (for web apps), or native application APIs.
- **Act**: Executes commands back to the external application (e.g., synthesizing clicks, API calls, or AppleScript commands).
- **Governed**: The `PhysicalChannel` pattern is used here — *not* `PhysicsLawBridge`. Both
  are first-mover Laws, but only `PhysicalChannel` (`src/Singularity/Physical/PhysicalChannel.hpp:23`)
  overrides `getIdentifier()` with a stable slug, and that override is the whole addressing
  contract: `resolveBeingToken` (`ActionModel.cpp:196`) matches an `@token` against
  `getIdentifier()`, and a first mover's generated `law-<N>` id differs every run because
  first movers are excluded from the save. Laws govern the channel via a stable identifier
  like `@foreign-channel.calendar.enabled`, which resolves only because `ForeignChannel`
  overrides `getIdentifier()` to return `"foreign-channel." + _appName`.

## 2. Translating State & Mapping Granularity

When the `ForeignChannel` senses the external app, it projects it into Earthcall using the **Composition Ladder** (K0-K3). 

**Mapping Granularity**: The raw incoming data tree is typically extremely noisy. We resolve this by supporting **both** granularities:
1. **Isolated Singularity-Level Zone**: The raw, unpruned OS/Accessibility tree is initially dumped into its own newly generated `Zone`, separate from the Person's main Zone.
2. **Authorable Alternative**: This isolated Zone is then integrated into the main Zone using explicitly authored `Relation`s and K4 `Law`s that filter, prune, and surface only the meaningful `Object`s to the Person.

## 3. Reconstructing Behavior (Law)

This is the hardest part mentioned in the manifesto: transferring functionality, not just pixels. An external app's internal logic is opaque. 

### A. Data Over Time (`dt`) and Async Logging
Rather than simple "before-and-after" snapshots, the channel asynchronously logs continuous graphs and tables of data over `dt`. 
To avoid **Sync Clashes**, the ML system does *not* operate in immediate-mode. The state changes are logged asynchronously, and the ML Formation processes this packaged data *after the fact* to understand the behavior as a whole.

### B. Person-Authored ML Formations
We cannot rely entirely on rigid, hardcoded first-mover AI that only knows categories from generic pretraining (e.g., classifying things merely as "cat", "dog", "button"). 
- The ML processors must be able to natively categorize into **all Earthcall primitives**: `Singular`, `Category`, `Person`, `Relationship`, `Community`, `Relation`, `Formation`, `Body`, `BodyPart`, `Law`, `Object`, `Zone`, `Home`, `Community Zone`, `Community Home`, `Ourverse`, and `Lexeme`.
- The architecture must remain open for **Person-authored Formation ML models**, allowing Persons to build, train, and utilize engine-native ML approaches rather than relying solely on black-box external API weights.

### C. Law Generation via Metalaws
When the ML Formation identifies a behavioral pattern (e.g., clicking "Next Month" changes the calendar grid), the AI does **not** directly mint new K4 Laws out of thin air ("AI cannot be pope").
Instead, the ML Formation **appeals to Person-authored Metalaws**. These Metalaws—authored by Zach or other Persons—dictate exactly how new K4 Laws are created, modified, or governed depending on the scenario, preserving full human authorship and architectural accountability.

## 4. Modifying and Syncing Back (Act)

When the user interacts with this reconstructed app inside Earthcall (e.g., dragging a calendar event `Object`):
1. **Earthcall native change**: The K3 K4 structures (like `Relation{type: "located-at"}`) are updated.
2. **Channel Output**: The `ForeignChannel` subscribes to these specific events and translates the property/relation change back into the adapter's protocol.
3. **External execution**: The adapter executes a macOS Accessibility drag-and-drop, or an API `PATCH` request, modifying the external app's actual state.
