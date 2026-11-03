# Law Line arguments for the remaining action kinds

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 00:14 PDT.

Zach requested the other missing action-operator arguments after requiring Metalaw-compiled Create and ordered multi-sentence registration. This pass extends that same compiler seam; it does not add domain classes, action enum values, alternate permissions, or per-kind parser lowering.

Sixteen authored words and sixteen compiler Metalaws now cover Lerp, Drive, Sequence, Parallel, Map, Flow, AddElement, RemoveElement, Synthesize, PlayAudio, AuthorZone, WritePixel, ElevatePixels, FileRead, FileWrite, and CodecTransform. Argument field names, required slots, optional defaults, and operator-specific signatures are held in the saved Laws. Named argument syntax senses literals, existing Piecewise/MathBindings via `expression:`, and nested action lists via `children:`. The generic template protocol supplies `$slot` references with optional authored `$default`. Unconsumed top-level fields, duplicate fields, missing required fields, invalid typed models, and absent compilers refuse. Closed fragments without signature metadata retain their existing exact trees.

Canonical coverage now includes FileRead/FileWrite/CodecTransform. Completion merges canonical duplicates with their authored spelling/opcode, retains signatures, and ranks clause words ahead of the enlarged action list so the existing bounded menu does not bury `then`. Typing a prefix narrows later alphabetic operators without increasing the menu bound.

## Evidence and limits

- WebGPU and focused test targets built successfully.
- Five CTest targets passed: law_line_test, law_line_zone_test, terminal_zones_test, mcp_authoring_surfaces_test, mcp_first_mover_bridge_test.
- Pure grammar tests deliberately compile Lerp into Publish through a supplied callback, proving that the reader does not decide the action kind. They check nested action lists, expression bindings, duplicate/conflicting fields, absent compilers, all canonical argument hints, and the 32-level invocation bound.
- The booted Zone test submits all 16 forms through the real Terminal hearing path and saved Metalaws, checks resulting kinds and exact model round-trips, and checks actual cube mutations for Lerp, Drive, Map, and Sequence. It checks missing/unknown/duplicate/wrongly typed arguments, compiler-disabled refusal, nonacting previews, authored optional timbre defaults, and completion signatures/clauses.
- The Zone patch preserves all old fields and collection prefixes while adding 16 Lexemes, 16 denotes Relations, and 32 Law references. Existing Law roots were preserved; reseeding leaves every Law Line save byte-identical. The recorded author is Zach and the injector is Codex; backups remain under scratch/backups/law-line outside saves.
- This verifies authoring arguments for all 16 kinds, not runtime operation of every native channel. Actual sound, pixel edits, file I/O, codec output, and Zone birth keep their existing executor/channel semantics and gates. ElevatePixels uses its existing Piecewise selector record; richer region notation is not newly supplied. Flow needs an authored bounded domain and suitable activation for continuous work; this pass adds expression syntax, not a new time policy. Person verification is open and no full-suite or pixel acceptance is claimed.

Future agents: change `law-line-compile-args-*` templates and `law-line-args-*` `sentence.arguments` data for operator-specific meanings. Preserve saved roots, inspect trace outcomes when a channel refuses, and do not add a kind-specific branch to `namedInvocation`. This work is indexed in [the Law Line task](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md) and [Person verification](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md).
