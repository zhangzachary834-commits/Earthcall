# Claude Opus 5's Refusals Audit — 2026-08-11

Scope: the whole of `src/` (303 C++/Python files, excluding `venv/`, `node_modules/`,
`third_party/`, `local_deps/`, `imgui/`) read against the five refusals in `CLAUDE.md`
and their governing documents — `NEW_KIND_FRAMEWORK.md` (Kind Floor §6, Admission Test §2),
`DIRECTORY_ORDERING.md` (§2, §3, §5, §7), `AUTHORED_CATEGORIES.md` (§10, §11),
`LAW_MIGRATION_FRAMEWORK.md` (§2 ladder).

Method: direct read of every header declaring a class, struct, or enum; call-site counts
for each kind-enum; cross-check of each finding against the doctrine text that governs it.
No files were modified. **No build was run** — see §0, which explains why one could not be.

Branch `sync-from-earthcall-main`, working tree at `d3b96de` + uncommitted changes.

> **Revision 2, 2026-08-11.** §1 has been rewritten following a ruling by the world's
> author on the status of the human form in the `Person` substrate. Two findings were
> withdrawn, three filed in their place, and §5 (Teleologically), P2, L1, L2 and L6 revised
> to follow. The ruling and its consequences are at the head of §1. Everything outside §1
> and the sections named here is unchanged from revision 1.

---

## §0 — The finding that governs the rest

**`Game` was not eliminated. It was promoted into the ontology.**

Two facts, both verifiable right now:

1. `src/Singularity/Core/Game.hpp`, `Game.cpp`, and six `Game*.cpp` files are deleted in
   the working tree (` D` in `git status`, still present at `HEAD`). Fourteen files still
   name `Core::Game`, and `src/ZonesOfEarth/Ourverse/OurverseUI.cpp:3` still does
   `#include "Game.hpp"`. The tree does not compile in its current state. The deletion
   happened partway through this audit, so this is in-flight work, not neglect — but the
   plan in §5 has to start from it.

2. The state that made `Game` a god object did not die with it. `Ourverse.hpp:52–55`
   carries, line for line, the four enums that stood at `Game.hpp:372–375` at `HEAD`:

   ```cpp
   enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };
   enum class Mode3D { None = -1, FacePaint = 0, FaceBrush, BrushCreate, Pottery,
                       Rotation, Selection, Morph, Combine, Sculpt, Graph };
   enum class ToolTarget3D { WorldObjects = 0 };
   enum class CreatorSection { Paint = 0, Create3D, Character, World, Assets,
                               Relations, Zones };
   ```

   alongside `showIntegrationUI` and the rest of the creator-panel state, in a class
   declared `class Ourverse : public Singular` with
   `getIdentifier() const override { return "Ourverse"; }`. `OurverseUI.cpp` is 61 KB.

The second fact is worse than the first, and it is the reason this audit exists.
`Game` was *honestly named*: `GAME_ELIMINATION_PLAN.md` is referenced in its own first
line — "FROZEN: No new code." A frozen, dishonestly-shaped class that everyone knows is
dishonestly shaped is a manageable debt, because the tree tells you the truth about it.

`Ourverse` is a **being**. It inherits `Singular`. It answers `getIdentifier()`. It is
addressable by law text. And the thing that is now addressable by law text is a mode
enum containing `Pottery` and a boolean called `showIntegrationUI`.

This is the single most expensive kind of error available in this repository, because it
is the one that cannot be seen from the outside. Every other finding below is a subsystem
that has not yet been migrated. This one is a migration that ran **backwards**: it moved
non-ontological state *into* the ontology and gave it the credentials of a being.

The refusals exist to stop exactly this. The general form stated in `CLAUDE.md` — *no
subsystem may define what a thing IS* — was violated in the specific direction the
documents did not anticipate: not a subsystem defining a being, but a subsystem
**becoming** one.

Everything else is §1–§4.

---

## §1 — Refusal 1: no new C++ class for a domain noun

> *Not `RobotEntity`, not `Vehicle`, not `Tree`, not `Category`. This principle also
> extends to hardcoded fields (e.g. `health` or `inventory` on a `Person`).*
> — `CLAUDE.md`; `NEW_KIND_FRAMEWORK.md` Kind Floor §1, §5

### Ruling of the world's author — 2026-08-11

The first draft of this audit filed the body-part classes as the largest violation in the
repository. **The world's author has ruled otherwise, and the ruling is recorded here
because it is doctrine, not an exception to it:**

> The fundamental human form — head, torso, arms, legs, hands, feet, fingers — is
> constitutive of what it is to be a `Body`. A human body cannot be conceived without its
> members. The `Person` substrate is not raw metaphysical metal the way the rest of the
> program is; it is a **vessel that represents the Person using it**. The human form
> therefore has a place as an invariant in the fundamental substrate — as a hardcoded
> class, field, member, or other hardcoded form.
>
> This does **not** mean every `Body` must conform. A `Body` may also be one of many
> custom avatars a Person authors, ranging from small round figures through Wii-style
> characters to nonhuman entities.

This is coherent and it changes the finding rather than merely waiving it. The refusals
protect against a subsystem deciding what things are *in a world the Person authors*. The
Person substrate is the one region that is not that world — it is the world's interface to
the human outside it, and an interface that varies is not an interface. `Body` is already
on Kind Floor §1's closed list of legitimate `Object` subclasses for exactly this reason
(Floor §2: a `Body` "implies embodied personhood, a Singular that is *someone*"). The
ruling extends the same reasoning one level down, from the vessel to its members.

**The two findings that survive the ruling are different from the ones it dismisses**, and
both follow from the ruling's own terms: an invariant must be stated correctly and
completely (V1), and an invariant that admits non-conforming cases must have a path for
them (V2). They are filed below, renumbered. The former V1/V2 are withdrawn.

### V0 — HIGH — the ruling is not written down where the refusals are

Not a code defect. A doctrine defect, and the most immediately dangerous item in this
section.

As of this audit, three documents say the opposite of the ruling:

- `CLAUDE.md` Refusal 1: *"No new C++ class for a domain noun. Not `RobotEntity`, not
  `Vehicle`, not `Tree`…"* — with no carve-out for anatomy.
- `NEW_KIND_FRAMEWORK.md` Kind Floor §1 enumerates the legitimate `Object` subclasses —
  `Person`, `Relation`, `Formation`, `Law`, `Zone`, `World`, `ObjectConcept`, `Body` —
  and states that **"the list closes to domain nouns permanently."** `BodyPart`, `Limb`,
  `Arm`, and their eight siblings are not on it.
- `NEW_KIND_FRAMEWORK.md` §2's Admission Test terminates at Q1 with *"author it. Write no
  C++. STOP."* for anything not requiring a hardware channel.

`CLAUDE.md` opens by warning that a reader is *"almost certainly about to do the standard
thing,"* and the standard thing for a competent agent reading those three documents
against `src/Person/Body/BodyPart/Limb/Arm.hpp` is to delete it. This audit's first draft
did precisely that, from precisely that reasoning, which is the demonstration rather than
the hypothetical.

An unstated exception to a written refusal does not survive contact with the next careful
reader. It will be "fixed," correctly by the documents and wrongly by the ontology, and
the fix will be defensible in review. The ruling must land in the doctrine:

1. `CLAUDE.md` Refusal 1 gains the Person-substrate carve-out and a one-line statement of
   why (vessel, not metal).
2. `NEW_KIND_FRAMEWORK.md` Kind Floor §1 admits `BodyPart` and the constitutive members
   explicitly, alongside Floor §2's existing reasoning about `Body` and personhood.
3. The carve-out is bounded in the same breath — it covers the constitutive human form
   under `Person/Body/` and nothing else, so it cannot be cited as precedent for
   `class Vehicle`.

L1 (§7) is the mechanical form of the same point: once the refusals are a test, the
carve-out is an allow-list entry, and the exception becomes impossible to lose.

### V1 — HIGH — the invariant is asserted incompletely, and conflates two tiers

The ruling names the constitutive members: **head, torso, arms, legs, hands, feet,
fingers.** What `src/Person/Body/` actually contains is:

| File | Class | In the ruling's list? |
|---|---|---|
| `Body/Head/Head.{hpp,cpp}` | `Head` | yes |
| `BodyPart/Limb/Torso.{hpp,cpp}` | `Torso` | yes |
| `BodyPart/Limb/Arm.{hpp,cpp}` | `Arm` | yes |
| `BodyPart/Limb/Leg.{hpp,cpp}` | `Leg` | yes |
| `BodyPart/Limb/Hand.{hpp,cpp}` | `Hand` | yes |
| `BodyPart/Limb/Foot.{hpp,cpp}` | `Foot` | yes |
| `BodyPart/Limb/Neck.{hpp,cpp}` | `Neck` | **no** |
| `BodyPart/Limb/Shoulder.{hpp,cpp}` | `Shoulder` | **no** |
| `BodyPart/Limb/ForeArm.{hpp,cpp}` | `ForeArm` | **no** |
| `BodyPart/Limb/ForeLeg.{hpp,cpp}` | `ForeLeg` | **no** |
| — | *finger* | **yes, and absent entirely** |

`grep -rni "finger\|digit\|phalan" src/Person` returns nothing. A member the ruling calls
constitutive of the human form does not exist in the substrate that is supposed to hold
the invariant. Hands are terminal.

The four unlisted classes are not an error either — they are a *different kind of thing*.
`Neck`, `Shoulder`, `ForeArm`, `ForeLeg` are articulation: the skeletal subdivision that
makes the constitutive members bend. That distinction is real and worth making explicit,
because the invariant is a claim about **what a human body has**, while articulation is a
claim about **how it moves** — and the second is far more negotiable than the first.
A Person's avatar may plausibly have a differently-jointed arm; it does not have a
differently-existing arm.

Right now both tiers are flattened into one inheritance chain (`Limb : BodyPart`, with
`Shoulder`, `Arm`, `ForeArm` as siblings), so nothing in the type system or the source
records which members are constitutive and which are articulation. State the two tiers,
then complete tier one — fingers included.

### V2 — HIGH — the ruling's second half has no implementation

The ruling explicitly permits non-conforming bodies: *"a `Body` could also just be one of
many of the Person's custom avatars… little cute balls or Wii-style figures to nonhuman
entities."* None of that is currently representable.

- `Person.hpp:230` — `Body body;` A Person holds **exactly one** `Body`, by value, taken
  in the constructor (`Person(Soul soul, Body body, const std::string& joyOrdering)`).
  "One of many of the Person's custom avatars" has no place to live.
- `Body.cpp:92` — `Body::createBasicAvatar` is the **only** assembly path in the file.
  Its 70 lines of `new Head()` / `new Torso()` / `new Shoulder(Side::Left)` are not merely
  *a* body; they are the only body the code can produce.
- There is no `ObjectConcept` path into `Body::formation` (`Body.hpp:23`), even though the
  member exists and `ObjectConcept` + `MemberTemplate` + `RelationTemplate`
  (`ObjectConcept.hpp:55–80`) is exactly the machinery an authored avatar needs.

So the invariant is hardcoded — correct under the ruling — and the authored path beside it
is absent. The result is a substrate that enforces the human form on *every* body, which is
what the ruling explicitly says it should not do. The refusal was wrong about the
hardcoding; it was right that a Person cannot currently author their own form, and that
half of the finding stands unchanged.

This is now the highest-value item in §1, because it is the one the ruling *asks for*
rather than tolerates.

### V1-withdrawn — the body-part class tower

*Withdrawn per the ruling above. Retained in outline so the reasoning is not re-derived and
re-filed by a later reader.*

The original finding was that `src/Person/Body/BodyPart/` contains eleven concrete C++
classes for anatomical domain nouns, each a subclass of
`BodyPart : public Object, public Formation`, in violation of Kind Floor §1:

| File | Class |
|---|---|
| `Body/Head/Head.{hpp,cpp}` | `Head` |
| `BodyPart/Limb/Torso.{hpp,cpp}` | `Torso` |
| `BodyPart/Limb/Neck.{hpp,cpp}` | `Neck` |
| `BodyPart/Limb/Shoulder.{hpp,cpp}` | `Shoulder` |
| `BodyPart/Limb/Arm.{hpp,cpp}` | `Arm` |
| `BodyPart/Limb/ForeArm.{hpp,cpp}` | `ForeArm` |
| `BodyPart/Limb/Hand.{hpp,cpp}` | `Hand` |
| `BodyPart/Limb/Leg.{hpp,cpp}` | `Leg` |
| `BodyPart/Limb/ForeLeg.{hpp,cpp}` | `ForeLeg` |
| `BodyPart/Limb/Foot.{hpp,cpp}` | `Foot` |
| `BodyPart/Limb.{hpp,cpp}` | `Limb` |

Kind Floor §1 names the closed list of legitimate `Object` subclasses — `Person`,
`Relation`, `Formation`, `Law`, `Zone`, `World`, `ObjectConcept`, `Body` — and `Arm` is
not on it. The argument was that `Body.cpp:92–167`'s `createBasicAvatar` is a First Mover
authoring the structure of a human body in C++ where `FIRST_MOVER_AUTHORING.md` says a
seed world authors it, and that `Body` already has an unused `Formation formation;`
member (`Body.hpp:23`) for the purpose.

**Why the argument fails against the ruling:** it treats `Body` as world-content, and
`Body` is not world-content. It is the vessel through which a human is present in the
world at all. `FIRST_MOVER_AUTHORING.md` governs what a First Mover puts *into* the world;
the shape of the human at the boundary is a property of the boundary. Kind Floor §2
already reasons this way about `Body` itself — the correction is that it stops one level
too high.

The one durable observation from the original finding, now demoted to a code note: seven
classes each declare an identical `enum class Side { Left, Right }`. Under the ruling
bilateral symmetry is itself part of the invariant, so the enum is fine — but it should be
declared once on `BodyPart` and inherited, not seven times.

### V2a — MEDIUM — `BodyPart::Type` misstates the invariant it now encodes

`BodyPart.hpp:15`:

```cpp
enum class Type { Undefined, Head, Torso, Arm, Leg, Hand, Foot };
```

Under the ruling this enum is *admitted* — it is the invariant human form written down,
which is exactly what the ruling asks for. **That raises the stakes on its correctness
rather than lowering them.** An incidental taxonomy that is a bit wrong is debt. A
canonical statement of what a human body is, that is wrong, is a canonical error.

