# Chess App Selection Race Condition Audit
**Date:** 2026-08-24
**Author:** Antigravity

## The Symptom
The user reported an inability to select and move pieces in the live Earthcall chess application ("I AM TRYING TO MOVE THE PAWNS AND I CANT"). 

The automated test suite (`chess_app_test.cpp`) was passing flawlessly because it mocked `object-clicked` and did not expose the specific non-deterministic execution order that caused the live app to fail.

## The Architectural Flaw
The issue stemmed from an event race condition between two laws that were both bound to `object-clicked`:
1. `law-chess-click`: Computes the grid coordinates of the raycast hit (`pointerWorld`) and maps them to `@state.chess.targetX` and `targetY`. It then publishes `square-clicked`.
2. `law-chess-select`: Maps the clicked piece's coordinates to `selectedX` and `selectedY`, sets `isSelected = true`, and publishes `piece-selected`.

Earthcall's Rete network and event bus **do not guarantee execution order** for multiple laws triggering on the same event. 

In the live application, if the agenda drained `law-chess-select` *before* `law-chess-click`:
1. The piece became selected, and `piece-selected` was published.
2. The event cascaded to `law-chess-deselect-others`, which exists to clear previous selections. It does this by checking if the currently evaluated piece's coordinates do NOT match `@state.chess.targetX` and `targetY`.
3. Because `law-chess-click` had not yet executed, `targetX` and `targetY` still held the coordinates of whatever square was clicked on the *previous* turn.
4. The coordinates mismatched, causing the engine to instantly evaluate the newly clicked piece as an "other" piece and deselect it (`isSelected = false`).

When the user subsequently clicked a destination square, the move laws (which require `IS_SELECTED`) evaluated to false, preventing any movement.

## The Fix
To enforce sequential execution without relying on implicit agenda ordering, the trigger for `law-chess-select` was migrated from the raw hardware edge (`object-clicked`) to the semantic edge (`square-clicked`).

```python
    add_law(
        "law-chess-select",
        "select-own-piece",
        0,
        ["square-clicked"], # Previously "object-clicked"
        ...
```

**Why this works:**
1. The user clicks a piece (`object-clicked`).
2. `law-chess-click` evaluates, maps the raycast to the discrete grid, sets `targetX`/`targetY` definitively, and publishes `square-clicked`. (Note: `publish("square-clicked")` preserves the subject of the original event).
3. `law-chess-select` now triggers on `square-clicked`. It securely sets the piece to selected and publishes `piece-selected`.
4. `law-chess-deselect-others` evaluates. Because `targetX` and `targetY` were guaranteed to be updated in step 2, the newly selected piece perfectly matches the target coordinates and is spared from deselection.

This completely resolves the silent deselection bug and strictly aligns the selection lifecycle with Earthcall's event cascades.
