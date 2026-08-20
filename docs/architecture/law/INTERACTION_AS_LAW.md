# Interaction as Law

**How Earthcall makes graphical interfaces: not a UI subsystem, but the Law system and
Singular set-to-set creation aimed at the pointer, the wheel, the keys — and then at
every other modality Singularity holds.**

**Status:** The Sense half is built and stepped from `Engine::update`
(`Singularity/Input/InteractionChannel`). The archetype laws are built as first-mover
factories (`Singularity/Input/ControlPatterns`). Anchored relation templates — what makes
an instantiated control a member of its category — are built on `ObjectConcept`. Tests:
`tests/interaction_channel_test.cpp` (14 cases, headless) and
`tests/control_patterns_test.cpp` (8 cases) pass; the full suite is 54/55, the one
failure being the deliberate `webgpu_particle_test`. **§11b, the manual protocol, has not
been run** — nothing below is a claim about how any of this feels in the window.

The spawn law (`shape-generator-3d-law`) arms on `@creation-channel.spawnLawArmed`
(L / "Spawn as law"). Console Create is a different being (`tool-create-3d-law`)
and a different bit (`active3DMode`). A later step is to replace the L/checkbox
chrome with a control authored as a being + a law (this document).

**Companion docs:** `LAW_AND_CREATION_SYSTEM.md` (what a Law is),
`AUTHORED_CATEGORIES.md` (what a control *kind* is), `NO_BLACK_BOX.md` (why the pointer's
state is registered), `LAW_MIGRATION_FRAMEWORK.md` (the Sense/Decide/Act seam this whole
document turns on), `ALGORITHMS_AS_LAW.md` (why the pointer sweep is not a loop in the
ordinary sense), `FIRST_MOVER_AUTHORING.md` §1 (the channel inventory this adds to).

---

## 0. What this document is for

Every engine that has ever shipped answers the question "how do I make a button?" with a
class. `Button`, `Widget`, `Control`, `UIElement` — a type hierarchy, an event dispatcher
that only UI objects can hear, a layout pass, a retained tree. Earthcall refuses all of
it, and the refusal is not asceticism: **a UI framework is a claim about what a thing IS,
and no subsystem in Earthcall may make that claim** (CLAUDE.md, the general form of the
six refusals).

So the question becomes: what is left, once you may not say what a button is?

The answer is the thesis of this document, and it is smaller than the thing it replaces.

---

## 1. The thesis

> **A control is a being that the Person can point at, plus a law that says what pointing
> at it means.**

Nothing else. Unpack it:

| Traditional UI | Earthcall |
|---|---|
| a `Button` class | an `Object` — the same one the shape tools make |
| a click handler | a `Law` with `Activation::OnEvent`, trigger `object-clicked` |
| the handler body | an `ActionModel` tree — `Set`, `Add`, `Map`, `Flow`, `Publish`, `Create` |
| "is this button enabled?" | a `ConditionModel` over the subject's property paths |
| a widget *kind* | a category being, joined by `instance-of` (`AUTHORED_CATEGORIES.md`) |
| a widget *factory* | an `ObjectConcept` — set-to-set creation |
| a *theme* | a `Material`, shared by name, diverged on the first stroke |
| the UI event queue | the `EventBus` every other being already publishes on |
| a layout engine | a `Formation`, and laws over its members' positions |

Not one row on the right was invented for this document. **Every mechanism a GUI needs
already existed in Earthcall for other reasons.** What was missing was the *sense*: the
engine could not say which being the Person was pointing at, and could not announce that
they had clicked it. Two gaps, both in the Sense seam, both permanently the engine's job
(`LAW_MIGRATION_FRAMEWORK.md` §1: *sensing is first movement*).

The user's own formulation of the same point, which this framework is built to satisfy:

> A button, minimally speaking, is really just a spatial Object with a Law with either
> `when(onMouseClicked) → act(math ops on the properties when activated)`, or
> `if(some PropertyPath to mouse clicking being true)`.

Those two forms are `Activation::OnEvent` and `Activation::WhileTrue`. Both already
exist. This document supplies what they were missing to point at.

### 1a. Set-to-set creation IS a law

Worth stating plainly, because the two systems this framework gathers are usually
described as separate. They are not:

