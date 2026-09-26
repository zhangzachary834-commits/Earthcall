# The Law Line — author Laws as sentences in the Mac Terminal

**Origin:** Zach, `docs/architecture/law/Natural Language Law Authoring.md` (2026-09-25).
**Design and record:** [`docs/plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md`](../../../../../plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md)

## Status

- **Rung 1 (2026-09-25):** built. Zach witnessed it: the "Red" sentence authored a Law, and the Law Graph showed the object-clicked trigger, `hp > 2`, and `set color 1 0 0`.
- **Rung 2 (2026-09-25, same day):** an ergonomic line, and more authored words.
  - Zach asked for: "tab should be able to actually select one and arrow keys should be able to move between them … like claude code cli … show me stuff temporarily without sending as a full message … think about other design and ergonomics".

## How to use it

1. Run `Run Earthcall.command` and keep its Terminal window beside the app. It prints *"The Law Line is listening."* above an `earthcall>` prompt.
2. In the app, switch to the **Law Line** Zone. Its seeded Laws are what hear the terminal. In any other Zone the terminal tells you nothing hears the line; the editor still completes and previews there.
3. Type. Everything below the prompt is temporary and redraws in place:
   - **The menu follows you.** It opens as you type a word and shows what each candidate means. It matches fuzzily: prefix, then word start, then letters in order (`gth` finds "greater than").
   - **Tab** takes the selected entry. On an empty word, Tab opens the menu of what may come next. **↑/↓** (or Tab/Shift-Tab) choose. **Enter** takes a choice you arrowed to. **Esc** closes the menu; a second Esc clears the line.
   - **Ghost text** (dim, after the cursor) is the rest of the selected word, or of a matching history line. **→** or **End** takes it.
   - **The line is coloured by how it is read:**
     - magenta: clause words;
     - bright magenta: presets;
     - green: actions;
     - yellow: operators;
     - cyan: conditions;
     - orange: values;
     - blue: events;
     - white: paths;
     - red underline: where the reading stopped.
   - **The panel under the line** shows:
     - the Law as it would be (`↳ WHEN … -> IF … -> THEN …`);
     - or what comes next (`next: a value`);
     - or, once you've moved past a word, the error with a `^` under it.
4. Press **Enter** to author. Only results stay in the scrollback: `✓ authored law_… (written by …)` in green, or `✗ refused: …` in red.
5. **Other keys:**
   - ↑/↓ on a closed menu walk history, which is kept across sessions in `saves/logs/terminal-history.txt`;
   - Ctrl-R searches history;
   - Ctrl-W / Ctrl-U / Ctrl-K delete a word, to the start, or to the end;
   - ⌥/Ctrl-arrows jump words;
   - pasted text arrives as one edit;
   - Ctrl-C clears the line, and Ctrl-C on an empty line twice quits Earthcall;
   - Ctrl-D closes only the Law Line;
   - Ctrl-L redraws.
6. `?` at the end previews without authoring. `?? word` searches the vocabulary, live in the panel: spellings, meanings, events, beings, properties.
7. The app's own log output appears **above** the prompt, dimmed. It is also saved to `saves/logs/earthcall-terminal.log`. It never breaks the line you're typing.

### Try these in the Law Line Zone

- `when clicked then set color gold`
- `when hovered then set color cyan`
- `on tick if hp is greater than 2 then add glow 0.1`
- `always if hp < 1 then set color gray`
- `?? color`

## Where meaning lives (so you can grow it without code)

- **A word is a Lexeme that `denotes` a Law.** Add a Lexeme, add a Law, and relate them with `denotes`; the word works immediately. The menu's description is the Law's name.
  - **Action/operator words:** a Law whose model has an **open slot**. A Set with no path is an action word ("set", "make"). A Compare with no path is an operator ("greater than").
  - **Value words:** a Set with **no path but a value**. "red" → (1, 0, 0); "on"/"yes" → true; "off"/"no" → false. "on" is also the structural trigger word, and the grammar position tells them apart.
  - **Presets:** a Law with **no open slot**. It fixes activation, scope, triggers, and any condition or action. Examples: "my event-triggered law", "always", "when clicked", "on hover", "when touched", "when I land", "when I jump". The empty `All()` fixes "no condition".