It is wrong in three places, and the source says so:

- `Shoulder.cpp:5` — `BodyPart::Type::Arm, // categorize as arm for now`
- `ForeArm.cpp:5` — `BodyPart::Type::Arm,`
- `ForeLeg.cpp:5` — `BodyPart::Type::Leg,`

A shoulder is not an arm. These three are the articulation tier of V1 being forced through
an enum that only has vocabulary for the constitutive tier — the conflation, showing up
exactly where V1 predicts it will. `Person.cpp:430–489` then switches on the result to
compute pose, so the pose of every human body in Earthcall is derived from a
classification the source itself marks `// for now`.

Two further consequences of the enum's new canonical status:

1. **It has no `Finger`,** because the substrate has no fingers (V1). Completing the
   invariant means appending here.
2. **If it is ever serialized, it becomes append-only under Refusal 3** — with all the
   permanence §3 describes. It currently is not persisted (`Body`'s parts are rebuilt by
   `createBasicAvatar`), which is the only reason `Undefined = 0` and the mislabelling are
   still cheap to fix. That window closes the moment a `Body` round-trips through a save.

Fix it before P2 writes anything that depends on it: split the tiers, add `Finger`, give
`Shoulder`/`ForeArm`/`ForeLeg`/`Neck` honest values.

### V3 — HIGH — tool state carved into `Person`

`Person.hpp:52–90` declares, as C++ members of the class that *strictly represents an
actual human being*:

```
activeTool, active3DMode, activeShapeKind, cursorHitPos, cursorHitNormal,
cursorSpawnPos, cursorSpawnRot, cursorSpawnScale, placementMode, gridSnap,
gridSnapSize, inFrontDistance, manualOffset, manualAnchorValid, manualAnchorPos,
manualAnchorRight, manualAnchorUp, manualAnchorForward, cursorHoveredBodyPart,
cameraPos, cameraForward, activeColor
```

Twenty-two fields. This is the `health`/`inventory`-on-`Person` case the refusal names,
except the domain is a paint program rather than an RPG.

**In fairness:** the comments make the intent explicit — *"Mirrors of the brush placement
state the creation tools run on, so a law can read and override what the hard-coded tool
used to consume directly."* That is `LAW_MIGRATION_FRAMEWORK.md` R1, the Property Bridge,
correctly identified and correctly motivated. This is not carelessness; it is rung 1 of a
six-rung ladder.

**The finding is that it stopped there, and that it is bridged onto the wrong being.**
R1 says make the state legible; it does not say make it a member of `Person`. A bridge
belongs on the *channel* whose state it exposes — the `PhysicsLawBridge` /
`PhysicalChannel` pattern, with a stable slug like `@creation-channel.placement-mode`.
Bridging onto `Person` means the ontological definition of a human being now contains
`gridSnapSize`, permanently, in the save format.

Worse, `Decide` came along with it. `Person::computeSpawnPosition()` and
`Person::spawnSurfaceOffset(normal)` (`Person.hpp:80–85`) are *logic*, not state. A
Person, in this ontology, now knows how to compute where a brush places a cube.

**The §1 ruling does not reach this finding, and sharpens it.** The ruling exempts what is
constitutive of a human's presence in the world — the vessel and its members. `position`,
`velocity`, `grounded`, `cameraPos`, `cameraForward` and `body` are that, and belong on
`Person` for the same reason `Head` belongs on `Body`. `gridSnapSize`, `placementMode` and
`activeShapeKind` are the state of a *tool the Person happens to be holding*, which is the
opposite category: contingent, session-scoped, and authored. The vessel argument is what
lets you tell the two apart, and by it these twenty-two fields separate cleanly — six stay,
sixteen leave.

### V4 — HIGH — `Object` holds a `BodyPart*`

`Object.hpp:47` forward-declares `class BodyPart;`. `Object.hpp:151`:

```cpp
BodyPart* part = nullptr;
```

The base being of every constructed thing in the world carries a raw pointer to a
Person-domain noun. The dependency runs backwards through the entire ontology: `Object`
is what `Person`'s parts are made of, and now `Object` also knows what a body part is.
This is Kind Floor §5's "private telemetry struct" in a different costume — a second
truth about composition living beside `elementFormation()`, which is the real one.

**The §1 ruling raises this finding rather than settling it.** The ruling's whole force
comes from the Person substrate being a region *apart* — vessel rather than metal. A
carve-out only means something if it has an edge, and `BodyPart* part` on `Object` is that
edge dissolving from the inside: the exempt region reaching down into the substrate the
exemption was defined against. Under the ruling, `Object` should know *less* about
`BodyPart` than before, not more. Every constructed thing in the world currently carries
eight bytes asserting it might be part of a human.

### V5 — HIGH — the painting subsystem lives inside `Object`

`Object.hpp:249–305` declares roughly thirty methods on the core being:

```
initFaceTextures, fillFaceColor, paintFace, paintFaceAdvanced, paintStroke,
smudgeFace, cloneFace, airbrushFace, addTextureLayer, deleteTextureLayer,
setActiveLayer, setLayerOpacity, setBlendMode, saveStrokeState, undoStroke,
clearStrokeHistory, setFaceColor, …
```

plus `std::vector<FaceTexture> faceTextures` and a legacy `float faceColors[6][3]`.

Layers, blend modes, and a per-face undo stack are a raster-graphics application. They
are here answering the question *what is an Object* with *a thing that can be
airbrushed*. `Material` (`ConstructedBeing/Material/Material.hpp:24`) is the correct
shape and already exists — `class Material : public Singular` with
`getIdentifier() → "material." + _name`, holding no GL state, resolved by name at draw
time. `AUTHORED_CATEGORIES.md` §9a calls the Material wiring **done**. The paint stack
never followed it.

### V6 — MEDIUM — `Body` customization as fields

`Body.hpp:16–27`: `std::string shape; std::string artStyle;
std::vector<std::string> adornments; float height; float hitboxHeight;`

`artStyle` and `adornments` are authored content in the type system. `getIdentifier()
const override { return shape + "_body"; }` makes a *stable law-addressable identifier*
out of a customization string — so renaming an art style renames the being, and the
non-negotiable on stable identifiers is broken by an aesthetic choice.

### Held

`CategoryManager` (`ConstructedBeing/CategoryManager.hpp:15`) **is not a violation**, and
deserves credit. It is a manager for authored beings (`std::shared_ptr<Object>`), not a
`Category` class, and `CategoryManager.cpp:75–81` carries an explicit, accurate
`// REMAINING DEBT` comment naming what is still wrong (`category.tool.brush` authored in
C++) and why it has not been fixed (nothing reads it; a seed world is needed first). That
comment is the standard the rest of the repository should be held to.

---

## §2 — Refusal 2: no new top-level directory for a subsystem

> *The top level is the ontology.* — `CLAUDE.md`; Kind Floor §6; `DIRECTORY_ORDERING.md` §2

Current `src/` top level:

```
ConstructedBeing  Person  Relation  Singularity  ZonesOfEarth  OurVerse     ← ontology
Identity  Integration  Rendering  Perspective  Util  Legacy                 ← the rest
```

### Not violations — sanctioned debt

