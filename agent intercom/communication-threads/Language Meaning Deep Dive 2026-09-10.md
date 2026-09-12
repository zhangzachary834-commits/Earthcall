# Language / Lexeme branch — contextual meaning research

**OpenCode (GPT-6 Astra)** · session `language-depth-20260910-115436` (local research-session label; harness UUID unavailable) · 2026-09-10 12:06 PDT.

Zach requested a deep dive into one branch with large-scale suggestions that do not repeat other models' proposals. I am tracing Language ingress → parsing → Lexemes/Relations → consumers/persistence, comparing against the Logos plan, semantic-network plan, Fable's language feedback and Zach's response, and the existing occurrence-of work.

The research focus is preservation of context and ambiguity: ingress currently retains `sourceClient` in its queue but does not consume it in `tick`, chooses `mgr.active()` at consumption, directly materializes parsed Relations, and resolves spellings through one global index. `Utterance` / `occurrence-of` already exist; their invention is not a new recommendation. Zach's stakeholder-owned mutable meanings are the human origin; scoped binding and language-specific operational mechanisms are the potential extension.

Working artifacts will be a scratch executable probe, an audit, and an Agenda research task. The probe will use synthetic in-memory fixtures and execute outside the real save tree. Current `src/terminal_entry.cpp` edits belong to an existing session and are relevant context for the terminal consumer.

## Measurement update — 2026-09-10 11:59:21 PDT

**OpenCode (GPT-6 Astra)** · session `language-depth-20260910-115436`.

Timestamp correction: the opener's `12:06 PDT` was an erroneous future time; that message was written between the measured session start (11:54:36 PDT) and this update. The scratch headers now use the measured 11:59 time.

Rebuilt and ran `logos_modality_test` and `relation_retry_lexeme_test`, both passing. A scratch executable linked against that same configured core reproduced ten current-behavior observations, including same-spelling type-Lexeme collapse in RelationManager, quotation/assertion parse equivalence, target/occurrence omission on successful parsing, and consumption-time Zone routing. These are synthetic in-memory diagnostics, not a deployed language feature or a visual acceptance claim. All executables ran in a disposable directory beneath the harness-approved temporary root.

## Research crystallization — evidence timestamp 2026-09-10 12:00:26 PDT

**OpenCode (GPT-6 Astra)** · session `language-depth-20260910-115436`.

The record is `docs/audits/LANGUAGE_CONTEXT_AND_SEMANTIC_BINDING_2026-09-10.md`, indexed by `docs/Agenda/Tasks/Specific Tasks/Contextual_Language_and_Semantic_Binding/Contextual_Language_and_Semantic_Binding.md`. Four proposed extensions: context/force-preserving language acts; scoped, edition-specific semantic linking; packed, inspectable interpretations; and partial bidirectional text/graph editing. The audit credits Zach's mutable stakeholder-owned meanings and distinguishes these extensions from Fable's feedback, the Logos/semantic-network plans, Second-Nature intent templates, and existing Utterance/type-Lexeme work.

Cross-session dependencies: the current Zone Lexeme codec emits only id/symbol; richer linguistic state should travel through the existing persistence-root work. `Zone::syncFormationMembers` does not preserve arbitrary occurrence beings, and the generic creation path explicitly refuses Lexeme birth. Those seams mean that simply using the detached Utterance class or adding a richer parser would not close the end-to-end path. No production change is part of this research pass.
