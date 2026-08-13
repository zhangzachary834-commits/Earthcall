# Next Step: Adapters and State Prototyping

Now that the core `ForeignChannel` class is stubbed out, the next step in our `INTEGRATION_FRAMEWORK` is to handle how raw data actually enters the system and what it looks like once it's there. 

## Proposed Changes

We will tackle this in two parts: C++ adapter scaffolding and JSON state prototyping.

### 1. C++ Adapter Scaffolding

We need to build an adapter that lives *behind* the channel. This adapter handles the specific wire protocol (e.g., macOS Accessibility APIs) and translates it into Earthcall's shared vocabulary (Property writes, Relations, ECA events).

#### [NEW] [MacOSAccessibilityAdapter.hpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.hpp)
#### [NEW] [MacOSAccessibilityAdapter.cpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.cpp)
- **Role**: Periodically polls or listens to macOS Accessibility events (like window changes or button clicks).
- **Rule Enforcement**: As mandated by the `NEW_KIND_FRAMEWORK`, this adapter will *not* be included by anything outside the `Foreign/` folder. It translates raw OS data into Earthcall primitives and feeds them to the engine.

### 2. JSON State Prototype

To see how the dual-granularity mapping actually works in practice, we need to prototype what the raw dump of an external app looks like when it becomes a `Zone` in Earthcall.

#### [NEW] [foreign_zone_dump.json](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/saves/fixtures/foreign_zone_dump.json)
- **Content**: A mock JSON file showing how a Calendar app's UI tree translates into Earthcall's K0-K3 structures.
- **Structure**: 
  - An isolated `Zone`.
  - A `Formation` representing the view hierarchy.
  - `Object`s representing individual UI elements (buttons, grids).
  - `Relation`s mapping the parent-child relationships and geometric positions.

## Verification Plan

- Verify that the C++ adapter files follow the architectural rules (no leaked includes, no domain-specific structs).
- Verify that the JSON prototype accurately uses Earthcall's native primitives (`Singular`, `Relation`, `Formation`, `Zone`) without inventing new categories.
