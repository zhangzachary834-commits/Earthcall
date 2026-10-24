# Addendum: Integrating HTML DOM Mirror Translation, Relation Lifecycle, and Language System Lexemes

## Snapshot projection into native relations

The [DomMirrorTranslator](../../src/Singularity/Foreign/Web/DomMirrorTranslator.cpp) admits a validated DOM snapshot, interns node and attribute/value Lexemes in `LanguageSystem`, and constructs node Formations. Each initial attribute is represented by directed `kHasAttribute` (node → attribute) and `kHasValue` (attribute → value) Relations. The translator also tracks session Lexemes and Relations for eventual retirement.

## Delta removal is narrower than session retirement

`DomDeltaKind::AttributeRemove` currently finds the node Formation, searches its members for a Lexeme whose **symbol** matches the removed attribute name, and calls `releaseMember` on that one member. It does **not** traverse the `kHasAttribute`/`kHasValue` edges to disambiguate an attribute, remove both Relations, release the value member, or remove the attribute/value Lexemes from `LanguageSystem` at that point.

Consequently, a description of attribute removal as a complete relational teardown would be incorrect for the current implementation. The symbol-based lookup and surviving relation/session records are a **lifecycle consistency gap** worth addressing with a focused test and implementation change; this addendum documents the current behavior rather than claiming the gap is already closed.

At the **whole-session** boundary, `retire()` clears the document and node Formations, clears relation collections, and removes tracked session Lexemes from `LanguageSystem`. That retirement path is distinct from a single attribute-removal delta.

## Verification anchors

- [DomMirrorTranslator.cpp](../../src/Singularity/Foreign/Web/DomMirrorTranslator.cpp): `registerNode`, `applyDelta` / `AttributeRemove`, and `retire`.
- [foreign_web_dom_projection_test.cpp](../../tests/singularity/foreign_web_dom_projection_test.cpp): DOM projection test coverage; do not mistake snapshot coverage for a proven full attribute-removal lifecycle.
