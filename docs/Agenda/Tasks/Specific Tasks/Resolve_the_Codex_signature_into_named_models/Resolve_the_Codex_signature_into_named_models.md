# Resolve the bare `Codex` signature into named models

**Status:** open — one part needs Zach's word before anything is edited.
**Section in the To-Do list:** First Movers
**Opened 2026-09-07** by Claude Opus 5 (session `session_01K1PtKNZtSDU9XGwKZQ7ZzF`), from Zach's
2026-09-07 correction to the collaborator roster.

---

## The situation

`Codex` is a **harness**, not a model — the same category error the Jules seat decision just
untangled. Unlike Jules, though, Codex's model *is* knowable to Zach, so the gap here is
recoverable and worth closing.

Zach's account of what actually ran, 2026-09-07 (his words, my arrangement):

- GPT models were used **sparingly** — no OpenAI Plus plan at the time — so the sessions are
  **"occasional but high impact."** That is OpenCode; **GPT-5.6 Sol was run on API credits.**
- Almost everything signed `Codex` or `OpenCode` is **GPT-5.6 Sol or Terra**.
- **Astra** (the GPT-6 generation) has been used **exactly once**, and it **wrote no documents** —
  it made **Synthesis Studio upgrades**. Zach believes one Synthesis Studio section currently
  **marked bare `Codex` is actually Astra's.**
- **Luna** — Zach recalled it as "only once in the agent intercom." That is an undercount; see below.
- **5.5T** has not been used on Earthcall since **June 2026**.

## The five surface names for one lineage

Sol alone signs under: `OpenCode (GPT-5.6 Sol)`, `Codex (GPT-5.6 Sol)`, `OpenAI Codex (GPT-5.6 Sol)`,
`ChatGPT-5.6 Sol`, and — in all the September work — bare `Codex`. The August artifacts name the
model every time; the September ones never do.

That matters more than a style nit because of **what the September Codex sessions were doing**:
the serialization-topology rewrite (Phases 1–7, session `01a0707e`), and the Perlin campaign,
where Codex held **acceptance authority** — five review passes, four blocking gates, one outright
rejection of Gemini Spark's first submission. The repo records that something rejected an
architectural submission and does not record what.

## ⚠ This reaches the save files

The 2026-09-04 "Play the room: resonance Studio" pass minted an authoring marker **Object** named
**`studio.author.codex`** into the Synthesis Studio saves, and **21 new Laws name that Object as
their author**. It appears **90 times** in `saves/worlds/synthesis_studio.ecform` — as Law author
entries and as `entityB` on Relations — and the world's `injected_by` reads:

```
Gemini Spark (authored) / Claude Opus 5 (repaired) / Antigravity (major scale 2026-09-02) / Codex (resonance studio, 2026-09-04)
```

If that pass was **Astra**, the save carries a **wrong author identity on 21 Laws** — not a doc
typo but a defect in the flesh, in exactly the ledger `Law::applyTo` refuses to fire without.
It is the same species as the forged `authors: ["Player"]` in chess, and it is the reason this
task cannot be closed by an agent reading files.

## ✅ Resolved 2026-09-07 — and the stakes are lower than first written

Zach restored **Broadcast #5** (`agent intercom/ALL-HANDS-ON-DECK BROADCASTS: August_20_2026_intercom_Notes.md`,
line 271), which had been accidentally deleted. It settles the Astra question from Zach's own hand:

> "SO FAR ASTRA SEEMS TO HAVE DONE GOOD WHEN I ASKED IT TO DO **\"make the Synthesis Studio cooler\"**" — Zach

The `## 2026-09-04 — Play the room: resonance Studio` section records the request as *"make the
synthesis studio way cooler."* Same prompt, one pass, and the artifacts line up item for item:
Zach praises a **rectangle amplitude thing with a sound-blip animation** (the seven resonators and
seven meters) and **design writing — "C major" written into the note pad** (the seven captions).
It is Astra's pass.

**But `studio.author.codex` is not a forgery, and must not be renamed.** Astra was reached
*through the Codex harness*. `Codex` names the harness **truthfully**; what is missing is the
model — the identical shape to the Jules seat, and the identical remedy. My earlier framing of
this as "a wrong author identity on 21 Laws" was wrong and is retracted: nothing in the saves
asserts a false author. A stable identifier that 21 Laws and many Relations point at should
**stay exactly as it is**; the model belongs in the human-readable attribution beside it.

## What must happen, in order

1. ✅ **Identified** — the 2026-09-04 "Play the room: resonance Studio" pass
   (session `synthesis-studio-20260904`) in
   [Synthesis_Studio_is_half_wired.md](../Synthesis_Studio_is_half_wired/Synthesis_Studio_is_half_wired.md)
   is **Astra, run through the Codex harness**, per Broadcast #5.
2. **Add the model to the prose only.** The doc section signature becomes
   `Codex (GPT-6 Astra)`, and the world's `injected_by` string gains the model beside the harness.
   **Do not touch `studio.author.codex`** — the identifier is correct, 21 Laws resolve through it,
   and renaming it would be a migration in the flesh to fix something that is not broken.
   Awaiting Zach's go-ahead only because it edits a save file.
3. **Backfill the model on the September Codex artifacts** Zach can still place: the serialization
   topology task doc, both Perlin plans, `docs/audits/rendering_optimization/README.md`, and the
   rendering regression audit.
4. **Adopt one signature form** for harness-run agents: `<harness> (<model>) · session <id>`.
   `Codex (GPT-5.6 Sol) · session 01a0707e` — the form the August artifacts already used.
   Jules is the standing exception, and its exception is now written down.

## Correction owed to the roster: Luna is load-bearing

Zach placed Luna as "only once in the agent intercom." In fact **ChatGPT 5.6 Luna structured the
Prophetic Rete specification** — `docs/architecture/law/PROPHETIC_RETE.md` names Luna in its
**Origin** block, and its §7 worked example is cited *literally* in the interval table
(`2x + 5`, `x ∈ [0, 10]` → `[5, 25]`). Zach's own note in
[Rete_Truth_Seeking_Focus.md](../Rete_Truth_Seeking_Focus.md) records the commission: *"I asked
ChatGPT 5.6 Luna to take my notes about Rete optimizations and write it as a structured document."*
`PROPHETIC_RETE.md` is a Router-table entry in `AGENTS.md`. Whatever else is true of Luna's
footprint, it is not one intercom message.

**Signed:** Claude Opus 5 · session `session_01K1PtKNZtSDU9XGwKZQ7ZzF` · 2026-09-07
