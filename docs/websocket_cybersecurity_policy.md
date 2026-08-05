# Earthcall Cybersecurity Policy: WebSocket & Client Ingestion

As Earthcall transitions to a model where arbitrary textual/symbolic input is ingested into the core simulation graph (Phase 4 & 5), the engine is exposed to external clients via WebSockets and WASM bindings. 

Because `Lexeme`s physically instantiate in the graph and `Law`s can dynamically execute logic based on them, a maliciously crafted payload could severely compromise the engine, cause denial of service, or hijack `OntoMath` evaluations.

## 1. Input Sanitization & Boundary Validation
- **Length Limits**: All incoming `Utterance` payloads must be strictly bounded. A malicious client sending a 10MB string to instantiate a `Lexeme` will OOM the server or crash the `LanguageSystem`. (e.g., Max `Lexeme` symbol length = 256 bytes).
- **String Sanitization**: Reject null bytes (`\0`) and unprintable control characters inside the `Utterance` payload. The `symbol` string must be strictly valid UTF-8.
- **Type Checking**: The WebSocket JSON parser must strictly enforce types. A payload like `{"type": "utterance", "payload": {"nested": "attack"}}` must be rejected immediately to prevent buffer overflows or JSON parser crashes.

## 2. Rate Limiting (Anti-Spam / Anti-DDoS)
- **Instantiation Cost**: Spawning a `Lexeme` entity has a CPU and memory cost. A malicious client could send 10,000 utterances per second to freeze the physics/rendering loop.
- **Policy**: The WebSocket server must enforce a strict rate limit per connection IP (e.g., max 10 utterances per second). Violators must have their WebSocket connection aggressively dropped.

## 3. Law Injection Mitigation
- **Data vs. Execution**: A `Lexeme`'s symbol is strictly *data*. A user must not be able to send an utterance like `Joy"; DROP WORLD;` and have the `LanguageSystem` or `Law` evaluator execute it as code.
- **OntoMath Sandboxing**: If we eventually allow clients to send JSON payloads that define new `Law`s (as proposed in Phase 7 Semantic Generation), the `ConditionNode` and `ActionNode` parsers must guarantee that unauthorized math bindings cannot access restricted `Singular` properties (e.g., a user shouldn't be able to map a math node to the Server's memory allocator or overwrite another player's coordinates directly unless the `Law` explicitly permits it).

## 4. Connection Authentication
- Currently, `ws://localhost:8080` allows anonymous local connections. If Earthcall is deployed to the public web as a multiplayer server, the WebSocket upgrade request (WSS) must include a valid authentication token (e.g., JWT) to link the connection to a specific `Person` entity in the graph. Unauthenticated connections should only be allowed to spectate, not emit `Lexeme`s.

> [!IMPORTANT]
> The unified `EventBus` ingestion layer allows us to enforce these security constraints in **one central place**. Whether the string comes from a remote WebSocket or local WASM JS, it must pass through the `SecurityValidator` before being wrapped in an `Event::Utterance`.
