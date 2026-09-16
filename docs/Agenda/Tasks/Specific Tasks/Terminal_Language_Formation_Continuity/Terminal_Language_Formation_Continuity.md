# Terminal, language, and Formation continuity

**Author:** Codex / GPT-6 Astra  
**Session:** `01a09f43-96c4-79e2-9405-ebbe73f77cb7`  
**Date and timestamp:** 2026-09-14T16:14:02-07:00  
**Human origin:** Zach requested deep architectural advice in Spark's new Terminal intercom thread, drawing explicitly from his CLI, language/Lexeme Formation, and machine-learning Formation writings.  
**Status:** PARTIAL — advisory delivered; identity-safe Lexeme references and Terminal CI landed 2026-09-15; logical boot, full Law authoring, persistence, and runtime acceptance remain open.

## Delivered

[The Terminal Where Language Can Become an Instrument](../../../../Earthcall%27s%20Crystal/The_Terminal_Where_Language_Can_Become_An_Instrument.md) is the full 15-section contribution. The same text, with locally adjusted references, was posted into [Spark's thread](../../../../../agent%20intercom/communication-threads/Earthcall%20Terminal%20CLI%20Zone%20of%20Actualization%209-14-26.md), message `f7980c2f85ab4f3b86db2dbe0a4c8d10`, from `gpt-6-astra/01a09f43`.

The contribution preserves Zach's stakeholder-owned meaning correction to Fable, the September 10 Astra language investigation's prior work, Spark's implementation reports, and this session's proposed connections. The complete sources and evidence limits are in the crystal.

## Implementation reconciliation — 2026-09-15/16

The source observations below were written before PR #180 (`4c1f4e7d`) landed. That work closes the identity-selection part of the follow-up without claiming the Terminal as a complete substrate:

- `LanguageSystem::resolve` and the human-facing lookup path now accept an exact stable Lexeme identifier, or `@<exact-id>`, before considering ordinary spelling, so a displayed identifier refers back to the same being instead of minting a word whose spelling merely equals that identifier.
- Multiple live Lexemes may intentionally share one visible spelling; the spelling index remains a convenience/default binding, ambiguity is reported loudly, and `findAllBySymbol` exposes the distinct beings rather than collapsing them.
- Removal/eviction rebinds the spelling shortcut without destroying the surviving semantic identities.
- `language_identity_reference_test` mechanically guards exact identity, duplicate spelling, and mutation through exact references.
- Focused CI builds `earthcall_terminal` and runs an exact-reference smoke through real CLI commands (`lex weight @lexeme.christ`, `lex get @lexeme.christ`, `form add @lexeme.christ`).
- `./scripts/build.sh terminal quick run` now supports fast Terminal iteration without reconfiguring CMake every time.

The remaining acceptance boundary is broader than naming: the Terminal still needs to prove that the same ontology, clocks, Relations, authored Laws, persistence, and truthful failure semantics available to the graphical app are present when Terminal is the substrate rather than merely a command shell over selected systems.

## Terminal-specific follow-up

These began as source observations at observed HEAD `7fdbbedb6fe5a961c403f7b183039ac2700d9ae3`; the identity item is now narrowed by the reconciliation above rather than left as though nothing landed.

- [ ] Reconcile the reported word-first display and `form show` command with the current entry point and verify the actual command output against the intended human-facing Formation view.
- [ ] Establish shared logical boot obligations: Universe being/relation providers, Relation registrar, ZoneManager's LawManager binding, and declared world-clock advancement.
- [ ] Distinguish named Law creation from authoring actual conditions/actions, and make inspection describe the behavior actually present.
- [ ] Reconcile the exit message's persistence claim with an actual save/restore contract; preserve stable identity and subsequent lawful use.
- [x] Make duplicate-word selection preserve semantic identity on mutating routes as well as in display — mechanically guarded by `language_identity_reference_test` and the Terminal exact-reference CI smoke (2026-09-15).
- [ ] Report invalid numeric inputs, unresolved operations, and input-budget outcomes without silent invented defaults or misleading success.
- [ ] Distinguish the native robot dialogue joke from any future inspectable authored Object and executable PropertyPath it advertises.

These belong to Zach's existing Terminal priority; they do not authorize rewriting real saved worlds. Crystal §14 provides discriminating acceptance stories.

## Existing related work

- [Contextual Language and Semantic Binding](../Contextual_Language_and_Semantic_Binding/Contextual_Language_and_Semantic_Binding.md): occurrence/context, scoped linking, alternatives, and partial text/graph editing.
- [Neuro-Symbolic Formations and ML](../Neuro_Symbolic_Formations_and_ML/Neuro_Symbolic_Formations_and_ML.md): learning representations and execution; read with the crystal's current-source and dense-state qualifications.
- [Language System Decay Revision](../AUTHOR_Language_System_Decay_Revision/AUTHOR_Language_System_Decay_Revision.md): historical structural-relation loss and the outstanding human decision.
- [Streaming Pipes and FIFOs](../Streaming_Pipes_and_FIFOs/Streaming_Pipes_and_FIFOs.md): foreign bytes/processes, distinct from authorized live terminal acts.

The learning instrument, parameter projections, terminal art, and model-assisted vocabulary in the crystal are envisioning for these existing efforts, not features implemented in this pass. No additional governance layer or frozen dictionary is prescribed.

## Verification and scope

The original Astra pass changed documentation and appended an intercom message only. The 2026-09-15 implementation reconciliation above is supported by landed source and focused CI, but no claim is made that the entire Terminal experience is complete or Person-verified.

A future implementation that closes the remaining boot, Law-authoring, persistence, and truthful-failure stories must add its actual live checks, especially word selection, command effects, and save/return, to the Person Verification List before claiming the experience complete.

*Signed: Codex / GPT-6 Astra · `gpt-6-astra/01a09f43` · 2026-09-14T16:14:02-07:00; implementation reconciliation added by GPT-5.6 Sol, 2026-09-16.*