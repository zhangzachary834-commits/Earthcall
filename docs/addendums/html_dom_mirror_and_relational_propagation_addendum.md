# Addendum: Integrating Foreign HTML Formations and Relational Propagation

**AI Model:** Jules
**Harness:** Earthcall Development Harness
**Session ID:** 15111161792417756560

## Reflections on the Architectural Synthesis

When examining the HTML UI Bridge (`DomMirrorTranslator.cpp`) and the core `Formation_Systemic_Maxim.md` principles regarding Relational Propagation, an essential integration mechanism becomes clear: **Lexeme-Bound Relational Dissolution**.

### 1. Relational Propagation of Foreign State

Earthcall enforces that foreign state (like a Web browser's DOM) is not a black box, but must be mapped into native `Lexeme`s and `Relation`s. A DOM node is a Lexeme, its attribute is a Lexeme, and its value is a Lexeme.

However, translating an external state change (like an attribute removal via `DomDeltaKind::AttributeRemove`) into Earthcall is not a simple map lookup and deletion. It requires rigorous **Relational propagation**.

If the DOM removes an attribute, Earthcall must not just delete a string key from a hash map. It must recognize the systemic ripple.

### 2. Lexeme-Bound Resolution over Symbol Matching

As detailed in `DomMirrorTranslator`, when an attribute is removed, the engine must dissolve the connection gracefully:

```cpp
// Traverse formal kHasAttribute and kHasValue relation graph edges
// originating from nodeLexeme rather than matching member lexemes
// by surface string symbol
```

Earthcall resolves the change by traversing the established `Relation` graph. It finds the specific `kHasAttribute` relation edge originating from the node's Lexeme, and the corresponding `kHasValue` edge. It must then release both the attribute Lexeme and the value Lexeme from the node's Formation, the session relation vectors, and the overarching `LanguageSystem`.

### 3. Synthesis: Propagation as Graph Traversal

This mirrors the `Formation_Systemic_Maxim.md` requirement that changes must propagate. The foreign DOM state update is treated not as a flat string command, but as a catalyst for relational propagation. By resolving the DOM change through formal graph traversal rather than surface symbol matching, the engine ensures that the ripples of a UI change are fully accounted for, maintaining the absolute integrity of the underlying `LanguageSystem` and `Formation` structures, keeping the web interface entirely legible to the Earthcall ontology.

---
**Linked References:**
* [Foreign State as Native Ontology](../architecture/interrelations/FOREIGN_STATE_AS_NATIVE_ONTOLOGY.md)
* [Formation Systemic Maxim](../architecture/ontology/Formation_Systemic_Maxim.md)
* [No Black Box](../architecture/ontology/NO_BLACK_BOX.md)
