# Building 2D and 3D Apps with Earthcall

**An authoring framework for agents and Persons coming from HTML, JavaScript, game
engines, or ordinary application frameworks.**

**Status (2026-09-06):** This is the integrative guide. The ontology and much of the
mechanism described here are implemented; the Person-facing authoring experience is
still embryonic. Every section distinguishes what can be used now from what remains a
design frontier. This document is not evidence that an unrun interaction works.

**Origin and extension.** Zachary Zhang supplied the skeleton and its governing intent:
teach “the robot guys” how to leave the HTML/JavaScript mental model; show where the
resources are and how to manipulate them; build Relations and unify them into
Formations; author Laws for process and behavior; and name the distance between the
prototype and the *Earthcall Ourverse Manifesto*. Zach also supplied the companion
*Foundational Design Specification* notes: art styles should be authored categories and
Materials; Persons should ultimately have granular mastery down to meaningful pixels or
pixel batches; 2D shape should arise from OntoMath; animation is Law over time; and Laws
should compound into higher Laws. Codex (GPT-5.6 Sol) has organized those instructions
into the operational framework below, reconciled them with the current repository, and
extended them with the visual-atomicity, projection, authoring, and verification rules
explicitly marked as such.

**Read beside:** [the manifesto](../../core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md),
[the architecture map](../README.md), [Interaction as Law](../law/INTERACTION_AS_LAW.md),
[Law and Creation](../law/LAW_AND_CREATION_SYSTEM.md),
[Authored Categories](../ontology/AUTHORED_CATEGORIES.md),
[New Kind Framework](../ontology/NEW_KIND_FRAMEWORK.md),
[No Black Box](../ontology/NO_BLACK_BOX.md),
[OntoMath](../mathematics/ONTOMATH_FRAMEWORK.md), and
[First Mover Authoring](../law/FIRST_MOVER_AUTHORING.md).

---

## 0. The first reversal

In a conventional application, source code is the order of truth:

```text
class / component -> state -> event handler -> renderer -> pixels
```

In Earthcall, the authored world is the order of truth:

```text
Person
  authors beings, properties, Relations, Formations, Concepts, and Laws
                                      |
                                      v
                       Singularity senses and acts
                                      |
                                      v
                 Screen / Audio / Physical / Foreign manifestation
```

The engine is the vessel. It senses hardware, keeps irreducible substrate invariants,
compiles authored mathematics, and presents authored beings through a modality. It must
not secretly decide what the authored thing *is*.

This changes the question. Do not begin with “Which class, component, widget, scene
graph, or framework feature should I add?” Begin with:

1. Which beings exist?
2. Who authored them, and in which Zone do they stand?
3. What properties can Law read and write?
4. What Relations make their connections real?
5. What Formation makes those connections one coherent whole?
6. Which Laws say how the whole changes?
7. Which Singularity channel senses or manifests that authored order?

An Earthcall app is the answer to those questions. It is not a special C++ object.

> **Working definition:** An Earthcall app is a Zone-held ecology of authored beings,
> unified by Relations and Formations, made reusable by Concepts and Categories, changed
> by Laws, and manifested through Singularity channels.

“2D” and “3D” describe how Screen manifests spatial mathematics. They do not create two
ontologies and they do not justify separate application frameworks.

---

## 1. Translation for web and game-engine minds

Use this table as a mental-model converter, not as a claim that the terms are identical.

| Conventional term | Earthcall construction |
|---|---|
| application | a Zone and the authored ecology it holds |
| component / entity | usually an `Object`; sometimes another existing kind of `Singular` |
| component state | registered or authored Properties, addressed by `PropertyPath` |
| DOM / scene hierarchy | Relations and one or more Formations; never an authoritative private tree |
| component type | Category being plus directed `instance-of` Relation |
| prefab / component template | `ObjectConcept`, captured from a set and instantiated with `Spawn` |
| CSS class / theme | Category defaults plus a Material being |
| event handler | `Law` with `Activation::OnEvent` |
| reactive effect | `Law` with `WhileTrue` or `OnBecomeTrue` |
| reducer / state transition | an `ActionModel` tree |
| selector / predicate | a `ConditionModel` tree |
| animation curve | OntoMath `Piecewise`, used by `Map`, `Flow`, or `Drive` |
| layout | a Formation and Laws over member positions; the reusable layout-law library is not built |
| UI event dispatcher | the common EventBus and Interaction channel |
| renderer | a Screen modality that reads Objects, Materials, and OntoMath |
| asset database | authored Material, texture, Lexeme, Concept, and Zone data, plus foreign resources behind channels |
| state persistence | Zone identity stores and session saves; never a second app-specific store by default |

The most important absences are deliberate:

- There is no `Button`, `Panel`, `Robot`, `Vehicle`, `ArtStyle`, or `App` class for a
  domain noun.
- There is no `src/UI/`, `src/Apps/`, or `src/MyProduct/` top-level subsystem.
- There is no enum value for a Person-authored category.
- There is no component-local state that Law cannot see merely because nobody exposed
  it.
- There is no render callback that is allowed to become the source of authored truth.

If the world needs a “button,” it authors an Object, relates it to a button Category,
and gives it a Law. If it needs a thousand, it captures a Concept and instantiates it.
If it needs a new kind tomorrow, a Person authors a new Category and Laws; no rebuild is
required.

---

## 2. The construction grammar

Earthcall’s app grammar is deliberately small. Richness comes from composition rather
than a growing catalog of types.

### 2.1 Singular — identity

A `Singular` is a discrete being with an identity that is not reducible to an anonymous
bag of values. Objects, Persons, Relations, Formations, Laws, Materials, Lexemes, Zones,
and Moments participate in this identity-bearing order.

For application authors, the default constructed being is an `Object`. Do not create a
new C++ class merely because an Object represents a chair, brush, note, dashboard tile,
planet, robot arm, or cathedral column. Those are authored meanings.

### 2.2 Property and PropertyPath — legible state

Properties are the vocabulary Laws can inspect and change. A `PropertyPath` is their
address:

```text
position.y
shape.majorR
face.0.color
@interaction-channel.dragX
@material.clay.opacity
@world.pointerOver
time.sinceApplied
```

There are two legitimate sources:

- **registered properties** expose invariant substrate state carried by an existing
  being or channel;
- **authored properties** are granted at runtime with `AddProperty` and persist with the
  being.

