# Law Line: Authored Appearance

*Claude Code · Claude Opus 5.5 · session `016Yb6bNBaKzNFWs41sMyMjx` · 2026-10-14 (PDT). An earlier draft of this file, written in the same session while the harness reported the model as Claude Sonnet 4.5, proposed string SGR codes and new `Word` fields; this version records what was built.*

## What Zach asked

> "MAKE THE LAW LINE IN THE TERMINAL MORE BEAUTIFUL USING LEXEME SYMBOLS OR OBJECTS WITH SYMBOLIC PROPERTIES OVER THE TERMINAL CHANNEL BUT FIRST PLAN THE APPEARNACE AND SET UP MORE IN THE MODALITY CHANNEL IF THERE ARE SUBSTRATE GAPS THAT NEED TO BE FILLED IN BEFORE WE CAN FULLY AUTHOR BEAUTIFUL VISUALS AND EFFECTS IN THE LAW LINE TERMINAL" — Zach, 2026-10-14

Origination:
- **From Zach:** Lexeme symbols carry the look, through the Terminal channel. Also from him: the substrate gaps come first, and Zone-scoped authored strings, as with `meaning.<event>` ("The Zone", 2026-10-05).
- **Mine (Claude Opus 5.5):**
  - the property names;
  - a single `look` question asked by the editor;
  - colour stored as vec3 and lowered at the channel;
  - swatch glyphs;
  - the "Illuminated Manuscript" theme proposed below.

## The principle

Appearance is **authored data on beings that already exist**: Lexemes and Zones. There is no new C++ field on Lexeme or Zone, no enum, and no theme class (Refusals 1, 3 and 7). The channel only *lowers* what is authored to the terminal it is talking to. An unauthored world looks exactly as it did before.

## Authored properties (ordinary dynamic properties)

