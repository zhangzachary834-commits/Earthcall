# Handoff: make a Person's gesture reach the chess board

**Date:** 2026-08-26
**Author:** Claude Opus 5
**Session:** https://claude.ai/code/session_01Ewyb3adQJuSmsgG2x5Yp7p
**For:** whoever picks this up next
**Root cause:** [`docs/audits/CHESS_APP_EVERY_GESTURE_IS_A_DRAG_2026-08-26.md`](CHESS_APP_EVERY_GESTURE_IS_A_DRAG_2026-08-26.md)
**Reproduction:** `scratch/probes/chess_app_full_loop_probe.cpp`

---

## Read this first

The chess laws are fine. The picking is fine. The save is fine. **Do not audit them again — three
sessions already did, and each one shipped a real but unrelated fix while the Person's gesture
kept vanishing.**

What is broken is the gesture classifier in `InteractionChannel::observe()`. Any pointer gesture
that travels more than **6 window points** between button-down and button-up is published as
`object-drag-started` / `object-drag-ended` instead of `object-clicked`. A trackpad click clears
6 points routinely. `saves/worlds/chess_app.json` binds **only** `object-clicked`. So the world
hears the Person, correctly identifies the pawn under their pointer, and then discards the
gesture because it arrived under a name nothing was written to answer.

