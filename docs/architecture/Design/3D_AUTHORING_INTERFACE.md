# The Hand and the Field — Earthcall 3D Authoring Interface

**Status:** design authored 2026-10-01; implementation not yet claimed.  
**Purpose:** make Earthcall's native 3D authoring surface smooth enough for a Person or an agent to sculpt serious form rather than fall back to primitive placement.  
**Read beside:** `Building 2D and 3D Apps with Earthcall Guide.md`, `INTERACTION_AS_LAW.md`, `FIRST_MOVER_AUTHORING.md`, `LAW_AND_CREATION_SYSTEM.md`, the geometry / OntoMath docs, and the visual-authorship guide when it lands.

---

## 0. Thesis

The viewport must become the primary editor.

A Person should not have to translate a visual intention into a chain of disconnected panels, internal mode flags, and primitive-spawn commands. The hand should be able to point at form, seize it, move it, carve it, blend it, measure it, relate it, and inspect the exact authored property or Law that changed.

The interface therefore follows one rule:

> **Direct manipulation is a gesture over authored truth, never a second hidden model of the world.**

The Creator Console may remain as reference chrome and a precise inspector, but it must not be the place where 3D creation *happens*. Rendering a panel and acting on the world are different seams. The current `CreationChannel` already encodes that correction: live tools are stepped from the update path; the console does not own their lifetime.

The goal is not to imitate Blender's ontology. It is to recover the ergonomics that make a mature 3D tool feel like an extension of the hand while keeping Earthcall's own order:

```
Person gesture
    ↓ sensed by Interaction / Creation channels
intent becomes visible, addressable state
    ↓
Property / Relation / Formation / Law
    ↓
Screen manifests the result
```

No parallel scene graph. No editor-only copy of geometry. No domain-specific C++ class because the toolbar needed a noun.

---

## 1. Existing ground that this interface must reuse

The repository already has more of the hand than the current surface reveals.

### 1.1 Creation is already a channel

`creation-channel` carries live tool state, active 3D mode, shape kind, placement, cursor hit, spawn transform, grid snap, colour, and implicit expression. Its state is stepped outside rendering.

That is the correct seam. New controls should write to this vocabulary or to the selected being's real registered/authored Properties, not invent another `EditorState` that later drifts.

### 1.2 The 3D tools already have named first-mover Laws

The current tool vocabulary includes Create, Select, Face Brush, Face Paint, Pottery, Rotate, Morph, Combine, Sculpt, and Graph, each registered as a named first mover while `Tool::*` retains Sense/Act.

The interface should make these tools discoverable and composable; it should not re-implement their behavior inside UI callbacks.

### 1.3 Interaction is already a channel

Pointer, click, drag, scroll, focus, and key edges belong to the Interaction channel. A future viewport gesture should be sensed there and interpreted through named controls / Laws rather than through arbitrary GLFW polling hidden in a panel.

### 1.4 Geometry is richer than the primitive menu

Earthcall already has SDF trees, CSG operations, implicit OntoMath-backed fields, authored material fields, and volumetric fields. The editor must expose that richness spatially. A Person who can only see “Sphere / Box / Torus” will author sphere / box / torus worlds no matter how capable the substrate beneath them is.

### 1.5 The UI must never advertise a capability it cannot execute

Historical interface audits caught controls that wrote into dead fields, fabricated rows, missing previews, and tool execution tied to an open panel. Those are not merely rough edges. In a world whose Properties and Laws are supposed to be legible, a control that appears causal but is not is a false statement.

Every visible control in this design must satisfy one of three states:

1. **live** — it reads/writes real world state;
2. **read-only** — it truthfully shows state but does not pretend to change it;
3. **disabled with a reason** — the capability does not exist yet.

Never stage-set chrome.

---

## 2. The minimum-maximum interface

Do not begin by building a giant DCC application.

The smallest surface that unlocks serious modeling is:

```
┌────────────────────────────────────────────────────────────────────┐
│ Tool / transform strip + selection breadcrumb + snap / space      │
├───────┬───────────────────────────────────────────────┬────────────┤
│ slim  │                                               │ Property   │
│ tool  │                 LIVE VIEWPORT                 │ Lens       │
│ rail  │                                               │            │
│       │          geometry is manipulated here         │            │
│       │                                               │            │
├───────┴───────────────────────────────────────────────┴────────────┤
│ contextual drawer: graph / relations / law / history / diagnostics│
└────────────────────────────────────────────────────────────────────┘
```

The viewport owns attention. Panels support it.

### The four persistent regions

**Viewport** — always largest; select, transform, sculpt, combine, inspect.

