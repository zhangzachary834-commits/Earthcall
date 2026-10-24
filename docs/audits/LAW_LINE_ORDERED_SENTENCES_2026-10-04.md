# Ordered Law Line sentences and property vocabulary

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-04 19:50 PDT.

Zach requested multiple Law sentences in one line, registered in their written order, plus add/remove/modify property. The existing generic Metalaw compiler seam now also accepts a structural sentence document. The saved `law-line-compile-sentences` Law reads `compilation.input.slot` and `opcode`, and writes an ordered-list template to `compilation.template`; the channel validates that its output contains every sensed sentence exactly once in source order. No compiler means no registration. Each registered Law records compiler provenance and uses the existing author/Zone-adoption path.

Top-level semicolons delimit sentences. Double-quoted strings and nested parentheses, lists, and dictionaries retain their semicolons. The whole batch is parsed before registration; invalid later syntax leaves every sentence unregistered. A trailing question mark previews without applying compiler Laws. Completion and live reading follow the last sentence. Names containing opcode words must be quoted. Immediate delete/search is not a batch Law sentence.

The three new Lexemes denote existing saved action Laws: `add property` → AddProperty, `remove property` → RemoveProperty, `modify property` → Set. No enum, domain class, or alternate property permissions were added. Removing an authored value can leave a previously materialized accessor addressable; Set can restore that value. The tests establish this existing behavior, and the task documents it rather than claiming strict absence guards.

Registration order is verified in LawManager's register; event agenda execution order is a separate concern. Ordered effects belong in one Law's Sequence (`and`). Adoption failures retain the existing per-Law semantics and report the number already registered; registration is not advertised as transactional. Foreign batch submission refuses before compilation because its API carries one separately scoped identifier, avoiding a grant bypass.

## Verification

- WebGPU app and focused test targets built successfully.
- Seven CTest targets passed: law_line_test, law_line_zone_test, terminal_zones_test, mcp_authoring_surfaces_test, mcp_first_mover_bridge_test, universal_singular_creation_test, no_black_box_test.
- Pure tests exercise sentence splitting and empty separators. The booted Zone test submits one line through the authored hearing path, checks source-order registration, applies the three resulting property Laws to the seeded cube, preserves quoted semicolons, previews without registration, refuses invalid later syntax, refuses when the batch compiler is disabled, rejects compiler output that drops sentences, checks final-sentence completion, and checks foreign refusal.
- The latest Zone patch preserves every prior field and collection prefix, appending three Lexemes, three denotes Relations, and one Law reference. The compiler root records Zach as author and authority 0. Byte-hash reseeding verifies all existing Law Line files unchanged.
- Existing Zone bytes are backed up outside saves under scratch/backups/law-line; existing Law roots were kept. Native TTY appearance and Person interaction are still open in the Person Verification List. This is not a full-suite or visual acceptance claim.

See [the Law Line task](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md) and [Person verification](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md).
