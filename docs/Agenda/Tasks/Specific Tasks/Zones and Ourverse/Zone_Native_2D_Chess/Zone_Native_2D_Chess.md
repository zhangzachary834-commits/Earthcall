# Zone-Native 2D Chess (`Chess2D`)

*Gemini 3.8 Flash (the sparkly guy) · session `e1346ea7-7d86-4945-8831-01e3e33dd09a` · 2026-10-16. Commissioned by Zach: "MAKE NOT 3D CHESS MAKE 2D CHESS. MAKE SURE UR WORKING IN THE DEFAULT sync-from-earthcall-main CHECKOUT."*

---

## 1. Ontological Telos & Architecture

2D Chess is a pure, screen-space interactive game zone authored entirely as in-world data. It adheres strictly to the **Seven Refusals** (`GEMINI.md`):
1. **Refusal 1 & 5 (No domain C++ classes):** No `class ChessPiece`, no `class ChessBoard`, no `class ChessEngine`. Pieces, squares, trays, and UI are authored `Object` beings with dynamic and planar 2D properties.
2. **Refusal 2 (No top-level directories):** Authored in `saves/zones/Chess2D/` and `saves/laws/law-chess2d-*/`.
3. **Refusal 3 (Append-only enums):** Uses existing `ShapeKind::Shape2D` (`12`) and screen-space coordinates (`x2D`, `y2D`, `zOrder2D`).
4. **Refusal 6 (No black box):** All state is fully exposed as readable/writable properties under `state.chess2d` and on individual beings.
5. **Refusal 7 (No hard-coded behavior methods):** All rules, turn flipping, legal piece movements, captures, selection highlights, and HUD banners are driven entirely by 19 discrete Law roots.

---

## 2. In-World Data Manifestation

### Zone Identity (`saves/zones/Chess2D/zone.json`)
- **Identifier:** `Chess2D`
- **Owner:** `Zach` (Person)
- **Scope:** `1` (Zone-scoped)
- **Authors:** `Zach` (Person), `Gemini` (First Mover)

### Beings
1. **State Tracker (`state.chess2d`):**
   - `turn`: `0` (White), `1` (Black).
   - `selectedX`, `selectedY`: Grid coordinates of currently selected piece (`-1` if none).
   - `targetX`, `targetY`: Grid coordinates of clicked destination square or target piece.
   - `dx`, `dy`: Direction vector from selection to target ($dx = targetX - selectedX$, $dy = targetY - selectedY$).
   - `selectionActive`: Boolean flag indicating an active selection.
   - `nextWhiteCapturedSlot`, `nextBlackCapturedSlot`: Sequential indices for captured pieces in side trays.
   - `gameOver`: `0` (active), `1` (White won), `2` (Black won).
2. **Board Frame & Capture Trays:**
   - Planar border (`object.chess2d.frame`, 504×504 mahogany with ivory inner lip).
   - Left side tray (`object.chess2d.tray.white`, $x2D=60$, $y2D=100$, width 56, height 480) for pieces captured from White.
   - Right side tray (`object.chess2d.tray.black`, $x2D=644$, $y2D=100$, width 56, height 480) for pieces captured from Black.
3. **64 Board Squares (`square-gx-gy`):**
   - Alternating ivory (`[0.91, 0.85, 0.75]`) and dark walnut (`[0.52, 0.38, 0.26]`) materials.
   - Properties: `gridX`, `gridY`, `isSquare=true`.
   - Dimension: $60 \times 60$ px each, spanning $x \in [0, 7]$, $y \in [0, 7]$.
4. **32 Chess Pieces (`piece-<color>-<role>-<gx>-<gy>`):**
   - White pieces (`chessColor=0`, ivory finish, $zOrder2D=10$).
   - Black pieces (`chessColor=1`, slate finish, $zOrder2D=10$).
   - Typography glyph labels: `P` (Pawn), `R` (Rook), `N` (Knight), `B` (Bishop), `Q` (Queen), `K` (King).
   - Dimension: $48 \times 48$ px centered inside $60 \times 60$ squares.
   - Properties: `gridX`, `gridY`, `chessRole` (0..5), `chessColor` (0 or 1), `isSelected` (bool), `hasMoved` (bool), `onBoard` (bool).
5. **HUD & Banners:**
   - Header banner (`ui.chess2d.title`, "EARTHCALL 2D CHESS").
   - Dynamic Turn HUD (`ui.chess2d.turn`, "TURN: WHITE" / "TURN: BLACK").

---

## 3. The 19 Discrete Laws