**Tool rail** — icons / short labels only for the current gesture family. It does not contain the shape itself.

**Property Lens** — the selected Singular's real identity and addressable Properties. Type → specific being → categories of properties. No editor-private substitutes.

**Context drawer** — normally collapsed. Opens for graph editing, Relations/Formations, Laws, operation history, or performance / field diagnostics.

This gives Earthcall one coherent place to work rather than a constellation of floating windows.

---

## 3. Selection: the first act of authorship

Selection must feel immediate, truthful, and Formation-aware.

### 3.1 Click

- Click visible geometry: select the nearest eligible being under the pointer.
- Click empty world: clear selection.
- Shift-click: add/remove from the selection set.
- Double-click: enter the selected compound / Formation context without destroying global identity.
- Escape: leave nested editing context before clearing selection.

Selection highlight must be rendered from actual selected identities, not a duplicated list maintained by the UI.

### 3.2 Box and lasso

Drag from empty space to box-select.

A later lasso gesture may be admitted once the Interaction channel exposes the required pointer path. Do not fake it as a new hidden subsystem.

### 3.3 Selection filter

A compact filter can constrain what the hand may acquire:

- Object
- Relation handle
- Formation
- Law / field handle
- All

The filter is a *reach constraint*, not a new ontology.

### 3.4 Selection breadcrumb

At the top of the viewport show the containment / editing context:

`Zone › Formation › Object › field node`

Each segment is clickable. This prevents the classic nested-modeling failure where the Person no longer knows which level is being edited.

---

## 4. Transform must be boringly excellent

Before advanced sculpting, the everyday acts must feel effortless.

### 4.1 One universal transform gizmo

The selected object gets a viewport gizmo with:

- translate axes + planes;
- rotate rings;
- scale axes + uniform center;
- pivot handle.

Do not require a separate console mode for every transform.

Keyboard accelerators may mirror mature 3D tools:

- `G` move;
- `R` rotate;
- `S` scale;
- `X/Y/Z` constrain;
- `Shift` fine movement;
- `Ctrl/Cmd` snap as appropriate;
- numeric entry after a transform begins;
- `Enter` commit;
- `Esc` cancel.

The exact key map is secondary to the invariant: every gesture visibly begins, previews, commits, and cancels.

### 4.2 Local / world / view space

A three-way selector:

- World
- Local
- View

The current space must always be visible near the gizmo or transform strip.

### 4.3 Pivot

Provide:

- object origin;
- selection median;
- active member;
- cursor / authored anchor.

Do not create a mysterious global editor pivot. If an anchor is persistent and meaningful, it must be a real authored or channel-visible fact.

### 4.4 Numeric truth

Dragging changes real Properties continuously.

Numeric fields in the Property Lens show the same values live and may be typed directly.

The viewport and inspector are two views of one state.

---

## 5. Placement should show the future before it becomes the past

Creation needs a ghost.

Before a click births a being, the Person should see exactly what will be authored.

### 5.1 Real transparent preview

Use the existing migration direction: a preview should be represented through the same geometry manifestation as a real Object, rendered transparently / as a ghost, not through a bespoke fake drawing path that eventually diverges.

It must show:

- actual primitive or field shape;
- actual scale and rotation;
- actual placement;
- actual snap;
- surface normal alignment where applicable;
- bound / extent for implicit fields.

### 5.2 Placement modes

Expose current placement as a compact segmented control:

- Surface
- In front
- Manual anchor

Grid snap and snap size sit beside it.

The cursor should visibly communicate which placement law is active.

### 5.3 Commit / repeat

Single click commits one being.

A “repeat” latch may keep the creation tool armed, but it must be visible. A hidden armed state is unacceptable.

Right click / Escape cancels the pending birth without creating anything.

---

## 6. Primitive creation is the doorway, not the room

The primitive picker should be tiny.

A searchable command / palette can expose:

- sphere
- box / rounded box
- ellipsoid
- cylinder
- cone
- torus
- convex / polyhedron
- implicit field

The main UI should *not* devote permanent real estate to a wall of primitive buttons. That visually teaches the wrong ontology.

Immediately after creation, the primitive exposes **on-canvas shape handles**:

- sphere radius;
- ellipsoid axes;
- cylinder radius and height;
- cone radius and height;
- torus major/minor radius;
- rounded-box extents and fillet;
- field bound / extent.

Dragging a handle changes the real shape parameter.

This is the first step away from toybox authoring: a primitive becomes a continuous form the hand can reshape rather than an immutable block dropped into space.

---

## 7. CSG should feel like sculpting, not tree surgery

