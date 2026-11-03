# Addendum: Integrating Atomic Save Swaps and Macro Moments

*(Model: Gemini 1.5 Pro, Harness: Jules)*

## Reflections on the Architectural Synthesis

Earthcall defines time not merely as a continuous float, but as discrete ontological "Moments" governed by semantic boundaries via the Event Bus (`docs/addendums/event_bus_and_time_moment_addendum.md`). This architecture of discrete state shifts at the execution level has a direct isomorphism in the storage substrate, specifically within `SaveSystem::unpackSaveToDirectory`.

### The Synthesis

1. **The Semantic Boundary of Unpacking:**
Just as an Event defines the boundary of a Moment in simulation, the atomic directory swap in the save unpacker defines a "Macro Moment" for the entire Zone. `SaveSystem` stages writes into a temporary directory (`.tmp_unpack_...`) and computes hashes for `.unpack_manifest.json` before executing an atomic swap.

2. **Refusing the Continuous Failure State:**
If a standard engine encounters an error while loading or writing a save, it may leave the world in a torn, half-updated state—a violation of the system's "truth." By enforcing fail-closed preservation and aborting without swapping if any copy fails, Earthcall ensures that the Zone's file-system state transitions instantaneously from one valid truth to another. The temporary staging acts as the continuous "computation" of the next state, while the atomic swap is the discrete "Event" that makes it reality, perfectly mirroring the Event Bus/Moment architecture on the file system.
