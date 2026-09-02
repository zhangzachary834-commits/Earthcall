# Synthesis Studio — End-to-End Completeness & Usability Audit

**Auditor:** Claude Opus 5 (Claude Code)
**Session:** `session_01TE6SKxkBCVf2THDMpdwHPW`
**Date:** 2026-09-02, 01:48 PDT
**Requested by:** Zach — "audit the Synthesis Studio … for end-to-end completeness and usability"
**Constraint honoured:** no build and no `ctest` run this pass — Antigravity is building/testing in a
concurrent session. Every finding below is read from source and from the save files. Where a claim
needs a live run to settle, it is marked **[NEEDS RUN]** and says exactly what would settle it.

---

## What the Synthesis Studio is

Four artifacts, authored by Gemini Spark on Zach's authority, 2026-08-31 → 2026-09-01:

| Artifact | What it holds |
|---|---|
| `scripts/author_synthesis_studio.py` (949 ln) | the First-Mover authoring script; writes both save files |
| `saves/worlds/synthesis_studio.json` (4566 ln) | the session: camera, 10 materials, 10 categories, the zone, 11 authored laws |
| `saves/zones/SynthesisStudio/zone.json` (4350 ln) | the identity-stable Zone: 30 objects, 17 relations |
| `tests/law/synthesis_studio_app_test.cpp` (246 ln) | the integration test, 5 assertions blocks |

The intent is a playable in-world app authored entirely as data: a 3D console (2 buttons, 4 chord
pads, a slider, an easel) plus a 2D HUD dock (9 screen-space controls), driven by 11 laws that spawn
orbs, play notes, draw strokes, and toggle an ambient theme. Ontologically it is exemplary — no C++
class, no enum value, no directory. Refusals 1–7 are all kept. The problems are all in whether it
*works*.

**Verdict: the skeleton is sound and the app is roughly half-wired — and on a HiDPI display the
reachable half is smaller still.** 3D clicking works and spawning works. **Every 2D control in the
engine is unclickable on a Retina Mac (§A0, found by Zach's play-test after the first pass of this
audit), every sound in the studio is silent, nothing labels itself, the slider teleports on load, the
theme toggle is a one-way latch, and two of the eleven laws duplicate first movers the engine already
registers.** Detail below, worst first.

*Revision note:* the first pass of this document opened "Clicking works." That was true only of the 3D
pick. §A0 is the correction, and it is the finding that explains what a Person actually experiences.

---

## A. Blocking — the feature does not happen at all

### A0. Every 2D control is unclickable on a HiDPI display. The 2D draw and the 2D pick are in different coordinate spaces.

**Found by Zach's play-test, 2026-09-02, not by reading — the audit's first pass missed it.** His report:
*"I see a 2D red square in top left corner of the screen. Below that is a chromatic series of squares
stacked beside each other… I clicked on the squares it seemed to do nothing."* Both halves of that are
one bug.

`Object::draw2DObject` lays a `Shape2D` out with `_x2D`, `_y2D`, `shapeParams.width2D/height2D`, inside
a bracket opened as `currentRenderer().begin2D(fbWu, fbHu)` where `fbW/fbH` come from
`glfwGetFramebufferSize` (`EngineRender.cpp:121-136`). Both backends build the ortho straight from
those numbers — `glm::orthoZO(0, width, height, 0, …)` / `glOrtho(0, width, height, 0, …)`. So
**`_x2D` is in FRAMEBUFFER PIXELS.**

`InteractionChannel::step` fills `sense.pointerX/pointerY` from `glfwGetCursorPos`, which returns
**WINDOW POINTS** — the header even says so (`InteractionChannel.hpp:177`). The function then computes
the scale factor explicitly and applies it… **to the 3D ray only**:

```cpp
const float scaleX = (winW > 0 && fW > 0) ? (float)fW / (float)winW : 1.0f;   // = 2.0 on Retina
…
fbX = sense.pointerX * scaleX;      // the 3D ray gets the scaled coords
…                                   // sense.pointerX itself is left unscaled
```

