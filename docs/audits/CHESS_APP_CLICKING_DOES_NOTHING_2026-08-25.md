# Chess App: "Clicking Does Nothing" — Audit and Fix
**Date:** 2026-08-25
**Author:** Claude Sonnet 5
**Session:** https://claude.ai/code/session_01FHTbwykpebnPbM4Lrh1XmB
**Asked by:** Zach — "can you fix the chess app clicking-does-nothing issue (or create the
logic if theres functionality gaps)"

## What was actually wrong

After loading a world from the **Load World** ImGui panel (menu → Load, or `L`), the panel
never closes itself on a successful load. `Rendering::loadWorld()` calls
`mgr.loadState(path, ctx)` and returns; nothing clears
`ZoneManager::SaveLoadState::showLoadWindow`. The window stays open, and while any ImGui
window is open, `ImGui::GetIO().WantCaptureMouse` is true. `Engine::update` passes that flag
straight into `InteractionChannel::step(..., WantCaptureMouse)`, which makes `observe()`
"blind" (`InteractionChannel.cpp:116`): no picking, no press/release edges, no
`object-clicked` — for *anything* in the world, not just chess.

So: load `chess_app`, and every subsequent click is silently eaten by the still-open Load
panel. To a Person this looks exactly like "I click a piece and nothing happens" — which is
the literal bug report — because from the world's point of view, nothing *did* happen; the
click never left ImGui.

This is a general interaction bug, not a chess-specific one. It happened to be maximally
visible in chess because chess is a click-heavy, walk-light activity: a Person playing chess
loads the save and immediately starts clicking, with no WASD movement in between that might
have prompted them to notice the panel was still there or to press Escape/M and dismiss it.

**The fix** (`AssetsConsole.cpp`): the Load button now closes `showLoadWindow`
(`sl.showLoadWindow = false`) after a successful load, matching what the **Save** window
already does for itself (`sl.showSaveWindow = false` on Save). The "Restore unsaved" button
and the Save Manager's small "Load" button got the same fix, for the same reason. Cursor
lock is untouched — `ensureCursorUnlocked()` already runs when the panel opens (existing
convention, shared with every other tool window), and nothing re-locks it, so the Person
lands back in the world with a visible, usable cursor and can click immediately.

## What was checked and is NOT the bug

Two prior sessions had already found and fixed real bugs here — a selection race condition
(`CHESS_APP_SELECTION_RACE_CONDITION_2026-08-24.md`) and a distant-sliding-check gap
(`CHESS_APP_LAW_AUDIT_2026-08-24.md`). Both fixes are intact. Before concluding the panel was
the culprit, this pass verified the rest of the pipeline directly, because `chess_app_test.cpp`
cannot see it — it hand-publishes `ECA::Event{"object-clicked", subject, ...}` with a subject
it already knows is correct, never calling `InteractionChannel::observe()` /
`Object::raycastFace()` at all. That is exactly the gap the selection-race audit named
("the automated test suite was passing flawlessly because it mocked object-clicked") and it
was still open.

- **Picking geometry for every piece shape chess uses.** `Object::raycastFace` in
  `ObjectRaycast.cpp` has a legacy `switch(_shapeKind)` covering only
  `Cube/Sphere/Cylinder/Cone/Polyhedron` — Ellipsoid (knight) and Ovoid (queen) are not in
  it. This looked like the bug at first read. It is not: `Object::setShape()` routes
  `Sphere/Cylinder/Cone/Ellipsoid/Ovoid/Paraboloid/Torus/RoundedBox` through the
  topology-based model (`_hasSmooth`/`_hasComplex`), which `raycastFace` checks *before*
  reaching that switch (`ObjectRaycast.cpp:58-78`). The switch is dead code for every named
  shape reachable through `setShape`/`from_json`'s `shapeKind` path — confusing, but not the
  cause. `Quadric::raycast` (Ellipsoid) and `raycastParametric`/`ovoidSDF` (Ovoid) both work
  correctly.
- **The real end-to-end click chain** (`InteractionChannel::observe()` → `pickSurface` →
  `object-clicked` → `law-chess-click` → `square-clicked` → `law-chess-select` → move/capture
  laws), driven exactly the way a Person's mouse drives it — not by hand-publishing the
  event. New regression test `tests/law/chess_click_geometry_test.cpp` proves both: a
  straight-down ray correctly picks one piece of each of the six shape kinds
  (`pawn/rook/knight/bishop/queen/king`), and a real press-then-release click sequence
  selects the e2 pawn and moves it to e4 through the actual engine machinery. The first
  attempt at this test produced two false alarms — a `Universe::setProvider` missing
  `categories.getAll()`/`materials.getAll()` (laws came up `Unauthored` — the "authored-by"
  relation to `grok-4.6` had nothing to bind to) and a missing `setRelationProvider` (the
  `IS_PIECE` condition, an `instance-of` relation check, always failed) — both artifacts of
  the test's own setup, not engine bugs. Fixed those, then the click chain worked cleanly.
  Left as a permanent regression precisely because it closes the gap the race-condition audit
  flagged as still open.

## What is still open (named, not fixed here)

- **No visual feedback for `isSelected`.** `scripts/author_chess.py` sets/clears
  `isSelected` for gating move legality; nothing paints, lifts, or otherwise marks the
  selected piece. This is Zach's other standing complaint (`To-do list.md`: "I need to be
  able to click on a piece and see visual feedback its the game-selected piece"). Out of
  scope for this pass — it is a feature to author (a law reacting to `isSelected`), not a
  bug to fix, and deserves its own pass so the shared-Material footgun
  (`AGENTS.md` Non-negotiables — paint via `Object::setFaceColor`/`ownMaterial`, never a
  shared `materials.resolve`) is respected if color is the chosen signal, or a Y-lift is used
  instead to sidestep it entirely.
- **Click-and-drag to move**, as an alternative to click-click, is the same to-do item and
  also not attempted here.

## Verified

- `chess_click_geometry_test` (new): 6/6 shape-kind picks, full real click→select→move
  sequence. Passes.
- `chess_app_test`: still green (65/65 → now 67 registered with the two additions from this
  session; this file is unaffected by the load-window fix since it never opens that window).
- Full clean build + `ctest`: 66/67 pass; the one failure (`frame_lag_test`) is the
  documented "do not measure lag on a busy laptop" case (`To-do list.md`, Performance) — two
  consecutive runs on this machine reported *different* metrics as the outlier with
  calibration drift 1.02x-1.15x against a machine running at 1.5x-2.8x its idle baseline,
  and this pass touched no per-frame or load-timing code path (three added lines assign an
  ImGui `bool`; everything else here is a new test file and documentation).