Zach asked for drag in the first place (`To-do list.md` line 218: *"be able to actually move it
if I drag to another square"*). Drag was never authored. Both halves of his report — clicking
does nothing, dragging does nothing — are the same defect: **every gesture he makes is a drag,
and drag has no law.**

Confirm it before you change anything:

```sh
cp scratch/probes/chess_app_full_loop_probe.cpp tests/law/
cmake -S . -B build <the flags in CLAUDE.md>
cmake --build build --target chess_app_full_loop_probe -j8
./build/chess_app_full_loop_probe              # still hand: pawn walks e2-e4
./build/chess_app_full_loop_probe --jitter=5   # real hand: object-drag-ended, nothing moves
./build/chess_app_full_loop_probe --locked     # boot cursor state: every click lands on (3,6)
```

Take the copy back out of `tests/` and reconfigure when you are done, or the glob leaves a
phantom target behind.

---

## The work, in the order it should land

### 1. Author the drag gesture into the chess app — the feature Zach actually asked for

**File:** `scripts/author_chess.py` → regenerate `saves/worlds/chess_app.json`.
**Do not hand-patch the JSON.** The generator is the source of truth for this world, and the save
is one of Zach's; regenerate it and say in your report which file you wrote, which beings you
added, and who is recorded as their author.

Two edits. Both work because `law-chess-click` reads the destination from
`@interaction-channel.pointerWorld`, which on any frame is the point under the pointer — the
event's *subject* never had to be the destination square.

**(a) A drag that starts on a piece selects it.** Add a law next to `law-chess-click`:

```python
add_law(
    "law-chess-drag-pick",
    "select-dragged-piece",
    0,                          # Activation::OnEvent
    ["object-drag-started"],
    IS_PIECE,                   # NOT the board — starting a drag on empty board must not move anything
    seq(
        {"kind": 8, "path": "@state.chess.targetX",
         "bindings": {"ptrX": "@interaction-channel.pointerWorld.x"},
         "function": {"input": "ptrX", "pieces": pointer_bins()}},
        {"kind": 8, "path": "@state.chess.targetY",
         "bindings": {"ptrZ": "@interaction-channel.pointerWorld.z"},
         "function": {"input": "ptrZ", "pieces": pointer_bins()}},
        publish("square-clicked"),
    ),
    scope=0,                    # Scope::Subject
)
```

The existing `law-chess-select` hears `square-clicked` with the piece as subject and selects it,
exactly as a click does today. No change to `law-chess-select`.

**(b) Releasing the drag moves the piece.** Add a second law:

```python
add_law(
    "law-chess-drag-drop",
    "drop-onto-square",
    0,
    ["object-drag-ended"],
    # Guard: object-drag-ended is published with the DRAGGED PIECE as subject even when the
    # pointer has left the world, and pointerWorld is then (0,0,0) — which the bins read as
    # square (4,4). Without this, letting go off the board teleports the piece to e5.
    compare("@interaction-channel.hoveredId", 1, pv("string", "")),   # Op::Ne
    seq(
        {"kind": 8, "path": "@state.chess.targetX",
         "bindings": {"ptrX": "@interaction-channel.pointerWorld.x"},
         "function": {"input": "ptrX", "pieces": pointer_bins()}},
        {"kind": 8, "path": "@state.chess.targetY",
         "bindings": {"ptrZ": "@interaction-channel.pointerWorld.z"},
         "function": {"input": "ptrZ", "pieces": pointer_bins()}},
        map_path("@state.chess.dx",
                 {"tx": "@state.chess.targetX", "sx": "@state.chess.selectedX"},
                 [{"c": 1.0, "factors": {"tx": 1.0}}, {"c": -1.0, "factors": {"sx": 1.0}}]),
        map_path("@state.chess.dy",
                 {"ty": "@state.chess.targetY", "sy": "@state.chess.selectedY"},
                 [{"c": 1.0, "factors": {"ty": 1.0}}, {"c": -1.0, "factors": {"sy": 1.0}}]),
        # The BOARD as subject, deliberately. The eight move laws are Scope::Everyone and read
        # isSelected, so the subject is nothing to them — but law-chess-select is Scope::Subject,
        # and publishing the piece here would re-select it in the same round as the move that
        # clears isSelected. Publishing the board makes select's IS_PIECE condition fail, which
        # is exactly what a click on a destination square does today.
        publish("square-clicked", "object.chess.board"),
    ),
    scope=0,
)
```

**Authorship constraint, do not get this wrong:** `LawManager::loadFromJson` reattaches authors
**by identifier against `Universe::beings()` and has no fallback** (unlike the merge path in
`ZoneManager.cpp:1348`). A law whose author is not a being in this world loads `Unauthored` and
`Law::applyTo` refuses to fire it — silently, from the Person's side. `grok-4.6` is a being in
`chess_app` (a category, `categories[4]`), which is why `AUTHOR` in the generator works. If you
want a different author on these two laws, **add that being to the world in the same pass** or
they will not fire, and the failure will look exactly like the bug you are fixing.

Follow the injection convention: `injected_by:` names you, `authors:` names the Person by whose
authority you acted.

### 2. Put the click/drag threshold where a law can see it

**File:** `src/Singularity/Input/Interaction/InteractionChannel.{hpp,cpp}`

`static constexpr float kClickSlopPixels = 6.0f` (`InteractionChannel.hpp:159`) decides what a
Person's gesture *means* — click or drag — and no law can read it, write it, or know it exists.
That is Refusal 6 on its face: *"nobody registered it yet" is not a permission level*. It is also
simply the wrong number: at `chess_app`'s own camera a pawn is ~28 window points across, so the
gesture classifier is stricter than the pick it classifies.

- Make it a member, `float clickSlopPixels`, registered in `buildProperties()` alongside
  `dragTotalX` / `dragging` — the vocabulary it belongs to.
- Raise the default. It needs to be resolution-independent: `dragTotal` is measured in *window*
  points while the ray is unprojected through *framebuffer* pixels, so the two disagree by the
  Retina scale today. Pick one space and say which in a comment.
- Consider a dwell term as well as a distance term, which is what the platform conventions this
  constant is standing in for actually do (Win32 `SM_CXDRAG` + double-click time; AppKit's
  hysteresis + interval). Distance alone cannot tell a shaky click from a short drag.

⚑ AUTHOR — the default value is Zach's call, not ours. Ask before picking a number; it is a
Person-facing feel constant.

### 3. Stop dropping clicks that fall between two frames

**File:** `src/Singularity/Input/Interaction/InteractionChannel.cpp:323`, and
`Engine::registerCallbacks` in `src/Singularity/Core/EngineInit.cpp:453`.

`step()` polls the button **level** once per frame with `glfwGetMouseButton`. A press-and-release
that completes between two polls is dropped whole. The same file already does the right thing for
the wheel — `noteScroll()` latches in the GLFW callback and `step()` drains it — and
`registerCallbacks` already installs a mouse-button callback (it publishes `onMouseClicked` from
there). Latch press/release edges the same way and drain them in `step()`.

At 60 fps the window is 17 ms and a human click survives. `chess_app` is the world the
**Performance** section of the to-do list is about. This is latent, independent of §1 and §2, and
worth closing regardless of whether it is currently firing.

### 4. Make the crosshair honest, or unlock it

**File:** `src/Singularity/Input/Interaction/InteractionChannel.cpp:358-365`

While the cursor is locked (`GLFW_CURSOR_DISABLED` — the **boot** state, `Engine.cpp:171`), the
pick ray leaves the viewport centre, not the pointer, and **nothing draws a reticle**. Every click
lands on whatever the camera is aimed at; for `chess_app`'s saved camera that is square (3,6),
deterministically, forever. See `--locked` in the probe.

Two things, both small:

- Register `pointerLocked` as a channel property so the state is legible to law at all (Refusal 6
  again — right now the pick's own frame of reference is invisible from inside the world).
- Draw the reticle when locked. A ray the Person cannot see the origin of is a black box aimed at
  their world.

Note that the `M` key re-locks the cursor on menu close (`EngineInit.cpp:344-357`) and `Escape`
toggles it, so a Person can land back in locked mode after loading a world without any signal
that their pointer stopped meaning anything.

### 5. Visual feedback for `isSelected` — still open, and it gates verification

Nothing paints, lifts, or marks the selected piece. This has been "still open" in three
consecutive audits, and it is the reason none of them could be closed by a Person: **with no
feedback, a Person cannot tell a working first click from a swallowed one**, so every one of these
bugs presents identically. Fix it early in the next pass, not last.

If colour is the signal, go through `Object::setFaceColor` / `Object::ownMaterial` — never
`materials.resolveOrDefault(obj->materialId())`, which repaints every piece sharing
`material.chess.white`. A Y-lift sidesteps the shared-Material problem entirely and is probably
the better first move.

### 6. Promote the probe, and close the hole it found

`scratch/probes/chess_app_full_loop_probe.cpp` should become a test — but the test that matters is
the one that **moves the pointer between press and release**. That is the single behaviour that
decides whether a Person's gesture is a click, and neither existing test varies it:
`chess_app_test` hand-publishes `object-clicked` with a subject it already knows is right, and
`chess_click_geometry_test` moves the pointer only *before* the press.

Add cases for: a click with human-scale travel selects; a drag from piece to square moves; a drag
released off the board does nothing; a click with the cursor locked is not silently answered by
the crosshair.

---

## What I verified this pass, so you don't repeat it

Sound, do not re-audit: the 41-law chain and its trigger bindings; authorship binding of
`grok-4.6`; picking geometry for all six piece shape kinds; the `saves/zones/Chess/zone.json`
identity store (35 objects, 38 relations, agrees with the session snapshot);
`interaction-channel` enabled after load; the ray arithmetic in `step()` against `EngineRender`'s
matrices for both `frustumNO` and `frustumZO` and against Retina scaling; and the
piece-to-square mapping (a piece is always smaller than its square, so a hit point on a piece can
never round into a neighbour and trip `law-chess-deselect-others`).

Wrong, and now superseded: the 2026-08-25 audit's claim that an open `Load World` panel holds
`WantCaptureMouse` true. `Load World` is a plain `ImGui::Begin`, not a modal
(`AssetsConsole.cpp:75`); an open-but-unhovered panel captures nothing. The panel-closes-on-Load
change that shipped with it is good manners and should stay — it was just never the cause, which
is why closing it changed nothing.

## What Zach should see when this lands

Load `chess_app` from the Load World panel. Press and hold on a white pawn, move to another
square, let go: the pawn is there. Click a pawn and click a square, with an ordinary un-steady
hand: the pawn moves. Neither should require holding the mouse still.
