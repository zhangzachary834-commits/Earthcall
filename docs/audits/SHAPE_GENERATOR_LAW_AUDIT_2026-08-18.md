# Audit — Shape Generator 3D Law — 2026-08-18

Scope: the Person-facing "Tool: Shape Generator 3D" law
(`Singularity::Core::createShapeGenerator3DLaw`, identifier `shape-generator-3d-law`),
its CreationChannel, the Creator Console / L-key arming surface, the developer
bypass `Tool::ShapeGenerator3D`, the seed `saves/tests/shape_generator_3d_law.json`,
and the two tests that claim to cover it.

Method: read of the factory, the spawn action, the click publisher, the console,
and the Law Graph event vocabulary; then a probe
(`scratch/probes/shape_generator_law_audit_probe.cpp`) against the same factory
boot instantiates. Existing `shape_generator_law_test` was re-run and still
passes. **Nobody clicked in the running app.** That is still Feature-sized
item 2 on the To-do list.

Branch `sync-from-earthcall-main`.

---

## Verdict

The law can fire. That is all the factory test proves, and it is not the same
as the law creating where the Person asked.

There are two creation paths that share English ("Create") and share channel
fields, and they do not share a loop:

| Path | How you arm it | Who places the object | Who the architecture names it |
|---|---|---|---|
| Developer bypass | Creator Console mode **Create** (F8) | `Tool::ShapeGenerator3D` calls `updatePlacement` then `World::addObject` | "debug bypass, not the Person-facing creation path" (`Tool.hpp`) |
| Law | **L** toggles `active3DMode == "Create"` | `Law::applyTo` reads `cursorSpawnTransform` | "the Person-facing creation path" |

When L is armed, the bypass **returns before** `updatePlacement`. Nothing else
in the live engine writes `cursorSpawnPos`. The law then reads a live, readable
transform whose translation is still the constructor default `(0,0,0)`, and
Spawn treats that as success.

The path that feels like a tool is the bypass. The path the architecture
presents as the tool is a silent origin-spawner. The tests stay green because
they call `updatePlacement` (or poke `cursorSpawnPos`) themselves.

That is the finding that governs the rest. Everything below is a consequence,
a twin, or a surface that cannot tell the Person which path they are on.

---

## What the law actually is

Boot (`Engine::initLogic`) builds a `FirstMoverLaw` named `"Tool: Shape Generator 3D"`:

- identifier `shape-generator-3d-law` (via `setLawIdentifier`, not `setObjectID`)
- authored by the booted Person (so `applyTo` is not `Unauthored`)
- `OnEvent`, trigger `onMouseClicked`
- condition `active3DMode == "Create"`
- action `spawn(concept-shape-3d)` with live overrides
  `cursorSpawnTransform`, `activeColor`, `activeShapeKind`

The click is published globally from the GLFW mouse callback for every left
press outside ImGui. The condition is the only mode gate. That part, fixed
2026-08-17, still holds — `shape_generator_law_test` re-ran green this session.

`saves/tests/shape_generator_3d_law.json` is a **different being**: a regular
authored law with generated id `law-3`, same name, same condition, same action,
same trigger. Loading it does not replace the first mover. It sits beside it.

---

## §1 — Functionality

### 1.1 CRITICAL — armed law places at the origin

**Reproduced** by `scratch/probes/shape_generator_law_audit_probe.cpp`:
armed click, `placementMode = "InFront"`, camera at `(0, 1.6, 3)` looking `-Z`,
`inFrontDistance = 2`. Expected `(0, 1.6, 1)`. Born at `(0, 0, 0)`.

`CreationChannel::updatePlacement` has exactly one runtime caller:
`Tool::ShapeGenerator3D` (`Tool.cpp`). That caller now returns early when
`active3DMode == "Create"` — the mutual-exclusion fix for the double-spawn
bug — **before** it updates the hit, the manual anchor, or the spawn
transform.

`getCursorSpawnTransform` is a computed property, but it only composes the
already-cached `cursorSpawnPos` / `Rot` / `Scale`. It does not see the
Person's camera. Spawn refuses an *unreadable* placement path; it does not
refuse a readable identity matrix. The failure is silent and recorded as
`Applied`.

The deleted `GameUpdate.cpp` used to push placement every frame. When the
fields moved onto `CreationChannel`, the per-frame push did not move with
them. Tests reconstructed the push. The engine did not.