and `observe()`'s 2D AABB test compares the **unscaled** pointer against the **framebuffer-space** rect
(`InteractionChannel.cpp:140-155`):

```cpp
const glm::vec4 rect = obj->getRect2D();          // framebuffer pixels
if (sense.pointerX >= rect.x && sense.pointerX <= rect.z &&   // window points
    sense.pointerY >= rect.y && sense.pointerY <= rect.w) { … }
```

On a Retina Mac (backing scale 2) every `Shape2D` therefore **draws at half its authored position and
is clickable at double it**:

| Being | Authored rect | Drawn (window points) | Clickable (window points) |
|---|---|---|---|
| `state.studio` | 100,100 → 200,200 | **50,50 → 100,100** — top-left corner | 100,100 → 200,200 |
| `hud.dock.bg` | 240,645 → 1040,700 | **120,322 → 520,350** — upper-left quadrant | 240,645 → 1040,700 |
| `hud.btn.spawn-orb` | 255,653 → 385,691 | **127,326 → 192,345** | 255,653 → 385,691 |

That is exactly what Zach saw: the red square in the top-left corner, and the dock — authored as a
bottom-of-screen strip — sitting *below and beside it* in the upper-left instead. The visible
rectangles have **no hit region on them at all**; the hit regions are down where nothing is drawn.

Two consequences beyond "the buttons don't work":

- **The phantom hit regions steal 3D clicks.** A 2D hit replaces any 3D hit unconditionally, so the
  invisible strip at window points (240–1040, 645–700) — which on a typical window sits right over the
  3D console — swallows clicks meant for the desk.
- **This is invisible on a scale-1 display**, which is why it survived. It breaks every `Shape2D` in
  the engine on every Mac, not just the Synthesis Studio's.

The tree is not consistent about this space elsewhere either: `Menu.cpp:123` opens its 2D bracket with
`winW/winH` (window points) while `EngineRender.cpp:132` opens the object bracket with `fbW/fbH`.

*Fix (smallest correct change):* scale the pointer into framebuffer space once in
`InteractionChannel::step` — the `scaleX`/`scaleY` it already computes — and store the framebuffer-space
value on the channel next to the window-point one, so the 2D AABB test and the 3D ray read the same
space and `dragTotalX`/`clickSlopPixels` keep their documented window-point meaning. Then pick one space
for `begin2D` across the tree and say which it is in `Renderer.hpp`'s boundary contract, because right
now two callers disagree.

*Also, separately:* `state.studio`'s face color is `[1.0, 0.0, 0.0]` — the un-authored default. The red
square Zach saw is a state holder with no appearance anyone chose. See A3.

### A1. Every sound is silent. `ActionNode::PlayAudio` is a stub with no listener.

`src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp:628-641`:

```cpp
case Kind::PlayAudio: {
    const PropertyPath freqPath = path;      // captured…
    const PropertyPath ampPath = input;      // …captured…
    const std::string matType = propertyName;// …captured…
    return [freqPath, ampPath, matType](const ECA::Event&, Singular& subject) {
        Core::EventBus::instance().publish(
            ECA::Event{"audio-synthesized", &subject, nullptr, std::time(nullptr)});
        emitEffect("PlayAudio", true);       // …and none of the three is ever read.
    };
}
```

and `src/Singularity/Audio/AudioSystem.cpp:123`:

```cpp
// We no longer subscribe to "audio-synthesized" - we use continuous sound-emitter objects.
```

`audio-synthesized` has exactly two occurrences in the tree: the publish above and that comment.
**Nothing listens.** So `law-studio-pad-play`, `law-studio-spawn-orb`, `law-studio-theme-toggle`,
`law-studio-draw-mode-toggle` and `law-stroke-hover-sound` — five of eleven laws, the entire musical
half of the app — produce no sound. Worse, `emitEffect("PlayAudio", true)` reports **success** into
the law audit trace, so the Law Graph and the application log both say the note played. That is
exactly the failure mode `ActionModel.hpp`'s own NodeOutcome comment was written to prevent
("a law whose every write silently failed report SUCCESS").

