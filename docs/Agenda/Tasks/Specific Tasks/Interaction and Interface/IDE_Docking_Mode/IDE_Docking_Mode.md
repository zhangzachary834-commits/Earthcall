# IDE Docking Mode for First Mover Window Tools

**Task:** Provide a toggleable mode that attaches ImGui windows to the screen edges as sidebars (Left, Right) and a bottom bar drawer (like an IDE), leaving the center 3D viewport clean and unobstructed.

**Status:** Landed and verified via unit tests (`ide_dock_manager_test` 13/13) and WebGPU compilation (2026-09-11). Manual in-app verification entry recorded in `Person Verification List.md`.

## Details
- **Architecture**:
  - Implemented `IDEDockManager` under `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/IDEDockManager.hpp` / `.cpp`.
  - Dear ImGui in the repository is 1.92.0 on the master branch without native dock space (`ImGuiConfigFlags_DockingEnable` is absent). Rather than vendor a separate ImGui fork or inject foreign docking libraries, `IDEDockManager` renders native viewport-edge containers (`DockSlot::Left`, `DockSlot::Right`, `DockSlot::Bottom`, and `DockSlot::Floating`).
  - Slots support tabbed panel hosting, active tab selection, panel collapsing/expanding, interactive edge splitters with resize mouse cursors (`ImGuiMouseCursor_ResizeEW` / `ResizeNS`), and per-panel pop-out float toggles (`[❐ Float]` / `[◧]`/`[◨]`/`[⬓]`).
  - Edge panels leave the central viewport clear for 3D world rendering and interactions.
- **Dynamic Corner Collision Resolution (Zero Dead Space)**:
  - Solves the corner void problem when Left/Right sidebars and Bottom drawer are open simultaneously.
  - Slot sequence counter tracks the most recently activated, opened, or interacted panel (`_leftSlotSeq`, `_rightSlotSeq`, `_bottomSlotSeq`).
  - If Left was touched more recently than Bottom (`_leftSlotSeq > _bottomSlotSeq`), Left Sidebar takes full height to the bottom of the screen, and Bottom Bar starts flush at `leftWidth`.
  - If Bottom was touched more recently (`_bottomSlotSeq >= _leftSlotSeq`), Bottom Bar extends to `X = 0`, filling the corner completely, and Left Sidebar shrinks flush above it.
  - Symmetrically applied for Right Sidebar vs Bottom Bar.
- **Stacked Multi-Window Mode**:
  - In addition to `SlotLayout::Tabbed`, sidebars and drawer support `SlotLayout::Stacked`.
  - Sidebars: Vertically stacked panes with accordion collapse (`[▼]` / `[▶]`), individual titles, float/dock/close buttons, child scroll regions, and interactive draggable splitters (`ResizeNS`).
  - Bottom Bar: Side-by-side columns with vertical column splitters (`ResizeEW`).
  - Quick toggle button `[☷ Stack]` / `[▤ Tabs]` available directly in panel headers and the top workspace bar whenever multiple windows are docked in a slot.
- **Window Content Extraction**:
  - `CreatorConsoleWindow`: extracted `renderCreatorConsoleContent()` supporting both native MenuBar and TabBar fallback.
  - `DeveloperToolsWindow`: extracted `renderDeveloperToolsContent()`.
  - `CreationWindow`: extracted `renderCreationContent()`.
  - `PerformanceMetricsWindow`: extracted `renderPerformanceMetricsContent()`.
  - `Chat`: extracted `renderContent()`.
  - `Engine`: extracted `renderKeymapContent()`.
- **Default Slot Layout**:
  - Left Sidebar: Creator Console (`F8`), Developer Tools (`` ` ``), Singular Creation (`F9`).
  - Right Sidebar: Performance & Coordinates (`F3`), Controls & Keymap (`K`).
  - Bottom Drawer: Chat (`H`).
- **Activation & Keybind**:
  - Toggle key: `F10` (bypasses ImGui keyboard capture via `KeyboardHandler.cpp`).
  - In-world menu: Main Menu (`M`) -> "Toggle IDE Mode".
  - Top workspace status pill bar with quick toggle, panel count, stacked layout switchers, and reset layout button.
- **Verification**:
  - Unit tests: `tests/ide_dock_manager_test.cpp` (13/13 passed).
  - Clean compilation: `earthcall_webgpu` builds and links without warnings/errors.
  - Person verification: Check added to `docs/Agenda/Tasks/For Zach/Person Verification List.md`.