CursorSnap and ManualDistance are dead on this path for the same reason:
`cursorHitPos` / `manualAnchor*` are only written inside the bypass, past
the same early return.

### 1.2 CRITICAL — loading the advertised seed double-spawns

**Reproduced** by the same probe: factory law + a `law-3` twin, one click,
two objects.

`LawManager::loadFromJson` keeps first movers and then **adds** every law
in the file. `docs/BUILD_AND_ENVIRONMENT.md` ("Two live-system notes") tells
a Person to load `saves/tests/shape_generator_3d_law.json` from Developer:
Test World Saves "to see a Person-authored law spawn a cube on click."
Doing that is how you get two cubes per click, both at the origin if L is
armed, or one-at-origin (law) plus one-placed (bypass) if the console is
also in Create and L is not — except the bypass steps aside when L *is*
armed, so the loaded twin is the only way an unarmed console click would
double. With L armed, both laws fire; the bypass does not.

The two beings do not share an identifier (`shape-generator-3d-law` vs
`law-3`), so nothing about first-mover identity prevents the collision.

### 1.3 HIGH — setting the CreationChannel down does not stop the spawn law

**Reproduced.** `channel.setEnabled(false)` stops `Tool::ShapeGenerator3D`
(`if (!channel.isEnabled()) return;`). It does not stop
`shape-generator-3d-law`. That law has its own `enabled` bit. The 2026-08-18
"first movers can be set down" work gates `@creation-channel.enabled` and
the bypass. The Person-facing spawn law is a *second* first mover and was
not included in that gate.

Three switches now claim to be "creation, off":

- `@creation-channel.enabled` — stops the bypass only
- `@shape-generator-3d-law.enabled` — stops the law only
- `active3DMode` — the arming bit, independent of both

### 1.4 HIGH — the published event is not in the house vocabulary

The engine publishes `onMouseClicked`. The Law Graph picker offers
`mouse-clicked` ("subject: the person"). Migration text and
`SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md` name `person-clicked-mouse`.
`AGENTS.md` requires past-tense `noun-verbed`.

A Person who authors a spawn law from the picker and binds the offered
`mouse-clicked` writes a law that will never hear the click. The factory
law is bound in C++ to the unpublished name, so it works and is invisible
as a template: opening it in the Law Graph shows a trigger string the
picker does not list.

The subject is also wrong for the offered meaning. The callback publishes
with `subject = CreationChannel`, not the Person. `@event.subject` in an
authored condition is the channel. The picker tooltip says otherwise.

### 1.5 HIGH — console "Create" and law "Create" are different bits

`CreatorConsoleState.current3DMode = Mode3D::BrushCreate` (the **Create**
button) never writes `channel.active3DMode`. L never writes
`current3DMode`. Default console mode is already BrushCreate.

Consequences, all from the code path, not from a click:

- Open F8, click in the world, never touch L: the bypass runs, placement
  works, the law does not fire. This is the experience that will be
  reported as "the shape generator works."
- Press L, leave the console on Select / Face Paint / Pottery: the law
  still fires on every left click. The console tool also runs. One click
  selects *and* births, or paints *and* births.
- Press L with the console on Create: the bypass steps aside, the law
  fires at the origin (1.1). Shape / colour / scale *do* get copied onto
  the channel in that branch, so the origin cube can have the right
  colour and the wrong place.

### 1.6 MEDIUM — live selection is only kind and colour, and only while the console is in BrushCreate

`applySpawnOverrides` writes `ShapeKind` and face colour. It keeps the
concept template's `ShapeParams` ("the selection carries no params").
Sphere radius, torus radii, fillet, ovoid asymmetry — every slider under
Shape in the console — live only on `CreatorConsoleState` and never reach
a law-born object.

`cursorSpawnScale` / `Rot` / `gridSnap` / `activeColor` *are* on the
channel, but they are copied from the console only inside
`case Mode3D::BrushCreate`. Close the console, or switch mode, and the
law uses whatever was last written, or the constructor defaults
(white cube, scale 1, no snap).

Polyhedron: the bypass refuses (`buildCurrentPolyhedron` is still a stub).
The law does not. `setShape(Polyhedron)` with no topology is a born object
with a kind and no faces to paint.

Patch / Field overrides in `applySpawnOverrides` mint a default 3×3 Bézier
grid or a unit sphere SDF. They are not the authored `concept-bezier-patch`
/ `concept-complex-sdf` from the SDF/Bézier plan.

### 1.7 MEDIUM — CursorSnap can rest, then snap back into the surface