The surviving audio path is the continuous emitter in `AudioSystem::tick()`
(`AudioSystem.cpp:179-192`), which requires `acoustic.isSoundEmitter` truthy on the object:

```
$ grep -c isSoundEmitter saves/worlds/synthesis_studio.json
0
```

**No object in the studio sets it.** The pads carry `acoustic.frequency` / `.amplitude` /
`.waveType` and are never heard on either path. Additionally `acoustic.waveType` is authored
`"crystal"`, which is not one of `sine|triangle|square|sawtooth` (`AudioSystem.cpp:220-223`), so even
once an emitter path is reached the timbre silently degrades to sine.

*Fix shape (two independent halves):* (i) give `PlayAudio` a real executor — read `freqPath`/`ampPath`
off the subject and hand them to `AudioSystem`, or make it stamp `acoustic.isSoundEmitter` + a decay
so the existing emitter loop plays it; report `emitEffect(..., false, "no audio channel bound")` when
it cannot. (ii) In the save, either author `acoustic.isSoundEmitter` on the pads or add an
`AddProperty` node to `law-studio-pad-play`. Also fix `waveType` to a name the synthesizer knows.

### A2. Nothing in the studio displays text. Every label is invisible.

`Object::draw2DObject` (`src/ConstructedBeing/Singular/Object/ObjectRender.cpp:553-585`) draws a
filled rectangle plus a 1px border. **It does not render text, and it does not branch on
`ShapeKind::Text2D`.** The studio authors `controlLabel` on 15 objects ("Spawn Harmonic Orb",
"Pulse Rate", "C5"…) and `displayName` on all 30; none of it reaches a pixel. The 3D "signs"
(`studio.sign.canvas`, `.spawn-orb`, `.toggle-theme`, `.music`, `.slider`) are plain `Cube`s with a
`displayName` — they are blank blocks.

Net effect for a Person who loads the world: a dock of **nine unlabeled colored rectangles** at the
bottom of the screen and five blank blocks on a desk. The app is undiscoverable by inspection.

*Fix shape:* this is engine work, not save work — `Text2D` needs a real render branch (the
`hud.banner.title` object is already authored as `Shape2D`/12 and should become `Text2D`/13 once
there is something to render). Until then the studio needs a different affordance; colored blocks
with no legend is not one.

### A3. `state.studio` is a stray 100×100 click-eating rectangle in the HUD.

`state.studio` is the ambient-state holder (`themeNight`, `pulseRate`, `spawnCount`, `soundFreq`,
`soundAmp`). It was authored as `shapeKind: 12` = `Shape2D`, with **no** `shape.width2D` /
`shape.height2D` / `x2D` / `y2D`. Defaults apply: `_x2D = _y2D = 100.0f`
(`Object.hpp:178-179`), `width2D = height2D = 100.0f` (`ObjectTypes.hpp:71-72`).

So it renders as a visible 100×100 quad at screen (100,100) — and worse, the 2D pick in
`InteractionChannel.cpp:140-163` replaces *any* 3D ray hit with *any* 2D hit regardless of z-order:

```cpp
if (hit2D) { hit = hit2D; ... }   // a 2D hit occludes 3D unconditionally
```

`state.studio` has `zOrder2D = 0`, so it loses to the real HUD (z 10/20), but it wins against the
entire 3D world underneath its rectangle. A Person pointing at the studio through that square
gets no 3D hover, no click, no draw.

*Fix shape:* a pure state holder should not be a `Shape2D`. Give it a non-2D `shapeKind` and
`physicalObject(0)` (the way `CategoryManager::loadFromJson` does for categories), or park it
off-screen with an explicit zero size.

---

## B. Wrong behavior — the feature happens, incorrectly

### B1. The theme "toggle" is a one-way latch: `themeNight := themeNight + 1` on a `bool`.

`law-studio-theme-toggle` (`author_synthesis_studio.py:761`) writes
`@state.studio.themeNight := tn + 1`. `themeNight` is authored `bool false`. First click → 1 → true.
Second click → 2 → coerced back to true. **It never turns off.** The same shape is in
`law-control-toggle-archetype`, which writes `controlOn := o + 1`.

