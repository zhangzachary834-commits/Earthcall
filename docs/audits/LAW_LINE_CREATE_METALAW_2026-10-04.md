# Law Line Create initializers: authored Metalaw compilation

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-04 18:46 PDT.

## Human direction and resulting surface

Zach requested `Create <SingularKind, properties: {(hardcoded), (authored)}>` and explicitly required Metalaws to compile Create Lexemes into the final Law. The implemented surface is generic parameterized action-Lexeme notation, backed by ordinary authored compiler Laws. There is no spawn-below-me verb, new action-kind enum, or alternate creation engine.

```text
called Beneath Me when clicked then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold, authored: {purpose: "A foothold"}}>
```

Each click creates one Object whose centre is at the speaking author's current position plus `(0, -3, 0)`. Add `if Identity @law-line-cube` before `then` to restrict the event subject. Creation is triggered later, not on sentence submission. `?` reads syntax and reports compilation deferred; Save Zone persists the compiled Law and resulting beings.

## Implementation and invariants

- `LawSentence` reads `<selector, properties: {...}>` structurally for any action Lexeme. It emits initializer/invocation records to a vocabulary callback. It does not choose Set, Map, AddProperty, or Create semantics for those records. Legacy nonparameterized sentence opcodes remain on their existing path.
- Expressions use the existing typed OntoMath AST and MathBindings, with finite scalars, vector triples, qualified reads, and arithmetic. Lists/dictionaries use existing typed Property containers; their entries are literal. Parser, template, and model nesting refuse beyond the explicit structural bounds.
- `TerminalChannel` exposes `compilation.input`, writable `compilation.template`/`compilation.error`, and derived `compilation.result`. Ordinary Laws explicitly targeting the Terminal Law may condition on input and supply JSON action-model templates. The channel performs only generic `$slot` JSON-pointer substitution and model validation/deserialization. No template means refusal. Different outputs mean refusal, regardless of register order. Template errors and unsupported action kinds refuse instead of defaulting to Set.
- Preview, menu, submit preflight, and trailing-question invocations do not run compiler Metalaws. Deferred placeholder models cannot pass the final enact path. Parameterized shared spellings retain denoted Law identities for the existing Metalaw ambiguity seam.
- The `Create` word's argument signature is authored `sentence.arguments` data. The `my` root is authored `sentence.root` data and resolves to the actual speaking author's identifier. Foreign authoring with no unique author cannot borrow the keyboard Person's pronoun. The final action retains the runtime property binding instead of freezing the value at authoring time.
- Existing Create child actions shape the newborn. Registered initializers use Set/Map; authored ones use AddProperty and optional Map. All existing TransferPolicy, Person/real-correspondent, authorship, and birth-adapter boundaries remain in force.
- `Object` has an authored direct-birth rule. An explicit `@prototype` uses the existing universal concrete-kind-preserving operation. Unknown unconfigured kind spellings refuse. Relation participant, newborn identity/name, local Ourverse extent, and arbitrary transactional initializer semantics are not invented by this syntax pass.
- Law identity serialization now round-trips granted properties, preserving the argument/root vocabulary metadata. The app and BootedEngineHarness working-set providers now enumerate the active Zone's direct Formation members and retained Singulars, after existing providers and with pointer deduplication. A prototype must be addressable by ordinary PropertyPath, not merely visible as a vocabulary Relation endpoint. This changes named reach, not Zone location/ownership semantics.

## Authored save patch

Added nine roots under `saves/laws/`: `law-line-act-create`, `law-line-value-cube`, `law-line-root-my`, and the six `law-line-compile-*` rules for Object/prototype invocation and registered/authored literal/expression initializers. All have `authors: ["Zach"]`, authority 0, and `injected_by` identifying Codex / GPT-6.1 Sol and this session. The six compiler Laws have an authored compilation-request trigger and explicitly target `terminal-channel`.

Appended three Lexemes (`Create`, `Cube`, `my`), three denotes Relations, and nine lawRefs to `saves/zones/LawLine/zone.json`. The patch preserves original bytes by inserting only the new root-array entries. All old values and array prefixes were checked unchanged, and the exact resulting bytes were checked against the insertion-only operation. Original Zone bytes and this session's earlier compiler-root revisions are retained in `scratch/backups/law-line/` (ignored, outside `saves/`). Dependencies were installed before the Zone references. A seed rerun reported nothing written. Existing Person-authored Law roots were retained unchanged.

The seed script's Python JSON round-trip check and the actual C++ Zone-load/save harness provide separate preservation witnesses. The harness copies roots into an isolated temporary SaveRoot; its generated test Laws/beings do not enter Zach's saved Zone. Source patch artifacts do not fabricate a screenshot or Person witness.

## Verification

Final verification: **seven focused tests passed**: `law_line_test`, `law_line_zone_test`, `terminal_zones_test`, `mcp_authoring_surfaces_test`, `mcp_first_mover_bridge_test`, `universal_singular_creation_test`, and `no_black_box_test`. `law_line_zone_test` reported **65/65 checks passed**. The core and `earthcall_webgpu` targets built successfully. Python compilation of the seed script and `git diff --check` passed.

The initial sandboxed CTest run passed the grammar, Zone, Terminal-Zone, and universal-creation tests. Its MCP tests could not bind localhost sockets (`Operation not permitted`); both passed on a sequential permission-enabled rerun. The property audit stalled during sandbox-blocked macOS service initialization; that identified test process was stopped, and the permission-enabled rerun passed in 0.61 seconds. These environment failures are superseded by the valid reruns, not attributed to the implementation.

The expanded Zone test independently witnesses runtime-relative placement at two different Person positions, gold color, authored initializers, syntax-preview nonmutation, seed-root metadata persistence, exact compiled-model round-trip, missing compiler refusal, changed-template semantics, malformed and conflicting compiler output refusal, foreign-pronoun separation, authored completion arguments, universal Lexeme prototype creation, sixteen-initializer syntax lists, unknown kinds, and duplicate-property refusal. Existing deletion/vocabulary/Zone persistence checks remain green.

Commands: `cmake --build build --target law_line_test law_line_zone_test earthcall_webgpu terminal_zones_test mcp_authoring_surfaces_test mcp_first_mover_bridge_test universal_singular_creation_test no_black_box_test -j8`; focused CTest selection for those seven tests, with the three environment-sensitive tests rerun as described. Diagnostic logs for this pass are `/private/tmp/earthcall-create-final-build.log`, `/private/tmp/earthcall-create-ctest.log`, `/private/tmp/earthcall-create-mcp-ctest.log`, `/private/tmp/earthcall-create-properties-ctest.log`, and `/private/tmp/earthcall-create-zone.log` (temporary, not durable artifacts).

These are focused verification, not a whole-suite verdict. `earthcall_webgpu` compilation is desktop-build evidence; injected TerminalChannel execution is runtime evidence without a TTY, not pixel or Person acceptance.

## Remaining Person and implementation work

[Person Verification List](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md) records completion, syntax preview, clicking, movement-relative placement, Law Graph/provenance inspection, and Save Zone/restart checks. Live visual/feel acceptance remains pending. General Map/Flow/Drive/Zone-condition text grammar, named-kind birth rules beyond Object, Relation participants, and broader initializer transactions remain open in [the Law Line task](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md). Arbitrary child actions retain the existing Create execution semantics: inspect runtime action traces for refused property writes; this pass does not promise atomic rollback of arbitrary world effects.
