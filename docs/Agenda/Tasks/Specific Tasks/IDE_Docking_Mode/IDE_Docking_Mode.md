# IDE Docking Mode for First Mover Window Tools

**Task:** Provide a toggleable mode that attaches ImGui windows to the screen edges as sidebars (Left, Right) and a bottom bar drawer (like an IDE), leaving the center 3D viewport clean and unobstructed.

**Status:** Landed and verified via unit tests (`ide_dock_manager_test` 9/9) and WebGPU compilation (2026-09-11). Manual in-app verification entry recorded in `Person Verification List.md`.

## Details
- **Architecture**:
  - Implemented `IDEDockManager` under `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/IDEDockManager.hpp` / `.cpp`.
  - Dear ImGui in the repository is 1.92.0 on the master branch without native dock space (`ImGuiConfigFlags_DockingEnable` is absent). Rather than vendor a separate ImGui fork or inject foreign docking libraries, `IDEDockManager` renders native viewport-edge containers (`DockSlot::Left`, `DockSlot::Right`, `DockSlot::Bottom`, and `DockSlot::Floating`).
  - Slots support tabbed panel hosting, active tab selection, panel collapsing/expanding, interactive edge splitters with resize mouse cursors (`ImGuiMouseCursor_ResizeEW` / `ResizeNS`), and per-panel pop-out float toggles (`[❐ Float]` / `[◧]`/`[◨]`/`[⬓]`).
  - Edge panels leave the central viewport clear for 3D world rendering and interactions.
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
  - Top workspace status pill bar with quick toggle, panel count, and reset layout button.
- **Verification**:
  - Unit tests: `tests/ide_dock_manager_test.cpp` (9/9 passed).
  - Clean compilation: `earthcall_webgpu` builds and links without warnings/errors.
  - Person verification: Check added to `docs/Agenda/Tasks/For Zach/Person Verification List.md`.