The engine's own first mover gets this right and the .cpp explains why, at length:
`createToggleLaw` (`ControlPatterns.cpp:145-160`) writes `controlOn := 1 - o`, with a 20-line comment
about why the flip has to live in the mathematics. The authored studio law did not adopt it.

*Fix:* `offset_terms("tn", 1.0)` → the `1 - tn` form (`{"c":1.0,"factors":{}}` plus
`{"c":-1.0,"factors":{"tn":1.0}}`).

### B2. Nothing reads `themeNight` or `pulseRate`. Two of the four controls have no consequence.

`themeNight` appears three times in the save (the property, and the law's path + binding) and
`pulseRate` twice. No law, no channel, and no C++ reads either one. So:

- **Toggle Theme button** — flips a boolean nobody consults. Nothing changes on screen.
- **Pulse Rate slider** — drives a number nobody consults (plus B3 below).

`spawnCount` is the same story (incremented, never read), but that one is defensible as a counter.

This is the single biggest *completeness* gap after audio: two of the studio's four control
archetypes are wired to dead state. A theme toggle needs a law that reads `@state.studio.themeNight`
and writes something visible — ambient/material color on `studio.platform.floor`, or a
`ConditionNode::compare` gating a color law.

### B3. The slider handle teleports off its track on the first tick, then runs unbounded.

`law-studio-slider-sync` is `WhileTrue`, `Scope::Everyone`, condition = *"is instance-of
category.control.slider"* — permanently true. Every tick it writes:

```
@state.studio.pulseRate := controlValue
position.x              := controlValue - 1.6
```

`studio.slider.handle` is authored at world x = 0.0 with `controlValue = 1.0`. **On the very first
frame after load the handle jumps to x = −0.6**, off the track (`studio.slider.track` sits at x = 0).
The authored placement and the authored law disagree, and the law wins immediately.

Then, dragging: the first mover `control-slider-law` (`ControlPatterns.cpp:166-194`) integrates
`dcontrolValue/dt = dragX · controlStep`. `controlStep` is authored `0.1` and `dragX` is a *pixel*
delta, so a 20 px/frame drag integrates at ~2 units/second — and **nothing clamps to the authored
`controlMin = 0.2` / `controlMax = 3.0`**; neither the archetype nor the studio law reads them. The
handle slides off the track in both directions and `pulseRate` goes negative.

*Fix:* pick an offset that agrees with the authored geometry (`position.x := (controlValue − 1.0)·k`
for a handle authored at the value's start), and either clamp in the studio law (an OntoMath
piecewise with bounds — which is what `Piecewise` is *for*: outside every piece is undefined, and a
law never fires on undefined math) or teach `control-slider-law` to honour `controlMin`/`controlMax`.

### B4. Two of the eleven laws duplicate first movers the engine registers on every boot.

`EngineInit.cpp:103` calls `syncRegisterControlPatterns`, which registers `control-button-law` and
`control-toggle-law` (`ControlPatterns.cpp:337-370`) as `FirstMoverLaw`s bound to `object-clicked`.
`LawManager::loadFromJson` (`Law.cpp:2283-2291`) **deliberately preserves first movers across a
load** and then adds the save's laws. The save's `firstMoverEnabled` map does not name either one,
so both stay enabled at their boot default.

The result on every click of a studio button:

| | condition | action |
|---|---|---|
| `control-button-law` (first mover) | instance-of `category.control.button` | publish `control-activated` |
| `law-control-button-archetype` (save) | instance-of `category.control.button` | publish `control-activated` |

Two identical laws, same subject, same tick. Same for the toggle pair — and there their *actions
disagree* (§B1: `1−o` vs `o+1`), so `controlOn` after one click depends on evaluation order.

**[NEEDS RUN]** whether the two `control-activated` publishes collapse into one Rete fact (which
would hide the double-spawn) or produce two agenda activations (two orbs per click). Settle it by
loading `synthesis_studio.json` in `earthcall_webgpu`, clicking Spawn Orb once, and counting
`interactive.harmonic.orb` objects in the zone.

Regardless of the answer this is a defect: the Law Graph shows a Person **two** button archetypes
and **two** toggle archetypes, and disabling either one changes nothing.

*Fix:* delete `law-control-button-archetype` and `law-control-toggle-archetype` from the script and
the save. The engine already provides both, first-wins, and `INTERACTION_AS_LAW.md` §6 says they are
the archetype. If the studio wants a *different* toggle, it should override by reusing the id
`control-toggle-law` — which is precisely what the "first-wins" comment in
`syncRegisterControlPatterns` was built to allow.

The other three duplicates (`law-art-stroke-draw`, `law-stroke-hover-sound`, `law-stroke-hover-glow`)
mirror `createStrokeDrawingLaw` / `createStrokeAcousticLaw` / `createStrokeGlowLaw`, which are
**declared, defined, and never called** — `seedArtCategories` and the three stroke factories have no
caller in `src/` at all (only `tests/law/add_relation_action_test.cpp` uses them). So those three do
not double-fire, but the engine is carrying three dead factories that a save had to re-author by
hand. Either register them in `syncRegisterControlPatterns` (and drop them from the save) or delete
them from `ControlPatterns.cpp`. Right now the tree says both things.

### B5. `active3DMode := "Draw"` is not a mode the creation channel knows, and the "toggle" never toggles back.

`law-studio-draw-mode-toggle` sets `@creation-channel.active3DMode := "Draw"` — unconditionally, with
no inverse. Clicking the Draw button a second time does nothing; there is no way back out of draw
mode from inside the studio.

`"Draw"` is also not in `kCreatorTools` (`CreationChannel.cpp:213-224`: Create, Select, FaceBrush,
FacePaint, Pottery, Rotate, Morph, Combine, Sculpt, Graph), so `creatorToolLawIdForMode("Draw")`
returns `""` and no creator-tool first mover arms. That is *intentional-looking* — the only consumer
is the stroke-draw law's condition, and the studio disables `shape-generator-3d-law` and
`tool-create-3d-law` in `firstMoverEnabled` so clicking does not also spawn cubes. But it means the
Creator Console's own mode display and the channel's mode are now out of sync, and pressing any
console tool button silently cancels draw mode with no feedback.

*Fix:* make it a real toggle (condition on the current value, or a `1−o`-style flip through a
`drawMode` bool on `state.studio` that the stroke law reads instead), and either register "Draw" in
`kCreatorTools` or document why it is deliberately outside the table.

### B6. Strokes drawn anywhere but a 3D surface land at the origin — or at screen pixel coordinates.

`law-art-stroke-draw` places newborn segments at `@interaction-channel.pointerWorld`.
`InteractionChannel::observe` sets that to `surface.point` when the ray hits geometry, `vec3(0)` when
it hits nothing (`InteractionChannel.cpp:167`), and — for a 2D hit —
`glm::vec3(sense.pointerX, sense.pointerY, 0.0f)` (`InteractionChannel.cpp:161`), i.e. **screen
pixels reinterpreted as world coordinates**.

So: drag on the easel → correct strokes. Drag on empty sky → every segment stacks at the world
origin. Drag across the HUD dock → segments appear ~700 units away. The studio has an
`studio.easel.canvas` with `isCanvas: true` and **no law reads `isCanvas`** — the intended
constraint was authored as data and never enforced.

*Fix:* add `@world.pointerOver == true` (already a registered world reading,
`InteractionChannel.cpp:75`) to the draw law's `All`, and ideally an `Overlaps`/`Related` clause
tying it to the canvas so `isCanvas` stops being decoration.

### B7. Newborn strokes are inaudible by construction, and newborn orbs stack on one point.

- `law-art-stroke-draw` sets `scale`, `color`, and one `instance-of` edge on each segment. It does
  **not** set `acoustic.frequency` / `.amplitude`. `law-stroke-hover-sound` then reads exactly those
  two paths — on objects that do not have them. Even with A1 fixed, hovering a stroke is silent.
- `law-studio-spawn-orb` has **no** `spawnPlacementPath` and hard-codes
  `position := (0.0, 1.6, 1.25)`. Every orb ever spawned occupies the same point. `spawnCount` is
  incremented and never used — it is the obvious binding for an offset
  (`position.x := spawnCount · 0.5 − …`), which would make the counter mean something.

### B8. `restY` is missing on all five HUD buttons; `law-studio-button-spring` binds an unread path.

`law-studio-button-spring` (`object-released`, all buttons) writes `position.y := restY`. The five
3D controls carry `restY`; the five `hud.*` controls do not. `readMathBindings` returns `nullopt`
when a bound path does not read, so the Map is undefined and does not write — correct behavior, no
crash — but it means the law sweeps and fails on half its subjects every release. The press-down
animation (`position.y := restY − 0.04`, in the spawn/pad laws) likewise never happens on the HUD.
Since 2D objects use `x2D`/`y2D` and not `position` anyway, the whole spring/press affordance is
3D-only and the HUD gives **no click feedback at all**.

---

## C. Data integrity — the save files

### C1. `saves/zones/SynthesisStudio/zone.json` has drifted from the script and lost 3 authored relations.

The script writes both files with identical `formationRelations` (20 edges). On disk, the world file
has 20 and the Zone identity file has **17**. The three missing:

```
category.art.stroke --subcategory-of--> category.art
category.control    --instance-of-->    author.gemini-spark
category.art        --instance-of-->    author.gemini-spark
```

`zone.json` (mtime 17:52) is an engine re-serialization — it carries `faceColors`, `renderMode`,
`authoritativeAxis`, `x2D`/`y2D`, float-rounded transforms, and a `deletable` block that the script
never writes. The world file (mtime 11:39) is the script's. So the app was run, the Zone was
persisted, and those three edges did not survive the round trip. Note which three: the edges that
survive are **exactly** the ones `seedControlCategories` re-creates at boot
(`ControlPatterns.cpp:66-98`), plus the object→category edges. The category-taxonomy edges that only
the save carries are gone. `applyFormationRelations` prints a REFUSED warning
(`Serialization.cpp:658-666`) and `Formation::addRelation` prints another (`Formation.cpp:79-96`) —
**[NEEDS RUN]** load the world with stderr captured and read which of the two fired; that names the
cause exactly.

Two things follow regardless of cause. (i) `category.art` currently has no author edge, so the art
taxonomy is unattributed — Refusal-6-adjacent and contrary to "nothing enters the world without an
author." (ii) There is **no reconciliation path**: re-running `author_synthesis_studio.py` restores
the three edges by clobbering the engine's persisted Zone (paint, positions, spawned strokes, orbs —
everything a Person did in there). "Save files are sacred"; this script overwrites one
unconditionally, with no backup and no merge.