`spawnSurfaceOffset` pushes the shape out along the hit normal by its
projected half-extent. Grid snap then rounds the result on all three
axes. A floor hit at `y = 1.76` with snap `1.0` goes `1.76 + 0.5 = 2.26`
then rounds to `2.0` and sinks. `basic_cube_law_test` Test 5 combines
offset and snap on a case that happens to stay above the plane; it does
not hold the invariant "rests on the surface."

The known CursorSnap miss (`Self_Lifting_Floor_Bug.md`: stale
`cursorHitPos` on ray miss) still sits on the bypass. The law path never
writes the hit at all (1.1), so "stale" is the zero default.

### 1.8 LOW — Manual Distance keys do nothing

`EngineInit` binds arrows / PageUp / PageDown as `manual_offset_*` to
empty lambdas. The pre-law `GameUpdate` actually stepped `manualOffset`.
The sliders in the console still write the channel; the keys named for
them do not.

### 1.9 LOW — `setShapeKind` does not know the newer kinds

`Object.hpp` `setShapeKind`'s switch handles Cube / Polyhedron / Sphere /
Cylinder / Cone only (compiler warning on the probe build). `setShape`
handles the rest. The law uses `setShape`, so this is not the spawn bug —
it is a loaded-save / `setShapeKind` caller bug waiting next door.

---

## §2 — User design and experience

### 2.1 There is no Person-facing creation gesture

L is polled in `DeveloperToolsWindow` **above** the window's `*open` gate,
with no `WantCaptureKeyboard` check, and with no HUD. The main menu also
binds L to **Load**. The keymap window is a stub. Typing an L into any
ImGui field (law filter, save name, chat if it were live) arms or disarms
the spawn law.

The console's Create button looks like the gesture. It is the other path
(1.5). A Person who does the obvious thing never touches the law. A Person
who discovers L gets no confirmation they have, and then a cube at the
world origin — often inside the floor or the vessel.

### 2.2 The hologram is a comment

Legacy `GameRender.cpp` drew a live preview from
`_player.getCursorSpawnTransform()`.
`renderCreatorConsole3DPreviews` is now:

```
// Render primitive preview...
```

There is no ghost, no armed-state colour, no "Create: InFront 2m" readout,
no failure toast. Spawn's `emitEffect("placement path unreadable")` is the
right kind of refusal and never triggers here, because the path *is*
readable. Transparent Failure is inverted: the bad case looks like
success.

### 2.3 Names do not say what was made

- Concept display name: `"Shape Generator 3D Cube"`. Newborns are
  `concept-shape-3d.birth-N.member-0` even when the live kind was a
  Sphere. The Person's object list reads as a pile of cubes.
- Two first movers, two windows, two "Create"s, one L, one F8, one grave
  (dev tools). `DeveloperToolsWindow.hpp` still claims the window
  "publishes onMouseClicked on left-click while armed." It does not; the
  GLFW callback does, always.
- The Law Graph lists the factory law under "First movers" with the
  activation tag `event`. Nothing on that row says it is the 3D create
  tool, or that L is its trigger, or that it is distinct from `law-3`.

### 2.4 Authoring the same law from the graph cannot reproduce it

To rebuild what boot builds, a Person must:

1. Not pick `mouse-clicked` from the list (1.4).
2. Type the unpublished string `onMouseClicked` as a custom event.
3. Point conditions at the CreationChannel, not the Person — despite the
   SDF/Bézier plan still documenting `@person.activeTool`.
4. Set Spawn's placement / colour / kind overrides to channel paths the
   picker will offer, then discover that those paths are stale unless
   something the Person cannot see has called `updatePlacement`.

The seed file they are told to study uses id `law-3`, which law text is
not supposed to name (stable-identifier rule). The factory they actually
boot uses `shape-generator-3d-law` and refuses to serialize.

### 2.5 The console over-promises relative to the law

The Create3D panel offers polyhedron variants, irregular types, custom
vertex counts (`generateCustom()` is empty), combine, clay, morph, and
shape-parameter sliders. None of that is on the law. The law is "one
concept, one kind override, one colour, one (stale) transform." The
panel is the pre-law tool's surface with the law taped to L.

A Person who sets Radius = 2 and clicks, on the law path, gets a unit
sphere (template params) at the origin. The slider lied.

### 2.6 Docs that will mislead the next session

- `docs/BUILD_AND_ENVIRONMENT.md` § Two live-system notes: load the seed
  to see the law work. That loads the twin (1.2).
