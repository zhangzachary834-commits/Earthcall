# Next Step: Phase 4 - Modifying and Syncing Back (Act)

We've handled sensing the external app and reconstructing its behavior. The final phase of our integration architecture is the **Act** phase: when a `Person` interacts with the reconstructed app *inside* Earthcall, how do we translate those native K3/K4 changes back into the original external application?

## Proposed Changes

We will tackle this by extending our existing adapter and scaffolding a Sync Manager that listens to Earthcall's native event system.

### 1. C++ Foreign Sync Manager

We need a system that subscribes to native Earthcall changes (like a Property update or an ECA event) and routes them back to the correct external channel.

#### [NEW] [ForeignSyncManager.hpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Sync/ForeignSyncManager.hpp)
#### [NEW] [ForeignSyncManager.cpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Sync/ForeignSyncManager.cpp)
- **Role**: Hooks into Earthcall's `Universe` and `LawManager`. It listens for specific ECA events (e.g., "object-dragged" or "button-clicked") that target `Object`s belonging to a `foreign-zone`.
- **Action**: When an event occurs, it translates the Earthcall identifier back into the external app's ID and delegates the action to the adapter.

### 2. Extending the Adapter (The "Act" methods)

We previously scaffolded the `MacOSAccessibilityAdapter` to *read* data. Now we need to scaffold the methods that *write* data back to the OS.

#### [MODIFY] [MacOSAccessibilityAdapter.hpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.hpp)
#### [MODIFY] [MacOSAccessibilityAdapter.cpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.cpp)
- **Changes**: Add reverse-translation methods like `executeClick(const std::string& osElementId)` and `executeMove(const std::string& osElementId, float newX, float newY)`.
- **Rule Enforcement**: These methods use the raw OS APIs (like `AXUIElementPerformAction`) to synthesize the interaction on the host machine, closing the loop.

## Verification Plan

- Verify the `ForeignSyncManager` properly subscribes to the event bus without holding domain state.
- Verify the Adapter's new methods cleanly separate the Earthcall identifiers from the raw OS identifiers.