**Singular set-to-set creation is a change-writer, which is what a Law is.** The only
question is who wrote it and how it is carried:

| Carrier | Example | Who authored it |
|---|---|---|
| hard-coded C++ | `Tool::ShapeGenerator3D`'s developer bypass | the engine, as first mover |
| first-mover law | `createShapeGenerator3DLaw` — a `Spawn` over `concept-shape-3d` | the engine, but as law text a Person can read and disable |
| Person-authored law | any `ActionNode::Create` / `Spawn` / `Synthesize` a Person writes | a Person |

`ActionNode::Kind::Synthesize`'s own comment says it: *set-to-set creation is a
COMPOSITION, not a second creation machine.* A concept is the remembered word; a law is
what says the word. This framework uses both halves and adds nothing between them.

---

## 2. What UI actually is

Traditional interface, stripped of its chrome, is **three senses and one question**:

- **clicking** — a discrete gesture aimed at a location
- **scrolling** — an unbounded, signed, quantised gesture
- **keyboard** — a symbol stream, addressed to whatever holds focus

…and the question: *which being was that aimed at?*

That is the entire input surface of every desktop GUI ever built. Replicating the
traditional frameworks therefore requires exactly this much of Earthcall: **listen for
the triad, resolve the aim, publish the edges, and let authored `ActionNode`s do the
rest.** §4 is that, and it is about four hundred lines of C++.

Everything past parity comes from noticing that the triad is an *accident of hardware*,
not a fact about interaction — and Earthcall already names, governs, and holds as vessel
a much larger set of modalities. §9 is that.

---

## 3. The seam

`LAW_MIGRATION_FRAMEWORK.md` cuts every subsystem three ways. The cut for interaction:

```
Sense    which being is under the pointer; is the button down; what key came in
         → ENGINE, FOREVER. Raycasts, GLFW, the window's captured-pointer veto.
         → Singularity/Input/InteractionChannel

Decide   is this being a control; is it enabled; does the Person have standing;
         is the value in range
         → LAW. ConditionModel over property paths and Related() category edges.

Act      what the click changes — a colour, a number, a spawned being, an event
         → LAW. ActionModel: Set / Add / Map / Flow / Publish / Create / Spawn.
```

The line between Sense and Decide is the line this framework defends. A pointer channel
that decided *"this object is a button, so highlight it"* would have taken the ontology's
job. It reports what the ray hit. Whether that is a button is a question only the world's
own graph can answer.

**Corollary — the channel has no idea what a control is, and must not.** Search
`InteractionChannel.{hpp,cpp}` for "button": the word appears only as the name of a
physical mouse button.

---

## 4. The Interaction Channel

A `Law` subclass with `isFirstMover() == true` and the stable identifier
`interaction-channel` — the same shape as `creation-channel` and `locomotion-channel`,
registered by `syncRegister` in `EngineInit`, stepped once per frame from
`Engine::update` (never from a render function; collapsing the Creator Console once froze
every 3D tool, and that lesson is in `FIRST_MOVER_AUTHORING.md` §1).

### 4a. The levels — registered properties

Read them as `@interaction-channel.<name>` from anywhere, or bind them as math variables.
All registered, because refusal #6 admits no "nobody registered it yet."

| Path | Type | What it holds |
|---|---|---|
| `enabled` | toggle | Law's own. Write `false` to set the pointer down. |
| `pointerX`, `pointerY` | number | screen pixels |
| `pointerWorld` | vector | where the ray met the picked being |
| `pointerNormal` | vector | that surface's outward normal |
| `pointerDistance` | number | along the ray to the hit; `0` = nothing hit |
| `hoveredId` | text | identifier of the being under the pointer; `""` = none |
| `hoveredFace` | number | which face; `-1` = none |
| `hoveredU`, `hoveredV` | number | where on that face, in `[0,1]` |
| `pressedId` | text | who received the press currently held |
| `focusedId` | text | last being clicked, until another is |
| `leftDown`, `rightDown`, `middleDown` | toggle | button levels |
| `scrollX`, `scrollY` | number | this frame's wheel delta |
| `scrollTotal` | number | accumulated wheel — a tuner reads this directly |
| `dragX`, `dragY` | number | pointer delta this frame, while pressed |
| `dragTotalX`, `dragTotalY` | number | since the press began; cleared on release |
| `dragging` | toggle | has the press travelled past the click slop |
| `lastKey`, `lastKeyCode` | text / number | the most recent key |
| `keyDown` | toggle | is it down |
| `shiftDown`, `ctrlDown`, `altDown` | toggle | modifier levels |