Earthcall's SDF composition can be far more ergonomic than a traditional mesh Boolean stack because the authored field is already the truth.

### 7.1 Two-object gesture

Select two or more compatible shapes.

Invoke a compact contextual pie / popup:

- Union
- Smooth union
- Intersect
- Subtract

Preview the result immediately before commit.

### 7.2 Spatial smoothing handle

For Smooth Union, do not make the Person hunt for a numeric “k” in a panel.

Render a handle at the joining region. Dragging it widens or narrows the blend while the Property Lens displays the exact value.

The value remains addressable and authorable; the handle is only a hand.

### 7.3 Subtraction has direction

When two operands are selected, highlight:

`A − B`

before commit.

Tab or a small swap gesture reverses operands.

Never make subtraction order implicit.

### 7.4 Preserve editability

Combining should not flatten authored structure into anonymous triangles.

The child fields remain individually selectable through nested editing / the graph drawer.

The whole may be treated as one authored Formation / compound where the ontology already supports that unity; do not invent a private editor hierarchy.

---

## 8. The Field Graph: advanced form without surrendering the viewport

The graph is for structure, not for basic placement.

Opening the bottom drawer on a field shows the current SDF / OntoMath structure.

Requirements:

- each node corresponds to real authored geometry/math;
- selection in graph selects the same substructure in the viewport;
- selection in viewport highlights the corresponding graph node;
- CSG connections are visible;
- parameters are editable through the same Property vocabulary;
- unsupported/unbuilt operations are not offered.

The graph must never become a second authoritative representation. It is a lens over the same field tree / mathematical text.

### 8.1 Formula and graph are peers

For implicit fields, show:

- formula / Law-Line-like textual expression;
- graph form where available;
- viewport result.

An edit in one updates the authored expression and therefore the others.

The goal is to let mathematical authors type while spatial authors drag.

---

## 9. Sculpt and Pottery: gestures over mathematical form

Existing Sculpt and Pottery tools should become spatial brushes with visible jurisdiction.

A brush cursor must show:

- radius;
- falloff;
- sign / operation;
- strength;
- affected surface / field.

A stroke should never be an invisible magic mutation.

### 9.1 Core brush intentions

Do not proliferate named art tools. Keep the mathematical verbs small:

- add / swell;
- subtract / carve;
- smooth;
- pull / push;
- pinch / flare where supported by the current substrate.

If a gesture cannot yet be represented truthfully by the geometry engine, disable it and say why. Do not add a fake “Clay Inflate” domain feature in C++ merely because a mature DCC has one.

### 9.2 Stroke history

A stroke is one authored operation for undo purposes even if it sampled many pointer positions.

The history surface should display the semantic act:

`Sculpt · subtract · object-17 · 2.4 s`

not thousands of internal samples.

---

## 10. Profiles, paths, sweeps: the next geometry frontier

This interface exposes a missing capability that the visual-authoring work now makes obvious: serious architecture and organic modeling often need an evolving cross-section, not another primitive.

Examples:

- column shafts and capitals;
- arches and ribs;
- roots and branches;
- vessels;
- cornices;
- railings;
- curved beams;
- terrain ridgelines.

Earthcall should eventually admit a mathematically general profile/path construction **only when the geometry substrate supports it as a reusable operation**.

Until that exists, the UI must not pretend it does.

When it is admitted, the intended interaction is:

1. draw / author a path;
2. author a 2D profile;
3. sweep / loft the profile along the path;
4. manipulate path and profile handles in the viewport;
5. preserve the construction as authored mathematics.

This is intentionally named as a frontier, not an implemented claim.

---

## 11. Formation editing: model wholes without losing beings

Multi-selection should offer:

- Form Formation
- Add to Formation
- Remove from Formation
- Enter Formation
- Dissolve Formation (when authorship / topology rules permit)

A Formation is not a folder.

The interface should visualize Relations among members when requested, and preserve the distinction between:

- spatial proximity;
- selection set;
- Relation;
- Formation.

Do not silently convert “I selected these together” into “these are one whole.”

---

## 12. Relations should be drawable in space

A Person should be able to select one being, invoke **Relate**, then select another.

The viewport previews a directed edge. A tiny chooser or Law-Line input provides the Relation type.

This is especially useful for visual semantics:

- supports
- attached-to
- frames
- illuminates
- follows
- instance-of

The edge is a real Relation and may be inspected in the Property Lens.

Relations are normally hidden in the beauty view; a topology overlay reveals them when authored structure matters.

---

## 13. Laws are part of the editor, not a separate universe

Geometry can respond.

When a Property is selected, the interface may offer:

**Author behavior…**

This opens the Law Line / Law author with that Property already denoted as context — not a new bespoke animation system.

Examples:

- drive `material.emission` over time;
- change mist density on an event;
- rotate a Formation while a condition holds;
- move a shape parameter through an OntoMath curve.

The viewport should preview Law-driven change where safe and reversible.

A timeline, if added, is a visualization of Law + Time / Moments. It is not a parallel animation subsystem.

---

## 14. Material and light authoring in the same spatial grammar

A selected surface / Object should expose material properties directly.

### 14.1 Material lens

Show only real material vocabulary:

- base / authored color field;
- roughness / reflectance properties that truly exist;
- emission / radiance fields that truly exist;
- texture / face properties that truly exist.

No generic PBR sliders unless the renderer actually supports the corresponding truth.

### 14.2 Eyedropper and assignment

An eyedropper selects the Material being, not merely an RGB triplet.

Dragging / assigning a Material establishes the real authored relation/property used by manifestation.

### 14.3 Light and volume bounds

Volumes get visible bounds in edit mode.

Their density / scattering / emission fields can be inspected alongside the geometry they illuminate.

This is also the place to teach performance locality: a volume or expensive field should show its jurisdiction in space.

---

## 15. Performance feedback should be visual and local

A smooth editor needs to tell the author when a beautiful act is computationally expensive *before* the frame collapses.

Optional diagnostics overlay:

- selected field bound;
- approximate field / node complexity;
- volume count affecting the current region;
- occluder / visibility workload where available;
- renderer parity / fallback warnings;
- unbounded or suspiciously huge field jurisdiction.

Do not turn this into a permanent profiler dashboard. It appears contextually when complexity matters.

The message should be causal:

> “This implicit field affects the entire Zone; bound it to reduce evaluation.”

not merely:

> “GPU 93%.”

---

## 16. Undo / redo is a floor

Every reversible direct-edit gesture must have an inverse or a recorded prior value.

The interface needs one coherent operation history across:

- transform;
- shape parameter edits;
- CSG composition;
- material edits;
- Formation membership;
- Relation creation/deletion where reversible;
- sculpt strokes when the representation supports faithful reversal.

`Cmd/Ctrl+Z` must mean the same thing whether the last act came from a gizmo, the Property Lens, or a graph handle.

If an act is not safely reversible, the interface must say so before commit.

Do not resurrect a second legacy history engine as hidden editor state. The operation record should point back to authored beings / properties.

---

## 17. “Comment the dream” inside the editor

The visual-authorship doctrine needs a native place to leave intent.

For a selected being / Formation / Law, expose an authored description / note surface where supported by existing properties or a general authored-property route.

Examples:

- “This aperture frames the distant citadel from the entrance.”
- “Keep this doorway human-scale; it anchors the tower's monumentality.”
- “Mist here reveals the beam; it must not obscure the sanctuary silhouette.”

The editor should make these notes easy to inspect during later edits.

Do not create an editor-only annotation database.

If no general authored textual property is yet appropriate, this remains a design requirement rather than a fake implementation.

---

## 18. Agent ergonomics and Person ergonomics share the same truth

An AI author and a human Person may use different input channels, but they should target the same authoring vocabulary.

A future agent tool call:

`set selected.shape.majorR = 4.2`

and a Person dragging the torus radius handle should produce the same world change.

A future agent composing a smooth union and a Person choosing Smooth Union from the viewport should create the same authored field structure.

This is the strongest defense against the “demo model can sculpt, Earthcall model can only place cubes” failure: the interface does not merely make the Person faster. It exposes high-level spatial operations as a coherent, inspectable grammar that agents can also invoke.

---

## 19. Mode design: visible latches, shallow modes

Earthcall already has named 3D modes. Keep them shallow.

A mode may change what a drag means, but:

- the active mode is always visible;
- Escape returns toward Select;
- selection does not disappear when switching tools;
- collapsing a panel never disables the tool;
- typing in text input never accidentally arms a world tool;
- every armed creation / destructive gesture has an obvious cursor or viewport indicator.

The default resting state is **Select / Transform**, not Create.

This reduces accidental births and makes the world feel manipulable rather than mode-locked.

---

## 20. Proposed viewport controls

A concrete first pass:

### Mouse / trackpad

- left click — select / commit current tool;
- shift-left — multi-select;
- drag gizmo — transform;
- drag empty — box select;
- right click — contextual action / cancel pending placement;
- scroll — ordinary navigation unless a focused handle explicitly consumes it.

### Keyboard