Both must behave as one vocabulary to Law. If a field matters to authored behavior and
is not reachable, that is not encapsulation; it is an ungovernable black box. If a value
is genuinely derived, it remains readable and refuses writes truthfully.

### 2.3 Relation — a connection that is itself a being

A Relation is not a string pair or a hidden edge. It has two real endpoints, a semantic
type, direction when direction is meaningful, weight, properties, and an event history.
Its identity is derived from its endpoints and type.

Examples:

```text
button-save --instance-of--> category.control.button
panel-title --attachment--> panel-background
slider --controls--> material.sky.opacity
column-a --supports--> arch
law-glow --authored-by--> person.zach
```

Do not mint an “empty Relation.” A Law using `AddRelation` is asserting that an actual
interaction, classification, provenance, attachment, or other connection has come into
being. Both endpoints must resolve to beings.

### 2.4 Formation — coherent plurality

A Formation gathers beings *and the Relations among them* into a higher-order unity.
Its members may include Objects, Relations, Lexemes, Laws, and other Formations. It is
not just an array and it is not every arbitrary graph.

Use a Formation when the connected beings must be addressed, governed, captured, or
reasoned about as one whole:

- the visual members and attachments of a reusable control;
- a palette and the Materials it orders;
- a scene layout;
- a set of laws that constitute one instrument;
- a rooted Category and its membership/taxonomy graph;
- the parts and Relations of a 3D construction.

The current runtime treats a rooted component or a sufficiently connected component as
a valid core; when no valid core exists, it reports the refusal and applies no topology
rewrite. The manifesto’s
deeper definition of Formation—three or more participants joined through a closed
relational unity—continues to govern the telos. Therefore, do not label a loose list a
Formation merely to obtain a collection API. If unity is not yet present, keep the
beings and Relations explicit until it is.

### 2.5 Category — authored kind

A domain kind is a Category being with a stable `category.<name>` identity. Instances
point to it with directed `instance-of` Relations; child categories point to parents
with `subcategory-of` Relations. Shared properties and a default Material live on the
Category root.

Categories answer “what kind of thing is this?” They do not answer “how do I reproduce
one?” That is the Concept’s job.

### 2.6 Concept — remembered construction

An `ObjectConcept` captures a selected set’s geometry, relative poses, authored
properties, and Relation templates. `Spawn` instantiates the remembered construction.
Anchored Relation templates preserve links from newborn members to external Category
beings.

Concepts answer “make another arrangement like this.” They are factories, not classes
and not categories.

### 2.7 Law — authored change

A Law is a named, authored, inspectable process:

```text
trigger / activation
        + scope
        + condition tree
        + action tree
        + authorship and provenance
        = change that may occur
```

Laws are where behavior lives. A method in a domain class is not a substitute. The
compiled closure is derived machinery; the serialized `ConditionModel` and
`ActionModel` are the Law’s text.

### 2.8 Zone — jurisdiction and place

A Zone is the immediate place and jurisdiction in which beings and Laws meet. An app
should normally have a stable Zone identity, not merely objects appended to a global
engine bag. Home, Community Zones, gathering Zones, and local application spaces have
different authorial meanings even when Screen can draw all of them.

### 2.9 Singularity channel — sensing and acting

A channel is admissible C++ when it bridges irreducible hardware, operating-system, or
foreign-process facts. It may sense a pointer, compile OntoMath into WGSL, send samples
to audio hardware, or communicate with a device. It must not define a domain noun or
choose authored behavior.

The seam is:

```text
Sense: hardware/world fact             -> channel, permanently
Decide: what that fact means           -> Law
Act: authored state change             -> Law
Manifest: translate authored form      -> channel
```

---

## 3. Where the resources live

These are the offices an author or implementation agent should reach first.

| Resource | Canonical location | Use |
|---|---|---|
| Singular and authored properties | [`src/ConstructedBeing/Singular/`](../../../src/ConstructedBeing/Singular/) | identity and the Law-visible state vocabulary |
| Object geometry and visual state | [`src/ConstructedBeing/Singular/Object/`](../../../src/ConstructedBeing/Singular/Object/) | 2D/3D representation, Concepts, geometry, face surfaces |
| Materials | [`src/ConstructedBeing/Material/`](../../../src/ConstructedBeing/Material/) | shared appearance beings and face textures |
| Relations and Formations | [`src/Relation/`](../../../src/Relation/) | first-class edges, coherent sets, topology, attachments |
| Laws | [`src/ZonesOfEarth/AuthorsOfLaw/`](../../../src/ZonesOfEarth/AuthorsOfLaw/) | conditions, actions, Rete, audit, law synthesis |
| Zones and Ourverse | [`src/ZonesOfEarth/`](../../../src/ZonesOfEarth/) | place, jurisdiction, identity persistence, gathering |
| OntoMath | [`src/Singularity/OntoMath/`](../../../src/Singularity/OntoMath/) | symbolic functions, fields, curves, calculus, shader input |
| Screen channel | [`src/Singularity/Screen/`](../../../src/Singularity/Screen/) | manifestation through WebGPU/OpenGL, not authored identity |
| Interaction channel | [`src/Singularity/Input/Interaction/`](../../../src/Singularity/Input/Interaction/) | pointer, focus, drag, scroll, and key sensing |
| Foreign/device channels | [`src/Singularity/`](../../../src/Singularity/) | hardware and software bridges by modality |
| Creator Console | [`FirstMoverWindowTools/CreatorConsole/`](../../../src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/) | temporary developer/First-Mover construction surface |
| Set-to-set authoring window | [`CreationWindow.cpp`](../../../src/Singularity/Screen/CreationWindow.cpp) | capture and instantiate Concepts; shown with F9 |
| Law Graph window | [`LawGraphWindow.cpp`](../../../src/Singularity/Screen/LawGraphWindow.cpp) | inspect and author Condition/Action trees |
| Serialized worlds | [`saves/worlds/`](../../../saves/worlds/) | Person-authored session/world artifacts; handle surgically |
| Zone identities | [`saves/zones/`](../../../saves/zones/) and [`saves/homes/`](../../../saves/homes/) | persistent place identity, separate from session composition |
| Executable examples | [`basic_2d_button_zone.json`](../../../saves/worlds/basic_2d_button_zone.json), [`synthesis_studio.json`](../../../saves/worlds/synthesis_studio.json), [`chess_app.json`](../../../saves/worlds/chess_app.json), [`shape_generator_3d_law.json`](../../../saves/tests/shape_generator_3d_law.json) | concrete authored patterns, not schema authorities |
| Build and verification | [`docs/BUILD_AND_ENVIRONMENT.md`](../../BUILD_AND_ENVIRONMENT.md) | canonical launch, build, test, and known-failure instructions |

