# Natural Language Law Authoring — the Law Line (terminal-first)

**Status:** rung 1 implemented and tested (2026-09-25). In-app witness pending; see the Person Verification List.
**Origin:** Zach, [`docs/architecture/law/Natural Language Law Authoring.md`](../architecture/law/Natural%20Language%20Law%20Authoring.md) (2026-09-25), plus seven corrections Zach gave while this was being planned and built. They are quoted below because each one changed the design.
**Task:** [Law_Line](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md)

---

## 1. What Zach asked for

Author a Law by typing one sentence, slash-command style. Tab auto-fills. There is a search, a way to view options, and **First-Mover-preconfigured presets** ("prefigured Singulars and Laws, not hard-coded") that fix some arguments and show only what is still open. The sentence is built from opcodes (the ConditionNode/ActionNode kinds) and their arguments, Python-like, with no brackets. The motivation is the cost of the ImGui Law Graph (Zach's O(c + mk + np) mouse arithmetic) and the Second-Nature Forge, which "turned out as a blue button that spams blue square laws". The Law Graph stays for debugging, the library, logs, and long chains.

## 2. Zach's corrections, and what each one changed

| # | Zach said (2026-09-25) | Consequence |
|---|---|---|
| 1 | "BRUHHHHHHHH REFUSAL #1 REMEMBER I SAID PRECONFIGURED NOT BESPOKE" | No bespoke ImGui window, no `law line` verb, no command system. |
| 2 | "implement this in the terminal first … Make something like a TerminalChannel in the Singularity … control WORLD from the MAC COMMAND CENTER … Keep the current terminal as a legacy separate system" | `Singularity/Terminal/TerminalChannel` is a modality channel of the **running app**. `earthcall_terminal` (`src/terminal_entry.cpp`) is untouched. |
| 3 | "dont rely on SecondNatureAuthoring … if it relies on a privileged set of bespoke semantics … Law sentence spoken needs to be an authored event" | No C++ subscriber to a privileged event name, and no header of event constants. `law-sentence-spoken` appears **only in seeded law text**. The channel acts when an authored Law writes its registered `speakRequests`. |
| 4 | "A JSON string overlap with same-string should not resolve to the typed value in a hardcoded way" | No `$HOLE` token substitution. Presets fix **clauses**; the sentence fills the rest. |
| 5 | "Follow the precedent of shared-name legitimacy where it is individuated by lexeme ID" · then: "resolved by metalaw as to which meaning it uses … refuse if no such resolving metalaw" | Display names may be shared. A spoken Law gets a minted `law_<uuid>` id. When a spelling stays ambiguous, **a Metalaw decides**; with none, the sentence is refused and the candidates are listed. |
| 6 | "operandPath needs to be considered alongside the broader PropertyPath-Memory/Opcode memory mastery movement" | Nothing was added to `ActionNode`. `set x to @y.z` is refused, and the refusal points at PROPERTY_STORAGE §2 "copy value". |
| 7 | "We only want a LawSentence to parse the modality channel of the Mac Terminal's command line. We don't want it in-world. Lexeme-as-opcode really should mean Lexeme <--Relation--> Law (the law is where the executable opcodes live)" · "a Lexeme is not strictly 'one word' … any semantic language unit, whether a word, phrase, a sentence, prefix/suffix, or keyboard smash" | The grammar lives in `Singularity/Terminal/`. A word means the **Law its Lexeme `denotes`**. Spellings are matched on characters: phrases, glued symbols (`hp>=3`), bound prefixes (`re-`), bound suffixes (`-ly`), and arbitrary strings all work. |

## 3. The design as built

```
Sense   TerminalChannel (C++)   libedit on stdin → level `lastLine`, edge `terminal-line-entered`
Decide  seeded Laws             on terminal-line-entered → Publish "law-sentence-spoken"
                                on law-sentence-spoken  → Add @terminal-channel.speakRequests 1
Act     TerminalChannel (C++)   when speakRequests advances: read lastLine with LawSentence,
                                author the Law (author = the being at authorPath, default
                                @interaction-channel.personId), bind triggers, join the Zone
Show    TerminalChannel         prints what is written to its `output`
```

- **Where meaning lives.** `LawSentence::classify(law)` reads a denoted Law's models:
  - an action whose slot is open (Set with no path, Publish with no event…) is an action word;
  - a condition whose slot is open (Compare with no path, IsKind(AnyBeing)…) is an operator or condition word;
  - a Law with no open slot is a **preset**: it fixes activation, scope, triggers, and a condition or action. An empty `All()` fixes "no condition".

  Several presets compose when they agree.
- **Invariants in C++** (the bootstrap):
  - the engine's opcode spellings (`ActionNode::kindName`, ConditionNode kinds and Ops, BeingKind, Activation/Scope);
  - structural words: called / named / my law called / on / fires / if / when / with condition / then / and / or / to / by / about / within / true / false.
- **Metalaw resolution.** On a live ambiguity the channel sets `ambiguity.symbol/.slot/.candidates` and applies every Law whose `targets` include `terminal-channel`. It accepts whatever candidate one of them wrote to `ambiguity.resolved`, then clears the question.
- **Named refusals, never silent drops.** These refuse and say why:
  - Map/Flow/Drive/InRegion/Zone/quantifiers (no OntoMath text parser yet) → "author it in the Law Graph";
  - Timeline clauses → wait for the Law/Timeline ontology;
  - `set … to @path` → waits for the binding algebra;
  - no author → "nothing enters the world without one";
  - a Zone where no Law hears the terminal → the terminal says so.
- **Tab** runs on the main thread. The libedit callback interface is pumped with a zero-timeout `select()` in `sense()`, so there is no reader thread and no snapshot of the vocabulary; completion reads the live world. The completion keeps the Person's typed case. Double-Tab lists candidates (the "view-options"). A trailing `?` previews; `?? word` searches.

## 4. Deliberately not built (rung 2+)

These are listed in the task doc:
- in-world text entry and display, and click-a-Singular-to-fill (Zach: not in-world for now);
- `set … to @path` (the binding movement);
- OntoMath expression text (Map/Flow);
- Timeline clauses;
- the Law Line in every Zone (today it lives in the `LawLine` Zone, because Law membership is per-Zone; carrying it everywhere is a Home/Ourverse question for Zach);
- migrating the legacy terminal's features (robot guy, word art, zone radar, lexeme constellation).

## 5. Implementation record

- **New:**
  - `src/Singularity/Terminal/{LawSentence,TerminalChannel}.{hpp,cpp}`
  - `tests/singularity/law_line_test.cpp` (grammar, and the channel end to end)
  - `tests/singularity/law_line_zone_test.cpp` (the real seed through the real Zone loader)
  - `scripts/seed_law_line.py`
- **Edited:**
  - `Engine.cpp`: `sense()` runs before `LawManager::tick()`, `act()` after it.
  - `EngineInit.cpp`: `syncRegister`.
  - `CMakeLists.txt`: system libedit plus `EARTHCALL_HAS_LIBEDIT`.
  - `ZoneManager.cpp`: an OnEvent Law with no trigger is refused only when **enabled**. A disabled Law is a source, and nothing wakes it.
- **Seeded (new files only; `authors: Zach`, `injected_by: Claude Opus 5.5`):**
  - `saves/zones/LawLine/zone.json`: the Law Line Cube, 53 Lexemes, and 53 `denotes` Relations;
  - 25 `saves/laws/law-line-*/law.json`: 17 opcode Laws, 4 presets, and 3 wiring Laws (hear, speak, scope-to-cube). All opcode and preset Laws are disabled.
- **Found by running the real app** (driven through a pseudo-terminal, 2026-09-25):
  - `earthcall_webgpu` compiles its own sources, so libedit had to be linked into it as well.
  - libedit's callback interface cannot see Tab while the terminal is in line mode. The channel therefore holds character mode itself (`ISIG` kept) and restores the terminal at exit.
  - Ctrl-D is not delivered as end-of-file in callback mode.
- **Verified:**
  - `law_line_test` passes.
  - `law_line_zone_test` passes 19/19, including that Save Zone keeps every seeded Lexeme and `denotes` Relation.
  - The full suite and its pre-existing failures are recorded in the task doc.

## 6. Rung 2 — the ergonomic line (same day)

Zach, after witnessing rung 1: *"tab shouldn't just display a list of all possibilities above me as a message. Tab should be able to actually select one and arrow keys should be able to move between them … like claude code cli … it can show me stuff temporarily without sending as a full message … think about other design and ergonomics"*, and *"make it more intuitive to use with more laws and singulars"*.

- **libedit was replaced**, because it has no selectable menu and its Tab prints into the scrollback. In its place is `Singularity/Terminal/LineEditor`: pure state plus a rendered frame, redrawn in place under the prompt, the way Claude Code/Ink, fish, and zsh menu-select work. `TerminalChannel` holds the terminal in raw mode, decodes keys (`KeyDecoder`), and draws frames in one synchronized write. It also relays the app's stdout/stderr above the region, through a read-only reader thread that also writes the session log.
- **Ergonomics (Claude's design, from Zach's request):**
  - a live fuzzy menu with meanings, where Tab/↑↓/Enter/Esc select;
  - ghost text from the selection or from history, with → to take it;
  - the line coloured by how it is read;
  - a transient panel: preview, `next: …`, an error with a caret only once a word is finished, live `??` search, and a key-hint footer;
  - persistent history and Ctrl-R;
  - bracketed paste;
  - Ctrl-C clears, then quits on a double press;
  - property rows show live values;
  - bare paths complete against the authored scope, or whatever was last clicked.

  The line's settings are registered properties.
- **More authored words** (data only, patched into the LawLine Zone):
  - value words: a Lexeme denoting a Set Law with no path but a value — 13 colours, plus on/yes/off/no;
  - trigger presets: when clicked, on hover, when the pointer leaves, when touched, when I land, when I jump, when the zone opens.

  `classify()` learned "value". `seed_law_line.py` became a patcher: it creates only missing files, only adds to the Zone, verifies the original is intact, backs it up, and replaces atomically.
- **Found by driving the real app in an emulated terminal** (`pyte`) and fixed:
  - next-word suggestions glued onto a finished word (`set co⇥` → `cofalse`);
  - red errors appeared on the word still being typed;
  - Law-Graph-only opcodes crowded the menu;
  - accepting `se` produced the engine's `Set` instead of the Person's `set`;
  - a recalled history line opened a menu that hijacked ↓.

  Each is now held by `line_editor_test`.

## 7. Rung 3: mouse, blanks, help, footer, dry run, confirmed deletion

**Zach's choices (2026-09-25):**
- mouse scrolling of the Tab lists;
- "the rest are great and we should do them now";
- typo-fixing and usage ranking deferred ("we need more robust Lexeme Formations first");
- "add a help command and make it aesthetic/beautiful";
- deletion: *"the metalaw that does the deletions should say 'are you sure you want to delete?' and then either yes or no saying no means no delete and requires your yes to delete."*

**What was built:**
- **Deletion** runs through two seeded Metalaws (`law-line-ask-before-deleting`, `law-line-delete-when-confirmed`) over existing opcodes. There is no new ActionNode kind:
  - Destroy now accepts a Law victim;
  - `LawManager::reapUnmade` retires it;
  - `ZoneManager::retireLawFromActiveZone` makes Save Zone drop it from `lawRefs`.
- **The editor gained:**
  - SGR mouse reporting (on only while a menu or help is open) with cursor-report row mapping;
  - PgUp/PgDn and F1;
  - `‹blank›` placeholders derived from each opcode's own argument signature (`argumentTemplate`), plus a preset's still-open clauses;
  - grouped menus, bold matched letters, and a `⤷` detail line;
  - a status footer;
  - a scrollable overlay for `help`.
- **The dry run** evaluates the compiled condition read-only.

**Verified:**
- `line_editor_test`, `law_line_test`, `law_line_zone_test` (40 checks), and `destroy_law_test`;
- the real app, driven in an emulated terminal: the help box and its wheel scrolling, mouse off after close, the grouped menu, a wheel-moved selection, a click that took `Add` with its blanks, and the blanks filled by typing and Tab.

---

*Claude Code · Claude Opus 5.5 · session `01WXmPy9U71FLqizbRYzMToZ` · 2026-09-25T02:25-07:00. Zach originated the Law Line, its grammar, presets, Tab, the terminal-first order, the TerminalChannel, the legacy split, and corrections 1–7 above. Claude originated: the libedit callback-interface pump, reading a denoted Law's shape as its opcode (open slot vs preset), composable presets, the request-counter invocation, Metalaw application by `targets`, character-level spelling forms, and the disabled-Law loader refinement.*