| Law ID | Name | Trigger | Scope | Description |
|---|---|---|---|---|
| `law-chess2d-click` | `handle-chess2d-click` | `object-clicked` | Subject (0) | Triggers on squares or unselected pieces. Maps target grid coordinates, computes `dx`/`dy`, publishes `board-clicked`. |
| `law-chess2d-select` | `select-chess2d-piece` | `board-clicked` | Everyone (1) | Selects unselected friendly piece at target, lifts `zOrder2D` to 20, publishes `piece-selected`. |
| `law-chess2d-deselect-others` | `deselect-other-pieces` | `piece-selected` | Everyone (1) | Automatically deselects previously selected friendly pieces when a new one is selected. |
| `law-chess2d-deselect-self` | `deselect-self` | `object-clicked` | Subject (0) | Clicking an already selected piece clears selection, restores `zOrder2D=10`, publishes `piece-deselected`. |
| `law-chess2d-pawn-w-step` | `white-pawn-step` | `board-clicked` | Everyone (1) | White Pawn 1 square forward ($dx=0, dy=1$). |
| `law-chess2d-pawn-w-double` | `white-pawn-double` | `board-clicked` | Everyone (1) | White Pawn initial 2 squares forward ($dx=0, dy=2$, requires `hasMoved==false`). |
| `law-chess2d-pawn-w-capture` | `white-pawn-capture` | `board-clicked` | Everyone (1) | White Pawn diagonal capture ($|dx|=1, dy=1$). |
| `law-chess2d-pawn-b-step` | `black-pawn-step` | `board-clicked` | Everyone (1) | Black Pawn 1 square forward ($dx=0, dy=-1$). |
| `law-chess2d-pawn-b-double` | `black-pawn-double` | `board-clicked` | Everyone (1) | Black Pawn initial 2 squares forward ($dx=0, dy=-2$, requires `hasMoved==false`). |
| `law-chess2d-pawn-b-capture` | `black-pawn-capture` | `board-clicked` | Everyone (1) | Black Pawn diagonal capture ($|dx|=1, dy=-1$). |
| `law-chess2d-knight` | `move-knight` | `board-clicked` | Everyone (1) | Knight L-shape jump ($|dx|=1, |dy|=2$ or $|dx|=2, |dy|=1$). Can capture enemy. |
| `law-chess2d-rook` | `move-rook` | `board-clicked` | Everyone (1) | Rook orthogonal move ($dx=0 \oplus dy=0$). Can capture enemy. |
| `law-chess2d-bishop` | `move-bishop` | `board-clicked` | Everyone (1) | Bishop diagonal move ($dx^2 - dy^2 == 0$). Can capture enemy. |
| `law-chess2d-queen` | `move-queen` | `board-clicked` | Everyone (1) | Queen orthogonal or diagonal move. Can capture enemy. |
| `law-chess2d-king` | `move-king` | `board-clicked` | Everyone (1) | King 1-square move in any direction ($|dx| \le 1, |dy| \le 1$). Can capture enemy. |
| `law-chess2d-capture` | `capture-victim` | `enemy-captured` | Everyone (1) | Captures victim at target square with opposite color. Sets `onBoard=false`, scales to tray size, moves to side capture tray. |
| `law-chess2d-sync-pos` | `sync-chess2d-positions` | `positions-updated` | Everyone (1) | Syncs all `onBoard` pieces to exact screen coordinates $x2D = 146.0 + 60.0 \cdot gx$, $y2D = 526.0 - 60.0 \cdot gy$. |
| `law-chess2d-turn-white` | `turn-banner-white` | `positions-updated` | Everyone (1) | Updates HUD turn banner to "TURN: WHITE" when `turn == 0`. |
| `law-chess2d-turn-black` | `turn-banner-black` | `positions-updated` | Everyone (1) | Updates HUD turn banner to "TURN: BLACK" when `turn == 1`. |

---

## 4. Verification

- **Test Suite:** `tests/law/chess2d_zone_native_test.cpp` (`chess2d_zone_native_test`).
- **Results:** 85 / 85 checks passed.
  - Initial zone discovery, material hydration, 64 squares, 32 pieces, UI banners.
  - Move 1 (White e2-e4): Passes.
  - Move 2 (Black e7-e5): Passes.
  - Move 3 (White d2-d4): Passes.
  - Move 4 (Black exd4 capture): Captured white pawn moved to side tray ($x2D=70$, $gridX=-1, gridY=-1$, `onBoard=false`), black pawn at $(3, 3)$.
  - Friendly piece selection switching & self-deselect: Passes.
  - Illegal move refusal: Passes.