The source is evidence for what the engine currently accepts. The architecture docs are
the order that source must serve. Existing saves are sacred authored worlds, not sample
JSON to rewrite casually.

---

## 4. The authoring path, end to end

This is the default sequence for building an app without carving it into C++.

### Step 1 — Name the Person and the telos

Write one sentence answering: *What good is this construction meant to make possible
for the Person or Persons who will inhabit it?* Name the actual authors. Do not begin by
listing technical features.

The answer determines the Zone, the Hierarchy of Joys Relation, the meaningful
categories, and the Laws worth writing. A technically functional world with no ordered
good is not complete by Earthcall’s own definition.

### Step 2 — Choose or author the Zone

Give the Zone a stable identifier. Decide whether it is a Person’s Home, an owned local
Zone, a Community Zone, or another existing authored place. Do not invent a top-level
folder or a special `AppWorld` class.

Keep jurisdiction explicit. A Law belongs to at least one Zone, and a visual Object’s
presence in a Zone is not the same claim as ownership of it.

### Step 3 — Author the vocabulary before the instances

Identify Categories such as:

```text
category.control
category.control.transport
category.instrument.note
category.surface.canvas
category.structure.column
```

Give Category roots stable identifiers, authored properties, default Materials, and
`authored-by` provenance. Use directed `subcategory-of` Relations to build the taxonomy.
Keep it acyclic.

Do not use a `type` string as the source of truth. A temporary compatibility label may
exist, but Laws should reason through real Relations.

### Step 4 — Author visual and extra-visual beings

Create Objects with stable ids, geometry, position, Material references, and authored
properties. Author Lexemes for meaning that should not be reduced to glyph pixels.
Author other existing Singular kinds when the thing truly has that ontology.

Separate:

- the being’s identity and authored form;
- the Material through which it appears;
- the Screen channel’s chosen execution strategy.

### Step 5 — Author Relations as the beings meet

Classify instances, attach parts, name provenance, and express semantic interaction.
Direction must match the sentence:

```text
instance --instance-of--> category
child --attachment--> parent
law --authored-by--> Person
```

If the Relation would be meaningless without an actual interaction or shared claim,
do not create it speculatively.

### Step 6 — Gather coherent wholes into Formations

Create a Formation when its members and Relations form an addressable unity. A single
app may carry several overlapping Formations:

- visual composition;
- control surface;
- semantic/category organization;
- law family;
- musical or physical structure.

Overlapping membership is allowed. One being can participate in several real unities.

### Step 7 — Add the properties Laws require

Use existing registered paths wherever the meaning is genuinely invariant. Add authored
properties for domain vocabulary:

```text
controlValue
selected
noteFrequency
inkPressure
openAmount
belongsToMeasure
```

Avoid vague catch-all blobs. A Law must be able to address the meaningful value directly.
Avoid mirroring the same truth in multiple paths; choose one source of truth and derive
the rest.

### Step 8 — Author Laws

Write the smallest meaningful Laws first: one trigger, one scope, one condition claim,
one action. Then compose Action trees or synthesize higher behavior after the primitive
Laws are truthful.

Test the Law while its effect is visibly obvious. A 20-pixel move is easier to witness
than a 1% tint change.

### Step 9 — Capture repeated constructions as Concepts

Once one instance is correct, capture it with its constituent set, relative transforms,
authored properties, and internal/external Relation templates. Instantiate it with
`Spawn`. Verify the newborn still carries Category membership and that existing Laws
reach it without reauthoring.

### Step 10 — Save, reload, and address again

A being that works only until reload is not yet an authored being. Verify:

```text
Law fires
-> state changes
-> world saves
-> process restarts / world reloads
-> identity resolves by the same stable id
-> Relations and Materials return
-> Law fires again
```

Use the application’s save path when possible. Direct save injection is a First-Mover
act and must follow the attribution and round-trip discipline in
[First Mover Authoring](../law/FIRST_MOVER_AUTHORING.md).

---

## 5. Building in 2D

### 5.1 What exists now

`ShapeKind::Shape2D` and `ShapeKind::Text2D` are existing append-only Screen
parameterizations. They are not domain categories.

A `Shape2D` is currently a screen-space, axis-aligned rectangle. Its useful properties
include:

| Property | Meaning |
|---|---|
| `x2D`, `y2D` | location in window points, from the top-left |
| `shape.width2D`, `shape.height2D` | rectangle size |
| `zOrder2D` | authored overlay order |
| `material` and face color | appearance source |
| authored properties | domain state such as `selected`, `controlValue`, or `labelFor` |

`Text2D` uses the same spatial frame and `textString`; it is a label without an automatic
plate behind it. 2D Objects render after the 3D scene in an orthographic Screen pass.
The Interaction channel can point at authored 2D Objects, so the same `object-clicked`,
drag, focus, scroll, and key events used by 3D beings can govern them.

The working minimal example is
[`basic_2d_button_zone.json`](../../../saves/worlds/basic_2d_button_zone.json): an authored
2D Object whose `object-clicked` Law changes `x2D`. Its point is not visual sophistication;
it proves that a control can be an Object plus Law rather than UI C++.

### 5.2 The 2D composition pattern

A conventional card, button, or panel should be authored as a set:

```text
background Shape2D
label Text2D
optional icon Object/Lexeme manifestation
Relations: attachment, labels, instance-of, authored-by
Formation: the coherent control
Concept: the captured repeatable pattern
Laws: interaction and layout behavior
```

The background is not a widget container. The label is not a child because a retained
UI tree says so. Their attachment and semantic relationship are authored Relations.

### 5.3 Layout

For now, simple layouts are direct authored positions. The architectural target is a
Formation plus Laws over its members’ `x2D`, `y2D`, width, height, and order. A row,
stack, grid, radial menu, or responsive panel is therefore a reusable family of Laws,
not a `LayoutKind` enum.

No general layout-law library exists yet. Do not describe it as shipped. If writing one,
prefer OntoMath mappings from container bounds and member index/order Relations to
coordinates. Keep intrinsic authored size distinct from the Screen channel’s framebuffer
scale.

### 5.4 Text and language