*Fix:* the script should refuse to overwrite an existing `saves/zones/SynthesisStudio/zone.json`
without an explicit `--force`, and say so. Also flip the two `instance-of author.gemini-spark` edges —
a category is not an instance of its author; `seedArtCategories` uses `authored-by`, which is the
right type and the one already in the vocabulary.

### C2. Seven material ids are dangling.

`materialId` referenced by objects but not defined in the save's `materials` array:

```
material.studio.C5   material.studio.E5   material.studio.G5   material.studio.B5
material.studio.sign.blue   material.studio.sign.gold   material.studio.sign.dark
```

That is all four chord pads and three of the five signs. `materials` defines exactly 10 names, all
used; the script authors these seven ids on objects and never mints the materials. The pads and
signs fall back to `resolveOrDefault` and render with no authored appearance.

### C3. First Movers and a Person are stored as *categories*.

`saves/worlds/synthesis_studio.json` `categories` holds ten entries, three of which are not
categories: `author.gemini-spark`, `grok-4.6`, `Zach`.

This is load-bearing, not cosmetic — `EngineInit.cpp:153-156` pushes every `CategoryManager` entry
into `Universe::beings()`, which is how `LawManager::loadFromJson`'s author reattachment
(`Law.cpp:2309-2320`) finds `author.gemini-spark` and how the laws avoid loading `Unauthored`. It
works. But it means a human Person (`Zach`) and two First Movers are living in the world as
`category.*` Objects with `physicalObject(0)`, which is the exact gap the To-do list already names:
*"Build first-class First Mover framework … no `FirstMover` as Singular, no property registration for
First Mover state."* The Synthesis Studio is the first concrete consumer of that gap; it should be
cited on that task.

