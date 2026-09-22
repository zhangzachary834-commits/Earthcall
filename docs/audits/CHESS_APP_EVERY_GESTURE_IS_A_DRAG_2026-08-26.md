# Chess App: every gesture a Person makes is classified as a drag, and no law hears a drag

**Date:** 2026-08-26
**Author:** Claude Opus 5
**Session:** https://claude.ai/code/session_01Ewyb3adQJuSmsgG2x5Yp7p
**Asked by:** Zach — *"pin down exactly why even though the other agents are saying the chess
engine is working I still load the world and try to click on a pawn but clicking another square
or dragging doesn't do anything, and they tried to fix this several times and it didn't work"*
**Supersedes:** [`CHESS_APP_CLICKING_DOES_NOTHING_2026-08-25.md`](CHESS_APP_CLICKING_DOES_NOTHING_2026-08-25.md)
(its root cause is wrong — see §4)
**Reproduction:** `scratch/probes/chess_app_full_loop_probe.cpp`

---

## 1. The finding, in one paragraph

The chess law chain works. The picking geometry works. The save file is intact. What fails is
one layer below all of it, in `InteractionChannel::observe()`: **a pointer gesture that travels
more than 6 window points between button-down and button-up is published as
`object-drag-started` / `object-drag-ended` instead of `object-clicked`
(`InteractionChannel.cpp:242`, `kClickSlopPixels = 6.0f` at `InteractionChannel.hpp:159`), and
no chess law listens for a drag** — `saves/worlds/chess_app.json`'s trigger table binds
`object-clicked` and nothing else. Six window points is roughly a quarter of a chess piece's
on-screen radius at the save's own camera. A trackpad click clears it easily. So the Person's
click reaches the right pawn, the world hears the gesture, and then throws it away because it
came in under a name nothing was written to answer. Zach asked for a **drag** gesture in the
first place (`To-do list.md` line 218: *"be able to actually move it if I drag to another
square"*), and drag was never authored. Both halves of his sentence — clicking does nothing,
dragging does nothing — are the same defect: **every gesture he makes is a drag, and drag has
no law.**

A second, independent defect makes it worse when it applies: **while the cursor is locked
(`GLFW_CURSOR_DISABLED`, which is the boot state — `Engine.cpp:171`, `MouseHandler.hpp:62`),
`step()` fires the pick ray from the viewport centre rather than from the pointer
(`InteractionChannel.cpp:358-365`), and nothing draws a reticle.** Every click then lands on
whatever the camera happens to be aimed at — for `chess_app`'s saved camera, square (3,6) —
regardless of where the Person is pointing.

---

## 2. The evidence

`scratch/probes/chess_app_full_loop_probe.cpp` reproduces the *running app*, not the test's
idealised path: it registers every first mover `EngineInit` registers, installs the same
`Universe` provider, loads `saves/worlds/chess_app.json` through `ZoneManager::loadState`,
computes the pick ray from `EngineRender`'s own matrices, and runs `observe → Zone::update →
applyFormationRelations → LawManager::tick` every frame the way `Engine::tick` does. It prints
every `object-*` edge as the EventBus carries it.

**A perfectly still click (0 px of travel), cursor unlocked — works:**

```
--- human click on the e2 pawn at (607.55,526.297) ---
      EDGE object-pressed  <- "piece-white-pawn-4-1"
      EDGE object-focused  <- "piece-white-pawn-4-1"
      EDGE object-released <- "piece-white-pawn-4-1"
      EDGE object-clicked  <- "piece-white-pawn-4-1"
  pawn isSelected = TRUE  selectionActive = TRUE  targetX=4 targetY=1  selectedX=4 selectedY=1
--- human click on the e4 square ---
      EDGE object-clicked  <- "object.chess.board"
  pawn now at (4,3)  turn=1
RESULT: pawn walked e2-e4.
```

**The same click with ~7 px of travel — a normal trackpad click:**

```
--- human click on the e2 pawn at (607.55,526.297) ---
      EDGE object-pressed      <- "piece-white-pawn-4-1"
      EDGE object-focused      <- "piece-white-pawn-4-1"
      EDGE object-drag-started <- "piece-white-pawn-4-1"
      EDGE object-released     <- "piece-white-pawn-4-1"
      EDGE object-drag-ended   <- "piece-white-pawn-4-1"
  pawn isSelected = false  selectionActive = false  targetX=-1 targetY=-1
--- human click on the e4 square ---
      EDGE object-drag-ended   <- "object.chess.board"
  pawn now at (4,1)  turn=0
RESULT: **pawn did not move**
```

The ray found the pawn. `object-pressed` names it correctly. Nothing downstream fires, because
`law-chess-click`'s only trigger is `object-clicked`.

**Threshold sweep** (travel is diagonal, so the reported jitter is per-axis):

| per-axis jitter | travel | result |
|---|---|---|
| 0 px | 0.0 px | pawn moves |
| 3 px | 4.2 px | pawn moves |
| 5 px | 7.1 px | **nothing** |
| 7 px | 9.9 px | **nothing** |
| 10 px | 14.1 px | **nothing** |

Holding the button longer does not matter (`--hold=1` through `--hold=20` all behave the same);
only travel does.

**Cursor locked (the boot state), 0 px of travel:**

```
--- human click on the e2 pawn at (607.55,526.297) ---
      EDGE object-pressed <- "object.chess.board"
      EDGE object-clicked <- "object.chess.board"
  pawn isSelected = false  targetX=3 targetY=6
RESULT: **pawn did not move**
```

The click *is* published — it just names the board square under the crosshair, two files and
five ranks away from where the Person was pointing.

---

## 3. Why the piece's on-screen size makes 6 px the wrong number

At `chess_app`'s saved camera — `pos (0, 10, -12)`, front `(0, -0.574, 0.819)`, 45° vertical
FOV — a pawn is ~0.44 world units across at ~13.6 units of range. On a 1280×720 window that is
**about 28 window points across, so a radius of 14**. The slop that decides "this was a drag,
not a click" is 6 — under half the radius of the thing being clicked. The gesture classifier is
strictly stricter than the pick it is classifying.

For comparison, the platform conventions this constant is standing in for are all measured with
a time component or against a larger box: Win32 `SM_CXDRAG` is 4 px at 96 DPI *and* AppKit's
drag hysteresis is paired with a dwell interval; browsers use ~5 CSS px. None of them is 6
device points with no time term on a Retina trackpad.

---

## 4. Why the previous three attempts missed

Each found something real and none of them was this.

- **`CHESS_APP_SELECTION_RACE_CONDITION_2026-08-24.md`** — a genuine race, genuinely fixed.
  Downstream of the gesture.
- **`CHESS_APP_LAW_AUDIT_2026-08-24.md`** — distant sliding check. Downstream of the gesture.
- **`CHESS_APP_CLICKING_DOES_NOTHING_2026-08-25.md`** — claimed the Load World panel staying
  open holds `ImGui::GetIO().WantCaptureMouse` true "while any ImGui window is open". **That
  premise is false.** ImGui raises `WantCaptureMouse` when the pointer is *over* a window, when
  an item is active, or for a modal — `Load World` is a plain `ImGui::Begin`
  (`AssetsConsole.cpp:75`), not a modal, so an open-but-unhovered panel captures nothing. The
  fix that shipped (closing the panel on Load) is a good manners fix and should stay; it was
  never the cause, which is why closing it changed nothing for Zach.
- **`fba21131`** — changed `dragTotal` from accumulated per-frame deltas to net displacement
  from the press point. Correct change, and it makes the threshold *measure the right
  quantity*. It left the threshold at 6, which is the part that bites.

The reason all three passed their tests is structural, and it is worth naming: **`chess_app_test`
hand-publishes `ECA::Event{"object-clicked", …}`, and `chess_click_geometry_test` calls
`observe()` with `pointerX`/`pointerY` left at their default 0 for the ray-driven cases and with
motion only *before* the press for the camera case.** Neither ever moves the pointer *between
press and release*. The one behaviour that decides whether a Person's gesture is a click is the
one behaviour no test varies.

---

## 5. What is NOT wrong (checked this pass, all sound)

- The law chain — 41 authored laws load, bind their triggers, and cascade correctly.
- Authorship — `grok-4.6` is a being in the world at `loadFromJson` time; no law comes up
  `Unauthored`.
- Picking geometry for all six piece shape kinds (re-confirmed via `chess_click_geometry_test`).
- The zone identity store — `saves/zones/Chess/zone.json` and the session snapshot agree on all
  35 objects and all 38 formation relations.
- `interaction-channel` is enabled after load (`firstMoverEnabled` restores it true).
- The ray arithmetic in `step()` — verified against `EngineRender`'s matrices for both
  `frustumNO` and `frustumZO` depth conventions, and against Retina framebuffer scaling.
- The piece-to-square mapping — a piece is always smaller than its square, so a hit point on a
  piece's surface can never round into a neighbour and trip `law-chess-deselect-others`.

---

## 6. One further defect, found while reading, not yet reproduced live

`InteractionChannel::step()` **polls** the button level once per frame
(`glfwGetMouseButton`, `InteractionChannel.cpp:323`) rather than draining an edge latched in the
GLFW callback — which is what the same file already does for the wheel (`noteScroll`, and
`Engine::registerCallbacks` already has a mouse-button callback it publishes `onMouseClicked`
from). A press-and-release that completes between two polls is therefore dropped whole. At 60 fps
the window is 17 ms and a human click survives; in a frame-starved world it does not. `chess_app`
is the world the Performance section of the to-do list is about. This is a latent second cause of
"nothing happened", independent of the two above, and it should be fixed regardless of whether it
is currently firing.

---

## 7. The doctrinal reading

`kClickSlopPixels` is a `static constexpr float` in a header. It decides what a Person's gesture
*means* — whether the world was clicked or dragged — and no law can read it, write it, or know it
exists. That is Refusal 6 exactly: *"nobody registered it yet" is not a permission level*. The
channel is supposed to sense and publish; deciding that this motion was a drag is a meaning
decision, and it is currently made behind a number the ontology cannot see. The fix in
[`docs/CHESS_GESTURE_HANDOFF_2026-08-26.md`](CHESS_GESTURE_HANDOFF_2026-08-26.md) §2 puts it
on the property registry where the rest of the channel's vocabulary already lives.