`Text2D.textString` can draw a label today, but a string is not the full ontology of
language. When the word’s meaning matters, author or resolve a `Lexeme` and relate the
visual label to it. Font, spacing, shaping, caret, IME, and robust Lexeme-driven layout
are not mature.

### 5.5 Current 2D limitations

- The “Professional 2D Design” and Creator Console Paint surfaces are intentionally
  disabled because their old tools were detached from Zone and would present controls
  that do nothing.
- The general authored shape is currently a rectangle; rich authored paths, curves,
  arbitrary 2D topology, and an OntoMath-native 2D shape taxonomy are not complete.
- WebGPU native line width is one pixel in the current path; thick strokes need geometry,
  not a pretend width setting.
- Text entry, IME, caret behavior, touch, and multi-pointer interaction are not built.
- There is no finished responsive-layout Law library.
- Pixel buffers and stroke history are not yet fully exposed as Law-addressable authored
  beings.

These are limits of the current manifestation and tools, not permission to build a
parallel UI framework.

---

## 6. Building in 3D

### 6.1 Available geometric parameterizations

The current `ShapeKind` vocabulary includes cube, polyhedron, sphere, cylinder, cone,
ellipsoid, ovoid, paraboloid, torus, rounded box, implicit Field, Bézier Patch, Shape2D,
and Text2D. The enum is append-only and serialized as integers. These values are ways the
substrate carries geometry; they are not domain kinds such as “building” or “robot.”

Use:

- analytic primitives for simple exact surfaces;
- `Polyhedron` for explicit vertex/face constructions;
- `Field` for implicit SDF and CSG expressions backed by OntoMath;
- `Patch` for Bézier control-net surfaces;
- Relations and Formations to build compound structures;
- Concepts to reproduce those structures.

### 6.2 The 3D construction path

The current developer path is the Creator Console (F8; F4 opens its 3D Create tab).
It can create the supported 3D parameterizations, including broad polyhedron options and
implicit expressions. F9 opens Singular set-to-set creation for capture and repetition.

Treat these windows as First-Mover scaffolding. They are not the final Earthcall app
authoring ontology. A finished construction must still become authored Objects,
Relations, Formations, Concepts, and Laws that survive without the window staying open.

### 6.3 Exact form and manifestation

An implicit or analytic Object’s mathematical truth must not be confused with the mesh
or shader used to show it. `renderMode` selects the representation strategy:

- `Auto` lets the channel choose its supported faithful path;
- `Analytic` insists on exact analytic manifestation where supported;
- `Mesh` forces the tessellated fallback.

The canonical app target is `earthcall_webgpu`. The OpenGL target falls back to cached
tessellation for analytic shapes; it does not raymarch them. Do not judge an authored
implicit form by the wrong executable.

### 6.4 Compound objects

A cathedral, machine, character prop, or instrument is not one giant Object by default.
Build meaningful members, then relate them:

```text
column --supports--> vault
key --attachment--> keybed
string --resonates-with--> sound-law
surface --instance-of--> category.paintable
```

Use attachment Relations when transforms should inherit. Use semantic Relations even
when no transform inheritance is required. Gather the coherent construction into a
Formation and capture it as a Concept only after its structure is truthful.

### 6.5 Current 3D limitations

- The Shape Generator 3D Law path is implemented and headlessly tested, but the older
  end-to-end audit still records Person-facing placement, duplicate-law, hologram, and
  live-click checks that must not be waved away.
- Physics still contains hardcoded behavior that should become individually set-down
  First Movers or authored Laws.
- Some channel strategies differ: exact WebGPU raymarching, mesh fallback, collision,
  and picking do not have identical support for every geometric form.
- Transparent mesh batches are not sorted back-to-front.
- A general Law-authored topology/CAD-continuity library over Zach’s hard-edge/soft-edge
  taxonomy remains a frontier.

---

## 7. Form, Materials, pixels, and art styles

This section carries Zach’s foundational design notes into an operational rule without
prematurely admitting a new C++ domain class.

### 7.1 Form is not the renderer’s output

**Design direction from Zach:** Form is the shape of how substance manifests, often the
mathematical and relational order among constitutive Singulars, pixel regions, control
points, or material-bearing parts. A renderer may project that Form, but does not own it.

Until Zach settles whether first-order `Form` deserves a distinct ontological admission,
represent a visual form with existing primitives:

```text
root authored being
  + Formation of meaningful constituents
  + Relations describing their arrangement
  + OntoMath describing continuous shape/value
  + Material references describing appearance
```

Do not create `class Form`, a `FormKind` enum, or a new top-level `Form/` directory as an
implementation shortcut. The old source region named `Form/` was deliberately renamed
`ConstructedBeing/`.

### 7.2 Material is a being, not GPU state

A Material owns authorable appearance such as base color, opacity, shininess, specular,
ambient, diffuse, and per-face textures. It contains no WebGPU/OpenGL handles as its
meaning. Screen resolves it into backend state at draw time.

Materials are shared by identifier. That sharing is powerful and dangerous:

- Change `material.clay.baseColor` when the author intends every member using clay to
  change.
- Use `Object::setFaceColor` or `Object::ownMaterial` when painting one Object. The first
  stroke diverges it onto its own Material identity.
- Never resolve a shared Material and mutate it as a shortcut for Object-local paint.

### 7.3 Art style is authored structure

Do not add an `ArtStyle` enum or rigid JSON list. Author an art-style Category or
Formation that relates Materials, palettes, line behavior, response Laws, lighting
assumptions, and semantic Lexemes. An Object may participate in several styles through
several Relations; a rigid enum cannot express that.

An art style can therefore govern:

- inherited/default Materials;
- Laws that map state or time to appearance;
- families of geometry and continuity constraints;
- cross-modal correspondences such as color-to-sound;
- permissible overrides in a particular Zone.

### 7.4 The visual-atomicity ladder

Zach’s goal is mastery down to individual pixels or meaningful pixel batches. The
extension here is a **promotion rule** that preserves that intent without forcing the
runtime to allocate a heavyweight C++ Object for every display sample:

| Level | Representation | When it becomes a being |
|---|---|---|
| raw display sample | Screen-channel metal beneath the authored world | never merely because hardware has a pixel |
| continuous image/field | OntoMath function over coordinates | when a Person authors the function or field as meaningful form |
| pixel region / tile / stroke | sparse authored region with properties and provenance | when the region has identity, meaning, or independent governance |
| visual Object | geometry plus Material manifestation | when it is a discrete constructed being |
| compound visual whole | Formation of beings and Relations | when its unity is itself meaningful |

