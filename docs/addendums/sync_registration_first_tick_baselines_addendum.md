# Integrating Sync Registration Initial Baselines and Defensive First Ticks

Model: Jules
Harness: Earthcall
Session ID: sess-001

In Earthcall, the transition from engine initialization to the first active simulation frame is a critical phase where systems are populated but have not yet received consistent sensory input. Two systems—the `FileWatcher` and the `InteractionChannel`—demonstrate interrelated defensive strategies to handle this initial state correctly during `syncRegister` and the first `tick()`, preventing false events and mathematical instability.

## Thought on Integration and Interrelation

The initialization phase of Earthcall components happens primarily during `EngineInit.cpp`, where various systems call their respective `syncRegister` methods. This phase establishes the laws and properties that govern the simulation, but it does so before the main runtime loop begins actively providing data.

For the `FileWatcher`, `FileWatcher::syncRegister` is responsible for registering laws and properties related to file system monitoring. A naive implementation might simply start watching directories. However, if the watcher starts blindly, the very first `tick()` during runtime would detect all pre-existing files on disk and erroneously broadcast a flurry of `file-created` events for files that were already there before the engine started. To prevent this, `watcher->rescanBaseline()` is explicitly invoked during `syncRegister`. This establishes a tracking baseline of the initial disk state. When the first runtime `tick()` occurs, it compares against this established baseline, ensuring that events are only triggered for genuine post-initialization file system modifications.

A parallel defensive strategy is required in the `InteractionChannel`, which handles window and pointer input. Before the windowing system fully spins up and provides valid dimensions, internal properties might hold uninitialized or zero states. When the first few events arrive, or when properties are observed, `windowWidth` and `windowHeight` might momentarily be zero. Because these dimensions are used to compute normalized pointer coordinates (`propPointerU()` and `propPointerV()` via division by `windowWidth`), a zero value would immediately cause a divide-by-zero exception, crashing the simulation on startup. To guard against this, `InteractionChannel` proactively clamps these values using `std::max(1, sense.windowWidth)` upon observation.

The interrelation between these mechanisms highlights a unified architectural philosophy: the system must not assume that the first tick represents a steady state. Whether it is suppressing false-positive events by establishing a historical baseline (`FileWatcher`) or preventing mathematical undefined behavior by clamping uninitialized dimensions (`InteractionChannel`), the engine is designed to smoothly traverse the volatile boundary between static initialization (`syncRegister`) and dynamic runtime (`tick`), ensuring stability from the very first frame.
