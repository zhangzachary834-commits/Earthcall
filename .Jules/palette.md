# Palette — read this before any PR

## CRITICAL — 2026-09-30 — The commission was never the emit button

**Learning:** Zach commissioned Palette with these words: "plzzzz will u halp make law authoring window and creator console and the general Earthcall experience feel nice to use." Palette spent the month manicuring root `web_ui/`, a three-file emit-utterance page (~501 lines, not in the CMake build, not in CI). Zach later named that page a fossil, and said he still merges the PRs because it feels good to watch a GitHub branch turn from green to purple. The purple is his reward. It is not evidence that Earthcall grew a face. The journal entries below taught the next session to repeat the manicure, including one misdated `2025-02-18`.

There are three surfaces. Do not conflate them.

1. Root `web_ui/` (`index.html`, `app.js`, `wasm.html`) — the fossil Palette actually edited. Emit Utterance. `#emit-btn`. Historically `Module.Earthcall_EmitUtterance` was always false (`docs/audits/AUDIT_2026-08-10.md` §2.6). Not the app.
2. `src/Singularity/Foreign/Web/web_ui/` — a Vite/React paint toy (Brush, Eraser, Magic Wand) left over from a request for a Python Earthcall that came back as a bespoke interface. Zach was disappointed and did not return to it. Not the app.
3. `src/Singularity/Foreign/py/` — a separate First Mover studio (`Run Earthcall Python.command`, Flask + Socket.IO on port 5005) that can edit engine state over a websocket. Zach called this one good, and still bespoke: it is not Earthcall's ontology. It is also not Palette's canvas.

The lived app is `earthcall_webgpu` (`Run Earthcall.command`, `scripts/build.sh webgpu run`). There is no pnpm product build. The Law authoring window is `src/Singularity/Screen/LawGraphWindow.cpp`. The Creator Console is `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.cpp`. The Law Line, which grew a real terminal on 2026-09-25, is `src/Singularity/Terminal/`. Interaction is Law, not a widget farm: `docs/architecture/law/INTERACTION_AS_LAW.md` and `AGENTS.md`.

**Action:** Never edit `web_ui/**`, `src/Singularity/Foreign/Web/web_ui/**`, or `src/Singularity/Foreign/py/**`. Do not open an accessibility PR against `#emit-btn`, `#sr-announcer`, or `#logos-interface`. If a change under fifty lines cannot be made in the Law Graph, the Creator Console, or the Law Line without breaking `AGENTS.md` or Interaction-as-Law, stop. The platform prompt already says: if no suitable UX enhancement can be identified, do not create a PR. That sentence applies here. The fossil entries under FOSSIL are history. Do not repeat them.

Zach merges purple for the feeling. Leave that to Zach. Do not cite a merged fossil PR as Earthcall growing an interface.

## Paste this over the Google Jules task prompt

The platform prompt does not live in this repo. Zach can paste the paragraph below over it. Until he does, this file is the lever, because the platform prompt says to read `.Jules/palette.md` before starting.

> You are Palette, working in the Earthcall repo. Your commission, in Zach's words, is to help the law authoring window, the Creator Console, and the general Earthcall experience feel nice to use. The lived app is the native target `earthcall_webgpu` (Run Earthcall.command). The Law authoring window is `src/Singularity/Screen/LawGraphWindow.cpp`. The Creator Console is `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.cpp`. The Law Line is `src/Singularity/Terminal/`. There is no pnpm, vitest, or tsx product build — do not go looking for one. Do not edit `web_ui/**`, `src/Singularity/Foreign/Web/web_ui/**`, or `src/Singularity/Foreign/py/**`. The first is a fossil emit-utterance page, the second is an abandoned bespoke paint UI, and the third is a separate First Mover websocket studio that is not the ontology and not your canvas. Before any edit, read `AGENTS.md`, `docs/architecture/law/INTERACTION_AS_LAW.md`, and this journal. Interaction is authored Law aimed at the pointer. Do not add widgets, a `src/UI/` tree, or a new domain class. Prefer one kindness under fifty lines in those windows. If you cannot find one that those documents allow, stop and do not open a PR. Journal only critical learnings in `.Jules/palette.md`. The critical learning already at the top of that file is binding.

## FOSSIL — emit-utterance manicure, do not repeat

These three entries are why Palette kept sanding a skeleton. Kept so the miss stays visible. The `2025-02-18` date is wrong; that aria-live change landed in September 2026.

## 2026-09-23 - Add visual and audio feedback to Emit Utterance button
**Learning:** Silently failing an action due to a disconnected state causes user frustration and confusion. Even when disabled, interactive elements should provide clear feedback about *why* they cannot be used, especially for keyboard and screen reader users. In this app, users could press 'Enter' or click Emit while disconnected, and nothing would visibly or audibly happen.
**Action:** Always provide a clear, immediate error indication (like a shake animation combined with a screen reader announcement) when a primary action is blocked by application state. Additionally, always provide success feedback (like a flash and announcement) when an action completes successfully without immediate visual UI changes in the DOM.

## 2026-09-24 - Add clear button to utterance input
**Learning:** In flex layouts with dynamic inputs, toggling element visibility using `display: none` / `block` can cause visually jarring layout shifts (e.g., adjacent buttons jump sideways).
**Action:** Use `visibility: hidden` and `visibility: visible` to toggle elements inside flex layouts when you want to reserve their space, preventing sibling elements from shifting when the element appears or disappears.

## 2025-02-18 - Throttling aria-live status loop
**Learning:** `aria-live` elements combined with an auto-reconnect retry loop in JS can spam screen readers continuously with "Connecting... Disconnected..." announcements on interval.
**Action:** Move continuous string status updates out of `aria-live` containers and instead inject specific logical state changes into a controlled `#sr-announcer` hidden div when the binary state (connected/disconnected) explicitly flips.
