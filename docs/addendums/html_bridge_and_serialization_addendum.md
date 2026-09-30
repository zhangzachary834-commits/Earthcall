# Addendum: Integrating Foreign HTML Formations and Split-Substrate Serialization

*(Model: Jules, Harness: default, Session ID: 94817263590123485)*

## Reflections on the Architectural Synthesis

The [HTML Lexeme Formation Bridge](../architecture/Integration/HTML_LEXEME_FORMATION_BRIDGE.md) and [Lexeme Relation Formation Serialization](../architecture/Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md) meet at a specific boundary: **authored durable capture**. The HTML bridge's live DOM mirror is session state; the architecture does not require ordinary browsing or ingestion to serialize that mirror.

### Live Foreign State Is Not Automatically Durable State

The HTML Bridge can expose foreign DOM structure to Earthcall as legible Lexemes, Relations, and Formations without claiming that every observed node becomes persistent Earthcall state. Navigation, reload, or foreign mutation may rebuild that live representation.

Persistence begins only when a Person authors a capture that should survive the foreign session. At that point, potentially large repeated vocabularies and relation graphs make the split-substrate serialization architecture relevant.

### Split-Substrate Serialization at the Capture Boundary

For an authored durable capture:

1. **Lexeme identity can be compactly represented.** Repeated vocabulary can use the serialization architecture's symbol-table representation rather than duplicating strings throughout a durable graph.
2. **Relations can use the graph substrate.** Authored captured structure can be represented through the relation/formation serialization substrate rather than a monolithic JSON transcription of a live DOM.
3. **Matter remains distinct from semantic structure.** Durable raster or binary matter, when the authored capture actually includes it, belongs in the matter substrate rather than being confused with the semantic graph.

These are properties of the durable representation. They are not a requirement that live HTML ingestion itself be persisted, nor a claim that the proposed serialization architecture is already the implementation path of the HTML bridge.

### Conclusion

The HTML Bridge makes foreign structure legible. Split-substrate serialization becomes relevant when a Person deliberately turns some of that observed structure into durable Earthcall state. Keeping that boundary explicit preserves the bridge's refusal to confuse sensed foreign state with authored persistent meaning.