- `DeveloperToolsWindow.hpp`: claims this window publishes the click.
- `SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md`: properties on
  `@person`, event `person-clicked-mouse`. The refusals audit already
  moved tool state off the Person; the plan was not updated.
- `Legacy_3D_Create_Tool_Restoration.md`: "Law activation is bound to
  edge-triggered L rather than an ImGui button to prevent dual-firing."
  Dual-firing was not actually gated until 2026-08-17, and L-vs-button
  does not prevent 1.2 or 1.5.
- Feature-sized 1 / 1a on the To-do list are marked done and verified.
  1a verified that the law *can fire*. It did not verify that a Person
  can make a shape where they are looking. Item 2 already admits this;
  1 / 1a read as if the loop is closed.

---

## §3 — What the tests actually prove

`tests/shape_generator_law_test.cpp` (re-run this session, all ok):

- the factory law is authored and stably named
- the condition is the mode gate
- a click in Create, **with `cursorSpawnPos` pre-written**, births one
  uniquely-identified object of the live kind
- the first mover is omitted from `toJson` and survives `loadFromJson`

It does not: call `updatePlacement`; check colour; load
`shape_generator_3d_law.json` beside the factory law; disable the
channel; bind `mouse-clicked`; or run without a hand-set transform.

`tests/basic_cube_law_test.cpp` builds its *own* law (generated id),
calls `updatePlacement` before every click, and dumps the seed that
became `law-3`. It is a placement-arithmetic test, not a boot-path test.
That split is why 1a could ship a working factory while the live frame
loop still had no placement push — the same class of hole 1a was written
to close.

---

## §4 — What "fixed" looks like

In order. Do not skip to a HUD.

1. **Make `cursorSpawnTransform` true at the moment the law reads it.**
   Either the engine pushes `updatePlacement(player.cameraPos, player.cameraForward)`
   every frame (what `GameUpdate` did), or the computed property pulls
   the Person's camera itself and the hit/anchor the channel already
   holds. The bypass must not be the only writer. Refuse CursorSnap when
   there is no hit this frame; do not keep the last point.

2. **One Create bit.** Console Create and L must write the same
   `active3DMode`. L is a fine shortcut; it is not a second mode. Guard
   it with `WantCaptureKeyboard`, and take Load off L or take L off Load.

3. **One spawn law in the register after a load.** Give the seed the
   factory identifier, or stop shipping a second being with the same
   name. `BUILD_AND_ENVIRONMENT.md` has to describe the boot law, not
   tell people to load the twin.

4. **One event name, past-tense, in the picker, in the publisher, and
   in the factory.** Subject: the Person. The channel is addressable by
   id (`@creation-channel.active3DMode`); it does not need to be the
   event's subject.

5. **Setting the channel down sets the spawn law down**, or the spawn
   law's condition includes `@creation-channel.enabled`. Two first
   movers for one gesture is how 1.3 happens.

6. **Preview from the same transform the law reads.** Restore the
   hologram against `cursorSpawnTransform` after (1). If the transform
   is the origin because placement has not been computed, the ghost
   sitting under the Person's feet is the truth; do not invent a second
   preview path.

7. **Register shape params if the sliders are real**, or take the
   sliders off the Person-facing surface until a concept / OntoMath
   field carries them. Same for polyhedron: refuse on the law path the
   way the bypass already refuses, loudly.

8. **A test that does not call `updatePlacement`.** The probe in
   `scratch/probes/shape_generator_law_audit_probe.cpp` is that test.
   Promote the origin-spawn and twin-law cases once (1) and (3) are
   closed so they cannot regress the way 1a did.

Then click it. Item 2 on the To-do list is still the only verification
that counts as "a Person can make a shape."

---

## §5 — Not verified in this audit

- A human click in `earthcall` / `earthcall_webgpu`.
- Whether the origin cube is visible under the default camera / ground
  plane (physics may immediately lift or bury it; see
  `coincident_spawn_probe` and the self-lifting-floor history).
- Bézier / SDF generation laws from `bezier_patch_law_test` on the live
  frame loop. They share `onMouseClicked` and `active3DMode` and will
  inherit 1.1 / 1.4 / 1.5.
- WASM / Python surfaces.

Probe build used the test target for one configure/build/run, then the
copy under `tests/` was deleted and CMake was reconfigured. The probe
source remains at `scratch/probes/shape_generator_law_audit_probe.cpp`.