| On | Property | Value | Means |
|---|---|---|---|
| a Lexeme | `appearance.glyph` | string | a sigil shown beside the word in the menu (`≥`, `⊤`, `●`) |
| a Lexeme | `appearance.color` | vec3 0..1, or `"#rrggbb"` | the word's colour, in the menu and in the typed line |
| a Lexeme | `appearance.style` | `"bold italic dim underline reverse strike overline"` | attributes laid over whichever colour wins |
| a Lexeme | `appearance.glyphColor` | vec3 / hex | the glyph's own colour (a colour word's swatch), apart from the word |
| a Zone | `appearance.<key>.<field>` | as above | `<key>` is `role.<role>`, `chrome.<element>`, or `lexeme.<identifier>` (that Lexeme, in this Zone's context) |

Colour is vec3 because that is how this codebase already spells colour (Object `color`, Material `baseColor`, the Law Line's own `gold`). So `add property @<lexeme>.appearance.color to gold` just works.

**Read order:** the Lexeme first, then the body's Zone, then the line's own Zone. The line's Zone speaks last, as `meaning.*` does.

**Roles:** clause, logic, filler, activation, scope, preset, action, operator, condition, kind, value, path, being, event, name, error.

**Chrome elements:**
- the line and menu: `prompt`, `ghost`, `echo`, `selected`, `chosen`, `header`, `description`, `detail`, `detail.text`;
- status lines: `error`, `preview`, `question`, `note`, `notice`, `caret`;
- blanks and footer: `blank`, `blank.selected`, `footer`, `hint`;
- the help page: `help.border` (its glyph may be six single-cell characters, `╭─╮│╰╯` in that order), `help.title`, `help.label`, `help.text`, `help.mark`.

The footer mark's **colour** is not authorable: green means this Zone hears the line, yellow means it does not. That is a fact, not a theme. Its glyph is authorable.

## Substrate gaps found and filled

1. **Lexeme `authoredProperties` never loaded.**
   - ZoneSerialization wrote them on Save Zone; `internZoneLexemes` (FormationSerialization.cpp) ignored them on load. An authored Lexeme property survived the save and vanished on restart.
   - Fixed: they are restored when the Zone is loaded.
2. **A word lost its Lexeme on the way to the screen.**
   - `Span` and `Suggestion` carried only a role.
   - Both now carry `lexemeId`. A shared spelling (two meanings, Metalaw decides) carries none, so no one Lexeme's look speaks for it.
3. **Colours were a hardcoded table in the editor.**
   - `LineEditor::look(key) -> {glyph, sgr, style, glyphSgr}` is the one question the editor asks. An empty answer keeps the built-in look.
   - TerminalChannel answers it from a derived cache of every `appearance.*` property. The cache is re-derived once per frame (measured at about 5.6 µs per `sense()` with 193 Lexemes), and a change redraws, so a Law that re-authors a look is seen on the next frame.
4. **Colour depth.**
   - Authored vec3 is lowered to `38;2;r;g;b` when `COLORTERM` is `truecolor` or `24bit`. Otherwise it goes to the nearest xterm-256 cube or grey entry, so gold becomes `38;5;220`.
   - The registered, writable `colorDepth` overrides a terminal that lies. Apple Terminal does not set COLORTERM.
5. **Display width.**
   - `visibleWidth` and the panel originally counted code points. The corrected implementation uses utf8proc grapheme boundaries and character widths: CJK and `✨` take 2 cells, combining marks remain with their base, and VS16 makes `❤️` 2. Joined emoji, flags and clipping share this display-cluster contract.
   - The help page, menu padding and click-to-cursor share the terminal width contract. See the review correction below for display-cluster handling and native-terminal limits.
6. **Chrome glyphs were literals.** `▸ ⤷ ✗ ↳ ◆ ^`, the help box and its colours are now `chrome.*` looks, with the old glyphs as defaults.

**Deferred** (Zach's call; it needs the Law/Timeline ontology): time-varying looks, i.e. OntoMath-valued colour sampled at a Timeline's `t`. Laws can already change looks on events, and the next frame shows the change.

## Proposed first theme for LawLine: "Illuminated Manuscript"

Medieval scribes wrote a manuscript's structure in red ink ("rubric" is from *ruber*). They laid gold leaf on what mattered most, and used lapis, verdigris and madder for the rest. A Law sentence has the same anatomy:
- its structure words (WHEN / IF / THEN) are rubricated;
- its presets are gilded;
- each word that has a mathematical meaning wears that meaning's sign.

```
❧ earthcall> when clicked then set color to g          (gold prompt; rubric "then"; verdigris "set")
   § Values
   ❧ ● gold         a Lexeme denoting law-line-value-gold     ← ● drawn in (1, .84, 0), the word in ochre
     ● green        a Lexeme denoting law-line-value-green
     ● gray         …
     ☙ (1, 0.84, 0) · "gold" denotes law-line-value-gold
  ✦ Law Line · hears ✓ · as Zach · 214 laws  │  tab take · ↑↓ choose
```

| Words | Glyph | Colour |
|---|---|---|
| clause / logic / filler (when, if, then, called, on, and…) | — | rubric vermilion (0.90, 0.38, 0.26) |
| presets (when clicked ◉, on hover ◌, pointer leaves ○, touched ⋈, land ⤓, jump ⤒, zone opens ⌂, always ∞, becomes true ⇡, no condition ∅, event-triggered ↯) | as listed | gold leaf (0.96, 0.78, 0.32), bold |
| actions (set ≔, add +, scale ×, publish ⇢, destroy ✕, grant ⊕, revoke ⊖, relate ↔, Create ❖) | as listed | verdigris (0.38, 0.80, 0.64) |
| comparisons (= ≠ < ≤ > ≥) | the sign itself | silver (0.74, 0.77, 0.86) |
| conditions (is a ∈, related ↔, touching ∩) | as listed | Tyrian violet (0.74, 0.56, 0.94) |
| colour values | ● in the colour it denotes (black: ○) | ochre (0.94, 0.70, 0.42) |
| on / yes / y, off / no / n | ⊤ / ⊥ | ochre |
| OntoMath (Sin Cos Tan ∿, Sqrt √, Union ∪, Intersection ∩, Difference ∖, Dot ·, Cross ×, Hadamard ⊙, Flow ∫, Map ↦, Sequence ⋯, Parallel ∥) | as listed | ochre |
| paths · beings · events · names | — | parchment (0.94, 0.90, 0.80) · lapis (0.46, 0.70, 1.0) · rose madder (0.95, 0.55, 0.70) · bold |
| chrome | `❧` selected, `§` section headers, `☙` detail, `✦` footer | gold; descriptions and hints in a muted ink (0.62, 0.58, 0.52) |

Errors default to a red underline and `✗`. Both `role.error` and `chrome.error` are authorable, so an authored theme can change their color, style and glyph. Error status and refusal behavior remain unchanged by appearance; this pass provides no immutable visual refusal marker.

The telos Lexemes (Christ, Logos, Lux, …) denote no Law and never appear in the menu. They get no glyphs: sacred names are not decoration.

## How it is written into the save

Through `scripts/seed_law_line.py`, as a patch, never a regeneration (FIRST_MOVER_AUTHORING §7 rule 8):
- keys are only **added**, inside the existing Lexeme entries and a new Zone `authoredProperties` object;
- a key that already exists is never changed, so a Person's own look always wins;
- the bytes are preserved, the old file is backed up to `scratch/backups/law-line/`, the new file is staged, nothing is verified erased, and the rename is atomic.

Recorded as `authors: Zach` (his DID), `injected_by: Claude Opus 5.5`.

## Verification

- `line_editor_test` `appearance()`:
  - cell widths;
  - Lexeme glyph and colour in the menu and the typed line;
  - a style-only role keeping its colour;
  - authored selection glyph;
  - swatch colour apart from the word;
  - `NO_COLOR` still shows glyphs;
  - wide glyphs never overflow a row.
- `law_line_zone_test` (real seed):
  - a Person authors a Zone chrome glyph and a Lexeme colour with ordinary Law Line sentences, and the menu wears them;
  - truecolor gives exact gold (255, 214, 0), and 256 colours give `38;5;220`;
  - Save Zone writes them; first hydration restores the Lexeme's look, ordinary admission preserves live edits/removals, and explicit snapshot restoration can rewind them;
  - per-frame cost is measured.
- Person checks (beauty is Zach's judgement) are in `docs/Agenda/Tasks/For Zach/Person Verification List.md`.


## Review corrections — 2026-10-14

Zach commissioned an audit of Claude's pass and then said “GO FIX.” The [audit](../audits/LAW_LINE_APPEARANCE_REVIEW_2026-10-14.md) preserves the original counterexamples and records follow-up verification. Codex extended the implementation at its codec and terminal modality boundaries; the optional authored theme remains held by Zach's “Don't touch the save yet.”

- Ordinary Zone/session admission hydrates properties only on newly interned Lexemes. Existing canonical Lexemes retain both edits and removals, including when another Zone names them. The explicit Restore-unsaved route separately admits snapshot replacement, including property absences. This distinction also reaches the stored-Singular codec. Relation endpoint retries never replay properties.
- Terminal measurement, clipping, menu/style runs, cursor movement and help rows use utf8proc extended grapheme boundaries, rather than separately styled code points. Printable glyph slots refuse invalid UTF-8, C0/C1 controls and Unicode line/paragraph separators; the authored property remains intact and an invalid glyph falls back to the existing printable look. Other panel text visibly replaces such controls.
- Widths use a deterministic one-cell ambiguous-character convention and two-cell emoji sequences. Font/emulator differences still require native-terminal acceptance; a test of rendered text is not a screenshot or proof about every terminal.
- Error appearance is authorable as documented above; no new author-policy restriction was invented during review.

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-14T22:30:39-07:00.
