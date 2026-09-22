# Contextual Language and Semantic Binding

**Author:** OpenCode (GPT-6 Astra; `openai/gpt-6-astra`)  
**Session:** `language-depth-20260910-115436` (local label; harness UUID unavailable)  
**Date / evidence timestamp:** 2026-09-10 12:00:26 PDT  
**Status:** requested branch research complete; proposed implementation work remains open.

## Origin and record

Zach requested one deep branch investigation with large-scale suggestions beyond the other models' existing recommendations. This task covers Language → Lexeme → Relation.

The human direction is Zach's steerable symbolic ecosystem, stakeholder-owned mutable meanings, and Terminal as Formations of Lexemes. The [audit](../../../../audits/LANGUAGE_CONTEXT_AND_SEMANTIC_BINDING_2026-09-10.md) identifies prior-model work and the exact extensions proposed here; introducing Utterance, AI Law synthesis, semantic decay, and generic multi-medium language are not claimed as new.

## Proposed workstreams

- [ ] **Context-preserving language acts:** retain input source/context/target/Moment and distinct source occurrences, with quotation, candidate interpretation, and operational effects distinguished by authored structure.
- [ ] **Scoped semantic linking:** preserve type-Lexeme identity through resolution, Relation equivalence, matching, and persistence; bind vocabulary editions and imports through stakeholder-authored policy.
- [ ] **Packed interpretations:** prototype a bounded generalized parser over authored grammar, with source spans, shared alternatives, visible completeness, and authored selection/commit behavior.
- [ ] **Bidirectional semantic editing:** establish identity-preserving round trips between one controlled authored language and its bound graph, retaining original wording separately from generated glosses.

These are research recommendations, not standing authorization to migrate Person-owned saves or settle the open semantic policy choices. Detailed rationale, mechanism, first exercises, scale dimensions, and dependencies are in audit §§5–7.

## Diagnostic work feeding those streams

- [ ] Preserve routing/source identity across queued input, including successful parses with an explicit target.
- [ ] Reconcile registered symbol writes with lookup keys and replace implicit last-interned selection with explicit scoped resolution.
- [ ] Reconcile equal-spelled type Lexemes with Relation IDs/deduplication and compatibility with existing string-tagged Laws.
- [ ] Distinguish repeated source spans and connect occurrences to the production ingress/lifetime path.
- [ ] Carry Lexeme authored state through the existing per-Zone persistence-root work; the current Zone codec emits only id/symbol.
- [ ] Distinguish authorable resource/residency policy from the current silent queue limits and removal of Lexemes at capacity.

The diagnostic probe reproduces ten observations; these bullets group their underlying mechanisms, not ten separate proposed subsystems.

## Verification performed

At revision `115b8e8b`, rebuilt `logos_modality_test` and `relation_retry_lexeme_test`; both passed. Linked and ran `scratch/probes/language_meaning_probe.cpp` against the same core using `scratch/probes/run_language_meaning_probe.py`; all ten current-behavior observations reproduced.

All three executables ran in a disposable temporary working directory with synthetic in-memory fixtures. No real save was loaded or injected. The codec observation tested emission in memory, not cold reload. Full command, fixture attribution, source references, and evidence limitations are in audit §4.

## First implementation milestone

Use one small authored language whose proposition can be asserted or quoted, whose referent can be ambiguous, and whose equal-spelled vocabulary definitions can be imported in opposite orders without changing a previously bound intention.

Future surface acceptance must include a Person judging whether selecting a phrase identifies the being they intended and whether text/graph edits preserve that intention; route those concrete checks to the Person Verification List when the surface exists.
