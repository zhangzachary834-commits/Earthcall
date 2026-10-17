# Integrating WebSocket Cybersecurity Policy and Natural Language Law Authoring

**How exposing the engine to external clients through WebSockets necessitates rigorous boundaries when parsing and compiling natural language Laws.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../policies/websocket_cybersecurity_policy.md` (Security constraints for remote client interactions)
*   `../architecture/law/Natural Language Law Authoring.md` (The terminal-first, natural language mechanism for authoring Laws)

---

## The Interrelation

The creation of the Natural Language Law Authoring system allows Persons to speak Laws into existence using text parsed in a terminal (`TerminalChannel`). As Earthcall moves into phases where it is exposed as a multiplayer server to the public web (Phase 4 & 5), this Natural Language channel inherently intersects with the WebSocket ingestion pathways defined in the Cybersecurity Policy.

Because Earthcall permits natural language utterances to be compiled into operational Law nodes (e.g., `Set`, `Lerp`, `Map`, `AuthorZone`), there exists a profound risk of Law Injection if an external WebSocket client sends maliciously crafted strings. The "No Black Box" principle requires everything to be legible and authored, meaning a string utterance like `called "Rogue Law" when clicked then Set @server.allocator to 0;` could potentially be interpreted as an executable `ActionNode` if not properly constrained.

The integration of these systems resolves the vulnerability through strict boundaries:

1. **The EventBus Ingestion Layer:** The `SecurityValidator` intercepts all WebSocket string payloads before they are wrapped in an `Event::Utterance`. It enforces UTF-8 validity, rejects unprintable control characters, and applies strict payload length limits, ensuring buffer overflows or parser crashes cannot occur during natural language analysis.
2. **Data vs. Execution in the LanguageSystem:** A `Lexeme` generated from a remote utterance remains strictly *data* until a compiler (like `law-line-compile-sentences`) explicitly maps it to an authorized Action form.
3. **OntoMath and TransferPolicy Sandboxing:** Even when a valid natural language sentence compiles into an `OntoMath` evaluation or a `Set` action, the `TransferPolicy` and the second-person authority bounds ensure that unauthorized math bindings or property mutations cannot occur. An unauthenticated or unauthorized connection's utterance might successfully compile into a Law, but that Law will simply be refused execution when it attempts to modify restricted Singularity properties.

This interrelation ensures that the terminal's expressive power remains intact without converting every open WebSocket into an unfettered command injection vector. The cybersecurity policy acts as the physical, limiting substrate over which the natural language authoring system can safely operate.

## Thoughts on Integration

*Model: Jules*
*Harness: default*
*Session ID: 11468276636518108894*

When reviewing these two documents, the immediate concern is the intersection of "expressive, text-based code generation" and "untrusted remote clients." Earthcall's decision to treat UI and interaction as physical phenomena in-world means that typing a sentence into a terminal is essentially throwing a physical `Lexeme` object into the engine.

If you don't secure the ingestion point, the Natural Language authoring system becomes the ultimate attack vector. A malicious user could just spam `Create <SingularKind>` sentences to cause an OOM, or try to use the `Set` action to overwrite core system paths. The beauty of Earthcall's architecture is that the solution isn't to dumb down the natural language parser. Instead, the solution relies on the existing physical properties of the engine: the `SecurityValidator` handles the gross physics of the string (length, byte validity), the `LanguageSystem` processes it as an inert object, and the `TransferPolicy` acts as the physical law that stops a rogue `Set` action from touching something it shouldn't. The cybersecurity rules aren't a separate, bolted-on framework; they are the literal physics preventing a bad actor from throwing an explosive `Lexeme` into the world.
