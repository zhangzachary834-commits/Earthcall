# 2D Interface Robustness Pass

*Claude Opus 5.5 · session `b0dcb70f-a02a-4081-8589-0aae3ab30551` · 2026-09-22.*

**Origin.** Zach asked: *"look for ways to make the current Singular/Law driven interfaces
even more robust (the 2D ones), give proposals"*, then approved doing Tier 0 plus the model
test (A). The audit, proposals, and fixes below are mine, read from
`Singularity/Input/Interaction/`, `Object::draw2DObject`, the 2D pass in `EngineRender.cpp`, and
`INTERACTION_AS_LAW.md`. No widget class, `src/UI/`, or enum value was added (Refusals 1–3).

Task: [2D_Interface_Robustness](../Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/2D_Interface_Robustness/2D_Interface_Robustness.md)

---

## Tier 0 — defects (done 2026-09-22 unless marked)

| # | Defect | Fix | Guard |
|---|---|---|---|
| 1 | Equal-`zOrder2D` overlap: draw puts the *last* being on top (`stable_sort`), pick kept the *first* (`>`). The click went to the plate underneath. | Pick uses `>=` (`InteractionChannel.cpp`). | `interaction_robustness_test` §1 |
| 2 | Same-frame press/release replay copied the wheel into every replayed edge: `scrollTotal` inflated, `object-scrolled` published N+1 times. | Replay senses zero the wheel. Replay moved into `observePending()` so tests run the real code; test 15 of `interaction_channel_test` had been a hand copy of it. | §2 + model |
| 3 | Window focus loss mid-press cleared `pressedId`/`dragging` silently: no `released`, no `drag-ended`. | `cancelPress()`: `released` → `drag-ended` (if travelled) → new `object-press-cancelled`; never `clicked`. All three buttons. | §3 + model |
| 4 | A pressed or focused being leaving the reachable set (Zone switch) orphaned the press, and focus kept routing keys to a being in another Zone. | Orphan check at the top of the edge pass cancels the press and publishes `object-unfocused`. | §4 + model |
| 5 | Slider archetype: `Flow` integrated `dragX` (already a per-frame delta) over `dt`, so it was frame-rate dependent and ~60× slower than `controlStep`. `controlMin`/`controlMax` were never read. Synthesis Studio wrote a second law to clamp one slider back. | `Sequence(Map v+d·s, Map clamp(v,lo,hi))` in one law. An unranged slider still moves: its clamp step fails to bind, and the trace shows it. | `control_patterns_test` §4a |
| 6 | Duplicate middle-release block (dead). Stale comments/doc (right/middle "not built"). | Removed / updated. | — |
| 7 | ⚑ `clickSlopPixels` is registered writable; §4b calls it a first-mover constant no law may widen. | **Not changed — Zach's call:** Kernel-tier in `TransferPolicy`, or revise §4b. | — |
| 8 | Null-subject edges are dropped by `publishEdge`: `key-pressed` with nothing focused and `object-scrolled` over nothing never publish, against §4b's table. A global key command is unauthorable. | **Not changed.** Needs a decision on what a subject-less edge means under `Scope::Subject`. | — |

**Mutation check.** Each of fixes 1–4 was reverted one at a time. The new test fails every time,
and for 3 and 4 the random model fails as well as the scripted case. `control_patterns_test`
§4a fails 5/5 against the old `Flow` slider.

## A — model-based interaction test (done)

`tests/singularity/interaction_robustness_test.cpp` drives `observePending()` with 10,000
seeded random frames across 5 seeds. Each frame can move the pointer, send a burst of up to 3
button edges, scroll the wheel, let a foreign UI take the pointer, lose window focus, or remove
and return a being. The event stream is checked against a gesture grammar: every press closes
once; click, drag and cancel keep their order; hover alternates; `scrollTotal` is exact. When
the stream breaks the grammar, the test prints the last 10 events. Add a new edge type here
whenever you add one to the channel.

## Tier 1 — proposed, not started (in value order)

- **B. Pointer capture from the press**, not from the slop, for 2D and 3D. This is the W3C
  Pointer Events model. The release must still test the true under-pointer being, so a
  release elsewhere stays a cancelled click. Make it an authored `pointerCapture` property.
- **C. Authored hit region.** The pick is the AABB of `width2D`×`height2D`. Add an optional
  OntoMath `Piecewise` over local (u,v), reusing the `ElevatePixels` defined-set evaluation.
  Also: an alpha threshold for image plates, and normalizing negative width/height (drawn but
  unpickable today).
- **D. Invisible means unpickable.** Something drawn at material opacity 0 should not take
  clicks. Disabled controls = a category + law, not a C++ field.
- **E. Relative placement + clipping.** A `positioned-in` Relation with offset and strength
  properties, solved by a layout law (Cassowary, as behind AutoLayout). A parent clips both
  draw and pick. This fills §12's missing layout library.
- **F. Keys.** A registered `keysDown` set (chords break the single `keyDown` today) and a
  `text-entered` character edge with the caret as a property (Language channel; already on the
  To-do).
- **G. Law-loop lint.** Use the Prophetic Rete analysis to flag two laws whose actions satisfy
  each other's conditions (the toggle trap) in the Law Graph.
- **H. Archetypes as authored save laws**, so a Person's edits survive (§12).
- **I. Focus traversal & accessibility.** A `focus-next` Relation for Tab order. A reader
  channel over category + `controlLabel`.

## For the next agent (Jules especially)

- **Read first:** `INTERACTION_AS_LAW.md` §4 and this file. Run `interaction_robustness_test`,
  `interaction_channel_test`, and `control_patterns_test` before and after touching the channel.
- **Pitfalls:**
  - Events published *outside* `observePending` (`onWindowFocus`, `noteKey`) must be inside
    the model's frame window. The test marks `before` at frame start for this reason; an
    early version missed it and reported false violations.
  - `Universe::beings()` is the resolver for beings that left the reachable set. Do not
    "simplify" `resolveBeing` into `findReachable`, or cancellations lose their subject.
  - Never add a "still dragging" per-frame event. `object-dragged` already fires only on
    movement.
  - 13 suite tests were already red on 2026-09-22 *before* this pass (chess ×5,
    synthesis_studio_app, zone_boot_hydration_relations, prism_cathedral, gpu_mastery,
    slow_adapter_zone_perf, webgpu_perlin_exact_gradient, quantifier_scaling timeout,
    frame_lag). Their failure lines were compared with and without this change: identical.
