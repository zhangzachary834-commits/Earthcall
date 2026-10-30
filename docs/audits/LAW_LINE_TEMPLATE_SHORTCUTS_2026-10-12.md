# Stored Law-Lexeme templates through authored shortcut Metalaws

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-12T13:17:56-07:00*

Zach asked for custom shortcuts such as Cmd+T that show only stored templates matching the current Law Line, with no C++ beyond modality gaps. He also directed this work to follow the constitutionalist's app-level save migration. His prior “PRECONFIGURED NOT BESPOKE” direction remains the governing distinction: the bindings and shelf are authored; Terminal code supplies input/output operations.

## What changed

The Terminal decoder admits canonical modified ASCII press chords from CSI-u, plus otherwise unused legacy Ctrl-letter bytes. Cmd/Super is encoded by modifier bit 8; an exact Cmd+T report is `ESC[116;9u`. Repeat/release reports are consumed without another edge, malformed numeric reports cannot overflow, and bracketed-pasted controls cannot become shortcuts. Existing legacy editor controls retain their meanings. [Protocol source](https://sw.kovidgoyal.net/kitty/keyboard-protocol/).

`TerminalChannel` publishes `terminal-key-pressed` with itself as subject, and registers readable `key.chord`, `key.presses`, `draft`, and byte-offset `cursor`. Device input is queued beneath the Kernel. Dispatch pauses after a chord or submitted line, so its Laws decide before later input changes the draft. Change notifications invalidate draft/cursor/chord readings; large pastes compare drafts once per read rather than once per byte.

The generic output surface is `completion.items`, `completion.openRequests`, `completion.handledRequests`, `completion.active`, `completion.visible`, and `completion.matchCount`. Entries may be strings, ordinary dictionaries (`text`, optional `snippet`, optional `description`), or readable qualified PropertyPaths. The channel filters by case-insensitive literal prefix before the cursor and preserves input-list order. It does not discover template nouns, hardcode a shelf, or choose key bindings. No-match output is explicit. Selection uses the existing menu/placeholder machinery and acknowledges the transient choice; ordinary completion resumes afterward.

A secret line exposes empty draft/zero cursor, publishes no chord edges, invokes no completion providers, and refuses a completion opening. Confirmation answers also suppress chord dispatch/completion opening. No global OS keyboard interceptor was added.

## Authored data and current persistence

Nine new Law roots are installed under `~/Library/Application Support/Earthcall/laws/`:

- `law-line-template-always`, `law-line-template-click`, `law-line-template-hover`, `law-line-template-cube`: disabled holders with editable completion dictionaries.
- `law-line-template-shelf`: a disabled holder whose authored `items` list refers to those dictionaries.
- `law-line-shortcut-cmd-t`, `law-line-shortcut-ctrl-t`: enabled event Metalaws comparing the sensed chord to their own editable `binding.chord`, mapping the shelf into `completion.items`, then requesting its display.
- `law-line-value-constructor-literal`, `law-line-compile-value-literal`: disabled generic value vocabulary plus its enabled structural identity compiler, allowing an authored sentence to store lists/dictionaries without a template parser branch.

Six Lexemes and six `denotes` Relations, together with nine lawRefs, were appended to the live `zones/LawLine/zone.ecform`. Their recorded author is Zach's existing keyed Person DID (`did:earthcall:dmvokvvtp4jwmhhkyzv23xanzyukspdv5aykm7okru3mdidyncga`); `injected_by` names this Codex session, and authority stays zero. Existing Zone objects, materials, quality/state fields, prior Lexemes/Relations and prior lawRefs are unchanged. Exact prior files are kept under checkout `scratch/backups/law-line/`, outside app discovery.

The template text is a stored sentence draft; selecting it does not activate its disabled holder. Completing/submitting the text compiles through existing sentence Metalaws. This uses the denoted Law's authored-property codec, avoiding reliance on legacy Lexeme-property restoration. There is no second persistence store or permission system.

The installer defaults to `earthcall_data_home.data_root()`, retains `EARTHCALL_SEED_ROOT` as an isolated `<root>/saves` override, respects `.ecsave` → `.ecform` → `.json` non-empty identity precedence, and refuses an unsupported native envelope before creating roots. Supported MigrationRoot JSON text is patched by append-only insertion; existing files are staged, original-byte checked, round-trip checked, and atomically replaced. New roots are never written over an existing identity. The source checkout's frozen saves were not reseeded.

Shortcut Laws deliberately have no explicit Terminal target. The sensed event supplies their subject, and an Identity condition admits only the Terminal. This matters because the existing invocation compiler directly probes explicit Terminal targets; adding the shortcut there would reapply it merely because the previous chord still matches. The regression tests exercise that boundary.

## Verification

- App plus `law_line_test`, `line_editor_test`, and `law_line_zone_test` build successfully.
- Focused installed-data witness: **37/37 checks pass**, including authored rebinds, matching-only output, insertion/first blank, empty/unmatched drafts, queued-chord ordering, compiler isolation, real paste/Enter storage, subsequent menu readback, Law encode/decode, and secret-input refusal/cancellation.
- `python3 tests/singularity/test_seed_law_line.py`: **3 tests pass** for app-root selection, native identity precedence, preservation of existing Zone/Law bytes and shadow files, keyed attribution, idempotence, legacy scratch override, and early unsupported-envelope refusal.
- `ctest --test-dir build --output-on-failure -R '^(law_line_test|line_editor_test|law_line_zone_test)$' -j2`: **3/3 pass**, with **266/266** Law Line integration checks (81.61 s total). CTest layers the current seed over an isolated copy of frozen fixtures, using `run_law_line_zone_test.py`; it does not silently omit the new Metalaws or operate on live saves.

These are engine/channel/editor and codec witnesses. Physical Cmd delivery, the menu's feel, and Person-confirmed restart continuity remain on the [Person Verification list](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#law-line-stored-template-shortcuts). No native app window was launched to claim that acceptance.

Installed-root checks also prove all original live Zone fields/array prefixes survive, total additions are exactly six Lexemes/six denotes Relations/nine lawRefs, and a second installer run changes none of the nine roots or native Zone bytes. Save additions are committed in the data-home repository as `80775ae`; unrelated `ui/` remains outside that commit. Source implementation/docs remain uncommitted in the shared Earthcall checkout.

## Practical limits and handoff

Cmd+T reaches Earthcall only when the terminal sends its report; a terminal's own Cmd+T action may consume it first. Ctrl+T works through its ordinary control byte. See the [CLI lesson](../architecture/law/LAW_AUTHORING_CLI_GUIDE.md#stored-templates-and-authored-shortcuts) and the [custom binding](../../examples/law_line_template_keybind.txt) / [stored list](../../examples/law_line_store_templates.txt) examples.

Matching is literal-prefix completion, not semantic unification or wildcard matching of already-filled blanks. The existing editor's blank gate still treats literal `‹…›` inside a quoted storage sentence as unfilled; the verified storage program therefore uses complete sentence texts. Default blank templates insert/select correctly, and a blank-containing stored text can be edited in the existing Property editor. A future distinction between literal and active snippet markers belongs in generic editor semantics, not a template-specific parser.

For future agents/Jules: preserve the constitutionalist's app-root history/log changes; keep generic channel operations separate from authored binding/shelf choice; retain input ordering and secret isolation; use actual editor submission tests for examples; do not fix a seed by regenerating inhabited saves. [Law Line task](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md#stored-template-shortcuts--2026-10-12).
