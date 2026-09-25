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

---

*Claude Code · Claude Opus 5.5 · session `01WXmPy9U71FLqizbRYzMToZ` · 2026-09-25T02:25-07:00. Zach originated the Law Line, its grammar, presets, Tab, the terminal-first order, the TerminalChannel, the legacy split, and corrections 1–7 above. Claude originated: the libedit callback-interface pump, reading a denoted Law's shape as its opcode (open slot vs preset), composable presets, the request-counter invocation, Metalaw application by `targets`, character-level spelling forms, and the disabled-Law loader refinement.*