The rule is: **virtualize quantity; promote meaning.** A million samples may be the
evaluation of one authored field. A specific stained-glass shard, brush stroke, or
selected pixel region may be a Singular because a Person has given it identity and a
governable role. This is not a retreat from granular control; it is the industry-optimal
route to it—symbolic and sparse authored truth compiled into batched GPU work.

The current code has not reached this model fully. `FaceTexture` still owns raw pixels,
layers, and stroke history as source-level structures; Laws can reach face color and
some layer structure, but not every pixel or stroke. Texture resolution is still a
fixed implementation default in places. This is explicit debt, not the intended ceiling.

### 7.5 2D-to-3D gradients

A 2D image, 3D surface, volumetric field, and sound should be able to share authored
mathematics without copying their truth into four formats. OntoMath is the bridge:

```text
one Piecewise / MathNode expression
  -> evaluated over (x, y) for a 2D manifestation
  -> evaluated over (x, y, z) for a field or surface
  -> sampled over t for sound
  -> read by Law as a property transformation
```

The channels may compile or sample the expression differently. They may not silently
change what it means. A future 2D/3D gradient tool should author mappings between domains
and Relations among their Forms, not bake an irreversible texture and declare the job
done.

---

## 8. Authoring Relations and unifying Formations

### 8.1 Begin with sentences

Before choosing a Relation tag, write the human sentence:

```text
This label names this control.
This control changes this material.
This key is attached to this instrument.
This object is an instance of this category.
This law was authored by this Person.
```

Then choose the direction that makes the sentence true. A directed Relation is satisfied
of its source, not both endpoints. `instance-of` therefore points from instance to
Category.

### 8.2 Use Relation properties deliberately

- `type` names the semantic bond; where meaning matters deeply, ground the vocabulary in
  Lexemes rather than proliferating arbitrary spellings.
- `directed` distinguishes asymmetric claims from mutual bonds.
- `weight` may express strength, but must not become an unexplained magic score.
- attachment data governs transform inheritance, not semantic meaning in general.
- Relation events carry the history of change in the bond.

### 8.3 Build the Formation around real structure

For a composite control:

```text
members: plate, label, icon
relations:
  label --attachment--> plate
  icon --attachment--> plate
  plate --instance-of--> category.control.button
root: plate, if this is a rooted category/composition view
```

For a layout:

```text
members: item-1, item-2, item-3, container
relations:
  item-1 --ordered-before--> item-2
  item-2 --ordered-before--> item-3
  each item --contained-by--> container
laws:
  map order and container bounds to x2D/y2D
```

For a 3D instrument:

```text
members: body, keys, resonant surfaces, note Laws, Materials
relations:
  keys attach to body
  each key instances a key Category
  keys control note Laws
  sound and light Laws share frequency/property bindings
```

### 8.4 Formation cautions

- Formations hold non-owning references; destruction must release a being from every
  Formation that holds it.
- A directed taxonomy edge still contributes to connectivity, even though it is not a
  mutual bond.
- A self-loop or same-type directed cycle is refused by the current Formation path.
- A Category Formation has a root; a plain set need not.
- A Relation graph serialized without resolvable endpoints is not restored as a
  plausible empty structure; fix the missing beings.
- Category inheritance is currently one hop in `Related`. Materialize inherited closure
  as Relations when Laws need it; do not assume traversal.

---

## 9. Authoring process and behavior with Laws

### 9.1 Choose activation by temporal meaning

| Meaning | Activation |
|---|---|
| “when the click happened” | `OnEvent` with `object-clicked` |
| “for as long as this remains true” | `WhileTrue` |
| “when this condition changes from false to true” | `OnBecomeTrue` |

Events are transition edges, named in the past tense. Do not publish “still-hovering”
every frame; read `@world.pointerOver` from a `WhileTrue` Law.

### 9.2 Choose scope truthfully

- `Subject` applies to the being carried by the event or explicit subject.
- `Everyone` ranges over the beings the Universe makes available and should be narrowed
  by properties, kind, Category Relations, Zone, or explicit targets.

An `Everyone` Law with a costly graph search can become quadratic. Prefer materialized
Relations and indexed PropertyPaths over repeated whole-world traversal.

### 9.3 Condition vocabulary

The current condition tree can compare properties, test regions, query Relations,
compose `All`/`Any`/`Not`, test Zone functions, inspect ontological kind or exact identity,
quantify `ForAny`/`ForAll`, and ask the physics substrate whether two geometric beings
overlap.

Use Categories for domain kind. `IsKind(Object)` means ontological C++ kind; it does not
mean “chair,” “button,” or “spaceship.”

### 9.4 Action vocabulary

The current action tree can:

- `Set`, `Add`, `Scale`, or `Lerp` a Property;
- `Map`, `Flow`, or `Drive` it through OntoMath;
- compose `Sequence` and `Parallel` branches;
- `Publish` an authored event;
- `Create` a generic Object;
- `Spawn` a captured Concept;
- `AddProperty` / `RemoveProperty`;
- `AddElement` / `RemoveElement`;
- `Destroy` an Object;
- `Synthesize` a construction from ordinary action children;
- `PlayAudio` through the registered Audio channel;
- author a Zone;
- `AddRelation` between existing beings.

No current Action kind authors a new `Law`. This is the central blocker for truly
second-nature MetaLaw authoring and Law Concepts.

### 9.5 Map, Flow, and Drive

These are different claims:

```text
Map:    p := F(inputs)              authored value
Flow:   p := p + f(inputs) * dt     authored rate of change
Drive:  p := curve(input)           authored response curve
```

Use `Map` for layout, color from state, or direct functional correspondence. Use `Flow`
for motion, accumulation, fading, and change over elapsed time. Use `Drive` when one
input continuously controls another through a curve.

OntoMath is exact where its supported algebra is exact and refuses unsupported operations
rather than forging an answer. Runtime `Flow` integration is currently numerical even
when closed-form reversal can describe the exact continuum.

### 9.6 Interaction patterns

A button should publish a semantic event instead of containing its final consequence:

```text
Law: control-button-archetype
  OnEvent object-clicked, Subject
  if Related(instance-of, category.control.button)
  then Publish control-activated(subject)

Law: save-button-meaning
  OnEvent control-activated, Subject
  if Identity(button-save)
  then <the authored save-related change>
```

