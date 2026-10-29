# Review: authored appearance for the Law Line

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-14T22:09:45-07:00*

**Current status:** the three findings below are corrected and all five focused tests pass; the WebGPU app builds. See [the correction pass](#correction-pass--implemented-and-verified) for evidence and remaining native acceptance checks. The original review is retained below as historical evidence.

Zach commissioned this review of the Sonnet-started Law Line appearance work, paused the review to commit, then asked it to continue. The inspected implementation is commit `07435d0ce9c9a33156660280a67e121fb9cdcfad` (parent `59232506f`), at merged checkout HEAD `edccd0968`, branch `sync-from-earthcall-main`. Scope: the Terminal/LawSentence/editor and Lexeme-restoration changes, their tests, and [Law Line: Authored Appearance](../plans/LAW_LINE_BEAUTIFUL_TERMINAL_APPEARANCE_2026-10-14.md). The commit also contains unrelated sky work, which this review does not certify. The plan now attributes its revised implementation to Claude Code / Claude Opus 5.5, noting that the initial draft was written while the harness reported Sonnet 4.5; that attribution is reported from the document, not independently established here.

**Original review verdict:** the authoring direction is sound, but do not treat the implementation as ready without fixing the live-state overwrite and the two output-layout failures below. Existing green tests do not cover those cases. The original review pass changed no production code or live saves and did not install the proposed theme.

## Findings

### P1 — Lexeme restoration overwrites already-live authored state

[FormationSerialization.cpp:33–36](../../src/Singularity/Storage/Serialization/Relation/FormationSerialization.cpp) writes every saved authored property into the result of `language.intern(...)`. `intern` returns an existing canonical Lexeme when that ID is already live. Formation hydration is rerun both during ordinary `ZoneManager::loadState` and after Law endpoints arrive; it is also called from the `applyZoneJson(..., replaceObjects=false)` live-preservation path. Restoring missing beings and merging references therefore now also rewinds existing beings.

Two actual production paths reproduce the loss:

1. Build a Zone holding a Lexeme whose `appearance.glyph` is `saved-glyph`, serialize it, change the live value to `unsaved-live-glyph`, and call `applyZoneJson(zone, oldJson, false)`. The result becomes **`saved-glyph`**.
2. Save a session through `ZoneManager::saveState`, change that Lexeme to `current-unsaved-glyph`, then invoke the ordinary `ZoneManager::loadState` on the older session. The result becomes **`snapshot-glyph`**. This is not the explicit Restore-unsaved route.

This applies to every authored Lexeme property, not only appearance, and can also affect a canonical Lexeme shared by multiple Zone records. Distinguish first hydration/explicit restoration from reference rehydration and live merges; do not let loading another snapshot replay properties into an existing live canonical being. Add a real session-load preservation case alongside the fresh-load restoration test. The supplied test removes the live property before restoring it, so it cannot detect an overwrite of a newer value.

### P2 — Styled menu runs split variation sequences and exceed the row budget

[LineEditor.cpp:1191–1194](../../src/Singularity/Terminal/LineEditor.cpp) feeds each UTF-8 code point separately into `PanelLine::add`. The new width implementation counts a one-cell base followed by VS16 as two cells only when both are in the same supplied string. Separating `❤` from its following variation selector makes clipping account for one cell even though `visibleWidth` of the complete result accounts for two.

A menu candidate containing 40 `❤️` sequences, rendered at width 28 with color disabled (the supported NO_COLOR route), emits a panel row that this same implementation measures as **49 cells**, exceeding the **27-cell** panel budget. In the real terminal this causes wrapping that the region-row and mouse mapping calculations do not describe. `HelpRow` also measures isolated code-point substrings and deserves the same regression case. Keep measurement, clipping and styled emission on coherent display clusters; a whole-string width assertion with a few isolated wide glyphs does not establish that property across SGR runs.

### P2 — Authored glyphs admit terminal controls without accounting for their effects

[TerminalChannel.cpp:1332–1333](../../src/Singularity/Terminal/TerminalChannel.cpp) accepts any nonempty string as a glyph. `PanelLine` copies its bytes, counts control characters as zero width, and returns a single logical row; `draw()` writes the result directly to the TTY. No glyph admissibility check prevents line breaks or cursor-control bytes.

With `chrome.selected.glyph = "\nextra-row"`, the real editor emits the extra LF while reporting only **2 rows**: its returned text contains **2 LF characters**, which already represent **3 physical rows**. Cursor restoration and subsequent erase/click mapping therefore address the wrong region. Reject or visibly encode control characters in printable glyph slots, or model their actual layout explicitly; do not interpret arbitrary transport controls as zero-width printable glyphs. The SGR color/style conversion already avoids copying arbitrary author-provided SGR strings, so the raw glyph branch should preserve the same separation between authored text and device controls.

## Plan review

The strong part is the ontology: appearance lives on existing Lexemes/Zones as authored properties, the Lexeme identity survives through Span/Suggestion, and terminal color lowering stays in the modality channel. There is no new domain-being class or theme enum. The optional manuscript palette is a proposal, and the task explicitly records Zach's “Don't touch the save yet”; leaving that theme unapplied is correct.

Two claims need tightening:

- “authored glyphs never shear the panel” is disproved by the layout counterexamples. The new width table is an improvement over counting code points, but it is not a complete display-cluster contract.
- Plan line 97 says errors retain the built-in red underline/cross and a theme cannot make them look like structure. The implementation exposes both `role.error` and `chrome.error`. A direct probe using `chrome.error = {glyph: "SUCCESS", sgr: "32"}` emits a green SUCCESS mark for an error, with no built-in cross. Either remove that claimed invariant and describe the actual authoring freedom, or obtain Zach's intended error-indication boundary before enforcing one. This review does not choose a new author-policy restriction for him.

The cache is clearly described as derived state and refreshes every frame. Its roughly 5.4 microsecond fixture measurement at 199 Lexemes is useful bounded evidence, not a scaling proof or native terminal latency measurement. Timeline-driven appearance remains correctly labeled deferred.

## Verification and reproduction

Freshly built `line_editor_test`, `law_line_test`, and `law_line_zone_test` at the committed snapshot. Ran:

```sh
ctest --test-dir build --output-on-failure -R '^(line_editor_test|law_line_test|law_line_zone_test)$' -j2
```

**3/3 passed**, including **279/279** Law Line integration checks; total 84.41 seconds. Their appearance-cost witness reported 5.38875 microseconds per sense call at 199 Lexemes. This is focused verification, not a full-suite or Person-acceptance verdict.

The independent counterexamples are retained in [the C++ probe](../../scratch/probes/law_line_appearance_review_2026_10_14.cpp), [its runner](../../scratch/probes/law_line_appearance_review_2026_10_14.py), and [the observed output](LAW_LINE_APPEARANCE_REVIEW_2026-10-14.json). Reproduce after building `line_editor_test`:

```sh
python3 scratch/probes/law_line_appearance_review_2026_10_14.py
```

The runner links production objects through the existing test link recipe. It uses an isolated temporary save root for the real session save/load and deletes that fixture afterward. It reports counterexamples; exit zero means the probe ran, not that the reviewed behaviors are correct. The width/control probes exercise the real LineEditor renderer and are followed through `draw()`'s unescaped TTY output in source; they are not screenshots from Zach's terminal. The preservation probe also executes actual `ZoneManager::loadState`.

For the next builder: fix preservation first, then cluster-coherent emission and printable glyph admission; retain both fresh-load and live-merge tests. Keep the appearance data authored and preserve Zach's explicit save-theme hold. No implementation fix or theme injection was made during this review.


## Correction pass — implemented and verified

Zach followed the review with “GO FIX.” The correction keeps Claude's authored Lexeme/Zone appearance architecture and changes only codec/reference admission and terminal text mechanics. No live save or optional theme was patched; the tests use isolated save roots. The checkout WebGPU executable was rebuilt, without installing or launching the macOS app bundle.

Ordinary admission now restores dynamic properties only when it interns a new canonical Lexeme. Reusing a live canonical identity preserves its values, deletions and references, even through another Zone. The explicit Restore-unsaved branch passes a separate restoration intent through Zone construction and stored-Singular restoration, and may restore both saved values and absences. Legacy Lexemes hydrate before stored graph decoding; later endpoint retries merely bind references. The universal creation witness now explicitly requests restoration when it intentionally replaces its old graph, and separately checks that ordinary shared-Lexeme admission preserves current state.

Terminal width, clipping, styled menu/word runs, cursor boundaries and help rows share utf8proc grapheme segmentation. The pinned, vendored [utf8proc source and license](../../third_party/utf8proc/README.earthcall.md) replace the handwritten partial Unicode tables. Emoji variation sequences, joined families, flags and combining accents remain whole when clipped/styled. Glyph lowering refuses invalid UTF-8, C0/C1 controls and Unicode line/paragraph separators; invalid authored glyphs fall back to a printable existing look without changing the authored property. Other panel text visibly replaces controls. Help-heading glyphs also clip to their box budget.

The plan now accurately states that error appearance is authorable. Its error color/cross are defaults, not an immutable author-policy guarantee. No new restriction on authored error styling was introduced.

### Follow-up evidence

| Original production-path witness | Result after correction |
|---|---|
| 40 hearts at terminal width 28 | Maximum panel width **27**, budget **27**; previously 49 |
| Newline in selection glyph | **1** LF / **2** reported rows, no extra-row payload emitted |
| `applyZoneJson(..., false)` | Newer `unsaved-live-glyph` preserved |
| Actual ordinary `ZoneManager::loadState` | Newer `current-unsaved-glyph` preserved |
| Authored green error marker | Remains authorable; documentation corrected |

[Observed follow-up JSON](LAW_LINE_APPEARANCE_FIX_2026-10-14.json) retains the exact production probe results. Its `reported_unsaved_preserved=false` remains an object-reporting detail in the object-free fixture; the actual Lexeme value is the preservation witness. The original review JSON remains unchanged for before/after comparison.

Built `line_editor_test`, `law_line_test`, `law_line_zone_test`, `unsaved_preserve_test`, `universal_singular_creation_test` and **`earthcall_webgpu`**. CTest passed **4/4** (`line_editor_test`, `law_line_test`, `law_line_zone_test`, `unsaved_preserve_test`); the separate universal-creation executable also passed. Integration reported **284/284** checks; preservation reported **17/17**. Cases include fresh hydration before stored references, live changes/removals, explicit restoration of absences, stored canonical preservation, whole-cluster editing, color/NO_COLOR menu budgets, real-channel control rejection and the real help heading.

CTest elapsed 358.40 seconds while the app build and independent probe also ran; that duration is not a comparable performance measurement against the earlier isolated pass. The bounded appearance sense witness reported 13.9629 microseconds at 199 Lexemes under that workload, below its existing 2 ms limit. No runtime latency or speedup claim follows.

Build/test logs for this run: `/private/tmp/earthcall-law-line-fix-build.log`, `earthcall-law-line-fix-creation-build.log`, `earthcall-law-line-fix-creation.log`, `earthcall-law-line-fix-ctest.log`, `earthcall-law-line-fix-app-build.log` and `earthcall-law-line-fix-probe.log` in the same temporary directory. These are local supporting logs, not durable repository artifacts. `git diff --check` passed. The full suite and native PTY/app interaction were not run; native font/redraw, restart and Person acceptance remain in the [Person Verification List](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#law-line-authored-appearance-2026-10-14).

For the next builder: retain the distinction between canonical live references and explicit restoration; do not infer permission to rewind shared Lexemes merely from replacing a Zone's Objects. Keep printable text separate from terminal controls, and keep glyph/word clusters intact across style boundaries. Preserve Zach's optional-theme hold and the independent native acceptance checks.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-14T22:30:39-07:00*