### C4. First load will silently shadow the hand-authored JSON with a generated `.ecform`.

`ZoneManager::loadState`'s `physical-matter` stage (`ZoneManager.cpp:1264-1282`) sees a legacy JSON
world with no `.ecmatter`, writes `saves/worlds/synthesis_studio.ecmatter`, and writes
`saves/worlds/synthesis_studio.ecform`. `SaveSystem::listWorlds` (`SaveSystem.cpp:64-90`) then
**prefers `.ecform` over `.json` for the same stem**. From the second load onward the app reads the
generated `.ecform`, and edits to the script's `.json` are ignored with no message.
`my_world.ecform` and `my_world_v2.ecform` in `saves/worlds/` show this has already happened twice.

*Fix (or at minimum document):* the migration should say what it did in `lastLoadReport`, and the
authoring script should delete a stale `.ecform`/`.ecmatter` pair when it rewrites the `.json`.

---

## D. The test proves less than it appears to

`tests/law/synthesis_studio_app_test.cpp` passes 5/5 (per the To-do list entry of 2026-08-31) and it
is registered with the right `WORKING_DIRECTORY` (`CMakeLists.txt:382-385`). But look at what it
asserts.

**It never loads a single law from the save.** Tests 2, 3, and 4 hand-build fresh `ActionNode`s in
C++ that *resemble* the authored ones, compile those, and assert the compiled C++ behaves:

```cpp
// Test 2 — this is NOT law-studio-spawn-orb; it is a C++ replica of it
ActionNode spawnOrbAction = ActionNode::create(1, "interactive.harmonic.orb", { … });
auto domainExec = spawnOrbAction.compile();
domainExec(activatedEvent, *zone);
assert(zone->getOwnedObjects().size() == countBefore + 1);
```

Test 5 is the only one that touches the authored laws, and it checks `lawsJson.size() == 11` and that
two ids are present. **Nothing deserializes `conditionModel` / `actionModel`, nothing binds a
trigger, nothing runs `LawManager::tick()`, nothing publishes an event.** Consequences:

- The replica in Test 2 sets `position (0, 1.6, 1.2)`; the save says `(0.0, 1.6, 1.25)`. The replica
  sets `scale 0.4`; the save says `0.45`. The replica omits the `@state.studio.spawnCount` increment
  and the `PlayAudio` node entirely. The test would pass unchanged if the save's laws were deleted.
- Test 3 calls `padAction.compile()` including `ActionNode::playAudio(...)` and asserts **nothing
  about the result** — it prints "executed audio and tactile reaction." That is how A1 (a PlayAudio
  that does nothing and reports success) survived a green test.
- Every finding in §A and §B above is invisible to this test.

*Fix shape:* the test should build a `LawManager`, call `loadFromJson(worldJson["authoredLaws"])`,
assert each law loaded **authored** (not `Unauthored` — the author reattachment in §C3 is exactly the
kind of thing that breaks quietly), publish a real `object-clicked` through the `EventBus`, run
`tick()`, and assert on the world that results. That is a rewrite of tests 2–4, not a patch, and it
is the single highest-value follow-up in this audit: it turns the studio from a fixture into a
regression net for the whole interaction-as-law stack.

---

## E. Documentation — there is none

```
$ grep -rl "Synthesis Studio\|SynthesisStudio" docs/ "agent intercom/"
(no matches)
```

The Synthesis Studio is not in `docs/architecture/`, not in `docs/audits/` (until this file), not in
`docs/plans/`, not in the Agenda, and not in the intercom. Four artifacts and 10,000 lines with no
prose anywhere saying what it is, how to open it, or what a Person should expect to see. Neither save
file carries the `injected_by:` marker CLAUDE.md requires for agent-injected saves (0 occurrences in
both the save and the script), though the laws do carry proper `provenance` `authored-by` edges to
`author.gemini-spark`, which is the substantive half of the covenant.

---