This separates “this control was activated” from “activation means this here.” Multiple
meaning Laws can hear the same event, each visible and governable.

A slider is a `WhileTrue` Law conditioned on Category membership and
`@world.pointerPressedOn`, mapping or flowing `@interaction-channel.dragX` into an
authored `controlValue`. A hover response is a `WhileTrue` Law over
`@world.pointerOver`. No UI class is required.

### 9.7 The cascade trap

Two Laws whose actions satisfy each other’s conditions are a loop, not a branch. The
canonical example is a toggle implemented as “if off, turn on” plus “if on, turn off”:
the first write activates the second in the next chain round and the click has no net
effect.

Author the flip as one mathematical action, such as `controlOn := 1 - controlOn`, or use
an event/condition that the action cannot invalidate. Bounds on chain rounds prevent
infinite execution; they do not make a wrong causal design correct.

### 9.8 Conflict and failure

Every path write should report whether it landed. A description without an executable
model is not behavior. A Law without resolvable authors is `Unauthored` and must not fire.

Law execution order for simultaneous conflicting writes is not yet Person-authorable.
Until conflict resolution is built, design single-source properties, avoid coincident
writers, or make the order explicit inside one `Sequence`. Do not rely on registration
order.

---

## 10. Reuse: Category, Concept, and Law together

These three are often confused:

| Question | Answer |
|---|---|
| What kind is it? | Category + `instance-of` |
| How do I make another? | Concept + `Spawn` |
| What does it do? | Law conditioned on Category/identity/Relations |

The complete reusable pattern is:

```text
1. Author one truthful instance.
2. Relate it to its Category.
3. Gather its parts and internal Relations.
4. Capture the set as a Concept.
5. Preserve external Category links as anchored Relation templates.
6. Author behavior once against Category membership.
7. Spawn many instances.
8. Verify the existing Law reaches every newborn.
```

That is Earthcall’s replacement for component classes and prefabs. The newborn does not
inherit hidden methods; it enters visible Relations that existing Laws can hear.

---

## 11. Resources and cross-modal manipulation

“Manipulate resources at will” means: manipulate every governable authored property
through the one PropertyPath/Law surface, subject to the one TransferPolicy authority
system and unconditional Kernel guards where a Person’s body is at stake. It does not
mean bypass authorship, ownership, or physical safety.

### 11.1 Visual resources

- Geometry lives as Object shape data, polyhedra, Fields/SDFs, or Patches.
- Appearance lives on Material beings and FaceTextures.
- Screen reads those and chooses backend execution.
- A Law manipulates the authored Properties, not GPU handles.

### 11.2 Language resources

Words with semantic identity are Lexemes. A glyph string may manifest a Lexeme, but is
not a substitute for it. Human-language processing and flexible Lexeme layout remain
partial.

### 11.3 Sound resources

OntoMath expressions can be sampled by the Audio channel, and `PlayAudio` can reach the
registered sink. The same mathematical text may organize a visible Field and a waveform.
The channel enforces an unconditional infrasound guard on the path to a Person’s body;
it refuses dangerous output rather than editing the authored mathematics.

### 11.4 Physical and foreign resources

Hardware and external software belong behind modality adapters under `Singularity/`.
The adapter senses and acts; it does not create a parallel ontology. Represent an
external mechanism as Objects, Properties, Relations, and Laws inside Earthcall, with
the foreign channel translating at the boundary.

### 11.5 Shared-resource rule

Resolve once, but never confuse sharing with ownership. If several beings name one
Material, mutating that Material changes all of them. If several Laws write one Property,
they are making competing claims. If several Persons depend on one Law, single-author
ownership is not sufficient governance. Shared dependence must become explicit Relation
and Formation structure.

---

## 12. Persistence, authorship, and sacred saves

Save files are the authored flesh of Earthcall, not disposable fixtures.

### 12.1 Prefer in-world causation

The preferred order is:

```text
Person action -> authored Law -> Create/Spawn/AddRelation -> saved world
```

Direct JSON injection is for seeds, bootstrapping, or tests that cannot yet be caused
in-world. It is a First-Mover act. When it is necessary:

- obtain authorization from the owning/stakeholder Person;
- use the repository’s `injected_by` convention;
- distinguish the injecting agent from the Person whose authority permits the injection;
- record `authors` and `authored-by` Relations honestly;
- never forge elevated authority or a First-Mover Law;
- preserve unknown enum/model data rather than normalizing it away;
- load, save, and diff the result before claiming it exists;
- report the exact files and beings created.

### 12.2 Stable identity

Any being named by Law text, a Relation, a Concept anchor, or a cross-session reference
needs a stable identifier. Generated `object-N` or `law-N` identifiers are not reliable
semantic names merely because they happen to resolve today.

Use namespaced identities where collision is plausible:

```text
category.control.button
material.synthesis.blue
studio.transport.play
zone.synthesis-studio
```

### 12.3 Save topology

Current worlds may have `.json`, `.ecform`, and `.ecmatter` siblings. Treat the family as
one persistence artifact: semantic form and binary matter must not drift. Zones and Homes
also have identity stores separate from a session’s composition.

Never replace a Zone identity with a stale session snapshot. Never discard Relations or
Materials while preserving only Objects; that leaves recognizable geometry with its
meaning or paint amputated.

### 12.4 Round-trip witness

A serious app must prove all of these independently:

- Object and authored-property round trip;
- Material and FaceTexture round trip;
- Relation endpoint restoration;
- Formation membership restoration;
- Law model, trigger, author, and target restoration;
- Concept and anchored Relation-template restoration;
- Zone identity continuity;
- stable identifier retargeting after reload.

The current regression witnesses include
[`object_roundtrip_test.cpp`](../../../tests/constructed-being/object_roundtrip_test.cpp),
[`zone_relation_roundtrip_test.cpp`](../../../tests/zones/zone_relation_roundtrip_test.cpp),
[`singular_set_to_set_test.cpp`](../../../tests/constructed-being/singular_set_to_set_test.cpp),
[`interaction_channel_test.cpp`](../../../tests/singularity/interaction_channel_test.cpp),
and [`control_patterns_test.cpp`](../../../tests/singularity/control_patterns_test.cpp).

---

## 13. Performance without surrendering authored truth