- `Q` Select
- `G` Move
- `R` Rotate
- `S` Scale
- `C` Create palette
- `B` CSG / combine menu when selection permits
- `F` frame selection
- `Tab` enter/leave nested field or Formation context
- `Esc` cancel → back out of context → clear selection
- `Cmd/Ctrl+Z` undo
- `Cmd/Ctrl+Shift+Z` redo

These are proposed ergonomics, not doctrine. The actual Key beings / KeyBind work may author them differently. What matters is consistency and visible truth.

---

## 21. Rungs: build the hand without building another empire

### Rung 0 — Viewport truth

- persistent selection highlight;
- universal transform gizmo;
- local/world space;
- numeric Property Lens synchronization;
- visible active mode;
- no tool lifetime tied to panel rendering.

**Exit:** select, move, rotate, scale one existing Object entirely from the viewport; Property Lens mirrors every value; collapse every panel and the tool still works.

### Rung 1 — Creation with preview

- searchable primitive/field palette;
- real transparent ghost;
- on-canvas shape handles;
- placement mode / snap visibility;
- commit / cancel / repeat.

**Exit:** spawn a torus, alter both radii spatially before/after birth, and verify the authored values persist after save/reload.

### Rung 2 — Spatial CSG

- multi-select;
- Union / SmoothUnion / Intersect / Subtract;
- operand-order preview;
- blend handle;
- nested selection.

**Exit:** author a recognizable compound shape without opening a node graph.

### Rung 3 — Graph / math duality

- field graph drawer;
- viewport ↔ graph cross-selection;
- implicit formula editor;
- live bound visualization.

**Exit:** alter one field through graph, text, and viewport handles and observe one underlying authored truth.

### Rung 4 — Sculpt / pottery ergonomics

- spatial brush cursor;
- radius / strength / falloff;
- semantic stroke history;
- stable undo/redo.

**Exit:** build an organic form whose constituent primitive ancestry is not visually obvious.

### Rung 5 — Relations / Formations / Laws in space

- Formation gestures;
- Relation draw gesture;
- “Author behavior…” handoff to Law Line;
- dynamic preview.

**Exit:** build a small environment whose geometry, structural Relations, Formation unity, and one reactive Law are all inspectable from one viewport workflow.

### Rung 6 — Advanced mathematical modeling

Only after the underlying geometry admits them:

- profiles / curves;
- sweep / loft;
- general deformations;
- richer authored field operations.

**Exit:** model an architectural column / arch / rib family without decomposing the result into visible toy primitives.

---

## 22. Non-negotiable refusals for the editor

1. **No second scene graph.** Relations and Formations remain authoritative.
2. **No editor-only geometry truth.** UI state may hold transient gesture state, never the meaning of the shape.
3. **No domain nouns in C++ because a toolbar wants one.**
4. **No hidden active mode.**
5. **No controls that write to nothing.**
6. **No action whose lifetime depends on a panel being rendered.**
7. **No fake preview path that disagrees with final manifestation.**
8. **No “advanced” tool that the current geometry cannot represent truthfully.**
9. **No separate animation system. Behavior is Law over Time.**
10. **No undo that only works for one tool family.**
11. **No performance optimization that silently changes authored appearance.**
12. **No interface which makes a primitive menu the center of 3D creation.**

---

## 23. The test scene

The editor should be proven on a scene specifically designed to defeat toybox authoring.

Build, entirely through the Person-facing interface:

1. a human-scale doorway;
2. a tall tapered support whose profile changes with height;
3. an arch / carved negative space;
4. a compound organic or mineral form;
5. a CSG object with a locally controlled smooth transition;
6. a bounded implicit field;
7. a material accent used structurally;
8. a mist volume with visible finite jurisdiction;
9. a Formation connecting several members;
10. one Relation;
11. one Law that changes a visual property on an event.

Then save, restart, reload, and continue editing the same forms.

If the easiest way to complete the scene is still “spawn many cubes/cylinders and place them,” the interface has not yet crossed the ergonomic threshold.

---

## 24. Final criterion

The interface succeeds when the Person stops thinking primarily about Earthcall's tool names and starts thinking about the form itself.

The hand should feel:

> seize this edge  
> widen this opening  
> taper this support  
> pull this crown upward  
> carve this cavity  
> soften this exact transition  
> frame that distant structure  
> bind these pieces into one whole  
> make this light answer when I enter

and Earthcall should translate those gestures into inspectable, authored mathematics, Properties, Relations, Formations, and Laws.

The primitive is still there.

The field is still exact.

The ontology is still visible.

But the tool disappears into the act.

That is the threshold from “3D controls” to **3D authorship**.