## What a Person actually sees today

Load `synthesis_studio` from the save menu. You get (confirmed against Zach's play-test, 2026-09-02):

- a dark room with a desk, an easel, a pedestal, five blank blocks (the "signs"), two blank
  button-blocks, four blank pad-blocks, a slider whose handle has **jumped 0.6 units off its track**;
- a **red square in the top-left corner** (`state.studio`, an invisible state holder with the default
  red face color) and, below and beside it in the upper-left quadrant rather than along the bottom, a
  **chromatic strip of nine unlabeled rectangles** — the HUD dock, drawn at half its authored position
  by §A0;
- **clicking any of those rectangles does nothing**, because their hit regions are at double the
  authored position, down where nothing is drawn — and those invisible regions swallow 3D clicks
  aimed at the console;
- clicking Spawn Orb: a gold sphere appears at (0, 1.6, 1.25) — every time, in the same place,
  possibly two at a time (§B4). **No sound.**
- clicking Toggle Theme: nothing visible, ever. The boolean latches on and stays.
- clicking a chord pad: the 3D pad dips 4 cm and springs back. **No sound.**
- clicking Draw Stroke, then dragging on the easel: gold dabs follow the pointer — this one works.
  Dragging anywhere else piles them at the origin. No way to turn draw mode back off.
- dragging the slider: the handle slides off the track and keeps going.

**Roughly half the authored intent reaches the Person.** The creation half (orbs, strokes,
categories, relations, spawn) is real and works. The *feedback* half — sound, labels, theme, the
slider's meaning — does not exist yet, and three of those four are blocked on engine work rather than
on the save.

---

## Recommended order of work

0. **Put the 2D pointer and the 2D rectangles in the same coordinate space** (§A0). Engine, small,
   and it is the one that decides whether a Person can touch the app at all. Everything below is
   unobservable until this lands.
1. **`ActionNode::PlayAudio` — implement it or make it fail honestly** (§A1). Engine. Unblocks five
   laws and removes a lying success report. Highest value in this list.
2. **Rewrite `synthesis_studio_app_test` to load and tick the authored laws** (§D). Test. Without
   this, every fix below is unverified and the next regression is silent again.
3. **Delete the two duplicated archetype laws from the save** (§B4) and decide the fate of the three
   uncalled stroke factories in `ControlPatterns.cpp` — register them or delete them.
4. **Save fixes, one script pass** (§B1 flip, §B3 offset+clamp, §B5 real toggle, §B6 pointerOver
   guard, §B7 acoustics + spawn offset, §B8 `restY` on HUD, §A3 `state.studio` shape, §C2 seven
   materials) — plus the `--force` guard and `.ecform` cleanup in §C1/§C4 **before** re-running it,
   or the Zone's persisted state is destroyed.
5. **Author the missing consequence laws** (§B2): something must read `themeNight` and `pulseRate`.
6. **Text rendering** (§A2). Engine, larger. Until it lands the studio needs some other legend.
7. **Write the doc** (§E) and add `injected_by:` to both saves.

---

## Verified vs. inferred

**Confirmed by Zach's play-test on 2026-09-02 (first-hand, not inferred):** A0 (both symptoms — the
top-left red square and the mis-positioned dock — and the dead clicks), A2 (no labels, no widget
affordances, "no tooltip, no manual"), A3.

**Read directly from source or the save files (high confidence):** A1 (both the stub and the absent
listener), A2, A3, B1, B2, B3 (authored values and the missing clamp), B5, B6, B7, B8, C1 (the 3-edge
delta and the mtime/field evidence), C2, C3, C4, D, E.

**Not settled without a run:** whether the duplicated archetype laws produce one or two
`control-activated` facts per click (§B4); the exact refusal that dropped the three relations (§C1);
and the on-screen result of every §A/§B item, since I did not launch `earthcall_webgpu` this pass.

No build, no `ctest`, and no file in the working tree was modified by this audit apart from this
document and the To-do list entry that accompanies it.

---
*Claude Opus 5 · `session_01TE6SKxkBCVf2THDMpdwHPW` · 2026-09-02 01:48 PDT*