`Integration/`, `Rendering/`, `Perspective/`, `Util/` are marked `⟂` in
`DIRECTORY_ORDERING.md` §2 and have **decided destinations** in §5
(`Singularity/Foreign/`, `Singularity/Screen/`, split, dissolve). `Legacy/` is listed in
§6 as "temporary by intent. Named honestly, which is why it is tolerable." These are
deferred decisions that were written down, which §2 explicitly says is the difference
between a plan and "a lie the tree tells every new reader."

They are overdue, not wrong. §5's exit test — *"nothing at `src/` top level is named
after a technology"* — currently fails on four counts.

### V7 — HIGH — `src/Identity/` is undeclared

`src/Identity/` holds 14 files (`FirstMoverRegister`, `IdentityLedger`, `KeyStore`,
`KeyPair`, `Claim`, `SingularId`, `PersonMigration`, `MigrateIdentitiesTool`). It appears
in the stage-1 top-level table of `DIRECTORY_ORDERING.md` §2 **not at all**, and in the
§5 `⟂` deferral table **not at all**. The only mention anywhere in `docs/architecture/` is
an incidental path reference in `FIRST_MOVER_AUTHORING.md:9`.

`CLAUDE.md`'s tree lists it. `DIRECTORY_ORDERING.md`, which `CLAUDE.md` names as the
governing document for directories, does not. The two disagree, and §7's checklist for
adding a directory was not run.

The substantive question is genuine and worth answering rather than assuming: identity,
authorship, and the ledger may well *be* ontological — "nothing enters the world without
an author" is a non-negotiable, which argues that the register of authors is a peer of
`Person` and not a subsystem. But that is an ontology event, and Kind Floor §6 says a
top-level directory "is a claim to peer status with the ontology itself." The claim was
made without being stated. Either it is ratified in `DIRECTORY_ORDERING.md` §2 or it
moves; what it cannot do is stay undeclared.

### V8 — HIGH — the Physical channel is split across the repository

The two halves of one channel:

```
src/Singularity/Physical/           src/Integration/py/robotics/
  PhysicalChannel.{hpp,cpp}           base_driver.py
  Adapters/SerialAdapter.{hpp,cpp}    jaka_driver.py
                                      connection_registry.py
                                      engine_sync.py
```

`DIRECTORY_ORDERING.md` §3 states the rule and calls the Network channel its flagship
case, in language that applies here verbatim: *"those files are now neighbours because
they are the same thing. They were previously separated by the width of the whole
repository, and nothing about the system justified it except that one of them had to be
interpreted."*

**The contents are compliant, and impressively so.** `engine_sync.py` speaks only
`PropertyWrite` / `RelationAssertion` / `Event` and says so in its docstring — it passes
`NEW_KIND_FRAMEWORK.md` §7c's wire test cleanly. `connection_registry.py` carries the
rename §7c demanded of `RobotManager`. `base_driver.py`'s docstring reads "Decides
nothing." Someone read §7 and implemented it correctly.

**Only the location is wrong**, and it is wrong in the exact way §3 was written to
prevent. It belongs at `src/Singularity/Physical/py/`.

