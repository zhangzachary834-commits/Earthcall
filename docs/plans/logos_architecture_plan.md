# Logos Modality & WebSockets Architecture Plan

The objective is to establish "Human Language" as a foundational modality within the `Singularity` substrate, and transition Earthcall to a WebSockets/WebGPU browser architecture. Rather than treating language processing as a separate NLP layer (the standard AI paradigm), we model words, symbols, and code as substrate-native `Singular` entities (`Lexeme`s). This instantiates the doctrine of the "Logos"—meaning is substrate-native.

## Phase 1: Singularity Language Modality (Completed)
- **`Lexeme` (`Singular`)**: Words exist as explicit physical entities within the simulation graph, possessing a `Formation` and a registered property vocabulary just like physical objects.
- **`LanguageSystem`**: A core engine subsystem responsible for tracking active `Lexeme`s in the universe.

## Phase 2: Stochastic OntoMath (Completed)
- **`ProbabilityForm` & `Stochastic` Nodes**: `OntoMath` handles scalar forms, piecewise functions, and now *stochastic* distributions (Gaussian, Uniform, Bernoulli) to evaluate relations probabilistically.

## Phase 3: The Logos Law Bridge (Completed)
- **`ConditionModel` & `ActionModel` Integration**: `Law`s can now filter for `Lexeme` entities. A user can author a Law that says: *When Lexeme "Joy" enters Zone X, apply a stochastic expansion field to its conceptual weight, and map that to the emission of nearby Objects.*

---

## Phase 4: Browser Interface & WebSocket Ingestion
We must replace the ImGui chat/input with an HTML5/JS input layer.
- **Action**: Build a lightweight HTML/JS frontend that captures user input (text or speech-to-text) and sends it over a WebSocket connection to the Earthcall engine.
- **Outcome**: The WebSocket receives the string "Joy" and triggers a system event: `Event::Utterance`.

## Phase 5: Phenomenological Instantiation (Spawning the Lexeme)
When a user emits a word (via text, speech, or otherwise), it must become a structural entity.
- **Action**: Write an engine handler that intercepts `Event::Utterance`, constructs a new `Lexeme` Singular (with symbol "Joy"), and assigns it to a `Zone` or `Formation` rather than hard-locking it to a 3D coordinate. 
- **Outcome**: "Joy" is now an entity in the graph. By being instantiated within a `Zone`, it inherits the medium of that Zone—whether that means manifesting in 3D space, appending to a 2D text buffer, or playing as an audio waveform.

## Phase 6: Multi-Medium Manifestation (WebGPU & Beyond)
Because `Lexeme`s aren't locked to 3D, their manifestation is medium-agnostic.
- **Action**: Update the WebGPU render pipeline (and other subsystems) to query `Lexeme` entities based on the `Zone` they inhabit. 
- **Outcome**: A `Lexeme` in a 3D spatial Zone renders as 3D SDF text or a particle aura, driven by its `conceptualWeight`. A `Lexeme` in a text-document Zone renders as a 2D font glyph. A `Lexeme` in an audio Zone is synthesized as sound.

## Phase 7: Semantic AI Generation (The Earthcall AI Bridge)
Hardcoding `Law`s for every word is impossible. The engine needs generative capacity.
- **Action**: Integrate a background AI service (LLM/embedding model). When a user speaks a new word, the AI determines its phenomenological properties.
- **Outcome**: The AI outputs Earthcall `Law`s and `Relation`s as JSON. It dynamically authors rules (e.g., "Melancholy" dampens the speed of nearby objects), acting as a co-author of the universe's physics.

## Phase 8: Multiplayer Synchronization (The WebSocket State)
Earthcall is a shared universe.
- **Action**: The WebSocket layer must serialize `Lexeme`s, `Relation`s, and dynamically generated `Law`s, broadcasting the differential state to all connected browser clients.
- **Outcome**: When User A says "Joy", the server calculates the stochastic pulse and broadcasts the graph delta, ensuring User B's WebGPU renderer sees the exact same physical manifestation.

---

## Verification Plan

1. **Browser Ingestion**: Send a raw string via WebSocket from a dummy HTML client and verify the C++ server receives `Event::Utterance`.
2. **Lexeme Spawning**: Assert that the C++ server creates a `Lexeme` entity with the correct `symbol` string at the emitting `Person`'s coordinates.
3. **WebGPU Rendering**: Verify the browser canvas draws the `Lexeme` (e.g., rendering SDF text or a designated debug shape) at the correct coordinates.
4. **Law Execution**: Author a JSON `Law` that maps "Joy" `conceptualWeight` to scale. Say "Joy", and verify the physical representation visibly pulses or grows on all connected browser clients.
