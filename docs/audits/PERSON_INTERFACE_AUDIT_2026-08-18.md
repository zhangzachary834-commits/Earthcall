# Audit — Person Interface and Experience — 2026-08-18

Scope: the whole surface a Person touches. The main menu
(`ConstructedBeing/Object/Formation/Menu/Menu.cpp` + its options in `EngineInit.cpp`),
the Creator Console and all seven of its tabs
(`Singularity/FirstMoverWindowTools/CreatorConsole/`), the Developer Tools window,
the Singular Set-to-Set Creation window, the Law Graph window, Chat, Cursor Tools,
the input layer (`Singularity/Input/`), cursor lock, and the ImGui/DPI setup.

Method: read of every Person-facing entry point and every control on it, then a
trace of each control to its consumer (or to nothing). Claims below cite
`file:line`. **Nobody clicked in the running app.** No build was run this
session; nothing here depends on runtime behaviour that reading cannot settle,
except where §10 says so.

Companion: `SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md` audits the creation *path*
in depth. This audit does not repeat it; where a finding here is the same defect
seen at surface scale, it says so and cross-references.

Branch `sync-from-earthcall-main`.

---

## Verdict

**The surface is not one surface. It is a working authoring tool with a stage
set built in front of it.**

Two windows — Singular Set-to-Set Creation (`Screen/CreationWindow.cpp`) and the
Law Graph (`Screen/LawGraphWindow.cpp`) — are the real thing. They name what a
being is before asking you to act on it, they show TransferPolicy tiers as
`kernel` / `open` / `GATED` rather than hiding them, they probe the property
registry instead of hand-listing it, and they say what was made
(`CreationWindow.cpp:349-351`, "Created %zu object(s)"). They are the standard.

The Creator Console — the window a Person is actually pointed at, the one the
main menu opens, the one bound to F8 — is mostly furniture. Of its seven tabs,
**five display data that does not exist and buttons wired to nothing**. Of the
main menu's eighteen options, **eight do nothing at all** and one opens a
different window than its label names.

Two different failures wear the same face here, and §11 separates them: controls
that **lost** their body in the `Game.hpp/cpp` refactor (commit `52689ee3`) and are
recoverable almost verbatim, and tabs that were **written new, already fake**. The
first is a restoration backlog. The second is the serious one.

This is worse than an unfinished feature. An unfinished feature is absent. This
surface tells a Person that Zone_Spawn and Zone_Wilderness exist
(`ZonesConsole.cpp:15-16`), that `material.clay` and `law.gravity` are assets
they hold (`AssetsConsole.cpp:27-34`), that a relation binds them to the Sun
(`RelationsConsole.cpp:21-22`). None of it is read from the world. It is typed into
the C++.

For a repository whose first line of discipline is Transparent Failure and whose
sixth refusal is No Black Box, a surface that fabricates world state is the most
serious thing in the tree. A black box hides what is true. This shows what is
false.

The ontological question the intercom thread opened — is the interface made of
the same beings as everything else, or is it C++ chrome around a world? — is
answered by the code as **chrome**, and the chrome is not honest chrome. That
fork (thread `person-interface-experience`, 2026-08-18) should be decided, but
nothing in it needs deciding before the findings below are fixed: a fabricated
Zones list is wrong under either branch.

---

## §0 — The five criteria

The intercom thread proposed an acceptance test for any Person-facing path:
*find it, see that it is armed, see where it lands, read back what was born, and
take it back.* Scored against the shipped surface:

| Criterion | State | Where |
|---|---|---|
| Find it | **Fails.** Cursor is locked at boot; F8 does not release it. No keymap anywhere; the menu option for one is dead. | §1.1, §1.2 |
| See it armed | **Fails.** The console's Create button and the law's arming bit are different bits; nothing on screen shows `active3DMode`. | §1.5 of the shape-generator audit; §2.6 here |
| See where it lands | **Fails.** The preview function is a comment. | §2.5 |
| Read back what was born | **Partial.** Set-to-Set says so; the console spawn path says nothing. | §4 |
| Take it back | **Fails, and worse than absent.** Undo is bound, visible, and empty. | §3.1 |

---

## §1 — First contact

### 1.1 BLOCKING — the console opens under a locked cursor

`Engine::init` sets `GLFW_CURSOR_DISABLED` (`Core/Engine.cpp:126`) and
`MouseHandler` constructs with `_cursorLocked = true`
(`Input/MouseHandler.cpp:16`). F8 (`EngineInit.cpp:322`), F9
(`EngineInit.cpp:325`) and backtick (`EngineInit.cpp:319`) toggle their windows
open **without releasing the cursor**. With the cursor disabled the pointer is
captured to the window centre and ImGui receives a pointer that runs away; the
window is on screen and cannot be clicked.