- **A Lexeme is any unit:** a phrase ("my law that fires when it becomes true"), a symbol run, a keyboard smash, a bound prefix `re-`, or a bound suffix `-ly`.
- **Shared spellings are legal.** Grammar position disambiguates first. If a spelling still has two meanings, a **Metalaw** must choose. That is a Law whose `targets` include `terminal-channel`, conditioned on `ambiguity.symbol` (and `ambiguity.slot`), writing a candidate from `ambiguity.candidates` into `ambiguity.resolved`. Without one, the spoken sentence is refused and the candidates are listed. While you type, the preview shows the first meaning and notes that a Metalaw decides when it is spoken; nothing is applied until you press Enter.
- **Scope for bare paths:** `@terminal-channel.scopeBeing` (the Law Line Zone sets it to the cube). When it's empty, bare paths complete against whatever you last clicked in the world (`@interaction-channel.focusedId`). Menu rows for properties show their live values (`hp = 3`).
- **The line's settings are registered properties** a Law may change: `menuRows`, `autoMenu`, `color` (off when `NO_COLOR` is set), `hints`, `relayLogs`, `prompt`.

## Next rungs

- [ ] **The Law Line in every Zone.** Laws are per-Zone membership (`lawRefs`), so the hearing Laws live in `LawLine`. Carrying them everywhere needs a Home/Ourverse carrier. ⚑ Zach's call.
- [ ] `set x to @other.path`: waits for the PropertyPath binding algebra ("copy value", [Property_Storage_and_OntoMath_Binding](../../Rendering%20and%20OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md)). The refusal already points there.
- [ ] OntoMath expression text, so Map/Flow/Drive/Zone conditions have a sentence form.
- [ ] Timeline clauses (after the Law/Timeline ontology; TIME_AND_MOMENT.md).
- [ ] Multi-line Python-style blocks (`when any:` / `then:` with indentation) for long chains. The editor would need Shift-Enter or a trailing `:` continuation.
- [ ] Undo of the last spoken Law as an authored act (a Lexeme denoting a Law that retires a Law), not a verb.
- [ ] Event meanings in the menu. Today an event shows how often it was heard. A meaning needs an authored home, not a central table.
- [ ] Migrate the legacy `earthcall_terminal` features (robot guy, word art, zone radar, lexeme constellation) onto the TerminalChannel, then retire it.
- [ ] In-world text entry. Zach deferred this: "We don't want it in-world" for now.

## Pitfalls for the next agent (Jules especially)

- **Do not add a verb, window, or event-name constant.** Every one of those was rejected while this was designed. The terminal only senses a line; seeded Laws decide that a line is spoken (`law-line-hear`, `law-line-speak`).
- **Do not add a word to `canonicalWords()`** unless it is a structural word or an engine opcode spelling. Meaning belongs in Lexeme → `denotes` → Law data. More words means patching the seed (below), not C++.
- **Do not add fields to `ActionNode` to make "set from path" work.** That belongs to the binding movement.
- **Seed changes follow FIRST_MOVER_AUTHORING §7.** `scripts/seed_law_line.py` is a patcher:
  - it creates only missing Law files and leaves any existing one alone;
  - it only *adds* Lexemes, Relations, and lawRefs to the Zone, verifies every original entry is untouched, and backs up the old file to `saves/backups/`;
  - it replaces the file atomically;
  - running it twice changes nothing.