Earthcall’s frontier is not “one naïve heap Object and one draw call per conceptual
atom.” It is authored granularity with compiled, cached, indexed, and batched execution.

Use these rules:

- Keep OntoMath and Relations as source truth; derive shader code, meshes, Rete nodes,
  and GPU batches as disposable execution artifacts.
- Materialize graph facts that Laws query repeatedly instead of traversing a taxonomy
  inside every `Everyone` sweep.
- Use Concepts and shared immutable data to reproduce structure without duplicating
  semantic authoring.
- Batch manifestations when geometry and resolved Material are identical; preserve
  per-instance transforms and authored identity.
- Promote meaningful pixel regions to beings and keep unpromoted samples virtual.
- Avoid allocation or shader recompilation for value-only changes that uniforms can
  carry.
- Treat bounds as doctrine. A design that needs unbounded chain rounds, call depth, or
  births per tick is wrongly shaped.
- Measure the booted, live path. A micro-benchmark that reconstructs a separate path is
  not evidence the app works.

Current hazards include `Everyone` sweeps, unbounded `Create`/`Spawn` birth rates, some
per-frame fact seeding and allocation debt, incomplete spatial indexing for conditions,
and channel-specific geometry fallbacks. Consult the live Agenda before making a
performance claim.

---

## 14. Maturity map: what Earthcall cannot honestly promise yet

This is the section agents should read before promising a conventional product on top of
the prototype.

| Area | Current truth | Frontier required by Zach’s intent |
|---|---|---|
| 2D authoring | screen-space rectangles/text render, serialize, and receive authored interaction; basic button is proven | restore/replace the disabled design tools with Law/First-Mover authoring; OntoMath-native paths, strokes, pixel regions, and layouts |
| 3D authoring | primitives, polyhedra, implicit Fields, Patches, Creator Console, Concepts, and Law creation exist | close live Shape Generator audit; deeper authored CAD/topology/continuity Law libraries |
| Form | geometry, OntoMath, Materials, and Formations carry much of it | settle first-order Form ontology and authoring without reintroducing a hardcoded class hierarchy |
| Materials/pixels | Materials are beings; face colors/layer controls are partly Law-visible; textures persist | Law-visible granular pixels/strokes, authorable resolution, sparse regions, and full no-black-box coverage |
| Relations/Formations | first-class endpoints, weights, history, attachments, rooted Categories, and persistence exist | make them load-bearing across more engine actions; settle remaining manifesto/runtime definition tensions |
| Categories | direct `instance-of` queries and rooted Category pattern exist | category home/registry, inherited closure propagation, conflict/diamond policy |
| Concepts | Object set capture/instantiate and anchored Relations exist | generalize `ObjectConcept` toward Singular creation without duplicating ontology |
| Laws | rich condition/action data trees, Rete, events, audit, creation, Relations, Zones, audio | Law-creating Action/MetaLaws, approachable Law Concepts, complete synthesis, authored conflict order |
| Interaction | pointer, hover, focus, click, drag, scroll, key sensing and archetype Laws | text entry, IME, touch, multi-pointer, right/middle edges, layout Law library |
| Time | world clock, `Moment`, `time.delta`, `time.sinceApplied`, partial exact reversal | scheduling, authored calendars/sequences, wall-time versus dropped-simulation-time relation |
| Physics | working modality and overlap predicate | decouple remaining hardcoded behavior into set-down First Movers/authored Laws |
| Language | Lexeme ontology and partial language channel exist | robust authoring, layout, symbolic interpretation, accountable semantic Formations |
| Multi-Person | Person, Home, Relationship/Community beginnings, Second-Person framework | mature identity, shared governance, visibility/likeness/conflict decisions, stakeholder ownership |
| Ourverse/Joys | local gathering, filaments, shared Joy seeds, and first Metalaw rung | make Joy ordering operationally load-bearing; mature local/ecumenical and Community realization |
| Security/authority | TransferPolicy spine, load-time authority clamp, Kernel body guards | complete property governance, native trust boundary, multi-Person capability/attestation realization |
| Earthcall as substrate | C++ app with WebGPU and OS/foreign adapters | the manifesto’s Earthcall-native Metal/OS reversal; current host OS remains beneath its audit reach |
| Person experience | powerful but developer-oriented ImGui tools and working authored examples | second-nature in-world authoring where Persons shape Laws and Forms without editing trees by hand |

This incompleteness is not a reason to imitate React, Unity, Blender, or a traditional UI
toolkit inside Earthcall. It is the map of which authored capabilities must be brought to
maturity next.

---

## 15. The frontier framework implied by Zach’s design

The following phases extend Zach’s notes into a coherent development order. They are a
direction, not a claim of implementation.

### Phase A — Honest authoring surfaces

Make every live control admit what it can do. Restore 2D creation only through real Zone
owners and Law-visible channels. Keep disabled controls disabled until their action path
exists. Close the Shape Generator and save/load manual protocols.

### Phase B — Visual Form as authored mathematics and Relation

Let Persons author 2D paths, regions, fields, control points, continuity, and mappings as
OntoMath plus Relations. Screen compiles these to GPU-efficient manifestations. Promote
meaningful regions and strokes to Singular identity without forcing every sample to be a
heavy being.

### Phase C — Law mastery over shape

Build foundational Law families for:

- hard/soft edges and continuity thresholds;
- surface, curve, polyhedral, patch, and implicit transformations;
- topology-preserving and topology-changing operations;
- 2D-to-3D projection and inverse projection where mathematically defined;
- animation as `Map`/`Flow`/`Drive` over the same properties;
- Category-governed behavior shared across every instance.

These are authored Law architectures, not method catalogs attached to shape classes.

### Phase D — MetaLaw and Concept multiplication

Add an authored way for a Law to create another Law or instantiate a Law Concept. Capture
micro-templates (“map this property through that curve”) and macro-templates (“make this
Formation an instrument/control/layout”). The Law Graph becomes an inspection and
refinement surface rather than the only composer.

### Phase E — One form, many modalities

Let Screen, Audio, Physical, Language, and Foreign channels read the same authored
OntoMath and relational structure. A visual control may also be sounded, physically
actuated, named, or shared without a second component tree.

### Phase F — Shared Person governance and Ourverse actualization

Make authorship, dependence, jurisdiction, conflict, and due weight explicit through
Persons, Relationships, Communities, Zones, Joys, and Ourverse Formations. No app is
fully realized merely because one Person can operate it locally.

### Phase G — native Metal mastery