One content note: `jaka_driver.py:23` hardcodes
`self.link_slugs = [f"robot-link{i+1}" for i in range(6)]` with
`base_slug="robot-base"`. That is the driver holding an assumption about world structure
— a small amount of *Decide* in a first mover, which §7b warns about explicitly ("If you
find yourself writing an `if` in the channel that consults world state…"). Six link slugs
is not an `if`, but it is the same seam.

### V9 — MEDIUM — `src/Integration/web_ui/` with committed `node_modules/`

`web_ui/` exists twice: at the repository root (sanctioned workshop, §6) and at
`src/Integration/web_ui/`, the latter carrying a full `node_modules/` tree (React, Vite,
rolldown, lightningcss with platform binaries). Two homes for one surface, and foreign
code inside the ontology tree rather than beside `third_party/`.

### V10 — MEDIUM — `src/Integration/py/venv/`

A committed Python virtualenv — Flask, Playwright with its bundled Chromium driver,
PyObjC, pillow with dylibs — several thousand files, inside `src/`. §6 says foreign code
is foreign and belongs named as such. This one is not even named.

---

## §3 — Refusal 3: no new enum value for a kind of thing

> *`BeingKind`, `ShapeKind`, `ConditionNode::Kind`, `ActionNode::Kind` are append-only
> and serialized as integers. Categories are authored beings, not enum members.*

### V11 — CRITICAL — `ShapeKind` has admitted domain nouns

`ObjectTypes.hpp:39`, 14 values, 74 use sites across 10 files:

```cpp
enum class ShapeKind {
    Cube=0, Polyhedron=1, Sphere=2, Cylinder=3, Cone=4,
    Ellipsoid=5, Ovoid=6, Paraboloid=7, Torus=8, RoundedBox=9,
    Field=10, Patch=11,
    Shape2D=12, Text2D=13
};
```

`NEW_KIND_FRAMEWORK.md` §5b admits appends to `ShapeKind` — but only for **"a shape
parameterization."** Values 0–11 are exactly that: the header's own comment says "The
identity is the `SpatialKind` category; `ShapeKind` is just which parameterization."

`Text2D` is not a parameterization of a surface. Text is a domain noun, and it was
admitted through the one door the framework left open for geometry. `Shape2D` is a
*dimensionality*, which is a different axis again.

Because the enum is serialized as an int, **12 and 13 are now permanent**. They cannot be
renumbered, reused, or withdrawn. The cost of this finding is not that it should be
reverted — it cannot be — but that the door §5b opened has been demonstrated to admit
things it was not meant to, and needs a narrower test before the next append.

### V12 — HIGH — two competing kind axes for the same question

`GeometryType { Cube, Sphere, Cylinder, Cone, Polyhedron }` (`ObjectTypes.hpp:34`,
77 use sites) is marked "legacy axis; retained for save migration and the polyhedron
path" — and is simultaneously live in `Physics::LawTarget`:

```cpp
bool limitByGeometry = false;
std::vector<Object::GeometryType> geometryTypes;   // Physics.hpp
```

So law targeting still filters on the legacy kind enum, beside a `limitBySpatialKind`
that filters on the newer one, beside a `limitByObjectType` that filters on a *string*.
Three answers to "what kind of thing is this" reachable from one struct.

### V13 — MEDIUM — `Tool::Type` × `Tool::Category`

`OurVerse/Tool.hpp:33` and `:142` declare a two-level taxonomy — ~25 tool types
(`Brush`, `Pencil`, `Pen`, `Marker`, `Airbrush`, `Chalk`, `Spray`, `Smudge`, `Clone`,
`Eraser`, `MagicWand`, `Marquee`, …) grouped under 10 categories (`Drawing`, `Erasing`,
`Selection`, `Shape`, `Text`, `Transform`, `Effects`, `Utility`, `Layer`, `Special`),
with `Category getCategory() const;` mapping between them.

This is a rooted acyclic category graph, written as a pair of enums and a switch. It is
the textbook case `AUTHORED_CATEGORIES.md` §10 gives a procedure for, and
`CategoryManager` has already tried to author its first node (`category.tool.brush`) and
found nothing reading it.

### V14 — MEDIUM — `Physics::LawType`

`Physics.hpp:151`: `enum class LawType { Gravity, AirResistance, Collision,
CustomForce, GravityField, CenterGravity };`

Six kinds of law, enumerated in C++, sitting beside a full ECA law system whose entire
purpose is that laws are authored. `LAW_MIGRATION_FRAMEWORK.md` §9 works gravity as its
first example migration and lands it at R4 seed laws. Every value here is a seed law that
has not been written.

### V15 — MEDIUM — `Legacy/DesignSystem.hpp`

Four more kind enums — `ShapeType` (:89), `EffectType` (:189), `SelectionType` (:255),
`TransformType` (:304) — in a folder that is honestly named and still compiled into the
binary. Honest naming buys tolerance, not immunity; these are reachable types.

### Exemplary — cite this, do not change it

`ConditionNode::Kind` (`ConditionModel.hpp:29`) is the model the rest should follow:

- values 12 and 13 **burned**, with the reason written inline and the consequence spelled
  out ("a saved world would load as something else entirely");
- `Unsupported = 255` as a landing place for kinds this build does not know, which
  "never holds (`compile()` answers false and says why in the audit log)";
- unknown JSON preserved in an `unsupported` field "so a load/save round trip does not
  destroy law text we merely cannot evaluate."

`BeingKind` (`:57`) likewise holds at eight structural categories with none added for a
domain noun, satisfying Kind Floor §3.

---

## §4 — Refusals 4 and 5

### Refusal 4 — `Body` is reserved for Persons — **HELD**

`Body : public Object` is constructed only by `Person`. No robot, device, or mechanism
has one. `PhysicalChannel` does not reference `Body`. `NEW_KIND_FRAMEWORK.md` Floor §2 is
satisfied.

The one leak runs the other way, and is already filed as **V4**: `Object` knowing about
`BodyPart` means the Person-domain reaches down into the base being even though the base
being never reaches up.

### Refusal 5 — `Person` means Human — **HELD, with a latent hazard**

Structurally sound, and deliberately so. `FirstMoverRegister.hpp:40`:

```cpp
enum class Kind { Person, Model };
```

A model is a First Mover, named as one, distinct from a Person at the register level.
Nothing in the tree models an AI as a `Person`.

**The hazard — V16, MEDIUM.** `src/Integration/py/agent/earthcall_agent.py` drives
Earthcall from outside via `pyautogui` keystrokes, `pygetwindow` focus, and Playwright.
It types into the same input channel a human uses. It holds no `SingularId`, registers
with no `FirstMoverRegister`, signs nothing, and leaves no entry in the `IdentityLedger`.

Refusal 5 is not violated — the agent is not modelled as a `Person`. But the invariant
Refusal 5 protects is: **writes made by a model are, at the substrate, indistinguishable
from writes made by the human whose keyboard it is impersonating.** `CLAUDE.md`'s
non-negotiable — *nothing enters the world without an author* — is enforced structurally
for laws (`Law::applyTo` returns `Unauthored` and refuses to fire) and not at all for
input. This is a gap in enforcement, not a gap in doctrine, and §6 argues it is the most
valuable thing in this document.

---

## §5 — Why this matters

### Architecturally

The refusals are not style rules; they are the mechanism by which this system stays
**one world**.

A C++ class for a domain noun creates a region of reality that only the code that
declared it can see. `Arm` has fields no law can read, a lifetime the `World` does not
own, and a shape no save file can vary. The moment `Arm` exists, three things become
impossible that were possible the moment before: a law cannot ask about arms in general;
a Person cannot author a body with a different arm; and a save file from a build that
knew about arms cannot be read by one that does not. `NEW_KIND_FRAMEWORK.md` §4 makes the
quantitative version of this argument. The qualitative version is that **each such class
is a small permanent partition of the world.**

Kind enums do the same thing on a shorter time horizon and a longer one simultaneously.
Short: `switch (shapeKind)` at 74 sites means adding a kind is a 74-site edit that the
compiler will only partly catch. Long: because they serialize as integers, the enum is
the save format. `ShapeKind::Text2D` is not a decision that can be revisited in a later
version — it is a decision that every future version must honour. Enums are the one place
in this codebase where **a mistake made on a Tuesday is load-bearing forever.**

Directories are the cheapest of the three to fix and the most expensive to leave, because
the tree is what a new reader — human or model — reads first and trusts most. A top-level
`Identity/` that no architecture document mentions teaches every subsequent reader that
top-level directories are things you may simply add.

And §0's finding is the compound failure: state that should have been dissolved was
instead given the credentials of a being. The partitions above are visible — you can see
`class Arm` and know what it costs. A god object wearing `: public Singular` is
*invisible*, because it satisfies every structural check the system has.

### Teleologically

`core/EarthcallOurverse.md` and the manifesto make the claim that Earthcall is an
ontology, and the reason the ontology is load-bearing is a claim about **who gets to say
what things are**.

Every domain noun in C++ is a sentence that reads: *the engineer decided this, and the
Person may not.* Twenty-two tool fields on `Person` say the engineer decided that being a
human in this world includes having a `gridSnapSize`. Seven kind-enums say the engineer
decided what kinds of thing there are, and `Text2D` says one of those decisions is now
permanent in the save format.

None of these were bad engineering. Every one of them is what a competent developer would
write. That is precisely the warning `CLAUDE.md` opens with: *"You are almost certainly
about to do the standard thing. The standard thing is usually wrong in this
repository."*

**But the §1 ruling shows the sentence has a second reading, and this is the part the
refusals as written do not capture.** For the Person substrate, *the engineer decided this,
and the Person may not* is not a failure — it is the point. A vessel that varies is not a
vessel. If a Person could author away their own hands, the world would lose the thing that
makes "a Person is present here" mean anything stable at all. The invariant is a
**guarantee to the human**, not a constraint on them: whatever else changes, you have a
body with hands, and the world knows what a hand is.

So the teleological question is not "how much can the Person author" but **where the
boundary of authorship falls, and whether the system can state it.** Two failures are
possible and this audit found one of each:

- Authorship stopping short of where it should reach — V2: a Person cannot author an
  avatar that is a small round figure, though the ruling says they should be able to.
- Authorship reaching where it should not — nothing found. The vessel is intact.

And a third failure, worse than either, which is what V0 is: **the boundary existing but
not being written down.** An unstated boundary is indistinguishable from an accident, and
gets corrected as one — this audit's own first draft is the proof. A system whose deepest
commitment is that a Person authors their world must be able to say, in its doctrine and
ideally in its build, exactly which things are *not* authored and why. Otherwise the line
is not a principle; it is wherever the type system happened to stop, and it will move every
time someone competent refactors.

The `Person`-as-human refusal has a second edge that 2026 makes sharp. Earthcall already
distinguishes `Kind::Person` from `Kind::Model` in its register, which is more than most
systems do. But an unregistered agent typing into the window means that at the substrate,
the world cannot answer *who made this*. Refusing to model an AI as a Person is a
statement about dignity. It only means something if the world can actually tell the
difference, and right now, for input, it cannot.

### Long-term

Ranked by how expensive each becomes if left:

1. **Serialized enum values are permanent.** Every kind-enum append is a decision that
   cannot be revisited. `ShapeKind` has already admitted two values it should not have.
   Cost of acting now: a narrower admission test. Cost of acting in a year: whatever the
   enum has accumulated, forever.

2. **`Ourverse` as a `Singular` will accrete.** A being that laws can address, holding UI
   state, is an attractor. Every future creator-panel feature has an obvious home, and
   the home is inside the ontology. The `Game` elimination took months; a second one, run
   against a class that is *ontologically credentialed*, will be harder, because the
   argument for moving it out is no longer "it is not a being."

3. **The missing avatar path gets harder every month.** Non-humanoid bodies, authored
   prosthetics, bodies that change over a life — all of these need somewhere to live
   beside the invariant form, and `Person.hpp:230`'s single `Body body;` by value is not
   it. Every subsystem that comes to assume one humanoid body per Person (pose, render,
   collision, camera, save) is another site to change when the second kind of body
   arrives. The invariant is cheap to keep; the *assumption that it is the only case* is
   what compounds.

   The same clock runs on `BodyPart::Type` (V2a) from the other direction: it is free to
   correct today and append-only forever the moment a `Body` round-trips through a save.

4. **Undeclared directories teach.** `Identity/` is one. The next reader who wants a
   top-level directory will cite it, correctly, as precedent.

5. **The authorship gap widens with use.** Every agent-driven session writes unattributed
   changes into save files. That history cannot be reconstructed later; provenance not
   captured at write time is gone.

---

## §6 — Implementation plan

Ordered by dependency, not severity. Each phase names its exit test.

### P0 — Unbreak the tree, and decide what `Ourverse` is (blocking)

The build does not compile; nothing else can be verified until it does.

1. Resolve the in-flight `Game` deletion: 14 files reference `Core::Game`,
   `OurverseUI.cpp:3` includes a header that no longer exists. Either complete the
   removal or restore and stage it deliberately.
2. **Then make the §0 decision explicitly, in writing**, before any code moves:
   `Ourverse` inherits `Singular`. Is it a being?
   - If **yes** — it holds `zones`, `homes`, `relations`, `ownedObjects`, and answers to
     a stable identifier. Defensible. Then `Mode3D`, `CreatorSection`, `ToolTarget3D`,
     `showIntegrationUI` and the creator-panel state **do not belong on it** and move to a
     non-ontological UI surface under `OurVerse/`.
   - If **no** — it is `Game` renamed, and `GAME_ELIMINATION_PLAN.md` still applies to it
     under its new name.

   The four enums are the test: if they can be justified on the being, the being is
   `Game`.
3. Re-run `cmake` (sources are globbed — a deleted `.cpp` requires reconfigure) and
   `ctest`. Baseline is 36/36.

**Exit test:** `grep -rn "Mode3D\|CreatorSection\|showIntegrationUI" src/ZonesOfEarth/`
returns nothing.

### P1 — Make categories real (unblocks P2, P3, P5)

Everything downstream needs authored categories to exist as a working substrate.
`AUTHORED_CATEGORIES.md` §9a says the Material precedent is **done** and describes the
resolution rule; §9b says category beings still need a home.

1. Give category beings a home in the `World` (§9b) — save/load, identity, presence in
   the `Universe` so `ForAny`/`ForAll` can quantify over them.
2. Implement the closure/propagation law of §5a so category membership is materialized
   and law-visible.
3. Implement the acyclicity check of §7 with a test.
4. **Author the first real seed world** (`FIRST_MOVER_AUTHORING.md` §4) containing the
   category DAG. This is what `CategoryManager.cpp:75`'s `REMAINING DEBT` comment is
   waiting for; once it exists, delete `ensureDefaults()`.
5. Point `Object::objectType` (the string at `Object.hpp:145`) at category resolution
   rather than free text, and make `Physics::LawTarget::limitByObjectType` resolve
   through the DAG.

**Exit test:** a law that fires on "every being in category X" works for an X authored in
a save file, with no C++ change, and a test proves it.

### P2 — Ratify the human form, then build the avatar path beside it

Revised per the §1 ruling. The body-part classes stay. What changes is that the invariant
gets stated properly and stops being the *only* case.

**P2a — ratify (doctrine, no code).** Closes V0.

1. Amend `CLAUDE.md` Refusal 1 with the Person-substrate carve-out and its one-line
   reason: the vessel is not world-content.
2. Amend `NEW_KIND_FRAMEWORK.md` Kind Floor §1 to admit `BodyPart` and the constitutive
   members, extending Floor §2's existing reasoning about `Body` and personhood.
3. Bound the carve-out in the same edit — `Person/Body/` and the constitutive human form
   only — so it cannot be cited for `class Vehicle`.

**P2b — complete and correct the invariant.** Closes V1, V2a. Do this before P2c, because
the avatar path will encode whatever distinctions exist when it is written.

4. Split the two tiers explicitly: constitutive members (head, torso, arms, legs, hands,
   feet, fingers) versus articulation (`Neck`, `Shoulder`, `ForeArm`, `ForeLeg`). Whether
   that is two enums, a predicate on `BodyPart`, or a namespace split is an implementation
   choice; that it is *visible in the source* is not.
5. Add `Finger`. It is named in the ruling and absent from the tree.
6. Give `Shoulder`, `ForeArm`, `ForeLeg`, `Neck` honest `Type` values and delete the three
   `// categorize as arm for now` comments. Update the pose switch at
   `Person.cpp:430–489` accordingly.
7. Hoist `enum class Side { Left, Right }` onto `BodyPart`; delete the seven copies.
8. **Do this before any `Body` is serialized.** After that, `BodyPart::Type` is append-only
   forever under Refusal 3 and these corrections stop being free.

**P2c — the avatar path.** Closes V2 — the half of the original finding the ruling keeps.

9. Let a `Person` hold more than one `Body`: a set of authored avatars plus the
   constitutive one, with an active selection. `Person.hpp:230`'s `Body body;` by value is
   the blocker, and the constructor signature
   `Person(Soul, Body, const std::string&)` goes with it.
10. Add an authored construction path beside `createBasicAvatar`: `Body` from an
    `ObjectConcept` spawned into `Body::formation` (`Body.hpp:23`, already present and
    unused). `MemberTemplate` + `RelationTemplate` (`ObjectConcept.hpp:55–80`) is exactly
    the machinery. `createBasicAvatar` is untouched — it remains the invariant path.
11. Define the correspondence between an authored avatar and the constitutive form, so a
    Person driving a ball-shaped avatar still has a canonical hand for anything that means
    *hand* (grasp, point, hold). This is the design question the whole phase turns on; see
    L2.
12. Move `Object::part` (`Object.hpp:151`) onto `elementFormation()` and drop the forward
    declaration at `Object.hpp:47`. Closes V4, and gives the carve-out an edge.

**Exit test:** a save file authors a non-humanoid avatar — a sphere, or a four-legged
figure — a Person wears it, and `createBasicAvatar` still produces the invariant human form
unchanged. Both in one fixture, because the point is that both exist.

### P3 — Move tool state off `Person`

`LAW_MIGRATION_FRAMEWORK.md` R1→R3, run properly this time.

1. Mint a `CreationChannel` first-mover being with a stable slug — `creation-channel` —
   following `PhysicsLawBridge` / `PhysicalChannel` (`isFirstMover() → true`, excluded
   from save, resolves targets by name, refuses writes when the target is missing).
2. Move all 22 fields from `Person.hpp:52–90` onto it as bridged properties:
   `@creation-channel.placement-mode`, `@creation-channel.grid-snap-size`, and so on.
3. Move `computeSpawnPosition()` and `spawnSurfaceOffset()` off `Person` entirely — they
   are `Decide` and belong in seed laws (R4), not on a being.
4. Keep the genuinely personal on `Person`: `position`, `velocity`, `grounded`,
   `cameraPos`, `cameraForward`. A Person has a body and a viewpoint. A Person does not
   have a grid snap.
5. Run the R5.5 parity probe: creation tools behave identically before and after.

**Exit test:** `Person.hpp` declares no field naming a tool, a mode, a cursor, or a grid.

### P4 — Directory stage 2

`DIRECTORY_ORDERING.md` §5, in its stated order — the Physical channel first, because §5
says it should "prove the modality-folder pattern on a channel built *knowing* the rule."

1. **Physical channel unification.** `src/Integration/py/robotics/*` →
   `src/Singularity/Physical/py/`. Move the four files; update the Flask wiring in
   `src/Integration/py/app.py`. Nothing else imports them. This is the cheapest finding in
   the document and the one that proves the pattern.
2. **Ratify or relocate `Identity/`.** Answer the §7 checklist in writing. If it is
   ontological, add it to `DIRECTORY_ORDERING.md` §2's table with its justification. If
   not, place it — `Person/Identity/` and `Singularity/Core/` are the candidates.
   Reconcile `CLAUDE.md`'s tree with `DIRECTORY_ORDERING.md` either way.
3. **Evict the foreign.** `src/Integration/py/venv/` and
   `src/Integration/web_ui/node_modules/` out of `src/` and into `.gitignore`; consolidate
   the two `web_ui/` trees onto the root one.
4. `Rendering/` → `Singularity/Screen/`. `Integration/` → `Singularity/Foreign/`.
   `Perspective/` splits (`KeyboardHandler`, `MouseHandler` → `Singularity/Input/`, which
   already exists; `PersonPerspective`, `AvatarHandler` → `Person/Perspective/`).
   `Util/` dissolves.

**Exit test:** §5's own — nothing at `src/` top level is named after a technology.

### P5 — The `Object` diet

1. Move the paint stack (`Object.hpp:249–305`, `ObjectPaint.cpp`) behind `Material` and a
   paint tool surface, following the Material resolution pattern
   `AUTHORED_CATEGORIES.md` §9a calls done.
2. Collapse `GeometryType` into `SpatialKind` + `ShapeKind`; keep a migration path for
   old saves. 77 use sites, mechanical, and it removes one of the three competing answers
   in `Physics::LawTarget`.
3. Author `Tool::Type` / `Tool::Category` (V13) as a category DAG on the P1 substrate.
   Retire `Physics::LawType` (V14) into seed laws per `LAW_MIGRATION_FRAMEWORK.md` §9.
4. Decide `Legacy/DesignSystem` (V15): retire, or fold its live parts in.

**Exit test:** `Object.hpp` declares no method whose name is a drawing tool.

---

## §7 — Where the frontier leaps are

These are not cleanups. Each is something the refusals make *possible* that a
conventionally-architected engine cannot reach — and each is largely already half-built
here, which is why they are worth naming.

### L1 — Executable doctrine: turn the five refusals into a test

Highest leverage item in this document, and buildable in a day.

`tests/refusals_test.cpp` (or a `scripts/` lint in the build) that fails on:

- a new `: public Object` / `: public Singular` subclass outside an allow-list matching
  Kind Floor §1's closed list;
- a new directory at `src/` top level not present in a checked-in manifest;
- a change to any serialized kind-enum that is not a strict append, **and** an append
  lacking a burn-record entry;
- `Person.hpp` growing a field matching a tool/mode/cursor pattern.

Every finding in this audit is mechanically detectable. `CLAUDE.md` currently asks each
reader — human and model — to hold five refusals in their head against 303 files. The
prose is excellent and it is still the wrong substrate for an invariant. The repository
already believes this: `Law::applyTo` returns `Unauthored` structurally rather than by
convention, and `CLAUDE.md` calls that out as "structural, not conventional." The
refusals deserve the same treatment.

This also directly addresses the failure mode `CLAUDE.md` opens by naming — an agent
doing the standard thing. A test says no at the moment of the mistake, which no document
can.

**And it is where the §1 ruling should ultimately live.** A carve-out written only in prose
is a carve-out that survives exactly as long as everyone remembers it; this audit's first
draft is the evidence. As an allow-list entry — `Person/Body/**` admitted, with the reason
in a comment beside it — the exception is enforced and explained in the same place it is
checked, and the next agent reading `Arm.hpp` against Kind Floor §1 gets an answer from the
build instead of an argument from a document. Doctrine that can be run does not have to be
remembered.

### L2 — The vessel and the avatar: an invariant form with authored bodies over it

Falls out of P2c, and the §1 ruling makes this a **stronger** frontier claim than the
version this audit first proposed. Fully-authored anatomy is not novel — it is a rig
format, and every engine has one. The invariant *underneath* the authored layer is the
part nobody has.

The two-tier model the ruling implies:

- **The vessel** — head, torso, arms, legs, hands, feet, fingers — hardcoded, guaranteed,
  identical for every Person. It is what the world can always assume about a human being
  present in it, and it never varies because a guarantee that varies is not one.
- **The avatar** — authored, unconstrained: a small round figure, a Wii-style character, a
  nonhuman entity, a body that gains and loses parts over a life with the change recorded
  as a `Relation` with a timeline rather than a mutation.
- **The correspondence between them** — the actual invention, and P2c step 11's design
  question.

That third piece is what makes this worth building. Define a mapping from authored avatar
geometry back onto the constitutive members, and *semantics survive re-embodiment*. A
Person wearing a ball has no arm, but the vessel says they have two, so "reach", "point",
"hold", "offer" all still mean something — the world resolves them through the vessel and
renders them through the avatar. A law written about hands works for every Person in every
avatar, forever, because it was never written about geometry.

Compare what every other system does. In a game engine, a non-humanoid avatar breaks every
humanoid animation and interaction, and the fix is per-avatar retargeting authored by hand.
In VR platforms, the ball-shaped avatars simply *do not have* the interactions, which is
why social VR converged on floating hands: the hands are the only part the system dares
assume. Earthcall would be asserting the assumption openly, at the substrate, and getting
the interactions back for free — including for avatars nobody has authored yet.

And it is the strongest available answer to the question the refusals invite: *if
everything is authored, what holds still?* The answer is the human. Which is the manifesto's
answer to nearly everything else, so it is the right shape for this too.

### L3 — One type system: the category DAG absorbs every kind-enum

P1 makes categories work; the leap is to route **everything** through them.

`ShapeKind`, `GeometryType`, `BodyPart::Type`, `Tool::Type`, `Tool::Category`,
`Physics::LawType`, `DesignSystem::ShapeType` — seven parallel taxonomies, each with its
own switch statements, none able to see the others. Collapse them into one rooted acyclic
Formation graph on the Material resolution rule.

The payoff is not tidiness. It is that `ConditionNode::Kind::IsKind` becomes
**taxonomy-aware for free**: a law written about "tools" fires for a brush, because the
category graph says a brush is a tool, and the law's author never enumerated brushes.
`Physics::LawTarget` collapses from three filter axes to one. And `AUTHORED_CATEGORIES.md`
§6's "attaching behavior to a kind" becomes reachable — behavior inherited down an
authored graph, which is inheritance the Person controls rather than the compiler.

`LAW_AND_CREATION_SYSTEM.md` §7's claim that `ObjectConcept` is **the one set-to-set
machine** is currently a claim about a class. This makes it a claim about the whole
system.

### L4 — The burn ledger as a first-class artifact

`ConditionNode::Kind`'s burned 12/13 is exemplary practice living in a code comment.
Generalize it: a machine-readable `docs/ONTOLOGY_EVENTS.md` (or a checked-in JSON)
recording every kind-enum value ever assigned, its meaning, its status
(`live` / `burned`), the commit that introduced it, and — for burns — the reason.

Then L1 can check it, `Unsupported = 255` handling can be generated rather than
hand-written per enum, and save compatibility across versions becomes an assertion
instead of a memory. The framework documents call enum appends "an ontology event that
needs the world's author's assent." An event that needs assent should have a record.

### L5 — Authorship at the input boundary

The most valuable thing in this document, and the one with the shortest distance between
where the code is and where the frontier is.

The pieces already exist: `FirstMoverRegister` with `Kind { Person, Model }`,
`IdentityLedger`, `KeyStore`, `KeyPair`, `Claim`, `SingularId`. `Law::applyTo` already
refuses to fire `Unauthored`. What is missing is one connection: **every write into the
world carries a signed author, and input is a write.**

Concretely: `earthcall_agent.py` registers as `Kind::Model`, receives a `SingularId`, and
its writes are attributed to it. The same for any future model-driven session, including
the ones that produced parts of this codebase. Then the world can answer, for any being
in it: *who made this — a human, or a model, and which one?*

In 2026 that question is being answered everywhere else with watermarks and metadata
that survive nothing. Here it would be **structural at the substrate**, enforced the same
way `Unauthored` is enforced, and saved with the world. Earthcall's `Person`-means-Human
refusal is a statement about dignity; this is the machinery that makes the statement
checkable. It is a genuinely novel property for a creative system to have, and it is
perhaps two weeks of work on foundations that are already laid.

### L6 — Bodies and machines as the same authored structure

The convergence of L2 and the Physical channel, and the cleanest possible demonstration
that the ontology is not decoration.

After P2c and P4: a JAKA arm's six links and an **authored avatar's** arm are the same kind
of thing — a `Formation` of beings joined by `Relation`s, neither of them a class. The only
difference is which `Singularity` modality channel writes their poses: `PhysicalChannel`
for one, animation and law for the other. Swap the channel and a robot arm is puppeteered
by a Person's gesture, or an avatar is driven by telemetry, and *no new code is written for
either*, because there was never a `Robot` type to be incompatible with a `Formation`.

**The §1 ruling makes this sharper, not weaker.** The invariant `Arm` class is not the
obstacle it would first appear to be — it is the thing being *mapped through*. L2's
correspondence layer is what lets a Person's gesture drive a six-link industrial arm at all:
the vessel says "this is a reach with the right arm," the correspondence resolves it onto
whatever six links are actually there, and the robot moves. Without an invariant to gesture
*from*, there is nothing to retarget but geometry, which is the retargeting problem every
robotics stack already has and nobody has solved generally.

So the division of labour is: `Arm` hardcoded because a human always has one;
robot links authored because a machine's structure is contingent; the correspondence
between them written once and reused for every machine thereafter.
`NEW_KIND_FRAMEWORK.md` §7 works the robot as a hypothetical to argue the framework. This
would make it the running proof — and it is a better proof with the vessel in it.

### L7 — The tree as beings

`SUBSTRATE_ORDERING.md` contemplates a world that reads its own docs and tests as beings.
The tractable first step is the directory tree: emit it as beings, and let a **metalaw**
refuse a top-level addition — the ontology enforcing its own shape from inside, rather
than from a document that asks readers to be careful.

This is L1 grown up: doctrine as a test is doctrine the build enforces; doctrine as
metalaw is doctrine *the world* enforces. It is the furthest-out item here and the one
that most completely closes the loop the manifesto opens.

---

## §8 — Summary

| # | Refusal | Finding | Severity |
|---|---|---|---|
| §0 | general | `Ourverse : public Singular` inherited `Game`'s UI state; tree does not compile | **CRITICAL** |
| V11 | 3 | `ShapeKind` admitted `Shape2D`/`Text2D`; values permanently burned | **CRITICAL** |
| V0 | 1 | The §1 ruling contradicts `CLAUDE.md` + Kind Floor §1 and is written in neither | HIGH |
| V1 | 1 | Invariant human form incomplete (no `Finger`) and conflated with articulation | HIGH |
| V2 | 1 | No authored-avatar path: one `Body` by value, `createBasicAvatar` the only builder | HIGH |
| V3 | 1 | 22 tool fields + placement logic on `Person` (16 leave, 6 stay) | HIGH |
| V4 | 1 | `Object` holds `BodyPart* part` — the carve-out has no edge | HIGH |
| V5 | 1 | ~30 paint methods on `Object` | HIGH |
| V7 | 2 | `src/Identity/` undeclared in `DIRECTORY_ORDERING.md` | HIGH |
| V8 | 2 | Physical channel split across the repo (contents compliant) | HIGH |
| V12 | 3 | `GeometryType` vs `SpatialKind` vs type-string in `LawTarget` | HIGH |
| V2a | 1, 3 | `BodyPart::Type` misstates the invariant; free to fix only until a `Body` is saved | MEDIUM |
| V6 | 1 | `Body` customization as fields; identifier derived from `shape` | MEDIUM |
| V9 | 2 | `src/Integration/web_ui/` + committed `node_modules/` | MEDIUM |
| V10 | 2 | Committed `venv/` inside `src/` | MEDIUM |
| V13 | 3 | `Tool::Type` × `Tool::Category` taxonomy as enums | MEDIUM |
| V14 | 3 | `Physics::LawType` — six law kinds as an enum | MEDIUM |
| V15 | 3 | Four kind enums in `Legacy/DesignSystem` | MEDIUM |
| V16 | 5 | Unregistered agent writes are indistinguishable from a Person's | MEDIUM |

**Withdrawn:** the original V1 (eleven body-part domain-noun classes) and the original V2's
refusal of `BodyPart::Type`, per the world's author's ruling of 2026-08-11 recorded at §1.
The Person substrate is a vessel representing the human, not world-content, and the
constitutive human form is an admitted invariant. The outline of the withdrawn argument is
retained at §1 so it is not re-derived and re-filed.

**Held:** Refusal 4 (`Body` reserved for Persons) fully. Refusal 5 structurally
(`FirstMoverRegister::Kind { Person, Model }`), with V16 as the enforcement gap.

**Exemplary, and worth citing as the standard:** `ConditionNode::Kind`'s burn record and
`Unsupported = 255` landing place; `ConditionModel::BeingKind` holding at eight
structural categories; `Material`'s by-name resolution; `engine_sync.py` passing the wire
test; `CategoryManager.cpp:75`'s honest `REMAINING DEBT` comment.