- **Opcode, value, and preset Laws are disabled on purpose.** They are meanings, not actors. The Zone loader allows a *disabled* OnEvent Law without a trigger for this reason (`ZoneManager.cpp`).
- **Order of the frame matters.** `TerminalChannel::sense()` runs before `LawManager::tick()`, and `act()` runs after it (`Engine.cpp`). Moving either breaks the one-frame line → Law path.
- **The editor is ours, not libedit's.** libedit's Tab can only print matches into the scrollback and has no selectable menu, so `LineEditor` owns the region and redraws it in place with ANSI (the way Claude Code/Ink, fish, and zsh menu-select do). It is pure and testable (`line_editor_test`); `TerminalChannel` only feeds it bytes and writes its frames.
- **The terminal is held in raw mode** (no `ICANON`/`ECHO`/`ISIG`/`IEXTEN`, no `ICRNL`/`IXON`). This is why Ctrl-C reaches the editor rather than killing the app. The Person's settings are captured before anything changes and restored on every exit path: quit, Ctrl-D, Ctrl-C twice, and fatal signals.
- **App output is relayed.** stdout/stderr are redirected into a pipe. A reader thread only *reads* it and appends to the session log; the main thread prints the lines above the region. Never let that thread write to the terminal, or it will tear the line. `relayLogs = false` turns this off at attach time.
- **Live reading must never act.** The per-frame vocabulary replaces the Metalaw resolver with "first meaning, decided when spoken". Metalaws are applied only by `speak()`.
- **A suggestion must never glue onto a finished word.** An empty tail offers "what comes next" only after a space. The running app showed `set co⇥` becoming `set cofalse` before this rule existed; `line_editor_test` holds it.
- **`earthcall_webgpu` compiles its own sources.** A link or define added only to `earthcall_core` does not reach the app.
- **One channel attaches, and only to a TTY.** Under ctest nothing attaches, which is why the tests use `inject()`, `setSink()`, and the pure `LineEditor`.

## Evidence

- `tests/singularity/line_editor_test.cpp` covers:
  - key decoding (arrows, word jumps, split escape sequences, UTF-8, bracketed paste, lone Esc);
  - the menu (auto-open, ↑/↓ with wrap, Tab/Enter accept, fuzzy matching, shared-prefix extension);
  - no gluing;
  - editing, history, the ghost, and Ctrl-R;
  - Ctrl-C/Ctrl-D outcomes;
  - the rendered frame (selection marker, footer, error caret, role colours).
- `tests/singularity/law_line_test.cpp` covers the grammar, spelling forms, presets, Metalaw resolution, refusals, Tab, and the channel end to end.
- `tests/singularity/law_line_zone_test.cpp` (22 checks) runs the real seed through the real Zone loader:
  - a spoken Law turns the cube red;
  - `when hovered then set color gold` turns it gold on hover;
  - value words and shared-spelling "on" are read correctly;
  - Save Zone keeps the Law, every Lexeme, and every `denotes` Relation.
- **Driven in the real app** through a pseudo-terminal rendered by a terminal emulator (`pyte`, 100×30). Confirmed:
  - the menu follows typing; ↓ moves the `▸`; Tab takes it;
  - the panel shows the live preview, `next: …`, and an error with a caret only after the word is finished;
  - history ghost text;
  - one permanent echo line per Enter;
  - Ctrl-C clears, warns, then quits, and the Person's terminal settings are restored;
  - app `std::cout`/`std::cerr` output appears above an intact half-typed line and lands in the session log.

- **Full suite (rung 2):** 232 of 246 pass. Of the 14 failures:
  - 11 also fail on the tree without the Law Line (chess ×5, synthesis_studio, gpu_mastery, slow_adapter_zone_perf, webgpu_perlin_exact_gradient, prism_cathedral, zone_boot_hydration_relations; checked against a clean HEAD worktree);
  - `frame_lag_test` LAGs at HEAD too;
  - `substrate_split_test` and `matter_scoped_writer_test` turned red with PR #274 ("couple matter generation to semantic-root commit"), which changed `.ecmatter` generation after rung 1. Nothing in the Law Line touches that path.

*Claude Code · Claude Opus 5.5 · session `01WXmPy9U71FLqizbRYzMToZ` · 2026-09-25.*
