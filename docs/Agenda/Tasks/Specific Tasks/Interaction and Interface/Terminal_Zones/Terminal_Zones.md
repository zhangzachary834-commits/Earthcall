# Terminal Zones: the line has a location

**Status:** built and verified 2026-09-30 (`terminal_zones_test`, `law_line_zone_test`), awaiting Zach's check (Person Verification List) and the opcode verification the gate below requires
**Section in the To-Do list:** Interaction and Interface
**Origin:** Zach, 2026-09-30: *"I CANT TYPE ANYTHING INTO THAT TERMINAL SESSION BC ALL MY LINES GO TO THE LAW AUTHORING CLI … WE NEED TO TREAT THIS LIKE ZONES (which terminal mode im in) AND MAKE AN OPCODE TO SWITCH BETWEEN ZONES"*.
**Recorded by:** Claude Code · Claude Opus 5.5 · session `08b0f730-6e49-4c49-b27f-3a89c810ca4b` · 2026-09-30.

---

## ⚠️ GATE: read this before building anything on top of Terminal Zones

Zach's instruction, verbatim intent, given with the decisions below:

> **Do not build more systems on top of this until we verify the opcodes are made as foundational minimal-maximal invariants.**

What that means for the next agent:

- `enter <zone>` (the line's move) and the Identity unlock (a secret line) are the only opcodes this task introduces, and both are **provisional** until Zach verifies them against the minimum-maximum principle (CLAUDE.md, the Seven Refusals).
- Do **not** add terminal verbs, modes, command tables, a World-command grammar, set-to-set commands, or any other layer that assumes these opcodes are final. The **World** Zone deliberately ships with *no* line-hearing Laws for exactly this reason. Its opcodes are the first thing to verify, not the first thing to build.
- Don't add an enum of terminal modes (Refusal 3). A "mode" is a real Zone, and what a line means there is the Laws that Zone holds.
- If you think an opcode is missing, write the argument here (why it's an invariant no Law could express) and stop.

## Decisions (Zach, 2026-09-30)

| Question | Zach's answer |
|---|---|
| What is a terminal zone? | **A real Earthcall Zone** (`saves/zones/<id>/`). The Laws in its closure are what hear the line; its Lexemes are its vocabulary. |
| What does the switch opcode move? | **Only the terminal line.** The Person's body stays in its world Zone. Two presences: the body in a world Zone, the line in a terminal Zone. |
| First zones | **Identity** (unlock or key the Person with a hidden passphrase), **Quiet** (lines mean nothing), **World** (commands; *gated*, see above), besides the existing **LawLine**. |

## Design (Opus 5.5, derived from those decisions)

- **The one new invariant: a held closure.** Before this, a Zone's Laws were live only for the Person's presence (`ZoneManager::switchTo`, `_activeZoneLawIds`). The line is a second presence, so ZoneManager gains `holdZoneClosure(holder, zone)` / `releaseZoneClosure(holder)`. The preflight is the *same* one `switchTo` uses, so authors must resolve, standing must hold, and nothing half-loads. A Law live for two presences is shared, and it's removed only when no presence still needs it.
- **`enter <zone>`** is recognised by the Terminal channel before a line is published. It must be. A Zone whose Laws decided whether you could leave it could trap the line (Quiet hears nothing), so the escape can't depend on any Zone's Laws. That's the argument for it being an invariant. `enter` alone lists Zones. The same move is also the channel's registered, writable property `zone`, so a Law can move the line too.
- **Edges:** `terminal-zone-exited` / `terminal-zone-entered` are published on the move. The prompt shows the Zone: `earthcall[LawLine]>`.
- **Which Laws hear a line:** every live Law that listens for `terminal-line-entered`. A world Zone may also hold such a Law, so the LawLine seed Law gains the condition `@terminal-channel.zone == "LawLine"`. Without it, standing in the LawLine world Zone would still turn every line into a Law wherever the line is.
- **Identity:** the Identity Zone's Law, on `terminal-zone-entered`, adds 1 to `@terminal-channel.unlockRequests`. The *channel* then takes the next line as a secret. That line is masked on screen, never published, never in history, never in any property, and it unlocks the Person (or keys them the first time, confirmed by typing twice). The Law asks and the kernel performs, the same pattern as `speakRequests`. The passphrase never becomes world state (NO_BLACK_BOX §5: a secret beneath the Kernel, named here).
- **Quiet:** a Zone with no Law that hears lines. Lines scroll and nothing acts.
- **World:** a Zone that exists and can be entered, with no line-hearing Laws. See the gate.

## What landed (2026-09-30)

- `ZoneManager::holdZoneClosure / releaseZoneClosure / heldZone / findZoneIndex`. `switchTo` and holding share one `prepareZoneLawClosure` preflight (extracted verbatim). A Law is removed only when no presence holds it.
- `TerminalChannel`: registered `zone` (write it to move the line), `unlockRequests`, `unlocksHandled`, `awaitingSecret`. `enter` is handled before a line is published, and the line is placed before the first line of the first frame. `terminal-zone-exited/entered` are published. The prompt reads `earthcall[LawLine]>`.
- `LineEditor::secret`: bullets, and no provider, history, ghost, ↑/↓ or search sees the text.
- `Identity/PersonPresence` (`unlockPresentPerson`, `keyPresentPerson`) is shared with boot's `EARTHCALL_KEY_PASSPHRASE` path.
- Seed `scripts/seed_terminal_zones.py`: Identity, Quiet and World Zones; `law-terminal-identity-unlock`; `law-line-hear` gains `if @terminal-channel.zone == "LawLine"` (one targeted patch with a backup; `seed_law_line.py` now seeds it that way).

Known limits: a held Zone's own *beings* (e.g. LawLine's cube and custom Lexemes) are visible to Laws only when the Person's body is also there. The Universe provider still reads the body's Zone. A world load (`LawManager::loadFromJson`) replaces non-first-mover Laws and can drop a held closure until the line re-enters.
