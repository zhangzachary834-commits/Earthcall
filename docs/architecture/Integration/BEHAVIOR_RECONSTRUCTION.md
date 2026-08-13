# Next Step: Behavior Reconstruction & Async Logging

The next major piece of our `INTEGRATION_FRAMEWORK` is Phase 3: Reconstructing the opaque internal logic of an external app. We must reconstruct its behavior by feeding continuous `dt` state logs to an ML classification system.

## Hybrid ML Approach

Based on the ontological constraints, we are utilizing a **hybrid approach** to machine learning:

1. **The External First Mover**: We will implement an external Python layer as a First Mover to perform the heavy-lifting classification of the async data logs.
2. **Exposed In-World Drivers**: The parameters that drive this Python First Mover (specific neural networks, classifiers, thresholds, etc.) will **not** be hidden. They will be exposed as legible, authorable `Properties` inside Earthcall. A `Person` can change, author, and govern these properties exactly like any other primitive.
3. **The Native "ML Formation"**: The ultimate goal is that a `Person` can author native `Formation`s and `Law`s (using Singular-set-to-set processes) that act as an ML system entirely within the engine's primitives (e.g., a neural network constructed out of `Relation`s).

## Proposed Changes

We will tackle this in three parts: C++ Async Logging, the Python First Mover Bridge, and the JSON State.

### 1. C++ Async State Logger

We need a lightweight buffer that captures the stream of OS accessibility events from our adapter and packages them over time (`dt`).

#### [NEW] [AsyncStateLogger.hpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/AsyncStateLogger.hpp)
#### [NEW] [AsyncStateLogger.cpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/AsyncStateLogger.cpp)
- **Role**: Sits beside the `MacOSAccessibilityAdapter`. As the adapter polls events, the logger buffers these state changes and flushes them to the Python First Mover.
- **Rule Enforcement**: Holds no domain logic. Strictly an IO buffer for the Foreign channel.

### 2. The ML First Mover Bridge (C++)

To expose the Python First Mover's ML parameters to the world, we need a bridge.

#### [NEW] [InferenceLawBridge.hpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/InferenceLawBridge.hpp)
#### [NEW] [InferenceLawBridge.cpp](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Foreign/InferenceLawBridge.cpp)
- **Role**: Functions similarly to the `PhysicsLawBridge`. It represents the external Python ML system as a first mover `Law`.
- **Properties**: Exposes properties like `model_type`, `confidence_threshold`, or `active_classifier`. Persons can write K4 Laws to govern these properties.

### 3. Metalaw & Behavior Synthesis Prototype

The Python First Mover reads the `dt` logs, but it must appeal to a Metalaw to propose K4 Laws for the external app.

#### [NEW] [metalaw_behavior_synthesis.json](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/saves/fixtures/metalaw_behavior_synthesis.json)
- **Content**: A mock JSON file showing a Metalaw authored by "Zach".
- **Structure**: Defines constraints on how the Python First Mover is allowed to generate K4 Laws for the Calendar App, preserving the "AI cannot be pope" rule.

## Verification Plan

- Verify the C++ `InferenceLawBridge` exposes the ML drivers as legible `Properties`.
- Verify the JSON Metalaw properly attributes authority to "Zach" (clamped to 0/Person authority).
