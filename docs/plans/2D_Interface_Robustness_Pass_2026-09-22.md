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
| 3 | Window focus loss mid-press cleared `pressedId`/`dragging` silently: no `released`, no `drag-ended`. | `closePress()` (was `cancelPress`): `released` → `drag-ended` (if travelled) → new `object-press-cancelled`; never `clicked`. All three buttons. | §3 + model |
| 4 | A pressed or focused being leaving the reachable set (Zone switch) orphaned the press: its release was lost. | ~~Hard cancel on leaving reach~~ — **revised 2026-09-23**, see below. Now the press persists and still closes on release (resolved across the Universe); leaving reach is sensed as `object-left-reach`. | §4, §4b + model |
| 5 | Slider archetype: `Flow` integrated `dragX` (already a per-frame delta) over `dt`, so it was frame-rate dependent and ~60× slower than `controlStep`. `controlMin`/`controlMax` were never read. Synthesis Studio wrote a second law to clamp one slider back. | `Sequence(Map v+d·s, Map clamp(v,lo,hi))` in one law. An unranged slider still moves: its clamp step fails to bind, and the trace shows it. | `control_patterns_test` §4a |
| 6 | Duplicate middle-release block (dead). Stale comments/doc (right/middle "not built"). | Removed / updated. | — |
| 7 | `clickSlopPixels` is registered writable; §4b called it a first-mover constant no law may widen. | **Zach, 2026-09-23: the doc changes.** It stays law-writable; §4b revised. | — |
| 8 | Null-subject edges were dropped: `key-pressed` with nothing focused and `object-scrolled` over nothing never published. | **Done 2026-09-23 per Zach:** such edges name the pointing Person as subject; every other edge carries the Person as `@event.object`. | §4c |

**Mutation check.** Each of fixes 1–4 was reverted one at a time. The new test fails every time,
and for 3 and 4 the random model fails as well as the scripted case. `control_patterns_test`
§4a fails 5/5 against the old `Flow` slider.

## Revision 2026-09-23 — Zach's three decisions

*Claude Opus 5.5, same session. Zach's words are quoted; the mechanisms are mine.*

1. **Click slop:** *"The doc should change."* `clickSlopPixels` stays authorable;
   `INTERACTION_AS_LAW.md` §4b now says so.
2. **No subjectless events:** *"events semantically always involve some Singular ... A key
   pressed without anything selected is still pressed by a Person inside a Zone."* The engine
   hands the channel its Person every frame (`setPointingPerson`, exposed as the read-only
   `personId`). `publishEdge` makes the Person the subject when no being was addressed, and
   the agent (`event.object`) otherwise. I added the agent part as an extension: it makes
   "who clicked" answerable, and multi-Person pointing will need it.
3. **Zone-switch cancel must be authorable, not hardcoded:** *"Person's currently located
   zone must be decoupled from whether a Zone is active--you may want to have the states of
   another Zone changing and running even when you aren't present in it."* So:
   - The channel no longer cancels on leaving reach. It publishes `object-left-reach` /
     `object-entered-reach` once per transition. The press and focus persist, and the
     physical release still closes the press.
   - Writes to `pressedId` / `focusedId` (and right/middle) are treated as authored gestures.
     The channel remembers what it last wrote itself (`_heldSeen`, `_focusSeen`). A difference
     is a law's Set, and becomes `released` → `drag-ended` → `press-cancelled`, or
     `unfocused` / `focused`. The cancel is now one ordinary law, `OnEvent object-left-reach →
     Set @interaction-channel.pressedId := ""`, run end to end in `control_patterns_test`
     §4a′. The same mechanism makes Tab-order focus (Tier 1 I) a law.
   - C++ still cancels in two cases, both facts about the machine: window focus loss (the OS
     stops reporting the button), and a being gone from the Universe entirely (nothing left
     to hold). In the second case `press-cancelled` names the Person.
   - **Follows Zach's direction toward continuous OntoMath reach:** `step()` still gathers
     `mgr.active().objects()` as the reachable set. That single gathering is what must change
     when Zone activity and Person location are decoupled; the edge vocabulary does not.

Mutation-checked: disabling the authored-write handling fails 5 checks across both tests.
The red suite tests (chess ×3 compared, zone_boot, prism, synthesis_studio) still fail on
identical lines.

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
  - The channel must be told its Person (`setPointingPerson`). Without one, edges that
    address no being are not published, and the headless tests only see them because they
    construct a `Person`.
  - Do not reintroduce a C++ cancel on leaving reach. It is authored (see the revision above).
  - A 2D plate's default position is (100, 100), not the origin. Set `x2D`/`y2D` in tests.
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