The three main-menu options that open the console — Brush Tool, Move Tool,
Character Architect Forge (`EngineInit.cpp:215`, `:223`, `:233`) — each end with
`if (_mouseHandler->isCursorLocked()) _mouseHandler->toggleCursorLock(_window);`.
So the menu path works and the keyboard path does not, and nothing tells the
Person that ESC is the difference.

**Fix:** releasing the cursor belongs with *any* gesture that opens a
pointer-driven window, not with three of five of them.

### 1.2 HIGH — there is no keymap, and the option to show one is dead

`_mainMenu.addOption("Controls / Keymap", GLFW_KEY_K, ...)` has a commented-out
body (`EngineInit.cpp:232`). Nothing else in the tree prints or draws the
bindings. A Person cannot discover ESC, F8, F9, backtick, L, M, F6, F7, or that
`C` teleports them to a zone whose name contains "Character"
(`EngineInit.cpp:337`).

This is the discoverability floor and it is below zero: the interface offers a
control for finding out how to use it, and the control is inert.

### 1.3 HIGH — eight of eighteen main-menu options do nothing

Verified one by one in `EngineInit.cpp`:

| Option | Line | State |
|---|---|---|
| Quick Save | 197 | empty lambda |
| Toggle Chat | 201 | commented out |
| Toggle Toolbar | 202 | commented out |
| Settings | 203 | empty lambda |
| Toggle ImGui Demo | 204 | empty lambda |
| Toggle Placement Insp. | 205 | empty lambda |
| Toggle Selection Insp. | 206 | empty lambda |
| Controls / Keymap | 232 | commented out |

Nine work. One is wrong (§1.4). A Person's first act in this world is a menu
where fewer than half the entries answer.

Six of those eight had a working body before commit `52689ee3` and can be restored
from `52689ee3~1` — see §11. Only **Settings** and **Toggle ImGui Demo** were never
implemented at all; the pre-refactor menu had twelve options and every one of them
answered.

"Quick Save" is the sharpest of these: a Person who builds for an hour, opens
the menu, and presses Quick Save has been told their work is safe. It was not
saved and nothing said so. `Save As...`, `Load` and `Save Manager` on the same
menu all work, which makes the dead one credible.

### 1.4 HIGH — "Toggle Creator Console" opens the other window

`EngineInit.cpp:210` — the option labelled **Toggle Creator Console** with key
F8 toggles `_creationConsoleOpen`, which is the *Singular Set-to-Set Creation*
window (`Engine.cpp:281`). The F8 **key** binding toggles `_creatorConsoleOpen`,
the actual Creator Console (`EngineInit.cpp:322`, `Engine.cpp:285`). The two
flags differ by four letters (`Engine.hpp:115-116`) and the menu picked the
wrong one.

So: press F8, get the Creator Console. Choose the menu item that says F8, get a
different window. Both are open-able; neither says which it is.

### 1.5 MEDIUM — the menu draws eight of its options outside its own panel

`Menu::draw` fixes `panelH = min(360, winH - 80)` (`Menu.cpp:74`), starts the
list at `panelY + 84`, and steps `lineH = 28` (`Menu.cpp:95`). Eighteen options
need `84 + 17·28 = 560` px of a panel that is at most 360 px tall. Every option
from index 10 on — **Toggle Placement Insp. through Quit** — is drawn below the
panel background, floating on the dimmed world. There is no scroll and no
clipping.

### 1.6 MEDIUM — the menu's mouse hit-testing is off by the display scale

`Menu::draw` is called with framebuffer dimensions (`EngineRender.cpp:102`,
`_mainMenu.draw(fbW, fbH)`) and computes the panel in framebuffer pixels.
`Menu::processInput` recomputes the same rectangle from
`glfwGetFramebufferSize` (`Menu.cpp:173`) and then compares it against
`glfwGetCursorPos` (`Menu.cpp:184`), which returns **window** coordinates.

On any display where framebuffer ≠ window size — every Retina Mac, which is this
project's primary platform — the clickable rows sit at twice the coordinates of
the drawn rows. The menu's mouse interaction targets empty space off to the
lower right of where the menu appears. Keyboard navigation (Up/Down/Enter,
`Menu.cpp:151-166`) still works, which is why this can have survived.

`Tool::buildMouseRay` handles exactly this correctly
(`FirstMoverWindowTools/Tool.cpp:115-123`) — it divides framebuffer by window
size and scales. The menu never got the same treatment.

### 1.7 LOW — menu hotkeys fire on level, and an early return leaves edges stale