### 4b. The edges — published events

Past-tense `noun-verbed`, published **on transitions only**. A per-frame "still hovering"
event would be the bug CLAUDE.md names outright; that is what the levels above are for,
read by a `WhileTrue` law.

| Event | Subject | Fires when |
|---|---|---|
| `object-hover-entered` | the being | the pointer arrives on it |
| `object-hover-exited` | the being | the pointer leaves it |
| `object-pressed` | the being | left button goes down on it |
| `object-released` | the being pressed | left button comes up, wherever the pointer is |
| `object-clicked` | the being | press **and** release on the same being, without travel |
| `object-drag-started` | the being pressed | the held pointer travels past the click slop |
| `object-drag-ended` | the being pressed | a travelled press is released |
| `object-scrolled` | the being under the pointer | any wheel notch (may have a null subject) |
| `object-focused` | the being | a press lands on it and it did not hold focus |
| `object-unfocused` | the being | focus moves elsewhere, **including to nothing** |
| `key-pressed` | the focused being, or null | a key goes down (repeats suppressed) |
| `key-released` | the focused being, or null | that key comes up |

Three of these carry decisions worth defending:

**`object-clicked` is not "the button came up."** A click is press and release on the
*same* being, with less than `kClickSlopPixels` of travel between them. Release
elsewhere is a **cancelled** click — the gesture every pointer surface in the world
honours, and the one a Person will reach for the first time they change their mind
mid-press.

**`object-unfocused` fires when focus goes to nothing.** A focus that can only be gained
is a focus that never leaves, and every keystroke after the first click would go on
reaching a being the Person walked away from.

**`kClickSlopPixels` is a first-mover constant, not a setting.** It is a fact about
hands. An authored law that could widen it could make every drag in the world a click.

### 4c. The readings — `@world.*`

`MathBinding.hpp` has carried a `registerWorldReading` extension point with **zero
callers** since it was written: readings *about* a subject, answered by whichever
modality channel knows. Interaction is what it was for.

| Reading | Answers |
|---|---|
| `@world.pointerOver` | is the pointer on **this** subject |
| `@world.pointerPressedOn` | is the held press **this** subject's |
| `@world.pointerFocused` | does **this** subject hold focus |
| `@world.pointerDistance` | how far the pointer's hit is from this subject |

The point of these is that a hover law needs **no per-object state at all**:

```
WhileTrue,  Everyone
  condition:  @world.pointerOver == true
  action:     Map color := <authored glow function>
```

Without them, "highlight what I am pointing at" needed either a bool on every object
nobody would remember to add, or the engine's `HighlightSystem` — which is exactly where
that behaviour lives today, hard-coded, where no law can reach it and no Person can
change it. That is a `LAW_MIGRATION_FRAMEWORK.md` rung this framework opens.

### 4d. What the channel deliberately does not do

