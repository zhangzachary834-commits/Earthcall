# Derived-State Ledger and HTML Bridge

**Status:** Architectural relationship; native DOM mirror components exist, but ledger-driven subscription/synchronization is not implemented here.

**Read with:** [Derived-State Ledger](../law/DERIVED_STATE_LEDGER.md), [HTML Lexeme Formation Bridge](../Integration/HTML_LEXEME_FORMATION_BRIDGE.md), and [No Black Box](../ontology/NO_BLACK_BOX.md).

## Distinct responsibilities

The Derived-State Ledger inventories *specific* derived structures, their declared dependencies, invalidation or renewal rules, and regression witnesses. It is an audit discipline, **not** a single universal event broadcaster or an API that every cache automatically registers with. Some entries are rebuilt on demand rather than invalidated through a subscription.

The HTML Lexeme Formation Bridge's September specification describes a **proposed end-to-end** Sense/Act integration with a foreign browser document. Since then, native `DomMirrorBridge`, `DomMirrorTranslator`, and wire-protocol code have been implemented under `src/Singularity/Foreign/Web/`; focused `foreign_web_dom_bridge_test`, `foreign_web_dom_projection_test`, and `foreign_web_dom_protocol_test` exercise parts of that implementation. These tests do **not** by themselves establish end-to-end browser convergence or a Derived-State Ledger subscription. Browser-owned DOM truth remains distinct from Earthcall's native Lexeme/Relation/Formation mirror: a browser change must be observed and reconciled, and an Earthcall actuation request is not proof of browser acceptance.

## The useful interrelation

The existing mirror and any future browser integration can *apply the ledger's discipline* to each derived DOM projection:

1. Name the foreign observation, its revision or request token, and the Earthcall graph state derived from it.
2. Declare which source changes invalidate or replace that projection, including browser-side mutations, failed actuation, disconnection, and Zone/being identity changes.
3. Decide whether the implementation uses explicit change notifications, ordered mutation edges, or bounded resnapshotting; do not assume a global ledger event stream exists.
4. Reconcile acknowledged browser results back through Sense before treating them as current foreign truth.
5. Give each declared dependency a witness that deliberately changes it and verifies refresh or conservative refusal.

A targeted UI update is a possible *downstream optimization*, not an existing guarantee of the Derived-State Ledger. Neither zero polling nor perfect synchronization follows merely from placing the two documents together.

**Boundary:** Authored Laws govern the legible native mirror; the browser remains authoritative for the live DOM, and the Foreign adapter must not become an uninspectable second source of ontology.
