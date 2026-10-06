# The Law Line — author Laws as sentences in the Mac Terminal

**Origin:** Zach, `docs/architecture/law/Natural Language Law Authoring.md` (2026-09-25).
**Design and record:** [`docs/plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md`](../../../../../plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md)

## Status

- **Practical guide (2026-10-05):** [Writing Earthcall Laws for humans and agents](../../../../../architecture/law/LAW_AUTHORING_CLI_GUIDE.md) supplies beginner lessons, creation and property recipes, ordered programs, all 26 action kinds, troubleshooting, and the authored compiler workflow; see [the guide record](#authoring-guide-2026-10-05).

- **Invisible Create fix (2026-10-05):** Zach's Objects and face assets existed but neither below-me nor view-relative cubes appeared. The birth resolver preferred inactive `World` over the active rendered Zone. That name preference is removed; the boot harness now includes inactive Zones and a full Engine/Terminal/Metalaw viewport capture shows the gold probe. Restart the rebuilt app to load the correction; [audit and native evidence](../../../../../audits/LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md). Existing Person-authored Laws and saves were not edited.
- **Person confirmation and sky spiral (2026-10-05):** Zach confirmed visible cube creation and the original Stairmaker now work; [the three-Law add-on](../../../../../../examples/law_line_sky_stairway.txt) grows branching eight-stone spirals with hover turns and colour restoration, verified alongside the original program through 162 focused checks and a full native Engine capture ([audit](../../../../../audits/SKY_STAIRWAY_AUTHORED_PROGRAM_2026-10-05.md)).

- **Rung 1 (2026-09-25):** built. Zach witnessed it: the "Red" sentence authored a Law, and the Law Graph showed the object-clicked trigger, `hp > 2`, and `set color 1 0 0`.
- **Rung 2 (2026-09-25, same day):** an ergonomic line, and more authored words.
  - Zach asked for: "tab should be able to actually select one and arrow keys should be able to move between them … like claude code cli … show me stuff temporarily without sending as a full message … think about other design and ergonomics".

## General Create initializers compiled by Metalaw — 2026-10-04

Zach asked for `Create <SingularKind, properties: {…}>` containing existing and authored properties, and explicitly required Metalaws to compile the Create Lexemes into the final Law. This extends the existing `Create`/prototype operation; it does not add a spawn-below-me verb or an action-kind enum.

Type a single line in `LawLine`:

```text
called Beneath Me when clicked then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold, authored: {purpose: "A foothold"}}>
```

Clicking an object supplies the trigger. The newborn's **centre** takes the speaking author's current `position` minus three on Y at each firing, rather than a fixed coordinate captured when the sentence was entered. In the production locomotion channel that Person position is at the feet, with the camera above it by Body.eyeHeight; the newborn can therefore be below the floor. Add `if Identity @law-line-cube` before `then` to listen only to clicks on the seeded cube. Add `?` to inspect syntax without compiling or creating a Law. A syntax preview explicitly says compilation is deferred; it does not claim to have evaluated Metalaws. Save Zone retains the compiled Law and newborns.

Flat initializers, or those inside `registered: {…}`, address existing property paths (e.g. `shape.kind`, not `shapeKind`). `authored: {…}` grants properties through the ordinary AddProperty action. Values include authored value words, quoted strings, booleans, finite numbers, parenthesized vector triples, literal lists/dictionaries, qualified property reads, and `+`, `-`, `*`, `/` expressions lowered to the existing typed OntoMath tree. Lists remain lists; `(1, 2, 3)` is a vector. Container entries must be literals; general dynamic container construction/alias topology remains in the Property storage task. Use whitespace around arithmetic on qualified paths because hyphens are also identifier characters.

`Object` has an authored direct-birth rule. Other existing Singulars can use an explicit live prototype:

```text
when clicked then Create <@lexeme.law-line.gold.value-gold, properties: {authored: {purpose: "An authored branch"}}>
```

The existing universal adapter preserves the prototype's concrete kind and existing birth/authority refusals. An unconfigured kind spelling refuses; no constructor is guessed. Person/real-correspondent birth remains prohibited. Relation birth still needs explicit participants; this initializer surface does not yet provide participant slots or new identity/endpoint clauses.

The channel reads the generic parameter record for **any** action Lexeme, plus expression notation. Each sensed initializer and final invocation becomes a registered `compilation.input` dictionary on the Terminal Law. Ordinary authored Laws targeting `terminal-channel` select an action-model template by condition and write `compilation.template` or `compilation.error`. The channel only substitutes JSON-pointer `$slot` references in that authored template and validates/deserializes the resulting existing ActionModel. Six seeded compiler Metalaws decide registered literal → Set, expression → Map, authored literal → AddProperty, authored expression → AddProperty + Map, Object invocation → Create, and prototype invocation → prototype Create. Removing a rule refuses; conflicting outputs refuse regardless of register order. The final sentence still uses the shared existing enact/persistence path, including foreign authorship. This is not a migration of every legacy sentence opcode to Metalaw.

`Create`, `Cube`, and `my` are three new Lexemes denoting ordinary Laws. `Cube` denotes integer geometry value 0. The `my` word's denoted Law carries authored `sentence.root`; its `$author` value resolves to the actual speaker's identifier. A foreign sentence does not borrow the local Person's pronoun. `Create`'s completion signature is authored `sentence.arguments` data. Law identity serialization now retains authored properties so those declarations survive save/reload.

Future agents: edit the six `law-line-compile-*` models to change compilation policy; do not put a `kind == Create` lowering switch in the parser. Compiled creation keeps existing child-action execution semantics and TransferPolicy gates; arbitrary initialization is not a new transactional world-mutation system. Inspect the action trace when an initializer is refused at runtime. Preview/completion/submit preflight must never apply compiler Metalaws.

The seed added **nine Law roots, three Lexemes, three denotes Relations, and nine lawRefs**, authored by **Zach** and injected by **Codex / GPT-6.1 Sol**. Existing Zone bytes are preserved by insertion into its root arrays. Original bytes are retained under `scratch/backups/law-line/`; existing Law roots were not rewritten. Seed rerun is idempotent. The WebGPU app built and seven focused tests passed, with `law_line_zone_test` at 65/65 checks; live Person acceptance is pending. Source: `scripts/seed_law_line.py`; automated evidence and remaining Person checks are recorded in [the audit](../../../../../audits/LAW_LINE_CREATE_METALAW_2026-10-04.md) and [Person Verification](../../../For%20Zach/Person%20Verification%20List.md).

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-04 18:46 PDT — implementation of Zach's general Create notation and Metalaw compilation requirement.*

## Continuous cubes below the author — 2026-10-05

Zach reported that the supplied continuously firing Law was created but no cubes appeared. That example used `Identity @Zach`; Identity compares the exact `getIdentifier()`, while a keyed Person's identifier is a `did:earthcall:…` value, not their display name. The previous unkeyed test Person concealed this mismatch. Do not loosen Identity or treat a display name as an identity alias.

The corrected [single-line example](../../../../../../examples/law_line_cubes_below.txt) is:

```text
called "Cubes Below Me" always if is a Person then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold}>
```

`always` selects WhileTrue; the Person condition prevents each new Object from also becoming a spawning subject. In the current one-Person runtime it creates one cube each tick, with its centre three Y units below the actual speaking author. In a future multi-Person domain this condition matches each Person, while `my` still denotes the author; it is not an author-identity restriction. New cubes overlap while the author stands still and may be beneath the floor. This example is deliberately continuous, with no automatic stopping condition.

The regression uses a test-only keyed Person and the actual Terminal → authored compiler Metalaws → LawManager tick path: the old exact-name guard creates nothing, the corrected example creates exactly one cube on each of two ticks, and the second placement follows the changed author position. `law_line_zone_test` passed. No production engine code, user-authored Laws, or inhabited saves were changed for this correction. The engine bridge was offline; native visibility remains a Person check.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 10:25 PDT — correction of the example supplied in response to Zach's continuous creation request.*

### Native visibility witness — 2026-10-05

Zach subsequently confirmed continuous firing and increasing Object count while seeing no cube below him. The [native WebGPU audit](../../../../../audits/LAW_LINE_CUBE_VISIBILITY_2026-10-05.md) submits the real example in an isolated LawLine fixture and draws the actual newborn. With the camera looking straight down, the cube changes 1,452 framebuffer bytes without a floor, while placing a floor at feet height hides every cube pixel. This proves the placement can render correctly while remaining occluded; it is not a capture of Zach's scene. F toggles flight and Space ascends, so a Person can rise more than three units above the floor and look down to check newly created cubes. Do not change the compiler to move below-feet creation above the floor.

Zach also reported that `(0, 3, 0)` remained invisible. This keeps his scene diagnosis open: overhead placement can be outside the view. The separately tested `examples/law_line_visible_probe.txt` uses `becomes true` to create one cube at `my.cameraPos + my.cameraForward * 3`. With first-person perspective (1) and a level aim, it should appear directly ahead. The native probe verifies exact placement, no repeat birth on a second tick, and 2,393 changed framebuffer bytes with the floor present. Ask for the actual scene and newest Object position if this probe also remains invisible; do not claim this fixture resolves every live visibility failure.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 11:28 PDT.*

## How to use it

1. Run `Run Earthcall.command` and keep its Terminal window beside the app. It prints *"The Law Line is listening."* above an `earthcall>` prompt.
2. The **line** starts in the Law Line Zone. Your body can be anywhere (Terminal Zones, 2026-09-30). The prompt says where the line is: `earthcall[Law Line]>`. `enter Quiet` makes lines mean nothing, `enter Identity` makes you present by key, `enter LawLine` comes back, and `enter` lists Zones.
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

### Rung 3 (2026-09-25): mouse, blanks, help, footer, dry run, deletion

Zach chose these from a proposal; he deferred typo-fixing and usage ranking ("needs more robust Lexeme Formations first").

- **Mouse:** the **wheel** scrolls the menu (and help); **clicking** a row takes it; clicking in the line moves the cursor. Mouse reporting is on *only while a menu or help is open*, so the rest of the time the Terminal scrolls and selects text as usual.
- **Blanks:** taking a word lays out its own arguments as `‹blanks›`, and the first is selected. `set` gives `set ‹path› to ‹value›`; `when clicked` gives `when clicked then ‹action›`.
  - Typing replaces the selected blank, and the menu shows what fits it.
  - Choosing from the menu fills it and moves to the next blank.
  - Tab jumps between blanks.
  - Enter waits until none are left.
- **The menu explains itself:**
  - Tab on an empty word lists everything next, **in sections** (Presets, Actions, Comparisons, Conditions, Values, Events, Properties, Beings & Laws, Clause words);
  - the letters you typed are **bold**;
  - a `⤷` line explains the selected entry: what a preset fixes, a value word's value, a property's live value;
  - **PgUp/PgDn** page through it.
- **Footer:** the last line always says the Zone, whether it hears the line (◆ green / yellow), the scope, who you're writing as, and how many live Laws there are.
- **Help:** `help`, `?` on an empty line, or **F1** opens a page in the panel. It contains the sentence shape, three examples from this world's words, what words exist, the keys, and where you are. It scrolls, and Esc closes it. It is a reading of the terminal, not a Law.
- **Dry run:** ending with `?` also says who the IF holds for right now ("right now the IF holds for 1 being: Law Line Cube"). This is read-only.
- **Deleting a Law** (Zach: *"the metalaw that does the deletions should say 'are you sure you want to delete?' … no means no delete and requires your yes"*):
  - `delete Blue` (also `remove`, `destroy`, `@law_…`) asks a question: `Are you sure you want to delete “Blue”? (yes / no) ›`.
  - **Only a yes deletes.** `no`, Esc and Ctrl-C keep it.
  - When several Laws share the name, it lists them and asks for a number first.
  - The question's words live in the seeded Metalaw `law-line-ask-before-deleting`, so you can rewrite them. The deletion itself is done by `law-line-delete-when-confirmed` (`Destroy @event.object`).
  - The terminal only senses the request and the answer, and publishes `terminal-deletion-requested` / `terminal-deletion-confirmed`.
  - A deleted Law leaves the Zone's `lawRefs` on save. Its own file stays in `saves/laws/` as history.

### Stuck? The line tells you

(Added after Zach's first unguided session, 2026-09-25: *"HALP IDK HOW TO USE THIS"*. He had picked WritePixel, AddElement, AuthorZone and "always" from the menu, and every Enter was refused.)

- An **empty line** shows a `try:` sentence built from this world's own words, plus the sentence's shape: `[preset] [called <name>] [on <event> | when …] [if <condition>] then <action>`.
- While you type, the panel says **`next: …  e.g. …`** with a real example.
- **Enter on an unfinished sentence authors nothing.** The line stays and the panel says what to add (`not yet — add then <action>, e.g. then set color red`).
- **The menu only offers words that can work where you are.** Canonical actions with no open-slot sentence form (WritePixel, AddElement, AuthorZone, …) are not offered as bare verbs. A Lexeme denoting a **complete authored ActionModel** can now be offered as a fragment, with no argument blanks. Neither is a word that would contradict your sentence ("always" after "when they collide").

- **A Law only listens for events this world knows** (after Zach's `fires when Spawn …` made a Law waiting for an event called "when"). Known means heard this session, bound by a Law (the trigger presets bind the common ones), or published by a Law. An unknown name is refused, with the closest known events offered. A structural word (`when`, `then`, …) is never an event name. To mint a new event on purpose, quote it: `on "door-opened"`. `publish` may always name a new event. `fires when clicked` reads the trigger preset.

### Try these in the Law Line Zone

- `when clicked then set color gold`
- `when hovered then set color cyan`
- `on tick if hp is greater than 2 then add glow 0.1`
- `always if hp < 1 then set color gray`
- `?? color`

## Where meaning lives (so you can grow it without code)

- **A word is a Lexeme that `denotes` a Law.** Add a Lexeme, add a Law, and relate them with `denotes`; the word works immediately. The menu's description is the Law's name.
  - **Action/operator words:** a Law whose model has an **open slot**. A Set with no path is an action word ("set", "make"). A Compare with no path is an operator ("greater than").
  - **Complete action fragments (2026-09-27):** an action-only Law with no trigger, condition, non-default scope/activation, Drive flag, targets, or jurisdiction can contribute its exact ActionModel wherever an action is expected. `Create` with nested children and `WritePixel` work through the same `denotes` Relation; a Metalaw resolves a shared spelling. Parameterized Create and the channel-action words are now seeded (2026-10-04/05); complete closed fragments still retain their exact authored trees.
  - **Value words:** a Set with **no path but a value**. "red" → (1, 0, 0); "on"/"yes" → true; "off"/"no" → false. "on" is also the structural trigger word, and the grammar position tells them apart.
  - **Presets:** a Law whose clauses must be kept together. It fixes activation, scope, triggers, and any condition or action. Examples: "my event-triggered law", "always", "when clicked", "on hover", "when touched", "when I land", "when I jump". The empty `All()` fixes "no condition".
- **A Lexeme is any unit:** a phrase ("my law that fires when it becomes true"), a symbol run, a keyboard smash, a bound prefix `re-`, or a bound suffix `-ly`.
- **Shared spellings are legal.** Grammar position disambiguates first. If a spelling still has two meanings, a **Metalaw** must choose. That is a Law whose `targets` include `terminal-channel`, conditioned on `ambiguity.symbol` (and `ambiguity.slot`), writing a candidate from `ambiguity.candidates` into `ambiguity.resolved`. Without one, the spoken sentence is refused and the candidates are listed. While you type, the preview shows the first meaning and notes that a Metalaw decides when it is spoken; nothing is applied until you press Enter.
- **Scope for bare paths:** `@terminal-channel.scopeBeing` (the Law Line Zone sets it to the cube). When it's empty, bare paths complete against whatever you last clicked in the world (`@interaction-channel.focusedId`). Menu rows for properties show their live values (`hp = 3`).
- **The line's settings are registered properties** a Law may change: `menuRows`, `autoMenu`, `color` (off when `NO_COLOR` is set), `hints`, `relayLogs`, `prompt`.

## Next rungs

- [x] **The Law Line in every Zone**: superseded by [Terminal Zones](../../Interaction%20and%20Interface/Terminal_Zones/Terminal_Zones.md) (Zach's decision, 2026-09-30). The *line* holds the LawLine closure (`enter LawLine`) wherever the body stands, so no Home/Ourverse carrier is needed. Known limit, recorded there: LawLine's own beings (the cube, custom Lexemes) are visible to Laws only while the body is also in LawLine.
- [x] **A non-Person writer can now arrive as a First Mover** (Claude Opus 5.5, session `08b0f730-6e49-4c49-b27f-3a89c810ca4b`, 2026-09-30, at Zach's request so Sonnet 4.5 could use the Law Line): `TerminalChannel::authorForeign` parses with the channel's own grammar and vocabulary and authors as the proven mover. `speak` and it share one `enact()`. It is reached over MCP as `earthcall_law_sentence`, which is socket `law_sentence` behind ForeignActuationGuard (scope over the Law's path and the active Zone). `?`/`??` stay read-only. Witness: `mcp_authoring_surfaces_test`, `mcp_first_mover_bridge_test`. The stdin path below is unchanged.
- [ ] **The line trusts its stdin, and stdin can lie** (Astra's Crystal §13, conceded in *The Terminal That Was Built Writes Back*). Anything that types into the Terminal, including a test process, authors as the Person `@interaction-channel.personId` names. A non-Person writer should arrive as a registered First Mover under a Person's grant, the same discipline as MCP.
- [x] **Fixed 2026-09-30 when it actually happened** (Zach keyed, and every Law authored "Zach" stopped resolving, including the Identity Zone's own Law). `Identity::personAnswersTo` bridges an old author name to a keyed Person **only** through the migration ledger, the same bridge Home reclaim uses. Boot treats a legacy profile the ledger signed over to a keyed profile on disk as superseded. Witness: `terminal_zones_test` 7b. Mythos's `was-called` Claim would make the ledger itself signed. Original item: **A spoken Law must survive its author getting a key** (Mythos, Interaction as Law thread, 2026-09-25). Spoken Laws record the author by the identifier `@interaction-channel.personId` resolves to *now*. That is spelling-resolved causation, and it unbinds when Zach takes a key. Untested. The fix is the `was-called` Claim Mythos proposed in `Succession_Is_Not_In_The_World`.
- [x] `set x to @other.path` (2026-10-05, [record below](#set-to-a-path-copy-value-2026-10-05)): the binding movement's typed `Map` passthrough is its "copy value", so an authored compiler Metalaw lowers the read. There is no parser lowering and no `operandPath`.
- [ ] Extend arithmetic notation to Zone conditions and richer bounded-function authoring; Create initialization and named Map/Flow expressions plus Drive curve arguments are implemented.
- [ ] Timeline clauses (after the Law/Timeline ontology; TIME_AND_MOMENT.md).
- [ ] Multi-line Python-style blocks (`when any:` / `then:` with indentation) for long chains. The editor would need Shift-Enter or a trailing `:` continuation.
- [ ] Undo of the last spoken Law as an authored act (a Lexeme denoting a Law that retires a Law), not a verb. **Blocked (2026-10-05) on the same reference seam.**
  - The last Law the line spoke is held only as the *string* `@terminal-channel.lastCreated`.
  - Deleting "it" would mean treating a string as a being reference. That is the hardcoded string→typed coercion Zach refused (2026-09-25).
  - It waits for a reference/alias cell in [Property_Storage_and_OntoMath_Binding](../../Rendering%20and%20OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md).
  - Meanwhile, `delete <name>` (with confirmation) undoes any spoken Law.
- [ ] Event meanings in the menu. Today an event shows how often it was heard. A meaning needs an authored home, not a central table.
- [ ] Migrate the legacy `earthcall_terminal` features (robot guy, word art, zone radar, lexeme constellation) onto the TerminalChannel, then retire it.
- [ ] In-world text entry. Zach deferred this: "We don't want it in-world" for now.

## Set to a path: copy value (2026-10-05)

The rung Zach left open on 2026-09-25 was "operandPath needs to be considered alongside the broader PropertyPath-Memory movement". Since then, the [Property storage task](../../Rendering%20and%20OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md) landed typed live bindings: a scalar `Map` passthrough (`ValueLeaf`) copies a typed value. That *is* its "copy value", so this rung reuses it and builds no new substrate.

```text
called "Copy Hp" when clicked then set @law-line-cube.glow to @law-line-cube.hp
called "Double Hp" when clicked then set glow to @law-line-cube.hp * 2
```

- **The parser:** when Set's value starts with `@` or an authored root (`my.`), the parser senses the expression and sends a `{"slot": "assignment", "opcode": "action.Set", "property", "function", "bindings"}` record through the same Metalaw compiler seam Create initializers use.
- **The Metalaw:** the saved Metalaw `law-line-compile-assignment-expression` turns that record into Map.
  - Disable or remove it, and such a sentence refuses. There is no fallback.
  - Literal Sets (`set glow to 1`, `set color red`) keep their exact existing model and never consult a compiler.
- **Tab** after `to ` offers both value words and paths.
- **Semantics:** the copy happens *when the Law fires*, as a value. It is not a live alias: later changes to `hp` do not flow into `glow` until the Law fires again. Aliases and shared cells remain the Property storage task's open work.

**Seed:**
- `scripts/seed_law_line.py` created `saves/laws/law-line-compile-assignment-expression/law.json`.
  - Its author is Zach's keyed identity `did:earthcall:dmvokv…`, read from the world's own `law-line-hear`, because his in-app save now records that identity.
  - It is `injected_by` Claude Opus 5.5.
- The script appended its id to LawLine's `lawRefs`. Nothing else changed: no Lexemes, no Relations, and no existing entry.
- LawLine is now saved natively (`zone.ecform`, Zach's commit `e4d7a72d`). So the patcher now edits the JSON inside the msgpack `MigrationRoot` wrapper with the same byte-preserving append. It backs up to `scratch/backups/law-line/` and renames atomically, and it would refuse any other ecform layout.
- `EARTHCALL_SEED_ROOT` lets a dry run patch a scratch copy first. That was done before touching the world.

**Witness:** `law_line_zone_test` 169/169. It covers:
- the Map lowering;
- the live copy;
- arithmetic over a read;
- the literal Set unchanged;
- refusal without the Metalaw;
- Tab offering the read path.

The test itself was repaired for Zach's native save. It reads `zone.ecform` and gives its scratch Person the seed's keyed author. It also quiets Zach's own spoken `law_<uuid>` Laws in the scratch copy, because his Stairmaker answers every click.

*Claude Code · Claude Opus 5.5 · session `01WXmPy9U71FLqizbRYzMToZ` · 2026-10-05.*

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
- **Destroy may now unmake a Law** (never a First Mover). `LawManager::reapUnmade` retires it *after* retracting its facts. `ZoneManager::retireLawFromActiveZone` records the retirement per Zone, because Save Zone otherwise only ever *appends* to `lawRefs`. Held by `destroy_law_test` and `law_line_zone_test`.
- **The deletion Metalaws carry no `targets`.** Laws targeting `terminal-channel` are applied by the ambiguity resolver, and a deleting Law must never run there.
- **Mouse reporting and cursor reports:** a click needs the terminal's answer to `ESC[6n` to know the region's screen row. Real terminals answer it; a test emulator must be told to (see the rung-3 probe). Reporting must be switched off on every exit path, including the signal handler.
- **Never look a being up per menu item.** `findBeing(id)` rebuilds and scans the whole Universe, so calling it once per offered being made `@` completion O(N²). At 708 beings in Zach's LawLine save, that was 530 ms per keystroke (Zach, 2026-10-05: "whenever I enter @ its really laggy").
  - `vocabulary()` now records each being's description as a string during its single walk, and `describedPropertiesOf` reads all of a being's properties from one lookup. Each keystroke now takes about 20 ms.
  - Never cache `Singular*` in a vocabulary: it can outlive a tick, and Laws and Relations may leave without a structural-revision bump.
  - Held by the `'@' completion is not quadratic` check in `law_line_zone_test`.
- **Pad by visible width, never bytes.** `·`, `‹` and `›` are several bytes (the help page's key rows broke once).
- **Never touch the Person's history when probing.** Set `EARTHCALL_TERMINAL_HISTORY` to a scratch file for any automated run. An agent's probe clean-up once deleted Zach's `saves/logs/terminal-history.txt` along with its own lines.
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

## Ordered sentences and property vocabulary (2026-10-04)

Zach requested multiple Law sentences in one submitted line, registered in the order written, plus add/remove/modify property. Separate complete sentences with top-level `;`:

```text
called "Add Note" when clicked then add property @law-line-cube.note to "hello"; called "Change Note" when hovered then modify property @law-line-cube.note to "updated"; called "Remove Note" when the pointer leaves then remove property @law-line-cube.note
```

The saved `law-line-compile-sentences` Metalaw conditions on `compilation.input.slot = "sentences"` and `opcode = "sentence.batch"`, and supplies `{"sentences":{"$slot":"/sentences"}}` through `compilation.template`. The channel senses delimiter structure, asks the existing generic Metalaw template seam for the document, validates the exact once-only source order, parses every sentence, and registers through the existing `enact` path. Missing/disabled compiler, conflicting outputs, empty sentences, or invalid later syntax refuses registration. Compiler provenance is retained on every new Law. There is no automatic batch fallback. Registration order does not promise event agenda firing order; use `then ... and ...` within one Law when ordered action execution is required.

A trailing `?` previews the whole batch without applying compiler Metalaws or registering Laws. Quoted semicolons and separators inside `()`, `[]`, or `{}` are preserved. Completion follows the final sentence. Immediate deletion/search cannot be mixed into a batch. Foreign `law_sentence` remains one separately authorized identifier per submission and explicitly refuses batches before compiling them.

`add property` denotes the existing AddProperty Law, `remove property` denotes RemoveProperty, and `modify property` denotes Set. Add grants an authored property; modify uses existing Set/path semantics. A never-addressable missing path refuses; an authored accessor materialized before removal remains addressable and Set can restore its erased value. Remove erases authored properties; registered engine paths retain their existing clear-value behavior and authority gates. These aliases preserve numeric `add` and existing `remove`/Destroy spellings. Property changes happen when the resulting Law fires, not on submission. No new action kinds were added.

All sentences are parsed before any are registered. Registration itself retains existing per-Law adoption semantics: a later adoption refusal reports how many earlier sentences registered; this is not a transactional mutation promise. Save Zone is still required to keep newly authored Laws after restart.

The seed append adds three Lexemes and their denotes Relations to the existing LawLine Zone and adds the sentence compiler Law root. Zach is the recorded author; Codex is the injector. Old Zone bytes are preserved, backed up outside saves, and atomically patched; existing Law roots are kept.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-04 19:46 PDT — implementation of Zach's ordered sentence and property vocabulary request.*

## Named arguments for the remaining action kinds (2026-10-05)

Zach requested argument forms for the missing action-operator kinds. Sixteen new Lexemes denote ordinary action Laws carrying authored `sentence.arguments` signatures; sixteen `law-line-compile-args-*` Metalaws compile their argument records. Their signatures and mapping into ActionModel fields live in saved authored data, not a per-kind parser branch.

| Operator | Argument form |
|---|---|
| Lerp | `<path: "‹path›", operand: ‹value›, factor: ‹number›>` |
| Drive | `<path: "‹path›", curve: {form: 1, coeffs: [0, 1]}, input: "‹path›">` |
| Sequence | `<children: [‹action›, ‹action›]>` |
| Parallel | `<children: [‹action›, ‹action›]>` |
| Map | `<path: "‹path›", expression: ‹expression›>` |
| Flow | `<path: "‹path›", expression: ‹rate expression›>` |
| AddElement | `<container: "‹being›", element: "‹being›">` |
| RemoveElement | `<container: "‹being›", element: "‹being›">` |
| Synthesize | `<children: [‹action›, ‹action›]>` |
| PlayAudio | `<frequencyPath: "‹path›", amplitudePath: "‹path›", timbre: "sine">` |
| AuthorZone | `<identifier: "‹identifier›", zoneKind: "‹authored kind›", owner: "‹being›", ownerKind: "‹kind›">` |
| WritePixel | `<facePath: "‹path›", uPath: "‹path›", vPath: "‹path›", colorPath: "‹path›">` |
| ElevatePixels | `<name: "‹property›", facePath: "‹path›", selector: {‹Piecewise model›}>` |
| FileRead | `<input: "‹file path property›", path: "‹destination property›">` |
| FileWrite | `<path: "‹file path property›", input: "‹content property›">` |
| CodecTransform | `<operation: "‹codec operation›", input: "‹source property›", path: "‹destination property›">` |

For example, these are complete Law sentences:

```text
called "Blend Glow" when clicked then Lerp <path: "@law-line-cube.glow", operand: 1, factor: 0.25>
called "Mapped Glow" when hovered then Map <path: "@law-line-cube.glow", expression: @law-line-cube.hp + 2>
called "Ordered Glow" when clicked then Sequence <children: [Set glow to 2, Add glow by 3]>
```

Use a top-level `;` to submit several at once. A trailing `?` remains a read-only syntax preview. Quoted path/token arguments are literal references, read later by the action; `expression:` is the generic arithmetic form for Map/Flow and builds the existing Piecewise and MathBindings. `children:` takes nested action sentences, including parameterized Create. Literal dictionaries/lists preserve model structure; vectors retain the typed vector envelope. Drive accepts its existing CurveModel record: form 0 constant, 1 polynomial, 2 sinusoid. ElevatePixels accepts the existing Piecewise selector record with local coordinates, guards, and bounds; this pass does not invent a separate region language. File and codec arguments identify properties holding file names, contents, and source values; CodecTransform operation names are existing codec properties such as `jsonCompact`, `jsonPretty`, or `base64`.

The generic template seam supports `$slot` JSON pointers plus an authored `$default` for optional inputs. Missing required inputs, duplicate arguments, unconsumed arguments, malformed models, or absent compiler Metalaws refuse. Optional owner/container/input/timbre values remain visible in compiler templates. A word with an authored signature cannot become an empty action merely by omitting its arguments. Complete closed fragments without that metadata retain their exact existing models. Completion displays the authored signature and merges canonical duplicates with the same spelling/opcode rather than inventing ambiguity.

Runtime execution still uses existing ActionModel behavior and channel/TransferPolicy gates. Authoring a pixel, sound, file, or Zone action does not prove the channel accepted its later execution. Flow's expression syntax is available, but bounded continuous behavior still requires an appropriately authored Piecewise domain and activation; an unbounded constant-rate sample is a syntax example, not a recommended indefinite session. Registration and Save Zone semantics are unchanged.

Save additions: 16 Lexemes, 16 denotes Relations, and 32 Law roots/references, all added to the existing LawLine closure under Zach's authorship with Codex injector attribution. Existing Zone bytes and Law roots are preserved; backups remain outside saves. Native interactive verification is recorded separately in the Person Verification List.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 00:09 PDT — Zach's missing action arguments request.*

## Click-local stairway example correction (2026-10-05)

Zach pasted the supplied four-sentence stairway and reported that clicking the cube did nothing. The original example restricted its maker Law to `Identity @law-line-cube`, while `enter LawLine` moves the Terminal line only, so another visible cube does not satisfy that condition. Its `my.position + (0, -3, 0)` placement can also leave new cubes below the floor. The exact original line does author and execute in isolation when its named cube is clicked; this is an example/context mismatch, not evidence of a broken compiler.

The corrected [one-line example](../../../../../../examples/law_line_stairway.txt) uses `if not stair is true` so any clicked unmarked cube is a launcher. Its Create children read `@event.subject.position` and place three steps at offsets `(0, 1, 0)`, `(1, 2, 0)`, `(2, 3, 0)`, above the clicked cube. Hover paints a marked step gold, departure paints it blue, and clicking one raises it by 0.5 without recursively spawning more. Start the app, `enter LawLine`, and submit the file's single line; expect four numbered authored acknowledgments, then click a visible cube. Save Zone still controls persistence. Previously saved duplicate Laws can still affect the same events; this pass does not delete or disable a Person's program.

The regression test loads the actual example file, authors it through Terminal's saved Metalaws, and sends pointer press/release through `InteractionChannel::observePending` and LawManager's event agenda. It verifies all three positions and the hover/leave/click effects. Earlier independent test fixture click Laws are disabled in the isolated test world before measuring this example. The engine was offline during diagnosis, so there is no claim of native pixel verification or confirmation of which cube Zach clicked. No production engine code or Person save was changed to fix this example.

## Sky spiral add-on (2026-10-05)

Zach confirmed the original Stairmaker works and asked for a cooler program. Keep the original four Laws enabled, submit `examples/law_line_sky_stairway.txt` once, then click an existing step. Eight cyan/violet flattened stones rise around a square spiral, ending in gold. Click any newly created stone to grow another turn from it; a stone grows its branch once, using visible authored `skyGrown` state. Hover highlights a jewel gold and turns it 25 degrees; departure restores its authored colour and orientation. The original click-to-rise behaviour remains active. The add-on requires no new compiler vocabulary or app rebuild; Save Zone persists submitted Laws and births.

The new line compiles through the same authored Metalaws, includes only three additional Laws, and uses existing ellipsoid manifestation (`shape.kind: 5`) rather than pretending cube radius parameters reshape a cube. The real pointer-channel regression keeps the original four Laws live throughout the add-on checks. Native proof and fixture authorship are in [the audit](../../../../../audits/SKY_STAIRWAY_AUTHORED_PROGRAM_2026-10-05.md). Enhanced Person appearance/gesture/durability checks remain open.

### Reactive hover follow-up

An exploratory `always if skyStep is true and hovered is true then Flow <path: "rotation.y", expression: 45>` authored a valid WhileTrue/Everyone Law and rotated a hovering jewel when applied directly, but its reactive application log remained empty. `hovered` read true and the world clock had positive delta in that fixture. Investigate candidate/fact propagation for this registered derived property before offering continuously hovering animations; read `docs/architecture/law/PROPHETIC_RETE.md` and `DERIVED_STATE_LEDGER.md` before changing the index. Preserve the final event-based add-on and its pointer/native consumers. This is an observation from the exploratory fixture, not proof of a general engine-wide failure.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 00:47 PDT — correction of the supplied example following Zach's failed CLI attempt.*

## Authoring guide — 2026-10-05

Zach asked for a guide that humans and agents can learn from. [The guide](../../../../../architecture/law/LAW_AUTHORING_CLI_GUIDE.md) starts with a clicked colour change and progresses through preview, addressing, activation, authored state, Create, composition, and the maintained Stairmaker example files. Its agent section points to the shared structural parser, authored signatures/templates, authority boundaries, and isolated verification surfaces. It distinguishes acceptance, firing, manifestation, and persistence, including the keyed identity and inactive-World errors uncovered with Zach. No world saves are edited by this documentation pass.

Verification: the guide's 13 executable lesson lines adopted the expected 17 Laws through the real Terminal/compiler path in a scratch copy of the isolated harness; preview adopted none, and all 176/176 checks passed (162 existing + 14 guide checks). [Result](../../../../../../scratch/verification/law-authoring-guide-2026-10-05/result.json). This is submission evidence plus existing functional checks, not a new native or advanced-channel witness.

Beginner readability and a first exercise followed by Save Zone/restart are tracked in [Person Verification](../../../For%20Zach/Person%20Verification%20List.md). Future agents: update the guide alongside changed authored signatures or parser semantics, verify its executable snippets through the real Terminal/Metalaw path, and retain the distinction between placeholder signatures and proven runtime programs.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 13:16 PDT — Zach requested the guide; Codex organized the lessons and reference from current source and the preceding authored programs.*