`Menu::processInput` loops the options testing `glfwGetKey(...) == GLFW_PRESS`
(`Menu.cpp:138`) — a level, not an edge, so a held key re-fires every frame; and
it `return`s on the first match (`Menu.cpp:140`) **before** updating
`_upPressedLast` / `_downPressedLast` / `_enterPressedLast` /
`_mouseLeftPressedLast`. The next frame after a hotkey therefore reads a
held key as a fresh press. This is the identical defect `Tool.cpp:449-454`
documents at length and fixed on the spawn path.

Note also that `S` and `A` are menu hotkeys (Quick Save, Save As) and also the
world's strafe keys. With the menu open, WASD reaches the save dialog.

### 1.8 LOW — six options display their key as `[?]`

`Menu::draw` renders the key label as a character only for
`GLFW_KEY_SPACE..GLFW_KEY_Z` and falls through to `"?"` otherwise
(`Menu.cpp:105-113`). Function keys and backtick are outside that range, so
Settings `[F2]`, ImGui Demo `[F3]`, Placement `[F4]`, Selection `[F5]`, Dev Mode
`` [`] `` and Creator Console `[F8]` all print `[?]`.

The two of those that actually work — Toggle Dev Mode and Toggle Creator
Console — are precisely the two whose key the menu refuses to name.

---

## §2 — The Creator Console is largely a stage set

### 2.1 CRITICAL — five of seven tabs display fabricated world state

- **Zones** (`ZonesConsole.cpp`): 20 lines. "Create New Zone" and "Edit
  Boundaries" are bare `ImGui::Button` calls with no `if`. The "Active Zones"
  list is two string literals, `Zone_Spawn` and `Zone_Wilderness` (`:15-16`). The real
  zones are one `mgr.zones()` call away and boot prints them to stdout
  (`EngineInit.cpp:180`).
- **Relations** (`RelationsConsole.cpp`): "Create New Relation" is inert. The
  "Active Relations" list is `Player <-> Zone_1` and `Player <-> Sun` (`:21-22`), both
  invented. The one live control on the tab is "Open Law Author", which works.
- **Assets** (`AssetsConsole.cpp`): "Save World" and "Load World" have their
  bodies commented out (`:16`, `:21`) while the save system works from the main
  menu. The asset browser lists `material.clay`, `material.stone`,
  `material.wood`, `law.gravity`, `law.collision` — typed in, not enumerated.
- **Character** (`CharacterConsole.cpp`): "Reset Skeleton" and "Import Model"
  are inert buttons; the lock checkbox writes a field nothing reads.
- **World** (`WorldConsole.cpp`): "Reset Day/Night Cycle" and "Adjust Gravity"
  are inert. "Wireframe (Global)" and "Cursor Tools Open" write state nothing
  reads (§3.2).

Only **3D Tools** does real work, and **Paint** is inert in a different way
(§2.2).

The severity is not the missing functionality; it is that a Person cannot tell
which list on this window is the world and which is a mock-up. `material.clay`
is a real identifier convention in this repo (`CLAUDE.md`, the paint rule), so
the fabricated list is *plausible*. That is what makes it a lie rather than a
placeholder.

**Minimum honest fix, cheap:** delete the invented rows, or replace them with
`ImGui::TextDisabled("not wired yet")`. Enumerating the real zones and relations
is a few lines each and is the better fix.

### 2.2 HIGH — the entire Paint tab is disconnected

`PaintConsole.cpp` offers 28 tool buttons across four groups. Pressing one calls
`setPaintTool` (`:23-28`), whose only live line sets
`state.current3DMode = Mode3D::None` — which makes the tool dispatch in
`render3DConsole` fall through to `default: break;`
(`Create3DConsole.cpp:501`). The zone call is commented out with the note
`// zone.setDesignTool(type); // doesn't exist`. `configurePaintBrushPreset` is
a function whose whole body is the comment `// commented out` (`:20-22`). The
colour picker's write is commented out (`:120`). Undo, Redo and Clear
History are three buttons with commented-out bodies (`:130-142`).

The tab renders a full professional tool belt in which nothing is connected to
anything. Compare `ElementalToolHandler.cpp:19`, which handles the same
situation honestly: it titles its window "Professional 2D Design (Disabled)" and
says the tools are under maintenance. That is the right pattern and it already
exists in the tree.

### 2.3 HIGH — every shape parameter slider writes to nothing

`Create3DConsole.cpp:148-180` renders Radius, Semi-axis X/Y/Z, Asymmetry,
Steepness, Major/Minor Radius, Half-height and Fillet into
`state.polyhedron.shapeParams`. That struct has **no consumer anywhere in
`src/`** — the BrushCreate sync (`:417-423`) copies shape *kind*, rotation,
scale, grid snap and colour onto the CreationChannel and nothing else, and
`Object::ShapeParams` is only ever set from serialization
(`Storage/Serialization.cpp:288`).

A Person sets a sphere's radius to 2.0 and gets the default sphere. Silently.
(The shape-generator audit records the same fact from the law side, §1.6.)

The whole **Polyhedron** section below it — ~90 lines: Tetrahedron /
Octahedron / Dodecahedron / Icosahedron, Faces, six irregular types, four
concave variants, custom vertex counts — writes `state.polyhedron.*`, which
likewise nothing reads. `generateCustom()` is a stub with the comment
`// Stub for custom generation` (`CreatorConsoleState.hpp:68-71`), and
`Engine::buildCurrentPolyhedron()` is `{} // dummy` (`Engine.hpp:134`). The
spawn path then refuses Polyhedron with a bare `return`
(`Tool.cpp:477-484`) — correctly, but silently: the Person clicks and nothing
happens and nothing says why.

### 2.4 HIGH — the tools only run while the window is open, uncollapsed, and on the right tab

Every 3D tool is *invoked from inside the render function*: the `switch` at
`Create3DConsole.cpp:415-503` calls `Tool::ShapeGenerator3D`,
`Tool::Selection3D`, `Tool::FaceBrush`, `Tool::FacePaint`, `Tool::Pottery3D`,
`Tool::Rotate3D`. That function runs only when `render3DConsole` is dispatched
(`CreatorConsoleWindow.cpp:61`), which requires `state.currentSection ==
Create3D` **and** `ImGui::Begin(...)` to return true — which is false when the
window is collapsed.

So: collapse the Creator Console, or click the "Zones" tab, and every 3D tool
stops working. The mode buttons still show **Create** highlighted green when the
window is reopened. The console's own state says armed; the world does not
respond. Nothing reports the disagreement.

Rendering and acting are the same function here. They are two different things
(the same distinction `Tool.hpp:130-144` argues for sensing vs. acting, one
layer up).

### 2.5 HIGH — the placement hologram is a function containing a comment

`renderCreatorConsole3DPreviews` (`CreatorConsoleWindow.cpp:83-89`) checks the
mode and then contains `// Render primitive preview...`. It is called every
frame from the render path (`EngineRender.cpp:93`). "See where it lands" has a
call site, a guard, and no body.

`docs/architecture/migration/ui_migration_todo.md` preserves the deleted gizmo/ghost code
for exactly this and states the intended migration (spawn real transparent
Objects). Cross-reference: shape-generator audit §2.2.

### 2.6 MEDIUM — nothing on screen ever shows `active3DMode`

`channel->active3DMode` is written in exactly one place in the whole tree —
the `L`-key toggle in `DeveloperToolsWindow.cpp:63` — and displayed in none. The
one bit that decides whether a click creates is invisible to the Person who owns
the click. (Shape-generator audit §1.5 covers the split; the surface consequence
is that neither of the two states has any indicator at all.)

### 2.7 MEDIUM — the arming key is polled without an ImGui capture check

`DeveloperToolsWindow.cpp:61` reads `glfwGetKey(window, GLFW_KEY_L)` directly,
outside `ImGui::GetIO().WantCaptureKeyboard`. Typing a lower-case `l` into any
text field — a save name, a law name, the chat box, the Law Graph's path entry —
toggles the world's creation law on or off. The Person gets no signal (§2.6),
and their next click in the world may birth an object.

`Tool::ShapeGenerator3D` has the mirror-image check for the mouse
(`Tool.cpp:476`, added because clicking the window's own button spawned a cube
behind it). The keyboard side never got it.

---

## §3 — Controls that write to nothing

### 3.1 CRITICAL — undo is bound, present, and empty

- `Z` → `"undo"` → `// Undo functionality temporarily disabled due to UI migration` (`EngineInit.cpp:365-367`)
- `Y` → `"redo"` → same (`EngineInit.cpp:368-370`)
- Paint tab's Undo / Redo / Clear History buttons → commented out (`PaintConsole.cpp:130-142`)

There is no reversal anywhere on the Person surface — but the machinery is still
in the tree, orphaned: `BrushSystem::undo/redo/clearHistory`
(`Screen/BrushSystem.hpp:124-126`) and `DesignSystem::undo/redo/clearHistory`
(`Legacy/DesignSystem.hpp:417-419`) are both intact and fully implemented. Nothing
constructs a `DesignSystem`, and `PaintConsole.cpp:4` already includes
`BrushSystem.hpp` without using it. The paint undo stack needs an owner, not an
implementation (§11).

This is not a missing convenience. `ONTOMATH_FRAMEWORK.md` §6 makes closed-form reversal load-bearing
for the mathematics, and `ActionModel.cpp:1180,1302` already reasons about
whether an authored action *has* an inverse ("annihilation has no inverse in the
law text"). The substrate takes reversibility seriously and the hand cannot
reach it.

A surface that only runs forward is the thing this repository has explicitly
refused for its mathematics.

### 3.2 MEDIUM — write-only console state

Fields on `CreatorConsoleState` that a control writes and nothing reads:

| Field | Control | Note |
|---|---|---|
| `wireframe` | "Wireframe (Global)" checkbox | no reader in `src/` |
| `cursorToolsOpen` | "Cursor Tools Open" checkbox | opens an indented `ImGui::Text("Cursor tool options...")` and nothing else |
| `characterDesignLocked` | "Lock Player Design" | no reader |
| `current3DTarget` | "World Objects" radio | one option, no second choice, no reader |
| `brush.useAdvanced2D` | "Advanced 2D Brush" | no reader |
| `useLegacy2DTools` | "Legacy 2D Tools" | `Engine` keeps its **own** `_useLegacy2DTools` (`Engine.hpp:120`); the console writes the other one |
| `combineBlend` | "Smoothness" / "Blend t" slider | `Engine` keeps its **own** `_combineBlend = 0.15f` (`Engine.hpp:189`); the fuse path uses the engine's |
| `morphVertexIndex` | — | never assigned outside its initializer, so the Morph panel reads `Selected: -1` forever and the vertex editor at `Create3DConsole.cpp:379` is unreachable |
| `patchCtrlIndex` | — | same; `Engine` has its own `_patchCtrlIndex` (`Engine.hpp:167`) |

The duplicated-field cases are the worst of these: the Person moves the
Smoothness slider, the value is stored, and the fuse uses a different variable
holding 0.15.

### 3.3 MEDIUM — two windows exist and are never rendered

`Chat::renderUI` (`FirstMoverWindowTools/Chat.cpp:11`) and
`CursorTools::renderUI` (`FirstMoverWindowTools/CursorTools.cpp:100`) are
complete, reasonable windows with no call site anywhere. The `H` binding that
would show chat is commented out in both the menu (`EngineInit.cpp:201`) and the
key table (`EngineInit.cpp:328`).

`Engine::_cursorTools` is declared (`Engine.hpp:109`) and has an accessor
(`Engine.cpp:366`) but is **never constructed**, so `getCursorTools()` returns
`nullptr` unconditionally. (No caller dereferences it today; it is a trap set
for the next one.) `CursorTools::pickObjectAtCursor3D` also has the §1.6
framebuffer/window coordinate mismatch (`CursorTools.cpp:38-52`) and would
mis-pick on Retina the day it is revived.

### 3.4 LOW — `KeyboardHandler`'s binding presets are dead scaffolding

`KeyboardHandler` declares ten `setup*Bindings()` methods (`KeyboardHandler.hpp:60-73`)
binding ~60 keys to action names with `[]{}` callbacks
(`KeyboardHandler.cpp:149-260`). **None of them is called from anywhere.** All
live bindings are written by hand in `EngineInit.cpp:299-388`.

Two hazards if a future session calls them: `_keyBindings` is keyed by key while
`_actionToKey` is keyed by action, and `bindKey` overwrites the first without
pruning the second — so `setupGameBindings()`, which binds `LEFT_SHIFT` first to
`camera_down` and then to `straight_line_mode` (`:218`, `:266`), leaves
`isActionTriggered("camera_down")` returning true for a key that is no longer
bound to it. And `updateGameInput` is an empty function still declared in the
header (`:265`).

This is also where agenda item 7 (Key and KeyBind as Singulars) will land, so
the dead table is worth deleting rather than fixing.

---

## §4 — Say what you made

`CLAUDE.md`'s one rule with no technical enforcement: *if you write into a save
file, or generate beings directly, tell the Person which file and which beings,
and who is recorded as their author.*

- **Set-to-Set window: complies.** `CreationWindow.cpp:349-351` reports
  `Created %zu object(s)`, keeps `lastCreatedIds`, describes each source being by
  kind and size before you act on it (`describeBeing`, `:47`), and shows the
  TransferPolicy tier of every property as `kernel` / `open` / `GATED`
  (`:269-273`). This is the model.
- **Console spawn path: silent.** `Tool::ShapeGenerator3D` records provenance
  (`Tool.cpp:514`) and prints nothing. No name, no id, no count, no author.
  (Shape-generator audit §2.3 covers the naming half.)
- **Save path: silent about location.** `Save As...` / `Save Manager` open
  windows; nothing on the Creator Console's Assets tab reports which file was
  written, and its own save buttons are dead (§2.1).

---

## §5 — The refusals, in the surface

### 5.1 Refusal 3 — the surface defines what kinds of doing exist

Four enums in the UI layer are taxonomies of activity carved into C++:

- `CreatorSection { Paint, Create3D, Character, World, Assets, Relations, Zones }` (`CreatorConsoleState.hpp:12`)
- `Mode3D` — 12 values (`:23`)
- `ToolTarget3D` — 2 values, one of which the UI never offers (`:36`)
- `Tool::Type` — ~50 values (`Tool.hpp`), the largest kind-taxonomy in the tree

This was raised in the intercom thread against `CreatorSection`; `Tool::Type` is
the bigger instance and was not named there. Note these are **not** the
append-only serialized enums (`BeingKind`, `ShapeKind`, `ConditionNode::Kind`) —
they are not written to saves, so they are cheap to change, which cuts both ways:
nothing stops a session from adding a tab, and the addition is an ontological
claim made by a render function.

### 5.2 Refusal 6 — the console state is the black box

`CreatorConsoleState` is a file-scope global (`CreatorConsoleState.cpp:5`)
reached through `getCreatorConsoleState()`. It carries the Person's live
intention — which tool they hold, which shape, what size, what colour, whether
snapping is on — and **not one field of it is a registered property path**. No
law can read what tool the Person is holding. No law can be authored to respond
to it. It is not gated; it is invisible, which `NO_BLACK_BOX.md` names as the one
access level no law can change.

The remedy already exists and is half-built: `CreationChannel` registers 21
paths (`CreationChannel.cpp:37-88`) and the console pushes six of them across in
one `case` (`Create3DConsole.cpp:417-423`). The pattern is proven; it just stops
after six.

### 5.3 What the good windows prove

`LawGraphWindow::knownPathOptions` (`LawGraphWindow.cpp`) probes an `Object`
prototype, a `Law` prototype and a `CreationChannel` prototype for their
registered properties rather than listing them, with a comment recording exactly
why hand-listing failed before ("a hand-written list of six paths against a
registry of twenty-one"). The Person half — `position`, `cameraPos`, `velocity`,
`telos`, `joys` — is still hand-listed, with a stated reason (minting a Person
prototype has identity consequences). That is a documented exception rather than
an oversight, and it is the remaining half of the thread's finding #2.

---

## §6 — Capability that exists and has no surface

Built, tested, and unreachable by any Person:

- **`ChangeRecorder`** (`ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.cpp`) —
  authoring by demonstration: watch property paths while the Person works with
  ordinary tools, then fit the traces into a law. Covered by
  `tests/change_recorder_test.cpp`. **No UI references it.**
- **`LawSynthesis`** (same directory) — same status.
- **Action reversibility analysis** (`ActionModel.cpp:1180,1302`) — the law layer
  can already say *why* an action cannot be undone. Nothing shows a Person that
  sentence.

The gap between the ontology and the surface is not only that the surface fakes
things it cannot do. It also hides things the world genuinely can do.

---

## §7 — Craft and accessibility

### 7.1 MEDIUM — no font, no style, no DPI handling

`Engine::init` creates the ImGui context and sets two config flags
(`Engine.cpp:132-137`). It never calls `AddFontFromFile`, `FontGlobalScale`,
`ScaleAllSizes`, or reads the monitor content scale — verified by grep across
`src/`. Every window renders in ImGui's built-in 13 px proggy font. On the
Retina displays this project is developed on, that is roughly 6.5 physical
points of body text.

The in-world menu is worse: `stb_easy_font` at a fixed unscaled size
(`Menu.cpp:88`), drawn in framebuffer pixels, with no scaling anywhere.

Credit where due: `ImGuiConfigFlags_NavEnableKeyboard` **is** set
(`Engine.cpp:134`), so ImGui windows are keyboard-navigable.

### 7.2 LOW — active state is signalled by colour fill alone

`pushActiveButtonStyle` (`CreatorConsoleState.cpp:14`) marks the selected tool,
mode and shape by filling the button green
(`0.30, 0.50, 0.31`) — the only difference between "this is the tool you are
holding" and "this is a tool". For a red-green colour-blind Person (~8% of men)
that green against ImGui's default grey-blue button is a weak distinction, and
there is no second cue: no border, no prefix, no check.

### 7.3 LOW — the tab bar is a menu bar

The console's seven sections are `ImGui::MenuItem`s inside a `BeginMenuBar`
(`CreatorConsoleWindow.cpp:31-53`), so they render as a horizontal strip of
labels with a checkmark rather than as tabs. `ImGui::BeginTabBar` exists and
communicates "these are seven views of one window" without a click to discover
it.

---

## §8 — Three surfaces, three vocabularies

There are three Person-facing front ends in the tree, each with its own
independently hardcoded list of what a tool is:

1. **ImGui console** — `Tool::Type`, ~50 values (`Tool.hpp`).
2. **React app** — `src/Singularity/Foreign/web_ui/src/App.jsx:6`, a `TOOLS`
   array of five (`brush`, `eraser`, `select`, `magic`, `clone`) and an
   eight-swatch `COLORS` array, talking socket.io to `localhost:5005`.
3. **WASM page** — `web_ui/` at the repo root (`app.js`, `wasm.html`).

None of the three derives its vocabulary from the other two or from the world.
Whatever the (a)/(b) fork decides, three divergent hand-written tool taxonomies
is a coherence problem now.

---

## §9 — What "fixed" looks like, in order

Ordered by *what a Person hits first*, not by effort.

**Stop lying (hours, no architecture decisions needed).**
1. Delete every fabricated list and inert button, or mark them
   `TextDisabled("not wired yet")` — Zones, Relations, Assets, Character, World
   tabs (§2.1); the Paint tool belt (§2.2). `ElementalToolHandler.cpp:19` is the
   in-tree precedent.
2. Delete or implement the eight dead menu options (§1.3). "Quick Save" first.
3. Point "Toggle Creator Console" at `_creatorConsoleOpen` (§1.4).
4. Either implement undo or unbind Z/Y and remove the Paint undo buttons (§3.1).
   A key that does nothing is worse than a key that is not there.

**Make first contact survivable (hours).**
5. Release the cursor from any gesture that opens a pointer window (§1.1).
6. Draw a keymap — the dead "Controls / Keymap" option is a fine home, and the
   binding table already exists to enumerate (§1.2).
7. Fix the menu's coordinate space (§1.6), overflow (§1.5), key labels (§1.8) and
   edge tracking (§1.7).

**Close the honesty gap on the create path (days; see the shape-generator audit §4).**
8. One Create bit (§2.6 + that audit's §1.5), shown on screen.
9. Gate `L` on `WantCaptureKeyboard` (§2.7).
10. Restore the hologram from `getCursorSpawnTransform()` (§2.5).
11. Report what was born, by name and id (§4).
12. Carry `ShapeParams` to the born object, or remove the sliders (§2.3).

**Then the fork (the thread's question).**
13. Move tool dispatch out of the render function (§2.4) — required under either
    branch of (a)/(b), and it is the change that makes (b) possible at all.
14. Register the console's state as property paths on a channel (§5.2) — under
    (a) this *is* the fix; under (b) it is the migration step that keeps law text
    working when the ImGui shell is demolished.
15. Decide and write down (a) vs (b) in `docs/Person Interface and Experience.md`,
    which is currently one line long.

---

## §10 — Not verified in this audit

- **Nothing was clicked in the running app.** Every finding is from reading the
  call path. The three that most deserve a run: §1.1 (cursor lock — the exact
  ImGui behaviour under `GLFW_CURSOR_DISABLED`), §1.6 (menu hit-testing — the
  scale factor is 2 on this machine but the symptom should be seen), and §2.4
  (collapse the console and confirm the tools stop).
- **No build was run.** No code was changed, so nothing here can have broken the
  build; but the line numbers were read against the working tree, which has
  uncommitted modifications to `Tool.cpp/hpp`, `ScalarForm`, `Sdf.cpp`,
  `DeveloperToolsWindow.cpp` and `SdfWgsl.cpp`.
- **Not audited:** the WASM and React front ends beyond §8, the Law Graph
  window's interaction design in depth (only its path vocabulary), `MathEditors`,
  `RelationManagerWindow`, `AdvancedFacePaint`, `BrushSystem`, and the audio
  channel's Person-facing side.
- **Deliberately not repeated:** the creation path's own defects, which
  `SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md` covers in depth and whose §1.1 was
  fixed after that audit was written.


---

## §11 — Provenance: what was amputated, and what was born fake

Added after the audit was first written, when the repository owner identified the
cause: a refactor that removed `Game.hpp/cpp` and split its work into `Engine`,
during which functionality was not carried across and has been restored by hand
since, one control at a time.

The deleting commit is **`52689ee3`** ("Created CategoryManager, removing
Game.hpp/cpp and worked on Singular set to set. Also, so... many... files..."). It
deleted six `Game*.cpp` plus `Game.hpp` and `GameInit.cpp`. **All of them are still
readable at `52689ee3~1`**, and three have working copies in `scratch/legacy/`
(`GameUpdate.cpp`, `GameRender.cpp`, `old_GameToolbar.cpp`, `original_game.hpp`).
Nothing is lost. This section exists so the next session restores rather than
reinvents.

```sh
git show 52689ee3~1:src/Singularity/Core/Game.hpp
git show 52689ee3~1:src/Singularity/Core/GameToolbar.cpp   # 1395 lines
git show 52689ee3~1:src/Singularity/Core/GameRender.cpp    #  491 lines
```

### 11a — Amputations: had a body, recoverable

| Dead control (§) | Original | Status of its dependencies today |
|---|---|---|
| Quick Save (§1.3) | `GameInit.cpp:152` → `saveStateWithLog()` | **Alive.** `ZoneManager::saveStateWithLog` (`ZoneManager.cpp:262`); `DeveloperToolsWindow.cpp` already builds the `SaveContext` it needs. One-line restore. |
| Controls / Keymap (§1.2) | `Game.hpp:419` `_showKeymapWindow`, accessor at `:351`; window body `GameRender.cpp:407-450` | **Alive.** ~45 lines of `BulletText` covering Core / Saves / Camera / Create. Copy it, correct the keys that moved, done. |
| Toggle Chat (§1.3, §3.3) | `Game.hpp:417` `_showChatWindow`, `GameInit.cpp:156` | **Alive.** `Chat::renderUI` is intact and complete; it needs a flag and a call site. |
| Toggle Toolbar (§1.3) | `Game.hpp:420` `_showToolbar`; `GameToolbar.cpp`, 1395 lines | Largest single restore. Also the origin of the Paint tab. |
| Toggle Placement Insp. (§1.3) | `Game.hpp:546` `renderPlacementInspector()` | **Partly reborn** as `Create3DConsole.cpp:68`, rewritten against the `CreationChannel`. The menu entry can point at that; do not restore the old one. |
| Toggle Selection Insp. (§1.3) | `Game.hpp:547` `renderSelectionInspector()` | No successor. Restore or delete the menu entry. |
| Paint tab's commented calls (§2.2) | `GameToolbar.cpp:234` `setDesignTool`, `:327` `setDrawColor`, `:444` `undo`, `:452` `clearHistory` — all against `Zone` | **Half alive.** The `Zone` methods are gone (`Zone::setDrawColor/undo/clearHistory` existed at `Zone.cpp:268/413/429` then, and no longer). But `BrushSystem` and `DesignSystem` still carry the whole undo stack (§3.1). What is missing is the owner: nothing constructs a `DesignSystem`, and `Zone` no longer holds either system. **This is the decision, not the code** — under Refusal 1 the answer is probably not "put them back on `Zone`." |
| Assets tab Save/Load (§2.1) | — | `mgr.getSaveLoadState()` works from the main menu today; the console buttons need wiring, not recovery. |

The recurring shape: the **callee** survived the refactor and the **caller** did not.
That is why every dead control still names its old API in a comment. Restoration is
mostly re-pointing, and only rarely rewriting.

### 11b — Fabrications: never existed

Verified by `git grep` against `52689ee3~1`: **no** `Zone_Spawn`, no
`Zone_Wilderness`, no `Player <-> Sun`, no `Reset Skeleton` anywhere in the
pre-refactor tree. The Zones, Relations, Assets, Character and World tabs
(`CreatorConsole/`) are **new work written after the refactor**, and their contents
were invented rather than lost.

So §2.1 is not refactor residue and will not be fixed by restoring anything. It is
placeholder content that reads as world state. The fix is to enumerate the real
thing, and every one of those is a short call that already exists:

- Zones → `mgr.zones()` (boot already prints them, `EngineInit.cpp:180`)
- Assets → `MaterialManager` already resolves `"material.clay"` by full identifier
  (`MaterialManager.cpp:6`) — the tab is listing by hand what the manager can list
  for real
- Relations → the `Relation` register
- Laws → `_lawManager->getAll()`, which `DeveloperToolsWindow` and the Law Graph
  both already walk

Two of the eight dead menu options — **Settings** and **Toggle ImGui Demo** — are
also fabrications in this sense: no pre-refactor body, nothing to restore.

### 11c — Consequence for the fix order

§9 item 1 said "delete or mark every fabricated list and dead control." Split it:

1. **Restore** (§11a) — Quick Save and Controls/Keymap first: both are near-verbatim
   copies, and Controls/Keymap alone repairs the §1.2 discoverability floor and
   documents the ESC that §1.1 depends on.
2. **Mark or delete** (§11b) — the five fabricated tabs, Settings, ImGui Demo. Marking
   is a stopgap; enumerating the real list is a few lines and is the actual fix.
3. **Decide, then restore** — the paint stack's owner. That one is an ontology
   question (Refusal 1) wearing a restoration's clothes, and it is why it has stayed
   commented out longest.