- **It does not decide anything about controls.** §3.
- **It does not own an "interaction mode."** Modes are authored conditions.
- **It does not filter which beings may be pointed at.** The caller supplies the
  reachable set (the active Zone's world). Reachability is the Zone's question.
- **It does not consume events.** There is no `event.handled` flag and no capture/bubble
  phase. Every law that hears an edge gets it. Two laws firing on one click is not a bug
  to be arbitrated by a framework — it is two authored claims, and if they conflict, the
  conflict is *in the world*, visible, and resolvable by a metalaw. A propagation-stopping
  UI framework is a permission system, and `NO_BLACK_BOX.md` §2 says there is exactly one
  of those and it is `TransferPolicy`.

---

## 5. Fixing what was already claimed

Two things this framework had to repair before it could stand on them. Both are the same
failure — a documented system with no caller — and both are named here so the next
reader does not rediscover them.

**`Object::updateHoverState` had no callers, anywhere.** `OBJECT_HOVER_EVENTS_SYSTEM.md`
is 199 lines describing enter/hover/exit events, `Object` published all three, and
nothing in `src/` ever invoked the function. The doc's own "Integration with Mouse
System" section is written in the conditional — *"you would typically…"* — which is the
tell. `InteractionChannel::observe` is the caller now.

**And it published its enter edge twice.** The test compared against
`_wasHoveredLastFrame`, a field written one frame *behind* `_isHovered`, so entering
compared against the state from two frames ago. It went unnoticed for exactly as long as
it had no caller. Fixed at `ObjectEvents.cpp`; `interaction_channel_test` holds it.

**`_isHovered` and `_hoverPoint` were unregistered.** State the engine kept about a being
that no law could read — refusal #6, and the one access level no law can ever change.
Now `hovered` and `hoverPoint` on `Object`, read-only (they are *derived* from the
pointer; a law that could write them would be lying to every other law).

---

## 6. The archetypes, as law text

These are the five shapes of control every GUI has ever had. Each is given below in the
vocabulary a Person authors in — `Singularity/Input/ControlPatterns` builds exactly
these, as first-mover laws, so a test can exercise them without a window.

Five of the six are registered on boot by `syncRegisterControlPatterns`. **The key
command is a factory only**, called with the key it binds: which key, on which control,
is an authored choice, and there is no default worth the engine picking.

Throughout: `controlValue`, `controlMin`, `controlMax`, `controlStep`, `controlOn`,
`controlLabel` are **authored dynamic properties** (`ActionNode::AddProperty`, or stamped
onto the concept at capture). They are not C++ members on anything. `Singular`'s dynamic
property fallback makes them resolve through `PropertyPath` like any registered one.

### 6a. Button — the discrete act

```
Trigger:     object-clicked
Activation:  OnEvent      Scope: Subject
Condition:   Related(instance-of, category.control.button)
Action:      Publish  "control-activated"  (subject: the law's subject)
```

The button publishes rather than *does*. That is the whole design: a button's meaning is
"this being was activated," and what activation *means* is a second law, authored
separately, that hears `control-activated` and conditions on which being it was. One
button, any number of consequences, each independently authorable and disableable — which
is what a click handler could never be.

### 6b. Toggle — the two-state act

```
Trigger:     object-clicked
Activation:  OnEvent      Scope: Subject
Condition:   Related(instance-of, category.control.toggle)
Action:      Sequence( Map controlOn := 1 - o     (binding: o -> controlOn),
                       Publish "control-activated" )
```

**This was authored first as a pair of laws, and the pair does not work.** The obvious
form — "if off, turn on" and "if on, turn off", one law each — puts the branch in the
condition calculus where it looks like it belongs, reads as two sentences in the Law
Graph, and lets a Person disable the off-switch without the on-switch. All of that is
true and none of it survives contact with the network:

> the on-law writes `controlOn`; the write marks the Rete state fact dirty; the dirty
> fact re-activates the off-law's `Compare` terminal in the **next chain round**; the
> off-law fires inside the same tick. One click, both laws, no net change.

`kMaxChainRounds` bounds the loop. It does not make it wrong less often.

The rule this is an instance of, and it generalises well past toggles:

> **Two laws whose actions satisfy each other's conditions are a loop, not a branch.**
> A branch needs a condition its own action cannot invalidate — which for a toggle is
> impossible by definition, because flipping the state *is* the action.

So the flip goes into the mathematics, where it re-triggers nothing: the condition tests
only category membership, which no action here touches. `bool` is an arithmetic
alternative of `PropertyValue`, so `controlOn` reads as 0/1 and the written double
coerces back on the way in.

**And that last clause was not true when this was written.** A `Map` writing 1.0 into a
property *granted* as `bool` replaced the bool with a double, so every later
`std::get_if<bool>` read it as false: the toggle flipped correctly and looked stuck.
Registered properties had always coerced (`PropertyPath::setValue`'s retry); authored
ones did not — the same law text behaving differently depending on whether the property
was written in C++ or granted by an author, which is the split this tree refuses
everywhere else. `DynamicPropertyBridge::setValue` now coerces arithmetic-to-arithmetic,
and `control_patterns_test` case 3 is the regression.

### 6c. Slider — the continuous act

```
Trigger:     (none — Activation: WhileTrue, Scope: Everyone)
Condition:   All( Related(instance-of, category.control.slider),
                  @world.pointerPressedOn == true )
Action:      Flow controlValue := f(dragX) · dt
             (bindings: d → @interaction-channel.dragX,
                        s → controlStep)
```

A slider is a `Flow`: the authored model is the *rate*, integrated each tick, which is
what dragging is. `Map` would author the position directly; `Flow` authors its
derivative, and OntoMath's exact derivative/antiderivative make them exact counterparts
(`ActionModel.hpp`). Bounds come from the authored `Piecewise`'s own domain — a slider
that stops at its ends stops because its mathematics is undefined past them, not because
a `clamp()` was called.

### 6d. Tuner — the scroll act

```
Trigger:     object-scrolled
Activation:  OnEvent      Scope: Subject
Condition:   Related(instance-of, category.control.tuner)
Action:      Map controlValue := f(v, s, n)
             (bindings: v → controlValue,
                        s → controlStep,
                        n → @interaction-channel.scrollY)
```

The wheel is the one traditional input that is *already* a signed quantity rather than a
gesture, so it maps onto authored mathematics with nothing in between.

### 6e. Key command — the symbolic act

```
Trigger:     key-pressed
Activation:  OnEvent      Scope: Subject
Condition:   All( Related(instance-of, category.control.key-command),
                  Compare(@interaction-channel.lastKey == "<the key>") )
Action:      Publish  "control-activated"
```

The subject is whatever holds focus. A key command bound to a being that nobody has
focused simply does not fire, which is the correct behaviour and cost nothing to get:
`key-pressed` carries a null subject when focus is empty, and a law with
`Scope::Subject` and no subject has nobody to apply to.

### 6f. Hover response — the continuous *feedback*

```
Activation:  WhileTrue    Scope: Everyone
Condition:   @world.pointerOver == true
Action:      Map <any property> := f(...)
```

The feedback half of every control above, and the clearest demonstration of §4c: it
names no being, carries no per-object state, and governs every being in the world at
once. Restrict it with a category condition when it should not.

---

## 7. From one control to many

Here is where set-to-set creation stops being a separate system.

A control, captured: `ObjectConcept::captureFromBeings` takes the selection — the
button's body, its label, its frame — and remembers geometry, pose, **and the authored
property values** (`MemberTemplate::captured`), plus the relations *between* the members
(`RelationTemplate`). Instantiate it anywhere, any number of times; each instantiation is
an independent being.

And until this framework, **each instantiation arrived orphaned from its category.**
`RelationTemplate` recorded edges only where *both* endpoints were inside the source
set — so `instance-of → category.control.button`, whose far end is a category being that
is deliberately *not* part of the button, was dropped on capture. The newborn had the
geometry and the properties and was not a member of anything, so not one of the archetype
laws in §6 could see it. A hundred buttons, none of them buttons.

**Anchored relation templates** are the fix. A `RelationTemplate` may now name its far
end either by member index (an inter-member edge, as before) or by **identifier** — an
external anchor, resolved in the Universe at instantiation:

```
aIndex: 0                     the newborn for member 0
bAnchorId: "category.control.button"    resolved by identifier at birth
type: "instance-of"
```

Captured automatically for every relation from a member to a being outside the set. An
anchor that has left the world by the time the concept is instantiated is **skipped, not
guessed** — the same rule the index form already followed, for the same reason: an edge
that silently retargets is worse than a missing one.

This is the joint the whole framework turns on:

> **Set-to-set creation supplies the carriers. Law supplies the behaviour. Category
> membership is the joint — and it is an edge in the world's own graph, authored, named,
> and revisable, not a type in a header.**

One law governs a thousand instances, because the law asks a question about the graph and
the graph answers for all of them. `Law::requiredProperties` / `couldApplyTo` already
keep the `Scope::Everyone` sweep from visiting beings that could not possibly match.

---

## 8. Why a control kind is a category and not a string

`AUTHORED_CATEGORIES.md` §11 lists `object.setObjectType("chair")` as an anti-pattern: *a
string is not a being; no structure, no author, no properties.* A `controlRole: "button"`
property would be that anti-pattern with a UI accent.

The category form gives, at no extra cost:

- **structure** — `category.control.toggle` is `subcategory-of` `category.control`, so a
  law about controls in general reaches toggles (once materialised, §5a of that document)
- **shared data** — the category being carries `materialId`, so every button in a world
  shares a look by carrying one edge, and a Person restyles all of them by repainting one
  being
- **authorship** — the category has `authored-by` provenance. A claim about what things
  are, signed.
- **revisability** — a Person can mint `category.control.radial-dial` this afternoon
  without a build, and the archetype laws they author for it are ordinary law text

The categories this framework seeds (`category.control` and its five children) are
**first-mover authored, not privileged**. Nothing in C++ knows their names except the
factory that seeds them, and a world that deletes them and mints its own loses nothing but
the default laws that named them.

---

## 9. Past the triad

Everything above reaches parity with a conventional GUI. The reason to build it *this*
way is what parity costs elsewhere and does not cost here.

The channel in §4 is one modality. Earthcall names, governs, and is vessel for several,
and **every one of them already registers property paths and publishes events** — which
is the entire interface the archetype laws in §6 consume. So a control driven by any of
them is not an extension of this framework. It is the framework, unchanged, with a
different path in the binding.

| Modality | Channel | What a control could be aimed by |
|---|---|---|
| pointer / wheel / keys | `Singularity/Input/InteractionChannel` | §4 — the traditional triad |
| the vessel's body | `Singularity/Input/LocomotionChannel` | a control activated by standing on it, by approach speed, by looking |
| sound | `Singularity/Audio`, `OntoMath` acoustics | a control tuned by a sung pitch; a dial whose value *is* an amplitude |
| hardware | `Singularity/Physical/PhysicalChannel` | a real sensor as a slider, with no adapter layer |
| language | `Singularity/Language`, `Logos` | a control activated by being *named* — `Publish` from a language law |
| foreign processes | `Singularity/Foreign` | a model's inference as the gesture (`InferenceLawBridge`) |
| network | `Singularity/Network` | another Person's pointer, over the wire, as an ordinary event |
| authored mathematics | `Singularity/OntoMath` | the control's response curve, exact and piecewise, not an easing enum |

Two consequences worth naming.

**Response is mathematics, not a curve preset.** `Map` and `Flow` take an authored
`Piecewise` with real bindings. A button's press animation is a function a Person wrote
and can differentiate — not `ease-in-out` chosen from a list of eight.

**A control needs no screen.** Nothing in §6 mentions rendering. `Object` is spatial;
whether it is drawn, sounded, printed, or sent over a wire is the business of whichever
channel reads it (`ONTOMATH_FRAMEWORK.md` §1: *a channel reads OntoMath; it never decides
what the thing is*). An interface authored once is available to every modality that can
present it — which is not a feature that was added, but a consequence of never having
built a UI-specific object in the first place.

---

## 10. The refusals

Held against the six (CLAUDE.md), plus the three this framework adds.

| Refused | Why | Instead |
|---|---|---|
| a `Button` / `Widget` / `Control` / `UIElement` class | refusal #1 — a domain noun | an `Object` in a category |
| a `src/UI/` or `src/Interface/` directory | refusal #2 — the top level is the ontology | `Singularity/Input/` — a modality channel |
| a `ControlKind` enum | refusal #3 | `category.control.*` beings |
| a `Body` for a control | refusal #4 — `Body` is a Person's | geometry and material components |
| modelling a control as a `Person` | refusal #5 | it is an `Object` |
| unregistered pointer state | refusal #6 | §4a, all of it |
| a UI-only event bus / capture & bubble | one bus; consumption is a permission system | §4d |
| a layout engine | a `Formation` plus laws over its members | `Formation` |
| a retained widget tree with a diff pass | the world is the retained tree | the world |

**And a refusal specific to this framework:** *no control law may be written in C++
inside the channel.* The archetypes in §6 are built by factories that emit
`ConditionModel` / `ActionModel` **data** — the same trees a Person authors, the same
trees that serialize. `createShapeGenerator3DLaw` is the precedent, and its history is
the argument: while it was C++ inlined in `Engine::initLogic`, it shipped for a day
unable to fire at all, because nothing could test what only booting a window could reach.

---

## 11. Verification

### 11a. Automated

`tests/interaction_channel_test.cpp` — the Sense half, headless (`observe()` takes a
`Sense` struct and a being list, precisely so no window is needed):

1. every advertised property path resolves, and to the type the picker claims
2. the pointer picks the **nearest** being, not the first
3. `object-hover-entered` fires **once** per entry — the two-frame-edge regression
4. `object-hover-exited` fires when the pointer moves to another being, and when it
   moves to nothing
5. `object-pressed` → `object-released` → `object-clicked` on the same being
6. release on a *different* being: `object-released` fires, `object-clicked` does **not**
7. travel past the slop: `object-drag-started`, and on release `object-drag-ended` with
   no `object-clicked`
8. focus: `object-focused` on press; `object-unfocused` when focus moves, **including to
   nothing**
9. `key-pressed` carries the focused being; repeats do not republish
10. `enabled := false` blinds the channel: no edges, and hovered beings are released
11. `uiCaptured`: same, for the frame the foreign surface owns the pointer

`tests/control_patterns_test.cpp` — the Decide/Act half:

1. every archetype law is authored (`Law::applyTo` refuses `Unauthored` otherwise)
2. the button law fires on the clicked being and on no other
3. the toggle flips on every one of six consecutive clicks — the regression that
   caught the cascading pair, and would catch a return to it; and the whole thing stops
   when a Person disables the law
4. the key command fires on its key and no other, and reaches nobody when nothing holds
   focus — rather than everybody
5. the tuner moves `controlValue` by `controlStep` per notch, in the sign of the notch,
   and one instance's step is its own
6. one law governs **many** instances through category membership
7. a captured control concept instantiates **as a member of its category** — the anchored
   relation, surviving a JSON round trip — and the archetype law reaches the newborn
   without being re-authored
8. a control whose category being has left the world is not governed, and is not guessed

### 11b. Manual protocol

The automated tests establish that the mechanism is correct. They cannot establish that a
Person can *use* it, and CLAUDE.md's standing instruction — **don't claim a doc is
verified because you read the source; run things** — applies with special force here,
because every failure mode of an interface is a failure of feel.

Run the engine (`docs/BUILD_AND_ENVIRONMENT.md` for flags), then, in order:

**A — the sense is live.**
1. Open the Law Graph window. In any property picker, find the group
   **Channel — Interaction**. Confirm the paths in §4a are listed. *Fails if:* the group
   is absent (the channel is not registered) or a path is missing (it is not registered).
2. Move the pointer over an object and watch `hoveredId` in that picker's live readout.
   It must change to the object's identifier and back to empty. *Fails if:* it never
   changes (no step call), lags a frame (stepped from a render function), or holds the
   last value after the pointer leaves.
3. Point at two overlapping objects. `hoveredId` must name the **nearer** one.
4. Open an ImGui window and move the pointer over it. `hoveredId` must go empty and stay
   empty. *Fails if:* the world keeps receiving the pointer under the panel — the
   `uiCaptured` veto is not wired.

**B — the edges are edges.**
5. Open the law audit log. Hover an object for several seconds without moving. Exactly
   **one** `object-hover-entered` must appear. *Fails if:* one per frame, or two.
6. Press and release on one object: `object-pressed`, `object-released`,
   `object-clicked`, in that order, once each.
7. Press on object A, drag onto object B, release. Expect `object-pressed`(A),
   `object-drag-started`(A), `object-drag-ended`(A), `object-released`(A) — and **no**
   `object-clicked`. *Fails if:* a click is recorded, or the subject is B.
8. Press on an object, move less than the slop, release: a click, and **no**
   `object-drag-started`. This is the tremor case; try it with a real hand, not a
   scripted move.
9. Click object A, then click empty space. Expect `object-focused`(A) then
   `object-unfocused`(A). *Fails if:* nothing fires on the second click — focus that
   cannot be lost.
10. Type with A focused, then click away and type again. The first keys carry A as
    subject; the second set carry none.

**C — the controls are authored.**
11. Seed the archetypes (the `ControlPatterns` factories, or a save file with them).
    Confirm they appear in the Law Graph **as ordinary laws with visible condition and
    action trees**. *Fails if:* any is opaque, or is listed under first movers with no
    text — a control law with no readable text is C++ wearing a law's name.
12. Click a button control. Confirm `control-activated` in the audit log with the button
    as subject.
13. Disable the button law from the Law Graph checkbox. Click again: nothing. Re-enable:
    it works. *Fails if:* the click still acts — behaviour is somewhere other than the
    law.
14. Write `@interaction-channel.enabled := false` from a law or the property editor. The
    entire pointer surface goes dead, and any highlighted object releases. Set it back.
15. Edit the tuner's `controlStep` on one instance. Only that instance changes rate.
16. Capture a control with the set-to-set console, instantiate it three times, and click
    each. All three must respond, **with no new law authored.** *Fails if:* only the
    original responds — the anchored `instance-of` did not survive capture, and §7 is the
    section to read.
17. Save the world, quit, reload. Repeat 12, 15, and 16. *Fails if:* the archetype laws
    are gone (they are first movers and should be re-seeded), the *authored* control
    properties are gone (they should serialize), or category membership is gone.

**D — the point of it.**
18. Author, in the Law Graph and without touching C++, a control that does something no
    traditional toolkit offers: a dial whose value is driven by the Person's **distance**
    from it (`@world.pointerDistance` or a locomotion path), or a button that can only be
    activated while a sound is playing. If this takes more than a few minutes of
    authoring, the framework has failed at its actual purpose, and the place it failed is
    the thing to write down.

Record what you actually ran and what it did. A protocol executed and reported is worth
more than a green suite; a protocol quoted as though executed is worth less than nothing.

---

## 12. What is not built

Named here so nobody reads absence as completion.

- **Text entry.** `key-pressed` carries a key, not a character stream: no IME, no
  composition, no caret. A text field is authorable today as a key-command law per key,
  which is honest and unpleasant. The right answer is a `Language` channel concern.
- **Category closure materialisation.** `Related` is one hop
  (`AUTHORED_CATEGORIES.md` §5a). A law conditioned on `category.control` will **not**
  see a being that is only an `instance-of category.control.button`. The archetype laws
  in §6 each name their own leaf category, so this does not bite them — but it will bite
  the first Person who writes a law about controls in general. The propagation law of
  §5a is the fix, and it is not written.
- **Multi-pointer / touch.** One pointer. The channel's state is singular throughout.
- **Key-command registration.** The factory exists; nothing calls it on boot (§6).
- **A Person's edit to an archetype does not survive a save.** The archetypes are
  `FirstMoverLaw`s, so `LawManager::toJson` skips them and `loadFromJson` preserves the
  engine's copies — which means an edit made in the Law Graph holds for the session and
  is replaced by the freshly seeded version on the next run. That is the correct
  behaviour for a *first mover* (its truth lives in the engine), and the wrong behaviour
  for a *starting point a Person is meant to revise*, which is what these actually are.
  The resolution is to seed them as ordinary authored laws in a world save
  (`FIRST_MOVER_AUTHORING.md` §4d) rather than from C++ on boot — the same move
  `shape-generator-3d-law` has waiting for it. Until then, §11b step 17 will show the
  seeded version returning, and that is not a bug in the loader.
- **Right and middle button edges.** Levels are registered; only the left button
  publishes press/release/click edges. Adding the others is mechanical and deliberately
  not done on speculation.
- **Occlusion by non-objects.** The pick sweeps the active Zone's objects. A Person's
  body does not occlude the ray.
- **A layout law library.** §10 says layout is a Formation plus laws. No such laws are
  written; the claim is architectural, not delivered.

---

## 13. The point underneath

Every UI framework in existence is a second ontology, bolted to the side of the first. It
has its own object model (widgets), its own event system (capture and bubble), its own
notion of identity (element ids), its own composition rule (the widget tree), and its own
idea of what a thing is allowed to be. Applications then spend their lives translating
between the two — which is what "binding," "view models," and "state management" all
are.

Earthcall has one ontology. A control is a being; pointing at it is an event; what
pointing means is a law; a kind of control is a category; a control you can make more of
is a concept. Nothing here is a UI concept, and that is precisely why anything in the
world can be an interface, and why an interface can be made of anything in the world.

The button was never a special thing. It was only ever an object that someone agreed to
mean something by.
