# Web Designer: Compiled Live Pages (and the Path to a Web Law Runtime)

## Status
- **Date opened**: 2026-10-18
- **Status**: Proposed. Nothing here is implemented. **Do not start until Sol's verdict on Web Designer round 5 and the merge of `earthcall_web_designer_modality`**; then work on a separate small branch.
- **Origin**: Zach's direction in conversation with Claude Code / Claude Opus 5.5 (session `c1339adf-4919-4ca2-bbde-3d0f0eb32a19`), 2026-10-17/18. Zach supplied the human idea: Earthcall's web modality should make interfaces *much more human and flexible*, and its first real use should be TeacherOps, the tutoring-hub coordination tool he built for his mom (`zhangzachary834-commits/teacherops`). Zach also supplied both corrections recorded in §1.
- **Parent task**: [Earthcall Web Designer Modality](../Web_Designer_Modality/Web_Designer_Modality.md) · **Architecture**: [WEB_DESIGNER_MODALITY.md §9](../../../../../architecture/Integration/WEB_DESIGNER_MODALITY.md#9-realization-mode-compiled-today-live-runtime-later)

## 1. What the Web Designer is today (Zach's two corrections)
1. **Earthcall is a C++ application.** No Python or web reimplementation of the ontology exists yet, so a website cannot "just run Laws," and the people using a site will not be running Earthcall.
2. **The Web Designer is a compiler.** Zach commissioned Sparkly Guy to make Earthcall Laws translate Lexemes into HTML that runs on the web *without* Earthcall. The exported page (`saves/web_designer/prototype.html`) is self-contained HTML/CSS plus a small script; a click reaches the EventBus only when the page is hosted inside Earthcall's own WebKit view (`window.webkit.messageHandlers.earthcall`). In an ordinary browser it only logs.

A web engine modeled on Earthcall's ontology itself, one that runs Laws live in the browser, is a **future endeavor**. This task lays the path from the compiler to it, so that each step is useful on its own.

## 2. Why: structure without rigidity
Small, informal organizations (TeacherOps' tutoring hub is the first) avoid polished vertical software because it forces them to bend to it, and they retreat to spreadsheets and chat because those bend to them, at the cost of everything living in one coordinator's head. Earthcall's offer is **real, connected structure whose views and rules are authored data, changeable in a sentence**.

TeacherOps' own thesis already matches the ontology: *the core object is the tutoring match*. A match is a **Relation** between a family, a student, and a teacher, not a row. Teachers, families, matches, absences, and lesson packages are therefore a strong exit test for Refusal 1: an entire real domain Zach did not invent, authored as data with zero C++ nouns.

## 3. Three stages
### Stage 1: Earthcall as studio, TeacherOps as the live app (possible as soon as the Web Designer merges)
- Earthcall authors each interface's form and layout and compiles it to standalone HTML.
- TeacherOps (Python/FastAPI, SQLite) keeps the data and the behavior (`list_followups`, matching, drafting).
- The coordinator never touches Earthcall. She states what she wants; Zach authors it; the page is recompiled and deployed.

### Stage 2: compiled pages that stay live
- **Rung A: governed data binding.** A medium-specific `web.data.source` property lets an authored list/item template be filled at view time from an allowed endpoint (first: TeacherOps' follow-up list). This is a **network capability** and must be governed exactly like raw CSS after round 5: an origin allowlist granted through authority, never a caller-set bool, and never arbitrary `fetch` emitted from authored strings.
- **Rung B: compile a subset of canonical Laws to page script.** Filters, sorts, and "when condition, set property" Laws are realized into the page's JS. The JS is a *realization* of canonical `ConditionNode`/`ActionNode` Laws, exactly as HTML is a realization of authored beings. **Do not invent a second web-specific Law language or serialization** (Sol, round 5, blocker 3). Emitted behavior carries the Law's identity (as `data-being-id` does for elements) so provenance survives into the page.
- Each Law shape that compiles to JS is one more piece of the future runtime, chosen by a real need rather than designed up front.

### Stage 3: a runtime that runs Laws directly (future)
A web (or Python) engine modeled on the ontology executes Laws live. Only here can a coordinator author by speaking to the page herself. Scope it from what stages 1 and 2 actually needed.

## 4. Milestone: "The Waiting Board"
One Earthcall-authored page, compiled by the Web Designer:
1. TeacherOps teachers, families, and matches arrive through the **Python sidecar** pattern ([FOREIGN_DATABASES.md §2](../../../../../architecture/Integration/FOREIGN_DATABASES.md#2-the-python-sidecar-data-ingestion)). SQLite stays foreign and never holds ontological state.
2. One list shows who is waiting on whom (Stage 2 Rung A).
3. One button marks a family as replied to.
4. Zach authors one sentence the coordinator asked for (e.g. "families running low on lessons go to the top"), recompiles, and the change appears the same day.

**Acceptance is a Person's, not a test's:** the coordinator understands the page within about 30 seconds and confirms the requested change fits how she works. When the milestone exists, write that check into [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md).

## 5. Pitfalls for whoever takes this
- **No tutoring nouns in C++.** No `class Teacher`, `class Match`, `enum LessonStatus`. All are authored beings, Relations, and Categories (Refusals 1 and 3).
- **The importer is not the author** (Sol, round 5). Records ingested from TeacherOps keep their recorded provenance; whoever runs the sync does not become their author.
- **No black box in the binding.** Bound data fields must be legible property paths, not opaque JSON blobs inside one property (Refusal 6).
- **Do not use the Waiting Board to bypass the open round-5 gates.** It builds on the merged modality.
- Stage 2 compiles Laws; it does not interpret them. A live interpreter in the browser is Stage 3 and needs its own design.

## 6. Open questions only Zach can answer
1. **Stewardship in Stage 1.** The coordinator asks and Zach authors. Is the resulting Law authored by Zach on her behalf, and does she need to exist in Earthcall as a Person for that "on behalf of" to be truthful? (Compare `injectedBy.onBehalfOf` in the round-5 vocabulary fixture.)
2. Is the Waiting Board's data a live sync, a periodic import, or a one-time import for the demo?
3. Unverified: can today's `ConditionModel` express time-since comparisons ("waited more than 2 days")? If not, that is new Law vocabulary, not a Web Designer feature.

## 7. Future agents
- Read the parent task, WEB_DESIGNER_MODALITY.md, and the Sun ↔ Sparkly Guy thread (`agent intercom/communication-threads/ontology-and-authorship/SUN_TO_SPARKLY_GUY_The_Zach_Plush_Incident_2026-10-13.md`) before touching `Singularity/Foreign/Web/Designer/`.
- Stage 1 needs no Earthcall code beyond the merged modality. Rung A is the first code change.