Continue moving the substrate boundary downward so pixels, audio samples, device acts,
memory, authority, and identity can be ordered by the same ontology instead of merely
translated through a host application. Until then, name what the host OS and foreign
libraries can do beyond Earthcall’s view.

---

## 16. Anti-patterns and their correction

| If you are about to… | Stop and author… |
|---|---|
| add `class Button`, `class Robot`, or `class ArtStyle` | Object(s), Category, Relations, Formation, Concept, Laws |
| add `WidgetKind::Slider` or `ShapeKind::Cathedral` | Category beings; reserve enums for substrate parameterizations |
| create `src/UI/` or `src/Apps/` | interaction or foreign channel under Singularity only if a true modality bridge is needed; authored app stays in Zones |
| store `type: "button"` as the classification | `instance-of -> category.control.button` |
| put behavior in a renderer or input callback | channel publishes sense; Law decides and acts |
| hide state inside a helper because “authors should not touch it” | register it, then govern writes through TransferPolicy; derived values may be read-only |
| repaint by mutating a resolved shared Material | `Object::setFaceColor` / `ownMaterial` for an instance; shared mutation only when intended |
| publish one event every frame | publish the transition edge once; use `WhileTrue` for level behavior |
| add two opposite Laws as a toggle | one mathematical flip or a non-self-invalidating branch |
| assume Category inheritance traverses automatically | materialize closure Relations, then query one hop |
| capture a Concept and forget external Relations | preserve anchored Category/provenance templates and test the newborn |
| serialize descriptions without models | write executable `ConditionModel` and `ActionModel` trees |
| hand-edit a sacred save to “make the demo work” | obtain authorization, attribute injection, round-trip, diff, and report every being |
| claim success from source inspection | run the live or headless path and record what actually happened |
| mirror all pixels as heap Objects | keep samples virtual; promote authored meaningful regions to beings |
| treat an external API model as a Person | foreign mechanism is an Object/channel; only an actual human is a Person |

---

## 17. Worked blueprints

### 17.1 A reusable 2D transport control

```text
Beings
  studio.play.plate           Shape2D
  studio.play.label           Text2D / related Lexeme
  category.control.transport  extra-spatial Category root
  material.transport.idle

Properties
  plate.controlOn = false
  plate.controlLabel = "Play"

Relations
  label --attachment--> plate
  plate --instance-of--> category.control.transport
  plate --authored-by--> Person

Formation
  studio.play.control = {plate, label, their Relations}

Laws
  object-clicked + membership -> Map controlOn := 1 - controlOn
  controlOn changed/true       -> Publish transport-played
  pointerOver                  -> Map Material response

Concept
  capture plate + label + internal attachment + anchored Category Relation

Witness
  click; state flips once; capture; spawn three; all three work; save/reload; repeat
```

### 17.2 A reactive 3D sculpture

```text
Beings
  sculpture.field             Field Object with authored OntoMath SDF
  sculpture.core              analytic/mesh Object
  material.sculpture
  lexeme.resonance

Properties
  field.resonance
  material.sculpture.baseColor
  core.rotation

Relations
  core --contained-by--> sculpture.field
  both --instance-of--> category.artifact.reactive
  sculpture --signifies--> lexeme.resonance

Formation
  sculpture.whole = geometry + meaning + Material/Law Relations

Laws
  pointerDistance -> Drive resonance
  resonance       -> Map SDF parameter
  resonance       -> Map Material color
  resonance       -> PlayAudio from the related OntoMath form

Concept
  capture the whole for instantiation in another Zone

Witness
  approach/point; geometry, color, and sound correspond; disable each Law independently;
  save/reload; same ids and Relations return
```

The second blueprint is the payoff: not three synchronized subsystems, but one authored
relational and mathematical form read by three manifestations.

---

## 18. Definition of done for an Earthcall app

An app is not done when a screenshot looks correct. It is done for the current rung when:

- its telos and human authors are named;
- every Law-addressed being has stable identity;
- it stands in an explicit Zone and jurisdiction;
- domain kinds are authored Categories, not code enums or strings;
- repeated constructions are Concepts, not copied source blocks;
- meaningful connections are Relations with resolvable endpoints;
- coherent wholes are Formations rather than private arrays;
- every governable state value is reachable by PropertyPath;
- behavior is inspectable Law text with real authors;
- events are edges and continuous behavior is a level-aware Law;
- competing writers are eliminated or explicitly ordered;
- Screen/Audio/Physical/Foreign code only senses, translates, and manifests;
- Materials are shared or diverged intentionally;
- save, reload, and stable re-targeting work;
- headless tests exercise the same path the app boots;
- a Person has performed every visual, auditory, tactile, or usability check that only a
  Person can witness;
- all remaining prototype limitations are stated without euphemism.

That is the full reversal: the app is not a program that happens to contain authored
data. It is authored reality whose program has learned how to serve it.

---

## 19. Quick start for an agent

1. Read this guide, then the companion doc for the first thing you will touch.
2. Launch the WebGPU app with the canonical command in
   [`docs/BUILD_AND_ENVIRONMENT.md`](../../BUILD_AND_ENVIRONMENT.md).
3. Inspect the closest working authored world rather than inventing a new schema.
4. Write a one-sentence telos and identify the Person author.
5. Create or select a Zone; name stable ids.
6. Author one visual Object and one obvious Law.
7. Add Category and `instance-of`; confirm `Related` is one hop.
8. Add real Relations and only then gather the Formation.
9. Capture repetition as a Concept; verify anchored Relations on the newborn.
10. Save, reload, diff, and run the relevant tests.
11. Execute the Person-facing manual protocol for anything visible or experiential.
12. Report exactly what was authored, where it lives, who authored it, and what remains.

If a step seems to require a new domain class, enum, method family, widget system, or
top-level directory, stop. That friction is usually the architecture protecting the
authored world from becoming a payload inside somebody else’s framework.

---

**Authorship record:** Zachary Zhang originated the requested guide, its listed scope,
and the foundational intent quoted and developed here. Codex (GPT-5.6 Sol) synthesized
the current operational framework and originated the explicit visual-atomicity promotion
rule, the phased frontier, the conventional-framework translation, and the two worked
blueprints in service of Zach’s intent.

**Signed:** Codex (GPT-5.6 Sol) · session
`01a077ed-8d0f-7882-9e63-7748558bd59a` · 2026-09-06 11:23 PDT
