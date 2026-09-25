# The Law Line — author Laws as sentences in the Mac Terminal

**Origin:** Zach, `docs/architecture/law/Natural Language Law Authoring.md` (2026-09-25).
**Design and record:** [`docs/plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md`](../../../../../plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md)

## Status

Rung 1 is built and tested (2026-09-25). The Terminal window that `Run Earthcall.command` opens reads sentences into Laws while the app runs. The in-app witness is open in the Person Verification List.

## How to use it (what Zach will do)

1. Run `Run Earthcall.command`. Keep its Terminal window visible beside the app. It prints *"The Law Line is listening…"* and an `earthcall>` prompt.
2. In the app, switch to the **Law Line** Zone. Its seeded Laws are what hear the terminal; in any other Zone the terminal tells you nothing hears it.
3. Type, pressing Tab as you go:
   `my event-triggered law called Red fires on object-clicked if hp is greater than 2 then set color 1 0 0`
   - Tab completes words, presets, events, and paths (`@law-line-cube.` then Tab lists the cube's properties). Double-Tab lists every option.
   - End with `?` to preview without authoring. Start with `??` to search, e.g. `?? color`.
4. Press Enter. The terminal prints `authored law_<uuid> (written by <you>)` and the law read back as a sentence. Clicking the Law Line Cube turns it red. Save Zone keeps the Law.

## Where meaning lives (so you can grow it without code)

- **A word is a Lexeme that `denotes` a Law.** Add a Lexeme, add a Law, and relate them with `denotes`; the word works immediately.
  - The Law's shape is the opcode. A Set with no path is an action word. A Compare with no path is an operator.
  - A Law with no open slot is a **preset** that fixes its activation, scope, triggers, and its condition or action. The empty `All()` fixes "no condition".
- **A Lexeme is any unit:** a phrase ("my law that fires when it becomes true"), a symbol run, a keyboard smash, a bound prefix `re-`, or a bound suffix `-ly`.
- **Shared spellings are legal.** Grammar position disambiguates first. If a spelling still has two meanings, a **Metalaw** must choose. That is a Law whose `targets` include `terminal-channel`, conditioned on `ambiguity.symbol` (and `ambiguity.slot`), writing a candidate from `ambiguity.candidates` into `ambiguity.resolved`. Without one, the sentence is refused and the candidates are listed.

## Next rungs

- [ ] **The Law Line in every Zone.** Laws are per-Zone membership (`lawRefs`), so the hearing Laws live in `LawLine`. Carrying them everywhere needs a Home/Ourverse carrier. ⚑ Zach's call.
- [ ] `set x to @other.path` — wait for the PropertyPath binding algebra ("copy value", [Property_Storage_and_OntoMath_Binding](../../Rendering%20and%20OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md)); the refusal already points there.
- [ ] OntoMath expression text, so Map/Flow/Drive/Zone conditions have a sentence form.
- [ ] Timeline clauses (after the Law/Timeline ontology; TIME_AND_MOMENT.md).
- [ ] Multi-line Python-style blocks (`when any:` / `then:` with indentation) for long chains.
- [ ] Migrate the legacy `earthcall_terminal` features (robot guy, word art, zone radar, lexeme constellation) onto the TerminalChannel, then retire it.
- [ ] In-world text entry, and clicking a Singular to fill an argument. Zach deferred this: "We don't want it in-world" for now.

## Pitfalls for the next agent (Jules especially)

- **Do not add a verb, window, or event-name constant.** Every one of those was rejected while this was designed. The terminal only senses a line; seeded Laws decide that a line is spoken (`law-line-hear`, `law-line-speak`).
- **Do not add a word to `canonicalWords()`** unless it is a structural word or an engine opcode spelling. Meaning belongs in Lexeme → `denotes` → Law data.
- **Do not add fields to `ActionNode` to make "set from path" work.** That belongs to the binding movement.
- **Seed changes follow FIRST_MOVER_AUTHORING §7.** `scripts/seed_law_line.py` only creates missing files and refuses partial overwrites. To change a seeded Law, patch that file; never regenerate the Zone.
- **Opcode and preset Laws are disabled on purpose.** They are meanings, not actors. The Zone loader allows a *disabled* OnEvent Law without a trigger for this reason (`ZoneManager.cpp`).
- **Order of the frame matters.** `TerminalChannel::sense()` runs before `LawManager::tick()`, and `act()` runs after it (`Engine.cpp`). Moving either breaks the one-frame line → Law path.
- **libedit is process-global.** Only one TerminalChannel attaches, and only when stdin and stdout are TTYs. Under ctest nothing attaches, which is why the tests use `inject()` and `setSink()`.
- **Character mode is held deliberately.** libedit's callback interface enters character mode only inside `el_gets`, which a terminal still in line mode never reaches before Enter, so Tab would silently do nothing. `holdCharacterMode()` keeps `ICANON`/`ECHO` off, but not `ISIG`, so Ctrl-C still works. It re-asserts after every line, and `restoreTerminal()` puts the Person's settings back at exit. Ctrl-D is *not* an end-of-file here (libedit's callback mode does not deliver it); closing the app is how the line ends.
- **`earthcall_webgpu` compiles its own sources.** A link or define added to `earthcall_core` does not reach the app. libedit is linked into both targets in `CMakeLists.txt`; the first in-app run was silent until that was found.
- **Verified in a real terminal** by driving `earthcall_webgpu` through a pseudo-terminal: the prompt attaches, `on tick then Destr⇥` completes to `Destroy`, a response leaves one clean prompt, and a Zone without the hearing Laws answers "no Law in this Zone hears…". Ctrl-C was verified to hand the terminal back in line mode with echo on. The Person's settings are captured *before* libedit touches the terminal and restored on every exit path, including fatal signals.

## Evidence

- `tests/singularity/law_line_test.cpp` covers the grammar, spelling forms, presets, Metalaw resolution, refusals, Tab, and the channel end to end.
- `tests/singularity/law_line_zone_test.cpp` (19 checks) runs the real seed through the real Zone loader:
  - a spoken Law, authored by the Person, turns the cube red;
  - Save Zone keeps the Law, every Lexeme, and every `denotes` Relation.

*Claude Code · Claude Opus 5.5 · session `01WXmPy9U71FLqizbRYzMToZ` · 2026-09-25.*
